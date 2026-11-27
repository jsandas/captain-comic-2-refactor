# Captain Comic 2 Reverse Engineering Guide

## Project Overview

This document tracks the reverse engineering architecture, data structures, and resource formats of **Captain Comic II: Fractured Reality** based on the complete, compilable decompilation in `comic2_standalone.asm` and `comic2_data.asm`.

### Source Files in `reference/comic2/`
- **[comic2_standalone.asm](file:///Users/hedge/workspace/captain-comic-2-refactor/reference/comic2/comic2_standalone.asm)** - 100% compilable 16-bit 8086 NASM assembly source for the code segment and MZ header (11,941 lines; assemble with `nasm -f bin comic2_standalone.asm -o comic2.exe`).
- **[comic2_data.asm](file:///Users/hedge/workspace/captain-comic-2-refactor/reference/comic2/comic2_data.asm)** - Disassembled data segments and embedded assets included at the end of `comic2_standalone.asm` (`CS:0xD5D0`, 3,069 lines, 141,040 bytes).
- **[unpacked.exe.test.c](file:///Users/hedge/workspace/captain-comic-2-refactor/reference/comic2/unpacked.exe.test.c)** - Ghidra C decompilation representation (9,357 lines).
- **Reference:** Captain Comic 1 disassembly from https://github.com/jsandas/comic-c

---

## Key Findings from Analysis

### Game Information
- **Title:** Captain Comic II - Fractured Reality, Version 1.5
- **Copyright:** Michael A. Denio, 1990
- **Architecture:** x86 16-bit real mode, multi-segment MZ executable (196,288 bytes unpacked)
- **Video Mode:** 320×200, 16-color EGA (BIOS Mode `0Dh`) with hardware page flipping (`A000:0000` and `A000:2000`)

### Common Patterns Between Comic 1 and Comic 2

#### 1. **Initialization Sequence (`entry` at `1000:0064`)**
```text
- Set DS = SEG_DATA (0x24FE)
- Disable PC speaker (port 0x61 &= 0xFC)
- Install interrupt handlers (sys_install_interrupts at 1000:09C9)
- Calibrate CPU loop speed against 1 tick of INT 08h (stored at cs:0x0062)
- Check for EGA adapter with 256KB VRAM (INT 10h AX=1200h BL=10h) and set Mode 0Dh
- Copy embedded player/UI graphics from SEG_STACK into CS/SEG_PALETTE
- Display startup screen, load KEYS.DEF, and prompt for keyboard/joystick setup (1000:02B4)
- Run intro cinematic (1000:7164) and enter main dispatcher (1000:5E52)
```

#### 2. **EGA Graphics Programming**
- **0x3C0/0x3C1** - Attribute Controller (16-color palette loading & vertical-retrace color cycling in INT 08h)
- **0x3C4/0x3C5** - Sequencer (Index 2 = Map Mask for plane write selection)
- **0x3CE/0x3CF** - Graphics Controller (Index 4 = Read Map Select for plane read selection)
- **0x3D4/0x3D5** - CRT Controller (Index 0Ch = Start Address High toggled between `0x00` and `0x20` for double buffering)
- **0x3DA** - Input Status Register 1 (Bit 3 = vertical retrace sync)

#### 3. **Interrupt Handlers (`sys_install_interrupts` at `1000:09C9`)**
- **INT 08h (`isr_timer_sound_int08` at `1000:0683`)** - 18.2 Hz hardware timer: divides by 2 to set `game_tick_flag` (`cs:0x0245`) at ~9.1 Hz, polls analog joystick when calibrated (`cs:0x0276 == 1`), advances PC speaker tone streams, and executes retrace-synchronized EGA palette-cycling scripts (`cs:0x0278`).
- **INT 09h (`isr_keyboard_int09` at `1000:05C2`)** - Raw keyboard scancode handler: updates directional/action key states (`cs:0x0258..0x025D`), latches last key press in `cs:0x0061`, and handles F1/F2 sound toggle. Chains to BIOS INT 09h when `cs:0x0060 == 1` (used during player name entry).
- **INT 03h (`snd_api_int03` at `1000:08C7`)** - Sound command API dispatcher (`int3`).
- **INT 15h (`isr_joystick_int15` at `1000:093A`)** - Custom `AH=84h` joystick polling hook reading port `0x201` using PIT channel 0 counter deltas.
- **INT 23h (`isr_ctrl_break_int23` at `1000:067C`)** - Clean exit on Ctrl-Break.

#### 4. **Sound System (`snd_api_int03` at `1000:08C7`)**
- PC speaker synthesis via PIT Channel 2 (ports `0x43` / `0x42`, frequency = `1,193,182 / divisor`) and port `0x61` bits 0-1.
- Command API (`INT 3`):
  - `AL=0`: Enable speaker gate (`cs:0x0248 = 1`)
  - `AL=1`: Play sound stream at `DS:BX` with priority `CL` (accepted if `CL >= cs:0x0249`)
  - `AL=2`: Disable/mute speaker gate (`cs:0x0248 = 0`)
  - `AL=3`: Stop active playback (`cs:0x0247 = 0`, `cs:0x0249 = 0`)
  - `AL=4`: Query playback active flag (returns `AX = cs:0x0247`)
- Stream format: repeating word pairs `(uint16_t pitch_divisor, uint16_t tick_duration)`, terminated by `pitch_divisor == 0`.
- Verified SFX stream offsets in `SEG_DATA` (`DS`):
  - `0x00DB`: Startup/title jingle
  - `0x93CC`: Room transition / Game Over jingle
  - `0x965E`: Hazard / water splash / transition header
  - `0x9676`: Water surface jump/exit
  - `0x968E`: Tile transformation (Wand / switch trigger)
  - `0x969A`: Player death
  - `0x96AE`: Overhead overlay icon pop
  - `0x96B6`: Game Over fanfare
  - `0x96DE` / `0x96F6`: Player jump / drop-through jump
  - `0x973C`: Room reveal transition
  - `0x9764` / `0x97CC`: Player teleport entry / exit
  - `0x9834` / `0x9894`: Transition wave / rising sprite event
  - `0x98E0` / `0x98FC`: Item pickup / inventory drop
  - `0x9904`: Extra life / boss defeated chime
  - `0x9A25` / `0x9A31` / `0x9A3D`: Monster destroyed / boss hit / player damaged
  - `0x9A80`: Fire player projectile

---

## Entity & World Data Structures

### 1. Player Character State (`SEG_DATA` / `DS`)
```text
ds:0x06C6 (comic_x)                 - World X coordinate in pixels (word)
ds:0x06C8 (comic_y)                 - World Y coordinate in pixels (word)
ds:0x06C2 (ds_scroll_x)             - Camera viewport X origin in pixels (word)
ds:0x06C4 (ds_scroll_y)             - Camera viewport Y origin in pixels (word)
ds:0x0892 (comic_x_vel)             - Horizontal velocity (-5..+5 px/tick, word)
ds:0x0894 (comic_y_vel)             - Vertical velocity (gravity +5/tick, max 23 px/tick, word)
ds:0x0896 (active_action_mode)      - Selected mode: 0=Blaster, 1=Wand, 2=Jetpack, 3=Pickaxe (byte)
ds:0x0897 (comic_is_physics_active) - 1 when jumping/falling (byte)
ds:0x0898 (comic_jump_counter)      - Remaining ticks of jump boost (byte)
ds:0x089D (comic_is_swimming)       - 1 when swimming underwater (byte)
ds:0x08A3 (comic_is_action_active)  - 1 during Mode 1 (Wand) animation (byte)
ds:0x08A6 (comic_is_hover_active)   - 1 during Mode 2 (Jetpack/Hover) flight (byte)
ds:0x08A7 (comic_is_attack_active)  - 1 during Mode 3 (Pickaxe) animation (byte)
ds:0x08AD (comic_facing)            - 1=Right, 2=Left (byte)
ds:0x08AE (comic_state)             - 0=Idle, 1=Walk, 2=Jump, 3=Swim, 4=Wand, 5=Hover, 6=Pickaxe (byte)
ds:0x08AF (comic_anim_frame)        - Walk/action frame cycle 1..3 (byte)
ds:0x08EA (c_meter_counter)         - Blaster C-energy meter (0..23, byte)
ds:0x08EB (star_meter_counter)      - Shield *-meter visual counter (0..12, byte)
ds:0x08ED (comic_hp)                - Shield health points (max 12, byte)
ds:0x028F (comic_num_lives_bcd)     - Packed BCD lives counter (word, drawn at Y=0xA9)
ds:0x021C (comic_firepower)         - Blastola Cola firepower level (0..6 active shots, byte)
ds:0x0222 (jetpack_fuel_bcd)        - Packed BCD jetpack fuel counter (word, drawn at Y=0x90)
ds:0x0224 (comic_gems_bcd)          - Packed BCD gems counter (word, drawn at Y=0x77, 100 = extra life)
ds:0x0934 (comic_score_bcd)         - 3-word (6-digit) packed BCD total score
```

### 2. Mapped World Object Table (12-byte records at `CS:0x0AF0`, count at `ds:0x0940`)
Populated into the active room list by `map_spawn_room_entities` (`1000:1A63`) and persisted in savegame files (`GAME.000`..`GAME.003` via `1000:633C`).

```text
Offset  Size    Description
+0x00   word    stage_id (matches ds:0x06FA; 0xFFFF = collected/removed)
+0x02   word    subroom_id (matches ds:0x06FC)
+0x04   word    x (world X in pixels)
+0x06   word    y (world Y in pixels)
+0x08   word    sprite_ptr (offset in SEG_PALETTE / cs:0x005E)
+0x0A   word    type_and_flags (bit 15 = non-collectible/static until triggered; low byte = pickup type index into cs:0x4367 or >= 0x80 for inventory item)
```

### 3. Active Room Entity List (8-byte records at `ds:0x0700`, count at `ds:0x06FE`)
Built on room entry by `map_spawn_room_entities` (`1000:1A63`) and scanned each frame by `ui_draw_text_string` / `ent_update_entities_in_viewport` (`1000:437B`).

```text
Offset  Size    Description
+0x00   word    x (0xFFFF = inactive/collected)
+0x02   word    y
+0x04   word    mapped_index_and_flag (bits 0..14 = index in CS:0x0AF0; bit 15 = static/unrevealed flag)
+0x06   word    sprite_ptr (offset in SEG_PALETTE / cs:0x005E)
```

### 4. Stage Monster Spawn Descriptors (12-byte records in `CS`, pointed to by `ds:0x06F8`, count at `ds:0x06F6`)
```text
Offset  Size    Description
+0x00   word    spawn_x (initial world X)
+0x02   word    spawn_y (initial world Y)
+0x04   word    archetype_ptr (offset in CS to 16-byte monster archetype record)
+0x06   word    spawn_state (low byte: 0=ready, 1=active in slot, 2=destroyed/cooldown; high byte: respawn timer mask 0x0700)
+0x08   word    current_x (runtime/persisted world X)
+0x0A   word    current_y (runtime/persisted world Y)
```

### 5. Active Monster Behavior Slots (6 × 32-byte records at `CS:0x48ED`)
Populated by `spr_render_monster` (`1000:49AD`) & `spr_get_monster_frame` (`1000:4A70`), and ticked every frame by `vid_flip_buffers` (`1000:4B25`) & `ent_render_all` (`1000:4F15`).

```c
typedef struct {
    /* First 16 bytes copied directly from CS:archetype_ptr */
    uint16_t hitbox_w;          // +0x00: Width in pixels
    uint16_t hitbox_h;          // +0x02: Height in pixels
    uint16_t type_flags;        // +0x04: Low byte = contact damage/score; High byte = type (0x01=boss, 0x7B..0x7F=platforms/springs, 0x80=NPC script, 0x8000=invulnerable to shots)
    uint16_t behavior_state;    // +0x06: 0 = inactive; 1..33 = index into AI jump table at CS:0x4AE3
    uint16_t anim_base_or_len;  // +0x08: Frame count when stationary, or moving frame base offset
    uint16_t anim_period;       // +0x0A: Frame count when moving
    uint16_t frames_left_ptr;   // +0x0C: CS pointer to word table of sprite offsets (facing 2 / left)
    uint16_t frames_right_ptr;  // +0x0E: CS pointer to word table of sprite offsets (facing 1 / right)
    /* Runtime state fields (+0x10 .. +0x1F) */
    int16_t  x;                 // +0x10: Current world X
    int16_t  y;                 // +0x12: Current world Y
    int16_t  vel_x;             // +0x14: Horizontal velocity / step delta
    int16_t  vel_y;             // +0x16: Vertical velocity / step delta
    uint16_t facing_dir;        // +0x18: 1 = right, 2 = left
    uint16_t anim_tick;         // +0x1A: Current animation frame index
    int16_t  ai_substate;       // +0x1C: AI state counter / dialogue script step
    uint16_t descriptor_ptr;    // +0x1E: Back-pointer to 12-byte spawn descriptor in CS
} RuntimeMonsterSlot32;
```

### 6. Transient Tile-Swap / Explosion Slots (6 × 9-byte records at `ds:0x08B2`, active count at `ds:0x08B1`)
Managed by `proj_spawn_burst` (`1000:3181`) and `proj_update_all` (`1000:31EB`).
```text
Offset  Size    Description
+0x00   byte    effect_type (0=inactive, 1=tile transform with commit on frame 3, 2=explosion puff A, 3=explosion puff B)
+0x01   byte    new_tile_id (written to *[+0x02] when effect_type == 1 finishes)
+0x02   word    tilemap_ptr (pointer into ds:0x09A8 tilemap buffer)
+0x04   byte    anim_frame (0..2; slot finishes when incremented to 3)
+0x05   word    world_x
+0x07   word    world_y
```

---

## Inventory / Mode Selection System

Captain Comic 2 separates **Action Modes** (cycled live during gameplay) from **Inventory Objetos** (selected via a modal pause bar):

1. **Action Mode Cycling (`ds:0x0896`, handled by `proj_check_monster_hit` at `1000:327D`)**:
   - Pressing **Page Up / `'I'`** (`cs:0x0255` = `0x49`) or **Page Down / `'Q'`** (`cs:0x0256` = `0x51`) steps `ds:0x0896` by `-1` or `+1` across modes `0..3`:
     - `0`: **Blaster** (always available; fires projectiles using C-meter `ds:0x08EA` and Blastola Cola count `ds:0x021C`)
     - `1`: **Wand** (requires `ds:0x021D == 1`; transforms tiles in front of Comic)
     - `2`: **Jetpack / Levitation** (requires `ds:0x021E == 1`; enables state-5 hover flight `ds:0x08A6 = 1`, consuming BCD fuel `ds:0x0222`)
     - `3`: **Pickaxe** (requires `ds:0x021F == 1`; interacts with/uncovers mapped objects in front of Comic)
   - `hud_draw_item_icon` (`1000:3A17`) updates the HUD mode indicator icons at `X = 0x100`.

2. **Inventory Slot Bar (`ds:0x0239` & `ds:0x0890`, handled by `game_pause_handler` at `1000:46E6`)**:
   - When Comic touches a mapped world object (`cs:0x0AF0`) whose type byte is `>= 0x80`, `1000:437B` places its `cs:0x0AF0` index into the first free slot (`0xFFFF`) of the 13-word array at `ds:0x0239` and draws its icon in the bottom bar (`Y = 0xB1`) via `ui_draw_dialog_message` (`1000:451C`).
   - Pressing **Home / `'G'`** (`cs:0x0257` = `0x47`) enters `1000:46E6`, drawing a selection cursor at slot `ds:0x0890` (`0..12`):
     - **Left / Right** moves `ds:0x0890` across the 13 slots.
     - **Fire** drops/activates the highlighted item at Comic's current `(comic_x, comic_y + 16)` back into the active room entity list (`ds:0x0700`), marks the source `ds:0x0239` slot as `0xFFFF` (empty), and sets a 20-tick blink timer (`ds:0x08A8 = 0x14`). (Special case: dropping the Clock `0xB3D2` in Stage 11 Room 7 at `(0x250, 0x50)` sets `ds:0x0221 = 1` and displays `"YOU HEAR A STANGE WHIRLING NOISE AS YOU SET THE CLOCK DOWN."`).

---

## Resource Files & Decode Formats

### 1. Signed-RLE Stream (`file_load_pack_rle` at `1000:1FA4` & `ent_init_monsters` at `1000:2072`)
Used for `.0` tileset graphics and `.1` room tilemap layouts:
```c
while (1) {
    int8_t control = (int8_t)*src++;
    if (control == 0) break;                // NUL terminator
    if (control < 0) {                      // Repeated run
        uint8_t value = *src++;
        memset(dst, value, -control);
        dst += -control;
    } else {                                // Literal run
        memcpy(dst, src, control);
        src += control;
        dst += control;
    }
}
```

### 2. 4-Plane EGA Blit Stream (`vid_load_palette_buffer` at `1000:79C7` & `vid_apply_palette` at `1000:7A13`)
Used for `FRPAK.001`..`FRPAK.007` and fullscreen/overlay graphics in `SEG_PALETTE`:
- Stream starts with `uint16_t row_span_bytes` (e.g. `0x1F40` = 8,000 bytes/plane for 320×200 fullscreen).
- For each of the 4 EGA planes (`CL = 0..3`), decodes packets until `DI - start_DI >= row_span_bytes`:
  - `packet & 0x80 == 0`: Copy `packet` literal bytes from `DS:SI` to `ES:DI`.
  - `packet & 0x80 != 0`: Repeat next byte `packet & 0x7F` times into `ES:DI`.
- `1000:79C7` writes bytes directly (opaque); `1000:7A13` OR-composites bytes into VRAM.

### 3. Stage Asset Tuples (`fr000.*` .. `fr014.*` via `map_load_room` at `1000:1B8D`)
- `ds:0x93EC` is a 15-entry word pointer table indexed by `stage_id` (`AX = 0..14`).
- Each entry points to an 8-byte tuple of 4 filename pointers in `ds_room_file_table` (`ds:0x9482`, line 2846 in `comic2_data.asm`):
  1. **`fr0XX.0`** (`file_load_pack_rle` at `1000:1FA4`):
     - First 6 bytes are read directly into `ds:0x0292` (solid wall threshold), `ds:0x0294` (solid floor threshold), and `ds:0x0296` (water/hazard threshold).
     - Remainder is signed-RLE decompressed into `ds:0x13A8` (128 bytes per 16×16 tile: 4 planes × 32 bytes).
  2. **`fr0XX.1`** (`file_load_raw_mask` at `1000:1FFE` & `ent_init_monsters` at `1000:2072`):
     - Loaded into `SEG_SCRATCH:0x0600`.
     - Header at `+0x0002` is `uint16_t stage_id` (must match `AX`).
     - Subroom directory starts at `+0x0004`: array of 6-byte entries indexed by `subroom_id` (`BX`):
       - `+0x00`: `uint16_t tile_w` (stored to `ds:0x0298`, pixel width `tile_w * 16` -> `ds:0x029C`)
       - `+0x02`: `uint16_t tile_h` (stored to `ds:0x029A`, pixel height `tile_h * 16` -> `ds:0x029E`)
       - `+0x04`: `uint16_t rle_data_off` (offset from `0x0600` to signed-RLE tilemap stream, unpacked into `ds:0x09A8`, with row pointers built at `ds:0x02A0`).
  3. **`fr0XX.2`** (`file_load_tileset` at `1000:2022`):
     - Raw planar sprite bank loaded at `SEG_PALETTE:0x0000` (`[cs:0x005E]`) containing monster sprites, items, and animations for the stage.
  4. **`fr0XX.3`** (`file_load_entity_data` at `1000:2049`, optional — pointer is `0x0000` if stage has no dialogue file):
     - Present for stages `1, 3, 4, 5, 6, 13`.
     - Reads `0x0C00` (3,072) bytes into `ds:0x9EC0` and XORs every byte with `0x25`. Contains word offset table followed by framed dialogue streams (`[width, height, chars...]`) used by `game_execute_script` (`1000:6806`).

### 4. Sprite Record Formats in `SEG_PALETTE` (`[cs:0x005E]`)
```c
typedef struct {
    uint16_t width_pixels;      // Divided by 8 for byte width per row
    uint16_t height_rows;
    /* Followed by 4 plane blocks of (width_pixels / 8) * height_rows bytes */
} OpaqueSpriteRecord;

typedef struct {
    uint16_t width_pixels;
    uint16_t height_rows;
    uint16_t mask_size_bytes;   // Offset from end of header (SI+6) to 4-plane color data
    /* 1-plane AND-mask bytes ((width_pixels / 8) * height_rows) start at +0x06 */
    /* 4-plane OR-image bytes start at +0x06 + mask_size_bytes */
} MaskedSpriteRecord;
```

---

## Key Code Landmarks in `comic2_standalone.asm` & `comic2_data.asm`

| File & Line | Address / Symbol | Purpose |
|-------------|------------------|---------|
| `comic2_standalone.asm:14` | `mz_header` | MS-DOS MZ header & 30 relocation entries |
| `comic2_standalone.asm:93` | `entry` (`1000:0064`) | Main executable entry point |
| `comic2_standalone.asm:337` | `game_tick_flag` (`1000:0245`) | In-code hardware, audio, key & joystick state table |
| `comic2_standalone.asm:385` | `timer_delay_ticks` (`1000:027A`) | `wait_n_ticks` frame synchronizer |
| `comic2_standalone.asm:761` | `isr_keyboard_int09` (`1000:05C2`) | Hardware interrupt handlers (INT 09h, 23h, 08h, 03h, 15h) |
| `comic2_standalone.asm:1351` | `1000:0AF0` | Mapped world object table (12-byte records) |
| `comic2_standalone.asm:1581` | `spr_render_player` (`1000:18E0`) | Viewport camera centering & monster slot activation |
| `comic2_standalone.asm:1825` | `map_load_room` (`1000:1B8D`) | Stage `.0`/`.1`/`.2`/`.3` asset loader |
| `comic2_standalone.asm:2793` | `game_main_loop` (`1000:2341`) | Master gameplay loop and state dispatcher |
| `comic2_standalone.asm:3417` | `player_update_physics` (`1000:2A10`) | Player jumping, gravity, and collision |
| `comic2_standalone.asm:3905` | `player_process_controls` (`1000:2EDC`) | Swimming / underwater physics |
| `comic2_standalone.asm:4414` | `ent_update_all` (`1000:330F`) | Jetpack / hover mode physics |
| `comic2_standalone.asm:5884` | `ui_draw_text_string` (`1000:437B`) | Active room entity rendering & pickup collision |
| `comic2_standalone.asm:6299` | `spr_render_monster` (`1000:49AD`) | Monster runtime slot builder & 33-state AI engine (`1000:4B25`) |
| `comic2_standalone.asm:7169` | `map_update_animated_tiles` (`1000:5CEB`) | Player projectile spawner & updater (`1000:5D5F`) |
| `comic2_standalone.asm:7330` | `sys_main_dispatcher` (`1000:5E52`) | Top-level game selection, save/load, and copy-protection flow |
| `comic2_standalone.asm:7911` | `game_execute_script` (`1000:6806`) | NPC interaction script & dialogue dispatcher |
| `comic2_standalone.asm:8858` | `ui_story_screen` (`1000:7164`) | Scripted intro cinematic sequence |
| `comic2_standalone.asm:9360` | `vid_init_ega_regs` (`1000:7741`) | Low-level EGA blitters, RLE decoders, and page flipping |
| `comic2_data.asm:12` | `seg_scratch_start` (`0x0D5D`) | Calibration text & 34,000-byte decompression buffer |
| `comic2_data.asm:36` | `seg_stack_start` (`0x160A`) | Stack segment & embedded core sprite/font bitmaps |
| `comic2_data.asm:1909` | `seg_palette_start` (`0x2206`) | EGA palettes & RLE title/intro streams |
| `comic2_data.asm:2677` | `seg_data_start` (`0x24FE`) | Main `DS` game state, filenames, sound effects, and buffers |

---

*Last Updated: 2026-11-26*
*Status: Synchronized with `comic2_standalone.asm` and `comic2_data.asm`*
