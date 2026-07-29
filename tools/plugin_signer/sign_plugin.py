#!/usr/bin/env python3
"""
Public repository guard: plugin signing is private.
"""

import sys


def main() -> int:
    print(
        "ERROR: Plugin binary signing has been moved to a private owner-controlled repository.\n"
        "Use plugins/private/themisdb-signing-tools for hyperscaler signing operations.",
        file=sys.stderr,
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

