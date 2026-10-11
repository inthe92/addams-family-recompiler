#include "game/god_mode.hpp"

namespace nes::game {
RuntimeServices GodMode::services(nes::runtime::Bus& bus, RuntimeServices original) {
    bus_=&bus;original_=original;
    return {this,
        [](void*p,uint16_t a,uint8_t t)->uint8_t{auto&s=*static_cast<GodMode*>(p);return s.original_.read8(s.original_.context,a,t);},
        [](void*p,uint16_t a,uint8_t v,uint8_t t){static_cast<GodMode*>(p)->write(a,v,t);},
        [](void*p,uint16_t a)->uint8_t{auto&s=*static_cast<GodMode*>(p);return s.original_.bank_for_pc(s.original_.context,a);},
        [](void*p,uint8_t t){auto&s=*static_cast<GodMode*>(p);s.original_.advance(s.original_.context,t);},
        [](void*p,CPUState*c,uint8_t m)->uint8_t{auto&s=*static_cast<GodMode*>(p);return s.original_.boundary(s.original_.context,c,m);}};
}
void GodMode::write(uint16_t address,uint8_t value,uint8_t offset) {
    auto&r=bus_->ram();
    if(enabled_&&address<0x2000&&r.read(0x045B)==0) {
        const auto physical=static_cast<uint16_t>(address&0x07FFu);
        // B1:8496 queues damage; B7:C5E2/C983/C9DE set the death state.
        // Send the replacement through the same bus to preserve timed writes.
        if(physical==0x04EA&&value) {value=0;++blocked_damage_;}
        if(physical==0x0070&&value) {value=0;rescue_pending_=true;}
        if(physical==0x04ED&&value<r.read(0x04ED))value=r.read(0x04ED);
    }
    original_.write8(original_.context,address,value,offset);
}
void GodMode::begin_frame() {
    if(!bus_||bus_->ram().read(0x045B)!=0)return;
    auto&r=bus_->ram();
    if(all_items_) {
        // Explicit progression/item values recovered from A6E7..A7F8 and
        // E70B..E7AA. Sheet-music delivery is completed, not repeatedly queued.
        const struct {uint16_t address;uint8_t value;} items[]={
            {0x0630,1},{0x0631,1},{0x0632,2},{0x0633,1},{0x0634,1},{0x0635,2},
            {0x0636,0},{0x0637,1},{0x0638,1},{0x0639,0x0D},
            {0x063B,2},{0x063C,1},{0x063D,2},{0x063E,1},{0x063F,2},
            {0x0640,1},{0x0641,1},{0x0642,1},{0x0643,1},
            {0x0621,4},{0x0623,5},{0x0624,0},{0x0625,5},{0x0626,0x80},
            {0x0608,3}};
        for(const auto&item:items)r.write(item.address,item.value);
        // Keep the ending timer063A, dialogue selector064A, scene requests,
        // collection bitmaps, player motion and camera under original control.
    }
    if(!enabled_)return;
    if(r.read(0x70))rescue_pending_=true;
    r.write(0x70,0);r.write(0x04EA,0);
    r.write(0x04E8,0x75);r.write(0x04E9,7);
    if(!r.read(0x04ED))r.write(0x04ED,3);
    if(rescue_pending_&&r.read(0x0471)==0xFF&&r.read(0xB1)==0) {
        // Reload through the game's own request/fade protocol (B7:F075),
        // rather than clamping Y and leaving Gomez trapped below the floor.
        r.write(0x0471,r.read(0x71));
        r.write(0xAF,0xF0);r.write(0xB0,1);r.write(0xB1,2);
        bus_->write8(0x4015,0);
        rescue_pending_=false;++rescues_;
    }
}
}
