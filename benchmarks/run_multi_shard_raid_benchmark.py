"""Retired Multi-Shard RAID benchmark entry point."""

import sys


RETIREMENT_MESSAGE = (
    "Retired: this runner used unverified API paths, counted missing reads as "
    "successful, and emitted placeholder resource metrics. No benchmark was "
    "run and no report was generated."
)


def main() -> int:
    print(RETIREMENT_MESSAGE, file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
