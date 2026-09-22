#!/usr/bin/env python3
# Usage: python wallpaper.py ../assets/wallpaper/wallpaper.jpg ../assets/wallpaper/wallpaper.h [--dim 0.7]
#        python wallpaper.py ../assets/wallpaper/wallpaper.jpg ../assets/wallpaper/wallpaper.h 
#
# Crops/resizes the input image to 240x240 (centre-crop to square first,
# so nothing looks stretched), optionally dims it so clock hands stay
# legible on top, and emits a const uint16_t array in RGB565 format
# ready to #include into main.c.

import sys
import argparse
from PIL import Image

W = H = 240

def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--dim", type=float, default=1.0,
                     help="brightness scale 0-1, e.g. 0.7 to darken for hand contrast")
    ap.add_argument("--name", default="wallpaper", help="C array name")
    args = ap.parse_args()

    img = Image.open(args.input).convert("RGB")

    # centre-crop to square, then resize to 240x240
    w, h = img.size
    side = min(w, h)
    left = (w - side) // 2
    top = (h - side) // 2
    img = img.crop((left, top, left + side, top + side)).resize((W, H), Image.LANCZOS)

    pixels = img.load()
    out = []
    for y in range(H):
        for x in range(W):
            r, g, b = pixels[x, y]
            if args.dim != 1.0:
                r = int(r * args.dim)
                g = int(g * args.dim)
                b = int(b * args.dim)
            out.append(to_rgb565(r, g, b))

    guard = args.name.upper() + "_H"
    with open(args.output, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"static const uint16_t {args.name}[{W * H}] = {{\n")
        for i in range(0, len(out), 12):
            row = out[i:i + 12]
            f.write("    " + ", ".join(f"0x{v:04X}" for v in row) + ",\n")
        f.write("};\n\n")
        f.write(f"#endif // {guard}\n")

    print(f"Wrote {args.output}: {W}x{H} RGB565 array '{args.name}'")

if __name__ == "__main__":
    main()