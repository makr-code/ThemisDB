#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

mkdir -p "${tmp_dir}/bin" "${tmp_dir}/models"
printf '#!/usr/bin/env bash\nexit 22\n' > "${tmp_dir}/bin/curl"
printf '#!/usr/bin/env bash\nexit 0\n' > "${tmp_dir}/bin/sleep"
chmod +x "${tmp_dir}/bin/curl" "${tmp_dir}/bin/sleep"

printf 'not the pinned model\n' > "${tmp_dir}/models/tinyllama.gguf"
if (
    cd "$repo_root"
    PATH="${tmp_dir}/bin:${PATH}" \
    MODEL_DIR="${tmp_dir}/models" \
    LLM_MODEL=tinyllama \
        bash .github/scripts/prepare-llm-model-cache.sh
); then
    echo "model cache accepted a TinyLlama file with the wrong SHA256" >&2
    exit 1
fi
[[ ! -e "${tmp_dir}/models/tinyllama.gguf" ]]

if (
    cd "$repo_root"
    MODEL_DIR="${tmp_dir}/models" \
    LLM_MODEL=tinyllama \
    LLM_MODEL_URL="https://example.invalid/unpinned.gguf" \
        bash .github/scripts/prepare-llm-model-cache.sh
); then
    echo "model cache accepted an unpinned TinyLlama URL override" >&2
    exit 1
fi

echo "Reusable TinyLlama cache rejects corrupt cached files and unpinned URL overrides."
