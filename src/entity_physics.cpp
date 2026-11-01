#include "comic2/entity_physics.hpp"

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

} // namespace comic2
