#include "game/actor_view.hpp"

namespace nes::game {
RuntimeServices ActorView::services(nes::runtime::Bus& bus) {
    if(bus_&&bus_!=&bus)throw std::runtime_error("ActorView cannot observe two buses");
    bus_=&bus;original_=bus.services();
    return {this,
        [](void*p,uint16_t a,uint8_t t)->uint8_t{auto&s=*static_cast<ActorView*>(p);return s.original_.read8(s.original_.context,a,t);},
        [](void*p,uint16_t a,uint8_t v,uint8_t t){auto&s=*static_cast<ActorView*>(p);s.original_.write8(s.original_.context,a,v,t);if(a==0x4014)s.observe_upload(v);},
        [](void*p,uint16_t a)->uint8_t{auto&s=*static_cast<ActorView*>(p);return s.original_.bank_for_pc(s.original_.context,a);},
        [](void*p,uint8_t t){auto&s=*static_cast<ActorView*>(p);s.original_.advance(s.original_.context,t);},
        [](void*p,CPUState*cpu,uint8_t mask)->uint8_t{auto&s=*static_cast<ActorView*>(p);s.observe_boundary(*cpu);return s.original_.boundary(s.original_.context,cpu,mask);},
        [](void*p,uint8_t total,uint8_t offsets){auto&s=*static_cast<ActorView*>(p);s.original_.poll_irq(s.original_.context,total,offsets);}};
}
void ActorView::observe_boundary(const CPUState& cpu) {
    const auto bank=bus_->cartridge().bank_for_pc(cpu.PC);
    if(bank==7&&(cpu.PC==0xC852||cpu.PC==0xC82F)) {
        pending_valid_.fill(false);streaming_pass_=false;
    }
    if(bank==1&&cpu.PC==0x8219) {pending_valid_.fill(false);streaming_pass_=true;}
    if(bank!=0||cpu.PC!=0x8FD2||!streaming_pass_)return;
    const auto&r=bus_->ram();const unsigned slot=r.read(0x98);
    if(slot>=12) {++rejected_poses_;return;}
    auto&p=pending_[slot];
    const auto read_word=[&r](uint16_t low,uint16_t high)->uint16_t {
        return static_cast<uint16_t>(uint16_t(r.read(low))|(uint16_t(r.read(high))<<8));
    };
    p.pose_address=read_word(0x0E,0x0F);
    p.relative_x=read_word(static_cast<uint16_t>(0x032Cu+slot),static_cast<uint16_t>(0x03D4u+slot));
    p.relative_y=read_word(static_cast<uint16_t>(0x0338u+slot),static_cast<uint16_t>(0x03ECu+slot));
    p.logical_camera=read_word(0x74,0x75);
    p.slot=uint8_t(slot);p.type=r.read(static_cast<uint16_t>(0x0320u+slot));p.oam_offset=r.read(0x76);
    pending_valid_[slot]=true;++captured_poses_;
}
void ActorView::observe_upload(uint8_t page) {
    ++uploads_;
    // At vblank N the just-finished frame is already numbered N. Both game
    // DMA uploads populate the next visible frame N+1; the latter upload wins.
    const uint64_t next=bus_->video().frames()+1;
    auto& frame=frames_[next%frames_.size()];
    frame.number=next;frame.poses=0;frame.sprites.clear();
    if(page!=2)return;
    const auto&r=bus_->ram();
    std::array<ActorPose,12> poses;
    std::array<bool,12> decoded{};
    std::array<int,64> owner;
    std::array<unsigned,64> record{};
    owner.fill(-1);
    for(unsigned slot=0;slot<12;++slot)if(pending_valid_[slot]) {
        try {poses[slot]=decode_actor_pose(pending_[slot]);decoded[slot]=true;}
        catch(const std::exception&) {++rejected_poses_;continue;}
        for(unsigned i=0;i<poses[slot].count;++i) {
            const auto index=poses[slot].sprites[i].oam_index;
            owner[index]=int(slot);record[index]=i;
        }
    }
    std::array<bool,12> accepted{};
    for(unsigned slot=0;slot<12;++slot)if(decoded[slot]) {
        const auto&pose=poses[slot];
        bool agrees=true;
        bool survives=false;
        for(unsigned i=0;i<pose.count;++i) {
            const auto&s=pose.sprites[i];const uint16_t a=uint16_t(0x0200+s.oam_index*4);
            if(owner[s.oam_index]!=int(slot)||record[s.oam_index]!=i)continue;
            survives=true;
            if(r.read(a)!=s.native_y||r.read(a+1)!=s.tile||r.read(a+2)!=s.attributes||
               (s.native_x_written&&r.read(a+3)!=s.native_x)) {agrees=false;break;}
        }
        if(!agrees) {++rejected_poses_;continue;}
        if(survives){accepted[slot]=true;++frame.poses;}
    }
    // Lower OAM indices have priority, including when the original byte cursor
    // wraps: the last actual writer owns that entry. Rendering policy decides
    // sprite/background priority per pixel, consuming each OAM entry once.
    for(unsigned index=0;index<64;++index)if(owner[index]>=0&&accepted[unsigned(owner[index])])
        frame.sprites.push_back(poses[unsigned(owner[index])].sprites[record[index]]);
}
const std::vector<ActorSprite>& ActorView::sprites_for_frame(uint64_t frame) const noexcept {
    const auto&slot=frames_[frame%frames_.size()];return slot.number==frame?slot.sprites:empty_;
}
unsigned ActorView::poses_for_frame(uint64_t frame) const noexcept {
    const auto&slot=frames_[frame%frames_.size()];return slot.number==frame?slot.poses:0;
}
}
