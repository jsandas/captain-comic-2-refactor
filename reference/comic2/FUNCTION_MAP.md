# Captain Comic 2 Function Map
# Cross-reference between `comic2_standalone.asm` / `comic2_data.asm` and verified engine behavior

> **About Labels in `comic2_standalone.asm`:**
> `comic2_standalone.asm` is a 100% compilable 16-bit NASM assembly source that builds the exact original `comic2.exe`. Some labels in `comic2_standalone.asm` were generated from an early heuristic pass where the label name disagrees with the actual instruction body (for example, `vid_clear_page` at `1000:77A3` is the masked 4-plane sprite blitter, and `map_redraw_dirty_tiles` at `1000:5D5F` is the player projectile update loop).
>
> Below, each entry lists:
> - **Address & Line** in `comic2_standalone.asm`
> - **Standalone ASM Label** (exact symbol in `comic2_standalone.asm`)
> - **Verified Role / Semantic Name** (what the code actually does, plus old `sub_XXXX` reference)

---

## Core System Functions

### Initialization, Lifecycle, and Timing
```text
1000:0064 (Line   93) entry                        → Main entry point: CPU speed calibration (cs:0x0062), EGA 256K check, Mode 0Dh init
1000:0202 (Line  289) sys_fatal_error              → Restore text mode, print fatal error string at DS:0x0046, and exit to DOS
1000:0215 (Line  304) sys_exit_to_dos              → Restore interrupt vectors, restore initial video mode, print BETA banner, INT 21h AH=4Ch
1000:027A (Line  385) timer_delay_ticks            → wait_n_ticks(AX) - synchronous delay against cs:0x0245 (game_tick_flag)
1000:028C (Line  397) sys_print_string             → Print NUL-terminated string at DS:BX via BIOS INT 10h AH=0Eh
1000:029F (Line  416) sys_print_xor_string         → Decode and print XOR-0x25 string at DS:BX via BIOS INT 10h AH=0Eh
1000:09C9 (Line 1232) sys_install_interrupts       → Install custom INT 09h, 08h, 23h, 03h, and 15h handlers
1000:0A5E (Line 1285) sys_restore_all_interrupts   → Restore original INT 09h, 08h, 23h, 03h, and 15h vectors
1000:0ABC (Line 1324) sys_detect_cpu               → Detect CPU generation (0=8086/8088, 1=80286, 2=80386+) via FLAGS bits 12-15
1000:5E52 (Line 7330) sys_main_dispatcher          → Seed PRNG from DOS time, run copy-protection hash, load FR.CFG, run game selection menu
```

### Interrupt Handlers
```text
1000:05C2 (Line  761) isr_keyboard_int09           → INT 09h handler: reads scancode from port 60h, updates key_state_* matrix (cs:0x0258..0x025D) and last key cs:0x0061
1000:067C (Line  857) isr_ctrl_break_int23         → INT 23h handler: jumps to sys_exit_to_dos
1000:0683 (Line  866) isr_timer_sound_int08        → INT 08h handler (18.2 Hz): divides tick by 2 for game_tick_flag (cs:0x0245), polls joystick if calibrated, drives PC speaker stream, runs retrace EGA palette script (cs:0x0278), chains original INT 08h
1000:08C7 (Line 1083) snd_api_int03                → INT 03h sound command dispatcher (AL=0..4)
1000:093A (Line 1137) isr_joystick_int15           → INT 15h (AH=84h) joystick handler: reads buttons (DL=0) or discharge timing via PIT counter (DL=1)
```

### Input System & Calibration
```text
1000:02EA (Line  456) file_load_keys_def_and_prompt → Load KEYS.DEF into cs:0x025E..0x0263 and wait for Title prompt key ('K'=keys, 'J'=joystick, Esc=exit)
1000:033C (Line  492) input_calibrate_joystick     → Interactive text-mode joystick center/axis calibration (stores thresholds in cs:0x026E..0x0276)
1000:0424 (Line  588) input_wait_joystick_or_key   → Wait for joystick button press+release transition or keyboard abort during calibration
1000:045C (Line  616) input_setup_keyboard         → Interactive text-mode key remapping UI and optional save to KEYS.DEF
1000:054B (Line  701) input_read_mapped_key        → Capture unique key scancode (rejecting duplicates) and print key name from SEG_SCRATCH:0x035F
1000:0599 (Line  738) sys_flush_keyboard_buffer    → (also labeled sys_restore_interrupts) Wait for next key-down scancode in cs:0x0061 and flush BIOS key buffer (0040:001A)
1000:096F (Line 1168) loc_096f                     → read_joystick_axis: time single joystick axis discharge on port 201h using PIT channel 0
1000:09AF (Line 1208) loc_09af                     → pit_count: latch and read 16-bit PIT channel 0 counter (ports 43h/40h)

In-Code Input & Hardware Variables (CS:0x0245 - CS:0x027A, Lines 337-378):
cs:0x0060                  → Chain-to-BIOS flag for INT 09h (1 during text entry)
cs:0x0061                  → Last key-down scancode
cs:0x0062                  → CPU speed calibration loop limit
cs:0x0245 game_tick_flag   → Set to 1 by INT 08h every 2nd timer tick
cs:0x0246 irq0_parity      → Toggles 0/1 each INT 08h tick
cs:0x0250 scancode_esc     → Esc (01h)
cs:0x0251..0254 scancode_f1..f4 → Function keys F1-F4 (3Bh, 3Ch, 3Dh, 3Eh; F1=sound on, F2=sound off)
cs:0x0255 scancode_pgup    → Page Up / 'I' (49h) - cycle mode backward
cs:0x0256 scancode_pgdn    → Page Down / 'Q' (51h) - cycle mode forward
cs:0x0257 scancode_home    → Home / 'G' (47h) - open inventory slot selector
cs:0x0258 key_state_left   → 1 if Left pressed
cs:0x0259 key_state_right  → 1 if Right pressed
cs:0x025A key_state_up     → 1 if Up pressed
cs:0x025B key_state_down   → 1 if Down pressed
cs:0x025C key_state_jump   → 1 if Space/Jump pressed
cs:0x025D key_state_fire   → 1 if R/Ins/Fire pressed
cs:0x025E..0263 map_scancode_* → Active scancode bindings (Left=4Bh, Right=4Dh, Up=48h, Down=50h, Jump=39h, Fire=52h)
cs:0x0264..0269 key_state_*_tmp → Edge-detection scratch state used during joystick polling in INT 08h
cs:0x026A..0276 joystick_* → Calibration center/threshold words and calibrated flag (cs:0x0276)
cs:0x0278 current_melody_ptr → Pointer in CS to active vertical-retrace EGA palette-cycling script
```

