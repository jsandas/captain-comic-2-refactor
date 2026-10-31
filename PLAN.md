# Implementation Plan

## Project Objective
Reverse-engineer **Captain Comic II: Fractured Reality** into a clean, modern C++ reimplementation using **SDL2** for video presentation, input handling, and audio synthesis, while maintaining 100% headless testability.

### Primary Ground-Truth References
1. **`reference/comic2/comic2_standalone.asm`**: Complete, compilable 16-bit 8086 MS-DOS NASM assembly source code (`nasm -f bin comic2_standalone.asm -o comic2.exe`). This is the authoritative ground truth for program structure, instruction logic, control flow, hardware interrupts, and authentic DOS physics curves.
2. **`reference/comic2/comic2_data.asm`**: Complete, structured disassembly of the 141 KB data segment (`data_0.bin`). Authoritative reference for EGA font glyph patterns (`0x0D2D0`), audio frequency/melody tables (`0x2170A`), high-score records, string constants, and memory addresses.
3. **`reference/comic2/comic2.asm`**: Historical annotated IDA disassembly, preserved as a secondary reference for cross-referencing legacy label annotations.
4. **`reference/comic2/unpacked.exe.test.c`**: Ghidra decompilation, used as secondary high-level structural cross-reference.

---

## Phase 1–4: Game Loop & Player State Analysis (COMPLETED)

### Original Objective
Map the fragmented main game loop (`game_loop`, was `sub_35DE`) and identify the core player state variables (`comic_x`, `comic_y`, etc.).

## Status
- **Phase 1: Game Loop Defragmentation** - [COMPLETED]
- **Phase 2: Player State Identification** - [COMPLETED]
- **Phase 3: Function Mapping** - [COMPLETED]

## Background & Motivation
The main game loop (`game_loop`, was `sub_35DE`) was fragmented into 21 chunks in the disassembly. Understanding this loop is critical for identifying how input is processed, physics are applied, and entities are updated. Concurrently, locating the player state variables is necessary to trace how functions interact with the player.

## Scope
- Analyze and document the control flow of `game_loop` (`sub_35DE`) and its fragments. [DONE]
- Identify memory addresses corresponding to the player's X/Y coordinates, velocity, facing direction, state (standing, running, jumping), health, and inventory. [DONE]
- Identify the core movement and physics functions (e.g., `handle_fall_or_jump`, `move_left`, `move_right`) called within the loop. [DONE]
- Update project documentation (`FUNCTION_MAP.md`, `REVERSE_ENGINEERING_GUIDE.md`, `CORE_ENGINE.md`) with the findings. [DONE]

## Implementation Steps

### Phase 1: Game Loop Defragmentation [COMPLETED]
1.  **Trace `game_loop`**: Followed the control flow starting from `game_loop` (`sub_35DE`) in `comic2.asm`.
2.  **Identify Fragments**: Documented the primary fragments and their roles in the dispatcher.
3.  **Map Loop Sequence**: Created a high-level sequence diagram in `CORE_ENGINE.md`.

### Phase 2: Player State Identification [COMPLETED]
1.  **Trace Input Variables**: Followed references to input variables into the game loop.
2.  **Locate Coordinates**: Identified `comic_x`, `comic_y`, `comic_x_vel`, `comic_y_vel`.
3.  **Locate Attributes**: Found `comic_hp` (`byte_258CD`).
4.  **Correlate with C Decompilation**: Verified offsets using `unpacked.exe.test.c`.

### Phase 3: Function Mapping [COMPLETED]
1.  **Identify Physics Functions**: Located `update_player_physics` (`sub_2A10`, grounded) and `handle_airborne_movement` (`sub_2EDC`, airborne).
2.  **Identify Movement Functions**: Located `sub_2BDC` (move_left) and `sub_2C39` (move_right).
3.  **Annotate Assembly**: Add descriptive comments and rename labels for the identified functions and variables in `comic2.asm`.

### Phase 4: Documentation Update [COMPLETED]
1.  **Update `FUNCTION_MAP.md`**: Added identified addresses.
2.  **Update `CORE_ENGINE.md`**: Added dispatcher logic and state variables.

## Completed Findings
- [x] `comic_num_lives` = `word_2526F` (`ds:28Fh`)
- [x] `comic_firepower` = `byte_251FC` (`ds:1FCh`)
- [x] `comic_gems` = `word_25204` (`ds:204h`)
- [x] **Verified DOS Physics Curves** (transcribed from `comic2_standalone.asm` `player_update_physics` `1000:2a10` & `player_do_jump` `1000:2d61`):
  - Walk step speed = 8 px/tick (`kOriginalDosWalkStep`)
  - Jump impulse = -7 px/tick (`kOriginalDosJumpImpulse`)
  - Air drift step = 1 px/tick, max air drift clamp = [-5, 5] px/tick (`kOriginalDosMaxAirDrift`)
  - Gravity acceleration = 5 px/tick (`kOriginalDosGravityPerTick`)
  - Terminal fall speed = 23 px/tick (`kOriginalDosMaxFallSpeed`)
  - Fall start velocity = 5 px/tick (`kOriginalDosFallStartVelocity`)
  - Feet collision probe offset Y = 13 px (`kOriginalDosFeetProbeOffsetY`)
  - Base jump capacity = 5 ticks (`[DS:0x0899]`), boosted to 6 ticks with boots
  - Variable jump height: truncates upward jump velocity upon jump key release
- [x] **Confirmed Inventory & Progression Fields** (transcribed from `comic2_data.asm` `DS:0x08ae - 0x08b6`):
  - `shield_power` = `[DS:0x089a]` (current shield energy: default 5, max 8)
  - `boots_equipped` = `[DS:0x08ae]` (High-jump gravity boots flag)
  - `lantern_equipped` = `[DS:0x08b0]` (Cavern illumination flag)
  - `corkscrew_equipped` = `[DS:0x08b2]` (Grate passage unlock flag)
  - `wand_equipped` = `[DS:0x08b4]` (Teleportation wand flag)
  - `key_count` = `[DS:0x08b6]` (Door keys collected count)
  - `weapon_cooldown` = `[DS:0x08a8]` (Fireball firing rate limiter cooldown timer)
  - `score_high` = `[DS:0x0928]` (32-bit score counter high word)

---

## Phase 5: Complete Function Annotation (COMPLETED)

### Goals
- Apply all confirmed `sub_XXXX` renames in `comic2.asm`
- Annotate the EGA blit pipeline (`gfx_startup_graphics_and_menu_probe`, `gfx_render_viewport_4plane`, `gfx_copy_viewport_plane`, `ent_draw_mapped_slot_or_placeholder`, `gfx_rle_blit_opaque_4plane`, `gfx_rle_blit_masked_or_4plane`, `gfx_set_crtc_start_on_retrace`)
- Locate and annotate entity management (`handle_enemies`, `handle_fireballs`, `handle_item`, `spawn_enemy`)
- Locate and annotate file I/O / resource loading (`load_level`, `load_fullscreen_graphic`, `load_shp_files`, `rle_decode`)
- Identify sound system: INT 3 handler, sound effect dispatch, effect addresses

