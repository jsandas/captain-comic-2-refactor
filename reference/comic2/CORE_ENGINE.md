# Captain Comic 2: Core Engine Analysis (`game_main_loop`)

The main gameplay loop and state machine begin at `game_main_loop` (`1000:2341`, line 2793 in `comic2_standalone.asm`) and span through `1000:39AD`. Unlike the fragmented IDA representation in the old `comic2.asm`, `comic2_standalone.asm` presents the contiguous linear code layout in `CS`.

> **Note on Labels in `comic2_standalone.asm`:** Several functions in `comic2_standalone.asm` carry auto-generated heuristic labels whose names do not match their actual behavior (for example, `vid_clear_page` at `1000:77A3` is actually the masked sprite blitter, and `map_redraw_dirty_tiles` at `1000:5D5F` is actually the player projectile update loop). In this document, both the **label in `comic2_standalone.asm`** and the **actual verified behavior** are listed.

## Main Dispatcher (`game_main_loop` at `1000:2341`)

Address: `1000:2341` (Line 2793 in `comic2_standalone.asm`)

### Per-Frame Setup:
- Flushes the BIOS keyboard ring buffer (`0040:001A` = `0040:001C`).
- Increments frame tick counter `ds:0x962B`, saves `SP` to `ds:0x08E8` (`ds_saved_sp`), cycles the 3-step player animation counter `ds:0x08AF` (`1..3`), resets `ds:0x08AE` (`comic_state` = 0), and decrements door-cooldown timer `ds:0x962A` if non-zero.

### Priority State Machine Sequence:
1.  **Death / Overlay / Dialog Freeze Check**
    *   Variables: `ds:0x08EC != 0` (death/teleport animation countdown) or `ds:0x0921 != 0` (modal dialog box active)
    *   Target: `loc_275c` (skips player control and jumps straight to rendering)
2.  **Special Room Event 1 (Stage 2 Scripted Carrier)**
    *   Variables: `ds:0x0924 > 1`
    *   Target: `ent_turn_around` (`1000:3657`, scripted horizontal carrier/event motion)
3.  **Special Room Event 2 (Stage 9 Scripted Minecart / Platform)**
    *   Variables: `ds:0x092C > 1`
    *   Target: `loc_386f` (`1000:386F`, scripted rail/cart physics and collision)
4.  **Swimming / Underwater Mode Check**
    *   Variables: `ds:0x089D != 0` (`comic_is_swimming`)
    *   Target: `player_process_controls` (`1000:2EDC`, underwater buoyancy & swimming movement)
5.  **Mode 2 (Jetpack / Levitation) Activation Check**
    *   Variables: `ds:0x0896 == 2` (and in stage 0 without item `ds:0x0222` or stage 14)
    *   If Up (`cs:0x025A`) or Fire (`cs:0x025D`) is pressed, calls `ent_destroy` (`1000:3610` — *actually starts timed 8-tick overlay sprite above Comic*).
6.  **Airborne / Jump / Gravity Physics Check**
    *   Variables: `ds:0x0897 != 0` (`comic_is_physics_active`)
    *   Target: `player_update_physics` (`1000:2A10`, jump boost, gravity integration, and landing checks)
7.  **Mode 1 (Wand / Tile-Transform Action) Animation Check**
    *   Variables: `ds:0x08A3 != 0` (`comic_is_action_active`)
    *   Target: `proj_init_trajectory` (`1000:3107` — *actually handles 3-tick state-4 wand/tile-trigger animation and spawns tile-swap effect via `proj_spawn_burst`*)
8.  **Mode 3 (Pickaxe / Object-Interact Action) Animation Check**
    *   Variables: `ds:0x08A7 != 0` (`comic_is_attack_active`)
    *   Target: `ent_update_ai` (`1000:34A9` — *actually handles 3-tick state-6 pickaxe/interact animation against active room entities at `ds:0x0700`*)
9.  **Stage 9 Cart Proximity Mount Check**
    *   Variables: `ds:0x092C == 1` and `0 <= (comic_x - ds:0x0930) <= 0x18`
    *   Target: `hud_update_score` (`1000:385F` — *actually latches Stage 9 cart mount state `ds:0x092C = 2` and enters `loc_386f`*)