---

## Graphics & Video Functions

### EGA Palette, Paging, and Blitting
```text
1000:015D (Line  197) vid_set_ega_palette          → (gfx_load_ega_palette_16_from_table) Load 16 EGA attribute registers from DS:DX on vertical retrace
1000:0185 (Line  231) vid_fade_screen              → Startup check for FRPAK.002 / FRPAK.007 before entering intro
1000:01C0 (Line  258) ui_intro_cutscene_setup      → Check disk files and jump to play_intro_cinematic (1000:7164)
1000:022B (Line  320) vid_set_active_page_0        → (gfx_disable_retrace_palette_script) Clear cs:0x022A and cs:0x0229 to disable ISR palette cycling
1000:0238 (Line  330) vid_set_active_page_1        → (gfx_enable_retrace_palette_script) Arm cs:0x0229=1 so next page flip enables ISR palette cycling
1000:02B4 (Line  436) vid_init_screen_pages        → (gfx_startup_graphics_and_menu_probe) Blit startup screen from SEG_PALETTE, play jingle, run key/joystick prompt
1000:1D2C (Line 2041) map_draw_background          → (gfx_render_viewport_4plane) Setup viewport rows and copy all 4 planes to active VRAM page
1000:1DC0 (Line 2106) map_draw_tile_column         → (gfx_copy_viewport_plane) Copy visible 13-word (208px) × 160-row viewport for one plane to A000h
1000:1E09 (Line 2154) map_draw_tile_row            → (gfx_draw_tile_id_at_pixel_xy) Draw one 16x16 tile ID (CL) at room pixel (AX, BX) into active room buffers
1000:1E86 (Line 2214) map_render_full_screen       → (gfx_render_room_tilemap_to_active_buffers) Render full room tilemap from ds:0x09A8 into offscreen buffers (A000:4000 & SEG_BUFFER_1..4)
1000:1F72 (Line 2323) map_set_tile_attr            → (gfx_copy_16x16_tile_plane_to_active_page) Copy one 16x16 tile plane to A000h offscreen cache
1000:1F89 (Line 2342) map_clear_tile_attr          → (gfx_copy_16x16_tile_plane_to_segment) Copy one 16x16 tile plane to SEG_BUFFER_1..4
1000:4339 (Line 5862) vid_set_write_mode_0         → (ega_set_map_mask) Set EGA Sequencer Map Mask (port 3C4h index 2) to AL
1000:434B (Line 5875) loc_434b (db block)          → Clear 8x8 glyph box at (BX, CX) on active page
1000:7741 (Line 9360) vid_init_ega_regs            → (gfx_init_double_buffer_pages) Set CRTC start=0x0000 and offscreen draw page cs:0x773F=0x2000
1000:774E (Line 9371) vid_wait_vblank              → (gfx_present_and_flip_page) Latch cs:0x022A, set CRTC start to cs:0x773F on retrace, and XOR cs:0x773F with 0x2000
1000:7765 (Line 9384) vid_wait_not_vblank          → (gfx_copy_page_4plane) Copy 0x1F40 bytes per plane (full 320x200 page) from A000:SI to A000:DI
1000:77A3 (Line 9425) vid_clear_page               → (gfx_blit_sprite_masked_active_page) Blit masked planar sprite at (AX, BX) from DS:SI to active page (aligned or shift-4)
1000:77F2 (Line 9473) vid_fill_plane               → (gfx_blit_masked_plane_aligned) Draw one masked plane row-span for byte-aligned X
1000:7833 (Line 9519) vid_set_read_plane           → (gfx_blit_sprite_masked_shift4_dispatch) Dispatch 4 planes for 4-pixel-shifted masked sprite
1000:7849 (Line 9536) vid_copy_page                → (gfx_blit_masked_plane_shift4) Draw one masked plane row-span with 4-bit nibble shift
1000:78C6 (Line 9616) vid_set_map_mask             → (gfx_blit_sprite_opaque_offset_both_pages) Blit opaque sprite (with mask-offset header) to both VRAM pages
1000:78E1 (Line 9635) vid_set_bit_mask             → (gfx_blit_sprite_opaque_offset_active_page) Blit opaque sprite (with mask-offset header) to active page
1000:790C (Line 9666) vid_set_write_mode_2         → (gfx_blit_sprite_opaque_both_pages) Blit opaque planar sprite to both VRAM pages
1000:7927 (Line 9685) vid_restore_write_mode       → (gfx_blit_sprite_opaque_active_page) Blit opaque planar sprite to DI page
1000:7962 (Line 9723) vid_set_color_compare        → (gfx_blit_sprite_plane_rows) Copy one opaque sprite plane row-span with 0x28 byte stride
1000:7990 (Line 9761) vid_load_default_palette     → (gfx_load_and_blit_opaque_file) Read file into SEG_SCRATCH:0600h and opaque-RLE blit to ES:DI
1000:7999 (Line 9773) vid_load_alt_palette         → (gfx_load_and_blit_masked_file) Read file into SEG_SCRATCH:0600h and masked-OR RLE blit to ES:DI
1000:79A2 (Line 9785) vid_set_overscan_color       → (io_read_file_to_seg001_0600) Open/read/close file DS:DX into SEG_SCRATCH:0600h
1000:79C7 (Line 9808) vid_load_palette_buffer      → (gfx_rle_blit_opaque_4plane) Decode 4-plane packetized RLE stream (opaque copy) to ES:DI
1000:79ED (Line 9830) vid_cycle_palette_step       → (gfx_rle_decode_copy_rowspan) Decode one plane of packetized RLE stream (copy)
1000:7A13 (Line 9859) vid_apply_palette            → (gfx_rle_blit_masked_or_4plane) Decode 4-plane packetized RLE stream (OR-composite) to ES:DI
1000:7A39 (Line 9881) vid_dim_palette              → (gfx_rle_decode_or_rowspan) Decode one plane of packetized RLE stream (OR-composite)
1000:7A68 (Line 9917) vid_out_dx_ax                → (ega_select_plane_read_write) Program EGA Sequencer Map Mask (1<<CL) and GC Read Map Select (CL)
1000:7A89 (Line 9944) vid_in_status                → (gfx_set_crtc_start_on_retrace) Wait for display enable, program CRTC Start High (3D4h idx 0Ch) = AH, wait for vertical retrace
1000:7AAB (Line 9972) vid_setup_blit_rect          → (gfx_clear_rect_both_pages) Zero-fill rectangular region on both VRAM pages
1000:7AC8 (Line 9993) vid_set_data_rotate          → (gfx_clear_rect_active_page) Zero-fill rectangular region (AX=x, BX=y, CX=w, DX=h) on page DI
1000:7B03 (Line 10030) vid_reset_data_rotate       → (gfx_clear_rect_plane_rows) Zero-fill rectangle rows for plane CL
1000:7B31 (Line 10068) vid_blit_tile_aligned       → (gfx_blit_sprite_masked_top_clipped) Masked sprite blitter with top-edge row clipping
1000:7B8B (Line 10121) vid_blit_tile_shifted       → (gfx_blit_masked_plane_aligned_top_clipped) Aligned plane blitter with top-clip skip
1000:7BD9 (Line 10172) vid_blit_glyph_aligned      → (gfx_blit_masked_shift4_top_clipped_dispatch) Shift-4 plane dispatch with top-clip skip
1000:7BEF (Line 10189) vid_blit_glyph_shifted      → (gfx_blit_masked_plane_shift4_top_clipped) Shift-4 plane blitter with top-clip skip
1000:7C79 (Line 10276) vid_blit_sprite_masked_aligned → (gfx_blit_sprite_masked_bottom_clipped) Masked sprite blitter with bottom-edge row clipping
1000:7CCD (Line 10325) vid_blit_sprite_masked_shifted → (gfx_blit_masked_plane_aligned_bottom_clipped) Aligned plane blitter with bottom-clip skip
1000:7D1B (Line 10376) vid_blit_small_sprite_aligned  → (gfx_blit_masked_shift4_bottom_clipped_dispatch) Shift-4 plane dispatch with bottom-clip skip
1000:7D31 (Line 10393) vid_blit_small_sprite_shifted  → (gfx_blit_masked_plane_shift4_bottom_clipped) Shift-4 plane blitter with bottom-clip skip
1000:7DBB (Line 10480) vid_blit_planar_block       → (draw_sprite_clipped) Convert world (AX, BX) to viewport coords, clip top/bottom against 208x160 window, and blit masked sprite from cs:0x005E:SI
```