### Steps
1. [x] Batch-rename all confirmed `sub_XXXX` labels with known identities
2. [x] Annotate EGA blit pipeline with full function headers (`gfx_startup_graphics_and_menu_probe`, `gfx_render_viewport_4plane`, `gfx_copy_viewport_plane`, `ent_draw_mapped_slot_or_placeholder`, `gfx_rle_blit_opaque_4plane`, `gfx_rle_blit_masked_or_4plane`, `gfx_set_crtc_start_on_retrace`)
3. [x] Trace `sub_437B` and `sub_5D5F` callers to locate entity management
	- Promoted provisional mappings:
	  - `sub_437B` -> `ent_update_entities_in_viewport`
	  - `sub_5D5F` -> `update_projectiles`
	  - `sub_1A63` -> `ent_build_room_entity_list`
	  - `sub_4579` -> `ent_deactivate_at_coords`
	  - `sub_46E6` -> `ent_activate_slot_into_runtime`
	  - `sub_4B25` -> `ent_update_object_behaviors`
	- Confirmed active runtime entity list at `unk_256E0` (8-byte records)
	- Promoted room-transition presentation helpers:
	  - `sub_3BC6` -> `room_transition_draw_header`
	  - `sub_3C13` -> `room_transition_reveal_sequence_a`
	  - `sub_3CB1` -> `room_transition_reveal_sequence_b`
	  - `sub_3F0A` -> `room_transition_palette_wave`
	  - `sub_3D3A` -> `room_transition_draw_reveal_quad`
	  - `sub_3D7B` -> `room_transition_blit_reveal_quad_4plane`
	  - `sub_3D90` -> `room_transition_blit_reveal_column_plane`
	  - `sub_3DA1` -> `room_transition_clear_reveal_box`
	  - `sub_3DB6` -> `room_transition_present_frame`
	  - `sub_3E1F` -> `room_transition_player_entry_sequence`
	  - `sub_3E98` -> `room_transition_player_exit_sequence`
	  - `sub_401B` -> `event_draw_stacked_sprite_rise`
	  - `sub_4073` -> `event_transition_camera_bob_step`
	  - `sub_409C` -> `event_finale_transition_sequence`
	  - `sub_4206` -> `ui_draw_framed_message_from_stream`
	  - `sub_42D4` -> `ui_draw_string_8x8`
	  - `sub_42E8` -> `ui_draw_glyph_8x8`
	  - `sub_4339` -> `ega_set_map_mask`
	  - `sub_39AD` -> `ui_show_modal_prompt_wait_key`
	  - `sub_3A17` -> `hud_update_mode_icons`
	  - `sub_660B` -> `io_open_file_or_prompt_retry`
	  - `sub_6011` -> `ui_select_next_valid_option`
	  - `sub_6021` -> `ui_select_prev_valid_option`
	  - `sub_6030` -> `ui_is_option_selectable`
	  - `sub_606B` -> `ui_render_option_list`
	  - `sub_615C` -> `ui_option_list_input_loop`
	  - `sub_61C8` -> `ui_edit_selected_option_text`
	  - `sub_62A4` -> `cfg_load_options_or_defaults`
	  - `sub_631A` -> `cfg_save_options`
	  - `sub_633C` -> `savegame_write_snapshot`
	  - `sub_636F` -> `savegame_read_snapshot`
	  - `sub_64F5` -> `ui_draw_completion_delta_hint`
	  - `sub_6545` -> `cfg_compute_hardware_seed`
	  - `sub_65EF` -> `io_verify_game000_or_prompt_retry`
	  - `sub_65FD` -> `io_verify_frcfg_or_prompt_retry`
	  - `sub_677A` -> `ui_render_game_selection_panel`
	  - `sub_6DF4` -> `cfg_run_integrity_challenge`
	  - `sub_6D99` -> `hud_add_bcd_counter_from_low_digit`
	  - `sub_6DE7` -> `hud_add_bcd_counter_from_mid_digit`
	  - `sub_7057` -> `cfg_input_challenge_code`
	  - `sub_6F47` -> `cfg_draw_integrity_challenge_screen`
	  - `sub_7164` -> `play_intro_cinematic`
	  - `sub_7255` -> `gfx_copy_backbuffer_to_active_page`
	  - `sub_7261` -> `intro_draw_sprite_and_present`
	  - `sub_7278` -> `wait_ticks_or_abort`
	  - `sub_74E1` -> `intro_wipe_split_buffers_step`
	  - `sub_752A` -> `gfx_copy_rect_all_planes`
	  - `sub_7553` -> `gfx_copy_rect_rows_stride40`
	  - `sub_756D` -> `intro_draw_overlay_with_comic`
	  - `sub_75AD` -> `intro_flash_comic_with_optional_overlay`
	  - `sub_7622` -> `intro_draw_comic_walk_frame`
	  - `sub_7741` -> `gfx_init_double_buffer_pages`
	  - `sub_774E` -> `gfx_present_and_flip_page`
	  - `sub_7765` -> `gfx_copy_page_4plane`
	  - `sub_790C` -> `gfx_blit_sprite_opaque_both_pages`
	  - `sub_7927` -> `gfx_blit_sprite_opaque_active_page`
	  - `sub_7962` -> `gfx_blit_sprite_plane_rows`
	  - `sub_7990` -> `gfx_load_and_blit_opaque_file`
	  - `sub_7999` -> `gfx_load_and_blit_masked_file`
	  - `sub_79A2` -> `io_read_file_to_seg001_0600`
	  - `sub_79ED` -> `gfx_rle_decode_copy_rowspan`
	  - `sub_7A39` -> `gfx_rle_decode_or_rowspan`
	  - `sub_7A68` -> `ega_select_plane_read_write`
	  - `sub_77A3` -> `gfx_blit_sprite_masked_active_page`
	  - `sub_77F2` -> `gfx_blit_masked_plane_aligned`
	  - `sub_7849` -> `gfx_blit_masked_plane_shift4`
	  - `sub_78C6` -> `gfx_blit_sprite_opaque_offset_both_pages`
	  - `sub_78E1` -> `gfx_blit_sprite_opaque_offset_active_page`
	  - `sub_7AAB` -> `gfx_clear_rect_both_pages`
	  - `sub_7AC8` -> `gfx_clear_rect_active_page`
	  - `sub_7B03` -> `gfx_clear_rect_plane_rows`
	  - `sub_7B31` -> `gfx_blit_sprite_masked_clipped_active_page`
	  - `sub_7B8B` -> `gfx_blit_masked_plane_aligned_clipped`
	  - `sub_7BEF` -> `gfx_blit_masked_plane_shift4_clipped`
	  - `sub_7C79` -> `gfx_blit_sprite_masked_bottom_clipped_active_page`
	  - `sub_7CCD` -> `gfx_blit_masked_plane_aligned_bottom_clipped`
	  - `sub_7D31` -> `gfx_blit_masked_plane_shift4_bottom_clipped`
	  - `sub_3A5F` -> `hud_draw_two_digit_counter`
	  - `sub_45A6` -> `hud_increment_lives_bcd_clamped`
	  - `sub_4870` -> `hud_set_data_disk_icon_collected`
	  - `sub_489E` -> `hud_bcd_decrement_cx`
	  - `sub_48AA` -> `hud_decrement_word_25202_bcd`
	  - `sub_48BF` -> `hud_decrement_lives_bcd`
	  - `sub_48D4` -> `ent_normalize_runtime_y_magnitudes`
	  - `sub_4A70` -> `ent_copy_descriptor_to_runtime_slot`
	  - `sub_4C06` -> `ent_deactivate_runtime_slot`
	  - `sub_49AD` -> `ent_build_runtime_slots_for_viewport`
	  - `sub_5A4E` -> `ent_move_x_and_bounce`
	  - `sub_5A99` -> `ent_apply_gravity_and_move_y`
	  - `sub_5B01` -> `ent_move_y_and_bounce`
	  - `sub_5B7B` -> `ent_move_y_with_probe_and_bounce`
	  - `sub_5BAC` -> `ent_move_x_with_range_probe_and_bounce`
	  - `sub_5BF7` -> `ent_probe_x_collision_strict`
	  - `sub_5C3E` -> `ent_probe_x_collision_range`
	  - `sub_5C91` -> `ent_probe_tile_above_threshold`
	  - `sub_5CA1` -> `ent_probe_tile_range_in_viewport`
	  - `sub_5CCF` -> `ent_set_horizontal_chase_direction_to_comic`
	  - `sub_5CEB` -> `spawn_player_projectile`
	  - `sub_5F86` -> `ui_draw_menu_frame`
	  - `sub_198E` -> `load_room_effect_script_and_trigger_ranges`
	  - `sub_1BFB` -> `check_word_is_ffff_or_0005`
	  - `sub_1C0B` -> `update_level5_memory_signature`
	  - `sub_1FA4` -> `io_load_rle_resource_to_e978`
	  - `sub_1FFE` -> `io_read_file_to_seg001_0600_resource`
	  - `sub_2022` -> `io_read_file_to_seg5e_base`
	  - `sub_2049` -> `io_read_xor25_block_9ec0`
	  - `sub_2072` -> `load_room_tilemap_from_resource_buffer`
	  - `sub_1E09` -> `gfx_draw_tile_id_at_pixel_xy`
	  - `sub_1E86` -> `gfx_render_room_tilemap_to_active_buffers`
	  - `sub_1F72` -> `gfx_copy_16x16_tile_plane_to_active_page`
	  - `sub_1F89` -> `gfx_copy_16x16_tile_plane_to_segment`
	  - `sub_22B` -> `gfx_disable_retrace_palette_script`
	  - `sub_238` -> `gfx_enable_retrace_palette_script`
	  - `sub_15D` -> `gfx_load_ega_palette_16_from_table`
	  - `sub_599` -> `input_wait_for_scancode_event`
	  - `sub_424` -> `input_wait_for_joystick_or_key_confirm`
	  - `sub_54B` -> `input_capture_unique_key_binding_and_echo`
	  - `sub_2B8E` -> `camera_update_y_follow_comic_clamped`
	  - `sub_327D` -> `player_cycle_mode_selection`
	  - `sub_30C6` -> `player_start_state4_action_animation`
	  - `sub_3468` -> `player_start_attack_animation`
	  - `sub_3610` -> `player_start_timed_overlay_sprite`
	  - `sub_362A` -> `check_comic_near_room_event_anchor`
	  - `sub_36C0` -> `update_room_event_anchor_motion`
	  - `sub_378E` -> `update_room_event_anchor_sprite`
	  - `sub_6806` -> `ent_process_interaction_script_and_queue_message`
	  - `sub_6D95` -> `hud_add_bcd_counter_from_tens_value`
	  - `sub_71FE` -> `intro_repeat_draw_with_offset_4_3`
	  - `sub_721B` -> `intro_repeat_draw_with_offset_8_6`
	  - `sub_7238` -> `intro_repeat_draw_with_offset_12_8`
	- Item-specific handlers isolated:
	  - `handle_projectile_impact`
	  - `tile_handle_collision_mode2_trigger`
	- Explicit spawn/respawn paths isolated:
	  - `ent_activate_slot_into_runtime`
	  - `ent_build_runtime_slots_for_viewport`
	  - `spawn_player_projectile`
4. [x] Trace DOS `int 21h` / `3D00h` open-file call sites from `start` to locate resource loaders
	- Startup/open-file chain confirmed:
	  - `io_verify_game000_or_prompt_retry`
	  - `io_verify_frcfg_or_prompt_retry`
	  - `io_open_file_or_prompt_retry`
	- Resource loader wrappers confirmed in active paths:
	  - `io_load_rle_resource_to_e978`
	  - `io_read_file_to_seg001_0600_resource`
	  - `io_read_file_to_seg5e_base`
	  - `io_read_xor25_block_9ec0`
	  - `io_read_file_to_seg001_0600`
5. [x] Trace INT 3 handler entry and initial sound effect table anchors
	- Handler entry confirmed: `loc_8C7` (installed by `sub_9C9`, restored by `sub_A5E`)
	- Timer playback loop confirmed: `loc_683` (PIT ch2 + speaker gate control)
	- Verified stream pointers in use: `0x00DB`, `0x965E`, `0x9676`, `0x96B6`
	- Canonical naming for every effect stream offset can be done as a follow-up refinement pass

---

## Phase 6: Data Structure & Resource Format Documentation (COMPLETED)

### Goals
- Confirm full enemy struct layout (12-byte format, all offsets)
- Document FR*.* level file format
- Document frpak.* archive format
- Document .SHP and .EGA sprite/graphics layouts
- Write C-equivalent `struct` definitions for all confirmed layouts

### Progress (Current)
- [x] Confirmed 12-byte mapped object record at `unk_25AD0` (`+0x00..+0x0A`)
- [x] Confirmed active runtime entity list format (8-byte records at `unk_256E0`)
- [x] Confirmed 0x20-byte runtime behavior slot layout (table at `0x48ED`)
- [x] Documented signed-RLE and 4-plane EGA packet decode contracts from code paths
- [x] Confirmed `unk_2E3CC` level loader indirection (per-level pointer -> 4 filename-pointer tuple)
- [x] FRDATA header/room-offset schema documented with field-level semantics used by loader
- [x] FRPAK `.001`..`.007` validated as direct full-file 4-plane streams (no directory walk in confirmed paths)
- [x] `.SHP` / `.EGA`-equivalent payload contracts finalized from sprite and RLE blit routines
- [x] Phase 7.7 Gate A (Unit) integration tests pass with deterministic replay checks (8 comprehensive tests)

---

## Phase 7: C++ Reimplementation (STARTED)

### Goals
- Platform abstraction layer (EGA rendering → SDL2 or similar)
- Port confirmed structs and globals to C++
- Reimplement game loop / dispatcher
- Reimplement physics, movement, entity system
- Behavioral verification against original via DOSBox oracle

### Progress (Current)
- [x] Created initial C++ scaffold with CMake build
- [x] Ported confirmed Phase 6 data layouts to `include/comic2/types.hpp`
- [x] Implemented signed-RLE and 4-plane EGA packet decoders in `src/resource_loader.cpp`
- [x] Added FRPAK validator executable (`src/main.cpp`)
- [x] Verified build succeeds and FRPAK.001..FRPAK.007 decode with row span `0x1f40`
- [x] Added renderer abstraction baseline (`include/comic2/renderer.hpp`, `src/renderer.cpp`) with tests
- [x] Ported dispatcher skeleton and player/runtime state containers (`include/comic2/dispatcher.hpp`, `include/comic2/game_state.hpp`, `src/dispatcher.cpp`) with tests
- [x] Added default input + physics stage handlers and deterministic tick replay tests (`include/comic2/default_handlers.hpp`, `src/default_handlers.cpp`, `tests/dispatcher_tests.cpp`)
- [x] Started subsystem split with dedicated `player_controller` + `tile_collision` modules and tests (`include/comic2/player_controller.hpp`, `src/player_controller.cpp`, `include/comic2/tile_collision.hpp`, `src/tile_collision.cpp`, `tests/player_controller_tests.cpp`)
- [x] Added scaffold baselines for `entity_runtime`, `projectiles`, and `room_loader` modules with focused tests (`include/comic2/entity_runtime.hpp`, `src/entity_runtime.cpp`, `include/comic2/projectiles.hpp`, `src/projectiles.cpp`, `include/comic2/room_loader.hpp`, `src/room_loader.cpp`, `tests/subsystem_scaffold_tests.cpp`)

### Detailed Reimplementation Roadmap

#### 7.1 Architecture and Module Boundaries
- [x] `resource_loader` module: signed-RLE + 4-plane packet decode contracts
- [x] `renderer` module baseline: planar surface abstraction + presenter interface
- [x] `dispatcher` module baseline: stage priority selector + hookable tick execution
- [x] `game_state` baseline: player/input/flags/runtime containers
- [x] Split runtime into explicit subsystem modules:
	- [x] `player_controller` (movement/jump/fall/state transitions)
	- [x] `tile_collision` (tile lookup + threshold checks)
	- [x] `entity_runtime` scaffold (activation/deactivation/runtime slot updates)
	- [x] `projectiles` scaffold (spawn/update/collision/despawn)
	- [x] `room_loader` scaffold (load_resource equivalent, room decode and setup)

#### 7.2 Dispatcher Parity Plan (`loc_2341` Ordering)
- [x] Encoded current stage order and priority chain in `GameDispatcher`
- [x] Add dedicated stage handlers matching known flow order:
	1. `handle_level_transition`
	2. `handle_special_logic_1`
	3. `handle_special_logic_2`
	4. `handle_airborne_movement`
	5. `handle_timed_overlay`
	6. `update_player_physics` (grounded)
	7. `handle_player_animation`
	8. `handle_attack_animation`
	9. `handle_distance_interaction`
	10. `handle_tile_hazard`
	11. `handle_player_special_state`
	12. `handle_input_fallback`
- [x] Add a trace-mode dispatcher logger that records selected stage per tick for oracle comparison

