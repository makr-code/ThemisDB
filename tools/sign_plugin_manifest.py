#!/usr/bin/env python3
"""
Public repository guard: plugin manifest signing is private.
"""

import sys


def main() -> int:
    print(
        "ERROR: Plugin manifest signing has been moved to a private owner-controlled repository.\n"
        "Use plugins/private/themisdb-signing-tools for Ed25519 manifest signing.",
        file=sys.stderr,
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

