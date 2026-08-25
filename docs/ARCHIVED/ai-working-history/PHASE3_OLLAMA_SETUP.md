# Phase 3 Ollama Setup Guide

## Quick Start (5 minutes)

### 1. Install Ollama

**Windows:**
```powershell
# Option A: Using winget
winget install ollama

# Option B: Manual download
# Visit: https://ollama.com/download/windows
# Download and run installer
```

**Verify Installation:**
```powershell
ollama --version
# Output: ollama version X.X.X
```

### 2. Start Ollama Service

```powershell
# Start in background
ollama serve

# Or as service (Windows)
# Ollama is installed as Windows Service by default
# Service name: ollama
# To check: Services.msc -> look for "ollama"
```

**Verify Running:**
```powershell
curl http://localhost:11434/api/tags

# Expected: JSON with empty models list
# {"models":[]}
```

### 3. Download Model

```bash
# In a new terminal (while ollama serve is running):
ollama pull deepseek-coder-v2:16b

# Time: ~8-10 minutes (10 GB download)
# Space: 10 GB required

# Monitor progress:
ollama list
# Output:
# NAME                        ID              SIZE    MODIFIED
# deepseek-coder-v2:16b       ...             10GB    2 seconds ago
```

### 4. Verify Setup

```bash
python tools/auto_phase3_codegen.py --check-ollama

# Expected output:
# [OK] Ollama found with model: deepseek-coder-v2:16b
```

---

## Model Selection

| Model | Size | VRAM | Speed | Quality | Recommended |
|-------|------|------|-------|---------|-------------|
| qwen2.5-coder:7b | 5.9 GB | 5 GB | Fast | Good | Fallback |
| deepseek-coder-v2:16b | 10 GB | 10 GB | Medium | **Excellent** | **PRIMARY** |
| codellama:34b | 20 GB | 16 GB | Slow | Excellent | Complex |

**For this project**: `deepseek-coder-v2:16b` is recommended.

---

## Available Models

```bash
# List all available models to download
ollama list --remote

# Show only code-related models
ollama list --remote | grep -i code

# Model catalog: https://ollama.com/library
```

---

## Troubleshooting

### Issue: "Connection refused"
```
Error: failed to connect to localhost:11434
```
**Solution:**
```bash
# Check if ollama is running
tasklist | findstr ollama

# If not running, start it:
ollama serve
```

### Issue: "Model not found"
```
Error: model 'deepseek-coder-v2:16b' not found
```
**Solution:**
```bash
# Download the model first
ollama pull deepseek-coder-v2:16b

# Wait 8-10 minutes
```

### Issue: "Out of memory"
```
Error: failed to allocate ... GPU memory
```
**Solution:**
```bash
# Use smaller model
ollama pull qwen2.5-coder:7b

# Or limit batch size
python tools/auto_phase3_codegen.py --module llm --max-tasks 5
```

### Issue: "Slow response"
```
Waiting for model response... (>30 seconds)
```
**Solution:**
```bash
# Model is too large for GPU memory
# Options:
# 1. Use smaller model (qwen2.5-coder:7b)
# 2. Close other applications
# 3. Increase GPU memory (if possible)
```

---

## Performance Tuning

### Increase Batch Size (if you have VRAM)
```bash
# More concurrent generations
python tools/auto_phase3_codegen.py --module llm --max-tasks 20
```

### Reduce Batch Size (if OOM)
```bash
# Fewer concurrent tasks
python tools/auto_phase3_codegen.py --module llm --max-tasks 3
```

### Monitor GPU Usage
```powershell
# Windows: GPU-Z or nvidia-smi
nvidia-smi

# Watch in real-time:
nvidia-smi dmon
```

---

## Integration with GitHub Copilot

Phase 3 uses **local Ollama** for draft code generation, then **Copilot** for refinement:

### Workflow

1. **Generate** (Ollama): Create draft code
   ```bash
   python tools/auto_phase3_codegen.py --module llm
   ```

2. **Review** (VS Code + Copilot)
   - Open generated code in editor
   - Copilot Chat: @copilot /review this code
   - Make refinements

3. **Validate** (Local)
   ```bash
   cmake --build --target llm
   ctest --filter LLM
   ```

4. **Commit** (Git)
   ```bash
   git add .
   git commit -m "Phase 3: {module} implementation"
   ```

---

## API Endpoints

If you want to use Ollama API directly:

```python
import requests

# Generate code
response = requests.post(
    'http://localhost:11434/api/generate',
    json={
        'model': 'deepseek-coder-v2:16b',
        'prompt': 'Implement a thread-safe queue in C++',
        'stream': False,
        'temperature': 0.3
    }
)

code = response.json()['response']
print(code)

# List models
response = requests.get('http://localhost:11434/api/tags')
models = response.json()['models']
for model in models:
    print(f"{model['name']} - {model['size']} bytes")
```

---

## Next Steps

1. **Complete setup**: Run steps 1-4 above
2. **Start Phase 3**: `python tools/auto_phase3_codegen.py --module llm --max-tasks 5`
3. **Review output**: Check `ai_working/phase3_llm_codegen.json`
4. **Refine with Copilot**: Open suggestions in VS Code
5. **Build & Test**: Validate generated code

---

## Support

For issues:
- Ollama Docs: https://ollama.com/docs
- Model Info: https://ollama.com/library
- Issues: https://github.com/ollama/ollama/issues

---

**Estimated Time**: 15-20 minutes total
**Disk Space**: ~15 GB (Ollama + model)
**Memory**: 10 GB VRAM recommended

Ready to run Phase 3! 🚀