---

## Room Transitions, Intro, UI, and Config

### Room Transition & Finale Sequences
```text
1000:20FE (Line 2567) game_start_room              → Enter doorway/transition: play transition effect, load target stage/room, place Comic at exit doorway, enter game_main_loop
1000:21B8 (Line 2650) game_respawn_player          → Teleport/respawn transition to Stage 5 Room 3 at (0x28, 0x60)
1000:21F2 (Line 2672) game_victory_sequence        → Trigger finale transition, verify Disk 1, and restart title/intro flow
1000:2205 (Line 2685) game_over_sequence           → Play transition SFX, load mapped transition target from ds:0x06F4 table, and resume game_main_loop
1000:2253 (Line 2701) game_switch_room             → Teleport to selected stage/room from stage selection menu (ds:0x021A)
1000:227B (Line 2720) spr_animate_player           → (game_init_room_and_hud) Reset meters, clamp fuel, load room, center camera, and enter game_main_loop
1000:2323 (Line 2779) spr_set_player_frame         → Setup initial coordinates (0x288, 0x70) for stage selected in menu and jump to 1000:227B
1000:3BC6 (Line 5306) snd_play_pickup              → (room_transition_draw_header) Play SFX 0x965E, draw transition header sprite 0x48D6, and present one tick
1000:3C13 (Line 5322) snd_play_blast               → (room_transition_reveal_sequence_a) Multi-step 2x2 quad room reveal sequence (variant A, SFX 0x973C)
1000:3CB1 (Line 5364) snd_play_hit                 → (room_transition_reveal_sequence_b) Multi-step 2x2 quad room reveal sequence (variant B, SFX 0x973C)
1000:3D3A (Line 5393) loc_3d3a (db block)          → (room_transition_draw_reveal_quad) Draw 2x2 reveal quad from transition frame indices
1000:3D7B (Line 5397) loc_3d7b (db block)          → (room_transition_blit_reveal_quad_4plane) Blit one reveal quad tile across 4 planes
1000:3D90 (Line 5399) loc_3d90 (db block)          → (room_transition_blit_reveal_column_plane) Per-plane reveal column blit (16 rows)
1000:3DA1 (Line 5400) loc_3da1 (db block)          → (room_transition_clear_reveal_box) Clear 0x20x0x20 box at transition-relative coordinates
1000:3DB6 (Line 5408) snd_play_jump                → (room_transition_present_frame) Update visible entities, flip page on vblank, and wait 1 tick
1000:3DC3 (Line 5414) loc_3dc3 (db block)          → (sprite_frame_table_offset) Convert frame index in AX to 128-byte tile offset at ds:0x13A8
1000:3DD7 (Line 5415) loc_3dd7 (db block)          → (viewport_xy_to_di) Convert viewport-relative (AX, BX) to VRAM offset in DI
1000:3DFB (Line 5424) spr_render_explosion         → (draw_player_sprite) Draw facing-specific standing Comic sprite at (comic_x, comic_y)
1000:3E1F (Line 5443) snd_play_door_open           → (room_transition_player_entry_sequence) Staged player-entry materialization sweep (SFX 0x9764)
1000:3E98 (Line 5470) snd_play_teleport            → (room_transition_player_exit_sequence) Staged player-exit dematerialization sweep (SFX 0x97CC)
1000:3F0A (Line 5498) snd_play_level_music         → (room_transition_palette_wave) Animated sprite wave sweep during transition (SFX 0x9834)
1000:401B (Line 5537) math_random_byte             → (event_draw_stacked_sprite_rise) Draw rising stacked sprites during boss/object event resolution (SFX 0x9894)
1000:4073 (Line 5559) math_calc_distance           → (event_transition_camera_bob_step) Single camera vertical bob step with background & entity redraw
1000:409C (Line 5581) boss_update_state            → (event_finale_transition_sequence) Finale sequence with camera shake, rising sprites, and handoff to Stage 0 Room 7
```

