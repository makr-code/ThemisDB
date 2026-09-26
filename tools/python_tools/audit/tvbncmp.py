#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


def _find_repo_root() -> Path:
    current = Path(__file__).resolve()
    for parent in current.parents:
        if (parent / "tools" / "verify_benchmark_mapping.py").is_file():
            return parent
    raise RuntimeError(f"Unable to locate repository root from {current}")


def _load_canonical_module():
    repo_root = _find_repo_root()
    canonical_path = repo_root / "tools" / "verify_benchmark_mapping.py"
    spec = importlib.util.spec_from_file_location("verify_benchmark_mapping", canonical_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load benchmark mapping verifier: {canonical_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    return _load_canonical_module().main()


if __name__ == "__main__":
    sys.exit(main())
