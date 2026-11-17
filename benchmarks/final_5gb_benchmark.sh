#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This final 5GB polyglot benchmark is retired: its former implementation used an unsafe' \
  'fixed output path, unchecked requests, extrapolated performance, and unsupported comparisons.' \
  'No benchmark was run and no report was generated.' >&2
exit 2
