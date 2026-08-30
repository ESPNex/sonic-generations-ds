import re

v00 = open('/tmp/main_v00.c').read()
f51 = open('/tmp/t1/engine/source/main.c').read()
pat = r'// ---------------- render ----------------.*?\n}\n'
m1 = re.search(pat, v00, re.S)
m2 = re.search(pat, f51, re.S)
assert m1 and m2, (bool(m1), bool(m2))
f51 = f51.replace(m2.group(0), m1.group(0))

MACROS = """#define SKY_U16   ARGB16(1, 12, 22, 31)
#define SKY2_U16  ARGB16(1, 20, 30, 31)
#define GRASS_U16 ARGB16(1, 6, 26, 8)
#define GRASS2_U16 ARGB16(1, 10, 30, 12)
#define DIRT_U16  ARGB16(1, 22, 14, 6)
#define DIRT2_U16 ARGB16(1, 28, 19, 9)
// ---------------- video ----------------"""

if 'SKY_U16' in f51 and '#define SKY_U16' not in f51:
    f51 = f51.replace('// ---------------- video ----------------', MACROS)
open('/tmp/t1/engine/source/main.c', 'w').write(f51)
print('T1: render v0.0 innestato', len(m1.group(0)), 'byte')
