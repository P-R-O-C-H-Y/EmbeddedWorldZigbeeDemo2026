#!/usr/bin/env python3
"""
Convert weather icon PNGs to 1-bit C headers for E-ink (4-gray panel).
Output format: row-major, 8 pixels per byte, MSB = left. 1 = black (drawn), 0 = white.
Matches blit_1bit_to_4g() in epd_ui.cpp. White background on E-ink.
"""
import re
import sys
from pathlib import Path
from typing import Tuple

try:
    from PIL import Image
except ImportError:
    print("Install Pillow: pip install Pillow", file=sys.stderr)
    sys.exit(1)

# Frame size on E-ink (icons are centered in this)
ICON_FRAME_W = 106
ICON_FRAME_H = 106

# Threshold: pixel luminance below this -> black (1). Above -> white (0). White background.
THRESHOLD = 192

# WMO codes that use 4G icons (only 0 and 45 stay 1-bit: SUN, FOG).
WMO_4G_IDS = (2, 3, 51, 61, 71, 95)

# -----------------------------------------------------------------------------
# 4G icon color reference (gray = R=G=B, luminance = R).
# Luminance = (R*299 + G*587 + B*114)/1000; for gray R=G=B so lum = R.
# -----------------------------------------------------------------------------
# WITH +1 shift (current): light/mid/dark get pushed one step darker on E-ink.
# WITHOUT +1 shift (new icons): use these for direct 1:1 mapping to E-ink levels:
#
#   E-ink 0 (white):      RGB(255,255,255)  #FFFFFF  lum 255   background
#   E-ink 1 (light gray): RGB(210,210,210)  #D2D2D2  lum 210   light gray
#   E-ink 2 (mid gray):   RGB(128,128,128)  #808080  lum 128   mid gray
#   E-ink 3 (black):      RGB(45,45,45)     #2D2D2D  lum 45    black
#
# Transparent: alpha < 64 → white. Avoid lum 254, 168, 90 (bucket edges).
# -----------------------------------------------------------------------------


def lum_to_4g(lum: int, alpha: int) -> int:
    """Map luminance and alpha to 4G level 0 (white) .. 3 (black).
    Only near-pure white stays 0 so light gray clouds become visible.
    """
    # Strongly transparent -> white (background)
    if alpha < 64:
        return 0
    # Only pure/near-pure white -> 0 to avoid white speckles between clouds (e.g. rain icon)
    if lum >= 254:
        return 0
    if lum >= 168:
        return 1   # light gray (cloud, fill gaps)
    if lum >= 90:
        return 2   # mid gray
    return 3       # dark / black


def _bytes_per_col_4g(h: int) -> int:
    return ((h - 1) // 8) * 2 + ((h - 1) % 8) // 4 + 1


def png_to_4g(path: Path) -> Tuple[bytes, int, int]:
    """Load PNG at actual size, convert to 4G column-major 2bpp. Returns (data, w, h)."""
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    bpc = _bytes_per_col_4g(h)
    out = bytearray(w * bpc)
    for x in range(w):
        for y in range(h):
            r, g, b, a = img.getpixel((x, y))
            lum = (r * 299 + g * 587 + b * 114) // 1000
            val = lum_to_4g(lum, a)
            col_byte = (y // 8) * 2 + (y % 8) // 4
            nibble_shift = (y % 4) * 2
            byte_ix = x * bpc + col_byte
            out[byte_ix] &= ~(3 << (6 - nibble_shift))
            out[byte_ix] |= (val << (6 - nibble_shift))
    return (bytes(out), w, h)


def png_to_1bit(path: Path) -> Tuple[bytes, int, int]:
    """Load PNG at actual size, convert to 1-bit row-major (8 px/byte, MSB left). Returns (data, w, h)."""
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    row_stride = (w + 7) // 8
    out = bytearray(row_stride * h)
    for y in range(h):
        for x in range(w):
            r, g, b, a = img.getpixel((x, y))
            if a < 128:
                bit = 0
            else:
                lum = (r * 299 + g * 587 + b * 114) // 1000
                bit = 1 if lum < THRESHOLD else 0
            byte_ix = y * row_stride + (x // 8)
            bit_ix = 7 - (x % 8)
            if bit:
                out[byte_ix] |= 1 << bit_ix
    return (bytes(out), w, h)


def emit_header(array_name: str, data: bytes, w: int, h: int, out_path: Path, comment: str = "") -> None:
    """Write a single C header with per-icon W and H (no PROGMEM; ESP32)."""
    if not comment:
        comment = f"1-bit {w}x{h} E-ink icon, row-major, 8 px/byte. White background."
    lines = [
        f"/* {comment} */",
        f"#ifndef WEATHER_ICON_{array_name.upper()}_H",
        f"#define WEATHER_ICON_{array_name.upper()}_H",
        "",
        f"#define WEATHER_ICON_{array_name.upper()}_W {w}",
        f"#define WEATHER_ICON_{array_name.upper()}_H {h}",
        "",
        f"const unsigned char {array_name}[{len(data)}] = {{",
    ]
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"  {hex_str},")
    lines.append("};")
    lines.append("")
    lines.append("#endif")
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def emit_4g_header(array_name: str, data: bytes, w: int, h: int, out_path: Path) -> None:
    """Write 4G (2bpp) column-major icon header with actual W and H."""
    comment = f"4-gray {w}x{h} E-ink icon, column-major, 2bpp. White background."
    emit_header(array_name, data, w, h, out_path, comment)


def main():
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    icons_dir = project_dir / "weather_icons"
    out_dir = project_dir / "weather_icons"  # Headers in weather_icons/

    if not icons_dir.is_dir():
        print(f"No directory: {icons_dir}", file=sys.stderr)
        sys.exit(1)

    pattern = re.compile(r"^weather_(\d+)\.png$")
    count = 0
    for png in sorted(icons_dir.iterdir(), key=lambda p: (len(p.name), p.name)):
        if not png.is_file() or png.suffix.lower() != ".png":
            continue
        m = pattern.match(png.name)
        if not m:
            continue
        wid = m.group(1)
        wid_int = int(wid)
        array_name = f"weather_{wid}"
        # 1-bit for all (actual PNG size)
        data, w1, h1 = png_to_1bit(png)
        out_path = out_dir / f"weather_icon_{wid}.h"
        emit_header(array_name, data, w1, h1, out_path)
        print(f"  {png.name} -> {out_path.name} ({w1}x{h1}, {len(data)} bytes 1-bit)")
        count += 1
        # 4G for icons that need grayscale
        if wid_int in WMO_4G_IDS:
            data_4g, w4, h4 = png_to_4g(png)
            name_4g = f"weather_{wid}_4g"
            path_4g = out_dir / f"weather_icon_{wid}_4g.h"
            emit_4g_header(name_4g, data_4g, w4, h4, path_4g)
            print(f"  {png.name} -> {path_4g.name} ({w4}x{h4}, {len(data_4g)} bytes 4G)")
            count += 1

    print(f"Generated {count} header(s) in {out_dir.relative_to(project_dir)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