10. **Tile Hazard Check**
    *   Calls: `map_get_tile_at` (`1000:1CFE`) at `(comic_x + 8, comic_y + 31)`
    *   If tile ID is in `(ds:0x0294, ds:0x0296]` -> `player_stop_jump` (`1000:2DC6` — *actually handles hazard/water transition into swimming or damage*)
11. **Jetpack / Hover Mode (`ds:0x08A6`) Check**
    *   Variables: `ds:0x08A6 != 0`
    *   Target: `ent_update_all` (`1000:330F` — *actually updates state-5 hover/jetpack vertical thrust, fuel depletion via `item_add_to_inventory`, and movement*)
12. **Grounded Input Handling (If No Active State)**
    *   **Jump Initiation (`loc_243f`)**:
        *   If `key_state_jump` (`cs:0x025C`) == 1 and `comic_jump_counter` (`ds:0x0898`) > 0:
            *   Sets `comic_is_physics_active` (`ds:0x0897`) = 1.
            *   If `key_state_down` (`cs:0x025B`) == 1, starts a drop-through jump (`comic_y_vel = 5`, `ds:0x089A = 2`); otherwise plays jump SFX (`0x96DE`) and enters `player_update_physics`.
    *   **Doorway / Room Transition Check (`loc_24a3` - `loc_2541`)**:
        *   If `key_state_up` (`cs:0x025A`) == 1 (and cooldown `ds:0x962A == 0`), checks special save-room coordinates or iterates the room's doorway table (`ds:0x06EE` count, `ds:0x06F0` pointer in CS, 18-byte records). If Comic stands in a doorway (and has required key/item), jumps to `game_start_room` (`1000:20FE`).
    *   **Horizontal Ground Movement (`loc_2548`)**:
        *   Resets `comic_x_vel` (`ds:0x0892`) = 0.
        *   If `key_state_left` (`cs:0x0258`) == 1: sets `comic_x_vel = -5` and calls `player_check_ceiling_collision` (`1000:2BDC` — *actually `move_player_left`*).
        *   If `key_state_right` (`cs:0x0259`) == 1: sets `comic_x_vel = +5` and calls `player_check_wall_collision_right` (`1000:2C39` — *actually `move_player_right`*).
    *   **Floor Support Check (`1000:2570`)**:
        *   Calls `player_check_swimming` (`1000:2C9D` — *actually `check_floor_collision`*) at `(comic_x, comic_y + 32)` with threshold `DI = ds:0x0294`.
        *   If not supported (CF=0): sets `comic_is_physics_active` (`ds:0x0897`) = 1, `comic_jump_counter` = 0, `comic_y_vel` = 5, and clamps `comic_x_vel` to `[-1, +1]`.
    *   **Conveyor Belt / Stage 9 Tile Check (`loc_25ba`)**:
        *   Checks for special conveyor tiles (`0x1C..0x1E`) in Stage 9 and applies automatic horizontal step.
    *   **Post-Movement Actions & Keys (`loc_267f` - `loc_275c`)**:
        *   Home / `'G'` (`cs:0x0257`): Calls `game_pause_handler` (`1000:46E6` — *actually the inventory slot selection & item drop/activation UI*).
        *   Fire (`cs:0x025D`): Depending on selected mode `ds:0x0896`:
            *   Mode 0 (Blaster): Calls `map_update_animated_tiles` (`1000:5CEB` — *actually `spawn_player_projectile`*) if `ds:0x08EA > 0` (C-energy meter) and `ds:0x021C > 0` (Blastola Cola firepower), then decrements C-meter via `hud_draw_char` (`1000:3B12`).
            *   Mode 1 (Wand): Calls `player_fire_projectile` (`1000:30C6` — *starts state-4 wand animation*).
            *   Mode 3 (Pickaxe): Calls `ent_spawn_enemy` (`1000:3468` — *starts state-6 pickaxe animation*).
        *   Esc (`cs:0x0250`): Calls `hud_draw_lives` (`1000:39AD` — *actually `ui_show_pause_quit_prompt`, showing `"GAME PAUSED PRESS (Q) TO QUIT"`*).
        *   PgUp / `'I'` (`cs:0x0255`) and PgDn / `'Q'` (`cs:0x0256`): Calls `proj_check_monster_hit` (`1000:327D` — *actually `player_cycle_mode_selection` with `AX = -1` or `+1`*).

