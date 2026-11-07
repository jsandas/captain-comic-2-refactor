#pragma once

#include <cstdint>
#include <limits>
#include <optional>

#include "comic2/game_state.hpp"

namespace comic2 {

struct TileCollisionConfig {
  std::uint8_t solid_tile_threshold = 0x01;
  std::uint8_t hazard_tile_min = 0xF0;
  std::uint8_t hazard_tile_max = 0xFF;
};

// Authentic tile IDs
inline constexpr std::uint8_t kPassableTileId = 0x00;
inline constexpr std::uint8_t kGrateTileId = 0x1A;
inline constexpr std::uint8_t kLockedDoorTileId = 0x1B;
inline constexpr std::uint8_t kWarpBeaconTileId = 0x1C;
inline constexpr std::uint8_t kActiveWarpPortalTileId = 0x1D;

struct TileTransformationConfig {
  std::uint8_t passable_tile = kPassableTileId;
  std::uint8_t grate_tile = kGrateTileId;
  std::uint8_t locked_door_tile = kLockedDoorTileId;
  std::uint8_t warp_beacon_tile = kWarpBeaconTileId;
  std::uint8_t active_warp_portal_tile = kActiveWarpPortalTileId;
  std::uint8_t solid_tile_threshold = 0x01;
};

std::optional<std::uint8_t> get_tile_at_pixels(const RoomTileGrid &grid,
                                               std::int16_t pixel_x,
                                               std::int16_t pixel_y);
bool set_tile_at_coords(RoomTileGrid &grid, std::uint16_t tile_x,
                        std::uint16_t tile_y, std::uint8_t tile_id);
bool set_tile_at_pixels(RoomTileGrid &grid, std::int16_t pixel_x,
                        std::int16_t pixel_y, std::uint8_t tile_id);

bool tile_meets_threshold(std::uint8_t tile_id, std::uint8_t threshold);
bool is_tile_passable(std::uint8_t tile_id,
                      const TileCollisionConfig &config = {});
bool is_tile_passable_at_pixels(const RoomTileGrid &grid, std::int16_t pixel_x,
                                std::int16_t pixel_y,
                                const TileCollisionConfig &config = {});
bool is_hazard_tile(std::uint8_t tile_id, const TileCollisionConfig &config);
bool update_player_hazard_state(RuntimeState &state,
                                const TileCollisionConfig &config);

// Tile transformation mechanics
bool try_break_grate_tile(RuntimeState &state, std::int16_t pixel_x,
                          std::int16_t pixel_y,
                          const TileTransformationConfig &config = {});
bool try_unlock_door_tile(RuntimeState &state, std::int16_t pixel_x,
                          std::int16_t pixel_y,
                          const TileTransformationConfig &config = {});
bool try_use_teleport_wand(RuntimeState &state,
                           const TileTransformationConfig &config = {});
bool try_activate_warp_beacon(RuntimeState &state, std::int16_t pixel_x,
                              std::int16_t pixel_y,
                              const TileTransformationConfig &config = {});
bool process_player_tile_interactions(
    RuntimeState &state, const TileTransformationConfig &config = {});

bool has_floor_support(const RuntimeState &state,
                       const TileCollisionConfig &config);
bool resolve_ground_contact(RuntimeState &state,
                            const TileCollisionConfig &config);

} // namespace comic2
