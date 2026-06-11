import argparse
import os
from pathlib import Path
from typing import List, Set

# Optional: better .gitignore support
try:
    import pathspec
    PATHSPEC_AVAILABLE = True
except ImportError:
    PATHSPEC_AVAILABLE = False


def load_gitignore_spec(root_dir: Path) -> "pathspec.PathSpec | None":
    gitignore_path = root_dir / ".gitignore"
    if not gitignore_path.exists():
        return None
    try:
        lines = gitignore_path.read_text(encoding="utf-8").splitlines()
        lines = [line.strip() for line in lines if line.strip() and not line.strip().startswith("#")]
        if PATHSPEC_AVAILABLE:
            return pathspec.PathSpec.from_lines("gitwildmatch", lines)
        return lines  # fallback
    except Exception:
        return None


def should_ignore_path(rel_path: str, gitignore_spec, hardcoded_ignores: Set[str]) -> bool:
    rel_str = str(rel_path).replace("\\", "/")

    # Hardcoded safety ignores
    if any(rel_str == ign or rel_str.startswith(ign + "/") for ign in hardcoded_ignores):
        return True

    if gitignore_spec is None:
        return False

    if PATHSPEC_AVAILABLE:
        return gitignore_spec.match_file(rel_str)
    else:
        import fnmatch
        for pattern in gitignore_spec:
            if fnmatch.fnmatch(rel_str, pattern) or fnmatch.fnmatch(rel_str, pattern.rstrip("/") + "/**"):
                return True
        return False


def build_directory_tree(root_dir: Path, gitignore_spec, hardcoded_ignores: Set[str], only_included: bool = False) -> str:
    """Build ASCII tree. If only_included=True, only show files that were actually collected."""
    lines: List[str] = [f"{root_dir.name}/"]
    included_files = set() if only_included else None

    # If we want only included files, we need the list first (we'll pass it later)
    def _recurse(path: Path, prefix: str = ""):
        try:
            entries = sorted(path.iterdir(), key=lambda p: p.name.lower())
        except (PermissionError, FileNotFoundError):
            return

        for i, entry in enumerate(entries):
            rel_to_root = entry.relative_to(root_dir)
            if should_ignore_path(rel_to_root, gitignore_spec, hardcoded_ignores):
                continue

            is_last = i == len(entries) - 1
            connector = "└── " if is_last else "├── "
            name = entry.name + ("/" if entry.is_dir() else "")

            # For "included only" tree, skip files that weren't collected
            if only_included and entry.is_file():
                if rel_to_root not in included_files:
                    continue

            lines.append(prefix + connector + name)

            if entry.is_dir():
                extension = "    " if is_last else "│   "
                _recurse(entry, prefix + extension)

    _recurse(root_dir)
    return "\n".join(lines)


def collect_code_files(root_dir: Path, gitignore_spec, hardcoded_ignores: Set[str], allowed_extensions: Set[str]) -> List[Path]:
    files: List[Path] = []

    for root, dirs, filenames in os.walk(root_dir):
        rel_root = Path(root).relative_to(root_dir)
        dirs[:] = [d for d in dirs if not should_ignore_path(rel_root / d, gitignore_spec, hardcoded_ignores)]

        for filename in filenames:
            file_path = Path(root) / filename
            rel_path = file_path.relative_to(root_dir)

            if should_ignore_path(rel_path, gitignore_spec, hardcoded_ignores):
                continue

            suffix = file_path.suffix.lower()
            if suffix in allowed_extensions or (suffix == "" and is_text_file(file_path)):
                files.append(file_path)

    return sorted(files, key=lambda p: str(p.relative_to(root_dir)))


def is_text_file(filepath: Path) -> bool:
    try:
        with open(filepath, "rb") as f:
            chunk = f.read(8192)
        chunk.decode("utf-8")
        return True
    except Exception:
        return False