### Frame Rendering & Entity Update (`loc_275c` at `1000:275C`)
1.  **Camera Shake (`ds:0x093B` / `ds:0x093A`)**: Oscillates `scroll_y` (`ds:0x06C4`) by ±2 pixels when active.
2.  **Render Background**: Calls `map_draw_background` (`1000:1D2C`) to copy the 4-plane tilemap viewport to the active EGA page.
3.  **Render Room Entities & Pickups**: Calls `ui_draw_text_string` (`1000:437B` — *actually `ent_update_entities_in_viewport`*, which draws visible static/mapped objects from `ds:0x0700` and checks player pickup/trigger contact).
4.  **Update & Render Projectiles**: Calls `map_redraw_dirty_tiles` (`1000:5D5F` — *actually `update_projectiles`*).
5.  **Update & Render Monsters**: Calls `vid_flip_buffers` (`1000:4B25` — *actually `ent_update_object_behaviors`*, which ticks active 32-byte monster slots at `cs:0x48ED` and renders them via `ent_render_all` / `spr_render_water_ripple`).
6.  **Render Player Sprite (`1000:27A0` - `1000:2902`)**: Draws death/teleport frames (if `ds:0x08EC > 0`), carrier/cart sprites, or directional Comic sprite from `cs:0x4882` (right) / `cs:0x4860` (left) via `vid_blit_planar_block` (`1000:7DBB`).
7.  **Render Transient Tile/Explosion Effects (`loc_291a`)**: If `ds:0x08B1 > 0`, calls `proj_update_all` (`1000:31EB` — *actually `ent_update_transient_slots` for 9-byte temporary animations at `ds:0x08B2`*).
8.  **Render Overlays & Dialogs (`loc_2924` - `loc_29d7`)**: Draws newly dropped inventory item blinking (`ds:0x08A8`), overhead thoughts/icons (`ds:0x091E`), modal dialog boxes (`ds:0x0921` via `ui_draw_window_box` at `1000:4206`), and increments shield meter on tick (`ds:0x08ED` via `hud_flash_item` at `1000:3B51`).
9.  **Page Flip & Sync (`1000:29D7` - `1000:2A0D`)**:
    *   Calls `vid_wait_vblank` (`1000:774E` — presents active EGA page on retrace and toggles `cs:0x773F` between `0x0000` and `0x2000`).
    *   Checks if `comic_y > room_pixel_height` (`ds:0x029E`) or death countdown finished (`loc_29e3`), decrementing lives and respawning or triggering Game Over.
    *   Calls `timer_delay_ticks` (`1000:027A`) with `AX = 1` and loops back to `game_main_loop`.

---

## Key Subroutines Quick Cross-Reference

