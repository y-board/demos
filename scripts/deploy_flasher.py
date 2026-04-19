"""Generate the demo-flasher repo contents from y-board-v4 demos.

Scans y-board-v4/<demo>/flasher.yml. For each demo with a flasher.yml AND a
matching merged binary in build/<demo>.bin, writes:

    <flasher_root>/bin/<demo>.bin
    <flasher_root>/manifests/<demo>.json   (esp-web-tools format)
    <flasher_root>/assets/<demo>.zip       (only if flasher.yml lists assets)

Then writes <flasher_root>/manifests/index.json — the catalog the UI fetches
to render demo cards. Optionally copies a template directory over the flasher
root (for index.html etc.) so the UI ships from this repo too.

Demos with no flasher.yml are skipped silently. Demos with a flasher.yml but
no built bin trigger a warning.
"""

import argparse
import json
import shutil
import sys
import zipfile
from pathlib import Path

import yaml


def load_demo(demo_dir: Path) -> dict | None:
    flasher_yml = demo_dir / "flasher.yml"
    if not flasher_yml.exists():
        return None
    meta = yaml.safe_load(flasher_yml.read_text())
    if not meta or "display_name" not in meta:
        print(f"!! {demo_dir.name}: flasher.yml missing display_name", file=sys.stderr)
        return None
    return {
        "id": demo_dir.name,
        "dir": demo_dir,
        "display_name": meta["display_name"],
        "icon": meta.get("icon", "📦"),
        "description": meta.get("description", ""),
        "asset_paths": meta.get("assets") or [],
    }


def write_bin(demo: dict, build_dir: Path, flasher_root: Path) -> bool:
    src = build_dir / f"{demo['id']}.bin"
    if not src.exists():
        print(f"!! {demo['id']}: missing built binary at {src}", file=sys.stderr)
        return False
    dst = flasher_root / "bin" / f"{demo['id']}.bin"
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    return True


def write_manifest(demo: dict, version: str, flasher_root: Path) -> None:
    manifest = {
        "name": demo["display_name"],
        "version": version,
        "new_install_prompt_erase": False,
        "builds": [
            {
                "chipFamily": "ESP32-S3",
                "parts": [{"path": f"../bin/{demo['id']}.bin", "offset": 0}],
            }
        ],
    }
    dst = flasher_root / "manifests" / f"{demo['id']}.json"
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text(json.dumps(manifest, indent=4) + "\n")


def write_assets_zip(demo: dict, flasher_root: Path) -> bool:
    """Zip listed assets preserving relative paths inside the demo dir."""
    paths = demo["asset_paths"]
    if not paths:
        return False

    files = []
    for rel in paths:
        full = demo["dir"] / rel
        if not full.exists():
            print(f"!! {demo['id']}: asset {rel} not found", file=sys.stderr)
            continue
        files.append((full, rel))

    if not files:
        return False

    dst = flasher_root / "assets" / f"{demo['id']}.zip"
    dst.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(dst, "w", zipfile.ZIP_DEFLATED) as z:
        for full, rel in files:
            z.write(full, arcname=rel)
    return True


def clean_generated_dirs(flasher_root: Path) -> None:
    """Drop previous bin/manifests/assets so removed demos disappear."""
    for sub in ("bin", "manifests", "assets"):
        d = flasher_root / sub
        if d.exists():
            shutil.rmtree(d)


def copy_template(template_dir: Path, flasher_root: Path) -> None:
    """Copy template files (index.html, css, js) over the flasher root."""
    for src in template_dir.rglob("*"):
        if not src.is_file():
            continue
        rel = src.relative_to(template_dir)
        dst = flasher_root / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--demos-root", type=Path, required=True,
                    help="Path to y-board-v4 directory")
    ap.add_argument("--build-dir", type=Path, required=True,
                    help="Directory containing <demo>.bin merged binaries")
    ap.add_argument("--flasher-root", type=Path, required=True,
                    help="Path to demo-flasher repo to populate")
    ap.add_argument("--version", required=True,
                    help="Manifest version string (e.g. commit short SHA)")
    ap.add_argument("--template-dir", type=Path,
                    help="Optional template dir to copy over the flasher root")
    args = ap.parse_args()

    demo_dirs = sorted(p for p in args.demos_root.iterdir() if p.is_dir())
    demos = [d for d in (load_demo(p) for p in demo_dirs) if d]

    if not demos:
        print("!! no demos with flasher.yml found", file=sys.stderr)
        return 1

    clean_generated_dirs(args.flasher_root)
    if args.template_dir:
        copy_template(args.template_dir, args.flasher_root)

    catalog = []
    for demo in demos:
        if not write_bin(demo, args.build_dir, args.flasher_root):
            continue
        write_manifest(demo, args.version, args.flasher_root)
        has_assets = write_assets_zip(demo, args.flasher_root)
        entry = {
            "id": demo["id"],
            "display_name": demo["display_name"],
            "icon": demo["icon"],
            "description": demo["description"],
            "manifest": f"manifests/{demo['id']}.json",
        }
        if has_assets:
            entry["assets"] = f"assets/{demo['id']}.zip"
        catalog.append(entry)
        print(f"   {demo['id']}: bin + manifest"
              + (" + assets.zip" if has_assets else ""))

    catalog.sort(key=lambda e: e["display_name"].lower())
    index = {"version": args.version, "demos": catalog}
    (args.flasher_root / "manifests").mkdir(parents=True, exist_ok=True)
    (args.flasher_root / "manifests" / "index.json").write_text(
        json.dumps(index, indent=4, ensure_ascii=False) + "\n"
    )
    print(f"Wrote {len(catalog)} demos to {args.flasher_root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
