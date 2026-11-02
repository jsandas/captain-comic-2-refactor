#pragma once

#include <cstdint>
#include <vector>

#include "comic2/types.hpp"

namespace comic2 {

struct RuntimeState;

struct ProjectileState {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t x_vel = 0;
  std::int16_t y_vel = 0;
  std::int16_t anim_frame = 0;
  bool active = true;

  bool operator==(const ProjectileState &) const = default;
};

struct ProjectileBounds {
  std::int16_t min_x = 0;
  std::int16_t max_x = 319;
  std::int16_t min_y = 0;
  std::int16_t max_y = 199;
};

struct ProjectileSpawnParams {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t x_vel = 0;
  std::int16_t y_vel = 0;
  std::uint8_t firepower = 1;
  bool facing_right = true;
  bool is_airborne = false;
};

// Default weapon cooldown in ticks between shots (matching DOS 0x14 / 20 ticks)
inline constexpr std::uint8_t kDefaultWeaponCooldownTicks = 20;

// Maximum firepower progression level (Levels 1-5, DS:0x08ac)
inline constexpr std::uint8_t kMaxFirepowerLevel = 5;

// Attempts to fire a player projectile if cooldown is 0, firepower > 0,
// and current active projectiles < firepower. Returns true if projectile
// spawned.
bool try_player_fire_projectile(RuntimeState &state);

void spawn_projectile(std::vector<ProjectileState> &projectiles, std::int16_t x,
                      std::int16_t y, std::int16_t x_vel, std::int16_t y_vel);

void spawn_player_projectile(std::vector<ProjectileState> &projectiles,
                             const ProjectileSpawnParams &params,
                             RuntimeState *state = nullptr);

void update_projectiles(std::vector<ProjectileState> &projectiles,
                        const ProjectileBounds &bounds,
                        const RoomTileGrid &room_grid,
                        std::int16_t viewport_min_x,
                        std::int16_t viewport_min_y,
                        std::int16_t viewport_width_pixels,
                        std::int16_t viewport_height_pixels);

void advance_runtime_projectiles(RuntimeState &state,
                                 const ProjectileBounds &bounds,
                                 std::int16_t viewport_min_x,
                                 std::int16_t viewport_min_y,
                                 std::int16_t viewport_width_pixels,
                                 std::int16_t viewport_height_pixels);

bool check_projectile_tile_collision(const ProjectileState &projectile,
                                     const RoomTileGrid &room_grid);

} // namespace comic2