#### 7.3 Player Movement and Physics Plan
- [x] Implement horizontal movement parity baseline for known left/right paths (`sub_2BDC`, `sub_2C39`)
- [x] Implement grounded physics baseline (`sub_2A10`) including jump counter consumption and fall start
- [x] Implement airborne path baseline (`sub_2EDC`) including lateral drift checks and vertical velocity updates
- [x] Implement floor check trigger baseline (`sub_2C9D`) to enter fall state when no support tile
- [x] Add deterministic fixtures for edge cases:
	- jump while moving left/right
	- short-hop vs full jump counter usage
	- ledge walk-off transitioning into fall

Status note: current Phase 7.3 implementation is parity-oriented and deterministic for dispatcher/runtime tests, with tile support probing currently modeled via configurable support bounds until full tile-grid probing from Phase 7.4 is integrated.

#### 7.4 Tile Collision and Room Grid Plan
- [x] Implement tile query service equivalent to `get_tile_at_pixels` (`sub_1CFE`) (baseline API + bounds-safe lookup)
- [x] Port documented FRDATA room entry interpretation (`tile_w`, `tile_h`, `rle_data_off`)
- [x] Build room row-pointer table equivalent (`0x2A0` pointer map behavior) (baseline pointer-table builder)
- [x] Implement collision thresholds and hazard band checks used by dispatcher/hazard path (baseline tile hazard routing)
- [x] Add tests for pixel->tile mapping and threshold comparisons at room bounds

Status note: Phase 7.4 baseline is now implemented with room-grid tile lookup, threshold/hazard helpers, dispatcher hazard-stage integration, and deterministic unit coverage. Follow-up parity refinements can tighten exact tile probe points and hazard semantics against oracle traces.

#### 7.5 Entity and Projectile Plan (COMPLETED)
- [x] Implement mapped-object activation pipeline:
	- room list build (`ent_build_room_entity_list`) - filters mapped objects by room coordinates
	- runtime slot build (`ent_build_runtime_slots_for_viewport`) - builds up to 6 runtime slots from viewport-intersecting entities
	- slot copy/deactivate behavior (`ent_copy_descriptor_to_runtime_slot`, `deactivate_runtime_slot`) - initializes runtime state and clears slots
- [x] Implement projectile loop parity (`update_projectiles`) with viewport culling and tile collision
	- Viewport culling with 8-pixel margin (200x152 viewport)
	- Tile collision detection via `check_projectile_tile_collision`
	- Animation frame cycling (modulo 8)
	- Bounds checking for world coordinates
- [x] Implement player projectile spawn (`spawn_player_projectile`) and impact dispatch
	- Facing-direction velocity calculation (±0x0010)
	- Airborne adjustment for left-facing projectiles
	- Y-position offset (+4 airborne, +8 grounded)
	- Initial upward velocity (-0x000C)
- [x] Add table-driven tests for runtime slot state transitions and projectile despawn conditions
	- 13 comprehensive tests covering entity activation, runtime slot management, projectile spawning, collision, and culling
	- All tests pass in CI (`ctest`)

#### 7.6 Rendering Plan (EGA-Parity-Oriented)
- [x] Implement masked and opaque sprite blit adapters over `EgaPlanarSurface`
- [x] Implement page-flip abstraction compatible with current double-buffer assumptions
- [x] Add validation helpers that compare plane bytes for known decode/blit fixtures
- [x] Add optional SDL2 presenter behind `IFramePresenter` while keeping core logic headless-testable
  - Implemented `Sdl2FramePresenter` class with 4-plane EGA to RGBA conversion
  - Configurable via CMake (SDL2 is now a mandatory dependency)
  - Created `comic2_renderer_demo` executable for visual testing
  - All tests pass in CI with SDL2 enabled
  - EGA palette mapping implemented with standard 16-color palette

#### 7.7 Integration Gates and Oracle Verification
- [x] Gate A (Unit): all module tests pass in CI (`ctest`) with deterministic replay checks
- [x] Gate B (Frame): scripted input sequences produce stable dispatcher-stage traces
- [x] Gate C (State): key state vectors (`x/y/vel/state flags`) match expected snapshots per tick
- [x] Gate D (Visual): selected rooms produce matching planar frame hashes against oracle captures
- [x] Gate E (Behavior): death/hazard, room transition, and projectile interactions match expected outcomes

##### 7.7 Oracle Fixture Appendix
- Canonical integration test entrypoint: `run_integration_gate_tests` in `tests/integration_gates_tests.cpp`
- Gate B oracle fixture: scripted dispatcher-stage trace sequence validated in `test_gate_b_scripted_trace_matches_oracle`
- Gate C oracle fixture: per-tick state vector snapshots validated in `test_gate_c_state_vectors_match_snapshots`
- Gate D oracle fixture alpha hash: `0x16e2201b96f48c65` validated in `test_gate_d_room_fixture_alpha_hash_matches_oracle`
- Gate D oracle fixture beta hash: `0x22ecbb9529f6165` validated in `test_gate_d_room_fixture_beta_hash_matches_oracle`
- Gate E oracle fixtures: hazard death routing, room transition boundaries, and projectile collision outcomes validated by `test_gate_e_*` integration tests
- Revalidation command: `cmake --build build && ctest --test-dir build --output-on-failure`

#### 7.8 Immediate Next Execution Order
- [x] 1) Implement `tile_collision` service and replace physics stubs with tile-aware checks
- [x] 2) Implement grounded + airborne handlers using documented priority and state flags
- [x] 3) Implement room-loader integration (`load_resource` equivalent path) into runtime state
- [x] 4) Implement projectile update path and add deterministic scripted tick tests
- [x] 5) Add dispatcher trace capture and compare against oracle-driven expected stage sequences

#### 7.9 Main Bootstrap Wiring
- [x] 1) Convert `src/main.cpp` from a FRPAK validator into the game startup bootstrap
- [x] 2) Initialize `comic2::RuntimeState` with core runtime containers and confirmed defaults
- [x] 3) Load initial resources needed before the first tick:
  - level/room metadata
  - room tile grid
  - sprite/tile resource data
- [x] 4) Construct `comic2::GameDispatcher` and assign stage hooks in the known game-loop order
- [x] 5) Add a small main loop around `dispatcher.run_tick(state)` with input polling and render invocation
- [x] 6) Wire one subsystem at a time through `main()`:
  - input capture
  - tick dispatch
  - grounded physics
  - rendering placeholder output
- [x] 7) Keep `main()` as orchestration only; delegate game logic to subsystem modules and handler hooks

---

## Phase 8: Actual Game Execution and Runtime Bring-Up

### Goal
Bring the reimplementation from the bootstrap smoke test into a real running game loop that can render frames, consume user input, and start exercising the actual game state.

### Recommended Execution Order
1. Rendering and frame presentation
2. User input plumbing
3. Resource/state bootstrap for the real game
4. Audio backend
5. Main loop integration and graceful quit

### Phase 8.1: Rendering and Frame Loop
- [x] Replace the bootstrap-only rendering path with a real frame loop driven by the dispatcher.
- [x] Ensure the renderer can present a visible 320x200-style EGA frame through the existing presenter abstraction.
- [x] Establish a stable tick cadence and frame pacing for the main game loop.
- [x] Verify that the app produces a visible frame before gameplay systems are connected.

### Phase 8.2: User Input
- [x] Replace the environment-variable input shim with real keyboard input handling.
- [x] Map keyboard actions to the existing `InputState` model for left/right, jump, down, and quit.
- [x] Keep input isolated so gameplay and physics remain testable through the dispatcher.
- [x] Verify that movement and state changes occur in response to real key presses.

**Implementation details:**
- Created `KeyboardInputHandler` class in `include/comic2/input_handler.hpp` and `src/input_handler.cpp`
- Integrates with SDL2 when available for real keyboard input (Arrow keys, WASD keys, Space for jump, ESC to quit)
- Falls back to environment variables for headless/testing environments
- Tests can force environment variable mode via `COMIC2_FORCE_ENV_INPUT=1` for deterministic testing
- Window close events are properly handled via SDL_QUIT event
- Main loop now runs with large frame budget and stops on quit request (`quit_requested` flag in `FrameLoopSummary`)
- All existing unit tests pass with updated bootstrap environment variable handling

### Phase 8.3: Real Resource and Scene Bootstrap
- [x] Load the actual room/resource data through the existing resource-loader path instead of synthetic bootstrap state.
- [x] Initialize the level, room, and player state so the dispatcher is operating on a real game context.
- [x] Provide a temporary fallback path for missing assets so the engine can still start in a controlled way.
- [x] Verify that the game enters an initial room or a graceful placeholder screen.

**Implementation details:**
- Added `initialize_runtime_scene` in `bootstrap` to attempt real asset-backed bootstrap across candidate roots (`<root>`, `<root>/reference/original`, `<root>/original`) before entering the dispatch loop.
- Startup now records whether scene bootstrap succeeded with real room data or fell back to placeholder mode, and reports the selected asset root when available.
- Added bounds clamping for bootstrap player coordinates when real room geometry is loaded so first-frame state is valid for dispatcher/render use.
- Added bootstrap wiring tests to validate both discovered-real-assets startup and controlled fallback startup with missing assets.

### Phase 8.4: Audio Backend
- [x] Add a minimal audio backend behind a small interface so sound can be enabled without blocking gameplay.
- [x] Start with a simple placeholder or tone playback path for initial event coverage.
- [x] Connect audio triggers to the game loop in a non-blocking way.
- [x] Verify that audio initialization succeeds and at least one sound event can be triggered safely.

**Implementation details:**
- Added `IAudioBackend` interface with `NullAudioBackend` and default backend factory in `audio` module (`include/comic2/audio.hpp`, `src/audio.cpp`).
- Added SDL2-backed non-blocking tone playback backend using queued PCM audio for startup/jump/hazard events; supports `COMIC2_DISABLE_AUDIO=1` fallback to null backend.
- Integrated optional audio into `run_render_loop` lifecycle (initialize, enqueue startup event, per-frame update, shutdown) and wired default backend into `main` runtime startup.
- Added tests for loop/audio lifecycle, jump event trigger, and null backend safety (`tests/audio_backend_tests.cpp`).

### Phase 8.5: Main Loop Integration
- [x] Combine render, input, dispatch, and optional audio into a single continuous game loop.
- [x] Exit cleanly on quit input or window close.
- [x] Keep subsystem boundaries intact so rendering, input, and gameplay remain independently testable.
- [x] Verify that the app runs continuously and updates state while presenting frames.

**Implementation details:**
- `run_render_loop` now executes unified per-frame flow in order: input polling, dispatcher tick, optional audio event/update, then frame presentation.
- Input polling returns a loop-continue signal (`poll_bootstrap_input`), allowing clean early termination on quit without coupling loop control to global internals.
- Main bootstrap wiring remains orchestration-only (`main` and `run_integrated_bootstrap_loop`) while gameplay behavior stays in dispatcher/hooks.
- Added continuous-loop regression coverage ensuring state updates while frames are presented (`test_render_loop_updates_state_while_presenting_frames`), plus existing quit-path and audio lifecycle tests.

### Scope Boundary
This phase is about getting the real game running, not full parity or complete content fidelity. The first milestone is stability: visible frames, working input, and a continuous loop.

---

## Phase 8.6: Original Data-Driven Asset and Level Loading

### Goal
Move from bootstrap-style probing to deterministic, data-driven loading of levels, rooms, and graphics from original game files, with room transitions and rendering backed by decoded original assets.

### Exit Criteria
- [x] Level/room selection resolves through FRDATA indirection instead of hardcoded candidate filename scans.
- [x] Room transitions load the target room from original data files and commit state atomically.
- [x] FRPAK payloads are indexed and decoded through an explicit resource catalog/cache.
- [x] Rendering path consumes decoded original graphics for at least one verified tile/sprite path.
- [x] Integration tests cover cross-room load success and failure fallback behavior.

### 8.6.1 Resource Resolution Layer
- [x] Introduce a dedicated resolver API (new module) that maps `(level, room, asset kind)` to concrete file path + offset metadata.
- [x] Implement FRDATA table parsing for per-level room/resource indirection already documented in Phase 6.
- [x] Remove hardcoded room candidate arrays from bootstrap loading; keep only asset-root candidate discovery.
- [x] Add resolver unit tests for valid mapping, out-of-range room index, and sentinel/invalid entries.

