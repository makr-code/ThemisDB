#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${repo_root}/scripts/ci-download-tinyllama.sh"

# These values come from the upstream release commit's Git LFS pointer.
[[ "$HF_URL" == "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/c1d7cb837a660d93ba28f936efb148591bfba3e9/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf" ]]
[[ "$EXPECTED_SHA256" == "9fecc3b3cd76bba89d504f29b616eedf7da85b96540e490ca5824d3f7d2776a0" ]]

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

printf 'GGUFpinned test model\n' > "$candidate"
EXPECTED_SHA256="$(sha256_of "$candidate")"
DEST="${tmp_dir}/tinyllama.gguf"

download_with_retry() {
    cp "$candidate" "$2"
}

download_from_huggingface >/dev/null
cmp "$candidate" "$DEST"
rm "$DEST"

printf 'GGUFcorrupted test model\n' > "$candidate"
if download_from_huggingface >/dev/null 2>&1; then
    echo "downloader accepted a GGUF with a mismatched checksum" >&2
    exit 1
fi
[[ ! -e "$DEST" && ! -e "${DEST}.tmp.$$" ]]

echo "TinyLlama downloader publishes only checksum-verified GGUF files."
