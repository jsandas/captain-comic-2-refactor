#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "comic2/audio.hpp"
#include "comic2/entity_runtime.hpp"
#include "comic2/projectiles.hpp"
#include "comic2/resource_formats.hpp"
#include "comic2/types.hpp"

namespace comic2 {

enum class MenuState {
  None = 0,
  Pause = 1,
  Options = 2,
  Help = 3,
  GameSelect = 4,
};

struct InputState {
  bool jump_pressed = false;
  bool left_pressed = false;
  bool right_pressed = false;
  bool down_pressed = false;
  bool mode_activate_pressed = false;
  bool pause_pressed = false;

  bool operator==(const InputState &) const = default;
};

enum class PlayerAnimationState : std::uint8_t {
  Idle = 0,
  WalkCycle = 1,
  JumpRise = 2,
  JumpFall = 3,
  Attack = 4,
  Hurt = 5,
  Death = 6,
};

struct PlayerState {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t x_vel = 0;
  std::int16_t y_vel = 0;

  bool is_airborne = false;
  bool is_physics_active = false;
  bool is_animation_active = false;
  bool is_attack_active = false;
  bool facing_right = true;

  std::uint8_t animation_state =
      static_cast<std::uint8_t>(PlayerAnimationState::Idle);
  std::uint8_t animation_frame = 0;
  std::uint8_t animation_ticks = 0;
  std::uint8_t attack_overlay_ticks = 0;
  std::uint8_t death_timer_ticks = 0;
  bool overlay_active = false;
  std::uint8_t overlay_ticks = 0;
  std::uint8_t overlay_sprite_frame = 0;
  std::uint8_t active_mode_effect = 0;
  std::uint8_t mode_effect_ticks = 0;

  std::uint8_t jump_counter = 0;
  std::uint8_t jump_counter_max = 5; // [DS:0x0899] base max jump capacity (5, increased to 6 with boots)
  std::uint8_t hp = 12;
  std::uint8_t shield_power = 5;     // [DS:0x089a] current shield energy (default 5, max 8)
  std::uint8_t firepower = 1;        // [DS:0x08ac] blast power level (1-5)
  std::uint8_t gems = 0;
  std::uint8_t lives = 3;
  std::uint16_t score = 0;
  std::uint16_t score_high = 0;      // [DS:0x0928] high word of 32-bit score counter
  std::uint8_t invuln_ticks = 0;     // [DS:0x0892] damage invulnerability frame countdown
  std::uint8_t damage_recoil_ticks = 0;
  std::uint8_t weapon_cooldown = 0;  // [DS:0x08a8] firing rate limiter cooldown timer

  // Confirmed inventory and equipment fields from comic2_data.asm (DS:0x08ae - 0x08b6):
  bool boots_equipped = false;       // [DS:0x08ae] High-jump / gravity boots flag
  bool lantern_equipped = false;     // [DS:0x08b0] Cavern lantern illumination flag
  bool corkscrew_equipped = false;   // [DS:0x08b2] Grate corkscrew unlock flag
  bool wand_equipped = false;        // [DS:0x08b4] Teleportation wand flag
  std::uint8_t key_count = 0;        // [DS:0x08b6] Door keys collected count

  bool operator==(const PlayerState &) const = default;
};

struct DispatcherFlags {
  bool level_transition_pending = false;
  bool special_logic1_active = false;
  bool special_logic2_active = false;
  bool timed_overlay_pending = false;
  bool distance_interaction_active = false;
  bool player_special_state_active = false;
  bool tile_hazard_triggered = false;
  bool room_event_triggered = false;

  bool operator==(const DispatcherFlags &) const = default;
};

struct PendingRoomTransition {
  std::uint16_t target_room = 0;
  std::int16_t target_player_x = 0;

