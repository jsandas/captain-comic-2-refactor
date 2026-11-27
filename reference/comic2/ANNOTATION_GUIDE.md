# Quick Reference: Common Code Patterns in Captain Comic 2

## How to Identify Functions

When you see these patterns in `comic2_standalone.asm` (and `comic2_data.asm`), they correspond to these functions:

### 1. Game Tick Wait Pattern
```asm
timer_delay_ticks:
    cmp  byte [cs:0x245], 0x1    ; game_tick_flag (1000:0245)
    jnz  timer_delay_ticks
    mov  byte [cs:0x245], 0x0
    dec  AX
    jnz  timer_delay_ticks
    ret
```
**→ This is `timer_delay_ticks` (1000:027A at line 385 in `comic2_standalone.asm`)**

### 2. EGA Plane Write Setup
```asm
mov  AL, 0x2           ; Map Mask register
mov  DX, 0x3C4         ; Sequencer Index
out  DX, AL
inc  DX
xchg AL, AH            ; Plane mask: 1, 2, 4, or 8
out  DX, AL
```
**→ Part of EGA register & blitting functions (`vid_set_write_mode_0` at 1000:4339, `vid_out_dx_ax` at 1000:7A68)**

### 3. EGA Plane Read Setup
```asm
mov  AH, CL            ; Plane index: 0-3
mov  AL, 0x4           ; Read Map Select
mov  DX, 0x3CE         ; Graphics Controller Index
out  DX, AL
inc  DX
xchg AL, AH
out  DX, AL
```
**→ Combined with write plane setup in `vid_out_dx_ax` (1000:7A68 at line 9917)**

### 4. Vertical Retrace Sync
```asm
mov  DX, 0x3DA         ; Input Status Register 1
loc_7aa5:
  in   AL, DX
  test AL, 0x8         ; Check VR bit
  jz   loc_7aa5
```
**→ Video buffer swap sync (`vid_in_status` at 1000:7A89, called by `vid_wait_vblank` at 1000:774E)**

### 5. File Open/Read/Close
```asm
mov  AX, 0x3D00        ; Open file (read-only)
int  0x21
jnc  .ok
jmp  near sys_fatal_error
.ok:
mov  BX, AX            ; Save handle
mov  AX, 0x3F00        ; Read
int  0x21
mov  AX, 0x3E00        ; Close
int  0x21
```
**→ Resource loading functions (`file_load_pack_rle` at 1000:1FA4, `file_load_raw_mask` at 1000:1FFE, `file_load_tileset` at 1000:2022, `file_load_entity_data` at 1000:2049, `vid_set_overscan_color` at 1000:79A2)**

### 6. Joystick Reading
```asm
loc_096f:
    cli                          ; Disable interrupts
    call loc_09af                ; Latch PIT counter
    push AX
    out  DX, AL                  ; Trigger joystick (port 0x201)
    mov  CX, word [cs:0x62]      ; Timeout from CPU speed calibration
loc_097a:
    in   AL, DX
    test AL, BL
    loopnz loc_097a
    ...
    sti                          ; Re-enable interrupts
```
**→ Joystick input (`isr_joystick_int15` at 1000:093A / `loc_096f` at line 1168)**

### 7. PC Speaker Sound
```asm
in   AL, 0x61          ; Read PPI port B
and  AL, 0xFC          ; or `or AL, 0x03` to manipulate speaker bits 0-1
out  0x61, AL
```
**→ Sound control (`snd_api_int03` at 1000:08C7, `isr_timer_sound_int08` at 1000:0683, and speaker disable in `entry` at 1000:0064)**

### 8. PIT Counter Read
```asm
loc_09af:
    mov  AL, 0x0
    out  0x43, AL      ; Latch command
    jmp  short $+2
    jmp  short $+2
    in   AL, 0x40      ; Low byte
    mov  AH, AL
    jmp  short $+2
    jmp  short $+2
    in   AL, 0x40      ; High byte
    xchg AH, AL
    ret
```
**→ Timing/joystick axis counter read (`loc_09af` at 1000:09AF, line 1208)**

## Key Memory Locations