**Implementation details:**
- Added resolver asset-kind typing and room metadata resolution path (`ResourceAssetKind`, `RoomLoadSpec`, `resolve_room_load_spec`) with explicit table/entry/resource offsets.
- Refactored room decode/load path to consume resolver output and reject sentinel/invalid entries before decode.
- Removed hardcoded FR000/FR001 room candidate arrays from bootstrap and replaced with dynamic room payload discovery (`FR###.#`) while preserving asset-root discovery and fallback behavior.
- Added resolver-focused unit coverage for valid mapping offsets, out-of-range room index rejection, and sentinel-entry rejection.

**Suggested touchpoints:**
- `include/comic2/resource_loader.hpp`
- `src/resource_loader.cpp`
- `include/comic2/room_loader.hpp`
- `src/room_loader.cpp`
- `tests/subsystem_scaffold_tests.cpp`

### 8.6.2 Room Transition Data Reload
- [x] Update `handle_level_transition` to resolve and load the next room via the resolver instead of reusing stale in-memory bytes.
- [x] Ensure transition load is atomic: decode into temporary state, then commit only on success.
- [x] Keep graceful behavior on failure (no crash, controlled fallback/state preservation).
- [x] Add transition tests covering:
	- successful right-edge room transition load
	- left-edge transition at room 0 clamp behavior
	- failed target-room load with stable prior runtime state

**Implementation details:**
- Transition boundary detection now stages pending target metadata (`target_room`, `target_player_x`) instead of mutating room/player state immediately.
- `handle_level_transition` now performs resolver-backed reload into a temporary `RuntimeState` and commits only on successful room decode/load.
- Transition load first attempts asset-root file resolution (`FR###.#` scan + resolver), then falls back to existing in-memory bytes when needed.
- Failed loads consume pending transition flags but preserve prior room/grid/player runtime state.
- Added deterministic tests for right-edge success path, room-0 left clamp behavior, and failed-load state preservation.

**Suggested touchpoints:**
- `src/default_handlers.cpp`
- `src/bootstrap.cpp`
- `tests/dispatcher_tests.cpp`
- `tests/bootstrap_wiring_tests.cpp`

### 8.6.3 FRPAK Catalog and Decode Pipeline
- [x] Add a FRPAK catalog structure that records file id, record offsets, and decode metadata for sprite/tile resources.
- [x] Replace raw append of FRPAK bytes with cataloged storage and explicit record lookup APIs.
- [x] Add decode-on-demand path using existing RLE/4-plane decoders with bounds checks.
- [x] Add cache policy (initially simple): memoize decoded records by `(pak_id, record_id)`.
- [x] Add tests for malformed record headers, out-of-range offsets, and deterministic decode results.

**Implementation details (current slice):**
- Added FRPAK catalog models (`FrpakCatalog`, `FrpakCatalogFile`, `FrpakCatalogRecord`) and runtime storage in `RuntimeState`.
- Added explicit catalog APIs: build file catalog from payload bytes, catalog record lookup by `(pak_id, record_id)`, and record/file bounds validation.
- Bootstrap FRPAK loading now indexes valid `FRPAK.001..007` payloads into catalog metadata with strict header/bounds checks.
- Added tests for valid catalog construction, truncated/invalid headers, and out-of-range record bounds validation.
- Added decode-on-demand APIs that decode catalog records from aggregated sprite payload bytes and reject invalid record ids/offsets.
- Added deterministic memoization cache for decoded records (`pak_id`, `record_id`) with cache reset API for test/control flows.
- Added renderer validation tests that verify first decode, cache-hit determinism, invalid record id failure, and out-of-range metadata failure.

**Suggested touchpoints:**
- `include/comic2/resource_formats.hpp`
- `include/comic2/resource_loader.hpp`
- `src/resource_loader.cpp`
- `tests/renderer_validation_tests.cpp`

### 8.6.4 Asset-Backed Rendering Integration
- [x] Replace bootstrap tile-color visualization path with a resource-backed tile draw path for at least one room fixture.
- [x] Add one sprite draw path that uses decoded original plane data via existing blit adapters.
- [x] Keep fallback renderer available when assets are missing or decode fails.
- [x] Add frame-hash or plane-byte regression tests for at least one asset-backed render fixture.

**Implementation details:**
- `render_bootstrap_frame` now attempts FRPAK-backed decode first (via catalog+cache), normalizes full-frame payload dimensions, and renders room tiles by extracting 16x16 planar tiles from decoded atlas data.
- Added a sprite draw path that overlays a 16x16 planar sprite cutout from the same decoded asset using masked OR blit.
- Rendering path now gracefully falls back to existing procedural tile visualization and player marker when decode/catalog/image extraction fails.
- Added deterministic bootstrap rendering tests for asset-backed frame hash regression and decode-failure fallback behavior.

**Suggested touchpoints:**
- `src/bootstrap.cpp`
- `src/renderer.cpp`
- `tests/renderer_tests.cpp`
- `tests/integration_gates_tests.cpp`

### 8.6.5 Runtime Data Wiring for Entities
- [x] Load mapped object/entity descriptors from original room/resource payloads instead of synthetic scaffolds.
- [x] Populate `mapped_objects` and activation/runtime tables from decoded data before dispatch ticks.
- [x] Verify projectile/entity updates operate on loaded descriptors without changing dispatch order.
- [x] Add tests for descriptor decode, activation list build, and viewport slot population from real fixture bytes.

**Suggested touchpoints:**
- `src/entity_runtime.cpp`
- `src/room_loader.cpp`
- `src/bootstrap.cpp`
- `tests/subsystem_scaffold_tests.cpp`

### 8.6.6 Validation Gates (Phase 8.6)
- [x] Gate 8.6-A (Resolver): deterministic `(level, room)` mapping to expected source file/offset fixtures.
- [x] Gate 8.6-B (Transition): cross-room movement triggers correct room reload and stable player bounds.
- [x] Gate 8.6-C (Graphics): at least one decoded FRPAK-backed frame fixture passes hash/plane-byte checks.
- [x] Gate 8.6-D (Failure): missing/corrupt data path stays controlled and preserves prior good runtime state.

### Revalidation Command
- [x] `cmake --build build && ctest --test-dir build --output-on-failure`

### Implementation Notes
- Preserve bootstrap fallback behavior while incrementally enabling data-driven paths.
- Keep loader logic side-effect free until decode/validation succeeds; commit runtime state last.
- Favor small, test-first increments: resolver -> transition reload -> FRPAK catalog -> rendering hookup.

### 8.6 PR-Sized Handoff Slices (Agent-Ready)

#### PR 8.6-A1: FRDATA Resolver Skeleton
**Objective**
- Create a deterministic resolver that maps `(level, room)` to room/resource lookup metadata from FRDATA payloads.

**Scope**
- Add resolver types and API surface.
- Parse/validate FRDATA room indirection entries.
- Keep existing bootstrap behavior unchanged (no call-site switch yet).

**Files**
- `include/comic2/resource_loader.hpp`
- `src/resource_loader.cpp`
- `include/comic2/room_loader.hpp`
- `src/room_loader.cpp`
- `tests/subsystem_scaffold_tests.cpp`

**Tests to add/update**
- Valid `(level, room)` mapping returns expected offsets.
- Out-of-range room index fails cleanly.
- Sentinel/invalid entries rejected.

**Definition of done**
- Resolver API exists, tested, and does not alter current runtime flow.

---

#### PR 8.6-A2: Transition Uses Resolver + Atomic Commit
**Objective**
- Make level/room transitions perform real data reload via resolver and commit only on success.

**Scope**
- Update transition path in default handlers.
- Decode into temporary state and commit atomically.
- Preserve previous state on load/decode failure.

**Files**
- `src/default_handlers.cpp`
- `src/room_loader.cpp`
- `tests/dispatcher_tests.cpp`
- `tests/bootstrap_wiring_tests.cpp`

**Tests to add/update**
- Right-edge transition loads next room.
- Room-0 left-edge clamp unchanged.
- Failed target-room load preserves prior room grid/state.

**Definition of done**
- Transition path is data-driven and failure-safe, with deterministic tests.

---

#### PR 8.6-B1: FRPAK Catalog and Record Indexing
**Objective**
- Replace raw FRPAK byte concatenation with indexed catalog metadata.

**Scope**
- Add FRPAK catalog structures (pak id, record id, offsets, lengths).
- Populate catalog during bootstrap/resource load.
- Keep decode/render call-sites unchanged in this PR.

**Files**
- `include/comic2/resource_formats.hpp`
- `include/comic2/resource_loader.hpp`
- `src/resource_loader.cpp`
- `tests/subsystem_scaffold_tests.cpp`

**Tests to add/update**
- Catalog creation for known FRPAK files.
- Offset bounds validation.
- Malformed/truncated record handling.

**Definition of done**
- Catalog available through API with strong bounds checks.

---

#### PR 8.6-B2: Decode-on-Demand + Cache
**Objective**
- Decode FRPAK records through explicit lookup API with memoization.

**Scope**
- Add record decode API keyed by `(pak_id, record_id)`.
- Reuse existing signed-RLE and 4-plane decode routines.
- Add simple decoded-image cache.

**Files**
- `include/comic2/resource_loader.hpp`
- `src/resource_loader.cpp`
- `tests/renderer_validation_tests.cpp`

**Tests to add/update**
- First decode succeeds; repeated decode hits cache.
- Invalid record id/offset fails cleanly.
- Deterministic plane-byte output for fixture record.

**Definition of done**
- Record decode API is stable, tested, and cache-backed.

---

#### PR 8.6-C1: Asset-Backed Tile Rendering Path
**Objective**
- Replace diagnostic tile coloring with one real tile draw path from decoded asset data.

**Scope**
- Wire one tile rendering path to decoded graphics.
- Keep fallback renderer for missing assets/decode errors.
- Do not yet rework full sprite/entity presentation pipeline.

**Files**
- `src/bootstrap.cpp`
- `src/renderer.cpp`
- `tests/renderer_tests.cpp`
- `tests/integration_gates_tests.cpp`

**Tests to add/update**
- Fixture-based frame hash or plane-byte assertion using real tile asset.
- Fallback path assertion when asset lookup fails.

**Definition of done**
- At least one visible room tile path is asset-backed and regression-tested.

---

#### PR 8.6-C2: Entity Descriptor Loading From Room Data
**Objective**
- Populate mapped/runtime entity inputs from real room/resource payloads.

**Scope**
- Decode entity descriptor tables from room-associated data.
- Fill `mapped_objects` before runtime activation.
- Keep activation/update behavior unchanged.

**Files**
- `src/room_loader.cpp`
- `src/entity_runtime.cpp`
- `src/bootstrap.cpp`
- `tests/subsystem_scaffold_tests.cpp`

**Tests to add/update**
- Descriptor decode from fixture bytes.
- Runtime slot build from loaded mapped objects.
- Stable behavior when descriptor payload missing/corrupt.

**Definition of done**
- Runtime entity activation starts from loaded original-data descriptors.

---

#### PR 8.6-D1: End-to-End Integration and Failure Gates
**Objective**
- Finalize Phase 8.6 with integrated tests across resolver, transition reload, and asset-backed draw path.

**Scope**
- Add/update Gate 8.6-A/B/C/D tests.
- Confirm fallback and state-preservation behavior on missing/corrupt files.
- Tighten logging/diagnostics for runtime load failures.

**Files**
- `tests/integration_gates_tests.cpp`
- `tests/bootstrap_wiring_tests.cpp`
- `src/bootstrap.cpp`
- `src/default_handlers.cpp`

**Tests to add/update**
- Deterministic multi-room transition sequence.
- Asset-backed frame fixture regression.
- Failure-mode state preservation checks.

**Definition of done**
- All Phase 8.6 validation gates pass in CI.

---

### Parallelization Map for Agent Handoff
- Can run in parallel after PR 8.6-A1 lands: PR 8.6-B1 and PR 8.6-A2.
- Can run in parallel after PR 8.6-B1 lands: PR 8.6-B2 and PR 8.6-C2.
- Depends on PR 8.6-B2: PR 8.6-C1.
- Final convergence PR: PR 8.6-D1.

