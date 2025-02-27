#!/usr/bin/env python

import os.path
import sys
from glob import glob

import click
import yaml


@click.command()
def clangd():
    """Generate .clangd file for local development."""

    assert "PICO_SDK_PATH" in os.environ, "PICO_SDK_PATH not set"

    pico_sdk_path = os.path.realpath(os.environ["PICO_SDK_PATH"])
    src = os.path.realpath(os.path.join(os.getcwd(), "src"))
    build = os.path.realpath(os.path.join(os.getcwd(), "build"))

    includes = [
        *glob(f"{pico_sdk_path}/src/common/*/include"),
        *glob(f"{pico_sdk_path}/src/rp2040/*/include"),
        *glob(f"{pico_sdk_path}/src/rp2350/*/include"),
        *glob(f"{pico_sdk_path}/src/rp2_common/*/include"),
        f"{pico_sdk_path}/lib/tinyusb/src",
        *glob(f"{src}/**/include", recursive=True),
        f"{build}/generated/pico_base",
        f"{build}/sdk/include",
        f"{build}/vendor/*/include",
    ]

    flags = [
        "-Wall",
        "-Wextra",
        "-xc",
        "-DCFG_TUSB_MCU=OPT_MCU_RP2040",
        "-I/usr/arm-none-eabi/include",
    ]

    yaml.safe_dump(
        {
            "CompileFlags": {
                "Compiler": "arm-none-eabi-gcc",
                "Add": [*flags, *[f"-I{inc}" for inc in includes]],
            }
        },
        sys.stdout,
    )


if __name__ == "__main__":
    clangd()
