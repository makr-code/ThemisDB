#!/usr/bin/env bash
set -euo pipefail

printf '%s\n' \
  'This Wire-vs-HTTP stress test is retired: its former implementation timed health checks' \
  'and empty TCP connections, counted failed requests as samples, and emitted unsupported conclusions.' \
  'No load was generated and no benchmark report was created.' >&2
exit 2
