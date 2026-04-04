#!/bin/bash
# Example script demonstrating themis-model CLI usage
# This script downloads required models for ThemisDB

set -e

echo "==================================="
echo "ThemisDB Model Setup Example"
echo "==================================="
echo ""

# Configuration
MODEL_DIR="models/default"
REQUIRED_MODELS=("phi3:mini-4k")

# Create model directory
echo "1. Creating model directory: $MODEL_DIR"
mkdir -p "$MODEL_DIR"
echo "   ✓ Directory created"
echo ""

# Check if themis-model is available
echo "2. Checking themis-model CLI availability..."
if ! command -v themis-model &> /dev/null; then
    echo "   ✗ themis-model not found in PATH"
    echo "   Please build it first:"
    echo "     cd build"
    echo "     cmake -DTHEMIS_ENABLE_LLM=ON .."
    echo "     make themis-model"
    exit 1
fi
echo "   ✓ themis-model found"
echo ""

# List currently installed models
echo "3. Current models:"
themis-model --model-dir "$MODEL_DIR" list
echo ""

# Download required models
echo "4. Downloading required models..."
for model in "${REQUIRED_MODELS[@]}"; do
    model_name=$(echo "$model" | tr ':' '-')
    
    # Check if already downloaded
    if themis-model --model-dir "$MODEL_DIR" list | grep -q "$model_name"; then
        echo "   ⊳ $model already downloaded, skipping"
    else
        echo "   ⊳ Downloading $model..."
        themis-model --model-dir "$MODEL_DIR" pull "$model"
    fi
done
echo ""

# Show final status
echo "5. All models ready!"
themis-model --model-dir "$MODEL_DIR" list
echo ""

echo "==================================="
echo "Setup complete!"
echo ""
echo "You can now:"
echo "1. Start ThemisDB server with LLM enabled"
echo "2. Use the downloaded models for inference"
echo "3. Train LoRA adapters on these models"
echo "==================================="