### UI, Menus, Savegames, and Copy Protection
```text
1000:39AD (Line 5085) hud_draw_lives               → (ui_show_pause_quit_prompt) Show modal pause/game-over message box, wait for key (Q to quit to menu)
1000:4206 (Line 5694) ui_draw_window_box           → (ui_draw_framed_message_from_stream) Draw centered bordered dialog box from `[width, height, text...]` stream at DS:SI
1000:42D4 (Line 5794) ui_clear_window              → (ui_draw_string_8x8) Draw NUL-terminated 8x8 string at (AX, BX)
1000:42E8 (Line 5815) ui_draw_window_border        → (ui_draw_glyph_8x8) Draw one 8x8 glyph (AL - 0x20) at (BX, CX) from font table at cs:0x005E:0xA6F4 and advance BX by 8
1000:5F86 (Line 7368) loc_5f86 (db block)          → (ui_draw_menu_frame) Clear screen and draw framed menu border and title header
1000:6011 (Line 7376) loc_6011 (db block)          → (ui_select_next_valid_option) Advance to next selectable option index
1000:6021 (Line 7377) loc_6021 (db block)          → (ui_select_prev_valid_option) Move to previous selectable option index
1000:6030 (Line 7378) loc_6030 (db block)          → (ui_is_option_selectable) Test whether option index is selectable
1000:606B (Line 7382) loc_606b (db block)          → (ui_render_option_list) Render player roster / medal awards / menu option list
1000:615C (Line 7397) loc_615c (db block)          → (ui_option_list_input_loop) Handle option-list navigation, confirm, and cancel input loop
1000:61C8 (Line 7410) input_poll_devices           → (ui_edit_selected_option_text) Inline keyboard text editor for entering new player name (INT 16h)
1000:62A4 (Line 7509) file_load_keys_def           → (cfg_load_options_or_defaults) Load 102-byte FR.CFG into ds:0x9C94 or zero-initialize defaults
1000:631A (Line 7548) file_save_high_scores        → (cfg_save_options) Write 102-byte ds:0x9C94 table to FR.CFG
1000:633C (Line 7570) file_load_high_scores        → (savegame_write_snapshot) Save mapped object table (cs:0x0AF0) and player state (ds:0x0210..0x028F) to GAME.00x
1000:636F (Line 7594) loc_636f (db block)          → (savegame_read_snapshot) Load mapped object table and player state from GAME.00x (verifies 0x0150 header word)
1000:63AB (Line 7598) loc_63ab (db block)          → End-of-session award/medal calculation and "DO YOU WISH SAVE YOUR PROGRESS?" prompt
1000:64F5 (Line 7620) loc_64f5 (db block)          → (ui_draw_completion_delta_hint) Draw "YOU HAVE EARNED A MEDAL!! / N MEDALS!" notification
1000:6545 (Line 7632) sys_generate_system_hash     → (cfg_compute_hardware_seed) Hash BIOS ROM (C000:0000, F000:0000, F000:FF80) to generate copy-protection seed
1000:65EF (Line 7711) file_check_config_exists     → (io_verify_disk1_or_prompt_retry) Verify DISK1 file presence or prompt "INSERT DISK 1..."
1000:65FD (Line 7720) loc_65fd (db block)          → (io_verify_disk2_or_prompt_retry) Verify DISK2 file presence or prompt "INSERT DISK 2..."
1000:660B (Line 7727) file_save_config             → (io_open_file_or_prompt_retry) Attempt file open (DS:DI); on failure show modal prompt (DS:SI) until retry succeeds or Esc exits
1000:6668 (Line 7757) game_save_load_menu          → (ui_stage_selection_menu) Load frdata.0 / frdata.1 and run 6-zone stage selection menu (NATURE, MAGIC, TIME, ICE, TECHNO, DREAMS)
1000:677A (Line 7854) ui_draw_slot_info            → (ui_render_stage_selection_panel) Render the 6 zone icons, highlight box, and zone title for stage selection
1000:6806 (Line 7911) game_execute_script          → (ent_process_interaction_script_and_queue_message) Dispatch per-NPC/object dialogue script and queue message from ds:0x9EC0
1000:6DF4 (Line 8514) ui_main_menu                 → (cfg_run_integrity_challenge) Check saved BIOS hash in FR.CFG; if mismatched, run one-time password challenge screen
1000:6F47 (Line 8658) ui_high_scores_screen        → (cfg_draw_integrity_challenge_screen) Draw "PASSWORD ENTRY SCREEN" frame, instructions, and grid coordinate prompt (e.g. A,123-)
1000:7057 (Line 8773) ui_instructions_screen       → (cfg_input_challenge_code) Input 5-digit challenge code on the Password Entry Screen
```