| Address | Label in `comic2_standalone.asm` | Actual Engine Role |
|---------|----------------------------------|--------------------|
| `1000:18E0` | `spr_render_player` | `setup_render_viewport` — Centers camera (`ds:0x06C2`/`0x06C4`) on Comic and builds monster slots (`1000:49AD`) |
| `1000:1B8D` | `map_load_room` | `load_resource` — Streams stage `.0` (RLE tiles), `.1` (room tilemaps), `.2` (graphics into `cs:0x005E`), `.3` (XOR-0x25 text/script) |
| `1000:1CFE` | `map_get_tile_at` | `get_tile_at_pixels(AX=x, BX=y)` — Returns tile ID in `AX` and pointer in `BX` |
| `1000:1D2C` | `map_draw_background` | `gfx_render_viewport_4plane` — Copies visible tilemap window from `SEG_BUFFER_1..4` to active VRAM page |
| `1000:2A10` | `player_update_physics` | `update_player_physics` — Jump/gravity physics and vertical/horizontal collision |
| `1000:2B8E` | `player_check_ground_collision` | `camera_update_y_follow_comic_clamped` — Keeps `scroll_y` (`ds:0x06C4`) within `[comic_y - 0x50, comic_y - 0x30]` |
| `1000:2BDC` | `player_check_ceiling_collision` | `move_player_left` — Grounded leftward movement wrapper |
| `1000:2BF7` | `player_check_wall_collision_left` | `move_left_step_grounded` — Steps Comic left 8px, checks collision (`1000:2D61`), and scrolls camera left |
| `1000:2C39` | `player_check_wall_collision_right` | `move_player_right` — Grounded rightward movement wrapper |
| `1000:2C54` | `player_check_climbing` | `move_right_step_grounded` — Steps Comic right 8px, checks collision (`1000:2D61`), and scrolls camera right |
| `1000:2C9D` | `player_check_swimming` | `check_floor_collision` — Checks tiles below/above Comic against threshold `DI` and triggers tile swaps |
| `1000:2D06` | `player_move_left` | `handle_tile_trigger_range_1` — Checks if tile is in `[ds:0x06D6, ds:0x06D7]` and spawns transient tile change |
| `1000:2D31` | `player_move_right` | `handle_tile_trigger_range_2` — Checks if tile is in `[ds:0x06E2, ds:0x06E3]` and spawns transient tile change |
| `1000:2D61` | `player_do_jump` | `tile_check_horizontal_step_clear` — Tests lateral tile clearance against `ds:0x0292` / `ds:0x0294` |
| `1000:2DC6` | `player_stop_jump` | `handle_water_entry_or_hazard` — Transitions Comic into/out of water (`ds:0x089D`) |
| `1000:2EDC` | `player_process_controls` | `handle_swimming_movement` — Underwater physics and controls (`ds:0x089D == 1`) |
| `1000:3181` | `proj_spawn_burst` | `ent_spawn_transient_slot` — Allocates one of 6 transient 9-byte animation/tile-swap slots at `ds:0x08B2` |
| `1000:31EB` | `proj_update_all` | `ent_update_transient_slots` — Advances and renders the 6 transient slots at `ds:0x08B2` |
| `1000:327D` | `proj_check_monster_hit` | `player_cycle_mode_selection` — Cycles active mode `ds:0x0896` (0..3) and updates HUD icon |
| `1000:330F` | `ent_update_all` | `handle_jetpack_hover_mode` — State-5 levitation/jetpack movement (`ds:0x08A6 == 1`) |
| `1000:437B` | `ui_draw_text_string` | `ent_update_entities_in_viewport` — Draws active room entities (`ds:0x0700`) and handles item pickup / trigger contact |
| `1000:4B25` | `vid_flip_buffers` | `ent_update_object_behaviors` — Updates and renders up to 6 active 32-byte monster behavior slots at `cs:0x48ED` |
| `1000:5CEB` | `map_update_animated_tiles` | `spawn_player_projectile` — Spawns a player projectile into the table at `ds:0x08EE` |
| `1000:5D5F` | `map_redraw_dirty_tiles` | `update_projectiles` — Moves, collides, and renders player projectiles at `ds:0x08EE` |
| `1000:774E` | `vid_wait_vblank` | `gfx_present_and_flip_page` — Sets CRTC start address on retrace and toggles active page offset `cs:0x773F` |
| `1000:77A3` | `vid_clear_page` | `gfx_blit_sprite_masked_active_page` — Masked 4-plane sprite blitter (aligned or 4-pixel shifted) |
| `1000:7DBB` | `vid_blit_planar_block` | `draw_sprite_clipped` — Clips world (X, Y) against camera (`ds:0x06C2`, `ds:0x06C4`) and dispatches masked sprite blitter |

---

## Projectile System (`1000:5CEB` & `1000:5D5F`)

The projectile system manages player-fired blasts using a 6-slot table of 8-byte records at `ds:0x08EE` (active slot count limit in `ds:0x021C`, up to 6 based on Blastola Cola firepower).

### Projectile Slot Layout (8 bytes at `ds:0x08EE`):
- `+0x00` (word): `x` — World X position (`0xFFFF` = inactive)
- `+0x02` (word): `y` — World Y position
- `+0x04` (word): `vel_x` — Horizontal velocity (`-16` left, `+16` right)
- `+0x06` (word): `wave_idx` — Corkscrew vertical wave table index (into `ds:0x9A8C`)

