#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
checker="${repo_root}/scripts/ci-check-llm-skip-budget.sh"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

success_log="${tmp_dir}/success.log"
printf '[ RUN      ] Example.Passes\n[       OK ] Example.Passes\n' > "$success_log"
bash "$checker" llm 0 "$success_log"

at_budget_log="${tmp_dir}/at-budget.log"
printf '[ RUN      ] Example.Skips\n[  SKIPPED ] Example.Skips (fixture unavailable)\n' > "$at_budget_log"
bash "$checker" llm 1 "$at_budget_log"

if bash "$checker" llm 0 "$at_budget_log" >/dev/null 2>&1; then
    echo "skip checker accepted a skip above the module budget" >&2
    exit 1
fi

empty_log="${tmp_dir}/empty.log"
: > "$empty_log"
if bash "$checker" llm 0 "$empty_log" >/dev/null 2>&1; then
    echo "skip checker accepted an empty test log" >&2
    exit 1
fi

if bash "$checker" llm -1 "$success_log" >/dev/null 2>&1; then
    echo "skip checker accepted a negative skip budget" >&2
    exit 1
fi

echo "LLM skip-budget checker covers passing, boundary, over-budget, and invalid evidence cases."