### Intro Cinematic Sequence
```text
1000:7164 (Line 8858) ui_story_screen              → (play_intro_cinematic) Play scripted intro sequence (loads frdemo.0, frpak.003..006, frdemo.1..2)
1000:71FE (Line 8916) ui_draw_story_page_1         → (intro_repeat_draw_with_offset_4_3) Repeat intro sprite draw/present loop with (-4, +3) offset
1000:721B (Line 8943) ui_draw_story_page_2         → (intro_repeat_draw_with_offset_8_6) Repeat intro sprite draw/present loop with (-8, +6) offset
1000:7238 (Line 8970) ui_draw_story_page_3         → (intro_repeat_draw_with_offset_12_8) Repeat intro sprite draw/present loop with (-12, +8) offset
1000:7255 (Line 8997) ui_wait_for_keypress         → (gfx_copy_backbuffer_to_active_page) Copy A000:6000h backbuffer into current active draw page
1000:7261 (Line 9008) ui_fade_in_story_page        → (intro_draw_sprite_and_present) Draw intro sprite frame, flip page, and wait 1 tick
1000:7278 (Line 9022) ui_wait_retrace_ticks        → (wait_ticks_or_abort) Wait AX ticks unless key pressed (aborts intro and jumps to menu/title exit)
1000:7298 (Line 9043) ui_title_screen              → Continuation of intro cinematic (loads frpak.003/004, runs split wipe and Comic walk/flash animations)
1000:74E1 (Line 9170) ui_draw_title_stars          → (intro_wipe_split_buffers_step) Compose one horizontal wipe step splitting columns between A000:4000h and A000:6000h
1000:752A (Line 9206) ui_update_title_star         → (gfx_copy_rect_all_planes) Copy rectangle across all 4 EGA planes
1000:7553 (Line 9234) math_lfsr_random             → (gfx_copy_rect_rows_stride40) Copy DX-byte rows for 0xA0 lines with 0x28 byte stride
1000:756D (Line 9253) loc_756d (db block)          → (intro_draw_overlay_with_comic) Copy 6000h backbuffer, draw overlay sprite + Comic sprite, and flip page
1000:75AD (Line 9263) ui_draw_title_logo           → (intro_flash_comic_with_optional_overlay) Run 10-frame Comic flash loop with optional overlay sprite and SFX 0xAC92
1000:7622 (Line 9311) ui_clear_title_logo          → (intro_draw_comic_walk_frame) Advance Comic walk animation frame (ds:0x08AF) and draw at (comic_x, comic_y)
```

---

## Gameplay Logic, Physics, HUD, and Entities

### Main Game Loop
```text
1000:2341 (Line 2793) game_main_loop               → Master gameplay loop (see CORE_ENGINE.md for full state machine breakdown)
```

### Player Physics, Movement, and Actions
```text
1000:18E0 (Line 1581) spr_render_player            → (setup_render_viewport) Center camera scroll_x/scroll_y on Comic, check Stage 2/9 scripted event triggers, and build monster slots (1000:49AD)
1000:1CFE (Line 2011) map_get_tile_at              → (get_tile_at_pixels) Convert pixel (AX, BX) to tile (X>>4, Y>>4) and look up tile ID from ds:0x02A0 row pointers
1000:2A10 (Line 3417) player_update_physics        → (update_player_physics) Grounded/airborne jump & gravity integration, lateral acceleration, ceiling/floor landing
1000:2B8E (Line 3557) player_check_ground_collision → (camera_update_y_follow_comic_clamped) Adjust scroll_y (ds:0x06C4) to keep Comic within vertical follow band [0x30..0x50]
1000:2BDC (Line 3598) player_check_ceiling_collision → (move_player_left) Set left facing (ds:0x08AD=2) and step left via 1000:2BF7
1000:2BF7 (Line 3614) player_check_wall_collision_left → (move_left_step_grounded) Step left 8px, check room left boundary & tile clearance (1000:2D61), scroll camera left
1000:2C32 (Line 3643) player_stop_x                → Zero horizontal velocity (ds:0x0892 = 0)
1000:2C39 (Line 3653) player_check_wall_collision_right → (move_player_right) Set right facing (ds:0x08AD=1) and step right via 1000:2C54
1000:2C54 (Line 3669) player_check_climbing        → (move_right_step_grounded) Step right 8px, check room right boundary & tile clearance (1000:2D61), scroll camera right
1000:2C9D (Line 3702) player_check_swimming        → (check_floor_collision) Check tiles at (AX, BX) and (AX+16, BX) against threshold DI; invoke tile-trigger ranges if armed
1000:2D06 (Line 3755) player_move_left             → (handle_tile_trigger_range_1) If tile AL is in [ds:0x06D6..0x06D7], spawn transient tile replacement from ds:0x06D8
1000:2D31 (Line 3785) player_move_right            → (handle_tile_trigger_range_2) If tile AL is in [ds:0x06E2..0x06E3], spawn transient tile replacement from ds:0x06E4
1000:2D61 (Line 3816) player_do_jump               → (tile_check_horizontal_step_clear) Check lateral step clearance at (AX, comic_y+13) and row below against ds:0x0292 / ds:0x0294
1000:2DC6 (Line 3868) player_stop_jump             → (handle_water_entry_or_hazard) Enter/exit swimming mode (ds:0x089D) with splash overlay and SFX 0x965E / 0x9676
1000:2EDC (Line 3905) player_process_controls      → (handle_swimming_movement) Underwater movement, buoyancy, ceiling/floor checks, and surface exit
1000:2FEE (Line 4018) player_take_damage           → (check_airborne_path_clear) Check vertical tile clearance across player width while swimming
1000:3019 (Line 4048) player_add_health            → (check_airborne_tile_threshold) Test if tile at (AX, BX) is within water tile threshold ds:0x0296
1000:3026 (Line 4063) player_check_hazard_collision → (move_left_step_airborne) Underwater/airborne leftward step (8px) with lateral check (1000:309D) and camera scroll
1000:3057 (Line 4089) player_zero_score_delta      → Zero horizontal velocity (ds:0x0892 = 0)
1000:305E (Line 4099) player_check_crush           → (move_right_step_airborne) Underwater/airborne rightward step (8px) with lateral check (1000:309D) and camera scroll
1000:309D (Line 4130) player_fall_death            → (check_airborne_side_clear) Check lateral tile clearance at comic_y and comic_y+15 against ds:0x0294
1000:30C6 (Line 4156) player_fire_projectile       → (player_start_state4_action_animation) Start 3-tick Mode 1 (Wand) animation (ds:0x08A3=1, ds:0x08AE=4)
1000:3107 (Line 4183) proj_init_trajectory         → (handle_player_animation) Update Mode 1 (Wand) animation and transform target tile in [ds:0x06CA..0x06CB] via ds:0x06CC table
1000:327D (Line 4355) proj_check_monster_hit       → (player_cycle_mode_selection) Cycle active mode ds:0x0896 (0..3) checking unlock flags ds:0x021D..0x021F and toggle hover ds:0x08A6
1000:330F (Line 4414) ent_update_all               → (handle_jetpack_hover_mode) State-5 hover/jetpack physics (ds:0x08A6=1), consuming fuel ds:0x0222 every 4th tick
1000:3468 (Line 4539) ent_spawn_enemy              → (player_start_attack_animation) Start 3-tick Mode 3 (Pickaxe) animation (ds:0x08A7=1, ds:0x08AE=6)
1000:34A9 (Line 4566) ent_update_ai                → (handle_attack_animation) Update Mode 3 (Pickaxe) animation and transform matching entity at ds:0x0700 via ds:0x0946 table
1000:35DE (Line 4694) ent_check_player_contact     → (check_horizontal_room_edge_transition) Check if Comic reached a horizontal room-edge transition zone (ds:0x06F2/0x06F4) and jump to 1000:2205
1000:3610 (Line 4722) ent_destroy                  → (player_start_timed_overlay_sprite) Latch overlay sprite AX into ds:0x091F for 8 ticks (ds:0x091E=8) and play SFX 0x96AE
1000:362A (Line 4739) ent_check_tile_collision     → (check_comic_near_room_event_anchor) Check Comic proximity to Stage 2 carrier (ds:0x0928/0x092A) and latch ds:0x0924=2
1000:3657 (Line 4765) ent_turn_around              → (handle_stage2_carrier_mounted) Update Comic while riding Stage 2 carrier (jump dismount, edge transition, camera scroll)
1000:36C0 (Line 4806) item_collect_pickup          → (update_room_event_anchor_motion) Advance Stage 2 carrier position along guide tiles (0x19..0x1B) and release Comic if blocked
1000:378E (Line 4889) hud_render_all               → (update_room_event_anchor_sprite) Draw Stage 2 carrier sprite and reseed its position when offscreen
1000:385F (Line 4966) hud_update_score             → (handle_stage9_cart_mounted) Latch and update Stage 9 rail/minecart ride (ds:0x092C=2, ds:0x0930/0x0932)
```