### Update Cycle (`map_redraw_dirty_tiles` / `update_projectiles` at `1000:5D5F`):
1.  **Iterate Table**: Loops `ds:0x021C` times starting at `SI = 0x08EE`.
2.  **Activity Check**: Skips slots where `word [SI] == 0xFFFF`.
3.  **Movement**:
    - `X = X + vel_x` (`[SI + 4]`).
    - If corkscrew mode is unlocked (`word [cs:0x1018] == 0xFFFF`), applies vertical offset from the 8-entry table at `ds:0x9A8C` indexed by `[SI + 6]`.
4.  **Tile Collision (`map_get_tile_at` at `1000:1CFE`)**:
    - Checks tile ID at projectile center `(X + 3, Y + 3)`.
    - If `Tile_ID <= ds:0x0292` (solid/interactive tile threshold):
        - Calls `player_move_left` (`1000:2D06` — *actually `handle_tile_trigger_range_1`*) to trigger shootable switches/destructible blocks.
        - Draws impact sprite (`0xA4D4` in `cs:0x005E`) and deactivates projectile (`word [SI] = 0xFFFF`).
5.  **Viewport Despawning**:
    - If projectile `X - scroll_x` is outside `[0, 0xC8]` or `Y - scroll_y` is outside `[0, 0x98]`, deactivates projectile.
6.  **Rendering**:
    - If active and on-screen, draws flying projectile sprite (`0xA4A6` in `cs:0x005E`) via `vid_clear_page` (`1000:77A3` — masked sprite blitter).
    - Monster hit detection against active projectiles is performed inside `ent_render_all` (`1000:4FD0` - `1000:5030`).

---

## State Variables (`SEG_DATA` / `DS` Base = `0x24FE`, Physical `0x24FE0`)

Note: In the old IDA disassembly (`comic2.asm`), `DS` variables were often named by their linear physical address (e.g., `ds:0x06C6` + `0x24FE0` = `word_256A6`). In `comic2_standalone.asm` and `comic2_data.asm`, all `DS` variables are referenced by their direct segment offset (`[0x06c6]`).

