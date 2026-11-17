#!/usr/bin/env python3
"""Retired entry point for unsupported specialized database comparisons."""

import argparse
import sys


RETIREMENT_MESSAGE = (
    "This comparative runner is disabled: its previous version used fabricated "
    "competitor timings, unchecked ThemisDB requests, and did not validate results. "
    "No comparative report was generated. Validated external comparison drivers "
    "remain open work; see benchmarks/BENCHMARK_STANDARDS.md and benchmarks/ROADMAP.md."
)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Retired: specialized comparative results are unavailable until "
            "validated reference database adapters are implemented."
        )
    )
    parser.parse_args(argv)
    print(RETIREMENT_MESSAGE, file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
