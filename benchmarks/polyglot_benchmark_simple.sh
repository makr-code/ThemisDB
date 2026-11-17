#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This polyglot benchmark is retired: its former implementation ignored request failures,' \
  'used unsupported API paths, and extrapolated unmeasured competitor performance.' \
  'No benchmark was run and no report was generated.' \
  'For measured ThemisDB HTTP workloads, see benchmarks/README.md.' >&2
exit 2
