#include "comic2/game_state.hpp"

#include "comic2/level_data.hpp"

namespace comic2 {

RuntimeState make_default_runtime_state() {
  RuntimeState state{};

  state.level_data = get_level_runtime_data(0);
  state.player.x = 64;
  state.player.y = 96;
  state.player.is_physics_active = true;
  state.player.facing_right = true;
  state.player.hp = 12;
  state.player.firepower = 1;
  state.player.gems = 0;
  state.player.lives = 3;
  state.player.invuln_ticks = 0;
  state.activation_toggle = 1;
  state.camera_x = 0;
  state.camera_y = 0;
  state.palette.loaded = false;
  state.palette.entries.fill({0, 0, 0, 255});

  return state;
}

} // namespace comic2