### Segments (`comic2_standalone.asm` & `comic2_data.asm`)
```text
CS (Code)       = 0x0000 (relocated to 0x1000 at runtime; offsets 0x0000..0xD5CF)
SEG_SCRATCH     = 0x0D5D (relocated 0x1D5D) - Decompression scratchpad & UI prompts
SEG_STACK       = 0x160A (relocated 0x260A) - Runtime stack (initial SP = 0x07D0)
SEG_PALETTE     = 0x2206 (relocated 0x3206) - EGA 16-color palette tables & RLE streams
SEG_DATA (DS)   = 0x24FE (relocated 0x34FE) - Primary game data, strings, & state
SEG_BUFFER_1..4 = 0x2FCC, 0x37CC, 0x3FCC, 0x47CC - Offscreen tile/sprite plane buffers
```

### In-Code Hardware & Input State Table (`CS:0245` - `CS:027A`, lines 337-378)
```text
cs:0x0060                    - Pass-through flag to chain original INT 09h BIOS handler
cs:0x0061                    - Last raw key-down scancode captured by INT 09h
cs:0x0062                    - CPU speed calibration count (computed in entry)
cs:0x0229 / cs:0x022a        - Retrace palette-cycling script latch and active flags
cs:0x0245 (game_tick_flag)   - Set to 1 by INT 08h every 2nd IRQ0 tick (~9.1 Hz game tick)
cs:0x0246 (irq0_parity)      - Divides 18.2 Hz IRQ0 by 2
cs:0x0247 (sound_is_playing) - 1 when PC speaker stream is actively playing
cs:0x0248 (sound_is_enabled) - 1 when sound gate is enabled (F1=on, F2=off)
cs:0x0249 (sound_priority)   - Priority of current active sound effect
cs:0x024a (sound_data_offset)- Current word pointer in active sound stream
cs:0x024c (sound_note_counter)- Remaining ticks for current note
cs:0x024e (sound_data_segment)- Segment of active sound stream
```

### Key Mappings (`CS:0250` - `CS:0269`, lines 345-370)
```text
cs:0x0250 (scancode_esc)     - Esc = 01h
cs:0x0251-0254 (scancode_f1..f4) - F1-F4 function keys (3Bh, 3Ch, 3Dh, 3Eh)
cs:0x0255 (scancode_pgup)    - Page Up / 'I' = 49h (cycle mode/item selection backward)
cs:0x0256 (scancode_pgdn)    - Page Down / 'Q' = 51h (cycle mode/item selection forward)
cs:0x0257 (scancode_home)    - Home / 'G' = 47h (open inventory/slot selection menu)
cs:0x0258 (key_state_left)   - 1 while Left held
cs:0x0259 (key_state_right)  - 1 while Right held
cs:0x025a (key_state_up)     - 1 while Up held
cs:0x025b (key_state_down)   - 1 while Down held
cs:0x025c (key_state_jump)   - 1 while Jump held
cs:0x025d (key_state_fire)   - 1 while Fire held
cs:0x025e (map_scancode_left)- Left arrow = 4Bh (customizable via KEYS.DEF)
cs:0x025f (map_scancode_right)- Right arrow = 4Dh
cs:0x0260 (map_scancode_up)  - Up arrow = 48h
cs:0x0261 (map_scancode_down)- Down arrow = 50h
cs:0x0262 (map_scancode_jump)- Space = 39h
cs:0x0263 (map_scancode_fire)- 'R' / Ins = 52h
```

### Inventory / Mode Selection
```text
ds:0x0890                   - Inventory slot cursor (0..12) used by the selection UI
ds:0x0896                   - Active mode index (0=none, 1=wand/fire, 2=jetpack/fly, 3=pickaxe)
proj_check_monster_hit      - (1000:327D) Adjusts the active mode index (ds:0x0896) when PgUp/PgDn (0x49/0x51) are pressed
game_pause_handler          - (1000:46E6) Entered when Home/'G' (0x47) is pressed; navigates inventory slots at ds:0x0239 and drops/activates the selected object into the room list at ds:0x0700
ui_draw_dialog_message      - (1000:451C) Resolves slot entry from ds:0x0239 into CS:0x0AF0 object record and draws icon or clears box
```

