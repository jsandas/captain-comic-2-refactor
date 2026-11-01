#pragma once

#include <cstdint>

#include "comic2/types.hpp"

namespace comic2 {

// Ports of the entity movement helpers from comic2_standalone.asm. Slot field
// mapping (cs:[si+N]): +0 hitbox_w, +2 hitbox_h, +10h x, +12h y,
// +14h param_a (x velocity), +16h param_b (y velocity), +18h dir_toggle.
//
// All helpers return true when the original routine sets carry (collision /
// bounce) and false when it clears carry (move accepted).
struct EntityPhysicsContext {
  const RoomTileGrid *grid = nullptr;
  // word_25272 / word_25274 / word_25276: tile-id thresholds. A probe reports
  // a hit when tile_id <= threshold (signed). -1 means "never blocks".
  std::int16_t threshold_up = -1;    // word_25272
  std::int16_t threshold_down = -1;  // word_25274
  std::int16_t threshold_range = -1; // word_25276 (-1 => use threshold_up)
  std::int16_t camera_x = 0;         // word_256A2
  std::int16_t camera_y = 0;         // word_256A4
};

inline constexpr std::int16_t kEntityOutOfRangeTile = 0x100;
inline constexpr std::int16_t kEntityMaxFallSpeed = 0x0C;
inline constexpr std::int16_t kEntityViewportWidth = 0xD0;
inline constexpr std::int16_t kEntityViewportHeight = 0xA0;

// 1000:4bf7 - horizontal probe at pixel x; true when blocked.
bool ent_probe_x_collision_strict(const EntityPhysicsContext &ctx,
                                  const RuntimeEntitySlot32 &slot,
                                  std::int16_t pixel_x);

// 1000:4a4e - move by dx; flips dir_toggle and negates param_a on a hit.
bool ent_move_x_and_bounce(const EntityPhysicsContext &ctx,
                           RuntimeEntitySlot32 &slot, std::int16_t dx);

// 1000:4a99 - ++vy (clamped to 12), move y, snap to tile on collision.
bool ent_apply_gravity_and_move_y(const EntityPhysicsContext &ctx,
                                  RuntimeEntitySlot32 &slot);

// 1000:4b01 - move y by param_b, reflecting param_b on tile/viewport hits.
bool ent_move_y_and_bounce(const EntityPhysicsContext &ctx,
                           RuntimeEntitySlot32 &slot);

} // namespace comic2
