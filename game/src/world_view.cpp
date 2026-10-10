#include "game/world_view.hpp"
#include "game/actor_view.hpp"
#include "game/world_maps.h"
#include "game/recovered_resources.h"
#include <algorithm>

namespace nes::game {
namespace {
int32_t raster_camera(uint16_t logical_camera,const runtime::VideoLineStyle& style) {
    // The CPU can already hold the next scroll while this completed frame
    // still uses the previous latch. Recover the nearest256pixel phase from
    // the actual coarse/fine scroll sampled at the start of this scanline.
    const int32_t phase=static_cast<int32_t>((style.scroll_address&31u)*8u+style.fine_x);
    int32_t delta=phase-static_cast<int32_t>(logical_camera&255u);
    if(delta>127)delta-=256;else if(delta<-128)delta+=256;
    return static_cast<int32_t>(logical_camera)+delta;
}
}
void WorldView::compose(const runtime::Bus& bus,const ActorView* actors) {
    const auto& ram=bus.ram(); const auto& video=bus.video();
    const uint8_t level=ram.read(0x71);
    extended_=ram.read(0x045B)==0 && ram.read(0x4D)==0 && level<37 && game_world_maps[level].columns;
    const uint16_t logical_camera=static_cast<uint16_t>(ram.read(0x74)|(static_cast<uint16_t>(ram.read(0x75))<<8));
    camera_x_=static_cast<uint16_t>(std::clamp(raster_camera(logical_camera,video.line_styles()[0]),0,65535));
    frame_.fill(0x0F);
    base_background_.fill(0x0F);
    background_opacity_.fill(0);
    lateral_sprite_pixels_=0;
    if(extended_) {
        const auto& map=game_world_maps[level];
        uint8_t edit_flags[10];
        for(unsigned i=0;i<10;++i)edit_flags[i]=ram.read(static_cast<uint16_t>(0x04B0+i));
        for(unsigned y=0;y<192;++y) {
            const auto& style=video.line_styles()[y];
            const int32_t line_camera=raster_camera(logical_camera,style);
            for(unsigned x=0;x<width;++x) {
                const int32_t world_x=line_camera+static_cast<int32_t>(x)-static_cast<int32_t>(original_left);
                uint8_t color=style.palette[0];
                if((style.mask&8u) && world_x>=0 && world_x<static_cast<int32_t>(map.column_count)*8) {
                    const auto& column=map.columns[static_cast<unsigned>(world_x)/8u];
                    const uint8_t tile=game_world_tile(level,static_cast<uint16_t>(world_x/8),static_cast<uint8_t>(y/8),edit_flags);
                    const uint32_t pattern=style.background_chr_offset+tile*16u+(y&7u);
                    if(pattern+8u<131072u) {
                        const unsigned bit=7u-(static_cast<unsigned>(world_x)&7u);
                        const unsigned pixel=((game_chr_data[pattern]>>bit)&1u)|(((game_chr_data[pattern+8u]>>bit)&1u)<<1);
                        if(pixel) {color=style.palette[column.palette_slots[y/8u]*4u+pixel];background_opacity_[y*width+x]=1;}
                    }
                }
                base_background_[y*width+x]=static_cast<uint8_t>(color&((style.mask&1u)?0x30u:0x3Fu));
            }
        }
    }
    std::copy(base_background_.begin(),base_background_.end(),frame_.begin());
    // Keep every original raster pixel, sprite and HUD change in the center.
    for(unsigned y=0;y<height;++y) {
        std::copy_n(video.frame().data()+y*runtime::VideoState::width,runtime::VideoState::width,
                    frame_.data()+y*width+original_left);
    }
    if(extended_ && actors) {
        // OAM order resolves sprite priority. A transparent sprite lets the
        // next record through; an opaque behind-background pixel still wins
        // over later sprites, even when it does not replace the background.
        std::array<uint8_t,width*192> covered{};
        const auto& sprites=actors->sprites_for_frame(video.frames());
        for(const auto& sprite:sprites) {
            for(int32_t y=std::max(sprite.y,0);y<std::min(sprite.y+16,192);++y) {
                const auto& style=video.line_styles()[static_cast<unsigned>(y)];
                if(!(style.mask&0x10u))continue;
                const unsigned sprite_height=(style.control&0x20u)?16u:8u;
                unsigned row=static_cast<unsigned>(y-sprite.y);if(row>=sprite_height)continue;
                if(sprite.attributes&0x80u)row=sprite_height-1u-row;
                const unsigned bank=sprite_height==16?(sprite.tile&1u):((style.control&8u)?1u:0u);
                const unsigned pattern=sprite_height==16?((sprite.tile&0xFEu)+row/8u)*16u+(row&7u):sprite.tile*16u+row;
                const uint32_t physical=style.chr_offsets[bank]+pattern;
                if(physical+8u>=131072u)continue;
                for(int32_t x=std::max(sprite.x,0);x<std::min(sprite.x+8,static_cast<int32_t>(width));++x) {
                    if(x>=static_cast<int32_t>(original_left)&&x<static_cast<int32_t>(original_left+256))continue;
                    const unsigned offset=static_cast<unsigned>(y)*width+static_cast<unsigned>(x);
                    if(covered[offset])continue;
                    const unsigned column=static_cast<unsigned>(x-sprite.x);
                    const unsigned bit=(sprite.attributes&0x40u)?column:7u-column;
                    const unsigned pixel=((game_chr_data[physical]>>bit)&1u)|(((game_chr_data[physical+8u]>>bit)&1u)<<1);
                    if(!pixel)continue;
                    covered[offset]=1;
                    if((sprite.attributes&0x20u)&&background_opacity_[offset])continue;
                    frame_[offset]=static_cast<uint8_t>(style.palette[0x10u+(sprite.attributes&3u)*4u+pixel]&((style.mask&1u)?0x30u:0x3Fu));
                    ++lateral_sprite_pixels_;
                }
            }
        }
    }
}
}