def get_language_for_file(file_path: Path) -> str:
    ext = file_path.suffix.lower()
    lang_map = {
        ".py": "python", ".pyw": "python",
        ".js": "javascript", ".mjs": "javascript", ".cjs": "javascript",
        ".ts": "typescript", ".jsx": "jsx", ".tsx": "tsx",
        ".html": "html", ".htm": "html",
        ".css": "css", ".scss": "scss", ".sass": "sass", ".less": "less",
        ".json": "json", ".yaml": "yaml", ".yml": "yaml", ".toml": "toml",
        ".xml": "xml", ".md": "markdown", ".markdown": "markdown",
        ".sh": "bash", ".bash": "bash", ".zsh": "bash",
        ".bat": "batch", ".cmd": "batch", ".ps1": "powershell",
        ".cpp": "cpp", ".cxx": "cpp", ".cc": "cpp", ".c": "c",
        ".h": "c", ".hpp": "cpp", ".hxx": "cpp",
        ".go": "go", ".rs": "rust", ".java": "java",
        ".kt": "kotlin", ".kts": "kotlin", ".rb": "ruby",
        ".php": "php", ".sql": "sql",
        ".gitignore": "gitignore",
    }
    return lang_map.get(ext, "text")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Gather codebase into one file for Claude - with .gitignore support + included files tree"
    )
    parser.add_argument("directory", nargs="?", default=".", help="Project root")
    parser.add_argument("-o", "--output", default="combined_codebase.md",
                        help="Output markdown file")
    parser.add_argument("--no-gitignore", action="store_true",
                        help="Disable .gitignore (use only default ignores)")

    args = parser.parse_args()

    root_dir = Path(args.directory).resolve()
    if not root_dir.is_dir():
        print(f"❌ Error: '{root_dir}' is not a valid directory.")
        return

    hardcoded_ignores = {
        ".git", "__pycache__", ".pytest_cache", ".mypy_cache", ".ruff_cache",
        "node_modules", "venv", ".venv", "env", ".env",
        "build", "dist", "target", "out", "logs", "coverage", "htmlcov",
        ".idea", ".vscode", ".DS_Store",
    }

    print(f"🔍 Scanning: {root_dir}")

    gitignore_spec = None
    if not args.no_gitignore:
        gitignore_spec = load_gitignore_spec(root_dir)
        if gitignore_spec and PATHSPEC_AVAILABLE:
            print("✅ Using .gitignore rules (build/, assets/, etc. will be skipped)")
        elif gitignore_spec:
            print("✅ .gitignore loaded (basic fallback mode)")

    allowed_extensions = {
        ".py", ".pyw", ".js", ".mjs", ".cjs", ".ts", ".jsx", ".tsx",
        ".html", ".htm", ".css", ".scss", ".sass", ".less",
        ".json", ".yaml", ".yml", ".toml", ".xml",
        ".md", ".markdown", ".txt",
        ".sh", ".bash", ".zsh", ".bat", ".cmd", ".ps1",
        ".cpp", ".cxx", ".cc", ".c", ".h", ".hpp", ".hxx",
        ".go", ".rs", ".java", ".kt", ".kts", ".rb", ".php", ".sql",
    }

    code_files = collect_code_files(root_dir, gitignore_spec, hardcoded_ignores, allowed_extensions)

    # Build both trees
    full_tree = build_directory_tree(root_dir, gitignore_spec, hardcoded_ignores, only_included=False)

    # For the included-only tree we need to mark which files were kept
    included_set = {f.relative_to(root_dir) for f in code_files}
    # Override the global for the second tree (simple trick)
    global included_files
    included_files = included_set
    included_tree = build_directory_tree(root_dir, gitignore_spec, hardcoded_ignores, only_included=True)

    output_path = Path(args.output).resolve()

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("# 🚀 Complete Codebase for Claude\n\n")
        f.write(f"**Project:** `{root_dir}`\n")
        f.write(f"**Generated:** {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M')}\n")
        if gitignore_spec:
            f.write("**Ignores:** .gitignore + default safety rules\n")
        f.write(f"**Files included:** {len(code_files)}\n\n")

        f.write("Copy everything below and paste into Claude.\n\n")
        f.write("---\n\n")

        f.write("## 📁 Full Directory Structure (with ignores applied)\n\n")
        f.write("```\n")
        f.write(full_tree)
        f.write("\n```\n\n")
        f.write("---\n\n")

        f.write(f"## 📄 All File Contents ({len(code_files)} files)\n\n")

        for file_path in code_files:
            rel_path = file_path.relative_to(root_dir)
            try:
                content = file_path.read_text(encoding="utf-8")
                lang = get_language_for_file(file_path)

                f.write(f"### `{rel_path}`\n\n")
                f.write(f"```{lang}\n")
                f.write(content.rstrip() + "\n")
                f.write("```\n\n")
                f.write("---\n\n")
            except Exception as e:
                f.write(f"### `{rel_path}` *(read error: {type(e).__name__})*\n\n")
                f.write("---\n\n")

        # === NEW SECTION: Tree of only included files ===
        f.write("## 📁 Included Files Tree (what Claude actually sees)\n\n")
        f.write("This tree shows **only** the files that were included in this document.\n\n")
        f.write("```\n")
        f.write(included_tree)
        f.write("\n```\n\n")

    print(f"✅ Done! Output saved to:\n   {output_path}")
    print(f"   Files included: {len(code_files)}")
    print("\nClaude now has:")
    print("   • Full project tree (with .gitignore respected)")
    print("   • Every included file with syntax highlighting")
    print("   • Clean tree at the end showing exactly what was sent")

    if not PATHSPEC_AVAILABLE:
        print("\n💡 Tip: For perfect .gitignore support run:")
        print("   pip install pathspec")


if __name__ == "__main__":
    main()
