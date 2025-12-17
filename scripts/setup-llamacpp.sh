#!/bin/bash
# Setup llama.cpp integration for ThemisDB v1.5.0
# This script adds llama.cpp as a git submodule and prepares the build system

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
echo "[1/4] Creating external directory..."
mkdir -p external

# Step 2: Add llama.cpp as submodule
echo ""
echo "[2/4] Adding llama.cpp as git submodule..."

if [ -d "external/llama.cpp" ]; then
    echo "  ⚠️  llama.cpp submodule already exists at external/llama.cpp"
    echo "  Updating submodule..."
    git submodule update --init --recursive external/llama.cpp
else
    echo "  Adding llama.cpp from GitHub..."
    git submodule add https://github.com/ggerganov/llama.cpp.git external/llama.cpp
    git submodule update --init --recursive
    echo "  ✅ llama.cpp submodule added"
fi

# Step 3: Checkout stable version (optional)
echo ""
echo "[3/4] Checking llama.cpp version..."
cd external/llama.cpp
LLAMA_VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "master")
echo "  Current version: $LLAMA_VERSION"
echo "  (You can checkout a specific tag if needed)"
cd "$THEMIS_ROOT"

# Step 4: Create .gitmodules if it doesn't exist
echo ""
echo "[4/4] Verifying git configuration..."
if [ -f ".gitmodules" ]; then
    echo "  ✅ .gitmodules exists"
    cat .gitmodules | grep "llama.cpp" || echo "  ⚠️  llama.cpp not in .gitmodules (will be added by git)"
else
    echo "  ⚠️  .gitmodules will be created by git"
fi

echo ""
echo "==================================================================="
echo "Setup Complete!"
echo "==================================================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Build with LLM support:"
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
