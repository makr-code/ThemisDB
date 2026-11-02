#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${repo_root}/scripts/ci-download-tinyllama.sh"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

candidate="${tmp_dir}/model.gguf"
printf 'pinned test model\n' > "$candidate"
expected="$(sha256_of "$candidate")"

verify_sha256 "$candidate" "$expected" >/dev/null

if verify_sha256 "$candidate" "0000000000000000000000000000000000000000000000000000000000000000" >/dev/null 2>&1; then
    echo "checksum verifier accepted a mismatched model" >&2
    exit 1
fi

echo "TinyLlama checksum verifier accepts matching hashes and rejects mismatches."