The inventory is not hard-coded as a fixed item order. The UI shows a fixed set of slots, but the contents come from runtime slot data (`ds:0x0239`) and mapped room/object tables (`cs:0x0AF0`).

### Interrupt Handlers (Installed in `sys_install_interrupts` at 1000:09C9)
```text
INT 08h (1000:0683 isr_timer_sound_int08) - Timer (18.2 Hz tick, joystick polling, PC speaker synth, palette script)
INT 09h (1000:05C2 isr_keyboard_int09)    - Keyboard scancodes
INT 03h (1000:08C7 snd_api_int03)         - Sound control API
INT 15h (1000:093A isr_joystick_int15)    - Joystick BIOS hook (AH=84h)
INT 23h (1000:067C isr_ctrl_break_int23)  - DOS Ctrl-Break clean exit hook
```

## Common Register Usage (Comic 1 & 2 patterns)

### Function Entry/Exit
```text
Caller-saved: AX, BX, CX, DX (may be trashed)
Callee-saved: SI, DI, BP (preserved when needed)
Stack grows down (SP decrements)
```

### Typical Parameter Passing
```text
AX      - Primary input/output, X coordinates, counts, sizes
BX      - Y coordinates, pointers, file handles, secondary input
CX      - Loop counters, widths, sizes, sound priority
DX      - Port I/O, heights, filename offsets (DS:DX)
SI      - Source pointer for sprite/tile/string ops
DI      - Destination pointer for VRAM (ES:DI) or entity slot pointers
DS      - Usually SEG_DATA (0x24FE), temporarily switched to CS (via cs:0x005E), SEG_SCRATCH, or SEG_PALETTE
ES      - Usually 0xA000 (EGA VRAM)
```

### Fixed-Point / Velocity Math
```text
Player X/Y coordinates (ds:0x06C6, ds:0x06C8) are in pixels (tiles are 16x16 pixels).
Horizontal velocity (ds:0x0892) is clamped to [-5, +5] pixels/tick.
Vertical velocity (ds:0x0894) has gravity +5/tick, terminal velocity 0x17 (23) pixels/tick.
```

## How to Annotate

### Step 1: Identify Function Purpose
Look at what it does:
- Does it touch ports `0x3C0`, `0x3C4`, `0x3CE`, `0x3D4`, `0x3DA`? → Video / EGA hardware
- Does it use `INT 0x21`? → File I/O or DOS system call
- Does it use `INT 0x10`? → BIOS video mode / teletype
- Does it execute `INT3` or manipulate port `0x61`? → Audio / PC Speaker
- Does it check `byte [cs:0x245]`? → Timing / frame sync

### Step 2: Check Existing Decompilation Headers
In `comic2_standalone.asm`, every function has a standardized header block:
```asm
; ----------------------------------------------------------------------------
; FUNCTION: <name>  [<Subsystem> / <Category>]
; ADDRESS:  1000:<offset>
; PURPOSE:  <description>
; ----------------------------------------------------------------------------
```
> **Note:** Some auto-generated function names in `comic2_standalone.asm` reflect early heuristic labels (e.g., `vid_clear_page` at `1000:77A3` is actually the masked sprite blitter, and `map_redraw_dirty_tiles` at `1000:5D5F` is actually the player projectile update loop). Always verify the instruction body and cross-reference `FUNCTION_MAP.md` and `CORE_ENGINE.md`!

## Quick Search Commands in Editor

### Find all EGA port usage:
```text
Search regex: (in|out).*0x3[cd][0-9a-f]
```

### Find all INT calls:
```text
Search regex: (int\s+0x[0-9a-f]+|int3)
```

### Find all function header blocks:
```text
Search regex: ^; FUNCTION:
```

### Find all references to a DS variable (e.g. player X at 0x6c6):
```text
Search regex: \[0x6c6\]
```

## Important Code Landmarks

