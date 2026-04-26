# ThemisDB Model CLI Tool

## Overview

The `themis-model` CLI tool provides an Ollama-style interface for managing LLM models in ThemisDB. It allows you to download, list, and manage models from the command line with beautiful progress indicators.

## Installation

The tool is built automatically when LLM support is enabled:

```bash
cmake -DTHEMIS_ENABLE_LLM=ON ..
make themis-model
```

## Commands

### Pull (Download) a Model

Download a model from Ollama or HuggingFace:

```bash
themis-model pull phi3:mini-4k
```

**Example output:**
```
Pulling model: phi3:mini-4k
Destination: models/default

▐████████████████████████████████████████▌ 45.2% (1.04 GB / 2.30 GB) 12.5 MB/s
```

### List Downloaded Models

Show all downloaded models:

```bash
themis-model list
```

**Example output:**
```
Downloaded models:

▐ phi-3-mini-4k-instruct-q4.gguf
  Size: 2.3 GB
  Path: models/default/phi-3-mini-4k-instruct-q4.gguf

▐ mistral-7b-instruct-v0.2-q4.gguf
  Size: 4.1 GB
  Path: models/default/mistral-7b-instruct-v0.2-q4.gguf
```

### Show Model Information

Display details about a specific model:

```bash
themis-model show phi-3-mini-4k-instruct-q4.gguf
```

**Example output:**
```
Model Information:

Name: phi-3-mini-4k-instruct-q4.gguf
Path: models/default/phi-3-mini-4k-instruct-q4.gguf
Size: 2.3 GB
Format: GGUF
```

### Remove a Model

Delete a downloaded model:

```bash
themis-model rm phi-3-mini-4k-instruct-q4.gguf
```

**Example output:**
```
Remove model: models/default/phi-3-mini-4k-instruct-q4.gguf
Are you sure? (y/N): y
✓ Model removed successfully.
```

## Options

### Custom Model Directory

Specify a custom directory for storing models:

```bash
themis-model --model-dir /path/to/models pull phi3:mini-4k
themis-model --model-dir /path/to/models list
```

## Usage Examples

### Download Phi-3 Model

```bash
# Download Phi-3 Mini 4K
themis-model pull phi3:mini-4k

# Download with custom directory
themis-model --model-dir ./my-models pull phi3:mini-4k
```

### Manage Models

```bash
# List all models
themis-model list

# Show model details
themis-model show phi-3-mini-4k-instruct-q4.gguf

# Remove a model
themis-model rm old-model.gguf
```

### First-Time Setup

When setting up ThemisDB for the first time, download the default model:

```bash
# Create model directory
mkdir -p models/default

# Download Phi-3 (default model)
themis-model pull phi3:mini-4k

# Verify download
themis-model list
```

## Integration with ThemisDB Server

The CLI tool uses the same ModelDownloader as the ThemisDB server. Models downloaded with the CLI are immediately available to the server.

### Automatic vs Manual Download

**Server Auto-Download:**
The ThemisDB server can auto-download models on first start:

```yaml
# config.yaml
llm:
  enabled: true
  auto_download: true
  model_path: "models/default/phi-3-mini-4k-instruct-q4.gguf"
```

**CLI Pre-Download:**
Pre-download models before starting the server:

```bash
# Download model first
themis-model pull phi3:mini-4k

# Start server (no download needed)
themis-server --config config.yaml
```

## Features

### Progress Display

- **Progress bar** - Visual progress indicator (Ollama-style)
- **Percentage** - Current download percentage
- **Speed** - Real-time download speed in MB/s
- **Size info** - Downloaded vs total size
- **Colored output** - Beautiful terminal colors

### Error Handling

- **Disk full detection** - Stops download if disk is full
- **Network errors** - Clear error messages for connection issues
- **Model not found** - Helpful suggestions when model doesn't exist
- **Graceful cancellation** - Ctrl+C cancels download cleanly

### Smart Features

- **Resume capability** - Resume interrupted downloads
- **Cache support** - Skip re-downloading existing models
- **Ollama integration** - Download from Ollama API
- **GGUF format** - Compatible with llama.cpp models

## Troubleshooting

### Model Not Found

```
Error: Model not found: phi3:mini-4k
```

**Solution:** Check Ollama is running:
```bash
curl http://localhost:11434/api/tags
```

### Download Failed

```
✗ Error: CURL error: Couldn't connect to server
```

**Solution:** Ensure Ollama is installed and running:
```bash
# Start Ollama
ollama serve

# Or check if it's already running
ps aux | grep ollama
```

### Disk Space Issues

```
✗ Error: Write failed (disk full?)
```

**Solution:** Free up disk space or use a different directory:
```bash
# Check disk space
df -h

# Use different directory
themis-model --model-dir /larger/disk/models pull phi3:mini-4k
```

## Comparison with Ollama

The `themis-model` CLI is designed to be similar to Ollama:

| Command | Ollama | ThemisDB |
|---------|--------|----------|
| Download model | `ollama pull phi3` | `themis-model pull phi3:mini-4k` |
| List models | `ollama list` | `themis-model list` |
| Show model | `ollama show phi3` | `themis-model show phi-3-mini.gguf` |
| Remove model | `ollama rm phi3` | `themis-model rm phi-3-mini.gguf` |

## Advanced Usage

### Batch Download

Download multiple models:

```bash
#!/bin/bash
models=("phi3:mini-4k" "mistral:7b" "llama3:8b")

for model in "${models[@]}"; do
    echo "Downloading $model..."
    themis-model pull "$model"
done
```

### Check Before Download

Verify if model exists before downloading:

```bash
themis-model list | grep -q "phi-3-mini" || themis-model pull phi3:mini-4k
```

### Automated Setup Script

```bash
#!/bin/bash
# setup_models.sh - Download required models

set -e

MODEL_DIR="models/default"
mkdir -p "$MODEL_DIR"

echo "Setting up ThemisDB models..."

# Download default model
if ! themis-model list | grep -q "phi-3-mini"; then
    echo "Downloading Phi-3 Mini..."
    themis-model pull phi3:mini-4k
fi

# Download additional models
if ! themis-model list | grep -q "mistral-7b"; then
    echo "Downloading Mistral 7B..."
    themis-model pull mistral:7b
fi

echo "✓ All models ready!"
themis-model list
```

## See Also

- [PHI3_INTEGRATION.md](PHI3_INTEGRATION.md) - Complete Phi-3 integration guide
- [PHI3_QUICKSTART.md](../../llm_orchestration/PHI3_QUICKSTART.md) - Phi-3 quick start guide
- [ModelDownloader API](../../include/llm/model_downloader.h) - C++ API documentation

## Support

For issues with model downloads:
1. Check Ollama is running
2. Verify network connectivity
3. Ensure sufficient disk space
4. Check model name is correct

For help: support@themisdb.io
