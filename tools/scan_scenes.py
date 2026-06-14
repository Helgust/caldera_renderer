"""Scan assets/scenes for .gltf files and refresh the Scene launch options.

The renderer is launched with `--scene <gltf-path>` (see source/main.cpp). The
VS Code workflow stores the pickable scenes under `launchOption.options.Scene`
in .vscode/settings.json. Keeping that list in sync by hand is tedious, so this
script rebuilds it from whatever .gltf files currently live on disk.

Both .gltf and .glb are matched so every Khronos sample variant shows up. Each
scene's display name is 'ModelFolder (variant-subpath)', e.g.
'Damaged Helmet (glTF-Embedded/DamagedHelmet.gltf)'. The sub-path carries the
variant (glTF / glTF-Binary / glTF-Embedded / glTF-Draco / ...) without having
to inspect file contents.

WARNING: SceneLoader::load still calls tinygltf::LoadASCIIFromFile, so .glb
(glTF-Binary) entries are listed for visibility but will fail to load until the
loader also handles LoadBinaryFromFile.

Usage:
    python tools/scan_scenes.py [--dry-run]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List

# Repo root is the parent of tools/, regardless of the current working dir.
ROOT_DIR = Path(__file__).resolve().parent.parent
SCENES_DIR = ROOT_DIR / "assets" / "scenes"
SETTINGS_FILE = ROOT_DIR / ".vscode" / "settings.json"


def find_scenes() -> List[Dict[str, str]]:
    """Return [{name, value}, ...] for every .gltf/.glb under assets/scenes.

    The display name is 'ModelFolder (variant-subpath)', e.g.
    'Damaged Helmet (glTF-Embedded/DamagedHelmet.gltf)'. The variant sub-path is
    unique per file, so the sample variants stay distinct on their own -- no
    extra disambiguation pass is needed.
    """
    scenes: List[Dict[str, str]] = []
    patterns = ("*.gltf", "*.glb")
    files = sorted(f for pattern in patterns for f in SCENES_DIR.rglob(pattern))
    for asset in files:
        rel = asset.relative_to(SCENES_DIR)
        # Convention is one scene per model folder, with the source variant in a
        # sub-folder (glTF / glTF-Binary / glTF-Embedded / ...). Name after the
        # model folder and tag with the variant sub-path so every variant reads
        # distinctly. A file sitting loose in scenes/ falls back to its stem.
        if len(rel.parts) > 1:
            base = rel.parts[0]
            sub = rel.relative_to(base).as_posix()
        else:
            base = asset.stem
            sub = asset.name
        scenes.append({
            "name": f"{base} ({sub})",
            # Paths are relative to the repo root with forward slashes, to
            # match the existing entries and stay platform-agnostic.
            "value": asset.relative_to(ROOT_DIR).as_posix(),
        })

    return scenes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would change without writing settings.json.",
    )
    args = parser.parse_args()

    if not SCENES_DIR.is_dir():
        print(f"error: scenes directory not found: {SCENES_DIR}", file=sys.stderr)
        return 1

    scenes = find_scenes()
    if not scenes:
        print(f"warning: no .gltf files found under {SCENES_DIR}")

    for scene in scenes:
        print(f"  {scene['name']:<24} {scene['value']}")
    print(f"Found {len(scenes)} scene(s).")

    # NOTE: settings.json may use JSONC (comments). json.load tolerates the
    # current comment-free file, but writing back would strip any comments that
    # get added later. Keep settings.json comment-free, or switch to a JSONC
    # parser here if that changes.
    settings = json.loads(SETTINGS_FILE.read_text(encoding="utf-8"))

    settings.setdefault("launchOption.options", {})["Scene"] = scenes

    # Keep the current selection if it still exists; otherwise pick the first.
    values = [s["value"] for s in scenes]
    current = settings.setdefault("launchOption.currentConfig", {})
    if scenes and current.get("Scene") not in values:
        current["Scene"] = values[0]
        print(f"Selection reset to: {values[0]}")

    if args.dry_run:
        print("\n--dry-run: settings.json left unchanged.")
        return 0

    SETTINGS_FILE.write_text(
        json.dumps(settings, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Updated {SETTINGS_FILE.relative_to(ROOT_DIR).as_posix()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