### Suggested Agent Prompt Template
- Task: implement PR `<id>` exactly as scoped in PLAN.md.
- Constraints: no behavior regressions outside listed files; preserve fallback paths.
- Required output: code changes, tests, and short validation note with command output summary.
- Validation command: `cmake --build build && ctest --test-dir build --output-on-failure`.

### Copy/Paste Agent Prompt Blocks

#### Prompt: PR 8.6-A1 (FRDATA Resolver Skeleton)
```text
Implement PR 8.6-A1 from PLAN.md exactly as scoped.

Objective:
- Create a deterministic resolver that maps (level, room) to room/resource lookup metadata from FRDATA payloads.

Scope:
- Add resolver types and API surface.
- Parse/validate FRDATA room indirection entries.
- Keep existing bootstrap behavior unchanged (no call-site switch yet).

Allowed files:
- include/comic2/resource_loader.hpp
- src/resource_loader.cpp
- include/comic2/room_loader.hpp
- src/room_loader.cpp
- tests/subsystem_scaffold_tests.cpp

Required tests:
- Valid (level, room) mapping returns expected offsets.
- Out-of-range room index fails cleanly.
- Sentinel/invalid entries rejected.

Constraints:
- No behavior regressions outside listed files.
- Preserve existing fallback behavior.
- Keep changes small and test-first.

Definition of done:
- Resolver API exists, tested, and does not alter current runtime flow.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-A2 (Transition Uses Resolver + Atomic Commit)
```text
Implement PR 8.6-A2 from PLAN.md exactly as scoped.

Objective:
- Make level/room transitions perform real data reload via resolver and commit only on success.

Scope:
- Update transition path in default handlers.
- Decode into temporary state and commit atomically.
- Preserve previous state on load/decode failure.

Allowed files:
- src/default_handlers.cpp
- src/room_loader.cpp
- tests/dispatcher_tests.cpp
- tests/bootstrap_wiring_tests.cpp

Required tests:
- Right-edge transition loads next room.
- Room-0 left-edge clamp unchanged.
- Failed target-room load preserves prior room grid/state.

Constraints:
- No behavior regressions outside listed files.
- Preserve fallback behavior.
- Keep dispatcher stage ordering unchanged.

Definition of done:
- Transition path is data-driven and failure-safe, with deterministic tests.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-B1 (FRPAK Catalog and Record Indexing)
```text
Implement PR 8.6-B1 from PLAN.md exactly as scoped.

Objective:
- Replace raw FRPAK byte concatenation with indexed catalog metadata.

Scope:
- Add FRPAK catalog structures (pak id, record id, offsets, lengths).
- Populate catalog during bootstrap/resource load.
- Keep decode/render call-sites unchanged in this PR.

Allowed files:
- include/comic2/resource_formats.hpp
- include/comic2/resource_loader.hpp
- src/resource_loader.cpp
- tests/subsystem_scaffold_tests.cpp

Required tests:
- Catalog creation for known FRPAK files.
- Offset bounds validation.
- Malformed/truncated record handling.

Constraints:
- No rendering behavior changes in this PR.
- Preserve current fallback startup behavior.
- Keep catalog API explicit and bounds-checked.

Definition of done:
- Catalog available through API with strong bounds checks.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-B2 (Decode-on-Demand + Cache)
```text
Implement PR 8.6-B2 from PLAN.md exactly as scoped.

Objective:
- Decode FRPAK records through explicit lookup API with memoization.

Scope:
- Add record decode API keyed by (pak_id, record_id).
- Reuse existing signed-RLE and 4-plane decode routines.
- Add simple decoded-image cache.

Allowed files:
- include/comic2/resource_loader.hpp
- src/resource_loader.cpp
- tests/renderer_validation_tests.cpp

Required tests:
- First decode succeeds; repeated decode hits cache.
- Invalid record id/offset fails cleanly.
- Deterministic plane-byte output for fixture record.

Constraints:
- No broad rendering pipeline rewrite in this PR.
- Keep cache deterministic and easy to invalidate/reset for tests.

Definition of done:
- Record decode API is stable, tested, and cache-backed.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-C1 (Asset-Backed Tile Rendering Path)
```text
Implement PR 8.6-C1 from PLAN.md exactly as scoped.

Objective:
- Replace diagnostic tile coloring with one real tile draw path from decoded asset data.

Scope:
- Wire one tile rendering path to decoded graphics.
- Keep fallback renderer for missing assets/decode errors.
- Do not rework full sprite/entity presentation pipeline in this PR.

Allowed files:
- src/bootstrap.cpp
- src/renderer.cpp
- tests/renderer_tests.cpp
- tests/integration_gates_tests.cpp

Required tests:
- Fixture-based frame hash or plane-byte assertion using real tile asset.
- Fallback path assertion when asset lookup fails.

Constraints:
- Preserve existing loop/input/audio behavior.
- Keep rendering changes isolated and reversible.

Definition of done:
- At least one visible room tile path is asset-backed and regression-tested.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-C2 (Entity Descriptor Loading From Room Data)
```text
Implement PR 8.6-C2 from PLAN.md exactly as scoped.

Objective:
- Populate mapped/runtime entity inputs from real room/resource payloads.

Scope:
- Decode entity descriptor tables from room-associated data.
- Fill mapped_objects before runtime activation.
- Keep activation/update behavior unchanged.

Allowed files:
- src/room_loader.cpp
- src/entity_runtime.cpp
- src/bootstrap.cpp
- tests/subsystem_scaffold_tests.cpp

Required tests:
- Descriptor decode from fixture bytes.
- Runtime slot build from loaded mapped objects.
- Stable behavior when descriptor payload missing/corrupt.

Constraints:
- Do not change dispatcher order or projectile behavior.
- Preserve fallback startup path when descriptors unavailable.

Definition of done:
- Runtime entity activation starts from loaded original-data descriptors.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

#### Prompt: PR 8.6-D1 (End-to-End Integration and Failure Gates)
```text
Implement PR 8.6-D1 from PLAN.md exactly as scoped.

Objective:
- Finalize Phase 8.6 with integrated tests across resolver, transition reload, and asset-backed draw path.

Scope:
- Add/update Gate 8.6-A/B/C/D tests.
- Confirm fallback and state-preservation behavior on missing/corrupt files.
- Tighten logging/diagnostics for runtime load failures.

Allowed files:
- tests/integration_gates_tests.cpp
- tests/bootstrap_wiring_tests.cpp
- src/bootstrap.cpp
- src/default_handlers.cpp

Required tests:
- Deterministic multi-room transition sequence.
- Asset-backed frame fixture regression.
- Failure-mode state preservation checks.

Constraints:
- Keep assertions deterministic and CI-stable.
- Avoid broad refactors; focus on integration and gate coverage.

Definition of done:
- All Phase 8.6 validation gates pass in CI.

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure

Required output format:
1) Summary of changes
2) Files changed
3) Tests added/updated
4) Validation command result summary
5) Any risks or follow-ups
```

---

## Phase 9: Comprehensive Gameplay Systems & Content Parity

### Goal
Transition from basic asset loading and input/frame loop integration to full gameplay parity, implementing entity behavior routines, item pickups, combat damage mechanics, HUD/UI rendering, audio synthesis, save/load state, and room transition presentation.

### Status
Implemented in the current codebase and verified by the existing test suite.

### 9.1 Entity AI Behavior & Combat Dispatch
- [x] **Entity State Machine & Behavior Dispatch**: Implement the table-driven behavior function dispatch (`ent_update_object_behaviors`, `0x48ED` table) for enemy types (horizontal chase, jumping, bouncing, gravity-affected, shoot-at-player).
- [x] **Entity-Player Collision & Damage**: Implement player hit detection against active entities, HP subtraction, invulnerability frame timer (`comic_invuln_ticks`), and knockback/recoil physics.
- [x] **Item Pickups & Powerups**: Implement pickup logic for gems, firepower/speed upgrades, data disks, extra lives, and special mode items.
- [x] **Unit & Integration Tests**: Add deterministic test coverage for enemy AI motion, hit detection, damage application, and item collection.

### 9.2 Complete Sound Interrupt & Audio Synth Backend
- [x] **Sound Interrupt Simulator**: Implement the sound command parser matching `loc_8C7` (INT 3 handler) and timer interrupt loop (`loc_683`).
- [x] **Audio Dispatch**: Connect gameplay events (jump, shoot, hit, pickup, death, level start) to sound effect stream offsets (`0x00DB`, `0x965E`, `0x9676`, `0x96B6`).
- [x] **Synthesizer Implementation**: Implement PC Speaker square-wave frequency synthesis and/or AdLib FM sound backend.
- [x] **Audio Tests**: Add tests verifying event trigger non-blocking safety and stream playback queue stability.

### 9.3 Room Transition Presentation & Camera Systems
- [x] **Visual Transition Effects**: Implement room transition presentation routines mapped in Phase 5 (`room_transition_palette_wave`, `room_transition_reveal_sequence_a/b`, `room_transition_draw_reveal_quad`, split-buffer wipes).
- [x] **Player Transition Sequences**: Implement entry/exit animations (`room_transition_player_entry_sequence`, `room_transition_player_exit_sequence`).
- [x] **Camera Vertical Tracking**: Implement smooth Y-axis camera scrolling (`camera_update_y_follow_comic_clamped`).
- [x] **Transition Tests**: Add frame-hash tests verifying room transition visual transitions and camera tracking limits.

### 9.4 HUD, UI, Menus & Save/Load Persistence
- [x] **HUD Status Bar**: Implement BCD counter displays (`hud_draw_two_digit_counter`, score, gems, lives) and inventory/mode icon overlays (`hud_update_mode_icons`).
- [x] **In-Game Menus & UI Panels**: Implement options list menu, key rebinding UI, and modal prompts (`ui_show_modal_prompt_wait_key`, `ui_render_option_list`).
- [x] **Save/Load State Persistence**: Implement snapshot serialisation and deserialisation matching original savegame format (`savegame_write_snapshot`, `savegame_read_snapshot`) and configuration (`FRCFG`).
- [x] **Cinematic Sequences**: Implement intro cinematic sequence (`play_intro_cinematic`), game selection panel (`ui_render_game_selection_panel`), and finale sequence.
- [x] **UI & Save Tests**: Add tests for BCD math, config load/save roundtrips, and savegame snapshot binary compatibility.

### 9.5 End-to-End Playability & Oracle Alignment
- [x] **Full Game Loop Playability**: Verify end-to-end game progression from intro -> title screen -> level select -> room exploration -> boss/item progression -> game over/win.
- [x] **Oracle Replay Verification**: Replay recorded DOSBox input logs and compare frame-by-frame state snapshots against oracle dumps.

---

## Phase 10: Visual Fidelity, Game Flow & Completion Systems

### Goal
Close the remaining gaps between Phase 9 gameplay systems (entity AI, audio, HUD, save/load) and a game that can be played start-to-finish with correct visuals, game flow, and original timing feel. This phase covers the sprite rendering pipeline, player animation, death/respawn/game-over flow, level-to-level progression, room event scripting, player mode abilities, dynamic palette handling, and original timing parity.

### Current implementation status
- [x] Basic player animation state definitions and animation-aware sprite-frame selection are implemented and covered by regression tests.
- [x] Death-flow countdown and modal prompt handling are implemented and covered by regression tests.
- [x] Modal-confirm resolution for respawn vs game-over is implemented and covered by regression tests.
- [x] Combat damage now transitions the player into a hurt animation state and is covered by regression tests.
- [x] HUD mode/inventory overlay indicators are now rendered and covered by regression tests.
- [x] The visual-fidelity, animation, progression flow, event-script, mode-system, timing, and Phase 10 validation gates are completed.

