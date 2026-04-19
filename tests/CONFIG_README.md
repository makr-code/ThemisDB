> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Test Configuration System

## Overview

ThemisDB tests use a **centralized YAML configuration system** to manage all test resources (models, LoRA adapters, GPU settings). This replaces the previous approach of hardcoded paths and environment variables.

## Files

| File | Purpose |
|------|---------|
| [test_config.yaml](test_config.yaml) | Central configuration in YAML format |
| [test_config.h](test_config.h) | C++ configuration loader (singleton pattern) |
| [test_helpers_llm.h](test_helpers_llm.h) | Helper functions for tests |
| [test_yaml_config_integration.cpp](test_yaml_config_integration.cpp) | Validation tests |
| [TEST_CONFIG_GUIDE.md](TEST_CONFIG_GUIDE.md) | Comprehensive guide (320+ lines) |

## Quick Start

### 1. Verify Configuration
```bash
# Check config file exists
cat tests/test_config.yaml
```

### 2. Run Tests
```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
ctest --preset linux-ninja-release -R integration
```

> <!-- TODO: verify against current source – legacy executable path below kept for reference -->
```bash
# Legacy (kept for historical reference):
cd build-msvc-ninja-release
.\cmake\tests\themis_tests.exe --gtest_filter='*LLM*'
```

Tests automatically load `test_config.yaml` and skip tests if resources unavailable.

## Configuration Structure

### Minimal Example
```yaml
llm:
  enabled: true
  models_dir: "./models"
  models:
    test_model: "test_model.gguf"

gpu:
  enabled: false
  type: "none"
```

### Full Example (see test_config.yaml)
```yaml
llm:
  enabled: true
  models_dir: "C:/path/to/models"
  ollama:
    enabled: true
    models_dir: "C:/Users/user/.ollama/models/blobs"
    models:
      llama3.2:
        hash: "dde5aa3..."
        size_gb: 1.88

lora:
  enabled: true
  adapters_dir: "./lora_adapters"
  test_adapters:
    legal-qa: "legal-qa.bin"

gpu:
  enabled: false
  type: "none"

test:
  skip_slow_tests: false
  timeout_seconds: 300
```

## Using in Tests

### Get Model Path
```cpp
#include "test_helpers_llm.h"

// Skip test if model not configured
std::string model_path = getRealModelPathOrSkip();
```

### Get LoRA Adapter
```cpp
std::string adapter_path = getLoRAAdapterPathOrSkip("legal-qa");
```

### Check GPU Availability
```cpp
if (hasGPU()) {
    // GPU code
} else {
    GTEST_SKIP() << "GPU not available";
}

// Or skip test if GPU required
requireGPUOrSkip();
```

### Direct Config Access
```cpp
auto& config = TestConfig::instance();
if (config.gpu().isCUDA()) {
    // CUDA-specific code
}
```

## Environment-Specific Configurations

### Development (No GPU, Skip Slow Tests)
```yaml
llm:
  enabled: true
  models_dir: "C:/local/models"

gpu:
  enabled: false

test:
  skip_slow_tests: true
```

### CI/CD (Fast, Small Models)
```yaml
llm:
  enabled: true
  models_dir: "/ci/models"
  models:
    test_model: "tinyllama-1.1b.gguf"

gpu:
  enabled: false

test:
  skip_slow_tests: true
  timeout_seconds: 60
```

### GPU Testing (Full Models)
```yaml
llm:
  enabled: true
  models_dir: "/gpu/models"

gpu:
  enabled: true
  type: "cuda"
  cuda:
    devices: [0, 1]

test:
  skip_slow_tests: false
```

## Override Configuration

Set `THEMIS_TEST_CONFIG` to use custom config:

```bash
# Windows PowerShell
$env:THEMIS_TEST_CONFIG = "C:\path\to\my_config.yaml"

# Linux/macOS
export THEMIS_TEST_CONFIG=/path/to/my_config.yaml

# Run tests
.\themis_tests.exe
```

## Configuration File Location

Searched in order:
1. `$THEMIS_TEST_CONFIG` environment variable
2. `../tests/test_config.yaml` (from build/cmake/tests/)
3. `../../tests/test_config.yaml` (from build/cmake/)
4. `./tests/test_config.yaml` (from root)
5. `./test_config.yaml` (current directory)

## Benefits Over Environment Variables

| Aspect | Env Variables | YAML Config |
|--------|---------------|-------------|
| Centralized | ❌ Scattered | ✅ Single file |
| Version Control | ❌ Not tracked | ✅ In git |
| Per-Environment | ⚠️ Manual | ✅ Easy override |
| Complex Config | ❌ Limited | ✅ Nested structures |
| Documentation | ❌ External | ✅ Self-documenting |
| Type Safety | ❌ Strings | ✅ C++ structs |

## Validation

Run validation tests:
```bash
ctest --preset linux-ninja-release -R YAMLConfigIntegrationTest
```

> <!-- TODO: verify against current source – legacy filter path kept for reference -->
```bash
# Legacy (kept for historical reference):
.\cmake\tests\themis_tests.exe --gtest_filter='YAMLConfigIntegrationTest.*'
```

These tests verify:
- Configuration loads successfully
- All sections present and valid
- Model paths resolved correctly
- Ollama integration working
- GPU settings properly configured

## Troubleshooting

### "Model not found" Skip Message

**Cause:** Model file path not configured

**Solution:** Add to `test_config.yaml`:
```yaml
llm:
  models_dir: "C:/path/to/models"
  models:
    model_name: "model_file.gguf"
```

### "LLM tests disabled"

**Cause:** LLM disabled in configuration

**Solution:** Enable in `test_config.yaml`:
```yaml
llm:
  enabled: true
```

### "GPU not available"

**Cause:** GPU not configured or not available

**Solution (if no GPU):** Keep GPU disabled:
```yaml
gpu:
  enabled: false
```

**Solution (if GPU present):** Configure GPU type:
```yaml
gpu:
  enabled: true
  type: "cuda"  # or "hip"
```

## See Also

- [TEST_CONFIG_GUIDE.md](TEST_CONFIG_GUIDE.md) - Complete guide
- [test_config.yaml](test_config.yaml) - Configuration file
- [test_helpers_llm.h](test_helpers_llm.h) - Helper functions
- [YAML_CONFIG_IMPLEMENTATION_SUMMARY.md](YAML_CONFIG_IMPLEMENTATION_SUMMARY.md) - Architecture overview
