import json
import shutil
import subprocess
from pathlib import Path

repo_root = Path(__file__).resolve().parent.parent
json_path = repo_root / 'tools' / 'extracted_level_data.json'
cpp_path = repo_root / 'src' / 'level_data.cpp'

with open(json_path) as f:
    levels = json.load(f)

cpp_lines = [
    '#include "comic2/level_data.hpp"',
    '',
    '#include <algorithm>',
    '#include <array>',
    '',
    'namespace comic2 {',
    'namespace {',
    '',
    f'const std::array<LevelRuntimeData, {len(levels)}> kLevelDataTable = {{',
]

for lvl_data in levels:
    lvl = lvl_data['level_id']
    assert lvl_data['touch_trigger']['values'], f"Level {lvl} touch trigger has empty values"
    assert lvl_data['proj_trigger']['values'], f"Level {lvl} projectile trigger has empty values"
    assert lvl_data['mode2_trigger']['values'], f"Level {lvl} mode2 trigger has empty values"
    t_vals = ', '.join(str(v) for v in lvl_data['touch_trigger']['values'])
    p_vals = ', '.join(str(v) for v in lvl_data['proj_trigger']['values'])
    m_vals = ', '.join(str(v) for v in lvl_data['mode2_trigger']['values'])

    cpp_lines.append(f'    // Level {lvl}')
    cpp_lines.append('    LevelRuntimeData{')
    cpp_lines.append(f'        .level_id = {lvl},')
    cpp_lines.append(f'        .room_count = {lvl_data["room_count"]},')
    cpp_lines.append(
        f'        .touch_trigger = {{{lvl_data["touch_trigger"]["min"]}, {lvl_data["touch_trigger"]["max"]}, {{{t_vals}}}}},'
    )
    cpp_lines.append(
        f'        .projectile_trigger = {{{lvl_data["proj_trigger"]["min"]}, {lvl_data["proj_trigger"]["max"]}, {{{p_vals}}}}},'
    )
    cpp_lines.append(
        f'        .mode2_trigger = {{{lvl_data["mode2_trigger"]["min"]}, {lvl_data["mode2_trigger"]["max"]}, {{{m_vals}}}}},'
    )

    # Transition zones
    cpp_lines.append('        .transition_zones = {')
    for rm_idx, zones in enumerate(lvl_data['transition_zones']):
        z_strs = [
            f'RoomTransitionZone{{{z["x"]}, {z["y_min"]}, {z["y_max"]}, {z["target_level"]}, {z["target_room"]}, {z["spawn_index"]}}}'
            for z in zones
        ]
        cpp_lines.append(f'            /* room {rm_idx} */ {{{", ".join(z_strs)}}},')
    cpp_lines.append('        },')

    # Active enemies (List C)
    cpp_lines.append('        .active_enemies = {')
    for rm_idx, enemies in enumerate(lvl_data['room_entities']):
        e_strs = [
            f'RoomEntitySpawnDef{{{e[0]}, {e[1]}, {e[2]}, {e[3]}, {e[4]}, {e[5]}}}'
            for e in enemies
        ]
        cpp_lines.append(f'            /* room {rm_idx} */ {{{", ".join(e_strs)}}},')
    cpp_lines.append('        },')

    # Special entities (List A)
    cpp_lines.append('        .special_entities = {')
    for rm_idx, specials in enumerate(lvl_data['list_a']):
        s_strs = [
            f'RoomEntitySpawnDef{{{s[0]}, {s[1]}, {s[2]}, {s[3]}, {s[4]}, {s[5]}}}'
            for s in specials
        ]
        cpp_lines.append(f'            /* room {rm_idx} */ {{{", ".join(s_strs)}}},')
    cpp_lines.append('        },')
    cpp_lines.append('    },')

cpp_lines.append('};')
cpp_lines.append('')
cpp_lines.append('} // namespace')
cpp_lines.append('')
cpp_lines.append("""const LevelRuntimeData *get_level_runtime_data(std::uint16_t level_id) {
  if (level_id >= kLevelDataTable.size()) {
    return nullptr;
  }
  return &kLevelDataTable[level_id];
}

std::optional<RoomTransitionZone>
check_room_transition_zone(std::uint16_t level_id, std::uint16_t room_id,
                           std::int16_t player_x, std::int16_t player_y) {
  const auto *lvl = get_level_runtime_data(level_id);
  if (!lvl || room_id >= lvl->transition_zones.size()) {
    return std::nullopt;
  }
  const auto &zones = lvl->transition_zones[room_id];
  const auto it = std::find_if(
      zones.begin(), zones.end(), [&](const RoomTransitionZone &zone) {
        return player_x == zone.x && player_y >= zone.y_min &&
               player_y <= zone.y_max;
      });
  if (it != zones.end()) {
    return *it;
  }
  return std::nullopt;
}

} // namespace comic2
""")

with open(cpp_path, 'w') as f:
    f.write('\n'.join(cpp_lines))

clang_format = shutil.which("clang-format")
if clang_format:
    subprocess.run([clang_format, "-i", str(cpp_path)], check=True)
else:
    print("Warning: clang-format not found; generated file was not formatted.")

print(f"Successfully generated {cpp_path}")
