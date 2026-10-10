#pragma once
#include "game/actor_pose.hpp"
#include "runtime/bus.hpp"
#include <array>
#include <vector>

namespace nes::game {
// Game-owned observational bridge. It delegates every original CPU/bus action
// unchanged, and preserves unclipped pose data paired with the actual OAM DMA.
class ActorView {
public:
    RuntimeServices services(nes::runtime::Bus& bus);
    const std::vector<ActorSprite>& sprites_for_frame(uint64_t frame) const noexcept;
    unsigned poses_for_frame(uint64_t frame) const noexcept;
    uint64_t captured_poses() const noexcept {return captured_poses_;}
    uint64_t uploads() const noexcept {return uploads_;}
    uint64_t rejected_poses() const noexcept {return rejected_poses_;}
private:
    struct Frame {uint64_t number=UINT64_MAX;unsigned poses{};std::vector<ActorSprite> sprites;};
    nes::runtime::Bus* bus_{};
    RuntimeServices original_{};
    std::array<ActorPoseSnapshot,12> pending_{};
    std::array<bool,12> pending_valid_{};
    std::array<Frame,4> frames_{};
    const std::vector<ActorSprite> empty_{};
    uint64_t captured_poses_{},uploads_{},rejected_poses_{};
    bool streaming_pass_{};
    void observe_boundary(const CPUState& cpu);
    void observe_upload(uint8_t page);
};
}
