#!/bin/bash
# Setup llama.cpp for ThemisDB LLM support
# This script clones llama.cpp to the ThemisDB root directory
# The llama.cpp directory is ignored by .gitignore and .dockerignore

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THEMIS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LLAMA_DIR="$THEMIS_ROOT/llama.cpp"

echo "=== ThemisDB llama.cpp Setup ==="
echo "ThemisDB root: $THEMIS_ROOT"
echo "llama.cpp target: $LLAMA_DIR"
echo ""

# Check if llama.cpp already exists
if [ -d "$LLAMA_DIR" ]; then
    echo "✓ llama.cpp directory exists at: $LLAMA_DIR"
    
    # Check if it's a git repository
    if [ -d "$LLAMA_DIR/.git" ]; then
        echo "✓ llama.cpp is a git repository"
        
        # Show current version
        cd "$LLAMA_DIR"
        CURRENT_COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
        CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
        echo "  Current: $CURRENT_BRANCH @ $CURRENT_COMMIT"
        
        # Ask if user wants to update
        read -p "Update llama.cpp to latest? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo "Updating llama.cpp..."
            git fetch origin
            git pull origin master
            echo "✓ llama.cpp updated"
        else
            echo "Keeping current version"
        fi
        cd "$THEMIS_ROOT"
    else
        echo "⚠ llama.cpp directory exists but is not a git repository"
        echo "  Please remove it manually and re-run this script:"
        echo "  rm -rf $LLAMA_DIR"
        exit 1
    fi
else
    echo "Cloning llama.cpp..."
    git clone https://github.com/ggerganov/llama.cpp.git "$LLAMA_DIR"
    
    cd "$LLAMA_DIR"
    CURRENT_COMMIT=$(git rev-parse --short HEAD)
    echo "✓ llama.cpp cloned successfully @ $CURRENT_COMMIT"
    cd "$THEMIS_ROOT"
fi

# Verify CMakeLists.txt exists
if [ ! -f "$LLAMA_DIR/CMakeLists.txt" ]; then
    echo "✗ Error: llama.cpp/CMakeLists.txt not found"
    echo "  The cloned repository may be incomplete or corrupted"
    exit 1
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "llama.cpp is ready at: $LLAMA_DIR"
echo ""
echo "Next steps:"
echo "  1. Build ThemisDB with LLM support:"
echo "     cmake -B build -DTHEMIS_ENABLE_LLM=ON"
echo ""
echo "  2. For CUDA support (NVIDIA GPU):"
echo "     cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_CUDA=ON"
echo ""
echo "  3. Build:"
echo "     cmake --build build -j\$(nproc)"
echo ""
