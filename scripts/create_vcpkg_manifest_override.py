#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def _normalize_overlay_path(entry: object, working_root: Path) -> str:
    path = Path(str(entry))
    if path.is_absolute():
        return str(path)
    return str((working_root / path).resolve())


def main() -> int:
    parser = argparse.ArgumentParser(description="Create CI-only vcpkg manifest override with builtin-baseline.")
    parser.add_argument("--working-directory", required=True)
    parser.add_argument("--override-dir", required=True)
    parser.add_argument("--baseline", required=True)
    args = parser.parse_args()

    working_root = Path(args.working_directory).resolve()
    override_root = Path(args.override_dir).resolve()
    override_root.mkdir(parents=True, exist_ok=True)

    manifest_path = working_root / "vcpkg.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["builtin-baseline"] = args.baseline
    (override_root / "vcpkg.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    config_path = working_root / "vcpkg-configuration.json"
    if config_path.exists():
        config = json.loads(config_path.read_text(encoding="utf-8"))
        overlays = config.get("overlay-ports", [])
        if isinstance(overlays, list):
            config["overlay-ports"] = [_normalize_overlay_path(entry, working_root) for entry in overlays]
        (override_root / "vcpkg-configuration.json").write_text(
            json.dumps(config, indent=2) + "\n",
            encoding="utf-8",
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
