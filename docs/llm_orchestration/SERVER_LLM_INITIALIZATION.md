# Server Initialization with EmbeddedLLM

## Overview

ThemisDB now supports automatic LLM initialization on server startup. This document explains how to configure and use the EmbeddedLLM feature.

## Configuration

### YAML Configuration

Add the following section to your `config.yaml`:

```yaml
llm:
  enabled: true              # Enable/disable LLM features
  required: false            # If true, server won't start if LLM init fails
  model_path: "models/tinyllama-1.1b-q4_0.gguf"
  model_id: "tinyllama"
  gpu_layers: 32             # 0 = CPU only, >0 = offload to GPU
  context_size: 4096         # Maximum context window
  threads: 4                 # CPU threads for inference
  enable_caching: true       # Enable response caching (future)
```

### JSON Configuration

```json
{
  "llm": {
    "enabled": true,
    "required": false,
    "model_path": "models/tinyllama-1.1b-q4_0.gguf",
    "model_id": "tinyllama",
    "gpu_layers": 32,
    "context_size": 4096,
    "threads": 4,
    "enable_caching": true
  }
}
```

## Model Files

### Supported Formats

- **GGUF files** (llama.cpp format)
- Quantized models (Q4_0, Q4_K_M, Q5_K_M, Q8_0, etc.)

### Recommended Models

1. **TinyLlama-1.1B** (~637MB Q4_0)
   - Good for testing and development
   - Fast on CPU
   - Download: `https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF`

2. **Llama-2-7B** (~3.8GB Q4_K_M)
   - Better quality responses
   - Requires more RAM/VRAM
   - Download: `https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF`

3. **Mistral-7B** (~4.1GB Q4_K_M)
   - Excellent quality
   - Good balance of speed/quality
   - Download: `https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF`

### Model Directory Structure

```
themisdb/
├── models/
│   ├── tinyllama-1.1b-q4_0.gguf
│   ├── llama-2-7b-q4_k_m.gguf
│   └── mistral-7b-q4_k_m.gguf
├── config.yaml
└── themis_server
```

## Starting the Server

### With Config File

```bash
./themis_server --config config.yaml
```

### With Command Line (uses default config paths)

```bash
# Server will search for config.yaml in:
# - ./config.yaml
# - ./config/config.yaml  
# - /etc/vccdb/config.yaml

./themis_server
```

### Environment-Specific Configs

```bash
# Development
./themis_server --config config/dev.yaml

# Production
./themis_server --config config/prod.yaml

# Testing (LLM disabled)
./themis_server --config config/test_no_llm.yaml
```

## Server Logs

### Successful Initialization

```
[INFO] Initializing EmbeddedLLM...
[INFO]   Model: models/tinyllama-1.1b-q4_0.gguf
[INFO]   GPU Layers: 32
[INFO]   Context Size: 4096
[INFO]   Threads: 4
[INFO] EmbeddedLLM initialized successfully: tinyllama (loaded)
```

### LLM Disabled

```
[INFO] EmbeddedLLM disabled in configuration
```

### LLM Not Compiled

```
[INFO] LLM support not compiled (THEMIS_ENABLE_LLM=OFF)
```

### Initialization Failure (Non-Critical)

```
[WARN] EmbeddedLLM initialization failed (non-critical): Model file not found
[WARN] LLM features will not be available
```

### Initialization Failure (Critical)

```
[ERROR] EmbeddedLLM initialization failed and is marked as required: Model file not found
```
Server exits with code 1.

## Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | bool | false | Enable LLM features |
| `required` | bool | false | Server fails to start if LLM init fails |
| `model_path` | string | "models/default.gguf" | Path to GGUF model file |
| `model_id` | string | "default" | Identifier for this model |
| `gpu_layers` | int | 0 | Layers to offload to GPU (0=CPU only) |
| `context_size` | int | 4096 | Maximum context window size |
| `threads` | int | 4 | CPU threads for inference |
| `enable_caching` | bool | true | Enable response caching (future) |

## GPU Configuration

### CUDA (NVIDIA)

```yaml
llm:
  enabled: true
  gpu_layers: 32  # Offload 32 layers to GPU
  model_path: "models/mistral-7b-q4_k_m.gguf"
```

