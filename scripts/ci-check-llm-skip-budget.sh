#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <module> <maximum-skips> <gtest-console-log>" >&2
    exit 2
fi

module="$1"
maximum_skips="$2"
console_log="$3"

if [[ ! "$module" =~ ^[a-z][a-z0-9_-]*$ ]]; then
    echo "ERROR: invalid module name: ${module}" >&2
    exit 2
fi
if [[ ! "$maximum_skips" =~ ^[0-9]+$ ]]; then
    echo "ERROR: maximum-skips must be a non-negative integer" >&2
    exit 2
fi
if [[ ! -s "$console_log" ]]; then
    echo "ERROR: ${module} test log is missing or empty: ${console_log}" >&2
    exit 1
fi

run_count="$(grep -Fc '[ RUN      ]' "$console_log" || true)"
skip_count="$(grep -Fc '[  SKIPPED ]' "$console_log" || true)"

if (( run_count == 0 )); then
    echo "ERROR: ${module} test log contains no executed GoogleTests" >&2
    exit 1
fi
if (( skip_count > maximum_skips )); then
    echo "ERROR: ${module} skip budget exceeded: ${skip_count} skipped, maximum ${maximum_skips}" >&2
    exit 1
fi

echo "${module} skip budget: ${skip_count}/${maximum_skips} (executed ${run_count} GoogleTests)"