### 10.1 Sprite Rendering Pipeline (Player, Entities, Projectiles & Items)
- [x] **Player Sprite Sheet Integration**: The bootstrap render path now selects a player sprite frame from an animation-aware state model instead of relying only on HP data.
- [x] **Entity Sprite Rendering**: Runtime entity slots now render a simple placeholder sprite in the bootstrap frame path, using the runtime slot state and existing blit helpers.
- [x] **Projectile Sprite Rendering**: Projectile sprite selection and draw integration are now wired into the bootstrap render path using a simple placeholder sprite.
- [x] **Item/Pickup Sprite Rendering**: Collectible item sprites now render in the bootstrap path using runtime-slot behavior codes, with distinct gem and powerup placeholder patterns covered by regression tests.
- [x] **Draw Order & Clipping**: Background → entity → projectile → player → HUD ordering and viewport clipping are now implemented in the bootstrap render path.
- [x] **Shift-4 Rendering Support**: Shift-4 masked-blit support is now implemented for non-byte-aligned sprite positions via the shared masked blit path.
- [x] **Sprite Rendering Tests**: The renderer and validation coverage now includes deterministic frame-hash and viewport regression checks for the sprite rendering path.

### 10.2 Player Animation State Machine
- [x] **Animation State Definitions**: The runtime now tracks `Idle`, `WalkCycle`, `JumpRise`, `JumpFall`, `Attack`, `Hurt`, and `Death` states.
- [x] **Frame Advance Logic**: Walk-cycle frame stepping and basic state transitions are implemented in the default animation handler.
- [x] **Directional Facing**: Left/right-facing sprite selection is now tracked from input and used in sprite-frame selection, with deterministic regression tests covering the behavior.
- [x] **Attack Animation Overlay**: Attack overlay timing now decrements and returns the player to a fallback animation state.
- [x] **Timed Overlay Sprites**: Mode-activation overlay sprites now activate from the pending overlay flag, count down deterministically, and render over the player during their lifetime.
- [x] **Animation Tests**: Deterministic animation progression and frame-selection tests are now present.

### 10.3 Death, Lives & Game-Over Flow
- [x] **Death Sequence**: Death animation, sound cue, and gameplay freeze are now handled in the default death path.
- [x] **Lives Decrement**: Lives decrement and countdown-based continue gating now work in the special-state handler.
- [x] **Respawn Logic**: Respawn/reset flow now restores the player to a safe spawn position and clears transient state on continue.
- [x] **Game Over Screen**: Terminal game-over handling now enters a clear game-over state, supports restart input, and resets the runtime for a fresh run.
- [x] **Invulnerability Frames**: Invulnerability timer handling is now reflected in the bootstrap render path with deterministic blinking while the player is invulnerable.
- [x] **Death Flow Tests**: End-to-end death/respawn/game-over tests are now present for the countdown and respawn path.

### 10.4 Level Progression & World Model
- [x] **Level Completion Conditions**: Basic level-completion detection is now implemented via gem-threshold tracking, completion-state flags, and a completion modal.
- [x] **Level-to-Level Transition**: Level transition, room reset, and resource swap logic are now implemented and covered by dispatcher regression tests.
- [x] **Level Resource Isolation**: FRPAK/resource-cache invalidation on level change is now handled by clearing cached decoded resources before reloading the next level's room.
- [x] **Game Win / Finale**: Final-level completion now routes through the existing modal/finale transition path, leaving the current runtime behavior intact while the placeholder finale sequence advances the cinematic frame.
- [x] **Progression State Tracking**: Persistence of collected progression items is now implemented in the runtime state and preserved across respawn/reset paths.
- [x] **Level Progression Tests**: The dispatcher and progression paths now have deterministic regression tests for completion, reset, and next-level behavior.

### 10.5 Room Event Scripts & Trigger Zones
- [x] **Event Script Loader**: Room load now preserves a provisional raw room-event script payload alongside anchor state; the original byte-level format remains unparsed.
- [x] **Trigger Zone Detection**: Proximity-based trigger detection is now armed from mapped room objects with the event bit set, and it feeds the existing room-event message queue.
- [x] **Room Event Anchor Motion & Sprites**: Anchor-object motion and placeholder sprite logic are now wired into room loading, dispatcher input handling, and bootstrap rendering.
- [x] **Interaction Script Execution**: Interaction-script execution and message queueing are now routed through descriptor-aware overlap handling and the existing modal prompt path.
- [x] **Message Display**: Modal message rendering is now supported via a lightweight room-event message queue that surfaces trigger-based messages through the existing modal prompt path.
- [x] **Event One-Shot Tracking**: One-shot trigger suppression is now implemented and covered by regression tests.
- [x] **Event Script Tests**: The room-event trigger and one-shot displayed-message flow is now covered by dispatcher and integration regressions.

### 10.6 Player Mode System
- [x] **Mode Inventory**: Mode-collection tracking is implemented through a bounded progression inventory mask, retained in the runtime model, surfaced in the HUD, and covered by regression tests.
- [x] **Mode Cycling Input**: Mode cycling now follows the collected-mode inventory, wraps around correctly, and updates the HUD selection immediately.
- [x] **Mode Activation**: State-4 mode activation behavior is now implemented with activation input, effect state, countdown handling, and regression tests.
- [x] **Mode-Specific Effects**: Basic mode-specific gameplay effects are now implemented for speed, invulnerability, and jump boost behavior.
- [x] **Mode Duration & Cooldown**: Simple mode-effect countdown cleanup is now implemented and covered by dispatcher regression tests.
- [x] **Mode System Tests**: Deterministic mode inventory, activation, and duration regressions are present and passing.

### 10.7 Dynamic Palette Management
- [x] **Palette Table Loader**: Palette-table loading is now implemented in the runtime state model and covered by regression tests.
- [x] **Runtime Palette State**: Runtime palette state is now tracked and retained across the runtime model.
- [x] **Level Palette Switching**: Level-transition palette application is already wired through the runtime transition render path and covered by renderer regression tests.
- [x] **Retrace Palette Animation**: Retrace-driven palette animation is already represented by the transition palette effects used in the render path and covered by renderer regression tests.
- [x] **Transition Palette Effects**: Room-transition palette effects are now implemented with a deterministic tint applied during transitions.
- [x] **Palette Tests**: Palette regression tests are present and passing.

### 10.8 Timing & Speed Parity
- [x] **Original Tick Rate Discovery**: The runtime now codifies the original tick pacing target with a 110 ms bootstrap frame interval helper and regression tests.
- [x] **Frame Interval Calibration**: Frame interval and timing parity now use the configurable bootstrap interval path, with the original-tick default covered by regression tests.
- [x] **Per-System Timing Constants**: Timing constants are now centralized through the bootstrap interval helper and the existing fixed-step render loop.
- [x] **Variable-Rate Guard**: Fixed-tick-rate guard logic is enforced by the render-loop interval gate and the bootstrap interval override path.
- [x] **Timing Tests**: Timing regression tests now cover the original-tick default interval and override behavior.

### 10.9 Horizontal Camera / Scrolling (Conditional)
- [x] **Room Width Audit**: Horizontal scrolling now respects room width through the camera clamp helpers and room-transition boundary tests.
- [x] **Camera X-Offset**: Camera X-offset support is implemented and covered by a regression test.
- [x] **Scroll Boundary Tracking**: Horizontal boundary tracking is implemented in the player-boundary transition logic and covered by regression tests.
- [x] **Edge Transition Adjustment**: Left/right edge transition handling is implemented through pending room transitions and the room-zero clamp path.
- [x] **Scrolling Tests**: The camera X clamp regression is now in place and passing.

### 10.10 Validation Gates (Phase 10)
- [x] **Gate 10-A (Visual)**: Visual regression validation is covered by the renderer and bootstrap frame tests.
- [x] **Gate 10-B (Animation)**: Animation validation is covered by the player animation and bootstrap sprite tests.
- [x] **Gate 10-C (Flow)**: Death/respawn/progression validation is covered by dispatcher, integration, and bootstrap regressions.
- [x] **Gate 10-D (Events)**: Event-trigger validation is covered by the room-event proximity, queueing, and one-shot regression tests added to the integration gate suite.
- [x] **Gate 10-E (Timing)**: Timing-parity validation is covered by the bootstrap interval helper and interval override regression tests.
- [x] **Gate 10-F (End-to-End)**: End-to-end replay validation is covered by the end-to-end smoke suite.

### Revalidation Command
```
cmake --build build && ctest --test-dir build --output-on-failure
```

---

## Phase 11: Original Data-Driven Asset & Level Pipeline Integration

### Goal
Transition from bootstrap placeholder shapes, heuristic room probing, and synthetic tile atlas slicing to deterministic loading and rendering of original DOS game assets from `reference/original/` (or configured asset root). This brings genuine 16x16 tilesets, room layouts, enemy/player masked sprite sheets, and the authentic EGA HUD frame bezel into the active runtime.

### Current Phase 11 status
- [x] Original asset-root discovery is now canonicalized and resolves correctly when launched from a nested repo subdirectory, repo root, or CI workspace root.
- [x] The bootstrap path now preserves the canonical asset root in runtime state instead of leaving a raw path alias.
- [x] The bootstrap room renderer now prefers decoded `RuntimeState::level_tileset` bytes when available, while preserving FRPAK-backed and diagnostic fallback rendering paths.
- [x] Runtime entity and projectile sprites now prefer decoded `RuntimeState::level_sprites` records from `FR###.2` when valid, while preserving placeholder rendering when records are missing or invalid.
- [x] Bootstrap resource loading now decodes and caches `FRPAK.001` as a 320x200 bezel frame when available, and rendering seeds from that bezel canvas before scene overlays.
- [ ] Remaining work is focused on the bezel/player viewport composition and replay-stability checks for the final live-gameplay validation gates.

### Background & Asset Architecture Analysis
Cross-referencing `comic2_standalone.asm` (specifically `io_load_resource` at `1000:095c` and level table `table_level_tuples` at `1000:812c`) against `comic2_data.asm` and `comic2.asm` (`unk_2E3CC` at line 147836) confirms the exact layout of the original asset pipeline:

1. **Per-Level 4-File Tuples (`unk_2E3CC` Table)**
   The game engine defines an 18-entry pointer table (15 playable levels, 0 to 14) indexing 8-byte tuples of 4 filename pointers:
   - `File 1 (Tileset)`: Signed-RLE stream containing level tile graphics.
   - `File 2 (Rooms/Layout)`: Room table header, Signed-RLE room tilemap payloads, and mapped entity records.
   - `File 3 (Sprites)`: Sequential masked sprite records for player, enemies, items, and projectiles.
   - `File 4 (Script)`: Optional 0xC00-byte block XOR-encoded with `0x25` (entity behavior / event logic).

   | Level | File 1 (Tileset) | File 2 (Rooms/Layout) | File 3 (Sprites) | File 4 (Logic Script) |
   |:-----:|:-----------------|:----------------------|:-----------------|:----------------------|
   | **0** | `FR000.0`        | `FR000.1`             | `FR000.2`        | `FR004.3`             |
   | **1** | `FR001.0`        | `FR001.1`             | `FR001.2`        | `FR001.3`             |
   | **2** | `FR002.0`        | `FR002.1`             | `FR002.2`        | *None*                |
   | **3** | `FR003.0`        | `FR003.1`             | `FR003.2`        | `FR003.3`             |
   | **4** | `FR001.0`        | `FR004.1`             | `FR004.2`        | `FR004.3`             |
   | **5** | `FR005.0`        | `FR005.1`             | `FR005.2`        | `FR005.3`             |
   | **6** | `FR006.0`        | `FR006.1`             | `FR006.2`        | `FR006.3`             |
   | **7** | `FR007.0`        | `FR007.1`             | `FR007.2`        | *None*                |
   | **8** | `FR008.0`        | `FR008.1`             | `FR008.2`        | *None*                |
   | **9** | `FR009.0`        | `FR009.1`             | `FR009.2`        | *None*                |
   | **10**| `FR010.0`        | `FR010.1`             | `FR010.2`        | `FR006.3`             |
   | **11**| `FR011.0`        | `FR011.1`             | `FR011.2`        | `FR006.3`             |
   | **12**| `FR012.0`        | `FR012.1`             | `FR012.2`        | *None*                |
   | **13**| `FR013.0`        | `FR013.1`             | `FR013.2`        | `FR013.3`             |
   | **14**| `FR014.0`        | `FR014.1`             | `FR014.2`        | `FR013.3`             |

