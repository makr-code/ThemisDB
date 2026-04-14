#!/usr/bin/env bash
# =============================================================================
# download_models.sh
# ThemisDB – LLM / LoRA / gguf model download & artifact preparation
#
# This script is the canonical entry point for preparing LLM/LoRA/gguf model
# artefacts required by ThemisDB benchmarks (Maßnahme #6 –
# PERFORMANCE_EXPECTATIONS.md §1.4).
#
# It delegates all work to scripts/prepare_llm_bench_artifacts.sh, which
# contains the full implementation.
#
# Usage:
#   ./scripts/download_models.sh [--stub-only] [--model-dir DIR] [--help]
#
# Options:
#   --stub-only      Force stub-model mode (sets THEMIS_LLM_STUB_MODELS=ON).
#                    Safe for CI runners without GPU / network access.
#   --model-dir DIR  Override the model directory (sets THEMIS_MODEL_DIR).
#   --help           Show this help text.
#
# Environment variables:
#   THEMIS_MODEL_DIR        Base directory for model files.
#                           Default: ${XDG_DATA_HOME:-$HOME/.local/share}/themis/models
#   THEMIS_LLM_STUB_MODELS  Set to "ON" to use minimal CI stub models.
#
# Exit codes:
#   0  All required artefacts are present (or were successfully created).
#   1  A required artefact is missing and could not be prepared.
#   2  Usage error / bad arguments.
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREPARE_SCRIPT="$SCRIPT_DIR/prepare_llm_bench_artifacts.sh"

if [[ ! -f "$PREPARE_SCRIPT" ]]; then
    echo "ERROR: Implementation script not found: $PREPARE_SCRIPT" >&2
    exit 1
fi

exec bash "$PREPARE_SCRIPT" "$@"
