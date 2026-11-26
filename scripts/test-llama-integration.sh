#!/usr/bin/env bash
# Verify the real gitlink and the missing/unpatched/mismatched source contracts.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THEMIS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${THEMIS_ROOT}/build/llama-submodule-contract-$(uname -s)"
cmake -DPROJECT_SOURCE_DIR="$THEMIS_ROOT" -P "$THEMIS_ROOT/cmake/LlamaCppSubmoduleCheck.cmake"
cmake -S "$THEMIS_ROOT/tests/cmake/llama_submodule" -B "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" -C Release --no-tests=error --output-on-failure
