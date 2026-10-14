#pragma once
#include "runtime/bus.hpp"
namespace nes::game {
// Capability adapter for the validated European profile, not a RAM API.
// Retains every original movement/collision instruction and every bus cycle.
class SuperJump {
public:
    RuntimeServices services(nes::runtime::Bus&, RuntimeServices, CPUState&);
    void request(unsigned steps) noexcept {requested_=steps<=12?steps:0;if(remaining_>requested_)remaining_=requested_;}
    unsigned extended_steps()const noexcept{return extended_steps_;}
private:
    nes::runtime::Bus* bus_{};
    CPUState* cpu_{};
    RuntimeServices original_{};
    unsigned requested_{};
    unsigned remaining_{},extended_steps_{};
    uint8_t room_{};
    bool eligible()const noexcept;
    void write(uint16_t,uint8_t,uint8_t);
};
}
