"""Retired quick-start entry point for the Multi-Shard RAID benchmark."""

import sys


RETIREMENT_MESSAGE = (
    "Retired: the Multi-Shard RAID benchmark infrastructure and workload are "
    "not validated. No cluster was started and no benchmark was run."
)


def main() -> int:
    print(RETIREMENT_MESSAGE, file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
