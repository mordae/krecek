#!/usr/bin/env python

import os.path
import sys
from glob import glob

import click
import yaml


@click.command()
def clangd():
    """Generate .clangd file for local development."""

    src = os.path.realpath(os.path.join(os.getcwd(), "src"))
    host = os.path.realpath(os.path.join(os.getcwd(), "host"))
    build = os.path.realpath(os.path.join(os.getcwd(), "build"))

    includes = [
        f"{host}/sdk/include",
        f"{src}/sdk/include",
        f"{src}/vendor/pico-tft/include",
        *glob(f"{build}/**/assets", recursive=True),
    ]

    flags = [
        "-Wall",
        "-Wextra",
        "-xc",
        "-DCFG_TUSB_MCU=OPT_MCU_RP2040",
        "-I/usr/include/SDL2",
        "-I/usr/include/freetype2",
        "-DTFT_SCALE=2",
        "-DTFT_DRIVE=TFT_DRIVER_ILI9341",
        "-DTFT_VSYNC=1",
        "-DTFT_SCALE=2",
        "-DTFT_SWAP_XY=1",
    ]

    yaml.safe_dump(
        {
            "CompileFlags": {
                "Compiler": "gcc",
                "Add": [*flags, *[f"-I{inc}" for inc in includes]],
            }
        },
        sys.stdout,
    )


if __name__ == "__main__":
    clangd()
