import os
import os.path
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

        img = Image.open(input_file).convert("RGBA")

        if img.height % img.width == 0:
            # Legacy tile size detection.
            tile_width = img.width
            tile_height = img.width
        else:
            # One large tile.
            tile_width = img.width
            tile_height = img.height

        # If file name includes WxH, use it instead.
        m = re.match(r".*?([0-9]+)x([0-9]+).*", os.path.basename(input_file.name))
        if m:
            tile_width = int(m.group(1))
            tile_height = int(m.group(2))

        x_tiles = img.width / tile_width
        y_tiles = img.height / tile_height

        assert not (x_tiles % 1.0), "Wrong image width for given tile width"
        assert not (y_tiles % 1.0), "Wrong image height for given tile height"

        x_tiles = int(x_tiles)
        y_tiles = int(y_tiles)

        with open(output, "w") as f:
            f.write("#pragma once\n")
            f.write("#include <sdk/image.h>\n\n")

            f.write(f"// From {os.path.basename(input_file.name)}\n")
            f.write(f"// That had {x_tiles}x{y_tiles} tiles\n")
            f.write(f"// Sized {tile_width}x{tile_height}\n\n")

            f.write(f"static const color_t raw_{var_name}_data[] = {{\n")

            for ty in range(y_tiles):
                for tx in range(x_tiles):
                    f.write(f"\t// Tile {tx}x{ty}\n")

                    for y in range(tile_height):
                        f.write("\t")

                        for x in range(tile_width):
                            r, g, b, a = cast(
                                tuple[int, int, int, int],
                                img.getpixel(
                                    (tx * tile_width + x, ty * tile_height + y)
                                ),
                            )

                            if a < 128:
                                r, g, b = 255, 63, 255

                            rgb565 = rgb888_to_rgb565(r, g, b)
                            f.write(f"{' ' if x else ''}0x{rgb565:04x},")

                        f.write("\n")

            f.write("};\n\n")

            if x_tiles == 1 and y_tiles == 1:
                f.write(f"#define {var_name.upper()}_WIDTH {img.width}\n")
                f.write(f"#define {var_name.upper()}_HEIGHT {img.height}\n\n")

                f.write(f"static const sdk_image_t image_{var_name} = {{\n")
                f.write(f"\t.width = {img.width},\n")
                f.write(f"\t.height = {img.height},\n")
                f.write(f"\t.data = raw_{var_name}_data,\n")
                f.write("};\n\n")

            f.write(f"#define TS_{var_name.upper()}_COUNT {y_tiles * x_tiles}\n\n")

            f.write(f"static const sdk_tileset_t ts_{var_name} = {{\n")
            f.write(f"\t.width = {tile_width},\n")
            f.write(f"\t.height = {tile_height},\n")
            f.write(f"\t.count = TS_{var_name.upper()}_COUNT,\n")
            f.write("\t.tiles = {\n")

            for y in range(y_tiles):
                for x in range(x_tiles):
                    offset = ((y * x_tiles) + x) * (tile_height * tile_width)
                    f.write(f"\t\traw_{var_name}_data + {offset},\n")

            f.write("\t},\n")
            f.write("};\n")

    except Exception as e:
        click.echo(f"Error: {str(e)}", err=True)
        raise click.Abort()


if __name__ == "__main__":
    convert()
