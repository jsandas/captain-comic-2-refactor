#include "comic2/tile_collision.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "comic2/audio.hpp"

namespace comic2 {

namespace {

constexpr std::int16_t kTileSizePixels = 16;
constexpr std::int16_t kPlayerWidthPixels = 16;
constexpr std::int16_t kPlayerHeightPixels = 32;
constexpr std::int16_t kFootProbeInsetPixels = 2;

bool has_valid_room_grid(const RoomTileGrid &grid) {
  return grid.tile_w > 0 && grid.tile_h > 0 &&
         grid.row_pointers.size() >= grid.tile_h;
}

bool is_solid_at_pixels(const RoomTileGrid &grid, std::int16_t pixel_x,
                        std::int16_t pixel_y,
                        const TileCollisionConfig &config) {
  const std::optional<std::uint8_t> tile_id =
      get_tile_at_pixels(grid, pixel_x, pixel_y);
  return tile_id.has_value() &&
         tile_meets_threshold(*tile_id, config.solid_tile_threshold);
}

bool is_hazard_at_pixels(const RoomTileGrid &grid, std::int16_t pixel_x,
                         std::int16_t pixel_y,
                         const TileCollisionConfig &config) {
  const std::optional<std::uint8_t> tile_id =
      get_tile_at_pixels(grid, pixel_x, pixel_y);
  return tile_id.has_value() && is_hazard_tile(*tile_id, config);
}

} // namespace

std::optional<std::uint8_t> get_tile_at_pixels(const RoomTileGrid &grid,
                                               std::int16_t pixel_x,
                                               std::int16_t pixel_y) {
  if (grid.tile_w == 0 || grid.tile_h == 0 ||
      grid.row_pointers.size() < grid.tile_h) {
    return std::nullopt;
  }

  if (pixel_x < 0 || pixel_y < 0) {
    return std::nullopt;
  }

  const std::int32_t tile_x =
      static_cast<std::int32_t>(pixel_x) / kTileSizePixels;
  const std::int32_t tile_y =
      static_cast<std::int32_t>(pixel_y) / kTileSizePixels;

  if (tile_x < 0 || tile_y < 0 || tile_x >= grid.tile_w ||
      tile_y >= grid.tile_h) {
    return std::nullopt;
  }

  const std::size_t row_base =
      grid.row_pointers[static_cast<std::size_t>(tile_y)];
  const std::size_t tile_off = row_base + static_cast<std::size_t>(tile_x);
  if (tile_off >= grid.tile_data.size()) {
    return std::nullopt;
  }

  return grid.tile_data[tile_off];
}

bool set_tile_at_coords(RoomTileGrid &grid, std::uint16_t tile_x,
                        std::uint16_t tile_y, std::uint8_t tile_id) {
  if (grid.tile_w == 0 || grid.tile_h == 0 ||
      grid.row_pointers.size() < grid.tile_h) {
    return false;
  }
  if (tile_x >= grid.tile_w || tile_y >= grid.tile_h) {
    return false;
  }
  const std::size_t row_base = grid.row_pointers[tile_y];
  const std::size_t tile_off = row_base + static_cast<std::size_t>(tile_x);
  if (tile_off >= grid.tile_data.size()) {
    return false;
  }
  grid.tile_data[tile_off] = tile_id;
  return true;
}

bool set_tile_at_pixels(RoomTileGrid &grid, std::int16_t pixel_x,
                        std::int16_t pixel_y, std::uint8_t tile_id) {
  if (pixel_x < 0 || pixel_y < 0) {
    return false;
  }
  const auto tile_x = static_cast<std::uint16_t>(pixel_x / kTileSizePixels);
  const auto tile_y = static_cast<std::uint16_t>(pixel_y / kTileSizePixels);
  return set_tile_at_coords(grid, tile_x, tile_y, tile_id);
}

bool tile_meets_threshold(std::uint8_t tile_id, std::uint8_t threshold) {
  return tile_id >= threshold;
}

bool is_tile_passable(std::uint8_t tile_id, const TileCollisionConfig &config) {
  return tile_id < config.solid_tile_threshold;
}

bool is_tile_passable_at_pixels(const RoomTileGrid &grid, std::int16_t pixel_x,
                                std::int16_t pixel_y,
                                const TileCollisionConfig &config) {
  const auto tile = get_tile_at_pixels(grid, pixel_x, pixel_y);
  if (!tile.has_value()) {
    return false;
  }
  return is_tile_passable(*tile, config);
}

bool is_hazard_tile(std::uint8_t tile_id, const TileCollisionConfig &config) {
  return tile_id >= config.hazard_tile_min && tile_id <= config.hazard_tile_max;
}

bool update_player_hazard_state(RuntimeState &state,
                                const TileCollisionConfig &config) {
  if (!has_valid_room_grid(state.room_grid)) {
    state.flags.tile_hazard_triggered = false;
    return false;
  }

  const std::int16_t probe_y =
      static_cast<std::int16_t>(state.player.y + kPlayerHeightPixels);
  const std::int16_t left_probe_x =
      static_cast<std::int16_t>(state.player.x + kFootProbeInsetPixels);
  const std::int16_t right_probe_x = static_cast<std::int16_t>(
      state.player.x + kPlayerWidthPixels - 1 - kFootProbeInsetPixels);

  const bool left_hazard =
      is_hazard_at_pixels(state.room_grid, left_probe_x, probe_y, config);
  const bool right_hazard =
      is_hazard_at_pixels(state.room_grid, right_probe_x, probe_y, config);

  state.flags.tile_hazard_triggered = left_hazard || right_hazard;
  return state.flags.tile_hazard_triggered;
}

bool has_floor_support(const RuntimeState &state,
                       const TileCollisionConfig &config) {
  if (!has_valid_room_grid(state.room_grid)) {
    return false;
  }

  const std::int16_t probe_y =
      static_cast<std::int16_t>(state.player.y + kPlayerHeightPixels);
  const std::int16_t left_probe_x =
      static_cast<std::int16_t>(state.player.x + kFootProbeInsetPixels);
  const std::int16_t right_probe_x = static_cast<std::int16_t>(
      state.player.x + kPlayerWidthPixels - 1 - kFootProbeInsetPixels);

  return is_solid_at_pixels(state.room_grid, left_probe_x, probe_y, config) ||
         is_solid_at_pixels(state.room_grid, right_probe_x, probe_y, config);
}

bool resolve_ground_contact(RuntimeState &state,
                            const TileCollisionConfig &config) {
  if (!has_valid_room_grid(state.room_grid)) {
    return false;
  }

  if (state.player.y_vel < 0) {
    return false;
  }

  const std::int16_t probe_y =
      static_cast<std::int16_t>(state.player.y + kPlayerHeightPixels);
  const std::int16_t left_probe_x =
      static_cast<std::int16_t>(state.player.x + kFootProbeInsetPixels);
  const std::int16_t right_probe_x = static_cast<std::int16_t>(
      state.player.x + kPlayerWidthPixels - 1 - kFootProbeInsetPixels);

  const bool left_solid =
      is_solid_at_pixels(state.room_grid, left_probe_x, probe_y, config);
  const bool right_solid =
      is_solid_at_pixels(state.room_grid, right_probe_x, probe_y, config);
  if (!left_solid && !right_solid) {
    return false;
  }

  const std::int16_t tile_top =
      static_cast<std::int16_t>((probe_y / kTileSizePixels) * kTileSizePixels);
  state.player.y = static_cast<std::int16_t>(tile_top - kPlayerHeightPixels);
  state.player.y_vel = 0;
  state.player.is_airborne = false;
  state.player.is_physics_active = false;
  return true;
}

bool try_break_grate_tile(RuntimeState &state, std::int16_t pixel_x,
                          std::int16_t pixel_y,
                          const TileTransformationConfig &config) {
  if (!state.player.corkscrew_equipped) {
    return false;
  }
  const auto tile = get_tile_at_pixels(state.room_grid, pixel_x, pixel_y);
  if (!tile.has_value() || *tile != config.grate_tile) {
    return false;
  }
  return set_tile_at_pixels(state.room_grid, pixel_x, pixel_y,
                            config.passable_tile);
}

bool try_unlock_door_tile(RuntimeState &state, std::int16_t pixel_x,
                          std::int16_t pixel_y,
                          const TileTransformationConfig &config) {
  if (state.player.key_count == 0) {
    return false;
  }
  const auto tile = get_tile_at_pixels(state.room_grid, pixel_x, pixel_y);
  if (!tile.has_value() || *tile != config.locked_door_tile) {
    return false;
  }
  --state.player.key_count;
  const bool changed = set_tile_at_pixels(state.room_grid, pixel_x, pixel_y,
                                          config.passable_tile);
  if (changed) {
    queue_audio_event(state, AudioEvent::DoorOpen);
  }
  return changed;
}

bool try_use_teleport_wand(RuntimeState &state,
                           const TileTransformationConfig &config) {
  (void)config;
  if (!state.player.wand_equipped) {
    return false;
  }
  if (state.player.wand_charges > 0) {
    --state.player.wand_charges;
  }
  if (state.warp_beacon.has_value()) {
    state.player.x = state.warp_beacon->first;
    state.player.y = state.warp_beacon->second;
  } else {
    state.player.x = state.room_entrance_x;
    state.player.y = state.room_entrance_y;
  }
  state.player.x_vel = 0;
  state.player.y_vel = 0;
  state.player.is_airborne = false;
  queue_audio_event(state, AudioEvent::Teleport);
  return true;
}

bool try_activate_warp_beacon(RuntimeState &state, std::int16_t pixel_x,
                              std::int16_t pixel_y,
                              const TileTransformationConfig &config) {
  if (!state.player.wand_equipped) {
    return false;
  }
  const auto tile = get_tile_at_pixels(state.room_grid, pixel_x, pixel_y);
  if (!tile.has_value() || *tile != config.warp_beacon_tile) {
    return false;
  }
  const auto tile_x = static_cast<std::uint16_t>(pixel_x / kTileSizePixels);
  const auto tile_y = static_cast<std::uint16_t>(pixel_y / kTileSizePixels);
  const bool changed = set_tile_at_coords(state.room_grid, tile_x, tile_y,
                                          config.active_warp_portal_tile);
  if (changed) {
    state.warp_beacon = {static_cast<std::int16_t>(tile_x * kTileSizePixels),
                         static_cast<std::int16_t>(tile_y * kTileSizePixels)};
    queue_audio_event(state, AudioEvent::Teleport);
  }
  return changed;
}

bool process_player_tile_interactions(RuntimeState &state,
                                      const TileTransformationConfig &config) {
  if (!has_valid_room_grid(state.room_grid)) {
    return false;
  }

  const std::int16_t px = state.player.x;
  const std::int16_t py = state.player.y;

  const std::pair<std::int16_t, std::int16_t> probe_points[] = {
      {static_cast<std::int16_t>(
           px + (state.player.facing_right ? kPlayerWidthPixels : -1)),
       static_cast<std::int16_t>(py + 8)},
      {static_cast<std::int16_t>(
           px + (state.player.facing_right ? kPlayerWidthPixels : -1)),
       static_cast<std::int16_t>(py + 24)},
      {static_cast<std::int16_t>(
           px + (state.player.facing_right ? -1 : kPlayerWidthPixels)),
       static_cast<std::int16_t>(py + 8)},
      {static_cast<std::int16_t>(
           px + (state.player.facing_right ? -1 : kPlayerWidthPixels)),
       static_cast<std::int16_t>(py + 24)},
      {static_cast<std::int16_t>(px + 8), static_cast<std::int16_t>(py + 16)},
      {static_cast<std::int16_t>(px + 4),
       static_cast<std::int16_t>(py + kPlayerHeightPixels)},
      {static_cast<std::int16_t>(px + 12),
       static_cast<std::int16_t>(py + kPlayerHeightPixels)},
      {static_cast<std::int16_t>(px + 8), static_cast<std::int16_t>(py - 1)},
  };

  std::vector<std::pair<std::uint16_t, std::uint16_t>> visited_tiles;
  for (const auto &[test_x, test_y] : probe_points) {
    if (test_x < 0 || test_y < 0) {
      continue;
    }
    const auto tx = static_cast<std::uint16_t>(test_x / kTileSizePixels);
    const auto ty = static_cast<std::uint16_t>(test_y / kTileSizePixels);
    if (tx >= state.room_grid.tile_w || ty >= state.room_grid.tile_h) {
      continue;
    }
    bool already_visited = false;
    for (const auto &vt : visited_tiles) {
      if (vt.first == tx && vt.second == ty) {
        already_visited = true;
        break;
      }
    }
    if (!already_visited) {
      visited_tiles.push_back({tx, ty});
    }
  }

  bool modified = false;
  for (const auto &[tx, ty] : visited_tiles) {
    const std::int16_t tile_pixel_x =
        static_cast<std::int16_t>(tx * kTileSizePixels);
    const std::int16_t tile_pixel_y =
        static_cast<std::int16_t>(ty * kTileSizePixels);
    const auto tile_id =
        get_tile_at_pixels(state.room_grid, tile_pixel_x, tile_pixel_y);
    if (!tile_id.has_value()) {
      continue;
    }

    if (*tile_id == config.grate_tile) {
      if (try_break_grate_tile(state, tile_pixel_x, tile_pixel_y, config)) {
        modified = true;
      }
    } else if (*tile_id == config.locked_door_tile) {
      if (try_unlock_door_tile(state, tile_pixel_x, tile_pixel_y, config)) {
        modified = true;
      }
    } else if (*tile_id == config.warp_beacon_tile) {
      if (try_activate_warp_beacon(state, tile_pixel_x, tile_pixel_y, config)) {
        modified = true;
      }
    }
  }

  return modified;
}

} // namespace comic2
