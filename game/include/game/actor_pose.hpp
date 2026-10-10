#pragma once
#include "game/recovered_resources.h"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace nes::game {
struct ActorPoseSnapshot {
    uint16_t pose_address{}, relative_x{}, relative_y{}, logical_camera{};
    uint8_t slot{}, type{}, oam_offset{};
};
struct ActorSprite {
    int32_t x{}, y{};
    uint8_t tile{}, attributes{}, oam_index{}, slot{}, type{};
    uint16_t pose_address{};
    // Actual eight-bit source composition, useful for validating the DMA latch.
    uint8_t native_y{}, native_x{};
    bool native_x_written{};
};
struct ActorPose {std::array<ActorSprite,64> sprites{};unsigned count{};};
inline int32_t actor_signed_position(uint16_t value) {
    return value<0x8000u?int32_t(value):int32_t(value)-65536;
}
inline uint8_t actor_pose_byte(uint16_t address) {
    if(address<0x8000u||address>=0xC000u)
        throw std::runtime_error("Entity pose source is outside compiled bank0");
    return game_prg_data[0][address-0x8000u];
}
inline ActorPose decode_actor_pose(const ActorPoseSnapshot& snapshot,int32_t original_left=85) {
    ActorPose result;
    // Actor coordinates are relative to the original OAM viewport. Background
    // scroll can lag logical RAM camera by one; that difference must not shift
    // sprites independently of the original image at the joining edge.
    const int32_t anchor_x=actor_signed_position(snapshot.relative_x)+original_left;
    const int32_t anchor_y=actor_signed_position(snapshot.relative_y);
    const uint8_t source_y=((snapshot.relative_x>>8)||(snapshot.relative_y>>8))?255:uint8_t(snapshot.relative_y);
    for(unsigned offset=0;offset<256;offset+=4) {
        const auto dy=actor_pose_byte(uint16_t(snapshot.pose_address+offset));
        if(!(dy&0x80u))return result;
        auto&s=result.sprites[result.count++];
        const auto dx=actor_pose_byte(uint16_t(snapshot.pose_address+offset+3));
        s.x=anchor_x+dx;s.y=anchor_y+int32_t(dy)-256+1;
        s.tile=actor_pose_byte(uint16_t(snapshot.pose_address+offset+1));
        s.attributes=actor_pose_byte(uint16_t(snapshot.pose_address+offset+2));
        s.oam_index=uint8_t(uint8_t(snapshot.oam_offset+offset)/4);
        s.slot=snapshot.slot;s.type=snapshot.type;s.pose_address=snapshot.pose_address;
        const unsigned sum=(snapshot.relative_x&255u)+dx;
        s.native_y=uint8_t(sum>255?248:uint8_t(source_y+dy));
        s.native_x=uint8_t(sum);s.native_x_written=sum<=255;
    }
    throw std::runtime_error("Entity pose has no bounded sentinel");
}
}