### HUD & Counters
```text
1000:3A17 (Line 5102) hud_draw_item_icon           → (hud_update_mode_icons) Draw deselected and active mode icons at X=0x100 from table ds:0x9710
1000:3A5F (Line 5132) hud_draw_shield_bar          → (hud_draw_two_digit_counter) Draw packed BCD two-digit counter CX (CH=tens, CL=ones) at (AX, BX) on both pages
1000:3A92 (Line 5162) hud_draw_keys                → (hud_star_meter_reset) Reset shield *-meter counter (ds:0x08EB=0) and draw empty baseline at (0x100, 0x43)
1000:3AA0 (Line 5174) hud_draw_digits              → (hud_c_meter_reset) Reset blaster C-meter counter (ds:0x08EA=0) and draw empty baseline at (0x100, 0x2A)
1000:3ACE (Line 5203) hud_draw_number_word         → (hud_c_meter_step_up) Recharge blaster C-meter (ds:0x08EA, max 23) by one step and draw glyph at Y=0x2A
1000:3B12 (Line 5239) hud_draw_char                → (hud_c_meter_step_down) Decrement blaster C-meter (ds:0x08EA) by one step and redraw glyph at Y=0x2A
1000:3B51 (Line 5272) hud_flash_item               → (hud_star_meter_step_up) Increment shield *-meter (ds:0x08EB, max 12) by one step and draw glyph at Y=0x43
1000:3B8C (Line 5296) loc_3b8c (db block)          → (hud_star_meter_step_down) Decrement shield *-meter (ds:0x08EB) by one step and redraw glyph at Y=0x43
1000:45A6 (Line 6058) snd_play_dialog_beep         → (hud_increment_lives_bcd_clamped) Play extra-life SFX 0x9904, increment lives BCD (ds:0x028F) up to 99, redraw at (0x120, 0xA9)
1000:4870 (Line 6224) snd_play_chest_open          → (hud_set_wand_icon_collected) Play SFX 0x98E0, set ds:0x021D=1 (wand unlocked), draw icon at (0xF0, 0x77)
1000:489E (Line 6238) item_check_has_item          → (hud_bcd_decrement_cx) Decrement packed BCD word in CX with decimal borrow
1000:48AA (Line 6252) item_add_to_inventory        → (hud_decrement_fuel_bcd) Decrement packed BCD fuel counter ds:0x0222 and redraw at (0x120, 0x90)
1000:48BF (Line 6260) loc_48bf (db block)          → (hud_decrement_lives_bcd) Decrement packed BCD lives counter ds:0x028F and redraw at (0x120, 0xA9)
1000:6D95 (Line 8453) loc_6d95 (db block)          → (hud_add_bcd_counter_from_tens_value) Shift AL to AH (tens) and add into 6-digit BCD score at ds:0x0934
1000:6DA4 (Line 8459) ui_prompt_yes_no             → (hud_add_bcd_counter_from_low_digit) Add AX (AH=tens, AL=ones) into BCD score chain at ds:0x0934..0x0938 and redraw at Y=0x10
1000:6DE7 (Line 8501) snd_play_cursor_move         → (hud_add_bcd_counter_from_mid_digit) Add AX into middle BCD score pair (ds:0x0936) and redraw
```

