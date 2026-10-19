#pragma once

#include <cstdint>

#include "comic2/game_state.hpp"
#include "comic2/tile_collision.hpp"

namespace comic2 {

inline constexpr std::uint8_t kModeSpeedBoost = 0x01U;
inline constexpr std::uint8_t kModeInvulnerability = 0x02U;
inline constexpr std::uint8_t kModeJumpBoost = 0x04U;

// Authentic DOS 8086 constants verified from
// reference/comic2/comic2_standalone.asm:
// - player_update_physics (1000:2a10):
//     sub word [0x894], 0x7  -> jump impulse per tick: -7
//     add CX, 0x5            -> gravity acceleration per tick: +5
//     cmp CX, 0x17           -> terminal fall velocity: 23 (0x17)
//     cmp CX, -5 / 5         -> air drift limits: [-5, 5]
// - player_do_jump (1000:2d61):
//     add BX, 0xd            -> feet probe Y offset: 13
// - loc_243f / loc_2698:
//     [DS:0x0899]            -> default max jump capacity: 5 ticks (6 with
//     boots)
// - loc_2484:
//     mov word [0x894], 0x5  -> fall start velocity: 5
inline constexpr std::int16_t kOriginalDosWalkStep = 8;
inline constexpr std::int16_t kOriginalDosJumpImpulse = -7;
inline constexpr std::int16_t kOriginalDosAirDriftStep = 1;
inline constexpr std::int16_t kOriginalDosMaxAirDrift = 5;
inline constexpr std::int16_t kOriginalDosGravityPerTick = 5;
inline constexpr std::int16_t kOriginalDosMaxFallSpeed = 23;
inline constexpr std::int16_t kOriginalDosFallStartVelocity = 5;
inline constexpr std::int16_t kOriginalDosFeetProbeOffsetY = 13;
inline constexpr std::uint8_t kOriginalDosDefaultJumpTicks = 5;

struct PlayerMotionConfig {
  std::int16_t walk_step = 8;
  std::int16_t jump_impulse = -5;
  std::int16_t air_drift_step = 1;
  std::int16_t max_air_drift = 5;
  std::int16_t gravity_per_tick = 1;
  std::int16_t max_fall_speed = 5;
  std::int16_t fall_start_velocity = 1;
  std::int16_t feet_probe_offset_y = 13;
  std::uint8_t default_jump_ticks = 5;
  bool truncate_jump_on_release = false;
  bool accumulate_air_drift = false;
};

inline PlayerMotionConfig make_original_dos_motion_config() {
  return PlayerMotionConfig{
      .walk_step = kOriginalDosWalkStep,
      .jump_impulse = kOriginalDosJumpImpulse,
      .air_drift_step = kOriginalDosAirDriftStep,
      .max_air_drift = kOriginalDosMaxAirDrift,
      .gravity_per_tick = kOriginalDosGravityPerTick,
      .max_fall_speed = kOriginalDosMaxFallSpeed,
      .fall_start_velocity = kOriginalDosFallStartVelocity,
      .feet_probe_offset_y = kOriginalDosFeetProbeOffsetY,
      .default_jump_ticks = kOriginalDosDefaultJumpTicks,
      .truncate_jump_on_release = true,
      .accumulate_air_drift = true,
  };
}

PlayerMotionConfig
get_effective_motion_config(const RuntimeState &state,
                            const PlayerMotionConfig &motion);
bool player_check_jump_tile_collision(
    const RuntimeState &state, std::int16_t test_x, std::int16_t test_y,
    const TileCollisionConfig &collision,
    std::int16_t feet_offset_y = kOriginalDosFeetProbeOffsetY);
void apply_input_tick(RuntimeState &state, const PlayerMotionConfig &motion);
void apply_grounded_physics_tick(RuntimeState &state,
                                 const PlayerMotionConfig &motion,
                                 const TileCollisionConfig &collision);
void apply_airborne_physics_tick(RuntimeState &state,
                                 const PlayerMotionConfig &motion,
                                 const TileCollisionConfig &collision);
void apply_physics_tick(RuntimeState &state, const PlayerMotionConfig &motion,
                        const TileCollisionConfig &collision);

} // namespace comic2