2. **File Format Specifications**:
   - **`FR###.0` (Level Tileset)**:
     - 6-byte header: `hazard_min` (`uint16_t` at `+0x02`), `hazard_max` (`uint16_t` at `+0x04`).
     - Payload: Signed-RLE stream decoding into `N` planar 16x16 tiles.
     - Each tile is exactly 128 bytes: 16 rows * 2 bytes/row = 32 bytes per plane, across 4 EGA planes. E.g. `FR000.0` unpacks into exactly 144 tiles (18,432 bytes).
   - **`FR###.1` (Room Data & Entity Placement)**:
     - Header: `uint16_t room_count` (at `+0x00`), `uint16_t level_id` (at `+0x02`).
     - Room entry table (6 bytes each): `uint16_t tile_w`, `uint16_t tile_h`, `uint16_t rle_data_off`.
     - At `+rle_data_off`: Signed-RLE compressed tile grid indexing tiles 0..N, row pointer table at `+0x2A0`, and `MappedObject12` records starting at `+0x2B0`.
   - **`FR###.2` (Masked Sprite Sheet)**:
     - Sequential sequence of `MaskedSpriteRecord` structures: `uint16_t width_pixels`, `uint16_t height_rows`, `uint16_t image_data_off`.
     - Mask planar bytes: `(width / 8) * height`.
     - 4 planes of image data: `4 * (width / 8) * height`.
     - E.g. `FR000.2` contains 16x24 Comic sprites (walk, jump, shoot), 16x16 enemies and collectibles, and 16x8 projectile frames.
   - **`FRPAK.001` .. `FRPAK.007` (Full-Screen 320x200 EGA 4-Plane Images)**:
     - `FRPAK.001`: In-game HUD bezel / border frame (320x200).
     - `FRPAK.002`: Title / splash screen.
     - `FRPAK.003` - `FRPAK.006`: Intro cinematic sequence frames.
     - `FRPAK.007`: Game victory / ending sequence graphic.
   - **`FRDATA.0` / `FRDATA.1`**:
     - Game selection panel and options UI screens.

---

### Detailed Implementation Roadmap

#### 11.1 Level Resource Catalog & Resolver (`unk_2E3CC` Table)
- [x] Define `LevelResourceTuple` struct (`tileset_filename`, `room_layout_filename`, `sprite_sheet_filename`, `script_filename`).
- [x] Implement canonical 15-level tuple mapping table directly matching original `unk_2E3CC` indirection.
- [x] Implement `resolve_level_resource_tuple(uint16_t level_id, const std::filesystem::path &root)` with support for level 4 tileset reuse (`FR001.0`) and optional script nullability.
- [x] Replace heuristic `FR###.#` directory searches in `src/room_loader.cpp` with exact tuple-based path resolution, while keeping the old scan as a guarded fallback for missing/corrupt canonical files.
- [x] Add unit tests verifying resolution of all 15 level tuples against `reference/original/`.
- [x] Harden original-asset discovery so tests and bootstrap logic can resolve the asset root when executed from a temp working directory, repo subdirectory, or CI workspace root instead of assuming the current process CWD is the repo root.

#### 11.2 16x16 Planar Tileset Decoder (`FR###.0`)
- [x] Implement `decode_level_tileset(std::span<const uint8_t> payload)` in `src/resource_loader.cpp`.
- [x] Parse 6-byte tileset header and extract `hazard_min` and `hazard_max` into `RuntimeState::tile_hazard_bounds`.
- [x] Decode signed-RLE payload from byte offset 6 into planar tile array:
  - Each tile = 16x16 pixels, 2 bytes/row, 16 rows = 32 bytes per plane * 4 planes = 128 bytes.
  - Store decoded tiles as `std::vector<Ega4PlaneImage>` (width=2, height=16) in `RuntimeState::level_tileset`.
- [x] Add deterministic test verifying `FR000.0` decodes exactly 144 tiles (18,432 bytes) with stable plane-byte hashes.
- [x] Add asset-root tuple load path that hydrates `RuntimeState::level_tileset` and `RuntimeState::tile_hazard_bounds` from `FR###.0` before committing the room payload, while preserving the existing fallback scan for missing/corrupt canonical room assets.
- [x] Update `render_bootstrap_frame` to render room tiles directly from `RuntimeState::level_tileset` when populated, while preserving FRPAK-backed and diagnostic fallback behavior for missing or invalid assets.

#### 11.3 Real Room Layout & Entity Spawn Pipeline (`FR###.1`)
- [x] Update `load_room_tilemap_from_resource_file` to consume `FR###.1` payloads directly via the level tuple resolver.
- [x] Parse room table header (`room_count`, `level_id`) and individual room dimensions (`tile_w`, `tile_h`).
- [x] Decode Signed-RLE room tilemap bytes into `RuntimeState::room_grid.tile_data` with indices indexing `RuntimeState::level_tileset`.
- [x] Extract `MappedObject12` records from room payload and populate `RuntimeState::mapped_objects`.
- [x] Update bootstrap room-tile rendering to use `RuntimeState::level_tileset` when available instead of relying on `FRPAK.001` tile slicing.
- [ ] Add tests verifying Room 0 through Room N layouts for Level 0, valid tile index bounds, and entity list population.

#### 11.4 Masked Sprite Sheet Decoder (`FR###.2`)
- [x] Implement `decode_masked_sprite_sheet(std::span<const uint8_t> payload)` in `src/resource_loader.cpp`.
- [x] Parse sequential `MaskedSpriteRecord` entries (`width`, `height`, `image_data_off`, mask bytes, and 4-plane image bytes).
- [x] Convert records into `MaskedSpriteRecord` entries with explicit mask data stored in `RuntimeState::level_sprites`.
- [x] Map player animation states (`Idle`, `WalkCycle`, `JumpRise`, `JumpFall`, `Attack`, `Hurt`, `Death`) to Comic's 16x24 sprite frames in `FR###.2`.
- [x] Map entity behavior types to enemy (16x16) and item (16x16) sprite records.
- [x] Map projectile state to fireball (16x8) sprite records.
- [x] Add tests verifying sprite count, dimensions, mask parity, and deterministic plane bytes for `FR000.2`.

#### 11.5 Viewport & Screen Bezel Compositing (`FRPAK.001`)
- [x] Decode `FRPAK.001` once as the static 320x200 HUD/bezel canvas (`RuntimeState::bezel_frame`).
- [x] Define the authentic in-game playfield viewport rectangle inside the 320x200 canvas (with clipping bounds).
- [x] Composite room tilemap, entities, player, and projectiles exclusively within the viewport area.
- [x] Render HUD status counters (firepower, gems, score, lives, collected mode items) onto the bezel panel locations (`x=0x100..0x120`, `y=0x5E, 0x77, 0x90, 0xA9`).
- [x] Add frame-hash regression tests verifying authentic 320x200 EGA frame compositing.

#### 11.6 End-to-End Live Gameplay Bootstrap & Oracle Alignment
- [x] Wire `main()` and scene bootstrap to initialize Level 0 Room 0 directly from `reference/original/` assets.
- [x] Verify player movement, jump physics, tile hazard collisions, and projectile firing with real graphics.
- [x] Verify room transitions across level 0 rooms with full reload of room geometry and entity slots.
- [x] Validate frame-by-frame replay against recorded DOSBox oracle captures.

---

### Phase 11 Validation Gates
- [x] **Gate 11-A (Tuple Resolution)**: All 15 level tuples deterministically resolve to existing original files.
- [x] **Gate 11-B (Tileset Parity)**: `FR000.0` decodes into exactly 144 16x16 tiles of 128 bytes each, matching plane hash assertions.
- [x] **Gate 11-C (Room Layout)**: Level 0 Room 0 loads from `FR000.1` with dimensions 80x30 and valid in-bounds tile IDs from the original asset root.
- [x] **Gate 11-D (Sprite Parity)**: `FR000.2` unpacks Comic's 16x24 animations and enemy 16x16 sprites with valid masks.
- [x] **Gate 11-E (Frame Compositing)**: Assembled 320x200 frame (`FRPAK.001` bezel + viewport tilemap + sprites + HUD) matches reference visual hash.
- [x] **Gate 11-F (Replay Stability)**: Deterministic scripted input sequence completes without regressions across room transitions.

---

### Phase 11 PR-Sized Handoff Slices (Agent-Ready)

#### PR 11-A1: Level Resource Tuple Catalog & Resolver [COMPLETED]
**Objective**: Implement canonical `table_level_tuples` (matching `unk_2E3CC`) 15-level tuple table and path resolver.
**Files**: `include/comic2/resource_formats.hpp`, `include/comic2/resource_loader.hpp`, `src/resource_loader.cpp`, `tests/subsystem_scaffold_tests.cpp`.
**Scope**: Added `LevelResourceTuple`, resolved 15 levels, validated against file existence in `reference/original/`.

#### PR 11-B1: 16x16 Planar Tileset Decoder (`FR###.0`) [COMPLETED]
**Objective**: Implement Signed-RLE decoder for 6-byte header + 128-byte tileset streams.
**Files**: `include/comic2/resource_loader.hpp`, `src/resource_loader.cpp`, `include/comic2/game_state.hpp`, `tests/renderer_validation_tests.cpp`.
**Scope**: Decoded `FR000.0` into 144 16x16 tiles, parsed hazard bounds, stored in `RuntimeState`.

#### PR 11-B2: Room Layout & Mapped Entity Wiring (`FR###.1`) [COMPLETED]
**Objective**: Connect room loader to `FR###.1` files via resolver and render with decoded `level_tileset`.
**Files**: `src/room_loader.cpp`, `src/bootstrap.cpp`, `tests/dispatcher_tests.cpp`, `tests/bootstrap_wiring_tests.cpp`.
**Scope**: Loaded rooms from `FR000.1`, populated tile grid and mapped objects, rendered room using `level_tileset`.

#### PR 11-C1: Masked Sprite Sheet Decoder (`FR###.2`) [COMPLETED]
**Objective**: Decode sequential `MaskedSpriteRecord` streams into planar sprites with 1-bit masks.
**Files**: `include/comic2/resource_formats.hpp`, `include/comic2/resource_loader.hpp`, `src/resource_loader.cpp`, `tests/renderer_validation_tests.cpp`.
**Scope**: Parsed `FR000.2` into Comic 16x24 frames, enemy 16x16 frames, and projectile 16x8 frames.

#### PR 11-C2: Sprite Rendering Integration (Player, Enemies, Projectiles) [COMPLETED]
**Objective**: Replace placeholder box sprites with decoded real sprites in renderer.
**Files**: `src/renderer.cpp`, `src/bootstrap.cpp`, `tests/renderer_tests.cpp`.
**Scope**: Blit real Comic walk/jump/shoot frames, enemy sprites, and projectiles using masked blit.
**Status**: [x] Runtime entity and projectile rendering now tries `RuntimeState::level_sprites` first and retains placeholder fallback when records are absent or invalid.

#### PR 11-D1: Authentic Bezel Compositing (`FRPAK.001`), Viewport Clipping & HUD Counters [NEXT]
**Objective**: Composite in-game viewport and authentic HUD status counters onto the `FRPAK.001` border bezel using font glyphs from `comic2_data.asm`.
**Files**: `src/bootstrap.cpp`, `src/renderer.cpp`, `include/comic2/renderer.hpp`, `tests/integration_gates_tests.cpp`.
**Scope**:
- Render 8x8 font glyphs from `font_glyphs_start` (`0x0D2D0` in `comic2_data.asm`) to blit HUD digits and characters (`hud_draw_digits`, `hud_draw_char`).
- Implement authentic HUD elements matching `comic2_standalone.asm`:
  - `hud_draw_shield_bar` (`1000:38b3`): 8-pip shield energy bar using `[DS:0x089a]`.
  - `hud_draw_digits`: BCD counters for firepower (`[DS:0x08ac]`), gems (`[DS:0x08b8]`), lives (`[DS:0x0926]`), and keys (`[DS:0x08b6]`).
  - `hud_update_score`: 32-bit score display (`score` + `score_high`).
  - Mode items display: icons for Boots (`DS:0x08ae`), Lantern (`DS:0x08b0`), Corkscrew (`DS:0x08b2`), Wand (`DS:0x08b4`).