| DS Offset | Old IDA Name | Label in `comic2_data.asm` / Role | Description |
|-----------|--------------|-----------------------------------|-------------|
| `ds:0x0210` | `word_251F0` | `ds_timer_ticks` / `spawn_x` | Saved player spawn X coordinate |
| `ds:0x0212` | `word_251F2` | `spawn_y` | Saved player spawn Y coordinate |
| `ds:0x021C` | `byte_251FC` | `comic_firepower` | Firepower level (max 6 active projectiles) |
| `ds:0x021D` | `byte_251FD` | `has_wand` | Mode 1 (Wand) unlocked flag |
| `ds:0x021E` | `byte_251FE` | `has_jetpack` | Mode 2 (Jetpack/Hover) unlocked flag |
| `ds:0x021F` | `byte_251FF` | `has_pickaxe` | Mode 3 (Pickaxe) unlocked flag |
| `ds:0x0220` | `byte_25200` | `quest_progress` | Main story/quest progression stage counter |
| `ds:0x0221` | `byte_25201` | `ds_current_room_id` / `clock_placed_flag` | Clock/event flag |
| `ds:0x0222` | `word_25202` | `jetpack_fuel_bcd` | Packed BCD counter for Mode 2 energy/fuel (drawn at HUD Y=0x90) |
| `ds:0x0224` | `word_25204` | `comic_gems_bcd` | Packed BCD counter for gems/crystals (drawn at HUD Y=0x77; 100 = extra life) |
| `ds:0x0239` | `unk_25219` | `inventory_slots` | 13-word inventory array (`0xFFFF` = empty, else index into `cs:0x0AF0`) |
| `ds:0x028F` | `word_2526F` | `comic_num_lives_bcd` | Packed BCD counter for remaining lives (drawn at HUD Y=0xA9) |
| `ds:0x0292` | `word_25272` | `tile_solid_threshold` | Max tile ID considered solid wall/ceiling |
| `ds:0x0294` | `word_25274` | `tile_floor_threshold` | Max tile ID considered solid floor |
| `ds:0x0296` | `word_25276` | `tile_hazard_threshold` | Max tile ID considered water/hazard range |
| `ds:0x0298` | `word_25278` | `room_width_tiles` | Current room width in 16x16 tiles |
| `ds:0x029A` | `word_2527A` | `room_height_tiles` | Current room height in 16x16 tiles |
| `ds:0x029C` | `word_2527C` | `room_width_pixels` | `room_width_tiles * 16` |
| `ds:0x029E` | `word_2527E` | `room_height_pixels` | `room_height_tiles * 16` |
| `ds:0x02A0` | `unk_25280` | `tilemap_row_ptrs` | Table of row pointers into `ds:0x09A8` tilemap buffer |
| `ds:0x06C2` | `word_256A2` | `ds_scroll_x` | Camera viewport X origin in world pixels |
| `ds:0x06C4` | `word_256A4` | `ds_scroll_y` | Camera viewport Y origin in world pixels |
| `ds:0x06C6` | `word_256A6` | `comic_x` | Player X world coordinate in pixels |
| `ds:0x06C8` | `word_256A8` | `comic_y` | Player Y world coordinate in pixels |
| `ds:0x06FA` | `word_256DA` | `current_stage` | Current stage/area ID (`0..14`, loads `fr000.*`..`fr014.*`) |
| `ds:0x06FC` | `word_256DC` | `current_subroom` | Current room index within the loaded stage |
| `ds:0x06FE` | `word_256DE` | `active_entity_count` | Number of active records in `ds:0x0700` |
| `ds:0x0700` | `unk_256E0` | `active_entity_list` | Array of 8-byte active room objects/pickups |
| `ds:0x0890` | `word_25870` | `inventory_cursor` | Selected slot index (`0..12`) in inventory bar |
| `ds:0x0892` | `word_25872` | `comic_x_vel` | Player horizontal velocity (`-5..+5`) |
| `ds:0x0894` | `word_25874` | `comic_y_vel` | Player vertical velocity |
| `ds:0x0896` | `byte_25876` | `active_action_mode` | Active tool/weapon mode (`0`=Blast, `1`=Wand, `2`=Jetpack, `3`=Pickaxe) |
| `ds:0x0897` | `byte_25877` | `comic_is_physics_active` | 1 when jumping or falling |
| `ds:0x0898` | `byte_25878` | `comic_jump_counter` | Remaining ticks for variable-height jump boost |
| `ds:0x089D` | `byte_2587D` | `comic_is_swimming` | 1 when underwater / swimming physics active |
| `ds:0x08A3` | `byte_25883` | `comic_is_action_active` | 1 during Mode 1 (Wand) animation |
| `ds:0x08A6` | `byte_25886` | `comic_is_hover_active` | 1 during Mode 2 (Jetpack/Hover) flight |
| `ds:0x08A7` | `byte_25887` | `comic_is_attack_active` | 1 during Mode 3 (Pickaxe) animation |
| `ds:0x08AD` | `byte_2588D` | `comic_facing` | `1` = Right, `2` = Left |
| `ds:0x08AE` | `byte_2588E` | `comic_state` | `0`=Idle, `1`=Walk, `2`=Jump/Fall, `3`=Swim, `4`=Wand, `5`=Hover, `6`=Pickaxe |
| `ds:0x08AF` | `byte_2588F` | `comic_anim_frame` | Cycling animation frame index (`1..3`) |
| `ds:0x08B1` | `byte_25891` | `transient_slot_count` | Number of active transient effects in `ds:0x08B2` (max 6) |
| `ds:0x08B2` | `unk_25892` | `transient_slots` | 6 × 9-byte transient tile-swap / explosion effect slots |
| `ds:0x08E8` | `word_258C8` | `ds_saved_sp` | Saved stack pointer for clean room/death transitions |
| `ds:0x08EA` | `byte_258CA` | `c_meter_counter` | Blaster C-energy meter level (`0..23`) |
| `ds:0x08EB` | `byte_258CB` | `star_meter_counter` | Shield `*-`meter visual counter (`0..12`) |
| `ds:0x08EC` | `byte_258CC` | `death_anim_timer` | Non-zero during player death/teleport animation |
| `ds:0x08ED` | `byte_258CD` | `comic_hp` | Player shield/health points (max 12) |
| `ds:0x08EE` | `unk_258CE` | `projectile_table` | 6 × 8-byte player projectile slots |
| `ds:0x0934` | `unk_25914` | `comic_score_bcd` | 3-word (6-digit) packed BCD score counter |
