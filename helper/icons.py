#!/usr/bin/env python3
# Converts up to 8 images into a single RGB565 C header for the icon module.
#
# Lives in helper/, reads from ../assets/icons by default, and picks up
# whatever image files it finds there (any name, any of png/jpg/jpeg/bmp),
# sorted alphabetically, assigning them to icon0_data .. icon7_data in
# that order. Fewer than 8 images is fine — remaining slots get a blank
# black placeholder so the build never breaks.
#
# Usage (run from helper/, or pass an explicit folder):
#   python icons.py
#   python icons.py ../assets/icons
#
# Output: assets/icons/icon_assets.h

import argparse
from pathlib import Path
from PIL import Image

ICON_SIZE = 40
ICON_COUNT = 8
EXTS = {".png", ".jpg", ".jpeg", ".bmp"}


def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_icon(path):
    img = Image.open(path).convert("RGB")
    w, h = img.size
    side = min(w, h)
    left = (w - side) // 2
    top = (h - side) // 2
    img = img.crop((left, top, left + side, top + side)).resize(
        (ICON_SIZE, ICON_SIZE), Image.LANCZOS
    )
    pixels = img.load()
    out = []
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            r, g, b = pixels[x, y]
            out.append(to_rgb565(r, g, b))
    return out


def blank_icon():
    return [0x0000] * (ICON_SIZE * ICON_SIZE)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "folder",
        nargs="?",
        default=str(Path(__file__).resolve().parent.parent / "assets" / "icons"),
        help="folder containing the source images (default: ../assets/icons relative to this script)",
    )
    args = ap.parse_args()

    folder = Path(args.folder)
    if not folder.is_dir():
        raise SystemExit(f"folder not found: {folder}")

    out_path = folder / "icon_assets.h"

    images = sorted(
        [p for p in folder.iterdir() if p.suffix.lower() in EXTS],
        key=lambda p: p.name.lower(),
    )

    if len(images) > ICON_COUNT:
        print(f"found {len(images)} images, only using the first {ICON_COUNT}")
    images = images[:ICON_COUNT]

    with open(out_path, "w") as f:
        f.write("#ifndef ICON_ASSETS_H\n#define ICON_ASSETS_H\n\n")
        f.write("#include <stdint.h>\n\n")

        for idx in range(ICON_COUNT):
            if idx < len(images):
                pixels = load_icon(images[idx])
                print(f"icon{idx}: {images[idx].name}")
            else:
                pixels = blank_icon()
                print(f"icon{idx}: missing, using blank placeholder")

            f.write(f"static const uint16_t icon{idx}_data[{ICON_SIZE * ICON_SIZE}] = {{\n")
            for i in range(0, len(pixels), 12):
                row = pixels[i:i + 12]
                f.write("    " + ", ".join(f"0x{v:04X}" for v in row) + ",\n")
            f.write("};\n\n")

        f.write("#endif // ICON_ASSETS_H\n")

    print(f"\nWrote {out_path}")


if __name__ == "__main__":
    main()