### Entity, Pickup, and Monster AI System
```text
1000:1A63 (Line 1724) map_spawn_room_entities      → (ent_build_room_entity_list) Load room doorway/edge/monster pointers from CS stage table (cs:0x16A8), populate active entity list at ds:0x0700 from cs:0x0AF0 for current (stage, subroom), and clear projectiles/transients
1000:3181 (Line 4235) proj_spawn_burst             → (ent_spawn_transient_slot) Allocate a 9-byte transient tile-swap/explosion slot at ds:0x08B2 (max 6)
1000:31EB (Line 4292) proj_update_all              → (ent_update_transient_slots) Advance 3-frame animations for active transient slots at ds:0x08B2, restore/commit tile IDs when finished
1000:437B (Line 5884) ui_draw_text_string          → (ent_update_entities_in_viewport) Scan active room entities at ds:0x0700, draw visible sprites, and on Comic contact dispatch pickup handlers via table at cs:0x4367 or Add item to inventory slots ds:0x0239
1000:451C (Line 5987) ui_draw_dialog_message       → (ent_draw_mapped_slot_or_placeholder) Draw inventory slot AX (from ds:0x0239) at (8 + 16*slot, 0xB1) on both pages, or clear 16x16 box if empty
1000:4579 (Line 6033) ui_erase_dialog              → (ent_deactivate_at_coords) Clear bit 15 (0x8000) of active entity at exact (AX, BX) in ds:0x0700
1000:46E6 (Line 6089) game_pause_handler           → (ent_activate_slot_into_runtime) Inventory selection loop (Left/Right moves cursor ds:0x0890, Fire drops/activates item into ds:0x0700 or places clock in Stage 11 Room 7, Home/'G' exits)
1000:48D4 (Line 6268) item_remove_from_inventory   → (ent_normalize_runtime_y_magnitudes) Convert negative Y values in ds:0x0700 back to positive magnitudes
1000:49AD (Line 6299) spr_render_monster           → (ent_build_runtime_slots_for_viewport) Build up to 6 active 32-byte monster behavior slots at cs:0x48ED from room monster descriptors (ds:0x06F8) intersecting viewport
1000:4A70 (Line 6376) spr_get_monster_frame        → (ent_copy_descriptor_to_runtime_slot) Copy 16-byte monster archetype header into cs:0x48ED slot and initialize position/state
1000:4AE3 (Line 6416) loc_4ae3 (db block)          → 33-entry word jump table of monster AI behavior handlers (indexed by `behavior_state - 1`, covering `1000:50E7`..`1000:59C9`)
1000:4B25 (Line 6427) vid_flip_buffers             → (ent_update_object_behaviors) Master monster update loop: ticks active slots at cs:0x48ED and spawns newly scrolled-in descriptors
1000:4C06 (Line 6526) math_clamp_coords            → (ent_deactivate_runtime_slot) Deactivate monster slot (`[cs:SI+6]=0`) and reset source descriptor coordinates
1000:4C25 (Line 6541) map_scroll_camera            → (ent_kill_monster_and_award_score) Spawn destruction puffs at monster position, play SFX 0x9A25, award score via 1000:6DE7
1000:4CC1 (Line 6591) map_check_room_bounds        → (ent_damage_boss_or_special_enemy) Handle projectile hit on `type_flags.hi == 1` boss/special enemy (decrements HP ds:0x9A22)
1000:4D41 (Line 6645) boss_render_sprites          → (ent_handle_player_monster_contact) Handle Comic contact with monster slot: triggers NPC dialogue (1000:6806), spring/platform bounce (0x7B..0x7F), or player shield damage & death
1000:4F15 (Line 6782) ent_render_all               → (ent_tick_and_collide_slot) Dispatch monster AI handler from cs:0x4AE3, cull offscreen slots, test projectile hits (ds:0x08EE) and player bounding-box contact
1000:5090 (Line 6932) spr_render_water_ripple      → (ent_advance_anim_and_draw) Advance monster animation tick (`[cs:SI+1Ah]`), select directional frame table (`+0Ch` or `+0Eh`), and blit via 1000:7DBB

Monster AI Movement & Collision Helpers (in db block 1000:50E7 - 1000:5CEA):
1000:5A4E (Line 7120) loc_5a4e                     → (ent_move_x_and_bounce) Attempt horizontal step BX; flip direction (`[cs:SI+18h]`) on tile collision
1000:5A99 (Line 7125) loc_5a99                     → (ent_apply_gravity_and_move_y) Integrate vertical velocity (`+16h`, clamped to 12) and resolve floor/ceiling collision
1000:5B01 (Line 7131) loc_5b01                     → (ent_move_y_and_bounce) Advance vertical motion and invert Y velocity on collision or viewport edge
1000:5B7B (Line 7139) loc_5b7b                     → (ent_move_y_with_probe_and_bounce) Advance vertical motion using range probe, bouncing on contact
1000:5BAC (Line 7142) loc_5bac                     → (ent_move_x_with_range_probe_and_bounce) Horizontal move using range-based tile probe
1000:5BF7 (Line 7147) loc_5bf7                     → (ent_probe_x_collision_strict) Strict X collision probe against solid threshold ds:0x0292
1000:5C3E (Line 7151) loc_5c3e                     → (ent_probe_x_collision_range) X collision probe accepting bounded tile range `(ds:0x0292..ds:0x0296]`
1000:5C91 (Line 7156) loc_5c91                     → (ent_probe_tile_above_threshold) Probe tile at (AX, BX) and pass when tile ID > DI
1000:5CA1 (Line 7157) loc_5ca1                     → (ent_probe_tile_range_in_viewport) Probe tile in viewport Y-range against `(ds:0x0294..ds:0x0296]`
1000:5CCF (Line 7160) loc_5ccf                     → (ent_set_horizontal_chase_direction_to_comic) Set monster horizontal direction (`[cs:SI+18h]`) toward `comic_x`

Player Projectiles:
1000:5CEB (Line 7169) map_update_animated_tiles    → (spawn_player_projectile) Allocate free projectile slot in ds:0x08EE (up to ds:0x021C) and play SFX 0x9A80
1000:5D5F (Line 7219) map_redraw_dirty_tiles       → (update_projectiles) Advance projectiles in ds:0x08EE, apply corkscrew wave, check tile collisions (1000:2D06), and draw sprite 0xA4A6 / 0xA4D4
```

