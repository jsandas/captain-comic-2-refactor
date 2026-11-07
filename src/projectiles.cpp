#include "comic2/projectiles.hpp"

#include <algorithm>

#include "comic2/audio.hpp"
#include "comic2/game_state.hpp"
#include "comic2/tile_collision.hpp"

namespace comic2 {

bool try_player_fire_projectile(RuntimeState &state) {
  if (state.player.weapon_cooldown > 0 || state.player.firepower == 0) {
    return false;
  }

  const std::size_t active_count = static_cast<std::size_t>(
      std::count_if(state.projectiles.begin(), state.projectiles.end(),
                    [](const ProjectileState &p) { return p.active; }));

  if (active_count >=
      std::min<std::uint8_t>(state.player.firepower, kMaxFirepowerLevel)) {
    return false;
  }

  const ProjectileSpawnParams params{
      .x = state.player.x,
      .y = state.player.y,
      .x_vel = 0,
      .y_vel = 0,
      .firepower = state.player.firepower,
      .facing_right = state.player.facing_right,
      .is_airborne = state.player.is_airborne,
  };

  spawn_player_projectile(state.projectiles, params, &state);
  state.player.weapon_cooldown = kDefaultWeaponCooldownTicks;
  state.player.is_attack_active = true;
  state.player.attack_overlay_ticks = 2;
  return true;
}

void spawn_projectile(std::vector<ProjectileState> &projectiles, std::int16_t x,
                      std::int16_t y, std::int16_t x_vel, std::int16_t y_vel) {
  projectiles.push_back(ProjectileState{
      .x = x, .y = y, .x_vel = x_vel, .y_vel = y_vel, .active = true});
}

void spawn_player_projectile(std::vector<ProjectileState> &projectiles,
                             const ProjectileSpawnParams &params,
                             RuntimeState *state) {
  const std::int16_t x_vel = params.facing_right ? 0x0010 : -0x0010;

  std::int16_t y = params.y + 4;
  if (!params.is_airborne) {
    y += 4;
  }
  std::int16_t y_vel = -0x000C;

  ProjectileState projectile;
  projectile.x = params.x + 4;
  projectile.y = y;
  projectile.x_vel = x_vel;
  projectile.y_vel = y_vel;
  projectile.anim_frame = 0;
  projectile.active = true;

  projectiles.push_back(projectile);
  if (state != nullptr) {
    queue_audio_event(*state, AudioEvent::Shoot);
  }
}

bool check_projectile_tile_collision(const ProjectileState &projectile,
                                     const RoomTileGrid &room_grid) {
  const std::int16_t probe_x = projectile.x + 3;
  const std::int16_t probe_y = projectile.y + 3;

  const auto tile_id = get_tile_at_pixels(room_grid, probe_x, probe_y);
  if (!tile_id.has_value()) {
    return false;
  }

  return tile_id.value() != 0;
}

void update_projectiles(std::vector<ProjectileState> &projectiles,
                        const ProjectileBounds &bounds,
                        const RoomTileGrid &room_grid,
                        std::int16_t viewport_min_x,
                        std::int16_t viewport_min_y,
                        std::int16_t viewport_width_pixels,
                        std::int16_t viewport_height_pixels) {
  const std::int16_t viewport_max_x = viewport_min_x + viewport_width_pixels;
  const std::int16_t viewport_max_y = viewport_min_y + viewport_height_pixels;

  for (auto &projectile : projectiles) {
    if (!projectile.active) {
      continue;
    }

    projectile.x = static_cast<std::int16_t>(projectile.x + projectile.x_vel);
    projectile.y = static_cast<std::int16_t>(projectile.y + projectile.y_vel);

    projectile.anim_frame =
        static_cast<std::int16_t>((projectile.anim_frame + 1) % 8);

    const bool outside_viewport = (projectile.x < viewport_min_x - 8) ||
                                  (projectile.x > viewport_max_x + 8) ||
                                  (projectile.y < viewport_min_y - 8) ||
                                  (projectile.y > viewport_max_y + 8);

    if (outside_viewport) {
      projectile.active = false;
      continue;
    }

    if (check_projectile_tile_collision(projectile, room_grid)) {
      projectile.active = false;
      continue;
    }

    if (projectile.x < bounds.min_x || projectile.x > bounds.max_x ||
        projectile.y < bounds.min_y || projectile.y > bounds.max_y) {
      projectile.active = false;
    }
  }
}

void advance_runtime_projectiles(RuntimeState &state,
                                 const ProjectileBounds &bounds,
                                 std::int16_t viewport_min_x,
                                 std::int16_t viewport_min_y,
                                 std::int16_t viewport_width_pixels,
                                 std::int16_t viewport_height_pixels) {
  if (state.player.weapon_cooldown > 0) {
    --state.player.weapon_cooldown;
  }
  if (state.input.fire_pressed) {
    try_player_fire_projectile(state);
  }
  if (state.player.corkscrew_equipped) {
    for (const auto &proj : state.projectiles) {
      if (proj.active) {
        try_break_grate_tile(state, static_cast<std::int16_t>(proj.x + 3),
                             static_cast<std::int16_t>(proj.y + 3));
      }
    }
  }
  update_projectiles(state.projectiles, bounds, state.room_grid, viewport_min_x,
                     viewport_min_y, viewport_width_pixels,
                     viewport_height_pixels);
}

} // namespace comic2
