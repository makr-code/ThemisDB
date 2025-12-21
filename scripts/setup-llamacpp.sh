#!/bin/bash
# Setup llama.cpp integration for ThemisDB v1.5.0
# This script performs a local, non-committed clone of llama.cpp into the repo root.

set -e

echo "==================================================================="
echo "ThemisDB v1.3.0 - llama.cpp Integration Setup"
echo "==================================================================="
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
THEMIS_ROOT="$(dirname "$SCRIPT_DIR")"

echo "ThemisDB Root: $THEMIS_ROOT"
cd "$THEMIS_ROOT"

# Step 1: Create external directory if it doesn't exist
echo ""
echo "[1/3] Preparing local llama.cpp directory..."
mkdir -p "${THEMIS_ROOT}"

# Step 2: Add llama.cpp as submodule
echo ""
echo "[2/3] Setting up local llama.cpp (no git submodule, ignored by Git/Docker)"

if [ -d "llama.cpp/.git" ] || [ -d "llama.cpp" ]; then
    echo "  ✅ llama.cpp already present at ./llama.cpp"
    echo "  Updating (git fetch) if repository detected..."
    if [ -d "llama.cpp/.git" ]; then
        (cd llama.cpp && git fetch --all --tags || true)
    fi
else
    echo "  Cloning llama.cpp into ./llama.cpp"
    git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
    echo "  ✅ llama.cpp cloned locally"
fi

# Step 3: Checkout stable version (optional)
echo ""
echo "[3/3] Checking llama.cpp version (if git repo present)..."
if [ -d "llama.cpp/.git" ]; then
    cd llama.cpp
    LLAMA_VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "master")
    echo "  Current version: $LLAMA_VERSION"
    echo "  (You can checkout a specific tag if needed)"
    cd "$THEMIS_ROOT"
else
    echo "  (llama.cpp is present without git metadata)"
fi

# Step 4: Create .gitmodules if it doesn't exist
echo ""
echo "Note: ./llama.cpp is intentionally excluded via .gitignore and .dockerignore."

echo ""
echo "==================================================================="
echo "Setup Complete!"
echo "==================================================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Build with LLM support (root llama.cpp preferred):"
echo "   cmake -B build -DTHEMIS_ENABLE_LLM=ON"
echo "   cmake --build build"
echo ""
echo "2. Build with CUDA support:"
echo "   cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_CUDA=ON"
echo "   cmake --build build"
echo ""
echo "3. Download a model (example):"
echo "   mkdir -p models"
echo "   # Download from HuggingFace:"
echo "   # https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF"
echo ""
echo "4. Test the integration:"
echo "   ./build/themis_server --config config/llm_config.example.yaml"
echo ""
echo "For more information, see:"
echo "  - docs/llm/LLAMA_CPP_INTEGRATION.md"
echo "  - docs/llm/LLM_PLUGIN_DEVELOPMENT_GUIDE.md"
echo "  - docs/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md"
echo ""
