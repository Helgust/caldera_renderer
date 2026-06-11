"""Scan assets/scenes for .gltf files and refresh the Scene launch options.

The renderer is launched with `--scene <gltf-path>` (see source/main.cpp). The
VS Code workflow stores the pickable scenes under `launchOption.options.Scene`
in .vscode/settings.json. Keeping that list in sync by hand is tedious, so this
script rebuilds it from whatever .gltf files currently live on disk.

Only ASCII .gltf is matched on purpose: SceneLoader::load calls
tinygltf::LoadASCIIFromFile, so binary .glb files would parse-fail at runtime.

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


def to_display_name(raw: str) -> str:
    """Turn a folder/file stem like 'damaged-helmet' into 'Damaged Helmet'."""
    return raw.replace("-", " ").replace("_", " ").title()


def find_scenes() -> List[Dict[str, str]]:
    """Return [{name, value}, ...] for every .gltf under assets/scenes."""
    scenes: List[Dict[str, str]] = []
    for gltf in sorted(SCENES_DIR.rglob("*.gltf")):
        rel = gltf.relative_to(SCENES_DIR)
        # Convention is one scene per sub-folder, so name after the folder.
        # A .gltf sitting loose directly in scenes/ falls back to its stem.
        base = rel.parts[0] if len(rel.parts) > 1 else gltf.stem
        scenes.append({
            "name": to_display_name(base),
            # Paths are relative to the repo root with forward slashes, to
            # match the existing entries and stay platform-agnostic.
            "value": gltf.relative_to(ROOT_DIR).as_posix(),
        })

    # Disambiguate any folders that hold more than one .gltf.
    seen: Dict[str, int] = {}
    for scene in scenes:
        seen[scene["name"]] = seen.get(scene["name"], 0) + 1
    for scene in scenes:
        if seen[scene["name"]] > 1:
            stem = Path(scene["value"]).stem
            scene["name"] = f"{scene['name']} ({to_display_name(stem)})"

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
