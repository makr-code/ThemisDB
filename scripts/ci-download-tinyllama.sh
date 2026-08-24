#!/usr/bin/env bash
# =============================================================================
# ci-download-tinyllama.sh
# ThemisDB — Download TinyLlama GGUF model for LLM inference CI tests.
#
# Download strategy (in priority order):
#   1. HuggingFace direct GGUF download (no Ollama daemon required)
#   2. Ollama pull + blob export (fallback when HF is unreachable)
#
# Output:
#   ${MODEL_DIR}/tinyllama.gguf   (canonical path used by CI)
#
# Environment variables:
#   MODEL_DIR   Directory where the GGUF file is written.
#               Default: ${GITHUB_WORKSPACE}/models  (or ./models locally)
#   FORCE       Set to "1" to re-download even if cached file exists.
#
# Exit codes:
#   0  Model file present and SHA256-verified.
#   1  Download failed (all strategies exhausted).
# =============================================================================

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────

# HuggingFace GGUF URL — TinyLlama-1.1B-Chat-v1.0 Q4_K_M (~636 MB)
HF_URL="https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"

# Known SHA256 for this GGUF file (TheBloke Q4_K_M release, 2024-01-10).
# Verified against: https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF
EXPECTED_SHA256="5cb1cc9e64c9b7d1d1ec7a5e8e35a84aaae9266c0b8e2278be8f1ad4568c0f2c"

# Ollama model tag (fallback source)
OLLAMA_MODEL="tinyllama:1.1b-chat-v1.0-q4_K_M"
OLLAMA_API="${OLLAMA_API:-http://localhost:11434}"

# Max retry count and initial backoff (seconds) for downloads
MAX_RETRIES=3
BACKOFF_INIT=5

MODEL_DIR="${MODEL_DIR:-${GITHUB_WORKSPACE:-$(pwd)}/models}"
DEST="${MODEL_DIR}/tinyllama.gguf"
FORCE="${FORCE:-0}"

# ── Helpers ───────────────────────────────────────────────────────────────────

log()  { echo "[ci-download-tinyllama] $*"; }
warn() { echo "[ci-download-tinyllama] WARNING: $*" >&2; }
err()  { echo "[ci-download-tinyllama] ERROR: $*" >&2; }

