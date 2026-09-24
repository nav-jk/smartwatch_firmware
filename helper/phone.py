#!/usr/bin/env python3

import argparse
from pathlib import Path
from PIL import Image

IMAGE_W = 220
IMAGE_H = 175


def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_phone(path):
    img = Image.open(path).convert("RGBA")

    scale = min(
        IMAGE_W / img.width,
        IMAGE_H / img.height
    )

    new_w = int(img.width * scale)
    new_h = int(img.height * scale)

    img = img.resize(
        (new_w, new_h),
        Image.LANCZOS
    )

    canvas = Image.new(
        "RGB",
        (IMAGE_W, IMAGE_H),
        (0, 0, 0)
    )

    x = (IMAGE_W - new_w) // 2
    y = (IMAGE_H - new_h) // 2

    canvas.paste(img, (x, y), img)

    pixels = canvas.load()

    out = []

    for y in range(IMAGE_H):
        for x in range(IMAGE_W):
            r, g, b = pixels[x, y]
            out.append(to_rgb565(r, g, b))

    return out


def main():
    ap = argparse.ArgumentParser()

    ap.add_argument(
        "input",
        help="input phone image"
    )

    ap.add_argument(
        "output",
        help="output C header"
    )

    args = ap.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)

    if not input_path.is_file():
        raise SystemExit(f"file not found: {input_path}")

    pixels = load_phone(input_path)

    with open(output_path, "w") as f:

        f.write("#ifndef PHONE_ASSETS_H\n")
        f.write("#define PHONE_ASSETS_H\n\n")

        f.write("#include <stdint.h>\n\n")

        f.write(f"#define PHONE_IMAGE_W {IMAGE_W}\n")
        f.write(f"#define PHONE_IMAGE_H {IMAGE_H}\n\n")

        f.write(
            f"static const uint16_t phone_image_data"
            f"[{IMAGE_W * IMAGE_H}] = {{\n"
        )

        for i in range(0, len(pixels), 12):
            row = pixels[i:i + 12]

            f.write(
                "    " +
                ", ".join(f"0x{v:04X}" for v in row) +
                ",\n"
            )

        f.write("};\n\n")
        f.write("#endif\n")

    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()