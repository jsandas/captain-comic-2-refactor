#pragma once

#include <cstdint>
#include <vector>

#include "comic2/types.hpp"

namespace comic2 {

struct RuntimeState;
struct ProjectileState;

inline constexpr std::uint16_t kBehaviorInactive = 0x0000;
inline constexpr std::uint16_t kBehaviorChase = 0x0001;
inline constexpr std::uint16_t kBehaviorBounce = 0x0002;
inline constexpr std::uint16_t kBehaviorJump = 0x0003;
inline constexpr std::uint16_t kBehaviorGem = 0x0004;
inline constexpr std::uint16_t kBehaviorPowerup = 0x0005;
inline constexpr std::uint16_t kBehaviorGravity = 0x0006;
inline constexpr std::uint16_t kBehaviorShoot = 0x0007;
inline constexpr std::uint16_t kBehaviorBoss = 0x0008;

// Authentic 33-entry behavior jump table handlers from comic2.asm (0x4AE3,
// 1000:50E7 - 1000:59C9)
inline constexpr std::uint16_t kBehaviorAsm01_LedgePatrol = 1;
inline constexpr std::uint16_t kBehaviorAsm02_ProximityHopper = 2;
inline constexpr std::uint16_t kBehaviorAsm03_OscillatingFloater = 3;
inline constexpr std::uint16_t kBehaviorAsm04_TimidFollower = 4;
inline constexpr std::uint16_t kBehaviorAsm05_Swooper = 5;
inline constexpr std::uint16_t kBehaviorAsm06_FlyingHomingChaser = 6;
inline constexpr std::uint16_t kBehaviorAsm07_StaticSentry = 7;
inline constexpr std::uint16_t kBehaviorAsm08_ProximitySentry = 8;
inline constexpr std::uint16_t kBehaviorAsm09_HeavyLeaper = 9;
inline constexpr std::uint16_t kBehaviorAsm10_GhostPhaser = 10;
inline constexpr std::uint16_t kBehaviorAsm11_HighBouncer = 11;
inline constexpr std::uint16_t kBehaviorAsm12_CeilingDropHazard = 12;
inline constexpr std::uint16_t kBehaviorAsm13_FastLedgePatrol = 13;
inline constexpr std::uint16_t kBehaviorAsm14_DirectionLockedWalker = 14;
inline constexpr std::uint16_t kBehaviorAsm15_GroundPounder = 15;
inline constexpr std::uint16_t kBehaviorAsm16_DiagonalBouncer = 16;
inline constexpr std::uint16_t kBehaviorAsm17_TrampolineJumper = 17;
inline constexpr std::uint16_t kBehaviorAsm18_WallJumpingRunner = 18;
inline constexpr std::uint16_t kBehaviorAsm19_DropTrap1 = 19;
inline constexpr std::uint16_t kBehaviorAsm20_SpikeDropTrap2 = 20;
inline constexpr std::uint16_t kBehaviorAsm21_HovercraftOscillator = 21;
inline constexpr std::uint16_t kBehaviorAsm22_TwoPhaseDrone = 22;
inline constexpr std::uint16_t kBehaviorAsm23_FastRammer = 23;
inline constexpr std::uint16_t kBehaviorAsm24_CeilingDropGhost = 24;
inline constexpr std::uint16_t kBehaviorAsm25_InvertedCeilingCrawler = 25;
inline constexpr std::uint16_t kBehaviorAsm26_StraightHorizCruiser = 26;
inline constexpr std::uint16_t kBehaviorAsm27_BossWalker = 27;
inline constexpr std::uint16_t kBehaviorAsm28_BossSwooper = 28;
inline constexpr std::uint16_t kBehaviorAsm29_LeftwardDart = 29;
inline constexpr std::uint16_t kBehaviorAsm30_TorpedoSprinter = 30;
inline constexpr std::uint16_t kBehaviorAsm31_HeavyStalactite = 31;
inline constexpr std::uint16_t kBehaviorAsm32_LeftSwoopDiver = 32;
inline constexpr std::uint16_t kBehaviorAsm33_RightSwoopDiver = 33;

inline constexpr std::uint16_t kEntityDropNone = 0x0000;
inline constexpr std::uint16_t kEntityDropGem = 0x0010;
inline constexpr std::uint16_t kEntityDropPowerup = 0x0020;
inline constexpr std::uint16_t kEntityInvulnerable = 0x8000;

struct ProjectileCombatResult {
  std::size_t projectiles_hit = 0;
  std::size_t entities_damaged = 0;
  std::size_t entities_defeated = 0;
  std::size_t items_dropped = 0;
};

struct EntityViewportBounds {
  std::int16_t min_x = 0;
  std::int16_t max_x = 319;
  std::int16_t min_y = 0;
  std::int16_t max_y = 199;
};

struct EntityActivationState {
  std::uint8_t first_active_slot = 0;
  std::uint8_t active_count = 0;
  std::uint8_t slot_data[9] = {};
  bool operator==(const EntityActivationState &) const = default;
};

bool is_runtime_slot_active(const RuntimeEntitySlot32 &slot);
void deactivate_runtime_slot(RuntimeEntitySlot32 &slot);
void prune_inactive_runtime_slots(std::vector<RuntimeEntitySlot32> &slots);

void ent_build_room_entity_list(
    const std::vector<MappedObject12> &mapped_objects, std::uint16_t room_x,
    std::uint16_t room_y, std::vector<ActiveEntity8> &active_entities,
    EntityActivationState &activation_state);

void ent_build_runtime_slots_for_viewport(
    const std::vector<ActiveEntity8> &active_entities,
    const std::vector<MappedObject12> &mapped_objects,
    std::vector<RuntimeEntitySlot32> &runtime_slots,
    const EntityViewportBounds &viewport_bounds,
    EntityActivationState &activation_state, std::uint16_t &activation_toggle);

void ent_copy_descriptor_to_runtime_slot(const MappedObject12 &descriptor,
                                         RuntimeEntitySlot32 &slot,
                                         std::uint16_t behavior_ptr,
                                         std::uint16_t &activation_toggle);

void ent_deactivate_runtime_slot(RuntimeEntitySlot32 &slot);

bool collides_with_projectile(const RuntimeEntitySlot32 &slot,
                              const ProjectileState &projectile);

ProjectileCombatResult apply_projectile_entity_collisions(RuntimeState &state);

bool spawn_item_drop(RuntimeState &state, std::int16_t x, std::int16_t y,
                     std::uint16_t item_behavior);

// Authentic descriptor-driven animation cycling (comic2.asm loc_5090)
std::uint16_t advance_entity_animation_tick(RuntimeEntitySlot32 &slot);
std::uint16_t compute_entity_animation_frame(const RuntimeEntitySlot32 &slot);

// Authentic 33-entry behavior execution and main entity update loop
void ent_execute_authentic_asm_behavior(GameState &state,
                                        RuntimeEntitySlot32 &slot,
                                        std::uint16_t asm_handler_index);
void ent_update_object_behaviors(GameState &state);

// Hostile enemy projectile helpers
bool spawn_enemy_projectile(GameState &state, std::int16_t x, std::int16_t y,
                            std::int16_t vx, std::int16_t vy);
void update_enemy_projectiles(GameState &state);
void trigger_player_death(GameState &state);

// Multi-phase boss state machine helpers
inline constexpr std::uint8_t kDefaultBossHP = 6;
inline constexpr std::uint8_t kBossInvulnerabilityTicks = 8;
void handle_boss_behavior(GameState &state, RuntimeEntitySlot32 &slot);

void update_entity_behaviors(RuntimeState &state);
void apply_entity_combat(RuntimeState &state);

} // namespace comic2
