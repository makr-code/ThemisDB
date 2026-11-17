#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This 5GB polyglot load test is retired: its former implementation used unsupported' \
  'ThemisDB endpoints, ignored database and HTTP failures, and reported incomparable workloads.' \
  'No database was modified and no benchmark report was generated.' \
  'Use a documented ThemisDB HTTP runner from benchmarks/README.md instead.' >&2
exit 2
