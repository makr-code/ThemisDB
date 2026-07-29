#!/usr/bin/env python3
"""
Public repository guard: build signing is private.
"""

import sys


def main() -> int:
    print(
        "ERROR: Build signing tooling has been moved to a private owner-controlled repository.\n"
        "Use the private hyperscaler signing pipeline (plugins/private/themisdb-signing-tools).",
        file=sys.stderr,
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

