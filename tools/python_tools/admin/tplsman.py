#!/usr/bin/env python3
"""
Plugin Manifest Signer (8.3 name: tplsman.py)

Dual-mode:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): pass --manifest
"""

from __future__ import annotations

import argparse
import sys
import hashlib
import os


def parse_args() -> argparse.Namespace:
        parser = argparse.ArgumentParser(
                description="ThemisDB plugin manifest signer (dual-mode UI + CLI)",
                formatter_class=argparse.RawTextHelpFormatter,
                epilog=(
                        "Examples:\n"
                        "  python tplsman.py\n"
                        "  python tplsman.py --manifest plugins/blob/filesystem/plugin.json"
                ),
        )
        parser.add_argument("--manifest", type=str, help="Path to plugin.json")
        parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
        return parser.parse_args()

def compute_sha256(file_path):
    """Compute SHA256 hash of a file"""
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def sign_manifest(manifest_path):
    """Generate signature file for a manifest"""
    if not os.path.exists(manifest_path):
        print(f"Error: Manifest not found: {manifest_path}", file=sys.stderr)
        return False
    
    # Compute hash
    hash_value = compute_sha256(manifest_path)
    
    # Write signature file
    sig_path = manifest_path + ".sig"
    with open(sig_path, "w") as f:
        f.write(hash_value + "\n")
    
    print(f"✓ Generated signature for {manifest_path}")
    print(f"  SHA256: {hash_value}")
    print(f"  Signature file: {sig_path}")
    
    return True


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Plugin Manifest Signer (tplsman)")
    print("=" * 40)
    manifest = ask("Path to plugin.json", "")
    if not manifest:
        print("ERROR: manifest path is required", file=sys.stderr)
        return 2
    return 0 if sign_manifest(manifest) else 4

def main():
    args = parse_args()

    if args.manifest is None and not args.headless:
        return interactive_mode()

    if not args.manifest:
        print("ERROR: --manifest is required in CLI mode", file=sys.stderr)
        return 2

    if not sign_manifest(args.manifest):
        return 4

    return 0

if __name__ == "__main__":
    sys.exit(main())
