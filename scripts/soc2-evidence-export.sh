#!/usr/bin/env bash
# soc2-evidence-export.sh — SOC 2 Type II automated evidence collection
#
# Phase 6.2: Invokes SecurityEvidenceCollector APIs via the admin REST endpoint,
# downloads the evidence bundle, and writes it to the specified output file.
#
# Usage:
#   soc2-evidence-export.sh [OPTIONS]
#
# Options:
#   --endpoint URL       ThemisDB admin endpoint (default: https://localhost:8443)
#   --window-days N      Evidence window in days (default: 7)
#   --output FILE        Output JSON file path (default: evidence-bundle-YYYYMMDD.json)
#   --admin-token TOKEN  Admin Bearer token for authentication
#
# Exit codes:
#   0  Success
#   1  Export failed or bundle invalid
#   2  ThemisDB endpoint unreachable

set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────────
ENDPOINT="https://localhost:8443"
WINDOW_DAYS=7
OUTPUT="evidence-bundle-$(date +%Y%m%d).json"
ADMIN_TOKEN=""

# ── Argument parsing ───────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --endpoint)      ENDPOINT="$2";     shift 2 ;;
        --window-days)   WINDOW_DAYS="$2";  shift 2 ;;
        --output)        OUTPUT="$2";       shift 2 ;;
        --admin-token)   ADMIN_TOKEN="$2";  shift 2 ;;
        *)               echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

# ── Validate connectivity ──────────────────────────────────────────────────────
echo "[soc2-evidence-export] Checking connectivity to ${ENDPOINT}..."
if ! curl -sk --max-time 10 "${ENDPOINT}/v1/health" | grep -q "ok"; then
    echo "[soc2-evidence-export] ERROR: ThemisDB endpoint unreachable at ${ENDPOINT}" >&2
    exit 2
fi

# ── Build auth header ──────────────────────────────────────────────────────────
AUTH_HEADER=""
if [[ -n "${ADMIN_TOKEN}" ]]; then
    AUTH_HEADER="-H 'Authorization: Bearer ${ADMIN_TOKEN}'"
fi

# ── Trigger evidence export ────────────────────────────────────────────────────
echo "[soc2-evidence-export] Requesting evidence bundle (window: ${WINDOW_DAYS} days)..."
HTTP_STATUS=$(curl -sk --write-out "%{http_code}" \
    --max-time 120 \
    -o "${OUTPUT}" \
    ${AUTH_HEADER:+"-H" "Authorization: Bearer ${ADMIN_TOKEN}"} \
    -H "Content-Type: application/json" \
    -H "X-Themis-Request-Purpose: soc2-evidence-export" \
    "${ENDPOINT}/v1/admin/security-evidence?window_days=${WINDOW_DAYS}")

if [[ "${HTTP_STATUS}" != "200" ]]; then
    echo "[soc2-evidence-export] ERROR: Evidence export returned HTTP ${HTTP_STATUS}" >&2
    cat "${OUTPUT}" >&2 || true
    exit 1
fi

# ── Validate bundle structure ─────────────────────────────────────────────────
echo "[soc2-evidence-export] Validating bundle structure..."
python3 - <<PYTHON
import json, sys
try:
    with open("${OUTPUT}") as f:
        bundle = json.load(f)
    required = ["bundle_id", "from_ms", "to_ms", "audit_log", "signature"]
    missing = [k for k in required if k not in bundle]
    if missing:
        print(f"ERROR: Bundle missing keys: {missing}", file=sys.stderr)
        sys.exit(1)
    print(f"Bundle OK: id={bundle['bundle_id']}")
except json.JSONDecodeError as e:
    print(f"ERROR: Invalid JSON in bundle: {e}", file=sys.stderr)
    sys.exit(1)
PYTHON

echo "[soc2-evidence-export] Evidence bundle written to: ${OUTPUT}"
echo "[soc2-evidence-export] Bundle size: $(wc -c < "${OUTPUT}") bytes"
