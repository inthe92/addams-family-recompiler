#include "game/gameplay_diagnostic.hpp"
#include "runtime/app_version.hpp"
#include "recomp/module_abi.h"
#include <chrono>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace nes::game {
namespace {
// Address identifiers only. 063A is an ending counter, not an inventory flag.
constexpr std::array<uint16_t, 26> fields={
    0x0608,0x0621,0x0623,0x0624,0x0625,0x0626,
    0x0630,0x0631,0x0632,0x0633,0x0634,0x0635,0x0636,0x0637,0x0638,0x0639,
    0x063B,0x063C,0x063D,0x063E,0x063F,0x0640,0x0641,0x0642,0x0643,0x04ED};
std::string number(unsigned n) { return std::to_string(n); }
std::string quote(const char* value) {
    std::string result="\"";constexpr char hex[]="0123456789abcdef";
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p) {
        if(*p=='"'||*p=='\\'){result+='\\';result+=char(*p);}
        else if(*p<32){result+="\\u00";result+=hex[*p>>4];result+=hex[*p&15];}
        else result+=char(*p);
    }
    return result+'"';
}
bool watched(uint16_t a) {
    if(a==0x70||a==0x71||a==0x80||a==0xB1||a==0x4D||a==0x045B||a==0x0471||a==0x063A)return true;
    for(auto field:fields)if(a==field)return true;
    return false;
}
}
std::filesystem::path GameplayDiagnostic::local_log_path(const std::filesystem::path& directory) {
    namespace fs=std::filesystem;
    const auto root=fs::weakly_canonical(fs::absolute(directory));
    for(auto p=root;!p.empty();) {
        if(fs::exists(p/".git")||p.extension()==".app"||p.extension()==".AppDir"||
           fs::is_directory(p/"tools/recompiler"))
            throw std::invalid_argument("Diagnostic logs must be outside repositories and application packages");
        const auto parent=p.parent_path();if(parent==p)break;p=parent;
    }
    fs::create_directories(root);
    const auto stamp=std::chrono::system_clock::now().time_since_epoch().count();
    for(unsigned i=0;i<100;++i) {
        auto path=root/("session-"+std::to_string(stamp)+"-"+number(i)+".jsonl");
        if(!fs::exists(path))return path;
    }
    throw std::runtime_error("Cannot allocate a diagnostic session filename");
}
void GameplayDiagnostic::event(const char* kind, const std::string& data) noexcept {
    if(!output_||io_failed_||ended_)return;
    try {
        *output_<<"{\"schema\":1,\"seq\":"<<sequence_++<<",\"event\":\""<<kind
            <<"\",\"cycle\":"<<event_cycle_<<",\"frame\":"<<event_frame_
            <<",\"cpu_available\":"<<(bus_?"true":"false")<<",\"pc\":"<<cpu_.PC
            <<",\"bank\":"<<unsigned(event_bank_)
            <<data<<"}\n";
        if(!*output_)io_failed_=true;
        const std::string_view type(kind);
        if(type=="session_start"||type=="session_end"||type=="error"||type=="uncovered_native_address"||
           type=="no_cpu_progress"||type=="no_video_frame_progress")flush();
    } catch(...) { io_failed_=true; }
}
void GameplayDiagnostic::flush() noexcept {
    if(!output_||io_failed_)return;
    try{output_->flush();if(!*output_)io_failed_=true;}catch(...){io_failed_=true;}
}
void GameplayDiagnostic::begin(std::ostream& output,bool god,bool items,bool mods) {
    if(started_)throw std::logic_error("Diagnostic session already started");
    output_=&output;started_=true;
    event("session_start",std::string(",\"app_version\":\"")+nes::app::version+"\",\"abi\":"+number(NES_RECOMP_ABI_VERSION));
    assistance(god,items,mods);
}
void GameplayDiagnostic::assistance(bool god,bool items,bool mods) {
    const bool changed=god!=god_||items!=items_||mods!=mods_;
    god_=god;items_=items;mods_=mods;assisted_=assisted_||god||items||mods;
    if(changed||sequence_==1)event("assistance_state",",\"god\":"+number(god)+
        ",\"all_items\":"+number(items)+",\"mods\":"+number(mods)+",\"assisted\":"+number(assisted_));
}
RuntimeServices GameplayDiagnostic::services(runtime::Bus& bus,RuntimeServices original,const CPUState& cpu) {
    if(!started_||io_failed_)return original;
    if(bus_)throw std::logic_error("Diagnostic services already attached");
    bus_=&bus;original_=original;cpu_=cpu;event_cycle_=bus.cycles();
    event_frame_=bus.video().frames();event_bank_=bus.cartridge().bank_for_pc(cpu.PC);
    const auto& r=bus.ram();screen_=r.read(0x045B);death_=r.read(0x70);
    for(unsigned i=0;i<fields.size();++i) {
        progress_[i]=r.read(fields[i]);
        if(progress_[i])event("progress_initial",",\"field\":"+number(fields[i])+",\"value\":"+number(progress_[i]));
    }
    low_bank_=bus.cartridge().bank_for_pc(0x8000);high_bank_=bus.cartridge().bank_for_pc(0xC000);
    last_frame_=bus.video().frames();frame_cycle_=slice_cycle_=bus.cycles();
    event("runtime_attached",",\"screen_state\":"+number(screen_)+",\"room_id\":"+number(r.read(0x71))+
        ",\"low_bank\":"+number(low_bank_)+",\"high_bank\":"+number(high_bank_));
    return {this,
        [](void* p,uint16_t a,uint8_t t)->uint8_t{auto& s=*static_cast<GameplayDiagnostic*>(p);return s.original_.read8(s.original_.context,a,t);},
        [](void* p,uint16_t a,uint8_t v,uint8_t t){static_cast<GameplayDiagnostic*>(p)->write(a,v,t);},
        [](void* p,uint16_t a)->uint8_t{auto& s=*static_cast<GameplayDiagnostic*>(p);return s.original_.bank_for_pc(s.original_.context,a);},
        [](void* p,uint8_t t){auto& s=*static_cast<GameplayDiagnostic*>(p);s.original_.advance(s.original_.context,t);},
        [](void* p,CPUState* c,uint8_t mask)->uint8_t{auto& s=*static_cast<GameplayDiagnostic*>(p);
            const auto result=s.original_.boundary?s.original_.boundary(s.original_.context,c,mask):0;
            s.cpu_=*c;s.event_cycle_=s.bus_->cycles();s.event_frame_=s.bus_->video().frames();
            s.event_bank_=s.bus_->cartridge().bank_for_pc(c->PC);
            if(s.dirty_||s.mapper_dirty_)s.observe();return result;},
        [](void* p,uint8_t total,uint8_t offsets){auto& s=*static_cast<GameplayDiagnostic*>(p);
            if(s.original_.poll_irq)s.original_.poll_irq(s.original_.context,total,offsets);}};
}
bool GameplayDiagnostic::stable_room() const {
    const auto& r=bus_->ram();
    return r.read(0x045B)==0&&r.read(0x0471)==0xFF&&r.read(0xB1)==0&&r.read(0x80)==0&&r.read(0x70)==0;
}
void GameplayDiagnostic::write(uint16_t a,uint8_t value,uint8_t offset) {
    const auto physical=uint16_t(a&0x07FFu);
    const bool ram_watch=a<0x2000&&watched(physical)&&!io_failed_;
    const auto before=ram_watch?bus_->ram().read(physical):0;
    original_.write8(original_.context,a,value,offset);
    if(io_failed_)return;
    event_cycle_=bus_->cycles()+offset;
    event_frame_=bus_->video().frames();event_bank_=bus_->cartridge().bank_for_pc(cpu_.PC);
    if(a>=0x8000)mapper_dirty_=true;
    if(!ram_watch)return;
    dirty_=true;
    const auto actual=bus_->ram().read(physical);
    if(actual==before)return; // RMW dummy writes and filtered GOD writes are not pickups.
    if(physical==0x0471)event("transition_request",",\"from\":"+number(before)+",\"to\":"+number(actual));
    // Only this naturally observed grant has a confirmed contact/site contract.
    if(physical==0x0633&&before==0&&actual==1&&bus_->ram().read(0x045B)==0&&
       bus_->ram().read(0x71)==2&&cpu_.PC==0xA80A&&
       bus_->cartridge().bank_for_pc(cpu_.PC)==1&&cpu_.X<12&&bus_->ram().read(uint16_t(0x0320+cpu_.X))==9)
        event("item_pickup",",\"item_flag\":1587,\"entity_type\":9,\"source\":\"native_write\"");
    if(physical==0x063A&&before==0&&actual!=0)event("progress_activation",",\"field\":1594,\"value\":"+number(actual));
    if(physical==0x045B&&actual==4&&cpu_.PC==0xC405&&bus_->cartridge().bank_for_pc(cpu_.PC)==7) {
        ending_seen_=true;event("ending_screen_observed");
    }
}
void GameplayDiagnostic::observe() {
    if(io_failed_)return;
    dirty_=mapper_dirty_=false;
    const auto& r=bus_->ram();const auto screen=r.read(0x045B), death=r.read(0x70);
    if(screen!=screen_)event("screen_state_changed",",\"from\":"+number(screen_)+",\"to\":"+number(screen));
    if(death_==0&&death!=0&&screen==0) {death_pending_=true;event("death",",\"state_field\":112,\"room_id\":"+number(r.read(0x71)));}
    if(death_!=0&&death==0)event("death_state_cleared");
    screen_=screen;death_=death;
    if(room_>=0&&(!stable_room()||r.read(0x71)!=room_)) {
        event("room_exit",",\"room_id\":"+number(unsigned(room_))+",\"reason\":\"state_change\"");room_=-1;candidate_=-1;candidate_frames_=0;
    }
    if(!stable_room()){candidate_=-1;candidate_frames_=0;}
    for(unsigned i=0;i<fields.size();++i) {
        const auto value=r.read(fields[i]);if(value==progress_[i])continue;
        event("progress_changed",",\"field\":"+number(fields[i])+",\"from\":"+number(progress_[i])+",\"to\":"+number(value));progress_[i]=value;
    }
    const auto low=bus_->cartridge().bank_for_pc(0x8000), high=bus_->cartridge().bank_for_pc(0xC000);
    if(low!=low_bank_||high!=high_bank_) {
        event("bank_mapping_changed",",\"low_from\":"+number(low_bank_)+",\"low_to\":"+number(low)+
            ",\"high_from\":"+number(high_bank_)+",\"high_to\":"+number(high));low_bank_=low;high_bank_=high;
    }
}
void GameplayDiagnostic::frame(const CPUState& cpu,bool god,bool items,bool mods) {
    if(!bus_||io_failed_||ended_)return;
    cpu_=cpu;event_cycle_=bus_->cycles();
    event_frame_=bus_->video().frames();event_bank_=bus_->cartridge().bank_for_pc(cpu.PC);
    assistance(god,items,mods);observe();
    const auto frame=bus_->video().frames();if(frame==last_frame_)return;
    last_frame_=frame;frame_cycle_=bus_->cycles();frame_warning_=false;
    if(!stable_room()) {candidate_=-1;candidate_frames_=0;flush();return;}
    const int id=bus_->ram().read(0x71);
    if(candidate_!=id){candidate_=id;candidate_frames_=0;}
    if(candidate_frames_<2)++candidate_frames_;
    if(candidate_frames_==2&&room_<0) {
        room_=id;event("room_enter",",\"room_id\":"+number(unsigned(id))+",\"layout_id\":"+number(bus_->ram().read(0x4D))+
            ",\"source\":\"two_stable_frame_samples\"");
        if(death_pending_){event("respawn_observed",",\"room_id\":"+number(unsigned(id)));death_pending_=false;}
    }
    flush(); // Buffered events reach disk once per completed frame.
}
void GameplayDiagnostic::slice(const CPUState& cpu) {
    if(!bus_||io_failed_||ended_)return;
    cpu_=cpu;event_cycle_=bus_->cycles();
    event_frame_=bus_->video().frames();event_bank_=bus_->cartridge().bank_for_pc(cpu.PC);
    if(event_cycle_==slice_cycle_) {if(idle_slices_<16)++idle_slices_;}
    else {idle_slices_=0;idle_warning_=false;}slice_cycle_=event_cycle_;
    if(idle_slices_==16&&!idle_warning_){event("no_cpu_progress");idle_warning_=true;}
    if(bus_->video().frames()==last_frame_&&event_cycle_>=frame_cycle_&&event_cycle_-frame_cycle_>=1662607&&!frame_warning_) {
        event("no_video_frame_progress");frame_warning_=true;
    }
}
void GameplayDiagnostic::missing_address(const CPUState& cpu) {
    cpu_=cpu;if(bus_){event_cycle_=bus_->cycles();event_frame_=bus_->video().frames();event_bank_=bus_->cartridge().bank_for_pc(cpu.PC);}event("uncovered_native_address");
}
void GameplayDiagnostic::error(const char* code) {event("error",",\"code\":"+quote(code));}
void GameplayDiagnostic::end(const char* reason) {
    if(!started_||ended_)return;
    if(room_>=0)event("room_exit",",\"room_id\":"+number(unsigned(room_))+",\"reason\":\"session_end\"");
    event("session_end",",\"reason\":"+quote(reason)+",\"assisted\":"+number(assisted_)+
        ",\"ending_screen_observed\":"+number(ending_seen_));ended_=true;
}
}
