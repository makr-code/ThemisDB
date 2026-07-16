#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from _teqbase import (
    ActionResult,
    config_migration_scan,
    create_parser,
    discover_instances,
    emit,
    embed_certificate,
    health_check,
    list_models,
    migrate_vector_json,
    run_gui,
    sha256_file,
)

TITLE = "Classification Dashboard (Python)"
ACTIONS = ['health', 'info']
KIND = "health"


def handle(action: str, args: argparse.Namespace) -> ActionResult:
    if action == "info":
        return ActionResult(True, f"{TITLE}: Python implementation active", {"kind": KIND})

    if KIND == "health":
        return health_check(args.base_url)

    if KIND == "cfgscan":
        root = Path(args.path or ".")
        patterns = [p.strip() for p in args.patterns.split(",") if p.strip()]
        return config_migration_scan(root, patterns)

    if KIND == "discover":
        ports = [int(p.strip()) for p in args.ports.split(",") if p.strip()]
        return discover_instances(args.subnet, args.host_start, args.host_end, ports)

    if KIND == "models":
        return list_models(Path(args.path or "models"))

    if KIND == "embed":
        if not args.path:
            return ActionResult(False, "--path is required for embed", code=2)
        return embed_certificate(Path(args.path), args.symbol)

    if KIND == "migrate":
        if not args.path:
            return ActionResult(False, "--path is required for migrate", code=2)
        out = Path(args.output) if args.output else Path(args.path).with_name("migrated_" + Path(args.path).name)
        return migrate_vector_json(Path(args.path), out)

    if KIND == "hash":
        if not args.path:
            return ActionResult(False, "--path is required", code=2)
        p = Path(args.path)
        if not p.exists() or not p.is_file():
            return ActionResult(False, f"File not found: {p}", code=2)
        return ActionResult(True, "SHA256 calculated", {"file": str(p), "sha256": sha256_file(p)})

    return ActionResult(False, f"Unsupported tool kind: {KIND}", code=3)


def main() -> int:
    parser = create_parser(TITLE, ACTIONS)
    args = parser.parse_args()

    if not args.action and not args.headless:
        return run_gui(TITLE, ACTIONS, handle, args)

    if not args.action:
        return emit(ActionResult(False, "--action is required in CLI mode", code=2), args.format)

    return emit(handle(args.action, args), args.format)


if __name__ == "__main__":
    raise SystemExit(main())