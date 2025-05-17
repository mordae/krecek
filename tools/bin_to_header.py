import os
import re
from typing import BinaryIO

import click


def sanitize_identifier(name: str):
    return re.sub(r"[^a-zA-Z0-9_]+", "_", name)


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
    """Convert binary file to C header with a static array."""
    try:
        os.makedirs(os.path.dirname(output), exist_ok=True)
        var_name = sanitize_identifier(name).lower()
        data = input_file.read()

        with open(output, "w") as f:
            f.write("#pragma once\n")
            f.write("#include <stdint.h>\n\n")

            f.write(f"#define {var_name.upper()}_SIZE {len(data)}\n")

            f.write(
                f"static const uint8_t __attribute__((aligned(4))) {var_name}[{var_name.upper()}_SIZE] = {{\n\t"
            )
            items: list[str] = []

            for byte in data:
                items.append(f"0x{byte:02x}")

            for i, item in enumerate(items):
                if i > 0:
                    if i % 8 == 0:
                        f.write(",\n\t")
                    else:
                        f.write(", ")
                f.write(item)

            f.write("};\n")

    except Exception as e:
        click.echo(f"Error: {str(e)}", err=True)
        raise click.Abort()


if __name__ == "__main__":
    convert()
