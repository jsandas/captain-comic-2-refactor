#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "comic2/room_loader.hpp"
#include "comic2/types.hpp"

namespace comic2 {

struct RoomTransitionZone {
  std::int16_t x = 0;
  std::int16_t y_min = 0;
  std::int16_t y_max = 0;
  std::uint16_t target_level = 0;
  std::uint16_t target_room = 0;
  std::uint16_t spawn_index = 0;

  bool operator==(const RoomTransitionZone &) const = default;
};

struct RoomEntitySpawnDef {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::uint16_t flags = 0;
  std::uint16_t sprite_index = 0;
  std::uint16_t param_a = 0;
  std::uint16_t param_b = 0;

  bool operator==(const RoomEntitySpawnDef &) const = default;
};

struct LevelRuntimeData {
  std::uint16_t level_id = 0;
  std::uint16_t room_count = 0;
  TileTriggerRange touch_trigger;
  TileTriggerRange projectile_trigger;
  TileTriggerRange mode2_trigger;
  std::vector<std::vector<RoomTransitionZone>>
      transition_zones; // indexed by room
  std::vector<std::vector<RoomEntitySpawnDef>>
      active_enemies; // indexed by room (List C)
  std::vector<std::vector<RoomEntitySpawnDef>>
      special_entities; // indexed by room (List A)
};

const LevelRuntimeData *get_level_runtime_data(std::uint16_t level_id);

std::optional<RoomTransitionZone>
check_room_transition_zone(std::uint16_t level_id, std::uint16_t room_id,
                           std::int16_t player_x, std::int16_t player_y);

} // namespace comic2
