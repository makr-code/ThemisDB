#!/usr/bin/env bash
# =============================================================================
# download-tinyllama-hf.sh
# Download TinyLlama GGUF directly from Hugging Face.
#
# Environment variables:
#   MODEL_DIR      target directory (default: ./models)
#   OUTPUT_NAME    output filename (default: tinyllama.gguf)
#   FORCE          1 = force re-download, 0 = keep cached file (default: 0)
#   LOCAL_SEARCH_DIRS colon-separated local search dirs for fallback
#                     (default: /opt/local-models/models:/opt/local-models/llama-cpp-models)
#   LOCAL_MODEL_GLOB shell glob for local model name matching
#                    (default: *tinyllama*.gguf)
#   HF_URL         optional override URL
#   EXPECTED_SHA256 optional expected SHA256 checksum
#   STRICT_SHA256  1 = fail on checksum mismatch, else warn (default: 0)
#   MAX_RETRIES    retry count (default: 3)
# =============================================================================

set -euo pipefail

HF_URL_DEFAULT="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
HF_URL="${HF_URL:-$HF_URL_DEFAULT}"
MODEL_DIR="${MODEL_DIR:-$(pwd)/models}"
OUTPUT_NAME="${OUTPUT_NAME:-tinyllama.gguf}"
DEST="${MODEL_DIR}/${OUTPUT_NAME}"
FORCE="${FORCE:-0}"
EXPECTED_SHA256="${EXPECTED_SHA256:-}"
STRICT_SHA256="${STRICT_SHA256:-0}"
MAX_RETRIES="${MAX_RETRIES:-3}"
LOCAL_SEARCH_DIRS="${LOCAL_SEARCH_DIRS:-/opt/local-models/models:/opt/local-models/llama-cpp-models}"
LOCAL_MODEL_GLOB="${LOCAL_MODEL_GLOB:-*tinyllama*.gguf}"

log()  { echo "[download-tinyllama-hf] $*"; }
warn() { echo "[download-tinyllama-hf] WARNING: $*" >&2; }
err()  { echo "[download-tinyllama-hf] ERROR: $*" >&2; }

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

gguf_magic_ok() {
    local file="$1"
    local magic
    magic=$(od -An -N4 -tx1 "$file" | tr -d ' \n' | tr '[:lower:]' '[:upper:]')
    [[ "$magic" == "47475546" || "$magic" == "46554747" ]]
}

copy_local_fallback_model() {
    local dst="$1"
    local dir
    local candidate

    IFS=':' read -r -a local_dirs <<< "$LOCAL_SEARCH_DIRS"
    for dir in "${local_dirs[@]}"; do
        if [ -z "$dir" ] || [ ! -d "$dir" ]; then
            continue
        fi

        # Prefer explicit default.gguf first, then TinyLlama-like names.
        candidate=""
        if [ -f "$dir/default.gguf" ]; then
            candidate="$dir/default.gguf"
        else
            candidate=$(find "$dir" -type f \( -iname "*tinyllama*.gguf" -o -name "$LOCAL_MODEL_GLOB" \) | head -n 1 || true)
            if [ -z "$candidate" ]; then
                continue
            fi
        fi

        if [ ! -f "$candidate" ]; then
            continue
        fi
            if ! gguf_magic_ok "$candidate"; then
                warn "Skipping local candidate with invalid GGUF header: $candidate"
                continue
            fi

            log "Using local TinyLlama fallback: $candidate"
            cp -f "$candidate" "$dst"
            return 0
    done

    return 1
}

download_with_retry() {
    local url="$1"
    local dst="$2"
    local attempt=1
    local backoff=5

    while [ "$attempt" -le "$MAX_RETRIES" ]; do
        log "Download attempt ${attempt}/${MAX_RETRIES}: ${url}"
        # Use HTTP/1.1 to avoid intermittent HTTP/2 CANCEL errors observed
        # on large Hugging Face transfers. Resume partial downloads across
        # attempts to avoid restarting from byte 0.
        if curl \
            --fail --location \
            --http1.1 \
            --connect-timeout 30 \
            --max-time 3600 \
            --retry 8 \
            --retry-all-errors \
            --retry-delay 2 \
            --continue-at - \
            --output "$dst" \
            "$url"; then
            return 0
        fi
        if [ "$attempt" -ge "$MAX_RETRIES" ]; then
            break
        fi
        warn "Attempt ${attempt} failed; retrying in ${backoff}s"
        sleep "$backoff"
        backoff=$((backoff * 2))
        attempt=$((attempt + 1))
    done
    return 1
}

mkdir -p "$MODEL_DIR"

if [ "$FORCE" != "1" ] && [ -f "$DEST" ]; then
    size=$(wc -c < "$DEST" | tr -d ' ')
    if [ "$size" -gt 100000000 ]; then
        log "Cached model found: $DEST"
        exit 0
    fi
    warn "Cached file exists but is too small (${size} bytes); re-downloading"
    rm -f "$DEST"
fi

if [ "$FORCE" != "1" ]; then
    if copy_local_fallback_model "$DEST"; then
        log "Local fallback model copied to: $DEST"
        exit 0
    fi
    log "No local TinyLlama GGUF found; falling back to Hugging Face"
fi

tmp="${DEST}.tmp.$$"
trap 'rm -f "$tmp"' EXIT

if ! download_with_retry "$HF_URL" "$tmp"; then
    err "Unable to download TinyLlama from Hugging Face"
    err "URL: $HF_URL"
    exit 1
fi

if ! gguf_magic_ok "$tmp"; then
    err "Downloaded file is not a valid GGUF"
    exit 1
fi

mv "$tmp" "$DEST"
log "Downloaded model: $DEST"

if [ -n "$EXPECTED_SHA256" ]; then
    actual="$(sha256_of "$DEST")"
    if [ "$actual" != "$EXPECTED_SHA256" ]; then
        msg="SHA256 mismatch expected=$EXPECTED_SHA256 actual=$actual"
        if [ "$STRICT_SHA256" = "1" ]; then
            err "$msg"
            exit 1
        fi
        warn "$msg"
    else
        log "SHA256 check passed"
    fi
fi

exit 0
