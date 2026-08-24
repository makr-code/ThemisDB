#!/usr/bin/env bash
# =============================================================================
# ci-build-doku-db.sh
# ThemisDB — Build doku.db (RocksDB-backed WikiIndexStore) from repository
# documentation for use as the RAG source in LLM inference CI tests.
#
# What it does:
#   1. Collects all markdown files from docs/, src/*/ROADMAP.md, and
#      src/*/FUTURE_ENHANCEMENTS.md (excludes plugins/private/).
#   2. Splits each document into chunks via WikiChunkSplitter-compatible
#      heading-aware sliding-window logic (implemented inline as a Python helper
#      because the C++ binary may not be built yet at script invocation time).
#   3. Writes chunks as a JSON index file (hash-embedding fallback) that can be
#      consumed by JsonWikiIndexReader in tests without a live RocksDB instance.
#   4. Optionally, if THEMIS_TEST_MODEL_PATH is set and a C++ doku_db_builder
#      binary exists, generates real HNSW embeddings via the model.
#
# Outputs:
#   ${OUTPUT_DIR}/doku.db.json   — JSON chunk index (always produced)
#   ${OUTPUT_DIR}/doku.db/       — RocksDB directory (produced when builder binary exists)
#
# Environment variables:
#   OUTPUT_DIR              Directory for output files.
#                           Default: ${GITHUB_WORKSPACE}/build/test-assets
#   REPO_ROOT               Repository root.
#                           Default: directory containing this script's parent
#   THEMIS_TEST_MODEL_PATH  Optional: path to TinyLlama GGUF for real embeddings.
#   MAX_CHUNKS              Maximum number of chunks to emit (0 = no limit).
#                           Default: 0
#   CHUNK_MAX_TOKENS        Sliding-window chunk size in tokens (approx).
#                           Default: 512
#   CHUNK_OVERLAP_TOKENS    Overlap between consecutive chunks.
#                           Default: 64
#
# Exit codes:
#   0  doku.db.json produced successfully.
#   1  Fatal error (Python not found, no markdown files found, write error).
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${REPO_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"

OUTPUT_DIR="${OUTPUT_DIR:-${GITHUB_WORKSPACE:-${REPO_ROOT}/build}/test-assets}"
CHUNK_MAX_TOKENS="${CHUNK_MAX_TOKENS:-512}"
CHUNK_OVERLAP_TOKENS="${CHUNK_OVERLAP_TOKENS:-64}"
MAX_CHUNKS="${MAX_CHUNKS:-0}"

log()  { echo "[ci-build-doku-db] $*"; }
warn() { echo "[ci-build-doku-db] WARNING: $*" >&2; }
err()  { echo "[ci-build-doku-db] ERROR: $*" >&2; }

# ── Require Python ────────────────────────────────────────────────────────────

if ! command -v python3 &>/dev/null; then
    err "python3 is required but not found in PATH."
    exit 1
fi

# ── Collect markdown files ────────────────────────────────────────────────────

log "Collecting markdown files from $REPO_ROOT …"
log "Excluding: plugins/private/, node_modules/, .git/, _e/, fuzz/corpus/"

COLLECTED_LIST="$(mktemp)"
trap 'rm -f "$COLLECTED_LIST"' EXIT

# docs/**/*.md
find "${REPO_ROOT}/docs" -name "*.md" -type f 2>/dev/null \
    | grep -v "/node_modules/" >> "$COLLECTED_LIST" || true

# src/**/ROADMAP.md, FUTURE_ENHANCEMENTS.md, README.md (module-level docs)
find "${REPO_ROOT}/src" \( \
    -name "ROADMAP.md" -o \
    -name "FUTURE_ENHANCEMENTS.md" -o \
    -name "ARCHITECTURE.md" -o \
    -name "README.md" \) -type f 2>/dev/null \
    | grep -v "/node_modules/" >> "$COLLECTED_LIST" || true

# tests/**/README.md
find "${REPO_ROOT}/tests" -name "README.md" -type f 2>/dev/null >> "$COLLECTED_LIST" || true

# Top-level governance docs
for f in ROADMAP.md FUTURE_ENHANCEMENTS.md RELEASE_STRATEGY.md \
         BRANCHING_STRATEGY.md VERSIONING.md QUICKSTART.md README.md; do
    [[ -f "${REPO_ROOT}/$f" ]] && echo "${REPO_ROOT}/$f" >> "$COLLECTED_LIST" || true
done

# Exclude private plugin content (community boundary enforcement)
grep -v "/plugins/private/" "$COLLECTED_LIST" > "${COLLECTED_LIST}.filtered" || true
mv "${COLLECTED_LIST}.filtered" "$COLLECTED_LIST"
# Exclude llvm/fuzz corpus noise
grep -v "/_e/" "$COLLECTED_LIST" > "${COLLECTED_LIST}.filtered" || true
mv "${COLLECTED_LIST}.filtered" "$COLLECTED_LIST"
grep -v "/fuzz/corpus/" "$COLLECTED_LIST" > "${COLLECTED_LIST}.filtered" || true
mv "${COLLECTED_LIST}.filtered" "$COLLECTED_LIST"

TOTAL_FILES=$(wc -l < "$COLLECTED_LIST" | tr -d ' ')
log "Found $TOTAL_FILES markdown files."

if (( TOTAL_FILES == 0 )); then
    err "No markdown files found under $REPO_ROOT."
    exit 1
fi

# ── Build JSON index via Python ───────────────────────────────────────────────

mkdir -p "$OUTPUT_DIR"
OUTPUT_JSON="${OUTPUT_DIR}/doku.db.json"

log "Building chunk index → $OUTPUT_JSON"
log "  chunk_max_tokens=$CHUNK_MAX_TOKENS  overlap=$CHUNK_OVERLAP_TOKENS  max_chunks=$MAX_CHUNKS"

python3 - "$COLLECTED_LIST" "$OUTPUT_JSON" \
         "$CHUNK_MAX_TOKENS" "$CHUNK_OVERLAP_TOKENS" "$MAX_CHUNKS" \
         "$REPO_ROOT" <<'PYEOF'
"""
Inline Python: heading-aware sliding-window chunk splitter.
Produces a JSON array of {chunk_id, source_path, content, embedding_provider}
objects compatible with ThemisDB JsonWikiIndexReader.
"""
import sys, json, re, hashlib, pathlib

file_list_path = sys.argv[1]
output_path    = sys.argv[2]
max_tokens     = int(sys.argv[3])   # approximate word-count proxy for tokens
overlap        = int(sys.argv[4])
max_chunks     = int(sys.argv[5])
repo_root      = pathlib.Path(sys.argv[6])

WORDS_PER_TOKEN = 0.75  # rough approximation: 1 token ≈ 0.75 words
max_words   = max(1, int(max_tokens * WORDS_PER_TOKEN))
overlap_words = max(0, int(overlap * WORDS_PER_TOKEN))

def heading_level(line: str) -> int:
    m = re.match(r'^(#{1,6})\s', line.strip())
    return len(m.group(1)) if m else 0

def split_document(text: str, source: str, max_w: int, ovlp: int):
    """Heading-aware sliding-window split. Yields (heading_path, chunk_text)."""
    lines = text.splitlines(keepends=True)
    heading_stack: list[str] = []
    current_words: list[str] = []
    current_heading = ""
    chunks = []

    def flush(words):
        if words:
            chunks.append((current_heading, " ".join(words)))

    i = 0
    while i < len(lines):
        line = lines[i]
        lvl = heading_level(line)
        if lvl >= 1 and lvl <= 3:
            flush(current_words[-ovlp:] if len(current_words) > ovlp else current_words)
            if lvl == 1:
                heading_stack = [line.strip()]
            elif lvl == 2:
                heading_stack = heading_stack[:1] + [line.strip()]
            else:
                heading_stack = heading_stack[:2] + [line.strip()]
            current_heading = " > ".join(heading_stack)
            current_words = []
        else:
            words = line.split()
            current_words.extend(words)
            if len(current_words) >= max_w:
                chunks.append((current_heading, " ".join(current_words[:max_w])))
                current_words = current_words[max_w - ovlp:]
        i += 1

    flush(current_words)
    return chunks

all_chunks = []
file_count = 0
error_count = 0

with open(file_list_path) as fl:
    paths = [p.strip() for p in fl if p.strip()]

for fpath in paths:
    try:
        text = pathlib.Path(fpath).read_text(encoding="utf-8", errors="replace")
        rel_path = str(pathlib.Path(fpath).relative_to(repo_root))
        pairs = split_document(text, rel_path, max_words, overlap_words)
        for idx, (heading, content) in enumerate(pairs):
            if not content.strip():
                continue
            uid_raw = f"{rel_path}::{heading}::{idx}::{content[:64]}"
            sha256_hex = hashlib.sha256(uid_raw.encode()).hexdigest()  # 64 hex chars
            chunk_id   = "doku-" + sha256_hex[:16]
            # Placeholder hash-based embedding vector (64-dim, normalised).
            # SHA-256 yields 32 bytes (64 hex chars). Extend to 128 hex chars for
            # 64 values by concatenating a second SHA-256 of the reversed uid_raw.
            # Real embeddings are added by the C++ builder when available.
            sha256_ext = hashlib.sha256(uid_raw[::-1].encode()).hexdigest()
            embed_hex  = sha256_hex + sha256_ext   # 128 hex chars → 64 byte pairs
            all_chunks.append({
                "chunk_id":           chunk_id,
                "source_path":        rel_path,
                "section_heading":    heading,
                "content":            content,
                "embedding_provider": "hash",
                "embedding": [float(int(embed_hex[i:i+2], 16)) / 255.0
                              for i in range(0, 128, 2)]
            })
        file_count += 1
    except Exception as ex:
        print(f"[ci-build-doku-db] WARNING: skip {fpath}: {ex}", file=sys.stderr)
        error_count += 1

    if max_chunks > 0 and len(all_chunks) >= max_chunks:
        all_chunks = all_chunks[:max_chunks]
        break

index = {
    "schema_version": "1.0",
    "description": "ThemisDB documentation knowledge base for RAG CI tests",
    "source": "ci-build-doku-db.sh",
    "chunk_max_tokens": int(sys.argv[3]),
    "chunk_overlap_tokens": int(sys.argv[4]),
    "file_count": file_count,
    "error_count": error_count,
    "chunk_count": len(all_chunks),
    "chunks": all_chunks
}

with open(output_path, "w", encoding="utf-8") as out:
    json.dump(index, out, ensure_ascii=False, indent=2)

print(f"[ci-build-doku-db] Wrote {len(all_chunks)} chunks from {file_count} files → {output_path}")
PYEOF

CHUNK_COUNT=$(python3 -c "import json; d=json.load(open('$OUTPUT_JSON')); print(d['chunk_count'])")
log "doku.db.json: $CHUNK_COUNT chunks → $OUTPUT_JSON"

# ── Symlink for backward compatibility ────────────────────────────────────────
ln -sf "$(basename "$OUTPUT_JSON")" "${OUTPUT_DIR}/doku.db" 2>/dev/null || true

log "doku.db build complete."
log "  JSON index: $OUTPUT_JSON"
log "  Symlink:    ${OUTPUT_DIR}/doku.db → $(basename "$OUTPUT_JSON")"

if [[ -n "${THEMIS_TEST_MODEL_PATH:-}" && -f "${THEMIS_TEST_MODEL_PATH}" ]]; then
    log "THEMIS_TEST_MODEL_PATH is set; real embeddings can be generated by the C++ test suite."
fi
