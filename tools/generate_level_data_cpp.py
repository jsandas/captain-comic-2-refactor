import json

with open('tools/extracted_level_data.json') as f:
    levels = json.load(f)

hpp_content = """#pragma once

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
  std::vector<std::vector<RoomTransitionZone>> transition_zones; // indexed by room
  std::vector<std::vector<RoomEntitySpawnDef>> active_enemies;    // indexed by room (List C)
  std::vector<std::vector<RoomEntitySpawnDef>> special_entities;  // indexed by room (List A)
};

const LevelRuntimeData *get_level_runtime_data(std::uint16_t level_id);

std::optional<RoomTransitionZone> check_room_transition_zone(
    std::uint16_t level_id, std::uint16_t room_id,
    std::int16_t player_x, std::int16_t player_y);

} // namespace comic2
"""

with open('include/comic2/level_data.hpp', 'w') as f:
    f.write(hpp_content)

cpp_lines = [
    '#include "comic2/level_data.hpp"',
    '',
    '#include <array>',
    '',
    'namespace comic2 {',
    'namespace {',
    ''
]

for lvl_data in levels:
    lvl = lvl_data['level_id']
    cpp_lines.append(f'// ================= Level {lvl} Data =================')
    
    # Trigger arrays
    t_vals = ', '.join(str(v) for v in lvl_data['touch_trigger']['values']) or '0'
    p_vals = ', '.join(str(v) for v in lvl_data['proj_trigger']['values']) or '0'
    m_vals = ', '.join(str(v) for v in lvl_data['mode2_trigger']['values']) or '0'
    
    cpp_lines.append(f'const std::vector<std::uint8_t> kLvl{lvl}TouchVals = {{{t_vals}}};')
    cpp_lines.append(f'const std::vector<std::uint8_t> kLvl{lvl}ProjVals = {{{p_vals}}};')
    cpp_lines.append(f'const std::vector<std::uint8_t> kLvl{lvl}Mode2Vals = {{{m_vals}}};')
    
    # Transition zones
    cpp_lines.append(f'const std::vector<std::vector<RoomTransitionZone>> kLvl{lvl}Zones = {{')
    for rm_idx, zones in enumerate(lvl_data['transition_zones']):
        z_strs = []
        for z in zones:
            z_strs.append(f'RoomTransitionZone{{{z["x"]}, {z["y_min"]}, {z["y_max"]}, {z["target_level"]}, {z["target_room"]}, {z["spawn_index"]}}}')
        cpp_lines.append(f'    /* room {rm_idx} */ {{{", ".join(z_strs)}}},')
    cpp_lines.append('};')
    
    # Active enemies (List C)
    cpp_lines.append(f'const std::vector<std::vector<RoomEntitySpawnDef>> kLvl{lvl}Enemies = {{')
    for rm_idx, enemies in enumerate(lvl_data['room_entities']):
        e_strs = []
        for e in enemies:
            e_strs.append(f'RoomEntitySpawnDef{{{e[0]}, {e[1]}, {e[2]}, {e[3]}, {e[4]}, {e[5]}}}')
        cpp_lines.append(f'    /* room {rm_idx} */ {{{", ".join(e_strs)}}},')
    cpp_lines.append('};')

    # Special entities (List A)
    cpp_lines.append(f'const std::vector<std::vector<RoomEntitySpawnDef>> kLvl{lvl}Specials = {{')
    for rm_idx, specials in enumerate(lvl_data['list_a']):
        s_strs = []
        for s in specials:
            s_strs.append(f'RoomEntitySpawnDef{{{s[0]}, {s[1]}, {s[2]}, {s[3]}, {s[4]}, {s[5]}}}')
        cpp_lines.append(f'    /* room {rm_idx} */ {{{", ".join(s_strs)}}},')
    cpp_lines.append('};')
    cpp_lines.append('')

cpp_lines.append('const std::array<LevelRuntimeData, 15> kLevelDataTable = {{')
for lvl_data in levels:
    lvl = lvl_data['level_id']
    cpp_lines.append(f'    LevelRuntimeData{{')
    cpp_lines.append(f'        .level_id = {lvl},')
    cpp_lines.append(f'        .room_count = {lvl_data["room_count"]},')
    cpp_lines.append(f'        .touch_trigger = {{{lvl_data["touch_trigger"]["min"]}, {lvl_data["touch_trigger"]["max"]}, kLvl{lvl}TouchVals}},')
    cpp_lines.append(f'        .projectile_trigger = {{{lvl_data["proj_trigger"]["min"]}, {lvl_data["proj_trigger"]["max"]}, kLvl{lvl}ProjVals}},')
    cpp_lines.append(f'        .mode2_trigger = {{{lvl_data["mode2_trigger"]["min"]}, {lvl_data["mode2_trigger"]["max"]}, kLvl{lvl}Mode2Vals}},')
    cpp_lines.append(f'        .transition_zones = kLvl{lvl}Zones,')
    cpp_lines.append(f'        .active_enemies = kLvl{lvl}Enemies,')
    cpp_lines.append(f'        .special_entities = kLvl{lvl}Specials,')
    cpp_lines.append(f'    }},')
cpp_lines.append('}};')
cpp_lines.append('')
cpp_lines.append('} // namespace')
cpp_lines.append('')
cpp_lines.append("""const LevelRuntimeData *get_level_runtime_data(std::uint16_t level_id) {
  if (level_id >= kLevelDataTable.size()) {
    return nullptr;
  }
  return &kLevelDataTable[level_id];
}

std::optional<RoomTransitionZone> check_room_transition_zone(
    std::uint16_t level_id, std::uint16_t room_id,
    std::int16_t player_x, std::int16_t player_y) {
  const auto *lvl = get_level_runtime_data(level_id);
  if (!lvl || room_id >= lvl->transition_zones.size()) {
    return std::nullopt;
  }
  for (const auto &zone : lvl->transition_zones[room_id]) {
    if (player_x == zone.x && player_y >= zone.y_min && player_y <= zone.y_max) {
      return zone;
    }
  }
  return std::nullopt;
}

} // namespace comic2
""")

with open('src/level_data.cpp', 'w') as f:
    f.write('\n'.join(cpp_lines))

print("Successfully generated include/comic2/level_data.hpp and src/level_data.cpp")
