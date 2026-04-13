#!/usr/bin/env bash
# =============================================================================
# prepare_llm_bench_artifacts.sh
# ThemisDB – LLM / LoRA / gguf benchmark artifact preparation (Maßnahme #6)
#
# Usage:
#   ./scripts/prepare_llm_bench_artifacts.sh [--stub-only] [--model-dir DIR]
#
# Options:
#   --stub-only      Force stub-model mode even when real models are available.
#                    Equivalent to setting THEMIS_LLM_STUB_MODELS=ON.
#   --model-dir DIR  Override the model directory (sets THEMIS_MODEL_DIR).
#   --help           Show this help text.
#
# Environment variables:
#   THEMIS_MODEL_DIR        Base directory for model files.
#                           Default: ${XDG_DATA_HOME:-$HOME/.local/share}/themis/models
#   THEMIS_LLM_STUB_MODELS  Set to "ON" to use stub (minimal CI) models.
#                           Automatically set to "ON" when --stub-only is passed.
#
# Exit codes:
#   0  All required artifacts are present (or were successfully created).
#   1  A required artifact is missing and no fallback is available.
#   2  Usage error / bad arguments.
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG_FILE="$REPO_ROOT/benchmarks/llm_bench_config.json"

DEFAULT_MODEL_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/themis/models"
MODEL_DIR="${THEMIS_MODEL_DIR:-$DEFAULT_MODEL_DIR}"
STUB_ONLY="${THEMIS_LLM_STUB_MODELS:-OFF}"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --stub-only)
            STUB_ONLY="ON"
            shift
            ;;
        --model-dir)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --model-dir requires an argument." >&2
                exit 2
            fi
            MODEL_DIR="$2"
            shift 2
            ;;
        --help|-h)
            sed -n '2,/^# =\+$/{ /^# =\+$/d; s/^# \{0,1\}//; p }' "$0"
            exit 0
            ;;
        *)
            echo "ERROR: Unknown argument: $1" >&2
            echo "Run '$0 --help' for usage." >&2
            exit 2
            ;;
    esac
done

export THEMIS_MODEL_DIR="$MODEL_DIR"
export THEMIS_LLM_STUB_MODELS="$STUB_ONLY"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
log()  { echo "[prepare_llm_bench_artifacts] $*"; }
warn() { echo "[prepare_llm_bench_artifacts] WARN: $*" >&2; }
die()  { echo "[prepare_llm_bench_artifacts] ERROR: $*" >&2; exit 1; }

check_command() {
    command -v "$1" >/dev/null 2>&1
}

# ---------------------------------------------------------------------------
# Validate config file
# ---------------------------------------------------------------------------
if [[ ! -f "$CONFIG_FILE" ]]; then
    die "Config file not found: $CONFIG_FILE"
fi

if check_command python3; then
    python3 -c "import json, sys; json.load(open('$CONFIG_FILE'))" \
        || die "Config file is not valid JSON: $CONFIG_FILE"
fi

log "Using config:    $CONFIG_FILE"
log "Model directory: $MODEL_DIR"
log "Stub models:     $STUB_ONLY"

# ---------------------------------------------------------------------------
# Create model directory layout
# ---------------------------------------------------------------------------
mkdir -p "$MODEL_DIR/gguf"
mkdir -p "$MODEL_DIR/lora"

# ---------------------------------------------------------------------------
# Stub model: tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
# ---------------------------------------------------------------------------
STUB_MODEL_PATH="$MODEL_DIR/gguf/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
STUB_MODEL_URL="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
export STUB_MODEL_PATH