---

## File I/O & Resource Streaming

```text
1000:198E (Line 1644) map_init_tileset             → (load_room_effect_script_and_trigger_ranges) Load per-stage retrace palette script pointer (cs:0x0278) and 3 tile-trigger replacement tables (ds:0x06CA, 0x06D6, 0x06E2) from cs:0x16A8
1000:1B8D (Line 1825) map_load_room                → (load_resource) Load stage AX (0..14) via tuple table at ds:0x93EC (.0, .1, .2, .3 files) and unpack subroom BX
1000:1BFB (Line 1882) util_calc_stride             → (check_word_is_ffff_or_0005) Return CF=0 if word at [cs:BX] is 0xFFFF or 0x0005
1000:1C0B (Line 1899) map_parse_room_header        → (update_level5_memory_signature) Anti-tamper / copy-protection check when entering Stage 5: verifies BIOS hash against FR.CFG (ds:0x9CF8) and sets stage portal table pointer ds:0x93A8
1000:1FA4 (Line 2364) file_load_pack_rle           → (io_load_rle_resource_to_13a8) Read 6-byte tile threshold header into ds:0x0292..0x0296, read RLE stream into SEG_SCRATCH:0600h, and unpack signed-RLE tileset into ds:0x13A8
1000:1FFE (Line 2417) file_load_raw_mask           → (io_read_file_to_seg_scratch_0600) Read `.1` stage room-tilemap bundle into SEG_SCRATCH:0600h
1000:2022 (Line 2441) file_load_tileset            → (io_read_file_to_seg5e_base) Read `.2` stage sprite/graphics blob into segment `[cs:0x005E]:0000h` (`SEG_PALETTE`)
1000:2049 (Line 2463) file_load_entity_data        → (io_read_xor25_block_9ec0) Read 0x0C00 bytes from `.3` stage dialogue file into ds:0x9EC0 and decode with XOR 0x25
1000:2072 (Line 2489) ent_init_monsters            → (load_room_tilemap_from_resource_buffer) Validate stage ID at SEG_SCRATCH:0602h, read 6-byte subroom entry BX (`tile_w`, `tile_h`, `rle_off`), unpack signed-RLE room tilemap into ds:0x09A8, and populate row pointers at ds:0x02A0
```

---

## Sound System (`snd_api_int03` & `isr_timer_sound_int08`)

### PC Speaker Architecture
```text
INT 3 vector install:
- sys_install_interrupts (1000:09C9) hooks vector 03h to snd_api_int03 (1000:08C7)
- sys_restore_all_interrupts (1000:0A5E) restores original vector on exit

INT 3 command handler (snd_api_int03 at 1000:08C7):
- AL=0 → speaker gate ON mode (cs:0x0248 = 1), speaker output forced OFF until next note
- AL=1 → queue/play sound stream at DS:BX if priority CL >= cs:0x0249
         stores priority in cs:0x0249, playing flag cs:0x0247 = 1, offset in cs:0x024A,
         segment in cs:0x024E, and initial note counter cs:0x024C = 1
- AL=2 → speaker gate OFF / mute mode (cs:0x0248 = 0), speaker output forced OFF
- AL=3 → hard stop (cs:0x0247 = 0, cs:0x0249 = 0), speaker output forced OFF
- AL=4 → query playback active flag (returns AX = cs:0x0247)

Timer-driven playback (isr_timer_sound_int08 at 1000:0683, lines 970-1007):
- When cs:0x0247 == 1, decrements note counter cs:0x024C each 18.2 Hz tick
- When counter reaches 0, reads next word pair from ES:[cs:0x024A]:
  word pitch_divisor, word tick_duration
- pitch_divisor == 0 terminates playback (cs:0x0247 = 0, cs:0x0249 = 0, port 61h bits 0-1 cleared)
- Non-zero divisor programs PIT channel 2 (port 43h = B6h, port 42h = low, high)
  and enables speaker output (port 61h |= 03h) if cs:0x0248 == 1
```

---

## Segment & Data Layout (`comic2_standalone.asm` & `comic2_data.asm`)

| Segment Constant | Compile Seg | Relocated Seg | File & Line Range | Purpose |
|------------------|-------------|---------------|-------------------|---------|
| `.header` | - | - | `comic2_standalone.asm` 13-62 | 512-byte MS-DOS MZ header & 30 relocation entries |
| `.text` (`CS`) | `0x0000` | `0x1000` | `comic2_standalone.asm` 64-11941 | All executable code (`0x0000..0x7E49`), mapped object table (`0x0AF0`), runtime monster slots (`0x48ED`), and stage descriptor/trigger tables (`0x7E4A..0xD5CF`) |
| `SEG_SCRATCH` | `0x0D5D` | `0x1D5D` | `comic2_data.asm` 12-30 | Joystick/key setup strings (`0x0000..0x05FF`) and 34,000-byte RLE/file scratch buffer (`0x0600..0x8ACF`) |
| `SEG_STACK` | `0x160A` | `0x260A` | `comic2_data.asm` 36-1903 | Runtime stack (`SP = 0x07D0`) plus embedded 8x8 font & core player/UI sprite graphics |
| `SEG_PALETTE` | `0x2206` | `0x3206` | `comic2_data.asm` 1909-2670 | Title/intro RLE planar streams (`0x0CDC`, `0x1CD0`) & dynamic `.2` stage sprite buffer (`[cs:0x005E]`) |
| `SEG_DATA` (`DS`) | `0x24FE` | `0x34FE` | `comic2_data.asm` 2677-3069 | Master game state variables, filenames, sound streams, and buffers (`0x0000..0xACDF`) |
| `SEG_BUFFER_1..4` | `0x2FCC..0x47CC` | `0x3FCC..0x57CC` | Runtime RAM beyond load image | Four 32KB offscreen EGA plane buffers for room tilemap rendering |
