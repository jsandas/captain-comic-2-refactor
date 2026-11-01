import json
import os
from pathlib import Path
import struct
import sys

def main():
    script_dir = Path(__file__).resolve().parent
    repo_root = script_dir.parent
    default_exe = repo_root / 'reference' / 'original' / 'unpacked.exe'

    exe_path = Path(sys.argv[1]) if len(sys.argv) > 1 else default_exe
    if not exe_path.exists():
        print(f"Error: {exe_path} not found")
        sys.exit(1)

    with open(exe_path, 'rb') as f:
        exe = f.read()

    # MZ DOS header offset:
    # In standard DOS MZ headers, e_cparhdr (offset 0x08) specifies the header size in
    # 16-byte paragraphs. For this pinned binary ('unpacked.exe'), e_cparhdr is exactly
    # 0x0020 (32 paragraphs = 512 bytes). We validate this assumption explicitly.
    header_paragraphs = struct.unpack_from('<H', exe, 0x08)[0]
    assert header_paragraphs == 0x20, (
        f"Pinned binary assumption violated: expected e_cparhdr = 0x20 (512 bytes), "
        f"got {header_paragraphs:#x}"
    )
    seg000 = exe[header_paragraphs * 16:]

    def parse_range(data, offset):
        min_t = data[offset]
        max_t = data[offset + 1]
        assert max_t >= min_t, (
            f"Invalid trigger range at offset {offset:#x}: max_tile ({max_t}) < min_tile ({min_t})"
        )
        cnt = max_t - min_t + 1
        assert offset + 2 + cnt <= len(data), (
            f"Trigger range data exceeds buffer at offset {offset:#x} "
            f"(needed {offset + 2 + cnt}, total {len(data)})"
        )
        vals = list(data[offset + 2:offset + 2 + cnt])
        return {'min': min_t, 'max': max_t, 'values': vals}

    # Level descriptor table at 0x16a8 (in original executable code segment, ending at 0x16c6)
    table_start = 0x16a8
    table_end = 0x16c6
    num_levels = int(sys.argv[2]) if len(sys.argv) > 2 else (table_end - table_start) // 2
    level_ptrs = struct.unpack(f'<{num_levels}H', seg000[table_start:table_start + num_levels * 2])

    levels = []

    for lvl, ptr in enumerate(level_ptrs):
        desc = struct.unpack('<8H', seg000[ptr:ptr+16])
        t_trig = parse_range(seg000, desc[1])
        p_trig = parse_range(seg000, desc[2])
        m_trig = parse_range(seg000, desc[3])

        # room count from list B
        # Invariant / Layout Assumption:
        # In the original binary data layout, the List B pointer table for a level
        # begins at offset pB (array of uint16 offsets to each room's transition zone list).
        # The first room's transition records (target of pointer 0) are placed in memory
        # immediately following this pointer table: first_target_B == pB + room_count * 2.
        # Therefore, the room count is derived from (first_target_B - pB) // 2.
        pB = desc[5]
        first_target_B = struct.unpack('<H', seg000[pB:pB+2])[0]
        assert first_target_B > pB and (first_target_B - pB) % 2 == 0, (
            f"Level {lvl} List B layout invariant failed: pB={pB:#x}, first_target_B={first_target_B:#x}"
        )
        room_count = (first_target_B - pB) // 2
        assert room_count > 0, f"Level {lvl} derived non-positive room_count: {room_count}"

        # List B: transition zones
        listB_ptrs = struct.unpack(f'<{room_count}H', seg000[pB:pB+room_count*2])
        rooms_transitions = []
        for rm, t in enumerate(listB_ptrs):
            cnt = struct.unpack('<H', seg000[t:t+2])[0]
            zones = []
            for i in range(cnt):
                entry = seg000[t+2+i*12 : t+2+(i+1)*12]
                x, y_min, y_max, f3, f4, f5 = struct.unpack('<6H', entry)
                zones.append({
                    'x': x,
                    'y_min': y_min,
                    'y_max': y_max,
                    'target_level': f3,
                    'target_room': f4,
                    'spawn_index': f5
                })
            rooms_transitions.append(zones)

        # List A: item / special entities
        pA = desc[4]
        listA_ptrs = struct.unpack(f'<{room_count}H', seg000[pA:pA+room_count*2])
        rooms_listA = []
        for rm, t in enumerate(listA_ptrs):
            cnt = struct.unpack('<H', seg000[t:t+2])[0]
            items = []
            for i in range(cnt):
                entry = seg000[t+2+i*12 : t+2+(i+1)*12]
                items.append(list(struct.unpack('<6H', entry)))
            rooms_listA.append(items)

        # List C: active enemy entities
        pC = desc[6]
        listC_ptrs = struct.unpack(f'<{room_count}H', seg000[pC:pC+room_count*2])
        rooms_listC = []
        for rm, t in enumerate(listC_ptrs):
            cnt = struct.unpack('<H', seg000[t:t+2])[0]
            items = []
            for i in range(cnt):
                entry = seg000[t+2+i*12 : t+2+(i+1)*12]
                items.append(list(struct.unpack('<6H', entry)))
            rooms_listC.append(items)

        levels.append({
            'level_id': lvl,
            'room_count': room_count,
            'touch_trigger': t_trig,
            'proj_trigger': p_trig,
            'mode2_trigger': m_trig,
            'transition_zones': rooms_transitions,
            'list_a': rooms_listA,
            'room_entities': rooms_listC
        })

    out_json = script_dir / 'extracted_level_data.json'
    with open(out_json, 'w') as out:
        json.dump(levels, out, indent=2)

    print(f"Extracted {len(levels)} levels successfully to {out_json}")

if __name__ == '__main__':
    main()
