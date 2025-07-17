import argparse
from pathlib import Path
import subprocess
import sys

# Configuration
FLASH_SIZE = "8MB"
CHIP = "esp32s3"
FRAMEWORK_DIR = Path.home() / ".platformio/packages/framework-arduinoespressif32"
ESPT_TOOL = "platformio/tool-esptoolpy"


def compile_demo(demo_path: Path):
    """Compile the demo located at demo_path."""

    # Clean the project first
    print(f"Cleaning demo: {demo_path.name}")
    cmd = ["pio", "run", "-d", str(demo_path), "--target", "clean"]
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to clean {demo_path.name}: {e}")
        sys.exit(1)

    # Compile the project
    print(f"Building demo: {demo_path.name}")
    cmd = ["pio", "run", "-d", str(demo_path)]
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to compile {demo_path.name}: {e}")
        sys.exit(1)


def merge_bin_for_demo(demo_path: Path, output_root: Path):
    """Generate merged-flash.bin for a given demo."""
    build_dir = demo_path / ".pio/build/esp32"
    output_bin = output_root / f"{demo_path.name}.bin"

    # Paths to required binaries
    bootloader_bin = build_dir / "bootloader.bin"
    partitions_bin = build_dir / "partitions.bin"
    boot_app0_bin = FRAMEWORK_DIR / "tools/partitions/boot_app0.bin"
    firmware_bin = build_dir / "firmware.bin"

    # Check if required files exist
    for file in [bootloader_bin, partitions_bin, boot_app0_bin, firmware_bin]:
        if not file.exists():
            print(f"!!!!!!!! Required file not found: {file}")
            return

    # Construct the esptool.py command
    cmd = [
        "pio",
        "pkg",
        "exec",
        "--package",
        ESPT_TOOL,
        "--",
        "esptool.py",
        "--chip",
        CHIP,
        "merge_bin",
        "-o",
        str(output_bin),
        "--flash_size",
        FLASH_SIZE,
        "0x0000",
        str(bootloader_bin),
        "0x8000",
        str(partitions_bin),
        "0xe000",
        str(boot_app0_bin),
        "0x10000",
        str(firmware_bin),
    ]

    # Run the command
    try:
        subprocess.run(cmd, check=True)
        print(f"Generated merged binary for {demo_path.name} at {output_bin}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to generate merged binary for {demo_path.name}: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate binary for a specified folder."
    )
    parser.add_argument(
        "folders", type=Path, nargs="+", help="Path(s) to the demo folder(s)"
    )
    parser.add_argument(
        "--only-merge",
        action="store_true",
        help="Only merge the binary without compiling the demo",
    )

    args = parser.parse_args()

    for folder in args.folders:
        folder = Path(folder)

        if not folder.is_dir():
            print(f"Specified path is not a directory: {folder}")
            sys.exit(1)

        current_dir = Path.cwd()
        output_root = current_dir / "build"
        output_root.mkdir(exist_ok=True)

        print(f"Processing demo: {folder.name}")
        if not args.only_merge:
            compile_demo(folder)
        merge_bin_for_demo(folder, output_root)


if __name__ == "__main__":
    main()