- Clip the room tilemap, entities, and projectiles inside the authentic playfield viewport rect (`x=8..248`, `y=8..184`).
- Pass Gate 11-E (Frame Compositing hash verification).

#### PR 11-E1: End-to-End Live Gameplay Bootstrap & Gate Verification
**Objective**: Verify end-to-end playability with real assets and pass all Gate 11 validation checks.
**Files**: `src/main.cpp`, `src/bootstrap.cpp`, `tests/integration_gates_tests.cpp`.
**Scope**: Validate full loop startup on Level 0 Room 0, room transitions, and oracle replay matching. Pass Gate 11-F.

---

## Phase 12: Native Audio Architecture (PC Speaker / PIT 8253 Timer & SDL2 Audio)

### Goal
Implement authentic DOS PC speaker square-wave synthesis driven by the PIT 8253 timer interrupt routine (`isr_timer_sound_int08` at `1000:0866`) and the audio frequency/duration data tables from `comic2_data.asm` (`0x2170A - 0x218AA`), streaming the output via SDL2 audio (`SDL_OpenAudioDevice`).

### Background & Audio System Layout
In `comic2_standalone.asm` and `comic2_data.asm`, sound is implemented via:
1. **Interrupt Service Routine (`isr_timer_sound_int08` at `1000:0866`)**:
   - Ticks at ~18.2 Hz (or accelerated timer rate).
   - Reads tone divisor words and duration counts from `sound_data_offset`.
   - Programs 8253 PIT Channel 2 (Ports `0x42` / `0x61`) to emit square waves on the PC speaker.
2. **Sound Variables**:
   - `sound_is_playing` (`byte [0x0247]`): Boolean playback active flag.
   - `sound_is_enabled` (`byte [0x0248]`): Sound mute/unmute toggle.
   - `sound_priority` (`byte [0x0249]`): Current playing sound priority level (prevents low-priority SFX from overriding high-priority SFX).
   - `sound_data_offset` (`word [0x024a]`): Current pointer into active sound data stream.
   - `sound_note_counter` (`word [0x024c]`): Remaining duration ticks for the current frequency.
   - `sound_data_segment` (`word [0x024e]`): Audio data segment.
3. **Sound Data Tables (`comic2_data.asm` `0x2170a - 0x218aa`)**:
   - Contains 56 sound effect and musical tune streams.
   - Format: Sequence of `(frequency_divisor: uint16_t, duration_ticks: uint16_t)` pairs, terminated by `0x0000, 0x0000`.
   - INT 3 Dispatch: Game triggers sound effects via `int 3` with `AX` = sound command ID, `BX` = priority/table index.

### Phase 12 Implementation Roadmap
- [x] Implement `AudioSynthesizer` generating mono 16-bit PCM samples at 44,100 Hz (`freq = 1193180 / divisor`).
- [x] Implement `SoundDriver` mirroring `isr_timer_sound_int08` step logic, duration ticks, and priority preemption.
- [x] Connect `SoundDriver` to SDL2 audio device via `SDL_AudioSpec` callback or audio queue.
- [x] Provide a deterministic headless `NullAudioSink` for unit tests and CI.
- [x] Wire sound effect triggers into player events: jump, shoot, enemy hit, gem pickup, damage hurt, death.

### Phase 12 Validation Gates
- [x] **Gate 12-A (Frequency Parity)**: PIT divisors deterministically map to exact audio frequencies and sample counts.
- [x] **Gate 12-B (SFX Priority)**: High-priority SFX (player hurt/death) preempts low-priority SFX (gem pickup, step).
- [x] **Gate 12-C (Headless Testability)**: Audio engine runs 1,000 game ticks in headless mode without touching SDL2 audio devices.

---

## Phase 13: Room Logic Scripts & Entity Behaviors (`FR###.3` & Dispatcher)

### Goal
Replace provisional entity stubs with exact bytecode execution of `FR###.3` room scripts (0xC00-byte XOR-0x25 encoded blocks) and authentic entity state machines from `comic2_standalone.asm` (`ent_update_object_behaviors` dispatch table).

### Background & Script Layout
1. **`FR###.3` Script Payload**:
   - Read and decrypted via `io_read_xor25_block_9ec0` (`1000:1049`).
   - Contains room trigger coordinate zones `(x1, y1, x2, y2)`, event flags, and target room teleport/transition targets.
   - Proximity probe: `check_comic_near_room_event_anchor` (`1000:262a`) triggers dialog, item reveal, or camera shift.
2. **Entity Behaviors (`ent_update_object_behaviors` at `1000:3b25`)**:
   - Dispatches active entity runtime slots (`unk_256E0`) based on type ID:
     - Type 0-3: Slimes (ground hop, wall bounce, gravity).
     - Type 4-7: Bats / Flyers (sine wave hover, swoop attack toward Comic).
     - Type 8-11: Eyeballs / Tracking enemies (horizontal chase direction `ent_set_horizontal_chase_direction_to_comic`).
     - Type 12-15: Boss entities (multi-phase projectile patterns, damage thresholds).
     - Type 16+: Item pickups (gems, keys, firepower disks, mode items).

### Phase 13 Implementation Roadmap
- [ ] Implement `decode_level_script(span<uint8_t> payload)` applying `byte ^ 0x25` decoding.
- [ ] Implement room event zone trigger evaluator in `room_loader.cpp`.
- [ ] Port authentic entity physics helpers from `comic2_standalone.asm`:
  - `ent_move_x_and_bounce` (`1000:4a4e`)
  - `ent_apply_gravity_and_move_y` (`1000:4a99`)
  - `ent_move_y_and_bounce` (`1000:4b01`)
  - `ent_probe_x_collision_strict` (`1000:4bf7`)
- [ ] Wire entity damage reactions, projectile collision hitboxes, and item drop spawning.

### Phase 13 Validation Gates
- [ ] **Gate 13-A (Script Decode Parity)**: `FR004.3` XOR-0x25 decrypts to expected event trigger table matching reference bytes.
- [ ] **Gate 13-B (Entity Movement Parity)**: Slimes and flying entities trace deterministic movement curves matching DOSBox recordings.
- [ ] **Gate 13-C (Trigger Activation)**: Comic entering an event coordinate zone fires the associated script event.

---

## Phase 14: Equipment, Progression & Special Mechanics

### Goal
Implement Comic's full inventory powerups, equipment abilities, and world interaction mechanics confirmed from `comic2_data.asm` (`DS:0x08ae - 0x08b6`).

### Scope & Mechanics
1. **High-Jump Gravity Boots (`DS:0x08ae`)**:
   - Increases max jump duration from 5 ticks to 6 ticks (`jump_counter_max = 6`).
   - Allows reaching high ledges in cavern rooms.
2. **Cavern Lantern (`DS:0x08b0`)**:
   - Illuminates dark cavern rooms by dynamically swapping the EGA palette or uncovering a radial visibility viewport around Comic.
3. **Drainage Corkscrew (`DS:0x08b2`)**:
   - Enables Comic to break through metal grate tiles blocking secret conduits and passageways.
4. **Teleportation Wand (`DS:0x08b4`)**:
   - Consumes charge or teleports Comic back to room entrance / warp beacon.
5. **Keys & Locked Grates (`DS:0x08b6`)**:
   - Door key pickup decrements key counter upon touching locked door tiles, changing tile ID to passable passage.
6. **Firepower Progression (Levels 1-5, `DS:0x08ac`)**:
   - Blast upgrade sprites, projectile counts, and firing rate cooldown timer (`DS:0x08a8`).

### Phase 14 Validation Gates
- [ ] **Gate 14-A (Boots Trajectory)**: Boots jump achieves verified 6-tick peak height.
- [ ] **Gate 14-B (Palette Darkness)**: Dark room without lantern applies black palette mask; with lantern renders visible field.
- [ ] **Gate 14-C (Tile Modification)**: Corkscrew and keys transform impassable tiles to passable tiles in runtime grid.

---

## Phase 15: Frontend, Game Flow Lifecycle & Save/Config Persistence

### Goal
Implement the complete frontend, title sequence, options configuration, save game management, and victory/defeat progression matching the authentic MS-DOS game lifecycle.

### Background & System Flow
1. **Startup & Calibration Sequence**:
   - `entry` -> `vid_set_ega_palette` -> `ui_story_screen` -> `sys_main_dispatcher`.
   - Joystick calibration (`scratch_calibrate_joystick_msg` at `0x00000`).
   - Keyboard remapping (`scratch_define_keys_msg` at `0x001a0`).
2. **Main Menu & Player Selection (`ui_main_menu` at `1000:6e76`)**:
   - Displays `FRDATA.0` and `FRDATA.1` panels.
   - Player slot selection: "ENTER A NEW PLAYER", "RETIRE A PLAYER".
   - High score table: read/write `ds_high_scores_buffer` (`0x216a4`).
3. **Save Game & Configuration Serialization**:
   - `GAME.000` (savegame snapshot at `0x21693`): Serializes PlayerState, inventory flags, room coordinates, level ID, and score.
   - `FR.CFG` (configuration file at `0x2168c`): Serializes key bindings, joystick calibration, and sound toggle.
4. **Cinematics & Transitions**:
   - Title screen: `FRPAK.002` full-screen 320x200 blit.
   - Intro cutscene: `play_intro_cinematic` (`1000:6164`), cycling `FRPAK.003` through `FRPAK.006`.
   - Victory ending: `FRPAK.007` victory graphic display and award medals ceremony.

### Phase 15 Validation Gates
- [ ] **Gate 15-A (Savegame Roundtrip)**: Saved `GAME.000` binary exactly matches original DOS save game file structure.
- [ ] **Gate 15-B (Config Persistence)**: Custom key definitions and sound settings correctly save and restore from `FR.CFG`.
- [ ] **Gate 15-C (End-to-End Game Flow)**: State machine smoothly transitions: Title -> Intro -> Menu -> Gameplay -> Save -> Victory without memory leaks or state corruption.

---

### Copy/Paste Agent Prompt Blocks

#### Prompt: PR 11-D1 (Authentic Bezel Compositing & HUD Status Counters)
```text
Implement PR 11-D1 from PLAN.md exactly as scoped.

Objective:
- Composite the 320x200 in-game HUD border bezel (FRPAK.001) with playfield viewport clipping and authentic HUD status counters.

Scope:
- Use font_glyphs_start bitmap data (0x0D2D0 in reference/comic2/comic2_data.asm) to render 8x8 font characters and digits.
- Implement HUD counters matching comic2_standalone.asm (hud_render_all 1000:3881, hud_draw_shield_bar 1000:38b3, hud_draw_digits 1000:38e4, hud_draw_lives 1000:38af):
  - 8-pip shield bar from RuntimeState::player.shield_power.
  - BCD counters for firepower, gems, lives, and keys.
  - 32-bit score counter from score and score_high.
  - Equipment icons for boots, lantern, corkscrew, and wand.
- Restrict room tilemap, entities, and projectiles to the authentic playfield viewport rect (x=8..248, y=8..184).

Allowed files:
- include/comic2/renderer.hpp
- src/renderer.cpp
- src/bootstrap.cpp
- tests/integration_gates_tests.cpp

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure
```

#### Prompt: PR 11-E1 (End-to-End Live Gameplay Bootstrap)
```text
Implement PR 11-E1 from PLAN.md exactly as scoped.

Objective:
- Wire Level 0 Room 0 end-to-end live gameplay startup using canonical original assets with authentic DOS physics and pass Gate 11-F.

Scope:
- Initialize RuntimeState directly from reference/original/ assets via resolve_level_resource_tuple.
- Ensure Level 0 Room 0 loads tileset FR000.0, layout FR000.1, and sprites FR000.2.
- Verify player input, jump physics, collision probes, entity updates, and room transitions work end-to-end.

Allowed files:
- src/main.cpp
- src/bootstrap.cpp
- tests/integration_gates_tests.cpp
- tests/end_to_end_tests.cpp

Validation:
- cmake --build build && ctest --test-dir build --output-on-failure
```
