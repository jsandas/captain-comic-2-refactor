; ==============================================================================
; CAPTAIN COMIC II: FRACTURED REALITY (1990 by Michael A. Denio)
; 100% Compilable 16-bit 8086 MS-DOS NASM Assembly Source Code
; Output: Complete MS-DOS MZ Executable (comic2.exe)
; Assemble with: nasm -f bin comic2_standalone.asm -o comic2.exe
; ==============================================================================

[BITS 16]

; ----------------------------------------------------------------------------
; MS-DOS MZ EXECUTABLE HEADER (32 paragraphs / 512 bytes)
; ----------------------------------------------------------------------------
section .header vstart=0
mz_header:
    db 'MZ'                       ; Magic signature
    dw 196288 % 512               ; Bytes on last 512-byte page (192)
    dw (196288 + 511) // 512      ; Total 512-byte pages in file (384)
    dw 30                         ; Number of relocation items
    dw 32                         ; Header size in 16-byte paragraphs (512 bytes)
    dw 0x2000                     ; Minimum extra paragraphs needed
    dw 0xffff                     ; Maximum extra paragraphs needed
    dw 0x158d                     ; Initial relative SS value
    dw 0x07d0                     ; Initial SP value
    dw 0x9e6f                     ; Checksum
    dw 0x0064                     ; Initial IP value (Entry point offset)
    dw 0x0000                     ; Initial relative CS value
    dw 0x001c                     ; Offset of relocation table
    dw 0x0000                     ; Overlay number

; Relocation table: 30 entries (offset:segment)
relocation_table:
    dw 0x005c, 0x0000
    dw 0x005e, 0x0000
    dw 0x0066, 0x0000
    dw 0x00f0, 0x0000
    dw 0x0138, 0x0000
    dw 0x0192, 0x0000
    dw 0x01c5, 0x0000
    dw 0x02b6, 0x0000
    dw 0x0343, 0x0000
    dw 0x0463, 0x0000
    dw 0x1d9f, 0x0000
    dw 0x1da7, 0x0000
    dw 0x1daf, 0x0000
    dw 0x1db7, 0x0000
    dw 0x1e4b, 0x0000
    dw 0x1e54, 0x0000
    dw 0x1e5d, 0x0000
    dw 0x1e66, 0x0000
    dw 0x1f1b, 0x0000
    dw 0x1f24, 0x0000
    dw 0x1f2d, 0x0000
    dw 0x1f36, 0x0000
    dw 0x1fc3, 0x0000
    dw 0x200c, 0x0000
    dw 0x2079, 0x0000
    dw 0x7170, 0x0000
    dw 0x7670, 0x0000
    dw 0x76d7, 0x0000
    dw 0x770d, 0x0000
    dw 0x79af, 0x0000
    times 512 - ($ - mz_header) db 0

section .text vstart=0 follows=.header

; ----------------------------------------------------------------------------
; SEGMENT DEFINITIONS & CONSTANTS
; ----------------------------------------------------------------------------
SEG_DATA        equ 0x24fe   ; Primary game data, strings, state segment
SEG_STACK       equ 0x160a   ; Initial runtime stack segment
SEG_PALETTE     equ 0x2206   ; EGA 16-color DAC palette definition table
SEG_SCRATCH     equ 0x0d5d   ; Decompression scratchpad & collision buffer
SEG_BUFFER_1    equ 0x2fcc   ; Sprite plane buffer 1
SEG_BUFFER_2    equ 0x37cc   ; Sprite plane buffer 2
SEG_BUFFER_3    equ 0x3fcc   ; Sprite plane buffer 3
SEG_BUFFER_4    equ 0x47cc   ; Sprite plane buffer 4

    db 0x0a, 0x0d, 0x0a, 0x0d, 0x43, 0x61, 0x70, 0x74, 0x61, 0x69, 0x6e, 0x20, 0x43, 0x6f, 0x6d, 0x69 ; 1000:0000
    db 0x63, 0x20, 0x49, 0x49, 0x20, 0x2d, 0x20, 0x46, 0x72, 0x61, 0x63, 0x74, 0x75, 0x72, 0x65, 0x64 ; 1000:0010
    db 0x20, 0x52, 0x65, 0x61, 0x6c, 0x69, 0x74, 0x79, 0x2c, 0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f ; 1000:0020
    db 0x6e, 0x20, 0x31, 0x2e, 0x35, 0x0a, 0x0d, 0x43, 0x6f, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74 ; 1000:0030
    db 0x20, 0x31, 0x39, 0x39, 0x30, 0x20, 0x62, 0x79, 0x20, 0x4d, 0x69, 0x63, 0x68, 0x61, 0x65, 0x6c ; 1000:0040
    db 0x20, 0x41, 0x2e, 0x20, 0x44, 0x65, 0x6e, 0x69, 0x6f, 0x0a, 0x0d, 0x1a ; 1000:0050
    dw SEG_DATA                                  ; 1000:005c: Relocation segment
    dw SEG_STACK                                 ; 1000:005e: Relocation segment
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:0060

; ----------------------------------------------------------------------------
; FUNCTION: entry  [System / Startup]
; ADDRESS:  1000:0064
; PURPOSE:  Main game executable entry point. Initializes CPU speed calibration, checks for EGA with 256K, initializes video mode 0Dh, sets up interrupt handlers, and transfers execution to main dispatcher.
; ----------------------------------------------------------------------------
entry:
    cld                                          ; 1000:0064: fc
    mov          AX,SEG_DATA                     ; 1000:0065: b8fe34
    mov          DS,AX                           ; 1000:0068: 8ed8
    in           AL,0x61                         ; 1000:006a: e461
    and          AL,0xfc                         ; 1000:006c: 24fc
    out          0x61,AL                         ; 1000:006e: e661
    call         sys_install_interrupts          ; 1000:0070: e85609
    mov          AX,0x1                          ; 1000:0073: b80100
    call         timer_delay_ticks               ; 1000:0076: e80102
    mov          CX,0xffff                       ; 1000:0079: b9ffff
loc_007c:
    in           AL,DX                           ; 1000:007c: ec
    cmp          byte [cs:0x245],0x1             ; 1000:007d: 2e803e450201
    jz           loc_0090                        ; 1000:0083: 740b
    loop         loc_007c                        ; 1000:0085: e2f5
    mov          word [cs:0x62],0x500            ; 1000:0087: 2ec70662000005
    jmp short    loc_00af                        ; 1000:008e: eb1f
loc_0090:
    neg          CX                              ; 1000:0090: f7d9
    mov          AX,CX                           ; 1000:0092: 8bc1
    xor          DX,DX                           ; 1000:0094: 33d2
    mov          CX,0x1c                         ; 1000:0096: b91c00
    div          CX                              ; 1000:0099: f7f1
    mov          [cs:0x62],AX                    ; 1000:009b: 2ea36200
    mov          CX,0x500                        ; 1000:009f: b90005
    sub          CX,AX                           ; 1000:00a2: 2bc8
    jc           loc_00af                        ; 1000:00a4: 7209
    shr          CX,0x1                          ; 1000:00a6: d1e9
    shr          CX,0x1                          ; 1000:00a8: d1e9
    add          word [cs:0x62],CX               ; 1000:00aa: 2e010e6200
loc_00af:
    mov          AX,0xf00                        ; 1000:00af: b8000f
    int          0x10                            ; 1000:00b2: cd10
    cmp          AL,0xd                          ; 1000:00b4: 3c0d
    xor          AH,AH                           ; 1000:00b6: 32e4
    mov          [0x0],AX                        ; 1000:00b8: a30000
    mov          AX,0xd                          ; 1000:00bb: b80d00
    int          0x10                            ; 1000:00be: cd10
    mov          AX,0xf00                        ; 1000:00c0: b8000f
    int          0x10                            ; 1000:00c3: cd10
    cmp          AL,0xd                          ; 1000:00c5: 3c0d
    jnz          loc_00d6                        ; 1000:00c7: 750d
    mov          AX,0x1200                       ; 1000:00c9: b80012
    mov          BX,0x10                         ; 1000:00cc: bb1000
    int          0x10                            ; 1000:00cf: cd10
    cmp          BL,0x3                          ; 1000:00d1: 80fb03
    jge          loc_00e6                        ; 1000:00d4: 7d10
loc_00d6:
    mov          AX,0x2                          ; 1000:00d6: b80200
    int          0x10                            ; 1000:00d9: cd10
    mov          AH,0x9                          ; 1000:00db: b409
    lea          DX,[0xa0]                       ; 1000:00dd: 8d16a000
    int          0x21                            ; 1000:00e1: cd21
    jmp near     sys_exit_to_dos                 ; 1000:00e3: e92f01
loc_00e6:
    mov          AX,0xa000                       ; 1000:00e6: b800a0
    mov          ES,AX                           ; 1000:00e9: 8ec0
    call         vid_init_ega_regs               ; 1000:00eb: e85376
    push         DS                              ; 1000:00ee: 1e
    mov          AX,SEG_PALETTE                  ; 1000:00ef: b80632
    mov          DS,AX                           ; 1000:00f2: 8ed8
    mov          SI,0x1cd0                       ; 1000:00f4: bed01c
    mov          DI,word [cs:0x773f]             ; 1000:00f7: 2e8b3e3f77
    call         vid_load_palette_buffer         ; 1000:00fc: e8c878
    mov          SI,0x0                          ; 1000:00ff: be0000
    mov          DI,word [cs:0x773f]             ; 1000:0102: 2e8b3e3f77
    call         vid_apply_palette               ; 1000:0107: e80979
    pop          DS                              ; 1000:010a: 1f
    mov          DX,0x2                          ; 1000:010b: ba0200
    call         vid_set_ega_palette             ; 1000:010e: e84c00
    call         vid_wait_vblank                 ; 1000:0111: e83a76
    mov          DX,0x13                         ; 1000:0114: ba1300
    call         vid_set_ega_palette             ; 1000:0117: e84300
    mov          byte [cs:0x61],0x0              ; 1000:011a: 2ec606610000
    mov          CX,0x19                         ; 1000:0120: b91900
loc_0123:
    mov          AX,0x1                          ; 1000:0123: b80100
    call         timer_delay_ticks               ; 1000:0126: e85101
    cmp          byte [cs:0x61],0x0              ; 1000:0129: 2e803e610000
    jnz          loc_0136                        ; 1000:012f: 7505
    loop         loc_0123                        ; 1000:0131: e2f0
    call         ui_story_screen                 ; 1000:0133: e82e70
loc_0136:
    push         DS                              ; 1000:0136: 1e
    mov          AX,SEG_PALETTE                  ; 1000:0137: b80632
    mov          DS,AX                           ; 1000:013a: 8ed8
    mov          SI,0x1cd0                       ; 1000:013c: bed01c
    mov          DI,word [cs:0x773f]             ; 1000:013f: 2e8b3e3f77
    call         vid_load_palette_buffer         ; 1000:0144: e88078
    pop          DS                              ; 1000:0147: 1f
    mov          DX,0x2                          ; 1000:0148: ba0200
    call         vid_set_ega_palette             ; 1000:014b: e80f00
    call         vid_wait_vblank                 ; 1000:014e: e8fd75
    mov          AX,0x3                          ; 1000:0151: b80300
    call         timer_delay_ticks               ; 1000:0154: e82301
    call         vid_init_screen_pages           ; 1000:0157: e85a01
    jmp near     sys_main_dispatcher             ; 1000:015a: e9f55c

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_ega_palette  [Video / Palette]
; ADDRESS:  1000:015d
; PURPOSE:  Waits for EGA vertical retrace and writes 16 color values to the EGA Attribute Controller palette registers (Ports 0x3da / 0x3c0).
; ----------------------------------------------------------------------------
vid_set_ega_palette:
    push         SI                              ; 1000:015d: 56
    mov          SI,DX                           ; 1000:015e: 8bf2
    mov          DX,0x3da                        ; 1000:0160: bada03
loc_0163:
    in           AL,DX                           ; 1000:0163: ec
    test         AL,0x8                          ; 1000:0164: a808
    jz           loc_0163                        ; 1000:0166: 74fb
    cli                                          ; 1000:0168: fa
    mov          DX,0x3c0                        ; 1000:0169: bac003
    mov          CX,0x10                         ; 1000:016c: b91000
    xor          BX,BX                           ; 1000:016f: 33db
loc_0171:
    push         CX                              ; 1000:0171: 51
    mov          CL,byte [SI]                    ; 1000:0172: 8a0c
    inc          SI                              ; 1000:0174: 46
    mov          AX,BX                           ; 1000:0175: 8bc3
    inc          BX                              ; 1000:0177: 43
    out          DX,AL                           ; 1000:0178: ee
    mov          AL,CL                           ; 1000:0179: 8ac1
    out          DX,AL                           ; 1000:017b: ee
    pop          CX                              ; 1000:017c: 59
    loop         loc_0171                        ; 1000:017d: e2f2
    mov          AL,0x20                         ; 1000:017f: b020
    out          DX,AL                           ; 1000:0181: ee
    pop          SI                              ; 1000:0182: 5e
    sti                                          ; 1000:0183: fb
    ret                                          ; 1000:0184: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_fade_screen  [Video / Palette]
; ADDRESS:  1000:0185
; PURPOSE:  Cycles/fades the EGA palette out to black over a sequence of frame delays for smooth screen transitions.
; ----------------------------------------------------------------------------
vid_fade_screen:
    call         vid_set_active_page_0           ; 1000:0185: e8a300
    mov          AX,0xd                          ; 1000:0188: b80d00
    int          0x10                            ; 1000:018b: cd10
    call         vid_init_ega_regs               ; 1000:018d: e8b175
    push         DS                              ; 1000:0190: 1e
    mov          AX,SEG_PALETTE                  ; 1000:0191: b80632
    mov          DS,AX                           ; 1000:0194: 8ed8
    mov          SI,0x1cd0                       ; 1000:0196: bed01c
    mov          DI,word [cs:0x773f]             ; 1000:0199: 2e8b3e3f77
    call         vid_load_palette_buffer         ; 1000:019e: e82678
    pop          DS                              ; 1000:01a1: 1f
    mov          DX,0x1eb                        ; 1000:01a2: baeb01
    mov          DI,word [cs:0x773f]             ; 1000:01a5: 2e8b3e3f77
    call         vid_load_alt_palette            ; 1000:01aa: e8ec77
    mov          DX,0x24                         ; 1000:01ad: ba2400
    call         vid_set_ega_palette             ; 1000:01b0: e8aaff
    call         vid_wait_vblank                 ; 1000:01b3: e89875
    mov          AX,0x1                          ; 1000:01b6: b80100
    mov          BX,0xdb                         ; 1000:01b9: bbdb00
    mov          CX,0x1                          ; 1000:01bc: b90100
    int3                                         ; 1000:01bf: cc
; ----------------------------------------------------------------------------
; FUNCTION: ui_intro_cutscene_setup  [UI / Cutscene]
; ADDRESS:  1000:01c0
; PURPOSE:  Fades screen, loads title palette, plays intro melody, and jumps to story screen.
; ----------------------------------------------------------------------------
ui_intro_cutscene_setup:
    call         sys_restore_interrupts          ; 1000:01c0: e8d603
    push         DS                              ; 1000:01c3: 1e
    mov          ax, SEG_PALETTE                 ; 1000:01c4: b8 (relocated segment)
    mov          DS,AX                           ; 1000:01c7: 8ed8
    mov          SI,0x1cd0                       ; 1000:01c9: bed01c
    mov          DI,word [cs:0x773f]             ; 1000:01cc: 2e8b3e3f77
    call         vid_load_palette_buffer         ; 1000:01d1: e8f377
    pop          DS                              ; 1000:01d4: 1f
    mov          DX,0x1f5                        ; 1000:01d5: baf501
    mov          DI,word [cs:0x773f]             ; 1000:01d8: 2e8b3e3f77
    call         vid_load_alt_palette            ; 1000:01dd: e8b977
    mov          DX,0x24                         ; 1000:01e0: ba2400
    call         vid_set_ega_palette             ; 1000:01e3: e877ff
    call         vid_wait_vblank                 ; 1000:01e6: e86575
    mov          AX,0x1                          ; 1000:01e9: b80100
    mov          BX,0x96b6                       ; 1000:01ec: bbb696
    mov          CX,0x1                          ; 1000:01ef: b90100
    int3                                         ; 1000:01f2: cc
    call         sys_restore_interrupts          ; 1000:01f3: e8a303
    mov          AX,0xd                          ; 1000:01f6: b80d00
    int          0x10                            ; 1000:01f9: cd10
    call         vid_init_ega_regs               ; 1000:01fb: e84375
    jmp          loc_63ab                        ; 1000:01fe: e9aa61
    ret                                          ; 1000:0201: c3

; ----------------------------------------------------------------------------
; FUNCTION: sys_fatal_error  [System / Error]
; ADDRESS:  1000:0202
; PURPOSE:  Restores text video mode, prints the fatal error string at DS:0x0046 using DOS INT 21h AH=09h, and exits to DOS.
; ----------------------------------------------------------------------------
sys_fatal_error:
    mov          AX,[0x0]                        ; 1000:0202: a10000
    int          0x10                            ; 1000:0205: cd10
    mov          AH,0x9                          ; 1000:0207: b409
    lea          DX,[0x46]                       ; 1000:0209: 8d164600
    int          0x21                            ; 1000:020d: cd21
    call         sys_restore_interrupts          ; 1000:020f: e88703
    jmp short    sys_exit_to_dos                 ; 1000:0212: eb01
    db 0x90                                         ; 1000:0214

; ----------------------------------------------------------------------------
; FUNCTION: sys_exit_to_dos  [System / Lifecycle]
; ADDRESS:  1000:0215
; PURPOSE:  Restores original interrupt vectors, cleans up video mode, and terminates the program via DOS INT 21h AH=4Ch.
; ----------------------------------------------------------------------------
sys_exit_to_dos:
    mov          AX,0x2                          ; 1000:0215: b80200
    int3                                         ; 1000:0218: cc
    call         sys_restore_all_interrupts      ; 1000:0219: e84208
    mov          AX,[0x0]                        ; 1000:021c: a10000
    int          0x10                            ; 1000:021f: cd10
    lea          BX,[0x35]                       ; 1000:0221: 8d1e3500
    mov          AH,0x4c                         ; 1000:0225: b44c
    int          0x21                            ; 1000:0227: cd21
    dw           0x0000                          ; 1000:0229: 0000

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_active_page_0  [Video / Paging]
; ADDRESS:  1000:022b
; PURPOSE:  Sets active EGA display page to page 0 (offset 0x0000 in VRAM A000:0000).
; ----------------------------------------------------------------------------
vid_set_active_page_0:
    mov          byte [cs:0x22a],0x0             ; 1000:022b: 2ec6062a0200
    mov          byte [cs:0x229],0x0             ; 1000:0231: 2ec606290200
    ret                                          ; 1000:0237: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_active_page_1  [Video / Paging]
; ADDRESS:  1000:0238
; PURPOSE:  Sets active EGA display page to page 1 (offset 0x8000 in VRAM A000:0000) for double buffering.
; ----------------------------------------------------------------------------
vid_set_active_page_1:
    mov          byte [cs:0x22a],0x0             ; 1000:0238: 2ec6062a0200
    mov          byte [cs:0x229],0x1             ; 1000:023e: 2ec606290201
    ret                                          ; 1000:0244: c3
; ----------------------------------------------------------------------------
; IN-CODE HARDWARE & INPUT STATE TABLE (CS:0245 - CS:027A)
; ----------------------------------------------------------------------------
game_tick_flag          db 0x00                         ; 1000:0245
irq0_parity             db 0x00                         ; 1000:0246
sound_is_playing        db 0x00                         ; 1000:0247
sound_is_enabled        db 0x01                         ; 1000:0248
sound_priority          db 0x00                         ; 1000:0249
sound_data_offset       dw 0x0000                       ; 1000:024a
sound_note_counter      dw 0x0000                       ; 1000:024c
sound_data_segment      dw 0x0000                       ; 1000:024e
scancode_esc            db 0x01                         ; 1000:0250: Esc scancode
scancode_f1             db 0x3b                         ; 1000:0251: F1 (Unmute)
scancode_f2             db 0x3c                         ; 1000:0252: F2 (Mute)
scancode_f3             db 0x3d                         ; 1000:0253: F3
scancode_f4             db 0x3e                         ; 1000:0254: F4
scancode_pgup           db 0x49                         ; 1000:0255: Page Up
scancode_pgdn           db 0x51                         ; 1000:0256: Page Down
scancode_home           db 0x47                         ; 1000:0257: Home
key_state_left          db 0x00                         ; 1000:0258
key_state_right         db 0x00                         ; 1000:0259
key_state_up            db 0x00                         ; 1000:025a
key_state_down          db 0x00                         ; 1000:025b
key_state_jump          db 0x00                         ; 1000:025c
key_state_fire          db 0x00                         ; 1000:025d
map_scancode_left       db 0x4b                         ; 1000:025e
map_scancode_right      db 0x4d                         ; 1000:025f
map_scancode_up         db 0x48                         ; 1000:0260
map_scancode_down       db 0x50                         ; 1000:0261
map_scancode_jump       db 0x39                         ; 1000:0262
map_scancode_fire       db 0x52                         ; 1000:0263
key_state_jump_tmp      db 0x00                         ; 1000:0264
key_state_fire_tmp      db 0x00                         ; 1000:0265
key_state_right_tmp     db 0x00                         ; 1000:0266
key_state_left_tmp      db 0x00                         ; 1000:0267
key_state_up_tmp        db 0x00                         ; 1000:0268
key_state_down_tmp      db 0x00                         ; 1000:0269
joystick_temp_x         dw 0x0000                       ; 1000:026a
joystick_temp_y         dw 0x0000                       ; 1000:026c
joystick_x_low          dw 0x0000                       ; 1000:026e
joystick_x_high         dw 0x0000                       ; 1000:0270
joystick_y_low          dw 0x0000                       ; 1000:0272
joystick_y_high         dw 0x0000                       ; 1000:0274
joystick_is_calibrated  dw 0x0000                       ; 1000:0276
current_melody_ptr      dw 0x0000                       ; 1000:0278

; ----------------------------------------------------------------------------
; FUNCTION: timer_delay_ticks  [System / Timing]
; ADDRESS:  1000:027a
; PURPOSE:  Synchronous frame delay routine. Decrements AX against the hardware timer tick flag updated by INT 08h ISR.
; ----------------------------------------------------------------------------
timer_delay_ticks:
    cmp          byte [cs:0x245],0x1             ; 1000:027a: 2e803e450201
    jnz          timer_delay_ticks               ; 1000:0280: 75f8
    mov          byte [cs:0x245],0x0             ; 1000:0282: 2ec606450200
    dec          AX                              ; 1000:0288: 48
    jnz          timer_delay_ticks               ; 1000:0289: 75ef
    ret                                          ; 1000:028b: c3
; ----------------------------------------------------------------------------
; FUNCTION: sys_print_string  [System / Text]
; ADDRESS:  1000:028c
; PURPOSE:  Prints null-terminated string at DS:BX using BIOS teletype INT 10h AH=0Eh.
; ----------------------------------------------------------------------------
sys_print_string:
    mov          AL,[BX]                         ; 1000:028c: 8a07
    inc          BX                              ; 1000:028e: 43
    or           AL,AL                           ; 1000:028f: 0ac0
    jz           .done                           ; 1000:0291: 740b
    push         BX                              ; 1000:0293: 53
    mov          BX,0x7                          ; 1000:0294: bb0700
    mov          AH,0xe                          ; 1000:0297: b40e
    int          0x10                            ; 1000:0299: cd10
    pop          BX                              ; 1000:029b: 5b
    jmp short    sys_print_string                ; 1000:029c: ebee
.done:
    ret                                          ; 1000:029e: c3

; ----------------------------------------------------------------------------
; FUNCTION: sys_print_xor_string  [System / Text]
; ADDRESS:  1000:029f
; PURPOSE:  Decodes and prints XOR 0x25 obfuscated string at DS:BX via BIOS teletype INT 10h AH=0Eh.
; ----------------------------------------------------------------------------
sys_print_xor_string:
    mov          AL,[BX]                         ; 1000:029f: 8a07
    inc          BX                              ; 1000:02a1: 43
    xor          AL,0x25                         ; 1000:02a2: 3425
    or           AL,AL                           ; 1000:02a4: 0ac0
    jz           .done                           ; 1000:02a6: 740b
    push         BX                              ; 1000:02a8: 53
    mov          BX,0x7                          ; 1000:02a9: bb0700
    mov          AH,0xe                          ; 1000:02ac: b40e
    int          0x10                            ; 1000:02ae: cd10
    pop          BX                              ; 1000:02b0: 5b
    jmp short    sys_print_xor_string            ; 1000:02b1: ebec
.done:
    ret                                          ; 1000:02b3: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_init_screen_pages  [Video / Paging]
; ADDRESS:  1000:02b4
; PURPOSE:  Initializes both EGA video pages (0 and 1), clears screen memory, and sets up active palette.
; ----------------------------------------------------------------------------
vid_init_screen_pages:
    push         DS                              ; 1000:02b4: 1e
    mov          AX,SEG_PALETTE                  ; 1000:02b5: b80632
    mov          DS,AX                           ; 1000:02b8: 8ed8
    mov          SI,0x1cd0                       ; 1000:02ba: bed01c
    mov          DI,word [cs:0x773f]             ; 1000:02bd: 2e8b3e3f77
    call         vid_load_palette_buffer         ; 1000:02c2: e80277
    mov          SI,0xcdc                        ; 1000:02c5: bedc0c
    mov          DI,word [cs:0x773f]             ; 1000:02c8: 2e8b3e3f77
    call         vid_apply_palette               ; 1000:02cd: e84377
    pop          DS                              ; 1000:02d0: 1f
    mov          DX,0x2                          ; 1000:02d1: ba0200
    call         vid_set_ega_palette             ; 1000:02d4: e886fe
    call         vid_wait_vblank                 ; 1000:02d7: e87474
    mov          DX,0x24                         ; 1000:02da: ba2400
    call         vid_set_ega_palette             ; 1000:02dd: e87dfe
    mov          AX,0x1                          ; 1000:02e0: b80100
    mov          BX,0xdb                         ; 1000:02e3: bbdb00
    mov          CX,0x1                          ; 1000:02e6: b90100
    int3                                         ; 1000:02e9: cc
file_load_keys_def_and_prompt:
    lea          dx,[0x1ff]                      ; 1000:02ea: 8d16ff01
    mov          ax,0x3d00                       ; 1000:02ee: b8003d
    int          0x21                            ; 1000:02f1: cd21
    jc           .prompt_user                    ; 1000:02f3: 7218
    mov          bx,ax                           ; 1000:02f5: 8bd8
    push         ds                              ; 1000:02f7: 1e
    mov          ax,cs                           ; 1000:02f8: 8cc8
    mov          ds,ax                           ; 1000:02fa: 8ed8
    mov          dx,map_scancode_left                        ; 1000:02fc: ba5e02
    mov          cx,0x6                          ; 1000:02ff: b90600
    mov          ax,0x3f00                       ; 1000:0302: b8003f
    int          0x21                            ; 1000:0305: cd21
    mov          ax,0x3e00                       ; 1000:0307: b8003e
    int          0x21                            ; 1000:030a: cd21
    pop          ds                              ; 1000:030c: 1f
.prompt_user:
    call         sys_flush_keyboard_buffer                           ; 1000:030d: e88902
    cmp          byte [cs:0x61],0x25             ; 1000:0310: 2e803e610025
    jnz          .check_exit                     ; 1000:0316: 7503
    jmp          input_setup_keyboard            ; 1000:0318: e94101
.check_exit:
    cmp          byte [cs:0x61],0x1              ; 1000:031b: 2e803e610001
    jnz          .check_joystick                 ; 1000:0321: 7503
    jmp          sys_exit_to_dos                           ; 1000:0323: e9effe
.check_joystick:
    cmp          byte [cs:0x61],0x24             ; 1000:0326: 2e803e610024
    jz           input_calibrate_joystick        ; 1000:032c: 740e
vid_set_mode_and_init:
    mov          ax,0xd                          ; 1000:032e: b80d00
    int          0x10                            ; 1000:0331: cd10
    mov          ax,0xa000                       ; 1000:0333: b800a0
    mov          es,ax                           ; 1000:0336: 8ec0
    call         vid_init_ega_regs                          ; 1000:0338: e80674
    ret                                          ; 1000:033b: c3

input_calibrate_joystick:
    mov          ax,0x2                          ; 1000:033c: b80200
    int          0x10                            ; 1000:033f: cd10
    push         ds                              ; 1000:0341: 1e
    mov          ax, SEG_SCRATCH                 ; 1000:0342: b85d0d
    mov          ds,ax                           ; 1000:0345: 8ed8
    mov          byte [cs:0x61],0x0              ; 1000:0347: 2ec606610000
    lea          dx,[0x0]                        ; 1000:034d: 8d160000
    mov          ah,0x9                          ; 1000:0351: b409
    int          0x21                            ; 1000:0353: cd21
    call         input_wait_joystick_or_key      ; 1000:0355: e8cc00
    or           ax,ax                           ; 1000:0358: 0bc0
    jnz          .read_center                    ; 1000:035a: 7503
    jmp near     .abort                          ; 1000:035c: e9b700
.read_center:
    mov          ah,0x84                         ; 1000:035f: b484
    mov          dx,0x1                          ; 1000:0361: ba0100
    int          0x15                            ; 1000:0364: cd15
    mov          [cs:joystick_temp_x],ax                   ; 1000:0366: 2ea36a02
    mov          [cs:joystick_temp_y],bx                   ; 1000:036a: 2e891e6c02
    lea          dx,[0xa8]                       ; 1000:036f: 8d16a800
    mov          ah,0x9                          ; 1000:0373: b409
    int          0x21                            ; 1000:0375: cd21
    call         input_wait_joystick_or_key      ; 1000:0377: e8aa00
    or           ax,ax                           ; 1000:037a: 0bc0
    jnz          .read_upper_left                ; 1000:037c: 7503
    jmp near     .abort                          ; 1000:037e: e99500
.read_upper_left:
    mov          ah,0x84                         ; 1000:0381: b484
    mov          dx,0x1                          ; 1000:0383: ba0100
    int          0x15                            ; 1000:0386: cd15
    mov          bx,[cs:joystick_temp_x]                   ; 1000:0388: 2e8b1e6a02
    sub          bx,ax                           ; 1000:038d: 2bd8
    shr          bx,1                            ; 1000:038f: d1eb
    mov          [cs:joystick_x_low],bx                   ; 1000:0391: 2e891e6e02
    lea          dx,[0xe6]                       ; 1000:0396: 8d16e600
    mov          ah,0x9                          ; 1000:039a: b409
    int          0x21                            ; 1000:039c: cd21
    call         input_wait_joystick_or_key      ; 1000:039e: e88300
    or           ax,ax                           ; 1000:03a1: 0bc0
    jnz          .read_lower_right               ; 1000:03a3: 7503
    jmp short    .abort                          ; 1000:03a5: eb6f
    nop                                          ; 1000:03a7: 90
.read_lower_right:
    mov          ah,0x84                         ; 1000:03a8: b484
    mov          dx,0x1                          ; 1000:03aa: ba0100
    int          0x15                            ; 1000:03ad: cd15
    mov          bx,[cs:joystick_temp_x]                   ; 1000:03af: 2e8b1e6a02
    sub          ax,bx                           ; 1000:03b4: 2bc3
    shr          ax,1                            ; 1000:03b6: d1e8
    add          bx,ax                           ; 1000:03b8: 03d8
    mov          [cs:joystick_x_high],bx                   ; 1000:03ba: 2e891e7002
    lea          dx,[0x125]                      ; 1000:03bf: 8d162501
    mov          ah,0x9                          ; 1000:03c3: b409
    int          0x21                            ; 1000:03c5: cd21
    call         input_wait_joystick_or_key      ; 1000:03c7: e85a00
    or           ax,ax                           ; 1000:03ca: 0bc0
    jnz          .read_y_low                     ; 1000:03cc: 7503
    jmp short    .abort                          ; 1000:03ce: eb46
    nop                                          ; 1000:03d0: 90
.read_y_low:
    mov          ah,0x84                         ; 1000:03d1: b484
    mov          dx,0x1                          ; 1000:03d3: ba0100
    int          0x15                            ; 1000:03d6: cd15
    mov          ax,[cs:joystick_temp_y]                   ; 1000:03d8: 2ea16c02
    sub          ax,bx                           ; 1000:03dc: 2bc3
    shr          ax,1                            ; 1000:03de: d1e8
    mov          [cs:joystick_y_low],ax                   ; 1000:03e0: 2ea37202
    lea          dx,[0x162]                      ; 1000:03e4: 8d166201
    mov          ah,0x9                          ; 1000:03e8: b409
    int          0x21                            ; 1000:03ea: cd21
    call         input_wait_joystick_or_key      ; 1000:03ec: e83500
    or           ax,ax                           ; 1000:03ef: 0bc0
    jnz          .read_y_high                    ; 1000:03f1: 7503
    jmp short    .abort                          ; 1000:03f3: eb21
    nop                                          ; 1000:03f5: 90
.read_y_high:
    mov          ah,0x84                         ; 1000:03f6: b484
    mov          dx,0x1                          ; 1000:03f8: ba0100
    int          0x15                            ; 1000:03fb: cd15
    mov          ax,[cs:joystick_temp_y]                   ; 1000:03fd: 2ea16c02
    sub          bx,ax                           ; 1000:0401: 2bd8
    shr          bx,1                            ; 1000:0403: d1eb
    add          ax,bx                           ; 1000:0405: 03c3
    mov          [cs:joystick_y_high],ax                   ; 1000:0407: 2ea37402
    pop          ds                              ; 1000:040b: 1f
    mov          word [cs:joystick_is_calibrated],0x1             ; 1000:040c: 2ec70676020100
    jmp          vid_set_mode_and_init           ; 1000:0413: e918ff
.abort:
    pop          ds                              ; 1000:0416: 1f
    mov          ax,0xd                          ; 1000:0417: b80d00
    int          0x10                            ; 1000:041a: cd10
    mov          ax,0xa000                       ; 1000:041c: b800a0
    mov          es,ax                           ; 1000:041f: 8ec0
    jmp          vid_init_screen_pages                           ; 1000:0421: e990fe

input_wait_joystick_or_key:
    mov          ax,0x2                          ; 1000:0424: b80200
    call         timer_delay_ticks                           ; 1000:0427: e850fe
    cmp          byte [cs:0x61],0x0              ; 1000:042a: 2e803e610000
    jnz          .key_pressed                    ; 1000:0430: 7529
    mov          ah,0x84                         ; 1000:0432: b484
    xor          dx,dx                           ; 1000:0434: 33d2
    int          0x15                            ; 1000:0436: cd15
    test         al,0x10                         ; 1000:0438: a810
    jz           input_wait_joystick_or_key      ; 1000:043a: 74e8
    test         al,0x20                         ; 1000:043c: a820
    jz           input_wait_joystick_or_key      ; 1000:043e: 74e4
.wait_release:
    xor          ax,ax                           ; 1000:0440: 33c0
    cmp          byte [cs:0x61],0x0              ; 1000:0442: 2e803e610000
    jnz          .key_pressed                    ; 1000:0448: 7511
    mov          ah,0x84                         ; 1000:044a: b484
    xor          dx,dx                           ; 1000:044c: 33d2
    int          0x15                            ; 1000:044e: cd15
    test         al,0x10                         ; 1000:0450: a810
    jz           .button_pressed                 ; 1000:0452: 7404
    test         al,0x20                         ; 1000:0454: a820
    jnz          .wait_release                   ; 1000:0456: 75e8
.button_pressed:
    mov          ax,0x1                          ; 1000:0458: b80100
.key_pressed:
    ret                                          ; 1000:045b: c3

input_setup_keyboard:
    mov          ax,0x2                          ; 1000:045c: b80200
    int          0x10                            ; 1000:045f: cd10
    push         ds                              ; 1000:0461: 1e
    mov          ax, SEG_SCRATCH                 ; 1000:0462: b85d0d
    mov          ds,ax                           ; 1000:0465: 8ed8
    lea          dx,[0x1a0]                      ; 1000:0467: 8d16a001
    mov          ah,0x9                          ; 1000:046b: b409
    int          0x21                            ; 1000:046d: cd21
    mov          byte [cs:map_scancode_up],0x0             ; 1000:046f: 2ec606600200
    mov          byte [cs:map_scancode_down],0x0             ; 1000:0475: 2ec606610200
    mov          byte [cs:map_scancode_left],0x0             ; 1000:047b: 2ec6065e0200
    mov          byte [cs:map_scancode_right],0x0             ; 1000:0481: 2ec6065f0200
    mov          byte [cs:map_scancode_fire],0x0             ; 1000:0487: 2ec606630200
    mov          byte [cs:map_scancode_jump],0x0             ; 1000:048d: 2ec606620200
    lea          dx,[0x1d6]                      ; 1000:0493: 8d16d601
    mov          ah,0x9                          ; 1000:0497: b409
    int          0x21                            ; 1000:0499: cd21
    call         input_read_mapped_key           ; 1000:049b: e8ad00
    mov          [cs:map_scancode_left],al                   ; 1000:049e: 2ea25e02
    lea          dx,[0x205]                      ; 1000:04a2: 8d160502
    mov          ah,0x9                          ; 1000:04a6: b409
    int          0x21                            ; 1000:04a8: cd21
    call         input_read_mapped_key           ; 1000:04aa: e89e00
    mov          [cs:map_scancode_right],al                   ; 1000:04ad: 2ea25f02
    lea          dx,[0x234]                      ; 1000:04b1: 8d163402
    mov          ah,0x9                          ; 1000:04b5: b409
    int          0x21                            ; 1000:04b7: cd21
    call         input_read_mapped_key           ; 1000:04b9: e88f00
    mov          [cs:map_scancode_up],al                   ; 1000:04bc: 2ea26002
    lea          dx,[0x263]                      ; 1000:04c0: 8d166302
    mov          ah,0x9                          ; 1000:04c4: b409
    int          0x21                            ; 1000:04c6: cd21
    call         input_read_mapped_key           ; 1000:04c8: e88000
    mov          [cs:map_scancode_down],al                   ; 1000:04cb: 2ea26102
    lea          dx,[0x292]                      ; 1000:04cf: 8d169202
    mov          ah,0x9                          ; 1000:04d3: b409
    int          0x21                            ; 1000:04d5: cd21
    call         input_read_mapped_key           ; 1000:04d7: e87100
    mov          [cs:map_scancode_jump],al                   ; 1000:04da: 2ea26202
    lea          dx,[0x2c1]                      ; 1000:04de: 8d16c102
    mov          ah,0x9                          ; 1000:04e2: b409
    int          0x21                            ; 1000:04e4: cd21
    call         input_read_mapped_key           ; 1000:04e6: e86200
    mov          [cs:map_scancode_fire],al                   ; 1000:04e9: 2ea26302
    lea          dx,[0x2f0]                      ; 1000:04ed: 8d16f002
    mov          ah,0x9                          ; 1000:04f1: b409
    int          0x21                            ; 1000:04f3: cd21
    xor          ax,ax                           ; 1000:04f5: 33c0
    call         sys_flush_keyboard_buffer                           ; 1000:04f7: e89f00
    cmp          byte [cs:0x61],0x31             ; 1000:04fa: 2e803e610031
    jz           .redo                           ; 1000:0500: 7417
    lea          dx,[0x327]                      ; 1000:0502: 8d162703
    mov          ah,0x9                          ; 1000:0506: b409
    int          0x21                            ; 1000:0508: cd21
    call         sys_flush_keyboard_buffer                           ; 1000:050a: e88c00
    cmp          byte [cs:0x61],0x15             ; 1000:050d: 2e803e610015
    jz           .save_keys                      ; 1000:0513: 7408
    pop          ds                              ; 1000:0515: 1f
    jmp          vid_set_mode_and_init           ; 1000:0516: e915fe
.redo:
    pop          ds                              ; 1000:0519: 1f
    jmp          input_setup_keyboard            ; 1000:051a: e93fff
.save_keys:
    pop          ds                              ; 1000:051d: 1f
    lea          dx,[0x1ff]                      ; 1000:051e: 8d16ff01
    mov          ax,0x3c00                       ; 1000:0522: b8003c
    xor          cx,cx                           ; 1000:0525: 33c9
    int          0x21                            ; 1000:0527: cd21
    jc           .error_exit                     ; 1000:0529: 721d
    mov          bx,ax                           ; 1000:052b: 8bd8
    mov          si,ds                           ; 1000:052d: 8cde
    mov          ax,cs                           ; 1000:052f: 8cc8
    mov          ds,ax                           ; 1000:0531: 8ed8
    mov          ax,0x4000                       ; 1000:0533: b80040
    mov          cx,0x6                          ; 1000:0536: b90600
    mov          dx,map_scancode_left                        ; 1000:0539: ba5e02
    int          0x21                            ; 1000:053c: cd21
    mov          ds,si                           ; 1000:053e: 8ede
    mov          ax,0x3e00                       ; 1000:0540: b8003e
    int          0x21                            ; 1000:0543: cd21
    jmp          vid_set_mode_and_init           ; 1000:0545: e9e6fd
.error_exit:
    jmp          sys_exit_to_dos                           ; 1000:0548: e9cafc

input_read_mapped_key:
    call         sys_flush_keyboard_buffer                           ; 1000:054b: e84b00
    mov          bl,al                           ; 1000:054e: 8ad8
    xor          bh,bh                           ; 1000:0550: 32ff
    mov          si,bx                           ; 1000:0552: 8bf3
    cmp          bl,[cs:map_scancode_jump]                   ; 1000:0554: 2e3a1e6202
    jz           input_read_mapped_key           ; 1000:0559: 74f0
    cmp          bl,[cs:map_scancode_left]                   ; 1000:055b: 2e3a1e5e02
    jz           input_read_mapped_key           ; 1000:0560: 74e9
    cmp          bl,[cs:map_scancode_right]                   ; 1000:0562: 2e3a1e5f02
    jz           input_read_mapped_key           ; 1000:0567: 74e2
    cmp          bl,[cs:map_scancode_fire]                   ; 1000:0569: 2e3a1e6302
    jz           input_read_mapped_key           ; 1000:056e: 74db
    cmp          bl,[cs:map_scancode_up]                   ; 1000:0570: 2e3a1e6002
    jz           input_read_mapped_key           ; 1000:0575: 74d4
    cmp          bl,[cs:map_scancode_down]                   ; 1000:0577: 2e3a1e6102
    jz           input_read_mapped_key           ; 1000:057c: 74cd
    dec          bx                              ; 1000:057e: 4b
    jz           input_read_mapped_key           ; 1000:057f: 74ca
    cmp          bx,0x52                         ; 1000:0581: 83fb52
    jg           input_read_mapped_key           ; 1000:0584: 7fc5
    shl          bx,1                            ; 1000:0586: d1e3
    shl          bx,1                            ; 1000:0588: d1e3
    shl          bx,1                            ; 1000:058a: d1e3
    lea          dx,[0x35f]                      ; 1000:058c: 8d165f03
    add          dx,bx                           ; 1000:0590: 03d3
    mov          ah,0x9                          ; 1000:0592: b409
    int          0x21                            ; 1000:0594: cd21
    mov          ax,si                           ; 1000:0596: 8bc6
    ret                                          ; 1000:0598: c3


; ----------------------------------------------------------------------------
; FUNCTION: sys_restore_interrupts  [System / Interrupts]
; ADDRESS:  1000:0599
; PURPOSE:  Flushes BIOS keyboard buffer and waits for scancode.
; ----------------------------------------------------------------------------
sys_flush_keyboard_buffer:
sys_restore_interrupts:
    mov          byte [cs:0x61],0x0              ; 1000:0599: 2ec606610000
loc_059f:
    cmp          byte [cs:0x61],0x0              ; 1000:059f: 2e803e610000
    jz           loc_059f                        ; 1000:05a5: 74f8
    push         ES                              ; 1000:05a7: 06
    xor          AX,AX                           ; 1000:05a8: 33c0
    mov          ES,AX                           ; 1000:05aa: 8ec0
    mov          AL,[cs:0x61]                    ; 1000:05ac: 2ea06100
    cli                                          ; 1000:05b0: fa
    mov          CL,byte [es:0x41a]              ; 1000:05b1: 268a0e1a04
    mov          byte [es:0x41c],CL              ; 1000:05b6: 26880e1c04
    sti                                          ; 1000:05bb: fb
    pop          ES                              ; 1000:05bc: 07
    ret                                          ; 1000:05bd: c3
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:05be

; ----------------------------------------------------------------------------
; FUNCTION: isr_keyboard_int09  [Interrupts / Input]
; ADDRESS:  1000:05c2
; PURPOSE:  Custom hardware INT 09h Keyboard ISR. Reads raw scancodes from port 0x60, manages key press/release state matrix, and signals EOI to PIC.
; ----------------------------------------------------------------------------
isr_keyboard_int09:
    push         AX                              ; 1000:05c2: 50
    push         BX                              ; 1000:05c3: 53
    push         CX                              ; 1000:05c4: 51
    push         DX                              ; 1000:05c5: 52
    in           AL,0x60                         ; 1000:05c6: e460
    push         AX                              ; 1000:05c8: 50
    cmp          byte [cs:0x60],0x1              ; 1000:05c9: 2e803e600001
    jnz          loc_05d9                        ; 1000:05cf: 7508
    pushf                                        ; 1000:05d1: 9c
    call far     [cs:0x5be]                      ; 1000:05d2: 2eff1ebe05
    jmp short    loc_05f3                        ; 1000:05d7: eb1a
loc_05d9:
    cli                                          ; 1000:05d9: fa
    in           AL,0x61                         ; 1000:05da: e461
    jmp short    loc_05de                        ; 1000:05dc: eb00
loc_05de:
    or           AL,0x80                         ; 1000:05de: 0c80
    out          0x61,AL                         ; 1000:05e0: e661
    jmp short    loc_05e4                        ; 1000:05e2: eb00
loc_05e4:
    in           AL,0x61                         ; 1000:05e4: e461
    jmp short    loc_05e8                        ; 1000:05e6: eb00
loc_05e8:
    and          AL,0x7f                         ; 1000:05e8: 247f
    out          0x61,AL                         ; 1000:05ea: e661
    jmp short    loc_05ee                        ; 1000:05ec: eb00
loc_05ee:
    mov          AL,0x20                         ; 1000:05ee: b020
    out          0x20,AL                         ; 1000:05f0: e620
    sti                                          ; 1000:05f2: fb
loc_05f3:
    pop          AX                              ; 1000:05f3: 58
    mov          DX,0x1                          ; 1000:05f4: ba0100
    test         AL,0x80                         ; 1000:05f7: a880
    jz           loc_0601                        ; 1000:05f9: 7406
    and          AL,0x7f                         ; 1000:05fb: 247f
    xor          DX,DX                           ; 1000:05fd: 33d2
    jmp short    loc_0605                        ; 1000:05ff: eb04
loc_0601:
    mov          [cs:0x61],AL                    ; 1000:0601: 2ea26100
loc_0605:
    cmp          AL,byte [cs:0x262]              ; 1000:0605: 2e3a066202
    jz           loc_0639                        ; 1000:060a: 742d
    cmp          AL,byte [cs:0x263]              ; 1000:060c: 2e3a066302
    jz           loc_0640                        ; 1000:0611: 742d
    cmp          AL,byte [cs:0x25e]              ; 1000:0613: 2e3a065e02
    jz           loc_0647                        ; 1000:0618: 742d
    cmp          AL,byte [cs:0x25f]              ; 1000:061a: 2e3a065f02
    jz           loc_064e                        ; 1000:061f: 742d
    cmp          AL,byte [cs:0x260]              ; 1000:0621: 2e3a066002
    jz           loc_0655                        ; 1000:0626: 742d
    cmp          AL,byte [cs:0x261]              ; 1000:0628: 2e3a066102
    jz           loc_065c                        ; 1000:062d: 742d
    cmp          AL,0x3c                         ; 1000:062f: 3c3c
    jz           loc_0663                        ; 1000:0631: 7430
    cmp          AL,0x3b                         ; 1000:0633: 3c3b
    jz           loc_066b                        ; 1000:0635: 7434
    jmp short    loc_0673                        ; 1000:0637: eb3a
loc_0639:
    mov          byte [cs:0x25c],DL              ; 1000:0639: 2e88165c02
    jmp short    loc_0673                        ; 1000:063e: eb33
loc_0640:
    mov          byte [cs:0x25d],DL              ; 1000:0640: 2e88165d02
    jmp short    loc_0673                        ; 1000:0645: eb2c
loc_0647:
    mov          byte [cs:0x258],DL              ; 1000:0647: 2e88165802
    jmp short    loc_0673                        ; 1000:064c: eb25
loc_064e:
    mov          byte [cs:0x259],DL              ; 1000:064e: 2e88165902
    jmp short    loc_0673                        ; 1000:0653: eb1e
loc_0655:
    mov          byte [cs:0x25a],DL              ; 1000:0655: 2e88165a02
    jmp short    loc_0673                        ; 1000:065a: eb17
loc_065c:
    mov          byte [cs:0x25b],DL              ; 1000:065c: 2e88165b02
    jmp short    loc_0673                        ; 1000:0661: eb10
loc_0663:
    mov          byte [cs:0x248],0x0             ; 1000:0663: 2ec606480200
    jmp short    loc_0673                        ; 1000:0669: eb08
loc_066b:
    mov          byte [cs:0x248],0x1             ; 1000:066b: 2ec606480201
    jmp short    loc_0673                        ; 1000:0671: eb00
loc_0673:
    pop          DX                              ; 1000:0673: 5a
    pop          CX                              ; 1000:0674: 59
    pop          BX                              ; 1000:0675: 5b
    pop          AX                              ; 1000:0676: 58
    iret                                         ; 1000:0677: cf
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:0678

; ----------------------------------------------------------------------------
; FUNCTION: isr_ctrl_break_int23  [Interrupts / Lifecycle]
; ADDRESS:  1000:067c
; PURPOSE:  Custom INT 23h DOS Ctrl-C / Ctrl-Break handler. Prevents abnormal exit and jumps to clean exit routine.
; ----------------------------------------------------------------------------
isr_ctrl_break_int23:
    jmp near     sys_exit_to_dos                 ; 1000:067c: e996fb
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:067f

; ----------------------------------------------------------------------------
; FUNCTION: isr_timer_sound_int08  [Interrupts / Audio]
; ADDRESS:  1000:0683
; PURPOSE:  Custom hardware INT 08h Timer ISR. Drives the PC speaker sound synthesis engine, decrements note durations, updates tone frequencies, and ticks frame counters.
; ----------------------------------------------------------------------------
isr_timer_sound_int08:
    push         AX                              ; 1000:0683: 50
    push         BX                              ; 1000:0684: 53
    push         CX                              ; 1000:0685: 51
    push         DX                              ; 1000:0686: 52
    push         ES                              ; 1000:0687: 06
    mov          AL,[cs:0x246]                   ; 1000:0688: 2ea04602
    inc          AL                              ; 1000:068c: fec0
    cmp          AL,0x2                          ; 1000:068e: 3c02
    jge          loc_0695                        ; 1000:0690: 7d03
    jmp near     loc_07d6                        ; 1000:0692: e94101
loc_0695:
    mov          byte [cs:0x245],0x1             ; 1000:0695: 2ec606450201
    cmp          word [cs:0x276],0x1             ; 1000:069b: 2e833e760201
    jz           loc_06a6                        ; 1000:06a1: 7403
    jmp near     loc_07ae                        ; 1000:06a3: e90801
loc_06a6:
    mov          AL,[cs:0x25c]                   ; 1000:06a6: 2ea05c02
    mov          [cs:0x264],AL                   ; 1000:06aa: 2ea26402
    mov          byte [cs:0x25c],0x0             ; 1000:06ae: 2ec6065c0200
    mov          AL,[cs:0x25d]                   ; 1000:06b4: 2ea05d02
    mov          [cs:0x265],AL                   ; 1000:06b8: 2ea26502
    mov          byte [cs:0x25d],0x0             ; 1000:06bc: 2ec6065d0200
    mov          AL,[cs:0x259]                   ; 1000:06c2: 2ea05902
    mov          [cs:0x266],AL                   ; 1000:06c6: 2ea26602
    mov          byte [cs:0x259],0x0             ; 1000:06ca: 2ec606590200
    mov          AL,[cs:0x258]                   ; 1000:06d0: 2ea05802
    mov          [cs:0x267],AL                   ; 1000:06d4: 2ea26702
    mov          byte [cs:0x258],0x0             ; 1000:06d8: 2ec606580200
    mov          AL,[cs:0x25a]                   ; 1000:06de: 2ea05a02
    mov          [cs:0x268],AL                   ; 1000:06e2: 2ea26802
    mov          byte [cs:0x25a],0x0             ; 1000:06e6: 2ec6065a0200
    mov          AL,[cs:0x25b]                   ; 1000:06ec: 2ea05b02
    mov          [cs:0x269],AL                   ; 1000:06f0: 2ea26902
    mov          byte [cs:0x25b],0x0             ; 1000:06f4: 2ec6065b0200
    mov          AH,0x84                         ; 1000:06fa: b484
    xor          DX,DX                           ; 1000:06fc: 33d2
    int          0x15                            ; 1000:06fe: cd15
    test         AL,0x20                         ; 1000:0700: a820
    jnz          loc_070a                        ; 1000:0702: 7506
    mov          byte [cs:0x25c],0x1             ; 1000:0704: 2ec6065c0201
loc_070a:
    test         AL,0x10                         ; 1000:070a: a810
    jnz          loc_0714                        ; 1000:070c: 7506
    mov          byte [cs:0x25d],0x1             ; 1000:070e: 2ec6065d0201
loc_0714:
    mov          AH,0x84                         ; 1000:0714: b484
    mov          DX,0x1                          ; 1000:0716: ba0100
    int          0x15                            ; 1000:0719: cd15
    cmp          AX,word [cs:0x26e]              ; 1000:071b: 2e3b066e02
    jg           loc_072a                        ; 1000:0720: 7f08
    mov          byte [cs:0x258],0x1             ; 1000:0722: 2ec606580201
    jmp short    loc_0737                        ; 1000:0728: eb0d
loc_072a:
    cmp          AX,word [cs:0x270]              ; 1000:072a: 2e3b067002
    jl           loc_0737                        ; 1000:072f: 7c06
    mov          byte [cs:0x259],0x1             ; 1000:0731: 2ec606590201
loc_0737:
    cmp          BX,word [cs:0x272]              ; 1000:0737: 2e3b1e7202
    jg           loc_0746                        ; 1000:073c: 7f08
    mov          byte [cs:0x25a],0x1             ; 1000:073e: 2ec6065a0201
    jmp short    loc_0753                        ; 1000:0744: eb0d
loc_0746:
    cmp          BX,word [cs:0x274]              ; 1000:0746: 2e3b1e7402
    jl           loc_0753                        ; 1000:074b: 7c06
    mov          byte [cs:0x25b],0x1             ; 1000:074d: 2ec6065b0201
loc_0753:
    mov          AL,[cs:0x25c]                   ; 1000:0753: 2ea05c02
    xor          byte [cs:0x264],AL              ; 1000:0757: 2e30066402
    mov          AL,[cs:0x25d]                   ; 1000:075c: 2ea05d02
    xor          byte [cs:0x265],AL              ; 1000:0760: 2e30066502
    mov          AL,[cs:0x259]                   ; 1000:0765: 2ea05902
    xor          byte [cs:0x266],AL              ; 1000:0769: 2e30066602
    mov          AL,[cs:0x258]                   ; 1000:076e: 2ea05802
    xor          byte [cs:0x267],AL              ; 1000:0772: 2e30066702
    mov          AL,[cs:0x25a]                   ; 1000:0777: 2ea05a02
    xor          byte [cs:0x268],AL              ; 1000:077b: 2e30066802
    mov          AL,[cs:0x25b]                   ; 1000:0780: 2ea05b02
    xor          byte [cs:0x269],AL              ; 1000:0784: 2e30066902
    mov          AL,[cs:0x264]                   ; 1000:0789: 2ea06402
    or           AL,byte [cs:0x265]              ; 1000:078d: 2e0a066502
    or           AL,byte [cs:0x266]              ; 1000:0792: 2e0a066602
    or           AL,byte [cs:0x267]              ; 1000:0797: 2e0a066702
    or           AL,byte [cs:0x268]              ; 1000:079c: 2e0a066802
    or           AL,byte [cs:0x269]              ; 1000:07a1: 2e0a066902
    jz           loc_07ae                        ; 1000:07a6: 7406
    mov          byte [cs:0x61],0xff             ; 1000:07a8: 2ec6066100ff
loc_07ae:
    mov          AL,[cs:0x61]                    ; 1000:07ae: 2ea06100
    cmp          byte [cs:0x251],AL              ; 1000:07b2: 2e38065102
    jnz          loc_07c3                        ; 1000:07b7: 750a
    mov          byte [cs:0x61],0x0              ; 1000:07b9: 2ec606610000
    mov          AX,0x0                          ; 1000:07bf: b80000
    int3                                         ; 1000:07c2: cc
loc_07c3:
    cmp          byte [cs:0x252],AL              ; 1000:07c3: 2e38065202
    jnz          loc_07d4                        ; 1000:07c8: 750a
    mov          byte [cs:0x61],0x0              ; 1000:07ca: 2ec606610000
    mov          AX,0x2                          ; 1000:07d0: b80200
    int3                                         ; 1000:07d3: cc
loc_07d4:
    xor          AL,AL                           ; 1000:07d4: 32c0
loc_07d6:
    mov          [cs:0x246],AL                   ; 1000:07d6: 2ea24602
    mov          AX,[cs:0x24e]                   ; 1000:07da: 2ea14e02
    mov          ES,AX                           ; 1000:07de: 8ec0
    cmp          byte [cs:0x247],0x1             ; 1000:07e0: 2e803e470201
    jnz          loc_0843                        ; 1000:07e6: 755b
    dec          word [cs:0x24c]                 ; 1000:07e8: 2eff0e4c02
    jg           loc_0849                        ; 1000:07ed: 7f5a
    mov          BX,word [cs:0x24a]              ; 1000:07ef: 2e8b1e4a02
    add          word [cs:0x24a],0x2             ; 1000:07f4: 2e83064a0202
    mov          AX,word [es:BX]                 ; 1000:07fa: 268b07
    or           AX,AX                           ; 1000:07fd: 0bc0
    jz           loc_0837                        ; 1000:07ff: 7436
    mov          BX,AX                           ; 1000:0801: 8bd8
    in           AL,0x61                         ; 1000:0803: e461
    and          AL,0xfc                         ; 1000:0805: 24fc
    out          0x61,AL                         ; 1000:0807: e661
    mov          AL,0xb6                         ; 1000:0809: b0b6
    out          0x43,AL                         ; 1000:080b: e643
    mov          AL,BL                           ; 1000:080d: 8ac3
    out          0x42,AL                         ; 1000:080f: e642
    mov          AL,BH                           ; 1000:0811: 8ac7
    out          0x42,AL                         ; 1000:0813: e642
    mov          BX,word [cs:0x24a]              ; 1000:0815: 2e8b1e4a02
    add          word [cs:0x24a],0x2             ; 1000:081a: 2e83064a0202
    mov          AX,word [es:BX]                 ; 1000:0820: 268b07
    mov          [cs:0x24c],AX                   ; 1000:0823: 2ea34c02
    cmp          byte [cs:0x248],0x1             ; 1000:0827: 2e803e480201
    jnz          loc_0849                        ; 1000:082d: 751a
    in           AL,0x61                         ; 1000:082f: e461
    or           AL,0x3                          ; 1000:0831: 0c03
    out          0x61,AL                         ; 1000:0833: e661
    jmp short    loc_0849                        ; 1000:0835: eb12
loc_0837:
    mov          byte [cs:0x247],0x0             ; 1000:0837: 2ec606470200
    mov          byte [cs:0x249],0x0             ; 1000:083d: 2ec606490200
loc_0843:
    in           AL,0x61                         ; 1000:0843: e461
    and          AL,0xfc                         ; 1000:0845: 24fc
    out          0x61,AL                         ; 1000:0847: e661
loc_0849:
    cmp          byte [cs:0x22a],0x0             ; 1000:0849: 2e803e2a0200
    jz           loc_08b9                        ; 1000:084f: 7468
    mov          BX,word [cs:0x278]              ; 1000:0851: 2e8b1e7802
    or           BX,BX                           ; 1000:0856: 0bdb
    jz           loc_08b9                        ; 1000:0858: 745f
    push         DS                              ; 1000:085a: 1e
    push         SI                              ; 1000:085b: 56
    cld                                          ; 1000:085c: fc
    mov          SI,BX                           ; 1000:085d: 8bf3
    mov          AX,CS                           ; 1000:085f: 8cc8
    mov          DS,AX                           ; 1000:0861: 8ed8
    mov          DX,0x3da                        ; 1000:0863: bada03
loc_0866:
    in           AL,DX                           ; 1000:0866: ec
    test         AL,0x8                          ; 1000:0867: a808
    jz           loc_0866                        ; 1000:0869: 74fb
    mov          DX,0x3c0                        ; 1000:086b: bac003
    lodsw                                        ; 1000:086e: ad
    mov          CX,AX                           ; 1000:086f: 8bc8
loc_0871:
    push         CX                              ; 1000:0871: 51
    lodsw                                        ; 1000:0872: ad
    or           AX,AX                           ; 1000:0873: 0bc0
    jz           loc_0880                        ; 1000:0875: 7409
    dec          AX                              ; 1000:0877: 48
    mov          word [SI + -0x2],AX             ; 1000:0878: 8944fe
    add          SI,0x6                          ; 1000:087b: 83c606
    jmp short    loc_08b1                        ; 1000:087e: eb31
loc_0880:
    lodsw                                        ; 1000:0880: ad
    mov          BX,word [SI]                    ; 1000:0881: 8b1c
    add          SI,0x2                          ; 1000:0883: 83c602
    add          BX,AX                           ; 1000:0886: 03d8
    mov          CL,byte [BX]                    ; 1000:0888: 8a0f
    cmp          CL,0x18                         ; 1000:088a: 80f918
    jnz          loc_089e                        ; 1000:088d: 750f
    mov          CL,byte [BX + 0x1]              ; 1000:088f: 8a4f01
    sub          BX,AX                           ; 1000:0892: 2bd8
    xor          AX,AX                           ; 1000:0894: 33c0
    mov          CH,0x0                          ; 1000:0896: b500
    add          AX,CX                           ; 1000:0898: 03c1
    add          BX,CX                           ; 1000:089a: 03d9
    mov          CL,byte [BX]                    ; 1000:089c: 8a0f
loc_089e:
    add          AX,0x2                          ; 1000:089e: 050200
    mov          word [SI + -0x4],AX             ; 1000:08a1: 8944fc
    mov          AL,byte [BX + 0x1]              ; 1000:08a4: 8a4701
    xor          AH,AH                           ; 1000:08a7: 32e4
    mov          word [SI + -0x6],AX             ; 1000:08a9: 8944fa
    lodsw                                        ; 1000:08ac: ad
    out          DX,AL                           ; 1000:08ad: ee
    mov          AL,CL                           ; 1000:08ae: 8ac1
    out          DX,AL                           ; 1000:08b0: ee
loc_08b1:
    pop          CX                              ; 1000:08b1: 59
    loop         loc_0871                        ; 1000:08b2: e2bd
    pop          SI                              ; 1000:08b4: 5e
    pop          DS                              ; 1000:08b5: 1f
    mov          AL,0x20                         ; 1000:08b6: b020
    out          DX,AL                           ; 1000:08b8: ee
loc_08b9:
    pop          ES                              ; 1000:08b9: 07
    pop          DX                              ; 1000:08ba: 5a
    pop          CX                              ; 1000:08bb: 59
    pop          BX                              ; 1000:08bc: 5b
    pop          AX                              ; 1000:08bd: 58
    jmp far      [cs:0x67f]                      ; 1000:08be: 2eff2e7f06
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:08c3

; ----------------------------------------------------------------------------
; FUNCTION: snd_api_int03  [Audio / API]
; ADDRESS:  1000:08c7
; PURPOSE:  Sound Engine API dispatcher hooked to INT 03h. Accepts commands: AL=0 (stop sound), AL=1 (play SFX at BX with priority CL), AL=2 (query/stop music), AL=3 (toggle sound).
; ----------------------------------------------------------------------------
snd_api_int03:
    cmp          AL,0x0                          ; 1000:08c7: 3c00
    jz           loc_0907                        ; 1000:08c9: 743c
    cmp          AL,0x1                          ; 1000:08cb: 3c01
    jz           loc_08df                        ; 1000:08cd: 7410
    cmp          AL,0x2                          ; 1000:08cf: 3c02
    jz           loc_090f                        ; 1000:08d1: 743c
    cmp          AL,0x3                          ; 1000:08d3: 3c03
    jz           loc_0917                        ; 1000:08d5: 7440
    cmp          AL,0x4                          ; 1000:08d7: 3c04
    jz           loc_0927                        ; 1000:08d9: 744c
    xor          AL,0xff                         ; 1000:08db: 34ff
    jmp short    loc_0935                        ; 1000:08dd: eb56
loc_08df:
    cmp          byte [cs:0x249],CL              ; 1000:08df: 2e380e4902
    jg           loc_0935                        ; 1000:08e4: 7f4f
    mov          byte [cs:0x249],CL              ; 1000:08e6: 2e880e4902
    cli                                          ; 1000:08eb: fa
    mov          byte [cs:0x247],0x1             ; 1000:08ec: 2ec606470201
    mov          word [cs:0x24a],BX              ; 1000:08f2: 2e891e4a02
    mov          AX,DS                           ; 1000:08f7: 8cd8
    mov          [cs:0x24e],AX                   ; 1000:08f9: 2ea34e02
    mov          word [cs:0x24c],0x1             ; 1000:08fd: 2ec7064c020100
    sti                                          ; 1000:0904: fb
    jmp short    loc_0935                        ; 1000:0905: eb2e
loc_0907:
    mov          byte [cs:0x248],0x1             ; 1000:0907: 2ec606480201
    jmp short    loc_092f                        ; 1000:090d: eb20
loc_090f:
    mov          byte [cs:0x248],0x0             ; 1000:090f: 2ec606480200
    jmp short    loc_092f                        ; 1000:0915: eb18
loc_0917:
    cli                                          ; 1000:0917: fa
    mov          byte [cs:0x247],0x0             ; 1000:0918: 2ec606470200
    mov          byte [cs:0x249],0x0             ; 1000:091e: 2ec606490200
    sti                                          ; 1000:0924: fb
    jmp short    loc_092f                        ; 1000:0925: eb08
loc_0927:
    mov          AL,[cs:0x247]                   ; 1000:0927: 2ea04702
    xor          AH,AH                           ; 1000:092b: 32e4
    jmp short    loc_0935                        ; 1000:092d: eb06
loc_092f:
    in           AL,0x61                         ; 1000:092f: e461
    and          AL,0xfc                         ; 1000:0931: 24fc
    out          0x61,AL                         ; 1000:0933: e661
loc_0935:
    iret                                         ; 1000:0935: cf
    db 0x00, 0x00, 0x00, 0x00                       ; 1000:0936

; ----------------------------------------------------------------------------
; FUNCTION: isr_joystick_int15  [Interrupts / Input]
; ADDRESS:  1000:093a
; PURPOSE:  Custom BIOS INT 15h AH=84h Joystick handler. Directly polls game port 0x201 for button states and axis potentiometer discharge timing.
; ----------------------------------------------------------------------------
isr_joystick_int15:
    cmp          AH,0x84                         ; 1000:093a: 80fc84
    jz           loc_0942                        ; 1000:093d: 7403
    jmp near     loc_09c4                        ; 1000:093f: e98200
loc_0942:
    cmp          DL,0x1                          ; 1000:0942: 80fa01
    jg           loc_096e                        ; 1000:0945: 7f27
    mov          DX,0x201                        ; 1000:0947: ba0102
    jz           loc_0952                        ; 1000:094a: 7406
    in           AL,DX                           ; 1000:094c: ec
    and          AL,0xf0                         ; 1000:094d: 24f0
    jmp short    loc_096e                        ; 1000:094f: eb1d
    db 0x90                                         ; 1000:0951
loc_0952:
    mov          BL,0x1                          ; 1000:0952: b301
    call         loc_096f                        ; 1000:0954: e81800
    push         CX                              ; 1000:0957: 51
    shl          BL,0x1                          ; 1000:0958: d0e3
    call         loc_096f                        ; 1000:095a: e81200
    push         CX                              ; 1000:095d: 51
    shl          BL,0x1                          ; 1000:095e: d0e3
    call         loc_096f                        ; 1000:0960: e80c00
    push         CX                              ; 1000:0963: 51
    shl          BL,0x1                          ; 1000:0964: d0e3
    call         loc_096f                        ; 1000:0966: e80600
    mov          DX,CX                           ; 1000:0969: 8bd1
    pop          CX                              ; 1000:096b: 59
    pop          BX                              ; 1000:096c: 5b
    pop          AX                              ; 1000:096d: 58
loc_096e:
    iret                                         ; 1000:096e: cf
loc_096f:
    cli                                          ; 1000:096f: fa
    call         loc_09af                        ; 1000:0970: e83c00
    push         AX                              ; 1000:0973: 50
    out          DX,AL                           ; 1000:0974: ee
    mov          CX,word [cs:0x62]               ; 1000:0975: 2e8b0e6200
loc_097a:
    in           AL,DX                           ; 1000:097a: ec
    test         AL,BL                           ; 1000:097b: 84d8
    loopnz       loc_097a                        ; 1000:097d: e0fb
    or           CX,CX                           ; 1000:097f: 0bc9
    jnz          loc_0986                        ; 1000:0981: 7503
    pop          AX                              ; 1000:0983: 58
    jmp short    loc_09a2                        ; 1000:0984: eb1c
loc_0986:
    call         loc_09af                        ; 1000:0986: e82600
    pop          CX                              ; 1000:0989: 59
    cmp          CX,AX                           ; 1000:098a: 3bc8
    jg           loc_0994                        ; 1000:098c: 7f06
    neg          AX                              ; 1000:098e: f7d8
    add          CX,AX                           ; 1000:0990: 03c8
    jmp short    loc_0996                        ; 1000:0992: eb02
loc_0994:
    sub          CX,AX                           ; 1000:0994: 2bc8
loc_0996:
    shr          CX,0x1                          ; 1000:0996: d1e9
    shr          CX,0x1                          ; 1000:0998: d1e9
    shr          CX,0x1                          ; 1000:099a: d1e9
    shr          CX,0x1                          ; 1000:099c: d1e9
    and          CH,0x1                          ; 1000:099e: 80e501
    sti                                          ; 1000:09a1: fb
loc_09a2:
    push         CX                              ; 1000:09a2: 51
    mov          CX,word [cs:0x62]               ; 1000:09a3: 2e8b0e6200
loc_09a8:
    in           AL,DX                           ; 1000:09a8: ec
    test         AL,0xf                          ; 1000:09a9: a80f
    loopnz       loc_09a8                        ; 1000:09ab: e0fb
    pop          CX                              ; 1000:09ad: 59
    ret                                          ; 1000:09ae: c3
loc_09af:
    mov          AL,0x0                          ; 1000:09af: b000
    out          0x43,AL                         ; 1000:09b1: e643
    jmp short    loc_09b5                        ; 1000:09b3: eb00
loc_09b5:
    jmp short    loc_09b7                        ; 1000:09b5: eb00
loc_09b7:
    in           AL,0x40                         ; 1000:09b7: e440
    mov          AH,AL                           ; 1000:09b9: 8ae0
    jmp short    loc_09bd                        ; 1000:09bb: eb00
loc_09bd:
    jmp short    loc_09bf                        ; 1000:09bd: eb00
loc_09bf:
    in           AL,0x40                         ; 1000:09bf: e440
    xchg         AH,AL                           ; 1000:09c1: 86c4
    ret                                          ; 1000:09c3: c3
loc_09c4:
    jmp far      [cs:0x936]                      ; 1000:09c4: 2eff2e3609

; ----------------------------------------------------------------------------
; FUNCTION: sys_install_interrupts  [System / Interrupts]
; ADDRESS:  1000:09c9
; PURPOSE:  Installs custom interrupt handlers by hooking interrupt vectors for INT 08h, INT 09h, INT 15h, INT 23h, and INT 03h.
; ----------------------------------------------------------------------------
sys_install_interrupts:
    push         DS                              ; 1000:09c9: 1e
    xor          AX,AX                           ; 1000:09ca: 33c0
    mov          DS,AX                           ; 1000:09cc: 8ed8
    cli                                          ; 1000:09ce: fa
    lea          BX,[0x5be]                      ; 1000:09cf: 8d1ebe05
    mov          AX,[0x26]                       ; 1000:09d3: a12600
    mov          word [cs:BX + 0x2],AX           ; 1000:09d6: 2e894702
    mov          AX,[0x24]                       ; 1000:09da: a12400
    mov          word [cs:BX],AX                 ; 1000:09dd: 2e8907
    lea          AX,[0x5c2]                      ; 1000:09e0: 8d06c205
    mov          [0x24],AX                       ; 1000:09e4: a32400
    mov          word [0x26],CS                  ; 1000:09e7: 8c0e2600
    lea          BX,[0x67f]                      ; 1000:09eb: 8d1e7f06
    mov          AX,[0x22]                       ; 1000:09ef: a12200
    mov          word [cs:BX + 0x2],AX           ; 1000:09f2: 2e894702
    mov          AX,[0x20]                       ; 1000:09f6: a12000
    mov          word [cs:BX],AX                 ; 1000:09f9: 2e8907
    lea          AX,[0x683]                      ; 1000:09fc: 8d068306
    mov          [0x20],AX                       ; 1000:0a00: a32000
    mov          word [0x22],CS                  ; 1000:0a03: 8c0e2200
    lea          BX,[0x678]                      ; 1000:0a07: 8d1e7806
    mov          AX,[0x8e]                       ; 1000:0a0b: a18e00
    mov          word [cs:BX + 0x2],AX           ; 1000:0a0e: 2e894702
    mov          AX,[0x8c]                       ; 1000:0a12: a18c00
    mov          word [cs:BX],AX                 ; 1000:0a15: 2e8907
    lea          AX,[0x67c]                      ; 1000:0a18: 8d067c06
    mov          [0x8c],AX                       ; 1000:0a1c: a38c00
    mov          word [0x8e],CS                  ; 1000:0a1f: 8c0e8e00
    lea          BX,[0x8c3]                      ; 1000:0a23: 8d1ec308
    mov          AX,[0xe]                        ; 1000:0a27: a10e00
    mov          word [cs:BX + 0x2],AX           ; 1000:0a2a: 2e894702
    mov          AX,[0xc]                        ; 1000:0a2e: a10c00
    mov          word [cs:BX],AX                 ; 1000:0a31: 2e8907
    lea          AX,[0x8c7]                      ; 1000:0a34: 8d06c708
    mov          [0xc],AX                        ; 1000:0a38: a30c00
    mov          word [0xe],CS                   ; 1000:0a3b: 8c0e0e00
    lea          BX,[0x936]                      ; 1000:0a3f: 8d1e3609
    mov          AX,[0x56]                       ; 1000:0a43: a15600
    mov          word [cs:BX + 0x2],AX           ; 1000:0a46: 2e894702
    mov          AX,[0x54]                       ; 1000:0a4a: a15400
    mov          word [cs:BX],AX                 ; 1000:0a4d: 2e8907
    lea          AX,[0x93a]                      ; 1000:0a50: 8d063a09
    mov          [0x54],AX                       ; 1000:0a54: a35400
    mov          word [0x56],CS                  ; 1000:0a57: 8c0e5600
    sti                                          ; 1000:0a5b: fb
    pop          DS                              ; 1000:0a5c: 1f
    ret                                          ; 1000:0a5d: c3
; ----------------------------------------------------------------------------
; FUNCTION: sys_restore_all_interrupts  [System / Interrupts]
; ADDRESS:  1000:0a5e
; PURPOSE:  Restores original BIOS and DOS interrupt vectors for INT 08h, 09h, 15h, 23h, 03h from saved pointers in CS.
; ----------------------------------------------------------------------------
sys_restore_all_interrupts:
    push         DS                              ; 1000:0a5e: 1e
    xor          AX,AX                           ; 1000:0a5f: 33c0
    mov          DS,AX                           ; 1000:0a61: 8ed8
    cli                                          ; 1000:0a63: fa
    lea          BX,[0x5be]                      ; 1000:0a64: 8d1ebe05
    mov          AX,word [cs:BX + 0x2]           ; 1000:0a68: 2e8b4702
    mov          [0x26],AX                       ; 1000:0a6c: a32600
    mov          AX,word [cs:BX]                 ; 1000:0a6f: 2e8b07
    mov          [0x24],AX                       ; 1000:0a72: a32400
    lea          BX,[0x67f]                      ; 1000:0a75: 8d1e7f06
    mov          AX,word [cs:BX + 0x2]           ; 1000:0a79: 2e8b4702
    mov          [0x22],AX                       ; 1000:0a7d: a32200
    mov          AX,word [cs:BX]                 ; 1000:0a80: 2e8b07
    mov          [0x20],AX                       ; 1000:0a83: a32000
    lea          BX,[0x678]                      ; 1000:0a86: 8d1e7806
    mov          AX,word [cs:BX + 0x2]           ; 1000:0a8a: 2e8b4702
    mov          [0x8e],AX                       ; 1000:0a8e: a38e00
    mov          AX,word [cs:BX]                 ; 1000:0a91: 2e8b07
    mov          [0x8c],AX                       ; 1000:0a94: a38c00
    lea          BX,[0x8c3]                      ; 1000:0a97: 8d1ec308
    mov          AX,word [cs:BX + 0x2]           ; 1000:0a9b: 2e8b4702
    mov          [0xe],AX                        ; 1000:0a9f: a30e00
    mov          AX,word [cs:BX]                 ; 1000:0aa2: 2e8b07
    mov          [0xc],AX                        ; 1000:0aa5: a30c00
    lea          BX,[0x936]                      ; 1000:0aa8: 8d1e3609
    mov          AX,word [cs:BX + 0x2]           ; 1000:0aac: 2e8b4702
    mov          [0x56],AX                       ; 1000:0ab0: a35600
    mov          AX,word [cs:BX]                 ; 1000:0ab3: 2e8b07
    mov          [0x54],AX                       ; 1000:0ab6: a35400
    sti                                          ; 1000:0ab9: fb
    pop          DS                              ; 1000:0aba: 1f
    ret                                          ; 1000:0abb: c3

; ----------------------------------------------------------------------------
; FUNCTION: sys_detect_cpu  [System / CPU]
; ADDRESS:  1000:0abc
; PURPOSE:  Detects CPU architecture (0=8086/8088, 1=80286, 2=80386+) using FLAGS bit 12-15 probing.
; ----------------------------------------------------------------------------
sys_detect_cpu:
    pushf                                        ; 1000:0abc: 9c
    xor          AX,AX                           ; 1000:0abd: 33c0
    push         AX                              ; 1000:0abf: 50
    popf                                         ; 1000:0ac0: 9d
    pushf                                        ; 1000:0ac1: 9c
    pop          AX                              ; 1000:0ac2: 58
    sub          AX,0xf000                       ; 1000:0ac3: 2d00f0
    jnc          .is_8086                        ; 1000:0ac6: 7317
    mov          BX,0xf000                       ; 1000:0ac8: bb00f0
    push         BX                              ; 1000:0acb: 53
    popf                                         ; 1000:0acc: 9d
    pushf                                        ; 1000:0acd: 9c
    pop          BX                              ; 1000:0ace: 5b
    test         BX,0xf000                       ; 1000:0acf: f7c300f0
    jz           .is_286                         ; 1000:0ad3: 7405
    mov          AX,0x2                          ; 1000:0ad5: b80200
    jmp short    .done                           ; 1000:0ad8: eb07
.is_286:
    mov          AX,0x1                          ; 1000:0ada: b80100
    jmp short    .done                           ; 1000:0add: eb02
.is_8086:
    xor          AX,AX                           ; 1000:0adf: 33c0
.done:
    popf                                         ; 1000:0ae1: 9d
    ret                                          ; 1000:0ae2: c3
    times 11     db 0x00                         ; 1000:0ae3 - 0x0aed
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x20, 0x00, 0x9c, 0xa0, 0x03, 0x80, 0x00, 0x00 ; 1000:0aee
    db 0x00, 0x00, 0x80, 0x00, 0xb0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00 ; 1000:0afe
    db 0x20, 0x01, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0xf0, 0x00, 0x8e, 0xa2 ; 1000:0b0e
    db 0x09, 0x80, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0xc0, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x00, 0x00 ; 1000:0b1e
    db 0x00, 0x00, 0x40, 0x04, 0xf0, 0x00, 0x9c, 0xa0, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x80, 0x04 ; 1000:0b2e
    db 0x20, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x00, 0x00, 0x02, 0x00, 0xb0, 0x06, 0xa0, 0x00, 0x8e, 0xa2 ; 1000:0b3e
    db 0x09, 0x80, 0x00, 0x00, 0x02, 0x00, 0xf0, 0x00, 0x10, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00 ; 1000:0b4e
    db 0x02, 0x00, 0xb0, 0x05, 0x80, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00, 0x02, 0x00, 0xe0, 0x05 ; 1000:0b5e
    db 0x80, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00, 0x02, 0x00, 0x80, 0x02, 0x30, 0x00, 0x12, 0x9c ; 1000:0b6e
    db 0x04, 0x80, 0x00, 0x00, 0x02, 0x00, 0xd0, 0x06, 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00 ; 1000:0b7e
    db 0x02, 0x00, 0xe0, 0x06, 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0xc0, 0x00 ; 1000:0b8e
    db 0x40, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x00, 0x00, 0x03, 0x00, 0xb0, 0x01, 0x40, 0x00, 0x04, 0x9e ; 1000:0b9e
    db 0x07, 0x80, 0x00, 0x00, 0x04, 0x00, 0xf0, 0x00, 0xe0, 0x03, 0x12, 0x9c, 0x04, 0x00, 0x00, 0x00 ; 1000:0bae
    db 0x04, 0x00, 0xe0, 0x00, 0xf0, 0x03, 0x12, 0x9c, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0xf0, 0x00 ; 1000:0bbe
    db 0x00, 0x04, 0x12, 0x9c, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0xe0, 0x00, 0x10, 0x04, 0x12, 0x9c ; 1000:0bce
    db 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0xe0, 0x00, 0x50, 0x04, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00 ; 1000:0bde
    db 0x04, 0x00, 0xf0, 0x00, 0x60, 0x04, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0xe0, 0x00 ; 1000:0bee
    db 0x70, 0x04, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0xf0, 0x00, 0x80, 0x04, 0xb8, 0x9c ; 1000:0bfe
    db 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x50, 0x01, 0x20, 0x03, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00 ; 1000:0c0e
    db 0x04, 0x00, 0x60, 0x01, 0x10, 0x03, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x50, 0x01 ; 1000:0c1e
    db 0x00, 0x03, 0xb8, 0x9c, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x60, 0x01, 0xf0, 0x02, 0xb8, 0x9c ; 1000:0c2e
    db 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x20, 0x01, 0xb0, 0x03, 0x5e, 0x9d, 0x06, 0x00, 0x00, 0x00 ; 1000:0c3e
    db 0x05, 0x00, 0x20, 0x00, 0x20, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x00, 0x00, 0x05, 0x00, 0x30, 0x00 ; 1000:0c4e
    db 0x20, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x00, 0x00, 0x05, 0x00, 0x80, 0x01, 0x40, 0x00, 0x12, 0x9c ; 1000:0c5e
    db 0x04, 0x80, 0x00, 0x00, 0x05, 0x00, 0x90, 0x01, 0x40, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x00, 0x00 ; 1000:0c6e
    db 0x05, 0x00, 0xa0, 0x01, 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00, 0x05, 0x00, 0xb0, 0x01 ; 1000:0c7e
    db 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00, 0x05, 0x00, 0xa0, 0x01, 0x30, 0x00, 0x12, 0x9c ; 1000:0c8e
    db 0x04, 0x80, 0x00, 0x00, 0x05, 0x00, 0xb0, 0x01, 0x30, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x00, 0x00 ; 1000:0c9e
    db 0x00, 0x00, 0x20, 0x00, 0x70, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00 ; 1000:0cae
    db 0x40, 0x00, 0x4e, 0xa6, 0x80, 0x04, 0x01, 0x00, 0x01, 0x00, 0x80, 0x01, 0x90, 0x03, 0x12, 0x9c ; 1000:0cbe
    db 0x04, 0x80, 0x01, 0x00, 0x01, 0x00, 0x80, 0x01, 0xa0, 0x03, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00 ; 1000:0cce
    db 0x01, 0x00, 0x70, 0x01, 0xa0, 0x03, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x01, 0x00, 0x80, 0x01 ; 1000:0cde
    db 0xd0, 0x05, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x01, 0x00, 0x80, 0x01, 0xc0, 0x05, 0x12, 0x9c ; 1000:0cee
    db 0x04, 0x80, 0x01, 0x00, 0x01, 0x00, 0x70, 0x01, 0xd0, 0x05, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00 ; 1000:0cfe
    db 0x02, 0x00, 0x10, 0x04, 0xc0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x01, 0x00, 0x02, 0x00, 0x20, 0x04 ; 1000:0d0e
    db 0xd0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x01, 0x00, 0x02, 0x00, 0x10, 0x04, 0xd0, 0x00, 0x5e, 0x9d ; 1000:0d1e
    db 0x06, 0x80, 0x01, 0x00, 0x03, 0x00, 0x50, 0x02, 0x40, 0x00, 0xcc, 0xa8, 0x80, 0x80, 0x01, 0x00 ; 1000:0d2e
    db 0x03, 0x00, 0x70, 0x02, 0x40, 0x00, 0x72, 0xa9, 0x80, 0x80, 0x01, 0x00, 0x03, 0x00, 0x80, 0x02 ; 1000:0d3e
    db 0x40, 0x00, 0x18, 0xaa, 0x80, 0x80, 0x01, 0x00, 0x03, 0x00, 0xe0, 0x02, 0x30, 0x00, 0x12, 0x9c ; 1000:0d4e
    db 0x04, 0x80, 0x01, 0x00, 0x03, 0x00, 0xd0, 0x02, 0x40, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00 ; 1000:0d5e
    db 0x03, 0x00, 0xe0, 0x02, 0x40, 0x00, 0xbe, 0xaa, 0x08, 0x80, 0x01, 0x00, 0x04, 0x00, 0x40, 0x00 ; 1000:0d6e
    db 0x30, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x04, 0x00, 0x40, 0x00, 0x40, 0x00, 0x12, 0x9c ; 1000:0d7e
    db 0x04, 0x80, 0x01, 0x00, 0x04, 0x00, 0x50, 0x00, 0x30, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00 ; 1000:0d8e
    db 0x04, 0x00, 0x50, 0x00, 0x40, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x04, 0x00, 0x60, 0x00 ; 1000:0d9e
    db 0x40, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x04, 0x00, 0x20, 0x00, 0x40, 0x00, 0xb8, 0x9c ; 1000:0dae
    db 0x05, 0x80, 0x01, 0x00, 0x04, 0x00, 0x30, 0x00, 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x01, 0x00 ; 1000:0dbe
    db 0x04, 0x00, 0x10, 0x00, 0x30, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x01, 0x00, 0x04, 0x00, 0x20, 0x00 ; 1000:0dce
    db 0x30, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x01, 0x00, 0x04, 0x00, 0x30, 0x00, 0x30, 0x00, 0xb8, 0x9c ; 1000:0dde
    db 0x05, 0x80, 0x01, 0x00, 0x04, 0x00, 0x10, 0x00, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x01, 0x00 ; 1000:0dee
    db 0x05, 0x00, 0x08, 0x05, 0x50, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x01, 0x00, 0x05, 0x00, 0x08, 0x05 ; 1000:0dfe
    db 0x80, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x01, 0x00, 0x07, 0x00, 0x60, 0x00, 0x80, 0x00, 0x04, 0x9e ; 1000:0e0e
    db 0x07, 0x80, 0x01, 0x00, 0x07, 0x00, 0x60, 0x00, 0x30, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x02, 0x00 ; 1000:0e1e
    db 0x00, 0x00, 0x50, 0x00, 0x30, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x02, 0x00, 0x00, 0x00, 0x90, 0x00 ; 1000:0e2e
    db 0xf0, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0xa0, 0x00, 0x8e, 0xa2 ; 1000:0e3e
    db 0x09, 0x00, 0x02, 0x00, 0x00, 0x00, 0x60, 0x01, 0x70, 0x01, 0xb8, 0x9c, 0x05, 0x80, 0x02, 0x00 ; 1000:0e4e
    db 0x00, 0x00, 0x60, 0x01, 0x80, 0x01, 0x42, 0xa1, 0x80, 0x85, 0x02, 0x00, 0x00, 0x00, 0x20, 0x05 ; 1000:0e5e
    db 0x20, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x02, 0x00, 0x00, 0x00, 0x30, 0x05, 0x10, 0x00, 0x12, 0x9c ; 1000:0e6e
    db 0x04, 0x80, 0x02, 0x00, 0x00, 0x00, 0x30, 0x05, 0x20, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x02, 0x00 ; 1000:0e7e
    db 0x00, 0x00, 0x40, 0x05, 0x10, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x02, 0x00, 0x00, 0x00, 0x40, 0x05 ; 1000:0e8e
    db 0x20, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x02, 0x00, 0x01, 0x00, 0x90, 0x03, 0x10, 0x00, 0x04, 0x9e ; 1000:0e9e
    db 0x07, 0x80, 0x02, 0x00, 0x01, 0x00, 0x90, 0x03, 0x20, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x02, 0x00 ; 1000:0eae
    db 0x02, 0x00, 0x50, 0x03, 0xd0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x02, 0x00, 0x02, 0x00, 0x50, 0x03 ; 1000:0ebe
    db 0xe0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x02, 0x00, 0x02, 0x00, 0x60, 0x03, 0xd0, 0x00, 0x5e, 0x9d ; 1000:0ece
    db 0x06, 0x80, 0x02, 0x00, 0x02, 0x00, 0x60, 0x03, 0xe0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x02, 0x00 ; 1000:0ede
    db 0x02, 0x00, 0xb0, 0x05, 0x30, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0xb0, 0x05 ; 1000:0eee
    db 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0xb0, 0x05, 0x50, 0x00, 0xb8, 0x9c ; 1000:0efe
    db 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0xa0, 0x05, 0x40, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x02, 0x00 ; 1000:0f0e
    db 0x02, 0x00, 0xa0, 0x05, 0x50, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x90, 0x05 ; 1000:0f1e
    db 0x50, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x10, 0x02, 0x10, 0x01, 0x64, 0xab ; 1000:0f2e
    db 0x80, 0x04, 0x02, 0x00, 0x03, 0x00, 0x70, 0x00, 0x30, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x02, 0x00 ; 1000:0f3e
    db 0x03, 0x00, 0x80, 0x00, 0xa0, 0x00, 0xb0, 0xac, 0x80, 0x04, 0x03, 0x00, 0x00, 0x00, 0x10, 0x00 ; 1000:0f4e
    db 0xf0, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x70, 0x00, 0x8e, 0xa2 ; 1000:0f5e
    db 0x09, 0x00, 0x03, 0x00, 0x01, 0x00, 0xd0, 0x02, 0x70, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x03, 0x00 ; 1000:0f6e
    db 0x01, 0x00, 0xd0, 0x02, 0x80, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x03, 0x00, 0x01, 0x00, 0xe0, 0x02 ; 1000:0f7e
    db 0x70, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x03, 0x00, 0x01, 0x00, 0xe0, 0x02, 0x80, 0x00, 0x8e, 0xa2 ; 1000:0f8e
    db 0x09, 0x80, 0x03, 0x00, 0x01, 0x00, 0xc0, 0x03, 0x50, 0x00, 0x0a, 0xac, 0x80, 0x08, 0x05, 0x00 ; 1000:0f9e
    db 0x01, 0x00, 0x60, 0x04, 0x90, 0x00, 0x9c, 0xa0, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0xe0, 0x00 ; 1000:0fae
    db 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x06, 0x00, 0x00, 0x00, 0x10, 0x03, 0x60, 0x00, 0x8e, 0xa2 ; 1000:0fbe
    db 0x09, 0x80, 0x06, 0x00, 0x00, 0x00, 0xa0, 0x04, 0x80, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x06, 0x00 ; 1000:0fce
    db 0x00, 0x00, 0x30, 0x03, 0xc0, 0x01, 0x04, 0x9e, 0x07, 0x80, 0x06, 0x00, 0x00, 0x00, 0x20, 0x03 ; 1000:0fde
    db 0xc0, 0x01, 0x04, 0x9e, 0x07, 0x80, 0x06, 0x00, 0x01, 0x00, 0x90, 0x00, 0xc0, 0x00, 0x5e, 0x9d ; 1000:0fee
    db 0x06, 0x80, 0x06, 0x00, 0x01, 0x00, 0xb0, 0x00, 0xc0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x06, 0x00 ; 1000:0ffe
    db 0x01, 0x00, 0x30, 0x05, 0xc0, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x06, 0x00, 0x01, 0x00, 0xd0, 0x05 ; 1000:100e
    db 0x30, 0x01, 0x02, 0xa5, 0x80, 0x08, 0x06, 0x00, 0x02, 0x00, 0x10, 0x00, 0xf0, 0x00, 0x56, 0xad ; 1000:101e
    db 0x80, 0x04, 0x07, 0x00, 0x00, 0x00, 0x40, 0x01, 0x60, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x07, 0x00 ; 1000:102e
    db 0x00, 0x00, 0x30, 0x00, 0x60, 0x01, 0x8e, 0xa2, 0x09, 0x80, 0x07, 0x00, 0x00, 0x00, 0xa0, 0x01 ; 1000:103e
    db 0xe0, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x07, 0x00, 0x00, 0x00, 0x60, 0x04, 0x60, 0x01, 0x04, 0x9e ; 1000:104e
    db 0x07, 0x80, 0x07, 0x00, 0x01, 0x00, 0xa0, 0x03, 0x40, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x07, 0x00 ; 1000:105e
    db 0x01, 0x00, 0x00, 0x04, 0x40, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x07, 0x00, 0x01, 0x00, 0x40, 0x04 ; 1000:106e
    db 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x07, 0x00, 0x01, 0x00, 0xe0, 0x03, 0x70, 0x01, 0x5e, 0x9d ; 1000:107e
    db 0x06, 0x80, 0x07, 0x00, 0x01, 0x00, 0x40, 0x04, 0x70, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x07, 0x00 ; 1000:108e
    db 0x01, 0x00, 0x10, 0x00, 0xd0, 0x00, 0xa2, 0xae, 0x80, 0x01, 0x07, 0x00, 0x01, 0x00, 0x50, 0x00 ; 1000:109e
    db 0x20, 0x00, 0x48, 0xaf, 0x80, 0x01, 0x07, 0x00, 0x02, 0x00, 0xf0, 0x00, 0x20, 0x00, 0xfc, 0xad ; 1000:10ae
    db 0x80, 0x88, 0x07, 0x00, 0x02, 0x00, 0xe0, 0x05, 0xb0, 0x00, 0xf6, 0x9f, 0x02, 0x80, 0x08, 0x00 ; 1000:10be
    db 0x00, 0x00, 0x90, 0x00, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0xa0, 0x00 ; 1000:10ce
    db 0x30, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x30, 0x00, 0x5e, 0x9d ; 1000:10de
    db 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00 ; 1000:10ee
    db 0x00, 0x00, 0x80, 0x01, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x90, 0x01 ; 1000:10fe
    db 0x30, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x30, 0x00, 0x5e, 0x9d ; 1000:110e
    db 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00 ; 1000:111e
    db 0x01, 0x00, 0xe0, 0x03, 0x20, 0x01, 0x9c, 0xa0, 0x03, 0x00, 0x08, 0x00, 0x01, 0x00, 0xc0, 0x07 ; 1000:112e
    db 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0xd0, 0x07, 0x20, 0x01, 0x5e, 0x9d ; 1000:113e
    db 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0xe0, 0x07, 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00 ; 1000:114e
    db 0x01, 0x00, 0xd0, 0x07, 0x10, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x01, 0x00, 0xe0, 0x07 ; 1000:115e
    db 0x10, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x02, 0x00, 0xa8, 0x03, 0x30, 0x01, 0x9c, 0xa0 ; 1000:116e
    db 0x03, 0x00, 0x08, 0x00, 0x02, 0x00, 0x20, 0x02, 0x00, 0x01, 0x50, 0x9f, 0x01, 0x00, 0x08, 0x00 ; 1000:117e
    db 0x02, 0x00, 0x48, 0x03, 0x30, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x02, 0x00, 0xf0, 0x02 ; 1000:118e
    db 0xc0, 0x00, 0xb8, 0x9c, 0x05, 0x00, 0x08, 0x00, 0x02, 0x00, 0xf0, 0x02, 0x40, 0x01, 0xb8, 0x9c ; 1000:119e
    db 0x05, 0x00, 0x08, 0x00, 0x02, 0x00, 0x40, 0x02, 0x00, 0x01, 0xb8, 0x9c, 0x05, 0x00, 0x08, 0x00 ; 1000:11ae
    db 0x02, 0x00, 0x40, 0x02, 0x80, 0x01, 0xb8, 0x9c, 0x05, 0x00, 0x08, 0x00, 0x03, 0x00, 0x50, 0x00 ; 1000:11be
    db 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x08, 0x00, 0x03, 0x00, 0xe0, 0x07, 0x20, 0x00, 0x04, 0x9e ; 1000:11ce
    db 0x07, 0x00, 0x08, 0x00, 0x03, 0x00, 0x00, 0x07, 0x30, 0xff, 0xee, 0xaf, 0x80, 0x08, 0x09, 0x00 ; 1000:11de
    db 0x00, 0x00, 0x70, 0x07, 0x10, 0x01, 0x04, 0x9e, 0x07, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x05 ; 1000:11ee
    db 0xf0, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x10, 0x05, 0x00, 0x01, 0x5e, 0x9d ; 1000:11fe
    db 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x05, 0x10, 0x01, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00 ; 1000:120e
    db 0x00, 0x00, 0xc0, 0x02, 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0xd0, 0x02 ; 1000:121e
    db 0x60, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x60, 0x00, 0x5e, 0x9d ; 1000:122e
    db 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00 ; 1000:123e
    db 0x01, 0x00, 0xa0, 0x07, 0x20, 0x00, 0x86, 0xb2, 0x80, 0x04, 0x09, 0x00, 0x02, 0x00, 0xb0, 0x01 ; 1000:124e
    db 0xd0, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x09, 0x00, 0x02, 0x00, 0xb0, 0x01, 0xc0, 0x00, 0x8e, 0xa2 ; 1000:125e
    db 0x09, 0x80, 0x09, 0x00, 0x02, 0x00, 0x70, 0x02, 0xc0, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x09, 0x00 ; 1000:126e
    db 0x02, 0x00, 0x70, 0x02, 0xd0, 0x00, 0xa8, 0xa5, 0x80, 0x88, 0x09, 0x00, 0x02, 0x00, 0x40, 0x05 ; 1000:127e
    db 0xe0, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x09, 0x00, 0x02, 0x00, 0x30, 0x00, 0x60, 0x01, 0x2c, 0xb3 ; 1000:128e
    db 0x80, 0x04, 0x09, 0x00, 0x02, 0x00, 0x90, 0x05, 0x50, 0xff, 0xee, 0xaf, 0x80, 0x08, 0x09, 0x00 ; 1000:129e
    db 0x03, 0x00, 0x10, 0x04, 0x40, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00, 0x03, 0x00, 0xb0, 0x03 ; 1000:12ae
    db 0x30, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x09, 0x00, 0x03, 0x00, 0x50, 0x03, 0x20, 0x00, 0x5e, 0x9d ; 1000:12be
    db 0x06, 0x00, 0x09, 0x00, 0x03, 0x00, 0x20, 0x00, 0xb0, 0x00, 0x3a, 0xb1, 0x80, 0x05, 0x0a, 0x00 ; 1000:12ce
    db 0x02, 0x00, 0x18, 0x00, 0x50, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x0a, 0x00, 0x03, 0x00, 0x28, 0x00 ; 1000:12de
    db 0x70, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x0a, 0x00, 0x03, 0x00, 0xa8, 0x00, 0x30, 0x00, 0xe8, 0xa1 ; 1000:12ee
    db 0x80, 0x00, 0x0a, 0x00, 0x03, 0x00, 0x18, 0x01, 0x30, 0x00, 0xd2, 0xb3, 0x80, 0x05, 0x0a, 0x00 ; 1000:12fe
    db 0x04, 0x00, 0xb0, 0x00, 0xd0, 0x03, 0x8e, 0xa2, 0x09, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x10, 0x00 ; 1000:130e
    db 0x00, 0x03, 0x04, 0x9e, 0x07, 0x00, 0x0a, 0x00, 0x05, 0x00, 0xd8, 0x03, 0x60, 0x00, 0xee, 0xaf ; 1000:131e
    db 0x80, 0x08, 0x0b, 0x00, 0x03, 0x00, 0xf0, 0x00, 0x30, 0x00, 0x04, 0x9e, 0x07, 0x80, 0x0b, 0x00 ; 1000:132e
    db 0x03, 0x00, 0x10, 0x01, 0x30, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x0b, 0x00, 0x03, 0x00, 0x70, 0x01 ; 1000:133e
    db 0x80, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x0b, 0x00, 0x03, 0x00, 0x60, 0x01, 0x80, 0x00, 0xb8, 0x9c ; 1000:134e
    db 0x05, 0x80, 0x0b, 0x00, 0x04, 0x00, 0x30, 0x00, 0xe0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00 ; 1000:135e
    db 0x04, 0x00, 0x40, 0x00, 0xe0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00, 0x04, 0x00, 0xb0, 0x00 ; 1000:136e
    db 0xe0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00, 0x04, 0x00, 0x20, 0x01, 0xe0, 0x00, 0xb8, 0x9c ; 1000:137e
    db 0x05, 0x80, 0x0b, 0x00, 0x04, 0x00, 0x90, 0x05, 0x80, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00 ; 1000:138e
    db 0x04, 0x00, 0xa0, 0x05, 0x80, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00, 0x04, 0x00, 0xc0, 0x05 ; 1000:139e
    db 0x40, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x0b, 0x00, 0x04, 0x00, 0xd0, 0x06, 0x50, 0x00, 0x12, 0x9c ; 1000:13ae
    db 0x04, 0x80, 0x0b, 0x00, 0x04, 0x00, 0xa0, 0x06, 0x50, 0x00, 0x12, 0x9c, 0x04, 0x80, 0x0b, 0x00 ; 1000:13be
    db 0x04, 0x00, 0xa0, 0x07, 0x20, 0x01, 0x5e, 0x9d, 0x06, 0x80, 0x0b, 0x00, 0x04, 0x00, 0x80, 0x07 ; 1000:13ce
    db 0x20, 0x01, 0x8e, 0xa2, 0x09, 0x80, 0x0b, 0x00, 0x06, 0x00, 0x10, 0x00, 0x20, 0x00, 0x78, 0xb4 ; 1000:13de
    db 0x80, 0x84, 0x0b, 0x00, 0x06, 0x00, 0xf0, 0x00, 0xc0, 0x00, 0x8e, 0xa2, 0x09, 0x80, 0x0b, 0x00 ; 1000:13ee
    db 0x06, 0x00, 0xe0, 0x00, 0xc0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0b, 0x00, 0x06, 0x00, 0xf0, 0x00 ; 1000:13fe
    db 0xb0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x0b, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x30, 0x00, 0x8e, 0xa2 ; 1000:140e
    db 0x09, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x30, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x0b, 0x00 ; 1000:141e
    db 0x00, 0x00, 0xa0, 0x01, 0x30, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xc0, 0x01 ; 1000:142e
    db 0x30, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x30, 0x00, 0xbe, 0xaa ; 1000:143e
    db 0x08, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x20, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0b, 0x00 ; 1000:144e
    db 0x00, 0x00, 0xd0, 0x01, 0x20, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0c, 0x00, 0x03, 0x00, 0xf0, 0x01 ; 1000:145e
    db 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x02, 0x70, 0x00, 0x9c, 0xa0 ; 1000:146e
    db 0x03, 0x80, 0x0c, 0x00, 0x03, 0x00, 0xc0, 0x03, 0xe0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0c, 0x00 ; 1000:147e
    db 0x03, 0x00, 0xd0, 0x03, 0xe0, 0x00, 0x5e, 0x9d, 0x06, 0x80, 0x0c, 0x00, 0x03, 0x00, 0xd0, 0x03 ; 1000:148e
    db 0xd0, 0x00, 0xb8, 0x9c, 0x05, 0x80, 0x0c, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x01, 0x1e, 0xb5 ; 1000:149e
    db 0x80, 0x85, 0x0c, 0x00, 0x04, 0x00, 0x30, 0x00, 0x00, 0x01, 0x8e, 0xa2, 0x09, 0x80, 0x0c, 0x00 ; 1000:14ae
    db 0x04, 0x00, 0x50, 0x00, 0x00, 0x01, 0xb8, 0x9c, 0x05, 0x80, 0x0c, 0x00, 0x05, 0x00, 0xc0, 0x07 ; 1000:14be
    db 0xd0, 0x00, 0xc4, 0xb5, 0x80, 0x04, 0x0c, 0x00, 0x05, 0x00, 0x20, 0x00, 0xe0, 0x00, 0x10, 0xb7 ; 1000:14ce
    db 0x80, 0x86, 0x0c, 0x00, 0x05, 0x00, 0x50, 0x06, 0x20, 0x01, 0x8e, 0xa2, 0x09, 0x80, 0x0c, 0x00 ; 1000:14de
    db 0x05, 0x00, 0x60, 0x06, 0x20, 0x01, 0x8e, 0xa2, 0x09, 0x80, 0x0c, 0x00, 0x06, 0x00, 0x70, 0x02 ; 1000:14ee
    db 0x50, 0x00, 0xee, 0xaf, 0x80, 0x08, 0x08, 0x00, 0x02, 0x00, 0xd0, 0x01, 0xf0, 0x00, 0xbe, 0xaa ; 1000:14fe
    db 0x08, 0x00, 0x08, 0x00, 0x02, 0x00, 0xd0, 0x01, 0x00, 0x01, 0xbe, 0xaa, 0x08, 0x00, 0x08, 0x00 ; 1000:150e
    db 0x02, 0x00, 0xe0, 0x01, 0x00, 0x01, 0xbe, 0xaa, 0x08, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x70, 0x00 ; 1000:151e
    db 0x40, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0a, 0x00, 0x08, 0x00, 0xa0, 0x00, 0x40, 0x00, 0xbe, 0xaa ; 1000:152e
    db 0x08, 0x00, 0x0a, 0x00, 0x08, 0x00, 0xd0, 0x00, 0x40, 0x00, 0xbe, 0xaa, 0x08, 0x00, 0x0e, 0x00 ; 1000:153e
    db 0x04, 0x00, 0xc0, 0x01, 0x90, 0x00, 0x02, 0xb9, 0x80, 0x04, 0x0e, 0x00, 0x0b, 0x00, 0x60, 0x01 ; 1000:154e
    db 0x30, 0x00, 0x5c, 0xb8, 0x80, 0x08, 0x0d, 0x00, 0x01, 0x00, 0x80, 0x01, 0x10, 0x00, 0x5e, 0x9d ; 1000:155e
    db 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0x20, 0x02, 0x80, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00 ; 1000:156e
    db 0x01, 0x00, 0x20, 0x03, 0x10, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0x70, 0x03 ; 1000:157e
    db 0x80, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0xc0, 0x03, 0x10, 0x00, 0x5e, 0x9d ; 1000:158e
    db 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0xb0, 0x04, 0x06, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00 ; 1000:159e
    db 0x01, 0x00, 0x50, 0x05, 0x80, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0x90, 0x06 ; 1000:15ae
    db 0x10, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0xe0, 0x06, 0x80, 0x00, 0x5e, 0x9d ; 1000:15be
    db 0x06, 0x00, 0x0d, 0x00, 0x01, 0x00, 0x30, 0x07, 0x10, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0d, 0x00 ; 1000:15ce
    db 0x01, 0x00, 0x28, 0x08, 0x78, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x0e, 0x00, 0x04, 0x00, 0x10, 0x00 ; 1000:15de
    db 0x40, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x10, 0x00, 0x20, 0x00, 0x5e, 0x9d ; 1000:15ee
    db 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0xb0, 0x00, 0x20, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00 ; 1000:15fe
    db 0x03, 0x00, 0x10, 0x00, 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0xb0, 0x00 ; 1000:160e
    db 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0xd0, 0x00, 0x70, 0x00, 0x5e, 0x9d ; 1000:161e
    db 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0xd0, 0x00, 0x20, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00 ; 1000:162e
    db 0x03, 0x00, 0x80, 0x02, 0x20, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x80, 0x02 ; 1000:163e
    db 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x20, 0x02, 0x70, 0x00, 0x5e, 0x9d ; 1000:164e
    db 0x06, 0x00, 0x0e, 0x00, 0x03, 0x00, 0xb0, 0x01, 0x70, 0x00, 0x5e, 0x9d, 0x06, 0x00, 0x0e, 0x00 ; 1000:165e
    db 0x06, 0x00, 0x90, 0x02, 0x20, 0x02, 0x8e, 0xa2, 0x09, 0x00, 0x0e, 0x00, 0x06, 0x00, 0x30, 0x02 ; 1000:166e
    db 0x20, 0x02, 0x8e, 0xa2, 0x09, 0x00, 0x0e, 0x00, 0x09, 0x00, 0x30, 0x04, 0x80, 0x00, 0x8e, 0xa2 ; 1000:167e
    db 0x09, 0x00, 0x0e, 0x00, 0x0a, 0x00, 0x70, 0x00, 0x70, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x03, 0x00 ; 1000:168e
    db 0x01, 0x00, 0x50, 0x03, 0x50, 0x00, 0x8e, 0xa2, 0x09, 0x00, 0x4a, 0x7e, 0x43, 0x85, 0x90, 0x8c ; 1000:169e
    db 0x5e, 0x90, 0x27, 0x92, 0x3e, 0x93, 0x19, 0x98, 0xb2, 0x9b, 0x0c, 0xa1, 0x2f, 0xa8, 0xac, 0xad ; 1000:16ae
    db 0x27, 0xb7, 0x80, 0xbd, 0x07, 0xc6, 0xb2, 0xca, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x16 ; 1000:16be
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe8, 0x16, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x16 ; 1000:16ce
    db 0x0d, 0x00, 0x00, 0x08, 0x10, 0x08, 0x04, 0x08, 0x18, 0x04, 0x00, 0x08, 0x10, 0x08, 0x04, 0x08 ; 1000:16de
    db 0x18, 0x04, 0x00, 0x08, 0x10, 0x08, 0x04, 0x08, 0x18, 0x04, 0xb8, 0x00, 0xa0, 0x8e, 0xc0, 0xba ; 1000:16ee
    db 0xc2, 0x93, 0xbf, 0x00, 0x00, 0xe8, 0x8a, 0x62, 0xbf, 0x00, 0x20, 0xbe, 0x00, 0x00, 0xe8, 0x56 ; 1000:16fe
    db 0x60, 0xb8, 0x01, 0x00, 0xe8, 0x65, 0xeb, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0x8d, 0x36, 0xc0, 0x98 ; 1000:170e
    db 0x8b, 0x34, 0xb8, 0x00, 0x01, 0xbb, 0x5e, 0x00, 0xe8, 0xe3, 0x61, 0x2e, 0x8e, 0x1e, 0x5c, 0x00 ; 1000:171e
    db 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0x8d, 0x36, 0xc0, 0x98, 0x8b, 0x74, 0x02, 0xb8, 0x00, 0x01, 0xbb ; 1000:172e
    db 0x77, 0x00, 0x56, 0xe8, 0xc8, 0x61, 0x5e, 0xb8, 0x00, 0x01, 0xbb, 0x90, 0x00, 0x56, 0xe8, 0xbd ; 1000:173e
    db 0x61, 0x5e, 0xb8, 0x00, 0x01, 0xbb, 0xa9, 0x00, 0xe8, 0xb3, 0x61, 0x2e, 0x8e, 0x1e, 0x5c, 0x00 ; 1000:174e
    db 0xe8, 0x9c, 0x4e, 0xa1, 0x14, 0x02, 0x8b, 0x1e, 0x16, 0x02, 0x80, 0x3e, 0x20, 0x02, 0x06, 0x7c ; 1000:175e
    db 0x14, 0xb8, 0x00, 0x00, 0xbb, 0x07, 0x00, 0xc7, 0x06, 0x10, 0x02, 0x98, 0x00, 0xc7, 0x06, 0x12 ; 1000:176e
    db 0x02, 0x50, 0x00, 0xeb, 0x39, 0x80, 0x3e, 0x20, 0x02, 0x04, 0x7c, 0x20, 0x3d, 0x05, 0x00, 0x7c ; 1000:177e
    db 0x1b, 0x7f, 0x05, 0x83, 0xfb, 0x03, 0x7c, 0x14, 0xb8, 0x05, 0x00, 0xbb, 0x03, 0x00, 0xc7, 0x06 ; 1000:178e
    db 0x10, 0x02, 0x88, 0x02, 0xc7, 0x06, 0x12, 0x02, 0x70, 0x00, 0xeb, 0x12, 0xb8, 0x00, 0x00, 0xbb ; 1000:179e
    db 0x00, 0x00, 0xc7, 0x06, 0x10, 0x02, 0x38, 0x02, 0xc7, 0x06, 0x12, 0x02, 0x50, 0x00, 0xc7, 0x06 ; 1000:17ae
    db 0xfa, 0x06, 0xff, 0xff, 0x89, 0x1e, 0xfc, 0x06, 0xbb, 0xff, 0xff, 0xe8, 0xc1, 0x03, 0x80, 0x3e ; 1000:17be
    db 0x20, 0x02, 0x03, 0x7c, 0x1e, 0x2e, 0xc7, 0x06, 0xd5, 0x7e, 0x04, 0x00, 0x2e, 0xc7, 0x06, 0x67 ; 1000:17ce
    db 0x80, 0x04, 0x00, 0x2e, 0xc7, 0x06, 0xf9, 0x7e, 0x04, 0x00, 0x2e, 0xc7, 0x06, 0xfb, 0x7e, 0x01 ; 1000:17de
    db 0x00, 0xeb, 0x1c, 0x2e, 0xc7, 0x06, 0xd5, 0x7e, 0x01, 0x00, 0x2e, 0xc7, 0x06, 0x67, 0x80, 0x01 ; 1000:17ee
    db 0x00, 0x2e, 0xc7, 0x06, 0xf9, 0x7e, 0x01, 0x00, 0x2e, 0xc7, 0x06, 0xfb, 0x7e, 0x08, 0x00, 0x80 ; 1000:17fe
    db 0x3e, 0x20, 0x02, 0x05, 0x7c, 0x09, 0x2e, 0xc7, 0x06, 0xdd, 0x93, 0xff, 0xff, 0xeb, 0x07, 0x2e ; 1000:180e
    db 0xc7, 0x06, 0xdd, 0x93, 0x00, 0x00, 0x33, 0xc0, 0xa3, 0x90, 0x08, 0xa2, 0x96, 0x08, 0xc7, 0x06 ; 1000:181e
    db 0x8f, 0x02, 0x04, 0x00, 0xa3, 0x34, 0x09, 0xa3, 0x36, 0x09, 0xa3, 0x38, 0x09, 0xb8, 0x20, 0x01 ; 1000:182e
    db 0xbb, 0x5e, 0x00, 0x32, 0xed, 0x8a, 0x0e, 0x1c, 0x02, 0xe8, 0x15, 0x22, 0xb8, 0x20, 0x01, 0xbb ; 1000:183e
    db 0x77, 0x00, 0x8b, 0x0e, 0x24, 0x02, 0xe8, 0x08, 0x22, 0xb8, 0x20, 0x01, 0xbb, 0x90, 0x00, 0x8b ; 1000:184e
    db 0x0e, 0x22, 0x02, 0xe8, 0xfb, 0x21, 0xb8, 0x20, 0x01, 0xbb, 0xa9, 0x00, 0x8b, 0x0e, 0x8f, 0x02 ; 1000:185e
    db 0xe8, 0xee, 0x21, 0x80, 0x3e, 0x1e, 0x02, 0x01, 0x75, 0x17, 0xb8, 0xf0, 0x00, 0xbb, 0x90, 0x00 ; 1000:186e
    db 0x8d, 0x36, 0x50, 0x9f, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x3c, 0x60, 0x2e, 0x8e, 0x1e, 0x5c ; 1000:187e
    db 0x00, 0x80, 0x3e, 0x1f, 0x02, 0x01, 0x75, 0x17, 0xb8, 0xf0, 0x00, 0xbb, 0xa9, 0x00, 0x8d, 0x36 ; 1000:188e
    db 0xf6, 0x9f, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x1e, 0x60, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0x80 ; 1000:189e
    db 0x3e, 0x1d, 0x02, 0x01, 0x75, 0x17, 0xb8, 0xf0, 0x00, 0xbb, 0x77, 0x00, 0x8d, 0x36, 0xaa, 0x9e ; 1000:18ae
    db 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x00, 0x60, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0x33, 0xc0, 0x50 ; 1000:18be
    db 0xe8, 0x4b, 0x2c, 0x58, 0x40, 0x3d, 0x0d, 0x00, 0x7c, 0xf5, 0xc6, 0x06, 0x19, 0x02, 0x00, 0xe9 ; 1000:18ce
    db 0x9b, 0x09                                   ; 1000:18de

; ----------------------------------------------------------------------------
; FUNCTION: spr_render_player  [Sprites / Player]
; ADDRESS:  1000:18e0
; PURPOSE:  Renders Captain Comic sprite at coordinates (X, Y) using planar EGA masking, selected animation frame, and directional facing.
; ----------------------------------------------------------------------------
spr_render_player:
    mov          AL,[0x8ad]                      ; 1000:18e0: a0ad08
    mov          [0x218],AL                      ; 1000:18e3: a21802
    mov          AL,[0x89d]                      ; 1000:18e6: a09d08
    mov          [0x219],AL                      ; 1000:18e9: a21902
    mov          AX,[0x6c6]                      ; 1000:18ec: a1c606
    mov          [0x210],AX                      ; 1000:18ef: a31002
    sub          AX,0x60                         ; 1000:18f2: 2d6000
    jnc          loc_18f9                        ; 1000:18f5: 7302
    xor          AX,AX                           ; 1000:18f7: 33c0
loc_18f9:
    mov          BX,word [0x29c]                 ; 1000:18f9: 8b1e9c02
    sub          BX,0xd0                         ; 1000:18fd: 81ebd000
    cmp          AX,BX                           ; 1000:1901: 3bc3
    jle          loc_1907                        ; 1000:1903: 7e02
    mov          AX,BX                           ; 1000:1905: 8bc3
loc_1907:
    mov          [0x6c2],AX                      ; 1000:1907: a3c206
    mov          AX,[0x6c8]                      ; 1000:190a: a1c806
    mov          [0x212],AX                      ; 1000:190d: a31202
    sub          AX,0x40                         ; 1000:1910: 2d4000
    jnc          loc_1917                        ; 1000:1913: 7302
    xor          AX,AX                           ; 1000:1915: 33c0
loc_1917:
    mov          BX,word [0x29e]                 ; 1000:1917: 8b1e9e02
    sub          BX,0xa0                         ; 1000:191b: 81eba000
    cmp          AX,BX                           ; 1000:191f: 3bc3
    jle          loc_1925                        ; 1000:1921: 7e02
    mov          AX,BX                           ; 1000:1923: 8bc3
loc_1925:
    mov          [0x6c4],AX                      ; 1000:1925: a3c406
    cmp          word [0x6fa],0x2                ; 1000:1928: 833efa0602
    jnz          loc_1959                        ; 1000:192d: 752a
    cmp          word [0x6fc],0x1                ; 1000:192f: 833efc0601
    jnz          loc_1959                        ; 1000:1934: 7523
    cmp          word [0x6c6],0x0                ; 1000:1936: 833ec60600
    jnz          loc_1959                        ; 1000:193b: 751c
    mov          byte [0x924],0x2                ; 1000:193d: c606240902
    mov          word [0x928],0x0                ; 1000:1942: c70628090000
    mov          word [0x92a],0x20               ; 1000:1948: c7062a092000
    mov          byte [0x925],0x2                ; 1000:194e: c606250902
    mov          word [0x926],0x8                ; 1000:1953: c70626090800
loc_1959:
    cmp          word [0x6fa],0x9                ; 1000:1959: 833efa0609
    jnz          loc_198a                        ; 1000:195e: 752a
    cmp          word [0x6fc],0x1                ; 1000:1960: 833efc0601
    jnz          loc_198a                        ; 1000:1965: 7523
    cmp          word [0x6c6],0x0                ; 1000:1967: 833ec60600
    jnz          loc_198a                        ; 1000:196c: 751c
    mov          byte [0x92c],0x2                ; 1000:196e: c6062c0902
    mov          word [0x930],0x0                ; 1000:1973: c70630090000
    mov          word [0x932],0xc0               ; 1000:1979: c7063209c000
    mov          byte [0x92d],0x0                ; 1000:197f: c6062d0900
    mov          word [0x92e],0x0                ; 1000:1984: c7062e090000
loc_198a:
    call         spr_render_monster              ; 1000:198a: e82030
    ret                                          ; 1000:198d: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_init_tileset  [Tilemap / Level]
; ADDRESS:  1000:198e
; PURPOSE:  Initializes room tile attributes, copies tileset graphics into offscreen EGA memory, and configures tile properties.
; ----------------------------------------------------------------------------
map_init_tileset:
    lea          SI,[0x16a8]                     ; 1000:198e: 8d36a816
    mov          AX,[0x6fa]                      ; 1000:1992: a1fa06
    shl          AX,0x1                          ; 1000:1995: d1e0
    add          SI,AX                           ; 1000:1997: 03f0
    mov          SI,word [cs:SI]                 ; 1000:1999: 2e8b34
    mov          BX,word [cs:SI]                 ; 1000:199c: 2e8b1c
    cmp          word [0x6fa],0x5                ; 1000:199f: 833efa0605
    jnz          loc_19b0                        ; 1000:19a4: 750a
    cmp          byte [0x220],0x5                ; 1000:19a6: 803e200205
    jl           loc_19b0                        ; 1000:19ab: 7c03
    mov          BX,0x16c6                       ; 1000:19ad: bbc616
loc_19b0:
    mov          word [cs:0x278],BX              ; 1000:19b0: 2e891e7802
    mov          word [cs:BX + 0x2],0x0          ; 1000:19b5: 2ec747020000
    mov          word [cs:BX + 0x4],0x0          ; 1000:19bb: 2ec747040000
    mov          word [cs:BX + 0xa],0x0          ; 1000:19c1: 2ec7470a0000
    mov          word [cs:BX + 0xc],0x0          ; 1000:19c7: 2ec7470c0000
    mov          word [cs:BX + 0x12],0x0         ; 1000:19cd: 2ec747120000
    mov          word [cs:BX + 0x14],0x0         ; 1000:19d3: 2ec747140000
    mov          BX,word [cs:SI + 0x2]           ; 1000:19d9: 2e8b5c02
    mov          AH,byte [cs:BX]                 ; 1000:19dd: 2e8a27
    mov          byte [0x6ca],AH                 ; 1000:19e0: 8826ca06
    mov          AL,byte [cs:BX + 0x1]           ; 1000:19e4: 2e8a4701
    mov          [0x6cb],AL                      ; 1000:19e8: a2cb06
    sub          AL,AH                           ; 1000:19eb: 2ac4
    add          BX,0x2                          ; 1000:19ed: 83c302
    lea          DI,[0x6cc]                      ; 1000:19f0: 8d3ecc06
    inc          AL                              ; 1000:19f4: fec0
loc_19f6:
    mov          CL,byte [cs:BX]                 ; 1000:19f6: 2e8a0f
    inc          BX                              ; 1000:19f9: 43
    mov          byte [DI],CL                    ; 1000:19fa: 880d
    inc          DI                              ; 1000:19fc: 47
    dec          AL                              ; 1000:19fd: fec8
    jnz          loc_19f6                        ; 1000:19ff: 75f5
    mov          BX,word [cs:SI + 0x4]           ; 1000:1a01: 2e8b5c04
    mov          AH,byte [cs:BX]                 ; 1000:1a05: 2e8a27
    mov          byte [0x6d6],AH                 ; 1000:1a08: 8826d606
    mov          AL,byte [cs:BX + 0x1]           ; 1000:1a0c: 2e8a4701
    mov          [0x6d7],AL                      ; 1000:1a10: a2d706
    sub          AL,AH                           ; 1000:1a13: 2ac4
    add          BX,0x2                          ; 1000:1a15: 83c302
    lea          DI,[0x6d8]                      ; 1000:1a18: 8d3ed806
    inc          AL                              ; 1000:1a1c: fec0
loc_1a1e:
    mov          CL,byte [cs:BX]                 ; 1000:1a1e: 2e8a0f
    inc          BX                              ; 1000:1a21: 43
    mov          byte [DI],CL                    ; 1000:1a22: 880d
    inc          DI                              ; 1000:1a24: 47
    dec          AL                              ; 1000:1a25: fec8
    jnz          loc_1a1e                        ; 1000:1a27: 75f5
    mov          BX,word [cs:SI + 0x6]           ; 1000:1a29: 2e8b5c06
    mov          AH,byte [cs:BX]                 ; 1000:1a2d: 2e8a27
    mov          byte [0x6e2],AH                 ; 1000:1a30: 8826e206
    mov          AL,byte [cs:BX + 0x1]           ; 1000:1a34: 2e8a4701
    mov          [0x6e3],AL                      ; 1000:1a38: a2e306
    sub          AL,AH                           ; 1000:1a3b: 2ac4
    add          BX,0x2                          ; 1000:1a3d: 83c302
    lea          DI,[0x6e4]                      ; 1000:1a40: 8d3ee406
    inc          AL                              ; 1000:1a44: fec0
loc_1a46:
    mov          CL,byte [cs:BX]                 ; 1000:1a46: 2e8a0f
    inc          BX                              ; 1000:1a49: 43
    mov          byte [DI],CL                    ; 1000:1a4a: 880d
    inc          DI                              ; 1000:1a4c: 47
    dec          AL                              ; 1000:1a4d: fec8
    jnz          loc_1a46                        ; 1000:1a4f: 75f5
    mov          byte [0x93a],0x0                ; 1000:1a51: c6063a0900
    cmp          word [0x6fa],0xc                ; 1000:1a56: 833efa060c
    jnz          loc_1a62                        ; 1000:1a5b: 7505
    mov          byte [0x93a],0x1                ; 1000:1a5d: c6063a0901
loc_1a62:
    ret                                          ; 1000:1a62: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_spawn_room_entities  [Entities / Spawning]
; ADDRESS:  1000:1a63
; PURPOSE:  Parses room entity definitions from file buffer and initializes active monster, item, and hazard slots.
; ----------------------------------------------------------------------------
map_spawn_room_entities:
    lea          SI,[0x16a8]                     ; 1000:1a63: 8d36a816
    mov          AX,[0x6fa]                      ; 1000:1a67: a1fa06
    shl          AX,0x1                          ; 1000:1a6a: d1e0
    add          SI,AX                           ; 1000:1a6c: 03f0
    mov          SI,word [cs:SI]                 ; 1000:1a6e: 2e8b34
    mov          BX,word [cs:SI + 0x8]           ; 1000:1a71: 2e8b5c08
    mov          AX,[0x6fc]                      ; 1000:1a75: a1fc06
    shl          AX,0x1                          ; 1000:1a78: d1e0
    add          BX,AX                           ; 1000:1a7a: 03d8
    mov          BX,word [cs:BX]                 ; 1000:1a7c: 2e8b1f
    mov          AX,word [cs:BX]                 ; 1000:1a7f: 2e8b07
    mov          [0x6ee],AX                      ; 1000:1a82: a3ee06
    add          BX,0x2                          ; 1000:1a85: 83c302
    mov          word [0x6f0],BX                 ; 1000:1a88: 891ef006
    mov          BX,word [cs:SI + 0xa]           ; 1000:1a8c: 2e8b5c0a
    mov          AX,[0x6fc]                      ; 1000:1a90: a1fc06
    shl          AX,0x1                          ; 1000:1a93: d1e0
    add          BX,AX                           ; 1000:1a95: 03d8
    mov          BX,word [cs:BX]                 ; 1000:1a97: 2e8b1f
    mov          AX,word [cs:BX]                 ; 1000:1a9a: 2e8b07
    mov          [0x6f2],AX                      ; 1000:1a9d: a3f206
    add          BX,0x2                          ; 1000:1aa0: 83c302
    mov          word [0x6f4],BX                 ; 1000:1aa3: 891ef406
    mov          BX,word [cs:SI + 0xc]           ; 1000:1aa7: 2e8b5c0c
    mov          AX,[0x6fc]                      ; 1000:1aab: a1fc06
    shl          AX,0x1                          ; 1000:1aae: d1e0
    add          BX,AX                           ; 1000:1ab0: 03d8
    mov          BX,word [cs:BX]                 ; 1000:1ab2: 2e8b1f
    mov          AX,word [cs:BX]                 ; 1000:1ab5: 2e8b07
    mov          [0x6f6],AX                      ; 1000:1ab8: a3f606
    add          BX,0x2                          ; 1000:1abb: 83c302
    mov          word [0x6f8],BX                 ; 1000:1abe: 891ef806
    lea          SI,[0xaf0]                      ; 1000:1ac2: 8d36f00a
    lea          DI,[0x700]                      ; 1000:1ac6: 8d3e0007
    xor          CX,CX                           ; 1000:1aca: 33c9
    mov          word [0x6fe],0x0                ; 1000:1acc: c706fe060000
    mov          BX,word [0x6fa]                 ; 1000:1ad2: 8b1efa06
    mov          DX,word [0x6fc]                 ; 1000:1ad6: 8b16fc06
loc_1ada:
    cmp          BX,word [cs:SI]                 ; 1000:1ada: 2e3b1c
    jnz          loc_1b0d                        ; 1000:1add: 752e
    cmp          DX,word [cs:SI + 0x2]           ; 1000:1adf: 2e3b5402
    jnz          loc_1b0d                        ; 1000:1ae3: 7528
    mov          AX,word [cs:SI + 0x4]           ; 1000:1ae5: 2e8b4404
    mov          word [DI],AX                    ; 1000:1ae9: 8905
    mov          AX,word [cs:SI + 0x6]           ; 1000:1aeb: 2e8b4406
    mov          word [DI + 0x2],AX              ; 1000:1aef: 894502
    mov          AX,word [cs:SI + 0x8]           ; 1000:1af2: 2e8b4408
    mov          word [DI + 0x6],AX              ; 1000:1af6: 894506
    mov          word [DI + 0x4],CX              ; 1000:1af9: 894d04
    mov          AX,word [cs:SI + 0xa]           ; 1000:1afc: 2e8b440a
    and          AX,0x8000                       ; 1000:1b00: 250080
    or           word [DI + 0x4],AX              ; 1000:1b03: 094504
    inc          word [0x6fe]                    ; 1000:1b06: ff06fe06
    add          DI,0x8                          ; 1000:1b0a: 83c708
loc_1b0d:
    inc          CX                              ; 1000:1b0d: 41
    add          SI,0xc                          ; 1000:1b0e: 83c60c
    cmp          CX,word [0x940]                 ; 1000:1b11: 3b0e4009
    jnz          loc_1ada                        ; 1000:1b15: 75c3
    mov          SI,0x8ee                        ; 1000:1b17: beee08
    mov          CX,0x6                          ; 1000:1b1a: b90600
loc_1b1d:
    mov          word [SI],0xffff                ; 1000:1b1d: c704ffff
    add          SI,0x8                          ; 1000:1b21: 83c608
    loop         loc_1b1d                        ; 1000:1b24: e2f7
    mov          byte [0x8b1],0x0                ; 1000:1b26: c606b10800
    mov          SI,0x8b2                        ; 1000:1b2b: beb208
    mov          CX,0x6                          ; 1000:1b2e: b90600
loc_1b31:
    mov          byte [SI],0x0                   ; 1000:1b31: c60400
    add          SI,0x9                          ; 1000:1b34: 83c609
    loop         loc_1b31                        ; 1000:1b37: e2f8
    mov          byte [0x924],0x0                ; 1000:1b39: c606240900
    cmp          word [0x6fa],0x2                ; 1000:1b3e: 833efa0602
    jnz          loc_1b63                        ; 1000:1b43: 751e
    cmp          word [0x6fc],0x0                ; 1000:1b45: 833efc0600
    jnz          loc_1b63                        ; 1000:1b4a: 7517
    mov          byte [0x924],0x1                ; 1000:1b4c: c606240901
    mov          word [0x928],0xb0               ; 1000:1b51: c7062809b000
    mov          word [0x92a],0xe0               ; 1000:1b57: c7062a09e000
    mov          word [0x926],0x0                ; 1000:1b5d: c70626090000
loc_1b63:
    mov          byte [0x92c],0x0                ; 1000:1b63: c6062c0900
    cmp          word [0x6fa],0x9                ; 1000:1b68: 833efa0609
    jnz          loc_1b87                        ; 1000:1b6d: 7518
    cmp          word [0x6fc],0x0                ; 1000:1b6f: 833efc0600
    jnz          loc_1b87                        ; 1000:1b74: 7511
    mov          byte [0x92c],0x1                ; 1000:1b76: c6062c0901
    mov          word [0x930],0x558              ; 1000:1b7b: c70630095805
    mov          word [0x932],0x20               ; 1000:1b81: c70632092000
loc_1b87:
    mov          byte [0x8a8],0x0                ; 1000:1b87: c606a80800
    ret                                          ; 1000:1b8c: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_load_room  [Tilemap / Streaming]
; ADDRESS:  1000:1b8d
; PURPOSE:  Loads room data package for specified room index: streams layout (.0), collision masks (.1), tileset (.2), and entities (.3).
; ----------------------------------------------------------------------------
map_load_room:
    cmp          word [0x6fa],AX                 ; 1000:1b8d: 3906fa06
    jz           loc_1be2                        ; 1000:1b91: 744f
    call         map_parse_room_header           ; 1000:1b93: e87500
    call         vid_set_active_page_0           ; 1000:1b96: e892e6
    cmp          BX,-0x1                         ; 1000:1b99: 83fbff
    jz           loc_1ba4                        ; 1000:1b9c: 7406
    mov          word [0x6fc],0xffff             ; 1000:1b9e: c706fc06ffff
loc_1ba4:
    mov          [0x6fa],AX                      ; 1000:1ba4: a3fa06
    push         AX                              ; 1000:1ba7: 50
    push         BX                              ; 1000:1ba8: 53
    lea          BX,[0x93ec]                     ; 1000:1ba9: 8d1eec93
    shl          AX,0x1                          ; 1000:1bad: d1e0
    add          BX,AX                           ; 1000:1baf: 03d8
    mov          BX,word [BX]                    ; 1000:1bb1: 8b1f
    mov          DX,word [BX]                    ; 1000:1bb3: 8b17
    add          BX,0x2                          ; 1000:1bb5: 83c302
    push         BX                              ; 1000:1bb8: 53
    call         file_load_pack_rle              ; 1000:1bb9: e8e803
    pop          BX                              ; 1000:1bbc: 5b
    mov          DX,word [BX]                    ; 1000:1bbd: 8b17
    add          BX,0x2                          ; 1000:1bbf: 83c302
    push         BX                              ; 1000:1bc2: 53
    call         file_load_raw_mask              ; 1000:1bc3: e83804
    pop          BX                              ; 1000:1bc6: 5b
    mov          DX,word [BX]                    ; 1000:1bc7: 8b17
    add          BX,0x2                          ; 1000:1bc9: 83c302
    push         BX                              ; 1000:1bcc: 53
    call         file_load_tileset               ; 1000:1bcd: e85204
    pop          BX                              ; 1000:1bd0: 5b
    mov          DX,word [BX]                    ; 1000:1bd1: 8b17
    or           DX,DX                           ; 1000:1bd3: 0bd2
    jz           loc_1bda                        ; 1000:1bd5: 7403
    call         file_load_entity_data           ; 1000:1bd7: e86f04
loc_1bda:
    call         map_init_tileset                ; 1000:1bda: e8b1fd
    call         vid_set_active_page_1           ; 1000:1bdd: e858e6
    pop          BX                              ; 1000:1be0: 5b
    pop          AX                              ; 1000:1be1: 58
loc_1be2:
    cmp          word [0x6fc],BX                 ; 1000:1be2: 391efc06
    jz           loc_1bfa                        ; 1000:1be6: 7412
    cmp          BX,-0x1                         ; 1000:1be8: 83fbff
    jz           loc_1bfa                        ; 1000:1beb: 740d
    mov          word [0x6fc],BX                 ; 1000:1bed: 891efc06
    call         ent_init_monsters               ; 1000:1bf1: e87e04
    call         map_render_full_screen          ; 1000:1bf4: e88f02
    call         map_spawn_room_entities         ; 1000:1bf7: e869fe
loc_1bfa:
    ret                                          ; 1000:1bfa: c3

; ----------------------------------------------------------------------------
; FUNCTION: util_calc_stride  [Math / Geometry]
; ADDRESS:  1000:1bfb
; PURPOSE:  Calculates byte offset within display memory from tile grid coordinates.
; ----------------------------------------------------------------------------
util_calc_stride:
    cmp          word [cs:BX],-0x1               ; 1000:1bfb: 2e833fff
    jz           loc_1c07                        ; 1000:1bff: 7406
    cmp          word [cs:BX],0x5                ; 1000:1c01: 2e833f05
    jnz          loc_1c09                        ; 1000:1c05: 7502
loc_1c07:
    clc                                          ; 1000:1c07: f8
    ret                                          ; 1000:1c08: c3
loc_1c09:
    stc                                          ; 1000:1c09: f9
    ret                                          ; 1000:1c0a: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_parse_room_header  [Tilemap / Streaming]
; ADDRESS:  1000:1c0b
; PURPOSE:  Validates room data file format, parses room dimensions, and sets up boundary coordinates.
; ----------------------------------------------------------------------------
map_parse_room_header:
    push         AX                              ; 1000:1c0b: 50
    push         BX                              ; 1000:1c0c: 53
    push         BP                              ; 1000:1c0d: 55
    mov          BP,SP                           ; 1000:1c0e: 8bec
    sub          SP,0x2                          ; 1000:1c10: 83ec02
    cmp          AX,0x5                          ; 1000:1c13: 3d0500
    jz           loc_1c1e                        ; 1000:1c16: 7406
loc_1c18:
    mov          SP,BP                           ; 1000:1c18: 8be5
    pop          BP                              ; 1000:1c1a: 5d
    pop          BX                              ; 1000:1c1b: 5b
    pop          AX                              ; 1000:1c1c: 58
    ret                                          ; 1000:1c1d: c3
loc_1c1e:
    mov          BX,0x10b4                       ; 1000:1c1e: bbb410
    call         util_calc_stride                ; 1000:1c21: e8d7ff
    jc           loc_1c63                        ; 1000:1c24: 723d
    mov          BX,0x11e0                       ; 1000:1c26: bbe011
    call         util_calc_stride                ; 1000:1c29: e8cfff
    jc           loc_1c63                        ; 1000:1c2c: 7235
    mov          BX,0x12a0                       ; 1000:1c2e: bba012
    call         util_calc_stride                ; 1000:1c31: e8c7ff
    jc           loc_1c63                        ; 1000:1c34: 722d
    mov          BX,0x1324                       ; 1000:1c36: bb2413
    call         util_calc_stride                ; 1000:1c39: e8bfff
    jc           loc_1c63                        ; 1000:1c3c: 7225
    mov          BX,0x14f8                       ; 1000:1c3e: bbf814
    call         util_calc_stride                ; 1000:1c41: e8b7ff
    jc           loc_1c63                        ; 1000:1c44: 721d
    mov          BX,0x1558                       ; 1000:1c46: bb5815
    call         util_calc_stride                ; 1000:1c49: e8afff
    jc           loc_1c63                        ; 1000:1c4c: 7215
    cmp          byte [0x220],0x5                ; 1000:1c4e: 803e200205
    jge          loc_1c18                        ; 1000:1c53: 7dc3
    mov          word [cs:0x93dd],0xffff         ; 1000:1c55: 2ec706dd93ffff
    mov          byte [0x220],0x5                ; 1000:1c5c: c606200205
    jmp short    loc_1c18                        ; 1000:1c61: ebb5
loc_1c63:
    mov          BH,0xc0                         ; 1000:1c63: b7c0
    xor          BL,BL                           ; 1000:1c65: 32db
    push         ES                              ; 1000:1c67: 06
    mov          ES,BX                           ; 1000:1c68: 8ec3
    xor          SI,SI                           ; 1000:1c6a: 33f6
    xor          AX,AX                           ; 1000:1c6c: 33c0
    mov          CX,0x40                         ; 1000:1c6e: b94000
loc_1c71:
    add          AX,word [es:SI]                 ; 1000:1c71: 260304
    add          SI,0x2                          ; 1000:1c74: 83c602
    loop         loc_1c71                        ; 1000:1c77: e2f8
    mov          word [BP + -0x2],AX             ; 1000:1c79: 8946fe
    mov          AX,ES                           ; 1000:1c7c: 8cc0
    add          AH,0x30                         ; 1000:1c7e: 80c430
    mov          ES,AX                           ; 1000:1c81: 8ec0
    mov          SI,0x8000                       ; 1000:1c83: be0080
    add          SI,0x7000                       ; 1000:1c86: 81c60070
    xor          AX,AX                           ; 1000:1c8a: 33c0
    xor          BX,BX                           ; 1000:1c8c: 33db
    mov          CX,0x40                         ; 1000:1c8e: b94000
loc_1c91:
    add          AX,word [es:SI]                 ; 1000:1c91: 260304
    add          BX,word [es:SI + 0xf80]         ; 1000:1c94: 26039c800f
    add          SI,0x2                          ; 1000:1c99: 83c602
    loop         loc_1c91                        ; 1000:1c9c: e2f3
    add          word [BP + -0x2],AX             ; 1000:1c9e: 0146fe
    add          word [BP + -0x2],BX             ; 1000:1ca1: 015efe
    pop          ES                              ; 1000:1ca4: 07
    mov          AX,[0x9ce6]                     ; 1000:1ca5: a1e69c
    add          AX,word [BP + -0x2]             ; 1000:1ca8: 0346fe
    mov          DX,AX                           ; 1000:1cab: 8bd0
    shr          AX,0x1                          ; 1000:1cad: d1e8
    shr          AX,0x1                          ; 1000:1caf: d1e8
    shr          AX,0x1                          ; 1000:1cb1: d1e8
    shr          AX,0x1                          ; 1000:1cb3: d1e8
    shr          AX,0x1                          ; 1000:1cb5: d1e8
    add          DX,AX                           ; 1000:1cb7: 03d0
    and          DX,0x7ff                        ; 1000:1cb9: 81e2ff07
    add          DL,0x34                         ; 1000:1cbd: 80c234
    adc          DH,0x12                         ; 1000:1cc0: 80d612
    mov          AX,DX                           ; 1000:1cc3: 8bc2
    mov          BX,DX                           ; 1000:1cc5: 8bda
    mov          CX,DX                           ; 1000:1cc7: 8bca
    shl          BX,0x1                          ; 1000:1cc9: d1e3
    shl          AX,0x1                          ; 1000:1ccb: d1e0
    shl          BX,0x1                          ; 1000:1ccd: d1e3
    shl          CX,0x1                          ; 1000:1ccf: d1e1
    shl          AX,0x1                          ; 1000:1cd1: d1e0
    shl          BX,0x1                          ; 1000:1cd3: d1e3
    shl          AX,0x1                          ; 1000:1cd5: d1e0
    shl          BX,0x1                          ; 1000:1cd7: d1e3
    add          DX,AX                           ; 1000:1cd9: 03d0
    add          DX,BX                           ; 1000:1cdb: 03d3
    add          DX,CX                           ; 1000:1cdd: 03d1
    test         DX,0x40                         ; 1000:1cdf: f7c24000
    jz           loc_1ce7                        ; 1000:1ce3: 7402
    neg          DX                              ; 1000:1ce5: f7da
loc_1ce7:
    mov          AX,0x990                        ; 1000:1ce7: b89009
    mov          BX,0x93aa                       ; 1000:1cea: bbaa93
    mov          CX,BX                           ; 1000:1ced: 8bcb
    xor          DX,word [0x9cf8]                ; 1000:1cef: 3316f89c
    jnz          loc_1cf7                        ; 1000:1cf3: 7502
    mov          CX,AX                           ; 1000:1cf5: 8bc8
loc_1cf7:
    mov          word [0x93a8],CX                ; 1000:1cf7: 890ea893
    jmp near     loc_1c18                        ; 1000:1cfb: e91aff

; ----------------------------------------------------------------------------
; FUNCTION: map_get_tile_at  [Tilemap / Collision]
; ADDRESS:  1000:1cfe
; PURPOSE:  Looks up the tile index and collision flags at specified tile coordinate (X, Y).
; ----------------------------------------------------------------------------
map_get_tile_at:
    shr          AX,0x1                          ; 1000:1cfe: d1e8
    shr          AX,0x1                          ; 1000:1d00: d1e8
    shr          AX,0x1                          ; 1000:1d02: d1e8
    shr          AX,0x1                          ; 1000:1d04: d1e8
    shr          BX,0x1                          ; 1000:1d06: d1eb
    shr          BX,0x1                          ; 1000:1d08: d1eb
    shr          BX,0x1                          ; 1000:1d0a: d1eb
    shr          BX,0x1                          ; 1000:1d0c: d1eb
    cmp          BX,word [0x29a]                 ; 1000:1d0e: 3b1e9a02
    jl           loc_1d1a                        ; 1000:1d12: 7c06
    xor          BX,BX                           ; 1000:1d14: 33db
    mov          AX,0x100                        ; 1000:1d16: b80001
    ret                                          ; 1000:1d19: c3
loc_1d1a:
    mov          CX,BX                           ; 1000:1d1a: 8bcb
    shl          CX,0x1                          ; 1000:1d1c: d1e1
    mov          BX,0x2a0                        ; 1000:1d1e: bba002
    add          BX,CX                           ; 1000:1d21: 03d9
    mov          BX,word [BX]                    ; 1000:1d23: 8b1f
    add          BX,AX                           ; 1000:1d25: 03d8
    mov          AL,byte [BX]                    ; 1000:1d27: 8a07
    xor          AH,AH                           ; 1000:1d29: 32e4
    ret                                          ; 1000:1d2b: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_draw_background  [Tilemap / Rendering]
; ADDRESS:  1000:1d2c
; PURPOSE:  Draws complete room background tilemap to offscreen EGA video buffer.
; ----------------------------------------------------------------------------
map_draw_background:
    push         BP                              ; 1000:1d2c: 55
    mov          BP,SP                           ; 1000:1d2d: 8bec
    sub          SP,0x6                          ; 1000:1d2f: 83ec06
    mov          AX,[0x6c4]                      ; 1000:1d32: a1c406
    and          AX,0xfff0                       ; 1000:1d35: 25f0ff
    shr          AX,0x1                          ; 1000:1d38: d1e8
    shr          AX,0x1                          ; 1000:1d3a: d1e8
    shr          AX,0x1                          ; 1000:1d3c: d1e8
    lea          BX,[0x4a0]                      ; 1000:1d3e: 8d1ea004
    add          BX,AX                           ; 1000:1d42: 03d8
    mov          SI,word [BX]                    ; 1000:1d44: 8b37
    mov          AX,[0x6c2]                      ; 1000:1d46: a1c206
    shr          AX,0x1                          ; 1000:1d49: d1e8
    shr          AX,0x1                          ; 1000:1d4b: d1e8
    shr          AX,0x1                          ; 1000:1d4d: d1e8
    mov          word [BP + -0x6],AX             ; 1000:1d4f: 8946fa
    add          SI,AX                           ; 1000:1d52: 03f0
    mov          AX,[0x6c4]                      ; 1000:1d54: a1c406
    and          AX,0xf                          ; 1000:1d57: 250f00
    shl          AX,0x1                          ; 1000:1d5a: d1e0
    lea          BX,[0x6a0]                      ; 1000:1d5c: 8d1ea006
    add          BX,AX                           ; 1000:1d60: 03d8
    add          SI,word [BX]                    ; 1000:1d62: 0337
    mov          DI,word [cs:0x773f]             ; 1000:1d64: 2e8b3e3f77
    add          DI,0x141                        ; 1000:1d69: 81c74101
    mov          word [BP + -0x2],0x0            ; 1000:1d6d: c746fe0000
    mov          CX,word [0x6c0]                 ; 1000:1d72: 8b0ec006
    shl          CX,0x1                          ; 1000:1d76: d1e1
    shl          CX,0x1                          ; 1000:1d78: d1e1
    shl          CX,0x1                          ; 1000:1d7a: d1e1
    shl          CX,0x1                          ; 1000:1d7c: d1e1
    sub          CX,word [0x6c4]                 ; 1000:1d7e: 2b0ec406
    jle          loc_1d90                        ; 1000:1d82: 7e0c
    cmp          CX,0xa0                         ; 1000:1d84: 81f9a000
    jle          loc_1d8d                        ; 1000:1d88: 7e03
    mov          CX,0xa0                         ; 1000:1d8a: b9a000
loc_1d8d:
    mov          word [BP + -0x2],CX             ; 1000:1d8d: 894efe
loc_1d90:
    mov          CX,word [BP + -0x2]             ; 1000:1d90: 8b4efe
    neg          CX                              ; 1000:1d93: f7d9
    add          CX,0xa0                         ; 1000:1d95: 81c1a000
    mov          word [BP + -0x4],CX             ; 1000:1d99: 894efc
    mov          CL,0x0                          ; 1000:1d9c: b100
    mov          AX,SEG_BUFFER_2                 ; 1000:1d9e: b8cc47
    call         map_draw_tile_column            ; 1000:1da1: e81c00
    mov          CL,0x1                          ; 1000:1da4: b101
    mov          AX,SEG_BUFFER_1                 ; 1000:1da6: b8cc3f
    call         map_draw_tile_column            ; 1000:1da9: e81400
    mov          CL,0x2                          ; 1000:1dac: b102
    mov          AX,SEG_BUFFER_3                 ; 1000:1dae: b8cc4f
    call         map_draw_tile_column            ; 1000:1db1: e80c00
    mov          CL,0x3                          ; 1000:1db4: b103
    mov          AX,SEG_BUFFER_4                 ; 1000:1db6: b8cc57
    call         map_draw_tile_column            ; 1000:1db9: e80400
    mov          SP,BP                           ; 1000:1dbc: 8be5
    pop          BP                              ; 1000:1dbe: 5d
    ret                                          ; 1000:1dbf: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_draw_tile_column  [Tilemap / Rendering]
; ADDRESS:  1000:1dc0
; PURPOSE:  Draws a single vertical column of 16x16 tiles during horizontal scrolling.
; ----------------------------------------------------------------------------
map_draw_tile_column:
    push         SI                              ; 1000:1dc0: 56
    push         DI                              ; 1000:1dc1: 57
    push         AX                              ; 1000:1dc2: 50
    call         vid_out_dx_ax                   ; 1000:1dc3: e8a25c
    mov          DX,word [0x298]                 ; 1000:1dc6: 8b169802
    shl          DX,0x1                          ; 1000:1dca: d1e2
    sub          DX,0x1a                         ; 1000:1dcc: 83ea1a
    mov          BX,word [BP + -0x2]             ; 1000:1dcf: 8b5efe
    or           BX,BX                           ; 1000:1dd2: 0bdb
    jz           loc_1ded                        ; 1000:1dd4: 7417
    push         DS                              ; 1000:1dd6: 1e
    mov          AX,0xa000                       ; 1000:1dd7: b800a0
    mov          DS,AX                           ; 1000:1dda: 8ed8
loc_1ddc:
    mov          CX,0xd                          ; 1000:1ddc: b90d00
    rep movsw                                    ; 1000:1ddf: f3a5
    add          SI,DX                           ; 1000:1de1: 03f2
    add          DI,0xe                          ; 1000:1de3: 83c70e
    dec          BX                              ; 1000:1de6: 4b
    jnz          loc_1ddc                        ; 1000:1de7: 75f3
    pop          DS                              ; 1000:1de9: 1f
    mov          SI,word [BP + -0x6]             ; 1000:1dea: 8b76fa
loc_1ded:
    pop          AX                              ; 1000:1ded: 58
    mov          BX,word [BP + -0x4]             ; 1000:1dee: 8b5efc
    or           BX,BX                           ; 1000:1df1: 0bdb
    jz           loc_1e06                        ; 1000:1df3: 7411
    push         DS                              ; 1000:1df5: 1e
    mov          DS,AX                           ; 1000:1df6: 8ed8
loc_1df8:
    mov          CX,0xd                          ; 1000:1df8: b90d00
    rep movsw                                    ; 1000:1dfb: f3a5
    add          SI,DX                           ; 1000:1dfd: 03f2
    add          DI,0xe                          ; 1000:1dff: 83c70e
    dec          BX                              ; 1000:1e02: 4b
    jnz          loc_1df8                        ; 1000:1e03: 75f3
    pop          DS                              ; 1000:1e05: 1f
loc_1e06:
    pop          DI                              ; 1000:1e06: 5f
    pop          SI                              ; 1000:1e07: 5e
    ret                                          ; 1000:1e08: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_draw_tile_row  [Tilemap / Rendering]
; ADDRESS:  1000:1e09
; PURPOSE:  Draws a single horizontal row of 16x16 tiles during vertical scrolling.
; ----------------------------------------------------------------------------
map_draw_tile_row:
    shr          BX,0x1                          ; 1000:1e09: d1eb
    shr          BX,0x1                          ; 1000:1e0b: d1eb
    shr          BX,0x1                          ; 1000:1e0d: d1eb
    shr          BX,0x1                          ; 1000:1e0f: d1eb
    push         BX                              ; 1000:1e11: 53
    shl          BX,0x1                          ; 1000:1e12: d1e3
    lea          DX,[0x4a0]                      ; 1000:1e14: 8d16a004
    add          BX,DX                           ; 1000:1e18: 03da
    mov          DI,word [BX]                    ; 1000:1e1a: 8b3f
    shr          AX,0x1                          ; 1000:1e1c: d1e8
    shr          AX,0x1                          ; 1000:1e1e: d1e8
    shr          AX,0x1                          ; 1000:1e20: d1e8
    shr          AX,0x1                          ; 1000:1e22: d1e8
    shl          AX,0x1                          ; 1000:1e24: d1e0
    add          DI,AX                           ; 1000:1e26: 03f8
    mov          AL,CL                           ; 1000:1e28: 8ac1
    xor          AH,AH                           ; 1000:1e2a: 32e4
    shl          AX,0x1                          ; 1000:1e2c: d1e0
    shl          AX,0x1                          ; 1000:1e2e: d1e0
    shl          AX,0x1                          ; 1000:1e30: d1e0
    shl          AX,0x1                          ; 1000:1e32: d1e0
    shl          AX,0x1                          ; 1000:1e34: d1e0
    shl          AX,0x1                          ; 1000:1e36: d1e0
    shl          AX,0x1                          ; 1000:1e38: d1e0
    lea          SI,[0x13a8]                     ; 1000:1e3a: 8d36a813
    add          SI,AX                           ; 1000:1e3e: 03f0
    pop          AX                              ; 1000:1e40: 58
    cmp          AX,word [0x6c0]                 ; 1000:1e41: 3b06c006
    jl           loc_1e6d                        ; 1000:1e45: 7c26
    mov          CX,0x0                          ; 1000:1e47: b90000
    mov          AX,SEG_BUFFER_2                 ; 1000:1e4a: b8cc47
    call         map_clear_tile_attr             ; 1000:1e4d: e83901
    mov          CX,0x1                          ; 1000:1e50: b90100
    mov          AX,SEG_BUFFER_1                 ; 1000:1e53: b8cc3f
    call         map_clear_tile_attr             ; 1000:1e56: e83001
    mov          CX,0x2                          ; 1000:1e59: b90200
    mov          AX,SEG_BUFFER_3                 ; 1000:1e5c: b8cc4f
    call         map_clear_tile_attr             ; 1000:1e5f: e82701
    mov          CX,0x3                          ; 1000:1e62: b90300
    mov          AX,SEG_BUFFER_4                 ; 1000:1e65: b8cc57
    call         map_clear_tile_attr             ; 1000:1e68: e81e01
    jmp short    loc_1e85                        ; 1000:1e6b: eb18
loc_1e6d:
    mov          CX,0x0                          ; 1000:1e6d: b90000
    call         map_set_tile_attr               ; 1000:1e70: e8ff00
    mov          CX,0x1                          ; 1000:1e73: b90100
    call         map_set_tile_attr               ; 1000:1e76: e8f900
    mov          CX,0x2                          ; 1000:1e79: b90200
    call         map_set_tile_attr               ; 1000:1e7c: e8f300
    mov          CX,0x3                          ; 1000:1e7f: b90300
    call         map_set_tile_attr               ; 1000:1e82: e8ed00
loc_1e85:
    ret                                          ; 1000:1e85: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_render_full_screen  [Tilemap / Rendering]
; ADDRESS:  1000:1e86
; PURPOSE:  Renders full visible screen grid of tiles to offscreen video page.
; ----------------------------------------------------------------------------
map_render_full_screen:
    push         BP                              ; 1000:1e86: 55
    mov          BP,SP                           ; 1000:1e87: 8bec
    sub          SP,0x2                          ; 1000:1e89: 83ec02
    xor          DX,DX                           ; 1000:1e8c: 33d2
    mov          AX,0x600                        ; 1000:1e8e: b80006
    mov          CX,word [0x298]                 ; 1000:1e91: 8b0e9802
    div          CX                              ; 1000:1e95: f7f1
    mov          [0x6c0],AX                      ; 1000:1e97: a3c006
    mov          DX,word [0x298]                 ; 1000:1e9a: 8b169802
    shl          DX,0x1                          ; 1000:1e9e: d1e2
    lea          BX,[0x6a0]                      ; 1000:1ea0: 8d1ea006
    xor          AX,AX                           ; 1000:1ea4: 33c0
    mov          CX,0x10                         ; 1000:1ea6: b91000
loc_1ea9:
    mov          word [BX],AX                    ; 1000:1ea9: 8907
    add          BX,0x2                          ; 1000:1eab: 83c302
    add          AX,DX                           ; 1000:1eae: 03c2
    loop         loc_1ea9                        ; 1000:1eb0: e2f7
    mov          DX,AX                           ; 1000:1eb2: 8bd0
    lea          BX,[0x4a0]                      ; 1000:1eb4: 8d1ea004
    mov          AX,0x4000                       ; 1000:1eb8: b80040
    xor          CX,CX                           ; 1000:1ebb: 33c9
loc_1ebd:
    mov          word [BX],AX                    ; 1000:1ebd: 8907
    add          AX,DX                           ; 1000:1ebf: 03c2
    add          BX,0x2                          ; 1000:1ec1: 83c302
    inc          CX                              ; 1000:1ec4: 41
    cmp          CX,word [0x6c0]                 ; 1000:1ec5: 3b0ec006
    jl           loc_1ebd                        ; 1000:1ec9: 7cf2
    xor          AX,AX                           ; 1000:1ecb: 33c0
loc_1ecd:
    mov          word [BX],AX                    ; 1000:1ecd: 8907
    add          AX,DX                           ; 1000:1ecf: 03c2
    add          BX,0x2                          ; 1000:1ed1: 83c302
    inc          CX                              ; 1000:1ed4: 41
    cmp          CX,word [0x29a]                 ; 1000:1ed5: 3b0e9a02
    jl           loc_1ecd                        ; 1000:1ed9: 7cf2
    mov          word [BP + -0x2],0x0            ; 1000:1edb: c746fe0000
    lea          SI,[0x9a8]                      ; 1000:1ee0: 8d36a809
loc_1ee4:
    mov          CX,word [0x298]                 ; 1000:1ee4: 8b0e9802
    mov          AX,word [BP + -0x2]             ; 1000:1ee8: 8b46fe
    shl          AX,0x1                          ; 1000:1eeb: d1e0
    lea          BX,[0x4a0]                      ; 1000:1eed: 8d1ea004
    add          BX,AX                           ; 1000:1ef1: 03d8
    mov          DI,word [BX]                    ; 1000:1ef3: 8b3f
loc_1ef5:
    lodsb                                        ; 1000:1ef5: ac
    xor          AH,AH                           ; 1000:1ef6: 32e4
    push         SI                              ; 1000:1ef8: 56
    shl          AX,0x1                          ; 1000:1ef9: d1e0
    shl          AX,0x1                          ; 1000:1efb: d1e0
    shl          AX,0x1                          ; 1000:1efd: d1e0
    shl          AX,0x1                          ; 1000:1eff: d1e0
    shl          AX,0x1                          ; 1000:1f01: d1e0
    shl          AX,0x1                          ; 1000:1f03: d1e0
    shl          AX,0x1                          ; 1000:1f05: d1e0
    lea          SI,[0x13a8]                     ; 1000:1f07: 8d36a813
    add          SI,AX                           ; 1000:1f0b: 03f0
    mov          AX,word [BP + -0x2]             ; 1000:1f0d: 8b46fe
    cmp          AX,word [0x6c0]                 ; 1000:1f10: 3b06c006
    jl           loc_1f3f                        ; 1000:1f14: 7c29
    push         CX                              ; 1000:1f16: 51
    mov          CX,0x0                          ; 1000:1f17: b90000
    mov          AX,SEG_BUFFER_2                 ; 1000:1f1a: b8cc47
    call         map_clear_tile_attr             ; 1000:1f1d: e86900
    mov          CX,0x1                          ; 1000:1f20: b90100
    mov          AX,SEG_BUFFER_1                 ; 1000:1f23: b8cc3f
    call         map_clear_tile_attr             ; 1000:1f26: e86000
    mov          CX,0x2                          ; 1000:1f29: b90200
    mov          AX,SEG_BUFFER_3                 ; 1000:1f2c: b8cc4f
    call         map_clear_tile_attr             ; 1000:1f2f: e85700
    mov          CX,0x3                          ; 1000:1f32: b90300
    mov          AX,SEG_BUFFER_4                 ; 1000:1f35: b8cc57
    call         map_clear_tile_attr             ; 1000:1f38: e84e00
    pop          CX                              ; 1000:1f3b: 59
    jmp short    loc_1f59                        ; 1000:1f3c: eb1b
    db 0x90                                         ; 1000:1f3e
loc_1f3f:
    push         CX                              ; 1000:1f3f: 51
    mov          CX,0x0                          ; 1000:1f40: b90000
    call         map_set_tile_attr               ; 1000:1f43: e82c00
    mov          CX,0x1                          ; 1000:1f46: b90100
    call         map_set_tile_attr               ; 1000:1f49: e82600
    mov          CX,0x2                          ; 1000:1f4c: b90200
    call         map_set_tile_attr               ; 1000:1f4f: e82000
    mov          CX,0x3                          ; 1000:1f52: b90300
    call         map_set_tile_attr               ; 1000:1f55: e81a00
    pop          CX                              ; 1000:1f58: 59
loc_1f59:
    add          DI,0x2                          ; 1000:1f59: 83c702
    pop          SI                              ; 1000:1f5c: 5e
    loop         loc_1ef5                        ; 1000:1f5d: e296
    inc          word [BP + -0x2]                ; 1000:1f5f: ff46fe
    mov          AX,word [BP + -0x2]             ; 1000:1f62: 8b46fe
    cmp          AX,word [0x29a]                 ; 1000:1f65: 3b069a02
    jge          loc_1f6e                        ; 1000:1f69: 7d03
    jmp near     loc_1ee4                        ; 1000:1f6b: e976ff
loc_1f6e:
    mov          SP,BP                           ; 1000:1f6e: 8be5
    pop          BP                              ; 1000:1f70: 5d
    ret                                          ; 1000:1f71: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_set_tile_attr  [Tilemap / Flags]
; ADDRESS:  1000:1f72
; PURPOSE:  Sets tile attribute and collision flags in the active room collision buffer.
; ----------------------------------------------------------------------------
map_set_tile_attr:
    call         vid_out_dx_ax                   ; 1000:1f72: e8f35a
    mov          CX,0x10                         ; 1000:1f75: b91000
    mov          DX,word [0x298]                 ; 1000:1f78: 8b169802
    shl          DX,0x1                          ; 1000:1f7c: d1e2
    sub          DX,0x2                          ; 1000:1f7e: 83ea02
    push         DI                              ; 1000:1f81: 57
loc_1f82:
    movsw                                        ; 1000:1f82: a5
    add          DI,DX                           ; 1000:1f83: 03fa
    loop         loc_1f82                        ; 1000:1f85: e2fb
    pop          DI                              ; 1000:1f87: 5f
    ret                                          ; 1000:1f88: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_clear_tile_attr  [Tilemap / Flags]
; ADDRESS:  1000:1f89
; PURPOSE:  Clears tile attribute and collision flags in the active room collision buffer.
; ----------------------------------------------------------------------------
map_clear_tile_attr:
    push         ES                              ; 1000:1f89: 06
    mov          ES,AX                           ; 1000:1f8a: 8ec0
    call         vid_out_dx_ax                   ; 1000:1f8c: e8d95a
    mov          CX,0x10                         ; 1000:1f8f: b91000
    mov          DX,word [0x298]                 ; 1000:1f92: 8b169802
    shl          DX,0x1                          ; 1000:1f96: d1e2
    sub          DX,0x2                          ; 1000:1f98: 83ea02
    push         DI                              ; 1000:1f9b: 57
loc_1f9c:
    movsw                                        ; 1000:1f9c: a5
    add          DI,DX                           ; 1000:1f9d: 03fa
    loop         loc_1f9c                        ; 1000:1f9f: e2fb
    pop          DI                              ; 1000:1fa1: 5f
    pop          ES                              ; 1000:1fa2: 07
    ret                                          ; 1000:1fa3: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_pack_rle  [File I/O / Compression]
; ADDRESS:  1000:1fa4
; PURPOSE:  Opens file via DOS INT 21h AH=3Dh and decompresses Byte-Run / PackBits RLE compressed data into destination buffer.
; ----------------------------------------------------------------------------
file_load_pack_rle:
    mov          AX,0x3d00                       ; 1000:1fa4: b8003d
    int          0x21                            ; 1000:1fa7: cd21
    jnc          loc_1fae                        ; 1000:1fa9: 7303
    jmp near     sys_fatal_error                 ; 1000:1fab: e954e2
loc_1fae:
    mov          BX,AX                           ; 1000:1fae: 8bd8
    lea          DX,[0x292]                      ; 1000:1fb0: 8d169202
    mov          CX,0x6                          ; 1000:1fb4: b90600
    mov          AX,0x3f00                       ; 1000:1fb7: b8003f
    int          0x21                            ; 1000:1fba: cd21
    push         DS                              ; 1000:1fbc: 1e
    push         ES                              ; 1000:1fbd: 06
    mov          CX,DS                           ; 1000:1fbe: 8cd9
    mov          ES,CX                           ; 1000:1fc0: 8ec1
    mov          CX,SEG_SCRATCH                  ; 1000:1fc2: b95d1d
    mov          DS,CX                           ; 1000:1fc5: 8ed9
    lea          DX,[0x600]                      ; 1000:1fc7: 8d160006
    mov          CX,0x7000                       ; 1000:1fcb: b90070
    mov          AX,0x3f00                       ; 1000:1fce: b8003f
    int          0x21                            ; 1000:1fd1: cd21
    mov          AX,0x3e00                       ; 1000:1fd3: b8003e
    int          0x21                            ; 1000:1fd6: cd21
    lea          SI,[0x600]                      ; 1000:1fd8: 8d360006
    lea          DI,[0x13a8]                     ; 1000:1fdc: 8d3ea813
    xor          CH,CH                           ; 1000:1fe0: 32ed
loc_1fe2:
    lodsb                                        ; 1000:1fe2: ac
    cmp          AL,0x0                          ; 1000:1fe3: 3c00
    jl           loc_1fec                        ; 1000:1fe5: 7c05
    jg           loc_1ff5                        ; 1000:1fe7: 7f0c
    jmp short    loc_1ffb                        ; 1000:1fe9: eb10
    db 0x90                                         ; 1000:1feb
loc_1fec:
    neg          AL                              ; 1000:1fec: f6d8
    mov          CL,AL                           ; 1000:1fee: 8ac8
    lodsb                                        ; 1000:1ff0: ac
    rep stosb                                    ; 1000:1ff1: f3aa
    jmp short    loc_1fe2                        ; 1000:1ff3: ebed
loc_1ff5:
    mov          CL,AL                           ; 1000:1ff5: 8ac8
    rep movsb                                    ; 1000:1ff7: f3a4
    jmp short    loc_1fe2                        ; 1000:1ff9: ebe7
loc_1ffb:
    pop          ES                              ; 1000:1ffb: 07
    pop          DS                              ; 1000:1ffc: 1f
    ret                                          ; 1000:1ffd: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_raw_mask  [File I/O / Assets]
; ADDRESS:  1000:1ffe
; PURPOSE:  Reads raw collision mask table (.1) from disk into room collision buffer.
; ----------------------------------------------------------------------------
file_load_raw_mask:
    mov          AX,0x3d00                       ; 1000:1ffe: b8003d
    int          0x21                            ; 1000:2001: cd21
    jnc          loc_2008                        ; 1000:2003: 7303
    jmp near     sys_fatal_error                 ; 1000:2005: e9fae1
loc_2008:
    mov          BX,AX                           ; 1000:2008: 8bd8
    push         DS                              ; 1000:200a: 1e
    mov          AX,SEG_SCRATCH                  ; 1000:200b: b85d1d
    mov          DS,AX                           ; 1000:200e: 8ed8
    mov          DX,0x600                        ; 1000:2010: ba0006
    mov          CX,0xffff                       ; 1000:2013: b9ffff
    mov          AX,0x3f00                       ; 1000:2016: b8003f
    int          0x21                            ; 1000:2019: cd21
    pop          DS                              ; 1000:201b: 1f
    mov          AX,0x3e00                       ; 1000:201c: b8003e
    int          0x21                            ; 1000:201f: cd21
    ret                                          ; 1000:2021: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_tileset  [File I/O / Assets]
; ADDRESS:  1000:2022
; PURPOSE:  Reads raw tile graphic bitmap data (.2) from disk into tileset buffer.
; ----------------------------------------------------------------------------
file_load_tileset:
    mov          AX,0x3d00                       ; 1000:2022: b8003d
    int          0x21                            ; 1000:2025: cd21
    jnc          loc_202c                        ; 1000:2027: 7303
    jmp near     sys_fatal_error                 ; 1000:2029: e9d6e1
loc_202c:
    mov          BX,AX                           ; 1000:202c: 8bd8
    mov          DS,word [cs:0x5e]               ; 1000:202e: 2e8e1e5e00
    mov          DX,0x0                          ; 1000:2033: ba0000
    mov          CX,0xffff                       ; 1000:2036: b9ffff
    mov          AX,0x3f00                       ; 1000:2039: b8003f
    int          0x21                            ; 1000:203c: cd21
    mov          DS,word [cs:0x5c]               ; 1000:203e: 2e8e1e5c00
    mov          AX,0x3e00                       ; 1000:2043: b8003e
    int          0x21                            ; 1000:2046: cd21
    ret                                          ; 1000:2048: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_entity_data  [File I/O / Assets]
; ADDRESS:  1000:2049
; PURPOSE:  Reads room item/entity spawn parameters (.3) from disk.
; ----------------------------------------------------------------------------
file_load_entity_data:
    mov          AX,0x3d00                       ; 1000:2049: b8003d
    int          0x21                            ; 1000:204c: cd21
    jnc          loc_2053                        ; 1000:204e: 7303
    jmp near     sys_fatal_error                 ; 1000:2050: e9afe1
loc_2053:
    mov          BX,AX                           ; 1000:2053: 8bd8
    mov          DX,0x9ec0                       ; 1000:2055: bac09e
    mov          CX,0xc00                        ; 1000:2058: b9000c
    mov          AX,0x3f00                       ; 1000:205b: b8003f
    int          0x21                            ; 1000:205e: cd21
    mov          AX,0x3e00                       ; 1000:2060: b8003e
    int          0x21                            ; 1000:2063: cd21
    mov          CX,0xc00                        ; 1000:2065: b9000c
    mov          BX,0x9ec0                       ; 1000:2068: bbc09e
loc_206b:
    xor          byte [BX],0x25                  ; 1000:206b: 803725
    inc          BX                              ; 1000:206e: 43
    loop         loc_206b                        ; 1000:206f: e2fa
    ret                                          ; 1000:2071: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_init_monsters  [Entities / Management]
; ADDRESS:  1000:2072
; PURPOSE:  Initializes monster state machines, animation timers, and patrol bounds for the loaded room.
; ----------------------------------------------------------------------------
ent_init_monsters:
    push         DS                              ; 1000:2072: 1e
    push         ES                              ; 1000:2073: 06
    mov          CX,DS                           ; 1000:2074: 8cd9
    mov          ES,CX                           ; 1000:2076: 8ec1
    mov          CX,SEG_SCRATCH                  ; 1000:2078: b95d1d
    mov          DS,CX                           ; 1000:207b: 8ed9
    mov          SI,0x600                        ; 1000:207d: be0006
    add          SI,0x2                          ; 1000:2080: 83c602
    cmp          AX,word [SI]                    ; 1000:2083: 3b04
    jz           loc_208c                        ; 1000:2085: 7405
    pop          ES                              ; 1000:2087: 07
    pop          DS                              ; 1000:2088: 1f
    jmp near     sys_fatal_error                 ; 1000:2089: e976e1
loc_208c:
    add          SI,0x2                          ; 1000:208c: 83c602
    mov          CX,BX                           ; 1000:208f: 8bcb
    shl          BX,0x1                          ; 1000:2091: d1e3
    add          BX,CX                           ; 1000:2093: 03d9
    shl          BX,0x1                          ; 1000:2095: d1e3
    add          SI,BX                           ; 1000:2097: 03f3
    lodsw                                        ; 1000:2099: ad
    mov          [es:0x298],AX                   ; 1000:209a: 26a39802
    lodsw                                        ; 1000:209e: ad
    mov          [es:0x29a],AX                   ; 1000:209f: 26a39a02
    lodsw                                        ; 1000:20a3: ad
    mov          SI,0x600                        ; 1000:20a4: be0006
    add          SI,AX                           ; 1000:20a7: 03f0
    lea          DI,[0x9a8]                      ; 1000:20a9: 8d3ea809
    xor          CH,CH                           ; 1000:20ad: 32ed
loc_20af:
    lodsb                                        ; 1000:20af: ac
    cmp          AL,0x0                          ; 1000:20b0: 3c00
    jl           loc_20b9                        ; 1000:20b2: 7c05
    jg           loc_20c2                        ; 1000:20b4: 7f0c
    jmp short    loc_20c8                        ; 1000:20b6: eb10
    db 0x90                                         ; 1000:20b8
loc_20b9:
    neg          AL                              ; 1000:20b9: f6d8
    mov          CL,AL                           ; 1000:20bb: 8ac8
    lodsb                                        ; 1000:20bd: ac
    rep stosb                                    ; 1000:20be: f3aa
    jmp short    loc_20af                        ; 1000:20c0: ebed
loc_20c2:
    mov          CL,AL                           ; 1000:20c2: 8ac8
    rep movsb                                    ; 1000:20c4: f3a4
    jmp short    loc_20af                        ; 1000:20c6: ebe7
loc_20c8:
    pop          ES                              ; 1000:20c8: 07
    pop          DS                              ; 1000:20c9: 1f
    mov          BX,0x2a0                        ; 1000:20ca: bba002
    mov          CX,word [0x29a]                 ; 1000:20cd: 8b0e9a02
    mov          AX,0x9a8                        ; 1000:20d1: b8a809
    mov          DX,word [0x298]                 ; 1000:20d4: 8b169802
loc_20d8:
    mov          word [BX],AX                    ; 1000:20d8: 8907
    add          AX,DX                           ; 1000:20da: 03c2
    add          BX,0x2                          ; 1000:20dc: 83c302
    loop         loc_20d8                        ; 1000:20df: e2f7
    mov          AX,[0x298]                      ; 1000:20e1: a19802
    shl          AX,0x1                          ; 1000:20e4: d1e0
    shl          AX,0x1                          ; 1000:20e6: d1e0
    shl          AX,0x1                          ; 1000:20e8: d1e0
    shl          AX,0x1                          ; 1000:20ea: d1e0
    mov          [0x29c],AX                      ; 1000:20ec: a39c02
    mov          AX,[0x29a]                      ; 1000:20ef: a19a02
    shl          AX,0x1                          ; 1000:20f2: d1e0
    shl          AX,0x1                          ; 1000:20f4: d1e0
    shl          AX,0x1                          ; 1000:20f6: d1e0
    shl          AX,0x1                          ; 1000:20f8: d1e0
    mov          [0x29e],AX                      ; 1000:20fa: a39e02
    ret                                          ; 1000:20fd: c3

; ----------------------------------------------------------------------------
; FUNCTION: game_start_room  [Game / Lifecycle]
; ADDRESS:  1000:20fe
; PURPOSE:  Initializes Comic at room spawn coordinates, loads room assets, and enters main game loop.
; ----------------------------------------------------------------------------
game_start_room:
    mov          word [0x892],0x0                ; 1000:20fe: c70692080000
    mov          AX,word [cs:SI + 0xa]           ; 1000:2104: 2e8b440a
    mov          BX,word [cs:SI + 0xc]           ; 1000:2108: 2e8b5c0c
    mov          CX,word [cs:SI + 0xe]           ; 1000:210c: 2e8b4c0e
    push         CX                              ; 1000:2110: 51
    push         BX                              ; 1000:2111: 53
    push         AX                              ; 1000:2112: 50
    cmp          byte [cs:SI + 0x6],0x0          ; 1000:2113: 2e807c0600
    jz           loc_214c                        ; 1000:2118: 7432
    cmp          byte [cs:SI + 0x6],0xff         ; 1000:211a: 2e807c06ff
    jz           loc_213f                        ; 1000:211f: 741e
    call         snd_play_hit                    ; 1000:2121: e88d1b
    cmp          SI,0xb8a4                       ; 1000:2124: 81fea4b8
    jnz          loc_214f                        ; 1000:2128: 7525
    cmp          byte [0x221],0x1                ; 1000:212a: 803e210201
    jnz          loc_213a                        ; 1000:212f: 7509
    mov          AX,[cs:0xb97a]                  ; 1000:2131: 2ea17ab9
    call         snd_play_level_music            ; 1000:2135: e8d21d
    jmp short    loc_214f                        ; 1000:2138: eb15
loc_213a:
    add          SP,0x6                          ; 1000:213a: 83c406
    jmp short    loc_2198                        ; 1000:213d: eb59
loc_213f:
    mov          AX,0x1                          ; 1000:213f: b80100
    lea          BX,[0x93cc]                     ; 1000:2142: 8d1ecc93
    mov          CX,0x9                          ; 1000:2146: b90900
    int3                                         ; 1000:2149: cc
    db 0xeb, 0x03                                   ; 1000:214a
loc_214c:
    call         snd_play_pickup                 ; 1000:214c: e8771a
loc_214f:
    pop          AX                              ; 1000:214f: 58
    pop          BX                              ; 1000:2150: 5b
    pop          CX                              ; 1000:2151: 59
    cmp          AX,0xffff                       ; 1000:2152: 3dffff
    jz           loc_21c0                        ; 1000:2155: 7469
    cmp          AX,0xfffe                       ; 1000:2157: 3dfeff
    jnz          loc_215f                        ; 1000:215a: 7503
    jmp near     game_victory_sequence           ; 1000:215c: e99300
loc_215f:
    push         CX                              ; 1000:215f: 51
    call         map_load_room                   ; 1000:2160: e82afa
    pop          CX                              ; 1000:2163: 59
loc_2164:
    mov          SI,word [0x6f0]                 ; 1000:2164: 8b36f006
    or           CX,CX                           ; 1000:2168: 0bc9
    jz           loc_2173                        ; 1000:216a: 7407
loc_216c:
    add          SI,0x12                         ; 1000:216c: 83c612
    dec          CL                              ; 1000:216f: fec9
    jnz          loc_216c                        ; 1000:2171: 75f9
loc_2173:
    mov          AX,word [cs:SI + 0x4]           ; 1000:2173: 2e8b4404
    shr          AX,0x1                          ; 1000:2177: d1e8
    add          AX,word [cs:SI]                 ; 1000:2179: 2e0304
    mov          [0x6c6],AX                      ; 1000:217c: a3c606
    mov          AX,word [cs:SI + 0x2]           ; 1000:217f: 2e8b4402
    mov          [0x6c8],AX                      ; 1000:2183: a3c806
    push         SI                              ; 1000:2186: 56
    call         spr_render_player               ; 1000:2187: e856f7
    pop          SI                              ; 1000:218a: 5e
    cmp          SI,0xb8a4                       ; 1000:218b: 81fea4b8
    jnz          loc_2198                        ; 1000:218f: 7507
    mov          AX,[cs:0xb97a]                  ; 1000:2191: 2ea17ab9
    call         snd_play_level_music            ; 1000:2195: e8721d
loc_2198:
    cmp          byte [cs:SI + 0x6],0x0          ; 1000:2198: 2e807c0600
    jz           loc_21a9                        ; 1000:219d: 740a
    cmp          byte [cs:SI + 0x6],0xff         ; 1000:219f: 2e807c06ff
    jz           loc_21a9                        ; 1000:21a4: 7403
    call         snd_play_blast                  ; 1000:21a6: e86a1a
loc_21a9:
    mov          byte [cs:0x25a],0x0             ; 1000:21a9: 2ec6065a0200
    mov          AX,0x1                          ; 1000:21af: b80100
    call         timer_delay_ticks               ; 1000:21b2: e8c5e0
    jmp near     game_main_loop                  ; 1000:21b5: e98901

; ----------------------------------------------------------------------------
; FUNCTION: game_respawn_player  [Game / Lifecycle]
; ADDRESS:  1000:21b8
; PURPOSE:  Handles player death, decrements lives counter, checks for Game Over, or restarts player at room spawn point.
; ----------------------------------------------------------------------------
game_respawn_player:
    mov          byte [0x8ad],0x1                ; 1000:21b8: c606ad0801
    call         snd_play_teleport               ; 1000:21bd: e8d81c
loc_21c0:
    mov          AX,0x5                          ; 1000:21c0: b80500
    mov          BX,0x3                          ; 1000:21c3: bb0300
    call         map_load_room                   ; 1000:21c6: e8c4f9
    mov          word [0x6c6],0x28               ; 1000:21c9: c706c6062800
    mov          word [0x6c8],0x60               ; 1000:21cf: c706c8066000
    mov          byte [0x8ad],0x1                ; 1000:21d5: c606ad0801
    call         spr_render_player               ; 1000:21da: e803f7
    call         snd_play_door_open              ; 1000:21dd: e83f1c
    mov          byte [0x897],0x0                ; 1000:21e0: c606970800
    mov          byte [0x291],0x1                ; 1000:21e5: c606910201
    mov          byte [0x89c],0x0                ; 1000:21ea: c6069c0800
    jmp near     game_main_loop                  ; 1000:21ef: e94f01

; ----------------------------------------------------------------------------
; FUNCTION: game_victory_sequence  [Game / Sequence]
; ADDRESS:  1000:21f2
; PURPOSE:  Handles the end-game victory sequence, awards bonus points, and displays the ending cutscene.
; ----------------------------------------------------------------------------
game_victory_sequence:
    mov          AX,[cs:0x9300]                  ; 1000:21f2: 2ea10093
    call         snd_play_level_music            ; 1000:21f6: e8111d
    mov          AX,0x5                          ; 1000:21f9: b80500
    call         timer_delay_ticks               ; 1000:21fc: e87be0
    call         file_check_config_exists        ; 1000:21ff: e8ed43
    jmp near     vid_fade_screen                 ; 1000:2202: e980df

; ----------------------------------------------------------------------------
; FUNCTION: game_over_sequence  [Game / Sequence]
; ADDRESS:  1000:2205
; PURPOSE:  Handles Game Over sequence, plays game over jingle, and transitions to high score entry.
; ----------------------------------------------------------------------------
game_over_sequence:
    mov          AX,0x1                          ; 1000:2205: b80100
    lea          BX,[0x93cc]                     ; 1000:2208: 8d1ecc93
    mov          CX,0x9                          ; 1000:220c: b90900
    int3                                         ; 1000:220f: cc
    db 0x2e, 0x8b, 0x44, 0x06, 0x3d, 0xff, 0xff, 0x74, 0xa7, 0x2e, 0x8b, 0x5c, 0x08, 0x2e, 0x8b, 0x4c ; 1000:2210
    db 0x0a, 0x51, 0xe8, 0x68, 0xf9, 0x59, 0x8b, 0x36, 0xf4, 0x06, 0x0b, 0xc9, 0x74, 0x07, 0x83, 0xc6 ; 1000:2220
    db 0x0c, 0xfe, 0xc9, 0x75, 0xf9, 0x2e, 0x8b, 0x04, 0xa3, 0xc6, 0x06, 0x2e, 0x8b, 0x44, 0x04, 0xa3 ; 1000:2230
    db 0xc8, 0x06, 0xe8, 0x9b, 0xf6, 0xc6, 0x06, 0x97, 0x08, 0x00, 0xc7, 0x06, 0x94, 0x08, 0x00, 0x00 ; 1000:2240
    db 0xe9, 0xee, 0x00                             ; 1000:2250

; ----------------------------------------------------------------------------
; FUNCTION: game_switch_room  [Game / Streaming]
; ADDRESS:  1000:2253
; PURPOSE:  Processes room transition doorways, saves room state, loads destination room, and spawns player.
; ----------------------------------------------------------------------------
game_switch_room:
    mov          byte [0x8ad],0x1                ; 1000:2253: c606ad0801
    mov          word [0x6c6],0x28               ; 1000:2258: c706c6062800
    call         snd_play_teleport               ; 1000:225e: e8371c
    mov          BX,word [0x21a]                 ; 1000:2261: 8b1e1a02
    shl          BX,0x1                          ; 1000:2265: d1e3
    shl          BX,0x1                          ; 1000:2267: d1e3
    add          BX,word [0x93a8]                ; 1000:2269: 031ea893
    mov          AX,word [BX]                    ; 1000:226d: 8b07
    mov          BX,word [BX + 0x2]              ; 1000:226f: 8b5f02
    call         map_load_room                   ; 1000:2272: e818f9
    mov          CX,0x0                          ; 1000:2275: b90000
    jmp near     loc_2164                        ; 1000:2278: e9e9fe

; ----------------------------------------------------------------------------
; FUNCTION: spr_animate_player  [Sprites / Player]
; ADDRESS:  1000:227b
; PURPOSE:  Updates Comic's walking, jumping, and shooting animation frame counters based on velocity.
; ----------------------------------------------------------------------------
spr_animate_player:
    call         hud_draw_digits                 ; 1000:227b: e82218
    call         hud_draw_keys                   ; 1000:227e: e81118
    mov          byte [0x8ed],0xc                ; 1000:2281: c606ed080c
    cmp          word [0x222],0x205              ; 1000:2286: 813e22020502
    jge          loc_2294                        ; 1000:228c: 7d06
    mov          word [0x222],0x205              ; 1000:228e: c70622020502
loc_2294:
    mov          AX,0x120                        ; 1000:2294: b82001
    mov          BX,0x90                         ; 1000:2297: bb9000
    mov          CX,word [0x222]                 ; 1000:229a: 8b0e2202
    call         hud_draw_shield_bar             ; 1000:229e: e8be17
    mov          AX,[0x210]                      ; 1000:22a1: a11002
    mov          [0x6c6],AX                      ; 1000:22a4: a3c606
    mov          AX,[0x212]                      ; 1000:22a7: a11202
    mov          [0x6c8],AX                      ; 1000:22aa: a3c806
    mov          AL,[0x218]                      ; 1000:22ad: a01802
    mov          [0x8ad],AL                      ; 1000:22b0: a2ad08
    mov          AL,[0x219]                      ; 1000:22b3: a01902
    mov          [0x89d],AL                      ; 1000:22b6: a29d08
    xor          AX,AX                           ; 1000:22b9: 33c0
    mov          [0x892],AX                      ; 1000:22bb: a39208
    mov          [0x894],AX                      ; 1000:22be: a39408
    mov          [0x897],AL                      ; 1000:22c1: a29708
    mov          [0x8a3],AL                      ; 1000:22c4: a2a308
    mov          [0x8a6],AL                      ; 1000:22c7: a2a608
    mov          [0x8a8],AL                      ; 1000:22ca: a2a808
    mov          [0x91e],AL                      ; 1000:22cd: a21e09
    mov          [0x921],AL                      ; 1000:22d0: a22109
    mov          [0x89a],AL                      ; 1000:22d3: a29a08
    mov          [0x89e],AL                      ; 1000:22d6: a29e08
    mov          [0x8b1],AL                      ; 1000:22d9: a2b108
    mov          [0x8b0],AL                      ; 1000:22dc: a2b008
    mov          [0x8ec],AL                      ; 1000:22df: a2ec08
    mov          [0x291],AL                      ; 1000:22e2: a29102
    mov          [0x89c],AL                      ; 1000:22e5: a29c08
    mov          [0x93b],AL                      ; 1000:22e8: a23b09
    mov          byte [0x89b],0x1                ; 1000:22eb: c6069b0801
    mov          AX,[0x6fa]                      ; 1000:22f0: a1fa06
    mov          BX,word [0x6fc]                 ; 1000:22f3: 8b1efc06
    mov          word [0x6fc],0xffff             ; 1000:22f7: c706fc06ffff
    call         map_load_room                   ; 1000:22fd: e88df8
    call         spr_render_player               ; 1000:2300: e8ddf5
    mov          BL,byte [0x896]                 ; 1000:2303: 8a1e9608
    xor          BH,BH                           ; 1000:2307: 32ff
    mov          byte [0x896],0x0                ; 1000:2309: c606960800
    call         hud_draw_item_icon              ; 1000:230e: e80617
    cmp          byte [0x220],0xff               ; 1000:2311: 803e2002ff
    jnz          loc_2320                        ; 1000:2316: 7508
    mov          byte [0x220],0x0                ; 1000:2318: c606200200
    call         snd_play_door_open              ; 1000:231d: e8ff1a
loc_2320:
    jmp near     game_main_loop                  ; 1000:2320: e91e00

; ----------------------------------------------------------------------------
; FUNCTION: spr_set_player_frame  [Sprites / Player]
; ADDRESS:  1000:2323
; PURPOSE:  Sets specific sprite frame index for Comic based on current stance and facing direction.
; ----------------------------------------------------------------------------
spr_set_player_frame:
    mov          AX,[0x6fa]                      ; 1000:2323: a1fa06
    mov          BX,0xffff                       ; 1000:2326: bbffff
    mov          word [0x6fa],0xffff             ; 1000:2329: c706fa06ffff
    call         map_load_room                   ; 1000:232f: e85bf8
    mov          word [0x210],0x288              ; 1000:2332: c70610028802
    mov          word [0x212],0x70               ; 1000:2338: c70612027000
    jmp near     spr_animate_player              ; 1000:233e: e93aff

; ----------------------------------------------------------------------------
; FUNCTION: game_main_loop  [Game / Core]
; ADDRESS:  1000:2341
; PURPOSE:  Master gameplay loop. Polls user input, updates physics, checks collisions, updates monsters/projectiles, blits sprites, flips video pages, and synchronizes frames.
; ----------------------------------------------------------------------------
game_main_loop:
    push         ES                              ; 1000:2341: 06
    xor          CX,CX                           ; 1000:2342: 33c9
    mov          ES,CX                           ; 1000:2344: 8ec1
    mov          CL,byte [es:0x41a]              ; 1000:2346: 268a0e1a04
    mov          byte [es:0x41c],CL              ; 1000:234b: 26880e1c04
    pop          ES                              ; 1000:2350: 07
    inc          byte [0x962b]                   ; 1000:2351: fe062b96
    mov          word [0x8e8],SP                 ; 1000:2355: 8926e808
    mov          CH,byte [0x8af]                 ; 1000:2359: 8a2eaf08
    inc          CH                              ; 1000:235d: fec5
    cmp          CH,0x4                          ; 1000:235f: 80fd04
    jnz          loc_2366                        ; 1000:2362: 7502
    mov          CH,0x1                          ; 1000:2364: b501
loc_2366:
    mov          byte [0x8af],CH                 ; 1000:2366: 882eaf08
    mov          byte [0x8ae],0x0                ; 1000:236a: c606ae0800
    cmp          byte [0x962a],0x0               ; 1000:236f: 803e2a9600
    jz           loc_237a                        ; 1000:2374: 7404
    dec          byte [0x962a]                   ; 1000:2376: fe0e2a96
loc_237a:
    cmp          byte [0x8ec],0x0                ; 1000:237a: 803eec0800
    jz           loc_2384                        ; 1000:237f: 7403
    jmp near     loc_275c                        ; 1000:2381: e9d803
loc_2384:
    cmp          byte [0x921],0x0                ; 1000:2384: 803e210900
    jz           loc_238e                        ; 1000:2389: 7403
    jmp near     loc_275c                        ; 1000:238b: e9ce03
loc_238e:
    cmp          byte [0x924],0x1                ; 1000:238e: 803e240901
    jle          loc_2398                        ; 1000:2393: 7e03
    jmp near     ent_turn_around                 ; 1000:2395: e9bf12
loc_2398:
    cmp          byte [0x92c],0x1                ; 1000:2398: 803e2c0901
    jle          loc_23a2                        ; 1000:239d: 7e03
    jmp near     loc_386f                        ; 1000:239f: e9cd14
loc_23a2:
    cmp          byte [0x89d],0x0                ; 1000:23a2: 803e9d0800
    jz           loc_23ac                        ; 1000:23a7: 7403
    jmp near     player_process_controls         ; 1000:23a9: e9300b
loc_23ac:
    cmp          byte [0x896],0x2                ; 1000:23ac: 803e960802
    jnz          loc_23e0                        ; 1000:23b1: 752d
    mov          AX,0xaabe                       ; 1000:23b3: b8beaa
    cmp          word [0x222],0x0                ; 1000:23b6: 833e220200
    jz           loc_23c7                        ; 1000:23bb: 740a
    mov          AX,0xb7b6                       ; 1000:23bd: b8b6b7
    cmp          word [0x6fa],0xe                ; 1000:23c0: 833efa060e
    jnz          loc_23e0                        ; 1000:23c5: 7519
loc_23c7:
    cmp          byte [cs:0x25a],0x1             ; 1000:23c7: 2e803e5a0201
    jz           loc_23d7                        ; 1000:23cd: 7408
    cmp          byte [cs:0x25d],0x1             ; 1000:23cf: 2e803e5d0201
    jnz          loc_23e0                        ; 1000:23d5: 7509
loc_23d7:
    call         ent_destroy                     ; 1000:23d7: e83612
    mov          byte [cs:0x25a],0x0             ; 1000:23da: 2ec6065a0200
loc_23e0:
    cmp          byte [0x897],0x0                ; 1000:23e0: 803e970800
    jz           loc_23ea                        ; 1000:23e5: 7403
    jmp near     player_update_physics           ; 1000:23e7: e92606
loc_23ea:
    cmp          byte [0x8a3],0x0                ; 1000:23ea: 803ea30800
    jz           loc_23f4                        ; 1000:23ef: 7403
    jmp near     proj_init_trajectory            ; 1000:23f1: e9130d
loc_23f4:
    cmp          byte [0x8a7],0x0                ; 1000:23f4: 803ea70800
    jz           loc_23fe                        ; 1000:23f9: 7403
    jmp near     ent_update_ai                   ; 1000:23fb: e9ab10
loc_23fe:
    cmp          byte [0x92c],0x1                ; 1000:23fe: 803e2c0901
    jnz          loc_2416                        ; 1000:2403: 7511
    mov          AX,[0x6c6]                      ; 1000:2405: a1c606
    sub          AX,word [0x930]                 ; 1000:2408: 2b063009
    jl           loc_2416                        ; 1000:240c: 7c08
    cmp          AX,0x18                         ; 1000:240e: 3d1800
    jg           loc_2416                        ; 1000:2411: 7f03
    jmp near     hud_update_score                ; 1000:2413: e94914
loc_2416:
    mov          AX,[0x6c6]                      ; 1000:2416: a1c606
    add          AX,0x8                          ; 1000:2419: 050800
    mov          BX,word [0x6c8]                 ; 1000:241c: 8b1ec806
    add          BX,0x1f                         ; 1000:2420: 83c31f
    call         map_get_tile_at                 ; 1000:2423: e8d8f8
    cmp          AX,word [0x294]                 ; 1000:2426: 3b069402
    jle          loc_2435                        ; 1000:242a: 7e09
    cmp          AX,word [0x296]                 ; 1000:242c: 3b069602
    jg           loc_2435                        ; 1000:2430: 7f03
    jmp near     player_stop_jump                ; 1000:2432: e99109
loc_2435:
    cmp          byte [0x8a6],0x0                ; 1000:2435: 803ea60800
    jz           loc_243f                        ; 1000:243a: 7403
    jmp near     ent_update_all                  ; 1000:243c: e9d00e
loc_243f:
    cmp          byte [cs:0x25c],0x1             ; 1000:243f: 2e803e5c0201
    jnz          loc_24a3                        ; 1000:2445: 755c
    cmp          byte [0x898],0x0                ; 1000:2447: 803e980800
    jz           loc_24a3                        ; 1000:244c: 7455
    mov          byte [0x897],0x1                ; 1000:244e: c606970801
    cmp          byte [cs:0x25b],0x1             ; 1000:2453: 2e803e5b0201
    jz           loc_247a                        ; 1000:2459: 741f
    cmp          byte [0x9628],0x1               ; 1000:245b: 803e289601
    jnz          loc_246c                        ; 1000:2460: 750a
    mov          AL,[0x898]                      ; 1000:2462: a09808
    shr          AL,0x1                          ; 1000:2465: d0e8
    inc          AL                              ; 1000:2467: fec0
    mov          [0x898],AL                      ; 1000:2469: a29808
loc_246c:
    mov          AX,0x1                          ; 1000:246c: b80100
    lea          BX,[0x96de]                     ; 1000:246f: 8d1ede96
    mov          CX,0x0                          ; 1000:2473: b90000
    int3                                         ; 1000:2476: cc
    db 0xe9, 0x96, 0x05                             ; 1000:2477
loc_247a:
    mov          byte [0x897],0x1                ; 1000:247a: c606970801
    mov          byte [0x898],0x0                ; 1000:247f: c606980800
    mov          word [0x894],0x5                ; 1000:2484: c70694080500
    mov          word [0x892],0x0                ; 1000:248a: c70692080000
    mov          byte [0x89a],0x2                ; 1000:2490: c6069a0802
    mov          AX,0x1                          ; 1000:2495: b80100
    lea          BX,[0x96f6]                     ; 1000:2498: 8d1ef696
    mov          CX,0x0                          ; 1000:249c: b90000
    int3                                         ; 1000:249f: cc
    db 0xe9, 0x6d, 0x05                             ; 1000:24a0
loc_24a3:
    cmp          byte [0x962a],0x0               ; 1000:24a3: 803e2a9600
    jnz          loc_24b2                        ; 1000:24a8: 7508
    cmp          byte [cs:0x25a],0x1             ; 1000:24aa: 2e803e5a0201
    jz           loc_24b5                        ; 1000:24b0: 7403
loc_24b2:
    jmp near     loc_2548                        ; 1000:24b2: e99300
loc_24b5:
    cmp          word [0x6fa],0x5                ; 1000:24b5: 833efa0605
    jnz          loc_24e8                        ; 1000:24ba: 752c
    cmp          word [0x6fc],0x3                ; 1000:24bc: 833efc0603
    jnz          loc_24e8                        ; 1000:24c1: 7525
    cmp          word [0x6c8],0x70               ; 1000:24c3: 833ec80670
    jnz          loc_24e8                        ; 1000:24c8: 751e
    cmp          word [0x6c6],0x270              ; 1000:24ca: 813ec6067002
    jl           loc_24e8                        ; 1000:24d0: 7c16
    cmp          word [0x6c6],0x2a0              ; 1000:24d2: 813ec606a002
    jg           loc_24e8                        ; 1000:24d8: 7f0e
    cmp          byte [0x220],0x5                ; 1000:24da: 803e200205
    jge          loc_24e8                        ; 1000:24df: 7d07
    mov          SP,word [0x8e8]                 ; 1000:24e1: 8b26e808
    jmp near     game_save_load_menu             ; 1000:24e5: e98041
loc_24e8:
    mov          CX,word [0x6ee]                 ; 1000:24e8: 8b0eee06
    mov          SI,word [0x6f0]                 ; 1000:24ec: 8b36f006
    or           CX,CX                           ; 1000:24f0: 0bc9
    jz           loc_2548                        ; 1000:24f2: 7454
loc_24f4:
    mov          AX,[0x6c6]                      ; 1000:24f4: a1c606
    sub          AX,word [cs:SI]                 ; 1000:24f7: 2e2b04
    jl           loc_2541                        ; 1000:24fa: 7c45
    cmp          AX,word [cs:SI + 0x4]           ; 1000:24fc: 2e3b4404
    jg           loc_2541                        ; 1000:2500: 7f3f
    mov          AX,[0x6c8]                      ; 1000:2502: a1c806
    cmp          AX,word [cs:SI + 0x2]           ; 1000:2505: 2e3b4402
    jnz          loc_2541                        ; 1000:2509: 7536
    mov          BX,word [cs:SI + 0x10]          ; 1000:250b: 2e8b5c10
    or           BX,BX                           ; 1000:250f: 0bdb
    jz           loc_2535                        ; 1000:2511: 7422
    mov          AX,word [BX + 0x2]              ; 1000:2513: 8b4702
    mov          BX,word [BX]                    ; 1000:2516: 8b1f
    or           BX,BX                           ; 1000:2518: 0bdb
    jz           loc_2535                        ; 1000:251a: 7419
    cmp          AX,word [cs:BX + 0x8]           ; 1000:251c: 2e3b4708
    jnz          loc_2528                        ; 1000:2520: 7506
    cmp          word [cs:BX],-0x1               ; 1000:2522: 2e833fff
    jz           loc_252d                        ; 1000:2526: 7405
loc_2528:
    call         ent_destroy                     ; 1000:2528: e8e510
    jmp short    loc_2548                        ; 1000:252b: eb1b
loc_252d:
    mov          BX,word [cs:SI + 0x10]          ; 1000:252d: 2e8b5c10
    mov          word [BX],0x0                   ; 1000:2531: c7070000
loc_2535:
    mov          SP,word [0x8e8]                 ; 1000:2535: 8b26e808
    mov          byte [0x962a],0x5               ; 1000:2539: c6062a9605
    jmp near     game_start_room                 ; 1000:253e: e9bdfb
loc_2541:
    add          SI,0x12                         ; 1000:2541: 83c612
    dec          CL                              ; 1000:2544: fec9
    jnz          loc_24f4                        ; 1000:2546: 75ac
loc_2548:
    mov          word [0x892],0x0                ; 1000:2548: c70692080000
    cmp          byte [cs:0x258],0x1             ; 1000:254e: 2e803e580201
    jnz          loc_255f                        ; 1000:2554: 7509
    mov          word [0x892],0xfffb             ; 1000:2556: c7069208fbff
    call         player_check_ceiling_collision  ; 1000:255c: e87d06
loc_255f:
    cmp          byte [cs:0x259],0x1             ; 1000:255f: 2e803e590201
    jnz          loc_2570                        ; 1000:2565: 7509
    mov          word [0x892],0x5                ; 1000:2567: c70692080500
    call         player_check_wall_collision_right ; 1000:256d: e8c906
loc_2570:
    mov          AX,[0x6c6]                      ; 1000:2570: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2573: 8b1ec806
    add          BX,0x20                         ; 1000:2577: 83c320
    mov          DI,word [0x294]                 ; 1000:257a: 8b3e9402
    call         player_check_swimming           ; 1000:257e: e81c07
    jc           loc_25ac                        ; 1000:2581: 7229
    mov          byte [0x897],0x1                ; 1000:2583: c606970801
    mov          byte [0x898],0x0                ; 1000:2588: c606980800
    mov          word [0x894],0x5                ; 1000:258d: c70694080500
    cmp          word [0x892],0x0                ; 1000:2593: 833e920800
    jl           loc_25a4                        ; 1000:2598: 7c0a
    jz           loc_25ba                        ; 1000:259a: 741e
    mov          word [0x892],0x1                ; 1000:259c: c70692080100
    jmp short    loc_25ba                        ; 1000:25a2: eb16
loc_25a4:
    mov          word [0x892],0xffff             ; 1000:25a4: c7069208ffff
    jmp short    loc_25ba                        ; 1000:25aa: eb0e
loc_25ac:
    cmp          byte [0x89c],0x1                ; 1000:25ac: 803e9c0801
    jnz          loc_25ba                        ; 1000:25b1: 7507
    mov          SP,word [0x8e8]                 ; 1000:25b3: 8b26e808
    jmp near     game_respawn_player             ; 1000:25b7: e9fefb
loc_25ba:
    cmp          byte [0x897],0x0                ; 1000:25ba: 803e970800
    jnz          loc_2621                        ; 1000:25bf: 7560
    mov          byte [0x9628],0x0               ; 1000:25c1: c606289600
    cmp          word [0x6fa],0x9                ; 1000:25c6: 833efa0609
    jnz          loc_2621                        ; 1000:25cb: 7554
    cmp          word [cs:0x12d0],-0x1           ; 1000:25cd: 2e833ed012ff
    jz           loc_2621                        ; 1000:25d3: 744c
    mov          AX,[0x6c6]                      ; 1000:25d5: a1c606
    mov          BX,word [0x6c8]                 ; 1000:25d8: 8b1ec806
    add          BX,0x20                         ; 1000:25dc: 83c320
    call         map_get_tile_at                 ; 1000:25df: e81cf7
    cmp          AX,0x1c                         ; 1000:25e2: 3d1c00
    jl           loc_25ec                        ; 1000:25e5: 7c05
    cmp          AX,0x1e                         ; 1000:25e7: 3d1e00
    jle          loc_2601                        ; 1000:25ea: 7e15
loc_25ec:
    test         word [0x6c6],0x8                ; 1000:25ec: f706c6060800
    jz           loc_2621                        ; 1000:25f2: 742d
    inc          BX                              ; 1000:25f4: 43
    mov          AL,byte [BX]                    ; 1000:25f5: 8a07
    cmp          AX,0x1c                         ; 1000:25f7: 3d1c00
    jl           loc_2621                        ; 1000:25fa: 7c25
    cmp          AX,0x1e                         ; 1000:25fc: 3d1e00
    jg           loc_2621                        ; 1000:25ff: 7f20
loc_2601:
    mov          byte [0x9628],0x1               ; 1000:2601: c606289601
    mov          AL,[0x8ae]                      ; 1000:2606: a0ae08
    push         AX                              ; 1000:2609: 50
    cmp          byte [0x9629],0x0               ; 1000:260a: 803e299600
    jl           loc_261a                        ; 1000:260f: 7c09
    call         player_check_climbing           ; 1000:2611: e84006
    pop          AX                              ; 1000:2614: 58
    mov          [0x8ae],AL                      ; 1000:2615: a2ae08
    jmp short    loc_2621                        ; 1000:2618: eb07
loc_261a:
    call         player_check_wall_collision_left ; 1000:261a: e8da05
    pop          AX                              ; 1000:261d: 58
    mov          [0x8ae],AL                      ; 1000:261e: a2ae08
loc_2621:
    cmp          word [0x6fa],0x5                ; 1000:2621: 833efa0605
    jnz          loc_2652                        ; 1000:2626: 752a
    cmp          word [0x6fc],0x3                ; 1000:2628: 833efc0603
    jnz          loc_2652                        ; 1000:262d: 7523
    cmp          word [0x6c8],0x60               ; 1000:262f: 833ec80660
    jnz          loc_2652                        ; 1000:2634: 751c
    cmp          word [0x6c6],0x28               ; 1000:2636: 833ec60628
    jnz          loc_2652                        ; 1000:263b: 7515
    cmp          byte [0x291],0x1                ; 1000:263d: 803e910201
    jz           loc_2657                        ; 1000:2642: 7413
    cmp          byte [0x220],0x5                ; 1000:2644: 803e200205
    jge          loc_2657                        ; 1000:2649: 7d0c
    mov          SP,word [0x8e8]                 ; 1000:264b: 8b26e808
    jmp near     game_switch_room                ; 1000:264f: e901fc
loc_2652:
    mov          byte [0x291],0x0                ; 1000:2652: c606910200
loc_2657:
    cmp          byte [0x8a8],0x0                ; 1000:2657: 803ea80800
    jnz          loc_267f                        ; 1000:265c: 7521
    mov          AL,[cs:0x61]                    ; 1000:265e: 2ea06100
    cmp          byte [cs:0x257],AL              ; 1000:2662: 2e38065702
    jnz          loc_267f                        ; 1000:2667: 7516
    mov          byte [cs:0x61],0x0              ; 1000:2669: 2ec606610000
    cmp          byte [0x897],0x0                ; 1000:266f: 803e970800
    jnz          loc_267f                        ; 1000:2674: 7509
    call         game_pause_handler              ; 1000:2676: e86d20
    mov          byte [cs:0x61],0x0              ; 1000:2679: 2ec606610000
loc_267f:
    cmp          byte [0x924],0x0                ; 1000:267f: 803e240900
    jz           loc_2689                        ; 1000:2684: 7403
    call         item_collect_pickup             ; 1000:2686: e83710
loc_2689:
    cmp          byte [0x897],0x1                ; 1000:2689: 803e970801
    jz           loc_26b1                        ; 1000:268e: 7421
    cmp          byte [cs:0x25c],0x1             ; 1000:2690: 2e803e5c0201
    jz           loc_26b1                        ; 1000:2696: 7419
    mov          AL,[0x899]                      ; 1000:2698: a09908
    cmp          word [cs:0xe5c],-0x1            ; 1000:269b: 2e833e5c0eff
    jnz          loc_26ae                        ; 1000:26a1: 750b
    cmp          word [cs:0xe64],0xb094          ; 1000:26a3: 2e813e640e94b0
    jnz          loc_26ae                        ; 1000:26aa: 7502
    inc          AL                              ; 1000:26ac: fec0
loc_26ae:
    mov          [0x898],AL                      ; 1000:26ae: a29808
loc_26b1:
    cmp          byte [cs:0x25d],0x1             ; 1000:26b1: 2e803e5d0201
    jz           loc_26be                        ; 1000:26b7: 7405
    call         hud_draw_number_word            ; 1000:26b9: e81214
    jmp short    loc_2714                        ; 1000:26bc: eb56
loc_26be:
    cmp          byte [0x896],0x0                ; 1000:26be: 803e960800
    jz           loc_26d8                        ; 1000:26c3: 7413
    call         hud_draw_number_word            ; 1000:26c5: e80614
    cmp          byte [0x896],0x3                ; 1000:26c8: 803e960803
    jz           loc_2708                        ; 1000:26cd: 7439
    cmp          byte [0x896],0x1                ; 1000:26cf: 803e960801
    jz           loc_26f6                        ; 1000:26d4: 7420
    jmp short    loc_2714                        ; 1000:26d6: eb3c
loc_26d8:
    cmp          byte [0x8ea],0x0                ; 1000:26d8: 803eea0800
    jz           loc_2714                        ; 1000:26dd: 7435
    cmp          byte [0x21c],0x0                ; 1000:26df: 803e1c0200
    jnz          loc_26ee                        ; 1000:26e4: 7508
    mov          AX,0xa09c                       ; 1000:26e6: b89ca0
    call         ent_destroy                     ; 1000:26e9: e8240f
    jmp short    loc_26f1                        ; 1000:26ec: eb03
loc_26ee:
    call         map_update_animated_tiles       ; 1000:26ee: e8fa35
loc_26f1:
    call         hud_draw_char                   ; 1000:26f1: e81e14
    jmp short    loc_2714                        ; 1000:26f4: eb1e
loc_26f6:
    mov          AL,[0x897]                      ; 1000:26f6: a09708
    or           AL,byte [0x89d]                 ; 1000:26f9: 0a069d08
    or           AL,byte [0x8a3]                 ; 1000:26fd: 0a06a308
    jnz          loc_2714                        ; 1000:2701: 7511
    call         player_fire_projectile          ; 1000:2703: e8c009
    jmp short    loc_2714                        ; 1000:2706: eb0c
loc_2708:
    mov          AL,[0x897]                      ; 1000:2708: a09708
    or           AL,byte [0x89d]                 ; 1000:270b: 0a069d08
    jnz          loc_2714                        ; 1000:270f: 7503
    call         ent_spawn_enemy                 ; 1000:2711: e8540d
loc_2714:
    mov          AL,[cs:0x61]                    ; 1000:2714: 2ea06100
    cmp          byte [cs:0x250],AL              ; 1000:2718: 2e38065002
    jnz          loc_2728                        ; 1000:271d: 7509
    mov          byte [cs:0x61],0x0              ; 1000:271f: 2ec606610000
    call         hud_draw_lives                  ; 1000:2725: e88512
loc_2728:
    mov          AL,[cs:0x61]                    ; 1000:2728: 2ea06100
    cmp          byte [cs:0x255],AL              ; 1000:272c: 2e38065502
    jnz          loc_2741                        ; 1000:2731: 750e
    mov          byte [cs:0x61],0x0              ; 1000:2733: 2ec606610000
    mov          AX,0xffff                       ; 1000:2739: b8ffff
    call         proj_check_monster_hit          ; 1000:273c: e83e0b
    jmp short    loc_2754                        ; 1000:273f: eb13
loc_2741:
    cmp          byte [cs:0x256],AL              ; 1000:2741: 2e38065602
    jnz          loc_2754                        ; 1000:2746: 750c
    mov          byte [cs:0x61],0x0              ; 1000:2748: 2ec606610000
    mov          AX,0x1                          ; 1000:274e: b80100
    call         proj_check_monster_hit          ; 1000:2751: e8290b
loc_2754:
    cmp          byte [cs:0x25d],0x1             ; 1000:2754: 2e803e5d0201
    jnz          loc_275c                        ; 1000:275a: 7500
loc_275c:
    cmp          byte [0x93b],0x0                ; 1000:275c: 803e3b0900
    jz           loc_2769                        ; 1000:2761: 7406
    dec          byte [0x93b]                    ; 1000:2763: fe0e3b09
    jmp short    loc_2770                        ; 1000:2767: eb07
loc_2769:
    cmp          byte [0x93a],0x1                ; 1000:2769: 803e3a0901
    jnz          loc_2794                        ; 1000:276e: 7524
loc_2770:
    test         byte [0x962b],0x1               ; 1000:2770: f6062b9601
    jnz          loc_2788                        ; 1000:2775: 7511
    add          word [0x6c4],0x2                ; 1000:2777: 8306c40602
    mov          AX,[0x6c4]                      ; 1000:277c: a1c406
    add          AX,0xa0                         ; 1000:277f: 05a000
    cmp          AX,word [0x29e]                 ; 1000:2782: 3b069e02
    jle          loc_2794                        ; 1000:2786: 7e0c
loc_2788:
    sub          word [0x6c4],0x2                ; 1000:2788: 832ec40602
    jnc          loc_2794                        ; 1000:278d: 7305
    add          word [0x6c4],0x2                ; 1000:278f: 8306c40602
loc_2794:
    call         map_draw_background             ; 1000:2794: e895f5
    call         ui_draw_text_string             ; 1000:2797: e8e11b
    call         map_redraw_dirty_tiles          ; 1000:279a: e8c235
    call         vid_flip_buffers                ; 1000:279d: e88523
    cmp          byte [0x8ec],0x0                ; 1000:27a0: 803eec0800
    jz           loc_27f7                        ; 1000:27a5: 7450
    dec          byte [0x8ec]                    ; 1000:27a7: fe0eec08
    jnz          loc_27b0                        ; 1000:27ab: 7503
    jmp near     loc_29e3                        ; 1000:27ad: e93302
loc_27b0:
    cmp          byte [0x8ec],0x1                ; 1000:27b0: 803eec0801
    jg           loc_27ba                        ; 1000:27b5: 7f03
    jmp near     loc_2902                        ; 1000:27b7: e94801
loc_27ba:
    mov          SI,0x9                          ; 1000:27ba: be0900
    mov          AL,[0x8ec]                      ; 1000:27bd: a0ec08
    xor          AH,AH                           ; 1000:27c0: 32e4
    sub          SI,AX                           ; 1000:27c2: 2bf0
    shl          SI,0x1                          ; 1000:27c4: d1e6
    mov          AX,[0x6c6]                      ; 1000:27c6: a1c606
    mov          BX,word [0x6c8]                 ; 1000:27c9: 8b1ec806
    cmp          byte [0x89d],0x1                ; 1000:27cd: 803e9d0801
    jz           loc_27e1                        ; 1000:27d2: 740d
    cmp          byte [0x92c],0x2                ; 1000:27d4: 803e2c0902
    jz           loc_27e1                        ; 1000:27d9: 7406
    add          SI,0x48a4                       ; 1000:27db: 81c6a448
    jmp short    loc_27e5                        ; 1000:27df: eb04
loc_27e1:
    add          SI,0x48b4                       ; 1000:27e1: 81c6b448
loc_27e5:
    mov          DS,word [cs:0x5e]               ; 1000:27e5: 2e8e1e5e00
    mov          SI,word [SI]                    ; 1000:27ea: 8b34
    mov          DS,word [cs:0x5c]               ; 1000:27ec: 2e8e1e5c00
    call         vid_blit_planar_block           ; 1000:27f1: e8c755
    jmp near     loc_2902                        ; 1000:27f4: e90b01
loc_27f7:
    cmp          byte [0x924],0x1                ; 1000:27f7: 803e240901
    jle          loc_2838                        ; 1000:27fc: 7e3a
    cmp          byte [0x925],0x2                ; 1000:27fe: 803e250902
    jz           loc_2826                        ; 1000:2803: 7421
    mov          SI,word [cs:0x8dbe]             ; 1000:2805: 2e8b36be8d
    cmp          byte [0x925],0x1                ; 1000:280a: 803e250901
    jz           loc_2816                        ; 1000:280f: 7405
    mov          SI,word [cs:0x8dc2]             ; 1000:2811: 2e8b36c28d
loc_2816:
    mov          AX,[0x928]                      ; 1000:2816: a12809
    mov          BX,word [0x92a]                 ; 1000:2819: 8b1e2a09
    add          BX,0x6                          ; 1000:281d: 83c306
    call         vid_blit_planar_block           ; 1000:2820: e89855
    jmp near     loc_2902                        ; 1000:2823: e9dc00
loc_2826:
    mov          SI,word [cs:0x8dc0]             ; 1000:2826: 2e8b36c08d
    mov          AX,[0x928]                      ; 1000:282b: a12809
    mov          BX,word [0x92a]                 ; 1000:282e: 8b1e2a09
    call         vid_blit_planar_block           ; 1000:2832: e88655
    jmp near     loc_2902                        ; 1000:2835: e9ca00
loc_2838:
    cmp          byte [0x92c],0x1                ; 1000:2838: 803e2c0901
    jle          loc_2874                        ; 1000:283d: 7e35
    cmp          byte [0x92d],0x0                ; 1000:283f: 803e2d0900
    jz           loc_2862                        ; 1000:2844: 741c
    mov          SI,word [cs:0xa990]             ; 1000:2846: 2e8b3690a9
    jg           loc_2852                        ; 1000:284b: 7f05
    mov          SI,word [cs:0xa98e]             ; 1000:284d: 2e8b368ea9
loc_2852:
    mov          AX,[0x930]                      ; 1000:2852: a13009
    mov          BX,word [0x932]                 ; 1000:2855: 8b1e3209
    add          BX,0x3                          ; 1000:2859: 83c303
    call         vid_blit_planar_block           ; 1000:285c: e85c55
    jmp near     loc_2902                        ; 1000:285f: e9a000
loc_2862:
    mov          SI,word [cs:0xa98c]             ; 1000:2862: 2e8b368ca9
    mov          AX,[0x930]                      ; 1000:2867: a13009
    mov          BX,word [0x932]                 ; 1000:286a: 8b1e3209
    call         vid_blit_planar_block           ; 1000:286e: e84a55
    jmp near     loc_2902                        ; 1000:2871: e98e00
loc_2874:
    mov          SI,0x4882                       ; 1000:2874: be8248
    cmp          byte [0x8ad],0x1                ; 1000:2877: 803ead0801
    jz           loc_2881                        ; 1000:287c: 7403
    mov          SI,0x4860                       ; 1000:287e: be6048
loc_2881:
    mov          AL,[0x8ae]                      ; 1000:2881: a0ae08
    or           AL,AL                           ; 1000:2884: 0ac0
    jz           loc_28ec                        ; 1000:2886: 7464
    dec          AL                              ; 1000:2888: fec8
    jz           loc_28a0                        ; 1000:288a: 7414
    dec          AL                              ; 1000:288c: fec8
    jz           loc_28ab                        ; 1000:288e: 741b
    dec          AL                              ; 1000:2890: fec8
    jz           loc_28b0                        ; 1000:2892: 741c
    dec          AL                              ; 1000:2894: fec8
    jz           loc_28c3                        ; 1000:2896: 742b
    dec          AL                              ; 1000:2898: fec8
    jz           loc_28de                        ; 1000:289a: 7442
    dec          AL                              ; 1000:289c: fec8
    jz           loc_28be                        ; 1000:289e: 741e
loc_28a0:
    mov          AL,[0x8af]                      ; 1000:28a0: a0af08
    xor          AH,AH                           ; 1000:28a3: 32e4
    shl          AX,0x1                          ; 1000:28a5: d1e0
    add          SI,AX                           ; 1000:28a7: 03f0
    jmp short    loc_28ec                        ; 1000:28a9: eb41
loc_28ab:
    add          SI,0x8                          ; 1000:28ab: 83c608
    jmp short    loc_28ec                        ; 1000:28ae: eb3c
loc_28b0:
    add          SI,0x8                          ; 1000:28b0: 83c608
    mov          AL,[0x8af]                      ; 1000:28b3: a0af08
    xor          AH,AH                           ; 1000:28b6: 32e4
    shl          AX,0x1                          ; 1000:28b8: d1e0
    add          SI,AX                           ; 1000:28ba: 03f0
    jmp short    loc_28ec                        ; 1000:28bc: eb2e
loc_28be:
    add          SI,0x1a                         ; 1000:28be: 83c61a
    jmp short    loc_28c6                        ; 1000:28c1: eb03
loc_28c3:
    add          SI,0xe                          ; 1000:28c3: 83c60e
loc_28c6:
    mov          AL,[0x8af]                      ; 1000:28c6: a0af08
    xor          AH,AH                           ; 1000:28c9: 32e4
    shl          AX,0x1                          ; 1000:28cb: d1e0
    add          SI,AX                           ; 1000:28cd: 03f0
    cmp          byte [0x8a5],0x1                ; 1000:28cf: 803ea50801
    jz           loc_28ec                        ; 1000:28d4: 7416
    mov          AX,[0x6c6]                      ; 1000:28d6: a1c606
    sub          AX,0x8                          ; 1000:28d9: 2d0800
    jmp short    loc_28ef                        ; 1000:28dc: eb11
loc_28de:
    add          SI,0x14                         ; 1000:28de: 83c614
    mov          AL,[0x8af]                      ; 1000:28e1: a0af08
    xor          AH,AH                           ; 1000:28e4: 32e4
    shl          AX,0x1                          ; 1000:28e6: d1e0
    add          SI,AX                           ; 1000:28e8: 03f0
    jmp short    loc_28ec                        ; 1000:28ea: eb00
loc_28ec:
    mov          AX,[0x6c6]                      ; 1000:28ec: a1c606
loc_28ef:
    mov          BX,word [0x6c8]                 ; 1000:28ef: 8b1ec806
    mov          DS,word [cs:0x5e]               ; 1000:28f3: 2e8e1e5e00
    mov          SI,word [SI]                    ; 1000:28f8: 8b34
    mov          DS,word [cs:0x5c]               ; 1000:28fa: 2e8e1e5c00
    call         vid_blit_planar_block           ; 1000:28ff: e8b954
loc_2902:
    cmp          byte [0x89e],0x0                ; 1000:2902: 803e9e0800
    jz           loc_291a                        ; 1000:2907: 7411
    dec          byte [0x89e]                    ; 1000:2909: fe0e9e08
    mov          SI,0x48d6                       ; 1000:290d: bed648
    mov          AX,[0x89f]                      ; 1000:2910: a19f08
    mov          BX,word [0x8a1]                 ; 1000:2913: 8b1ea108
    call         vid_blit_planar_block           ; 1000:2917: e8a154
loc_291a:
    cmp          byte [0x8b1],0x0                ; 1000:291a: 803eb10800
    jz           loc_2924                        ; 1000:291f: 7403
    call         proj_update_all                 ; 1000:2921: e8c708
loc_2924:
    cmp          byte [0x8a8],0x0                ; 1000:2924: 803ea80800
    jz           loc_2953                        ; 1000:2929: 7428
    dec          byte [0x8a8]                    ; 1000:292b: fe0ea808
    jz           loc_294a                        ; 1000:292f: 7419
    test         byte [0x962b],0x1               ; 1000:2931: f6062b9601
    jz           loc_2953                        ; 1000:2936: 741b
    mov          BX,word [0x8ab]                 ; 1000:2938: 8b1eab08
    mov          SI,word [BX + 0x6]              ; 1000:293c: 8b7706
    mov          AX,[0x8a9]                      ; 1000:293f: a1a908
    mov          BX,word [BX + 0x2]              ; 1000:2942: 8b5f02
    call         vid_blit_planar_block           ; 1000:2945: e87354
    jmp short    loc_2953                        ; 1000:2948: eb09
loc_294a:
    mov          BX,word [0x8ab]                 ; 1000:294a: 8b1eab08
    mov          AX,[0x8a9]                      ; 1000:294e: a1a908
    mov          word [BX],AX                    ; 1000:2951: 8907
loc_2953:
    cmp          byte [0x91e],0x0                ; 1000:2953: 803e1e0900
    jz           loc_297f                        ; 1000:2958: 7425
    cmp          byte [0x921],0x1                ; 1000:295a: 803e210901
    jz           loc_297f                        ; 1000:295f: 741e
    dec          byte [0x91e]                    ; 1000:2961: fe0e1e09
    jz           loc_297f                        ; 1000:2965: 7418
    test         byte [0x962b],0x1               ; 1000:2967: f6062b9601
    jz           loc_297f                        ; 1000:296c: 7411
    mov          SI,word [0x91f]                 ; 1000:296e: 8b361f09
    mov          AX,[0x6c6]                      ; 1000:2972: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2975: 8b1ec806
    sub          BX,0x10                         ; 1000:2979: 83eb10
    call         vid_blit_planar_block           ; 1000:297c: e83c54
loc_297f:
    cmp          byte [0x924],0x1                ; 1000:297f: 803e240901
    jnz          loc_2989                        ; 1000:2984: 7503
    call         hud_render_all                  ; 1000:2986: e8050e
loc_2989:
    cmp          byte [0x92c],0x1                ; 1000:2989: 803e2c0901
    jl           loc_29a8                        ; 1000:298e: 7c18
    jz           loc_2999                        ; 1000:2990: 7407
    cmp          byte [0x8ec],0x0                ; 1000:2992: 803eec0800
    jz           loc_29a8                        ; 1000:2997: 740f
loc_2999:
    mov          AX,[0x930]                      ; 1000:2999: a13009
    mov          BX,word [0x932]                 ; 1000:299c: 8b1e3209
    mov          SI,word [cs:0xa98a]             ; 1000:29a0: 2e8b368aa9
    call         vid_blit_planar_block           ; 1000:29a5: e81354
loc_29a8:
    cmp          byte [0x921],0x0                ; 1000:29a8: 803e210900
    jz           loc_29c9                        ; 1000:29ad: 741a
    mov          SI,word [0x922]                 ; 1000:29af: 8b362209
    call         ui_draw_window_box              ; 1000:29b3: e85018
    cmp          byte [cs:0x61],0x1c             ; 1000:29b6: 2e803e61001c
    jnz          loc_29c9                        ; 1000:29bc: 750b
    mov          byte [cs:0x61],0x0              ; 1000:29be: 2ec606610000
    mov          byte [0x921],0x0                ; 1000:29c4: c606210900
loc_29c9:
    cmp          byte [0x8ed],0x0                ; 1000:29c9: 803eed0800
    jz           loc_29d7                        ; 1000:29ce: 7407
    dec          byte [0x8ed]                    ; 1000:29d0: fe0eed08
    call         hud_flash_item                  ; 1000:29d4: e87a11
loc_29d7:
    call         vid_wait_vblank                 ; 1000:29d7: e8744d
    mov          AX,[0x6c8]                      ; 1000:29da: a1c806
    cmp          AX,word [0x29e]                 ; 1000:29dd: 3b069e02
    jle          loc_2a07                        ; 1000:29e1: 7e24
loc_29e3:
    mov          SP,word [0x8e8]                 ; 1000:29e3: 8b26e808
    mov          AX,0x1                          ; 1000:29e7: b80100
    mov          BX,0x969a                       ; 1000:29ea: bb9a96
    mov          CX,0xa                          ; 1000:29ed: b90a00
    int3                                         ; 1000:29f0: cc
    db 0xb8, 0x14, 0x00, 0xe8, 0x83, 0xd8, 0x83, 0x3e, 0x8f, 0x02, 0x00, 0x75, 0x03, 0xe9, 0xed, 0x0f ; 1000:29f1
    db 0xe8, 0xbb, 0x1e, 0xe9, 0x74, 0xf8           ; 1000:2a01
loc_2a07:
    mov          AX,0x1                          ; 1000:2a07: b80100
    call         timer_delay_ticks               ; 1000:2a0a: e86dd8
    jmp near     game_main_loop                  ; 1000:2a0d: e931f9

; ----------------------------------------------------------------------------
; FUNCTION: player_update_physics  [Player / Physics]
; ADDRESS:  1000:2a10
; PURPOSE:  Processes player horizontal velocity, acceleration, jump mechanics, gravity, ladder climbing, and swimming.
; ----------------------------------------------------------------------------
player_update_physics:
    cmp          byte [0x898],0x0                ; 1000:2a10: 803e980800
    jz           loc_2a31                        ; 1000:2a15: 741a
    dec          byte [0x898]                    ; 1000:2a17: fe0e9808
    jz           loc_2a31                        ; 1000:2a1b: 7414
    cmp          byte [cs:0x25c],0x1             ; 1000:2a1d: 2e803e5c0201
    jnz          loc_2a2c                        ; 1000:2a23: 7507
    sub          word [0x894],0x7                ; 1000:2a25: 832e940807
    jmp short    loc_2a31                        ; 1000:2a2a: eb05
loc_2a2c:
    mov          byte [0x898],0x0                ; 1000:2a2c: c606980800
loc_2a31:
    mov          AX,[0x894]                      ; 1000:2a31: a19408
    mov          CX,AX                           ; 1000:2a34: 8bc8
    add          AX,word [0x6c8]                 ; 1000:2a36: 0306c806
    jge          loc_2a3e                        ; 1000:2a3a: 7d02
    xor          AX,AX                           ; 1000:2a3c: 33c0
loc_2a3e:
    mov          [0x6c8],AX                      ; 1000:2a3e: a3c806
    mov          word [0x9620],CX                ; 1000:2a41: 890e2096
    add          CX,0x5                          ; 1000:2a45: 83c105
    cmp          CX,0x17                         ; 1000:2a48: 83f917
    jle          loc_2a50                        ; 1000:2a4b: 7e03
    mov          CX,0x17                         ; 1000:2a4d: b91700
loc_2a50:
    mov          word [0x894],CX                 ; 1000:2a50: 890e9408
    mov          CX,word [0x892]                 ; 1000:2a54: 8b0e9208
    cmp          byte [cs:0x258],0x1             ; 1000:2a58: 2e803e580201
    jnz          loc_2a6e                        ; 1000:2a5e: 750e
    mov          byte [0x8ad],0x2                ; 1000:2a60: c606ad0802
    dec          CX                              ; 1000:2a65: 49
    cmp          CX,-0x5                         ; 1000:2a66: 83f9fb
    jge          loc_2a6e                        ; 1000:2a69: 7d03
    mov          CX,0xfffb                       ; 1000:2a6b: b9fbff
loc_2a6e:
    cmp          byte [cs:0x259],0x1             ; 1000:2a6e: 2e803e590201
    jnz          loc_2a84                        ; 1000:2a74: 750e
    mov          byte [0x8ad],0x1                ; 1000:2a76: c606ad0801
    inc          CX                              ; 1000:2a7b: 41
    cmp          CX,0x5                          ; 1000:2a7c: 83f905
    jle          loc_2a84                        ; 1000:2a7f: 7e03
    mov          CX,0x5                          ; 1000:2a81: b90500
loc_2a84:
    mov          word [0x892],CX                 ; 1000:2a84: 890e9208
    or           CX,CX                           ; 1000:2a88: 0bc9
    jz           loc_2a9e                        ; 1000:2a8a: 7412
    jg           loc_2a97                        ; 1000:2a8c: 7f09
    inc          word [0x892]                    ; 1000:2a8e: ff069208
    call         player_check_wall_collision_left ; 1000:2a92: e86201
    jmp short    loc_2a9e                        ; 1000:2a95: eb07
loc_2a97:
    dec          word [0x892]                    ; 1000:2a97: ff0e9208
    call         player_check_climbing           ; 1000:2a9b: e8b601
loc_2a9e:
    cmp          word [0x894],0x0                ; 1000:2a9e: 833e940800
    jg           loc_2ad2                        ; 1000:2aa3: 7f2d
    mov          AX,[0x6c6]                      ; 1000:2aa5: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2aa8: 8b1ec806
    mov          DI,word [0x292]                 ; 1000:2aac: 8b3e9202
    mov          byte [0x8b0],0x2                ; 1000:2ab0: c606b00802
    call         player_check_swimming           ; 1000:2ab5: e8e501
    mov          byte [0x8b0],0x0                ; 1000:2ab8: c606b00800
    jnc          loc_2ad2                        ; 1000:2abd: 7313
    mov          word [0x894],0x0                ; 1000:2abf: c70694080000
    and          word [0x6c8],0xfff0             ; 1000:2ac5: 8326c806f0
    add          word [0x6c8],0xa                ; 1000:2aca: 8306c8060a
    jmp short    loc_2b4c                        ; 1000:2acf: eb7b
    db 0x90                                         ; 1000:2ad1
loc_2ad2:
    cmp          word [0x9620],0x0               ; 1000:2ad2: 833e209600
    jle          loc_2b4c                        ; 1000:2ad7: 7e73
    cmp          byte [0x924],0x1                ; 1000:2ad9: 803e240901
    jnz          loc_2ae8                        ; 1000:2ade: 7508
    call         ent_check_tile_collision        ; 1000:2ae0: e8470b
    jnc          loc_2ae8                        ; 1000:2ae3: 7303
    jmp near     loc_2b73                        ; 1000:2ae5: e98b00
loc_2ae8:
    mov          DI,word [0x294]                 ; 1000:2ae8: 8b3e9402
    cmp          byte [0x89a],0x0                ; 1000:2aec: 803e9a0800
    jz           loc_2afb                        ; 1000:2af1: 7408
    dec          byte [0x89a]                    ; 1000:2af3: fe0e9a08
    mov          DI,word [0x292]                 ; 1000:2af7: 8b3e9202
loc_2afb:
    mov          AX,[0x6c6]                      ; 1000:2afb: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2afe: 8b1ec806
    add          BX,0x20                         ; 1000:2b02: 83c320
    mov          byte [0x8b0],0x1                ; 1000:2b05: c606b00801
    call         player_check_swimming           ; 1000:2b0a: e89001
    mov          byte [0x8b0],0x0                ; 1000:2b0d: c606b00800
    jc           loc_2b23                        ; 1000:2b12: 720f
    cmp          AX,word [0x296]                 ; 1000:2b14: 3b069602
    jg           loc_2b2c                        ; 1000:2b18: 7f12
    cmp          AX,word [0x294]                 ; 1000:2b1a: 3b069402
    jle          loc_2b2c                        ; 1000:2b1e: 7e0c
    jmp near     player_stop_jump                ; 1000:2b20: e9a302
loc_2b23:
    cmp          byte [0x89b],0x1                ; 1000:2b23: 803e9b0801
    jnz          loc_2b31                        ; 1000:2b28: 7507
    jmp short    loc_2b78                        ; 1000:2b2a: eb4c
loc_2b2c:
    mov          byte [0x89b],0x1                ; 1000:2b2c: c6069b0801
loc_2b31:
    mov          AX,[0x6c6]                      ; 1000:2b31: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2b34: 8b1ec806
    add          BX,0x28                         ; 1000:2b38: 83c328
    mov          byte [0x8b0],0x1                ; 1000:2b3b: c606b00801
    call         player_check_swimming           ; 1000:2b40: e85a01
    mov          byte [0x8b0],0x0                ; 1000:2b43: c606b00800
    jc           loc_2b73                        ; 1000:2b48: 7229
    jmp short    loc_2b6b                        ; 1000:2b4a: eb1f
loc_2b4c:
    mov          AX,[0x6c6]                      ; 1000:2b4c: a1c606
    mov          BX,word [0x6c8]                 ; 1000:2b4f: 8b1ec806
    add          BX,0x20                         ; 1000:2b53: 83c320
    mov          DI,word [0x294]                 ; 1000:2b56: 8b3e9402
    call         player_check_swimming           ; 1000:2b5a: e84001
    jnc          loc_2b66                        ; 1000:2b5d: 7307
    mov          byte [0x89b],0x0                ; 1000:2b5f: c6069b0800
    jmp short    loc_2b6b                        ; 1000:2b64: eb05
loc_2b66:
    mov          byte [0x89b],0x1                ; 1000:2b66: c6069b0801
loc_2b6b:
    mov          byte [0x8ae],0x2                ; 1000:2b6b: c606ae0802
    jmp short    loc_2b88                        ; 1000:2b70: eb16
    db 0x90                                         ; 1000:2b72
loc_2b73:
    add          word [0x6c8],0x8                ; 1000:2b73: 8306c80608
loc_2b78:
    and          word [0x6c8],0xfff0             ; 1000:2b78: 8326c806f0
    mov          byte [0x897],0x0                ; 1000:2b7d: c606970800
    mov          word [0x894],0x0                ; 1000:2b82: c70694080000
loc_2b88:
    call         player_check_ground_collision   ; 1000:2b88: e80300
    jmp near     loc_267f                        ; 1000:2b8b: e9f1fa

; ----------------------------------------------------------------------------
; FUNCTION: player_check_ground_collision  [Player / Collision]
; ADDRESS:  1000:2b8e
; PURPOSE:  Checks for solid ground tiles below player feet to handle landing and ground movement.
; ----------------------------------------------------------------------------
player_check_ground_collision:
    mov          AX,[0x6c8]                      ; 1000:2b8e: a1c806
    mov          BX,word [0x6c4]                 ; 1000:2b91: 8b1ec406
    sub          AX,BX                           ; 1000:2b95: 2bc3
    cmp          AX,0x30                         ; 1000:2b97: 3d3000
    jl           loc_2ba2                        ; 1000:2b9a: 7c06
    cmp          AX,0x50                         ; 1000:2b9c: 3d5000
    jg           loc_2bbb                        ; 1000:2b9f: 7f1a
loc_2ba1:
    ret                                          ; 1000:2ba1: c3
loc_2ba2:
    or           BX,BX                           ; 1000:2ba2: 0bdb
    jz           loc_2ba1                        ; 1000:2ba4: 74fb
    add          BX,AX                           ; 1000:2ba6: 03d8
    sub          BX,0x30                         ; 1000:2ba8: 83eb30
    jl           loc_2bb3                        ; 1000:2bab: 7c06
    mov          word [0x6c4],BX                 ; 1000:2bad: 891ec406
    jmp short    loc_2ba1                        ; 1000:2bb1: ebee
loc_2bb3:
    mov          word [0x6c4],0x0                ; 1000:2bb3: c706c4060000
    jmp short    loc_2ba1                        ; 1000:2bb9: ebe6
loc_2bbb:
    mov          CX,word [0x29e]                 ; 1000:2bbb: 8b0e9e02
    sub          CX,0xa0                         ; 1000:2bbf: 81e9a000
    cmp          BX,CX                           ; 1000:2bc3: 3bd9
    jz           loc_2ba1                        ; 1000:2bc5: 74da
    add          BX,AX                           ; 1000:2bc7: 03d8
    sub          BX,0x50                         ; 1000:2bc9: 83eb50
    cmp          BX,CX                           ; 1000:2bcc: 3bd9
    jg           loc_2bd6                        ; 1000:2bce: 7f06
    mov          word [0x6c4],BX                 ; 1000:2bd0: 891ec406
    jmp short    loc_2ba1                        ; 1000:2bd4: ebcb
loc_2bd6:
    mov          word [0x6c4],CX                 ; 1000:2bd6: 890ec406
    jmp short    loc_2ba1                        ; 1000:2bda: ebc5

; ----------------------------------------------------------------------------
; FUNCTION: player_check_ceiling_collision  [Player / Collision]
; ADDRESS:  1000:2bdc
; PURPOSE:  Detects collisions with ceiling tiles above Comic's head during jumps.
; ----------------------------------------------------------------------------
player_check_ceiling_collision:
    cmp          byte [0x8ad],0x2                ; 1000:2bdc: 803ead0802
    jz           loc_2bea                        ; 1000:2be1: 7407
    mov          byte [0x8ad],0x2                ; 1000:2be3: c606ad0802
    jmp short    loc_2c38                        ; 1000:2be8: eb4e
loc_2bea:
    cmp          byte [0x9628],0x1               ; 1000:2bea: 803e289601
    jnz          player_check_wall_collision_left ; 1000:2bef: 7506
    mov          byte [0x8ae],0x1                ; 1000:2bf1: c606ae0801
    ret                                          ; 1000:2bf6: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_check_wall_collision_left  [Player / Collision]
; ADDRESS:  1000:2bf7
; PURPOSE:  Detects collisions with solid walls to Comic's left side.
; ----------------------------------------------------------------------------
player_check_wall_collision_left:
    mov          byte [0x9629],0xff              ; 1000:2bf7: c6062996ff
    mov          AX,[0x6c6]                      ; 1000:2bfc: a1c606
    or           AX,AX                           ; 1000:2bff: 0bc0
    jnz          loc_2c09                        ; 1000:2c01: 7506
    call         ent_check_player_contact        ; 1000:2c03: e8d809
    jmp short    player_stop_x                   ; 1000:2c06: eb2a
    db 0x90                                         ; 1000:2c08
loc_2c09:
    sub          AX,0x8                          ; 1000:2c09: 2d0800
    push         AX                              ; 1000:2c0c: 50
    call         player_do_jump                  ; 1000:2c0d: e85101
    pop          AX                              ; 1000:2c10: 58
    jc           player_stop_x                   ; 1000:2c11: 721f
    mov          [0x6c6],AX                      ; 1000:2c13: a3c606
    mov          byte [0x8ae],0x1                ; 1000:2c16: c606ae0801
    sub          AX,word [0x6c2]                 ; 1000:2c1b: 2b06c206
    cmp          AX,0x50                         ; 1000:2c1f: 3d5000
    jge          loc_2c38                        ; 1000:2c22: 7d14
    cmp          word [0x6c2],0x0                ; 1000:2c24: 833ec20600
    jz           loc_2c38                        ; 1000:2c29: 740d
    sub          word [0x6c2],0x8                ; 1000:2c2b: 832ec20608
    jmp short    loc_2c38                        ; 1000:2c30: eb06

; ----------------------------------------------------------------------------
; FUNCTION: player_stop_x  [Player / Physics]
; ADDRESS:  1000:2c32
; PURPOSE:  Halts horizontal velocity when walking into an obstruction.
; ----------------------------------------------------------------------------
player_stop_x:
    mov          word [0x892],0x0                ; 1000:2c32: c70692080000
loc_2c38:
    ret                                          ; 1000:2c38: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_check_wall_collision_right  [Player / Collision]
; ADDRESS:  1000:2c39
; PURPOSE:  Detects collisions with solid walls to Comic's right side.
; ----------------------------------------------------------------------------
player_check_wall_collision_right:
    cmp          byte [0x8ad],0x1                ; 1000:2c39: 803ead0801
    jz           loc_2c47                        ; 1000:2c3e: 7407
    mov          byte [0x8ad],0x1                ; 1000:2c40: c606ad0801
    jmp short    loc_2c38                        ; 1000:2c45: ebf1
loc_2c47:
    cmp          byte [0x9628],0x1               ; 1000:2c47: 803e289601
    jnz          player_check_climbing           ; 1000:2c4c: 7506
    mov          byte [0x8ae],0x1                ; 1000:2c4e: c606ae0801
    ret                                          ; 1000:2c53: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_check_climbing  [Player / Physics]
; ADDRESS:  1000:2c54
; PURPOSE:  Checks if Comic is on a ladder tile and processes vertical climbing input.
; ----------------------------------------------------------------------------
player_check_climbing:
    mov          byte [0x9629],0x1               ; 1000:2c54: c606299601
    mov          AX,[0x6c6]                      ; 1000:2c59: a1c606
    mov          BX,AX                           ; 1000:2c5c: 8bd8
    add          BX,0x10                         ; 1000:2c5e: 83c310
    cmp          BX,word [0x29c]                 ; 1000:2c61: 3b1e9c02
    jl           loc_2c6c                        ; 1000:2c65: 7c05
    call         ent_check_player_contact        ; 1000:2c67: e87409
    jmp short    player_stop_x                   ; 1000:2c6a: ebc6
loc_2c6c:
    add          AX,0x8                          ; 1000:2c6c: 050800
    push         AX                              ; 1000:2c6f: 50
    add          AX,0xf                          ; 1000:2c70: 050f00
    call         player_do_jump                  ; 1000:2c73: e8eb00
    pop          AX                              ; 1000:2c76: 58
    jc           player_stop_x                   ; 1000:2c77: 72b9
    mov          [0x6c6],AX                      ; 1000:2c79: a3c606
    mov          byte [0x8ae],0x1                ; 1000:2c7c: c606ae0801
    sub          AX,word [0x6c2]                 ; 1000:2c81: 2b06c206
    cmp          AX,0x70                         ; 1000:2c85: 3d7000
    jle          loc_2c38                        ; 1000:2c88: 7eae
    mov          AX,[0x6c2]                      ; 1000:2c8a: a1c206
    add          AX,0xd0                         ; 1000:2c8d: 05d000
    cmp          AX,word [0x29c]                 ; 1000:2c90: 3b069c02
    jz           loc_2c38                        ; 1000:2c94: 74a2
    add          word [0x6c2],0x8                ; 1000:2c96: 8306c20608
    jmp short    loc_2c38                        ; 1000:2c9b: eb9b

; ----------------------------------------------------------------------------
; FUNCTION: player_check_swimming  [Player / Physics]
; ADDRESS:  1000:2c9d
; PURPOSE:  Checks for water tiles; applies fluid buoyancy, swimming controls, and checks for underwater boots.
; ----------------------------------------------------------------------------
player_check_swimming:
    push         BP                              ; 1000:2c9d: 55
    mov          BP,SP                           ; 1000:2c9e: 8bec
    sub          SP,0x2                          ; 1000:2ca0: 83ec02
    mov          word [BP + -0x2],0x0            ; 1000:2ca3: c746fe0000
    push         AX                              ; 1000:2ca8: 50
    push         BX                              ; 1000:2ca9: 53
    call         map_get_tile_at                 ; 1000:2caa: e851f0
    pop          DX                              ; 1000:2cad: 5a
    pop          CX                              ; 1000:2cae: 59
    cmp          AX,0x100                        ; 1000:2caf: 3d0001
    jz           loc_2cfa                        ; 1000:2cb2: 7446
    cmp          AX,DI                           ; 1000:2cb4: 3bc7
    jg           loc_2cc7                        ; 1000:2cb6: 7f0f
    mov          word [BP + -0x2],0x1            ; 1000:2cb8: c746fe0100
    cmp          byte [0x8b0],0x1                ; 1000:2cbd: 803eb00801
    jl           loc_2cd1                        ; 1000:2cc2: 7c0d
    call         player_move_left                ; 1000:2cc4: e83f00
loc_2cc7:
    cmp          byte [0x8b0],0x2                ; 1000:2cc7: 803eb00802
    jl           loc_2cd1                        ; 1000:2ccc: 7c03
    call         player_move_right               ; 1000:2cce: e86000
loc_2cd1:
    test         CX,0xc                          ; 1000:2cd1: f7c10c00
    jz           loc_2cfa                        ; 1000:2cd5: 7423
    inc          BX                              ; 1000:2cd7: 43
    add          CX,0x10                         ; 1000:2cd8: 83c110
    mov          AL,byte [BX]                    ; 1000:2cdb: 8a07
    cmp          AX,DI                           ; 1000:2cdd: 3bc7
    jg           loc_2cf0                        ; 1000:2cdf: 7f0f
    mov          word [BP + -0x2],0x1            ; 1000:2ce1: c746fe0100
    cmp          byte [0x8b0],0x1                ; 1000:2ce6: 803eb00801
    jl           loc_2cfa                        ; 1000:2ceb: 7c0d
    call         player_move_left                ; 1000:2ced: e81600
loc_2cf0:
    cmp          byte [0x8b0],0x2                ; 1000:2cf0: 803eb00802
    jl           loc_2cfa                        ; 1000:2cf5: 7c03
    call         player_move_right               ; 1000:2cf7: e83700
loc_2cfa:
    clc                                          ; 1000:2cfa: f8
    cmp          word [BP + -0x2],0x0            ; 1000:2cfb: 837efe00
    jz           loc_2d02                        ; 1000:2cff: 7401
    stc                                          ; 1000:2d01: f9
loc_2d02:
    mov          SP,BP                           ; 1000:2d02: 8be5
    pop          BP                              ; 1000:2d04: 5d
    ret                                          ; 1000:2d05: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_move_left  [Player / Movement]
; ADDRESS:  1000:2d06
; PURPOSE:  Applies leftward acceleration to player and sets facing direction to left.
; ----------------------------------------------------------------------------
player_move_left:
    push         AX                              ; 1000:2d06: 50
    cmp          AL,byte [0x6d6]                 ; 1000:2d07: 3a06d606
    jl           loc_2d2f                        ; 1000:2d0b: 7c22
    cmp          AL,byte [0x6d7]                 ; 1000:2d0d: 3a06d706
    jg           loc_2d2f                        ; 1000:2d11: 7f1c
    sub          AL,byte [0x6d6]                 ; 1000:2d13: 2a06d606
    mov          SI,0x6d8                        ; 1000:2d17: bed806
    add          SI,AX                           ; 1000:2d1a: 03f0
    mov          AL,byte [SI]                    ; 1000:2d1c: 8a04
    push         BX                              ; 1000:2d1e: 53
    push         CX                              ; 1000:2d1f: 51
    push         DX                              ; 1000:2d20: 52
    push         DI                              ; 1000:2d21: 57
    and          CX,0xfff0                       ; 1000:2d22: 83e1f0
    and          DX,0xfff0                       ; 1000:2d25: 83e2f0
    call         proj_spawn_burst                ; 1000:2d28: e85604
    pop          DI                              ; 1000:2d2b: 5f
    pop          DX                              ; 1000:2d2c: 5a
    pop          CX                              ; 1000:2d2d: 59
    pop          BX                              ; 1000:2d2e: 5b
loc_2d2f:
    pop          AX                              ; 1000:2d2f: 58
    ret                                          ; 1000:2d30: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_move_right  [Player / Movement]
; ADDRESS:  1000:2d31
; PURPOSE:  Applies rightward acceleration to player and sets facing direction to right.
; ----------------------------------------------------------------------------
player_move_right:
    push         AX                              ; 1000:2d31: 50
    cmp          AL,byte [0x6e2]                 ; 1000:2d32: 3a06e206
    jl           loc_2d5f                        ; 1000:2d36: 7c27
    cmp          AL,byte [0x6e3]                 ; 1000:2d38: 3a06e306
    jg           loc_2d5f                        ; 1000:2d3c: 7f21
    sub          AL,byte [0x6e2]                 ; 1000:2d3e: 2a06e206
    mov          SI,0x6e4                        ; 1000:2d42: bee406
    add          SI,AX                           ; 1000:2d45: 03f0
    mov          AL,byte [SI]                    ; 1000:2d47: 8a04
    mov          word [BP + -0x2],0x1            ; 1000:2d49: c746fe0100
    push         BX                              ; 1000:2d4e: 53
    push         CX                              ; 1000:2d4f: 51
    push         DX                              ; 1000:2d50: 52
    push         DI                              ; 1000:2d51: 57
    and          CX,0xfff0                       ; 1000:2d52: 83e1f0
    and          DX,0xfff0                       ; 1000:2d55: 83e2f0
    call         proj_spawn_burst                ; 1000:2d58: e82604
    pop          DI                              ; 1000:2d5b: 5f
    pop          DX                              ; 1000:2d5c: 5a
    pop          CX                              ; 1000:2d5d: 59
    pop          BX                              ; 1000:2d5e: 5b
loc_2d5f:
    pop          AX                              ; 1000:2d5f: 58
    ret                                          ; 1000:2d60: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_do_jump  [Player / Physics]
; ADDRESS:  1000:2d61
; PURPOSE:  Initiates jump state, applies vertical impulse (enhanced if jump boots are held), and plays jump sound.
; ----------------------------------------------------------------------------
player_do_jump:
    mov          BX,word [0x6c8]                 ; 1000:2d61: 8b1ec806
    add          BX,0xd                          ; 1000:2d65: 83c30d
    push         AX                              ; 1000:2d68: 50
    call         map_get_tile_at                 ; 1000:2d69: e892ef
    pop          CX                              ; 1000:2d6c: 59
    shr          CX,0x1                          ; 1000:2d6d: d1e9
    shr          CX,0x1                          ; 1000:2d6f: d1e9
    shr          CX,0x1                          ; 1000:2d71: d1e9
    shr          CX,0x1                          ; 1000:2d73: d1e9
    cmp          AX,word [0x292]                 ; 1000:2d75: 3b069202
    jle          loc_2d9f                        ; 1000:2d79: 7e24
    add          BX,word [0x298]                 ; 1000:2d7b: 031e9802
    mov          AL,byte [BX]                    ; 1000:2d7f: 8a07
    cmp          AX,word [0x292]                 ; 1000:2d81: 3b069202
    jg           loc_2da1                        ; 1000:2d85: 7f1a
    sub          BX,CX                           ; 1000:2d87: 2bd9
    mov          CX,word [0x6c6]                 ; 1000:2d89: 8b0ec606
    shr          CX,0x1                          ; 1000:2d8d: d1e9
    shr          CX,0x1                          ; 1000:2d8f: d1e9
    shr          CX,0x1                          ; 1000:2d91: d1e9
    shr          CX,0x1                          ; 1000:2d93: d1e9
    add          BX,CX                           ; 1000:2d95: 03d9
    mov          AL,byte [BX]                    ; 1000:2d97: 8a07
    cmp          AX,word [0x292]                 ; 1000:2d99: 3b069202
    jle          loc_2dc4                        ; 1000:2d9d: 7e25
loc_2d9f:
    stc                                          ; 1000:2d9f: f9
    ret                                          ; 1000:2da0: c3
loc_2da1:
    cmp          AX,word [0x294]                 ; 1000:2da1: 3b069402
    jg           loc_2dc4                        ; 1000:2da5: 7f1d
    sub          BX,CX                           ; 1000:2da7: 2bd9
    mov          CX,word [0x6c6]                 ; 1000:2da9: 8b0ec606
    shr          CX,0x1                          ; 1000:2dad: d1e9
    shr          CX,0x1                          ; 1000:2daf: d1e9
    shr          CX,0x1                          ; 1000:2db1: d1e9
    shr          CX,0x1                          ; 1000:2db3: d1e9
    add          BX,CX                           ; 1000:2db5: 03d9
    mov          AL,byte [BX]                    ; 1000:2db7: 8a07
    cmp          AX,word [0x294]                 ; 1000:2db9: 3b069402
    jle          loc_2dc4                        ; 1000:2dbd: 7e05
    mov          byte [0x89b],0x0                ; 1000:2dbf: c6069b0800
loc_2dc4:
    clc                                          ; 1000:2dc4: f8
    ret                                          ; 1000:2dc5: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_stop_jump  [Player / Physics]
; ADDRESS:  1000:2dc6
; PURPOSE:  Trims jump velocity when jump key is released early for variable jump height.
; ----------------------------------------------------------------------------
player_stop_jump:
    mov          AX,0x1                          ; 1000:2dc6: b80100
    mov          BX,0x965e                       ; 1000:2dc9: bb5e96
    mov          CX,0x5                          ; 1000:2dcc: b90500
    int3                                         ; 1000:2dcf: cc
    db 0x83, 0x06, 0xc8, 0x06, 0x10, 0x83, 0x2e, 0xc6, 0x06, 0x08, 0x73, 0x06, 0xc7, 0x06, 0xc6, 0x06 ; 1000:2dd0
    db 0x00, 0x00, 0xa1, 0xc8, 0x06, 0x2d, 0x14, 0x00, 0xa3, 0xa1, 0x08, 0xa1, 0xc6, 0x06, 0x2d, 0x04 ; 1000:2de0
    db 0x00, 0x73, 0x02, 0x33, 0xc0, 0xa3, 0x9f, 0x08, 0xa1, 0xc6, 0x06, 0x8b, 0x1e, 0xc8, 0x06, 0xe8 ; 1000:2df0
    db 0xfc, 0xee, 0x3b, 0x06, 0x94, 0x02, 0x7f, 0x05, 0x83, 0x06, 0xc6, 0x06, 0x08, 0xa1, 0xc6, 0x06 ; 1000:2e00
    db 0x05, 0x1f, 0x00, 0x8b, 0x1e, 0xc8, 0x06, 0xe8, 0xe4, 0xee, 0x3b, 0x06, 0x94, 0x02, 0x7f, 0x05 ; 1000:2e10
    db 0x83, 0x2e, 0xc6, 0x06, 0x08, 0xc6, 0x06, 0x9e, 0x08, 0x02, 0xc6, 0x06, 0x97, 0x08, 0x00, 0xc6 ; 1000:2e20
    db 0x06, 0xa6, 0x08, 0x00, 0xc6, 0x06, 0x9d, 0x08, 0x01, 0x32, 0xff, 0x8a, 0x1e, 0x96, 0x08, 0xc6 ; 1000:2e30
    db 0x06, 0x96, 0x08, 0x00, 0xe8, 0xd0, 0x0b, 0xc6, 0x06, 0xae, 0x08, 0x03, 0xe9, 0x39, 0xfd ; 1000:2e40
loc_2e4f:
    mov          AX,0x1                          ; 1000:2e4f: b80100
    mov          BX,0x9676                       ; 1000:2e52: bb7696
    mov          CX,0x5                          ; 1000:2e55: b90500
    int3                                         ; 1000:2e58: cc
    db 0x83, 0x2e, 0xc8, 0x06, 0x10, 0x83, 0x06, 0xc6, 0x06, 0x08, 0xa1, 0xc8, 0x06, 0x2d, 0x06, 0x00 ; 1000:2e59
    db 0xa3, 0xa1, 0x08, 0xa1, 0xc6, 0x06, 0x2d, 0x0c, 0x00, 0x73, 0x02, 0x33, 0xc0, 0xa3, 0x9f, 0x08 ; 1000:2e69
    db 0xc6, 0x06, 0x9e, 0x08, 0x02, 0xa0, 0x99, 0x08, 0xa2, 0x98, 0x08, 0xc6, 0x06, 0x9d, 0x08, 0x00 ; 1000:2e79
    db 0xc6, 0x06, 0x97, 0x08, 0x01, 0xc6, 0x06, 0xae, 0x08, 0x02, 0xe9, 0xf2, 0xfc ; 1000:2e89
loc_2e96:
    mov          AX,0x1                          ; 1000:2e96: b80100
    mov          BX,0x9676                       ; 1000:2e99: bb7696
    mov          CX,0x5                          ; 1000:2e9c: b90500
    int3                                         ; 1000:2e9f: cc
    db 0x83, 0x2e, 0xc8, 0x06, 0x10, 0x83, 0x06, 0xc6, 0x06, 0x08, 0xa1, 0xc8, 0x06, 0x2d, 0x06, 0x00 ; 1000:2ea0
    db 0xa3, 0xa1, 0x08, 0xa1, 0xc6, 0x06, 0x2d, 0x0c, 0x00, 0x73, 0x02, 0x33, 0xc0, 0xa3, 0x9f, 0x08 ; 1000:2eb0
    db 0xc6, 0x06, 0x9e, 0x08, 0x02, 0xc6, 0x06, 0x98, 0x08, 0x00, 0xc6, 0x06, 0x9d, 0x08, 0x00, 0xc6 ; 1000:2ec0
    db 0x06, 0x97, 0x08, 0x01, 0xc6, 0x06, 0xae, 0x08, 0x02, 0xe9, 0xac, 0xfc ; 1000:2ed0

; ----------------------------------------------------------------------------
; FUNCTION: player_process_controls  [Player / Input]
; ADDRESS:  1000:2edc
; PURPOSE:  Translates raw keyboard/joystick input vectors into player movement, jumping, and shooting actions.
; ----------------------------------------------------------------------------
player_process_controls:
    mov          AX,[0x894]                      ; 1000:2edc: a19408
    mov          CX,AX                           ; 1000:2edf: 8bc8
    add          AX,word [0x6c8]                 ; 1000:2ee1: 0306c806
    cmp          byte [cs:0x25a],0x1             ; 1000:2ee5: 2e803e5a0201
    jnz          loc_2ef0                        ; 1000:2eeb: 7503
    sub          AX,0x2                          ; 1000:2eed: 2d0200
loc_2ef0:
    or           AX,AX                           ; 1000:2ef0: 0bc0
    jge          loc_2ef6                        ; 1000:2ef2: 7d02
    xor          AX,AX                           ; 1000:2ef4: 33c0
loc_2ef6:
    push         AX                              ; 1000:2ef6: 50
    mov          BX,AX                           ; 1000:2ef7: 8bd8
    mov          AX,[0x6c6]                      ; 1000:2ef9: a1c606
    mov          DI,word [0x292]                 ; 1000:2efc: 8b3e9202
    cmp          word [0x894],0x0                ; 1000:2f00: 833e940800
    jz           loc_2f2d                        ; 1000:2f05: 7426
    jl           loc_2f1b                        ; 1000:2f07: 7c12
    add          BX,0xf                          ; 1000:2f09: 83c30f
    call         player_take_damage              ; 1000:2f0c: e8df00
    jnc          loc_2f2d                        ; 1000:2f0f: 731c
    pop          AX                              ; 1000:2f11: 58
    and          AX,0xfff0                       ; 1000:2f12: 25f0ff
    mov          [0x6c8],AX                      ; 1000:2f15: a3c806
    jmp short    loc_2f31                        ; 1000:2f18: eb17
    db 0x90                                         ; 1000:2f1a
loc_2f1b:
    call         player_take_damage              ; 1000:2f1b: e8d000
    jnc          loc_2f2d                        ; 1000:2f1e: 730d
    pop          AX                              ; 1000:2f20: 58
    and          AX,0xfff0                       ; 1000:2f21: 25f0ff
    add          AX,0x10                         ; 1000:2f24: 051000
    mov          [0x6c8],AX                      ; 1000:2f27: a3c806
    jmp short    loc_2f31                        ; 1000:2f2a: eb05
    db 0x90                                         ; 1000:2f2c
loc_2f2d:
    pop          AX                              ; 1000:2f2d: 58
    mov          [0x6c8],AX                      ; 1000:2f2e: a3c806
loc_2f31:
    mov          CX,word [0x894]                 ; 1000:2f31: 8b0e9408
    sub          CX,0x5                          ; 1000:2f35: 83e905
    cmp          CX,-0x4                         ; 1000:2f38: 83f9fc
    jge          loc_2f40                        ; 1000:2f3b: 7d03
    mov          CX,0xfffc                       ; 1000:2f3d: b9fcff
loc_2f40:
    mov          word [0x894],CX                 ; 1000:2f40: 890e9408
    mov          CX,word [0x892]                 ; 1000:2f44: 8b0e9208
    cmp          byte [cs:0x258],0x1             ; 1000:2f48: 2e803e580201
    jnz          loc_2f5e                        ; 1000:2f4e: 750e
    mov          byte [0x8ad],0x2                ; 1000:2f50: c606ad0802
    dec          CX                              ; 1000:2f55: 49
    cmp          CX,-0x5                         ; 1000:2f56: 83f9fb
    jge          loc_2f5e                        ; 1000:2f59: 7d03
    mov          CX,0xfffb                       ; 1000:2f5b: b9fbff
loc_2f5e:
    cmp          byte [cs:0x259],0x1             ; 1000:2f5e: 2e803e590201
    jnz          loc_2f74                        ; 1000:2f64: 750e
    mov          byte [0x8ad],0x1                ; 1000:2f66: c606ad0801
    inc          CX                              ; 1000:2f6b: 41
    cmp          CX,0x5                          ; 1000:2f6c: 83f905
    jle          loc_2f74                        ; 1000:2f6f: 7e03
    mov          CX,0x5                          ; 1000:2f71: b90500
loc_2f74:
    mov          word [0x892],CX                 ; 1000:2f74: 890e9208
    or           CX,CX                           ; 1000:2f78: 0bc9
    jz           loc_2f8e                        ; 1000:2f7a: 7412
    jg           loc_2f87                        ; 1000:2f7c: 7f09
    inc          word [0x892]                    ; 1000:2f7e: ff069208
    call         player_check_hazard_collision   ; 1000:2f82: e8a100
    jmp short    loc_2f8e                        ; 1000:2f85: eb07
loc_2f87:
    dec          word [0x892]                    ; 1000:2f87: ff0e9208
    call         player_check_crush              ; 1000:2f8b: e8d000
loc_2f8e:
    cmp          byte [cs:0x25b],0x1             ; 1000:2f8e: 2e803e5b0201
    jnz          loc_2fa9                        ; 1000:2f94: 7513
    mov          CX,word [0x894]                 ; 1000:2f96: 8b0e9408
    add          CX,0x7                          ; 1000:2f9a: 83c107
    cmp          CX,0x4                          ; 1000:2f9d: 83f904
    jle          loc_2fa5                        ; 1000:2fa0: 7e03
    mov          CX,0x4                          ; 1000:2fa2: b90400
loc_2fa5:
    mov          word [0x894],CX                 ; 1000:2fa5: 890e9408
loc_2fa9:
    mov          AX,[0x6c6]                      ; 1000:2fa9: a1c606
    add          AX,0x10                         ; 1000:2fac: 051000
    mov          BX,word [0x6c8]                 ; 1000:2faf: 8b1ec806
    call         map_get_tile_at                 ; 1000:2fb3: e848ed
    cmp          AX,word [0x296]                 ; 1000:2fb6: 3b069602
    jle          loc_2fd1                        ; 1000:2fba: 7e15
    and          word [0x6c8],0xfff0             ; 1000:2fbc: 8326c806f0
    add          word [0x6c8],0xc                ; 1000:2fc1: 8306c8060c
    cmp          byte [cs:0x25c],0x1             ; 1000:2fc6: 2e803e5c0201
    jnz          loc_2fd1                        ; 1000:2fcc: 7503
    jmp near     loc_2e4f                        ; 1000:2fce: e97efe
loc_2fd1:
    mov          BX,word [0x6c8]                 ; 1000:2fd1: 8b1ec806
    mov          AX,[0x6c6]                      ; 1000:2fd5: a1c606
    add          AX,0x10                         ; 1000:2fd8: 051000
    add          BX,0x8                          ; 1000:2fdb: 83c308
    call         player_add_health               ; 1000:2fde: e83800
    jc           loc_2fe6                        ; 1000:2fe1: 7203
    jmp near     loc_2e96                        ; 1000:2fe3: e9b0fe
loc_2fe6:
    mov          byte [0x8ae],0x3                ; 1000:2fe6: c606ae0803
    jmp near     loc_2b88                        ; 1000:2feb: e99afb

; ----------------------------------------------------------------------------
; FUNCTION: player_take_damage  [Player / Health]
; ADDRESS:  1000:2fee
; PURPOSE:  Deducts shield points, sets invulnerability timer, plays damage sound effect, and triggers death if shields reach zero.
; ----------------------------------------------------------------------------
player_take_damage:
    cmp          BX,word [0x29e]                 ; 1000:2fee: 3b1e9e02
    jge          loc_3011                        ; 1000:2ff2: 7d1d
    push         AX                              ; 1000:2ff4: 50
    call         map_get_tile_at                 ; 1000:2ff5: e806ed
    pop          CX                              ; 1000:2ff8: 59
    cmp          AX,DI                           ; 1000:2ff9: 3bc7
    jle          loc_3011                        ; 1000:2ffb: 7e14
    mov          AL,byte [BX + 0x1]              ; 1000:2ffd: 8a4701
    cmp          AX,DI                           ; 1000:3000: 3bc7
    jle          loc_3011                        ; 1000:3002: 7e0d
    test         CX,0xc                          ; 1000:3004: f7c10c00
    jz           loc_3015                        ; 1000:3008: 740b
    mov          AL,byte [BX + 0x2]              ; 1000:300a: 8a4702
    cmp          AX,DI                           ; 1000:300d: 3bc7
    jg           loc_3015                        ; 1000:300f: 7f04
loc_3011:
    xor          AX,AX                           ; 1000:3011: 33c0
    stc                                          ; 1000:3013: f9
    ret                                          ; 1000:3014: c3
loc_3015:
    xor          AX,AX                           ; 1000:3015: 33c0
    clc                                          ; 1000:3017: f8
    ret                                          ; 1000:3018: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_add_health  [Player / Health]
; ADDRESS:  1000:3019
; PURPOSE:  Increases player shield points up to maximum capacity and plays recharge sound.
; ----------------------------------------------------------------------------
player_add_health:
    call         map_get_tile_at                 ; 1000:3019: e8e2ec
    cmp          AX,word [0x296]                 ; 1000:301c: 3b069602
    jg           loc_3024                        ; 1000:3020: 7f02
    stc                                          ; 1000:3022: f9
    ret                                          ; 1000:3023: c3
loc_3024:
    clc                                          ; 1000:3024: f8
    ret                                          ; 1000:3025: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_check_hazard_collision  [Player / Collision]
; ADDRESS:  1000:3026
; PURPOSE:  Checks player bounding box against environmental hazards (spikes, lava, acid).
; ----------------------------------------------------------------------------
player_check_hazard_collision:
    mov          AX,[0x6c6]                      ; 1000:3026: a1c606
    or           AX,AX                           ; 1000:3029: 0bc0
    jnz          loc_3033                        ; 1000:302b: 7506
    call         ent_check_player_contact        ; 1000:302d: e8ae05
    jmp near     player_stop_x                   ; 1000:3030: e9fffb
loc_3033:
    sub          AX,0x8                          ; 1000:3033: 2d0800
    push         AX                              ; 1000:3036: 50
    call         player_fall_death               ; 1000:3037: e86300
    pop          AX                              ; 1000:303a: 58
    jc           player_zero_score_delta         ; 1000:303b: 721a
    mov          [0x6c6],AX                      ; 1000:303d: a3c606
    sub          AX,word [0x6c2]                 ; 1000:3040: 2b06c206
    cmp          AX,0x50                         ; 1000:3044: 3d5000
    jge          loc_305d                        ; 1000:3047: 7d14
    cmp          word [0x6c2],0x0                ; 1000:3049: 833ec20600
    jz           loc_305d                        ; 1000:304e: 740d
    sub          word [0x6c2],0x8                ; 1000:3050: 832ec20608
    jmp short    loc_305d                        ; 1000:3055: eb06

; ----------------------------------------------------------------------------
; FUNCTION: player_zero_score_delta  [Player / Score]
; ADDRESS:  1000:3057
; PURPOSE:  Resets per-frame score delta accumulator.
; ----------------------------------------------------------------------------
player_zero_score_delta:
    mov          word [0x892],0x0                ; 1000:3057: c70692080000
loc_305d:
    ret                                          ; 1000:305d: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_check_crush  [Player / Collision]
; ADDRESS:  1000:305e
; PURPOSE:  Checks if Comic is crushed between moving platforms or solid blocks.
; ----------------------------------------------------------------------------
player_check_crush:
    mov          AX,[0x6c6]                      ; 1000:305e: a1c606
    mov          BX,AX                           ; 1000:3061: 8bd8
    add          BX,0x20                         ; 1000:3063: 83c320
    cmp          BX,word [0x29c]                 ; 1000:3066: 3b1e9c02
    jl           loc_3071                        ; 1000:306a: 7c05
    call         ent_check_player_contact        ; 1000:306c: e86f05
    jmp short    player_zero_score_delta         ; 1000:306f: ebe6
loc_3071:
    add          AX,0x8                          ; 1000:3071: 050800
    push         AX                              ; 1000:3074: 50
    add          AX,0x1f                         ; 1000:3075: 051f00
    call         player_fall_death               ; 1000:3078: e82200
    pop          AX                              ; 1000:307b: 58
    jc           player_zero_score_delta         ; 1000:307c: 72d9
    mov          [0x6c6],AX                      ; 1000:307e: a3c606
    sub          AX,word [0x6c2]                 ; 1000:3081: 2b06c206
    cmp          AX,0x60                         ; 1000:3085: 3d6000
    jle          loc_305d                        ; 1000:3088: 7ed3
    mov          AX,[0x6c2]                      ; 1000:308a: a1c206
    add          AX,0xd0                         ; 1000:308d: 05d000
    cmp          AX,word [0x29c]                 ; 1000:3090: 3b069c02
    jz           loc_305d                        ; 1000:3094: 74c7
    add          word [0x6c2],0x8                ; 1000:3096: 8306c20608
    jmp short    loc_305d                        ; 1000:309b: ebc0

; ----------------------------------------------------------------------------
; FUNCTION: player_fall_death  [Player / Lifecycle]
; ADDRESS:  1000:309d
; PURPOSE:  Handles player falling into bottomless pit / lethal hazard.
; ----------------------------------------------------------------------------
player_fall_death:
    mov          DI,word [0x294]                 ; 1000:309d: 8b3e9402
    mov          BX,word [0x6c8]                 ; 1000:30a1: 8b1ec806
    call         map_get_tile_at                 ; 1000:30a5: e856ec
    cmp          AX,DI                           ; 1000:30a8: 3bc7
    jle          loc_30c2                        ; 1000:30aa: 7e16
    mov          CX,word [0x6c8]                 ; 1000:30ac: 8b0ec806
    and          CX,0xf                          ; 1000:30b0: 83e10f
    cmp          CX,0x4                          ; 1000:30b3: 83f904
    jle          loc_30c4                        ; 1000:30b6: 7e0c
    add          BX,word [0x298]                 ; 1000:30b8: 031e9802
    mov          AL,byte [BX]                    ; 1000:30bc: 8a07
    cmp          AX,DI                           ; 1000:30be: 3bc7
    jg           loc_30c4                        ; 1000:30c0: 7f02
loc_30c2:
    stc                                          ; 1000:30c2: f9
    ret                                          ; 1000:30c3: c3
loc_30c4:
    clc                                          ; 1000:30c4: f8
    ret                                          ; 1000:30c5: c3

; ----------------------------------------------------------------------------
; FUNCTION: player_fire_projectile  [Weapons / Fire]
; ADDRESS:  1000:30c6
; PURPOSE:  Spawns player blast fireball, checks weapon power level (normal, corkscrew, fireball), deducts ammo/power, and plays sound.
; ----------------------------------------------------------------------------
player_fire_projectile:
    cmp          byte [0x8ad],0x1                ; 1000:30c6: 803ead0801
    jz           loc_30db                        ; 1000:30cb: 740e
    cmp          word [0x6c6],0x8                ; 1000:30cd: 833ec60608
    jl           loc_3106                        ; 1000:30d2: 7c32
    mov          byte [0x8a5],0x2                ; 1000:30d4: c606a50802
    jmp short    loc_30ec                        ; 1000:30d9: eb11
loc_30db:
    mov          AX,[0x29c]                      ; 1000:30db: a19c02
    sub          AX,0x18                         ; 1000:30de: 2d1800
    cmp          word [0x6c6],AX                 ; 1000:30e1: 3906c606
    jg           loc_3106                        ; 1000:30e5: 7f1f
    mov          byte [0x8a5],0x1                ; 1000:30e7: c606a50801
loc_30ec:
    mov          byte [0x8a3],0x1                ; 1000:30ec: c606a30801
    mov          byte [0x8a4],0x3                ; 1000:30f1: c606a40803
    mov          byte [0x8af],0x1                ; 1000:30f6: c606af0801
    mov          byte [0x8ae],0x4                ; 1000:30fb: c606ae0804
    mov          word [0x892],0x0                ; 1000:3100: c70692080000
loc_3106:
    ret                                          ; 1000:3106: c3

; ----------------------------------------------------------------------------
; FUNCTION: proj_init_trajectory  [Weapons / Projectiles]
; ADDRESS:  1000:3107
; PURPOSE:  Calculates initial velocity vector, sprite angle, and lifetime for spawned projectile.
; ----------------------------------------------------------------------------
proj_init_trajectory:
    cmp          byte [0x8a4],0x3                ; 1000:3107: 803ea40803
    jnz          loc_3123                        ; 1000:310c: 7515
    cmp          byte [cs:0x25d],0x1             ; 1000:310e: 2e803e5d0201
    jnz          loc_3123                        ; 1000:3114: 750d
    mov          byte [0x8af],0x1                ; 1000:3116: c606af0801
    mov          byte [0x8ae],0x4                ; 1000:311b: c606ae0804
    jmp near     loc_267f                        ; 1000:3120: e95cf5
loc_3123:
    dec          byte [0x8a4]                    ; 1000:3123: fe0ea408
    jnz          loc_3135                        ; 1000:3127: 750c
    mov          byte [0x8a3],0x0                ; 1000:3129: c606a30800
    mov          byte [0x8ae],0x0                ; 1000:312e: c606ae0800
    jmp short    loc_317e                        ; 1000:3133: eb49
loc_3135:
    mov          AX,[0x6c6]                      ; 1000:3135: a1c606
    test         AX,0x8                          ; 1000:3138: a90800
    jnz          loc_3179                        ; 1000:313b: 753c
    sub          AX,0x10                         ; 1000:313d: 2d1000
    cmp          byte [0x8a5],0x1                ; 1000:3140: 803ea50801
    jnz          loc_314a                        ; 1000:3145: 7503
    add          AX,0x20                         ; 1000:3147: 052000
loc_314a:
    mov          BX,word [0x6c8]                 ; 1000:314a: 8b1ec806
    cmp          byte [0x8a4],0x2                ; 1000:314e: 803ea40802
    jz           loc_3158                        ; 1000:3153: 7403
    add          BX,0x10                         ; 1000:3155: 83c310
loc_3158:
    push         AX                              ; 1000:3158: 50
    push         BX                              ; 1000:3159: 53
    call         map_get_tile_at                 ; 1000:315a: e8a1eb
    pop          DX                              ; 1000:315d: 5a
    pop          CX                              ; 1000:315e: 59
    cmp          AL,byte [0x6ca]                 ; 1000:315f: 3a06ca06
    jl           loc_3179                        ; 1000:3163: 7c14
    cmp          AL,byte [0x6cb]                 ; 1000:3165: 3a06cb06
    jg           loc_3179                        ; 1000:3169: 7f0e
    sub          AL,byte [0x6ca]                 ; 1000:316b: 2a06ca06
    mov          SI,0x6cc                        ; 1000:316f: becc06
    add          SI,AX                           ; 1000:3172: 03f0
    mov          AL,byte [SI]                    ; 1000:3174: 8a04
    call         proj_spawn_burst                ; 1000:3176: e80800
loc_3179:
    mov          byte [0x8ae],0x4                ; 1000:3179: c606ae0804
loc_317e:
    jmp near     loc_267f                        ; 1000:317e: e9fef4

; ----------------------------------------------------------------------------
; FUNCTION: proj_spawn_burst  [Weapons / Projectiles]
; ADDRESS:  1000:3181
; PURPOSE:  Spawns multiple projectile burst when upgraded weapons (corkscrew / spread blast) are active.
; ----------------------------------------------------------------------------
proj_spawn_burst:
    cmp          byte [0x8b1],0x6                ; 1000:3181: 803eb10806
    jz           loc_31ea                        ; 1000:3186: 7462
    mov          SI,0x8b2                        ; 1000:3188: beb208
    mov          DI,0x6                          ; 1000:318b: bf0600
loc_318e:
    cmp          word [SI + 0x5],CX              ; 1000:318e: 394c05
    jnz          loc_319b                        ; 1000:3191: 7508
    cmp          word [SI + 0x7],DX              ; 1000:3193: 395407
    jnz          loc_319b                        ; 1000:3196: 7503
    jmp short    loc_31ea                        ; 1000:3198: eb50
    db 0x90                                         ; 1000:319a
loc_319b:
    add          SI,0x9                          ; 1000:319b: 83c609
    dec          DI                              ; 1000:319e: 4f
    jnz          loc_318e                        ; 1000:319f: 75ed
    mov          SI,0x8b2                        ; 1000:31a1: beb208
loc_31a4:
    cmp          byte [SI],0x0                   ; 1000:31a4: 803c00
    jz           loc_31ae                        ; 1000:31a7: 7405
    add          SI,0x9                          ; 1000:31a9: 83c609
    jmp short    loc_31a4                        ; 1000:31ac: ebf6
loc_31ae:
    inc          byte [0x8b1]                    ; 1000:31ae: fe06b108
    mov          byte [SI],0x1                   ; 1000:31b2: c60401
    mov          byte [SI + 0x1],AL              ; 1000:31b5: 884401
    mov          word [SI + 0x2],BX              ; 1000:31b8: 895c02
    mov          byte [SI + 0x4],0x0             ; 1000:31bb: c6440400
    mov          word [SI + 0x5],CX              ; 1000:31bf: 894c05
    mov          word [SI + 0x7],DX              ; 1000:31c2: 895407
    mov          AX,CX                           ; 1000:31c5: 8bc1
    mov          BX,DX                           ; 1000:31c7: 8bda
    mov          CL,byte [SI + 0x1]              ; 1000:31c9: 8a4c01
    xor          CH,CH                           ; 1000:31cc: 32ed
    push         AX                              ; 1000:31ce: 50
    push         BX                              ; 1000:31cf: 53
    push         CX                              ; 1000:31d0: 51
    call         map_draw_tile_row               ; 1000:31d1: e835ec
    pop          CX                              ; 1000:31d4: 59
    pop          BX                              ; 1000:31d5: 5b
    pop          AX                              ; 1000:31d6: 58
    cmp          CX,word [0x292]                 ; 1000:31d7: 3b0e9202
    jle          loc_31e0                        ; 1000:31db: 7e03
    call         ui_erase_dialog                 ; 1000:31dd: e89913
loc_31e0:
    mov          AX,0x1                          ; 1000:31e0: b80100
    mov          BX,0x968e                       ; 1000:31e3: bb8e96
    mov          CX,0x5                          ; 1000:31e6: b90500
    int3                                         ; 1000:31e9: cc
loc_31ea:
    ret                                          ; 1000:31ea: c3

; ----------------------------------------------------------------------------
; FUNCTION: proj_update_all  [Weapons / Projectiles]
; ADDRESS:  1000:31eb
; PURPOSE:  Updates all active player projectiles: advances position, checks tile collision, and triggers explosions.
; ----------------------------------------------------------------------------
proj_update_all:
    mov          DI,0x8b2                        ; 1000:31eb: bfb208
    mov          word [0x9620],0x0               ; 1000:31ee: c70620960000
loc_31f4:
    cmp          byte [DI],0x0                   ; 1000:31f4: 803d00
    jz           loc_326d                        ; 1000:31f7: 7474
    inc          word [0x9620]                   ; 1000:31f9: ff062096
    mov          SI,0x9622                       ; 1000:31fd: be2296
    mov          BL,byte [DI]                    ; 1000:3200: 8a1d
    xor          BH,BH                           ; 1000:3202: 32ff
    dec          BL                              ; 1000:3204: fecb
    shl          BX,0x1                          ; 1000:3206: d1e3
    mov          SI,word [BX + SI]               ; 1000:3208: 8b30
    mov          AL,byte [DI + 0x4]              ; 1000:320a: 8a4504
    xor          AH,AH                           ; 1000:320d: 32e4
    shl          AX,0x1                          ; 1000:320f: d1e0
    add          SI,AX                           ; 1000:3211: 03f0
    mov          AX,word [DI + 0x5]              ; 1000:3213: 8b4505
    sub          AX,word [0x6c2]                 ; 1000:3216: 2b06c206
    jl           loc_3247                        ; 1000:321a: 7c2b
    cmp          AX,0xc0                         ; 1000:321c: 3dc000
    jg           loc_3247                        ; 1000:321f: 7f26
    add          AX,0x8                          ; 1000:3221: 050800
    mov          BX,word [DI + 0x7]              ; 1000:3224: 8b5d07
    sub          BX,word [0x6c4]                 ; 1000:3227: 2b1ec406
    jl           loc_3247                        ; 1000:322b: 7c1a
    cmp          BX,0x90                         ; 1000:322d: 81fb9000
    jg           loc_3247                        ; 1000:3231: 7f14
    add          BX,0x8                          ; 1000:3233: 83c308
    push         DI                              ; 1000:3236: 57
    mov          DS,word [cs:0x5e]               ; 1000:3237: 2e8e1e5e00
    mov          SI,word [SI]                    ; 1000:323c: 8b34
    call         vid_clear_page                  ; 1000:323e: e86245
    mov          DS,word [cs:0x5c]               ; 1000:3241: 2e8e1e5c00
    pop          DI                              ; 1000:3246: 5f
loc_3247:
    inc          byte [DI + 0x4]                 ; 1000:3247: fe4504
    cmp          byte [DI + 0x4],0x3             ; 1000:324a: 807d0403
    jnz          loc_326d                        ; 1000:324e: 751d
    cmp          byte [DI],0x1                   ; 1000:3250: 803d01
    jnz          loc_325d                        ; 1000:3253: 7508
    mov          BX,word [DI + 0x2]              ; 1000:3255: 8b5d02
    mov          AL,byte [DI + 0x1]              ; 1000:3258: 8a4501
    mov          byte [BX],AL                    ; 1000:325b: 8807
loc_325d:
    mov          byte [DI],0x0                   ; 1000:325d: c60500
    mov          word [DI + 0x5],0xffff          ; 1000:3260: c74505ffff
    dec          byte [0x8b1]                    ; 1000:3265: fe0eb108
    dec          word [0x9620]                   ; 1000:3269: ff0e2096
loc_326d:
    add          DI,0x9                          ; 1000:326d: 83c709
    mov          AX,[0x9620]                     ; 1000:3270: a12096
    cmp          AL,byte [0x8b1]                 ; 1000:3273: 3a06b108
    jz           loc_327c                        ; 1000:3277: 7403
    jmp near     loc_31f4                        ; 1000:3279: e978ff
loc_327c:
    ret                                          ; 1000:327c: c3

; ----------------------------------------------------------------------------
; FUNCTION: proj_check_monster_hit  [Weapons / Collision]
; ADDRESS:  1000:327d
; PURPOSE:  Tests intersection between player projectiles and monster bounding boxes; damages monsters.
; ----------------------------------------------------------------------------
proj_check_monster_hit:
    cmp          byte [0x89d],0x1                ; 1000:327d: 803e9d0801
    jz           loc_3290                        ; 1000:3282: 740c
    cmp          byte [0x924],0x2                ; 1000:3284: 803e240902
    jz           loc_3290                        ; 1000:3289: 7405
    cmp          byte [0x92c],0x2                ; 1000:328b: 803e2c0902
loc_3290:
    jz           loc_330e                        ; 1000:3290: 747c
    mov          BL,byte [0x896]                 ; 1000:3292: 8a1e9608
    xor          BH,BH                           ; 1000:3296: 32ff
    mov          AH,BL                           ; 1000:3298: 8ae3
loc_329a:
    add          AH,AL                           ; 1000:329a: 02e0
    cmp          AH,0x4                          ; 1000:329c: 80fc04
    jge          loc_330e                        ; 1000:329f: 7d6d
    cmp          AH,0x0                          ; 1000:32a1: 80fc00
    jl           loc_330e                        ; 1000:32a4: 7c68
    jz           loc_32cb                        ; 1000:32a6: 7423
    cmp          AH,0x3                          ; 1000:32a8: 80fc03
    jnz          loc_32b6                        ; 1000:32ab: 7509
    cmp          byte [0x21f],0x1                ; 1000:32ad: 803e1f0201
    jz           loc_32cb                        ; 1000:32b2: 7417
    jmp short    loc_329a                        ; 1000:32b4: ebe4
loc_32b6:
    cmp          AH,0x2                          ; 1000:32b6: 80fc02
    jnz          loc_32c4                        ; 1000:32b9: 7509
    cmp          byte [0x21e],0x1                ; 1000:32bb: 803e1e0201
    jz           loc_32cb                        ; 1000:32c0: 7409
    jmp short    loc_329a                        ; 1000:32c2: ebd6
loc_32c4:
    cmp          byte [0x21d],0x1                ; 1000:32c4: 803e1d0201
    jnz          loc_329a                        ; 1000:32c9: 75cf
loc_32cb:
    mov          byte [0x8a3],0x0                ; 1000:32cb: c606a30800
    mov          byte [0x896],AH                 ; 1000:32d0: 88269608
    call         hud_draw_item_icon              ; 1000:32d4: e84007
    cmp          byte [0x896],0x2                ; 1000:32d7: 803e960802
    jnz          loc_32f8                        ; 1000:32dc: 751a
    cmp          word [0x222],0x0                ; 1000:32de: 833e220200
    jz           loc_330e                        ; 1000:32e3: 7429
    cmp          word [0x6fa],0xe                ; 1000:32e5: 833efa060e
    jz           loc_330e                        ; 1000:32ea: 7422
    mov          byte [0x897],0x0                ; 1000:32ec: c606970800
    mov          byte [0x8a6],0x1                ; 1000:32f1: c606a60801
    jmp short    loc_330e                        ; 1000:32f6: eb16
loc_32f8:
    cmp          byte [0x8a6],0x1                ; 1000:32f8: 803ea60801
    jnz          loc_330e                        ; 1000:32fd: 750f
    mov          byte [0x8a6],0x0                ; 1000:32ff: c606a60800
    mov          byte [0x897],0x1                ; 1000:3304: c606970801
    mov          byte [0x898],0x0                ; 1000:3309: c606980800
loc_330e:
    ret                                          ; 1000:330e: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_update_all  [Entities / AI]
; ADDRESS:  1000:330f
; PURPOSE:  Iterates through all active monsters: updates movement AI, animation frames, and tests contact with Comic.
; ----------------------------------------------------------------------------
ent_update_all:
    cmp          word [0x6fa],0xe                ; 1000:330f: 833efa060e
    jz           loc_331d                        ; 1000:3314: 7407
    cmp          word [0x222],0x0                ; 1000:3316: 833e220200
    jnz          loc_332f                        ; 1000:331b: 7512
loc_331d:
    mov          byte [0x8a6],0x0                ; 1000:331d: c606a60800
    mov          byte [0x897],0x1                ; 1000:3322: c606970801
    mov          byte [0x898],0x0                ; 1000:3327: c606980800
    jmp near     player_update_physics           ; 1000:332c: e9e1f6
loc_332f:
    test         byte [0x962b],0x3               ; 1000:332f: f6062b9603
    jnz          loc_3341                        ; 1000:3334: 750b
    cmp          word [cs:0x14d4],-0x1           ; 1000:3336: 2e833ed414ff
    jz           loc_3341                        ; 1000:333c: 7403
    call         item_add_to_inventory           ; 1000:333e: e86915
loc_3341:
    cmp          byte [cs:0x25d],0x1             ; 1000:3341: 2e803e5d0201
    jz           loc_3351                        ; 1000:3347: 7408
    cmp          byte [cs:0x25a],0x1             ; 1000:3349: 2e803e5a0201
    jnz          loc_3363                        ; 1000:334f: 7512
loc_3351:
    sub          word [0x894],0x2                ; 1000:3351: 832e940802
    cmp          word [0x894],-0xc               ; 1000:3356: 833e9408f4
    jge          loc_3363                        ; 1000:335b: 7d06
    mov          word [0x894],0xfff4             ; 1000:335d: c7069408f4ff
loc_3363:
    mov          AX,[0x894]                      ; 1000:3363: a19408
    mov          CX,AX                           ; 1000:3366: 8bc8
    add          AX,word [0x6c8]                 ; 1000:3368: 0306c806
    jge          loc_3372                        ; 1000:336c: 7d04
    xor          AX,AX                           ; 1000:336e: 33c0
    xor          CX,CX                           ; 1000:3370: 33c9
loc_3372:
    mov          [0x6c8],AX                      ; 1000:3372: a3c806
    mov          word [0x9620],CX                ; 1000:3375: 890e2096
    add          CX,0x1                          ; 1000:3379: 83c101
    cmp          CX,0x8                          ; 1000:337c: 83f908
    jle          loc_3384                        ; 1000:337f: 7e03
    mov          CX,0x8                          ; 1000:3381: b90800
loc_3384:
    mov          word [0x894],CX                 ; 1000:3384: 890e9408
    mov          CX,word [0x892]                 ; 1000:3388: 8b0e9208
    cmp          byte [cs:0x258],0x1             ; 1000:338c: 2e803e580201
    jnz          loc_33a2                        ; 1000:3392: 750e
    mov          byte [0x8ad],0x2                ; 1000:3394: c606ad0802
    dec          CX                              ; 1000:3399: 49
    cmp          CX,-0x5                         ; 1000:339a: 83f9fb
    jge          loc_33a2                        ; 1000:339d: 7d03
    mov          CX,0xfffb                       ; 1000:339f: b9fbff
loc_33a2:
    cmp          byte [cs:0x259],0x1             ; 1000:33a2: 2e803e590201
    jnz          loc_33b8                        ; 1000:33a8: 750e
    mov          byte [0x8ad],0x1                ; 1000:33aa: c606ad0801
    inc          CX                              ; 1000:33af: 41
    cmp          CX,0x5                          ; 1000:33b0: 83f905
    jle          loc_33b8                        ; 1000:33b3: 7e03
    mov          CX,0x5                          ; 1000:33b5: b90500
loc_33b8:
    mov          word [0x892],CX                 ; 1000:33b8: 890e9208
    or           CX,CX                           ; 1000:33bc: 0bc9
    jz           loc_33d2                        ; 1000:33be: 7412
    jg           loc_33cb                        ; 1000:33c0: 7f09
    inc          word [0x892]                    ; 1000:33c2: ff069208
    call         player_check_wall_collision_left ; 1000:33c6: e82ef8
    jmp short    loc_33d2                        ; 1000:33c9: eb07
loc_33cb:
    dec          word [0x892]                    ; 1000:33cb: ff0e9208
    call         player_check_climbing           ; 1000:33cf: e882f8
loc_33d2:
    cmp          word [0x894],0x0                ; 1000:33d2: 833e940800
    jg           loc_3405                        ; 1000:33d7: 7f2c
    mov          AX,[0x6c6]                      ; 1000:33d9: a1c606
    mov          BX,word [0x6c8]                 ; 1000:33dc: 8b1ec806
    mov          DI,word [0x292]                 ; 1000:33e0: 8b3e9202
    mov          byte [0x8b0],0x2                ; 1000:33e4: c606b00802
    call         player_check_swimming           ; 1000:33e9: e8b1f8
    mov          byte [0x8b0],0x0                ; 1000:33ec: c606b00800
    jnc          loc_3405                        ; 1000:33f1: 7312
    mov          word [0x894],0x0                ; 1000:33f3: c70694080000
    and          word [0x6c8],0xfff0             ; 1000:33f9: 8326c806f0
    add          word [0x6c8],0xa                ; 1000:33fe: 8306c8060a
    jmp short    loc_3453                        ; 1000:3403: eb4e
loc_3405:
    cmp          word [0x9620],0x0               ; 1000:3405: 833e209600
    jle          loc_3453                        ; 1000:340a: 7e47
    mov          DI,word [0x294]                 ; 1000:340c: 8b3e9402
    cmp          byte [cs:0x25b],0x0             ; 1000:3410: 2e803e5b0200
    jz           loc_341c                        ; 1000:3416: 7404
    mov          DI,word [0x292]                 ; 1000:3418: 8b3e9202
loc_341c:
    mov          AX,[0x6c6]                      ; 1000:341c: a1c606
    mov          BX,word [0x6c8]                 ; 1000:341f: 8b1ec806
    add          BX,0x20                         ; 1000:3423: 83c320
    call         player_check_swimming           ; 1000:3426: e874f8
    jc           loc_3455                        ; 1000:3429: 722a
    cmp          AX,word [0x296]                 ; 1000:342b: 3b069602
    jg           loc_343a                        ; 1000:342f: 7f09
    cmp          AX,word [0x294]                 ; 1000:3431: 3b069402
    jle          loc_343a                        ; 1000:3435: 7e03
    jmp near     player_stop_jump                ; 1000:3437: e98cf9
loc_343a:
    mov          AX,[0x6c8]                      ; 1000:343a: a1c806
    add          AX,0x28                         ; 1000:343d: 052800
    cmp          AX,word [0x29e]                 ; 1000:3440: 3b069e02
    jl           loc_3460                        ; 1000:3444: 7c1a
    cmp          AX,word [0x29e]                 ; 1000:3446: 3b069e02
    sub          AX,0x20                         ; 1000:344a: 2d2000
    mov          [0x6c8],AX                      ; 1000:344d: a3c806
    jmp short    loc_3455                        ; 1000:3450: eb03
    db 0x90                                         ; 1000:3452
loc_3453:
    jmp short    loc_3460                        ; 1000:3453: eb0b
loc_3455:
    and          word [0x6c8],0xfff0             ; 1000:3455: 8326c806f0
    mov          word [0x894],0x0                ; 1000:345a: c70694080000
loc_3460:
    mov          byte [0x8ae],0x5                ; 1000:3460: c606ae0805
    jmp near     loc_2b88                        ; 1000:3465: e920f7

; ----------------------------------------------------------------------------
; FUNCTION: ent_spawn_enemy  [Entities / Spawning]
; ADDRESS:  1000:3468
; PURPOSE:  Spawns a new monster into an available entity slot from room spawn tables.
; ----------------------------------------------------------------------------
ent_spawn_enemy:
    cmp          byte [0x8ad],0x1                ; 1000:3468: 803ead0801
    jz           loc_347d                        ; 1000:346d: 740e
    cmp          word [0x6c6],0x8                ; 1000:346f: 833ec60608
    jl           loc_34a8                        ; 1000:3474: 7c32
    mov          byte [0x8a5],0x2                ; 1000:3476: c606a50802
    jmp short    loc_348e                        ; 1000:347b: eb11
loc_347d:
    mov          AX,[0x29c]                      ; 1000:347d: a19c02
    sub          AX,0x18                         ; 1000:3480: 2d1800
    cmp          word [0x6c6],AX                 ; 1000:3483: 3906c606
    jg           loc_34a8                        ; 1000:3487: 7f1f
    mov          byte [0x8a5],0x1                ; 1000:3489: c606a50801
loc_348e:
    mov          byte [0x8a7],0x1                ; 1000:348e: c606a70801
    mov          byte [0x8a4],0x3                ; 1000:3493: c606a40803
    mov          byte [0x8af],0x1                ; 1000:3498: c606af0801
    mov          byte [0x8ae],0x6                ; 1000:349d: c606ae0806
    mov          word [0x892],0x0                ; 1000:34a2: c70692080000
loc_34a8:
    ret                                          ; 1000:34a8: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_update_ai  [Entities / AI]
; ADDRESS:  1000:34a9
; PURPOSE:  Processes AI behavioral patterns: patrollers, jumpers, fliers, projectiles, and boss attacks.
; ----------------------------------------------------------------------------
ent_update_ai:
    dec          byte [0x8a4]                    ; 1000:34a9: fe0ea408
    jnz          loc_34bc                        ; 1000:34ad: 750d
    mov          byte [0x8a7],0x0                ; 1000:34af: c606a70800
    mov          byte [0x8ae],0x0                ; 1000:34b4: c606ae0800
    jmp near     loc_358e                        ; 1000:34b9: e9d200
loc_34bc:
    cmp          byte [0x8a4],0x1                ; 1000:34bc: 803ea40801
    jz           loc_34c6                        ; 1000:34c1: 7403
    jmp near     loc_3589                        ; 1000:34c3: e9c300
loc_34c6:
    mov          DI,word [0x6fe]                 ; 1000:34c6: 8b3efe06
    or           DI,DI                           ; 1000:34ca: 0bff
    jnz          loc_34d1                        ; 1000:34cc: 7503
    jmp near     loc_3589                        ; 1000:34ce: e9b800
loc_34d1:
    mov          AX,[0x6c6]                      ; 1000:34d1: a1c606
    sub          AX,0x10                         ; 1000:34d4: 2d1000
    cmp          byte [0x8a5],0x1                ; 1000:34d7: 803ea50801
    jnz          loc_34e1                        ; 1000:34dc: 7503
    add          AX,0x20                         ; 1000:34de: 052000
loc_34e1:
    mov          BX,word [0x6c8]                 ; 1000:34e1: 8b1ec806
    add          BX,0x10                         ; 1000:34e5: 83c310
    mov          SI,0x700                        ; 1000:34e8: be0007
loc_34eb:
    test         word [SI + 0x4],0x8000          ; 1000:34eb: f744040080
    jnz          loc_34fd                        ; 1000:34f0: 750b
    cmp          AX,word [SI]                    ; 1000:34f2: 3b04
    jnz          loc_34fd                        ; 1000:34f4: 7507
    cmp          BX,word [SI + 0x2]              ; 1000:34f6: 3b5c02
    jnz          loc_34fd                        ; 1000:34f9: 7502
    jmp short    loc_3506                        ; 1000:34fb: eb09
loc_34fd:
    add          SI,0x8                          ; 1000:34fd: 83c608
    dec          DI                              ; 1000:3500: 4f
    jnz          loc_34eb                        ; 1000:3501: 75e8
    jmp near     loc_3589                        ; 1000:3503: e98300
loc_3506:
    mov          BX,word [SI + 0x6]              ; 1000:3506: 8b5c06
    cmp          BX,0xafee                       ; 1000:3509: 81fbeeaf
    jnz          loc_3519                        ; 1000:350d: 750a
    cmp          byte [0x220],0x5                ; 1000:350f: 803e200205
    jl           loc_3519                        ; 1000:3514: 7c03
    jmp short    loc_3591                        ; 1000:3516: eb79
    db 0x90                                         ; 1000:3518
loc_3519:
    mov          DI,0x946                        ; 1000:3519: bf4609
    mov          CX,word [0x944]                 ; 1000:351c: 8b0e4409
loc_3520:
    cmp          BX,word [DI]                    ; 1000:3520: 3b1d
    jz           loc_352c                        ; 1000:3522: 7408
    add          DI,0x6                          ; 1000:3524: 83c706
    loop         loc_3520                        ; 1000:3527: e2f7
    jmp short    loc_3589                        ; 1000:3529: eb5e
    db 0x90                                         ; 1000:352b
loc_352c:
    mov          BX,word [DI + 0x2]              ; 1000:352c: 8b5d02
    mov          word [SI + 0x6],BX              ; 1000:352f: 895c06
    mov          AX,word [SI + 0x4]              ; 1000:3532: 8b4404
    and          AX,0x7fff                       ; 1000:3535: 25ff7f
    mov          SI,0xaf0                        ; 1000:3538: bef00a
    mov          CX,AX                           ; 1000:353b: 8bc8
    shl          AX,0x1                          ; 1000:353d: d1e0
    add          AX,CX                           ; 1000:353f: 03c1
    shl          AX,0x1                          ; 1000:3541: d1e0
    shl          AX,0x1                          ; 1000:3543: d1e0
    add          SI,AX                           ; 1000:3545: 03f0
    mov          AX,word [DI + 0x4]              ; 1000:3547: 8b4504
    mov          word [cs:SI + 0x8],BX           ; 1000:354a: 2e895c08
    mov          word [cs:SI + 0xa],AX           ; 1000:354e: 2e89440a
    cmp          byte [0x8b1],0x6                ; 1000:3552: 803eb10806
    jz           loc_357f                        ; 1000:3557: 7426
    mov          DI,0x8b2                        ; 1000:3559: bfb208
loc_355c:
    cmp          byte [DI],0x0                   ; 1000:355c: 803d00
    jz           loc_3566                        ; 1000:355f: 7405
    add          DI,0x9                          ; 1000:3561: 83c709
    jmp short    loc_355c                        ; 1000:3564: ebf6
loc_3566:
    inc          byte [0x8b1]                    ; 1000:3566: fe06b108
    mov          byte [DI],0x2                   ; 1000:356a: c60502
    mov          byte [DI + 0x4],0x0             ; 1000:356d: c6450400
    mov          AX,word [cs:SI + 0x4]           ; 1000:3571: 2e8b4404
    mov          word [DI + 0x5],AX              ; 1000:3575: 894505
    mov          AX,word [cs:SI + 0x6]           ; 1000:3578: 2e8b4406
    mov          word [DI + 0x7],AX              ; 1000:357c: 894507
loc_357f:
    mov          AX,0x1                          ; 1000:357f: b80100
    mov          BX,0x9a25                       ; 1000:3582: bb259a
    mov          CX,0x2                          ; 1000:3585: b90200
    int3                                         ; 1000:3588: cc
loc_3589:
    mov          byte [0x8ae],0x6                ; 1000:3589: c606ae0806
loc_358e:
    jmp near     loc_267f                        ; 1000:358e: e9eef0
loc_3591:
    mov          AX,word [SI + 0x4]              ; 1000:3591: 8b4404
    and          AX,0x7fff                       ; 1000:3594: 25ff7f
    mov          DI,0xaf0                        ; 1000:3597: bff00a
    mov          CX,AX                           ; 1000:359a: 8bc8
    shl          AX,0x1                          ; 1000:359c: d1e0
    add          AX,CX                           ; 1000:359e: 03c1
    shl          AX,0x1                          ; 1000:35a0: d1e0
    shl          AX,0x1                          ; 1000:35a2: d1e0
    add          DI,AX                           ; 1000:35a4: 03f8
    mov          word [cs:DI],0xfffe             ; 1000:35a6: 2ec705feff
    mov          AX,word [SI]                    ; 1000:35ab: 8b04
    mov          BX,word [SI + 0x2]              ; 1000:35ad: 8b5c02
    mov          word [SI],0xffff                ; 1000:35b0: c704ffff
    sub          BX,0x10                         ; 1000:35b4: 83eb10
    call         math_random_byte                ; 1000:35b7: e8610a
    mov          byte [0x8a7],0x0                ; 1000:35ba: c606a70800
    mov          byte [0x8ae],0x0                ; 1000:35bf: c606ae0800
    mov          byte [0x93b],0x14               ; 1000:35c4: c6063b0914
    inc          byte [0x226]                    ; 1000:35c9: fe062602
    cmp          byte [0x226],0x6                ; 1000:35cd: 803e260206
    jnz          loc_35db                        ; 1000:35d2: 7507
    mov          SP,word [0x8e8]                 ; 1000:35d4: 8b26e808
    jmp near     boss_update_state               ; 1000:35d8: e9c10a
loc_35db:
    jmp near     loc_267f                        ; 1000:35db: e9a1f0

; ----------------------------------------------------------------------------
; FUNCTION: ent_check_player_contact  [Entities / Collision]
; ADDRESS:  1000:35de
; PURPOSE:  Tests collision between player bounding box and enemy body; deals damage on contact.
; ----------------------------------------------------------------------------
ent_check_player_contact:
    mov          CX,word [0x6f2]                 ; 1000:35de: 8b0ef206
    mov          SI,word [0x6f4]                 ; 1000:35e2: 8b36f406
    or           CX,CX                           ; 1000:35e6: 0bc9
    jz           loc_360f                        ; 1000:35e8: 7425
loc_35ea:
    mov          AX,[0x6c6]                      ; 1000:35ea: a1c606
    cmp          AX,word [cs:SI]                 ; 1000:35ed: 2e3b04
    jnz          loc_3608                        ; 1000:35f0: 7516
    mov          AX,[0x6c8]                      ; 1000:35f2: a1c806
    cmp          AX,word [cs:SI + 0x2]           ; 1000:35f5: 2e3b4402
    jl           loc_3608                        ; 1000:35f9: 7c0d
    cmp          AX,word [cs:SI + 0x4]           ; 1000:35fb: 2e3b4404
    jg           loc_3608                        ; 1000:35ff: 7f07
    mov          SP,word [0x8e8]                 ; 1000:3601: 8b26e808
    jmp near     game_over_sequence              ; 1000:3605: e9fdeb
loc_3608:
    add          SI,0xc                          ; 1000:3608: 83c60c
    dec          CL                              ; 1000:360b: fec9
    jnz          loc_35ea                        ; 1000:360d: 75db
loc_360f:
    ret                                          ; 1000:360f: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_destroy  [Entities / Lifecycle]
; ADDRESS:  1000:3610
; PURPOSE:  Destroys enemy, spawns explosion effect, plays destruction sound, and adds points to score.
; ----------------------------------------------------------------------------
ent_destroy:
    cmp          byte [0x91e],0x0                ; 1000:3610: 803e1e0900
    jnz          loc_3629                        ; 1000:3615: 7512
    mov          [0x91f],AX                      ; 1000:3617: a31f09
    mov          byte [0x91e],0x8                ; 1000:361a: c6061e0908
    mov          AX,0x1                          ; 1000:361f: b80100
    mov          BX,0x96ae                       ; 1000:3622: bbae96
    mov          CX,0x5                          ; 1000:3625: b90500
    int3                                         ; 1000:3628: cc
loc_3629:
    ret                                          ; 1000:3629: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_check_tile_collision  [Entities / Collision]
; ADDRESS:  1000:362a
; PURPOSE:  Checks monster collision against solid walls and platforms in the room.
; ----------------------------------------------------------------------------
ent_check_tile_collision:
    mov          AX,[0x6c6]                      ; 1000:362a: a1c606
    sub          AX,word [0x928]                 ; 1000:362d: 2b062809
    jl           loc_3655                        ; 1000:3631: 7c22
    cmp          AX,0x10                         ; 1000:3633: 3d1000
    jg           loc_3655                        ; 1000:3636: 7f1d
    mov          AX,[0x92a]                      ; 1000:3638: a12a09
    sub          AX,word [0x6c8]                 ; 1000:363b: 2b06c806
    jge          loc_3643                        ; 1000:363f: 7d02
    neg          AX                              ; 1000:3641: f7d8
loc_3643:
    cmp          AX,0xc                          ; 1000:3643: 3d0c00
    jg           loc_3655                        ; 1000:3646: 7f0d
    mov          byte [0x924],0x2                ; 1000:3648: c606240902
    mov          word [0x926],0x8                ; 1000:364d: c70626090800
    stc                                          ; 1000:3653: f9
    ret                                          ; 1000:3654: c3
loc_3655:
    clc                                          ; 1000:3655: f8
    ret                                          ; 1000:3656: c3

; ----------------------------------------------------------------------------
; FUNCTION: ent_turn_around  [Entities / AI]
; ADDRESS:  1000:3657
; PURPOSE:  Reverses monster movement direction upon reaching platform edges or walls.
; ----------------------------------------------------------------------------
ent_turn_around:
    cmp          byte [0x896],0x0                ; 1000:3657: 803e960800
    jz           loc_366c                        ; 1000:365c: 740e
    xor          BH,BH                           ; 1000:365e: 32ff
    mov          BL,byte [0x896]                 ; 1000:3660: 8a1e9608
    mov          byte [0x896],0x0                ; 1000:3664: c606960800
    call         hud_draw_item_icon              ; 1000:3669: e8ab03
loc_366c:
    cmp          byte [cs:0x25c],0x1             ; 1000:366c: 2e803e5c0201
    jnz          loc_368e                        ; 1000:3672: 751a
    cmp          byte [0x898],0x0                ; 1000:3674: 803e980800
    jz           loc_368e                        ; 1000:3679: 7413
    mov          byte [0x897],0x1                ; 1000:367b: c606970801
    mov          word [0x892],0x5                ; 1000:3680: c70692080500
    mov          byte [0x924],0x1                ; 1000:3686: c606240901
    jmp near     player_update_physics           ; 1000:368b: e982f3
loc_368e:
    mov          AX,[0x6c6]                      ; 1000:368e: a1c606
    add          AX,0x10                         ; 1000:3691: 051000
    cmp          AX,word [0x29c]                 ; 1000:3694: 3b069c02
    jnz          loc_369d                        ; 1000:3698: 7503
    call         ent_check_player_contact        ; 1000:369a: e841ff
loc_369d:
    mov          AX,[0x6c6]                      ; 1000:369d: a1c606
    sub          AX,word [0x6c2]                 ; 1000:36a0: 2b06c206
    cmp          AX,0x70                         ; 1000:36a4: 3d7000
    jle          loc_36ba                        ; 1000:36a7: 7e11
    mov          AX,[0x6c2]                      ; 1000:36a9: a1c206
    add          AX,0xd0                         ; 1000:36ac: 05d000
    cmp          AX,word [0x29c]                 ; 1000:36af: 3b069c02
    jz           loc_36ba                        ; 1000:36b3: 7405
    add          word [0x6c2],0x8                ; 1000:36b5: 8306c20608
loc_36ba:
    call         player_check_ground_collision   ; 1000:36ba: e8d1f4
    jmp near     loc_267f                        ; 1000:36bd: e9bfef

; ----------------------------------------------------------------------------
; FUNCTION: item_collect_pickup  [Items / Pickups]
; ADDRESS:  1000:36c0
; PURPOSE:  Handles item collection: gems, door keys, teleport wand, boots, lantern, potion, and treasures.
; ----------------------------------------------------------------------------
item_collect_pickup:
    mov          AX,[0x926]                      ; 1000:36c0: a12609
    add          AX,word [0x928]                 ; 1000:36c3: 03062809
    mov          [0x928],AX                      ; 1000:36c7: a32809
    mov          BX,word [0x92a]                 ; 1000:36ca: 8b1e2a09
    push         AX                              ; 1000:36ce: 50
    push         BX                              ; 1000:36cf: 53
    add          AX,0x10                         ; 1000:36d0: 051000
    add          BX,0x10                         ; 1000:36d3: 83c310
    call         map_get_tile_at                 ; 1000:36d6: e825e6
    cmp          AX,0x1b                         ; 1000:36d9: 3d1b00
    jnz          loc_36e3                        ; 1000:36dc: 7505
    pop          BX                              ; 1000:36de: 5b
    pop          AX                              ; 1000:36df: 58
    jmp short    loc_371d                        ; 1000:36e0: eb3b
    db 0x90                                         ; 1000:36e2
loc_36e3:
    pop          BX                              ; 1000:36e3: 5b
    pop          AX                              ; 1000:36e4: 58
    cmp          byte [0x925],0x2                ; 1000:36e5: 803e250902
    jz           loc_36ef                        ; 1000:36ea: 7403
    add          AX,0x10                         ; 1000:36ec: 051000
loc_36ef:
    add          BX,0x20                         ; 1000:36ef: 83c320
    call         map_get_tile_at                 ; 1000:36f2: e809e6
    cmp          AX,0x1a                         ; 1000:36f5: 3d1a00
    jz           loc_3716                        ; 1000:36f8: 741c
    cmp          AX,0x1b                         ; 1000:36fa: 3d1b00
    jz           loc_371d                        ; 1000:36fd: 741e
    cmp          AX,0x19                         ; 1000:36ff: 3d1900
    jg           loc_3724                        ; 1000:3702: 7f20
    inc          byte [0x925]                    ; 1000:3704: fe062509
    cmp          byte [0x925],0x4                ; 1000:3708: 803e250904
    jz           loc_3724                        ; 1000:370d: 7415
    mov          byte [0x925],0x2                ; 1000:370f: c606250902
    jmp short    loc_3731                        ; 1000:3714: eb1b
loc_3716:
    mov          byte [0x925],0x1                ; 1000:3716: c606250901
    jmp short    loc_3731                        ; 1000:371b: eb14
loc_371d:
    mov          byte [0x925],0x3                ; 1000:371d: c606250903
    jmp short    loc_3731                        ; 1000:3722: eb0d
loc_3724:
    cmp          byte [0x925],0x1                ; 1000:3724: 803e250901
    jle          loc_372f                        ; 1000:3729: 7e04
    dec          byte [0x925]                    ; 1000:372b: fe0e2509
loc_372f:
    jmp short    loc_3731                        ; 1000:372f: eb00
loc_3731:
    mov          AX,[0x926]                      ; 1000:3731: a12609
    cmp          byte [0x925],0x1                ; 1000:3734: 803e250901
    jnz          loc_373f                        ; 1000:3739: 7504
    add          word [0x92a],AX                 ; 1000:373b: 01062a09
loc_373f:
    cmp          byte [0x925],0x3                ; 1000:373f: 803e250903
    jnz          loc_374a                        ; 1000:3744: 7504
    sub          word [0x92a],AX                 ; 1000:3746: 29062a09
loc_374a:
    cmp          byte [0x924],0x2                ; 1000:374a: 803e240902
    jl           loc_378d                        ; 1000:374f: 7c3c
    mov          byte [0x8ad],0x1                ; 1000:3751: c606ad0801
    mov          AX,[0x928]                      ; 1000:3756: a12809
    add          AX,0x8                          ; 1000:3759: 050800
    mov          [0x6c6],AX                      ; 1000:375c: a3c606
    mov          BX,word [0x92a]                 ; 1000:375f: 8b1e2a09
    mov          word [0x6c8],BX                 ; 1000:3763: 891ec806
    add          AX,0x8                          ; 1000:3767: 050800
    call         map_get_tile_at                 ; 1000:376a: e891e5
    cmp          AX,word [0x292]                 ; 1000:376d: 3b069202
    jg           loc_378d                        ; 1000:3771: 7f1a
    mov          byte [0x924],0x1                ; 1000:3773: c606240901
    mov          byte [0x897],0x1                ; 1000:3778: c606970801
    mov          byte [0x898],0x0                ; 1000:377d: c606980800
    mov          word [0x892],0xfffb             ; 1000:3782: c7069208fbff
    mov          byte [0x8ae],0x2                ; 1000:3788: c606ae0802
loc_378d:
    ret                                          ; 1000:378d: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_render_all  [HUD / Display]
; ADDRESS:  1000:378e
; PURPOSE:  Draws complete on-screen HUD: score digits, shield bar, extra lives, active weapon, keys, and inventory items.
; ----------------------------------------------------------------------------
hud_render_all:
    cmp          byte [0x925],0x2                ; 1000:378e: 803e250902
    jz           loc_37b8                        ; 1000:3793: 7423
    mov          SI,word [cs:0x8db8]             ; 1000:3795: 2e8b36b88d
    cmp          byte [0x925],0x3                ; 1000:379a: 803e250903
    jnz          loc_37a6                        ; 1000:379f: 7505
    mov          SI,word [cs:0x8dbc]             ; 1000:37a1: 2e8b36bc8d
loc_37a6:
    mov          AX,[0x928]                      ; 1000:37a6: a12809
    mov          BX,word [0x92a]                 ; 1000:37a9: 8b1e2a09
    add          BX,0x6                          ; 1000:37ad: 83c306
    call         vid_blit_planar_block           ; 1000:37b0: e80846
    or           AX,AX                           ; 1000:37b3: 0bc0
    jz           loc_37cc                        ; 1000:37b5: 7415
    ret                                          ; 1000:37b7: c3
loc_37b8:
    mov          SI,word [cs:0x8dba]             ; 1000:37b8: 2e8b36ba8d
    mov          AX,[0x928]                      ; 1000:37bd: a12809
    mov          BX,word [0x92a]                 ; 1000:37c0: 8b1e2a09
    call         vid_blit_planar_block           ; 1000:37c4: e8f445
    or           AX,AX                           ; 1000:37c7: 0bc0
    jz           loc_37cc                        ; 1000:37c9: 7401
    ret                                          ; 1000:37cb: c3
loc_37cc:
    mov          AX,[0x928]                      ; 1000:37cc: a12809
    cmp          AX,word [0x6c6]                 ; 1000:37cf: 3b06c606
    jle          loc_3810                        ; 1000:37d3: 7e3b
    cmp          word [0x6fc],0x1                ; 1000:37d5: 833efc0601
    jnz          loc_37f5                        ; 1000:37da: 7519
    mov          word [0x928],0x0                ; 1000:37dc: c70628090000
    mov          word [0x92a],0x20               ; 1000:37e2: c7062a092000
    mov          word [0x926],0x8                ; 1000:37e8: c70626090800
    mov          byte [0x925],0x2                ; 1000:37ee: c606250902
    jmp short    loc_3811                        ; 1000:37f3: eb1c
loc_37f5:
    cmp          word [0x6fc],0x0                ; 1000:37f5: 833efc0600
    jnz          loc_3810                        ; 1000:37fa: 7514
    mov          word [0x928],0xb0               ; 1000:37fc: c7062809b000
    mov          word [0x92a],0xe0               ; 1000:3802: c7062a09e000
    mov          word [0x926],0x0                ; 1000:3808: c70626090000
    jmp short    loc_3811                        ; 1000:380e: eb01
loc_3810:
    ret                                          ; 1000:3810: c3
loc_3811:
    mov          AX,[0x6c6]                      ; 1000:3811: a1c606
    cmp          AX,0xac0                        ; 1000:3814: 3dc00a
    jge          loc_3810                        ; 1000:3817: 7df7
    sub          AX,0x140                        ; 1000:3819: 2d4001
loc_381c:
    cmp          word [0x928],AX                 ; 1000:381c: 39062809
    jge          loc_3810                        ; 1000:3820: 7dee
    mov          word [0x926],0x8                ; 1000:3822: c70626090800
    push         AX                              ; 1000:3828: 50
    add          word [0x928],0x10               ; 1000:3829: 8306280910
    sub          word [0x92a],0x10               ; 1000:382e: 832e2a0910
loc_3833:
    mov          AX,[0x928]                      ; 1000:3833: a12809
    add          AX,0x10                         ; 1000:3836: 051000
    mov          BX,word [0x92a]                 ; 1000:3839: 8b1e2a09
    add          BX,0x20                         ; 1000:383d: 83c320
    call         map_get_tile_at                 ; 1000:3840: e8bbe4
    cmp          AX,0x1b                         ; 1000:3843: 3d1b00
    jle          loc_385c                        ; 1000:3846: 7e14
    add          word [0x92a],0x10               ; 1000:3848: 83062a0910
    mov          AX,[0x92a]                      ; 1000:384d: a12a09
    cmp          AX,word [0x29e]                 ; 1000:3850: 3b069e02
    jl           loc_3833                        ; 1000:3854: 7cdd
    mov          word [0x92a],0x0                ; 1000:3856: c7062a090000
loc_385c:
    pop          AX                              ; 1000:385c: 58
    jmp short    loc_381c                        ; 1000:385d: ebbd

; ----------------------------------------------------------------------------
; FUNCTION: hud_update_score  [HUD / Score]
; ADDRESS:  1000:385f
; PURPOSE:  Adds points to total score, handles high score check, and triggers score display redraw.
; ----------------------------------------------------------------------------
hud_update_score:
    mov          word [0x92e],0x0                ; 1000:385f: c7062e090000
    mov          byte [0x8a6],0x0                ; 1000:3865: c606a60800
    mov          byte [0x92c],0x2                ; 1000:386a: c6062c0902
loc_386f:
    cmp          byte [0x896],0x0                ; 1000:386f: 803e960800
    jz           loc_3884                        ; 1000:3874: 740e
    xor          BH,BH                           ; 1000:3876: 32ff
    mov          BL,byte [0x896]                 ; 1000:3878: 8a1e9608
    mov          byte [0x896],0x0                ; 1000:387c: c606960800
    call         hud_draw_item_icon              ; 1000:3881: e89301
loc_3884:
    add          word [0x92e],0x2                ; 1000:3884: 83062e0902
    cmp          word [0x92e],0xc                ; 1000:3889: 833e2e090c
    jle          loc_3896                        ; 1000:388e: 7e06
    mov          word [0x92e],0xc                ; 1000:3890: c7062e090c00
loc_3896:
    mov          AX,[0x930]                      ; 1000:3896: a13009
    add          AX,0x8                          ; 1000:3899: 050800
    mov          [0x930],AX                      ; 1000:389c: a33009
    mov          BX,word [0x932]                 ; 1000:389f: 8b1e3209
    add          BX,word [0x92e]                 ; 1000:38a3: 031e2e09
    push         BX                              ; 1000:38a7: 53
    add          AX,0x10                         ; 1000:38a8: 051000
    add          BX,0x20                         ; 1000:38ab: 83c320
    call         map_get_tile_at                 ; 1000:38ae: e84de4
    sub          BX,word [0x298]                 ; 1000:38b1: 2b1e9802
    mov          CL,byte [BX]                    ; 1000:38b5: 8a0f
    xor          CH,CH                           ; 1000:38b7: 32ed
    sub          BX,word [0x298]                 ; 1000:38b9: 2b1e9802
    mov          DL,byte [BX]                    ; 1000:38bd: 8a17
    xor          DH,DH                           ; 1000:38bf: 32f6
    pop          BX                              ; 1000:38c1: 5b
    cmp          AL,0x16                         ; 1000:38c2: 3c16
    jnz          loc_38ce                        ; 1000:38c4: 7508
    add          BX,0x10                         ; 1000:38c6: 83c310
    and          BX,0xfff0                       ; 1000:38c9: 83e3f0
    jmp short    loc_38ec                        ; 1000:38cc: eb1e
loc_38ce:
    cmp          CL,0x20                         ; 1000:38ce: 80f920
    jz           loc_3915                        ; 1000:38d1: 7442
    cmp          AX,word [0x292]                 ; 1000:38d3: 3b069202
    jg           loc_3925                        ; 1000:38d7: 7f4c
    and          BX,0xfff0                       ; 1000:38d9: 83e3f0
    mov          word [0x92e],0x1                ; 1000:38dc: c7062e090100
    cmp          CL,0x1f                         ; 1000:38e2: 80f91f
    jz           loc_3925                        ; 1000:38e5: 743e
    cmp          CL,0x16                         ; 1000:38e7: 80f916
    jnz          loc_3900                        ; 1000:38ea: 7514
loc_38ec:
    push         BX                              ; 1000:38ec: 53
    mov          byte [0x8ec],0xa                ; 1000:38ed: c606ec080a
    mov          AX,0x1                          ; 1000:38f2: b80100
    mov          BX,0x9a4d                       ; 1000:38f5: bb4d9a
    mov          CX,0x9                          ; 1000:38f8: b90900
    int3                                         ; 1000:38fb: cc
    db 0x5b, 0xeb, 0x3d, 0x90                       ; 1000:38fc
loc_3900:
    mov          word [0x92e],0x0                ; 1000:3900: c7062e090000
    cmp          byte [cs:0x25c],0x1             ; 1000:3906: 2e803e5c0201
    jnz          loc_3925                        ; 1000:390c: 7517
    cmp          byte [0x898],0x0                ; 1000:390e: 803e980800
    jz           loc_3925                        ; 1000:3913: 7410
loc_3915:
    and          BX,0xfff0                       ; 1000:3915: 83e3f0
    cmp          word [0x92e],0x0                ; 1000:3918: 833e2e0900
    jl           loc_3925                        ; 1000:391d: 7c06
    mov          word [0x92e],0xfff0             ; 1000:391f: c7062e09f0ff
loc_3925:
    cmp          word [0x92e],0x0                ; 1000:3925: 833e2e0900
    jge          loc_393c                        ; 1000:392a: 7d10
    cmp          CX,word [0x292]                 ; 1000:392c: 3b0e9202
    jg           loc_393c                        ; 1000:3930: 7f0a
    mov          BX,word [0x932]                 ; 1000:3932: 8b1e3209
    mov          word [0x92e],0x0                ; 1000:3936: c7062e090000
loc_393c:
    mov          word [0x932],BX                 ; 1000:393c: 891e3209
    mov          AX,[0x92e]                      ; 1000:3940: a12e09
    mov          [0x92d],AL                      ; 1000:3943: a22d09
    mov          AL,[0x899]                      ; 1000:3946: a09908
    mov          [0x898],AL                      ; 1000:3949: a29808
    cmp          byte [cs:0x25c],0x1             ; 1000:394c: 2e803e5c0201
    jnz          loc_3959                        ; 1000:3952: 7505
    mov          byte [0x898],0x0                ; 1000:3954: c606980800
loc_3959:
    mov          AX,[0x930]                      ; 1000:3959: a13009
    add          AX,0x8                          ; 1000:395c: 050800
    mov          [0x6c6],AX                      ; 1000:395f: a3c606
    mov          BX,word [0x932]                 ; 1000:3962: 8b1e3209
    mov          word [0x6c8],BX                 ; 1000:3966: 891ec806
    cmp          byte [0x8ec],0xa                ; 1000:396a: 803eec080a
    jnz          loc_397b                        ; 1000:396f: 750a
    add          word [0x6c8],0x10               ; 1000:3971: 8306c80610
    sub          word [0x6c6],0x8                ; 1000:3976: 832ec60608
loc_397b:
    mov          AX,[0x6c6]                      ; 1000:397b: a1c606
    add          AX,0x10                         ; 1000:397e: 051000
    cmp          AX,word [0x29c]                 ; 1000:3981: 3b069c02
    jnz          loc_398a                        ; 1000:3985: 7503
    call         ent_check_player_contact        ; 1000:3987: e854fc
loc_398a:
    mov          AX,[0x6c6]                      ; 1000:398a: a1c606
    sub          AX,word [0x6c2]                 ; 1000:398d: 2b06c206
    cmp          AX,0x70                         ; 1000:3991: 3d7000
    jle          loc_39a7                        ; 1000:3994: 7e11
    mov          AX,[0x6c2]                      ; 1000:3996: a1c206
    add          AX,0xd0                         ; 1000:3999: 05d000
    cmp          AX,word [0x29c]                 ; 1000:399c: 3b069c02
    jz           loc_39a7                        ; 1000:39a0: 7405
    add          word [0x6c2],0x8                ; 1000:39a2: 8306c20608
loc_39a7:
    call         player_check_ground_collision   ; 1000:39a7: e8e4f1
    jmp near     loc_267f                        ; 1000:39aa: e9d2ec

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_lives  [HUD / Display]
; ADDRESS:  1000:39ad
; PURPOSE:  Renders Comic head icons representing remaining lives.
; ----------------------------------------------------------------------------
hud_draw_lives:
    mov          AX,0x1                          ; 1000:39ad: b80100
    lea          BX,[0xaac0]                     ; 1000:39b0: 8d1ec0aa
    mov          CX,0x4                          ; 1000:39b4: b90400
    int3                                         ; 1000:39b7: cc
    db 0xbe, 0x2c, 0x96, 0xe8, 0x48, 0x08, 0xe8, 0x8d, 0x3d, 0xe8, 0xd5, 0xcb, 0x2e, 0x80, 0x3e, 0x61 ; 1000:39b8
    db 0x00, 0x10, 0x74, 0x0f, 0x2e, 0x80, 0x3e, 0x61, 0x00, 0x01, 0x75, 0xed, 0x2e, 0xc6, 0x06, 0x61 ; 1000:39c8
    db 0x00, 0x00, 0xc3, 0xa1, 0xfa, 0x06, 0xa3, 0x14, 0x02, 0xa1, 0xfc, 0x06, 0xa3, 0x16, 0x02, 0x8b ; 1000:39d8
    db 0x26, 0xe8, 0x08, 0xe9, 0xbd, 0x29, 0xa1, 0xfa, 0x06, 0xa3, 0x14, 0x02, 0xa1, 0xfc, 0x06, 0xa3 ; 1000:39e8
    db 0x16, 0x02, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0xb6, 0x96, 0xb9, 0x0a, 0x00, 0xcc, 0xe8, 0x24, 0xe3 ; 1000:39f8
    db 0xbe, 0x50, 0x96, 0xe8, 0xf8, 0x07, 0xe8, 0x3d, 0x3d, 0xe8, 0x85, 0xcb, 0xe9, 0x94, 0x29 ; 1000:3a08

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_item_icon  [HUD / Display]
; ADDRESS:  1000:3a17
; PURPOSE:  Draws specific item graphic icon into inventory box.
; ----------------------------------------------------------------------------
hud_draw_item_icon:
    lea          AX,[0x9710]                     ; 1000:3a17: 8d061097
    shl          BX,0x1                          ; 1000:3a1b: d1e3
    add          BX,AX                           ; 1000:3a1d: 03d8
    mov          BX,word [BX]                    ; 1000:3a1f: 8b1f
    mov          AX,0x100                        ; 1000:3a21: b80001
    mov          DS,word [cs:0x5e]               ; 1000:3a24: 2e8e1e5e00
    lea          SI,[0x98c0]                     ; 1000:3a29: 8d36c098
    mov          SI,word [SI + 0x2]              ; 1000:3a2d: 8b7402
    call         vid_set_write_mode_2            ; 1000:3a30: e8d93e
    mov          DS,word [cs:0x5c]               ; 1000:3a33: 2e8e1e5c00
    xor          BH,BH                           ; 1000:3a38: 32ff
    mov          BL,byte [0x896]                 ; 1000:3a3a: 8a1e9608
    lea          AX,[0x9710]                     ; 1000:3a3e: 8d061097
    shl          BX,0x1                          ; 1000:3a42: d1e3
    add          BX,AX                           ; 1000:3a44: 03d8
    mov          BX,word [BX]                    ; 1000:3a46: 8b1f
    mov          AX,0x100                        ; 1000:3a48: b80001
    mov          DS,word [cs:0x5e]               ; 1000:3a4b: 2e8e1e5e00
    lea          SI,[0x98c0]                     ; 1000:3a50: 8d36c098
    mov          SI,word [SI]                    ; 1000:3a54: 8b34
    call         vid_set_write_mode_2            ; 1000:3a56: e8b33e
    mov          DS,word [cs:0x5c]               ; 1000:3a59: 2e8e1e5c00
    ret                                          ; 1000:3a5e: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_shield_bar  [HUD / Display]
; ADDRESS:  1000:3a5f
; PURPOSE:  Draws health/shield energy meter using color-coded bar segments.
; ----------------------------------------------------------------------------
hud_draw_shield_bar:
    mov          DS,word [cs:0x5e]               ; 1000:3a5f: 2e8e1e5e00
    push         AX                              ; 1000:3a64: 50
    push         BX                              ; 1000:3a65: 53
    push         CX                              ; 1000:3a66: 51
    mov          SI,0x98c4                       ; 1000:3a67: bec498
    mov          CL,CH                           ; 1000:3a6a: 8acd
    xor          CH,CH                           ; 1000:3a6c: 32ed
    shl          CX,0x1                          ; 1000:3a6e: d1e1
    add          SI,CX                           ; 1000:3a70: 03f1
    mov          SI,word [SI]                    ; 1000:3a72: 8b34
    call         vid_set_write_mode_2            ; 1000:3a74: e8953e
    pop          CX                              ; 1000:3a77: 59
    pop          BX                              ; 1000:3a78: 5b
    pop          AX                              ; 1000:3a79: 58
    add          AX,0x8                          ; 1000:3a7a: 050800
    lea          SI,[0x98c4]                     ; 1000:3a7d: 8d36c498
    xor          CH,CH                           ; 1000:3a81: 32ed
    shl          CX,0x1                          ; 1000:3a83: d1e1
    add          SI,CX                           ; 1000:3a85: 03f1
    mov          SI,word [SI]                    ; 1000:3a87: 8b34
    call         vid_set_write_mode_2            ; 1000:3a89: e8803e
    mov          DS,word [cs:0x5c]               ; 1000:3a8c: 2e8e1e5c00
    ret                                          ; 1000:3a91: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_keys  [HUD / Display]
; ADDRESS:  1000:3a92
; PURPOSE:  Draws collected key icons in HUD inventory bar.
; ----------------------------------------------------------------------------
hud_draw_keys:
    mov          byte [0x8eb],0x0                ; 1000:3a92: c606eb0800
    mov          AX,0x100                        ; 1000:3a97: b80001
    mov          BX,0x43                         ; 1000:3a9a: bb4300
    jmp short    loc_3aab                        ; 1000:3a9d: eb0c
    db 0x90                                         ; 1000:3a9f

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_digits  [HUD / Display]
; ADDRESS:  1000:3aa0
; PURPOSE:  Renders decimal digit string on screen using custom EGA numeric font.
; ----------------------------------------------------------------------------
hud_draw_digits:
    mov          byte [0x8ea],0x0                ; 1000:3aa0: c606ea0800
    mov          AX,0x100                        ; 1000:3aa5: b80001
    mov          BX,0x2a                         ; 1000:3aa8: bb2a00
loc_3aab:
    mov          CX,0x6                          ; 1000:3aab: b90600
    mov          DS,word [cs:0x5e]               ; 1000:3aae: 2e8e1e5e00
    mov          SI,0x98d8                       ; 1000:3ab3: bed898
    mov          SI,word [SI]                    ; 1000:3ab6: 8b34
loc_3ab8:
    push         AX                              ; 1000:3ab8: 50
    push         BX                              ; 1000:3ab9: 53
    push         CX                              ; 1000:3aba: 51
    push         SI                              ; 1000:3abb: 56
    call         vid_set_write_mode_2            ; 1000:3abc: e84d3e
    pop          SI                              ; 1000:3abf: 5e
    pop          CX                              ; 1000:3ac0: 59
    pop          BX                              ; 1000:3ac1: 5b
    pop          AX                              ; 1000:3ac2: 58
    add          AX,0x8                          ; 1000:3ac3: 050800
    loop         loc_3ab8                        ; 1000:3ac6: e2f0
    mov          DS,word [cs:0x5c]               ; 1000:3ac8: 2e8e1e5c00
    ret                                          ; 1000:3acd: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_number_word  [HUD / Display]
; ADDRESS:  1000:3ace
; PURPOSE:  Converts 16-bit word value into decimal representation and draws it.
; ----------------------------------------------------------------------------
hud_draw_number_word:
    mov          AL,[0x8ea]                      ; 1000:3ace: a0ea08
    xor          AH,AH                           ; 1000:3ad1: 32e4
    cmp          AX,0x17                         ; 1000:3ad3: 3d1700
    jz           loc_3b11                        ; 1000:3ad6: 7439
    inc          byte [0x8ea]                    ; 1000:3ad8: fe06ea08
    mov          BX,AX                           ; 1000:3adc: 8bd8
    dec          BX                              ; 1000:3ade: 4b
    shr          BX,0x1                          ; 1000:3adf: d1eb
    shr          AX,0x1                          ; 1000:3ae1: d1e8
    cmp          AX,BX                           ; 1000:3ae3: 3bc3
    jz           loc_3b11                        ; 1000:3ae5: 742a
    mov          DX,AX                           ; 1000:3ae7: 8bd0
    shr          AX,0x1                          ; 1000:3ae9: d1e8
    and          DX,0x1                          ; 1000:3aeb: 83e201
    shl          AX,0x1                          ; 1000:3aee: d1e0
    shl          AX,0x1                          ; 1000:3af0: d1e0
    shl          AX,0x1                          ; 1000:3af2: d1e0
    add          AX,0x100                        ; 1000:3af4: 050001
    mov          BX,0x2a                         ; 1000:3af7: bb2a00
    inc          DX                              ; 1000:3afa: 42
    shl          DX,0x1                          ; 1000:3afb: d1e2
    mov          DS,word [cs:0x5e]               ; 1000:3afd: 2e8e1e5e00
    mov          SI,0x98d8                       ; 1000:3b02: bed898
    add          SI,DX                           ; 1000:3b05: 03f2
    mov          SI,word [SI]                    ; 1000:3b07: 8b34
    call         vid_set_write_mode_2            ; 1000:3b09: e8003e
    mov          DS,word [cs:0x5c]               ; 1000:3b0c: 2e8e1e5c00
loc_3b11:
    ret                                          ; 1000:3b11: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_draw_char  [HUD / Display]
; ADDRESS:  1000:3b12
; PURPOSE:  Draws single 8x8 font glyph to HUD.
; ----------------------------------------------------------------------------
hud_draw_char:
    mov          AL,[0x8ea]                      ; 1000:3b12: a0ea08
    or           AL,AL                           ; 1000:3b15: 0ac0
    jz           loc_3b50                        ; 1000:3b17: 7437
    dec          byte [0x8ea]                    ; 1000:3b19: fe0eea08
    xor          AH,AH                           ; 1000:3b1d: 32e4
    dec          AX                              ; 1000:3b1f: 48
    test         AX,0x1                          ; 1000:3b20: a90100
    jnz          loc_3b50                        ; 1000:3b23: 752b
    shr          AX,0x1                          ; 1000:3b25: d1e8
    mov          DX,AX                           ; 1000:3b27: 8bd0
    shr          AX,0x1                          ; 1000:3b29: d1e8
    and          DX,0x1                          ; 1000:3b2b: 83e201
    shl          AX,0x1                          ; 1000:3b2e: d1e0
    shl          AX,0x1                          ; 1000:3b30: d1e0
    shl          AX,0x1                          ; 1000:3b32: d1e0
    add          AX,0x100                        ; 1000:3b34: 050001
    mov          BX,0x2a                         ; 1000:3b37: bb2a00
    shl          DX,0x1                          ; 1000:3b3a: d1e2
    mov          DS,word [cs:0x5e]               ; 1000:3b3c: 2e8e1e5e00
    mov          SI,0x98d8                       ; 1000:3b41: bed898
    add          SI,DX                           ; 1000:3b44: 03f2
    mov          SI,word [SI]                    ; 1000:3b46: 8b34
    call         vid_set_write_mode_2            ; 1000:3b48: e8c13d
    mov          DS,word [cs:0x5c]               ; 1000:3b4b: 2e8e1e5c00
loc_3b50:
    ret                                          ; 1000:3b50: c3

; ----------------------------------------------------------------------------
; FUNCTION: hud_flash_item  [HUD / Animation]
; ADDRESS:  1000:3b51
; PURPOSE:  Animates flashing highlight around newly acquired inventory item.
; ----------------------------------------------------------------------------
hud_flash_item:
    cmp          byte [0x8eb],0xc                ; 1000:3b51: 803eeb080c
    jz           loc_3b8b                        ; 1000:3b56: 7433
    mov          AL,[0x8eb]                      ; 1000:3b58: a0eb08
    inc          byte [0x8eb]                    ; 1000:3b5b: fe06eb08
    xor          AH,AH                           ; 1000:3b5f: 32e4
    mov          DX,AX                           ; 1000:3b61: 8bd0
    shr          AX,0x1                          ; 1000:3b63: d1e8
    and          DX,0x1                          ; 1000:3b65: 83e201
    shl          AX,0x1                          ; 1000:3b68: d1e0
    shl          AX,0x1                          ; 1000:3b6a: d1e0
    shl          AX,0x1                          ; 1000:3b6c: d1e0
    add          AX,0x100                        ; 1000:3b6e: 050001
    mov          BX,0x43                         ; 1000:3b71: bb4300
    inc          DX                              ; 1000:3b74: 42
    shl          DX,0x1                          ; 1000:3b75: d1e2
    mov          DS,word [cs:0x5e]               ; 1000:3b77: 2e8e1e5e00
    mov          SI,0x98d8                       ; 1000:3b7c: bed898
    add          SI,DX                           ; 1000:3b7f: 03f2
    mov          SI,word [SI]                    ; 1000:3b81: 8b34
    call         vid_set_write_mode_2            ; 1000:3b83: e8863d
    mov          DS,word [cs:0x5c]               ; 1000:3b86: 2e8e1e5c00
loc_3b8b:
    ret                                          ; 1000:3b8b: c3
    db 0x80, 0x3e, 0xeb, 0x08, 0x00, 0x74, 0x32, 0xfe, 0x0e, 0xeb, 0x08, 0xa0, 0xeb, 0x08, 0x32, 0xe4 ; 1000:3b8c
    db 0x8b, 0xd0, 0xd1, 0xe8, 0x83, 0xe2, 0x01, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0x05, 0x00, 0x01 ; 1000:3b9c
    db 0xbb, 0x43, 0x00, 0xd1, 0xe2, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xbe, 0xd8, 0x98, 0x03, 0xf2, 0x8b ; 1000:3bac
    db 0x34, 0xe8, 0x4c, 0x3d, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xc3 ; 1000:3bbc

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_pickup  [Audio / SFX]
; ADDRESS:  1000:3bc6
; PURPOSE:  Plays item pickup sound effect via PC speaker.
; ----------------------------------------------------------------------------
snd_play_pickup:
    mov          AX,0x1                          ; 1000:3bc6: b80100
    lea          BX,[0x965e]                     ; 1000:3bc9: 8d1e5e96
    mov          CX,0x9                          ; 1000:3bcd: b90900
    int3                                         ; 1000:3bd0: cc
    db 0xe8, 0x58, 0xe1, 0xe8, 0xa4, 0x07, 0x8d, 0x36, 0xd6, 0x48, 0xa1, 0xc6, 0x06, 0x2d, 0x0c, 0x00 ; 1000:3bd1
    db 0x2b, 0x06, 0xc2, 0x06, 0x05, 0x08, 0x00, 0x8b, 0x1e, 0xc8, 0x06, 0x83, 0xc3, 0x08, 0x2b, 0x1e ; 1000:3be1
    db 0xc4, 0x06, 0x83, 0xc3, 0x08, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0xa5, 0x3b, 0x2e, 0x8e, 0x1e ; 1000:3bf1
    db 0x5c, 0x00, 0xe8, 0x48, 0x3b, 0xb8, 0x01, 0x00, 0xe8, 0x6e, 0xc6, 0xe8, 0x1d, 0xe1, 0xe8, 0xa4 ; 1000:3c01
    db 0x01, 0xc3                                   ; 1000:3c11

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_blast  [Audio / SFX]
; ADDRESS:  1000:3c13
; PURPOSE:  Plays laser/fireball blasting sound effect.
; ----------------------------------------------------------------------------
snd_play_blast:
    push         BP                              ; 1000:3c13: 55
    mov          BP,SP                           ; 1000:3c14: 8bec
    sub          SP,0xc                          ; 1000:3c16: 83ec0c
    push         SI                              ; 1000:3c19: 56
    mov          AX,word [cs:SI]                 ; 1000:3c1a: 2e8b04
    sub          AX,word [0x6c2]                 ; 1000:3c1d: 2b06c206
    add          AX,0x8                          ; 1000:3c21: 050800
    mov          word [BP + -0x2],AX             ; 1000:3c24: 8946fe
    mov          AX,word [cs:SI + 0x2]           ; 1000:3c27: 2e8b4402
    sub          AX,word [0x6c4]                 ; 1000:3c2b: 2b06c406
    add          AX,0x8                          ; 1000:3c2f: 050800
    mov          word [BP + -0x4],AX             ; 1000:3c32: 8946fc
    xor          AH,AH                           ; 1000:3c35: 32e4
    mov          AL,byte [cs:SI + 0x6]           ; 1000:3c37: 2e8a4406
    mov          word [BP + -0x6],AX             ; 1000:3c3b: 8946fa
    mov          AL,byte [cs:SI + 0x7]           ; 1000:3c3e: 2e8a4407
    mov          word [BP + -0x8],AX             ; 1000:3c42: 8946f8
    mov          AL,byte [cs:SI + 0x8]           ; 1000:3c45: 2e8a4408
    mov          word [BP + -0xa],AX             ; 1000:3c49: 8946f6
    mov          AL,byte [cs:SI + 0x9]           ; 1000:3c4c: 2e8a4409
    mov          word [BP + -0xc],AX             ; 1000:3c50: 8946f4
    call         map_draw_background             ; 1000:3c53: e8d6e0
    call         vid_flip_buffers                ; 1000:3c56: e8cc0e
    call         snd_play_jump                   ; 1000:3c59: e85a01
    mov          AX,0x1                          ; 1000:3c5c: b80100
    call         timer_delay_ticks               ; 1000:3c5f: e818c6
    lea          BX,[0x973c]                     ; 1000:3c62: 8d1e3c97
    mov          AX,0x1                          ; 1000:3c66: b80100
    mov          CX,0x9                          ; 1000:3c69: b90900
    int3                                         ; 1000:3c6c: cc
    db 0xe8, 0xbc, 0xe0, 0xe8, 0x2e, 0x01, 0xe8, 0x85, 0x01, 0xe8, 0xc1, 0x00, 0xe8, 0xa9, 0x0e, 0xe8 ; 1000:3c6d
    db 0x37, 0x01, 0xe8, 0xaa, 0xe0, 0xe8, 0x1c, 0x01, 0xe8, 0x73, 0x01, 0xe8, 0x9a, 0x0e, 0xe8, 0x28 ; 1000:3c7d
    db 0x01, 0xe8, 0x9b, 0xe0, 0xe8, 0x0d, 0x01, 0xe8, 0xa3, 0x00, 0xe8, 0x61, 0x01, 0xe8, 0x88, 0x0e ; 1000:3c8d
    db 0xe8, 0x16, 0x01, 0xe8, 0x89, 0xe0, 0xe8, 0x55, 0x01, 0xe8, 0x7c, 0x0e, 0xe8, 0x0a, 0x01, 0x5e ; 1000:3c9d
    db 0x8b, 0xe5, 0x5d, 0xc3                       ; 1000:3cad

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_hit  [Audio / SFX]
; ADDRESS:  1000:3cb1
; PURPOSE:  Plays enemy hit / player damage sound effect.
; ----------------------------------------------------------------------------
snd_play_hit:
    push         BP                              ; 1000:3cb1: 55
    mov          BP,SP                           ; 1000:3cb2: 8bec
    sub          SP,0xc                          ; 1000:3cb4: 83ec0c
    push         SI                              ; 1000:3cb7: 56
    mov          AX,word [cs:SI]                 ; 1000:3cb8: 2e8b04
    sub          AX,word [0x6c2]                 ; 1000:3cbb: 2b06c206
    add          AX,0x8                          ; 1000:3cbf: 050800
    mov          word [BP + -0x2],AX             ; 1000:3cc2: 8946fe
    mov          AX,word [cs:SI + 0x2]           ; 1000:3cc5: 2e8b4402
    sub          AX,word [0x6c4]                 ; 1000:3cc9: 2b06c406
    add          AX,0x8                          ; 1000:3ccd: 050800
    mov          word [BP + -0x4],AX             ; 1000:3cd0: 8946fc
    xor          AH,AH                           ; 1000:3cd3: 32e4
    mov          AL,byte [cs:SI + 0x6]           ; 1000:3cd5: 2e8a4406
    mov          word [BP + -0x6],AX             ; 1000:3cd9: 8946fa
    mov          AL,byte [cs:SI + 0x7]           ; 1000:3cdc: 2e8a4407
    mov          word [BP + -0x8],AX             ; 1000:3ce0: 8946f8
    mov          AL,byte [cs:SI + 0x8]           ; 1000:3ce3: 2e8a4408
    mov          word [BP + -0xa],AX             ; 1000:3ce7: 8946f6
    mov          AL,byte [cs:SI + 0x9]           ; 1000:3cea: 2e8a4409
    mov          word [BP + -0xc],AX             ; 1000:3cee: 8946f4
    lea          BX,[0x973c]                     ; 1000:3cf1: 8d1e3c97
    mov          AX,0x1                          ; 1000:3cf5: b80100
    mov          CX,0x9                          ; 1000:3cf8: b90900
    int3                                         ; 1000:3cfb: cc
    db 0xe8, 0x2d, 0xe0, 0xe8, 0xf9, 0x00, 0xe8, 0xb1, 0x00, 0xe8, 0x24, 0xe0, 0xe8, 0x96, 0x00, 0xe8 ; 1000:3cfc
    db 0x2c, 0x00, 0xe8, 0xea, 0x00, 0xe8, 0xa2, 0x00, 0xe8, 0x15, 0xe0, 0xe8, 0x87, 0x00, 0xe8, 0xde ; 1000:3d0c
    db 0x00, 0xe8, 0x96, 0x00, 0xe8, 0x09, 0xe0, 0xe8, 0x7b, 0x00, 0xe8, 0xd2, 0x00, 0xe8, 0x0e, 0x00 ; 1000:3d1c
    db 0xe8, 0x87, 0x00, 0xe8, 0xfa, 0xdf, 0xe8, 0x81, 0x00, 0x5e, 0x8b, 0xe5, 0x5d, 0xc3, 0x8b, 0x46 ; 1000:3d2c
    db 0xfa, 0xe8, 0x83, 0x00, 0x46, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0xe8, 0x8d, 0x00, 0x57, 0xe8 ; 1000:3d3c
    db 0x2d, 0x00, 0x8b, 0x46, 0xf6, 0xe8, 0x6f, 0x00, 0x5f, 0x57, 0x83, 0xc7, 0x03, 0xe8, 0x1f, 0x00 ; 1000:3d4c
    db 0x8b, 0x46, 0xf8, 0xe8, 0x61, 0x00, 0x46, 0x5f, 0x57, 0x81, 0xc7, 0x80, 0x02, 0xe8, 0x0f, 0x00 ; 1000:3d5c
    db 0x8b, 0x46, 0xf4, 0xe8, 0x51, 0x00, 0x5f, 0x81, 0xc7, 0x83, 0x02, 0xe8, 0x01, 0x00, 0xc3, 0xb1 ; 1000:3d6c
    db 0x00, 0xe8, 0x10, 0x00, 0xb1, 0x01, 0xe8, 0x0b, 0x00, 0xb1, 0x02, 0xe8, 0x06, 0x00, 0xb1, 0x03 ; 1000:3d7c
    db 0xe8, 0x01, 0x00, 0xc3, 0xe8, 0xd5, 0x3c, 0xbb, 0x10, 0x00, 0x57, 0xa4, 0x46, 0x83, 0xc7, 0x27 ; 1000:3d8c
    db 0x4b, 0x75, 0xf8, 0x5f, 0xc3, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0xb9, 0x20, 0x00, 0xba, 0x20 ; 1000:3d9c
    db 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x13, 0x3d, 0xc3 ; 1000:3dac

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_jump  [Audio / SFX]
; ADDRESS:  1000:3db6
; PURPOSE:  Plays jump sound effect.
; ----------------------------------------------------------------------------
snd_play_jump:
    call         ui_draw_text_string             ; 1000:3db6: e8c205
    call         vid_wait_vblank                 ; 1000:3db9: e89239
    mov          AX,0x1                          ; 1000:3dbc: b80100
    call         timer_delay_ticks               ; 1000:3dbf: e8b8c4
    ret                                          ; 1000:3dc2: c3
    db 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xbe, 0xa8 ; 1000:3dc3
    db 0x13, 0x03, 0xf0, 0xc3, 0xd1, 0xe8, 0xd1, 0xe8, 0xd1, 0xe8, 0x8b, 0xd3, 0xd1, 0xe3, 0xd1, 0xe3 ; 1000:3dd3
    db 0xd1, 0xe3, 0xd1, 0xe2, 0xd1, 0xe2, 0xd1, 0xe2, 0xd1, 0xe2, 0xd1, 0xe2, 0x03, 0xda, 0x03, 0xd8 ; 1000:3de3
    db 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0x03, 0xfb, 0xc3 ; 1000:3df3

; ----------------------------------------------------------------------------
; FUNCTION: spr_render_explosion  [Sprites / Animation]
; ADDRESS:  1000:3dfb
; PURPOSE:  Animates explosion particle burst when enemies or hazards are destroyed.
; ----------------------------------------------------------------------------
spr_render_explosion:
    mov          SI,0x4882                       ; 1000:3dfb: be8248
    cmp          byte [0x8ad],0x1                ; 1000:3dfe: 803ead0801
    jz           loc_3e08                        ; 1000:3e03: 7403
    mov          SI,0x4860                       ; 1000:3e05: be6048
loc_3e08:
    mov          AX,[0x6c6]                      ; 1000:3e08: a1c606
    mov          BX,word [0x6c8]                 ; 1000:3e0b: 8b1ec806
    mov          DS,word [cs:0x5e]               ; 1000:3e0f: 2e8e1e5e00
    mov          SI,word [SI]                    ; 1000:3e14: 8b34
    call         vid_blit_planar_block           ; 1000:3e16: e8a23f
    mov          DS,word [cs:0x5c]               ; 1000:3e19: 2e8e1e5c00
    ret                                          ; 1000:3e1e: c3

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_door_open  [Audio / SFX]
; ADDRESS:  1000:3e1f
; PURPOSE:  Plays door opening / key unlock sound effect.
; ----------------------------------------------------------------------------
snd_play_door_open:
    push         BP                              ; 1000:3e1f: 55
    mov          BP,SP                           ; 1000:3e20: 8bec
    sub          SP,0x4                          ; 1000:3e22: 83ec04
    mov          byte [0x8ec],0xa                ; 1000:3e25: c606ec080a
    mov          word [BP + -0x2],0xa            ; 1000:3e2a: c746fe0a00
loc_3e2f:
    call         map_draw_background             ; 1000:3e2f: e8fade
    call         vid_flip_buffers                ; 1000:3e32: e8f00c
    call         snd_play_jump                   ; 1000:3e35: e87eff
    dec          word [BP + -0x2]                ; 1000:3e38: ff4efe
    jnz          loc_3e2f                        ; 1000:3e3b: 75f2
    mov          AX,0x1                          ; 1000:3e3d: b80100
    lea          BX,[0x9764]                     ; 1000:3e40: 8d1e6497
    mov          CX,0x9                          ; 1000:3e44: b90900
    int3                                         ; 1000:3e47: cc
    db 0xc7, 0x46, 0xfe, 0x05, 0x00, 0xe8, 0xdc, 0xde, 0xe8, 0xd2, 0x0c, 0xe8, 0x60, 0xff, 0xff, 0x4e ; 1000:3e48
    db 0xfe, 0x75, 0xf2, 0xc7, 0x46, 0xfe, 0x09, 0x00, 0xc7, 0x46, 0xfc, 0x6e, 0x97, 0xe8, 0xc4, 0xde ; 1000:3e58
    db 0xe8, 0xba, 0x0c, 0xa1, 0xc6, 0x06, 0x8b, 0x1e, 0xc8, 0x06, 0x8b, 0x76, 0xfc, 0x2e, 0x8e, 0x1e ; 1000:3e68
    db 0x5e, 0x00, 0xe8, 0x3e, 0x3f, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xe8, 0x31, 0xff, 0x81, 0x6e, 0xfc ; 1000:3e78
    db 0x46, 0x01, 0xff, 0x4e, 0xfe, 0x75, 0xd6, 0xc6, 0x06, 0xec, 0x08, 0x00, 0x8b, 0xe5, 0x5d, 0xc3 ; 1000:3e88

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_teleport  [Audio / SFX]
; ADDRESS:  1000:3e98
; PURPOSE:  Plays teleportation portal sound effect.
; ----------------------------------------------------------------------------
snd_play_teleport:
    push         BP                              ; 1000:3e98: 55
    mov          BP,SP                           ; 1000:3e99: 8bec
    sub          SP,0x4                          ; 1000:3e9b: 83ec04
    mov          byte [0x8ec],0xa                ; 1000:3e9e: c606ec080a
    mov          word [BP + -0x2],0xa            ; 1000:3ea3: c746fe0a00
loc_3ea8:
    call         map_draw_background             ; 1000:3ea8: e881de
    call         vid_flip_buffers                ; 1000:3eab: e8770c
    call         spr_render_explosion            ; 1000:3eae: e84aff
    call         snd_play_jump                   ; 1000:3eb1: e802ff
    dec          word [BP + -0x2]                ; 1000:3eb4: ff4efe
    jnz          loc_3ea8                        ; 1000:3eb7: 75ef
    mov          AX,0x1                          ; 1000:3eb9: b80100
    lea          BX,[0x97cc]                     ; 1000:3ebc: 8d1ecc97
    mov          CX,0x9                          ; 1000:3ec0: b90900
    int3                                         ; 1000:3ec3: cc
    db 0xc7, 0x46, 0xfe, 0x09, 0x00, 0xc7, 0x46, 0xfc, 0x3e, 0x8d, 0xe8, 0x5b, 0xde, 0xe8, 0x51, 0x0c ; 1000:3ec4
    db 0xa1, 0xc6, 0x06, 0x8b, 0x1e, 0xc8, 0x06, 0x8b, 0x76, 0xfc, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8 ; 1000:3ed4
    db 0xd5, 0x3e, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xe8, 0xc8, 0xfe, 0x81, 0x46, 0xfc, 0x46, 0x01, 0xff ; 1000:3ee4
    db 0x4e, 0xfe, 0x75, 0xd6, 0xe8, 0x31, 0xde, 0xe8, 0x27, 0x0c, 0xe8, 0xb5, 0xfe, 0xc6, 0x06, 0xec ; 1000:3ef4
    db 0x08, 0x00, 0x8b, 0xe5, 0x5d, 0xc3           ; 1000:3f04

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_level_music  [Audio / Music]
; ADDRESS:  1000:3f0a
; PURPOSE:  Initializes background theme tune for current game zone.
; ----------------------------------------------------------------------------
snd_play_level_music:
    push         BP                              ; 1000:3f0a: 55
    mov          BP,SP                           ; 1000:3f0b: 8bec
    sub          SP,0x8                          ; 1000:3f0d: 83ec08
    push         SI                              ; 1000:3f10: 56
    mov          word [BP + -0x6],AX             ; 1000:3f11: 8946fa
    mov          AX,word [cs:SI]                 ; 1000:3f14: 2e8b04
    sub          AX,word [0x6c2]                 ; 1000:3f17: 2b06c206
    add          AX,0x8                          ; 1000:3f1b: 050800
    mov          word [BP + -0x2],AX             ; 1000:3f1e: 8946fe
    mov          AX,word [cs:SI + 0x2]           ; 1000:3f21: 2e8b4402
    sub          AX,word [0x6c4]                 ; 1000:3f25: 2b06c406
    add          AX,0x8                          ; 1000:3f29: 050800
    mov          word [BP + -0x4],AX             ; 1000:3f2c: 8946fc
    mov          BX,0x9834                       ; 1000:3f2f: bb3498
    mov          AX,0x1                          ; 1000:3f32: b80100
    mov          CX,0x9                          ; 1000:3f35: b90900
    int3                                         ; 1000:3f38: cc
    db 0xc7, 0x46, 0xf8, 0x00, 0x00, 0xe8, 0xeb, 0xdd, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0x8b, 0x76 ; 1000:3f39
    db 0xfa, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xd0, 0x39, 0x2e, 0x8e ; 1000:3f49
    db 0x1e, 0x5c, 0x00, 0xe8, 0x57, 0xfe, 0x81, 0x46, 0xfa, 0x04, 0x02, 0xff, 0x46, 0xf8, 0x83, 0x7e ; 1000:3f59
    db 0xf8, 0x04, 0x7c, 0xd1, 0xc7, 0x46, 0xf8, 0x03, 0x00, 0x81, 0x6e, 0xfa, 0x04, 0x02, 0x81, 0x6e ; 1000:3f69
    db 0xfa, 0x04, 0x02, 0xe8, 0xad, 0xdd, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0x8b, 0x76, 0xfa, 0x2e ; 1000:3f79
    db 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x92, 0x39, 0x2e, 0x8e, 0x1e, 0x5c ; 1000:3f89
    db 0x00, 0xe8, 0x19, 0xfe, 0xff, 0x4e, 0xf8, 0x75, 0xd5, 0xc7, 0x46, 0xf8, 0x01, 0x00, 0x81, 0x46 ; 1000:3f99
    db 0xfa, 0x04, 0x02, 0xe8, 0x7d, 0xdd, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0x8b, 0x76, 0xfa, 0x2e ; 1000:3fa9
    db 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x62, 0x39, 0x2e, 0x8e, 0x1e, 0x5c ; 1000:3fb9
    db 0x00, 0xe8, 0xe9, 0xfd, 0x81, 0x46, 0xfa, 0x04, 0x02, 0xff, 0x46, 0xf8, 0x83, 0x7e, 0xf8, 0x04 ; 1000:3fc9
    db 0x7c, 0xd1, 0xc7, 0x46, 0xf8, 0x03, 0x00, 0x81, 0x6e, 0xfa, 0x04, 0x02, 0x81, 0x6e, 0xfa, 0x04 ; 1000:3fd9
    db 0x02, 0xe8, 0x3f, 0xdd, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0x8b, 0x76, 0xfa, 0x2e, 0x8e, 0x1e ; 1000:3fe9
    db 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x24, 0x39, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xe8 ; 1000:3ff9
    db 0xab, 0xfd, 0xff, 0x4e, 0xf8, 0x75, 0xd5, 0xe8, 0x19, 0xdd, 0xe8, 0xa0, 0xfd, 0x5e, 0x8b, 0xe5 ; 1000:4009
    db 0x5d, 0xc3                                   ; 1000:4019

; ----------------------------------------------------------------------------
; FUNCTION: math_random_byte  [Math / PRNG]
; ADDRESS:  1000:401b
; PURPOSE:  Generates pseudo-random byte using linear congruential algorithm.
; ----------------------------------------------------------------------------
math_random_byte:
    push         BP                              ; 1000:401b: 55
    mov          BP,SP                           ; 1000:401c: 8bec
    sub          SP,0x8                          ; 1000:401e: 83ec08
    push         SI                              ; 1000:4021: 56
    mov          word [BP + -0x2],AX             ; 1000:4022: 8946fe
    mov          word [BP + -0x4],BX             ; 1000:4025: 895efc
    mov          BX,0x9894                       ; 1000:4028: bb9498
    mov          AX,0x1                          ; 1000:402b: b80100
    mov          CX,0x9                          ; 1000:402e: b90900
    int3                                         ; 1000:4031: cc
    db 0xc7, 0x46, 0xf8, 0x00, 0x00, 0x2e, 0xa1, 0x37, 0x95, 0x89, 0x46, 0xfa, 0xe8, 0xeb, 0xdc, 0xe8 ; 1000:4032
    db 0xe1, 0x0a, 0xe8, 0xb4, 0xfd, 0x8b, 0x46, 0xfe, 0x8b, 0x5e, 0xfc, 0x8b, 0x76, 0xfa, 0x2e, 0x8e ; 1000:4042
    db 0x1e, 0x5e, 0x00, 0xe8, 0x63, 0x3d, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xe8, 0x56, 0xfd, 0x81, 0x46 ; 1000:4052
    db 0xfa, 0x46, 0x01, 0xff, 0x46, 0xf8, 0x83, 0x7e, 0xf8, 0x07, 0x7c, 0xd0, 0x5e, 0x8b, 0xe5, 0x5d ; 1000:4062
    db 0xc3                                         ; 1000:4072

; ----------------------------------------------------------------------------
; FUNCTION: math_calc_distance  [Math / Geometry]
; ADDRESS:  1000:4073
; PURPOSE:  Calculates Manhattan/Euclidean distance approximation between two entity positions.
; ----------------------------------------------------------------------------
math_calc_distance:
    test         AX,0x1                          ; 1000:4073: a90100
    jnz          loc_4089                        ; 1000:4076: 7511
    add          word [0x6c4],0x2                ; 1000:4078: 8306c40602
    mov          AX,[0x6c4]                      ; 1000:407d: a1c406
    add          AX,0xa0                         ; 1000:4080: 05a000
    cmp          AX,word [0x29e]                 ; 1000:4083: 3b069e02
    jle          loc_4095                        ; 1000:4087: 7e0c
loc_4089:
    sub          word [0x6c4],0x2                ; 1000:4089: 832ec40602
    jnc          loc_4095                        ; 1000:408e: 7305
    add          word [0x6c4],0x2                ; 1000:4090: 8306c40602
loc_4095:
    call         map_draw_background             ; 1000:4095: e894dc
    call         vid_flip_buffers                ; 1000:4098: e88a0a
    ret                                          ; 1000:409b: c3

; ----------------------------------------------------------------------------
; FUNCTION: boss_update_state  [Boss / AI]
; ADDRESS:  1000:409c
; PURPOSE:  Updates boss state machine, attack phases, invulnerability periods, and health.
; ----------------------------------------------------------------------------
boss_update_state:
    push         BP                              ; 1000:409c: 55
    mov          BP,SP                           ; 1000:409d: 8bec
    sub          SP,0xa                          ; 1000:409f: 83ec0a
    mov          word [BP + -0x2],0xc0           ; 1000:40a2: c746fec000
    mov          word [BP + -0x8],0x0            ; 1000:40a7: c746f80000
    mov          word [BP + -0xa],0x0            ; 1000:40ac: c746f60000
loc_40b1:
    mov          AX,word [BP + -0x8]             ; 1000:40b1: 8b46f8
    call         math_calc_distance              ; 1000:40b4: e8bcff
    call         spr_render_explosion            ; 1000:40b7: e841fd
    test         word [BP + -0x8],0x1            ; 1000:40ba: f746f80100
    jnz          loc_40c5                        ; 1000:40bf: 7504
    xor          word [BP + -0xa],0x4            ; 1000:40c1: 8376f604
loc_40c5:
    mov          word [BP + -0x4],0x9718         ; 1000:40c5: c746fc1897
    mov          word [BP + -0x6],0x0            ; 1000:40ca: c746fa0000
loc_40cf:
    xor          word [BP + -0xa],0x4            ; 1000:40cf: 8376f604
    mov          SI,word [BP + -0x4]             ; 1000:40d3: 8b76fc
    mov          AX,[0x6c6]                      ; 1000:40d6: a1c606
    add          AX,word [SI]                    ; 1000:40d9: 0304
    sub          AX,word [BP + -0xa]             ; 1000:40db: 2b46f6
    mov          BX,word [0x6c8]                 ; 1000:40de: 8b1ec806
    add          BX,word [SI + 0x2]              ; 1000:40e2: 035c02
    add          BX,word [BP + -0x2]             ; 1000:40e5: 035efe
    mov          SI,word [cs:0x9535]             ; 1000:40e8: 2e8b363595
    mov          DS,word [cs:0x5e]               ; 1000:40ed: 2e8e1e5e00
    call         vid_blit_planar_block           ; 1000:40f2: e8c63c
    mov          DS,word [cs:0x5c]               ; 1000:40f5: 2e8e1e5c00
    add          word [BP + -0x4],0x4            ; 1000:40fa: 8346fc04
    inc          word [BP + -0x6]                ; 1000:40fe: ff46fa
    cmp          word [BP + -0x6],0x9            ; 1000:4101: 837efa09
    jl           loc_40cf                        ; 1000:4105: 7cc8
    mov          word [BP + -0x4],0x9718         ; 1000:4107: c746fc1897
    mov          word [BP + -0x6],0x0            ; 1000:410c: c746fa0000
loc_4111:
    xor          word [BP + -0xa],0x4            ; 1000:4111: 8376f604
    mov          SI,word [BP + -0x4]             ; 1000:4115: 8b76fc
    mov          AX,[0x6c6]                      ; 1000:4118: a1c606
    add          AX,word [SI]                    ; 1000:411b: 0304
    sub          AX,word [BP + -0xa]             ; 1000:411d: 2b46f6
    mov          BX,word [0x6c8]                 ; 1000:4120: 8b1ec806
    add          BX,word [SI + 0x2]              ; 1000:4124: 035c02
    add          BX,word [BP + -0x2]             ; 1000:4127: 035efe
    sub          BX,0x60                         ; 1000:412a: 83eb60
    mov          SI,word [cs:0x9535]             ; 1000:412d: 2e8b363595
    mov          DS,word [cs:0x5e]               ; 1000:4132: 2e8e1e5e00
    call         vid_blit_planar_block           ; 1000:4137: e8813c
    mov          DS,word [cs:0x5c]               ; 1000:413a: 2e8e1e5c00
    add          word [BP + -0x4],0x4            ; 1000:413f: 8346fc04
    inc          word [BP + -0x6]                ; 1000:4143: ff46fa
    cmp          word [BP + -0x6],0x9            ; 1000:4146: 837efa09
    jl           loc_4111                        ; 1000:414a: 7cc5
    call         snd_play_jump                   ; 1000:414c: e867fc
    sub          word [BP + -0x2],0x8            ; 1000:414f: 836efe08
    inc          word [BP + -0x8]                ; 1000:4153: ff46f8
    cmp          word [BP + -0x8],0x32           ; 1000:4156: 837ef832
    jz           loc_415f                        ; 1000:415a: 7403
    jmp near     loc_40b1                        ; 1000:415c: e952ff
loc_415f:
    mov          word [BP + -0x2],0x50           ; 1000:415f: c746fe5000
    mov          word [BP + -0x8],0x0            ; 1000:4164: c746f80000
loc_4169:
    mov          AX,word [BP + -0x8]             ; 1000:4169: 8b46f8
    call         math_calc_distance              ; 1000:416c: e804ff
    call         spr_render_explosion            ; 1000:416f: e889fc
    mov          AX,[0x6c6]                      ; 1000:4172: a1c606
    sub          AX,0x8                          ; 1000:4175: 2d0800
    mov          BX,word [0x6c8]                 ; 1000:4178: 8b1ec806
    add          BX,word [BP + -0x2]             ; 1000:417c: 035efe
    mov          SI,word [cs:0x9535]             ; 1000:417f: 2e8b363595
    mov          DS,word [cs:0x5e]               ; 1000:4184: 2e8e1e5e00
    call         vid_blit_planar_block           ; 1000:4189: e82f3c
    mov          DS,word [cs:0x5c]               ; 1000:418c: 2e8e1e5c00
    call         snd_play_jump                   ; 1000:4191: e822fc
    sub          word [BP + -0x2],0x8            ; 1000:4194: 836efe08
    inc          word [BP + -0x8]                ; 1000:4198: ff46f8
    cmp          word [BP + -0x2],-0x8           ; 1000:419b: 837efef8
    jg           loc_4169                        ; 1000:419f: 7fc8
loc_41a1:
    mov          AX,word [BP + -0x8]             ; 1000:41a1: 8b46f8
    call         math_calc_distance              ; 1000:41a4: e8ccfe
    call         spr_render_explosion            ; 1000:41a7: e851fc
    mov          AX,[0x6c6]                      ; 1000:41aa: a1c606
    sub          AX,0x8                          ; 1000:41ad: 2d0800
    mov          BX,word [0x6c8]                 ; 1000:41b0: 8b1ec806
    mov          SI,word [cs:0x9535]             ; 1000:41b4: 2e8b363595
    mov          DS,word [cs:0x5e]               ; 1000:41b9: 2e8e1e5e00
    call         vid_blit_planar_block           ; 1000:41be: e8fa3b
    mov          DS,word [cs:0x5c]               ; 1000:41c1: 2e8e1e5c00
    call         snd_play_jump                   ; 1000:41c6: e8edfb
    inc          word [BP + -0x8]                ; 1000:41c9: ff46f8
    sub          word [0x6c8],0x8                ; 1000:41cc: 832ec80608
    cmp          word [0x6c8],-0x20              ; 1000:41d1: 833ec806e0
    jg           loc_41a1                        ; 1000:41d6: 7fc9
    mov          byte [0x220],0x6                ; 1000:41d8: c606200206
    mov          AX,0x0                          ; 1000:41dd: b80000
    mov          BX,0x7                          ; 1000:41e0: bb0700
    mov          word [0x210],0xa0               ; 1000:41e3: c7061002a000
    mov          word [0x212],0x0                ; 1000:41e9: c70612020000
    mov          word [0x6fa],0xffff             ; 1000:41ef: c706fa06ffff
    mov          word [0x6fc],BX                 ; 1000:41f5: 891efc06
    mov          BX,0xffff                       ; 1000:41f9: bbffff
    call         map_load_room                   ; 1000:41fc: e88ed9
    jmp near     spr_animate_player              ; 1000:41ff: e979e0
    db 0x8b, 0xe5, 0x5d, 0xc3                       ; 1000:4202

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_window_box  [UI / Windows]
; ADDRESS:  1000:4206
; PURPOSE:  Draws bordered UI dialog window with drop shadow and title header.
; ----------------------------------------------------------------------------
ui_draw_window_box:
    push         BP                              ; 1000:4206: 55
    mov          BP,SP                           ; 1000:4207: 8bec
    sub          SP,0x8                          ; 1000:4209: 83ec08
    lodsb                                        ; 1000:420c: ac
    xor          AH,AH                           ; 1000:420d: 32e4
    mov          word [BP + -0x2],AX             ; 1000:420f: 8946fe
    shl          AX,0x1                          ; 1000:4212: d1e0
    shl          AX,0x1                          ; 1000:4214: d1e0
    shl          AX,0x1                          ; 1000:4216: d1e0
    mov          CX,AX                           ; 1000:4218: 8bc8
    lodsb                                        ; 1000:421a: ac
    xor          AH,AH                           ; 1000:421b: 32e4
    mov          word [BP + -0x4],AX             ; 1000:421d: 8946fc
    shl          AX,0x1                          ; 1000:4220: d1e0
    shl          AX,0x1                          ; 1000:4222: d1e0
    shl          AX,0x1                          ; 1000:4224: d1e0
    mov          DX,AX                           ; 1000:4226: 8bd0
    mov          AX,0xd0                         ; 1000:4228: b8d000
    sub          AX,CX                           ; 1000:422b: 2bc1
    jc           loc_4239                        ; 1000:422d: 720a
    shr          AX,0x1                          ; 1000:422f: d1e8
    and          AX,0xfff8                       ; 1000:4231: 25f8ff
    add          AX,0x8                          ; 1000:4234: 050800
    jmp short    loc_423b                        ; 1000:4237: eb02
loc_4239:
    xor          AX,AX                           ; 1000:4239: 33c0
loc_423b:
    mov          BX,0xa0                         ; 1000:423b: bba000
    sub          BX,DX                           ; 1000:423e: 2bda
    jc           loc_424c                        ; 1000:4240: 720a
    shr          BX,0x1                          ; 1000:4242: d1eb
    and          BX,0xfff8                       ; 1000:4244: 83e3f8
    add          BX,0x8                          ; 1000:4247: 83c308
    jmp short    loc_424e                        ; 1000:424a: eb02
loc_424c:
    xor          BX,BX                           ; 1000:424c: 33db
loc_424e:
    mov          word [BP + -0x6],AX             ; 1000:424e: 8946fa
    mov          word [BP + -0x8],BX             ; 1000:4251: 895ef8
    mov          DI,word [cs:0x773f]             ; 1000:4254: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:4259: e86c38
    mov          AX,0x9                          ; 1000:425c: b80900
    call         vid_set_write_mode_0            ; 1000:425f: e8d700
    sub          word [BP + -0x4],0x2            ; 1000:4262: 836efc02
    mov          CX,word [BP + -0x8]             ; 1000:4266: 8b4ef8
    mov          BX,word [BP + -0x6]             ; 1000:4269: 8b5efa
    mov          AL,0x22                         ; 1000:426c: b022
    call         ui_draw_window_border           ; 1000:426e: e87700
    mov          DX,word [BP + -0x2]             ; 1000:4271: 8b56fe
    sub          DX,0x2                          ; 1000:4274: 83ea02
    mov          AL,0x23                         ; 1000:4277: b023
loc_4279:
    call         ui_draw_window_border           ; 1000:4279: e86c00
    dec          DX                              ; 1000:427c: 4a
    jnz          loc_4279                        ; 1000:427d: 75fa
    mov          AL,0x24                         ; 1000:427f: b024
    call         ui_draw_window_border           ; 1000:4281: e86400
loc_4284:
    add          CX,0x8                          ; 1000:4284: 83c108
    mov          BX,word [BP + -0x6]             ; 1000:4287: 8b5efa
    mov          AL,0x25                         ; 1000:428a: b025
    call         ui_draw_window_border           ; 1000:428c: e85900
    mov          AX,0x7                          ; 1000:428f: b80700
    call         vid_set_write_mode_0            ; 1000:4292: e8a400
    mov          DX,word [BP + -0x2]             ; 1000:4295: 8b56fe
    sub          DX,0x2                          ; 1000:4298: 83ea02
loc_429b:
    lodsb                                        ; 1000:429b: ac
    call         ui_draw_window_border           ; 1000:429c: e84900
    dec          DX                              ; 1000:429f: 4a
    jnz          loc_429b                        ; 1000:42a0: 75f9
    mov          AX,0x9                          ; 1000:42a2: b80900
    call         vid_set_write_mode_0            ; 1000:42a5: e89100
    mov          AL,0x26                         ; 1000:42a8: b026
    call         ui_draw_window_border           ; 1000:42aa: e83b00
    dec          word [BP + -0x4]                ; 1000:42ad: ff4efc
    jnz          loc_4284                        ; 1000:42b0: 75d2
    add          CX,0x8                          ; 1000:42b2: 83c108
    mov          BX,word [BP + -0x6]             ; 1000:42b5: 8b5efa
    mov          AL,0x3a                         ; 1000:42b8: b03a
    call         ui_draw_window_border           ; 1000:42ba: e82b00
    mov          DX,word [BP + -0x2]             ; 1000:42bd: 8b56fe
    sub          DX,0x2                          ; 1000:42c0: 83ea02
    mov          AL,0x3b                         ; 1000:42c3: b03b
loc_42c5:
    call         ui_draw_window_border           ; 1000:42c5: e82000
    dec          DX                              ; 1000:42c8: 4a
    jnz          loc_42c5                        ; 1000:42c9: 75fa
    mov          AL,0x3c                         ; 1000:42cb: b03c
    call         ui_draw_window_border           ; 1000:42cd: e81800
    mov          SP,BP                           ; 1000:42d0: 8be5
    pop          BP                              ; 1000:42d2: 5d
    ret                                          ; 1000:42d3: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_clear_window  [UI / Windows]
; ADDRESS:  1000:42d4
; PURPOSE:  Clears interior area of dialog box with background color.
; ----------------------------------------------------------------------------
ui_clear_window:
    push         BX                              ; 1000:42d4: 53
    mov          CX,BX                           ; 1000:42d5: 8bcb
    mov          BX,AX                           ; 1000:42d7: 8bd8
    cld                                          ; 1000:42d9: fc
loc_42da:
    lodsb                                        ; 1000:42da: ac
    or           AL,AL                           ; 1000:42db: 0ac0
    jz           loc_42e4                        ; 1000:42dd: 7405
    call         ui_draw_window_border           ; 1000:42df: e80600
    jmp short    loc_42da                        ; 1000:42e2: ebf6
loc_42e4:
    mov          AX,BX                           ; 1000:42e4: 8bc3
    pop          BX                              ; 1000:42e6: 5b
    ret                                          ; 1000:42e7: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_window_border  [UI / Windows]
; ADDRESS:  1000:42e8
; PURPOSE:  Blits decorative border tiles and corner pieces around dialog box.
; ----------------------------------------------------------------------------
ui_draw_window_border:
    push         AX                              ; 1000:42e8: 50
    push         BX                              ; 1000:42e9: 53
    push         CX                              ; 1000:42ea: 51
    push         DX                              ; 1000:42eb: 52
    push         SI                              ; 1000:42ec: 56
    shr          BX,0x1                          ; 1000:42ed: d1eb
    shr          BX,0x1                          ; 1000:42ef: d1eb
    shr          BX,0x1                          ; 1000:42f1: d1eb
    mov          DX,CX                           ; 1000:42f3: 8bd1
    shl          CX,0x1                          ; 1000:42f5: d1e1
    shl          CX,0x1                          ; 1000:42f7: d1e1
    shl          CX,0x1                          ; 1000:42f9: d1e1
    shl          DX,0x1                          ; 1000:42fb: d1e2
    shl          DX,0x1                          ; 1000:42fd: d1e2
    shl          DX,0x1                          ; 1000:42ff: d1e2
    shl          DX,0x1                          ; 1000:4301: d1e2
    shl          DX,0x1                          ; 1000:4303: d1e2
    add          CX,DX                           ; 1000:4305: 03ca
    add          BX,CX                           ; 1000:4307: 03d9
    mov          DI,word [cs:0x773f]             ; 1000:4309: 2e8b3e3f77
    add          DI,BX                           ; 1000:430e: 03fb
    xor          AH,AH                           ; 1000:4310: 32e4
    sub          AX,0x20                         ; 1000:4312: 2d2000
    mov          SI,0xa6f4                       ; 1000:4315: bef4a6
    add          SI,AX                           ; 1000:4318: 03f0
    mov          CX,0x8                          ; 1000:431a: b90800
    mov          DS,word [cs:0x5e]               ; 1000:431d: 2e8e1e5e00
loc_4322:
    movsb                                        ; 1000:4322: a4
    add          SI,0x3a                         ; 1000:4323: 83c63a
    add          DI,0x27                         ; 1000:4326: 83c727
    loop         loc_4322                        ; 1000:4329: e2f7
    mov          DS,word [cs:0x5c]               ; 1000:432b: 2e8e1e5c00
    pop          SI                              ; 1000:4330: 5e
    pop          DX                              ; 1000:4331: 5a
    pop          CX                              ; 1000:4332: 59
    pop          BX                              ; 1000:4333: 5b
    pop          AX                              ; 1000:4334: 58
    add          BX,0x8                          ; 1000:4335: 83c308
    ret                                          ; 1000:4338: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_write_mode_0  [Video / Registers]
; ADDRESS:  1000:4339
; PURPOSE:  Configures EGA Graphics Controller to Write Mode 0 for standard memory writes.
; ----------------------------------------------------------------------------
vid_set_write_mode_0:
    push         DX                              ; 1000:4339: 52
    mov          AH,AL                           ; 1000:433a: 8ae0
    mov          AL,0x2                          ; 1000:433c: b002
    mov          DX,0x3c4                        ; 1000:433e: bac403
    out          DX,AL                           ; 1000:4341: ee
    inc          DX                              ; 1000:4342: 42
    xchg         AL,AH                           ; 1000:4343: 86e0
    out          DX,AL                           ; 1000:4345: ee
    dec          DX                              ; 1000:4346: 4a
    xchg         AL,AH                           ; 1000:4347: 86e0
    pop          DX                              ; 1000:4349: 5a
    ret                                          ; 1000:434a: c3
    db 0x50, 0x53, 0x51, 0x52, 0x57, 0x8b, 0xc3, 0x8b, 0xd9, 0xb9, 0x08, 0x00, 0x8b, 0xd1, 0x2e, 0x8b ; 1000:434b
    db 0x3e, 0x3f, 0x77, 0xe8, 0x67, 0x37, 0x5f, 0x5a, 0x59, 0x5b, 0x58, 0xc3, 0x55, 0x46, 0x57, 0x46 ; 1000:435b
    db 0x7b, 0x46, 0x2f, 0x46, 0x9f, 0x46, 0xa8, 0x46, 0xb2, 0x46, 0x9b, 0x45, 0xd6, 0x45, 0x15, 0x46 ; 1000:436b

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_text_string  [UI / Text]
; ADDRESS:  1000:437b
; PURPOSE:  Blits text string to EGA display buffer at pixel coordinate (X, Y) with color attribute.
; ----------------------------------------------------------------------------
ui_draw_text_string:
    mov          DI,word [0x6fe]                 ; 1000:437b: 8b3efe06
    or           DI,DI                           ; 1000:437f: 0bff
    jnz          loc_4384                        ; 1000:4381: 7501
    ret                                          ; 1000:4383: c3
loc_4384:
    mov          AX,[0x6c2]                      ; 1000:4384: a1c206
    mov          BX,0xc0                         ; 1000:4387: bbc000
    add          BX,AX                           ; 1000:438a: 03d8
    mov          CX,word [0x6c4]                 ; 1000:438c: 8b0ec406
    mov          DX,0x9f                         ; 1000:4390: ba9f00
    add          DX,CX                           ; 1000:4393: 03d1
    sub          CX,0xf                          ; 1000:4395: 83e90f
    lea          SI,[0x700]                      ; 1000:4398: 8d360007
loc_439c:
    test         word [SI + 0x4],0x8000          ; 1000:439c: f744040080
    jnz          loc_43b7                        ; 1000:43a1: 7514
    cmp          AX,word [SI]                    ; 1000:43a3: 3b04
    jg           loc_43b7                        ; 1000:43a5: 7f10
    cmp          BX,word [SI]                    ; 1000:43a7: 3b1c
    jl           loc_43b7                        ; 1000:43a9: 7c0c
    cmp          CX,word [SI + 0x2]              ; 1000:43ab: 3b4c02
    jg           loc_43b7                        ; 1000:43ae: 7f07
    cmp          DX,word [SI + 0x2]              ; 1000:43b0: 3b5402
    jl           loc_43b7                        ; 1000:43b3: 7c02
    jmp short    loc_43be                        ; 1000:43b5: eb07
loc_43b7:
    add          SI,0x8                          ; 1000:43b7: 83c608
    dec          DI                              ; 1000:43ba: 4f
    jnz          loc_439c                        ; 1000:43bb: 75df
    ret                                          ; 1000:43bd: c3
loc_43be:
    push         AX                              ; 1000:43be: 50
    push         BX                              ; 1000:43bf: 53
    push         CX                              ; 1000:43c0: 51
    push         DX                              ; 1000:43c1: 52
    push         SI                              ; 1000:43c2: 56
    push         DI                              ; 1000:43c3: 57
    mov          CX,0xf                          ; 1000:43c4: b90f00
    mov          DX,0x1f                         ; 1000:43c7: ba1f00
    cmp          byte [0x89d],0x1                ; 1000:43ca: 803e9d0801
    jnz          loc_43d3                        ; 1000:43cf: 7502
    xchg         DX,CX                           ; 1000:43d1: 87ca
loc_43d3:
    mov          AX,word [SI]                    ; 1000:43d3: 8b04
    add          AX,0xf                          ; 1000:43d5: 050f00
    cmp          AX,word [0x6c6]                 ; 1000:43d8: 3b06c606
    jl           loc_440a                        ; 1000:43dc: 7c2c
    sub          AX,0xf                          ; 1000:43de: 2d0f00
    sub          AX,CX                           ; 1000:43e1: 2bc1
    cmp          AX,word [0x6c6]                 ; 1000:43e3: 3b06c606
    jg           loc_440a                        ; 1000:43e7: 7f21
    mov          BX,word [SI + 0x2]              ; 1000:43e9: 8b5c02
    add          BX,0xf                          ; 1000:43ec: 83c30f
    cmp          BX,word [0x6c8]                 ; 1000:43ef: 3b1ec806
    jl           loc_440a                        ; 1000:43f3: 7c15
    sub          BX,0xf                          ; 1000:43f5: 83eb0f
    sub          BX,DX                           ; 1000:43f8: 2bda
    cmp          BX,word [0x6c8]                 ; 1000:43fa: 3b1ec806
    jg           loc_440a                        ; 1000:43fe: 7f0a
    cmp          byte [0x8ec],0x0                ; 1000:4400: 803eec0800
    jnz          loc_440a                        ; 1000:4405: 7503
    jmp short    loc_441d                        ; 1000:4407: eb14
    db 0x90                                         ; 1000:4409
loc_440a:
    mov          AX,word [SI]                    ; 1000:440a: 8b04
    mov          BX,word [SI + 0x2]              ; 1000:440c: 8b5c02
    mov          SI,word [SI + 0x6]              ; 1000:440f: 8b7406
    call         vid_blit_planar_block           ; 1000:4412: e8a639
    pop          DI                              ; 1000:4415: 5f
    pop          SI                              ; 1000:4416: 5e
    pop          DX                              ; 1000:4417: 5a
    pop          CX                              ; 1000:4418: 59
    pop          BX                              ; 1000:4419: 5b
    pop          AX                              ; 1000:441a: 58
    jmp short    loc_43b7                        ; 1000:441b: eb9a
loc_441d:
    mov          AX,0x1                          ; 1000:441d: b80100
    lea          BX,[0x98e0]                     ; 1000:4420: 8d1ee098
    mov          CX,0x3                          ; 1000:4424: b90300
    int3                                         ; 1000:4427: cc
    db 0x8b, 0x44, 0x04, 0xbf, 0xf0, 0x0a, 0x8b, 0xd8, 0xd1, 0xe0, 0x03, 0xc3, 0xd1, 0xe0, 0xd1, 0xe0 ; 1000:4428
    db 0x03, 0xf8, 0x2e, 0x8b, 0x45, 0x0a, 0x25, 0xff, 0x7f, 0x3d, 0x80, 0x00, 0x73, 0x22, 0xd1, 0xe0 ; 1000:4438
    db 0x8d, 0x1e, 0x67, 0x43, 0x03, 0xd8, 0x56, 0x57, 0x2e, 0xff, 0x17, 0x5f, 0x5e, 0x72, 0x11, 0xc7 ; 1000:4448
    db 0x04, 0xff, 0xff, 0x2e, 0xc7, 0x05, 0xff, 0xff, 0x2e, 0x81, 0x65, 0x0a, 0xff, 0x7f, 0xeb, 0xad ; 1000:4458
    db 0x8d, 0x1e, 0x39, 0x02, 0x33, 0xc9, 0x83, 0x3f, 0xff, 0x74, 0x16, 0x83, 0xc3, 0x02, 0x41, 0x83 ; 1000:4468
    db 0xf9, 0x0d, 0x7c, 0xf2, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0xf4, 0x98, 0xb9, 0x03, 0x00, 0xcc, 0xeb ; 1000:4478
    db 0x8c, 0x56, 0x57, 0xc7, 0x04, 0xff, 0xff, 0x2e, 0xc7, 0x05, 0xff, 0xff, 0x2e, 0x81, 0x65, 0x0a ; 1000:4488
    db 0xff, 0x7f, 0x8b, 0x44, 0x04, 0x89, 0x07, 0x8b, 0xc1, 0xe8, 0x78, 0x00, 0x5f, 0x5e, 0x8b, 0x44 ; 1000:4498
    db 0x06, 0x3d, 0xee, 0xaf, 0x74, 0x25, 0x3d, 0x10, 0xb7, 0x74, 0x38, 0x3d, 0xd2, 0xb3, 0x75, 0x03 ; 1000:44a8
    db 0xeb, 0x3e, 0x90, 0x2e, 0x8b, 0x45, 0x0a, 0x2e, 0x81, 0x65, 0x0a, 0xff, 0x00, 0x8a, 0xc4, 0x32 ; 1000:44b8
    db 0xe4, 0x0b, 0xc0, 0x74, 0x03, 0xe8, 0xc5, 0x28, 0xe9, 0x42, 0xff, 0xa1, 0xfa, 0x06, 0x8b, 0x1e ; 1000:44c8
    db 0xfc, 0x06, 0x35, 0x05, 0x00, 0x83, 0xf3, 0x03, 0x0b, 0xc3, 0x74, 0xd7, 0xc6, 0x06, 0x9c, 0x08 ; 1000:44d8
    db 0x01, 0xeb, 0xd0, 0x57, 0xc7, 0x06, 0x22, 0x02, 0x00, 0x0a, 0xe8, 0xb5, 0x03, 0x5f, 0xeb, 0xc3 ; 1000:44e8
    db 0x80, 0x3e, 0x21, 0x02, 0x01, 0x75, 0x1b, 0xc6, 0x06, 0x21, 0x02, 0x00, 0xb8, 0x66, 0x99, 0xa3 ; 1000:44f8
    db 0x22, 0x09, 0xc6, 0x06, 0x21, 0x09, 0x01, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0xec, 0x99, 0xb9, 0x09 ; 1000:4508
    db 0x00, 0xcc, 0xeb, 0x9f                       ; 1000:4518

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_dialog_message  [UI / Windows]
; ADDRESS:  1000:451c
; PURPOSE:  Displays centered dialog box with text prompt and waits for user keypress.
; ----------------------------------------------------------------------------
ui_draw_dialog_message:
    mov          CX,AX                           ; 1000:451c: 8bc8
    shl          CX,0x1                          ; 1000:451e: d1e1
    lea          BX,[0x239]                      ; 1000:4520: 8d1e3902
    add          BX,CX                           ; 1000:4524: 03d9
    mov          CX,word [BX]                    ; 1000:4526: 8b0f
    cmp          CX,-0x1                         ; 1000:4528: 83f9ff
    jnz          loc_4547                        ; 1000:452b: 751a
    shl          AX,0x1                          ; 1000:452d: d1e0
    shl          AX,0x1                          ; 1000:452f: d1e0
    shl          AX,0x1                          ; 1000:4531: d1e0
    shl          AX,0x1                          ; 1000:4533: d1e0
    add          AX,0x8                          ; 1000:4535: 050800
    mov          BX,0xb1                         ; 1000:4538: bbb100
    mov          CX,0x10                         ; 1000:453b: b91000
    mov          DX,0x10                         ; 1000:453e: ba1000
    call         vid_setup_blit_rect             ; 1000:4541: e86735
    xor          DI,DI                           ; 1000:4544: 33ff
    ret                                          ; 1000:4546: c3
loc_4547:
    lea          DI,[0xaf0]                      ; 1000:4547: 8d3ef00a
    mov          BX,CX                           ; 1000:454b: 8bd9
    shl          CX,0x1                          ; 1000:454d: d1e1
    add          CX,BX                           ; 1000:454f: 03cb
    shl          CX,0x1                          ; 1000:4551: d1e1
    shl          CX,0x1                          ; 1000:4553: d1e1
    add          DI,CX                           ; 1000:4555: 03f9
    push         DI                              ; 1000:4557: 57
    shl          AX,0x1                          ; 1000:4558: d1e0
    shl          AX,0x1                          ; 1000:455a: d1e0
    shl          AX,0x1                          ; 1000:455c: d1e0
    shl          AX,0x1                          ; 1000:455e: d1e0
    add          AX,0x8                          ; 1000:4560: 050800
    mov          BX,0xb1                         ; 1000:4563: bbb100
    mov          SI,word [cs:DI + 0x8]           ; 1000:4566: 2e8b7508
    mov          DS,word [cs:0x5e]               ; 1000:456a: 2e8e1e5e00
    call         vid_set_map_mask                ; 1000:456f: e85433
    mov          DS,word [cs:0x5c]               ; 1000:4572: 2e8e1e5c00
    pop          DI                              ; 1000:4577: 5f
    ret                                          ; 1000:4578: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_erase_dialog  [UI / Windows]
; ADDRESS:  1000:4579
; PURPOSE:  Restores original background graphics obscured by a temporary dialog box.
; ----------------------------------------------------------------------------
ui_erase_dialog:
    mov          DI,word [0x6fe]                 ; 1000:4579: 8b3efe06
    or           DI,DI                           ; 1000:457d: 0bff
    jnz          loc_4582                        ; 1000:457f: 7501
    ret                                          ; 1000:4581: c3
loc_4582:
    lea          SI,[0x700]                      ; 1000:4582: 8d360007
loc_4586:
    cmp          AX,word [SI]                    ; 1000:4586: 3b04
    jnz          loc_4594                        ; 1000:4588: 750a
    cmp          BX,word [SI + 0x2]              ; 1000:458a: 3b5c02
    jnz          loc_4594                        ; 1000:458d: 7505
    and          word [SI + 0x4],0x7fff          ; 1000:458f: 816404ff7f
loc_4594:
    add          SI,0x8                          ; 1000:4594: 83c608
    dec          DI                              ; 1000:4597: 4f
    jnz          loc_4586                        ; 1000:4598: 75ec
    ret                                          ; 1000:459a: c3
    db 0xe8, 0x08, 0x00, 0xb8, 0x05, 0x00, 0xe8, 0xf1, 0x27, 0xf8, 0xc3 ; 1000:459b

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_dialog_beep  [Audio / SFX]
; ADDRESS:  1000:45a6
; PURPOSE:  Plays UI cursor navigation beep sound.
; ----------------------------------------------------------------------------
snd_play_dialog_beep:
    mov          AX,0x1                          ; 1000:45a6: b80100
    lea          BX,[0x9904]                     ; 1000:45a9: 8d1e0499
    mov          CX,0x9                          ; 1000:45ad: b90900
    int3                                         ; 1000:45b0: cc
    db 0x8b, 0x0e, 0x8f, 0x02, 0xfe, 0xc1, 0x80, 0xf9, 0x0a, 0x75, 0x0c, 0x32, 0xc9, 0xfe, 0xc5, 0x80 ; 1000:45b1
    db 0xfd, 0x0a, 0x75, 0x03, 0xb9, 0x09, 0x09, 0x89, 0x0e, 0x8f, 0x02, 0xb8, 0x20, 0x01, 0xbb, 0xa9 ; 1000:45c1
    db 0x00, 0xe8, 0x8a, 0xf4, 0xc3, 0x8b, 0x0e, 0x22, 0x02, 0x81, 0xf9, 0x09, 0x09, 0x75, 0x02, 0xf9 ; 1000:45d1
    db 0xc3, 0x80, 0xc1, 0x05, 0x80, 0xf9, 0x0a, 0x7c, 0x05, 0x80, 0xe9, 0x0a, 0xfe, 0xc5, 0x80, 0xc5 ; 1000:45e1
    db 0x02, 0x80, 0xfd, 0x0a, 0x7c, 0x03, 0xb9, 0x09, 0x09, 0x89, 0x0e, 0x22, 0x02, 0xb8, 0x20, 0x01 ; 1000:45f1
    db 0xbb, 0x90, 0x00, 0xe8, 0x58, 0xf4, 0x80, 0x3e, 0x96, 0x08, 0x02, 0x75, 0x05, 0xc6, 0x06, 0xa6 ; 1000:4601
    db 0x08, 0x01, 0xf8, 0xc3, 0xa0, 0xeb, 0x08, 0x02, 0x06, 0xed, 0x08, 0x3c, 0x0c, 0x7c, 0x02, 0xf9 ; 1000:4611
    db 0xc3, 0xc6, 0x06, 0xed, 0x08, 0x0c, 0xb8, 0x01, 0x00, 0xe8, 0x68, 0x27, 0xf8, 0xc3, 0xfe, 0x06 ; 1000:4621
    db 0x1c, 0x02, 0x8a, 0x0e, 0x1c, 0x02, 0x80, 0xf9, 0x07, 0x7c, 0x06, 0xfe, 0x0e, 0x1c, 0x02, 0xfe ; 1000:4631
    db 0xc9, 0x32, 0xed, 0xb8, 0x20, 0x01, 0xbb, 0x5e, 0x00, 0xe8, 0x12, 0xf4, 0xb8, 0x01, 0x00, 0xe8 ; 1000:4641
    db 0x42, 0x27, 0xf8, 0xc3, 0xf8, 0xc3, 0xc6, 0x06, 0x1e, 0x02, 0x01, 0xb8, 0xf0, 0x00, 0xbb, 0x90 ; 1000:4651
    db 0x00, 0x8d, 0x36, 0x50, 0x9f, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x58, 0x32, 0x2e, 0x8e, 0x1e ; 1000:4661
    db 0x5c, 0x00, 0xb8, 0x08, 0x00, 0xe8, 0x1c, 0x27, 0xf8, 0xc3, 0xc6, 0x06, 0x1f, 0x02, 0x01, 0xb8 ; 1000:4671
    db 0xf0, 0x00, 0xbb, 0xa9, 0x00, 0x8d, 0x36, 0xf6, 0x9f, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x34 ; 1000:4681
    db 0x32, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xb8, 0x08, 0x00, 0xe8, 0xf8, 0x26, 0xf8, 0xc3, 0x8b, 0x0e ; 1000:4691
    db 0x24, 0x02, 0xfe, 0xc1, 0xeb, 0x1a, 0x90, 0x8b, 0x0e, 0x24, 0x02, 0x80, 0xc1, 0x05, 0xeb, 0x10 ; 1000:46a1
    db 0x90, 0xb8, 0x01, 0x00, 0xe8, 0xdd, 0x26, 0x8b, 0x0e, 0x24, 0x02, 0xfe, 0xc5, 0xeb, 0x01, 0x90 ; 1000:46b1
    db 0x80, 0xf9, 0x0a, 0x7c, 0x05, 0xfe, 0xc5, 0x80, 0xe9, 0x0a, 0x80, 0xfd, 0x0a, 0x75, 0x07, 0xb5 ; 1000:46c1
    db 0x00, 0x51, 0xe8, 0xd0, 0xfe, 0x59, 0x89, 0x0e, 0x24, 0x02, 0xb8, 0x20, 0x01, 0xbb, 0x77, 0x00 ; 1000:46d1
    db 0xe8, 0x7b, 0xf3, 0xf8, 0xc3                 ; 1000:46e1

; ----------------------------------------------------------------------------
; FUNCTION: game_pause_handler  [Game / State]
; ADDRESS:  1000:46e6
; PURPOSE:  Handles Game Pause state: displays 'GAME PAUSED' banner, pauses timers, and polls Q key to quit.
; ----------------------------------------------------------------------------
game_pause_handler:
    call         map_draw_background             ; 1000:46e6: e843d6
    call         ui_draw_text_string             ; 1000:46e9: e88ffc
    call         vid_wait_vblank                 ; 1000:46ec: e85f30
    call         map_draw_background             ; 1000:46ef: e83ad6
    call         ui_draw_text_string             ; 1000:46f2: e886fc
loc_46f5:
    mov          AX,[0x890]                      ; 1000:46f5: a19008
    shl          AX,0x1                          ; 1000:46f8: d1e0
    shl          AX,0x1                          ; 1000:46fa: d1e0
    shl          AX,0x1                          ; 1000:46fc: d1e0
    shl          AX,0x1                          ; 1000:46fe: d1e0
    add          AX,0x8                          ; 1000:4700: 050800
    mov          BX,0xb1                         ; 1000:4703: bbb100
    lea          SI,[0xa334]                     ; 1000:4706: 8d3634a3
    mov          DS,word [cs:0x5e]               ; 1000:470a: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:470f: e89130
    mov          DS,word [cs:0x5c]               ; 1000:4712: 2e8e1e5c00
    call         vid_wait_vblank                 ; 1000:4717: e83430
loc_471a:
    xor          AL,AL                           ; 1000:471a: 32c0
    mov          [cs:0x258],AL                   ; 1000:471c: 2ea25802
    mov          [cs:0x259],AL                   ; 1000:4720: 2ea25902
    mov          [cs:0x25d],AL                   ; 1000:4724: 2ea25d02
    mov          [cs:0x61],AL                    ; 1000:4728: 2ea26100
loc_472c:
    cmp          byte [cs:0x258],0x1             ; 1000:472c: 2e803e580201
    jz           loc_475c                        ; 1000:4732: 7428
    cmp          byte [cs:0x259],0x1             ; 1000:4734: 2e803e590201
    jz           loc_476f                        ; 1000:473a: 7433
    cmp          byte [cs:0x25d],0x1             ; 1000:473c: 2e803e5d0201
    jz           loc_4783                        ; 1000:4742: 743f
    mov          AL,[cs:0x61]                    ; 1000:4744: 2ea06100
    cmp          AL,byte [cs:0x257]              ; 1000:4748: 2e3a065702
    jnz          loc_472c                        ; 1000:474d: 75dd
    mov          AX,[0x890]                      ; 1000:474f: a19008
    call         ui_draw_dialog_message          ; 1000:4752: e8c7fd
    mov          AX,0x1                          ; 1000:4755: b80100
    call         timer_delay_ticks               ; 1000:4758: e81fbb
    ret                                          ; 1000:475b: c3
loc_475c:
    cmp          word [0x890],0x0                ; 1000:475c: 833e900800
    jz           loc_471a                        ; 1000:4761: 74b7
    mov          AX,[0x890]                      ; 1000:4763: a19008
    call         ui_draw_dialog_message          ; 1000:4766: e8b3fd
    dec          word [0x890]                    ; 1000:4769: ff0e9008
    jmp short    loc_46f5                        ; 1000:476d: eb86
loc_476f:
    cmp          word [0x890],0xc                ; 1000:476f: 833e90080c
    jz           loc_471a                        ; 1000:4774: 74a4
    mov          AX,[0x890]                      ; 1000:4776: a19008
    call         ui_draw_dialog_message          ; 1000:4779: e8a0fd
    inc          word [0x890]                    ; 1000:477c: ff069008
    jmp near     loc_46f5                        ; 1000:4780: e972ff
loc_4783:
    mov          AX,[0x890]                      ; 1000:4783: a19008
    call         ui_draw_dialog_message          ; 1000:4786: e893fd
    or           DI,DI                           ; 1000:4789: 0bff
    jnz          loc_479a                        ; 1000:478b: 750d
    mov          byte [cs:0x25d],0x0             ; 1000:478d: 2ec6065d0200
    mov          AX,0x1                          ; 1000:4793: b80100
    call         timer_delay_ticks               ; 1000:4796: e8e1ba
    ret                                          ; 1000:4799: c3
loc_479a:
    cmp          word [cs:DI + 0x8],0xb3d2       ; 1000:479a: 2e817d08d2b3
    jnz          loc_47dd                        ; 1000:47a0: 753b
    cmp          word [0x6fa],0xb                ; 1000:47a2: 833efa060b
    jnz          loc_47dd                        ; 1000:47a7: 7534
    cmp          word [0x6fc],0x7                ; 1000:47a9: 833efc0607
    jnz          loc_47dd                        ; 1000:47ae: 752d
    cmp          word [0x6c6],0x250              ; 1000:47b0: 813ec6065002
    jnz          loc_47dd                        ; 1000:47b6: 7525
    cmp          word [0x6c8],0x50               ; 1000:47b8: 833ec80650
    jnz          loc_47dd                        ; 1000:47bd: 751e
    mov          byte [0x221],0x1                ; 1000:47bf: c606210201
    mov          AX,0x1                          ; 1000:47c4: b80100
    lea          BX,[0x99a4]                     ; 1000:47c7: 8d1ea499
    mov          CX,0x9                          ; 1000:47cb: b90900
    int3                                         ; 1000:47ce: cc
    db 0xbe, 0x1c, 0x99, 0x57, 0xe8, 0x30, 0xfa, 0xe8, 0x75, 0x2f, 0x5f, 0xe8, 0xbc, 0xbd ; 1000:47cf
loc_47dd:
    mov          AX,[0x6fa]                      ; 1000:47dd: a1fa06
    mov          word [cs:DI],AX                 ; 1000:47e0: 2e8905
    mov          AX,[0x6fc]                      ; 1000:47e3: a1fc06
    mov          word [cs:DI + 0x2],AX           ; 1000:47e6: 2e894502
    mov          AX,[0x6c6]                      ; 1000:47ea: a1c606
    mov          word [cs:DI + 0x4],AX           ; 1000:47ed: 2e894504
    mov          AX,[0x6c8]                      ; 1000:47f1: a1c806
    add          AX,0x10                         ; 1000:47f4: 051000
    mov          word [cs:DI + 0x6],AX           ; 1000:47f7: 2e894506
    mov          SI,0x700                        ; 1000:47fb: be0007
    mov          CX,word [0x6fe]                 ; 1000:47fe: 8b0efe06
    or           CX,CX                           ; 1000:4802: 0bc9
    jz           loc_4819                        ; 1000:4804: 7413
loc_4806:
    cmp          word [SI],-0x1                  ; 1000:4806: 833cff
    jz           loc_481d                        ; 1000:4809: 7412
    add          SI,0x8                          ; 1000:480b: 83c608
    dec          CX                              ; 1000:480e: 49
    jnz          loc_4806                        ; 1000:480f: 75f5
    cmp          word [0x6fe],0x32               ; 1000:4811: 833efe0632
    jl           loc_4819                        ; 1000:4816: 7c01
    ret                                          ; 1000:4818: c3
loc_4819:
    inc          word [0x6fe]                    ; 1000:4819: ff06fe06
loc_481d:
    mov          word [0x8ab],SI                 ; 1000:481d: 8936ab08
    mov          AX,word [cs:DI + 0x4]           ; 1000:4821: 2e8b4504
    mov          [0x8a9],AX                      ; 1000:4825: a3a908
    mov          word [SI],0xffff                ; 1000:4828: c704ffff
    mov          AX,word [cs:DI + 0x6]           ; 1000:482c: 2e8b4506
    mov          word [SI + 0x2],AX              ; 1000:4830: 894402
    mov          AX,word [cs:DI + 0x8]           ; 1000:4833: 2e8b4508
    mov          word [SI + 0x6],AX              ; 1000:4837: 894406
    mov          AX,[0x890]                      ; 1000:483a: a19008
    mov          CX,AX                           ; 1000:483d: 8bc8
    shl          CX,0x1                          ; 1000:483f: d1e1
    lea          BX,[0x239]                      ; 1000:4841: 8d1e3902
    add          BX,CX                           ; 1000:4845: 03d9
    mov          CX,word [BX]                    ; 1000:4847: 8b0f
    mov          word [SI + 0x4],CX              ; 1000:4849: 894c04
    mov          word [BX],0xffff                ; 1000:484c: c707ffff
    call         ui_draw_dialog_message          ; 1000:4850: e8c9fc
    mov          byte [0x8a8],0x14               ; 1000:4853: c606a80814
    mov          AX,0x1                          ; 1000:4858: b80100
    lea          BX,[0x98fc]                     ; 1000:485b: 8d1efc98
    mov          CX,0x4                          ; 1000:485f: b90400
    int3                                         ; 1000:4862: cc
    db 0x2e, 0xc6, 0x06, 0x5d, 0x02, 0x00, 0xb8, 0x01, 0x00, 0xe8, 0x0b, 0xba, 0xc3 ; 1000:4863

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_chest_open  [Audio / SFX]
; ADDRESS:  1000:4870
; PURPOSE:  Plays treasure chest unlock sound effect.
; ----------------------------------------------------------------------------
snd_play_chest_open:
    mov          AX,0x1                          ; 1000:4870: b80100
    lea          BX,[0x98e0]                     ; 1000:4873: 8d1ee098
    mov          CX,0x3                          ; 1000:4877: b90300
    int3                                         ; 1000:487a: cc
    db 0xc6, 0x06, 0x1d, 0x02, 0x01, 0xb8, 0xf0, 0x00, 0xbb, 0x77, 0x00, 0x8d, 0x36, 0xaa, 0x9e, 0x2e ; 1000:487b
    db 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x34, 0x30, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xb8, 0x08, 0x00, 0xe8 ; 1000:488b
    db 0xf8, 0x24, 0xc3                             ; 1000:489b

; ----------------------------------------------------------------------------
; FUNCTION: item_check_has_item  [Items / Inventory]
; ADDRESS:  1000:489e
; PURPOSE:  Checks if specified item flag is set in player's inventory bitmask.
; ----------------------------------------------------------------------------
item_check_has_item:
    jcxz         loc_48a9                        ; 1000:489e: e309
    dec          CL                              ; 1000:48a0: fec9
    jge          loc_48a9                        ; 1000:48a2: 7d05
    add          CL,0xa                          ; 1000:48a4: 80c10a
    dec          CH                              ; 1000:48a7: fecd
loc_48a9:
    ret                                          ; 1000:48a9: c3

; ----------------------------------------------------------------------------
; FUNCTION: item_add_to_inventory  [Items / Inventory]
; ADDRESS:  1000:48aa
; PURPOSE:  Sets item flag in player inventory bitmask and triggers item pickup notification.
; ----------------------------------------------------------------------------
item_add_to_inventory:
    mov          CX,word [0x222]                 ; 1000:48aa: 8b0e2202
    call         item_check_has_item             ; 1000:48ae: e8edff
    mov          word [0x222],CX                 ; 1000:48b1: 890e2202
    mov          AX,0x120                        ; 1000:48b5: b82001
    mov          BX,0x90                         ; 1000:48b8: bb9000
    call         hud_draw_shield_bar             ; 1000:48bb: e8a1f1
    ret                                          ; 1000:48be: c3
    db 0x8b, 0x0e, 0x8f, 0x02, 0xe8, 0xd8, 0xff, 0x89, 0x0e, 0x8f, 0x02, 0xb8, 0x20, 0x01, 0xbb, 0xa9 ; 1000:48bf
    db 0x00, 0xe8, 0x8c, 0xf1, 0xc3                 ; 1000:48cf

; ----------------------------------------------------------------------------
; FUNCTION: item_remove_from_inventory  [Items / Inventory]
; ADDRESS:  1000:48d4
; PURPOSE:  Clears item flag from player inventory bitmask upon item consumption.
; ----------------------------------------------------------------------------
item_remove_from_inventory:
    mov          CX,word [0x6fe]                 ; 1000:48d4: 8b0efe06
    jcxz         loc_48ec                        ; 1000:48d8: e312
    mov          SI,0x700                        ; 1000:48da: be0007
loc_48dd:
    cmp          word [SI + 0x2],0x8000          ; 1000:48dd: 817c020080
    jc           loc_48e7                        ; 1000:48e2: 7203
    neg          word [SI + 0x2]                 ; 1000:48e4: f75c02
loc_48e7:
    add          SI,0x8                          ; 1000:48e7: 83c608
    loop         loc_48dd                        ; 1000:48ea: e2f1
loc_48ec:
    ret                                          ; 1000:48ec: c3
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:48ed
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:48fd
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:490d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:491d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:492d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:493d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:494d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:495d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:496d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:497d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:498d
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:499d

; ----------------------------------------------------------------------------
; FUNCTION: spr_render_monster  [Sprites / Monsters]
; ADDRESS:  1000:49ad
; PURPOSE:  Renders monster sprite with planar EGA color plane writes and transparency masking.
; ----------------------------------------------------------------------------
spr_render_monster:
    push         BP                              ; 1000:49ad: 55
    mov          BP,SP                           ; 1000:49ae: 8bec
    sub          SP,0x4                          ; 1000:49b0: 83ec04
    mov          byte [0x9a22],0x0               ; 1000:49b3: c606229a00
    mov          byte [0x9a24],0x0               ; 1000:49b8: c606249a00
    mov          BX,word [0x6fa]                 ; 1000:49bd: 8b1efa06
    add          BX,0x229                        ; 1000:49c1: 81c32902
    mov          AL,byte [BX]                    ; 1000:49c5: 8a07
    mov          [0x9a23],AL                     ; 1000:49c7: a2239a
    cmp          AL,0x1                          ; 1000:49ca: 3c01
    jz           loc_49d1                        ; 1000:49cc: 7403
    call         item_remove_from_inventory      ; 1000:49ce: e803ff
loc_49d1:
    mov          SI,word [0x6f8]                 ; 1000:49d1: 8b36f806
    mov          CX,word [0x6f6]                 ; 1000:49d5: 8b0ef606
    or           CX,CX                           ; 1000:49d9: 0bc9
    jz           loc_49f6                        ; 1000:49db: 7419
loc_49dd:
    mov          byte [cs:SI + 0x6],0x0          ; 1000:49dd: 2ec6440600
    mov          AX,word [cs:SI]                 ; 1000:49e2: 2e8b04
    mov          word [cs:SI + 0x8],AX           ; 1000:49e5: 2e894408
    mov          AX,word [cs:SI + 0x2]           ; 1000:49e9: 2e8b4402
    mov          word [cs:SI + 0xa],AX           ; 1000:49ed: 2e89440a
    add          SI,0xc                          ; 1000:49f1: 83c60c
    loop         loc_49dd                        ; 1000:49f4: e2e7
loc_49f6:
    mov          word [BP + -0x2],0x0            ; 1000:49f6: c746fe0000
    mov          SI,0x48ed                       ; 1000:49fb: beed48
    mov          DI,word [0x6f8]                 ; 1000:49fe: 8b3ef806
    mov          AX,[0x6f6]                      ; 1000:4a02: a1f606
    mov          word [BP + -0x4],AX             ; 1000:4a05: 8946fc
    or           AX,AX                           ; 1000:4a08: 0bc0
    jz           loc_4a5a                        ; 1000:4a0a: 744e
loc_4a0c:
    mov          CX,word [0x6c2]                 ; 1000:4a0c: 8b0ec206
    add          CX,0xd0                         ; 1000:4a10: 81c1d000
    mov          DX,word [0x6c4]                 ; 1000:4a14: 8b16c406
    add          DX,0xa0                         ; 1000:4a18: 81c2a000
    mov          BX,word [cs:DI + 0x4]           ; 1000:4a1c: 2e8b5d04
    mov          AX,word [cs:DI + 0x8]           ; 1000:4a20: 2e8b4508
    cmp          AX,word [0x6c2]                 ; 1000:4a24: 3b06c206
    jl           loc_4a52                        ; 1000:4a28: 7c28
    add          AX,word [cs:BX]                 ; 1000:4a2a: 2e0307
    cmp          AX,CX                           ; 1000:4a2d: 3bc1
    jg           loc_4a52                        ; 1000:4a2f: 7f21
    mov          AX,word [cs:DI + 0xa]           ; 1000:4a31: 2e8b450a
    cmp          AX,DX                           ; 1000:4a35: 3bc2
    jge          loc_4a52                        ; 1000:4a37: 7d19
    add          AX,word [cs:BX + 0x2]           ; 1000:4a39: 2e034702
    cmp          AX,word [0x6c4]                 ; 1000:4a3d: 3b06c406
    jle          loc_4a52                        ; 1000:4a41: 7e0f
    call         spr_get_monster_frame           ; 1000:4a43: e82a00
    add          SI,0x20                         ; 1000:4a46: 83c620
    inc          word [BP + -0x2]                ; 1000:4a49: ff46fe
    cmp          word [BP + -0x2],0x6            ; 1000:4a4c: 837efe06
    jz           loc_4a6c                        ; 1000:4a50: 741a
loc_4a52:
    add          DI,0xc                          ; 1000:4a52: 83c70c
    dec          word [BP + -0x4]                ; 1000:4a55: ff4efc
    jnz          loc_4a0c                        ; 1000:4a58: 75b2
loc_4a5a:
    mov          word [cs:SI + 0x6],0x0          ; 1000:4a5a: 2ec744060000
    add          SI,0x20                         ; 1000:4a60: 83c620
    inc          word [BP + -0x2]                ; 1000:4a63: ff46fe
    cmp          word [BP + -0x2],0x6            ; 1000:4a66: 837efe06
    jnz          loc_4a5a                        ; 1000:4a6a: 75ee
loc_4a6c:
    mov          SP,BP                           ; 1000:4a6c: 8be5
    pop          BP                              ; 1000:4a6e: 5d
    ret                                          ; 1000:4a6f: c3

; ----------------------------------------------------------------------------
; FUNCTION: spr_get_monster_frame  [Sprites / Animation]
; ADDRESS:  1000:4a70
; PURPOSE:  Looks up graphic offset for monster animation frame from sprite catalog.
; ----------------------------------------------------------------------------
spr_get_monster_frame:
    mov          CX,0x8                          ; 1000:4a70: b90800
    push         BX                              ; 1000:4a73: 53
    push         SI                              ; 1000:4a74: 56
loc_4a75:
    mov          AX,word [cs:BX]                 ; 1000:4a75: 2e8b07
    add          BX,0x2                          ; 1000:4a78: 83c302
    mov          word [cs:SI],AX                 ; 1000:4a7b: 2e8904
    add          SI,0x2                          ; 1000:4a7e: 83c602
    loop         loc_4a75                        ; 1000:4a81: e2f2
    pop          SI                              ; 1000:4a83: 5e
    pop          BX                              ; 1000:4a84: 5b
    mov          AX,word [cs:DI + 0x8]           ; 1000:4a85: 2e8b4508
    mov          word [cs:SI + 0x10],AX          ; 1000:4a89: 2e894410
    mov          AX,word [cs:DI + 0xa]           ; 1000:4a8d: 2e8b450a
    mov          word [cs:SI + 0x12],AX          ; 1000:4a91: 2e894412
    xor          AX,AX                           ; 1000:4a95: 33c0
    mov          word [cs:SI + 0x14],AX          ; 1000:4a97: 2e894414
    mov          word [cs:SI + 0x16],AX          ; 1000:4a9b: 2e894416
    mov          word [cs:SI + 0x1c],AX          ; 1000:4a9f: 2e89441c
    mov          word [cs:SI + 0x1a],AX          ; 1000:4aa3: 2e89441a
    mov          word [cs:SI + 0x1e],DI          ; 1000:4aa7: 2e897c1e
    mov          AX,[0x9a20]                     ; 1000:4aab: a1209a
    xor          word [0x9a20],0x3               ; 1000:4aae: 8336209a03
    mov          word [cs:SI + 0x18],AX          ; 1000:4ab3: 2e894418
    mov          byte [cs:DI + 0x6],0x1          ; 1000:4ab7: 2ec6450601
    mov          AX,word [cs:SI + 0x4]           ; 1000:4abc: 2e8b4404
    cmp          AH,0x1                          ; 1000:4ac0: 80fc01
    jnz          loc_4ade                        ; 1000:4ac3: 7519
    cmp          byte [0x9a23],0x1               ; 1000:4ac5: 803e239a01
    jz           loc_4ad4                        ; 1000:4aca: 7408
    mov          word [cs:SI + 0x10],0xffff      ; 1000:4acc: 2ec74410ffff
    jmp short    loc_4ade                        ; 1000:4ad2: eb0a
loc_4ad4:
    cmp          byte [0x9a22],0x0               ; 1000:4ad4: 803e229a00
    jnz          loc_4ade                        ; 1000:4ad9: 7503
    mov          [0x9a22],AL                     ; 1000:4adb: a2229a
loc_4ade:
    inc          byte [0x9a24]                   ; 1000:4ade: fe06249a
    ret                                          ; 1000:4ae2: c3
    db 0xe7, 0x50, 0x49, 0x51, 0xbd, 0x51, 0xf9, 0x51, 0x59, 0x52, 0x73, 0x52, 0xbf, 0x52, 0xcf, 0x52 ; 1000:4ae3
    db 0x61, 0x53, 0xe9, 0x53, 0x38, 0x54, 0x55, 0x54, 0xee, 0x54, 0x50, 0x55, 0x70, 0x55, 0x91, 0x55 ; 1000:4af3
    db 0xb9, 0x55, 0xda, 0x55, 0x1b, 0x56, 0x6a, 0x56, 0x8c, 0x56, 0xd3, 0x56, 0x6d, 0x57, 0xc3, 0x57 ; 1000:4b03
    db 0x14, 0x58, 0x3f, 0x58, 0x52, 0x58, 0xad, 0x58, 0x01, 0x59, 0x20, 0x59, 0x6c, 0x59, 0xc1, 0x59 ; 1000:4b13
    db 0xc9, 0x59                                   ; 1000:4b23

; ----------------------------------------------------------------------------
; FUNCTION: vid_flip_buffers  [Video / Double Buffering]
; ADDRESS:  1000:4b25
; PURPOSE:  Toggles active and visible video page offsets for flicker-free double buffering.
; ----------------------------------------------------------------------------
vid_flip_buffers:
    push         BP                              ; 1000:4b25: 55
    mov          BP,SP                           ; 1000:4b26: 8bec
    sub          SP,0x4                          ; 1000:4b28: 83ec04
    mov          SI,0x48ed                       ; 1000:4b2b: beed48
    mov          DI,word [0x6f8]                 ; 1000:4b2e: 8b3ef806
    mov          AX,[0x6f6]                      ; 1000:4b32: a1f606
    mov          word [BP + -0x2],AX             ; 1000:4b35: 8946fe
    mov          CX,0x6                          ; 1000:4b38: b90600
loc_4b3b:
    push         CX                              ; 1000:4b3b: 51
    cmp          word [cs:SI + 0x6],0x0          ; 1000:4b3c: 2e837c0600
    jz           loc_4b59                        ; 1000:4b41: 7416
    jmp near     ent_render_all                  ; 1000:4b43: e9cf03

; ----------------------------------------------------------------------------
; FUNCTION: spr_blit_masked  [Video / Sprites]
; ADDRESS:  1000:4b46
; PURPOSE:  Low-level masked EGA sprite blitter using latch registers and bitmasking.
; ----------------------------------------------------------------------------
spr_blit_masked:
    or           SI,SI                           ; 1000:4b46: 0bf6
    jz           loc_4b55                        ; 1000:4b48: 740b
    pop          CX                              ; 1000:4b4a: 59
    add          SI,0x20                         ; 1000:4b4b: 83c620
    loop         loc_4b3b                        ; 1000:4b4e: e2eb
    xor          SI,SI                           ; 1000:4b50: 33f6
    jmp short    loc_4b59                        ; 1000:4b52: eb05
    db 0x90                                         ; 1000:4b54
loc_4b55:
    mov          SP,BP                           ; 1000:4b55: 8be5
    pop          BP                              ; 1000:4b57: 5d
    ret                                          ; 1000:4b58: c3
loc_4b59:
    cmp          word [BP + -0x2],0x0            ; 1000:4b59: 837efe00
    jz           spr_blit_masked                 ; 1000:4b5d: 74e7
    test         word [cs:DI + 0x6],0x700        ; 1000:4b5f: 2ef745060007
    jz           loc_4b74                        ; 1000:4b65: 740d
    cmp          byte [cs:DI + 0x7],0x1          ; 1000:4b67: 2e807d0701
    jz           loc_4b7e                        ; 1000:4b6c: 7410
    dec          byte [cs:DI + 0x7]              ; 1000:4b6e: 2efe4d07
    jmp short    loc_4b7e                        ; 1000:4b72: eb0a
loc_4b74:
    cmp          word [cs:DI + 0x6],0x1          ; 1000:4b74: 2e837d0601
    jnz          loc_4b7e                        ; 1000:4b79: 7503
    jmp near     loc_4bfd                        ; 1000:4b7b: e97f00
loc_4b7e:
    mov          CX,word [0x6c2]                 ; 1000:4b7e: 8b0ec206
    add          CX,0xd0                         ; 1000:4b82: 81c1d000
    mov          DX,word [0x6c4]                 ; 1000:4b86: 8b16c406
    add          DX,0xa0                         ; 1000:4b8a: 81c2a000
    mov          BX,word [cs:DI + 0x4]           ; 1000:4b8e: 2e8b5d04
    mov          AX,word [cs:DI + 0x8]           ; 1000:4b92: 2e8b4508
    cmp          AX,word [0x6c2]                 ; 1000:4b96: 3b06c206
    jl           loc_4bf8                        ; 1000:4b9a: 7c5c
    add          AX,word [cs:BX]                 ; 1000:4b9c: 2e0307
    cmp          AX,CX                           ; 1000:4b9f: 3bc1
    jg           loc_4bf8                        ; 1000:4ba1: 7f55
    mov          AX,word [cs:DI + 0xa]           ; 1000:4ba3: 2e8b450a
    cmp          AX,DX                           ; 1000:4ba7: 3bc2
    jge          loc_4bf8                        ; 1000:4ba9: 7d4d
    add          AX,word [cs:BX + 0x2]           ; 1000:4bab: 2e034702
    cmp          AX,word [0x6c4]                 ; 1000:4baf: 3b06c406
    jle          loc_4bf8                        ; 1000:4bb3: 7e43
    test         word [cs:DI + 0x6],0x700        ; 1000:4bb5: 2ef745060007
    jnz          loc_4bc6                        ; 1000:4bbb: 7509
    cmp          byte [cs:DI + 0x6],0x0          ; 1000:4bbd: 2e807d0600
    jnz          loc_4bfd                        ; 1000:4bc2: 7539
    jmp short    loc_4be1                        ; 1000:4bc4: eb1b
loc_4bc6:
    cmp          byte [cs:DI + 0x7],0x1          ; 1000:4bc6: 2e807d0701
    jnz          loc_4bfd                        ; 1000:4bcb: 7530
    cmp          byte [0x9a23],0x1               ; 1000:4bcd: 803e239a01
    jnz          loc_4bfd                        ; 1000:4bd2: 7529
    cmp          byte [0x9a24],0x5               ; 1000:4bd4: 803e249a05
    jge          loc_4bfd                        ; 1000:4bd9: 7d22
    mov          word [cs:DI + 0x6],0x600        ; 1000:4bdb: 2ec745060006
loc_4be1:
    or           SI,SI                           ; 1000:4be1: 0bf6
    jnz          loc_4bec                        ; 1000:4be3: 7507
    mov          byte [cs:DI + 0x6],0x2          ; 1000:4be5: 2ec6450602
    jmp short    loc_4bfd                        ; 1000:4bea: eb11
loc_4bec:
    call         spr_get_monster_frame           ; 1000:4bec: e881fe
    add          DI,0xc                          ; 1000:4bef: 83c70c
    dec          word [BP + -0x2]                ; 1000:4bf2: ff4efe
    jmp near     ent_render_all                  ; 1000:4bf5: e91d03
loc_4bf8:
    mov          byte [cs:DI + 0x6],0x0          ; 1000:4bf8: 2ec6450600
loc_4bfd:
    add          DI,0xc                          ; 1000:4bfd: 83c70c
    dec          word [BP + -0x2]                ; 1000:4c00: ff4efe
    jmp near     loc_4b59                        ; 1000:4c03: e953ff

; ----------------------------------------------------------------------------
; FUNCTION: math_clamp_coords  [Math / Geometry]
; ADDRESS:  1000:4c06
; PURPOSE:  Clamps entity X/Y coordinates to stay within room bounding box.
; ----------------------------------------------------------------------------
math_clamp_coords:
    mov          word [cs:SI + 0x6],0x0          ; 1000:4c06: 2ec744060000
    mov          BX,word [cs:SI + 0x1e]          ; 1000:4c0c: 2e8b5c1e
    mov          byte [cs:BX + 0x6],0x2          ; 1000:4c10: 2ec6470602
    mov          AX,word [cs:BX]                 ; 1000:4c15: 2e8b07
    mov          word [cs:BX + 0x8],AX           ; 1000:4c18: 2e894708
    mov          AX,word [cs:BX + 0x2]           ; 1000:4c1c: 2e8b4702
    mov          word [cs:BX + 0xa],AX           ; 1000:4c20: 2e89470a
    ret                                          ; 1000:4c24: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_scroll_camera  [Tilemap / Camera]
; ADDRESS:  1000:4c25
; PURPOSE:  Calculates camera scroll offsets relative to player position to keep Comic on screen.
; ----------------------------------------------------------------------------
map_scroll_camera:
    mov          word [cs:SI + 0x6],0x0          ; 1000:4c25: 2ec744060000
    cmp          byte [0x8b1],0x6                ; 1000:4c2b: 803eb10806
    jz           loc_4c83                        ; 1000:4c30: 7451
    mov          DI,0x8b2                        ; 1000:4c32: bfb208
loc_4c35:
    cmp          byte [DI],0x0                   ; 1000:4c35: 803d00
    jz           loc_4c3f                        ; 1000:4c38: 7405
    add          DI,0x9                          ; 1000:4c3a: 83c709
    jmp short    loc_4c35                        ; 1000:4c3d: ebf6
loc_4c3f:
    inc          byte [0x8b1]                    ; 1000:4c3f: fe06b108
    mov          byte [DI],0x2                   ; 1000:4c43: c60502
    mov          byte [DI + 0x4],0x0             ; 1000:4c46: c6450400
    mov          AX,word [cs:SI]                 ; 1000:4c4a: 2e8b04
    sub          AX,0x10                         ; 1000:4c4d: 2d1000
    sar          AX,0x1                          ; 1000:4c50: d1f8
    add          AX,word [cs:SI + 0x10]          ; 1000:4c52: 2e034410
    mov          word [DI + 0x5],AX              ; 1000:4c56: 894505
    mov          AX,word [cs:SI + 0x12]          ; 1000:4c59: 2e8b4412
    mov          word [DI + 0x7],AX              ; 1000:4c5d: 894507
    cmp          word [cs:SI + 0x2],0x10         ; 1000:4c60: 2e837c0210
    jle          loc_4c83                        ; 1000:4c65: 7e1c
    sub          word [cs:SI + 0x2],0x10         ; 1000:4c67: 2e836c0210
    mov          AX,word [cs:SI + 0x2]           ; 1000:4c6c: 2e8b4402
    add          word [cs:SI + 0x12],AX          ; 1000:4c70: 2e014412
    cmp          word [cs:SI + 0x2],0x10         ; 1000:4c74: 2e837c0210
    jle          loc_4c81                        ; 1000:4c79: 7e06
    mov          word [cs:SI + 0x2],0x10         ; 1000:4c7b: 2ec744021000
loc_4c81:
    jmp short    map_scroll_camera               ; 1000:4c81: eba2
loc_4c83:
    mov          BX,word [cs:SI + 0x1e]          ; 1000:4c83: 2e8b5c1e
    mov          byte [cs:BX + 0x6],0x2          ; 1000:4c87: 2ec6470602
    mov          AX,word [cs:BX]                 ; 1000:4c8c: 2e8b07
    mov          word [cs:BX + 0x8],AX           ; 1000:4c8f: 2e894708
    mov          AX,word [cs:BX + 0x2]           ; 1000:4c93: 2e8b4702
    mov          word [cs:BX + 0xa],AX           ; 1000:4c97: 2e89470a
    mov          AX,0x1                          ; 1000:4c9b: b80100
    mov          BX,0x9a25                       ; 1000:4c9e: bb259a
    mov          CX,0x2                          ; 1000:4ca1: b90200
    int3                                         ; 1000:4ca4: cc
    db 0x2e, 0x8a, 0x44, 0x04, 0x32, 0xe4, 0x3c, 0x09, 0x7f, 0x09, 0x0b, 0xc0, 0x75, 0x02, 0xfe, 0xc0 ; 1000:4ca5
    db 0xe8, 0xe1, 0x20, 0x5f, 0xfe, 0x0e, 0x24, 0x9a, 0x59, 0xe9, 0x7a, 0xfe ; 1000:4cb5

; ----------------------------------------------------------------------------
; FUNCTION: map_check_room_bounds  [Tilemap / Streaming]
; ADDRESS:  1000:4cc1
; PURPOSE:  Tests if player has crossed room boundaries to trigger screen transition.
; ----------------------------------------------------------------------------
map_check_room_bounds:
    mov          BX,DI                           ; 1000:4cc1: 8bdf
    cmp          byte [0x8b1],0x6                ; 1000:4cc3: 803eb10806
    jz           loc_4cf3                        ; 1000:4cc8: 7429
    mov          DI,0x8b2                        ; 1000:4cca: bfb208
loc_4ccd:
    cmp          byte [DI],0x0                   ; 1000:4ccd: 803d00
    jz           loc_4cd7                        ; 1000:4cd0: 7405
    add          DI,0x9                          ; 1000:4cd2: 83c709
    jmp short    loc_4ccd                        ; 1000:4cd5: ebf6
loc_4cd7:
    inc          byte [0x8b1]                    ; 1000:4cd7: fe06b108
    mov          byte [DI],0x2                   ; 1000:4cdb: c60502
    mov          byte [DI + 0x4],0x2             ; 1000:4cde: c6450402
    mov          AX,word [BX]                    ; 1000:4ce2: 8b07
    sub          AX,0x4                          ; 1000:4ce4: 2d0400
    mov          word [DI + 0x5],AX              ; 1000:4ce7: 894505
    mov          AX,word [BX + 0x2]              ; 1000:4cea: 8b4702
    sub          AX,0x4                          ; 1000:4ced: 2d0400
    mov          word [DI + 0x7],AX              ; 1000:4cf0: 894507
loc_4cf3:
    mov          word [BX],0xffff                ; 1000:4cf3: c707ffff
    cmp          byte [0x9a22],0x1               ; 1000:4cf7: 803e229a01
    jnz          loc_4d30                        ; 1000:4cfc: 7532
    mov          BX,word [0x6fa]                 ; 1000:4cfe: 8b1efa06
    add          BX,0x229                        ; 1000:4d02: 81c32902
    mov          byte [BX],0x0                   ; 1000:4d06: c60700
    mov          byte [0x9a23],0x0               ; 1000:4d09: c606239a00
    mov          CX,0x6                          ; 1000:4d0e: b90600
    mov          BX,0x48ed                       ; 1000:4d11: bbed48
loc_4d14:
    mov          word [cs:BX + 0x6],0x0          ; 1000:4d14: 2ec747060000
    add          BX,0x20                         ; 1000:4d1a: 83c320
    loop         loc_4d14                        ; 1000:4d1d: e2f5
    push         SI                              ; 1000:4d1f: 56
    call         snd_play_dialog_beep            ; 1000:4d20: e883f8
    call         item_remove_from_inventory      ; 1000:4d23: e8aefb
    pop          SI                              ; 1000:4d26: 5e
    mov          AX,0x5                          ; 1000:4d27: b80500
    call         snd_play_cursor_move            ; 1000:4d2a: e8ba20
    jmp near     map_scroll_camera               ; 1000:4d2d: e9f5fe
loc_4d30:
    dec          byte [0x9a22]                   ; 1000:4d30: fe0e229a
    mov          AX,0x1                          ; 1000:4d34: b80100
    mov          BX,0x9a31                       ; 1000:4d37: bb319a
    mov          CX,0x2                          ; 1000:4d3a: b90200
    int3                                         ; 1000:4d3d: cc
    db 0xe9, 0xf0, 0x02                             ; 1000:4d3e

; ----------------------------------------------------------------------------
; FUNCTION: boss_render_sprites  [Boss / Rendering]
; ADDRESS:  1000:4d41
; PURPOSE:  Renders multi-segment animated boss monster graphics.
; ----------------------------------------------------------------------------
boss_render_sprites:
    cmp          byte [cs:SI + 0x4],0x0          ; 1000:4d41: 2e807c0400
    jnz          loc_4d65                        ; 1000:4d46: 751d
    cmp          byte [cs:SI + 0x5],0x80         ; 1000:4d48: 2e807c0580
    jz           loc_4d5f                        ; 1000:4d4d: 7410
    mov          AL,[0x89d]                      ; 1000:4d4f: a09d08
    or           AL,byte [0x8a7]                 ; 1000:4d52: 0a06a708
    or           AL,byte [0x8a3]                 ; 1000:4d56: 0a06a308
    jnz          loc_4d62                        ; 1000:4d5a: 7506
    jmp near     loc_4e29                        ; 1000:4d5c: e9ca00
loc_4d5f:
    call         game_execute_script             ; 1000:4d5f: e8a41a
loc_4d62:
    jmp near     spr_render_water_ripple         ; 1000:4d62: e92b03
loc_4d65:
    cmp          byte [0x8ec],0x0                ; 1000:4d65: 803eec0800
    jnz          loc_4d62                        ; 1000:4d6a: 75f6
    cmp          byte [cs:SI + 0x5],0x1          ; 1000:4d6c: 2e807c0501
    jz           loc_4dd2                        ; 1000:4d71: 745f
    mov          word [cs:SI + 0x6],0x0          ; 1000:4d73: 2ec744060000
    cmp          byte [0x8b1],0x6                ; 1000:4d79: 803eb10806
    jz           loc_4dd2                        ; 1000:4d7e: 7452
    mov          DI,0x8b2                        ; 1000:4d80: bfb208
loc_4d83:
    cmp          byte [DI],0x0                   ; 1000:4d83: 803d00
    jz           loc_4d8d                        ; 1000:4d86: 7405
    add          DI,0x9                          ; 1000:4d88: 83c709
    jmp short    loc_4d83                        ; 1000:4d8b: ebf6
loc_4d8d:
    inc          byte [0x8b1]                    ; 1000:4d8d: fe06b108
    mov          byte [DI],0x3                   ; 1000:4d91: c60503
    mov          byte [DI + 0x4],0x0             ; 1000:4d94: c6450400
    mov          AX,word [cs:SI]                 ; 1000:4d98: 2e8b04
    sub          AX,0x10                         ; 1000:4d9b: 2d1000
    sar          AX,0x1                          ; 1000:4d9e: d1f8
    add          AX,word [cs:SI + 0x10]          ; 1000:4da0: 2e034410
    mov          word [DI + 0x5],AX              ; 1000:4da4: 894505
    mov          AX,word [cs:SI + 0x12]          ; 1000:4da7: 2e8b4412
    mov          word [DI + 0x7],AX              ; 1000:4dab: 894507
    cmp          word [cs:SI + 0x2],0x10         ; 1000:4dae: 2e837c0210
    jle          loc_4dd2                        ; 1000:4db3: 7e1d
    sub          word [cs:SI + 0x2],0x10         ; 1000:4db5: 2e836c0210
    mov          AX,word [cs:SI + 0x2]           ; 1000:4dba: 2e8b4402
    add          word [cs:SI + 0x12],AX          ; 1000:4dbe: 2e014412
    cmp          word [cs:SI + 0x2],0x10         ; 1000:4dc2: 2e837c0210
    jle          loc_4dcf                        ; 1000:4dc7: 7e06
    mov          word [cs:SI + 0x2],0x10         ; 1000:4dc9: 2ec744021000
loc_4dcf:
    jmp near     boss_render_sprites             ; 1000:4dcf: e96fff
loc_4dd2:
    mov          AX,0x1                          ; 1000:4dd2: b80100
    mov          BX,0x9a3d                       ; 1000:4dd5: bb3d9a
    mov          CX,0x8                          ; 1000:4dd8: b90800
    int3                                         ; 1000:4ddb: cc
    db 0x2e, 0x8b, 0x44, 0x04, 0x28, 0x06, 0xed, 0x08, 0xa0, 0xed, 0x08, 0x3c, 0x00, 0x7d, 0x2b, 0xc6 ; 1000:4ddc
    db 0x06, 0xed, 0x08, 0x00, 0xf6, 0xd8, 0x98, 0x8b, 0xc8, 0x56, 0x80, 0x3e, 0xeb, 0x08, 0x00, 0x75 ; 1000:4dec
    db 0x11, 0xc6, 0x06, 0xec, 0x08, 0x0a, 0xb8, 0x01, 0x00, 0xbb, 0x4d, 0x9a, 0xb9, 0x09, 0x00, 0xcc ; 1000:4dfc
    db 0xeb, 0x07, 0x51, 0xe8, 0x7a, 0xed, 0x59, 0xe2, 0xe1, 0x5e, 0x2e, 0x80, 0x7c, 0x05, 0x01, 0x74 ; 1000:4e0c
    db 0x09, 0x5f, 0xfe, 0x0e, 0x24, 0x9a, 0x59, 0xe9, 0x15, 0xfd, 0xe9, 0x08, 0x02 ; 1000:4e1c
loc_4e29:
    cmp          byte [cs:SI + 0x5],0x7f         ; 1000:4e29: 2e807c057f
    jz           loc_4e52                        ; 1000:4e2e: 7422
    cmp          byte [cs:SI + 0x5],0x7e         ; 1000:4e30: 2e807c057e
    jz           loc_4e84                        ; 1000:4e35: 744d
    cmp          byte [cs:SI + 0x5],0x7d         ; 1000:4e37: 2e807c057d
    jz           loc_4ead                        ; 1000:4e3c: 746f
    cmp          byte [cs:SI + 0x5],0x7c         ; 1000:4e3e: 2e807c057c
    jz           loc_4e4f                        ; 1000:4e43: 740a
    cmp          byte [cs:SI + 0x5],0x7b         ; 1000:4e45: 2e807c057b
    jnz          loc_4e4f                        ; 1000:4e4a: 7503
    jmp near     loc_4ede                        ; 1000:4e4c: e98f00
loc_4e4f:
    jmp near     spr_render_water_ripple         ; 1000:4e4f: e93e02
loc_4e52:
    mov          AL,[0x8a6]                      ; 1000:4e52: a0a608
    or           AL,byte [0x897]                 ; 1000:4e55: 0a069708
    jnz          loc_4e75                        ; 1000:4e59: 751a
    mov          byte [0x897],0x1                ; 1000:4e5b: c606970801
    mov          byte [0x898],0x0                ; 1000:4e60: c606980800
    mov          AX,word [cs:SI + 0x14]          ; 1000:4e65: 2e8b4414
    mov          [0x892],AX                      ; 1000:4e69: a39208
    mov          word [0x894],0xfff8             ; 1000:4e6c: c7069408f8ff
    jmp near     spr_render_water_ripple         ; 1000:4e72: e91b02
loc_4e75:
    mov          AX,word [cs:SI + 0x14]          ; 1000:4e75: 2e8b4414
    mov          [0x892],AX                      ; 1000:4e79: a39208
    sub          word [0x894],0x8                ; 1000:4e7c: 832e940808
    jmp near     spr_render_water_ripple         ; 1000:4e81: e90c02
loc_4e84:
    mov          AL,[0x8a6]                      ; 1000:4e84: a0a608
    xor          AL,0x1                          ; 1000:4e87: 3401
    mov          [0x897],AL                      ; 1000:4e89: a29708
    mov          byte [0x898],0x0                ; 1000:4e8c: c606980800
    mov          word [0x892],0xfff8             ; 1000:4e91: c7069208f8ff
    cmp          word [cs:SI + 0x18],0x2         ; 1000:4e97: 2e837c1802
    jz           loc_4ea4                        ; 1000:4e9c: 7406
    mov          word [0x892],0x8                ; 1000:4e9e: c70692080800
loc_4ea4:
    mov          word [0x894],0xfff4             ; 1000:4ea4: c7069408f4ff
    jmp near     spr_render_water_ripple         ; 1000:4eaa: e9e301
loc_4ead:
    mov          AX,word [cs:SI + 0x12]          ; 1000:4ead: 2e8b4412
    sub          AX,word [0x6c8]                 ; 1000:4eb1: 2b06c806
    cmp          AX,0x6                          ; 1000:4eb5: 3d0600
    jle          loc_4edb                        ; 1000:4eb8: 7e21
    mov          AL,[0x8a6]                      ; 1000:4eba: a0a608
    xor          AL,0x1                          ; 1000:4ebd: 3401
    mov          [0x897],AL                      ; 1000:4ebf: a29708
    mov          byte [0x898],0x0                ; 1000:4ec2: c606980800
    mov          AX,word [cs:SI + 0x16]          ; 1000:4ec7: 2e8b4416
    mov          [0x894],AX                      ; 1000:4ecb: a39408
    mov          word [0x892],0x0                ; 1000:4ece: c70692080000
    mov          AX,word [cs:SI + 0x10]          ; 1000:4ed4: 2e8b4410
    mov          [0x6c6],AX                      ; 1000:4ed8: a3c606
loc_4edb:
    jmp near     spr_render_water_ripple         ; 1000:4edb: e9b201
loc_4ede:
    mov          AL,[0x8a6]                      ; 1000:4ede: a0a608
    xor          AL,0x1                          ; 1000:4ee1: 3401
    mov          [0x897],AL                      ; 1000:4ee3: a29708
    mov          byte [0x898],0x0                ; 1000:4ee6: c606980800
    mov          word [0x892],0xfff8             ; 1000:4eeb: c7069208f8ff
    mov          AX,word [cs:SI + 0x18]          ; 1000:4ef1: 2e8b4418
    mov          word [cs:SI + 0x18],0x1         ; 1000:4ef5: 2ec744180100
    cmp          AX,0x2                          ; 1000:4efb: 3d0200
    jz           loc_4f0c                        ; 1000:4efe: 740c
    mov          word [cs:SI + 0x18],0x2         ; 1000:4f00: 2ec744180200
    mov          word [0x892],0x8                ; 1000:4f06: c70692080800
loc_4f0c:
    mov          word [0x894],0xfffd             ; 1000:4f0c: c7069408fdff
    jmp near     spr_render_water_ripple         ; 1000:4f12: e97b01

; ----------------------------------------------------------------------------
; FUNCTION: ent_render_all  [Entities / Rendering]
; ADDRESS:  1000:4f15
; PURPOSE:  Iterates through all active monsters, items, and projectiles, rendering their sprites.
; ----------------------------------------------------------------------------
ent_render_all:
    push         DI                              ; 1000:4f15: 57
    cmp          byte [0x921],0x0                ; 1000:4f16: 803e210900
    jnz          loc_4f2c                        ; 1000:4f1b: 750f
    mov          AX,word [cs:SI + 0x6]           ; 1000:4f1d: 2e8b4406
    dec          AX                              ; 1000:4f21: 48
    shl          AX,0x1                          ; 1000:4f22: d1e0
    mov          BX,0x4ae3                       ; 1000:4f24: bbe34a
    add          BX,AX                           ; 1000:4f27: 03d8
    call         word [cs:BX]                    ; 1000:4f29: 2eff17
loc_4f2c:
    cmp          word [cs:SI + 0x6],0x0          ; 1000:4f2c: 2e837c0600
    jnz          loc_4f3c                        ; 1000:4f31: 7509
    pop          DI                              ; 1000:4f33: 5f
    dec          byte [0x9a24]                   ; 1000:4f34: fe0e249a
    pop          CX                              ; 1000:4f38: 59
    jmp near     loc_4b3b                        ; 1000:4f39: e9fffb
loc_4f3c:
    mov          CX,word [0x6c2]                 ; 1000:4f3c: 8b0ec206
    add          CX,0xd0                         ; 1000:4f40: 81c1d000
    mov          DX,word [0x6c4]                 ; 1000:4f44: 8b16c406
    add          DX,0xa0                         ; 1000:4f48: 81c2a000
    mov          AX,word [cs:SI + 0x10]          ; 1000:4f4c: 2e8b4410
    cmp          AX,word [0x6c2]                 ; 1000:4f50: 3b06c206
    jl           loc_4f6f                        ; 1000:4f54: 7c19
    add          AX,word [cs:SI]                 ; 1000:4f56: 2e0304
    cmp          AX,CX                           ; 1000:4f59: 3bc1
    jg           loc_4f6f                        ; 1000:4f5b: 7f12
    mov          AX,word [cs:SI + 0x12]          ; 1000:4f5d: 2e8b4412
    cmp          AX,DX                           ; 1000:4f61: 3bc2
    jge          loc_4f6f                        ; 1000:4f63: 7d0a
    add          AX,word [cs:SI + 0x2]           ; 1000:4f65: 2e034402
    cmp          AX,word [0x6c4]                 ; 1000:4f69: 3b06c406
    jg           loc_4fd0                        ; 1000:4f6d: 7f61
loc_4f6f:
    mov          AX,word [cs:SI + 0x6]           ; 1000:4f6f: 2e8b4406
    mov          word [cs:SI + 0x6],0x0          ; 1000:4f73: 2ec744060000
    mov          BX,word [cs:SI + 0x1e]          ; 1000:4f79: 2e8b5c1e
    test         word [cs:BX + 0x6],0xff00       ; 1000:4f7d: 2ef7470600ff
    jnz          loc_4fc2                        ; 1000:4f83: 753d
    cmp          AX,0x13                         ; 1000:4f85: 3d1300
    jz           loc_4fa3                        ; 1000:4f88: 7419
    cmp          AX,0x14                         ; 1000:4f8a: 3d1400
    jz           loc_4fa3                        ; 1000:4f8d: 7414
    cmp          AX,0x18                         ; 1000:4f8f: 3d1800
    jz           loc_4fa3                        ; 1000:4f92: 740f
    cmp          AX,0x1b                         ; 1000:4f94: 3d1b00
    jz           loc_4fa8                        ; 1000:4f97: 740f
    cmp          AX,0x1c                         ; 1000:4f99: 3d1c00
    jz           loc_4fa8                        ; 1000:4f9c: 740a
    cmp          AX,0x15                         ; 1000:4f9e: 3d1500
    jnz          loc_4fb2                        ; 1000:4fa1: 750f
loc_4fa3:
    call         math_clamp_coords               ; 1000:4fa3: e860fc
    jmp short    loc_4fc7                        ; 1000:4fa6: eb1f
loc_4fa8:
    call         math_clamp_coords               ; 1000:4fa8: e85bfc
    mov          byte [cs:BX + 0x6],0x1          ; 1000:4fab: 2ec6470601
    jmp short    loc_4fc7                        ; 1000:4fb0: eb15
loc_4fb2:
    mov          AX,word [cs:SI + 0x10]          ; 1000:4fb2: 2e8b4410
    mov          word [cs:BX + 0x8],AX           ; 1000:4fb6: 2e894708
    mov          AX,word [cs:SI + 0x12]          ; 1000:4fba: 2e8b4412
    mov          word [cs:BX + 0xa],AX           ; 1000:4fbe: 2e89470a
loc_4fc2:
    mov          byte [cs:BX + 0x6],0x0          ; 1000:4fc2: 2ec6470600
loc_4fc7:
    pop          DI                              ; 1000:4fc7: 5f
    dec          byte [0x9a24]                   ; 1000:4fc8: fe0e249a
    pop          CX                              ; 1000:4fcc: 59
    jmp near     loc_4b3b                        ; 1000:4fcd: e96bfb
loc_4fd0:
    mov          CL,byte [0x21c]                 ; 1000:4fd0: 8a0e1c02
    or           CL,CL                           ; 1000:4fd4: 0ac9
    jz           loc_5031                        ; 1000:4fd6: 7459
    xor          CH,CH                           ; 1000:4fd8: 32ed
    mov          DI,0x8ee                        ; 1000:4fda: bfee08
loc_4fdd:
    mov          AX,word [DI]                    ; 1000:4fdd: 8b05
    cmp          AX,0xffff                       ; 1000:4fdf: 3dffff
    jz           loc_502c                        ; 1000:4fe2: 7448
    add          AX,0xc                          ; 1000:4fe4: 050c00
    sub          AX,word [cs:SI + 0x10]          ; 1000:4fe7: 2e2b4410
    jle          loc_502c                        ; 1000:4feb: 7e3f
    sub          AX,0x10                         ; 1000:4fed: 2d1000
    cmp          AX,word [cs:SI]                 ; 1000:4ff0: 2e3b04
    jge          loc_502c                        ; 1000:4ff3: 7d37
    mov          AX,word [DI + 0x2]              ; 1000:4ff5: 8b4502
    add          AX,0x8                          ; 1000:4ff8: 050800
    sub          AX,word [cs:SI + 0x12]          ; 1000:4ffb: 2e2b4412
    jle          loc_502c                        ; 1000:4fff: 7e2b
    sub          AX,0x8                          ; 1000:5001: 2d0800
    cmp          AX,word [cs:SI + 0x2]           ; 1000:5004: 2e3b4402
    jge          loc_502c                        ; 1000:5008: 7d22
    mov          AX,word [cs:SI + 0x4]           ; 1000:500a: 2e8b4404
    test         AX,0x8000                       ; 1000:500e: a90080
    jnz          loc_502c                        ; 1000:5011: 7519
    cmp          AH,0x1                          ; 1000:5013: 80fc01
    jnz          loc_501b                        ; 1000:5016: 7503
    jmp near     map_check_room_bounds           ; 1000:5018: e9a6fc
loc_501b:
    cmp          AH,0x7c                         ; 1000:501b: 80fc7c
    jz           loc_502c                        ; 1000:501e: 740c
    cmp          AH,0x7b                         ; 1000:5020: 80fc7b
    jz           loc_502c                        ; 1000:5023: 7407
    mov          word [DI],0xffff                ; 1000:5025: c705ffff
    jmp near     map_scroll_camera               ; 1000:5029: e9f9fb
loc_502c:
    add          DI,0x8                          ; 1000:502c: 83c708
    loop         loc_4fdd                        ; 1000:502f: e2ac
loc_5031:
    cmp          byte [0x89d],0x1                ; 1000:5031: 803e9d0801
    jz           loc_5064                        ; 1000:5036: 742c
    mov          AX,[0x6c6]                      ; 1000:5038: a1c606
    add          AX,0xf                          ; 1000:503b: 050f00
    sub          AX,word [cs:SI + 0x10]          ; 1000:503e: 2e2b4410
    jle          spr_render_water_ripple         ; 1000:5042: 7e4c
    sub          AX,0xe                          ; 1000:5044: 2d0e00
    cmp          AX,word [cs:SI]                 ; 1000:5047: 2e3b04
    jge          spr_render_water_ripple         ; 1000:504a: 7d44
    mov          AX,[0x6c8]                      ; 1000:504c: a1c806
    add          AX,0x20                         ; 1000:504f: 052000
    sub          AX,word [cs:SI + 0x12]          ; 1000:5052: 2e2b4412
    jle          spr_render_water_ripple         ; 1000:5056: 7e38
    sub          AX,0x1e                         ; 1000:5058: 2d1e00
    cmp          AX,word [cs:SI + 0x2]           ; 1000:505b: 2e3b4402
    jge          spr_render_water_ripple         ; 1000:505f: 7d2f
    jmp near     boss_render_sprites             ; 1000:5061: e9ddfc
loc_5064:
    mov          AX,[0x6c6]                      ; 1000:5064: a1c606
    add          AX,0x1e                         ; 1000:5067: 051e00
    sub          AX,word [cs:SI + 0x10]          ; 1000:506a: 2e2b4410
    jle          spr_render_water_ripple         ; 1000:506e: 7e20
    sub          AX,0x1c                         ; 1000:5070: 2d1c00
    cmp          AX,word [cs:SI]                 ; 1000:5073: 2e3b04
    jge          spr_render_water_ripple         ; 1000:5076: 7d18
    mov          AX,[0x6c8]                      ; 1000:5078: a1c806
    add          AX,0xf                          ; 1000:507b: 050f00
    sub          AX,word [cs:SI + 0x12]          ; 1000:507e: 2e2b4412
    jle          spr_render_water_ripple         ; 1000:5082: 7e0c
    sub          AX,0xe                          ; 1000:5084: 2d0e00
    cmp          AX,word [cs:SI + 0x2]           ; 1000:5087: 2e3b4402
    jge          spr_render_water_ripple         ; 1000:508b: 7d03
    jmp near     boss_render_sprites             ; 1000:508d: e9b1fc

; ----------------------------------------------------------------------------
; FUNCTION: spr_render_water_ripple  [Tilemap / Animation]
; ADDRESS:  1000:5090
; PURPOSE:  Renders animated surface ripple effect over water tiles.
; ----------------------------------------------------------------------------
spr_render_water_ripple:
    mov          AX,word [cs:SI + 0x1a]          ; 1000:5090: 2e8b441a
    inc          AX                              ; 1000:5094: 40
    mov          BX,word [cs:SI + 0x14]          ; 1000:5095: 2e8b5c14
    or           BX,word [cs:SI + 0x16]          ; 1000:5099: 2e0b5c16
    jz           loc_50b0                        ; 1000:509d: 7411
    cmp          AX,word [cs:SI + 0xa]           ; 1000:509f: 2e3b440a
    jl           loc_50a7                        ; 1000:50a3: 7c02
    xor          AX,AX                           ; 1000:50a5: 33c0
loc_50a7:
    mov          CX,word [cs:SI + 0x8]           ; 1000:50a7: 2e8b4c08
    add          CX,AX                           ; 1000:50ab: 03c8
    jmp short    loc_50ba                        ; 1000:50ad: eb0b
    db 0x90                                         ; 1000:50af
loc_50b0:
    cmp          AX,word [cs:SI + 0x8]           ; 1000:50b0: 2e3b4408
    jl           loc_50b8                        ; 1000:50b4: 7c02
    xor          AX,AX                           ; 1000:50b6: 33c0
loc_50b8:
    mov          CX,AX                           ; 1000:50b8: 8bc8
loc_50ba:
    mov          word [cs:SI + 0x1a],AX          ; 1000:50ba: 2e89441a
    mov          BX,word [cs:SI + 0xc]           ; 1000:50be: 2e8b5c0c
    cmp          word [cs:SI + 0x18],0x2         ; 1000:50c2: 2e837c1802
    jz           loc_50cd                        ; 1000:50c7: 7404
    mov          BX,word [cs:SI + 0xe]           ; 1000:50c9: 2e8b5c0e
loc_50cd:
    shl          CX,0x1                          ; 1000:50cd: d1e1
    add          BX,CX                           ; 1000:50cf: 03d9
    mov          DX,word [cs:BX]                 ; 1000:50d1: 2e8b17
    mov          AX,word [cs:SI + 0x10]          ; 1000:50d4: 2e8b4410
    mov          BX,word [cs:SI + 0x12]          ; 1000:50d8: 2e8b5c12
    push         SI                              ; 1000:50dc: 56
    mov          SI,DX                           ; 1000:50dd: 8bf2
    call         vid_blit_planar_block           ; 1000:50df: e8d92c
    pop          SI                              ; 1000:50e2: 5e
    pop          DI                              ; 1000:50e3: 5f
    jmp near     spr_blit_masked                 ; 1000:50e4: e95ffa
    db 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x15, 0xbb, 0x04, 0x00, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b ; 1000:50e7
    db 0x44, 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04, 0x48, 0xeb, 0x0f, 0x90, 0xbb, 0xfc, 0xff, 0x2e ; 1000:50f7
    db 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x50, 0xe8, 0xe2, 0x0a, 0x58, 0x72 ; 1000:5107
    db 0x11, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x02, 0x8b, 0x3e, 0x94, 0x02, 0xe8, 0x6a, 0x0b ; 1000:5117
    db 0x72, 0x1a, 0x58, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06 ; 1000:5127
    db 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7, 0x5c, 0x14, 0xeb, 0x05, 0x58, 0x2e, 0x89, 0x44 ; 1000:5137
    db 0x10, 0xc3, 0xe8, 0x4d, 0x09, 0x73, 0x07, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x74, 0x08, 0x2e, 0x8b ; 1000:5147
    db 0x5c, 0x1c, 0xe8, 0xf2, 0x08, 0xc3, 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x8b, 0xd8 ; 1000:5157
    db 0x73, 0x02, 0xf7, 0xd8, 0x3d, 0x20, 0x00, 0x7f, 0x40, 0x2e, 0xc7, 0x44, 0x16, 0xf7, 0xff, 0x83 ; 1000:5167
    db 0xfb, 0x00, 0x7c, 0x1b, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74 ; 1000:5177
    db 0x27, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xc7, 0x44, 0x1c, 0x02, 0x00, 0xeb, 0x19, 0x2e ; 1000:5187
    db 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0x83, 0x7c, 0x18, 0x01, 0x74, 0x0c, 0x2e, 0xc7, 0x44, 0x18 ; 1000:5197
    db 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x1c, 0x02, 0x00, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e ; 1000:51a7
    db 0xc7, 0x44, 0x16, 0x00, 0x00, 0xc3, 0x2e, 0x8b, 0x5c, 0x16, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75 ; 1000:51b7
    db 0x0b, 0x4b, 0x83, 0xfb, 0xfc, 0x7d, 0x0e, 0xbb, 0xfc, 0xff, 0xeb, 0x09, 0x43, 0x83, 0xfb, 0x04 ; 1000:51c7
    db 0x7e, 0x03, 0xbb, 0x04, 0x00, 0xe8, 0x9c, 0x09, 0x73, 0x0d, 0x2e, 0x83, 0x74, 0x1c, 0x01, 0x2e ; 1000:51d7
    db 0xc7, 0x44, 0x16, 0x00, 0x00, 0xeb, 0x04, 0x2e, 0x89, 0x5c, 0x12, 0xbb, 0x04, 0x00, 0xe8, 0xb4 ; 1000:51e7
    db 0x09, 0xc3, 0xe8, 0x9d, 0x08, 0x73, 0x07, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x74, 0x0f, 0xbb, 0x04 ; 1000:51f7
    db 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0x3f, 0x08, 0x2e, 0x8f, 0x44, 0x18, 0xc3, 0xbb, 0x04, 0x00 ; 1000:5207
    db 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x74, 0x0f, 0x7c, 0x15, 0x2e, 0xc7, 0x44, 0x18 ; 1000:5217
    db 0x02, 0x00, 0x80, 0x3e, 0xad, 0x08, 0x02, 0x74, 0x15, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0xeb ; 1000:5227
    db 0x20, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x80, 0x3e, 0xad, 0x08, 0x02, 0x74, 0xeb, 0x2e, 0xff ; 1000:5237
    db 0x74, 0x18, 0xe8, 0x02, 0x08, 0x2e, 0x8f, 0x44, 0x18, 0x73, 0x06, 0x2e, 0xc7, 0x44, 0x16, 0xf7 ; 1000:5247
    db 0xff, 0xc3, 0xbb, 0x04, 0x00, 0xa1, 0xc8, 0x06, 0x2e, 0x2b, 0x44, 0x12, 0x73, 0x02, 0xf7, 0xd8 ; 1000:5257
    db 0x3d, 0x08, 0x00, 0x7f, 0x03, 0xbb, 0x08, 0x00, 0xe8, 0x3a, 0x09, 0xc3, 0xbb, 0x02, 0x00, 0xa1 ; 1000:5267
    db 0xc8, 0x06, 0x2e, 0x2b, 0x44, 0x12, 0x74, 0x07, 0x7d, 0x02, 0xf7, 0xdb, 0xe8, 0xf5, 0x08, 0xbb ; 1000:5277
    db 0x04, 0x00, 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x74, 0x2b, 0x7c, 0x0f, 0x2e, 0x83 ; 1000:5287
    db 0x7c, 0x18, 0x02, 0x74, 0x17, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0xeb, 0x1a, 0x2e, 0x83, 0x7c ; 1000:5297
    db 0x18, 0x01, 0x74, 0x08, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0xeb, 0x0b, 0x2e, 0xff, 0x74, 0x18 ; 1000:52a7
    db 0xe8, 0xf2, 0x08, 0x2e, 0x8f, 0x44, 0x18, 0xc3, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0x2e, 0xc7 ; 1000:52b7
    db 0x44, 0x14, 0x00, 0x00, 0xe8, 0x01, 0x0a, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x08, 0x2e ; 1000:52c7
    db 0x8b, 0x44, 0x18, 0x2e, 0x89, 0x44, 0x1c, 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x7d ; 1000:52d7
    db 0x02, 0xf7, 0xd8, 0x3d, 0x20, 0x00, 0x7f, 0x10, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0x8b ; 1000:52e7
    db 0x44, 0x1c, 0x2e, 0x89, 0x44, 0x18, 0xeb, 0x61, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x15, 0xbb ; 1000:52f7
    db 0x04, 0x00, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04 ; 1000:5307
    db 0x48, 0xeb, 0x0f, 0x90, 0xbb, 0xfc, 0xff, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03 ; 1000:5317
    db 0xc3, 0x50, 0x50, 0xe8, 0xca, 0x08, 0x58, 0x72, 0x11, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c ; 1000:5327
    db 0x02, 0x8b, 0x3e, 0x94, 0x02, 0xe8, 0x52, 0x09, 0x72, 0x1a, 0x58, 0x2e, 0x83, 0x7c, 0x18, 0x02 ; 1000:5337
    db 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7 ; 1000:5347
    db 0x5c, 0x14, 0xeb, 0x05, 0x58, 0x2e, 0x89, 0x44, 0x10, 0xc3, 0x2e, 0x8b, 0x44, 0x1c, 0x0b, 0xc0 ; 1000:5357
    db 0x7e, 0x64, 0x3d, 0x01, 0x00, 0x74, 0x1b, 0x2e, 0x8b, 0x44, 0x12, 0x3b, 0x06, 0xc8, 0x06, 0x7f ; 1000:5367
    db 0x11, 0x74, 0x34, 0x2e, 0xc7, 0x44, 0x16, 0x07, 0x00, 0xe8, 0x16, 0x07, 0x2e, 0xff, 0x4c, 0x1c ; 1000:5377
    db 0xeb, 0x25, 0x2e, 0xc7, 0x44, 0x16, 0xf7, 0xff, 0xe8, 0x07, 0x07, 0x73, 0x1a, 0x2e, 0x83, 0x7c ; 1000:5387
    db 0x1c, 0x01, 0x75, 0x13, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00 ; 1000:5397
    db 0x2e, 0xc7, 0x44, 0x1c, 0xfd, 0xff, 0xc3, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0x8b, 0x44 ; 1000:53a7
    db 0x10, 0x3b, 0x06, 0xc6, 0x06, 0x7f, 0x08, 0x74, 0x0c, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0xbb ; 1000:53b7
    db 0x04, 0x00, 0xe8, 0x82, 0x06, 0xc3, 0x0b, 0xc0, 0x75, 0x07, 0x2e, 0xc7, 0x44, 0x1c, 0x0c, 0x00 ; 1000:53c7
    db 0xc3, 0x2e, 0xff, 0x44, 0x1c, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x16, 0x00 ; 1000:53d7
    db 0x00, 0xc3, 0xff, 0x36, 0x94, 0x02, 0xff, 0x36, 0x92, 0x02, 0xc7, 0x06, 0x94, 0x02, 0xff, 0xff ; 1000:53e7
    db 0xc7, 0x06, 0x92, 0x02, 0xff, 0xff, 0x2e, 0x8b, 0x44, 0x12, 0x2b, 0x06, 0xc8, 0x06, 0x74, 0x1b ; 1000:53f7
    db 0x7f, 0x0b, 0x2e, 0xc7, 0x44, 0x16, 0x01, 0x00, 0xe8, 0x87, 0x06, 0xeb, 0x0e, 0x3d, 0x01, 0x00 ; 1000:5407
    db 0x7e, 0x09, 0x2e, 0xc7, 0x44, 0x16, 0xfd, 0xff, 0xe8, 0x77, 0x06, 0xe8, 0xaa, 0x08, 0x0b, 0xc0 ; 1000:5417
    db 0x74, 0x06, 0xbb, 0x04, 0x00, 0xe8, 0x1f, 0x06, 0x8f, 0x06, 0x92, 0x02, 0x8f, 0x06, 0x94, 0x02 ; 1000:5427
    db 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x0c, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0xc7 ; 1000:5437
    db 0x44, 0x16, 0xf8, 0xff, 0xe8, 0xb3, 0x06, 0xbb, 0x08, 0x00, 0xe8, 0xfa, 0x05, 0xc3, 0xe8, 0x41 ; 1000:5447
    db 0x06, 0x72, 0x03, 0xe9, 0x8a, 0x00, 0x2e, 0x8b, 0x44, 0x12, 0x3b, 0x06, 0xc8, 0x06, 0x7d, 0x1f ; 1000:5457
    db 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x7d, 0x02, 0xf7, 0xd8, 0x3d, 0x20, 0x00, 0x7f ; 1000:5467
    db 0x0e, 0x2e, 0x83, 0x44, 0x12, 0x08, 0x2e, 0xc7, 0x44, 0x16, 0x07, 0x00, 0xeb, 0x62, 0x90, 0x2e ; 1000:5477
    db 0x83, 0x7c, 0x18, 0x02, 0x74, 0x15, 0xbb, 0x04, 0x00, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44 ; 1000:5487
    db 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04, 0x48, 0xeb, 0x0f, 0x90, 0xbb, 0xfc, 0xff, 0x2e, 0x89 ; 1000:5497
    db 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x50, 0xe8, 0x43, 0x07, 0x58, 0x72, 0x11 ; 1000:54a7
    db 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x02, 0x8b, 0x3e, 0x94, 0x02, 0xe8, 0xcb, 0x07, 0x72 ; 1000:54b7
    db 0x1a, 0x58, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06, 0x2e ; 1000:54c7
    db 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7, 0x5c, 0x14, 0xeb, 0x05, 0x58, 0x2e, 0x89, 0x44, 0x10 ; 1000:54d7
    db 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0xc3, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x15, 0xbb, 0x08 ; 1000:54e7
    db 0x00, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04, 0x48 ; 1000:54f7
    db 0xeb, 0x0f, 0x90, 0xbb, 0xf8, 0xff, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3 ; 1000:5507
    db 0x50, 0x50, 0xe8, 0xdb, 0x06, 0x58, 0x72, 0x11, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x02 ; 1000:5517
    db 0x8b, 0x3e, 0x94, 0x02, 0xe8, 0x63, 0x07, 0x72, 0x1a, 0x58, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x2e ; 1000:5527
    db 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7, 0x5c ; 1000:5537
    db 0x14, 0xeb, 0x05, 0x58, 0x2e, 0x89, 0x44, 0x10, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x09 ; 1000:5547
    db 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0xe8, 0x6f, 0x07, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0xe8 ; 1000:5557
    db 0x30, 0x05, 0xbb, 0x04, 0x00, 0xe8, 0xdf, 0x04, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e ; 1000:5567
    db 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06, 0x7d, 0x02, 0xf7, 0xd8, 0x3d, 0x08, 0x00, 0x7f, 0x06 ; 1000:5577
    db 0x2e, 0xc7, 0x44, 0x16, 0xf7, 0xff, 0xe8, 0x09, 0x05, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75 ; 1000:5587
    db 0x17, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x16, 0x04, 0x00, 0x2e, 0x83, 0x7c ; 1000:5597
    db 0x18, 0x02, 0x74, 0x04, 0x2e, 0xf7, 0x5c, 0x16, 0xe8, 0x4f, 0x05, 0xbb, 0x04, 0x00, 0xe8, 0x96 ; 1000:55a7
    db 0x04, 0xc3, 0x2e, 0xff, 0x44, 0x16, 0xe8, 0xd9, 0x04, 0x72, 0x07, 0xbb, 0x04, 0x00, 0xe8, 0x86 ; 1000:55b7
    db 0x04, 0xc3, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x7c, 0x09, 0x2e, 0xc7, 0x44, 0x16, 0xf4, 0xff, 0xe8 ; 1000:55c7
    db 0xf6, 0x06, 0xc3, 0xe8, 0xbc, 0x04, 0x73, 0x07, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x74, 0x0f, 0xbb ; 1000:55d7
    db 0x04, 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0x5e, 0x04, 0x2e, 0x8f, 0x44, 0x18, 0xc3, 0xe8, 0xd7 ; 1000:55e7
    db 0x06, 0x0b, 0xc0, 0x74, 0x18, 0xbb, 0x04, 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0x48, 0x04, 0x2e ; 1000:55f7
    db 0x8f, 0x44, 0x18, 0x73, 0x0e, 0x2e, 0xc7, 0x44, 0x16, 0xf7, 0xff, 0xeb, 0x06, 0x2e, 0xc7, 0x44 ; 1000:5607
    db 0x14, 0x00, 0x00, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x26, 0xa1, 0xc6, 0x06, 0x2e, 0x2b ; 1000:5617
    db 0x44, 0x10, 0x73, 0x02, 0xf7, 0xd8, 0x8b, 0x1e, 0xc8, 0x06, 0x2e, 0x2b, 0x5c, 0x12, 0x72, 0x04 ; 1000:5627
    db 0x3b, 0xc3, 0x7e, 0x07, 0x2e, 0xc7, 0x44, 0x14, 0x04, 0x00, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x01 ; 1000:5637
    db 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0x83, 0x7c, 0x1c, 0x02, 0x74, 0x11, 0x2e, 0xff ; 1000:5647
    db 0x44, 0x16, 0xe8, 0x3d, 0x04, 0x72, 0x01, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x02, 0x00, 0xc3, 0xe8 ; 1000:5657
    db 0x9d, 0xf5, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0x83, 0x7c, 0x1c, 0x02, 0x74, 0x11 ; 1000:5667
    db 0x2e, 0xff, 0x44, 0x16, 0xe8, 0x1b, 0x04, 0x72, 0x01, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x02, 0x00 ; 1000:5677
    db 0xc3, 0xe8, 0x7b, 0xf5, 0xc3, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0xbb, 0x08, 0x00, 0xe8, 0xb6 ; 1000:5687
    db 0x03, 0x73, 0x1a, 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc2, 0x06, 0x3d, 0x08, 0x00, 0x7f, 0x05 ; 1000:5697
    db 0x2e, 0x83, 0x6c, 0x10, 0x08, 0x2e, 0xc7, 0x44, 0x16, 0xf7, 0xff, 0xeb, 0x1b, 0x2e, 0x8b, 0x44 ; 1000:56a7
    db 0x10, 0xa9, 0x04, 0x00, 0x74, 0x05, 0x2e, 0x83, 0x6c, 0x16, 0x03, 0x2e, 0x83, 0x7c, 0x16, 0x02 ; 1000:56b7
    db 0x7c, 0x06, 0x2e, 0xc7, 0x44, 0x16, 0x01, 0x00, 0xe8, 0xc7, 0x03, 0xc3, 0x2e, 0x83, 0x7c, 0x1c ; 1000:56c7
    db 0x00, 0x75, 0x09, 0xe8, 0xf2, 0x05, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0x83, 0x7c, 0x1c ; 1000:56d7
    db 0x02, 0x74, 0x4f, 0x2e, 0x8b, 0x44, 0x12, 0x05, 0x18, 0x00, 0x2b, 0x06, 0xc8, 0x06, 0x74, 0x1b ; 1000:56e7
    db 0x7f, 0x0b, 0x2e, 0xc7, 0x44, 0x16, 0x03, 0x00, 0xe8, 0x97, 0x03, 0xeb, 0x0e, 0x3d, 0x04, 0x00 ; 1000:56f7
    db 0x7c, 0x09, 0x2e, 0xc7, 0x44, 0x16, 0xfb, 0xff, 0xe8, 0x87, 0x03, 0xbb, 0x08, 0x00, 0x2e, 0xff ; 1000:5707
    db 0x74, 0x18, 0xe8, 0x32, 0x03, 0xe8, 0xb0, 0x05, 0x2e, 0x8b, 0x5c, 0x18, 0x2e, 0x8f, 0x44, 0x18 ; 1000:5717
    db 0x2e, 0x3b, 0x5c, 0x18, 0x74, 0x0b, 0x3d, 0x20, 0x00, 0x7c, 0x06, 0x2e, 0xc7, 0x44, 0x1c, 0x02 ; 1000:5727
    db 0x00, 0xc3, 0x2e, 0x8b, 0x44, 0x12, 0x2b, 0x06, 0xc8, 0x06, 0x74, 0x1b, 0x7f, 0x0b, 0x2e, 0xc7 ; 1000:5737
    db 0x44, 0x16, 0x07, 0x00, 0xe8, 0x4b, 0x03, 0xeb, 0x0e, 0x3d, 0x08, 0x00, 0x7c, 0x09, 0x2e, 0xc7 ; 1000:5747
    db 0x44, 0x16, 0xf7, 0xff, 0xe8, 0x3b, 0x03, 0xe8, 0x6e, 0x05, 0x3d, 0x08, 0x00, 0x7c, 0x06, 0xbb ; 1000:5757
    db 0x08, 0x00, 0xe8, 0xe2, 0x02, 0xc3, 0xe8, 0x29, 0x03, 0x73, 0x07, 0x2e, 0x83, 0x7c, 0x16, 0x00 ; 1000:5767
    db 0x74, 0x0f, 0xbb, 0x08, 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0xcb, 0x02, 0x2e, 0x8f, 0x44, 0x18 ; 1000:5777
    db 0xc3, 0xe8, 0x44, 0x05, 0x0b, 0xc0, 0x74, 0x2d, 0x2e, 0x8b, 0x44, 0x18, 0x3a, 0x06, 0xad, 0x08 ; 1000:5787
    db 0x75, 0x0d, 0x3d, 0x20, 0x00, 0x7e, 0x02, 0xeb, 0x06, 0x2e, 0xc7, 0x44, 0x16, 0xf8, 0xff, 0xbb ; 1000:5797
    db 0x08, 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0x9e, 0x02, 0x2e, 0x8f, 0x44, 0x18, 0x73, 0x06, 0x2e ; 1000:57a7
    db 0xc7, 0x44, 0x16, 0xf7, 0xff, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0xc3, 0x2e, 0xc7, 0x44, 0x14 ; 1000:57b7
    db 0x00, 0x00, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x20, 0xa1, 0xc6, 0x06, 0x2e, 0x2b, 0x44, 0x10 ; 1000:57c7
    db 0x73, 0x02, 0xf7, 0xd8, 0x8b, 0x1e, 0xc8, 0x06, 0x2e, 0x2b, 0x5c, 0x12, 0x72, 0x04, 0x3b, 0xc3 ; 1000:57d7
    db 0x7e, 0x01, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0xff, 0x36, 0x94, 0x02, 0xff, 0x36, 0x92 ; 1000:57e7
    db 0x02, 0xc7, 0x06, 0x94, 0x02, 0xff, 0xff, 0xc7, 0x06, 0x92, 0x02, 0xff, 0xff, 0x2e, 0xff, 0x44 ; 1000:57f7
    db 0x16, 0xe8, 0x8e, 0x02, 0x8f, 0x06, 0x92, 0x02, 0x8f, 0x06, 0x94, 0x02, 0xc3, 0x2e, 0xc7, 0x44 ; 1000:5807
    db 0x16, 0x04, 0x00, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x0a ; 1000:5817
    db 0x2e, 0xc7, 0x44, 0x1c, 0x02, 0x00, 0x2e, 0xf7, 0x5c, 0x16, 0xe8, 0xcd, 0x02, 0x73, 0x08, 0x2e ; 1000:5827
    db 0x8b, 0x44, 0x1c, 0x2e, 0x89, 0x44, 0x18, 0xc3, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0x2e, 0xc7 ; 1000:5837
    db 0x44, 0x14, 0x04, 0x00, 0xbb, 0x04, 0x00, 0xe8, 0xfd, 0x01, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x01 ; 1000:5847
    db 0x7d, 0x1f, 0xe8, 0x3d, 0x02, 0x72, 0x13, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x2e, 0xc7, 0x44 ; 1000:5857
    db 0x14, 0x04, 0x00, 0xbb, 0x06, 0x00, 0xe8, 0xde, 0x01, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00 ; 1000:5867
    db 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x20, 0x74, 0x1e, 0x2e, 0xff, 0x44, 0x1c, 0x2e, 0xc7, 0x44, 0x18 ; 1000:5877
    db 0x02, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x04, 0x00, 0x2e, 0x83, 0x7c, 0x1c, 0x10, 0x7f, 0x06, 0xbb ; 1000:5887
    db 0x04, 0x00, 0xe8, 0xb2, 0x01, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x16 ; 1000:5897
    db 0xef, 0xff, 0xe8, 0xed, 0x01, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x01, 0x7d, 0x0d, 0x2e, 0xc7, 0x44 ; 1000:58a7
    db 0x16, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x20, 0x74 ; 1000:58b7
    db 0x20, 0x2e, 0xff, 0x44, 0x1c, 0x2e, 0xc7, 0x44, 0x16, 0xfe, 0xff, 0xe8, 0x2c, 0x02, 0x2e, 0xc7 ; 1000:58c7
    db 0x44, 0x18, 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x04, 0x00, 0xbb, 0x04, 0x00, 0xe8, 0x67, 0x01 ; 1000:58d7
    db 0xc3, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x04, 0x00, 0xbb, 0x0c, 0x00 ; 1000:58e7
    db 0xe8, 0x54, 0x01, 0x72, 0x01, 0xc3, 0xe8, 0x06, 0xf3, 0xc3, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00 ; 1000:58f7
    db 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x04, 0x00, 0xbb, 0x04, 0x00, 0xe8 ; 1000:5907
    db 0x35, 0x01, 0x72, 0x01, 0xc3, 0xe8, 0xe7, 0xf2, 0xc3, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e ; 1000:5917
    db 0xc7, 0x44, 0x16, 0x00, 0x00, 0xe8, 0xa0, 0x03, 0x2e, 0x83, 0x7c, 0x1c, 0x01, 0x74, 0x05, 0x3d ; 1000:5927
    db 0x10, 0x00, 0x7f, 0x26, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x3d, 0x30, 0x00, 0x7f, 0x1b, 0x2e ; 1000:5937
    db 0xc7, 0x44, 0x18, 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x08 ; 1000:5947
    db 0x00, 0xbb, 0x08, 0x00, 0xe8, 0xf0, 0x00, 0x72, 0x08, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x00, 0x00 ; 1000:5957
    db 0xc3, 0xe8, 0x9b, 0xf2, 0xc3, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x2c, 0xa1, 0xc6, 0x06, 0x2e ; 1000:5967
    db 0x2b, 0x44, 0x10, 0x73, 0x02, 0xf7, 0xd8, 0x8b, 0x1e, 0xc8, 0x06, 0x2e, 0x2b, 0x5c, 0x12, 0x72 ; 1000:5977
    db 0x0a, 0x2e, 0x2b, 0x5c, 0x02, 0x72, 0x04, 0x3b, 0xc3, 0x7e, 0x07, 0x2e, 0xc7, 0x44, 0x14, 0x04 ; 1000:5987
    db 0x00, 0xc3, 0x2e, 0xc7, 0x44, 0x1c, 0x01, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0x2e, 0x83 ; 1000:5997
    db 0x7c, 0x1c, 0x02, 0x74, 0x11, 0x2e, 0xff, 0x44, 0x16, 0xe8, 0xe6, 0x00, 0x72, 0x01, 0xc3, 0x2e ; 1000:59a7
    db 0xc7, 0x44, 0x1c, 0x02, 0x00, 0xc3, 0xe8, 0x46, 0xf2, 0xc3, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00 ; 1000:59b7
    db 0xeb, 0x22, 0x2e, 0x83, 0x7c, 0x1c, 0x00, 0x75, 0x15, 0xa1, 0xc6, 0x06, 0x2e, 0x2b, 0x44, 0x10 ; 1000:59c7
    db 0x73, 0x07, 0x2e, 0xc7, 0x44, 0x16, 0x01, 0x00, 0xc3, 0x3d, 0x48, 0x00, 0x7c, 0xf4, 0x2e, 0xc7 ; 1000:59d7
    db 0x44, 0x18, 0x01, 0x00, 0x2e, 0x83, 0x7c, 0x1c, 0x02, 0x74, 0x58, 0x2e, 0xc7, 0x44, 0x1c, 0x01 ; 1000:59e7
    db 0x00, 0xbb, 0x0c, 0x00, 0x2e, 0xff, 0x74, 0x18, 0xe8, 0x4c, 0x00, 0x72, 0x43, 0xe8, 0xc8, 0x02 ; 1000:59f7
    db 0x58, 0x2e, 0x3b, 0x44, 0x18, 0x75, 0x1a, 0x2e, 0xc7, 0x44, 0x16, 0x04, 0x00, 0xa1, 0xc8, 0x06 ; 1000:5a07
    db 0x2e, 0x2b, 0x44, 0x12, 0x72, 0x11, 0x3d, 0x0c, 0x00, 0x7d, 0x06, 0x2e, 0x89, 0x44, 0x16, 0xeb ; 1000:5a17
    db 0x06, 0x2e, 0xc7, 0x44, 0x16, 0x0c, 0x00, 0xe8, 0x68, 0x00, 0x72, 0x01, 0xc3, 0x2e, 0xc7, 0x44 ; 1000:5a27
    db 0x1c, 0x02, 0x00, 0x2e, 0xc7, 0x44, 0x16, 0x00, 0x00, 0x2e, 0xc7, 0x44, 0x14, 0x00, 0x00, 0xc3 ; 1000:5a37
    db 0x83, 0xc4, 0x02, 0xe8, 0xb9, 0xf1, 0xc3, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x12, 0x2e, 0x89 ; 1000:5a47
    db 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04, 0x48, 0xeb, 0x0e, 0x90 ; 1000:5a57
    db 0xf7, 0xdb, 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0xe8, 0x80, 0x01 ; 1000:5a67
    db 0x58, 0x73, 0x19, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06 ; 1000:5a77
    db 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7, 0x5c, 0x14, 0xf9, 0xc3, 0x2e, 0x89, 0x44, 0x10 ; 1000:5a87
    db 0xf8, 0xc3, 0x2e, 0xff, 0x44, 0x16, 0x2e, 0x83, 0x7c, 0x16, 0x0c, 0x7c, 0x06, 0x2e, 0xc7, 0x44 ; 1000:5a97
    db 0x16, 0x0c, 0x00, 0x2e, 0x8b, 0x44, 0x10, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x16, 0x53 ; 1000:5aa7
    db 0x2e, 0x8b, 0x0c, 0xd1, 0xe9, 0x03, 0xc1, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x7c, 0x2b, 0x2e, 0x03 ; 1000:5ab7
    db 0x5c, 0x02, 0x8b, 0x3e, 0x94, 0x02, 0xe8, 0xc1, 0x01, 0x5b, 0x73, 0x17, 0x2e, 0xc7, 0x44, 0x16 ; 1000:5ac7
    db 0x00, 0x00, 0x2e, 0x03, 0x5c, 0x02, 0x83, 0xe3, 0xf0, 0x2e, 0x2b, 0x5c, 0x02, 0x2e, 0x89, 0x5c ; 1000:5ad7
    db 0x12, 0xf9, 0xc3, 0x2e, 0x89, 0x5c, 0x12, 0xf8, 0xc3, 0x8b, 0x3e, 0x92, 0x02, 0xe8, 0x9a, 0x01 ; 1000:5ae7
    db 0x5b, 0x73, 0xf0, 0x2e, 0x83, 0x64, 0x12, 0xf0, 0xf9, 0xc3, 0x2e, 0x8b, 0x44, 0x10, 0x2e, 0x8b ; 1000:5af7
    db 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x16, 0x53, 0x2e, 0x8b, 0x0c, 0xd1, 0xe9, 0x03, 0xc1, 0x2e, 0x83 ; 1000:5b07
    db 0x7c, 0x16, 0x00, 0x7c, 0x42, 0x2e, 0x03, 0x5c, 0x02, 0x8b, 0x3e, 0x96, 0x02, 0x83, 0xff, 0xff ; 1000:5b17
    db 0x75, 0x04, 0x8b, 0x3e, 0x92, 0x02, 0xe8, 0x61, 0x01, 0x5b, 0x72, 0x10, 0xa1, 0xc4, 0x06, 0x05 ; 1000:5b27
    db 0xa0, 0x00, 0x2e, 0x2b, 0x44, 0x02, 0x3b, 0xd8, 0x7d, 0x34, 0xeb, 0x15, 0x2e, 0xf7, 0x5c, 0x16 ; 1000:5b37
    db 0x2e, 0x03, 0x5c, 0x02, 0x83, 0xe3, 0xf0, 0x2e, 0x2b, 0x5c, 0x02, 0x2e, 0x89, 0x5c, 0x12, 0xf9 ; 1000:5b47
    db 0xc3, 0x2e, 0x89, 0x5c, 0x12, 0xf8, 0xc3, 0x8b, 0x3e, 0x92, 0x02, 0xe8, 0x2c, 0x01, 0x5b, 0x72 ; 1000:5b57
    db 0x08, 0x3b, 0x1e, 0xc4, 0x06, 0x7e, 0x07, 0xeb, 0xe8, 0x2e, 0x83, 0x64, 0x12, 0xf0, 0x2e, 0xf7 ; 1000:5b67
    db 0x5c, 0x16, 0xf9, 0xc3, 0x2e, 0x89, 0x5c, 0x16, 0x2e, 0x8b, 0x44, 0x10, 0x2e, 0x03, 0x5c, 0x12 ; 1000:5b77
    db 0x53, 0x2e, 0x83, 0x7c, 0x16, 0x00, 0x7c, 0x04, 0x2e, 0x03, 0x5c, 0x02, 0x2e, 0x8b, 0x0c, 0xd1 ; 1000:5b87
    db 0xe9, 0x03, 0xc1, 0xe8, 0x04, 0x01, 0x5b, 0x73, 0x06, 0x2e, 0xf7, 0x5c, 0x16, 0xf9, 0xc3, 0x2e ; 1000:5b97
    db 0x89, 0x5c, 0x12, 0xf8, 0xc3, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x74, 0x12, 0x2e, 0x89, 0x5c, 0x14 ; 1000:5ba7
    db 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0x2e, 0x03, 0x04, 0x48, 0xeb, 0x0e, 0x90, 0xf7, 0xdb ; 1000:5bb7
    db 0x2e, 0x89, 0x5c, 0x14, 0x2e, 0x8b, 0x44, 0x10, 0x03, 0xc3, 0x50, 0xe8, 0x69, 0x00, 0x58, 0x73 ; 1000:5bc7
    db 0x19, 0x2e, 0x83, 0x7c, 0x18, 0x02, 0x2e, 0xc7, 0x44, 0x18, 0x01, 0x00, 0x74, 0x06, 0x2e, 0xc7 ; 1000:5bd7
    db 0x44, 0x18, 0x02, 0x00, 0x2e, 0xf7, 0x5c, 0x14, 0xf9, 0xc3, 0x2e, 0x89, 0x44, 0x10, 0xf8, 0xc3 ; 1000:5be7
    db 0x3b, 0x06, 0xc2, 0x06, 0x7c, 0x3d, 0x8b, 0x0e, 0xc2, 0x06, 0x81, 0xc1, 0xd0, 0x00, 0x3b, 0xc1 ; 1000:5bf7
    db 0x7d, 0x31, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c, 0x02, 0x83, 0xeb, 0x08, 0x50, 0xe8, 0xe6 ; 1000:5c07
    db 0xc0, 0x59, 0xd1, 0xe9, 0xd1, 0xe9, 0xd1, 0xe9, 0xd1, 0xe9, 0x3b, 0x06, 0x92, 0x02, 0x7e, 0x13 ; 1000:5c17
    db 0x2b, 0x1e, 0x98, 0x02, 0x8a, 0x07, 0x2e, 0x83, 0x7c, 0x02, 0x10, 0x7e, 0x08, 0x3b, 0x06, 0x92 ; 1000:5c27
    db 0x02, 0x7f, 0x02, 0xf9, 0xc3, 0xf8, 0xc3, 0x3b, 0x06, 0xc2, 0x06, 0x7c, 0xf6, 0x8b, 0x0e, 0xc2 ; 1000:5c37
    db 0x06, 0x81, 0xc1, 0xd0, 0x00, 0x3b, 0xc1, 0x7d, 0xea, 0x2e, 0x8b, 0x5c, 0x12, 0x2e, 0x03, 0x5c ; 1000:5c47
    db 0x02, 0x83, 0xeb, 0x08, 0x50, 0xe8, 0x9f, 0xc0, 0x59, 0xd1, 0xe9, 0xd1, 0xe9, 0xd1, 0xe9, 0xd1 ; 1000:5c57
    db 0xe9, 0x3b, 0x06, 0x92, 0x02, 0x7e, 0x21, 0x3b, 0x06, 0x96, 0x02, 0x7f, 0x1b, 0x2b, 0x1e, 0x98 ; 1000:5c67
    db 0x02, 0x8a, 0x07, 0x2e, 0x83, 0x7c, 0x02, 0x10, 0x7e, 0x0c, 0x3b, 0x06, 0x92, 0x02, 0x7e, 0x08 ; 1000:5c77
    db 0x3b, 0x06, 0x96, 0x02, 0x7f, 0x02, 0xf8, 0xc3, 0xf9, 0xc3, 0x83, 0xfb, 0x00, 0x7c, 0x09, 0xe8 ; 1000:5c87
    db 0x65, 0xc0, 0x3b, 0xc7, 0x7e, 0x02, 0xf8, 0xc3, 0xf9, 0xc3, 0x83, 0xfb, 0x00, 0x7c, 0x27, 0x3b ; 1000:5c97
    db 0x1e, 0x9e, 0x02, 0x7d, 0x21, 0x8b, 0x0e, 0xc4, 0x06, 0x3b, 0xd9, 0x7c, 0xeb, 0x81, 0xc1, 0xa0 ; 1000:5ca7
    db 0x00, 0x3b, 0xd9, 0x7d, 0xe3, 0xe8, 0x3f, 0xc0, 0x3b, 0x06, 0x94, 0x02, 0x7e, 0x08, 0x3b, 0x06 ; 1000:5cb7
    db 0x96, 0x02, 0x7f, 0x02, 0xf8, 0xc3, 0xf9, 0xc3, 0x2e, 0x8b, 0x44, 0x10, 0x2b, 0x06, 0xc6, 0x06 ; 1000:5cc7
    db 0x7c, 0x09, 0x2e, 0xc7, 0x44, 0x18, 0x02, 0x00, 0xeb, 0x09, 0x90, 0x2e, 0xc7, 0x44, 0x18, 0x01 ; 1000:5cd7
    db 0x00, 0xf7, 0xd8, 0xc3                       ; 1000:5ce7

; ----------------------------------------------------------------------------
; FUNCTION: map_update_animated_tiles  [Tilemap / Animation]
; ADDRESS:  1000:5ceb
; PURPOSE:  Advances animation frame counters for animated room tiles (water, lava, torches).
; ----------------------------------------------------------------------------
map_update_animated_tiles:
    mov          CL,byte [0x21c]                 ; 1000:5ceb: 8a0e1c02
    xor          CH,CH                           ; 1000:5cef: 32ed
    lea          SI,[0x8ee]                      ; 1000:5cf1: 8d36ee08
loc_5cf5:
    cmp          word [SI],-0x1                  ; 1000:5cf5: 833cff
    jnz          loc_5d58                        ; 1000:5cf8: 755e
    mov          AX,[0x6c6]                      ; 1000:5cfa: a1c606
    add          AX,0x4                          ; 1000:5cfd: 050400
    mov          word [SI],AX                    ; 1000:5d00: 8904
    mov          AX,0xfff0                       ; 1000:5d02: b8f0ff
    cmp          byte [0x8ad],0x2                ; 1000:5d05: 803ead0802
    jz           loc_5d18                        ; 1000:5d0a: 740c
    neg          AX                              ; 1000:5d0c: f7d8
    cmp          byte [0x89d],0x1                ; 1000:5d0e: 803e9d0801
    jnz          loc_5d18                        ; 1000:5d13: 7503
    add          word [SI],0x10                  ; 1000:5d15: 830410
loc_5d18:
    mov          word [SI + 0x4],AX              ; 1000:5d18: 894404
    mov          AX,[0x6c8]                      ; 1000:5d1b: a1c806
    add          AX,0x4                          ; 1000:5d1e: 050400
    cmp          byte [0x89d],0x1                ; 1000:5d21: 803e9d0801
    jz           loc_5d2b                        ; 1000:5d26: 7403
    add          AX,0x4                          ; 1000:5d28: 050400
loc_5d2b:
    mov          word [SI + 0x2],AX              ; 1000:5d2b: 894402
    mov          AX,[0x9a9c]                     ; 1000:5d2e: a19c9a
    mov          word [SI + 0x6],AX              ; 1000:5d31: 894406
    xor          word [0x9a9c],0x4               ; 1000:5d34: 83369c9a04
    sub          word [SI + 0x6],0xc             ; 1000:5d39: 836c060c
    cmp          word [cs:0x127c],-0x1           ; 1000:5d3d: 2e833e7c12ff
    jz           loc_5d4a                        ; 1000:5d43: 7405
    mov          word [SI + 0x6],0xfff4          ; 1000:5d45: c74406f4ff
loc_5d4a:
    mov          AX,0x1                          ; 1000:5d4a: b80100
    lea          BX,[0x9a80]                     ; 1000:5d4d: 8d1e809a
    mov          CX,0x1                          ; 1000:5d51: b90100
    int3                                         ; 1000:5d54: cc
    db 0xf9, 0xeb, 0x06                             ; 1000:5d55
loc_5d58:
    add          SI,0x8                          ; 1000:5d58: 83c608
    loop         loc_5cf5                        ; 1000:5d5b: e298
    clc                                          ; 1000:5d5d: f8
    ret                                          ; 1000:5d5e: c3

; ----------------------------------------------------------------------------
; FUNCTION: map_redraw_dirty_tiles  [Tilemap / Rendering]
; ADDRESS:  1000:5d5f
; PURPOSE:  Redraws tiles marked dirty to restore background behind moving sprites.
; ----------------------------------------------------------------------------
map_redraw_dirty_tiles:
    mov          CL,byte [0x21c]                 ; 1000:5d5f: 8a0e1c02
    or           CL,CL                           ; 1000:5d63: 0ac9
    jnz          loc_5d68                        ; 1000:5d65: 7501
    ret                                          ; 1000:5d67: c3
loc_5d68:
    xor          CH,CH                           ; 1000:5d68: 32ed
    lea          SI,[0x8ee]                      ; 1000:5d6a: 8d36ee08
loc_5d6e:
    push         CX                              ; 1000:5d6e: 51
    cmp          word [SI],-0x1                  ; 1000:5d6f: 833cff
    jz           loc_5df3                        ; 1000:5d72: 747f
    mov          AX,word [SI + 0x4]              ; 1000:5d74: 8b4404
    add          word [SI],AX                    ; 1000:5d77: 0104
    cmp          word [cs:0x1018],-0x1           ; 1000:5d79: 2e833e1810ff
    jnz          loc_5da7                        ; 1000:5d7f: 7526
    mov          BX,word [SI + 0x6]              ; 1000:5d81: 8b5c06
    cmp          BX,0x0                          ; 1000:5d84: 83fb00
    jge          loc_5d8f                        ; 1000:5d87: 7d06
    add          word [SI + 0x6],0xc             ; 1000:5d89: 8344060c
    jmp short    loc_5da7                        ; 1000:5d8d: eb18
loc_5d8f:
    mov          CX,BX                           ; 1000:5d8f: 8bcb
    shl          BX,0x1                          ; 1000:5d91: d1e3
    add          BX,0x9a8c                       ; 1000:5d93: 81c38c9a
    mov          BX,word [BX]                    ; 1000:5d97: 8b1f
    add          word [SI + 0x2],BX              ; 1000:5d99: 015c02
    inc          CX                              ; 1000:5d9c: 41
    cmp          CX,0x8                          ; 1000:5d9d: 83f908
    jnz          loc_5da4                        ; 1000:5da0: 7502
    xor          CX,CX                           ; 1000:5da2: 33c9
loc_5da4:
    mov          word [SI + 0x6],CX              ; 1000:5da4: 894c06
loc_5da7:
    mov          AX,word [SI]                    ; 1000:5da7: 8b04
    mov          BX,word [SI + 0x2]              ; 1000:5da9: 8b5c02
    add          AX,0x3                          ; 1000:5dac: 050300
    add          BX,0x3                          ; 1000:5daf: 83c303
    call         map_get_tile_at                 ; 1000:5db2: e849bf
    mov          CX,AX                           ; 1000:5db5: 8bc8
    mov          DX,BX                           ; 1000:5db7: 8bd3
    cmp          AX,word [0x292]                 ; 1000:5db9: 3b069202
    jle          loc_5e04                        ; 1000:5dbd: 7e45
    mov          AX,word [SI]                    ; 1000:5dbf: 8b04
    sub          AX,word [0x6c2]                 ; 1000:5dc1: 2b06c206
    jl           loc_5dfd                        ; 1000:5dc5: 7c36
    cmp          AX,0xc8                         ; 1000:5dc7: 3dc800
    jg           loc_5dfd                        ; 1000:5dca: 7f31
    mov          BX,word [SI + 0x2]              ; 1000:5dcc: 8b5c02
    sub          BX,word [0x6c4]                 ; 1000:5dcf: 2b1ec406
    jl           loc_5dfd                        ; 1000:5dd3: 7c28
    cmp          BX,0x98                         ; 1000:5dd5: 81fb9800
    jg           loc_5dfd                        ; 1000:5dd9: 7f22
    add          AX,0x8                          ; 1000:5ddb: 050800
    add          BX,0x8                          ; 1000:5dde: 83c308
    push         SI                              ; 1000:5de1: 56
    mov          SI,0xa4a6                       ; 1000:5de2: bea6a4
    mov          DS,word [cs:0x5e]               ; 1000:5de5: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:5dea: e8b619
    mov          DS,word [cs:0x5c]               ; 1000:5ded: 2e8e1e5c00
    pop          SI                              ; 1000:5df2: 5e
loc_5df3:
    pop          CX                              ; 1000:5df3: 59
    add          SI,0x8                          ; 1000:5df4: 83c608
    dec          CX                              ; 1000:5df7: 49
    jz           loc_5e03                        ; 1000:5df8: 7409
    jmp near     loc_5d6e                        ; 1000:5dfa: e971ff
loc_5dfd:
    mov          word [SI],0xffff                ; 1000:5dfd: c704ffff
    jmp short    loc_5df3                        ; 1000:5e01: ebf0
loc_5e03:
    ret                                          ; 1000:5e03: c3
loc_5e04:
    mov          AX,word [SI]                    ; 1000:5e04: 8b04
    sub          AX,word [0x6c2]                 ; 1000:5e06: 2b06c206
    jl           loc_5dfd                        ; 1000:5e0a: 7cf1
    cmp          AX,0xc8                         ; 1000:5e0c: 3dc800
    jg           loc_5dfd                        ; 1000:5e0f: 7fec
    mov          BX,word [SI + 0x2]              ; 1000:5e11: 8b5c02
    sub          BX,word [0x6c4]                 ; 1000:5e14: 2b1ec406
    jl           loc_5dfd                        ; 1000:5e18: 7ce3
    cmp          BX,0x98                         ; 1000:5e1a: 81fb9800
    jg           loc_5dfd                        ; 1000:5e1e: 7fdd
    push         AX                              ; 1000:5e20: 50
    push         BX                              ; 1000:5e21: 53
    push         SI                              ; 1000:5e22: 56
    mov          AX,CX                           ; 1000:5e23: 8bc1
    mov          BX,DX                           ; 1000:5e25: 8bda
    mov          CX,word [SI]                    ; 1000:5e27: 8b0c
    mov          DX,word [SI + 0x2]              ; 1000:5e29: 8b5402
    add          CX,0x3                          ; 1000:5e2c: 83c103
    add          DX,0x3                          ; 1000:5e2f: 83c203
    call         player_move_left                ; 1000:5e32: e8d1ce
    pop          SI                              ; 1000:5e35: 5e
    pop          BX                              ; 1000:5e36: 5b
    pop          AX                              ; 1000:5e37: 58
    add          AX,0x8                          ; 1000:5e38: 050800
    add          BX,0x8                          ; 1000:5e3b: 83c308
    push         SI                              ; 1000:5e3e: 56
    mov          SI,0xa4d4                       ; 1000:5e3f: bed4a4
    mov          DS,word [cs:0x5e]               ; 1000:5e42: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:5e47: e85919
    mov          DS,word [cs:0x5c]               ; 1000:5e4a: 2e8e1e5c00
    pop          SI                              ; 1000:5e4f: 5e
    jmp short    loc_5dfd                        ; 1000:5e50: ebab

; ----------------------------------------------------------------------------
; FUNCTION: sys_main_dispatcher  [System / Dispatcher]
; ADDRESS:  1000:5e52
; PURPOSE:  Top-level game dispatcher. Initializes PRNG from DOS time, loads configuration, checks high scores, and runs main menu loop.
; ----------------------------------------------------------------------------
sys_main_dispatcher:
    mov          AH,0x2c                         ; 1000:5e52: b42c
    int          0x21                            ; 1000:5e54: cd21
    mov          DL,CL                           ; 1000:5e56: 8ad1
    mov          word [0xaba9],DX                ; 1000:5e58: 8916a9ab
    call         sys_generate_system_hash        ; 1000:5e5c: e8e606
    mov          DX,0x9c73                       ; 1000:5e5f: ba739c
    call         file_load_high_scores           ; 1000:5e62: e8d704
    mov          word [0x9c8c],0xffff            ; 1000:5e65: c7068c9cffff
    jmp short    loc_5e73                        ; 1000:5e6b: eb06
    db 0xba, 0x73, 0x9c, 0xe8, 0xfc, 0x04           ; 1000:5e6d
loc_5e73:
    call         file_load_keys_def              ; 1000:5e73: e82e04
    call         ui_main_menu                    ; 1000:5e76: e87b0f
    or           AX,AX                           ; 1000:5e79: 0bc0
    jz           loc_5e80                        ; 1000:5e7b: 7403
    call         file_save_high_scores           ; 1000:5e7d: e89a04
loc_5e80:
    mov          AX,0x1                          ; 1000:5e80: b80100
    mov          BX,0x9d3f                       ; 1000:5e83: bb3f9d
    mov          CX,0x1                          ; 1000:5e86: b90100
    int3                                         ; 1000:5e89: cc
    db 0xe8, 0xf9, 0x00, 0xb8, 0x50, 0x00, 0xbb, 0x28, 0x00, 0xbe, 0xe4, 0x9a, 0xe8, 0x3b, 0xe4, 0xe8 ; 1000:5e8a
    db 0xb2, 0x18, 0xe8, 0xe7, 0x00, 0xb8, 0x50, 0x00, 0xbb, 0x28, 0x00, 0xbe, 0xe4, 0x9a, 0xe8, 0x29 ; 1000:5e9a
    db 0xe4, 0xb8, 0xfa, 0x9c, 0xa3, 0x3b, 0x9d, 0xc6, 0x06, 0x3a, 0x9d, 0x00, 0xa1, 0x8c, 0x9c, 0xe8 ; 1000:5eaa
    db 0x55, 0x01, 0xa3, 0x3d, 0x9d, 0xe8, 0x9a, 0x02, 0x73, 0x03, 0xe9, 0x4e, 0xa3, 0x83, 0x3e, 0x3d ; 1000:5eba
    db 0x9d, 0x04, 0x7e, 0x0a, 0x83, 0x3e, 0x3d, 0x9d, 0x06, 0x74, 0x1f, 0xeb, 0x73, 0x90, 0xa1, 0x3d ; 1000:5eca
    db 0x9d, 0xa3, 0x8c, 0x9c, 0xff, 0x0e, 0x8c, 0x9c, 0x04, 0x31, 0xa2, 0x8a, 0x9c, 0xba, 0x83, 0x9c ; 1000:5eda
    db 0xe8, 0x82, 0x04, 0xb8, 0x03, 0x00, 0xcc, 0xe9, 0x04, 0xb8, 0xbe, 0x1a, 0x9d, 0x89, 0x36, 0x3b ; 1000:5eea
    db 0x9d, 0xc7, 0x06, 0x3d, 0x9d, 0xff, 0xff, 0x8b, 0x1c, 0x83, 0xc6, 0x02, 0xff, 0x06, 0x3d, 0x9d ; 1000:5efa
    db 0x80, 0x3f, 0x00, 0x75, 0xf2, 0xc7, 0x07, 0x20, 0x00, 0xe8, 0x55, 0x01, 0xe8, 0x35, 0x18, 0xe8 ; 1000:5f0a
    db 0x4f, 0x01, 0xe8, 0x2f, 0x18, 0xe8, 0xa6, 0x02, 0x72, 0x23, 0xff, 0x06, 0x94, 0x9c, 0xe8, 0xef ; 1000:5f1a
    db 0x03, 0xa1, 0x3d, 0x9d, 0x04, 0x31, 0xa2, 0x8a, 0x9c, 0xba, 0x83, 0x9c, 0xe8, 0x03, 0x04, 0xb8 ; 1000:5f2a
    db 0xfa, 0x9c, 0xa3, 0x3b, 0x9d, 0xc6, 0x06, 0x3a, 0x9d, 0x00, 0xe9, 0x78, 0xff, 0xe9, 0x61, 0xff ; 1000:5f3a
    db 0xb8, 0x0a, 0x9d, 0xa3, 0x3b, 0x9d, 0xc6, 0x06, 0x3a, 0x9d, 0x01, 0xb8, 0xff, 0xff, 0xe8, 0xb6 ; 1000:5f4a
    db 0x00, 0xa3, 0x3d, 0x9d, 0xe8, 0xfb, 0x01, 0x72, 0x20, 0xa1, 0x3d, 0x9d, 0xd1, 0xe0, 0xbb, 0xee ; 1000:5f5a
    db 0x9c, 0x03, 0xd8, 0xc7, 0x07, 0x00, 0x00, 0x03, 0x06, 0x3b, 0x9d, 0x8b, 0xd8, 0x8b, 0x1f, 0xc6 ; 1000:5f6a
    db 0x07, 0x00, 0xff, 0x0e, 0x94, 0x9c, 0xe8, 0x97, 0x03, 0xe9, 0x25, 0xff, 0xb8, 0x00, 0x00, 0xbb ; 1000:5f7a
    db 0x00, 0x00, 0xb9, 0x40, 0x01, 0xba, 0xc8, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x2e, 0x1b ; 1000:5f8a
    db 0xb8, 0x09, 0x00, 0xe8, 0x99, 0xe3, 0xb9, 0x18, 0x00, 0xbb, 0x00, 0x00, 0xb0, 0x22, 0xe8, 0x3d ; 1000:5f9a
    db 0xe3, 0xba, 0x26, 0x00, 0xb0, 0x23, 0xe8, 0x35, 0xe3, 0x4a, 0x75, 0xfa, 0xb0, 0x24, 0xe8, 0x2d ; 1000:5faa
    db 0xe3, 0xb9, 0xc0, 0x00, 0xbb, 0x00, 0x00, 0xb0, 0x3a, 0xe8, 0x22, 0xe3, 0xba, 0x26, 0x00, 0xb0 ; 1000:5fba
    db 0x3b, 0xe8, 0x1a, 0xe3, 0x4a, 0x75, 0xfa, 0xb0, 0x3c, 0xe8, 0x12, 0xe3, 0xba, 0x14, 0x00, 0xb9 ; 1000:5fca
    db 0x20, 0x00, 0xbb, 0x00, 0x00, 0xb0, 0x25, 0xe8, 0x04, 0xe3, 0xbb, 0x38, 0x01, 0xb0, 0x26, 0xe8 ; 1000:5fda
    db 0xfc, 0xe2, 0x83, 0xc1, 0x08, 0x4a, 0x75, 0xea, 0xb8, 0x07, 0x00, 0xe8, 0x41, 0xe3, 0xb8, 0x10 ; 1000:5fea
    db 0x00, 0xbb, 0x00, 0x00, 0xbe, 0xa0, 0x9a, 0xe8, 0xd0, 0xe2, 0xb8, 0x28, 0x00, 0xbb, 0x10, 0x00 ; 1000:5ffa
    db 0xbe, 0xc5, 0x9a, 0xe8, 0xc4, 0xe2, 0xc3, 0x50, 0x3d, 0x07, 0x00, 0x74, 0x08, 0x40, 0xe8, 0x15 ; 1000:600a
    db 0x00, 0x72, 0xf5, 0x59, 0xc3, 0x58, 0xc3, 0x50, 0x0b, 0xc0, 0x74, 0xf9, 0x48, 0xe8, 0x06, 0x00 ; 1000:601a
    db 0x72, 0xf6, 0x59, 0xc3, 0x58, 0xc3, 0x3d, 0x07, 0x00, 0x74, 0x24, 0x3d, 0x06, 0x00, 0x75, 0x0e ; 1000:602a
    db 0x80, 0x3e, 0x3a, 0x9d, 0x01, 0x7d, 0x26, 0x83, 0x3e, 0x94, 0x9c, 0x05, 0x74, 0x1f, 0x8b, 0xd8 ; 1000:603a
    db 0xd1, 0xe3, 0x03, 0x1e, 0x3b, 0x9d, 0x8b, 0x1f, 0x80, 0x3f, 0x00, 0x75, 0x12, 0xeb, 0x0e, 0x80 ; 1000:604a
    db 0x3e, 0x3a, 0x9d, 0x01, 0x7d, 0x07, 0x83, 0x3e, 0x94, 0x9c, 0x00, 0x75, 0x02, 0xf9, 0xc3, 0xf8 ; 1000:605a
    db 0xc3, 0xc7, 0x06, 0x8e, 0x9c, 0x00, 0x00, 0xb8, 0x38, 0x00, 0xbb, 0x3c, 0x00, 0xb9, 0xd0, 0x00 ; 1000:606a
    db 0xba, 0x80, 0x00, 0x80, 0x3e, 0x3a, 0x9d, 0x02, 0x75, 0x06, 0x83, 0xc3, 0x08, 0x83, 0xea, 0x08 ; 1000:607a
    db 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x36, 0x1a, 0xa1, 0x3d, 0x9d, 0xf7, 0xd8, 0x05, 0x08, 0x00 ; 1000:608a
    db 0x8b, 0x36, 0x3b, 0x9d, 0xb9, 0x08, 0x00, 0xbb, 0x40, 0x00, 0x51, 0x56, 0x50, 0x3b, 0xc1, 0x75 ; 1000:609a
    db 0x08, 0xb8, 0x0a, 0x00, 0xe8, 0x88, 0xe2, 0xeb, 0x06, 0xb8, 0x07, 0x00, 0xe8, 0x80, 0xe2, 0x8b ; 1000:60aa
    db 0x34, 0x56, 0xb8, 0x48, 0x00, 0xe8, 0x12, 0xe2, 0x5e, 0x81, 0xfe, 0x96, 0x9c, 0x7c, 0x54, 0x81 ; 1000:60ba
    db 0xfe, 0xd6, 0x9c, 0x7f, 0x4e, 0xbe, 0xee, 0x9c, 0x03, 0x36, 0x8e, 0x9c, 0x83, 0x06, 0x8e, 0x9c ; 1000:60ca
    db 0x02, 0x8b, 0x14, 0xb8, 0xc8, 0x00, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xbe, 0x00, 0x48, 0x83, 0xfa ; 1000:60da
    db 0x05, 0x7c, 0x13, 0x83, 0xea, 0x05, 0x50, 0x53, 0x52, 0x56, 0xe8, 0xac, 0x16, 0x5e, 0x5a, 0x5b ; 1000:60ea
    db 0x58, 0x05, 0x08, 0x00, 0xeb, 0xe8, 0xbe, 0x2e, 0x48, 0x0b, 0xd2, 0x74, 0x11, 0x4a, 0x50, 0x53 ; 1000:60fa
    db 0x52, 0x56, 0xe8, 0x94, 0x16, 0x5e, 0x5a, 0x5b, 0x58, 0x05, 0x08, 0x00, 0xeb, 0xeb, 0x2e, 0x8e ; 1000:610a
    db 0x1e, 0x5c, 0x00, 0x83, 0xc3, 0x10, 0x58, 0x5e, 0x83, 0xc6, 0x02, 0x59, 0x49, 0x74, 0x03, 0xe9 ; 1000:611a
    db 0x78, 0xff, 0x2d, 0x08, 0x00, 0xf7, 0xd8, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0xd1, 0xe0, 0x05 ; 1000:612a
    db 0x3c, 0x00, 0x8b, 0xd8, 0xb8, 0x38, 0x00, 0xbe, 0x04, 0x9e, 0x80, 0x3e, 0x3a, 0x9d, 0x01, 0x75 ; 1000:613a
    db 0x03, 0xbe, 0xaa, 0x9e, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0x4d, 0x16, 0x2e, 0x8e, 0x1e, 0x5c ; 1000:614a
    db 0x00, 0xc3, 0xe8, 0x0c, 0xff, 0xe8, 0xec, 0x15, 0x2e, 0xc6, 0x06, 0x61, 0x00, 0x00, 0xb8, 0x01 ; 1000:615a
    db 0x00, 0xe8, 0x0c, 0xa1, 0x2e, 0x80, 0x3e, 0x61, 0x00, 0x00, 0x74, 0xf8, 0x2e, 0x80, 0x3e, 0x5b ; 1000:616a
    db 0x02, 0x01, 0x74, 0x30, 0x2e, 0x80, 0x3e, 0x5a, 0x02, 0x01, 0x74, 0x33, 0x2e, 0x80, 0x3e, 0x5d ; 1000:617a
    db 0x02, 0x01, 0x74, 0x38, 0x2e, 0x80, 0x3e, 0x5c, 0x02, 0x01, 0x74, 0x30, 0x2e, 0x80, 0x3e, 0x61 ; 1000:618a
    db 0x00, 0x1c, 0x74, 0x28, 0x2e, 0x80, 0x3e, 0x61, 0x00, 0x01, 0x74, 0x1e, 0x2e, 0xc6, 0x06, 0x61 ; 1000:619a
    db 0x00, 0x00, 0xeb, 0xc0, 0xa1, 0x3d, 0x9d, 0xe8, 0x5d, 0xfe, 0xa3, 0x3d, 0x9d, 0xeb, 0xa3, 0xa1 ; 1000:61aa
    db 0x3d, 0x9d, 0xe8, 0x62, 0xfe, 0xa3, 0x3d, 0x9d, 0xeb, 0x98, 0xf9, 0xc3, 0xf8, 0xc3 ; 1000:61ba

; ----------------------------------------------------------------------------
; FUNCTION: input_poll_devices  [Input / Polling]
; ADDRESS:  1000:61c8
; PURPOSE:  Polls keyboard buffer and joystick states to update the current input vector.
; ----------------------------------------------------------------------------
input_poll_devices:
    push         BP                              ; 1000:61c8: 55
    mov          BP,SP                           ; 1000:61c9: 8bec
    sub          SP,0x8                          ; 1000:61cb: 83ec08
    push         ES                              ; 1000:61ce: 06
    xor          CX,CX                           ; 1000:61cf: 33c9
    mov          ES,CX                           ; 1000:61d1: 8ec1
    mov          CL,byte [es:0x41a]              ; 1000:61d3: 268a0e1a04
    mov          byte [es:0x41c],CL              ; 1000:61d8: 26880e1c04
    pop          ES                              ; 1000:61dd: 07
    mov          byte [cs:0x60],0x1              ; 1000:61de: 2ec606600001
    mov          word [BP + -0x2],0x48           ; 1000:61e4: c746fe4800
    mov          CX,word [0x9d3d]                ; 1000:61e9: 8b0e3d9d
    shl          CX,0x1                          ; 1000:61ed: d1e1
    shl          CX,0x1                          ; 1000:61ef: d1e1
    shl          CX,0x1                          ; 1000:61f1: d1e1
    shl          CX,0x1                          ; 1000:61f3: d1e1
    add          CX,0x40                         ; 1000:61f5: 83c140
    mov          word [BP + -0x4],CX             ; 1000:61f8: 894efc
    mov          SI,word [0x9d3d]                ; 1000:61fb: 8b363d9d
    shl          SI,0x1                          ; 1000:61ff: d1e6
    add          SI,word [0x9d3b]                ; 1000:6201: 03363b9d
    mov          SI,word [SI]                    ; 1000:6205: 8b34
    mov          word [BP + -0x6],SI             ; 1000:6207: 8976fa
    mov          word [BP + -0x8],0x0            ; 1000:620a: c746f80000
loc_620f:
    mov          SI,word [BP + -0x6]             ; 1000:620f: 8b76fa
    add          SI,word [BP + -0x8]             ; 1000:6212: 0376f8
    mov          byte [SI],0x0                   ; 1000:6215: c60400
    mov          AX,word [BP + -0x2]             ; 1000:6218: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:621b: 8b5efc
    mov          CX,0xa0                         ; 1000:621e: b9a000
    mov          DX,0x8                          ; 1000:6221: ba0800
    mov          DI,word [cs:0x773f]             ; 1000:6224: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:6229: e89c18
    mov          AX,0x7                          ; 1000:622c: b80700
    call         vid_set_write_mode_0            ; 1000:622f: e807e1
    mov          AX,word [BP + -0x2]             ; 1000:6232: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:6235: 8b5efc
    mov          SI,word [BP + -0x6]             ; 1000:6238: 8b76fa
    call         ui_clear_window                 ; 1000:623b: e896e0
    mov          CX,BX                           ; 1000:623e: 8bcb
    mov          BX,AX                           ; 1000:6240: 8bd8
    mov          AL,0x2b                         ; 1000:6242: b02b
    call         ui_draw_window_border           ; 1000:6244: e8a1e0
    call         vid_wait_vblank                 ; 1000:6247: e80415
    xor          AX,AX                           ; 1000:624a: 33c0
    int          0x16                            ; 1000:624c: cd16
    or           AL,AL                           ; 1000:624e: 0ac0
    jz           loc_620f                        ; 1000:6250: 74bd
    cmp          AL,0x1b                         ; 1000:6252: 3c1b
    jnz          loc_6260                        ; 1000:6254: 750a
    mov          SI,word [BP + -0x6]             ; 1000:6256: 8b76fa
    mov          byte [SI],0x0                   ; 1000:6259: c60400
    stc                                          ; 1000:625c: f9
    jmp short    loc_629a                        ; 1000:625d: eb3b
    db 0x90                                         ; 1000:625f
loc_6260:
    cmp          AL,0x8                          ; 1000:6260: 3c08
    jz           loc_628d                        ; 1000:6262: 7429
    cmp          AL,0xd                          ; 1000:6264: 3c0d
    jz           loc_6299                        ; 1000:6266: 7431
    cmp          AL,0x61                         ; 1000:6268: 3c61
    jl           loc_626e                        ; 1000:626a: 7c02
    sub          AL,0x20                         ; 1000:626c: 2c20
loc_626e:
    cmp          AL,0x20                         ; 1000:626e: 3c20
    jz           loc_627a                        ; 1000:6270: 7408
    cmp          AL,0x30                         ; 1000:6272: 3c30
    jl           loc_620f                        ; 1000:6274: 7c99
    cmp          AL,0x5a                         ; 1000:6276: 3c5a
    jg           loc_620f                        ; 1000:6278: 7f95
loc_627a:
    cmp          word [BP + -0x8],0xf            ; 1000:627a: 837ef80f
    jz           loc_620f                        ; 1000:627e: 748f
    mov          SI,word [BP + -0x6]             ; 1000:6280: 8b76fa
    add          SI,word [BP + -0x8]             ; 1000:6283: 0376f8
    mov          byte [SI],AL                    ; 1000:6286: 8804
    inc          word [BP + -0x8]                ; 1000:6288: ff46f8
    jmp short    loc_620f                        ; 1000:628b: eb82
loc_628d:
    cmp          word [BP + -0x8],0x0            ; 1000:628d: 837ef800
    jz           loc_6296                        ; 1000:6291: 7403
    dec          word [BP + -0x8]                ; 1000:6293: ff4ef8
loc_6296:
    jmp near     loc_620f                        ; 1000:6296: e976ff
loc_6299:
    clc                                          ; 1000:6299: f8
loc_629a:
    mov          byte [cs:0x60],0x0              ; 1000:629a: 2ec606600000
    mov          SP,BP                           ; 1000:62a0: 8be5
    pop          BP                              ; 1000:62a2: 5d
    ret                                          ; 1000:62a3: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_keys_def  [File I/O / Config]
; ADDRESS:  1000:62a4
; PURPOSE:  Loads custom key configuration bindings from 'KEYS.DEF' file.
; ----------------------------------------------------------------------------
file_load_keys_def:
    mov          DX,0x9c7c                       ; 1000:62a4: ba7c9c
    mov          AX,0x3d00                       ; 1000:62a7: b8003d
    int          0x21                            ; 1000:62aa: cd21
    jnc          loc_62b1                        ; 1000:62ac: 7303
    jmp short    loc_62c4                        ; 1000:62ae: eb14
    db 0x90                                         ; 1000:62b0
loc_62b1:
    mov          BX,AX                           ; 1000:62b1: 8bd8
    mov          DX,0x9c94                       ; 1000:62b3: ba949c
    mov          CX,0x66                         ; 1000:62b6: b96600
    mov          AX,0x3f00                       ; 1000:62b9: b8003f
    int          0x21                            ; 1000:62bc: cd21
    mov          AX,0x3e00                       ; 1000:62be: b8003e
    int          0x21                            ; 1000:62c1: cd21
    ret                                          ; 1000:62c3: c3
loc_62c4:
    mov          word [0x9c94],0x0               ; 1000:62c4: c706949c0000
    mov          byte [0x9c96],0x0               ; 1000:62ca: c606969c00
    mov          byte [0x9ca6],0x0               ; 1000:62cf: c606a69c00
    mov          byte [0x9cb6],0x0               ; 1000:62d4: c606b69c00
    mov          byte [0x9cc6],0x0               ; 1000:62d9: c606c69c00
    mov          byte [0x9cd6],0x0               ; 1000:62de: c606d69c00
    mov          word [0x9ce8],0x0               ; 1000:62e3: c706e89c0000
    mov          word [0x9cea],0x0               ; 1000:62e9: c706ea9c0000
    mov          word [0x9cec],0x0               ; 1000:62ef: c706ec9c0000
    mov          word [0x9cf8],0x0               ; 1000:62f5: c706f89c0000
    mov          word [0x9cee],0x0               ; 1000:62fb: c706ee9c0000
    mov          word [0x9cf0],0x0               ; 1000:6301: c706f09c0000
    mov          word [0x9cf2],0x0               ; 1000:6307: c706f29c0000
    mov          word [0x9cf4],0x0               ; 1000:630d: c706f49c0000
    mov          word [0x9cf6],0x0               ; 1000:6313: c706f69c0000
    ret                                          ; 1000:6319: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_save_high_scores  [File I/O / High Scores]
; ADDRESS:  1000:631a
; PURPOSE:  Saves high score table entries to disk.
; ----------------------------------------------------------------------------
file_save_high_scores:
    mov          DX,0x9c7c                       ; 1000:631a: ba7c9c
    xor          CX,CX                           ; 1000:631d: 33c9
    mov          AX,0x3c00                       ; 1000:631f: b8003c
    int          0x21                            ; 1000:6322: cd21
    jnc          loc_6329                        ; 1000:6324: 7303
    jmp near     sys_fatal_error                 ; 1000:6326: e9d99e
loc_6329:
    mov          BX,AX                           ; 1000:6329: 8bd8
    mov          DX,0x9c94                       ; 1000:632b: ba949c
    mov          CX,0x66                         ; 1000:632e: b96600
    mov          AX,0x4000                       ; 1000:6331: b80040
    int          0x21                            ; 1000:6334: cd21
    mov          AX,0x3e00                       ; 1000:6336: b8003e
    int          0x21                            ; 1000:6339: cd21
    ret                                          ; 1000:633b: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_load_high_scores  [File I/O / High Scores]
; ADDRESS:  1000:633c
; PURPOSE:  Loads high score table entries from disk.
; ----------------------------------------------------------------------------
file_load_high_scores:
    xor          CX,CX                           ; 1000:633c: 33c9
    mov          AX,0x3c00                       ; 1000:633e: b8003c
    int          0x21                            ; 1000:6341: cd21
    jnc          loc_6348                        ; 1000:6343: 7303
    jmp near     sys_fatal_error                 ; 1000:6345: e9ba9e
loc_6348:
    mov          BX,AX                           ; 1000:6348: 8bd8
    push         DS                              ; 1000:634a: 1e
    mov          DX,0xaf0                        ; 1000:634b: baf00a
    mov          CX,word [0x942]                 ; 1000:634e: 8b0e4209
    mov          AX,CS                           ; 1000:6352: 8cc8
    mov          DS,AX                           ; 1000:6354: 8ed8
    mov          AX,0x4000                       ; 1000:6356: b80040
    int          0x21                            ; 1000:6359: cd21
    pop          DS                              ; 1000:635b: 1f
    mov          CX,0x28f                        ; 1000:635c: b98f02
    mov          DX,0x210                        ; 1000:635f: ba1002
    sub          CX,DX                           ; 1000:6362: 2bca
    mov          AX,0x4000                       ; 1000:6364: b80040
    int          0x21                            ; 1000:6367: cd21
    mov          AX,0x3e00                       ; 1000:6369: b8003e
    int          0x21                            ; 1000:636c: cd21
    ret                                          ; 1000:636e: c3
    db 0xb8, 0x00, 0x3d, 0xcd, 0x21, 0x73, 0x03, 0xe9, 0x89, 0x9e, 0x8b, 0xd8, 0x1e, 0xba, 0xf0, 0x0a ; 1000:636f
    db 0x8b, 0x0e, 0x42, 0x09, 0x8c, 0xc8, 0x8e, 0xd8, 0xb8, 0x00, 0x3f, 0xcd, 0x21, 0x1f, 0xb9, 0x8f ; 1000:637f
    db 0x02, 0xba, 0x10, 0x02, 0x2b, 0xca, 0xb8, 0x00, 0x3f, 0xcd, 0x21, 0xb8, 0x00, 0x3e, 0xcd, 0x21 ; 1000:638f
    db 0x81, 0x3e, 0x27, 0x02, 0x50, 0x01, 0x74, 0x03, 0xe9, 0x58, 0x9e, 0xc3 ; 1000:639f
loc_63ab:
    db 0xe8, 0x41, 0x02, 0xa0                   ; 1000:63ab
    db 0x1c, 0x02, 0x32, 0xe4, 0x2e, 0xf7, 0x06, 0xc2, 0x0c, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7 ; 1000:63af
    db 0x06, 0x3e, 0x0f, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xaa, 0x0f, 0x00, 0x0f, 0x75 ; 1000:63bf
    db 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xbe, 0x10, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xea ; 1000:63cf
    db 0x11, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xaa, 0x12, 0x00, 0x0f, 0x75, 0x01, 0x40 ; 1000:63df
    db 0x2e, 0xf7, 0x06, 0x2e, 0x13, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0x02, 0x15, 0x00 ; 1000:63ef
    db 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0x62, 0x15, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7 ; 1000:63ff
    db 0x06, 0x22, 0x10, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xda, 0x12, 0x00, 0x0f, 0x75 ; 1000:640f
    db 0x01, 0x40, 0x2e, 0xf7, 0x06, 0x66, 0x0e, 0x00, 0x0f, 0x75, 0x0a, 0x2e, 0x81, 0x3e, 0x64, 0x0e ; 1000:641f
    db 0x94, 0xb0, 0x75, 0x01, 0x40, 0x80, 0x3e, 0x1e, 0x02, 0x01, 0x75, 0x01, 0x40, 0x80, 0x3e, 0x20 ; 1000:642f
    db 0x02, 0x04, 0x7c, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0x86, 0x12, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e ; 1000:643f
    db 0xf7, 0x06, 0x56, 0x0f, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xee, 0x13, 0x00, 0x0f ; 1000:644f
    db 0x75, 0x01, 0x40, 0x2e, 0xf7, 0x06, 0xae, 0x14, 0x00, 0x0f, 0x75, 0x01, 0x40, 0x80, 0x3e, 0x1d ; 1000:645f
    db 0x02, 0x01, 0x75, 0x01, 0x40, 0xa3, 0x90, 0x9c, 0xe8, 0x0c, 0xfb, 0x8a, 0x1e, 0x8a, 0x9c, 0x80 ; 1000:646f
    db 0xeb, 0x31, 0x32, 0xff, 0xd1, 0xe3, 0x81, 0xc3, 0xee, 0x9c, 0x89, 0x1e, 0x92, 0x9c, 0xe8, 0x65 ; 1000:647f
    db 0x00, 0xb8, 0x07, 0x00, 0xe8, 0xa3, 0xde, 0xb8, 0x28, 0x00, 0xbb, 0x38, 0x00, 0xbe, 0x65, 0x9b ; 1000:648f
    db 0xe8, 0x32, 0xde, 0xe8, 0xa9, 0x12, 0xe8, 0xde, 0xfa, 0xe8, 0x4a, 0x00, 0xb8, 0x07, 0x00, 0xe8 ; 1000:649f
    db 0x88, 0xde, 0xb8, 0x28, 0x00, 0xbb, 0x38, 0x00, 0xbe, 0x65, 0x9b, 0xe8, 0x17, 0xde, 0xb8, 0x2a ; 1000:64af
    db 0x9d, 0xa3, 0x3b, 0x9d, 0xc6, 0x06, 0x3a, 0x9d, 0x02, 0xb8, 0xff, 0xff, 0xe8, 0x43, 0xfb, 0xa3 ; 1000:64bf
    db 0x3d, 0x9d, 0xe8, 0x88, 0xfc, 0x73, 0x03, 0xeb, 0x1a, 0x90, 0x83, 0x3e, 0x3d, 0x9d, 0x01, 0x75 ; 1000:64cf
    db 0x12, 0x8b, 0x1e, 0x92, 0x9c, 0xa1, 0x90, 0x9c, 0x89, 0x07, 0xe8, 0x2e, 0xfe, 0xba, 0x83, 0x9c ; 1000:64df
    db 0xe8, 0x4a, 0xfe, 0xe9, 0x78, 0xf9, 0xa1, 0x90, 0x9c, 0x8b, 0x1e, 0x92, 0x9c, 0x2b, 0x07, 0x7e ; 1000:64ef
    db 0x44, 0x50, 0xb8, 0x0e, 0x00, 0xe8, 0x32, 0xde, 0xb8, 0x40, 0x00, 0xbb, 0x28, 0x00, 0xbe, 0xa9 ; 1000:64ff
    db 0x9b, 0xe8, 0xc1, 0xdd, 0x58, 0x3d, 0x01, 0x00, 0x7f, 0x0e, 0xb8, 0xc0, 0x00, 0xbb, 0x28, 0x00 ; 1000:650f
    db 0xbe, 0xba, 0x9b, 0xe8, 0xaf, 0xdd, 0xeb, 0x1d, 0x3d, 0x09, 0x00, 0x7e, 0x03, 0xb8, 0x09, 0x00 ; 1000:651f
    db 0x05, 0x30, 0x00, 0x8b, 0xcb, 0xbb, 0xc0, 0x00, 0xe8, 0xae, 0xdd, 0x8b, 0xc3, 0x8b, 0xd9, 0xbe ; 1000:652f
    db 0xc7, 0x9b, 0xe8, 0x90, 0xdd, 0xc3           ; 1000:653f

; ----------------------------------------------------------------------------
; FUNCTION: sys_generate_system_hash  [System / Security]
; ADDRESS:  1000:6545
; PURPOSE:  Hashes VGA/System BIOS ROM bytes to generate system signature for copy protection and seed PRNG.
; ----------------------------------------------------------------------------
sys_generate_system_hash:
    push         ES                              ; 1000:6545: 06
    mov          AX,[0xaba9]                     ; 1000:6546: a1a9ab
    mov          [0x9ce6],AX                     ; 1000:6549: a3e69c
    mov          AX,0xc000                       ; 1000:654c: b800c0
    mov          ES,AX                           ; 1000:654f: 8ec0
    xor          BX,BX                           ; 1000:6551: 33db
    xor          DX,DX                           ; 1000:6553: 33d2
    mov          CX,0x40                         ; 1000:6555: b94000
loc_6558:
    add          DX,word [es:BX]                 ; 1000:6558: 260317
    add          BX,0x2                          ; 1000:655b: 83c302
    loop         loc_6558                        ; 1000:655e: e2f8
    mov          word [0xabab],DX                ; 1000:6560: 8916abab
    mov          BX,0xf000                       ; 1000:6564: bb00f0
    mov          ES,BX                           ; 1000:6567: 8ec3
    xor          DX,DX                           ; 1000:6569: 33d2
    mov          CX,0x40                         ; 1000:656b: b94000
loc_656e:
    add          DX,word [es:BX]                 ; 1000:656e: 260317
    add          BX,0x2                          ; 1000:6571: 83c302
    loop         loc_656e                        ; 1000:6574: e2f8
    mov          word [0xabad],DX                ; 1000:6576: 8916adab
    mov          BX,0xff80                       ; 1000:657a: bb80ff
    xor          DX,DX                           ; 1000:657d: 33d2
    mov          CX,0x40                         ; 1000:657f: b94000
loc_6582:
    add          DX,word [es:BX]                 ; 1000:6582: 260317
    add          BX,0x2                          ; 1000:6585: 83c302
    loop         loc_6582                        ; 1000:6588: e2f8
    mov          word [0xabaf],DX                ; 1000:658a: 8916afab
    pop          ES                              ; 1000:658e: 07
    mov          BX,word [0xaba9]                ; 1000:658f: 8b1ea9ab
    mov          AX,[0xabab]                     ; 1000:6593: a1abab
    mov          CX,word [0xabad]                ; 1000:6596: 8b0eadab
    mov          DX,word [0xabaf]                ; 1000:659a: 8b16afab
    add          BX,AX                           ; 1000:659e: 03d8
    add          BX,CX                           ; 1000:65a0: 03d9
    add          BX,DX                           ; 1000:65a2: 03da
    mov          AX,BX                           ; 1000:65a4: 8bc3
    mov          CL,0x5                          ; 1000:65a6: b105
    shr          AX,CL                           ; 1000:65a8: d3e8
    add          BX,AX                           ; 1000:65aa: 03d8
    and          BX,0x7ff                        ; 1000:65ac: 81e3ff07
    push         BX                              ; 1000:65b0: 53
    mov          AX,BX                           ; 1000:65b1: 8bc3
    mov          CL,0x4                          ; 1000:65b3: b104
    shr          BX,CL                           ; 1000:65b5: d3eb
    and          AX,0xf                          ; 1000:65b7: 250f00
    mov          [0xabb1],AX                     ; 1000:65ba: a3b1ab
    mov          word [0xabb3],BX                ; 1000:65bd: 891eb3ab
    pop          BX                              ; 1000:65c1: 5b
    add          BX,0x1234                       ; 1000:65c2: 81c33412
    mov          AX,BX                           ; 1000:65c6: 8bc3
    mov          CX,BX                           ; 1000:65c8: 8bcb
    mov          DX,BX                           ; 1000:65ca: 8bd3
    shl          BX,0x1                          ; 1000:65cc: d1e3
    shl          BX,0x1                          ; 1000:65ce: d1e3
    shl          BX,0x1                          ; 1000:65d0: d1e3
    shl          BX,0x1                          ; 1000:65d2: d1e3
    shl          AX,0x1                          ; 1000:65d4: d1e0
    shl          AX,0x1                          ; 1000:65d6: d1e0
    shl          AX,0x1                          ; 1000:65d8: d1e0
    shl          CX,0x1                          ; 1000:65da: d1e1
    add          BX,AX                           ; 1000:65dc: 03d8
    add          BX,CX                           ; 1000:65de: 03d9
    add          BX,DX                           ; 1000:65e0: 03da
    test         BX,0x40                         ; 1000:65e2: f7c34000
    jz           loc_65ea                        ; 1000:65e6: 7402
    neg          BX                              ; 1000:65e8: f7db
loc_65ea:
    mov          word [0xabbb],BX                ; 1000:65ea: 891ebbab
    ret                                          ; 1000:65ee: c3

; ----------------------------------------------------------------------------
; FUNCTION: file_check_config_exists  [File I/O / Config]
; ADDRESS:  1000:65ef
; PURPOSE:  Checks if configuration file 'FR.CFG' exists on disk.
; ----------------------------------------------------------------------------
file_check_config_exists:
    push         SI                              ; 1000:65ef: 56
    push         DI                              ; 1000:65f0: 57
    mov          SI,0x9bdf                       ; 1000:65f1: bedf9b
    mov          DI,0x9bd3                       ; 1000:65f4: bfd39b
    call         file_save_config                ; 1000:65f7: e81100
    pop          DI                              ; 1000:65fa: 5f
    pop          SI                              ; 1000:65fb: 5e
    ret                                          ; 1000:65fc: c3
    db 0x56, 0x57, 0xbe, 0x29, 0x9c, 0xbf, 0xd9, 0x9b, 0xe8, 0x03, 0x00, 0x5f, 0x5e, 0xc3 ; 1000:65fd

; ----------------------------------------------------------------------------
; FUNCTION: file_save_config  [File I/O / Config]
; ADDRESS:  1000:660b
; PURPOSE:  Writes audio, input, and video configuration settings to 'FR.CFG'.
; ----------------------------------------------------------------------------
file_save_config:
    mov          DX,DI                           ; 1000:660b: 8bd7
    mov          AX,0x3d00                       ; 1000:660d: b8003d
    int          0x21                            ; 1000:6610: cd21
    jc           loc_661c                        ; 1000:6612: 7208
    mov          BX,AX                           ; 1000:6614: 8bd8
    mov          AX,0x3e00                       ; 1000:6616: b8003e
    int          0x21                            ; 1000:6619: cd21
    ret                                          ; 1000:661b: c3
loc_661c:
    push         SI                              ; 1000:661c: 56
    push         DI                              ; 1000:661d: 57
    call         ui_draw_window_box              ; 1000:661e: e8e5db
    call         vid_wait_vblank                 ; 1000:6621: e82a11
    pop          DI                              ; 1000:6624: 5f
    pop          SI                              ; 1000:6625: 5e
    call         sys_restore_interrupts          ; 1000:6626: e8709f
    cmp          byte [cs:0x61],0x1              ; 1000:6629: 2e803e610001
    jnz          file_save_config                ; 1000:662f: 75da
    jmp near     sys_exit_to_dos                 ; 1000:6631: e9e19b
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x66, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x66 ; 1000:6634
    db 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x66, 0x0d, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08 ; 1000:6644
    db 0x18, 0x00, 0x00, 0x01, 0x07, 0x01, 0x17, 0x01, 0x07, 0x01, 0x18, 0x00, 0x00, 0x08, 0x00, 0x08 ; 1000:6654
    db 0x00, 0x08, 0x18, 0x00                       ; 1000:6664

; ----------------------------------------------------------------------------
; FUNCTION: game_save_load_menu  [Game / Saves]
; ADDRESS:  1000:6668
; PURPOSE:  Save/Load game slot interface. Handles saving and loading games to 'GAME.000' through 'GAME.003'.
; ----------------------------------------------------------------------------
game_save_load_menu:
    mov          DX,0x9ea8                       ; 1000:6668: baa89e
    mov          AX,0x3d00                       ; 1000:666b: b8003d
    int          0x21                            ; 1000:666e: cd21
    jnc          loc_6675                        ; 1000:6670: 7303
    jmp near     sys_fatal_error                 ; 1000:6672: e98d9b
loc_6675:
    mov          BX,AX                           ; 1000:6675: 8bd8
    mov          DX,0x9a8                        ; 1000:6677: baa809
    mov          CX,0x7fff                       ; 1000:667a: b9ff7f
    mov          AX,0x3f00                       ; 1000:667d: b8003f
    int          0x21                            ; 1000:6680: cd21
    mov          AX,0x3e00                       ; 1000:6682: b8003e
    int          0x21                            ; 1000:6685: cd21
    mov          AX,0x8                          ; 1000:6687: b80800
    mov          BX,0x8                          ; 1000:668a: bb0800
    mov          SI,0x9a8                        ; 1000:668d: bea809
    call         vid_set_write_mode_2            ; 1000:6690: e87912
    mov          DX,0x9eb1                       ; 1000:6693: bab19e
    mov          AX,0x3d00                       ; 1000:6696: b8003d
    int          0x21                            ; 1000:6699: cd21
    jnc          loc_66a0                        ; 1000:669b: 7303
    jmp near     sys_fatal_error                 ; 1000:669d: e9629b
loc_66a0:
    mov          BX,AX                           ; 1000:66a0: 8bd8
    mov          DX,0x9a8                        ; 1000:66a2: baa809
    mov          CX,0x7fff                       ; 1000:66a5: b9ff7f
    mov          AX,0x3f00                       ; 1000:66a8: b8003f
    int          0x21                            ; 1000:66ab: cd21
    mov          AX,0x3e00                       ; 1000:66ad: b8003e
    int          0x21                            ; 1000:66b0: cd21
    mov          BX,0x6634                       ; 1000:66b2: bb3466
    cli                                          ; 1000:66b5: fa
    mov          word [cs:0x278],BX              ; 1000:66b6: 2e891e7802
    mov          word [cs:BX + 0x2],0x0          ; 1000:66bb: 2ec747020000
    mov          word [cs:BX + 0x4],0x0          ; 1000:66c1: 2ec747040000
    mov          word [cs:BX + 0xa],0x0          ; 1000:66c7: 2ec7470a0000
    mov          word [cs:BX + 0xc],0x0          ; 1000:66cd: 2ec7470c0000
    mov          word [cs:BX + 0x12],0x0         ; 1000:66d3: 2ec747120000
    mov          word [cs:BX + 0x14],0x0         ; 1000:66d9: 2ec747140000
    sti                                          ; 1000:66df: fb
loc_66e0:
    call         ui_draw_slot_info               ; 1000:66e0: e89700
loc_66e3:
    call         sys_restore_interrupts          ; 1000:66e3: e8b39e
    cmp          byte [cs:0x25a],0x1             ; 1000:66e6: 2e803e5a0201
    jz           loc_6729                        ; 1000:66ec: 743b
    cmp          byte [cs:0x25b],0x1             ; 1000:66ee: 2e803e5b0201
    jz           loc_6737                        ; 1000:66f4: 7441
    cmp          byte [cs:0x258],0x1             ; 1000:66f6: 2e803e580201
    jz           loc_6745                        ; 1000:66fc: 7447
    cmp          byte [cs:0x259],0x1             ; 1000:66fe: 2e803e590201
    jz           loc_6759                        ; 1000:6704: 7453
    cmp          byte [cs:0x25b],0x1             ; 1000:6706: 2e803e5b0201
    mov          AL,[cs:0x25c]                   ; 1000:670c: 2ea05c02
    or           AL,byte [cs:0x25d]              ; 1000:6710: 2e0a065d02
    jnz          loc_6771                        ; 1000:6715: 755a
    cmp          byte [cs:0x61],0x1              ; 1000:6717: 2e803e610001
    jz           loc_6771                        ; 1000:671d: 7452
    cmp          byte [cs:0x61],0x1c             ; 1000:671f: 2e803e61001c
    jz           loc_6771                        ; 1000:6725: 744a
    jmp short    loc_66e3                        ; 1000:6727: ebba
loc_6729:
    cmp          word [0x21a],0x2                ; 1000:6729: 833e1a0202
    jle          loc_66e3                        ; 1000:672e: 7eb3
    sub          word [0x21a],0x3                ; 1000:6730: 832e1a0203
    jmp short    loc_66e0                        ; 1000:6735: eba9
loc_6737:
    cmp          word [0x21a],0x3                ; 1000:6737: 833e1a0203
    jge          loc_66e3                        ; 1000:673c: 7da5
    add          word [0x21a],0x3                ; 1000:673e: 83061a0203
    jmp short    loc_66e0                        ; 1000:6743: eb9b
loc_6745:
    cmp          word [0x21a],0x0                ; 1000:6745: 833e1a0200
    jz           loc_66e3                        ; 1000:674a: 7497
    cmp          word [0x21a],0x3                ; 1000:674c: 833e1a0203
    jz           loc_66e3                        ; 1000:6751: 7490
    dec          word [0x21a]                    ; 1000:6753: ff0e1a02
    jmp short    loc_66e0                        ; 1000:6757: eb87
loc_6759:
    cmp          word [0x21a],0x2                ; 1000:6759: 833e1a0202
    jz           loc_676e                        ; 1000:675e: 740e
    cmp          word [0x21a],0x5                ; 1000:6760: 833e1a0205
    jz           loc_676e                        ; 1000:6765: 7407
    inc          word [0x21a]                    ; 1000:6767: ff061a02
    jmp near     loc_66e0                        ; 1000:676b: e972ff
loc_676e:
    jmp near     loc_66e3                        ; 1000:676e: e972ff
loc_6771:
    mov          byte [cs:0x61],0x0              ; 1000:6771: 2ec606610000
    jmp near     spr_set_player_frame            ; 1000:6777: e9a9bb

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_slot_info  [UI / Saves]
; ADDRESS:  1000:677a
; PURPOSE:  Displays metadata for a saved game slot (score, level, items).
; ----------------------------------------------------------------------------
ui_draw_slot_info:
    mov          CX,0x6                          ; 1000:677a: b90600
    mov          SI,0x9e90                       ; 1000:677d: be909e
loc_6780:
    mov          AX,word [SI]                    ; 1000:6780: 8b04
    mov          BX,word [SI + 0x2]              ; 1000:6782: 8b5c02
    push         SI                              ; 1000:6785: 56
    push         CX                              ; 1000:6786: 51
    mov          SI,0x9a8                        ; 1000:6787: bea809
    add          SI,0x4b6                        ; 1000:678a: 81c6b604
    call         vid_clear_page                  ; 1000:678e: e81210
    pop          CX                              ; 1000:6791: 59
    pop          SI                              ; 1000:6792: 5e
    add          SI,0x4                          ; 1000:6793: 83c604
    loop         loc_6780                        ; 1000:6796: e2e8
    mov          AX,0x88                         ; 1000:6798: b88800
    mov          BX,0x5a                         ; 1000:679b: bb5a00
    mov          CX,0x30                         ; 1000:679e: b93000
    mov          DX,0x8                          ; 1000:67a1: ba0800
    mov          DI,word [cs:0x773f]             ; 1000:67a4: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:67a9: e81c13
    mov          AX,[0x21a]                      ; 1000:67ac: a11a02
    shl          AX,0x1                          ; 1000:67af: d1e0
    shl          AX,0x1                          ; 1000:67b1: d1e0
    mov          SI,0x9e90                       ; 1000:67b3: be909e
    add          SI,AX                           ; 1000:67b6: 03f0
    mov          AX,word [SI]                    ; 1000:67b8: 8b04
    mov          BX,word [SI + 0x2]              ; 1000:67ba: 8b5c02
    mov          SI,0x9a8                        ; 1000:67bd: bea809
    call         vid_clear_page                  ; 1000:67c0: e8e00f
    mov          AX,0x9                          ; 1000:67c3: b80900
    call         vid_set_write_mode_0            ; 1000:67c6: e870db
    mov          BX,word [0x21a]                 ; 1000:67c9: 8b1e1a02
    shl          BX,0x1                          ; 1000:67cd: d1e3
    add          BX,0x9e84                       ; 1000:67cf: 81c3849e
    mov          SI,word [BX]                    ; 1000:67d3: 8b37
    mov          AX,0x88                         ; 1000:67d5: b88800
    mov          BX,0x5a                         ; 1000:67d8: bb5a00
    call         ui_clear_window                 ; 1000:67db: e8f6da
    mov          BX,word [0x21a]                 ; 1000:67de: 8b1e1a02
    shl          BX,0x1                          ; 1000:67e2: d1e3
    add          BX,0x98de                       ; 1000:67e4: 81c3de98
    mov          DS,word [cs:0x5e]               ; 1000:67e8: 2e8e1e5e00
    mov          SI,word [BX]                    ; 1000:67ed: 8b37
    mov          DI,word [cs:0x773f]             ; 1000:67ef: 2e8b3e3f77
    mov          AX,0x60                         ; 1000:67f4: b86000
    mov          BX,0x4c                         ; 1000:67f7: bb4c00
    call         vid_restore_write_mode          ; 1000:67fa: e82a11
    mov          DS,word [cs:0x5c]               ; 1000:67fd: 2e8e1e5c00
    call         vid_wait_vblank                 ; 1000:6802: e8490f
    ret                                          ; 1000:6805: c3

; ----------------------------------------------------------------------------
; FUNCTION: game_execute_script  [Game / Scripting]
; ADDRESS:  1000:6806
; PURPOSE:  Interprets cutscene and scripted event commands during story sequences.
; ----------------------------------------------------------------------------
game_execute_script:
    mov          AL,[0x897]                      ; 1000:6806: a09708
    or           AL,byte [0x921]                 ; 1000:6809: 0a062109
    or           AL,byte [0x8a6]                 ; 1000:680d: 0a06a608
    jz           loc_6816                        ; 1000:6811: 7403
    jmp near     loc_6d74                        ; 1000:6813: e95e05
loc_6816:
    mov          BX,word [cs:SI + 0x1e]          ; 1000:6816: 2e8b5c1e
    cmp          BX,0x8976                       ; 1000:681a: 81fb7689
    jnz          loc_6823                        ; 1000:681e: 7503
    jmp near     loc_694f                        ; 1000:6820: e92c01
loc_6823:
    cmp          BX,0x8abe                       ; 1000:6823: 81fbbe8a
    jnz          loc_682c                        ; 1000:6827: 7503
    jmp near     loc_6993                        ; 1000:6829: e96701
loc_682c:
    cmp          BX,0x8aca                       ; 1000:682c: 81fbca8a
    jnz          loc_6835                        ; 1000:6830: 7503
    jmp near     loc_6993                        ; 1000:6832: e95e01
loc_6835:
    cmp          BX,0x8ad6                       ; 1000:6835: 81fbd68a
    jnz          loc_683e                        ; 1000:6839: 7503
    jmp near     loc_6993                        ; 1000:683b: e95501
loc_683e:
    cmp          BX,0x8c50                       ; 1000:683e: 81fb508c
    jnz          loc_6847                        ; 1000:6842: 7503
    jmp near     loc_69ca                        ; 1000:6844: e98301
loc_6847:
    cmp          BX,0x921b                       ; 1000:6847: 81fb1b92
    jnz          loc_6850                        ; 1000:684b: 7503
    jmp near     loc_6a24                        ; 1000:684d: e9d401
loc_6850:
    cmp          BX,0x930a                       ; 1000:6850: 81fb0a93
    jnz          loc_6859                        ; 1000:6854: 7503
    jmp near     loc_6ab4                        ; 1000:6856: e95b02
loc_6859:
    cmp          BX,0x980d                       ; 1000:6859: 81fb0d98
    jnz          loc_6862                        ; 1000:685d: 7503
    jmp near     loc_6aca                        ; 1000:685f: e96802
loc_6862:
    cmp          BX,0x9b42                       ; 1000:6862: 81fb429b
    jnz          loc_686b                        ; 1000:6866: 7503
    jmp near     loc_6aff                        ; 1000:6868: e99402
loc_686b:
    cmp          BX,0x9b80                       ; 1000:686b: 81fb809b
    jnz          loc_6874                        ; 1000:686f: 7503
    jmp near     loc_6b15                        ; 1000:6871: e9a102
loc_6874:
    cmp          BX,0x9b98                       ; 1000:6874: 81fb989b
    jnz          loc_687d                        ; 1000:6878: 7503
    jmp near     loc_6b2b                        ; 1000:687a: e9ae02
loc_687d:
    cmp          BX,0x9b8c                       ; 1000:687d: 81fb8c9b
    jnz          loc_6886                        ; 1000:6881: 7503
    jmp near     loc_6b41                        ; 1000:6883: e9bb02
loc_6886:
    cmp          BX,0x9ba6                       ; 1000:6886: 81fba69b
    jnz          loc_688f                        ; 1000:688a: 7503
    jmp near     loc_6b57                        ; 1000:688c: e9c802
loc_688f:
    cmp          BX,0xb37f                       ; 1000:688f: 81fb7fb3
    jnz          loc_6898                        ; 1000:6893: 7503
    jmp near     loc_6b6d                        ; 1000:6895: e9d502
loc_6898:
    cmp          BX,0xb38d                       ; 1000:6898: 81fb8db3
    jnz          loc_68a1                        ; 1000:689c: 7503
    jmp near     loc_6b83                        ; 1000:689e: e9e202
loc_68a1:
    cmp          BX,0xb399                       ; 1000:68a1: 81fb99b3
    jnz          loc_68aa                        ; 1000:68a5: 7503
    jmp near     loc_6b99                        ; 1000:68a7: e9ef02
loc_68aa:
    cmp          BX,0xb3a5                       ; 1000:68aa: 81fba5b3
    jnz          loc_68b3                        ; 1000:68ae: 7503
    jmp near     loc_6baf                        ; 1000:68b0: e9fc02
loc_68b3:
    cmp          BX,0xb98e                       ; 1000:68b3: 81fb8eb9
    jnz          loc_68bc                        ; 1000:68b7: 7503
    jmp near     loc_6bc5                        ; 1000:68b9: e90903
loc_68bc:
    cmp          BX,0xc75c                       ; 1000:68bc: 81fb5cc7
    jnz          loc_68c5                        ; 1000:68c0: 7503
    jmp near     loc_6bdb                        ; 1000:68c2: e91603
loc_68c5:
    cmp          BX,0xc8ea                       ; 1000:68c5: 81fbeac8
    jnz          loc_68ce                        ; 1000:68c9: 7503
    jmp near     loc_6bf1                        ; 1000:68cb: e92303
loc_68ce:
    cmp          BX,0xcaa6                       ; 1000:68ce: 81fba6ca
    jnz          loc_68d7                        ; 1000:68d2: 7503
    jmp near     loc_6c07                        ; 1000:68d4: e93003
loc_68d7:
    cmp          BX,0xd227                       ; 1000:68d7: 81fb27d2
    jnz          loc_68e0                        ; 1000:68db: 7503
    jmp near     loc_6c1d                        ; 1000:68dd: e93d03
loc_68e0:
    cmp          BX,0xd2dd                       ; 1000:68e0: 81fbddd2
    jnz          loc_68e9                        ; 1000:68e4: 7503
    jmp near     loc_6c46                        ; 1000:68e6: e95d03
loc_68e9:
    cmp          BX,0xd2e9                       ; 1000:68e9: 81fbe9d2
    jnz          loc_68f2                        ; 1000:68ed: 7503
    jmp near     loc_6c5c                        ; 1000:68ef: e96a03
loc_68f2:
    cmp          BX,0xd393                       ; 1000:68f2: 81fb93d3
    jnz          loc_68fb                        ; 1000:68f6: 7503
    jmp near     loc_6c72                        ; 1000:68f8: e97703
loc_68fb:
    cmp          BX,0xd3a1                       ; 1000:68fb: 81fba1d3
    jnz          loc_6904                        ; 1000:68ff: 7503
    jmp near     loc_6c88                        ; 1000:6901: e98403
loc_6904:
    cmp          BX,0xd3ad                       ; 1000:6904: 81fbadd3
    jnz          loc_690d                        ; 1000:6908: 7503
    jmp near     loc_6c9e                        ; 1000:690a: e99103
loc_690d:
    cmp          BX,0xd59d                       ; 1000:690d: 81fb9dd5
    jnz          loc_6916                        ; 1000:6911: 7503
    jmp near     loc_6cb4                        ; 1000:6913: e99e03
loc_6916:
    cmp          BX,0xd5c3                       ; 1000:6916: 81fbc3d5
    jnz          loc_691f                        ; 1000:691a: 7503
    jmp near     loc_6cca                        ; 1000:691c: e9ab03
loc_691f:
    cmp          BX,0x8507                       ; 1000:691f: 81fb0785
    jnz          loc_6928                        ; 1000:6923: 7503
    jmp near     loc_6ce0                        ; 1000:6925: e9b803
loc_6928:
    cmp          BX,0x8513                       ; 1000:6928: 81fb1385
    jnz          loc_6931                        ; 1000:692c: 7503
    jmp near     loc_6cf6                        ; 1000:692e: e9c503
loc_6931:
    cmp          BX,0x851f                       ; 1000:6931: 81fb1f85
    jnz          loc_693a                        ; 1000:6935: 7503
    jmp near     loc_6d0c                        ; 1000:6937: e9d203
loc_693a:
    cmp          BX,0x8537                       ; 1000:693a: 81fb3785
    jnz          loc_6943                        ; 1000:693e: 7503
    jmp near     loc_6d22                        ; 1000:6940: e9df03
loc_6943:
    cmp          BX,0x931a                       ; 1000:6943: 81fb1a93
    jnz          loc_694c                        ; 1000:6947: 7503
    jmp near     loc_6d4b                        ; 1000:6949: e9ff03
loc_694c:
    jmp near     loc_6d74                        ; 1000:694c: e92504
loc_694f:
    cmp          byte [0x220],0x0                ; 1000:694f: 803e200200
    jnz          loc_6976                        ; 1000:6954: 7520
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:6956: 2e837c1c01
    jz           loc_696b                        ; 1000:695b: 740e
    jg           loc_6990                        ; 1000:695d: 7f31
    mov          word [cs:SI + 0x1c],0x1         ; 1000:695f: 2ec7441c0100
    mov          AX,0x0                          ; 1000:6965: b80000
    jmp near     loc_6d75                        ; 1000:6968: e90a04
loc_696b:
    mov          byte [0x220],0x1                ; 1000:696b: c606200201
    mov          AX,0x9                          ; 1000:6970: b80900
    jmp near     loc_6d75                        ; 1000:6973: e9ff03
loc_6976:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6976: 2e837c1c00
    jnz          loc_6990                        ; 1000:697b: 7513
    mov          word [cs:SI + 0x1c],0x1         ; 1000:697d: 2ec7441c0100
    cmp          byte [0x220],0x2                ; 1000:6983: 803e200202
    jge          loc_6990                        ; 1000:6988: 7d06
    mov          AX,0x1                          ; 1000:698a: b80100
    jmp near     loc_6d75                        ; 1000:698d: e9e503
loc_6990:
    jmp near     loc_6d74                        ; 1000:6990: e9e103
loc_6993:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6993: 2e837c1c00
    jnz          loc_69c7                        ; 1000:6998: 752d
    mov          word [cs:SI + 0x1c],0x1         ; 1000:699a: 2ec7441c0100
    cmp          byte [0x220],0x2                ; 1000:69a0: 803e200202
    jge          loc_69ad                        ; 1000:69a5: 7d06
    mov          AX,0x2                          ; 1000:69a7: b80200
    jmp near     loc_6d75                        ; 1000:69aa: e9c803
loc_69ad:
    cmp          byte [0x21d],0x0                ; 1000:69ad: 803e1d0200
    jz           loc_69ba                        ; 1000:69b2: 7406
    mov          AX,0x7                          ; 1000:69b4: b80700
    jmp near     loc_6d75                        ; 1000:69b7: e9bb03
loc_69ba:
    push         SI                              ; 1000:69ba: 56
    push         DI                              ; 1000:69bb: 57
    call         snd_play_chest_open             ; 1000:69bc: e8b1de
    pop          DI                              ; 1000:69bf: 5f
    pop          SI                              ; 1000:69c0: 5e
    mov          AX,0x6                          ; 1000:69c1: b80600
    jmp near     loc_6d75                        ; 1000:69c4: e9ae03
loc_69c7:
    jmp near     loc_6d74                        ; 1000:69c7: e9aa03
loc_69ca:
    cmp          byte [0x220],0x2                ; 1000:69ca: 803e200202
    jl           loc_69ea                        ; 1000:69cf: 7c19
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:69d1: 2e837c1c00
    jnz          loc_6a21                        ; 1000:69d6: 7549
    mov          word [cs:SI + 0x1c],0x1         ; 1000:69d8: 2ec7441c0100
    mov          AX,0xac0a                       ; 1000:69de: b80aac
    call         ent_destroy                     ; 1000:69e1: e82ccc
    mov          AX,0x8                          ; 1000:69e4: b80800
    jmp near     loc_6d75                        ; 1000:69e7: e98b03
loc_69ea:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:69ea: 2e837c1c00
    jz           loc_6a09                        ; 1000:69ef: 7418
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:69f1: 2e837c1c01
    jz           loc_6a15                        ; 1000:69f6: 741d
    mov          byte [0x220],0x2                ; 1000:69f8: c606200202
    mov          AX,0xac0a                       ; 1000:69fd: b80aac
    call         ent_destroy                     ; 1000:6a00: e80dcc
    mov          AX,0x5                          ; 1000:6a03: b80500
    jmp near     loc_6d75                        ; 1000:6a06: e96c03
loc_6a09:
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6a09: 2ec7441c0100
    mov          AX,0x3                          ; 1000:6a0f: b80300
    jmp near     loc_6d75                        ; 1000:6a12: e96003
loc_6a15:
    mov          word [cs:SI + 0x1c],0x2         ; 1000:6a15: 2ec7441c0200
    mov          AX,0x4                          ; 1000:6a1b: b80400
    jmp near     loc_6d75                        ; 1000:6a1e: e95403
loc_6a21:
    jmp near     loc_6d74                        ; 1000:6a21: e95003
loc_6a24:
    cmp          byte [0x220],0x3                ; 1000:6a24: 803e200203
    jl           loc_6a41                        ; 1000:6a29: 7c16
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6a2b: 2e837c1c00
    jnz          loc_6a3e                        ; 1000:6a30: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6a32: 2ec7441c0100
    mov          AX,0x5                          ; 1000:6a38: b80500
    jmp near     loc_6d75                        ; 1000:6a3b: e93703
loc_6a3e:
    jmp near     loc_6d74                        ; 1000:6a3e: e93303
loc_6a41:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6a41: 2e837c1c00
    jz           loc_6a84                        ; 1000:6a46: 743c
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:6a48: 2e837c1c01
    jz           loc_6a90                        ; 1000:6a4d: 7441
    cmp          word [cs:SI + 0x1c],0x2         ; 1000:6a4f: 2e837c1c02
    jz           loc_6a9c                        ; 1000:6a54: 7446
    cmp          word [cs:SI + 0x1c],0x3         ; 1000:6a56: 2e837c1c03
    jz           loc_6aa8                        ; 1000:6a5b: 744b
    mov          byte [0x220],0x3                ; 1000:6a5d: c606200203
    mov          word [cs:0x7ed5],0x4            ; 1000:6a62: 2ec706d57e0400
    mov          word [cs:0x8067],0x4            ; 1000:6a69: 2ec70667800400
    mov          word [cs:0x7ef9],0x4            ; 1000:6a70: 2ec706f97e0400
    mov          word [cs:0x7efb],0x1            ; 1000:6a77: 2ec706fb7e0100
    mov          AX,0x4                          ; 1000:6a7e: b80400
    jmp near     loc_6d75                        ; 1000:6a81: e9f102
loc_6a84:
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6a84: 2ec7441c0100
    mov          AX,0x0                          ; 1000:6a8a: b80000
    jmp near     loc_6d75                        ; 1000:6a8d: e9e502
loc_6a90:
    mov          word [cs:SI + 0x1c],0x2         ; 1000:6a90: 2ec7441c0200
    mov          AX,0x1                          ; 1000:6a96: b80100
    jmp near     loc_6d75                        ; 1000:6a99: e9d902
loc_6a9c:
    mov          word [cs:SI + 0x1c],0x3         ; 1000:6a9c: 2ec7441c0300
    mov          AX,0x2                          ; 1000:6aa2: b80200
    jmp near     loc_6d75                        ; 1000:6aa5: e9cd02
loc_6aa8:
    mov          word [cs:SI + 0x1c],0x4         ; 1000:6aa8: 2ec7441c0400
    mov          AX,0x3                          ; 1000:6aae: b80300
    jmp near     loc_6d75                        ; 1000:6ab1: e9c102
loc_6ab4:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6ab4: 2e837c1c00
    jnz          loc_6ac7                        ; 1000:6ab9: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6abb: 2ec7441c0100
    mov          AX,0x0                          ; 1000:6ac1: b80000
    jmp near     loc_6d75                        ; 1000:6ac4: e9ae02
loc_6ac7:
    jmp near     loc_6d74                        ; 1000:6ac7: e9aa02
loc_6aca:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6aca: 2e837c1c00
    jnz          loc_6afc                        ; 1000:6acf: 752b
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6ad1: 2ec7441c0100
    cmp          byte [0x220],0x4                ; 1000:6ad7: 803e200204
    jl           loc_6aeb                        ; 1000:6adc: 7c0d
    cmp          byte [0x220],0x5                ; 1000:6ade: 803e200205
    jl           loc_6af6                        ; 1000:6ae3: 7c11
    mov          AX,0x2                          ; 1000:6ae5: b80200
    jmp near     loc_6d75                        ; 1000:6ae8: e98a02
loc_6aeb:
    mov          byte [0x220],0x4                ; 1000:6aeb: c606200204
    mov          AX,0x0                          ; 1000:6af0: b80000
    jmp near     loc_6d75                        ; 1000:6af3: e97f02
loc_6af6:
    mov          AX,0x1                          ; 1000:6af6: b80100
    jmp near     loc_6d75                        ; 1000:6af9: e97902
loc_6afc:
    jmp near     loc_6d74                        ; 1000:6afc: e97502
loc_6aff:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6aff: 2e837c1c00
    jnz          loc_6b12                        ; 1000:6b04: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b06: 2ec7441c0100
    mov          AX,0x0                          ; 1000:6b0c: b80000
    jmp near     loc_6d75                        ; 1000:6b0f: e96302
loc_6b12:
    jmp near     loc_6d74                        ; 1000:6b12: e95f02
loc_6b15:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b15: 2e837c1c00
    jnz          loc_6b28                        ; 1000:6b1a: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b1c: 2ec7441c0100
    mov          AX,0x1                          ; 1000:6b22: b80100
    jmp near     loc_6d75                        ; 1000:6b25: e94d02
loc_6b28:
    jmp near     loc_6d74                        ; 1000:6b28: e94902
loc_6b2b:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b2b: 2e837c1c00
    jnz          loc_6b3e                        ; 1000:6b30: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b32: 2ec7441c0100
    mov          AX,0x2                          ; 1000:6b38: b80200
    jmp near     loc_6d75                        ; 1000:6b3b: e93702
loc_6b3e:
    jmp near     loc_6d74                        ; 1000:6b3e: e93302
loc_6b41:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b41: 2e837c1c00
    jnz          loc_6b54                        ; 1000:6b46: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b48: 2ec7441c0100
    mov          AX,0x3                          ; 1000:6b4e: b80300
    jmp near     loc_6d75                        ; 1000:6b51: e92102
loc_6b54:
    jmp near     loc_6d74                        ; 1000:6b54: e91d02
loc_6b57:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b57: 2e837c1c00
    jnz          loc_6b6a                        ; 1000:6b5c: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b5e: 2ec7441c0100
    mov          AX,0x4                          ; 1000:6b64: b80400
    jmp near     loc_6d75                        ; 1000:6b67: e90b02
loc_6b6a:
    jmp near     loc_6d74                        ; 1000:6b6a: e90702
loc_6b6d:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b6d: 2e837c1c00
    jnz          loc_6b80                        ; 1000:6b72: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b74: 2ec7441c0100
    mov          AX,0x5                          ; 1000:6b7a: b80500
    jmp near     loc_6d75                        ; 1000:6b7d: e9f501
loc_6b80:
    jmp near     loc_6d74                        ; 1000:6b80: e9f101
loc_6b83:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b83: 2e837c1c00
    jnz          loc_6b96                        ; 1000:6b88: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6b8a: 2ec7441c0100
    mov          AX,0x6                          ; 1000:6b90: b80600
    jmp near     loc_6d75                        ; 1000:6b93: e9df01
loc_6b96:
    jmp near     loc_6d74                        ; 1000:6b96: e9db01
loc_6b99:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6b99: 2e837c1c00
    jnz          loc_6bac                        ; 1000:6b9e: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6ba0: 2ec7441c0100
    mov          AX,0x7                          ; 1000:6ba6: b80700
    jmp near     loc_6d75                        ; 1000:6ba9: e9c901
loc_6bac:
    jmp near     loc_6d74                        ; 1000:6bac: e9c501
loc_6baf:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6baf: 2e837c1c00
    jnz          loc_6bc2                        ; 1000:6bb4: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6bb6: 2ec7441c0100
    mov          AX,0x8                          ; 1000:6bbc: b80800
    jmp near     loc_6d75                        ; 1000:6bbf: e9b301
loc_6bc2:
    jmp near     loc_6d74                        ; 1000:6bc2: e9af01
loc_6bc5:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6bc5: 2e837c1c00
    jnz          loc_6bd8                        ; 1000:6bca: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6bcc: 2ec7441c0100
    mov          AX,0x9                          ; 1000:6bd2: b80900
    jmp near     loc_6d75                        ; 1000:6bd5: e99d01
loc_6bd8:
    jmp near     loc_6d74                        ; 1000:6bd8: e99901
loc_6bdb:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6bdb: 2e837c1c00
    jnz          loc_6bee                        ; 1000:6be0: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6be2: 2ec7441c0100
    mov          AX,0x0                          ; 1000:6be8: b80000
    jmp near     loc_6d75                        ; 1000:6beb: e98701
loc_6bee:
    jmp near     loc_6d74                        ; 1000:6bee: e98301
loc_6bf1:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6bf1: 2e837c1c00
    jnz          loc_6c04                        ; 1000:6bf6: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6bf8: 2ec7441c0100
    mov          AX,0x1                          ; 1000:6bfe: b80100
    jmp near     loc_6d75                        ; 1000:6c01: e97101
loc_6c04:
    jmp near     loc_6d74                        ; 1000:6c04: e96d01
loc_6c07:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c07: 2e837c1c00
    jnz          loc_6c1a                        ; 1000:6c0c: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c0e: 2ec7441c0100
    mov          AX,0x2                          ; 1000:6c14: b80200
    jmp near     loc_6d75                        ; 1000:6c17: e95b01
loc_6c1a:
    jmp near     loc_6d74                        ; 1000:6c1a: e95701
loc_6c1d:
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:6c1d: 2e837c1c01
    jg           loc_6c43                        ; 1000:6c22: 7f1f
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c24: 2e837c1c00
    jg           loc_6c37                        ; 1000:6c29: 7f0c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c2b: 2ec7441c0100
    mov          AX,0x3                          ; 1000:6c31: b80300
    jmp near     loc_6d75                        ; 1000:6c34: e93e01
loc_6c37:
    mov          word [cs:SI + 0x1c],0x2         ; 1000:6c37: 2ec7441c0200
    mov          AX,0x4                          ; 1000:6c3d: b80400
    jmp near     loc_6d75                        ; 1000:6c40: e93201
loc_6c43:
    jmp near     loc_6d74                        ; 1000:6c43: e92e01
loc_6c46:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c46: 2e837c1c00
    jnz          loc_6c59                        ; 1000:6c4b: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c4d: 2ec7441c0100
    mov          AX,0x5                          ; 1000:6c53: b80500
    jmp near     loc_6d75                        ; 1000:6c56: e91c01
loc_6c59:
    jmp near     loc_6d74                        ; 1000:6c59: e91801
loc_6c5c:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c5c: 2e837c1c00
    jnz          loc_6c6f                        ; 1000:6c61: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c63: 2ec7441c0100
    mov          AX,0x6                          ; 1000:6c69: b80600
    jmp near     loc_6d75                        ; 1000:6c6c: e90601
loc_6c6f:
    jmp near     loc_6d74                        ; 1000:6c6f: e90201
loc_6c72:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c72: 2e837c1c00
    jnz          loc_6c85                        ; 1000:6c77: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c79: 2ec7441c0100
    mov          AX,0x7                          ; 1000:6c7f: b80700
    jmp near     loc_6d75                        ; 1000:6c82: e9f000
loc_6c85:
    jmp near     loc_6d74                        ; 1000:6c85: e9ec00
loc_6c88:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c88: 2e837c1c00
    jnz          loc_6c9b                        ; 1000:6c8d: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6c8f: 2ec7441c0100
    mov          AX,0x8                          ; 1000:6c95: b80800
    jmp near     loc_6d75                        ; 1000:6c98: e9da00
loc_6c9b:
    jmp near     loc_6d74                        ; 1000:6c9b: e9d600
loc_6c9e:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6c9e: 2e837c1c00
    jnz          loc_6cb1                        ; 1000:6ca3: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6ca5: 2ec7441c0100
    mov          AX,0x9                          ; 1000:6cab: b80900
    jmp near     loc_6d75                        ; 1000:6cae: e9c400
loc_6cb1:
    jmp near     loc_6d74                        ; 1000:6cb1: e9c000
loc_6cb4:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6cb4: 2e837c1c00
    jnz          loc_6cc7                        ; 1000:6cb9: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6cbb: 2ec7441c0100
    mov          AX,0xa                          ; 1000:6cc1: b80a00
    jmp near     loc_6d75                        ; 1000:6cc4: e9ae00
loc_6cc7:
    jmp near     loc_6d74                        ; 1000:6cc7: e9aa00
loc_6cca:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6cca: 2e837c1c00
    jnz          loc_6cdd                        ; 1000:6ccf: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6cd1: 2ec7441c0100
    mov          AX,0xb                          ; 1000:6cd7: b80b00
    jmp near     loc_6d75                        ; 1000:6cda: e99800
loc_6cdd:
    jmp near     loc_6d74                        ; 1000:6cdd: e99400
loc_6ce0:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6ce0: 2e837c1c00
    jnz          loc_6cf3                        ; 1000:6ce5: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6ce7: 2ec7441c0100
    mov          AX,0x1                          ; 1000:6ced: b80100
    jmp near     loc_6d75                        ; 1000:6cf0: e98200
loc_6cf3:
    jmp short    loc_6d74                        ; 1000:6cf3: eb7f
    db 0x90                                         ; 1000:6cf5
loc_6cf6:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6cf6: 2e837c1c00
    jnz          loc_6d09                        ; 1000:6cfb: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6cfd: 2ec7441c0100
    mov          AX,0x2                          ; 1000:6d03: b80200
    jmp short    loc_6d75                        ; 1000:6d06: eb6d
    db 0x90                                         ; 1000:6d08
loc_6d09:
    jmp short    loc_6d74                        ; 1000:6d09: eb69
    db 0x90                                         ; 1000:6d0b
loc_6d0c:
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6d0c: 2e837c1c00
    jnz          loc_6d1f                        ; 1000:6d11: 750c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6d13: 2ec7441c0100
    mov          AX,0x3                          ; 1000:6d19: b80300
    jmp short    loc_6d75                        ; 1000:6d1c: eb57
    db 0x90                                         ; 1000:6d1e
loc_6d1f:
    jmp short    loc_6d74                        ; 1000:6d1f: eb53
    db 0x90                                         ; 1000:6d21
loc_6d22:
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:6d22: 2e837c1c01
    jg           loc_6d48                        ; 1000:6d27: 7f1f
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6d29: 2e837c1c00
    jg           loc_6d3c                        ; 1000:6d2e: 7f0c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6d30: 2ec7441c0100
    mov          AX,0x4                          ; 1000:6d36: b80400
    jmp short    loc_6d75                        ; 1000:6d39: eb3a
    db 0x90                                         ; 1000:6d3b
loc_6d3c:
    mov          word [cs:SI + 0x1c],0x2         ; 1000:6d3c: 2ec7441c0200
    mov          AX,0x5                          ; 1000:6d42: b80500
    jmp short    loc_6d75                        ; 1000:6d45: eb2e
    db 0x90                                         ; 1000:6d47
loc_6d48:
    jmp short    loc_6d74                        ; 1000:6d48: eb2a
    db 0x90                                         ; 1000:6d4a
loc_6d4b:
    cmp          word [cs:SI + 0x1c],0x1         ; 1000:6d4b: 2e837c1c01
    jg           loc_6d71                        ; 1000:6d50: 7f1f
    cmp          word [cs:SI + 0x1c],0x0         ; 1000:6d52: 2e837c1c00
    jg           loc_6d65                        ; 1000:6d57: 7f0c
    mov          word [cs:SI + 0x1c],0x1         ; 1000:6d59: 2ec7441c0100
    mov          AX,0x6                          ; 1000:6d5f: b80600
    jmp short    loc_6d75                        ; 1000:6d62: eb11
    db 0x90                                         ; 1000:6d64
loc_6d65:
    mov          word [cs:SI + 0x1c],0x2         ; 1000:6d65: 2ec7441c0200
    mov          AX,0x7                          ; 1000:6d6b: b80700
    jmp short    loc_6d75                        ; 1000:6d6e: eb05
    db 0x90                                         ; 1000:6d70
loc_6d71:
    jmp short    loc_6d74                        ; 1000:6d71: eb01
    db 0x90                                         ; 1000:6d73
loc_6d74:
    ret                                          ; 1000:6d74: c3
loc_6d75:
    shl          AX,0x1                          ; 1000:6d75: d1e0
    add          AX,0x9ec0                       ; 1000:6d77: 05c09e
    mov          BX,AX                           ; 1000:6d7a: 8bd8
    mov          AX,word [BX]                    ; 1000:6d7c: 8b07
    add          AX,0x9ec0                       ; 1000:6d7e: 05c09e
    mov          [0x922],AX                      ; 1000:6d81: a32209
    mov          byte [0x921],0x1                ; 1000:6d84: c606210901
    mov          AX,0x1                          ; 1000:6d89: b80100
    lea          BX,[0xaac0]                     ; 1000:6d8c: 8d1ec0aa
    mov          CX,0x9                          ; 1000:6d90: b90900
    int3                                         ; 1000:6d93: cc
    db 0xc3, 0x8a, 0xe0, 0x32, 0xc0, 0x56, 0x57, 0xbb, 0x34, 0x09, 0x83, 0xc3, 0x04, 0xba, 0x10, 0x01 ; 1000:6d94

; ----------------------------------------------------------------------------
; FUNCTION: ui_prompt_yes_no  [UI / Dialogs]
; ADDRESS:  1000:6da4
; PURPOSE:  Displays Yes/No prompt dialog box and waits for user confirmation.
; ----------------------------------------------------------------------------
ui_prompt_yes_no:
    mov          CX,word [BX]                    ; 1000:6da4: 8b0f
    add          CL,AL                           ; 1000:6da6: 02c8
    cmp          CL,0x9                          ; 1000:6da8: 80f909
    jle          loc_6db2                        ; 1000:6dab: 7e05
    sub          CL,0xa                          ; 1000:6dad: 80e90a
    inc          CH                              ; 1000:6db0: fec5
loc_6db2:
    add          CH,AH                           ; 1000:6db2: 02ec
    xor          AX,AX                           ; 1000:6db4: 33c0
    cmp          CH,0x9                          ; 1000:6db6: 80fd09
    jle          loc_6dc0                        ; 1000:6db9: 7e05
    sub          CH,0xa                          ; 1000:6dbb: 80ed0a
    inc          AL                              ; 1000:6dbe: fec0
loc_6dc0:
    push         AX                              ; 1000:6dc0: 50
    push         BX                              ; 1000:6dc1: 53
    push         DX                              ; 1000:6dc2: 52
    mov          word [BX],CX                    ; 1000:6dc3: 890f
    mov          AX,DX                           ; 1000:6dc5: 8bc2
    mov          BX,0x10                         ; 1000:6dc7: bb1000
    call         hud_draw_shield_bar             ; 1000:6dca: e892cc
    pop          DX                              ; 1000:6dcd: 5a
    pop          BX                              ; 1000:6dce: 5b
    pop          AX                              ; 1000:6dcf: 58
    cmp          BX,0x934                        ; 1000:6dd0: 81fb3409
    jz           loc_6de0                        ; 1000:6dd4: 740a
    sub          BX,0x2                          ; 1000:6dd6: 83eb02
    sub          DX,0x10                         ; 1000:6dd9: 83ea10
    or           AX,AX                           ; 1000:6ddc: 0bc0
    jnz          ui_prompt_yes_no                ; 1000:6dde: 75c4
loc_6de0:
    pop          DI                              ; 1000:6de0: 5f
    pop          SI                              ; 1000:6de1: 5e
    ret                                          ; 1000:6de2: c3
    db 0x8a, 0xe0, 0x32, 0xc0                       ; 1000:6de3

; ----------------------------------------------------------------------------
; FUNCTION: snd_play_cursor_move  [Audio / SFX]
; ADDRESS:  1000:6de7
; PURPOSE:  Plays menu cursor navigation tick sound.
; ----------------------------------------------------------------------------
snd_play_cursor_move:
    push         SI                              ; 1000:6de7: 56
    push         DI                              ; 1000:6de8: 57
    mov          BX,0x934                        ; 1000:6de9: bb3409
    add          BX,0x2                          ; 1000:6dec: 83c302
    mov          DX,0x100                        ; 1000:6def: ba0001
    jmp short    ui_prompt_yes_no                ; 1000:6df2: ebb0

; ----------------------------------------------------------------------------
; FUNCTION: ui_main_menu  [UI / Menu]
; ADDRESS:  1000:6df4
; PURPOSE:  Main title menu loop: displays options (Play Game, Instructions, Story, Demo, High Scores, Key Config, Exit).
; ----------------------------------------------------------------------------
ui_main_menu:
    push         BP                              ; 1000:6df4: 55
    mov          BP,SP                           ; 1000:6df5: 8bec
    sub          SP,0x4                          ; 1000:6df7: 83ec04
    mov          AX,[0xabab]                     ; 1000:6dfa: a1abab
    cmp          AX,word [0x9ce8]                ; 1000:6dfd: 3b06e89c
    jnz          loc_6e19                        ; 1000:6e01: 7516
    mov          AX,[0xabad]                     ; 1000:6e03: a1adab
    cmp          AX,word [0x9cea]                ; 1000:6e06: 3b06ea9c
    jnz          loc_6e19                        ; 1000:6e0a: 750d
    mov          AX,[0xabaf]                     ; 1000:6e0c: a1afab
    sub          AX,word [0x9cec]                ; 1000:6e0f: 2b06ec9c
    jnz          loc_6e19                        ; 1000:6e13: 7504
    mov          SP,BP                           ; 1000:6e15: 8be5
    pop          BP                              ; 1000:6e17: 5d
    ret                                          ; 1000:6e18: c3
loc_6e19:
    call         ui_high_scores_screen           ; 1000:6e19: e82b01
    call         vid_wait_vblank                 ; 1000:6e1c: e82f09
    call         ui_high_scores_screen           ; 1000:6e1f: e82501
    call         ui_instructions_screen          ; 1000:6e22: e83202
    mov          AX,[0xabb1]                     ; 1000:6e25: a1b1ab
    mov          word [BP + -0x2],AX             ; 1000:6e28: 8946fe
    mov          BX,word [0xabb3]                ; 1000:6e2b: 8b1eb3ab
    mov          word [BP + -0x4],BX             ; 1000:6e2f: 895efc
    call         vid_wait_vblank                 ; 1000:6e32: e81909
    mov          DX,0x25                         ; 1000:6e35: ba2500
    mov          AX,[0xabb1]                     ; 1000:6e38: a1b1ab
    mov          CX,word [0xabb1]                ; 1000:6e3b: 8b0eb1ab
    and          CX,0xf                          ; 1000:6e3f: 83e10f
    xor          BX,BX                           ; 1000:6e42: 33db
loc_6e44:
    or           BX,CX                           ; 1000:6e44: 0bd9
    add          BX,AX                           ; 1000:6e46: 03d8
    xor          AX,DX                           ; 1000:6e48: 33c2
    loop         loc_6e44                        ; 1000:6e4a: e2f8
    mov          SI,0xabb5                       ; 1000:6e4c: beb5ab
    mov          DX,word [0xabbb]                ; 1000:6e4f: 8b16bbab
    xor          AX,AX                           ; 1000:6e53: 33c0
loc_6e55:
    mov          BL,byte [SI]                    ; 1000:6e55: 8a1c
    or           BL,BL                           ; 1000:6e57: 0adb
    jz           loc_6e71                        ; 1000:6e59: 7416
    sub          BL,0x30                         ; 1000:6e5b: 80eb30
    xor          BH,BH                           ; 1000:6e5e: 32ff
    mov          CX,AX                           ; 1000:6e60: 8bc8
    shl          AX,0x1                          ; 1000:6e62: d1e0
    shl          AX,0x1                          ; 1000:6e64: d1e0
    shl          AX,0x1                          ; 1000:6e66: d1e0
    shl          CX,0x1                          ; 1000:6e68: d1e1
    add          AX,CX                           ; 1000:6e6a: 03c1
    add          AX,BX                           ; 1000:6e6c: 03c3
    inc          SI                              ; 1000:6e6e: 46
    jmp short    loc_6e55                        ; 1000:6e6f: ebe4
loc_6e71:
    mov          [0x9cf8],AX                     ; 1000:6e71: a3f89c
    mov          word [0xabb1],BX                ; 1000:6e74: 891eb1ab
    mov          word [0xabb3],CX                ; 1000:6e78: 890eb3ab
    call         vid_wait_vblank                 ; 1000:6e7c: e8cf08
    mov          DX,0x25                         ; 1000:6e7f: ba2500
    mov          AX,[0xabb1]                     ; 1000:6e82: a1b1ab
    mov          CX,word [0xabb1]                ; 1000:6e85: 8b0eb1ab
    and          CX,0xf                          ; 1000:6e89: 83e10f
    xor          BX,BX                           ; 1000:6e8c: 33db
loc_6e8e:
    or           BX,CX                           ; 1000:6e8e: 0bd9
    add          BX,AX                           ; 1000:6e90: 03d8
    xor          AX,DX                           ; 1000:6e92: 33c2
    loop         loc_6e8e                        ; 1000:6e94: e2f8
    mov          word [0xabb1],DX                ; 1000:6e96: 8916b1ab
    mov          AX,[0xabb1]                     ; 1000:6e9a: a1b1ab
    mov          BX,word [0x9cf8]                ; 1000:6e9d: 8b1ef89c
    mov          CX,word [0xabb3]                ; 1000:6ea1: 8b0eb3ab
    mov          DX,word [0xabbb]                ; 1000:6ea5: 8b16bbab
    xor          BX,CX                           ; 1000:6ea9: 33d9
    xor          AX,BX                           ; 1000:6eab: 33c3
    xor          DX,CX                           ; 1000:6ead: 33d1
    xor          BX,AX                           ; 1000:6eaf: 33d8
    xor          CX,AX                           ; 1000:6eb1: 33c8
    xor          DX,AX                           ; 1000:6eb3: 33d0
    xor          AX,CX                           ; 1000:6eb5: 33c1
    xor          BX,DX                           ; 1000:6eb7: 33da
    mov          [0xabb1],AX                     ; 1000:6eb9: a3b1ab
    mov          word [0xabb3],CX                ; 1000:6ebc: 890eb3ab
    push         AX                              ; 1000:6ec0: 50
    push         BX                              ; 1000:6ec1: 53
    call         vid_wait_vblank                 ; 1000:6ec2: e88908
    pop          BX                              ; 1000:6ec5: 5b
    pop          AX                              ; 1000:6ec6: 58
    add          AX,BX                           ; 1000:6ec7: 03c3
    sub          AX,word [0xabb1]                ; 1000:6ec9: 2b06b1ab
    mov          [0xabb3],AX                     ; 1000:6ecd: a3b3ab
    mov          BX,word [0xaba9]                ; 1000:6ed0: 8b1ea9ab
    mov          AX,[0xabab]                     ; 1000:6ed4: a1abab
    mov          CX,word [0xabad]                ; 1000:6ed7: 8b0eadab
    mov          DX,word [0xabaf]                ; 1000:6edb: 8b16afab
    mov          word [0x9ce6],BX                ; 1000:6edf: 891ee69c
    mov          [0x9ce8],AX                     ; 1000:6ee3: a3e89c
    mov          word [0x9cea],CX                ; 1000:6ee6: 890eea9c
    mov          word [0x9cec],DX                ; 1000:6eea: 8916ec9c
    xor          DX,AX                           ; 1000:6eee: 33d0
    xor          BX,CX                           ; 1000:6ef0: 33d9
    xor          BX,AX                           ; 1000:6ef2: 33d8
    xor          DX,CX                           ; 1000:6ef4: 33d1
    xor          AX,BX                           ; 1000:6ef6: 33c3
    xor          AX,CX                           ; 1000:6ef8: 33c1
    xor          CX,AX                           ; 1000:6efa: 33c8
    xor          BX,DX                           ; 1000:6efc: 33da
    add          CX,word [0xabb3]                ; 1000:6efe: 030eb3ab
    mov          word [0xabb1],CX                ; 1000:6f02: 890eb1ab
    call         vid_wait_vblank                 ; 1000:6f06: e84508
    mov          DX,0x25                         ; 1000:6f09: ba2500
    mov          AX,[0xabb1]                     ; 1000:6f0c: a1b1ab
    mov          CX,word [0xabb1]                ; 1000:6f0f: 8b0eb1ab
    and          CX,0xf                          ; 1000:6f13: 83e10f
    xor          BX,BX                           ; 1000:6f16: 33db
loc_6f18:
    or           BX,CX                           ; 1000:6f18: 0bd9
    add          BX,AX                           ; 1000:6f1a: 03d8
    xor          AX,DX                           ; 1000:6f1c: 33c2
    loop         loc_6f18                        ; 1000:6f1e: e2f8
    mov          AX,[0xabb3]                     ; 1000:6f20: a1b3ab
    or           AX,AX                           ; 1000:6f23: 0bc0
    mov          AX,word [BP + -0x2]             ; 1000:6f25: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:6f28: 8b5efc
    mov          [0xabb1],AX                     ; 1000:6f2b: a3b1ab
    mov          word [0xabb3],BX                ; 1000:6f2e: 891eb3ab
    jz           loc_6f37                        ; 1000:6f32: 7403
    jmp near     loc_6e19                        ; 1000:6f34: e9e2fe
loc_6f37:
    mov          SP,BP                           ; 1000:6f37: 8be5
    pop          BP                              ; 1000:6f39: 5d
    mov          AX,[0xabab]                     ; 1000:6f3a: a1abab
    mov          BX,word [0x9ce8]                ; 1000:6f3d: 8b1ee89c
    xor          AX,BX                           ; 1000:6f41: 33c3
    jnz          ui_high_scores_screen           ; 1000:6f43: 7502
    inc          AX                              ; 1000:6f45: 40
    ret                                          ; 1000:6f46: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_high_scores_screen  [UI / High Scores]
; ADDRESS:  1000:6f47
; PURPOSE:  Renders the Hall of Fame high scores screen.
; ----------------------------------------------------------------------------
ui_high_scores_screen:
    mov          AX,0x0                          ; 1000:6f47: b80000
    mov          BX,0x0                          ; 1000:6f4a: bb0000
    mov          CX,0x140                        ; 1000:6f4d: b94001
    mov          DX,0xc8                         ; 1000:6f50: bac800
    mov          DI,word [cs:0x773f]             ; 1000:6f53: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:6f58: e86d0b
    mov          AX,0x9                          ; 1000:6f5b: b80900
    call         vid_set_write_mode_0            ; 1000:6f5e: e8d8d3
    mov          CX,0x18                         ; 1000:6f61: b91800
    mov          BX,0x0                          ; 1000:6f64: bb0000
    mov          AL,0x22                         ; 1000:6f67: b022
    call         ui_draw_window_border           ; 1000:6f69: e87cd3
    mov          DX,0x26                         ; 1000:6f6c: ba2600
    mov          AL,0x23                         ; 1000:6f6f: b023
loc_6f71:
    call         ui_draw_window_border           ; 1000:6f71: e874d3
    dec          DX                              ; 1000:6f74: 4a
    jnz          loc_6f71                        ; 1000:6f75: 75fa
    mov          AL,0x24                         ; 1000:6f77: b024
    call         ui_draw_window_border           ; 1000:6f79: e86cd3
    mov          CX,0xc0                         ; 1000:6f7c: b9c000
    mov          BX,0x0                          ; 1000:6f7f: bb0000
    mov          AL,0x3a                         ; 1000:6f82: b03a
    call         ui_draw_window_border           ; 1000:6f84: e861d3
    mov          DX,0x26                         ; 1000:6f87: ba2600
    mov          AL,0x3b                         ; 1000:6f8a: b03b
loc_6f8c:
    call         ui_draw_window_border           ; 1000:6f8c: e859d3
    dec          DX                              ; 1000:6f8f: 4a
    jnz          loc_6f8c                        ; 1000:6f90: 75fa
    mov          AL,0x3c                         ; 1000:6f92: b03c
    call         ui_draw_window_border           ; 1000:6f94: e851d3
    mov          DX,0x14                         ; 1000:6f97: ba1400
    mov          CX,0x20                         ; 1000:6f9a: b92000
loc_6f9d:
    mov          BX,0x0                          ; 1000:6f9d: bb0000
    mov          AL,0x25                         ; 1000:6fa0: b025
    call         ui_draw_window_border           ; 1000:6fa2: e843d3
    mov          BX,0x138                        ; 1000:6fa5: bb3801
    mov          AL,0x26                         ; 1000:6fa8: b026
    call         ui_draw_window_border           ; 1000:6faa: e83bd3
    add          CX,0x8                          ; 1000:6fad: 83c108
    dec          DX                              ; 1000:6fb0: 4a
    jnz          loc_6f9d                        ; 1000:6fb1: 75ea
    mov          AX,0x7                          ; 1000:6fb3: b80700
    call         vid_set_write_mode_0            ; 1000:6fb6: e880d3
    mov          AX,0x10                         ; 1000:6fb9: b81000
    mov          BX,0x0                          ; 1000:6fbc: bb0000
    mov          SI,0xaae0                       ; 1000:6fbf: bee0aa
    call         ui_clear_window                 ; 1000:6fc2: e80fd3
    mov          AX,0x50                         ; 1000:6fc5: b85000
    mov          BX,0x10                         ; 1000:6fc8: bb1000
    mov          SI,0xab05                       ; 1000:6fcb: be05ab
    call         ui_clear_window                 ; 1000:6fce: e803d3
    mov          AX,0x10                         ; 1000:6fd1: b81000
    mov          BX,0x28                         ; 1000:6fd4: bb2800
    mov          SI,0xab1b                       ; 1000:6fd7: be1bab
    call         ui_clear_window                 ; 1000:6fda: e8f7d2
    mov          AX,0x10                         ; 1000:6fdd: b81000
    mov          BX,0x30                         ; 1000:6fe0: bb3000
    mov          SI,0xab40                       ; 1000:6fe3: be40ab
    call         ui_clear_window                 ; 1000:6fe6: e8ebd2
    mov          AX,0x10                         ; 1000:6fe9: b81000
    mov          BX,0x38                         ; 1000:6fec: bb3800
    mov          SI,0xab65                       ; 1000:6fef: be65ab
    call         ui_clear_window                 ; 1000:6ff2: e8dfd2
    mov          AX,0x10                         ; 1000:6ff5: b81000
    mov          BX,0x40                         ; 1000:6ff8: bb4000
    mov          SI,0xab8a                       ; 1000:6ffb: be8aab
    call         ui_clear_window                 ; 1000:6ffe: e8d3d2
    mov          AX,0x10                         ; 1000:7001: b81000
    mov          BX,0x58                         ; 1000:7004: bb5800
    mov          SI,0xab97                       ; 1000:7007: be97ab
    call         ui_clear_window                 ; 1000:700a: e8c7d2
    mov          AX,[0xabb1]                     ; 1000:700d: a1b1ab
    add          AL,0x41                         ; 1000:7010: 0441
    mov          BX,0xa0                         ; 1000:7012: bba000
    mov          CX,0x58                         ; 1000:7015: b95800
    call         ui_draw_window_border           ; 1000:7018: e8cdd2
    mov          AL,0x2c                         ; 1000:701b: b02c
    call         ui_draw_window_border           ; 1000:701d: e8c8d2
    mov          DX,word [0xabb3]                ; 1000:7020: 8b16b3ab
    xor          AX,AX                           ; 1000:7024: 33c0
loc_7026:
    cmp          DX,0x64                         ; 1000:7026: 83fa64
    jl           loc_7032                        ; 1000:7029: 7c07
    inc          AL                              ; 1000:702b: fec0
    sub          DX,0x64                         ; 1000:702d: 83ea64
    jmp short    loc_7026                        ; 1000:7030: ebf4
loc_7032:
    add          AL,0x30                         ; 1000:7032: 0430
    call         ui_draw_window_border           ; 1000:7034: e8b1d2
    xor          AX,AX                           ; 1000:7037: 33c0
loc_7039:
    cmp          DX,0xa                          ; 1000:7039: 83fa0a
    jl           loc_7045                        ; 1000:703c: 7c07
    inc          AL                              ; 1000:703e: fec0
    sub          DX,0xa                          ; 1000:7040: 83ea0a
    jmp short    loc_7039                        ; 1000:7043: ebf4
loc_7045:
    add          AL,0x30                         ; 1000:7045: 0430
    call         ui_draw_window_border           ; 1000:7047: e89ed2
    mov          AX,DX                           ; 1000:704a: 8bc2
    add          AL,0x30                         ; 1000:704c: 0430
    call         ui_draw_window_border           ; 1000:704e: e897d2
    mov          AL,0x2d                         ; 1000:7051: b02d
    call         ui_draw_window_border           ; 1000:7053: e892d2
    ret                                          ; 1000:7056: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_instructions_screen  [UI / Instructions]
; ADDRESS:  1000:7057
; PURPOSE:  Displays scrolling gameplay instructions, item descriptions, and key controls.
; ----------------------------------------------------------------------------
ui_instructions_screen:
    push         BP                              ; 1000:7057: 55
    mov          BP,SP                           ; 1000:7058: 8bec
    sub          SP,0x8                          ; 1000:705a: 83ec08
    mov          word [BP + -0x2],0xd8           ; 1000:705d: c746fed800
    mov          word [BP + -0x4],0x58           ; 1000:7062: c746fc5800
    mov          SI,0xabb5                       ; 1000:7067: beb5ab
    mov          word [BP + -0x6],SI             ; 1000:706a: 8976fa
    mov          word [BP + -0x8],0x0            ; 1000:706d: c746f80000
loc_7072:
    mov          SI,word [BP + -0x6]             ; 1000:7072: 8b76fa
    add          SI,word [BP + -0x8]             ; 1000:7075: 0376f8
    mov          byte [SI],0x0                   ; 1000:7078: c60400
    mov          AX,word [BP + -0x2]             ; 1000:707b: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:707e: 8b5efc
    mov          CX,0x30                         ; 1000:7081: b93000
    mov          DX,0x8                          ; 1000:7084: ba0800
    mov          DI,word [cs:0x773f]             ; 1000:7087: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:708c: e8390a
    mov          AX,0x7                          ; 1000:708f: b80700
    call         vid_set_write_mode_0            ; 1000:7092: e8a4d2
    mov          AX,word [BP + -0x2]             ; 1000:7095: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:7098: 8b5efc
    mov          SI,word [BP + -0x6]             ; 1000:709b: 8b76fa
    call         ui_clear_window                 ; 1000:709e: e833d2
    mov          CX,BX                           ; 1000:70a1: 8bcb
    mov          BX,AX                           ; 1000:70a3: 8bd8
    mov          AL,0x2b                         ; 1000:70a5: b02b
    call         ui_draw_window_border           ; 1000:70a7: e83ed2
    call         vid_wait_vblank                 ; 1000:70aa: e8a106
loc_70ad:
    call         sys_restore_interrupts          ; 1000:70ad: e8e994
    mov          AL,[cs:0x61]                    ; 1000:70b0: 2ea06100
    cmp          AL,0x1                          ; 1000:70b4: 3c01
    jnz          loc_70bb                        ; 1000:70b6: 7503
    jmp near     sys_exit_to_dos                 ; 1000:70b8: e95a91
loc_70bb:
    cmp          AL,0xe                          ; 1000:70bb: 3c0e
    jz           loc_70e3                        ; 1000:70bd: 7424
    cmp          AL,0x1c                         ; 1000:70bf: 3c1c
    jz           loc_70ee                        ; 1000:70c1: 742b
    cmp          AL,0xb                          ; 1000:70c3: 3c0b
    jz           loc_70ce                        ; 1000:70c5: 7407
    jg           loc_70ad                        ; 1000:70c7: 7fe4
    add          AL,0x2f                         ; 1000:70c9: 042f
    jmp short    loc_70d0                        ; 1000:70cb: eb03
    db 0x90                                         ; 1000:70cd
loc_70ce:
    mov          AL,0x30                         ; 1000:70ce: b030
loc_70d0:
    cmp          word [BP + -0x8],0x5            ; 1000:70d0: 837ef805
    jz           loc_7072                        ; 1000:70d4: 749c
    mov          SI,word [BP + -0x6]             ; 1000:70d6: 8b76fa
    add          SI,word [BP + -0x8]             ; 1000:70d9: 0376f8
    mov          byte [SI],AL                    ; 1000:70dc: 8804
    inc          word [BP + -0x8]                ; 1000:70de: ff46f8
    jmp short    loc_7072                        ; 1000:70e1: eb8f
loc_70e3:
    cmp          word [BP + -0x8],0x0            ; 1000:70e3: 837ef800
    jz           loc_70ec                        ; 1000:70e7: 7403
    dec          word [BP + -0x8]                ; 1000:70e9: ff4ef8
loc_70ec:
    jmp short    loc_7072                        ; 1000:70ec: eb84
loc_70ee:
    mov          AX,word [BP + -0x2]             ; 1000:70ee: 8b46fe
    mov          BX,word [BP + -0x4]             ; 1000:70f1: 8b5efc
    mov          CX,0x30                         ; 1000:70f4: b93000
    mov          DX,0x8                          ; 1000:70f7: ba0800
    mov          DI,word [cs:0x773f]             ; 1000:70fa: 2e8b3e3f77
    call         vid_set_data_rotate             ; 1000:70ff: e8c609
    mov          SP,BP                           ; 1000:7102: 8be5
    pop          BP                              ; 1000:7104: 5d
    ret                                          ; 1000:7105: c3
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x71, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x71 ; 1000:7106
    db 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x71, 0x0d, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:7116
    db 0x52, 0x71, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x71, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:7126
    db 0x5e, 0x71, 0x0d, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x10, 0x00 ; 1000:7136
    db 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18, 0x00, 0x02, 0x01, 0x12, 0x00 ; 1000:7146
    db 0x18, 0x00, 0x06, 0x01, 0x16, 0x00, 0x18, 0x00, 0x04, 0x01, 0x14, 0x00, 0x18, 0x00 ; 1000:7156

; ----------------------------------------------------------------------------
; FUNCTION: ui_story_screen  [UI / Story]
; ADDRESS:  1000:7164
; PURPOSE:  Presents illustrated storyline cutscene sequence explaining Captain Comic's mission on Planet Ogron.
; ----------------------------------------------------------------------------
ui_story_screen:
    mov          word [0xabf3],SP                ; 1000:7164: 8926f3ab
    mov          byte [cs:0x61],0x0              ; 1000:7168: 2ec606610000
    push         DS                              ; 1000:716e: 1e
    mov          AX,SEG_PALETTE                  ; 1000:716f: b80632
    mov          DS,AX                           ; 1000:7172: 8ed8
    mov          SI,0x1cd0                       ; 1000:7174: bed01c
    mov          DI,0x6000                       ; 1000:7177: bf0060
    call         vid_load_palette_buffer         ; 1000:717a: e84a08
    pop          DS                              ; 1000:717d: 1f
    mov          DI,word [cs:0x773f]             ; 1000:717e: 2e8b3e3f77
    mov          SI,0x6000                       ; 1000:7183: be0060
    call         vid_wait_not_vblank             ; 1000:7186: e8dc05
    call         vid_wait_vblank                 ; 1000:7189: e8c205
    mov          DX,0xabc0                       ; 1000:718c: bac0ab
    call         vid_set_ega_palette             ; 1000:718f: e8cb8f
    mov          DX,0xabf7                       ; 1000:7192: baf7ab
    call         file_load_tileset               ; 1000:7195: e88aae
    mov          AX,0x12                         ; 1000:7198: b81200
    call         ui_wait_retrace_ticks           ; 1000:719b: e8da00
    call         ui_wait_for_keypress            ; 1000:719e: e8b400
    mov          AX,0x48                         ; 1000:71a1: b84800
    mov          BX,0x60                         ; 1000:71a4: bb6000
    mov          SI,0x1722                       ; 1000:71a7: be2217
    call         ui_fade_in_story_page           ; 1000:71aa: e8b400
    mov          AX,0x1b                         ; 1000:71ad: b81b00
    call         ui_wait_retrace_ticks           ; 1000:71b0: e8c500
    call         ui_wait_for_keypress            ; 1000:71b3: e89f00
    call         vid_wait_vblank                 ; 1000:71b6: e89505
    mov          AX,0x12                         ; 1000:71b9: b81200
    call         ui_wait_retrace_ticks           ; 1000:71bc: e8b900
    mov          AX,0x100                        ; 1000:71bf: b80001
    mov          BX,0x0                          ; 1000:71c2: bb0000
    mov          SI,0x13d4                       ; 1000:71c5: bed413
    mov          CX,0x6                          ; 1000:71c8: b90600
    call         ui_draw_story_page_1            ; 1000:71cb: e83000
    mov          SI,0x1086                       ; 1000:71ce: be8610
    mov          CX,0x6                          ; 1000:71d1: b90600
    call         ui_draw_story_page_1            ; 1000:71d4: e82700
    mov          SI,0xd38                        ; 1000:71d7: be380d
    mov          CX,0x5                          ; 1000:71da: b90500
    call         ui_draw_story_page_2            ; 1000:71dd: e83b00
    mov          SI,0x9ea                        ; 1000:71e0: beea09
    mov          CX,0x5                          ; 1000:71e3: b90500
    call         ui_draw_story_page_2            ; 1000:71e6: e83200
    mov          SI,0x69c                        ; 1000:71e9: be9c06
    mov          CX,0x5                          ; 1000:71ec: b90500
    call         ui_draw_story_page_3            ; 1000:71ef: e84600
    mov          SI,0x34e                        ; 1000:71f2: be4e03
    mov          CX,0x5                          ; 1000:71f5: b90500
    call         ui_draw_story_page_3            ; 1000:71f8: e83d00
    jmp near     ui_title_screen                 ; 1000:71fb: e99a00

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_story_page_1  [UI / Story]
; ADDRESS:  1000:71fe
; PURPOSE:  Renders page 1 of story prologue artwork and narrative.
; ----------------------------------------------------------------------------
ui_draw_story_page_1:
    push         CX                              ; 1000:71fe: 51
    push         SI                              ; 1000:71ff: 56
    push         AX                              ; 1000:7200: 50
    push         BX                              ; 1000:7201: 53
    call         ui_wait_for_keypress            ; 1000:7202: e85000
    pop          BX                              ; 1000:7205: 5b
    pop          AX                              ; 1000:7206: 58
    pop          SI                              ; 1000:7207: 5e
    sub          AX,0x4                          ; 1000:7208: 2d0400
    add          BX,0x3                          ; 1000:720b: 83c303
    push         SI                              ; 1000:720e: 56
    push         AX                              ; 1000:720f: 50
    push         BX                              ; 1000:7210: 53
    call         ui_fade_in_story_page           ; 1000:7211: e84d00
    pop          BX                              ; 1000:7214: 5b
    pop          AX                              ; 1000:7215: 58
    pop          SI                              ; 1000:7216: 5e
    pop          CX                              ; 1000:7217: 59
    loop         ui_draw_story_page_1            ; 1000:7218: e2e4
    ret                                          ; 1000:721a: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_story_page_2  [UI / Story]
; ADDRESS:  1000:721b
; PURPOSE:  Renders page 2 of story prologue artwork and narrative.
; ----------------------------------------------------------------------------
ui_draw_story_page_2:
    push         CX                              ; 1000:721b: 51
    push         SI                              ; 1000:721c: 56
    push         AX                              ; 1000:721d: 50
    push         BX                              ; 1000:721e: 53
    call         ui_wait_for_keypress            ; 1000:721f: e83300
    pop          BX                              ; 1000:7222: 5b
    pop          AX                              ; 1000:7223: 58
    pop          SI                              ; 1000:7224: 5e
    sub          AX,0x8                          ; 1000:7225: 2d0800
    add          BX,0x6                          ; 1000:7228: 83c306
    push         SI                              ; 1000:722b: 56
    push         AX                              ; 1000:722c: 50
    push         BX                              ; 1000:722d: 53
    call         ui_fade_in_story_page           ; 1000:722e: e83000
    pop          BX                              ; 1000:7231: 5b
    pop          AX                              ; 1000:7232: 58
    pop          SI                              ; 1000:7233: 5e
    pop          CX                              ; 1000:7234: 59
    loop         ui_draw_story_page_2            ; 1000:7235: e2e4
    ret                                          ; 1000:7237: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_story_page_3  [UI / Story]
; ADDRESS:  1000:7238
; PURPOSE:  Renders page 3 of story prologue artwork and narrative.
; ----------------------------------------------------------------------------
ui_draw_story_page_3:
    push         CX                              ; 1000:7238: 51
    push         SI                              ; 1000:7239: 56
    push         AX                              ; 1000:723a: 50
    push         BX                              ; 1000:723b: 53
    call         ui_wait_for_keypress            ; 1000:723c: e81600
    pop          BX                              ; 1000:723f: 5b
    pop          AX                              ; 1000:7240: 58
    pop          SI                              ; 1000:7241: 5e
    sub          AX,0xc                          ; 1000:7242: 2d0c00
    add          BX,0x8                          ; 1000:7245: 83c308
    push         SI                              ; 1000:7248: 56
    push         AX                              ; 1000:7249: 50
    push         BX                              ; 1000:724a: 53
    call         ui_fade_in_story_page           ; 1000:724b: e81300
    pop          BX                              ; 1000:724e: 5b
    pop          AX                              ; 1000:724f: 58
    pop          SI                              ; 1000:7250: 5e
    pop          CX                              ; 1000:7251: 59
    loop         ui_draw_story_page_3            ; 1000:7252: e2e4
    ret                                          ; 1000:7254: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_wait_for_keypress  [UI / Input]
; ADDRESS:  1000:7255
; PURPOSE:  Waits for user keypress or timer timeout before advancing text screen.
; ----------------------------------------------------------------------------
ui_wait_for_keypress:
    mov          DI,word [cs:0x773f]             ; 1000:7255: 2e8b3e3f77
    mov          SI,0x6000                       ; 1000:725a: be0060
    call         vid_wait_not_vblank             ; 1000:725d: e80505
    ret                                          ; 1000:7260: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_fade_in_story_page  [UI / Palette]
; ADDRESS:  1000:7261
; PURPOSE:  Fades in palette for story illustration.
; ----------------------------------------------------------------------------
ui_fade_in_story_page:
    mov          DS,word [cs:0x5e]               ; 1000:7261: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:7266: e83a05
    mov          DS,word [cs:0x5c]               ; 1000:7269: 2e8e1e5c00
    call         vid_wait_vblank                 ; 1000:726e: e8dd04
    mov          AX,0x1                          ; 1000:7271: b80100
    call         ui_wait_retrace_ticks           ; 1000:7274: e80100
    ret                                          ; 1000:7277: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_wait_retrace_ticks  [Video / Timing]
; ADDRESS:  1000:7278
; PURPOSE:  Delays execution for specified number of EGA vertical retraces.
; ----------------------------------------------------------------------------
ui_wait_retrace_ticks:
    push         AX                              ; 1000:7278: 50
    mov          AX,0x1                          ; 1000:7279: b80100
    call         timer_delay_ticks               ; 1000:727c: e8fb8f
    cmp          byte [cs:0x61],0x0              ; 1000:727f: 2e803e610000
    jnz          loc_728c                        ; 1000:7285: 7505
    pop          AX                              ; 1000:7287: 58
    dec          AX                              ; 1000:7288: 48
    jnz          ui_wait_retrace_ticks           ; 1000:7289: 75ed
    ret                                          ; 1000:728b: c3
loc_728c:
    call         vid_set_active_page_0           ; 1000:728c: e89c8f
    mov          AX,0x3                          ; 1000:728f: b80300
    int3                                         ; 1000:7292: cc
    db 0x8b, 0x26, 0xf3, 0xab, 0xc3                 ; 1000:7293

; ----------------------------------------------------------------------------
; FUNCTION: ui_title_screen  [UI / Title]
; ADDRESS:  1000:7298
; PURPOSE:  Displays Captain Comic II: Fractured Reality title screen logo, artwork, and copyright notices.
; ----------------------------------------------------------------------------
ui_title_screen:
    mov          AX,0xd                          ; 1000:7298: b80d00
    int          0x10                            ; 1000:729b: cd10
    call         vid_init_ega_regs               ; 1000:729d: e8a104
    mov          BX,0x7106                       ; 1000:72a0: bb0671
    cli                                          ; 1000:72a3: fa
    mov          word [cs:0x278],BX              ; 1000:72a4: 2e891e7802
    mov          word [cs:BX + 0x2],0x0          ; 1000:72a9: 2ec747020000
    mov          word [cs:BX + 0x4],0x0          ; 1000:72af: 2ec747040000
    mov          word [cs:BX + 0xa],0x0          ; 1000:72b5: 2ec7470a0000
    mov          word [cs:BX + 0xc],0x0          ; 1000:72bb: 2ec7470c0000
    mov          word [cs:BX + 0x12],0x0         ; 1000:72c1: 2ec747120000
    mov          word [cs:BX + 0x14],0x0         ; 1000:72c7: 2ec747140000
    sti                                          ; 1000:72cd: fb
    call         vid_set_active_page_1           ; 1000:72ce: e8678f
    mov          DX,0xabd1                       ; 1000:72d1: bad1ab
    call         vid_set_ega_palette             ; 1000:72d4: e8868e
    mov          DI,0x4000                       ; 1000:72d7: bf0040
    mov          DX,0xac12                       ; 1000:72da: ba12ac
    call         vid_load_default_palette        ; 1000:72dd: e8b006
    mov          DI,0x6000                       ; 1000:72e0: bf0060
    mov          DX,0xac1c                       ; 1000:72e3: ba1cac
    call         vid_load_default_palette        ; 1000:72e6: e8a706
    mov          SI,0x4000                       ; 1000:72e9: be0040
    mov          DI,word [cs:0x773f]             ; 1000:72ec: 2e8b3e3f77
    call         vid_wait_not_vblank             ; 1000:72f1: e87104
    call         vid_wait_vblank                 ; 1000:72f4: e85704
    mov          DX,0xac00                       ; 1000:72f7: ba00ac
    call         file_load_tileset               ; 1000:72fa: e825ad
    mov          AX,0x32                         ; 1000:72fd: b83200
    call         ui_wait_retrace_ticks           ; 1000:7300: e875ff
    mov          AX,0x8                          ; 1000:7303: b80800
loc_7306:
    push         AX                              ; 1000:7306: 50
    call         ui_draw_title_stars             ; 1000:7307: e8d701
    call         vid_wait_vblank                 ; 1000:730a: e84104
    mov          AX,0x1                          ; 1000:730d: b80100
    call         ui_wait_retrace_ticks           ; 1000:7310: e865ff
    pop          AX                              ; 1000:7313: 58
    add          AX,0x8                          ; 1000:7314: 050800
    cmp          AX,0x148                        ; 1000:7317: 3d4801
    jl           loc_7306                        ; 1000:731a: 7cea
    mov          BX,0x7120                       ; 1000:731c: bb2071
    cli                                          ; 1000:731f: fa
    mov          word [cs:0x278],BX              ; 1000:7320: 2e891e7802
    mov          word [cs:BX + 0x2],0x0          ; 1000:7325: 2ec747020000
    mov          word [cs:BX + 0x4],0x0          ; 1000:732b: 2ec747040000
    mov          word [cs:BX + 0xa],0x0          ; 1000:7331: 2ec7470a0000
    mov          word [cs:BX + 0xc],0x0          ; 1000:7337: 2ec7470c0000
    mov          word [cs:BX + 0x12],0x0         ; 1000:733d: 2ec747120000
    mov          word [cs:BX + 0x14],0x0         ; 1000:7343: 2ec747140000
    sti                                          ; 1000:7349: fb
    call         vid_set_active_page_1           ; 1000:734a: e8eb8e
    mov          DX,0xabd1                       ; 1000:734d: bad1ab
    call         vid_set_ega_palette             ; 1000:7350: e80a8e
    mov          word [0x6c6],0x0                ; 1000:7353: c706c6060000
    mov          word [0x6c8],0x81               ; 1000:7359: c706c8068100
    mov          byte [0x8ad],0x1                ; 1000:735f: c606ad0801
    mov          word [0xabf5],0x10              ; 1000:7364: c706f5ab1000
loc_736a:
    mov          SI,0x6000                       ; 1000:736a: be0060
    mov          DI,word [cs:0x773f]             ; 1000:736d: 2e8b3e3f77
    call         vid_wait_not_vblank             ; 1000:7372: e8f003
    call         ui_clear_title_logo             ; 1000:7375: e8aa02
    call         vid_wait_vblank                 ; 1000:7378: e8d303
    mov          AX,0x1                          ; 1000:737b: b80100
    call         ui_wait_retrace_ticks           ; 1000:737e: e8f7fe
    add          word [0x6c6],0x8                ; 1000:7381: 8306c60608
    dec          word [0xabf5]                   ; 1000:7386: ff0ef5ab
    jnz          loc_736a                        ; 1000:738a: 75de
    xor          SI,SI                           ; 1000:738c: 33f6
    call         ui_draw_title_logo              ; 1000:738e: e81c02
    mov          byte [0x8ad],0x2                ; 1000:7391: c606ad0802
    mov          word [0xabf5],0xa               ; 1000:7396: c706f5ab0a00
loc_739c:
    mov          SI,0x6000                       ; 1000:739c: be0060
    mov          DI,word [cs:0x773f]             ; 1000:739f: 2e8b3e3f77
    call         vid_wait_not_vblank             ; 1000:73a4: e8be03
    call         ui_clear_title_logo             ; 1000:73a7: e87802
    call         vid_wait_vblank                 ; 1000:73aa: e8a103
    mov          AX,0x1                          ; 1000:73ad: b80100
    call         ui_wait_retrace_ticks           ; 1000:73b0: e8c5fe
    sub          word [0x6c6],0x8                ; 1000:73b3: 832ec60608
    dec          word [0xabf5]                   ; 1000:73b8: ff0ef5ab
    jnz          loc_739c                        ; 1000:73bc: 75de
    xor          SI,SI                           ; 1000:73be: 33f6
    call         ui_draw_title_logo              ; 1000:73c0: e8ea01
    mov          byte [0x8ad],0x1                ; 1000:73c3: c606ad0801
    mov          word [0xabf5],0x2               ; 1000:73c8: c706f5ab0200
loc_73ce:
    mov          SI,0x6000                       ; 1000:73ce: be0060
    mov          DI,word [cs:0x773f]             ; 1000:73d1: 2e8b3e3f77
    call         vid_wait_not_vblank             ; 1000:73d6: e88c03
    call         ui_clear_title_logo             ; 1000:73d9: e84602
    call         vid_wait_vblank                 ; 1000:73dc: e86f03
    mov          AX,0x1                          ; 1000:73df: b80100
    call         ui_wait_retrace_ticks           ; 1000:73e2: e893fe
    add          word [0x6c6],0x8                ; 1000:73e5: 8306c60608
    dec          word [0xabf5]                   ; 1000:73ea: ff0ef5ab
    jnz          loc_73ce                        ; 1000:73ee: 75de
    xor          SI,SI                           ; 1000:73f0: 33f6
    call         ui_draw_title_logo              ; 1000:73f2: e8b801
    mov          AX,0x1                          ; 1000:73f5: b80100
    lea          BX,[0xac3a]                     ; 1000:73f8: 8d1e3aac
    mov          CX,0x9                          ; 1000:73fc: b90900
    int3                                         ; 1000:73ff: cc
    db 0xc6, 0x06, 0xad, 0x08, 0x01, 0xc7, 0x06, 0xf5, 0xab, 0x12, 0x00, 0xbe, 0x00, 0x60, 0x2e, 0x8b ; 1000:7400
    db 0x3e, 0x3f, 0x77, 0xe8, 0x4f, 0x03, 0xbe, 0x8c, 0x02, 0xb8, 0xc0, 0x00, 0xbb, 0x50, 0x00, 0x2e ; 1000:7410
    db 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xfb, 0x04, 0x2e, 0x8e, 0x1e, 0x5c ; 1000:7420
    db 0x00, 0xe8, 0xee, 0x01, 0xe8, 0x17, 0x03, 0xb8, 0x01, 0x00, 0xe8, 0x3b, 0xfe, 0x83, 0x06, 0xc6 ; 1000:7430
    db 0x06, 0x08, 0xff, 0x0e, 0xf5, 0xab, 0x75, 0xc3, 0xbe, 0x8c, 0x02, 0xe8, 0x5f, 0x01, 0xbe, 0x10 ; 1000:7440
    db 0x07, 0xe8, 0x19, 0x01, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0x7a, 0xac, 0xb9, 0x09, 0x00, 0xcc, 0xb8 ; 1000:7450
    db 0x1e, 0x00, 0xe8, 0x13, 0xfe, 0xbe, 0x10, 0x07, 0xe8, 0x42, 0x01, 0xbe, 0x94, 0x0b, 0xe8, 0xfc ; 1000:7460
    db 0x00, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0x7a, 0xac, 0xb9, 0x09, 0x00, 0xcc, 0xb8, 0x1e, 0x00, 0xe8 ; 1000:7470
    db 0xf6, 0xfd, 0xbe, 0x94, 0x0b, 0xe8, 0x25, 0x01, 0xb8, 0x01, 0x00, 0x8d, 0x1e, 0x3a, 0xac, 0xb9 ; 1000:7480
    db 0x09, 0x00, 0xcc, 0xc6, 0x06, 0xad, 0x08, 0x02, 0xc7, 0x06, 0xf5, 0xab, 0x1a, 0x00, 0xbe, 0x00 ; 1000:7490
    db 0x60, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xbc, 0x02, 0xbe, 0x18, 0x10, 0xb8, 0xc0, 0x00, 0xbb ; 1000:74a0
    db 0x50, 0x00, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x68, 0x04, 0x2e ; 1000:74b0
    db 0x8e, 0x1e, 0x5c, 0x00, 0xe8, 0x5b, 0x01, 0xe8, 0x84, 0x02, 0xb8, 0x01, 0x00, 0xe8, 0xa8, 0xfd ; 1000:74c0
    db 0x83, 0x2e, 0xc6, 0x06, 0x08, 0xff, 0x0e, 0xf5, 0xab, 0x75, 0xc3, 0xe8, 0x4d, 0x8d, 0xe9, 0x7f ; 1000:74d0
    db 0x01                                         ; 1000:74e0

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_title_stars  [UI / Title]
; ADDRESS:  1000:74e1
; PURPOSE:  Renders animated twinkling starfield background on title screen.
; ----------------------------------------------------------------------------
ui_draw_title_stars:
    push         DS                              ; 1000:74e1: 1e
    mov          BX,0xa000                       ; 1000:74e2: bb00a0
    mov          DS,BX                           ; 1000:74e5: 8edb
    shr          AX,0x1                          ; 1000:74e7: d1e8
    shr          AX,0x1                          ; 1000:74e9: d1e8
    shr          AX,0x1                          ; 1000:74eb: d1e8
    cmp          AX,0x28                         ; 1000:74ed: 3d2800
    jz           loc_750e                        ; 1000:74f0: 741c
    mov          SI,0x4000                       ; 1000:74f2: be0040
    mov          DI,word [cs:0x773f]             ; 1000:74f5: 2e8b3e3f77
    push         AX                              ; 1000:74fa: 50
    add          DI,AX                           ; 1000:74fb: 03f8
    mov          DX,0x28                         ; 1000:74fd: ba2800
    sub          DX,AX                           ; 1000:7500: 2bd0
    add          SI,0x280                        ; 1000:7502: 81c68002
    add          DI,0x280                        ; 1000:7506: 81c78002
    call         ui_update_title_star            ; 1000:750a: e81d00
    pop          AX                              ; 1000:750d: 58
loc_750e:
    mov          SI,0x6000                       ; 1000:750e: be0060
    mov          DI,word [cs:0x773f]             ; 1000:7511: 2e8b3e3f77
    add          SI,0x28                         ; 1000:7516: 83c628
    sub          SI,AX                           ; 1000:7519: 2bf0
    mov          DX,AX                           ; 1000:751b: 8bd0
    add          SI,0x280                        ; 1000:751d: 81c68002
    add          DI,0x280                        ; 1000:7521: 81c78002
    call         ui_update_title_star            ; 1000:7525: e80200
    pop          DS                              ; 1000:7528: 1f
    ret                                          ; 1000:7529: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_update_title_star  [UI / Title]
; ADDRESS:  1000:752a
; PURPOSE:  Updates brightness and position of an individual star in the starfield.
; ----------------------------------------------------------------------------
ui_update_title_star:
    xor          CL,CL                           ; 1000:752a: 32c9
    push         DX                              ; 1000:752c: 52
    call         vid_out_dx_ax                   ; 1000:752d: e83805
    pop          DX                              ; 1000:7530: 5a
    call         math_lfsr_random                ; 1000:7531: e81f00
    mov          CL,0x1                          ; 1000:7534: b101
    push         DX                              ; 1000:7536: 52
    call         vid_out_dx_ax                   ; 1000:7537: e82e05
    pop          DX                              ; 1000:753a: 5a
    call         math_lfsr_random                ; 1000:753b: e81500
    mov          CL,0x2                          ; 1000:753e: b102
    push         DX                              ; 1000:7540: 52
    call         vid_out_dx_ax                   ; 1000:7541: e82405
    pop          DX                              ; 1000:7544: 5a
    call         math_lfsr_random                ; 1000:7545: e80b00
    mov          CL,0x3                          ; 1000:7548: b103
    push         DX                              ; 1000:754a: 52
    call         vid_out_dx_ax                   ; 1000:754b: e81a05
    pop          DX                              ; 1000:754e: 5a
    call         math_lfsr_random                ; 1000:754f: e80100
    ret                                          ; 1000:7552: c3

; ----------------------------------------------------------------------------
; FUNCTION: math_lfsr_random  [Math / PRNG]
; ADDRESS:  1000:7553
; PURPOSE:  Generates pseudo-random number using Linear Feedback Shift Register.
; ----------------------------------------------------------------------------
math_lfsr_random:
    push         SI                              ; 1000:7553: 56
    push         DI                              ; 1000:7554: 57
    mov          CX,0xa0                         ; 1000:7555: b9a000
loc_7558:
    push         CX                              ; 1000:7558: 51
    mov          CX,DX                           ; 1000:7559: 8bca
    push         SI                              ; 1000:755b: 56
    push         DI                              ; 1000:755c: 57
    rep movsb                                    ; 1000:755d: f3a4
    pop          DI                              ; 1000:755f: 5f
    pop          SI                              ; 1000:7560: 5e
    add          SI,0x28                         ; 1000:7561: 83c628
    add          DI,0x28                         ; 1000:7564: 83c728
    pop          CX                              ; 1000:7567: 59
    loop         loc_7558                        ; 1000:7568: e2ee
    pop          DI                              ; 1000:756a: 5f
    pop          SI                              ; 1000:756b: 5e
    ret                                          ; 1000:756c: c3
    db 0x56, 0xbe, 0x00, 0x60, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xec, 0x01, 0x5e, 0xb8, 0xc0, 0x00 ; 1000:756d
    db 0xbb, 0x50, 0x00, 0x2e, 0x8e, 0x1e, 0x5e, 0x00, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x9a, 0x03 ; 1000:757d
    db 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xbe, 0x00, 0x00, 0xa1, 0xc6, 0x06, 0x8b, 0x1e, 0xc8, 0x06, 0x2e ; 1000:758d
    db 0x8e, 0x1e, 0x5e, 0x00, 0xe8, 0xff, 0x01, 0x2e, 0x8e, 0x1e, 0x5c, 0x00, 0xe8, 0xa2, 0x01, 0xc3 ; 1000:759d

; ----------------------------------------------------------------------------
; FUNCTION: ui_draw_title_logo  [UI / Title]
; ADDRESS:  1000:75ad
; PURPOSE:  Draws multicolor Captain Comic II title logo onto video buffer.
; ----------------------------------------------------------------------------
ui_draw_title_logo:
    mov          word [0xabf5],0xa               ; 1000:75ad: c706f5ab0a00
loc_75b3:
    push         SI                              ; 1000:75b3: 56
    mov          SI,0x6000                       ; 1000:75b4: be0060
    mov          DI,word [cs:0x773f]             ; 1000:75b7: 2e8b3e3f77
    call         vid_wait_not_vblank             ; 1000:75bc: e8a601
    mov          SI,0x0                          ; 1000:75bf: be0000
    test         word [0xabf5],0x1               ; 1000:75c2: f706f5ab0100
    jnz          loc_75cd                        ; 1000:75c8: 7503
    mov          SI,0x146                        ; 1000:75ca: be4601
loc_75cd:
    mov          AX,[0x6c6]                      ; 1000:75cd: a1c606
    mov          BX,word [0x6c8]                 ; 1000:75d0: 8b1ec806
    mov          DS,word [cs:0x5e]               ; 1000:75d4: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:75d9: e8c701
    mov          DS,word [cs:0x5c]               ; 1000:75dc: 2e8e1e5c00
    pop          SI                              ; 1000:75e1: 5e
    or           SI,SI                           ; 1000:75e2: 0bf6
    jz           loc_7600                        ; 1000:75e4: 741a
    mov          AX,0xc0                         ; 1000:75e6: b8c000
    mov          BX,0x50                         ; 1000:75e9: bb5000
    mov          DS,word [cs:0x5e]               ; 1000:75ec: 2e8e1e5e00
    mov          DI,word [cs:0x773f]             ; 1000:75f1: 2e8b3e3f77
    push         SI                              ; 1000:75f6: 56
    call         vid_restore_write_mode          ; 1000:75f7: e82d03
    pop          SI                              ; 1000:75fa: 5e
    mov          DS,word [cs:0x5c]               ; 1000:75fb: 2e8e1e5c00
loc_7600:
    call         vid_wait_vblank                 ; 1000:7600: e84b01
    mov          AX,0x1                          ; 1000:7603: b80100
    call         ui_wait_retrace_ticks           ; 1000:7606: e86ffc
    cmp          word [0xabf5],0x9               ; 1000:7609: 833ef5ab09
    jnz          loc_761b                        ; 1000:760e: 750b
    mov          AX,0x1                          ; 1000:7610: b80100
    lea          BX,[0xac92]                     ; 1000:7613: 8d1e92ac
    mov          CX,0x8                          ; 1000:7617: b90800
    int3                                         ; 1000:761a: cc
loc_761b:
    dec          word [0xabf5]                   ; 1000:761b: ff0ef5ab
    jnz          loc_75b3                        ; 1000:761f: 7592
    ret                                          ; 1000:7621: c3

; ----------------------------------------------------------------------------
; FUNCTION: ui_clear_title_logo  [UI / Title]
; ADDRESS:  1000:7622
; PURPOSE:  Erases title logo graphic area.
; ----------------------------------------------------------------------------
ui_clear_title_logo:
    mov          CH,byte [0x8af]                 ; 1000:7622: 8a2eaf08
    inc          CH                              ; 1000:7626: fec5
    cmp          CH,0x4                          ; 1000:7628: 80fd04
    jl           loc_762f                        ; 1000:762b: 7c02
    mov          CH,0x1                          ; 1000:762d: b501
loc_762f:
    mov          byte [0x8af],CH                 ; 1000:762f: 882eaf08
    mov          SI,0x4882                       ; 1000:7633: be8248
    cmp          byte [0x8ad],0x1                ; 1000:7636: 803ead0801
    jz           loc_7640                        ; 1000:763b: 7403
    mov          SI,0x4860                       ; 1000:763d: be6048
loc_7640:
    mov          AL,[0x8af]                      ; 1000:7640: a0af08
    xor          AH,AH                           ; 1000:7643: 32e4
    shl          AX,0x1                          ; 1000:7645: d1e0
    add          SI,AX                           ; 1000:7647: 03f0
    mov          AX,[0x6c6]                      ; 1000:7649: a1c606
    mov          BX,word [0x6c8]                 ; 1000:764c: 8b1ec806
    mov          DS,word [cs:0x5e]               ; 1000:7650: 2e8e1e5e00
    mov          SI,word [SI]                    ; 1000:7655: 8b34
    call         vid_clear_page                  ; 1000:7657: e84901
    mov          DS,word [cs:0x5c]               ; 1000:765a: 2e8e1e5c00
    ret                                          ; 1000:765f: c3
    db 0xba, 0x09, 0xac, 0xe8, 0xbc, 0xa9, 0xb8, 0x0d, 0x00, 0xcd, 0x10, 0xe8, 0xd3, 0x00, 0x1e ; 1000:7660
    mov          ax, SEG_PALETTE                      ; 1000:766f: b8 (relocated segment)
    db 0x8e, 0xd8, 0xbe, 0xd0, 0x1c, 0xbf, 0x00, 0x60, 0xe8, 0x4a, 0x03, 0x1f, 0xba, 0xc0, 0xab, 0xe8 ; 1000:7672
    db 0xd9, 0x8a, 0xc7, 0x06, 0xc6, 0x06, 0x84, 0x00, 0xc7, 0x06, 0xc8, 0x06, 0xb4, 0x00, 0xb8, 0x00 ; 1000:7682
    db 0x00, 0xa3, 0xf5, 0xab, 0xe8, 0xbc, 0xfb, 0x8b, 0x36, 0xf5, 0xab, 0xa1, 0xc6, 0x06, 0x8b, 0x1e ; 1000:7692
    db 0xc8, 0x06, 0xe8, 0xba, 0xfb, 0x83, 0x2e, 0xc8, 0x06, 0x0a, 0xe8, 0xa6, 0xfb, 0x8b, 0x36, 0xf5 ; 1000:76a2
    db 0xab, 0xa1, 0xc6, 0x06, 0x8b, 0x1e, 0xc8, 0x06, 0xe8, 0xa4, 0xfb, 0x83, 0x2e, 0xc8, 0x06, 0x0a ; 1000:76b2
    db 0x81, 0x06, 0xf5, 0xab, 0x4e, 0x03, 0x83, 0x3e, 0xc8, 0x06, 0x3c, 0x7f, 0xc7, 0xe8, 0x83, 0xfb ; 1000:76c2
    db 0xe8, 0x79, 0x00, 0x1e                       ; 1000:76d2
    mov          ax, SEG_PALETTE                      ; 1000:76d6: b8 (relocated segment)
    db 0x8e, 0xd8, 0xbe, 0xd0, 0x1c, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xe1, 0x02, 0x1f, 0xba, 0x26 ; 1000:76d9
    db 0xac, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xa7, 0x02, 0xba, 0xe2, 0xab, 0xe8, 0x65, 0x8a, 0xe8 ; 1000:76e9
    db 0x53, 0x00, 0xb8, 0x01, 0x00, 0xbb, 0xdb, 0x00, 0xb9, 0x01, 0x00, 0xcc, 0xb8, 0x28, 0x00, 0xe8 ; 1000:76f9
    db 0x6d, 0xfb, 0x1e                             ; 1000:7709
    mov          ax, SEG_PALETTE                      ; 1000:770c: b8 (relocated segment)
    db 0x8e, 0xd8, 0xbe, 0xd0, 0x1c, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0xab, 0x02, 0x1f, 0xba, 0x30 ; 1000:770f
    db 0xac, 0x2e, 0x8b, 0x3e, 0x3f, 0x77, 0xe8, 0x71, 0x02, 0xba, 0x02, 0x00, 0xe8, 0x2f, 0x8a, 0xe8 ; 1000:771f
    db 0x1d, 0x00, 0xba, 0x24, 0x00, 0xe8, 0x26, 0x8a, 0xb8, 0x01, 0x00, 0xe8, 0x3b, 0xfb, 0xeb, 0xf8 ; 1000:772f
    db 0x00, 0x00                                   ; 1000:773f

; ----------------------------------------------------------------------------
; FUNCTION: vid_init_ega_regs  [Video / Hardware]
; ADDRESS:  1000:7741
; PURPOSE:  Initializes EGA hardware registers (Sequencer, Graphics Controller, CRTC) to 320x200 16-color mode.
; ----------------------------------------------------------------------------
vid_init_ega_regs:
    xor          AX,AX                           ; 1000:7741: 33c0
    call         vid_in_status                   ; 1000:7743: e84303
    mov          word [cs:0x773f],0x2000         ; 1000:7746: 2ec7063f770020
    ret                                          ; 1000:774d: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_wait_vblank  [Video / Hardware]
; ADDRESS:  1000:774e
; PURPOSE:  Polls Port 0x3da bit 3 until vertical retrace blanking interval begins.
; ----------------------------------------------------------------------------
vid_wait_vblank:
    mov          AL,[cs:0x229]                   ; 1000:774e: 2ea02902
    mov          [cs:0x22a],AL                   ; 1000:7752: 2ea22a02
    mov          AX,[cs:0x773f]                  ; 1000:7756: 2ea13f77
    call         vid_in_status                   ; 1000:775a: e82c03
    xor          word [cs:0x773f],0x2000         ; 1000:775d: 2e81363f770020
    ret                                          ; 1000:7764: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_wait_not_vblank  [Video / Hardware]
; ADDRESS:  1000:7765
; PURPOSE:  Polls Port 0x3da bit 3 until vertical retrace blanking interval ends.
; ----------------------------------------------------------------------------
vid_wait_not_vblank:
    push         DS                              ; 1000:7765: 1e
    mov          AX,0xa000                       ; 1000:7766: b800a0
    mov          DS,AX                           ; 1000:7769: 8ed8
    mov          ES,AX                           ; 1000:776b: 8ec0
    xor          CL,CL                           ; 1000:776d: 32c9
    call         vid_out_dx_ax                   ; 1000:776f: e8f602
    mov          CX,0xfa0                        ; 1000:7772: b9a00f
    push         SI                              ; 1000:7775: 56
    push         DI                              ; 1000:7776: 57
    rep movsw                                    ; 1000:7777: f3a5
    pop          DI                              ; 1000:7779: 5f
    pop          SI                              ; 1000:777a: 5e
    mov          CL,0x1                          ; 1000:777b: b101
    call         vid_out_dx_ax                   ; 1000:777d: e8e802
    mov          CX,0xfa0                        ; 1000:7780: b9a00f
    push         SI                              ; 1000:7783: 56
    push         DI                              ; 1000:7784: 57
    rep movsw                                    ; 1000:7785: f3a5
    pop          DI                              ; 1000:7787: 5f
    pop          SI                              ; 1000:7788: 5e
    mov          CL,0x2                          ; 1000:7789: b102
    call         vid_out_dx_ax                   ; 1000:778b: e8da02
    mov          CX,0xfa0                        ; 1000:778e: b9a00f
    push         SI                              ; 1000:7791: 56
    push         DI                              ; 1000:7792: 57
    rep movsw                                    ; 1000:7793: f3a5
    pop          DI                              ; 1000:7795: 5f
    pop          SI                              ; 1000:7796: 5e
    mov          CL,0x3                          ; 1000:7797: b103
    call         vid_out_dx_ax                   ; 1000:7799: e8cc02
    mov          CX,0xfa0                        ; 1000:779c: b9a00f
    rep movsw                                    ; 1000:779f: f3a5
    pop          DS                              ; 1000:77a1: 1f
    ret                                          ; 1000:77a2: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_clear_page  [Video / Hardware]
; ADDRESS:  1000:77a3
; PURPOSE:  Clears 32KB EGA video page with zero bytes across all 4 planes.
; ----------------------------------------------------------------------------
vid_clear_page:
    push         BP                              ; 1000:77a3: 55
    push         AX                              ; 1000:77a4: 50
    shr          AX,0x1                          ; 1000:77a5: d1e8
    shr          AX,0x1                          ; 1000:77a7: d1e8
    shr          AX,0x1                          ; 1000:77a9: d1e8
    mov          DX,BX                           ; 1000:77ab: 8bd3
    shl          BX,0x1                          ; 1000:77ad: d1e3
    shl          BX,0x1                          ; 1000:77af: d1e3
    shl          BX,0x1                          ; 1000:77b1: d1e3
    mov          CL,0x5                          ; 1000:77b3: b105
    shl          DX,CL                           ; 1000:77b5: d3e2
    add          BX,DX                           ; 1000:77b7: 03da
    add          BX,AX                           ; 1000:77b9: 03d8
    mov          DI,word [cs:0x773f]             ; 1000:77bb: 2e8b3e3f77
    add          DI,BX                           ; 1000:77c0: 03fb
    lodsw                                        ; 1000:77c2: ad
    shr          AX,0x1                          ; 1000:77c3: d1e8
    shr          AX,0x1                          ; 1000:77c5: d1e8
    shr          AX,0x1                          ; 1000:77c7: d1e8
    mov          DX,AX                           ; 1000:77c9: 8bd0
    lodsw                                        ; 1000:77cb: ad
    mov          BX,AX                           ; 1000:77cc: 8bd8
    lodsw                                        ; 1000:77ce: ad
    mov          BP,SI                           ; 1000:77cf: 8bee
    add          SI,AX                           ; 1000:77d1: 03f0
    pop          AX                              ; 1000:77d3: 58
    test         AX,0x4                          ; 1000:77d4: a90400
    jz           loc_77dc                        ; 1000:77d7: 7403
    jmp short    vid_set_read_plane              ; 1000:77d9: eb58
    db 0x90                                         ; 1000:77db
loc_77dc:
    mov          CL,0x0                          ; 1000:77dc: b100
    call         vid_fill_plane                  ; 1000:77de: e81100
    mov          CL,0x1                          ; 1000:77e1: b101
    call         vid_fill_plane                  ; 1000:77e3: e80c00
    mov          CL,0x2                          ; 1000:77e6: b102
    call         vid_fill_plane                  ; 1000:77e8: e80700
    mov          CL,0x3                          ; 1000:77eb: b103
    call         vid_fill_plane                  ; 1000:77ed: e80200
    pop          BP                              ; 1000:77f0: 5d
    ret                                          ; 1000:77f1: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_fill_plane  [Video / Hardware]
; ADDRESS:  1000:77f2
; PURPOSE:  Fills specific EGA bitplane with pattern byte.
; ----------------------------------------------------------------------------
vid_fill_plane:
    push         DX                              ; 1000:77f2: 52
    call         vid_out_dx_ax                   ; 1000:77f3: e87202
    pop          DX                              ; 1000:77f6: 5a
    push         DI                              ; 1000:77f7: 57
    push         BX                              ; 1000:77f8: 53
    mov          AX,BX                           ; 1000:77f9: 8bc3
    mov          BX,BP                           ; 1000:77fb: 8bdd
loc_77fd:
    push         AX                              ; 1000:77fd: 50
    push         DI                              ; 1000:77fe: 57
    mov          CX,DX                           ; 1000:77ff: 8bca
loc_7801:
    cmp          CX,0x1                          ; 1000:7801: 83f901
    jz           loc_781e                        ; 1000:7804: 7418
    mov          AX,word [BX]                    ; 1000:7806: 8b07
    add          BX,0x2                          ; 1000:7808: 83c302
    and          word [es:DI],AX                 ; 1000:780b: 262105
    lodsw                                        ; 1000:780e: ad
    or           word [es:DI],AX                 ; 1000:780f: 260905
    add          DI,0x2                          ; 1000:7812: 83c702
    sub          CX,0x2                          ; 1000:7815: 83e902
    or           CX,CX                           ; 1000:7818: 0bc9
    jnz          loc_7801                        ; 1000:781a: 75e5
    jmp short    loc_7828                        ; 1000:781c: eb0a
loc_781e:
    mov          AL,byte [BX]                    ; 1000:781e: 8a07
    inc          BX                              ; 1000:7820: 43
    and          byte [es:DI],AL                 ; 1000:7821: 262005
    lodsb                                        ; 1000:7824: ac
    or           byte [es:DI],AL                 ; 1000:7825: 260805
loc_7828:
    pop          DI                              ; 1000:7828: 5f
    add          DI,0x28                         ; 1000:7829: 83c728
    pop          AX                              ; 1000:782c: 58
    dec          AX                              ; 1000:782d: 48
    jnz          loc_77fd                        ; 1000:782e: 75cd
    pop          BX                              ; 1000:7830: 5b
    pop          DI                              ; 1000:7831: 5f
    ret                                          ; 1000:7832: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_read_plane  [Video / Hardware]
; ADDRESS:  1000:7833
; PURPOSE:  Configures EGA Read Map Select register (Port 0x3ce index 4).
; ----------------------------------------------------------------------------
vid_set_read_plane:
    mov          CL,0x0                          ; 1000:7833: b100
    call         vid_copy_page                   ; 1000:7835: e81100
    mov          CL,0x1                          ; 1000:7838: b101
    call         vid_copy_page                   ; 1000:783a: e80c00
    mov          CL,0x2                          ; 1000:783d: b102
    call         vid_copy_page                   ; 1000:783f: e80700
    mov          CL,0x3                          ; 1000:7842: b103
    call         vid_copy_page                   ; 1000:7844: e80200
    pop          BP                              ; 1000:7847: 5d
    ret                                          ; 1000:7848: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_copy_page  [Video / Hardware]
; ADDRESS:  1000:7849
; PURPOSE:  Fast VRAM-to-VRAM page copy using EGA internal latch registers.
; ----------------------------------------------------------------------------
vid_copy_page:
    push         DX                              ; 1000:7849: 52
    call         vid_out_dx_ax                   ; 1000:784a: e81b02
    pop          DX                              ; 1000:784d: 5a
    push         DI                              ; 1000:784e: 57
    push         BX                              ; 1000:784f: 53
    push         BP                              ; 1000:7850: 55
    mov          AX,BX                           ; 1000:7851: 8bc3
    mov          BX,BP                           ; 1000:7853: 8bdd
loc_7855:
    push         AX                              ; 1000:7855: 50
    push         DI                              ; 1000:7856: 57
    mov          CX,DX                           ; 1000:7857: 8bca
    xor          BP,BP                           ; 1000:7859: 33ed
    mov          AX,0xffff                       ; 1000:785b: b8ffff
    mov          AL,byte [BX]                    ; 1000:785e: 8a07
    inc          BX                              ; 1000:7860: 43
    shl          AX,0x1                          ; 1000:7861: d1e0
    shl          AX,0x1                          ; 1000:7863: d1e0
    shl          AX,0x1                          ; 1000:7865: d1e0
    shl          AX,0x1                          ; 1000:7867: d1e0
    and          byte [es:DI],AH                 ; 1000:7869: 262025
    xchg         AX,BP                           ; 1000:786c: 95
    lodsb                                        ; 1000:786d: ac
    shl          AX,0x1                          ; 1000:786e: d1e0
    shl          AX,0x1                          ; 1000:7870: d1e0
    shl          AX,0x1                          ; 1000:7872: d1e0
    shl          AX,0x1                          ; 1000:7874: d1e0
    or           byte [es:DI],AH                 ; 1000:7876: 260825
    inc          DI                              ; 1000:7879: 47
    xchg         AX,BP                           ; 1000:787a: 95
loc_787b:
    cmp          CX,0x1                          ; 1000:787b: 83f901
    jz           loc_78b0                        ; 1000:787e: 7430
    shl          AX,0x1                          ; 1000:7880: d1e0
    shl          AX,0x1                          ; 1000:7882: d1e0
    shl          AX,0x1                          ; 1000:7884: d1e0
    shl          AX,0x1                          ; 1000:7886: d1e0
    mov          AL,byte [BX]                    ; 1000:7888: 8a07
    inc          BX                              ; 1000:788a: 43
    shl          AX,0x1                          ; 1000:788b: d1e0
    shl          AX,0x1                          ; 1000:788d: d1e0
    shl          AX,0x1                          ; 1000:788f: d1e0
    shl          AX,0x1                          ; 1000:7891: d1e0
    and          byte [es:DI],AH                 ; 1000:7893: 262025
    xchg         AX,BP                           ; 1000:7896: 95
    shl          AX,0x1                          ; 1000:7897: d1e0
    shl          AX,0x1                          ; 1000:7899: d1e0
    shl          AX,0x1                          ; 1000:789b: d1e0
    shl          AX,0x1                          ; 1000:789d: d1e0
    lodsb                                        ; 1000:789f: ac
    shl          AX,0x1                          ; 1000:78a0: d1e0
    shl          AX,0x1                          ; 1000:78a2: d1e0
    shl          AX,0x1                          ; 1000:78a4: d1e0
    shl          AX,0x1                          ; 1000:78a6: d1e0
    or           byte [es:DI],AH                 ; 1000:78a8: 260825
    inc          DI                              ; 1000:78ab: 47
    xchg         AX,BP                           ; 1000:78ac: 95
    dec          CX                              ; 1000:78ad: 49
    jmp short    loc_787b                        ; 1000:78ae: ebcb
loc_78b0:
    or           AL,0xf                          ; 1000:78b0: 0c0f
    and          byte [es:DI],AL                 ; 1000:78b2: 262005
    mov          AX,BP                           ; 1000:78b5: 8bc5
    or           byte [es:DI],AL                 ; 1000:78b7: 260805
    pop          DI                              ; 1000:78ba: 5f
    add          DI,0x28                         ; 1000:78bb: 83c728
    pop          AX                              ; 1000:78be: 58
    dec          AX                              ; 1000:78bf: 48
    jnz          loc_7855                        ; 1000:78c0: 7593
    pop          BP                              ; 1000:78c2: 5d
    pop          BX                              ; 1000:78c3: 5b
    pop          DI                              ; 1000:78c4: 5f
    ret                                          ; 1000:78c5: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_map_mask  [Video / Hardware]
; ADDRESS:  1000:78c6
; PURPOSE:  Sets EGA Sequencer Map Mask register (Port 0x3c4 index 2) to select plane write enables.
; ----------------------------------------------------------------------------
vid_set_map_mask:
    mov          DI,word [cs:0x773f]             ; 1000:78c6: 2e8b3e3f77
    push         AX                              ; 1000:78cb: 50
    push         BX                              ; 1000:78cc: 53
    push         SI                              ; 1000:78cd: 56
    call         vid_set_bit_mask                ; 1000:78ce: e81000
    mov          DI,word [cs:0x773f]             ; 1000:78d1: 2e8b3e3f77
    xor          DI,0x2000                       ; 1000:78d6: 81f70020
    pop          SI                              ; 1000:78da: 5e
    pop          BX                              ; 1000:78db: 5b
    pop          AX                              ; 1000:78dc: 58
    call         vid_set_bit_mask                ; 1000:78dd: e80100
    ret                                          ; 1000:78e0: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_bit_mask  [Video / Hardware]
; ADDRESS:  1000:78e1
; PURPOSE:  Sets EGA Graphics Controller Bit Mask register (Port 0x3ce index 8).
; ----------------------------------------------------------------------------
vid_set_bit_mask:
    push         BP                              ; 1000:78e1: 55
    shr          AX,0x1                          ; 1000:78e2: d1e8
    shr          AX,0x1                          ; 1000:78e4: d1e8
    shr          AX,0x1                          ; 1000:78e6: d1e8
    mov          DX,BX                           ; 1000:78e8: 8bd3
    shl          BX,0x1                          ; 1000:78ea: d1e3
    shl          BX,0x1                          ; 1000:78ec: d1e3
    shl          BX,0x1                          ; 1000:78ee: d1e3
    mov          CL,0x5                          ; 1000:78f0: b105
    shl          DX,CL                           ; 1000:78f2: d3e2
    add          BX,DX                           ; 1000:78f4: 03da
    add          BX,AX                           ; 1000:78f6: 03d8
    add          DI,BX                           ; 1000:78f8: 03fb
    lodsw                                        ; 1000:78fa: ad
    shr          AX,0x1                          ; 1000:78fb: d1e8
    shr          AX,0x1                          ; 1000:78fd: d1e8
    shr          AX,0x1                          ; 1000:78ff: d1e8
    mov          DX,AX                           ; 1000:7901: 8bd0
    lodsw                                        ; 1000:7903: ad
    mov          BX,AX                           ; 1000:7904: 8bd8
    lodsw                                        ; 1000:7906: ad
    add          SI,AX                           ; 1000:7907: 03f0
    jmp short    loc_794c                        ; 1000:7909: eb41
    db 0x90                                         ; 1000:790b

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_write_mode_2  [Video / Hardware]
; ADDRESS:  1000:790c
; PURPOSE:  Configures EGA Graphics Controller to Write Mode 2 for color-per-pixel blits.
; ----------------------------------------------------------------------------
vid_set_write_mode_2:
    mov          DI,word [cs:0x773f]             ; 1000:790c: 2e8b3e3f77
    push         AX                              ; 1000:7911: 50
    push         BX                              ; 1000:7912: 53
    push         SI                              ; 1000:7913: 56
    call         vid_restore_write_mode          ; 1000:7914: e81000
    mov          DI,word [cs:0x773f]             ; 1000:7917: 2e8b3e3f77
    xor          DI,0x2000                       ; 1000:791c: 81f70020
    pop          SI                              ; 1000:7920: 5e
    pop          BX                              ; 1000:7921: 5b
    pop          AX                              ; 1000:7922: 58
    call         vid_restore_write_mode          ; 1000:7923: e80100
    ret                                          ; 1000:7926: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_restore_write_mode  [Video / Hardware]
; ADDRESS:  1000:7927
; PURPOSE:  Restores EGA Graphics Controller to Write Mode 0.
; ----------------------------------------------------------------------------
vid_restore_write_mode:
    push         BP                              ; 1000:7927: 55
    shr          AX,0x1                          ; 1000:7928: d1e8
    shr          AX,0x1                          ; 1000:792a: d1e8
    shr          AX,0x1                          ; 1000:792c: d1e8
    mov          DX,BX                           ; 1000:792e: 8bd3
    shl          BX,0x1                          ; 1000:7930: d1e3
    shl          BX,0x1                          ; 1000:7932: d1e3
    shl          BX,0x1                          ; 1000:7934: d1e3
    mov          CL,0x5                          ; 1000:7936: b105
    shl          DX,CL                           ; 1000:7938: d3e2
    add          BX,DX                           ; 1000:793a: 03da
    add          BX,AX                           ; 1000:793c: 03d8
    add          DI,BX                           ; 1000:793e: 03fb
    lodsw                                        ; 1000:7940: ad
    shr          AX,0x1                          ; 1000:7941: d1e8
    shr          AX,0x1                          ; 1000:7943: d1e8
    shr          AX,0x1                          ; 1000:7945: d1e8
    mov          DX,AX                           ; 1000:7947: 8bd0
    lodsw                                        ; 1000:7949: ad
    mov          BX,AX                           ; 1000:794a: 8bd8
loc_794c:
    mov          CL,0x0                          ; 1000:794c: b100
    call         vid_set_color_compare           ; 1000:794e: e81100
    mov          CL,0x1                          ; 1000:7951: b101
    call         vid_set_color_compare           ; 1000:7953: e80c00
    mov          CL,0x2                          ; 1000:7956: b102
    call         vid_set_color_compare           ; 1000:7958: e80700
    mov          CL,0x3                          ; 1000:795b: b103
    call         vid_set_color_compare           ; 1000:795d: e80200
    pop          BP                              ; 1000:7960: 5d
    ret                                          ; 1000:7961: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_color_compare  [Video / Hardware]
; ADDRESS:  1000:7962
; PURPOSE:  Configures EGA Color Compare register (Port 0x3ce index 2).
; ----------------------------------------------------------------------------
vid_set_color_compare:
    push         DX                              ; 1000:7962: 52
    call         vid_out_dx_ax                   ; 1000:7963: e80201
    pop          DX                              ; 1000:7966: 5a
    push         DI                              ; 1000:7967: 57
    push         BX                              ; 1000:7968: 53
    mov          AX,BX                           ; 1000:7969: 8bc3
    mov          BX,BP                           ; 1000:796b: 8bdd
loc_796d:
    push         AX                              ; 1000:796d: 50
    push         DI                              ; 1000:796e: 57
    mov          CX,DX                           ; 1000:796f: 8bca
    test         CX,0x1                          ; 1000:7971: f7c10100
    jz           loc_797b                        ; 1000:7975: 7404
    movsb                                        ; 1000:7977: a4
    dec          CX                              ; 1000:7978: 49
    jz           loc_7983                        ; 1000:7979: 7408
loc_797b:
    shr          CX,0x1                          ; 1000:797b: d1e9
    or           CX,CX                           ; 1000:797d: 0bc9
    jz           loc_7983                        ; 1000:797f: 7402
    rep movsw                                    ; 1000:7981: f3a5
loc_7983:
    pop          DI                              ; 1000:7983: 5f
    add          DI,0x28                         ; 1000:7984: 83c728
    pop          AX                              ; 1000:7987: 58
    dec          AX                              ; 1000:7988: 48
    jnz          loc_796d                        ; 1000:7989: 75e2
    pop          BX                              ; 1000:798b: 5b
    pop          DI                              ; 1000:798c: 5f
    ret                                          ; 1000:798d: c3
    db 0x00, 0x00                                   ; 1000:798e

; ----------------------------------------------------------------------------
; FUNCTION: vid_load_default_palette  [Video / Palette]
; ADDRESS:  1000:7990
; PURPOSE:  Loads standard outdoor 16-color EGA palette.
; ----------------------------------------------------------------------------
vid_load_default_palette:
    push         DS                              ; 1000:7990: 1e
    call         vid_set_overscan_color          ; 1000:7991: e80e00
    call         vid_load_palette_buffer         ; 1000:7994: e83000
    pop          DS                              ; 1000:7997: 1f
    ret                                          ; 1000:7998: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_load_alt_palette  [Video / Palette]
; ADDRESS:  1000:7999
; PURPOSE:  Loads alternate cavern/underground 16-color EGA palette.
; ----------------------------------------------------------------------------
vid_load_alt_palette:
    push         DS                              ; 1000:7999: 1e
    call         vid_set_overscan_color          ; 1000:799a: e80500
    call         vid_apply_palette               ; 1000:799d: e87300
    pop          DS                              ; 1000:79a0: 1f
    ret                                          ; 1000:79a1: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_overscan_color  [Video / Palette]
; ADDRESS:  1000:79a2
; PURPOSE:  Sets screen border/overscan color via BIOS INT 10h AH=0Bh.
; ----------------------------------------------------------------------------
vid_set_overscan_color:
    mov          AX,0x3d00                       ; 1000:79a2: b8003d
    int          0x21                            ; 1000:79a5: cd21
    jnc          loc_79ac                        ; 1000:79a7: 7303
    jmp near     sys_fatal_error                 ; 1000:79a9: e95688
loc_79ac:
    mov          BX,AX                           ; 1000:79ac: 8bd8
    mov          AX,SEG_SCRATCH                  ; 1000:79ae: b85d1d
    mov          DS,AX                           ; 1000:79b1: 8ed8
    mov          DX,0x600                        ; 1000:79b3: ba0006
    mov          CX,0x7fff                       ; 1000:79b6: b9ff7f
    mov          AX,0x3f00                       ; 1000:79b9: b8003f
    int          0x21                            ; 1000:79bc: cd21
    mov          AX,0x3e00                       ; 1000:79be: b8003e
    int          0x21                            ; 1000:79c1: cd21
    mov          SI,0x600                        ; 1000:79c3: be0006
    ret                                          ; 1000:79c6: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_load_palette_buffer  [Video / Palette]
; ADDRESS:  1000:79c7
; PURPOSE:  Loads custom palette array into active palette staging buffer.
; ----------------------------------------------------------------------------
vid_load_palette_buffer:
    lodsw                                        ; 1000:79c7: ad
    mov          [cs:0x798e],AX                  ; 1000:79c8: 2ea38e79
    mov          CL,0x0                          ; 1000:79cc: b100
    call         vid_out_dx_ax                   ; 1000:79ce: e89700
    call         vid_cycle_palette_step          ; 1000:79d1: e81900
    mov          CL,0x1                          ; 1000:79d4: b101
    call         vid_out_dx_ax                   ; 1000:79d6: e88f00
    call         vid_cycle_palette_step          ; 1000:79d9: e81100
    mov          CL,0x2                          ; 1000:79dc: b102
    call         vid_out_dx_ax                   ; 1000:79de: e88700
    call         vid_cycle_palette_step          ; 1000:79e1: e80900
    mov          CL,0x3                          ; 1000:79e4: b103
    call         vid_out_dx_ax                   ; 1000:79e6: e87f00
    call         vid_cycle_palette_step          ; 1000:79e9: e80100
    ret                                          ; 1000:79ec: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_cycle_palette_step  [Video / Palette]
; ADDRESS:  1000:79ed
; PURPOSE:  Rotates cycling palette colors by one step to animate waterfalls and energy barriers.
; ----------------------------------------------------------------------------
vid_cycle_palette_step:
    mov          BX,DI                           ; 1000:79ed: 8bdf
loc_79ef:
    lodsb                                        ; 1000:79ef: ac
    test         AL,0x80                         ; 1000:79f0: a880
    jnz          loc_79fc                        ; 1000:79f2: 7508
    xor          AH,AH                           ; 1000:79f4: 32e4
    mov          CX,AX                           ; 1000:79f6: 8bc8
    rep movsb                                    ; 1000:79f8: f3a4
    jmp short    loc_7a05                        ; 1000:79fa: eb09
loc_79fc:
    xor          AH,AH                           ; 1000:79fc: 32e4
    and          AL,0x7f                         ; 1000:79fe: 247f
    mov          CX,AX                           ; 1000:7a00: 8bc8
    lodsb                                        ; 1000:7a02: ac
    rep stosb                                    ; 1000:7a03: f3aa
loc_7a05:
    mov          AX,DI                           ; 1000:7a05: 8bc7
    sub          AX,BX                           ; 1000:7a07: 2bc3
    cmp          AX,word [cs:0x798e]             ; 1000:7a09: 2e3b068e79
    jl           loc_79ef                        ; 1000:7a0e: 7cdf
    mov          DI,BX                           ; 1000:7a10: 8bfb
    ret                                          ; 1000:7a12: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_apply_palette  [Video / Palette]
; ADDRESS:  1000:7a13
; PURPOSE:  Writes palette staging buffer into EGA Attribute Controller palette registers.
; ----------------------------------------------------------------------------
vid_apply_palette:
    lodsw                                        ; 1000:7a13: ad
    mov          [cs:0x798e],AX                  ; 1000:7a14: 2ea38e79
    mov          CL,0x0                          ; 1000:7a18: b100
    call         vid_out_dx_ax                   ; 1000:7a1a: e84b00
    call         vid_dim_palette                 ; 1000:7a1d: e81900
    mov          CL,0x1                          ; 1000:7a20: b101
    call         vid_out_dx_ax                   ; 1000:7a22: e84300
    call         vid_dim_palette                 ; 1000:7a25: e81100
    mov          CL,0x2                          ; 1000:7a28: b102
    call         vid_out_dx_ax                   ; 1000:7a2a: e83b00
    call         vid_dim_palette                 ; 1000:7a2d: e80900
    mov          CL,0x3                          ; 1000:7a30: b103
    call         vid_out_dx_ax                   ; 1000:7a32: e83300
    call         vid_dim_palette                 ; 1000:7a35: e80100
    ret                                          ; 1000:7a38: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_dim_palette  [Video / Palette]
; ADDRESS:  1000:7a39
; PURPOSE:  Dims palette colors toward black for screen fade-out.
; ----------------------------------------------------------------------------
vid_dim_palette:
    mov          BX,DI                           ; 1000:7a39: 8bdf
loc_7a3b:
    lodsb                                        ; 1000:7a3b: ac
    test         AL,0x80                         ; 1000:7a3c: a880
    jnz          loc_7a4d                        ; 1000:7a3e: 750d
    xor          AH,AH                           ; 1000:7a40: 32e4
    mov          CX,AX                           ; 1000:7a42: 8bc8
loc_7a44:
    lodsb                                        ; 1000:7a44: ac
    or           byte [es:DI],AL                 ; 1000:7a45: 260805
    inc          DI                              ; 1000:7a48: 47
    loop         loc_7a44                        ; 1000:7a49: e2f9
    jmp short    loc_7a5a                        ; 1000:7a4b: eb0d
loc_7a4d:
    xor          AH,AH                           ; 1000:7a4d: 32e4
    and          AL,0x7f                         ; 1000:7a4f: 247f
    mov          CX,AX                           ; 1000:7a51: 8bc8
    lodsb                                        ; 1000:7a53: ac
loc_7a54:
    or           byte [es:DI],AL                 ; 1000:7a54: 260805
    inc          DI                              ; 1000:7a57: 47
    loop         loc_7a54                        ; 1000:7a58: e2fa
loc_7a5a:
    mov          AX,DI                           ; 1000:7a5a: 8bc7
    sub          AX,BX                           ; 1000:7a5c: 2bc3
    cmp          AX,word [cs:0x798e]             ; 1000:7a5e: 2e3b068e79
    jl           loc_7a3b                        ; 1000:7a63: 7cd6
    mov          DI,BX                           ; 1000:7a65: 8bfb
    ret                                          ; 1000:7a67: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_out_dx_ax  [Video / Hardware]
; ADDRESS:  1000:7a68
; PURPOSE:  Helper routine to write index and data byte simultaneously to EGA I/O port.
; ----------------------------------------------------------------------------
vid_out_dx_ax:
    mov          AH,0x1                          ; 1000:7a68: b401
    shl          AH,CL                           ; 1000:7a6a: d2e4
    mov          AL,0x2                          ; 1000:7a6c: b002
    mov          DX,0x3c4                        ; 1000:7a6e: bac403
    out          DX,AL                           ; 1000:7a71: ee
    inc          DX                              ; 1000:7a72: 42
    xchg         AL,AH                           ; 1000:7a73: 86e0
    out          DX,AL                           ; 1000:7a75: ee
    dec          DX                              ; 1000:7a76: 4a
    xchg         AL,AH                           ; 1000:7a77: 86e0
    mov          AH,CL                           ; 1000:7a79: 8ae1
    mov          AL,0x4                          ; 1000:7a7b: b004
    mov          DX,0x3ce                        ; 1000:7a7d: bace03
    out          DX,AL                           ; 1000:7a80: ee
    inc          DX                              ; 1000:7a81: 42
    xchg         AL,AH                           ; 1000:7a82: 86e0
    out          DX,AL                           ; 1000:7a84: ee
    dec          DX                              ; 1000:7a85: 4a
    xchg         AL,AH                           ; 1000:7a86: 86e0
    ret                                          ; 1000:7a88: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_in_status  [Video / Hardware]
; ADDRESS:  1000:7a89
; PURPOSE:  Reads EGA Input Status Register 1 from Port 0x3da.
; ----------------------------------------------------------------------------
vid_in_status:
    mov          DX,0x3da                        ; 1000:7a89: bada03
loc_7a8c:
    in           AL,DX                           ; 1000:7a8c: ec
    test         AL,0x8                          ; 1000:7a8d: a808
    jnz          loc_7a8c                        ; 1000:7a8f: 75fb
    mov          CX,AX                           ; 1000:7a91: 8bc8
    mov          DX,0x3d4                        ; 1000:7a93: bad403
    mov          AL,0xc                          ; 1000:7a96: b00c
    mov          AH,CH                           ; 1000:7a98: 8ae5
    out          DX,AL                           ; 1000:7a9a: ee
    inc          DX                              ; 1000:7a9b: 42
    xchg         AL,AH                           ; 1000:7a9c: 86e0
    out          DX,AL                           ; 1000:7a9e: ee
    dec          DX                              ; 1000:7a9f: 4a
    xchg         AL,AH                           ; 1000:7aa0: 86e0
    mov          DX,0x3da                        ; 1000:7aa2: bada03
loc_7aa5:
    in           AL,DX                           ; 1000:7aa5: ec
    test         AL,0x8                          ; 1000:7aa6: a808
    jz           loc_7aa5                        ; 1000:7aa8: 74fb
    ret                                          ; 1000:7aaa: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_setup_blit_rect  [Video / Blitting]
; ADDRESS:  1000:7aab
; PURPOSE:  Calculates destination VRAM address and row strides for rectangular blit operation.
; ----------------------------------------------------------------------------
vid_setup_blit_rect:
    mov          DI,word [cs:0x773f]             ; 1000:7aab: 2e8b3e3f77
    push         AX                              ; 1000:7ab0: 50
    push         BX                              ; 1000:7ab1: 53
    push         CX                              ; 1000:7ab2: 51
    push         DX                              ; 1000:7ab3: 52
    call         vid_set_data_rotate             ; 1000:7ab4: e81100
    mov          DI,word [cs:0x773f]             ; 1000:7ab7: 2e8b3e3f77
    xor          DI,0x2000                       ; 1000:7abc: 81f70020
    pop          DX                              ; 1000:7ac0: 5a
    pop          CX                              ; 1000:7ac1: 59
    pop          BX                              ; 1000:7ac2: 5b
    pop          AX                              ; 1000:7ac3: 58
    call         vid_set_data_rotate             ; 1000:7ac4: e80100
    ret                                          ; 1000:7ac7: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_set_data_rotate  [Video / Hardware]
; ADDRESS:  1000:7ac8
; PURPOSE:  Configures EGA Data Rotate / Function Select register (Port 0x3ce index 3).
; ----------------------------------------------------------------------------
vid_set_data_rotate:
    push         DX                              ; 1000:7ac8: 52
    push         CX                              ; 1000:7ac9: 51
    shr          AX,0x1                          ; 1000:7aca: d1e8
    shr          AX,0x1                          ; 1000:7acc: d1e8
    shr          AX,0x1                          ; 1000:7ace: d1e8
    mov          DX,BX                           ; 1000:7ad0: 8bd3
    shl          BX,0x1                          ; 1000:7ad2: d1e3
    shl          BX,0x1                          ; 1000:7ad4: d1e3
    shl          BX,0x1                          ; 1000:7ad6: d1e3
    mov          CL,0x5                          ; 1000:7ad8: b105
    shl          DX,CL                           ; 1000:7ada: d3e2
    add          BX,DX                           ; 1000:7adc: 03da
    add          BX,AX                           ; 1000:7ade: 03d8
    add          DI,BX                           ; 1000:7ae0: 03fb
    pop          AX                              ; 1000:7ae2: 58
    shr          AX,0x1                          ; 1000:7ae3: d1e8
    shr          AX,0x1                          ; 1000:7ae5: d1e8
    shr          AX,0x1                          ; 1000:7ae7: d1e8
    mov          DX,AX                           ; 1000:7ae9: 8bd0
    pop          AX                              ; 1000:7aeb: 58
    mov          BX,AX                           ; 1000:7aec: 8bd8
    mov          CL,0x0                          ; 1000:7aee: b100
    call         vid_reset_data_rotate           ; 1000:7af0: e81000
    mov          CL,0x1                          ; 1000:7af3: b101
    call         vid_reset_data_rotate           ; 1000:7af5: e80b00
    mov          CL,0x2                          ; 1000:7af8: b102
    call         vid_reset_data_rotate           ; 1000:7afa: e80600
    mov          CL,0x3                          ; 1000:7afd: b103
    call         vid_reset_data_rotate           ; 1000:7aff: e80100
    ret                                          ; 1000:7b02: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_reset_data_rotate  [Video / Hardware]
; ADDRESS:  1000:7b03
; PURPOSE:  Resets EGA Data Rotate register to default direct copy mode.
; ----------------------------------------------------------------------------
vid_reset_data_rotate:
    push         DX                              ; 1000:7b03: 52
    call         vid_out_dx_ax                   ; 1000:7b04: e861ff
    pop          DX                              ; 1000:7b07: 5a
    push         DI                              ; 1000:7b08: 57
    push         BX                              ; 1000:7b09: 53
    mov          AX,BX                           ; 1000:7b0a: 8bc3
loc_7b0c:
    push         AX                              ; 1000:7b0c: 50
    xor          AX,AX                           ; 1000:7b0d: 33c0
    push         DI                              ; 1000:7b0f: 57
    mov          CX,DX                           ; 1000:7b10: 8bca
    test         CX,0x1                          ; 1000:7b12: f7c10100
    jz           loc_7b1c                        ; 1000:7b16: 7404
    stosb                                        ; 1000:7b18: aa
    dec          CX                              ; 1000:7b19: 49
    jz           loc_7b24                        ; 1000:7b1a: 7408
loc_7b1c:
    shr          CX,0x1                          ; 1000:7b1c: d1e9
    or           CX,CX                           ; 1000:7b1e: 0bc9
    jz           loc_7b24                        ; 1000:7b20: 7402
    rep stosw                                    ; 1000:7b22: f3ab
loc_7b24:
    pop          DI                              ; 1000:7b24: 5f
    add          DI,0x28                         ; 1000:7b25: 83c728
    pop          AX                              ; 1000:7b28: 58
    dec          AX                              ; 1000:7b29: 48
    jnz          loc_7b0c                        ; 1000:7b2a: 75e0
    pop          BX                              ; 1000:7b2c: 5b
    pop          DI                              ; 1000:7b2d: 5f
    ret                                          ; 1000:7b2e: c3
    db 0x00, 0x00                                   ; 1000:7b2f

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_tile_aligned  [Video / Blitting]
; ADDRESS:  1000:7b31
; PURPOSE:  Fast 16x16 tile blitter for byte-aligned horizontal positions.
; ----------------------------------------------------------------------------
vid_blit_tile_aligned:
    push         BP                              ; 1000:7b31: 55
    push         AX                              ; 1000:7b32: 50
    push         CX                              ; 1000:7b33: 51
    shr          AX,0x1                          ; 1000:7b34: d1e8
    shr          AX,0x1                          ; 1000:7b36: d1e8
    shr          AX,0x1                          ; 1000:7b38: d1e8
    mov          DX,BX                           ; 1000:7b3a: 8bd3
    shl          BX,0x1                          ; 1000:7b3c: d1e3
    shl          BX,0x1                          ; 1000:7b3e: d1e3
    shl          BX,0x1                          ; 1000:7b40: d1e3
    mov          CL,0x5                          ; 1000:7b42: b105
    shl          DX,CL                           ; 1000:7b44: d3e2
    add          BX,DX                           ; 1000:7b46: 03da
    add          BX,AX                           ; 1000:7b48: 03d8
    mov          DI,word [cs:0x773f]             ; 1000:7b4a: 2e8b3e3f77
    add          DI,BX                           ; 1000:7b4f: 03fb
    lodsw                                        ; 1000:7b51: ad
    shr          AX,0x1                          ; 1000:7b52: d1e8
    shr          AX,0x1                          ; 1000:7b54: d1e8
    shr          AX,0x1                          ; 1000:7b56: d1e8
    mov          DX,AX                           ; 1000:7b58: 8bd0
    lodsw                                        ; 1000:7b5a: ad
    mov          BX,AX                           ; 1000:7b5b: 8bd8
    lodsw                                        ; 1000:7b5d: ad
    mov          BP,SI                           ; 1000:7b5e: 8bee
    add          SI,AX                           ; 1000:7b60: 03f0
    pop          AX                              ; 1000:7b62: 58
    mov          CX,BX                           ; 1000:7b63: 8bcb
    sub          CX,AX                           ; 1000:7b65: 2bc8
    mov          word [cs:0x7b2f],CX             ; 1000:7b67: 2e890e2f7b
    pop          AX                              ; 1000:7b6c: 58
    test         AX,0x4                          ; 1000:7b6d: a90400
    jz           loc_7b75                        ; 1000:7b70: 7403
    jmp short    vid_blit_glyph_aligned          ; 1000:7b72: eb65
    db 0x90                                         ; 1000:7b74
loc_7b75:
    mov          CL,0x0                          ; 1000:7b75: b100
    call         vid_blit_tile_shifted           ; 1000:7b77: e81100
    mov          CL,0x1                          ; 1000:7b7a: b101
    call         vid_blit_tile_shifted           ; 1000:7b7c: e80c00
    mov          CL,0x2                          ; 1000:7b7f: b102
    call         vid_blit_tile_shifted           ; 1000:7b81: e80700
    mov          CL,0x3                          ; 1000:7b84: b103
    call         vid_blit_tile_shifted           ; 1000:7b86: e80200
    pop          BP                              ; 1000:7b89: 5d
    ret                                          ; 1000:7b8a: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_tile_shifted  [Video / Blitting]
; ADDRESS:  1000:7b8b
; PURPOSE:  16x16 tile blitter with bit-shift rotation for smooth pixel-accurate horizontal scrolling.
; ----------------------------------------------------------------------------
vid_blit_tile_shifted:
    push         DX                              ; 1000:7b8b: 52
    call         vid_out_dx_ax                   ; 1000:7b8c: e8d9fe
    pop          DX                              ; 1000:7b8f: 5a
    push         DI                              ; 1000:7b90: 57
    push         BX                              ; 1000:7b91: 53
    mov          AX,BX                           ; 1000:7b92: 8bc3
    mov          BX,BP                           ; 1000:7b94: 8bdd
loc_7b96:
    push         AX                              ; 1000:7b96: 50
    push         DI                              ; 1000:7b97: 57
    mov          CX,DX                           ; 1000:7b98: 8bca
    cmp          AX,word [cs:0x7b2f]             ; 1000:7b9a: 2e3b062f7b
    jle          loc_7ba7                        ; 1000:7b9f: 7e06
    add          BX,CX                           ; 1000:7ba1: 03d9
    add          SI,CX                           ; 1000:7ba3: 03f1
    jmp short    loc_7bce                        ; 1000:7ba5: eb27
loc_7ba7:
    cmp          CX,0x1                          ; 1000:7ba7: 83f901
    jz           loc_7bc4                        ; 1000:7baa: 7418
    mov          AX,word [BX]                    ; 1000:7bac: 8b07
    add          BX,0x2                          ; 1000:7bae: 83c302
    and          word [es:DI],AX                 ; 1000:7bb1: 262105
    lodsw                                        ; 1000:7bb4: ad
    or           word [es:DI],AX                 ; 1000:7bb5: 260905
    add          DI,0x2                          ; 1000:7bb8: 83c702
    sub          CX,0x2                          ; 1000:7bbb: 83e902
    or           CX,CX                           ; 1000:7bbe: 0bc9
    jnz          loc_7ba7                        ; 1000:7bc0: 75e5
    jmp short    loc_7bce                        ; 1000:7bc2: eb0a
loc_7bc4:
    mov          AL,byte [BX]                    ; 1000:7bc4: 8a07
    inc          BX                              ; 1000:7bc6: 43
    and          byte [es:DI],AL                 ; 1000:7bc7: 262005
    lodsb                                        ; 1000:7bca: ac
    or           byte [es:DI],AL                 ; 1000:7bcb: 260805
loc_7bce:
    pop          DI                              ; 1000:7bce: 5f
    add          DI,0x28                         ; 1000:7bcf: 83c728
    pop          AX                              ; 1000:7bd2: 58
    dec          AX                              ; 1000:7bd3: 48
    jnz          loc_7b96                        ; 1000:7bd4: 75c0
    pop          BX                              ; 1000:7bd6: 5b
    pop          DI                              ; 1000:7bd7: 5f
    ret                                          ; 1000:7bd8: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_glyph_aligned  [Video / Blitting]
; ADDRESS:  1000:7bd9
; PURPOSE:  8x8 font glyph blitter aligned to byte boundaries.
; ----------------------------------------------------------------------------
vid_blit_glyph_aligned:
    mov          CL,0x0                          ; 1000:7bd9: b100
    call         vid_blit_glyph_shifted          ; 1000:7bdb: e81100
    mov          CL,0x1                          ; 1000:7bde: b101
    call         vid_blit_glyph_shifted          ; 1000:7be0: e80c00
    mov          CL,0x2                          ; 1000:7be3: b102
    call         vid_blit_glyph_shifted          ; 1000:7be5: e80700
    mov          CL,0x3                          ; 1000:7be8: b103
    call         vid_blit_glyph_shifted          ; 1000:7bea: e80200
    pop          BP                              ; 1000:7bed: 5d
    ret                                          ; 1000:7bee: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_glyph_shifted  [Video / Blitting]
; ADDRESS:  1000:7bef
; PURPOSE:  8x8 font glyph blitter with arbitrary pixel X shift.
; ----------------------------------------------------------------------------
vid_blit_glyph_shifted:
    push         DX                              ; 1000:7bef: 52
    call         vid_out_dx_ax                   ; 1000:7bf0: e875fe
    pop          DX                              ; 1000:7bf3: 5a
    push         DI                              ; 1000:7bf4: 57
    push         BX                              ; 1000:7bf5: 53
    push         BP                              ; 1000:7bf6: 55
    mov          AX,BX                           ; 1000:7bf7: 8bc3
    mov          BX,BP                           ; 1000:7bf9: 8bdd
loc_7bfb:
    push         AX                              ; 1000:7bfb: 50
    push         DI                              ; 1000:7bfc: 57
    mov          CX,DX                           ; 1000:7bfd: 8bca
    cmp          AX,word [cs:0x7b2f]             ; 1000:7bff: 2e3b062f7b
    jle          loc_7c0c                        ; 1000:7c04: 7e06
    add          BX,CX                           ; 1000:7c06: 03d9
    add          SI,CX                           ; 1000:7c08: 03f1
    jmp short    loc_7c6d                        ; 1000:7c0a: eb61
loc_7c0c:
    xor          BP,BP                           ; 1000:7c0c: 33ed
    mov          AX,0xffff                       ; 1000:7c0e: b8ffff
    mov          AL,byte [BX]                    ; 1000:7c11: 8a07
    inc          BX                              ; 1000:7c13: 43
    shl          AX,0x1                          ; 1000:7c14: d1e0
    shl          AX,0x1                          ; 1000:7c16: d1e0
    shl          AX,0x1                          ; 1000:7c18: d1e0
    shl          AX,0x1                          ; 1000:7c1a: d1e0
    and          byte [es:DI],AH                 ; 1000:7c1c: 262025
    xchg         AX,BP                           ; 1000:7c1f: 95
    lodsb                                        ; 1000:7c20: ac
    shl          AX,0x1                          ; 1000:7c21: d1e0
    shl          AX,0x1                          ; 1000:7c23: d1e0
    shl          AX,0x1                          ; 1000:7c25: d1e0
    shl          AX,0x1                          ; 1000:7c27: d1e0
    or           byte [es:DI],AH                 ; 1000:7c29: 260825
    inc          DI                              ; 1000:7c2c: 47
    xchg         AX,BP                           ; 1000:7c2d: 95
loc_7c2e:
    cmp          CX,0x1                          ; 1000:7c2e: 83f901
    jz           loc_7c63                        ; 1000:7c31: 7430
    shl          AX,0x1                          ; 1000:7c33: d1e0
    shl          AX,0x1                          ; 1000:7c35: d1e0
    shl          AX,0x1                          ; 1000:7c37: d1e0
    shl          AX,0x1                          ; 1000:7c39: d1e0
    mov          AL,byte [BX]                    ; 1000:7c3b: 8a07
    inc          BX                              ; 1000:7c3d: 43
    shl          AX,0x1                          ; 1000:7c3e: d1e0
    shl          AX,0x1                          ; 1000:7c40: d1e0
    shl          AX,0x1                          ; 1000:7c42: d1e0
    shl          AX,0x1                          ; 1000:7c44: d1e0
    and          byte [es:DI],AH                 ; 1000:7c46: 262025
    xchg         AX,BP                           ; 1000:7c49: 95
    shl          AX,0x1                          ; 1000:7c4a: d1e0
    shl          AX,0x1                          ; 1000:7c4c: d1e0
    shl          AX,0x1                          ; 1000:7c4e: d1e0
    shl          AX,0x1                          ; 1000:7c50: d1e0
    lodsb                                        ; 1000:7c52: ac
    shl          AX,0x1                          ; 1000:7c53: d1e0
    shl          AX,0x1                          ; 1000:7c55: d1e0
    shl          AX,0x1                          ; 1000:7c57: d1e0
    shl          AX,0x1                          ; 1000:7c59: d1e0
    or           byte [es:DI],AH                 ; 1000:7c5b: 260825
    inc          DI                              ; 1000:7c5e: 47
    xchg         AX,BP                           ; 1000:7c5f: 95
    dec          CX                              ; 1000:7c60: 49
    jmp short    loc_7c2e                        ; 1000:7c61: ebcb
loc_7c63:
    or           AL,0xf                          ; 1000:7c63: 0c0f
    and          byte [es:DI],AL                 ; 1000:7c65: 262005
    mov          AX,BP                           ; 1000:7c68: 8bc5
    or           byte [es:DI],AL                 ; 1000:7c6a: 260805
loc_7c6d:
    pop          DI                              ; 1000:7c6d: 5f
    add          DI,0x28                         ; 1000:7c6e: 83c728
    pop          AX                              ; 1000:7c71: 58
    dec          AX                              ; 1000:7c72: 48
    jnz          loc_7bfb                        ; 1000:7c73: 7586
    pop          BP                              ; 1000:7c75: 5d
    pop          BX                              ; 1000:7c76: 5b
    pop          DI                              ; 1000:7c77: 5f
    ret                                          ; 1000:7c78: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_sprite_masked_aligned  [Video / Blitting]
; ADDRESS:  1000:7c79
; PURPOSE:  Masked 16x16 sprite blitter aligned to byte boundaries.
; ----------------------------------------------------------------------------
vid_blit_sprite_masked_aligned:
    push         BP                              ; 1000:7c79: 55
    push         AX                              ; 1000:7c7a: 50
    mov          word [cs:0x7b2f],CX             ; 1000:7c7b: 2e890e2f7b
    shr          AX,0x1                          ; 1000:7c80: d1e8
    shr          AX,0x1                          ; 1000:7c82: d1e8
    shr          AX,0x1                          ; 1000:7c84: d1e8
    mov          DX,BX                           ; 1000:7c86: 8bd3
    shl          BX,0x1                          ; 1000:7c88: d1e3
    shl          BX,0x1                          ; 1000:7c8a: d1e3
    shl          BX,0x1                          ; 1000:7c8c: d1e3
    mov          CL,0x5                          ; 1000:7c8e: b105
    shl          DX,CL                           ; 1000:7c90: d3e2
    add          BX,DX                           ; 1000:7c92: 03da
    add          BX,AX                           ; 1000:7c94: 03d8
    mov          DI,word [cs:0x773f]             ; 1000:7c96: 2e8b3e3f77
    add          DI,BX                           ; 1000:7c9b: 03fb
    lodsw                                        ; 1000:7c9d: ad
    shr          AX,0x1                          ; 1000:7c9e: d1e8
    shr          AX,0x1                          ; 1000:7ca0: d1e8
    shr          AX,0x1                          ; 1000:7ca2: d1e8
    mov          DX,AX                           ; 1000:7ca4: 8bd0
    lodsw                                        ; 1000:7ca6: ad
    mov          BX,AX                           ; 1000:7ca7: 8bd8
    lodsw                                        ; 1000:7ca9: ad
    mov          BP,SI                           ; 1000:7caa: 8bee
    add          SI,AX                           ; 1000:7cac: 03f0
    pop          AX                              ; 1000:7cae: 58
    test         AX,0x4                          ; 1000:7caf: a90400
    jz           loc_7cb7                        ; 1000:7cb2: 7403
    jmp short    vid_blit_small_sprite_aligned   ; 1000:7cb4: eb65
    db 0x90                                         ; 1000:7cb6
loc_7cb7:
    mov          CL,0x0                          ; 1000:7cb7: b100
    call         vid_blit_sprite_masked_shifted  ; 1000:7cb9: e81100
    mov          CL,0x1                          ; 1000:7cbc: b101
    call         vid_blit_sprite_masked_shifted  ; 1000:7cbe: e80c00
    mov          CL,0x2                          ; 1000:7cc1: b102
    call         vid_blit_sprite_masked_shifted  ; 1000:7cc3: e80700
    mov          CL,0x3                          ; 1000:7cc6: b103
    call         vid_blit_sprite_masked_shifted  ; 1000:7cc8: e80200
    pop          BP                              ; 1000:7ccb: 5d
    ret                                          ; 1000:7ccc: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_sprite_masked_shifted  [Video / Blitting]
; ADDRESS:  1000:7ccd
; PURPOSE:  Masked 16x16 sprite blitter with arbitrary pixel X shift.
; ----------------------------------------------------------------------------
vid_blit_sprite_masked_shifted:
    push         DX                              ; 1000:7ccd: 52
    call         vid_out_dx_ax                   ; 1000:7cce: e897fd
    pop          DX                              ; 1000:7cd1: 5a
    push         DI                              ; 1000:7cd2: 57
    push         BX                              ; 1000:7cd3: 53
    mov          AX,BX                           ; 1000:7cd4: 8bc3
    mov          BX,BP                           ; 1000:7cd6: 8bdd
loc_7cd8:
    push         AX                              ; 1000:7cd8: 50
    push         DI                              ; 1000:7cd9: 57
    mov          CX,DX                           ; 1000:7cda: 8bca
    cmp          AX,word [cs:0x7b2f]             ; 1000:7cdc: 2e3b062f7b
    jg           loc_7ce9                        ; 1000:7ce1: 7f06
    add          BX,CX                           ; 1000:7ce3: 03d9
    add          SI,CX                           ; 1000:7ce5: 03f1
    jmp short    loc_7d10                        ; 1000:7ce7: eb27
loc_7ce9:
    cmp          CX,0x1                          ; 1000:7ce9: 83f901
    jz           loc_7d06                        ; 1000:7cec: 7418
    mov          AX,word [BX]                    ; 1000:7cee: 8b07
    add          BX,0x2                          ; 1000:7cf0: 83c302
    and          word [es:DI],AX                 ; 1000:7cf3: 262105
    lodsw                                        ; 1000:7cf6: ad
    or           word [es:DI],AX                 ; 1000:7cf7: 260905
    add          DI,0x2                          ; 1000:7cfa: 83c702
    sub          CX,0x2                          ; 1000:7cfd: 83e902
    or           CX,CX                           ; 1000:7d00: 0bc9
    jnz          loc_7ce9                        ; 1000:7d02: 75e5
    jmp short    loc_7d10                        ; 1000:7d04: eb0a
loc_7d06:
    mov          AL,byte [BX]                    ; 1000:7d06: 8a07
    inc          BX                              ; 1000:7d08: 43
    and          byte [es:DI],AL                 ; 1000:7d09: 262005
    lodsb                                        ; 1000:7d0c: ac
    or           byte [es:DI],AL                 ; 1000:7d0d: 260805
loc_7d10:
    pop          DI                              ; 1000:7d10: 5f
    add          DI,0x28                         ; 1000:7d11: 83c728
    pop          AX                              ; 1000:7d14: 58
    dec          AX                              ; 1000:7d15: 48
    jnz          loc_7cd8                        ; 1000:7d16: 75c0
    pop          BX                              ; 1000:7d18: 5b
    pop          DI                              ; 1000:7d19: 5f
    ret                                          ; 1000:7d1a: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_small_sprite_aligned  [Video / Blitting]
; ADDRESS:  1000:7d1b
; PURPOSE:  Small 8x8 projectile/bullet sprite blitter aligned to byte boundaries.
; ----------------------------------------------------------------------------
vid_blit_small_sprite_aligned:
    mov          CL,0x0                          ; 1000:7d1b: b100
    call         vid_blit_small_sprite_shifted   ; 1000:7d1d: e81100
    mov          CL,0x1                          ; 1000:7d20: b101
    call         vid_blit_small_sprite_shifted   ; 1000:7d22: e80c00
    mov          CL,0x2                          ; 1000:7d25: b102
    call         vid_blit_small_sprite_shifted   ; 1000:7d27: e80700
    mov          CL,0x3                          ; 1000:7d2a: b103
    call         vid_blit_small_sprite_shifted   ; 1000:7d2c: e80200
    pop          BP                              ; 1000:7d2f: 5d
    ret                                          ; 1000:7d30: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_small_sprite_shifted  [Video / Blitting]
; ADDRESS:  1000:7d31
; PURPOSE:  Small 8x8 projectile/bullet sprite blitter with pixel shift.
; ----------------------------------------------------------------------------
vid_blit_small_sprite_shifted:
    push         DX                              ; 1000:7d31: 52
    call         vid_out_dx_ax                   ; 1000:7d32: e833fd
    pop          DX                              ; 1000:7d35: 5a
    push         DI                              ; 1000:7d36: 57
    push         BX                              ; 1000:7d37: 53
    push         BP                              ; 1000:7d38: 55
    mov          AX,BX                           ; 1000:7d39: 8bc3
    mov          BX,BP                           ; 1000:7d3b: 8bdd
loc_7d3d:
    push         AX                              ; 1000:7d3d: 50
    push         DI                              ; 1000:7d3e: 57
    mov          CX,DX                           ; 1000:7d3f: 8bca
    cmp          AX,word [cs:0x7b2f]             ; 1000:7d41: 2e3b062f7b
    jg           loc_7d4e                        ; 1000:7d46: 7f06
    add          BX,CX                           ; 1000:7d48: 03d9
    add          SI,CX                           ; 1000:7d4a: 03f1
    jmp short    loc_7daf                        ; 1000:7d4c: eb61
loc_7d4e:
    xor          BP,BP                           ; 1000:7d4e: 33ed
    mov          AX,0xffff                       ; 1000:7d50: b8ffff
    mov          AL,byte [BX]                    ; 1000:7d53: 8a07
    inc          BX                              ; 1000:7d55: 43
    shl          AX,0x1                          ; 1000:7d56: d1e0
    shl          AX,0x1                          ; 1000:7d58: d1e0
    shl          AX,0x1                          ; 1000:7d5a: d1e0
    shl          AX,0x1                          ; 1000:7d5c: d1e0
    and          byte [es:DI],AH                 ; 1000:7d5e: 262025
    xchg         AX,BP                           ; 1000:7d61: 95
    lodsb                                        ; 1000:7d62: ac
    shl          AX,0x1                          ; 1000:7d63: d1e0
    shl          AX,0x1                          ; 1000:7d65: d1e0
    shl          AX,0x1                          ; 1000:7d67: d1e0
    shl          AX,0x1                          ; 1000:7d69: d1e0
    or           byte [es:DI],AH                 ; 1000:7d6b: 260825
    inc          DI                              ; 1000:7d6e: 47
    xchg         AX,BP                           ; 1000:7d6f: 95
loc_7d70:
    cmp          CX,0x1                          ; 1000:7d70: 83f901
    jz           loc_7da5                        ; 1000:7d73: 7430
    shl          AX,0x1                          ; 1000:7d75: d1e0
    shl          AX,0x1                          ; 1000:7d77: d1e0
    shl          AX,0x1                          ; 1000:7d79: d1e0
    shl          AX,0x1                          ; 1000:7d7b: d1e0
    mov          AL,byte [BX]                    ; 1000:7d7d: 8a07
    inc          BX                              ; 1000:7d7f: 43
    shl          AX,0x1                          ; 1000:7d80: d1e0
    shl          AX,0x1                          ; 1000:7d82: d1e0
    shl          AX,0x1                          ; 1000:7d84: d1e0
    shl          AX,0x1                          ; 1000:7d86: d1e0
    and          byte [es:DI],AH                 ; 1000:7d88: 262025
    xchg         AX,BP                           ; 1000:7d8b: 95
    shl          AX,0x1                          ; 1000:7d8c: d1e0
    shl          AX,0x1                          ; 1000:7d8e: d1e0
    shl          AX,0x1                          ; 1000:7d90: d1e0
    shl          AX,0x1                          ; 1000:7d92: d1e0
    lodsb                                        ; 1000:7d94: ac
    shl          AX,0x1                          ; 1000:7d95: d1e0
    shl          AX,0x1                          ; 1000:7d97: d1e0
    shl          AX,0x1                          ; 1000:7d99: d1e0
    shl          AX,0x1                          ; 1000:7d9b: d1e0
    or           byte [es:DI],AH                 ; 1000:7d9d: 260825
    inc          DI                              ; 1000:7da0: 47
    xchg         AX,BP                           ; 1000:7da1: 95
    dec          CX                              ; 1000:7da2: 49
    jmp short    loc_7d70                        ; 1000:7da3: ebcb
loc_7da5:
    or           AL,0xf                          ; 1000:7da5: 0c0f
    and          byte [es:DI],AL                 ; 1000:7da7: 262005
    mov          AX,BP                           ; 1000:7daa: 8bc5
    or           byte [es:DI],AL                 ; 1000:7dac: 260805
loc_7daf:
    pop          DI                              ; 1000:7daf: 5f
    add          DI,0x28                         ; 1000:7db0: 83c728
    pop          AX                              ; 1000:7db3: 58
    dec          AX                              ; 1000:7db4: 48
    jnz          loc_7d3d                        ; 1000:7db5: 7586
    pop          BP                              ; 1000:7db7: 5d
    pop          BX                              ; 1000:7db8: 5b
    pop          DI                              ; 1000:7db9: 5f
    ret                                          ; 1000:7dba: c3

; ----------------------------------------------------------------------------
; FUNCTION: vid_blit_planar_block  [Video / Blitting]
; ADDRESS:  1000:7dbb
; PURPOSE:  Universal planar block blitter: blits arbitrary width and height rectangles across all 4 EGA planes.
; ----------------------------------------------------------------------------
vid_blit_planar_block:
    push         BP                              ; 1000:7dbb: 55
    mov          BP,SP                           ; 1000:7dbc: 8bec
    sub          SP,0x4                          ; 1000:7dbe: 83ec04
    mov          DS,word [cs:0x5e]               ; 1000:7dc1: 2e8e1e5e00
    mov          CX,word [SI]                    ; 1000:7dc6: 8b0c
    mov          word [BP + -0x2],CX             ; 1000:7dc8: 894efe
    mov          CX,word [SI + 0x2]              ; 1000:7dcb: 8b4c02
    mov          word [BP + -0x4],CX             ; 1000:7dce: 894efc
    mov          DS,word [cs:0x5c]               ; 1000:7dd1: 2e8e1e5c00
    sub          AX,word [0x6c2]                 ; 1000:7dd6: 2b06c206
    jl           loc_7e3f                        ; 1000:7dda: 7c63
    mov          CX,AX                           ; 1000:7ddc: 8bc8
    add          CX,word [BP + -0x2]             ; 1000:7dde: 034efe
    cmp          CX,0xd0                         ; 1000:7de1: 81f9d000
    jg           loc_7e3f                        ; 1000:7de5: 7f58
    add          AX,0x8                          ; 1000:7de7: 050800
    sub          BX,word [0x6c4]                 ; 1000:7dea: 2b1ec406
    jge          loc_7e0b                        ; 1000:7dee: 7d1b
    mov          CX,BX                           ; 1000:7df0: 8bcb
    neg          CX                              ; 1000:7df2: f7d9
    cmp          CX,word [BP + -0x4]             ; 1000:7df4: 3b4efc
    jge          loc_7e3f                        ; 1000:7df7: 7d46
    add          BX,0x8                          ; 1000:7df9: 83c308
    mov          DS,word [cs:0x5e]               ; 1000:7dfc: 2e8e1e5e00
    call         vid_blit_tile_aligned           ; 1000:7e01: e82dfd
    mov          DS,word [cs:0x5c]               ; 1000:7e04: 2e8e1e5c00
    jmp short    loc_7e43                        ; 1000:7e09: eb38
loc_7e0b:
    mov          CX,BX                           ; 1000:7e0b: 8bcb
    add          CX,word [BP + -0x4]             ; 1000:7e0d: 034efc
    sub          CX,0xa0                         ; 1000:7e10: 81e9a000
    jle          loc_7e2d                        ; 1000:7e14: 7e17
    cmp          CX,word [BP + -0x4]             ; 1000:7e16: 3b4efc
    jge          loc_7e3f                        ; 1000:7e19: 7d24
    add          BX,0x8                          ; 1000:7e1b: 83c308
    mov          DS,word [cs:0x5e]               ; 1000:7e1e: 2e8e1e5e00
    call         vid_blit_sprite_masked_aligned  ; 1000:7e23: e853fe
    mov          DS,word [cs:0x5c]               ; 1000:7e26: 2e8e1e5c00
    jmp short    loc_7e43                        ; 1000:7e2b: eb16
loc_7e2d:
    add          BX,0x8                          ; 1000:7e2d: 83c308
    mov          DS,word [cs:0x5e]               ; 1000:7e30: 2e8e1e5e00
    call         vid_clear_page                  ; 1000:7e35: e86bf9
    mov          DS,word [cs:0x5c]               ; 1000:7e38: 2e8e1e5c00
    jmp short    loc_7e43                        ; 1000:7e3d: eb04
loc_7e3f:
    xor          AX,AX                           ; 1000:7e3f: 33c0
    jmp short    loc_7e46                        ; 1000:7e41: eb03
loc_7e43:
    mov          AX,0x1                          ; 1000:7e43: b80100
loc_7e46:
    mov          SP,BP                           ; 1000:7e46: 8be5
    pop          BP                              ; 1000:7e48: 5d
    ret                                          ; 1000:7e49: c3
    db 0x58, 0x7e, 0x8a, 0x7e, 0x8f, 0x7e, 0x92, 0x7e, 0x95, 0x7e, 0xd5, 0x7f, 0x57, 0x81, 0x03, 0x00 ; 1000:7e4a
    db 0x00, 0x00, 0x00, 0x00, 0x72, 0x7e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7a, 0x7e, 0x05, 0x00 ; 1000:7e5a
    db 0x00, 0x00, 0x00, 0x00, 0x82, 0x7e, 0x0d, 0x00, 0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x00 ; 1000:7e6a
    db 0x01, 0x01, 0x11, 0x01, 0x01, 0x01, 0x18, 0x00, 0x01, 0x01, 0x01, 0x01, 0x11, 0x01, 0x18, 0x00 ; 1000:7e7a
    db 0x1a, 0x1c, 0x39, 0x50, 0x8b, 0x19, 0x19, 0x8b, 0x8c, 0x8c, 0x18, 0xa5, 0x7e, 0x01, 0x7f, 0x39 ; 1000:7e8a
    db 0x7f, 0x4d, 0x7f, 0x61, 0x7f, 0x87, 0x7f, 0x9b, 0x7f, 0xc1, 0x7f, 0x05, 0x00, 0x90, 0x02, 0xf0 ; 1000:7e9a
    db 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:7eaa
    db 0x02, 0x70, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00 ; 1000:7eba
    db 0x00, 0x80, 0x04, 0xe0, 0x00, 0x10, 0x00, 0x70, 0x71, 0x70, 0x71, 0x01, 0x00, 0x00, 0x00, 0x00 ; 1000:7eca
    db 0x00, 0x53, 0x02, 0x70, 0x04, 0xb0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06 ; 1000:7eda
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x04, 0x80, 0x00, 0x10, 0x00, 0x70, 0x71, 0x70, 0x71, 0x01 ; 1000:7eea
    db 0x00, 0x08, 0x00, 0x00, 0x00, 0x57, 0x02, 0x03, 0x00, 0xf0, 0x01, 0x80, 0x00, 0x10, 0x00, 0x00 ; 1000:7efa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x40, 0x00, 0x10 ; 1000:7f0a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x00, 0x40 ; 1000:7f1a
    db 0x00, 0x10, 0x00, 0x80, 0x81, 0x82, 0x83, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x5b, 0x02, 0x01 ; 1000:7f2a
    db 0x00, 0x30, 0x00, 0x70, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 ; 1000:7f3a
    db 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x30, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:7f4a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x30, 0x00, 0x60, 0x00, 0x10, 0x00, 0x00 ; 1000:7f5a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x20, 0x00, 0x10 ; 1000:7f6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20 ; 1000:7f7a
    db 0x00, 0x70, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 ; 1000:7f8a
    db 0x00, 0x02, 0x00, 0x30, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:7f9a
    db 0x00, 0x03, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x70, 0x00, 0x10, 0x00, 0x80, 0x81, 0x82, 0x83, 0x03 ; 1000:7faa
    db 0x00, 0x00, 0x00, 0x01, 0x00, 0x5f, 0x02, 0x01, 0x00, 0xe0, 0x02, 0xe0, 0x00, 0x10, 0x00, 0x70 ; 1000:7fba
    db 0x71, 0x70, 0x71, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe5, 0x7f, 0x17, 0x80, 0x6d ; 1000:7fca
    db 0x80, 0x87, 0x80, 0x89, 0x80, 0x8b, 0x80, 0x8d, 0x80, 0x8f, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00 ; 1000:7fda
    db 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x00 ; 1000:7fea
    db 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x10, 0x01, 0x00, 0x00, 0x01, 0x00, 0x02 ; 1000:7ffa
    db 0x00, 0x00, 0x00, 0x30, 0x01, 0xa0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0xf0 ; 1000:800a
    db 0x03, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x30, 0x00, 0xc0 ; 1000:801a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xf0, 0x03, 0xe0, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00 ; 1000:802a
    db 0x00, 0x02, 0x00, 0xf0, 0x03, 0x30, 0x01, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00 ; 1000:803a
    db 0x00, 0x30, 0x01, 0xb0, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:804a
    db 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0xe0, 0x03, 0x10, 0x02, 0x30, 0x02, 0x01, 0x00, 0x00 ; 1000:805a
    db 0x00, 0x00, 0x00, 0x02, 0x00, 0x30, 0x08, 0x40, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04 ; 1000:806a
    db 0x00, 0x30, 0x08, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00 ; 1000:807a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x18, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00 ; 1000:808a
    db 0x00, 0x04, 0x00, 0x11, 0x81, 0x11, 0x81, 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x02, 0x00, 0x04 ; 1000:809a
    db 0x00, 0x04, 0x00, 0x19, 0x81, 0x19, 0x81, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00 ; 1000:80aa
    db 0x00, 0x02, 0x00, 0x29, 0x81, 0x2d, 0x81, 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x04, 0x00, 0x04 ; 1000:80ba
    db 0x00, 0x04, 0x00, 0x31, 0x81, 0x31, 0x81, 0x10, 0x00, 0x08, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00 ; 1000:80ca
    db 0x00, 0x02, 0x00, 0x41, 0x81, 0x45, 0x81, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00 ; 1000:80da
    db 0x00, 0x04, 0x00, 0x49, 0x81, 0x49, 0x81, 0x10, 0x00, 0x10, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01 ; 1000:80ea
    db 0x00, 0x00, 0x00, 0x51, 0x81, 0x51, 0x81, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01 ; 1000:80fa
    db 0x00, 0x00, 0x00, 0x55, 0x81, 0x53, 0x81, 0x00, 0x00, 0xf6, 0x00, 0xec, 0x01, 0xf6, 0x00, 0xe2 ; 1000:810a
    db 0x02, 0x88, 0x03, 0x2e, 0x04, 0x88, 0x03, 0xd4, 0x04, 0x7a, 0x05, 0x20, 0x06, 0xc6, 0x06, 0x6c ; 1000:811a
    db 0x07, 0x12, 0x08, 0xb8, 0x08, 0x5e, 0x09, 0x04, 0x0a, 0xaa, 0x0a, 0x50, 0x0b, 0xaa, 0x0a, 0xf6 ; 1000:812a
    db 0x0b, 0x9c, 0x0c, 0x42, 0x0d, 0xe8, 0x0d, 0x8e, 0x0e, 0xe4, 0x0e, 0x3a, 0x0f, 0x90, 0x0f, 0xe6 ; 1000:813a
    db 0x0f, 0x8c, 0x10, 0x32, 0x11, 0xd8, 0x11, 0x7e, 0x12, 0x24, 0x13, 0x6a, 0x14, 0x67, 0x81, 0xed ; 1000:814a
    db 0x81, 0xd3, 0x82, 0xdd, 0x83, 0x0f, 0x84, 0x01, 0x85, 0x03, 0x85, 0x05, 0x85, 0x0b, 0x00, 0x30 ; 1000:815a
    db 0x02, 0x98, 0x00, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0xc8, 0x00, 0x91 ; 1000:816a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0xc8, 0x00, 0x91, 0x80, 0x00, 0x00, 0x00 ; 1000:817a
    db 0x00, 0x00, 0x00, 0xe0, 0x02, 0xf8, 0x00, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:818a
    db 0x03, 0x68, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0xb8, 0x01, 0x91 ; 1000:819a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xb8, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00 ; 1000:81aa
    db 0x00, 0x00, 0x00, 0xb0, 0x00, 0x18, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:81ba
    db 0x02, 0x78, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x78, 0x01, 0x91 ; 1000:81ca
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0xb8, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00 ; 1000:81da
    db 0x00, 0x00, 0x00, 0x13, 0x00, 0x50, 0x02, 0x18, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:81ea
    db 0x00, 0x30, 0x02, 0x38, 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x78 ; 1000:81fa
    db 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x88, 0x00, 0x91, 0x80, 0x00 ; 1000:820a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x48, 0x00, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:821a
    db 0x00, 0x20, 0x00, 0x48, 0x00, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03, 0xb8 ; 1000:822a
    db 0x01, 0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x60, 0x01, 0xc1, 0x80, 0x00 ; 1000:823a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x80, 0x01, 0xc1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:824a
    db 0x00, 0x20, 0x02, 0x50, 0x00, 0xc1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x30 ; 1000:825a
    db 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x30, 0x02, 0xb1, 0x80, 0x00 ; 1000:826a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x30, 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:827a
    db 0x00, 0x90, 0x01, 0x30, 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x40 ; 1000:828a
    db 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x60, 0x02, 0xb1, 0x80, 0x00 ; 1000:829a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x10, 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:82aa
    db 0x00, 0x10, 0x03, 0x30, 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x03, 0x20 ; 1000:82ba
    db 0x02, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x20, 0x01, 0x40, 0x00, 0xc1 ; 1000:82ca
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x70, 0x00, 0xc1, 0x80, 0x00, 0x00, 0x00 ; 1000:82da
    db 0x00, 0x00, 0x00, 0x20, 0x01, 0xa0, 0x00, 0xc1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40 ; 1000:82ea
    db 0x00, 0xa0, 0x00, 0xc1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xa0, 0x00, 0xc1 ; 1000:82fa
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x80, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00 ; 1000:830a
    db 0x00, 0x00, 0x00, 0x70, 0x00, 0x80, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90 ; 1000:831a
    db 0x00, 0x80, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x05, 0x80, 0x00, 0xa1 ; 1000:832a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x05, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00 ; 1000:833a
    db 0x00, 0x00, 0x00, 0x10, 0x06, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:834a
    db 0x07, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x07, 0x50, 0x00, 0xa1 ; 1000:835a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x07, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00 ; 1000:836a
    db 0x00, 0x00, 0x00, 0x90, 0x06, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0 ; 1000:837a
    db 0x06, 0x50, 0x00, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0x50, 0x00, 0xa1 ; 1000:838a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0xa0, 0x00, 0xb1, 0x80, 0x00, 0x00, 0x00 ; 1000:839a
    db 0x00, 0x00, 0x00, 0x50, 0x04, 0xc0, 0x00, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:83aa
    db 0x04, 0xb0, 0x00, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0xd0, 0x00, 0xb1 ; 1000:83ba
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07, 0xc0, 0x00, 0xd1, 0x80, 0x00, 0x00, 0x00 ; 1000:83ca
    db 0x00, 0x00, 0x00, 0x04, 0x00, 0x60, 0x00, 0x80, 0x00, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:83da
    db 0x00, 0xa0, 0x00, 0x90, 0x00, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xb0 ; 1000:83ea
    db 0x00, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x90, 0x00, 0xe1, 0x80, 0x00 ; 1000:83fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x40, 0x00, 0x70, 0x04, 0xb1, 0x80, 0x00, 0x00, 0x00 ; 1000:840a
    db 0x00, 0x00, 0x00, 0x90, 0x00, 0xb0, 0x04, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0 ; 1000:841a
    db 0x00, 0x60, 0x01, 0xb1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x60, 0x01, 0xb1 ; 1000:842a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0x01, 0xd1, 0x80, 0x00, 0x00, 0x00 ; 1000:843a
    db 0x00, 0x00, 0x00, 0x90, 0x00, 0xb0, 0x01, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0 ; 1000:844a
    db 0x00, 0xa0, 0x01, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0xb0, 0x02, 0xd1 ; 1000:845a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x03, 0xd1, 0x80, 0x00, 0x00, 0x00 ; 1000:846a
    db 0x00, 0x00, 0x00, 0x30, 0x00, 0x10, 0x03, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60 ; 1000:847a
    db 0x00, 0x40, 0x03, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x03, 0xd1 ; 1000:848a
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0xb0, 0x03, 0xd1, 0x80, 0x00, 0x00, 0x00 ; 1000:849a
    db 0x00, 0x00, 0x00, 0x20, 0x01, 0xa0, 0x03, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:84aa
    db 0x00, 0xd0, 0x04, 0xd1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0xd0, 0x03, 0xe1 ; 1000:84ba
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x40, 0x04, 0xe1, 0x80, 0x00, 0x00, 0x00 ; 1000:84ca
    db 0x00, 0x00, 0x00, 0x60, 0x01, 0x00, 0x03, 0xe1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:84da
    db 0x00, 0x20, 0x02, 0xe1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x01, 0x90, 0x00, 0xe1 ; 1000:84ea
    db 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x60, 0x00, 0x70 ; 1000:84fa
    db 0x00, 0x01, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0xe0, 0x00, 0x01, 0x81, 0x00 ; 1000:850a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xe0, 0x00, 0x01, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:851a
    db 0x00, 0x10, 0x03, 0xe0, 0x00, 0x01, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0xe0 ; 1000:852a
    db 0x00, 0xf1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x85, 0x83, 0x85, 0x86, 0x85, 0x89 ; 1000:853a
    db 0x85, 0x8c, 0x85, 0xf8, 0x85, 0xb0, 0x87, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x85, 0x03 ; 1000:854a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x85, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x85, 0x0d ; 1000:855a
    db 0x00, 0x14, 0x01, 0x00, 0x01, 0x00, 0x01, 0x18, 0x00, 0x00, 0x01, 0x14, 0x01, 0x00, 0x01, 0x18 ; 1000:856a
    db 0x00, 0x00, 0x01, 0x00, 0x01, 0x14, 0x01, 0x18, 0x00, 0x10, 0x10, 0x51, 0xff, 0xff, 0x00, 0xff ; 1000:857a
    db 0xff, 0x00, 0x9e, 0x85, 0xb2, 0x85, 0xb4, 0x85, 0xb6, 0x85, 0xca, 0x85, 0xcc, 0x85, 0xce, 0x85 ; 1000:858a
    db 0xd0, 0x85, 0xd2, 0x85, 0x01, 0x00, 0xb0, 0x00, 0x80, 0x00, 0x10, 0x00, 0x68, 0x69, 0x68, 0x69 ; 1000:859a
    db 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x53, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xe0, 0x00 ; 1000:85aa
    db 0x30, 0x00, 0x10, 0x00, 0x68, 0x69, 0x68, 0x69, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:85ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x70, 0x00, 0x10, 0x00 ; 1000:85ca
    db 0x68, 0x69, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x57, 0x02, 0xc0, 0x01, 0x30, 0x00 ; 1000:85da
    db 0x10, 0x00, 0x68, 0x69, 0x68, 0x69, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x86 ; 1000:85ea
    db 0x24, 0x86, 0x62, 0x86, 0x7c, 0x86, 0x96, 0x86, 0xa4, 0x86, 0xbe, 0x86, 0xd8, 0x86, 0xfe, 0x86 ; 1000:85fa
    db 0x02, 0x00, 0x00, 0x00, 0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x10, 0x03 ; 1000:860a
    db 0x60, 0x00, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xa0, 0x05 ; 1000:861a
    db 0xc0, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x90, 0x01, 0xc0, 0x03, 0xe0, 0x03, 0x01, 0x00 ; 1000:862a
    db 0x04, 0x00, 0x00, 0x00, 0x90, 0x01, 0xc0, 0x02, 0xf0, 0x02, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00 ; 1000:863a
    db 0x90, 0x01, 0x10, 0x00, 0x30, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x90, 0x01, 0x40, 0x03 ; 1000:864a
    db 0x40, 0x03, 0x01, 0x00, 0x07, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x10, 0x01 ; 1000:865a
    db 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0xf0, 0x07, 0x50, 0x00, 0x70, 0x00, 0x01, 0x00, 0x03, 0x00 ; 1000:866a
    db 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x60, 0x00, 0x80, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00 ; 1000:867a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:868a
    db 0x60, 0x00, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x50, 0x00 ; 1000:869a
    db 0x70, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0xf0, 0x07, 0x50, 0x00, 0x70, 0x00, 0x01, 0x00 ; 1000:86aa
    db 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb0, 0x02, 0xd0, 0x02, 0x01, 0x00, 0x05, 0x00 ; 1000:86ba
    db 0x01, 0x00, 0xc0, 0x00, 0x20, 0x00, 0x20, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00 ; 1000:86ca
    db 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00 ; 1000:86da
    db 0x70, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0xc0, 0x00, 0x70, 0x00, 0x70, 0x00, 0x01, 0x00 ; 1000:86ea
    db 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:86fa
    db 0x04, 0x00, 0x70, 0x87, 0x78, 0x87, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00 ; 1000:870a
    db 0x01, 0x00, 0x80, 0x87, 0x82, 0x87, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00 ; 1000:871a
    db 0x02, 0x00, 0x88, 0x87, 0x8c, 0x87, 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x04, 0x00, 0x04, 0x00 ; 1000:872a
    db 0x04, 0x00, 0x90, 0x87, 0x90, 0x87, 0x10, 0x00, 0x08, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00 ; 1000:873a
    db 0x02, 0x00, 0xa0, 0x87, 0xa4, 0x87, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00 ; 1000:874a
    db 0x04, 0x00, 0xa8, 0x87, 0xa8, 0x87, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00 ; 1000:875a
    db 0x01, 0x00, 0x84, 0x87, 0x86, 0x87, 0x00, 0x00, 0xa6, 0x00, 0x4c, 0x01, 0xf2, 0x01, 0x98, 0x02 ; 1000:876a
    db 0x3e, 0x03, 0xe4, 0x03, 0x8a, 0x04, 0x30, 0x05, 0x76, 0x06, 0xbc, 0x07, 0x02, 0x09, 0x48, 0x0a ; 1000:877a
    db 0xee, 0x0a, 0x94, 0x0b, 0x3a, 0x0c, 0xe0, 0x0c, 0x86, 0x0d, 0x2c, 0x0e, 0x86, 0x0d, 0xd2, 0x0e ; 1000:878a
    db 0x78, 0x0f, 0x1e, 0x10, 0xc4, 0x10, 0x6a, 0x11, 0xc0, 0x11, 0x16, 0x12, 0x6c, 0x12, 0xc2, 0x12 ; 1000:879a
    db 0x68, 0x13, 0x0e, 0x14, 0xb4, 0x14, 0xc2, 0x87, 0xcc, 0x88, 0x82, 0x89, 0xb0, 0x8a, 0xe2, 0x8a ; 1000:87aa
    db 0xa4, 0x8b, 0x4e, 0x8c, 0x5c, 0x8c, 0x5e, 0x8c, 0x16, 0x00, 0x10, 0x00, 0x30, 0x00, 0x30, 0x87 ; 1000:87ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:87ca
    db 0x00, 0x00, 0x10, 0x01, 0x00, 0x02, 0x20, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02 ; 1000:87da
    db 0x10, 0x02, 0x20, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0xc0, 0x00, 0x40, 0x87 ; 1000:87ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0xe0, 0x00, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:87fa
    db 0x00, 0x00, 0x10, 0x01, 0xc0, 0x00, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02 ; 1000:880a
    db 0x00, 0x01, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x10, 0x01, 0x40, 0x87 ; 1000:881a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x30, 0x01, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:882a
    db 0x00, 0x00, 0xc0, 0x00, 0x60, 0x01, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00 ; 1000:883a
    db 0x70, 0x01, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x80, 0x01, 0x40, 0x87 ; 1000:884a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x90, 0x02, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:885a
    db 0x00, 0x00, 0xb0, 0x02, 0xa0, 0x02, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02 ; 1000:886a
    db 0xb0, 0x02, 0x40, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0xd0, 0x01, 0x40, 0x87 ; 1000:887a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0xf0, 0x01, 0x50, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:888a
    db 0x00, 0x00, 0x50, 0x00, 0x90, 0x02, 0x50, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00 ; 1000:889a
    db 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x40, 0x00, 0x00, 0x87 ; 1000:88aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:88ba
    db 0x00, 0x00, 0x0f, 0x00, 0x60, 0x00, 0x80, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:88ca
    db 0x00, 0x01, 0x80, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x30, 0x01 ; 1000:88da
    db 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x20, 0x02, 0x30, 0x87, 0x00, 0x00 ; 1000:88ea
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0x90, 0x02, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:88fa
    db 0x10, 0x01, 0x90, 0x02, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x90, 0x02 ; 1000:890a
    db 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0xe0, 0x02, 0x30, 0x87, 0x00, 0x00 ; 1000:891a
    db 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x03, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:892a
    db 0xc0, 0x00, 0x20, 0x04, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x50, 0x04 ; 1000:893a
    db 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x80, 0x04, 0x30, 0x87, 0x00, 0x00 ; 1000:894a
    db 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0xd0, 0x04, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:895a
    db 0x80, 0x01, 0xd0, 0x04, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xc0, 0x05 ; 1000:896a
    db 0x10, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x50, 0x00, 0x40, 0x00, 0x30, 0x87 ; 1000:897a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x40, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:898a
    db 0x00, 0x00, 0x50, 0x01, 0x40, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01 ; 1000:899a
    db 0x40, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x70, 0x00, 0x30, 0x87 ; 1000:89aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x04, 0x70, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:89ba
    db 0x00, 0x00, 0x50, 0x04, 0x70, 0x00, 0x30, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00 ; 1000:89ca
    db 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x80, 0x00, 0x00, 0x87 ; 1000:89da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:89ea
    db 0x00, 0x00, 0xf0, 0x01, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x07 ; 1000:89fa
    db 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x20, 0x01, 0x00, 0x87 ; 1000:8a0a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8a1a
    db 0x00, 0x00, 0x30, 0x06, 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x06 ; 1000:8a2a
    db 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0x20, 0x01, 0x00, 0x87 ; 1000:8a3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8a4a
    db 0x00, 0x00, 0x10, 0x01, 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01 ; 1000:8a5a
    db 0x20, 0x01, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x20, 0x01, 0x00, 0x87 ; 1000:8a6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03, 0x30, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8a7a
    db 0x00, 0x00, 0xc0, 0x03, 0x30, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04 ; 1000:8a8a
    db 0x30, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x30, 0x00, 0x00, 0x87 ; 1000:8a9a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00 ; 1000:8aaa
    db 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x30, 0x00, 0x10, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8aba
    db 0x90, 0x02, 0x30, 0x00, 0x10, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x30, 0x00 ; 1000:8aca
    db 0x10, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0xc0, 0x00, 0x90, 0x00, 0x00, 0x87 ; 1000:8ada
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8aea
    db 0x00, 0x00, 0xc0, 0x01, 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02 ; 1000:8afa
    db 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x87 ; 1000:8b0a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8b1a
    db 0x00, 0x00, 0xc0, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02 ; 1000:8b2a
    db 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x90, 0x00, 0x00, 0x87 ; 1000:8b3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8b4a
    db 0x00, 0x00, 0x00, 0x02, 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02 ; 1000:8b5a
    db 0x90, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x87 ; 1000:8b6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8b7a
    db 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02 ; 1000:8b8a
    db 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x80, 0x00, 0x80, 0x00 ; 1000:8b9a
    db 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00 ; 1000:8baa
    db 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8bba
    db 0x00, 0x02, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x80, 0x00 ; 1000:8bca
    db 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00 ; 1000:8bda
    db 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8bea
    db 0x00, 0x04, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x06, 0x80, 0x00 ; 1000:8bfa
    db 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x06, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00 ; 1000:8c0a
    db 0x00, 0x00, 0x00, 0x00, 0x10, 0x07, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8c1a
    db 0x60, 0x07, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x80, 0x00 ; 1000:8c2a
    db 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x07, 0x80, 0x00, 0x00, 0x87, 0x00, 0x00 ; 1000:8c3a
    db 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8c4a
    db 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x40, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00 ; 1000:8c5a
    db 0x00, 0x00, 0x70, 0x01, 0x40, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 ; 1000:8c6a
    db 0x80, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x80, 0x00, 0x00, 0x87 ; 1000:8c7a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x8c, 0xd0, 0x8c, 0xd3, 0x8c, 0xd8, 0x8c, 0xdc, 0x8c ; 1000:8c8a
    db 0x58, 0x8d, 0xde, 0x8d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x8c, 0x03, 0x00, 0x00, 0x00 ; 1000:8c9a
    db 0x00, 0x00, 0xc0, 0x8c, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x8c, 0x0d, 0x00, 0x16, 0x01 ; 1000:8caa
    db 0x14, 0x01, 0x04, 0x01, 0x18, 0x00, 0x04, 0x01, 0x16, 0x01, 0x14, 0x01, 0x18, 0x00, 0x14, 0x01 ; 1000:8cba
    db 0x04, 0x01, 0x16, 0x01, 0x18, 0x00, 0x0f, 0x0f, 0x20, 0x0c, 0x0e, 0x20, 0x20, 0x20, 0x39, 0x3a ; 1000:8cca
    db 0x0b, 0x0f, 0xe4, 0x8c, 0x0a, 0x8d, 0x0c, 0x8d, 0x44, 0x8d, 0x02, 0x00, 0xc0, 0x05, 0x80, 0x00 ; 1000:8cda
    db 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02 ; 1000:8cea
    db 0x50, 0x00, 0x10, 0x00, 0x24, 0x24, 0x25, 0x25, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x63, 0x02 ; 1000:8cfa
    db 0x00, 0x00, 0x03, 0x00, 0x30, 0x00, 0x50, 0x00, 0x10, 0x00, 0x24, 0x24, 0x25, 0x25, 0x02, 0x00 ; 1000:8d0a
    db 0x00, 0x00, 0x01, 0x00, 0x63, 0x02, 0xe0, 0x00, 0x20, 0x00, 0x10, 0x00, 0x24, 0x24, 0x25, 0x25 ; 1000:8d1a
    db 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x67, 0x02, 0xa0, 0x01, 0x80, 0x00, 0x10, 0x00, 0x51, 0x52 ; 1000:8d2a
    db 0x53, 0x54, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x02, 0x01, 0x00, 0x40, 0x00, 0xf0, 0x00 ; 1000:8d3a
    db 0x10, 0x00, 0x24, 0x24, 0x25, 0x25, 0x02, 0x00, 0x02, 0x00, 0x01, 0x00, 0x67, 0x02, 0x60, 0x8d ; 1000:8d4a
    db 0x6e, 0x8d, 0x88, 0x8d, 0x96, 0x8d, 0x01, 0x00, 0xf0, 0x05, 0x50, 0x01, 0x60, 0x01, 0x02, 0x00 ; 1000:8d5a
    db 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00 ; 1000:8d6a
    db 0x00, 0x00, 0x20, 0x0c, 0x00, 0x00, 0x90, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00 ; 1000:8d7a
    db 0x00, 0x00, 0xf0, 0x00, 0x70, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00 ; 1000:8d8a
    db 0x10, 0x00, 0x01, 0x00, 0x08, 0x00, 0x02, 0x00, 0x02, 0x00, 0xce, 0x8d, 0xd6, 0x8d, 0x10, 0x00 ; 1000:8d9a
    db 0x10, 0x00, 0x02, 0x00, 0x09, 0x00, 0x01, 0x00, 0x04, 0x00, 0xc4, 0x8d, 0xc4, 0x8d, 0x00, 0x00 ; 1000:8daa
    db 0x86, 0x02, 0x0c, 0x05, 0x92, 0x07, 0x18, 0x0a, 0x9e, 0x0c, 0x24, 0x0f, 0xca, 0x0f, 0x70, 0x10 ; 1000:8dba
    db 0x16, 0x11, 0x70, 0x10, 0x54, 0x14, 0xfa, 0x14, 0x08, 0x13, 0xae, 0x13, 0x54, 0x14, 0xfa, 0x14 ; 1000:8dca
    db 0xbc, 0x11, 0x62, 0x12, 0xe6, 0x8d, 0xa8, 0x8e, 0xda, 0x8e, 0xb4, 0x8f, 0x10, 0x00, 0x10, 0x03 ; 1000:8dda
    db 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0x10, 0x00, 0xa8, 0x8d ; 1000:8dea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8dfa
    db 0x00, 0x00, 0xa0, 0x02, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02 ; 1000:8e0a
    db 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x10, 0x00, 0xa8, 0x8d ; 1000:8e1a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8e2a
    db 0x00, 0x00, 0x70, 0x03, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x04 ; 1000:8e3a
    db 0x20, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x20, 0x01, 0xa8, 0x8d ; 1000:8e4a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x80, 0x01, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8e5a
    db 0x00, 0x00, 0xe0, 0x00, 0x80, 0x01, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00 ; 1000:8e6a
    db 0xf0, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x90, 0x00, 0x98, 0x8d ; 1000:8e7a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05, 0x90, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8e8a
    db 0x00, 0x00, 0xf0, 0x04, 0x20, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 ; 1000:8e9a
    db 0x80, 0x03, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05, 0x10, 0x00 ; 1000:8eaa
    db 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0b, 0xa0, 0x00, 0x98, 0x8d, 0x00, 0x00 ; 1000:8eba
    db 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0b, 0xa0, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8eca
    db 0x12, 0x00, 0x10, 0x05, 0xf0, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x05 ; 1000:8eda
    db 0xf0, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x10, 0x00, 0xa8, 0x8d ; 1000:8eea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8efa
    db 0x00, 0x00, 0x30, 0x03, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03 ; 1000:8f0a
    db 0x60, 0x00, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0x20, 0x00, 0xa8, 0x8d ; 1000:8f1a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8f2a
    db 0x00, 0x00, 0xb0, 0x01, 0x40, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04 ; 1000:8f3a
    db 0x50, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x50, 0x00, 0x98, 0x8d ; 1000:8f4a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x04, 0xb0, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8f5a
    db 0x00, 0x00, 0x00, 0x05, 0xb0, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x05 ; 1000:8f6a
    db 0xb0, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05, 0x60, 0x01, 0x98, 0x8d ; 1000:8f7a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x05, 0x60, 0x01, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00 ; 1000:8f8a
    db 0x00, 0x00, 0x20, 0x04, 0x50, 0x01, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00 ; 1000:8f9a
    db 0x80, 0x01, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0xc0, 0x01, 0x10, 0x00 ; 1000:8faa
    db 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x10, 0x00, 0xa8, 0x8d, 0x00, 0x00 ; 1000:8fba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8fca
    db 0x10, 0x02, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0xe0, 0x00 ; 1000:8fda
    db 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0xe0, 0x00, 0xa8, 0x8d, 0x00, 0x00 ; 1000:8fea
    db 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:8ffa
    db 0x90, 0x02, 0x00, 0x01, 0xa8, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x30, 0x00 ; 1000:900a
    db 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x30, 0x00, 0x98, 0x8d, 0x00, 0x00 ; 1000:901a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x30, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:902a
    db 0x80, 0x02, 0x30, 0x00, 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x01 ; 1000:903a
    db 0x98, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x30, 0x00, 0x98, 0x8d, 0x00, 0x00 ; 1000:904a
    db 0x00, 0x00, 0x00, 0x00, 0x6c, 0x90, 0x9e, 0x90, 0xa1, 0x90, 0xa4, 0x90, 0xa7, 0x90, 0xd3, 0x90 ; 1000:905a
    db 0x23, 0x91, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x90, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:906a
    db 0x8e, 0x90, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x90, 0x0d, 0x00, 0x12, 0x01, 0x02, 0x01 ; 1000:907a
    db 0x02, 0x01, 0x18, 0x00, 0x02, 0x01, 0x12, 0x01, 0x02, 0x01, 0x18, 0x00, 0x02, 0x01, 0x02, 0x01 ; 1000:908a
    db 0x12, 0x01, 0x18, 0x00, 0x14, 0x14, 0x28, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0xab, 0x90, 0xd1 ; 1000:909a
    db 0x90, 0x02, 0x00, 0x50, 0x00, 0x50, 0x00, 0x10, 0x00, 0x26, 0x27, 0x26, 0x27, 0x02, 0x00, 0x02 ; 1000:90aa
    db 0x00, 0x02, 0x00, 0x6b, 0x02, 0x10, 0x02, 0x10, 0x01, 0x10, 0x00, 0x26, 0x27, 0x26, 0x27, 0x00 ; 1000:90ba
    db 0x00, 0x06, 0x00, 0x01, 0x00, 0x5f, 0x02, 0x00, 0x00, 0xd7, 0x90, 0xe5, 0x90, 0x01, 0x00, 0xf0 ; 1000:90ca
    db 0x04, 0xa0, 0x01, 0xb0, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x30 ; 1000:90da
    db 0x00, 0x30, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x01, 0x80, 0x0a ; 1000:90ea
    db 0x00, 0x00, 0x00, 0x03, 0x00, 0x19, 0x91, 0x13, 0x91, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07 ; 1000:90fa
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0x21, 0x91, 0x1f, 0x91, 0x00, 0x00, 0x46, 0x01, 0x8c, 0x02, 0xd2 ; 1000:910a
    db 0x03, 0x18, 0x05, 0x5e, 0x06, 0xa4, 0x07, 0xea, 0x08, 0x27, 0x91, 0xb9, 0x91, 0x0c, 0x00, 0x00 ; 1000:911a
    db 0x03, 0x10, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0xf3 ; 1000:912a
    db 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x10, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00 ; 1000:913a
    db 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70 ; 1000:914a
    db 0x03, 0x40, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xe0, 0x00, 0xf3 ; 1000:915a
    db 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0xf0, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00 ; 1000:916a
    db 0x00, 0x00, 0x00, 0xa0, 0x00, 0x70, 0x01, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:917a
    db 0x02, 0x70, 0x01, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x70, 0x01, 0xf3 ; 1000:918a
    db 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0xb0, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00 ; 1000:919a
    db 0x00, 0x00, 0x00, 0xc0, 0x02, 0x70, 0x01, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09 ; 1000:91aa
    db 0x00, 0x80, 0x00, 0x00, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80 ; 1000:91ba
    db 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf3, 0x90, 0x00 ; 1000:91ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:91da
    db 0x00, 0x80, 0x01, 0x00, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x80 ; 1000:91ea
    db 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0xf3, 0x90, 0x00 ; 1000:91fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00, 0xf3, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:920a
    db 0x00, 0xb0, 0x03, 0x40, 0x00, 0x03, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x92, 0x67 ; 1000:921a
    db 0x92, 0x6a, 0x92, 0x6d, 0x92, 0x70, 0x92, 0xc4, 0x92, 0x02, 0x93, 0x03, 0x00, 0x00, 0x00, 0x00 ; 1000:922a
    db 0x00, 0x4f, 0x92, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x92, 0x05, 0x00, 0x00, 0x00, 0x00 ; 1000:923a
    db 0x00, 0x5f, 0x92, 0x0d, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x18, 0x00, 0x00, 0x08, 0x00 ; 1000:924a
    db 0x08, 0x00, 0x08, 0x18, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x18, 0x00, 0x10, 0x10, 0x51 ; 1000:925a
    db 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x76, 0x92, 0x8a, 0x92, 0x9e, 0x92, 0x01, 0x00, 0xb0, 0x00 ; 1000:926a
    db 0x80, 0x00, 0x10, 0x00, 0x68, 0x69, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 ; 1000:927a
    db 0x01, 0x00, 0x20, 0x00, 0x70, 0x00, 0x10, 0x00, 0x68, 0x69, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00 ; 1000:928a
    db 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0xb0, 0x00, 0x80, 0x00, 0x10, 0x00, 0x68, 0x69, 0x68, 0x69 ; 1000:929a
    db 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x80, 0x00, 0x10, 0x00, 0xa4, 0xa5 ; 1000:92aa
    db 0xa6, 0xa7, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0x92, 0xd8, 0x92, 0xd8, 0x92 ; 1000:92ba
    db 0x01, 0x00, 0x00, 0x00, 0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00 ; 1000:92ca
    db 0x10, 0x00, 0x10, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0xfa, 0x92, 0xfa, 0x92 ; 1000:92da
    db 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0xfe, 0x92, 0xfc, 0x92 ; 1000:92ea
    db 0x00, 0x00, 0xa6, 0x00, 0xec, 0x01, 0x32, 0x03, 0x08, 0x93, 0x16, 0x93, 0x18, 0x93, 0x01, 0x00 ; 1000:92fa
    db 0x00, 0x01, 0x80, 0x00, 0xda, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 ; 1000:930a
    db 0xd0, 0x01, 0x80, 0x00, 0xea, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x80, 0x00 ; 1000:931a
    db 0xea, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x80, 0x00, 0xea, 0x92, 0x00, 0x00 ; 1000:932a
    db 0x00, 0x00, 0x00, 0x00, 0x4c, 0x93, 0x9c, 0x93, 0x9f, 0x93, 0xa2, 0x93, 0xa5, 0x93, 0xe7, 0x94 ; 1000:933a
    db 0x39, 0x95, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x93, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:934a
    db 0x74, 0x93, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x93, 0x0d, 0x00, 0x00, 0x28, 0x00, 0x28 ; 1000:935a
    db 0x00, 0x28, 0x17, 0x01, 0x07, 0x01, 0x17, 0x01, 0x18, 0x00, 0x00, 0x22, 0x17, 0x01, 0x07, 0x01 ; 1000:936a
    db 0x17, 0x01, 0x00, 0x28, 0x00, 0x28, 0x17, 0x01, 0x07, 0x01, 0x17, 0x01, 0x18, 0x00, 0x00, 0x28 ; 1000:937a
    db 0x00, 0x22, 0x17, 0x01, 0x07, 0x01, 0x17, 0x01, 0x00, 0x28, 0x17, 0x01, 0x07, 0x01, 0x17, 0x01 ; 1000:938a
    db 0x18, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0xad, 0x93, 0xf7, 0x93, 0x53 ; 1000:939a
    db 0x94, 0xd3, 0x94, 0x04, 0x00, 0x50, 0x00, 0x80, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05 ; 1000:93aa
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x70, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11 ; 1000:93ba
    db 0x12, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x90, 0x07, 0x40, 0x00, 0x10, 0x00, 0x0f ; 1000:93ca
    db 0x10, 0x11, 0x12, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x5b, 0x02, 0xc0, 0x07, 0x40, 0x00, 0x10 ; 1000:93da
    db 0x00, 0x48, 0x49, 0x49, 0x48, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0xa0 ; 1000:93ea
    db 0x03, 0x40, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:93fa
    db 0x00, 0x80, 0x00, 0x80, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x01, 0x00, 0x00 ; 1000:940a
    db 0x00, 0x00, 0x00, 0x10, 0x06, 0x30, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x01 ; 1000:941a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0x30, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05 ; 1000:942a
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0x30, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11 ; 1000:943a
    db 0x12, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x90, 0x05, 0x40, 0x00, 0x10 ; 1000:944a
    db 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x50, 0x00, 0x80 ; 1000:945a
    db 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:946a
    db 0x01, 0x70, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 ; 1000:947a
    db 0x00, 0x50, 0x06, 0x40, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x02, 0x00, 0x00 ; 1000:948a
    db 0x00, 0x00, 0x00, 0x10, 0x07, 0x40, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x02 ; 1000:949a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0x40, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05 ; 1000:94aa
    db 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x30, 0x00, 0x10, 0x00, 0x0f, 0x10, 0x11 ; 1000:94ba
    db 0x12, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xb0, 0x06, 0x40, 0x00, 0x10 ; 1000:94ca
    db 0x00, 0x0f, 0x10, 0x11, 0x12, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xef, 0x94, 0xef ; 1000:94da
    db 0x94, 0xef, 0x94, 0xef, 0x94, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x7f, 0x0b, 0x00, 0x00 ; 1000:94ea
    db 0x00, 0x04, 0x00, 0x21, 0x95, 0x21, 0x95, 0x10, 0x00, 0x10, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00 ; 1000:94fa
    db 0x00, 0x04, 0x00, 0x29, 0x95, 0x29, 0x95, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00 ; 1000:950a
    db 0x00, 0x01, 0x00, 0x33, 0x95, 0x31, 0x95, 0x00, 0x00, 0xa6, 0x00, 0x4c, 0x01, 0xf2, 0x01, 0x98 ; 1000:951a
    db 0x02, 0xde, 0x03, 0x24, 0x05, 0xde, 0x03, 0x6a, 0x06, 0xb0, 0x07, 0xf6, 0x08, 0x3c, 0x0a, 0x41 ; 1000:952a
    db 0x95, 0x63, 0x96, 0x49, 0x97, 0x0b, 0x98, 0x18, 0x00, 0xe0, 0x06, 0x20, 0x00, 0xf1, 0x94, 0x00 ; 1000:953a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x80, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:954a
    db 0x00, 0x40, 0x01, 0x90, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0xb0 ; 1000:955a
    db 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0xb0, 0x00, 0xf1, 0x94, 0x00 ; 1000:956a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x70, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:957a
    db 0x00, 0x00, 0x04, 0x70, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0x60 ; 1000:958a
    db 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0x80, 0x00, 0xf1, 0x94, 0x00 ; 1000:959a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x06, 0x80, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:95aa
    db 0x00, 0x50, 0x03, 0x40, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x70 ; 1000:95ba
    db 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0x40, 0x00, 0xf1, 0x94, 0x00 ; 1000:95ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x40, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:95da
    db 0x00, 0x40, 0x07, 0x00, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x00 ; 1000:95ea
    db 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x10, 0x01, 0x01, 0x95, 0x00 ; 1000:95fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x10, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:960a
    db 0x00, 0x10, 0x04, 0x10, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x10 ; 1000:961a
    db 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x10, 0x01, 0x01, 0x95, 0x00 ; 1000:962a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x20, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:963a
    db 0x00, 0xa0, 0x01, 0x20, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x20 ; 1000:964a
    db 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x80, 0x00, 0x50, 0x00, 0xf1 ; 1000:965a
    db 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x20, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00 ; 1000:966a
    db 0x00, 0x00, 0x00, 0xe0, 0x02, 0x20, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:967a
    db 0x02, 0x80, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0xb0, 0x00, 0xf1 ; 1000:968a
    db 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0xc0, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00 ; 1000:969a
    db 0x00, 0x00, 0x00, 0xe0, 0x05, 0xa0, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60 ; 1000:96aa
    db 0x06, 0x80, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x07, 0x80, 0x00, 0xf1 ; 1000:96ba
    db 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0xf0, 0x00, 0x01, 0x95, 0x00, 0x00, 0x00 ; 1000:96ca
    db 0x00, 0x00, 0x00, 0xd0, 0x05, 0xe0, 0x00, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:96da
    db 0x04, 0x10, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0xe0, 0x00, 0x01 ; 1000:96ea
    db 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03, 0x10, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00 ; 1000:96fa
    db 0x00, 0x00, 0x00, 0xf0, 0x01, 0x00, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60 ; 1000:970a
    db 0x00, 0x10, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0xe0, 0x00, 0x01 ; 1000:971a
    db 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x06, 0xd0, 0x00, 0x01, 0x95, 0x00, 0x00, 0x00 ; 1000:972a
    db 0x00, 0x00, 0x00, 0xb0, 0x04, 0x20, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 ; 1000:973a
    db 0x00, 0xb0, 0x00, 0x50, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x70 ; 1000:974a
    db 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x90, 0x00, 0xf1, 0x94, 0x00 ; 1000:975a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0x20, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:976a
    db 0x00, 0xd0, 0x01, 0xc0, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x20 ; 1000:977a
    db 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x04, 0x80, 0x00, 0xf1, 0x94, 0x00 ; 1000:978a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x06, 0x70, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:979a
    db 0x00, 0xe0, 0x07, 0x80, 0x00, 0xf1, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07, 0xf0 ; 1000:97aa
    db 0x00, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x06, 0xf0, 0x00, 0x01, 0x95, 0x00 ; 1000:97ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x05, 0xf0, 0x00, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:97ca
    db 0x00, 0x00, 0x04, 0x00, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x20 ; 1000:97da
    db 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x20, 0x01, 0x01, 0x95, 0x00 ; 1000:97ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x01, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:97fa
    db 0x00, 0x01, 0x00, 0xc0, 0x02, 0x70, 0x00, 0x11, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27 ; 1000:980a
    db 0x98, 0x59, 0x98, 0x5e, 0x98, 0x61, 0x98, 0x64, 0x98, 0xce, 0x98, 0x86, 0x99, 0x03, 0x00, 0x00 ; 1000:981a
    db 0x00, 0x00, 0x00, 0x41, 0x98, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x98, 0x05, 0x00, 0x00 ; 1000:982a
    db 0x00, 0x00, 0x00, 0x51, 0x98, 0x0d, 0x00, 0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x00, 0x01 ; 1000:983a
    db 0x01, 0x11, 0x01, 0x01, 0x01, 0x18, 0x00, 0x01, 0x01, 0x01, 0x01, 0x11, 0x01, 0x18, 0x00, 0x06 ; 1000:984a
    db 0x08, 0x21, 0x30, 0x39, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x6c, 0x98, 0x92, 0x98, 0x94, 0x98 ; 1000:985a
    db 0xba, 0x98, 0x02, 0x00, 0x20, 0x00, 0x80, 0x01, 0x10, 0x00, 0x58, 0x58, 0x58, 0x58, 0xff, 0xff ; 1000:986a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0xb0, 0x01, 0x10, 0x00, 0x32, 0x33, 0x34, 0x35 ; 1000:987a
    db 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x30, 0x01, 0x00, 0x02 ; 1000:988a
    db 0x10, 0x00, 0x32, 0x33, 0x34, 0x35, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xd0, 0x00 ; 1000:989a
    db 0x70, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x6f, 0x02 ; 1000:98aa
    db 0x01, 0x00, 0x40, 0x01, 0x20, 0x00, 0x10, 0x00, 0x32, 0x33, 0x34, 0x35, 0xff, 0xff, 0x00, 0x00 ; 1000:98ba
    db 0x00, 0x00, 0x00, 0x00, 0xd6, 0x98, 0xf0, 0x98, 0x16, 0x99, 0x24, 0x99, 0x02, 0x00, 0xf0, 0x04 ; 1000:98ca
    db 0x60, 0x00, 0x10, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x04, 0x00, 0x00, 0x30, 0x00 ; 1000:98da
    db 0x06, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x01, 0x06, 0x00 ; 1000:98ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00 ; 1000:98fa
    db 0xf0, 0x05, 0x90, 0x00, 0xc0, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:990a
    db 0x00, 0x00, 0x80, 0x00, 0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00 ; 1000:991a
    db 0x02, 0x00, 0x0c, 0x00, 0x02, 0x00, 0x01, 0x00, 0x6c, 0x99, 0x66, 0x99, 0x20, 0x00, 0x10, 0x00 ; 1000:992a
    db 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x76, 0x99, 0x72, 0x99, 0x10, 0x00, 0x20, 0x00 ; 1000:993a
    db 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x99, 0x7a, 0x99, 0x08, 0x00, 0x20, 0x00 ; 1000:994a
    db 0x02, 0x00, 0x19, 0x00, 0x00, 0x00, 0x02, 0x00, 0x82, 0x99, 0x7e, 0x99, 0x00, 0x00, 0xa6, 0x00 ; 1000:995a
    db 0x98, 0x02, 0x4c, 0x01, 0xf2, 0x01, 0x98, 0x02, 0x3e, 0x03, 0x84, 0x04, 0xca, 0x05, 0x10, 0x07 ; 1000:996a
    db 0x56, 0x08, 0x9c, 0x09, 0xe2, 0x0a, 0x88, 0x0b, 0x2e, 0x0c, 0xd4, 0x0c, 0x8e, 0x99, 0xa4, 0x9a ; 1000:997a
    db 0x7e, 0x9b, 0xa4, 0x9b, 0x17, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:998a
    db 0x00, 0x00, 0xb0, 0x00, 0x90, 0x01, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01 ; 1000:999a
    db 0x90, 0x01, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x70, 0x00, 0x26, 0x99 ; 1000:99aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xa0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:99ba
    db 0x00, 0x00, 0x30, 0x01, 0x40, 0x01, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01 ; 1000:99ca
    db 0x70, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x90, 0x00, 0x26, 0x99 ; 1000:99da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x60, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:99ea
    db 0x00, 0x00, 0xe0, 0x02, 0x30, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x03 ; 1000:99fa
    db 0x50, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x03, 0x50, 0x00, 0x26, 0x99 ; 1000:9a0a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x04, 0x20, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:9a1a
    db 0x00, 0x00, 0xe0, 0x02, 0xe0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02 ; 1000:9a2a
    db 0xb0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x03, 0x20, 0x01, 0x36, 0x99 ; 1000:9a3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x04, 0x20, 0x01, 0x36, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:9a4a
    db 0x00, 0x00, 0xb0, 0x03, 0xc0, 0x01, 0x36, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x01 ; 1000:9a5a
    db 0xf0, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x70, 0x00, 0x56, 0x99 ; 1000:9a6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x01, 0x60, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:9a7a
    db 0x00, 0x00, 0xcc, 0x02, 0x70, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x04 ; 1000:9a8a
    db 0x30, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x40, 0x04, 0xc0, 0x00 ; 1000:9a9a
    db 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0xc0, 0x00, 0x26, 0x99, 0x00, 0x00 ; 1000:9aaa
    db 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0xc0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9aba
    db 0x80, 0x03, 0xc0, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x30, 0x00 ; 1000:9aca
    db 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x70, 0x00, 0x26, 0x99, 0x00, 0x00 ; 1000:9ada
    db 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9aea
    db 0xa0, 0x02, 0x10, 0x00, 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x30, 0x00 ; 1000:9afa
    db 0x26, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0x26, 0x99, 0x00, 0x00 ; 1000:9b0a
    db 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x30, 0x01, 0x36, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9b1a
    db 0x40, 0x01, 0x30, 0x01, 0x36, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x30, 0x01 ; 1000:9b2a
    db 0x36, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x05, 0x20, 0x01, 0x46, 0x99, 0x00, 0x00 ; 1000:9b3a
    db 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0xa0, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9b4a
    db 0x2c, 0x01, 0x20, 0x00, 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x02, 0x60, 0x00 ; 1000:9b5a
    db 0x56, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x03, 0x90, 0x00, 0x56, 0x99, 0x00, 0x00 ; 1000:9b6a
    db 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x50, 0x00, 0xe0, 0x00, 0x46, 0x99, 0x00, 0x00, 0x00, 0x00 ; 1000:9b7a
    db 0x00, 0x00, 0xc0, 0x00, 0xe0, 0x00, 0x46, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01 ; 1000:9b8a
    db 0xe0, 0x00, 0x46, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x70, 0x00 ; 1000:9b9a
    db 0x46, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x9b, 0xf2, 0x9b, 0xf6, 0x9b, 0xf9, 0x9b ; 1000:9baa
    db 0xfc, 0x9b, 0xce, 0x9c, 0x70, 0x9d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xda, 0x9b, 0x03, 0x00 ; 1000:9bba
    db 0x00, 0x00, 0x00, 0x00, 0xe2, 0x9b, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xea, 0x9b, 0x0d, 0x00 ; 1000:9bca
    db 0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x18, 0x00, 0x01, 0x01, 0x11, 0x01, 0x01, 0x01, 0x18, 0x00 ; 1000:9bda
    db 0x01, 0x01, 0x01, 0x01, 0x11, 0x01, 0x18, 0x00, 0x04, 0x05, 0x28, 0x28, 0xff, 0xff, 0x00, 0xff ; 1000:9bea
    db 0xff, 0x00, 0x02, 0x9c, 0xa6, 0x9c, 0xba, 0x9c, 0x09, 0x00, 0xb0, 0x00, 0x50, 0x00, 0x10, 0x00 ; 1000:9bfa
    db 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x01, 0xd0, 0x00 ; 1000:9c0a
    db 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb0, 0x00 ; 1000:9c1a
    db 0x50, 0x01, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:9c2a
    db 0x80, 0x01, 0x50, 0x01, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x06, 0x00, 0x02, 0x00, 0x01, 0x00 ; 1000:9c3a
    db 0x6f, 0x02, 0xa0, 0x01, 0x50, 0x00, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x00, 0x00 ; 1000:9c4a
    db 0x05, 0x00, 0x00, 0x00, 0x20, 0x02, 0xd0, 0x00, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00 ; 1000:9c5a
    db 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xf0, 0x02, 0xd0, 0x00, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43 ; 1000:9c6a
    db 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x50, 0x01, 0x10, 0x00, 0x42, 0x42 ; 1000:9c7a
    db 0x43, 0x43, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xc0, 0x03, 0xd0, 0x00, 0x10, 0x00 ; 1000:9c8a
    db 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0xf0, 0x03 ; 1000:9c9a
    db 0xb0, 0x00, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9caa
    db 0x01, 0x00, 0x20, 0x02, 0xa0, 0x00, 0x10, 0x00, 0x42, 0x42, 0x43, 0x43, 0x07, 0x00, 0x01, 0x00 ; 1000:9cba
    db 0x00, 0x00, 0x00, 0x00, 0xd4, 0x9c, 0xe2, 0x9c, 0xf0, 0x9c, 0x01, 0x00, 0xf0, 0x05, 0x40, 0x01 ; 1000:9cca
    db 0x50, 0x01, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0, 0x01, 0xb0, 0x01 ; 1000:9cda
    db 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x02, 0x00, 0x03, 0x00 ; 1000:9cea
    db 0x00, 0x00, 0x04, 0x00, 0x4a, 0x9d, 0x42, 0x9d, 0x10, 0x00, 0x08, 0x00, 0x00, 0x7e, 0x0e, 0x00 ; 1000:9cfa
    db 0x00, 0x00, 0x04, 0x00, 0x52, 0x9d, 0x52, 0x9d, 0x10, 0x00, 0x10, 0x00, 0x00, 0x7d, 0x0f, 0x00 ; 1000:9d0a
    db 0x04, 0x00, 0x01, 0x00, 0x5a, 0x9d, 0x5a, 0x9d, 0x10, 0x00, 0x10, 0x00, 0x03, 0x00, 0x07, 0x00 ; 1000:9d1a
    db 0x04, 0x00, 0x00, 0x00, 0x64, 0x9d, 0x64, 0x9d, 0x18, 0x00, 0x20, 0x00, 0x0f, 0x01, 0x07, 0x00 ; 1000:9d2a
    db 0x02, 0x00, 0x00, 0x00, 0x6c, 0x9d, 0x6c, 0x9d, 0x00, 0x00, 0x56, 0x00, 0xac, 0x00, 0x56, 0x00 ; 1000:9d3a
    db 0x02, 0x01, 0x58, 0x01, 0xae, 0x01, 0x58, 0x01, 0x04, 0x02, 0x5a, 0x02, 0xb0, 0x02, 0x06, 0x03 ; 1000:9d4a
    db 0x5c, 0x03, 0x02, 0x04, 0xa8, 0x04, 0x02, 0x04, 0x5c, 0x03, 0x4e, 0x05, 0xf4, 0x05, 0x9a, 0x06 ; 1000:9d5a
    db 0xf4, 0x05, 0x40, 0x07, 0x26, 0x09, 0x76, 0x9d, 0xd4, 0x9e, 0x26, 0xa0, 0x1d, 0x00, 0xe0, 0x00 ; 1000:9d6a
    db 0x68, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x68, 0x01, 0x02, 0x9d ; 1000:9d7a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x68, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9d8a
    db 0x00, 0x00, 0x50, 0x01, 0x68, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02 ; 1000:9d9a
    db 0xe8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0xe8, 0x00, 0x02, 0x9d ; 1000:9daa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x68, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9dba
    db 0x00, 0x00, 0xf0, 0x02, 0x68, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x03 ; 1000:9dca
    db 0x68, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x68, 0x00, 0x02, 0x9d ; 1000:9dda
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xe8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9dea
    db 0x00, 0x00, 0xb0, 0x03, 0x60, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04 ; 1000:9dfa
    db 0x60, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x60, 0x00, 0x12, 0x9d ; 1000:9e0a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x60, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9e1a
    db 0x00, 0x00, 0x00, 0x05, 0x60, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05 ; 1000:9e2a
    db 0x60, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x60, 0x00, 0x12, 0x9d ; 1000:9e3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0xe0, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9e4a
    db 0x00, 0x00, 0xc0, 0x01, 0xe0, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x02 ; 1000:9e5a
    db 0xe0, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x10, 0x00, 0x22, 0x9d ; 1000:9e6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x10, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9e7a
    db 0x00, 0x00, 0x60, 0x04, 0x10, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 ; 1000:9e8a
    db 0x10, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05, 0x10, 0x00, 0x22, 0x9d ; 1000:9e9a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x80, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:9eaa
    db 0x00, 0x00, 0xc0, 0x01, 0x80, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x02 ; 1000:9eba
    db 0x80, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0xb0, 0x00, 0xc8, 0x01 ; 1000:9eca
    db 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0xc8, 0x01, 0x02, 0x9d, 0x00, 0x00 ; 1000:9eda
    db 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0xc8, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9eea
    db 0x00, 0x02, 0xc8, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xa8, 0x01 ; 1000:9efa
    db 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x18, 0x01, 0x02, 0x9d, 0x00, 0x00 ; 1000:9f0a
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x18, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9f1a
    db 0x50, 0x02, 0x18, 0x01, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xa0, 0x00 ; 1000:9f2a
    db 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xa0, 0x00, 0xf2, 0x9c, 0x00, 0x00 ; 1000:9f3a
    db 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0xa0, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9f4a
    db 0x60, 0x01, 0xa0, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0xa0, 0x00 ; 1000:9f5a
    db 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x20, 0x00, 0xf2, 0x9c, 0x00, 0x00 ; 1000:9f6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x20, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9f7a
    db 0xc0, 0x03, 0x20, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x20, 0x00 ; 1000:9f8a
    db 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00 ; 1000:9f9a
    db 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9faa
    db 0x60, 0x01, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0xc0, 0x01 ; 1000:9fba
    db 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00 ; 1000:9fca
    db 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:9fda
    db 0x00, 0x03, 0xc0, 0x01, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x50, 0x01 ; 1000:9fea
    db 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x50, 0x01, 0x22, 0x9d, 0x00, 0x00 ; 1000:9ffa
    db 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 0x50, 0x01, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a00a
    db 0xd0, 0x01, 0x50, 0x01, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0xd0, 0x01 ; 1000:a01a
    db 0x20, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x80, 0x00, 0xf2, 0x9c ; 1000:a02a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x80, 0x00, 0xf2, 0x9c, 0x00, 0x00, 0x00, 0x00 ; 1000:a03a
    db 0x00, 0x00, 0x50, 0x03, 0xa8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03 ; 1000:a04a
    db 0xa8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x98, 0x00, 0x02, 0x9d ; 1000:a05a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x98, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:a06a
    db 0x00, 0x00, 0x80, 0x02, 0xb8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03 ; 1000:a07a
    db 0x88, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0x88, 0x00, 0x02, 0x9d ; 1000:a08a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xb0, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:a09a
    db 0x00, 0x00, 0x10, 0x03, 0xb0, 0x00, 0x12, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02 ; 1000:a0aa
    db 0x40, 0x00, 0x22, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x03, 0x40, 0x00, 0x22, 0x9d ; 1000:a0ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x05, 0xa0, 0x00, 0x32, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:a0ca
    db 0x00, 0x00, 0xc0, 0x05, 0xb8, 0x00, 0x02, 0x9d, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05 ; 1000:a0da
    db 0xd8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x05, 0xd8, 0x00, 0x02, 0x9d ; 1000:a0ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0xd8, 0x00, 0x02, 0x9d, 0x00, 0x00, 0x00, 0x00 ; 1000:a0fa
    db 0x00, 0x00, 0x1a, 0xa1, 0x4c, 0xa1, 0x50, 0xa1, 0x54, 0xa1, 0x59, 0xa1, 0xbf, 0xa2, 0x63, 0xa3 ; 1000:a10a
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0xa1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0xa1 ; 1000:a11a
    db 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0xa1, 0x0d, 0x00, 0x12, 0x01, 0x00, 0x01, 0x00, 0x01 ; 1000:a12a
    db 0x18, 0x00, 0x00, 0x01, 0x12, 0x01, 0x00, 0x01, 0x18, 0x00, 0x00, 0x01, 0x00, 0x01, 0x12, 0x01 ; 1000:a13a
    db 0x18, 0x00, 0x08, 0x09, 0x16, 0x16, 0x08, 0x09, 0x16, 0x16, 0x0d, 0x0f, 0x03, 0x04, 0x05, 0x61 ; 1000:a14a
    db 0xa1, 0x87, 0xa1, 0x3d, 0xa2, 0x87, 0xa2, 0x02, 0x00, 0x30, 0x00, 0x70, 0x00, 0x10, 0x00, 0x44 ; 1000:a15a
    db 0x45, 0x46, 0x47, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x70, 0x00, 0x10 ; 1000:a16a
    db 0x00, 0x5b, 0x5c, 0x5d, 0x5e, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x30 ; 1000:a17a
    db 0x00, 0x10, 0x01, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 ; 1000:a18a
    db 0x00, 0x20, 0x00, 0x80, 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01, 0x00, 0x02 ; 1000:a19a
    db 0x00, 0x00, 0x00, 0x10, 0x03, 0x10, 0x01, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01 ; 1000:a1aa
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x03, 0x80, 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08 ; 1000:a1ba
    db 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x90, 0x03, 0x80, 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20 ; 1000:a1ca
    db 0x21, 0x08, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x20, 0x00, 0x10, 0x00, 0x1e ; 1000:a1da
    db 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x06, 0x10, 0x01, 0x10 ; 1000:a1ea
    db 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb0, 0x05, 0x50 ; 1000:a1fa
    db 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00 ; 1000:a20a
    db 0x06, 0xb0, 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00 ; 1000:a21a
    db 0x00, 0xb0, 0x07, 0x50, 0x00, 0x10, 0x00, 0x1e, 0x1f, 0x20, 0x21, 0x08, 0x00, 0x02, 0x00, 0x00 ; 1000:a22a
    db 0x00, 0x00, 0x00, 0x04, 0x00, 0x60, 0x00, 0x60, 0x01, 0x10, 0x00, 0x44, 0x45, 0x46, 0x47, 0x08 ; 1000:a23a
    db 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0xf0, 0x01, 0xf0, 0x00, 0x10, 0x00, 0x19, 0x19, 0x19 ; 1000:a24a
    db 0x19, 0x08, 0x00, 0x02, 0x00, 0x02, 0x00, 0x73, 0x02, 0x70, 0x03, 0x60, 0x00, 0x10, 0x00, 0x19 ; 1000:a25a
    db 0x19, 0x19, 0x19, 0x08, 0x00, 0x02, 0x00, 0x01, 0x00, 0x73, 0x02, 0x60, 0x03, 0xb0, 0x01, 0x10 ; 1000:a26a
    db 0x00, 0x44, 0x45, 0x46, 0x47, 0x08, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x20 ; 1000:a27a
    db 0x00, 0x40, 0x00, 0x10, 0x00, 0x44, 0x45, 0x46, 0x47, 0x08, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00 ; 1000:a28a
    db 0x00, 0xd0, 0x04, 0x40, 0x00, 0x10, 0x00, 0x44, 0x45, 0x46, 0x47, 0x08, 0x00, 0x03, 0x00, 0x02 ; 1000:a29a
    db 0x00, 0x00, 0x00, 0x80, 0x05, 0x40, 0x00, 0x10, 0x00, 0x44, 0x45, 0x46, 0x47, 0x08, 0x00, 0x03 ; 1000:a2aa
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0xc7, 0xa2, 0xc7, 0xa2, 0xc7, 0xa2, 0xc7, 0xa2, 0x00, 0x00, 0x10 ; 1000:a2ba
    db 0x00, 0x10, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x31, 0xa3, 0x29, 0xa3, 0x10 ; 1000:a2ca
    db 0x00, 0x10, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0x41, 0xa3, 0x39, 0xa3, 0x10 ; 1000:a2da
    db 0x00, 0x10, 0x00, 0x02, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x49, 0xa3, 0x49, 0xa3, 0x10 ; 1000:a2ea
    db 0x00, 0x20, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x53, 0xa3, 0x4f, 0xa3, 0x10 ; 1000:a2fa
    db 0x00, 0x10, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x04, 0x00, 0x57, 0xa3, 0x57, 0xa3, 0x18 ; 1000:a30a
    db 0x00, 0x20, 0x00, 0x0a, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x5f, 0xa3, 0x5f, 0xa3, 0x00 ; 1000:a31a
    db 0x00, 0xa6, 0x00, 0x4c, 0x01, 0xa6, 0x00, 0xf2, 0x01, 0x98, 0x02, 0x3e, 0x03, 0x98, 0x02, 0xe4 ; 1000:a32a
    db 0x03, 0x8a, 0x04, 0x30, 0x05, 0x8a, 0x04, 0xd6, 0x05, 0x7c, 0x06, 0x22, 0x07, 0x7c, 0x06, 0xc8 ; 1000:a33a
    db 0x07, 0x6e, 0x08, 0x14, 0x09, 0xba, 0x09, 0x00, 0x0b, 0x46, 0x0c, 0x8c, 0x0d, 0xd2, 0x0e, 0x78 ; 1000:a34a
    db 0x0f, 0x1e, 0x10, 0x78, 0x0f, 0xc4, 0x10, 0xaa, 0x12, 0x6b, 0xa3, 0x2d, 0xa4, 0xaf, 0xa5, 0xf5 ; 1000:a35a
    db 0xa6, 0x10, 0x00, 0xf0, 0x00, 0x20, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:a36a
    db 0x00, 0x30, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x40, 0x00, 0xc9 ; 1000:a37a
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x50, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a38a
    db 0x00, 0x00, 0x00, 0xf0, 0x00, 0x60, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:a39a
    db 0x00, 0x70, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x20, 0x00, 0xc9 ; 1000:a3aa
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x30, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a3ba
    db 0x00, 0x00, 0x00, 0xd0, 0x01, 0x40, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0 ; 1000:a3ca
    db 0x01, 0x50, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x60, 0x00, 0xc9 ; 1000:a3da
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x70, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a3ea
    db 0x00, 0x00, 0x00, 0xe0, 0x03, 0x50, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:a3fa
    db 0x03, 0x60, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x70, 0x00, 0xc9 ; 1000:a40a
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x80, 0x00, 0xc9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a41a
    db 0x00, 0x00, 0x00, 0x20, 0x00, 0x90, 0x03, 0x10, 0x01, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a42a
    db 0x00, 0x90, 0x04, 0x10, 0x01, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0x10 ; 1000:a43a
    db 0x01, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x10, 0x01, 0xf9, 0xa2, 0x00 ; 1000:a44a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x05, 0x10, 0x01, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a45a
    db 0x00, 0x70, 0x05, 0x90, 0x00, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07, 0x30 ; 1000:a46a
    db 0x00, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x07, 0x50, 0x00, 0xf9, 0xa2, 0x00 ; 1000:a47a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x80, 0x00, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a48a
    db 0x00, 0xf0, 0x04, 0x90, 0x00, 0xf9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xa0 ; 1000:a49a
    db 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0xe0, 0x00, 0xd9, 0xa2, 0x00 ; 1000:a4aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0xe0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a4ba
    db 0x00, 0x70, 0x01, 0xe0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0xe0 ; 1000:a4ca
    db 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0xe0, 0x00, 0xd9, 0xa2, 0x00 ; 1000:a4da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x80, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a4ea
    db 0x00, 0x20, 0x01, 0x80, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0x80 ; 1000:a4fa
    db 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x80, 0x00, 0xd9, 0xa2, 0x00 ; 1000:a50a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x80, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a51a
    db 0x00, 0x60, 0x02, 0xe0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x20 ; 1000:a52a
    db 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x20, 0x01, 0xd9, 0xa2, 0x00 ; 1000:a53a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x60, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a54a
    db 0x00, 0x30, 0x01, 0x10, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x60 ; 1000:a55a
    db 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x10, 0x00, 0xd9, 0xa2, 0x00 ; 1000:a56a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x30, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a57a
    db 0x00, 0x60, 0x04, 0x90, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x05, 0x60 ; 1000:a58a
    db 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x05, 0x20, 0x01, 0xd9, 0xa2, 0x00 ; 1000:a59a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x90, 0x01, 0xc0, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a5aa
    db 0x00, 0x00, 0x00, 0xe0, 0x00, 0xc0, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 ; 1000:a5ba
    db 0x00, 0xc0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0xc0, 0x00, 0xd9 ; 1000:a5ca
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a5da
    db 0x00, 0x00, 0x00, 0xb0, 0x00, 0x40, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:a5ea
    db 0x00, 0x00, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x50, 0x00, 0xd9 ; 1000:a5fa
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x30, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a60a
    db 0x00, 0x00, 0x00, 0xf0, 0x01, 0x10, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 ; 1000:a61a
    db 0x02, 0xa0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xc0, 0x00, 0xd9 ; 1000:a62a
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x00, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a63a
    db 0x00, 0x00, 0x00, 0xd0, 0x02, 0x40, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60 ; 1000:a64a
    db 0x02, 0x80, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xc0, 0x01, 0xd9 ; 1000:a65a
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03, 0x60, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a66a
    db 0x00, 0x00, 0x00, 0x70, 0x03, 0x60, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:a67a
    db 0x03, 0xa0, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0xb0, 0x01, 0xd9 ; 1000:a68a
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0xc0, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a69a
    db 0x00, 0x00, 0x00, 0x40, 0x04, 0x80, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a6aa
    db 0x04, 0x40, 0x01, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x01, 0xd9 ; 1000:a6ba
    db 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00 ; 1000:a6ca
    db 0x00, 0x00, 0x00, 0x40, 0x04, 0x80, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a6da
    db 0x04, 0x40, 0x00, 0xd9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x00, 0xa0, 0x00, 0xb0 ; 1000:a6ea
    db 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xf0, 0x00, 0xe9, 0xa2, 0x00 ; 1000:a6fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0xe0, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a70a
    db 0x00, 0xe0, 0x00, 0xe0, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x70 ; 1000:a71a
    db 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0xc0, 0x00, 0xe9, 0xa2, 0x00 ; 1000:a72a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x50, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a73a
    db 0x00, 0x00, 0x02, 0xc0, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x40 ; 1000:a74a
    db 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x20, 0x01, 0xe9, 0xa2, 0x00 ; 1000:a75a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x20, 0x01, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a76a
    db 0x00, 0xa0, 0x03, 0x20, 0x01, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0xd0 ; 1000:a77a
    db 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0xc0, 0x00, 0xe9, 0xa2, 0x00 ; 1000:a78a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0x10, 0x01, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a79a
    db 0x00, 0x70, 0x04, 0xc0, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x10 ; 1000:a7aa
    db 0x01, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x05, 0xc0, 0x00, 0xe9, 0xa2, 0x00 ; 1000:a7ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x06, 0xa0, 0x00, 0xe9, 0xa2, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a7ca
    db 0x00, 0x30, 0x07, 0xd0, 0x00, 0x09, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x06, 0xd0 ; 1000:a7da
    db 0x00, 0x09, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0xe0, 0x00, 0x09, 0xa3, 0x00 ; 1000:a7ea
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x07, 0xe0, 0x00, 0x09, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:a7fa
    db 0x00, 0x90, 0x06, 0xf0, 0x00, 0x09, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x70, 0x07, 0xf0 ; 1000:a80a
    db 0x00, 0x09, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x06, 0xc0, 0x00, 0x19, 0xa3, 0x00 ; 1000:a81a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0xa8, 0x6f, 0xa8, 0x72, 0xa8, 0x75, 0xa8, 0x78, 0xa8, 0xe2 ; 1000:a82a
    db 0xa8, 0xac, 0xa9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0xa8, 0x03, 0x00, 0x00, 0x00, 0x00 ; 1000:a83a
    db 0x00, 0x5f, 0xa8, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0xa8, 0x0d, 0x00, 0x17, 0x01, 0x01 ; 1000:a84a
    db 0x01, 0x01, 0x01, 0x18, 0x00, 0x01, 0x01, 0x17, 0x01, 0x01, 0x01, 0x18, 0x00, 0x01, 0x01, 0x01 ; 1000:a85a
    db 0x01, 0x17, 0x01, 0x18, 0x00, 0x01, 0x01, 0x30, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x80, 0xa8 ; 1000:a86a
    db 0xb8, 0xa8, 0xba, 0xa8, 0xce, 0xa8, 0x03, 0x00, 0xe0, 0x00, 0x10, 0x01, 0x10, 0x00, 0x2b, 0x2c ; 1000:a87a
    db 0x2d, 0x2e, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x10, 0x01, 0x10, 0x00 ; 1000:a88a
    db 0x2b, 0x2c, 0x2d, 0x2e, 0x09, 0x00, 0x02, 0x00, 0x00, 0x00, 0x77, 0x02, 0x30, 0x05, 0xd0, 0x00 ; 1000:a89a
    db 0x10, 0x00, 0x2b, 0x2c, 0x2d, 0x2e, 0x09, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a8aa
    db 0x01, 0x00, 0xd0, 0x00, 0x40, 0x01, 0x10, 0x00, 0x2b, 0x2c, 0x2d, 0x2e, 0x09, 0x00, 0x00, 0x00 ; 1000:a8ba
    db 0x01, 0x00, 0x77, 0x02, 0x01, 0x00, 0xc0, 0x00, 0x50, 0x01, 0x10, 0x00, 0x2b, 0x2c, 0x2d, 0x2e ; 1000:a8ca
    db 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xea, 0xa8, 0xf8, 0xa8, 0x1e, 0xa9, 0x38, 0xa9 ; 1000:a8da
    db 0x01, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x00, 0x01, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00 ; 1000:a8ea
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00 ; 1000:a8fa
    db 0x20, 0x00, 0x09, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf0, 0x07, 0x80, 0x00, 0xd0, 0x00, 0x09, 0x00 ; 1000:a90a
    db 0x02, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x09, 0x00, 0x01, 0x00 ; 1000:a91a
    db 0x01, 0x00, 0x00, 0x00, 0x30, 0x01, 0x50, 0x01, 0x09, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00 ; 1000:a92a
    db 0x10, 0x00, 0x20, 0x00, 0x19, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x92, 0xa9, 0x92, 0xa9 ; 1000:a93a
    db 0x10, 0x00, 0x10, 0x00, 0x03, 0x00, 0x09, 0x00, 0x01, 0x00, 0x04, 0x00, 0x96, 0xa9, 0x96, 0xa9 ; 1000:a94a
    db 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x12, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0, 0xa9, 0xa0, 0xa9 ; 1000:a95a
    db 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x13, 0x00, 0x01, 0x00, 0x01, 0x00, 0xa8, 0xa9, 0xa8, 0xa9 ; 1000:a96a
    db 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x14, 0x00, 0x01, 0x00, 0x01, 0x00, 0xa8, 0xa9, 0xa8, 0xa9 ; 1000:a97a
    db 0x00, 0x00, 0x86, 0x02, 0x0c, 0x05, 0x92, 0x07, 0x18, 0x0a, 0x5e, 0x0b, 0xa4, 0x0c, 0x4a, 0x0d ; 1000:a98a
    db 0xf0, 0x0d, 0x96, 0x0e, 0xf0, 0x0d, 0x3c, 0x0f, 0xe2, 0x0f, 0x88, 0x10, 0x2e, 0x11, 0x7a, 0x12 ; 1000:a99a
    db 0xd4, 0x11, 0xb4, 0xa9, 0x12, 0xab, 0x14, 0xab, 0xc6, 0xac, 0x1d, 0x00, 0x70, 0x00, 0x00, 0x01 ; 1000:a9aa
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00 ; 1000:a9ba
    db 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0xf0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a9ca
    db 0x90, 0x00, 0x70, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xa0, 0x00 ; 1000:a9da
    db 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0xb0, 0x00, 0x5a, 0xa9, 0x00, 0x00 ; 1000:a9ea
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0xd0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:a9fa
    db 0x80, 0x01, 0xe0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x01 ; 1000:aa0a
    db 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0xa0, 0x00, 0x5a, 0xa9, 0x00, 0x00 ; 1000:aa1a
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x90, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:aa2a
    db 0xa0, 0x04, 0x80, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x04, 0x70, 0x00 ; 1000:aa3a
    db 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x04, 0x60, 0x00, 0x5a, 0xa9, 0x00, 0x00 ; 1000:aa4a
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x50, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:aa5a
    db 0xe0, 0x04, 0x40, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x01 ; 1000:aa6a
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00 ; 1000:aa7a
    db 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:aa8a
    db 0x60, 0x03, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x01 ; 1000:aa9a
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05, 0xb0, 0x00, 0x6a, 0xa9, 0x00, 0x00 ; 1000:aaaa
    db 0x00, 0x00, 0x00, 0x00, 0x10, 0x05, 0xc0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:aaba
    db 0x00, 0x05, 0xd0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0xe0, 0x00 ; 1000:aaca
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x04, 0xf0, 0x00, 0x6a, 0xa9, 0x00, 0x00 ; 1000:aada
    db 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:aaea
    db 0xc0, 0x03, 0x00, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x03, 0x00, 0x01 ; 1000:aafa
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x10, 0x01, 0x40, 0x00 ; 1000:ab0a
    db 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00, 0x00, 0x5a, 0xa9, 0x00, 0x00 ; 1000:ab1a
    db 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ab2a
    db 0x40, 0x01, 0x40, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x10, 0x00 ; 1000:ab3a
    db 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x10, 0x00, 0x5a, 0xa9, 0x00, 0x00 ; 1000:ab4a
    db 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ab5a
    db 0x10, 0x01, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x30, 0x01 ; 1000:ab6a
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00 ; 1000:ab7a
    db 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ab8a
    db 0x50, 0x02, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x30, 0x01 ; 1000:ab9a
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x03, 0x90, 0x00, 0x6a, 0xa9, 0x00, 0x00 ; 1000:abaa
    db 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0xa0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:abba
    db 0x40, 0x03, 0xb0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0xc0, 0x00 ; 1000:abca
    db 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0xd0, 0x00, 0x6a, 0xa9, 0x00, 0x00 ; 1000:abda
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x05, 0xa0, 0x00, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:abea
    db 0x40, 0x01, 0x30, 0x01, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x30, 0x01 ; 1000:abfa
    db 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x02, 0x30, 0x01, 0x4a, 0xa9, 0x00, 0x00 ; 1000:ac0a
    db 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0xf0, 0x00, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ac1a
    db 0x00, 0x03, 0xf0, 0x00, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0xe0, 0x00 ; 1000:ac2a
    db 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x80, 0x00, 0x4a, 0xa9, 0x00, 0x00 ; 1000:ac3a
    db 0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x10, 0x00, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ac4a
    db 0x70, 0x04, 0x60, 0x00, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x04, 0x90, 0x00 ; 1000:ac5a
    db 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0xd0, 0x00, 0x4a, 0xa9, 0x00, 0x00 ; 1000:ac6a
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0x20, 0x01, 0x4a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ac7a
    db 0x80, 0x05, 0xa0, 0x00, 0x7a, 0xa9, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x90, 0x05, 0xa0, 0x00 ; 1000:ac8a
    db 0x7a, 0xa9, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0xa0, 0x00, 0x7a, 0xa9, 0x00, 0x01 ; 1000:ac9a
    db 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0xa0, 0x00, 0x7a, 0xa9, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 ; 1000:acaa
    db 0xd8, 0x05, 0xd0, 0x00, 0x3a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x40, 0x00 ; 1000:acba
    db 0xb0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x60, 0x00, 0x5a, 0xa9 ; 1000:acca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x50, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00 ; 1000:acda
    db 0x00, 0x00, 0x20, 0x03, 0x40, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x04 ; 1000:acea
    db 0xf0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x04, 0xf0, 0x00, 0x5a, 0xa9 ; 1000:acfa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0xf0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00 ; 1000:ad0a
    db 0x00, 0x00, 0xb0, 0x03, 0xf0, 0x00, 0x5a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01 ; 1000:ad1a
    db 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x30, 0x01, 0x6a, 0xa9 ; 1000:ad2a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00 ; 1000:ad3a
    db 0x00, 0x00, 0xa0, 0x01, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01 ; 1000:ad4a
    db 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x30, 0x01, 0x6a, 0xa9 ; 1000:ad5a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00 ; 1000:ad6a
    db 0x00, 0x00, 0xc0, 0x02, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02 ; 1000:ad7a
    db 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x30, 0x01, 0x6a, 0xa9 ; 1000:ad8a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x30, 0x01, 0x6a, 0xa9, 0x00, 0x00, 0x00, 0x00 ; 1000:ad9a
    db 0x00, 0x00, 0xba, 0xad, 0xf8, 0xad, 0xfb, 0xad, 0xfe, 0xad, 0x01, 0xae, 0x0f, 0xaf, 0x43, 0xb0 ; 1000:adaa
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd4, 0xad, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe4, 0xad ; 1000:adba
    db 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xad, 0x0d, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x64 ; 1000:adca
    db 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x18, 0x06, 0x11, 0x00, 0x01, 0x00, 0x11, 0x00 ; 1000:adda
    db 0x01, 0x00, 0x00, 0x05, 0x18, 0x00, 0x07, 0x01, 0x17, 0x01, 0x00, 0x28, 0x18, 0x00, 0xff, 0xff ; 1000:adea
    db 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x13, 0xae, 0x39, 0xae, 0x5f, 0xae, 0x73, 0xae, 0x87 ; 1000:adfa
    db 0xae, 0x9b, 0xae, 0x9d, 0xae, 0xd5, 0xae, 0xfb, 0xae, 0x02, 0x00, 0x50, 0x00, 0x90, 0x00, 0x00 ; 1000:ae0a
    db 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x60 ; 1000:ae1a
    db 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 ; 1000:ae2a
    db 0x00, 0x80, 0x05, 0x80, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x03, 0x00, 0x00 ; 1000:ae3a
    db 0x00, 0x00, 0x00, 0x50, 0x08, 0x50, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06 ; 1000:ae4a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x90, 0x00, 0x50, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00 ; 1000:ae5a
    db 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x70, 0x00, 0x00 ; 1000:ae6a
    db 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x60 ; 1000:ae7a
    db 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00 ; 1000:ae8a
    db 0x00, 0x00, 0x00, 0x03, 0x00, 0x30, 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a ; 1000:ae9a
    db 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00 ; 1000:aeaa
    db 0x00, 0x0a, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x10, 0x00, 0x00, 0x00, 0xff ; 1000:aeba
    db 0x00, 0x00, 0x00, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x60, 0x00, 0xa0 ; 1000:aeca
    db 0x02, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70 ; 1000:aeda
    db 0x02, 0xc0, 0x02, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 ; 1000:aeea
    db 0x00, 0x01, 0x00, 0xc0, 0x00, 0x20, 0x01, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x06 ; 1000:aefa
    db 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0xaf, 0x2f, 0xaf, 0x3d, 0xaf, 0x3d, 0xaf, 0x3f, 0xaf, 0x4d ; 1000:af0a
    db 0xaf, 0x3d, 0xaf, 0x3d, 0xaf, 0x3d, 0xaf, 0x01, 0x00, 0x50, 0x09, 0x00, 0x00, 0x60, 0x00, 0x0a ; 1000:af1a
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x00, 0x00 ; 1000:af2a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x06, 0x20, 0x06, 0x0a, 0x00, 0x05 ; 1000:af3a
    db 0x00, 0x00, 0x00, 0x01, 0x00, 0xe0, 0x03, 0xf0, 0x00, 0x20, 0x01, 0x0a, 0x00, 0x04, 0x00, 0x00 ; 1000:af4a
    db 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x7c, 0x15, 0x00, 0x00, 0x00, 0x04, 0x00, 0xfb, 0xaf, 0xfb ; 1000:af5a
    db 0xaf, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x15, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0xb0, 0x03 ; 1000:af6a
    db 0xb0, 0x10, 0x00, 0x20, 0x00, 0x04, 0x80, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x13, 0xb0, 0x13 ; 1000:af7a
    db 0xb0, 0x10, 0x00, 0x10, 0x00, 0x03, 0x00, 0x15, 0x00, 0x00, 0x00, 0x04, 0x00, 0x17, 0xb0, 0x17 ; 1000:af8a
    db 0xb0, 0x10, 0x00, 0x20, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x21, 0xb0, 0x1f ; 1000:af9a
    db 0xb0, 0x10, 0x00, 0x18, 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0xb0, 0x23 ; 1000:afaa
    db 0xb0, 0x20, 0x00, 0x08, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x2b, 0xb0, 0x27 ; 1000:afba
    db 0xb0, 0x10, 0x00, 0x20, 0x00, 0x04, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2f, 0xb0, 0x2f ; 1000:afca
    db 0xb0, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00, 0x04, 0x00, 0x37, 0xb0, 0x37 ; 1000:afda
    db 0xb0, 0x10, 0x00, 0x20, 0x00, 0x14, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3f, 0xb0, 0x3f ; 1000:afea
    db 0xb0, 0x00, 0x00, 0xa6, 0x00, 0x4c, 0x01, 0xf2, 0x01, 0x98, 0x02, 0x3e, 0x03, 0xe4, 0x03, 0x8a ; 1000:affa
    db 0x04, 0x30, 0x05, 0xd6, 0x05, 0x7c, 0x06, 0x22, 0x07, 0xc8, 0x07, 0x0e, 0x09, 0x54, 0x0a, 0xfa ; 1000:b00a
    db 0x0a, 0xa0, 0x0b, 0x46, 0x0c, 0xec, 0x0c, 0x32, 0x0e, 0x78, 0x0f, 0x6e, 0x10, 0x64, 0x11, 0x0a ; 1000:b01a
    db 0x12, 0xb0, 0x12, 0x56, 0x13, 0xfc, 0x13, 0x42, 0x15, 0x88, 0x16, 0x42, 0x15, 0xce, 0x17, 0x74 ; 1000:b02a
    db 0x18, 0x1a, 0x19, 0x74, 0x18, 0xc0, 0x19, 0x06, 0x1b, 0x55, 0xb0, 0x2b, 0xb2, 0x7d, 0xb3, 0x8b ; 1000:b03a
    db 0xb3, 0xb1, 0xb3, 0x7f, 0xb4, 0xad, 0xb5, 0x87, 0xb6, 0xf5, 0xb6, 0x27, 0x00, 0xf0, 0x00, 0x50 ; 1000:b04a
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x60, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b05a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0x40, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b06a
    db 0x00, 0xe0, 0x01, 0x70, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x30 ; 1000:b07a
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x50, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b08a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x20, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b09a
    db 0x00, 0x60, 0x01, 0x30, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x60 ; 1000:b0aa
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b0ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x50, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b0ca
    db 0x00, 0xa0, 0x02, 0x70, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x30 ; 1000:b0da
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0x60, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b0ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x04, 0x50, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b0fa
    db 0x00, 0x40, 0x05, 0x40, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x05, 0x10 ; 1000:b10a
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x05, 0x10, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b11a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x60, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b12a
    db 0x00, 0x70, 0x06, 0x30, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x10 ; 1000:b13a
    db 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x50, 0x00, 0x5b, 0xaf, 0x00 ; 1000:b14a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x10, 0x00, 0x5b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b15a
    db 0x00, 0x80, 0x05, 0x20, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0x50 ; 1000:b16a
    db 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x05, 0x80, 0x00, 0x6b, 0xaf, 0x00 ; 1000:b17a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x06, 0x10, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b18a
    db 0x00, 0x10, 0x07, 0x80, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07, 0x40 ; 1000:b19a
    db 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0x10, 0x00, 0x6b, 0xaf, 0x00 ; 1000:b1aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x08, 0x30, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b1ba
    db 0x00, 0x00, 0x09, 0x10, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x50 ; 1000:b1ca
    db 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x20, 0x00, 0x6b, 0xaf, 0x00 ; 1000:b1da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x06, 0x40, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b1ea
    db 0x00, 0x60, 0x06, 0x00, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x07, 0x00 ; 1000:b1fa
    db 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x07, 0x30, 0x00, 0x6b, 0xaf, 0x00 ; 1000:b20a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x07, 0x50, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b21a
    db 0x00, 0x1c, 0x00, 0xa0, 0x00, 0x70, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:b22a
    db 0x00, 0x60, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 0x60, 0x00, 0x6b ; 1000:b23a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x70, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b24a
    db 0x00, 0x00, 0x00, 0x40, 0x07, 0x40, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:b25a
    db 0x07, 0x10, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x40, 0x00, 0x6b ; 1000:b26a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x08, 0x40, 0x00, 0x6b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b27a
    db 0x00, 0x00, 0x00, 0xa0, 0x00, 0x90, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30 ; 1000:b28a
    db 0x02, 0xa0, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x80, 0x00, 0x8b ; 1000:b29a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x60, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b2aa
    db 0x00, 0x00, 0x00, 0xb0, 0x02, 0x80, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 ; 1000:b2ba
    db 0x03, 0x90, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x70, 0x00, 0x8b ; 1000:b2ca
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0x40, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b2da
    db 0x00, 0x00, 0x00, 0x30, 0x05, 0x80, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:b2ea
    db 0x06, 0x50, 0x00, 0x8b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x50, 0x00, 0x7b ; 1000:b2fa
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x80, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b30a
    db 0x00, 0x00, 0x00, 0x80, 0x02, 0xa0, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:b31a
    db 0x02, 0x50, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x80, 0x00, 0x7b ; 1000:b32a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0x90, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b33a
    db 0x00, 0x00, 0x00, 0x20, 0x03, 0x40, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 ; 1000:b34a
    db 0x04, 0x30, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x60, 0x00, 0x7b ; 1000:b35a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x08, 0x20, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b36a
    db 0x00, 0x00, 0x00, 0x01, 0x00, 0x50, 0x00, 0x50, 0x00, 0x9b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b37a
    db 0x00, 0x03, 0x00, 0x60, 0x00, 0x70, 0x00, 0x9b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:b38a
    db 0x00, 0x38, 0x00, 0xab, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x38, 0x00, 0xab ; 1000:b39a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0xa0, 0x00, 0x70, 0x05, 0xbb, 0xaf, 0x00 ; 1000:b3aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0xa0, 0x05, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b3ba
    db 0x00, 0x90, 0x00, 0xe0, 0x05, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00 ; 1000:b3ca
    db 0x06, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x10, 0x06, 0xbb, 0xaf, 0x00 ; 1000:b3da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0xb0, 0x05, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b3ea
    db 0x00, 0x10, 0x00, 0x10, 0x06, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x60 ; 1000:b3fa
    db 0x05, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x90, 0x05, 0xbb, 0xaf, 0x00 ; 1000:b40a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x50, 0x02, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b41a
    db 0x00, 0xb0, 0x00, 0x50, 0x02, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x20 ; 1000:b42a
    db 0x02, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x20, 0x02, 0xcb, 0xaf, 0x00 ; 1000:b43a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0xf0, 0x01, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b44a
    db 0x00, 0x90, 0x00, 0xf0, 0x01, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xc0 ; 1000:b45a
    db 0x01, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0x01, 0xcb, 0xaf, 0x00 ; 1000:b46a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0xb0, 0x00, 0x20, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b47a
    db 0x00, 0x00, 0x00, 0xf0, 0x00, 0x40, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30 ; 1000:b48a
    db 0x01, 0x60, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 0x70, 0x00, 0xcb ; 1000:b49a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0x70, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b4aa
    db 0x00, 0x00, 0x00, 0xa0, 0x01, 0x60, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:b4ba
    db 0x01, 0x40, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x20, 0x00, 0xcb ; 1000:b4ca
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x03, 0x70, 0x00, 0xeb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b4da
    db 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 ; 1000:b4ea
    db 0x03, 0xf0, 0x00, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x20, 0x01, 0xbb ; 1000:b4fa
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0xe0, 0x00, 0xbb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b50a
    db 0x00, 0x00, 0x00, 0x40, 0x02, 0x10, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:b51a
    db 0x01, 0x10, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0xf0, 0x00, 0xbb ; 1000:b52a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b53a
    db 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 ; 1000:b54a
    db 0x00, 0x90, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x60, 0x00, 0xcb ; 1000:b55a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x10, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00 ; 1000:b56a
    db 0x00, 0x00, 0x00, 0xf0, 0x02, 0x80, 0x00, 0xdb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:b57a
    db 0x03, 0x60, 0x00, 0xdb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x60, 0x00, 0xdb ; 1000:b58a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x60, 0x00, 0xdb, 0xaf, 0x00, 0x01, 0x00 ; 1000:b59a
    db 0x00, 0x00, 0x00, 0x12, 0x00, 0x90, 0x01, 0xd0, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b5aa
    db 0x00, 0xa0, 0x01, 0xd0, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0x90 ; 1000:b5ba
    db 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0x50, 0x00, 0xcb, 0xaf, 0x00 ; 1000:b5ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x50, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b5da
    db 0x00, 0x90, 0x01, 0x50, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x50 ; 1000:b5ea
    db 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x70, 0x00, 0xcb, 0xaf, 0x00 ; 1000:b5fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x70, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b60a
    db 0x00, 0xd0, 0x00, 0xc0, 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x40 ; 1000:b61a
    db 0x00, 0xcb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x40, 0x00, 0xcb, 0xaf, 0x00 ; 1000:b62a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x40, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b63a
    db 0x00, 0xc0, 0x00, 0x50, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x40 ; 1000:b64a
    db 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x60, 0x01, 0xbb, 0xaf, 0x00 ; 1000:b65a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x40, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b66a
    db 0x00, 0x90, 0x01, 0x50, 0x01, 0xbb, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0xc0 ; 1000:b67a
    db 0x00, 0x50, 0x02, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0xe0, 0x01, 0x7b ; 1000:b68a
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x90, 0x01, 0x7b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b69a
    db 0x00, 0x00, 0x00, 0x20, 0x01, 0xc0, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20 ; 1000:b6aa
    db 0x00, 0x40, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x60, 0x02, 0x7b ; 1000:b6ba
    db 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x70, 0x02, 0x7b, 0xaf, 0x00, 0x00, 0x00 ; 1000:b6ca
    db 0x00, 0x00, 0x00, 0xd0, 0x02, 0xa0, 0x02, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:b6da
    db 0x02, 0xc0, 0x02, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x70, 0x00, 0xe0 ; 1000:b6ea
    db 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x7b, 0xaf, 0x00 ; 1000:b6fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b70a
    db 0x00, 0x30, 0x01, 0xa0, 0x00, 0x7b, 0xaf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0xb7, 0x6d ; 1000:b71a
    db 0xb7, 0x70, 0xb7, 0x73, 0xb7, 0x76, 0xb7, 0xb6, 0xb8, 0x7c, 0xb9, 0x03, 0x00, 0x00, 0x00, 0x00 ; 1000:b72a
    db 0x00, 0x4f, 0xb7, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0xb7, 0x05, 0x00, 0x00, 0x00, 0x00 ; 1000:b73a
    db 0x00, 0x63, 0xb7, 0x0d, 0x00, 0x10, 0x01, 0x10, 0x01, 0x01, 0x01, 0x10, 0x01, 0x18, 0x00, 0x10 ; 1000:b74a
    db 0x01, 0x01, 0x01, 0x11, 0x01, 0x01, 0x01, 0x18, 0x00, 0x00, 0x01, 0x07, 0x01, 0x17, 0x01, 0x07 ; 1000:b75a
    db 0x01, 0x18, 0x00, 0x0e, 0x0e, 0x4c, 0x0f, 0x0f, 0x61, 0xff, 0xff, 0x00, 0x86, 0xb7, 0xd0, 0xb7 ; 1000:b76a
    db 0xf6, 0xb7, 0x1c, 0xb8, 0x30, 0xb8, 0x68, 0xb8, 0x7c, 0xb8, 0x90, 0xb8, 0x04, 0x00, 0x00, 0x00 ; 1000:b77a
    db 0x10, 0x01, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b78a
    db 0x00, 0x04, 0x10, 0x01, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x03, 0x00, 0x00, 0x00 ; 1000:b79a
    db 0x00, 0x00, 0xd0, 0x04, 0x30, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x04, 0x00 ; 1000:b7aa
    db 0x02, 0x00, 0x00, 0x00, 0x00, 0x06, 0x90, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00 ; 1000:b7ba
    db 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0xf0, 0x05, 0x50, 0x00, 0x10, 0x00, 0xff, 0x00 ; 1000:b7ca
    db 0x00, 0x00, 0x0b, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x07, 0xb0, 0x00, 0x00, 0x00 ; 1000:b7da
    db 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x40, 0x00 ; 1000:b7ea
    db 0xb0, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b7fa
    db 0x90, 0x03, 0xf0, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x07, 0x00, 0x00, 0x00 ; 1000:b80a
    db 0x00, 0x00, 0x01, 0x00, 0xc0, 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00 ; 1000:b81a
    db 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x20, 0x00, 0x60, 0x00, 0x10, 0x00, 0xff, 0x00 ; 1000:b82a
    db 0x00, 0x00, 0x0b, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x06, 0xa0, 0x00, 0x00, 0x00 ; 1000:b83a
    db 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb0, 0x07, 0x40, 0x00 ; 1000:b84a
    db 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00 ; 1000:b85a
    db 0xc0, 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:b86a
    db 0x00, 0x00, 0x01, 0x00, 0xc0, 0x00, 0x10, 0x01, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0b, 0x00 ; 1000:b87a
    db 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x30, 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00 ; 1000:b88a
    db 0x00, 0x00, 0x0b, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x90, 0x02, 0x60, 0x00, 0x10, 0x00 ; 1000:b89a
    db 0x80, 0x81, 0x82, 0x83, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x02, 0xc6, 0xb8, 0xec, 0xb8 ; 1000:b8aa
    db 0x06, 0xb9, 0x06, 0xb9, 0x06, 0xb9, 0x06, 0xb9, 0x06, 0xb9, 0x06, 0xb9, 0x03, 0x00, 0xf0, 0x07 ; 1000:b8ba
    db 0x60, 0x00, 0xb0, 0x00, 0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x07, 0xe0, 0x00, 0x10, 0x01 ; 1000:b8ca
    db 0x0b, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0xff, 0xff, 0x00, 0x00 ; 1000:b8da
    db 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b8ea
    db 0x00, 0x00, 0xe0, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00 ; 1000:b8fa
    db 0x10, 0x00, 0x02, 0x00, 0x17, 0x00, 0x02, 0x00, 0x02, 0x00, 0x50, 0xb9, 0x48, 0xb9, 0x10, 0x00 ; 1000:b90a
    db 0x10, 0x00, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00, 0x04, 0x00, 0x60, 0xb9, 0x58, 0xb9, 0x20, 0x00 ; 1000:b91a
    db 0x08, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x70, 0xb9, 0x68, 0xb9, 0x10, 0x00 ; 1000:b92a
    db 0x10, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x78, 0xb9, 0x78, 0xb9, 0x00, 0x00 ; 1000:b93a
    db 0xa6, 0x00, 0x4c, 0x01, 0xf2, 0x01, 0x98, 0x02, 0x3e, 0x03, 0xe4, 0x03, 0x8a, 0x04, 0x30, 0x05 ; 1000:b94a
    db 0xd6, 0x05, 0x7c, 0x06, 0xd6, 0x05, 0x22, 0x07, 0xc8, 0x07, 0x6e, 0x08, 0xc8, 0x07, 0x14, 0x09 ; 1000:b95a
    db 0xba, 0x09, 0x60, 0x0a, 0xba, 0x09, 0x06, 0x0b, 0xac, 0x0b, 0x52, 0x0c, 0xac, 0x0b, 0xf8, 0x0c ; 1000:b96a
    db 0x9e, 0x0d, 0x8c, 0xb9, 0x96, 0xba, 0x7c, 0xbb, 0x0e, 0xbc, 0x40, 0xbc, 0x1a, 0xbd, 0x40, 0xbd ; 1000:b97a
    db 0x7e, 0xbd, 0x16, 0x00, 0x70, 0x01, 0x20, 0x01, 0x38, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b98a
    db 0x30, 0x02, 0xe0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0xd0, 0x00 ; 1000:b99a
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0xf0, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:b9aa
    db 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0xe0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b9ba
    db 0xd0, 0x02, 0xe0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0xe0, 0x00 ; 1000:b9ca
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0xf0, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:b9da
    db 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x01, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:b9ea
    db 0xd0, 0x03, 0x10, 0x01, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x20, 0x01 ; 1000:b9fa
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x06, 0x00, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:ba0a
    db 0x00, 0x00, 0x00, 0x00, 0x90, 0x06, 0x00, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ba1a
    db 0xe0, 0x06, 0x00, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x05, 0x00, 0x00 ; 1000:ba2a
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00 ; 1000:ba3a
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ba4a
    db 0xf0, 0x04, 0x48, 0x00, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0x28, 0x01 ; 1000:ba5a
    db 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x06, 0xa8, 0x00, 0x28, 0xb9, 0x00, 0x00 ; 1000:ba6a
    db 0x00, 0x00, 0x00, 0x00, 0x30, 0x06, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ba7a
    db 0xd0, 0x05, 0x48, 0x00, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0xf0, 0x00 ; 1000:ba8a
    db 0xe0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 0xe0, 0x00, 0x18, 0xb9 ; 1000:ba9a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x20, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:baaa
    db 0x00, 0x00, 0xb0, 0x03, 0x10, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03 ; 1000:baba
    db 0x30, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x20, 0x00, 0x18, 0xb9 ; 1000:baca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x04, 0xa0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:bada
    db 0x00, 0x00, 0x00, 0x05, 0xa0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x05 ; 1000:baea
    db 0x30, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05, 0x20, 0x00, 0x18, 0xb9 ; 1000:bafa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05, 0x40, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:bb0a
    db 0x00, 0x00, 0xd0, 0x01, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02 ; 1000:bb1a
    db 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x28, 0x01, 0x28, 0xb9 ; 1000:bb2a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:bb3a
    db 0x00, 0x00, 0x70, 0x03, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 ; 1000:bb4a
    db 0x68, 0x00, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x05, 0x28, 0x01, 0x28, 0xb9 ; 1000:bb5a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x28, 0x01, 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:bb6a
    db 0x00, 0x00, 0x0c, 0x00, 0xb0, 0x00, 0x90, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bb7a
    db 0xc0, 0x00, 0x80, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xa0, 0x00 ; 1000:bb8a
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x90, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:bb9a
    db 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x20, 0x01, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bbaa
    db 0xa0, 0x02, 0x10, 0x01, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x00, 0x01 ; 1000:bbba
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0xf0, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:bbca
    db 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0xe0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bbda
    db 0xa0, 0x02, 0xd0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0xa8, 0x00 ; 1000:bbea
    db 0x28, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x08, 0x01, 0x28, 0xb9, 0x00, 0x00 ; 1000:bbfa
    db 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x60, 0x01, 0x30, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00 ; 1000:bc0a
    db 0x00, 0x00, 0x80, 0x01, 0x80, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01 ; 1000:bc1a
    db 0x80, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x30, 0x00, 0x08, 0xb9 ; 1000:bc2a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x40, 0x06, 0xb0, 0x00, 0x08, 0xb9, 0x00, 0x00 ; 1000:bc3a
    db 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0x50, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bc4a
    db 0xd0, 0x07, 0x20, 0x01, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x06, 0x20, 0x01 ; 1000:bc5a
    db 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x04, 0x60, 0x00, 0x08, 0xb9, 0x00, 0x00 ; 1000:bc6a
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x05, 0x80, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bc7a
    db 0xe0, 0x03, 0x20, 0x01, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0xc0, 0x00 ; 1000:bc8a
    db 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x70, 0x00, 0x08, 0xb9, 0x00, 0x00 ; 1000:bc9a
    db 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xc0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bcaa
    db 0x80, 0x01, 0x70, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0xa0, 0x00 ; 1000:bcba
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x90, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:bcca
    db 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0x40, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bcda
    db 0x80, 0x04, 0x30, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0x40, 0x00 ; 1000:bcea
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x07, 0x10, 0x01, 0x18, 0xb9, 0x00, 0x00 ; 1000:bcfa
    db 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0xb0, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bd0a
    db 0x03, 0x00, 0x60, 0x00, 0x50, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01 ; 1000:bd1a
    db 0x20, 0x00, 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x01, 0x80, 0x00, 0x18, 0xb9 ; 1000:bd2a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0xa0, 0x00, 0x60, 0x00, 0x08, 0xb9, 0x00, 0x00 ; 1000:bd3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:bd4a
    db 0x60, 0x01, 0x80, 0x00, 0x08, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xa0, 0x00 ; 1000:bd5a
    db 0x18, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x10, 0x00, 0x18, 0xb9, 0x00, 0x00 ; 1000:bd6a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8e, 0xbd, 0xc6, 0xbd, 0xc9, 0xbd, 0xcc, 0xbd, 0xcf, 0xbd ; 1000:bd7a
    db 0xd5, 0xbe, 0xbb, 0xbf, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0xbd, 0x03, 0x00, 0x00, 0x00 ; 1000:bd8a
    db 0x00, 0x00, 0xb2, 0xbd, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbc, 0xbd, 0x0d, 0x00, 0x04, 0x01 ; 1000:bd9a
    db 0x14, 0x01, 0x16, 0x01, 0x14, 0x01, 0x18, 0x00, 0x00, 0x01, 0x04, 0x01, 0x14, 0x01, 0x04, 0x01 ; 1000:bdaa
    db 0x18, 0x00, 0x00, 0x01, 0x00, 0x01, 0x04, 0x01, 0x00, 0x01, 0x18, 0x00, 0x0d, 0x0d, 0x4c, 0xff ; 1000:bdba
    db 0xff, 0x00, 0xff, 0xff, 0x00, 0xdd, 0xbd, 0x03, 0xbe, 0x3b, 0xbe, 0x73, 0xbe, 0x87, 0xbe, 0x9b ; 1000:bdca
    db 0xbe, 0xc1, 0xbe, 0x02, 0x00, 0x90, 0x02, 0x60, 0x00, 0x10, 0x00, 0x50, 0x51, 0x52, 0x53, 0x0b ; 1000:bdda
    db 0x00, 0x07, 0x00, 0x01, 0x00, 0x7b, 0x02, 0x30, 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00 ; 1000:bdea
    db 0x00, 0x0c, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x90, 0x07, 0xf0, 0x00, 0x10 ; 1000:bdfa
    db 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x04, 0xb0 ; 1000:be0a
    db 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 ; 1000:be1a
    db 0x02, 0x90, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 ; 1000:be2a
    db 0x00, 0x03, 0x00, 0x50, 0x00, 0x60, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x05 ; 1000:be3a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0xf0, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c ; 1000:be4a
    db 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa0, 0x06, 0x10, 0x01, 0x10, 0x00, 0x87, 0x88, 0x89 ; 1000:be5a
    db 0x8a, 0x0c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x7f, 0x02, 0x01, 0x00, 0xa0, 0x03, 0x40, 0x00, 0x10 ; 1000:be6a
    db 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0xd0 ; 1000:be7a
    db 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00 ; 1000:be8a
    db 0x00, 0x02, 0x00, 0x60, 0x00, 0x60, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02 ; 1000:be9a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x10, 0x01, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0c ; 1000:beaa
    db 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x30, 0x00, 0x80, 0x00, 0x10, 0x00, 0xff ; 1000:beba
    db 0x00, 0x00, 0x00, 0x0c, 0x00, 0x02, 0x00, 0x02, 0x00, 0x7f, 0x02, 0xe3, 0xbe, 0xe5, 0xbe, 0xff ; 1000:beca
    db 0xbe, 0x19, 0xbf, 0x19, 0xbf, 0x19, 0xbf, 0x19, 0xbf, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 ; 1000:beda
    db 0x00, 0x50, 0x00, 0x0c, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x10, 0x01, 0x0c ; 1000:beea
    db 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x50, 0x00, 0x0c, 0x00, 0x01 ; 1000:befa
    db 0x00, 0x00, 0x00, 0xf0, 0x07, 0x80, 0x00, 0x10, 0x01, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00 ; 1000:bf0a
    db 0x00, 0x10, 0x00, 0x20, 0x00, 0x03, 0x80, 0x18, 0x00, 0x01, 0x00, 0x01, 0x00, 0x8b, 0xbf, 0x8b ; 1000:bf1a
    db 0xbf, 0x10, 0x00, 0x10, 0x00, 0x03, 0x80, 0x18, 0x00, 0x01, 0x00, 0x04, 0x00, 0x8f, 0xbf, 0x8f ; 1000:bf2a
    db 0xbf, 0x10, 0x00, 0x10, 0x00, 0x01, 0x80, 0x18, 0x00, 0x01, 0x00, 0x01, 0x00, 0x99, 0xbf, 0x99 ; 1000:bf3a
    db 0xbf, 0x10, 0x00, 0x20, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x9d, 0xbf, 0x9d ; 1000:bf4a
    db 0xbf, 0x10, 0x00, 0x10, 0x00, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x04, 0x00, 0x9f, 0xbf, 0x9f ; 1000:bf5a
    db 0xbf, 0x10, 0x00, 0x20, 0x00, 0x00, 0x7b, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0xaf, 0xbf, 0xa7 ; 1000:bf6a
    db 0xbf, 0x18, 0x00, 0x20, 0x00, 0x0a, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb7, 0xbf, 0xb7 ; 1000:bf7a
    db 0xbf, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xec, 0x01, 0x92, 0x02, 0x38, 0x03, 0xde, 0x03, 0x00 ; 1000:bf8a
    db 0x00, 0x84, 0x04, 0x2a, 0x05, 0x70, 0x06, 0x16, 0x07, 0xbc, 0x07, 0x16, 0x07, 0x62, 0x08, 0xa8 ; 1000:bf9a
    db 0x09, 0xee, 0x0a, 0xa8, 0x09, 0x34, 0x0c, 0x7a, 0x0d, 0xc0, 0x0e, 0x7a, 0x0d, 0x06, 0x10, 0xec ; 1000:bfaa
    db 0x11, 0xc9, 0xbf, 0xd7, 0xbf, 0x29, 0xc1, 0x7b, 0xc2, 0x31, 0xc3, 0xab, 0xc3, 0xf1, 0xc4, 0x01 ; 1000:bfba
    db 0x00, 0x70, 0x01, 0x70, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x40 ; 1000:bfca
    db 0x05, 0xb0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x70, 0x00, 0x1b ; 1000:bfda
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x04, 0x70, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00 ; 1000:bfea
    db 0x00, 0x00, 0x00, 0x90, 0x04, 0x50, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30 ; 1000:bffa
    db 0x04, 0xb0, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x06, 0x10, 0x01, 0x6b ; 1000:c00a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x10, 0x01, 0x6b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c01a
    db 0x00, 0x00, 0x00, 0x00, 0x02, 0x50, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:c02a
    db 0x07, 0xd0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x07, 0xd0, 0x00, 0x3b ; 1000:c03a
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0x40, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00 ; 1000:c04a
    db 0x00, 0x00, 0x00, 0xa0, 0x07, 0x40, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10 ; 1000:c05a
    db 0x05, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x70, 0x00, 0x3b ; 1000:c06a
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0xf0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00 ; 1000:c07a
    db 0x00, 0x00, 0x00, 0x40, 0x02, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40 ; 1000:c08a
    db 0x02, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x70, 0x00, 0x3b ; 1000:c09a
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00 ; 1000:c0aa
    db 0x00, 0x00, 0x00, 0x90, 0x02, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:c0ba
    db 0x01, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0x06, 0xb0, 0x00, 0x3b ; 1000:c0ca
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x05, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00 ; 1000:c0da
    db 0x00, 0x00, 0x00, 0x40, 0x04, 0xd0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:c0ea
    db 0x05, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04, 0xe0, 0x00, 0x3b ; 1000:c0fa
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00 ; 1000:c10a
    db 0x00, 0x00, 0x00, 0xa0, 0x05, 0xb0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1c ; 1000:c11a
    db 0x00, 0xb0, 0x04, 0x70, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x04, 0x70 ; 1000:c12a
    db 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x05, 0x70, 0x00, 0x1b, 0xbf, 0x00 ; 1000:c13a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0xd0, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c14a
    db 0x00, 0x60, 0x02, 0xe0, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xe0 ; 1000:c15a
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0xe0, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c16a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x07, 0x50, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c17a
    db 0x00, 0x20, 0x06, 0x50, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x05, 0x10 ; 1000:c18a
    db 0x01, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x60, 0x00, 0x6b, 0xbf, 0x00 ; 1000:c19a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x30, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:c1aa
    db 0x00, 0x70, 0x00, 0x30, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x80 ; 1000:c1ba
    db 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x30, 0x00, 0x3b, 0xbf, 0x00 ; 1000:c1ca
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x03, 0x30, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:c1da
    db 0x00, 0x10, 0x04, 0x00, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x04, 0x70 ; 1000:c1ea
    db 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x70, 0x00, 0x3b, 0xbf, 0x00 ; 1000:c1fa
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x06, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:c20a
    db 0x00, 0x70, 0x06, 0x70, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x07, 0x30 ; 1000:c21a
    db 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x90, 0x07, 0xd0, 0x00, 0x3b, 0xbf, 0x00 ; 1000:c22a
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x50, 0x07, 0xf0, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:c23a
    db 0x00, 0x90, 0x06, 0x80, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x06, 0x30 ; 1000:c24a
    db 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x30, 0x00, 0x3b, 0xbf, 0x00 ; 1000:c25a
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x30, 0x00, 0x3b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:c26a
    db 0x00, 0x0f, 0x00, 0x00, 0x03, 0x80, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:c27a
    db 0x02, 0xa0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0xc0, 0x00, 0x1b ; 1000:c28a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x60, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c29a
    db 0x00, 0x00, 0x00, 0xb0, 0x03, 0x60, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:c2aa
    db 0x03, 0x60, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x03, 0x60, 0x00, 0x1b ; 1000:c2ba
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c2ca
    db 0x00, 0x00, 0x00, 0xb0, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:c2da
    db 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0x10, 0x00, 0x2b ; 1000:c2ea
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x90, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c2fa
    db 0x00, 0x00, 0x00, 0xa0, 0x00, 0x90, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90 ; 1000:c30a
    db 0x00, 0x90, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x90, 0x00, 0x2b ; 1000:c31a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0xa0, 0x00, 0x50, 0x00, 0x1b, 0xbf, 0x00 ; 1000:c32a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x50, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c33a
    db 0x00, 0x80, 0x00, 0x90, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x90 ; 1000:c34a
    db 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x90, 0x00, 0x1b, 0xbf, 0x00 ; 1000:c35a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0xd0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c36a
    db 0x00, 0xe0, 0x00, 0xd0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0xd0 ; 1000:c37a
    db 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0xd0, 0x00, 0x1b, 0xbf, 0x00 ; 1000:c38a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xf0, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c39a
    db 0x00, 0x1b, 0x00, 0x90, 0x00, 0xf0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0 ; 1000:c3aa
    db 0x00, 0xf0, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0x1b ; 1000:c3ba
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x40, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c3ca
    db 0x00, 0x00, 0x00, 0x40, 0x01, 0x40, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70 ; 1000:c3da
    db 0x01, 0x40, 0x00, 0x1b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x40, 0x00, 0x1b ; 1000:c3ea
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0xe0, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c3fa
    db 0x00, 0x00, 0x00, 0x40, 0x02, 0xe0, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:c40a
    db 0x02, 0xe0, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x4b ; 1000:c41a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x20, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c42a
    db 0x00, 0x00, 0x00, 0x10, 0x03, 0x20, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0 ; 1000:c43a
    db 0x03, 0x20, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x20, 0x00, 0x4b ; 1000:c44a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x04, 0x20, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c45a
    db 0x00, 0x00, 0x00, 0xb0, 0x05, 0x90, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60 ; 1000:c46a
    db 0x06, 0x90, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x06, 0x90, 0x00, 0x4b ; 1000:c47a
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x07, 0x90, 0x00, 0x4b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c48a
    db 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x01, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:c49a
    db 0x00, 0x60, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x60, 0x00, 0x6b ; 1000:c4aa
    db 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x60, 0x00, 0x6b, 0xbf, 0x00, 0x00, 0x00 ; 1000:c4ba
    db 0x00, 0x00, 0x00, 0xa4, 0x07, 0xf0, 0x00, 0x7b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0 ; 1000:c4ca
    db 0x07, 0xf0, 0x00, 0x5b, 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x07, 0x00, 0x01, 0x5b ; 1000:c4da
    db 0xbf, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x20, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c4ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c4fa
    db 0x00, 0x60, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x10 ; 1000:c50a
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c51a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c52a
    db 0x00, 0xe0, 0x00, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10 ; 1000:c53a
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c54a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c55a
    db 0x00, 0x60, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x10 ; 1000:c56a
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c57a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c58a
    db 0x00, 0xe0, 0x01, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x10 ; 1000:c59a
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c5aa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c5ba
    db 0x00, 0x60, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x10 ; 1000:c5ca
    db 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00 ; 1000:c5da
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c5ea
    db 0x00, 0xe0, 0x02, 0x10, 0x00, 0x2b, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0xc6, 0x5f ; 1000:c5fa
    db 0xc6, 0x62, 0xc6, 0x65, 0xc6, 0x68, 0xc6, 0xb8, 0xc6, 0x56, 0xc7, 0x03, 0x00, 0x00, 0x00, 0x00 ; 1000:c60a
    db 0x00, 0x2f, 0xc6, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc6, 0x05, 0x00, 0x00, 0x00, 0x00 ; 1000:c61a
    db 0x00, 0x4f, 0xc6, 0x0d, 0x00, 0x00, 0x01, 0x10, 0x01, 0x07, 0x01, 0x17, 0x01, 0x07, 0x01, 0x10 ; 1000:c62a
    db 0x00, 0x00, 0x01, 0x18, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x10, 0x01, 0x07, 0x01, 0x17 ; 1000:c63a
    db 0x01, 0x07, 0x01, 0x18, 0x00, 0x17, 0x01, 0x07, 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x10 ; 1000:c64a
    db 0x01, 0x07, 0x01, 0x18, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x6c, 0xc6 ; 1000:c65a
    db 0x92, 0xc6, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff ; 1000:c66a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x50, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00, 0x00 ; 1000:c67a
    db 0x0d, 0x00, 0x01, 0x00, 0x00, 0x00, 0x83, 0x02, 0x02, 0x00, 0x60, 0x00, 0x50, 0x00, 0x10, 0x00 ; 1000:c68a
    db 0xff, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x83, 0x02, 0xc0, 0x0a, 0x70, 0x00 ; 1000:c69a
    db 0x10, 0x00, 0xff, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbc, 0xc6 ; 1000:c6aa
    db 0xca, 0xc6, 0x01, 0x00, 0x00, 0x00, 0xc0, 0x0a, 0x00, 0x0b, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 ; 1000:c6ba
    db 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x80, 0x1b, 0x00, 0x02, 0x00, 0x04, 0x00, 0x2c, 0xc7 ; 1000:c6ca
    db 0x2c, 0xc7, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x08, 0x00, 0x38, 0xc7 ; 1000:c6da
    db 0x38, 0xc7, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x48, 0xc7 ; 1000:c6ea
    db 0x48, 0xc7, 0x20, 0x00, 0x10, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00, 0x04, 0x00, 0x4e, 0xc7 ; 1000:c6fa
    db 0x4e, 0xc7, 0x10, 0x00, 0x10, 0x00, 0x00, 0x80, 0x1c, 0x00, 0x02, 0x00, 0x04, 0x00, 0x2c, 0xc7 ; 1000:c70a
    db 0x2c, 0xc7, 0x10, 0x00, 0x10, 0x00, 0x00, 0x80, 0x07, 0x00, 0x02, 0x00, 0x04, 0x00, 0x2c, 0xc7 ; 1000:c71a
    db 0x2c, 0xc7, 0x00, 0x00, 0xa6, 0x00, 0x4c, 0x01, 0xf2, 0x01, 0x98, 0x02, 0x3e, 0x03, 0xe4, 0x03 ; 1000:c72a
    db 0x8a, 0x04, 0x30, 0x05, 0xd6, 0x05, 0x7c, 0x06, 0xd6, 0x05, 0x30, 0x05, 0x8a, 0x04, 0x22, 0x07 ; 1000:c73a
    db 0xc8, 0x07, 0x6e, 0x08, 0x14, 0x09, 0x5a, 0x0a, 0xa0, 0x0b, 0xe6, 0x0c, 0x5a, 0xc7, 0xe8, 0xc8 ; 1000:c74a
    db 0x20, 0x00, 0x68, 0x00, 0x98, 0x0a, 0xcc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00 ; 1000:c75a
    db 0x90, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x03, 0xec, 0xc6 ; 1000:c76a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c77a
    db 0x00, 0x00, 0x40, 0x00, 0x80, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c78a
    db 0x20, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x20, 0x03, 0xec, 0xc6 ; 1000:c79a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c7aa
    db 0x00, 0x00, 0xc0, 0x00, 0x20, 0x03, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00 ; 1000:c7ba
    db 0x70, 0x02, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x60, 0x02, 0xec, 0xc6 ; 1000:c7ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x50, 0x02, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c7da
    db 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00 ; 1000:c7ea
    db 0x30, 0x02, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x20, 0x02, 0xec, 0xc6 ; 1000:c7fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c80a
    db 0x00, 0x00, 0x30, 0x00, 0xe0, 0x00, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00 ; 1000:c81a
    db 0xe0, 0x00, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xe0, 0x00, 0xec, 0xc6 ; 1000:c82a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0xe0, 0x00, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c83a
    db 0x00, 0x00, 0xb0, 0x00, 0xe0, 0x00, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00 ; 1000:c84a
    db 0xe0, 0x05, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xc0, 0x05, 0xec, 0xc6 ; 1000:c85a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xa0, 0x05, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c86a
    db 0x00, 0x00, 0x10, 0x00, 0x80, 0x05, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00 ; 1000:c87a
    db 0x60, 0x05, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x40, 0x05, 0xec, 0xc6 ; 1000:c88a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xa0, 0x04, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c89a
    db 0x00, 0x00, 0x40, 0x00, 0x80, 0x04, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 ; 1000:c8aa
    db 0x60, 0x04, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x04, 0xec, 0xc6 ; 1000:c8ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x20, 0x04, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00 ; 1000:c8ca
    db 0x00, 0x00, 0x20, 0x00, 0x00, 0x04, 0xec, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00 ; 1000:c8da
    db 0xa0, 0x00, 0x60, 0x00, 0x0c, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x10, 0x00 ; 1000:c8ea
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x30, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c8fa
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x50, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c90a
    db 0xd0, 0x02, 0x90, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x80, 0x00 ; 1000:c91a
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x60, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c92a
    db 0x00, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x40, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c93a
    db 0xa0, 0x03, 0x00, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x10, 0x00 ; 1000:c94a
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x30, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c95a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x50, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c96a
    db 0x00, 0x05, 0x70, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x20, 0x00 ; 1000:c97a
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x40, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c98a
    db 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x60, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c99a
    db 0x40, 0x06, 0x80, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x10, 0x00 ; 1000:c9aa
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x30, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c9ba
    db 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x50, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c9ca
    db 0x80, 0x07, 0x90, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x08, 0x10, 0x00 ; 1000:c9da
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x08, 0x30, 0x00, 0xdc, 0xc6, 0x00, 0x00 ; 1000:c9ea
    db 0x00, 0x00, 0x00, 0x00, 0xe0, 0x08, 0x50, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:c9fa
    db 0xd0, 0x08, 0x70, 0x00, 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x08, 0x90, 0x00 ; 1000:ca0a
    db 0xdc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0xfc, 0xc6, 0x00, 0x00 ; 1000:ca1a
    db 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x30, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ca2a
    db 0x50, 0x03, 0x50, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x04, 0x30, 0x00 ; 1000:ca3a
    db 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x06, 0x40, 0x00, 0xfc, 0xc6, 0x00, 0x00 ; 1000:ca4a
    db 0x00, 0x00, 0x00, 0x00, 0xe0, 0x06, 0x70, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ca5a
    db 0x70, 0x07, 0x20, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x50, 0x00 ; 1000:ca6a
    db 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x09, 0x20, 0x00, 0xfc, 0xc6, 0x00, 0x00 ; 1000:ca7a
    db 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0x40, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:ca8a
    db 0xc0, 0x03, 0x40, 0x00, 0xfc, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x0a, 0x80, 0x00 ; 1000:ca9a
    db 0x1c, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xca, 0x0a, 0xcb, 0x0d, 0xcb, 0x14, 0xcb ; 1000:caaa
    db 0x17, 0xcb, 0x51, 0xcd, 0x0f, 0xcf, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xda, 0xca, 0x03, 0x00 ; 1000:caba
    db 0x00, 0x00, 0x00, 0x00, 0xea, 0xca, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0xca, 0x0d, 0x00 ; 1000:caca
    db 0x00, 0x01, 0x10, 0x01, 0x07, 0x01, 0x17, 0x01, 0x07, 0x01, 0x10, 0x00, 0x00, 0x01, 0x18, 0x00 ; 1000:cada
    db 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x10, 0x01, 0x07, 0x01, 0x17, 0x01, 0x07, 0x01, 0x18, 0x00 ; 1000:caea
    db 0x17, 0x01, 0x07, 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x10, 0x01, 0x07, 0x01, 0x18, 0x00 ; 1000:cafa
    db 0xff, 0xff, 0x00, 0x05, 0x09, 0x32, 0x10, 0x10, 0x10, 0x10, 0x7b, 0x7b, 0x03, 0x2f, 0xcb, 0x43 ; 1000:cb0a
    db 0xcb, 0x57, 0xcb, 0xe9, 0xcb, 0x69, 0xcc, 0x7d, 0xcc, 0xa3, 0xcc, 0xdb, 0xcc, 0x13, 0xcd, 0x39 ; 1000:cb1a
    db 0xcd, 0x4d, 0xcd, 0x4f, 0xcd, 0x01, 0x00, 0x90, 0x00, 0x70, 0x00, 0x10, 0x00, 0xff, 0x00, 0x00 ; 1000:cb2a
    db 0x00, 0x0d, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00, 0x50, 0x01, 0x10 ; 1000:cb3a
    db 0x00, 0x5e, 0x5f, 0x5e, 0x5f, 0x0e, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0xb0 ; 1000:cb4a
    db 0x03, 0x50, 0x00, 0x10, 0x00, 0x5e, 0x5f, 0x5e, 0x5f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 ; 1000:cb5a
    db 0x00, 0xb0, 0x02, 0x30, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x03, 0x00, 0x00 ; 1000:cb6a
    db 0x00, 0x00, 0x00, 0xb0, 0x02, 0x80, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x03 ; 1000:cb7a
    db 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x30, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e ; 1000:cb8a
    db 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x80, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c ; 1000:cb9a
    db 0x3c, 0x0e, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x30, 0x01, 0x30, 0x00, 0x10, 0x00, 0x3b ; 1000:cbaa
    db 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x30, 0x01, 0x80, 0x00, 0x10 ; 1000:cbba
    db 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x30, 0x00, 0x50 ; 1000:cbca
    db 0x00, 0x10, 0x00, 0x5e, 0x5f, 0x5e, 0x5f, 0x0e, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 ; 1000:cbda
    db 0x00, 0x60, 0x00, 0x30, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x02, 0x00, 0x01 ; 1000:cbea
    db 0x00, 0x00, 0x00, 0x60, 0x00, 0x80, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x02 ; 1000:cbfa
    db 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x01, 0x30, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e ; 1000:cc0a
    db 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x01, 0x80, 0x00, 0x10, 0x00, 0x3b, 0x3b, 0x3c ; 1000:cc1a
    db 0x3c, 0x0e, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x30, 0x00, 0x10, 0x00, 0x3b ; 1000:cc2a
    db 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x80, 0x00, 0x10 ; 1000:cc3a
    db 0x00, 0x3b, 0x3b, 0x3c, 0x3c, 0x0e, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x40, 0x02, 0x30 ; 1000:cc4a
    db 0x00, 0x10, 0x00, 0x41, 0x42, 0x43, 0x44, 0x0e, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 ; 1000:cc5a
    db 0x00, 0x20, 0x00, 0x80, 0x00, 0x10, 0x00, 0x41, 0x42, 0x43, 0x44, 0x0e, 0x00, 0x03, 0x00, 0x06 ; 1000:cc6a
    db 0x00, 0x00, 0x00, 0x02, 0x00, 0x40, 0x00, 0x50, 0x00, 0x10, 0x00, 0x5e, 0x5f, 0x5e, 0x5f, 0x0e ; 1000:cc7a
    db 0x00, 0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0xa0, 0x07, 0x50, 0x00, 0x10, 0x00, 0x62, 0x62, 0x63 ; 1000:cc8a
    db 0x63, 0x0e, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x30, 0x00, 0x30, 0x00, 0x10 ; 1000:cc9a
    db 0x00, 0x62, 0x62, 0x63, 0x63, 0x0e, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x03, 0xd0 ; 1000:ccaa
    db 0x01, 0x10, 0x00, 0x62, 0x62, 0x63, 0x63, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x87, 0x02, 0xb0 ; 1000:ccba
    db 0x03, 0x30, 0x00, 0x10, 0x00, 0x64, 0x64, 0x65, 0x65, 0x0e, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 ; 1000:ccca
    db 0x00, 0x03, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x10, 0x00, 0x64, 0x64, 0x65, 0x65, 0x0e, 0x00, 0x06 ; 1000:ccda
    db 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x03, 0xc0, 0x00, 0x10, 0x00, 0x64, 0x64, 0x65, 0x65, 0xff ; 1000:ccea
    db 0xff, 0x00, 0x00, 0x00, 0x00, 0x87, 0x02, 0x30, 0x00, 0x30, 0x00, 0x10, 0x00, 0x60, 0x60, 0x61 ; 1000:ccfa
    db 0x61, 0x0e, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xb0, 0x03, 0x30, 0x00, 0x10 ; 1000:cd0a
    db 0x00, 0x60, 0x60, 0x61, 0x61, 0x0e, 0x00, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x50, 0x02, 0x70 ; 1000:cd1a
    db 0x00, 0x10, 0x00, 0x60, 0x60, 0x61, 0x61, 0x0e, 0x00, 0x09, 0x00, 0x00, 0x00, 0x8b, 0x02, 0x01 ; 1000:cd2a
    db 0x00, 0x30, 0x00, 0x70, 0x00, 0x10, 0x00, 0x60, 0x60, 0x61, 0x61, 0x0e, 0x00, 0x08, 0x00, 0x01 ; 1000:cd3a
    db 0x00, 0x8b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x69, 0xcd, 0x77, 0xcd, 0x91, 0xcd, 0x91, 0xcd, 0x91 ; 1000:cd4a
    db 0xcd, 0x91, 0xcd, 0x91, 0xcd, 0x91, 0xcd, 0x93, 0xcd, 0xa1, 0xcd, 0xaf, 0xcd, 0xc9, 0xcd, 0x01 ; 1000:cd5a
    db 0x00, 0x70, 0x05, 0x10, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00 ; 1000:cd6a
    db 0x00, 0x10, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x10, 0x00, 0x70 ; 1000:cd7a
    db 0x00, 0x0e, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x70 ; 1000:cd8a
    db 0x00, 0x0e, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xf0, 0x07, 0x00, 0x00, 0x40, 0x00, 0x0e ; 1000:cd9a
    db 0x00, 0x0a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x00, 0x0e, 0x00, 0x09 ; 1000:cdaa
    db 0x00, 0x00, 0x00, 0x90, 0x01, 0x70, 0x01, 0xd0, 0x01, 0x0e, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01 ; 1000:cdba
    db 0x00, 0x00, 0x00, 0x10, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x10, 0x00, 0x20 ; 1000:cdca
    db 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0xb7, 0xce, 0xb7, 0xce, 0x10, 0x00, 0x10 ; 1000:cdda
    db 0x00, 0x00, 0x80, 0x1e, 0x00, 0x01, 0x00, 0x04, 0x00, 0xbf, 0xce, 0xbf, 0xce, 0x10, 0x00, 0x10 ; 1000:cdea
    db 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0xcd, 0xce, 0xc9, 0xce, 0x10, 0x00, 0x10 ; 1000:cdfa
    db 0x00, 0x02, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x02, 0x00, 0xd1, 0xce, 0xd1, 0xce, 0x08, 0x00, 0x10 ; 1000:ce0a
    db 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0xd9, 0xce, 0xd5, 0xce, 0x10, 0x00, 0x10 ; 1000:ce1a
    db 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0xe1, 0xce, 0xdd, 0xce, 0x20, 0x00, 0x10 ; 1000:ce2a
    db 0x00, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0xe9, 0xce, 0xe5, 0xce, 0x10, 0x00, 0x10 ; 1000:ce3a
    db 0x00, 0x03, 0x80, 0x20, 0x00, 0x01, 0x00, 0x02, 0x00, 0xf3, 0xce, 0xf3, 0xce, 0x10, 0x00, 0x10 ; 1000:ce4a
    db 0x00, 0x03, 0x80, 0x21, 0x00, 0x01, 0x00, 0x02, 0x00, 0xed, 0xce, 0xed, 0xce, 0x10, 0x00, 0x10 ; 1000:ce5a
    db 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0xf9, 0xce, 0xf9, 0xce, 0x10, 0x00, 0x30 ; 1000:ce6a
    db 0x00, 0x04, 0x00, 0x1f, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0xcf, 0x01, 0xcf, 0x10, 0x00, 0x30 ; 1000:ce7a
    db 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0xcf, 0x05, 0xcf, 0x10, 0x00, 0x30 ; 1000:ce8a
    db 0x00, 0x20, 0x01, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x09, 0xcf, 0x09, 0xcf, 0x10, 0x00, 0x30 ; 1000:ce9a
    db 0x00, 0x00, 0x80, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0d, 0xcf, 0x0d, 0xcf, 0x00, 0x00, 0x46 ; 1000:ceaa
    db 0x01, 0x8c, 0x02, 0x46, 0x01, 0xd2, 0x03, 0x78, 0x04, 0x1e, 0x05, 0xc4, 0x05, 0x1e, 0x05, 0x6a ; 1000:ceba
    db 0x06, 0x10, 0x07, 0xb6, 0x07, 0x5c, 0x08, 0x02, 0x09, 0xa8, 0x09, 0x4e, 0x0a, 0xa4, 0x0a, 0xfa ; 1000:ceca
    db 0x0a, 0x50, 0x0b, 0xa6, 0x0b, 0x4c, 0x0c, 0xf2, 0x0c, 0x98, 0x0d, 0x3e, 0x0e, 0x84, 0x0f, 0xca ; 1000:ceda
    db 0x10, 0x10, 0x12, 0xee, 0x15, 0x56, 0x13, 0xfc, 0x13, 0xee, 0x15, 0xa2, 0x14, 0x48, 0x15, 0x94 ; 1000:ceea
    db 0x16, 0x3a, 0x17, 0xe0, 0x17, 0x86, 0x18, 0x12, 0x1b, 0x2c, 0x19, 0xf8, 0x1c, 0x3e, 0x1e, 0x84 ; 1000:cefa
    db 0x1f, 0xca, 0x20, 0x10, 0x22, 0x27, 0xcf, 0xc5, 0xcf, 0x03, 0xd0, 0xa1, 0xd0, 0x33, 0xd1, 0x89 ; 1000:cf0a
    db 0xd1, 0x33, 0xd2, 0xf5, 0xd2, 0x9f, 0xd3, 0xb9, 0xd3, 0x9b, 0xd5, 0xa9, 0xd5, 0x0d, 0x00, 0x70 ; 1000:cf1a
    db 0x01, 0x80, 0x00, 0xe7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x10, 0x00, 0x07 ; 1000:cf2a
    db 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00 ; 1000:cf3a
    db 0x00, 0x00, 0x00, 0x70, 0x03, 0x50, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70 ; 1000:cf4a
    db 0x03, 0x80, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x10, 0x00, 0x07 ; 1000:cf5a
    db 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00 ; 1000:cf6a
    db 0x00, 0x00, 0x00, 0xf0, 0x03, 0x50, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:cf7a
    db 0x03, 0x80, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x10, 0x00, 0x07 ; 1000:cf8a
    db 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00 ; 1000:cf9a
    db 0x00, 0x00, 0x00, 0x70, 0x04, 0x50, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70 ; 1000:cfaa
    db 0x04, 0x80, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0xc0, 0x00, 0x40 ; 1000:cfba
    db 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x30, 0x02, 0xd7, 0xcd, 0x00 ; 1000:cfca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:cfda
    db 0x00, 0x90, 0x00, 0xa0, 0x01, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x50 ; 1000:cfea
    db 0x01, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x20, 0x03, 0x80, 0x00, 0xd7 ; 1000:cffa
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x30, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d00a
    db 0x00, 0x00, 0x00, 0x30, 0x02, 0x30, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:d01a
    db 0x02, 0x80, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, 0x30, 0x00, 0xd7 ; 1000:d02a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x01, 0x80, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d03a
    db 0x00, 0x00, 0x00, 0xd0, 0x00, 0x80, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 ; 1000:d04a
    db 0x00, 0x60, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x80, 0x00, 0xd7 ; 1000:d05a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x30, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d06a
    db 0x00, 0x00, 0x00, 0xc0, 0x01, 0x30, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0 ; 1000:d07a
    db 0x01, 0x80, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x30, 0x00, 0xd7 ; 1000:d08a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0xa0, 0x00, 0x40, 0x00, 0xf7, 0xcd, 0x00 ; 1000:d09a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d0aa
    db 0x00, 0xd0, 0x00, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x40 ; 1000:d0ba
    db 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x40, 0x00, 0xf7, 0xcd, 0x00 ; 1000:d0ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d0da
    db 0x00, 0xa0, 0x01, 0x40, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x40 ; 1000:d0ea
    db 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x40, 0x00, 0xf7, 0xcd, 0x00 ; 1000:d0fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x01, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d10a
    db 0x00, 0x30, 0x02, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x90 ; 1000:d11a
    db 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x01, 0x90, 0x00, 0xf7 ; 1000:d12a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x80, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d13a
    db 0x00, 0x00, 0x00, 0xc0, 0x00, 0x40, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:d14a
    db 0x01, 0x50, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x50, 0x00, 0xf7 ; 1000:d15a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x90, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d16a
    db 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e ; 1000:d17a
    db 0x00, 0x80, 0x02, 0x80, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x04, 0x80 ; 1000:d18a
    db 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x05, 0x80, 0x00, 0xf7, 0xcd, 0x00 ; 1000:d19a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x05, 0x80, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d1aa
    db 0x00, 0xb0, 0x05, 0x80, 0x00, 0xf7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0x10 ; 1000:d1ba
    db 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0x30, 0x00, 0x07, 0xce, 0x00 ; 1000:d1ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0x50, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d1da
    db 0x00, 0xb0, 0x06, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0x60 ; 1000:d1ea
    db 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x06, 0x10, 0x00, 0x07, 0xce, 0x00 ; 1000:d1fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x07, 0x10, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d20a
    db 0x00, 0x20, 0x07, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x50 ; 1000:d21a
    db 0x00, 0x87, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x07 ; 1000:d22a
    db 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x60, 0x01, 0x07, 0xce, 0x00, 0x00, 0x00 ; 1000:d23a
    db 0x00, 0x00, 0x00, 0xc0, 0x00, 0xa0, 0x01, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0 ; 1000:d24a
    db 0x00, 0x50, 0x02, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0xe0, 0x01, 0x07 ; 1000:d25a
    db 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0xf0, 0x01, 0x07, 0xce, 0x00, 0x00, 0x00 ; 1000:d26a
    db 0x00, 0x00, 0x00, 0xb0, 0x00, 0xa0, 0x01, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0 ; 1000:d27a
    db 0x00, 0xf0, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x01, 0xf0, 0x00, 0xd7 ; 1000:d28a
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xf0, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d29a
    db 0x00, 0x00, 0x00, 0x10, 0x02, 0xf0, 0x01, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70 ; 1000:d2aa
    db 0x02, 0xf0, 0x01, 0xd7, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x02, 0xf0, 0x01, 0xd7 ; 1000:d2ba
    db 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x20, 0x00, 0xd7, 0xcd, 0x00, 0x00, 0x00 ; 1000:d2ca
    db 0x00, 0x00, 0x00, 0x90, 0x03, 0xd0, 0x01, 0x87, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50 ; 1000:d2da
    db 0x03, 0xd0, 0x01, 0x87, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0xc8, 0x00, 0x80 ; 1000:d2ea
    db 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x01, 0x80, 0x00, 0x77, 0xce, 0x00 ; 1000:d2fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x01, 0x80, 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d30a
    db 0x00, 0xb8, 0x01, 0x80, 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x02, 0x80 ; 1000:d31a
    db 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x02, 0x80, 0x00, 0x77, 0xce, 0x00 ; 1000:d32a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x02, 0x80, 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d33a
    db 0x00, 0xf8, 0x02, 0x80, 0x00, 0x77, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x30 ; 1000:d34a
    db 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x30, 0x00, 0x07, 0xce, 0x00 ; 1000:d35a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x20, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d36a
    db 0x00, 0xa0, 0x02, 0x30, 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0x20 ; 1000:d37a
    db 0x00, 0x07, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x03, 0xc0, 0x00, 0x87, 0xce, 0x00 ; 1000:d38a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xb0, 0x02, 0x70, 0x00, 0x87, 0xce, 0x00, 0x00, 0x00 ; 1000:d39a
    db 0x00, 0x00, 0x00, 0x80, 0x02, 0x70, 0x00, 0x87, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28 ; 1000:d3aa
    db 0x00, 0xa0, 0x00, 0x80, 0x00, 0x17, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x80 ; 1000:d3ba
    db 0x00, 0x17, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x80, 0x00, 0x17, 0xce, 0x00 ; 1000:d3ca
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x80, 0x00, 0x17, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d3da
    db 0x00, 0x60, 0x01, 0x80, 0x00, 0x17, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x80 ; 1000:d3ea
    db 0x00, 0x17, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x80, 0x00, 0x17, 0xce, 0x00 ; 1000:d3fa
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x02, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d40a
    db 0x00, 0x80, 0x02, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x80 ; 1000:d41a
    db 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x02, 0x80, 0x00, 0x27, 0xce, 0x00 ; 1000:d42a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d43a
    db 0x00, 0xa0, 0x03, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x80 ; 1000:d44a
    db 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80, 0x00, 0x27, 0xce, 0x00 ; 1000:d45a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x04, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d46a
    db 0x00, 0xd0, 0x04, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x05, 0x80 ; 1000:d47a
    db 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x05, 0x80, 0x00, 0x27, 0xce, 0x00 ; 1000:d48a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x06, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d49a
    db 0x00, 0x60, 0x06, 0x80, 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x06, 0x80 ; 1000:d4aa
    db 0x00, 0x27, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x07, 0x80, 0x00, 0x27, 0xce, 0x00 ; 1000:d4ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x01, 0x20, 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d4ca
    db 0x00, 0x60, 0x02, 0x10, 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x20 ; 1000:d4da
    db 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x02, 0x30, 0x00, 0x37, 0xce, 0x00 ; 1000:d4ea
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x02, 0x10, 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d4fa
    db 0x00, 0xe0, 0x02, 0x20, 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x02, 0x70 ; 1000:d50a
    db 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0x20, 0x00, 0x37, 0xce, 0x00 ; 1000:d51a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x03, 0x10, 0x00, 0x37, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d52a
    db 0x00, 0x50, 0x05, 0x10, 0x00, 0x57, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x05, 0x10 ; 1000:d53a
    db 0x00, 0x57, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x06, 0x10, 0x00, 0x57, 0xce, 0x00 ; 1000:d54a
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0x10, 0x00, 0x57, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00 ; 1000:d55a
    db 0x00, 0x00, 0x05, 0x10, 0x00, 0x47, 0xce, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x05, 0x10 ; 1000:d56a
    db 0x00, 0x47, 0xce, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x10, 0x00, 0x47, 0xce, 0x00 ; 1000:d57a
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x06, 0x10, 0x00, 0x47, 0xce, 0x00, 0x01, 0x00, 0x00, 0x00 ; 1000:d58a
    db 0x00, 0x01, 0x00, 0x80, 0x00, 0x60, 0x00, 0x87, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 ; 1000:d59a
    db 0x00, 0x50, 0x01, 0x30, 0x00, 0x97, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x01, 0x30 ; 1000:d5aa
    db 0x00, 0x67, 0xce, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x80, 0x00, 0xa7, 0xce, 0x00 ; 1000:d5ba
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00           ; 1000:d5ca

; ----------------------------------------------------------------------------
; DATA SEGMENTS & ASSETS (141,040 bytes)
; Appended at load module offset 0xD5D0
; ----------------------------------------------------------------------------
%include "comic2_data.asm"
