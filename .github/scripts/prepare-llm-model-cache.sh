#!/usr/bin/env bash
set -euo pipefail

INCLUDE_TINYLLAMA="${INCLUDE_TINYLLAMA:-true}"
LLM_MODEL="${LLM_MODEL:-tinyllama}"
LLM_MODEL_URL="${LLM_MODEL_URL:-}"
LLM_MODEL_FILE="${LLM_MODEL_FILE:-}"
TINYLLAMA_FORCE_DOWNLOAD="${TINYLLAMA_FORCE_DOWNLOAD:-false}"
MODEL_DIR="${MODEL_DIR:-${PWD}/models}"

trim_lower() {
    printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

is_truthy() {
    local value
    value="$(trim_lower "$1")"
    [[ "$value" == "1" || "$value" == "true" || "$value" == "on" || "$value" == "yes" ]]
}

resolve_model_url() {
    local model_key="$1"

    case "$model_key" in
        tinyllama|TinyLlama|tinyllama-1.1b)
            printf '%s' "https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf"
            ;;
        qwen2.5-coder|qwen2.5-coder-7b-instruct|Qwen2.5-Coder|Qwen2.5-Coder-7B-Instruct)
            printf '%s' "https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/qwen2.5-coder-7b-instruct-q4_k_m.gguf"
            ;;
        llama3.2|llama-3.2|llama-3.2-3b-instruct|llama-3.2-3b)
            printf '%s' "https://huggingface.co/unsloth/Meta-Llama-3.2-3B-Instruct-GGUF/resolve/main/Meta-Llama-3.2-3B-Instruct-Q4_K_M.gguf"
            ;;
        *)
            return 1
            ;;
    esac
}

mkdir -p "$MODEL_DIR"

if ! is_truthy "$INCLUDE_TINYLLAMA"; then
    echo "LLM model cache disabled by INCLUDE_TINYLLAMA=${INCLUDE_TINYLLAMA}"
    exit 0
fi

MODEL_KEY="${LLM_MODEL:-tinyllama}"
MODEL_FILE="${LLM_MODEL_FILE:-${MODEL_KEY}.gguf}"
TARGET_PATH="${MODEL_DIR}/${MODEL_FILE}"

if [[ -n "$LLM_MODEL_URL" ]]; then
    MODEL_URL="$LLM_MODEL_URL"
elif [[ -n "$MODEL_KEY" ]]; then
    MODEL_URL="$(resolve_model_url "$MODEL_KEY" || true)"
fi

if [[ -n "${MODEL_KEY}" && "${MODEL_KEY}" != "custom" && "${MODEL_KEY}" != "none" && "${MODEL_KEY}" != "disabled" ]]; then
    if [[ -z "$MODEL_URL" ]]; then
        echo "ERROR: No URL available for LLM model '${MODEL_KEY}'. Set llm_model_url or llm_model_file in CI."
        exit 1
    fi
fi

if is_truthy "$TINYLLAMA_FORCE_DOWNLOAD"; then
    rm -f "$TARGET_PATH"
fi

if [[ -f "$TARGET_PATH" ]]; then
    echo "Reusing cached model: $TARGET_PATH"
    exit 0
fi

MODEL_DIR="$MODEL_DIR" \
OUTPUT_NAME="$MODEL_FILE" \
FORCE=0 \
HF_URL="$MODEL_URL" \
LOCAL_SEARCH_DIRS="${MODEL_DIR}:/opt/local-models/models:/opt/local-models/llama-cpp-models:/opt/themis-models" \
LOCAL_MODEL_GLOB="*${MODEL_KEY}*.gguf" \
bash "${PWD}/scripts/download-tinyllama-hf.sh"

echo "Prepared model cache artifact at ${TARGET_PATH}"
exit 0
