from pathlib import Path
import subprocess

# Configuration
FLASH_SIZE = "8MB"
CHIP = "esp32s3"
FRAMEWORK_DIR = Path.home() / ".platformio/packages/framework-arduinoespressif32"
ESPT_TOOL = "platformio/tool-esptoolpy"


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
        print(f"Generated merged-flash.bin for {demo_path.name} at {output_bin}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to generate merged-flash.bin for {demo_path.name}: {e}")


def main():
    """Main function to process all demo folders."""
    current_dir = Path.cwd()
    output_root = current_dir / "build"
    output_root.mkdir(exist_ok=True)

    for folder in current_dir.iterdir():
        if (
            folder.is_dir()
            and folder.name is not "build"
            and not folder.name.startswith(".")
        ):
            print(f"Processing demo: {folder.name}")
            merge_bin_for_demo(folder, output_root)


if __name__ == "__main__":
    main()
