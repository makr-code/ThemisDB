#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This CHIMERA comparative benchmark is retired: its former implementation ignored operation failures,' \
  'estimated p99 from average latency, and emitted unsupported database conclusions.' \
  'No benchmark was run and no report was generated.' \
  'Use a supported ThemisDB HTTP runner from benchmarks/README.md.' >&2
exit 2
