#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This legacy 5GB benchmark coordinator is retired: its load and stress stages used' \
  'unsupported or incomparable workloads and could report success after failed requests.' \
  'No test data, database changes, or reports were produced.' \
  'Choose a supported ThemisDB HTTP runner from benchmarks/README.md.' >&2
exit 2
