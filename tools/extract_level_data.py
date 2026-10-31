import struct
import json
import os
import sys

def main():
    exe_path = sys.argv[1] if len(sys.argv) > 1 else 'reference/original/unpacked.exe'
    if not os.path.exists(exe_path):
        print(f"Error: {exe_path} not found")
        sys.exit(1)

    with open(exe_path, 'rb') as f:
        exe = f.read()

    # MZ header offset: 512 bytes (0x20 paragraphs)
    seg000 = exe[512:]

    def parse_range(data, offset):
        min_t = data[offset]
        max_t = data[offset+1]
        cnt = max_t - min_t + 1
        vals = list(data[offset+2:offset+2+cnt])
        return {'min': min_t, 'max': max_t, 'values': vals}

    # Level descriptor table at 0x16a8 (in original executable code segment)
    level_ptrs = struct.unpack('<15H', seg000[0x16a8:0x16a8+30])

    levels = []

    for lvl, ptr in enumerate(level_ptrs):
        desc = struct.unpack('<8H', seg000[ptr:ptr+16])
        t_trig = parse_range(seg000, desc[1])
        p_trig = parse_range(seg000, desc[2])
        m_trig = parse_range(seg000, desc[3])

        # room count from list B
        pB = desc[5]
        first_target_B = struct.unpack('<H', seg000[pB:pB+2])[0]
        room_count = (first_target_B - pB) // 2

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

    out_json = 'tools/extracted_level_data.json'
    with open(out_json, 'w') as out:
        json.dump(levels, out, indent=2)

    print(f"Extracted {len(levels)} levels successfully to {out_json}")

if __name__ == '__main__':
    main()
