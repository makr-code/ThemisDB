#!/usr/bin/env bash
# verify-vcpkg-baseline-signature.sh — verify that vcpkg builtin-baseline is a signed commit
#
# Usage:
#   verify-vcpkg-baseline-signature.sh [vcpkg-json-path] [evidence-output-json]
#
# Exit codes:
#   0  baseline commit exists and is cryptographically verified by GitHub
#   1  verification failed / baseline missing / commit not signed
#   2  invalid usage

set -euo pipefail

PYTHON_BIN="${PYTHON_BIN:-}"
if [[ -z "${PYTHON_BIN}" ]]; then
    if command -v python3 >/dev/null 2>&1; then
        PYTHON_BIN="python3"
    elif command -v python >/dev/null 2>&1; then
        PYTHON_BIN="python"
    else
        echo "[verify-vcpkg-baseline] ERROR: no Python interpreter found (tried python3 and python)" >&2
        exit 1
    fi
fi

VCPKG_JSON_PATH="${1:-vcpkg.json}"
OUTPUT_JSON_PATH="${2:-sbom-vcpkg-baseline-verification.json}"

if [[ ! -f "${VCPKG_JSON_PATH}" ]]; then
    echo "[verify-vcpkg-baseline] ERROR: file not found: ${VCPKG_JSON_PATH}" >&2
    exit 2
fi

BASELINE="$(
"${PYTHON_BIN}" - "${VCPKG_JSON_PATH}" <<'PYTHON'
import json
import re
import sys

path = sys.argv[1]
with open(path, encoding="utf-8") as f:
    doc = json.load(f)

baseline = doc.get("builtin-baseline")
if not baseline:
    raise SystemExit("missing builtin-baseline in vcpkg.json")
if not re.fullmatch(r"[0-9a-f]{40}", baseline):
    raise SystemExit(f"builtin-baseline is not a 40-char lowercase SHA: {baseline}")
print(baseline)
PYTHON
)"

echo "[verify-vcpkg-baseline] builtin-baseline: ${BASELINE}"

RESPONSE_JSON="$(mktemp)"
trap 'rm -f "${RESPONSE_JSON}"' EXIT

if [[ "${CI:-}" != "true" && -n "${VCPKG_BASELINE_VERIFICATION_RESPONSE:-}" ]]; then
    # Test-only override: allowed only outside CI to support local integration tests.
    # Production and CI runs always use the live GitHub API.
    cp "${VCPKG_BASELINE_VERIFICATION_RESPONSE}" "${RESPONSE_JSON}"
else
    CURL_ARGS=(-fsSL
        -H "Accept: application/vnd.github+json"
        -H "X-GitHub-Api-Version: 2022-11-28"
    )
    # Authenticated requests get 10 000 req/hr vs 60 req/hr for anonymous calls.
    if [[ -n "${GITHUB_TOKEN:-}" ]]; then
        CURL_ARGS+=(-H "Authorization: Bearer ${GITHUB_TOKEN}")
    fi
    curl "${CURL_ARGS[@]}" \
        "https://api.github.com/repos/microsoft/vcpkg/commits/${BASELINE}" \
        > "${RESPONSE_JSON}"
fi

"${PYTHON_BIN}" - "${RESPONSE_JSON}" "${BASELINE}" "${OUTPUT_JSON_PATH}" <<'PYTHON'
import json
import sys

response_path, baseline, output_path = sys.argv[1:4]
with open(response_path, encoding="utf-8") as f:
    commit = json.load(f)

resolved_sha = (commit.get("sha") or "").lower()
if resolved_sha != baseline:
    raise SystemExit(
        f"baseline mismatch: expected {baseline}, got {resolved_sha or '<empty>'}"
    )

verification = commit.get("commit", {}).get("verification", {}) or {}
verified = bool(verification.get("verified"))
reason = verification.get("reason") or "unknown"
signature = verification.get("signature") or ""

if not verified:
    raise SystemExit(
        f"commit is not cryptographically verified by GitHub (reason={reason})"
    )
if "BEGIN PGP SIGNATURE" not in signature:
    raise SystemExit("commit is verified but does not expose a PGP signature payload")

evidence = {
    "repository": "microsoft/vcpkg",
    "builtin_baseline": baseline,
    "resolved_sha": resolved_sha,
    "verified": verified,
    "verification_reason": reason,
    "commit_html_url": commit.get("html_url"),
}

with open(output_path, "w", encoding="utf-8") as out:
    json.dump(evidence, out, indent=2, sort_keys=True)
    out.write("\n")

print(
    "[verify-vcpkg-baseline] Verified signed vcpkg baseline commit:",
    evidence["resolved_sha"],
)
print("[verify-vcpkg-baseline] Evidence written to:", output_path)
PYTHON
