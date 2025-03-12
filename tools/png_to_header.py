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
@click.option(
    "-d",
    "--outdir",
    type=click.Path(
        dir_okay=True,
        file_okay=False,
        writable=True,
        executable=True,
        exists=False,
    ),
    default=".",
    help="Output directory (default: current directory)",
)
def convert(input_file: BinaryIO, outdir: str):
    """Convert PNG image to C header with RGB565 static array"""
    try:
        # Create output directory if it doesn't exist
        os.makedirs(outdir, exist_ok=True)

        base_name = os.path.basename(input_file.name)
        var_name = sanitize_identifier(base_name).lower()
        output_file = os.path.join(outdir, base_name + ".h")

        img = Image.open(input_file).convert("RGB")

        with open(output_file, "w") as f:
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
                f.write(f"\nstatic const sdk_tileset_t ts_{var_name} = {{\n\t")

                f.write(f".width = {img.width},\n\t")
                f.write(f".height = {img.width},\n\t")
                f.write(f".count = {img.height // img.width},\n\t")
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
