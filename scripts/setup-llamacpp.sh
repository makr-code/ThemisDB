#!/usr/bin/env bash
# Initialize the reviewed fork revision, never a moving upstream branch.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THEMIS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
git -C "$THEMIS_ROOT" submodule update --init --depth 1 -- llama.cpp
cmake -DPROJECT_SOURCE_DIR="$THEMIS_ROOT" -P "$THEMIS_ROOT/cmake/LlamaCppSubmoduleCheck.cmake"
echo "Pinned llama.cpp submodule ready."
