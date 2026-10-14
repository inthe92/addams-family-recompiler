#include "game/super_jump.hpp"
namespace nes::game {
bool SuperJump::eligible()const noexcept {
    const auto& r=bus_->ram();
    // Only ordinary player motion. Fail closed in menus, loading, death,
    // ladders, ropes, alternate motion, actor attachment and ceiling contact.
    return r.read(0x71)==0 && r.read(0x045B)==0 && r.read(0x0471)==0xFF && !r.read(0xB1) &&
        !r.read(0x80) && !r.read(0x70) && !r.read(0x0600) && !r.read(0x72) &&
        !r.read(0x7C) && !r.read(0x060A) && !r.read(0x060D) &&
        !r.read(0x061C) && !r.read(0x73) && !r.read(0x1C);
}
RuntimeServices SuperJump::services(nes::runtime::Bus& bus,RuntimeServices s,CPUState& cpu){
    bus_=&bus;cpu_=&cpu;original_=s;
    return {this,
        [](void*p,uint16_t a,uint8_t t){auto&q=*static_cast<SuperJump*>(p);return q.original_.read8(q.original_.context,a,t);},
        [](void*p,uint16_t a,uint8_t v,uint8_t t){static_cast<SuperJump*>(p)->write(a,v,t);},
        [](void*p,uint16_t a){auto&q=*static_cast<SuperJump*>(p);return q.original_.bank_for_pc(q.original_.context,a);},
        [](void*p,uint8_t t){auto&q=*static_cast<SuperJump*>(p);q.original_.advance(q.original_.context,t);},
        [](void*p,CPUState*c,uint8_t m){auto&q=*static_cast<SuperJump*>(p);return q.original_.boundary(q.original_.context,c,m);},
        [](void*p,uint8_t n,uint8_t o){auto&q=*static_cast<SuperJump*>(p);q.original_.poll_irq(q.original_.context,n,o);}};
}
void SuperJump::write(uint16_t a,uint8_t v,uint8_t t){
    if(requested_){
        const auto& r=bus_->ram();
        if(!eligible()||r.read(0x71)!=room_)remaining_=0;
        if(a==0x15 && original_.bank_for_pc(original_.context,cpu_->PC)==7){
            // Verified player jump-start store, followed by its phase INC.
            // Never extend an existing fall or activate a jump mid-flight.
            if(cpu_->PC==0xECFE && v==1 && r.read(0x15)==0 && (r.read(0x045D)&0x80) && eligible()){
                room_=r.read(0x71);remaining_=requested_;
            }
            // INC zp has old/new writes at offsets 3/4. Replace only the new
            // phase (6->7) with 6. Both have identical N/Z; no position writes.
            if(cpu_->PC==0xEE2A && t==4 && v==7 && r.read(0x15)==6 && remaining_ && eligible() && r.read(0x13)>40){
                v=6;--remaining_;++extended_steps_;
            }
            // A phase reset/release/ceiling correction must not carry a lease.
            else if(cpu_->PC!=0xEE2A && cpu_->PC!=0xECFE)remaining_=0;
        }
    }
    original_.write8(original_.context,a,v,t);
}
}