ensure_stub_model() {
    if [[ -f "$STUB_MODEL_PATH" ]]; then
        log "Stub model already present: $STUB_MODEL_PATH"
        return 0
    fi

    log "Stub model not found. Attempting download from HuggingFace..."
    local downloaded=0
    if check_command wget; then
        wget -q --show-progress -O "$STUB_MODEL_PATH" "$STUB_MODEL_URL" \
            && downloaded=1 \
            || { rm -f "$STUB_MODEL_PATH"; warn "wget download failed."; }
    elif check_command curl; then
        curl -fL --progress-bar -o "$STUB_MODEL_PATH" "$STUB_MODEL_URL" \
            && downloaded=1 \
            || { rm -f "$STUB_MODEL_PATH"; warn "curl download failed."; }
    else
        warn "Neither wget nor curl is available. Cannot download stub model."
    fi

    if [[ $downloaded -eq 1 ]]; then
        log "Downloaded stub model successfully."
        return 0
    fi

    # Fallback for offline / air-gapped CI: generate a minimal valid-header
    # GGUF stub so that path-existence checks pass and benchmarks can emit a
    # clean SkipWithError rather than a hard crash.
    log "Download failed. Creating minimal offline GGUF stub at $STUB_MODEL_PATH ..."
    if check_command python3; then
        python3 - <<'PYEOF'
import struct, os, pathlib

out = pathlib.Path(os.environ["STUB_MODEL_PATH"])
out.parent.mkdir(parents=True, exist_ok=True)

# Minimal GGUF v3 header: magic + version + tensor_count + kv_count
# Real loaders will reject this as it has no tensors – benchmarks guarded
# by LLMArtifactPreflight will detect the empty model and SkipWithError.
magic   = b"GGUF"
version = struct.pack("<I", 3)          # GGUF version 3
tensors = struct.pack("<Q", 0)          # 0 tensors
kvpairs = struct.pack("<Q", 0)          # 0 KV pairs
data = magic + version + tensors + kvpairs
out.write_bytes(data)
print(f"[prepare_llm_bench_artifacts] Offline GGUF stub written: {out} ({len(data)} bytes)")
PYEOF
    else
        # Last resort: write magic bytes so the file exists
        printf 'GGUF' > "$STUB_MODEL_PATH"
        warn "python3 not available; wrote bare GGUF magic to stub file."
    fi

    if [[ -f "$STUB_MODEL_PATH" ]]; then
        log "Offline GGUF stub ready: $STUB_MODEL_PATH"
        return 0
    fi

    warn "Could not create offline GGUF stub."
    return 1
}

# ---------------------------------------------------------------------------
# Stub LoRA adapter (minimal binary blob, rank 8)
# Create a placeholder stub if no real adapter exists
# ---------------------------------------------------------------------------
STUB_LORA_PATH="$MODEL_DIR/lora/legal_lora_stub.bin"

ensure_stub_lora() {
    if [[ -f "$STUB_LORA_PATH" ]]; then
        log "Stub LoRA adapter already present: $STUB_LORA_PATH"
        return 0
    fi

    log "Creating minimal stub LoRA adapter at $STUB_LORA_PATH ..."
    if check_command python3; then
        python3 - <<'PYEOF'
import struct, os, pathlib

out = pathlib.Path(os.environ["STUB_LORA_PATH"])
out.parent.mkdir(parents=True, exist_ok=True)

rank = 8
hidden = 64
# Header: magic(4) + version(4) + rank(4) + hidden(4)
data  = struct.pack("<4sIII", b"LORA", 1, rank, hidden)
# Weight bytes: rank * hidden * 2 (float16 zeros)
data += bytes(rank * hidden * 2)
out.write_bytes(data)
print(f"[prepare_llm_bench_artifacts] Stub LoRA written: {out} ({len(data)} bytes)")
PYEOF
    else
        # Fallback: create an empty placeholder so the path check passes
        : > "$STUB_LORA_PATH"
        warn "python3 not available; created empty LoRA placeholder."
    fi
}

export STUB_LORA_PATH

# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------
MISSING=0

if [[ "$STUB_ONLY" == "ON" ]]; then
    log "Stub-model mode enabled (THEMIS_LLM_STUB_MODELS=ON)."
    ensure_stub_model || MISSING=1
    ensure_stub_lora  || MISSING=1
else
    # Real-model mode: check THEMIS_MODEL_DIR for expected files
    REAL_MODEL_PATH="$MODEL_DIR/gguf/Meta-Llama-3-8B-Instruct.Q4_K_M.gguf"
    if [[ -f "$REAL_MODEL_PATH" ]]; then
        log "Real model found: $REAL_MODEL_PATH"
    else
        warn "Real model not found at: $REAL_MODEL_PATH"
        warn "Falling back to stub model for CI compatibility."
        STUB_ONLY="ON"
        export THEMIS_LLM_STUB_MODELS="ON"
        ensure_stub_model || MISSING=1
        ensure_stub_lora  || MISSING=1
    fi
fi

# ---------------------------------------------------------------------------
# Final status
# ---------------------------------------------------------------------------
if [[ $MISSING -ne 0 ]]; then
    die "One or more required artifacts are missing and could not be prepared. " \
        "Set THEMIS_LLM_STUB_MODELS=ON or provide models in THEMIS_MODEL_DIR ($MODEL_DIR)."
fi

log "All LLM/LoRA benchmark artifacts are ready."
log "  THEMIS_MODEL_DIR        = $THEMIS_MODEL_DIR"
log "  THEMIS_LLM_STUB_MODELS  = $THEMIS_LLM_STUB_MODELS"
