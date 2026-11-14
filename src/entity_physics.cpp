#include "comic2/entity_physics.hpp"

#include "comic2/game_state.hpp"
#include "comic2/tile_collision.hpp"

namespace comic2 {

namespace {

// get_tile_at_pixels (1000:1cff): returns 0x100 when out of the grid.
std::int16_t tile_at(const EntityPhysicsContext &ctx, std::int16_t px,
                     std::int16_t py) {
  if (ctx.grid == nullptr) {
    return kEntityOutOfRangeTile;
  }
  const auto tile = get_tile_at_pixels(*ctx.grid, px, py);
  return tile.has_value() ? static_cast<std::int16_t>(*tile)
                          : kEntityOutOfRangeTile;
}

// ent_probe_tile_above_threshold: carry when py < 0 or tile <= threshold.
bool probe_tile(const EntityPhysicsContext &ctx, std::int16_t px,
                std::int16_t py, std::int16_t threshold) {
  if (py < 0) {
    return true;
  }
  return tile_at(ctx, px, py) <= threshold;
}

std::int16_t snap16(std::int32_t v) {
  return static_cast<std::int16_t>(v & 0xFFF0);
}

} // namespace

bool ent_probe_x_collision_strict(const EntityPhysicsContext &ctx,
                                  const RuntimeEntitySlot32 &slot,
                                  std::int16_t pixel_x) {
  if (pixel_x < ctx.camera_x ||
      pixel_x >=
          static_cast<std::int16_t>(ctx.camera_x + kEntityViewportWidth)) {
    return true;
  }
  const auto probe_y = static_cast<std::int16_t>(
      slot.y + static_cast<std::int16_t>(slot.hitbox_h) - 8);
  return tile_at(ctx, pixel_x, probe_y) <= ctx.threshold_up;
}

bool ent_move_x_and_bounce(const EntityPhysicsContext &ctx,
                           RuntimeEntitySlot32 &slot, std::int16_t dx) {
  const bool facing_left = slot.dir_toggle == 2;
  if (facing_left) {
    dx = static_cast<std::int16_t>(-dx);
  }
  slot.param_a = dx;
  const auto new_x = static_cast<std::int16_t>(slot.x + dx);
  const auto probe_x =
      facing_left ? new_x
                  : static_cast<std::int16_t>(
                        new_x + static_cast<std::int16_t>(slot.hitbox_w) - 1);

  if (ent_probe_x_collision_strict(ctx, slot, probe_x)) {
    slot.dir_toggle = facing_left ? 1 : 2;
    slot.param_a = static_cast<std::int16_t>(-slot.param_a);
    return true;
  }
  slot.x = new_x;
  return false;
}

bool ent_apply_gravity_and_move_y(const EntityPhysicsContext &ctx,
                                  RuntimeEntitySlot32 &slot) {
  ++slot.param_b;
  if (slot.param_b >= kEntityMaxFallSpeed) {
    slot.param_b = kEntityMaxFallSpeed;
  }
  const auto h = static_cast<std::int16_t>(slot.hitbox_h);
  const auto probe_x = static_cast<std::int16_t>(
      slot.x + static_cast<std::int16_t>(slot.hitbox_w >> 1));
  const auto new_y = static_cast<std::int16_t>(slot.y + slot.param_b);

  if (slot.param_b >= 0) {
    const auto foot = static_cast<std::int16_t>(new_y + h);
    if (probe_tile(ctx, probe_x, foot, ctx.threshold_down)) {
      slot.param_b = 0;
      slot.y = static_cast<std::int16_t>(snap16(foot) - h);
      return true;
    }
    slot.y = new_y;
    return false;
  }

  if (probe_tile(ctx, probe_x, new_y, ctx.threshold_up)) {
    slot.y = snap16(slot.y);
    return true;
  }
  slot.y = new_y;
  return false;
}

bool ent_move_y_and_bounce(const EntityPhysicsContext &ctx,
                           RuntimeEntitySlot32 &slot) {
  const auto h = static_cast<std::int16_t>(slot.hitbox_h);
  const auto probe_x = static_cast<std::int16_t>(
      slot.x + static_cast<std::int16_t>(slot.hitbox_w >> 1));
  const auto new_y = static_cast<std::int16_t>(slot.y + slot.param_b);

  if (slot.param_b >= 0) {
    const auto foot = static_cast<std::int16_t>(new_y + h);
    const auto threshold =
        ctx.threshold_range == -1 ? ctx.threshold_up : ctx.threshold_range;
    if (probe_tile(ctx, probe_x, foot, threshold)) {
      slot.param_b = static_cast<std::int16_t>(-slot.param_b);
      slot.y = static_cast<std::int16_t>(snap16(foot) - h);
      return true;
    }
    const auto limit =
        static_cast<std::int16_t>(ctx.camera_y + kEntityViewportHeight - h);
    if (new_y >= limit) {
      slot.param_b = static_cast<std::int16_t>(-slot.param_b);
      return true;
    }
    slot.y = new_y;
    return false;
  }

  if (probe_tile(ctx, probe_x, new_y, ctx.threshold_up)) {
    slot.y = snap16(slot.y);
    slot.param_b = static_cast<std::int16_t>(-slot.param_b);
    return true;
  }
  if (new_y <= ctx.camera_y) {
    slot.param_b = static_cast<std::int16_t>(-slot.param_b);
    return true;
  }
  slot.y = new_y;
  return false;
}

EntityPhysicsContext make_physics_context(const GameState &state) {
  EntityPhysicsContext ctx;
  ctx.grid = state.grid != nullptr ? state.grid : &state.room_grid;
  ctx.threshold_up = state.threshold_up != -1 ? state.threshold_up : 1;
  ctx.threshold_down = state.threshold_down != -1 ? state.threshold_down : 1;
  ctx.threshold_range = state.threshold_range;
  ctx.camera_x = static_cast<std::int16_t>(state.camera_x);
  ctx.camera_y = static_cast<std::int16_t>(state.camera_y);
  return ctx;
}

bool ent_probe_tile_above_threshold(const GameState &state,
                                    std::int16_t pixel_x, std::int16_t pixel_y,
                                    std::uint16_t threshold) {
  if (pixel_x < 0 || pixel_y < 0) {
    return true;
  }
  const auto *grid = state.grid != nullptr ? state.grid : &state.room_grid;
  if (grid == nullptr || grid->tile_w == 0) {
    return true;
  }
  const auto tile = get_tile_at_pixels(*grid, pixel_x, pixel_y);
  if (!tile.has_value()) {
    return true;
  }
  const auto tile_id = static_cast<std::int8_t>(*tile);
  const auto thresh = static_cast<std::int8_t>(threshold & 0xFF);
  return tile_id > thresh;
}

bool ent_probe_tile_range_solid(const GameState &state, std::int16_t pixel_x,
                                std::int16_t pixel_y, std::int16_t span,
                                std::uint16_t threshold) {
  if (pixel_x < 0 || pixel_y < 0) {
    return true;
  }
  const auto *grid = state.grid != nullptr ? state.grid : &state.room_grid;
  if (grid == nullptr || grid->tile_w == 0) {
    return true;
  }
  const std::int16_t thresh =
      threshold != 0 ? static_cast<std::int16_t>(threshold & 0xFF)
                     : (state.threshold_up != -1 ? state.threshold_up : 1);
  const std::int16_t step = 16;
  for (std::int16_t y = pixel_y; y < pixel_y + span; y += step) {
    const auto tile = get_tile_at_pixels(*grid, pixel_x, y);
    if (!tile.has_value() || static_cast<std::int16_t>(*tile) <= thresh) {
      return true;
    }
  }
  const std::int16_t bottom_y = static_cast<std::int16_t>(pixel_y + span - 1);
  const auto tile = get_tile_at_pixels(*grid, pixel_x, bottom_y);
  if (!tile.has_value() || static_cast<std::int16_t>(*tile) <= thresh) {
    return true;
  }
  return false;
}

bool ent_probe_tile_range_in_viewport(const GameState &state,
                                      std::int16_t pixel_x,
                                      std::int16_t pixel_y, std::int16_t span,
                                      std::uint16_t threshold) {
  const std::int16_t cam_x = static_cast<std::int16_t>(state.camera_x);
  if (pixel_x < cam_x) {
    return true;
  }
  if ((pixel_x - cam_x) >= 0xF0) {
    return true;
  }
  return ent_probe_tile_range_solid(state, pixel_x, pixel_y, span, threshold);
}

bool ent_probe_x_collision_range(const GameState &state,
                                 const RuntimeEntitySlot32 &slot,
                                 std::int16_t delta_x) {
  const std::int16_t probe_x =
      (delta_x >= 0)
          ? static_cast<std::int16_t>(slot.x + slot.hitbox_w + delta_x - 1)
          : static_cast<std::int16_t>(slot.x + delta_x);
  const std::uint16_t threshold =
      state.threshold_up != -1 ? static_cast<std::uint16_t>(state.threshold_up)
                               : 1U;
  return ent_probe_tile_range_in_viewport(state, probe_x, slot.y, slot.hitbox_h,
                                          threshold);
}

bool ent_probe_y_collision_and_clamp(const GameState &state,
                                     RuntimeEntitySlot32 &slot) {
  const auto *grid = state.grid != nullptr ? state.grid : &state.room_grid;
  if (grid == nullptr || grid->tile_w == 0) {
    return false;
  }
  const auto h = static_cast<std::int16_t>(slot.hitbox_h);
  const auto probe_x = static_cast<std::int16_t>(slot.x + (slot.hitbox_w >> 1));
  const auto new_y = static_cast<std::int16_t>(slot.y + slot.param_b);

  if (slot.param_b >= 0) {
    const auto foot = static_cast<std::int16_t>(new_y + h);
    const auto threshold =
        state.threshold_down != -1 ? state.threshold_down : 1;
    const auto tile = get_tile_at_pixels(*grid, probe_x, foot);
    if (!tile.has_value() || static_cast<std::int16_t>(*tile) <= threshold) {
      slot.y = static_cast<std::int16_t>((foot & 0xFFF0) - h);
      return true;
    }
    return false;
  }

  const auto threshold = state.threshold_up != -1 ? state.threshold_up : 1;
  const auto tile = get_tile_at_pixels(*grid, probe_x, new_y);
  if (!tile.has_value() || static_cast<std::int16_t>(*tile) <= threshold) {
    slot.y = static_cast<std::int16_t>(slot.y & 0xFFF0);
    return true;
  }
  return false;
}

bool ent_move_y_with_probe_and_bounce(const GameState &state,
                                      RuntimeEntitySlot32 &slot) {
  if (ent_probe_y_collision_and_clamp(state, slot)) {
    if (slot.param_b < 0) {
      slot.param_b = 1;
    } else {
      slot.param_b = static_cast<std::int16_t>(-slot.param_b);
    }
    return true;
  }
  slot.y = static_cast<std::int16_t>(slot.y + slot.param_b);
  return false;
}

bool ent_move_x_with_range_probe_and_bounce(const GameState &state,
                                            RuntimeEntitySlot32 &slot,
                                            std::int16_t speed) {
  const std::int16_t dx =
      (slot.dir_toggle == 1) ? speed : static_cast<std::int16_t>(-speed);
  slot.param_a = dx;
  if (ent_probe_x_collision_range(state, slot, dx)) {
    slot.dir_toggle = (slot.dir_toggle == 1) ? 2 : 1;
    slot.param_a = static_cast<std::int16_t>(-slot.param_a);
    return true;
  }
  slot.x = static_cast<std::int16_t>(slot.x + slot.param_a);
  return false;
}

void ent_set_horizontal_chase_direction_to_comic(const GameState &state,
                                                 RuntimeEntitySlot32 &slot) {
  slot.dir_toggle = 1;
  if (state.comic_x < slot.x) {
    slot.dir_toggle = 2;
  }
}

bool ent_probe_x_collision_strict(const GameState &state,
                                  const RuntimeEntitySlot32 &slot,
                                  std::int16_t pixel_x) {
  return ent_probe_x_collision_strict(make_physics_context(state), slot,
                                      pixel_x);
}

bool ent_move_x_and_bounce(const GameState &state, RuntimeEntitySlot32 &slot,
                           std::int16_t dx) {
  return ent_move_x_and_bounce(make_physics_context(state), slot, dx);
}

bool ent_apply_gravity_and_move_y(const GameState &state,
                                  RuntimeEntitySlot32 &slot) {
  return ent_apply_gravity_and_move_y(make_physics_context(state), slot);
}

bool ent_move_y_and_bounce(const GameState &state, RuntimeEntitySlot32 &slot) {
  return ent_move_y_and_bounce(make_physics_context(state), slot);
}

} // namespace comic2
