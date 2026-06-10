#!/usr/bin/env python3
"""
Compatibility shim for legacy gap_scanner_v3.py invocation.

This module forwards execution to the new uniform scanner orchestrator
(tools/gs3_orchestrator.py) so historical commands continue to work.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Keep behavior stable when executed directly from repository root or tools/.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from gs3_orchestrator import main as gs3_main


class UnifiedGapScannerV3:
    """Backward-compatible facade for older integrations."""

    def __init__(self, repo_root: str = ".", output_dir: str = "ai_working"):
        self.repo_root = repo_root
        self.output_dir = output_dir

    def run_complete_scan(self):
        # Delegate to the modern tools.scanners.UnifiedGapScanner wrapper.
        from tools.scanners import UnifiedGapScanner

        scanner = UnifiedGapScanner(self.repo_root, self.output_dir)
        return scanner.run_complete_scan()


def _legacy_main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("repo_root", nargs="?", default="./src")
    parser.add_argument("output_dir", nargs="?", default="ai_working/gap_scan_results.json")
    parser.add_argument("--help", action="store_true")

    known, _unknown = parser.parse_known_args(argv)

    if known.help:
        print("Use: python tools/gs3_orchestrator.py [source_dir] [--output <file>] [--verbose]")
        return 0

    forwarded = [known.repo_root]
    if known.output_dir:
        forwarded.extend(["--output", known.output_dir])

    return _invoke_gs3(forwarded)


def _invoke_gs3(args: list[str]) -> int:
    old_argv = sys.argv[:]
    try:
        sys.argv = ["gs3_orchestrator.py", *args]
        return gs3_main()
    finally:
        sys.argv = old_argv


def main() -> int:
    return _legacy_main(sys.argv[1:])


if __name__ == "__main__":
    raise SystemExit(main())