### `comic2_standalone.asm`
- Lines 13-62: MS-DOS MZ Executable Header & relocation table (`section .header`)
- Lines 69-76: Segment definitions (`SEG_DATA`, `SEG_STACK`, `SEG_PALETTE`, `SEG_SCRATCH`, `SEG_BUFFER_1..4`)
- Lines 78-83: Copyright banner (`Captain Comic II - Fractured Reality, Version 1.5`)
- Line 93: `entry` (`1000:0064`) - Main entry point
- Line 337: In-code hardware & input state table (`1000:0245` - `1000:027A`)
- Line 385: `timer_delay_ticks` (`1000:027A`)
- Line 436: `vid_init_screen_pages` (`1000:02B4`) - Startup graphics & key/joystick setup
- Line 761: `isr_keyboard_int09` (`1000:05C2`), `isr_timer_sound_int08` (`1000:0683`), `snd_api_int03` (`1000:08C7`), `isr_joystick_int15` (`1000:093A`)
- Line 1232: `sys_install_interrupts` (`1000:09C9`) & `sys_restore_all_interrupts` (`1000:0A5E`)
- Line 1351: Mapped world object table in CS (`1000:0AF0`, 12-byte records)
- Line 1581: `spr_render_player` (`1000:18E0`) - Camera viewport setup & monster slot activation
- Line 1825: `map_load_room` (`1000:1B8D`) - Level/room asset loader
- Line 2793: `game_main_loop` (`1000:2341`) - Master gameplay loop
- Line 3417: `player_update_physics` (`1000:2A10`) - Grounded/jumping physics
- Line 6299: `spr_render_monster` (`1000:49AD`) & `vid_flip_buffers` (`1000:4B25`) - Monster runtime slot builder & behavior update loop
- Line 7169: `map_update_animated_tiles` (`1000:5CEB`) & `map_redraw_dirty_tiles` (`1000:5D5F`) - Projectile spawn & update loop
- Line 7330: `sys_main_dispatcher` (`1000:5E52`) - Top-level menu/game flow dispatcher
- Line 9360: Low-level EGA blitters & page management (`1000:7741` - `1000:7E49`)
- Line 10535: Per-stage descriptor & tile-trigger tables (`1000:7E4A` - `1000:D5CF`)
- Line 11941: `%include "comic2_data.asm"`

### `comic2_data.asm`
- Line 12: `seg_scratch_start` (`SEG_SCRATCH` `0x0D5D`) - Joystick/keyboard calibration prompts & 34,000-byte decompression buffer (`0x0600`)
- Line 36: `seg_stack_start` (`SEG_STACK` `0x160A`) - Runtime stack & embedded 8x8 font/sprite data (`0x0D2D0`)
- Line 1909: `seg_palette_start` (`SEG_PALETTE` `0x2206`) - EGA palette tables & RLE graphics streams
- Line 2677: `seg_data_start` (`SEG_DATA` `0x24FE`) - Main data segment (`DS:0x0000` - `DS:0xACDF`)

## Validation Checklist

When you think you've identified or verified a function, check:

- [ ] Does the instruction body match the header description? (Beware of heuristic misnomers!)
- [ ] Do the registers and DS/CS segment overrides make sense for this operation?
- [ ] Are there similar calls elsewhere with similar parameters?
- [ ] Does it fit in the overall program flow?
- [ ] Can you trace where it's called from?

## Annotation Confidence Legend

Use this confidence rating in function headers and mapping notes:

- **High**
    - Multiple independent signals agree (instruction pattern, call-site behavior, register usage, and side effects).
    - No major contradictions in control flow.
- **Medium**
    - Core behavior is clear, but one or two signals are incomplete or indirect.
    - Naming is likely correct but still needs broader call-site verification.
- **Low**
    - Purpose is plausible but evidence is sparse, conflicting, or local-only.
    - Keep provisional naming (`UNKNOWN_*` or subsystem-prefixed placeholder).

---
*This guide assumes familiarity with x86 assembly and DOS programming.*
*Refer to [FUNCTION_MAP.md](file:///Users/hedge/workspace/captain-comic-2-refactor/reference/comic2/FUNCTION_MAP.md) and [CORE_ENGINE.md](file:///Users/hedge/workspace/captain-comic-2-refactor/reference/comic2/CORE_ENGINE.md) for detailed function locations.*
