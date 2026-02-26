#!/usr/bin/env python3
"""
Convert a PNG image to 4-gray (2bpp) column-major C header for E-ink.
Matches format used in epd_ui.cpp blit_4g_icon_to_4g (and tools/png_to_epd_header.py).
Usage: python png_to_4g_header.py <input.png> [output.h]
Output: C header with NO_SIGNAL_4G_W, NO_SIGNAL_4G_H and no_signal_4g[].
Requires: Pillow (pip install Pillow)
"""
import sys
import os

def _bytes_per_col_4g(h):
    return ((h - 1) // 8) * 2 + ((h - 1) % 8) // 4 + 1

def lum_to_4g(lum: int, alpha: int) -> int:
    if alpha < 64:
        return 0
    if lum >= 254:
        return 0
    if lum >= 168:
        return 1
    if lum >= 90:
        return 2
    return 3

def png_to_4g(png_path, out_path=None):
    try:
        from PIL import Image
    except ImportError:
        print("Install Pillow: pip install Pillow", file=sys.stderr)
        return False

    img = Image.open(png_path).convert("RGBA")
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

    if out_path is None:
        project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        out_path = os.path.join(project_dir, "weather_icons", "no_signal_4g.h")
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)

    with open(out_path, "w") as f:
        f.write("/* 4-gray E-ink icon, column-major, 2bpp. Generated from %s */\n" % os.path.basename(png_path))
        f.write("#ifndef NO_SIGNAL_4G_H\n#define NO_SIGNAL_4G_H\n\n")
        f.write("#define NO_SIGNAL_4G_W %u\n#define NO_SIGNAL_4G_H %u\n\n" % (w, h))
        f.write("const unsigned char no_signal_4g[%u] = {\n" % len(out))
        for i in range(0, len(out), 16):
            chunk = out[i:i+16]
            f.write("  " + ", ".join("0x%02X" % c for c in chunk) + ",\n")
        f.write("};\n\n#endif /* NO_SIGNAL_4G_H */\n")

    print("Wrote %s (%u x %u, %u bytes)" % (out_path, w, h, len(out)))
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python png_to_4g_header.py <no_signal.png> [output.h]", file=sys.stderr)
        sys.exit(1)
    ok = png_to_4g(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
    sys.exit(0 if ok else 1)