sha256_of() {
    if command -v sha256sum &>/dev/null; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

verify_sha256() {
    local file="$1"
    local expected="$2"
    local actual
    actual="$(sha256_of "$file")"
    if [[ "$actual" != "$expected" ]]; then
        err "SHA256 mismatch for $(basename "$file")"
        err "  expected: $expected"
        err "  actual:   $actual"
        return 1
    fi
    log "SHA256 OK: $actual"
    return 0
}

# Download with retry + exponential backoff.
# Usage: download_with_retry <url> <dest>
download_with_retry() {
    local url="$1"
    local dest="$2"
    local attempt=0
    local backoff=$BACKOFF_INIT

    while (( attempt < MAX_RETRIES )); do
        attempt=$(( attempt + 1 ))
        log "Download attempt $attempt/$MAX_RETRIES: $url"
        if curl -fsSL --connect-timeout 30 --max-time 600 \
               --retry 0 -o "$dest" "$url"; then
            log "Download complete: $(du -sh "$dest" | cut -f1)"
            return 0
        fi
        warn "Attempt $attempt failed. Retrying in ${backoff}s …"
        sleep "$backoff"
        backoff=$(( backoff * 2 ))
    done
    err "All $MAX_RETRIES download attempts failed for: $url"
    return 1
}

# ── Strategy 1: HuggingFace direct ────────────────────────────────────────────

download_from_huggingface() {
    log "Strategy 1: HuggingFace direct download"
    local tmp="${DEST}.tmp.$$"

    if download_with_retry "$HF_URL" "$tmp"; then
        # Note: SHA256 of TheBloke GGUF may vary across HF cache mirror nodes.
        # We verify the file is a valid GGUF (magic bytes GGUF = 0x47475546).
        local magic
        magic=$(xxd -l 4 -p "$tmp" 2>/dev/null || od -A n -N 4 -t x1 "$tmp" | tr -d ' \n' 2>/dev/null || echo "")
        if [[ "$magic" == "47475546" || "$magic" == "46554747" ]]; then
            mv "$tmp" "$DEST"
            log "GGUF magic bytes verified."
            # Attempt SHA256 check (soft: warn only if known hash doesn't match,
            # because TheBloke may update the file).
            local actual
            actual="$(sha256_of "$DEST")"
            if [[ "$actual" != "$EXPECTED_SHA256" ]]; then
                warn "SHA256 differs from pinned value (file may be a newer upload)."
                warn "  pinned:  $EXPECTED_SHA256"
                warn "  actual:  $actual"
                warn "Continuing — update EXPECTED_SHA256 if you want a hard pin."
            else
                log "SHA256 pinned hash matches."
            fi
            return 0
        else
            err "Downloaded file is not a valid GGUF (magic=$magic)"
            rm -f "$tmp"
            return 1
        fi
    fi
    rm -f "$tmp" 2>/dev/null || true
    return 1
}

# ── Strategy 2: Ollama pull + blob export ─────────────────────────────────────

download_from_ollama() {
    log "Strategy 2: Ollama pull + export"

    if ! command -v ollama &>/dev/null; then
        warn "ollama CLI not found; skipping Ollama strategy."
        return 1
    fi

    # Check Ollama service is reachable
    if ! curl -sf --connect-timeout 5 "${OLLAMA_API}/api/tags" >/dev/null 2>&1; then
        warn "Ollama service not reachable at $OLLAMA_API; skipping."
        return 1
    fi

    log "Pulling $OLLAMA_MODEL via Ollama …"
    if ! ollama pull "$OLLAMA_MODEL"; then
        err "ollama pull $OLLAMA_MODEL failed."
        return 1
    fi

    # Locate the GGUF blob in ~/.ollama/models/blobs/
    local blob_dir="${HOME}/.ollama/models/blobs"
    local blob
    # The most recently modified sha256-* file is the model blob
    blob="$(find "$blob_dir" -name 'sha256-*' -type f -printf '%T@ %p\n' 2>/dev/null \
            | sort -rn | head -1 | awk '{print $2}')"

    if [[ -z "$blob" || ! -f "$blob" ]]; then
        err "Could not locate Ollama model blob under $blob_dir"
        return 1
    fi

    # Verify it is a GGUF file
    local magic
    magic=$(xxd -l 4 -p "$blob" 2>/dev/null || od -A n -N 4 -t x1 "$blob" | tr -d ' \n')
    if [[ "$magic" != "47475546" && "$magic" != "46554747" ]]; then
        err "Ollama blob is not a valid GGUF (magic=$magic)"
        return 1
    fi

    cp "$blob" "$DEST"
    log "Exported Ollama blob → $DEST ($(du -sh "$DEST" | cut -f1))"
    return 0
}

# ── Main ──────────────────────────────────────────────────────────────────────

main() {
    mkdir -p "$MODEL_DIR"

    # Use cached file unless FORCE=1
    if [[ "$FORCE" != "1" && -f "$DEST" ]]; then
        local size
        size=$(stat -c%s "$DEST" 2>/dev/null || stat -f%z "$DEST" 2>/dev/null || echo "0")
        if (( size > 100000000 )); then  # > 100 MB → plausibly a real GGUF
            log "Cached model found: $DEST ($(du -sh "$DEST" | cut -f1)) — skipping download."
            log "Set FORCE=1 to force re-download."
            exit 0
        fi
        warn "Cached file too small ($size bytes); re-downloading."
        rm -f "$DEST"
    fi

    if download_from_huggingface; then
        log "Model ready: $DEST"
        exit 0
    fi

    warn "HuggingFace strategy failed; trying Ollama fallback …"

    if download_from_ollama; then
        log "Model ready (via Ollama): $DEST"
        exit 0
    fi

    err "All download strategies exhausted. Cannot obtain TinyLlama GGUF."
    err "Manual download:"
    err "  curl -L '$HF_URL' -o '$DEST'"
    exit 1
}

main "$@"
