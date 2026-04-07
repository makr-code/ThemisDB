#!/usr/bin/env bash
# post_issue_updates.sh — Postet Analyse-Kommentare auf alle PERF-D Issues
# Voraussetzung: gh auth login (mit Schreibrecht auf makr-code/ThemisDB)
#
# Verwendung: bash docs/performance/post_issue_updates.sh

set -euo pipefail

REPO="makr-code/ThemisDB"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/issue_comments" && pwd)"

echo "=== Posting analysis updates to PERF-D issues ==="

post_comment() {
    local issue_num="$1"
    local file="$2"
    echo "→ Posting to #${issue_num} from ${file}..."
    gh issue comment "$issue_num" --repo "$REPO" --body-file "$file"
    echo "  ✅ Done"
}

# D1 — Timeseries Write
post_comment 4432 "$DIR/4432_d1_timeseries_write.md"

# D1-A — AdaptiveFlush existiert bereits
post_comment 4438 "$DIR/4438_d1a_adaptive_flush.md"

# D1-B — Integration TimeSeriesStore
post_comment 4439 "$DIR/4439_d1b_integration.md"

# D1-C — Tests: falsches Benchmark-Ziel
post_comment 4437 "$DIR/4437_d1c_tests.md"

# D2 — Gorilla Decode
post_comment 4435 "$DIR/4435_d2_gorilla_decode.md"

# D3 — Vector Insert
post_comment 4431 "$DIR/4431_d3_vector_insert.md"

# D4 — 2PC
post_comment 4433 "$DIR/4433_d4_2pc.md"

# D5 — Blob Write
post_comment 4434 "$DIR/4434_d5_blob_write.md"

# D7 — Query Engine (No-Op Benchmark)
post_comment 4436 "$DIR/4436_d7_query_engine.md"

echo ""
echo "=== Closing obsolete issues ==="

echo "→ Closing #4438 (AdaptiveFlushController already exists as TSAutoBuffer)..."
gh issue close 4438 --repo "$REPO" \
    --comment "Closing: TSAutoBuffer (src/timeseries/ts_auto_buffer.cpp, 611 Zeilen) implementiert bereits die beschriebene AdaptiveFlushController-Klasse vollständig. Scope auf #4439 (TimeSeriesStore-Anbindung) verlagert."

echo "→ Closing #4436 (D7: no real gap, benchmark is no-op)..."
gh issue close 4436 --repo "$REPO" \
    --comment "Closing: QueryEngineBench/SimpleEvaluation ist ein No-Op-Benchmark (misst nie die Query Engine). Kein echtes Performance-Gap. Echte Query-Metriken (3.43 M ops/s Simple AQL) erfüllen alle Ziele. Ggf. neues Issue für echten Query-Benchmark erstellen."

echo ""
echo "=== All updates posted successfully ==="
