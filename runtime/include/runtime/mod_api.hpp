#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace nes::mods {
// Mod API negotiation is independent of the generated module's ABI v3.
// Host-validated declarative values; no paths or process pointers.
struct Setting {std::string key,label,type,value;double minimum{},maximum{},step{1};std::vector<std::string> choices;std::string default_value;};
struct Profile {std::string name,summary;bool active{};};
struct Target {std::string engine_version,game_id,rom_sha256,resource_catalog;};
enum Permission : uint8_t { observe_player=1,overlay_text=2,memory_read=4,memory_observe=8,gameplay_jump=32,graphics_layers=64,ui_draw=128 };
// Session evidence remains assisted after withdrawing a gameplay request.
class GameplayAssistance {
public:
    bool observe(bool legacy,unsigned requested_jump)noexcept {used_=used_||legacy||requested_jump!=0;return used_;}
private:
    bool used_{};
};
struct PlayerSnapshot {int x{},y{};};
struct GameSnapshot {
    std::string game_id;
    std::string state="unavailable";
    std::optional<unsigned> room;
    std::optional<PlayerSnapshot> player;
};
}
