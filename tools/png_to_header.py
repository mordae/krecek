import os
import re
from typing import BinaryIO, cast

import click
from PIL import Image


def sanitize_identifier(name: str):
    return re.sub(r"[^a-zA-Z0-9_]+", "_", name)


def rgb888_to_rgb565(r, g, b):
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


@click.command()
@click.argument("input_file", type=click.File("rb"))
@click.option("-n", "--name", type=str)
@click.option(
    "-o",
    "--output",
    type=click.Path(dir_okay=False, writable=True, resolve_path=True),
    help="Output file name",
)
def convert(input_file: BinaryIO, name: str, output: str):
    """Convert PNG image to C header with RGB565 static array"""
    try:
        os.makedirs(os.path.dirname(output), exist_ok=True)
        var_name = sanitize_identifier(name).lower()

        img = Image.open(input_file).convert("RGB")

        with open(output, "w") as f:
            f.write("#pragma once\n")
            f.write("#include <sdk/image.h>\n\n")

            f.write(f"#define {var_name.upper()}_WIDTH {img.width}\n")
            f.write(f"#define {var_name.upper()}_HEIGHT {img.height}\n\n")

            f.write(f"static const sdk_image_t image_{var_name} = {{\n\t")
            f.write(f".width = {img.width},\n\t")
            f.write(f".height = {img.height},\n\t")
            f.write(".data = {\n\t\t")

            pixels = []
            for y in range(img.height):
                for x in range(img.width):
                    r, g, b = cast(tuple[int, int, int], img.getpixel((x, y)))
                    rgb565 = rgb888_to_rgb565(r, g, b)
                    pixels.append(f"0x{rgb565:04x}")

            for i, pixel in enumerate(pixels):
                if i > 0:
                    if i % 8 == 0:
                        f.write(",\n\t\t")
                    else:
                        f.write(", ")
                f.write(pixel)

            f.write(",\n\t},\n")
            f.write("};\n")

            if img.height % img.width == 0:
                f.write(
                    f"\n#define TS_{var_name.upper()}_COUNT {img.height // img.width}\n"
                )

                f.write(f"\nstatic const sdk_tileset_t ts_{var_name} = {{\n\t")

                f.write(f".width = {img.width},\n\t")
                f.write(f".height = {img.width},\n\t")
                f.write(f".count = TS_{var_name.upper()}_COUNT,\n\t")
                f.write(".tiles = {\n")

                for i in range(img.height // img.width):
                    f.write(
                        f"\t\timage_{var_name}.data + {img.width * img.width * i},\n"
                    )

                f.write("\t},\n")

                f.write("};\n")

    except Exception as e:
        click.echo(f"Error: {str(e)}", err=True)
        raise click.Abort()


if __name__ == "__main__":
    convert()
