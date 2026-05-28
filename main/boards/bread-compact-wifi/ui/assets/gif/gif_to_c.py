#!/usr/bin/env python3
"""Embed splash_bg.gif as a C byte array for firmware."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "splash_bg.gif"
DST = ROOT.parent / "images" / "splash_bg_gif.c"

data = SRC.read_bytes()
lines = ['#include "../../splash_bg_gif.h"', '', 'const uint8_t splash_bg_gif[] = {']
for i in range(0, len(data), 16):
    chunk = data[i : i + 16]
    suffix = ',' if i + 16 < len(data) else ''
    hexes = ','.join(f'0x{b:02x}' for b in chunk)
    lines.append(f'    {hexes}{suffix}')
lines.append('};')
lines.append(f'const uint32_t splash_bg_gif_size = {len(data)};')
lines.append('')
DST.write_text('\n'.join(lines), encoding='utf-8')
print(f'Wrote {DST} ({len(data)} bytes)')
