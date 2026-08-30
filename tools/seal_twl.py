#!/usr/bin/env python3
"""seal_twl.py — sigilla la ROM come DSi-ONLY (TWL).
- unit code = 3 (DSi esclusivo: rifiutato dal DS originale)
- ricontrollo CRC16 dell'header NDS
- boost CPU: SCFG_CLK9 (0x04004010) bit0 via libnds setCpuClock… gestito in
  engine (main.c) se disponibile; qui solo header.
Uso: python3 tools/seal_twl.py ROM.nds
"""
import sys, struct

def crc16(data):
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
            crc &= 0xFFFF
    return crc

def main(path):
    b = bytearray(open(path, 'rb').read())
    old = b[0x12]
    b[0x12] = 3                       # unit code: 3 = DSi-exclusive
    # header CRC su 0x20..0x15F
    b[0x15E:0x160] = struct.pack('<H', crc16(bytes(b[0x20:0x160])))
    open(path, 'wb').write(bytes(b))
    print('%s: unitcode %d -> 3 (DSi-ONLY), header CRC ricalcolato (%d byte)' % (path, old, len(b)))

if __name__ == '__main__':
    main(sys.argv[1])