### Metal (Apple Silicon)

```yaml
llm:
  enabled: true
  gpu_layers: -1  # Use all available GPU
  model_path: "models/mistral-7b-q4_k_m.gguf"
```

### CPU Only

```yaml
llm:
  enabled: true
  gpu_layers: 0   # CPU inference only
  threads: 8      # Use 8 CPU threads
  model_path: "models/tinyllama-1.1b-q4_0.gguf"
```

## Using the LLM

### From AQL

```sql
-- Simple generation
LET response = LLM INFER "What is ThemisDB?"
RETURN response

-- With options
LLM INFER "Summarize this document" 
OPTIONS {"temperature": 0.7, "max_tokens": 200}

-- Embeddings
LET vector = LLM EMBED "machine learning database"
RETURN vector
```

### From HTTP API

```bash
# Generate text
curl -X POST http://localhost:8765/api/llm/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "What is ThemisDB?", "max_tokens": 100}'

# Get embeddings
curl -X POST http://localhost:8765/api/llm/embed \
  -H "Content-Type: application/json" \
  -d '{"text": "semantic search query"}'

# Streaming (SSE)
curl -N http://localhost:8765/api/llm/stream \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Count from 1 to 5"}'
```

### From C++ Code

```cpp
// Anywhere in your code
std::string result = THEMIS_LLM_GENERATE("What is 2+2?");

std::vector<float> embedding = THEMIS_LLM_EMBED("search query");

std::vector<ChatMessage> messages = {
    {"system", "You are helpful"},
    {"user", "Hello!"}
};
std::string chat_response = THEMIS_LLM_CHAT(messages);
```

## Health Check

### Check LLM Status

```bash
curl http://localhost:8765/api/health
```

Response:
```json
{
  "status": "healthy",
  "llm": {
    "enabled": true,
    "ready": true,
    "model": "tinyllama (loaded)"
  }
}
```

## Troubleshooting

### Model File Not Found

**Error**: `Model file not found: models/tinyllama.gguf`

**Solution**: 
1. Download the model file
2. Place it in the `models/` directory
3. Update `model_path` in config

### Out of Memory

**Error**: `Failed to allocate memory for model`

**Solutions**:
- Use a smaller quantized model (Q4_0 instead of Q8_0)
- Reduce `gpu_layers` or set to 0 for CPU inference
- Reduce `context_size`

### Slow Performance

**Solutions**:
- Increase `gpu_layers` (use GPU)
- Increase `threads` for CPU inference
- Use a smaller model
- Use a more quantized model (Q4_K_M)

### LLM Features Not Available

**Check**:
1. Is LLM enabled in config? (`llm.enabled: true`)
2. Is ThemisDB compiled with LLM support? (Check logs for "THEMIS_ENABLE_LLM=OFF")
3. Did LLM initialization succeed? (Check logs)

## Performance Tips

1. **Use GPU**: Set `gpu_layers > 0` for 10-100x speedup
2. **Quantization**: Q4_K_M offers best quality/speed balance
3. **Context Size**: Smaller context = faster inference
4. **Thread Count**: Set to CPU core count for best CPU performance
5. **Model Size**: Smaller models = faster inference, lower quality

## Example Configurations

### Development (Fast, CPU)

```yaml
llm:
  enabled: true
  model_path: "models/tinyllama-1.1b-q4_0.gguf"
  gpu_layers: 0
  context_size: 2048
  threads: 4
```

### Production (Quality, GPU)

```yaml
llm:
  enabled: true
  required: true
  model_path: "models/mistral-7b-q4_k_m.gguf"
  gpu_layers: 32
  context_size: 4096
  threads: 8
```

### Testing (Disabled)

```yaml
llm:
  enabled: false
```

## Next Steps

1. Start server with LLM config
2. Verify initialization in logs
3. Test with simple query
4. Monitor performance
5. Adjust configuration as needed

See also:
- `INTEGRATION_ISSUES.md` - Integration tasks
- `CODE_REVIEW.md` - Code quality review
- `examples/embedded_llm_examples.cpp` - Usage examples