  bool operator==(const PendingRoomTransition &) const = default;
};

struct RoomTransitionState {
  bool active = false;
  bool player_frozen = false;
  bool completed = false;
  std::uint16_t effect_type = 0;
  std::uint16_t frame_index = 0;
  std::uint16_t tick_count = 0;
  std::uint16_t target_room = 0;
  std::int16_t target_player_x = 0;
  std::int16_t player_entry_offset = 0;
  std::int16_t player_exit_offset = 0;
  std::uint8_t palette_tint = 0;
  std::uint8_t palette_shift = 0;

  bool operator==(const RoomTransitionState &) const = default;
};

struct RoomEventAnchorState {
  bool active = false;
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t velocity_x = 0;
  std::int16_t velocity_y = 0;

  bool operator==(const RoomEventAnchorState &) const = default;
};

struct RoomEventScriptState {
  bool loaded = false;
  std::vector<std::uint8_t> raw_script_bytes;
  std::vector<std::uint8_t> raw_trigger_bytes;

  bool operator==(const RoomEventScriptState &) const = default;
};

struct EgaPaletteState {
  bool loaded = false;
  std::array<std::array<std::uint8_t, 4>, 16> entries{};

  bool operator==(const EgaPaletteState &) const = default;
};

struct ProgressionState {
  std::uint8_t flags = 0;
  bool gems_collected = false;
  bool firepower_unlocked = false;
  bool lives_available = false;
  std::uint8_t mode_inventory_mask = 0;

  // Inventory collection state confirmed from comic2_data.asm and savegame snapshot:
  bool boots_acquired = false;
  bool lantern_acquired = false;
  bool corkscrew_acquired = false;
  bool wand_acquired = false;
  std::uint8_t keys_collected = 0;

  bool operator==(const ProgressionState &) const = default;
};

struct UiState {
  MenuState menu_state = MenuState::None;
  std::size_t selected_option_index = 0;
  std::vector<std::string> option_labels;
  std::uint8_t active_mode_mask = 0;
  std::uint8_t inventory_mask = 0;
  std::uint8_t cinematic_frame = 0;
  std::uint8_t music_volume = 4;
  std::uint8_t sfx_volume = 4;
  std::uint8_t display_scale = 1;
  bool modal_active = false;
  std::uint8_t awaited_key = 0;
  std::string modal_prompt;
  std::string pending_event_message;
  bool room_event_consumed = false;
  bool modal_confirmed = false;
  bool modal_game_over = false;
  bool game_over = false;
  bool level_complete_modal = false;
};

struct RuntimeState {
  std::uint16_t current_level = 0;
  std::uint16_t current_room = 0;
  FrdataRoomEntry room_entry{};
  std::filesystem::path assets_root;
  std::optional<PendingRoomTransition> pending_room_transition;

  RoomTileGrid room_grid;
  std::array<std::uint16_t, 2> tile_hazard_bounds{0, 0};
  std::vector<Ega4PlaneImage> level_tileset;
  std::vector<MaskedSpriteRecord> level_sprites;
  std::vector<std::uint8_t> room_resource_bytes;
  std::vector<std::uint8_t> level_metadata_bytes;
  FrpakCatalog frpak_catalog;
  std::vector<std::uint8_t> sprite_resource_bytes;
  std::vector<FrpakDecodedRecordCacheEntry> frpak_decode_cache;
  std::optional<Ega4PlaneImage> bezel_frame;

  PlayerState player;
  InputState input;
  DispatcherFlags flags;
  ProgressionState progression;
  UiState ui;
  bool level_complete = false;
  std::uint8_t level_completion_gems_required = 0;
  std::int32_t camera_x = 0;
  std::int32_t camera_y = 0;
  EgaPaletteState palette;
  RoomTransitionState transition_state;
  RoomEventAnchorState room_event_anchor;
  RoomEventScriptState room_event_script;

  std::vector<MappedObject12> mapped_objects;
  std::vector<ActiveEntity8> active_entities;
  std::vector<RuntimeEntitySlot32> runtime_slots;
  EntityActivationState activation_state;
  std::uint16_t activation_toggle = 1;

  std::vector<ProjectileState> projectiles;
  AudioDispatchState audio;
};

RuntimeState make_default_runtime_state();

} // namespace comic2
