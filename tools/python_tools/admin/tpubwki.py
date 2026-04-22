#!/usr/bin/env python3
"""
ThemisDB Wiki Publisher (8.3 name: tpubwki.py)

Dual-mode:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): --action export
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

import yaml


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_DOCS_DIR = REPO_ROOT / "docs"
DEFAULT_OUT_DIR = REPO_ROOT / "wiki_out"
DEFAULT_MKDOCS_YML = REPO_ROOT / "mkdocs.yml"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB wiki publisher (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tpubwki.py\n"
            "  python tpubwki.py --action export\n"
            "  python tpubwki.py --action export --docs-dir ./docs --out-dir ./wiki_out"
        ),
    )
    parser.add_argument("--action", choices=["export"], help="Action in CLI mode")
    parser.add_argument("--docs-dir", type=Path, default=DEFAULT_DOCS_DIR, help="Source docs directory")
    parser.add_argument("--source", type=Path, help="Legacy alias for --docs-dir")
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR, help="Output wiki directory")
    parser.add_argument("--mkdocs", type=Path, default=DEFAULT_MKDOCS_YML, help="mkdocs.yml path")
    parser.add_argument("--filter", type=str, default="", help="Legacy compatibility option (currently informational)")
    parser.add_argument("--dry-run", action="store_true", help="Show resolved paths and exit without writing files")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    return parser.parse_args()


def load_nav(mkdocs_yml: Path):
    with open(mkdocs_yml, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)
    return cfg.get("nav", [])


def ensure_clean_out(out_dir: Path):
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)


def copy_docs(docs_dir: Path, out_dir: Path):
    # Copy all markdown and assets preserving structure
    for root, _, files in os.walk(docs_dir):
        rel = Path(root).relative_to(docs_dir)
        (out_dir / rel).mkdir(parents=True, exist_ok=True)
        for fn in files:
            src = Path(root) / fn
            if src.suffix.lower() in {".md", ".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp"}:
                dst = out_dir / rel / fn
                shutil.copy2(src, dst)


def write_sidebar(nav, out_dir: Path):
    sidebar = []

    def add_item(item, level=0):
        indent = "  " * level
        if isinstance(item, dict):
            for title, value in item.items():
                if isinstance(value, list):
                    sidebar.append(f"{indent}- {title}")
                    for child in value:
                        add_item(child, level + 1)
                else:
                    # value is path
                    link = path_to_wiki_link(value)
                    sidebar.append(f"{indent}- [{title}]({link})")
        else:
            # string path without label
            link = path_to_wiki_link(item)
            sidebar.append(f"{indent}- [{Path(item).stem}]({link})")

    for entry in nav:
        add_item(entry, 0)

    (out_dir / "_Sidebar.md").write_text("\n".join(sidebar) + "\n", encoding="utf-8")


def path_to_wiki_link(p):
    p = str(p)
    if p == "index.md":
        return "Home.md"
    return p


def make_home(docs_dir: Path, out_dir: Path):
    # index.md -> Home.md
    src = docs_dir / "index.md"
    if src.exists():
        shutil.copy2(src, out_dir / "Home.md")


def export_wiki(docs_dir: Path, out_dir: Path, mkdocs_yml: Path, dry_run: bool = False, filter_name: str = "") -> int:
    if not docs_dir.exists() or not docs_dir.is_dir():
        print(f"ERROR: docs directory not found: {docs_dir}", file=sys.stderr)
        return 4
    if not mkdocs_yml.exists() or not mkdocs_yml.is_file():
        print(f"ERROR: mkdocs config not found: {mkdocs_yml}", file=sys.stderr)
        return 4

    if dry_run:
        print("Dry-run mode: no files written")
        print(f"docs_dir={docs_dir}")
        print(f"out_dir={out_dir}")
        print(f"mkdocs={mkdocs_yml}")
        if filter_name:
            print(f"filter={filter_name}")
        return 0

    ensure_clean_out(out_dir)
    copy_docs(docs_dir, out_dir)
    make_home(docs_dir, out_dir)
    nav = load_nav(mkdocs_yml)
    write_sidebar(nav, out_dir)
    print(f"Wiki export prepared at: {out_dir}")
    return 0


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Wiki Publisher (tpubwki)")
    print("=" * 31)
    docs = Path(ask("Docs directory", str(DEFAULT_DOCS_DIR)))
    out = Path(ask("Output directory", str(DEFAULT_OUT_DIR)))
    mkdocs = Path(ask("mkdocs.yml path", str(DEFAULT_MKDOCS_YML)))
    return export_wiki(docs, out, mkdocs)


def main() -> int:
    args = parse_args()
    docs_dir = args.source if args.source is not None else args.docs_dir

    if args.action is None and not args.headless:
        return interactive_mode()

    if args.action != "export":
        print("ERROR: --action export is required in CLI mode", file=sys.stderr)
        return 2

    return export_wiki(docs_dir, args.out_dir, args.mkdocs, dry_run=args.dry_run, filter_name=args.filter)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
