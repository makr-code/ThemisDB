> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Test Configuration Guide

## Overview

ThemisDB tests use a centralized YAML configuration file to manage test resources (models, LoRA adapters, GPU settings). This replaces the previous approach of hardcoded paths and environment variables.

## Configuration File Location

**Default:** `tests/test_config.yaml`

**Override:** Set `THEMIS_TEST_CONFIG` environment variable:
```bash
# Windows PowerShell
$env:THEMIS_TEST_CONFIG = "C:\path\to\custom_test_config.yaml"

# Linux/macOS
export THEMIS_TEST_CONFIG=/path/to/custom_test_config.yaml
```

## Quick Start

### 1. Verify Configuration File Exists

```bash
# Check if test_config.yaml exists
ls tests/test_config.yaml
```

### 2. Configure Model Paths

Edit `tests/test_config.yaml`:

```yaml
llm:
  enabled: true
  models_dir: "C:/VCC/themis/build-msvc-ninja-release/models"
  
  models:
    test_model: "test_model.gguf"
    llama-2-7b: "llama-2-7b.gguf"
    mistral-7b-v0.1: "mistral-7b-v0.1.gguf"
```

### 3. Run Tests

```bash
cd build-msvc-ninja-release
.\cmake\tests\themis_tests.exe --gtest_filter='*LLM*'
```
<!-- Legacy: prefer cmake --preset -->

Tests will automatically load configuration and skip tests if resources unavailable.

## Configuration Structure

### LLM Models

```yaml
llm:
  enabled: true                      # Enable/disable LLM tests
  models_dir: "./models"             # Base directory for local models
  default_model: "test_model"        # Default model for tests
  
  # Local models (relative to models_dir)
  models:
    test_model: "test_model.gguf"
    llama-2-7b: "llama-2-7b.gguf"
  
  # Ollama integration (optional)
  ollama:
    enabled: true
    models_dir: "C:/Users/mkrueger/.ollama/models/blobs"
    models:
      llama3.2:
        hash: "dde5aa3fc5ffc17176b5e8bdc82f587b24b2678c6c66101bf7da77af9f7ccdff"
        size_gb: 1.88
```

**Model Resolution Order:**
1. Check if model name is an alias in `llm.models`
2. If Ollama enabled, check for `sha256-<hash>` in Ollama blobs directory
3. Check `models_dir/<filename>`
4. If not found, test is skipped (not failed)

### LoRA Adapters

```yaml
lora:
  enabled: true
  adapters_dir: "./lora_adapters"
  
  test_adapters:
    legal-qa: "legal-qa.bin"
    medical: "medical.bin"
    finance: "finance.bin"
```

**Usage in tests:**
```cpp
std::string adapter_path = getLoRAAdapterPathOrSkip("legal-qa");
// adapter_path = "./lora_adapters/legal-qa.bin"
```

### GPU Configuration

```yaml
gpu:
  enabled: false         # Set to true if GPU available
  type: "none"           # "cuda", "hip", or "none"
  
  cuda:
    devices: [0]         # CUDA device IDs
  
  vram:
    total_gb: 24
```

**Usage in tests:**
```cpp
requireGPUOrSkip();  // Skips test if GPU not configured
// or
if (hasGPU()) {
    // GPU-specific code
}
```

### Test Execution Settings

```yaml
test:
  skip_slow_tests: false       # Skip tests with long inference times
  timeout_seconds: 300         # Max test duration
  
  data:
    fixtures_dir: "./test_fixtures"
    temp_dir: "./test_temp"
```

## Helper Functions

### Model Path Resolution

```cpp
#include "test_helpers_llm.h"

// Get model path (skips test if not found)
std::string model = getRealModelPathOrSkip();
std::string model = getRealModelPathOrSkip("llama-2-7b");

// Get model directory
std::string dir = getRealModelDirOrSkip();

// Check without skipping
if (hasRealModels()) {
    // Tests can run
}
```

### LoRA Adapters

```cpp
// Get adapter path (skips test if not found)
std::string adapter = getLoRAAdapterPathOrSkip("legal-qa");
```

### GPU Checks

```cpp
// Require GPU (skip test if unavailable)
requireGPUOrSkip();

// Check without skipping
if (hasGPU()) {
    // GPU-specific code
}
```

### Slow Test Filtering

```cpp
// Skip test if skip_slow_tests enabled
shouldSkipSlowTest();
```

## Environment-Specific Configurations

### Development Machine (No GPU)

```yaml
llm:
  enabled: true
  models_dir: "C:/VCC/themis/build-msvc-ninja-release/models"

gpu:
  enabled: false
  type: "none"

lora:
  enabled: false  # Skip LoRA tests during development

test:
  skip_slow_tests: true  # Skip long inference tests
```

### CI/CD Pipeline

```yaml
llm:
  enabled: true
  models_dir: "/ci/models"
  
  models:
    test_model: "tinyllama-1.1b-q4.gguf"  # Small model for fast tests

gpu:
  enabled: false

test:
  timeout_seconds: 60
  skip_slow_tests: true
```

### GPU Test Server

```yaml
llm:
  enabled: true
  models_dir: "/opt/themis/models"

gpu:
  enabled: true
  type: "cuda"
  cuda:
    devices: [0, 1]  # Multi-GPU testing

lora:
  enabled: true
  adapters_dir: "/opt/themis/lora"

test:
  skip_slow_tests: false  # Run full inference tests
```

## Migration from Environment Variables

### Old Approach (Environment Variables)

```bash
# PowerShell
$env:THEMIS_LLM_MODELS_PATH = "C:\Users\mkrueger\.ollama\models\blobs"
$env:THEMIS_HAS_GPU = "0"

# Run tests
.\themis_tests.exe
```
<!-- Legacy: prefer cmake --preset -->

### New Approach (YAML Config)

**Step 1:** Configure `test_config.yaml`
```yaml
llm:
  ollama:
    enabled: true
    models_dir: "C:/Users/mkrueger/.ollama/models/blobs"

gpu:
  enabled: false
```

**Step 2:** Run tests (no environment variables needed)
```bash
.\themis_tests.exe
```

### Benefits of YAML Config

| Feature | Environment Variables | YAML Config |
|---------|----------------------|-------------|
| Centralized | ❌ Scattered | ✅ Single file |
| Version Control | ❌ Not tracked | ✅ In git |
| Per-Dev Customization | ⚠️ Manual setup | ✅ Override with `THEMIS_TEST_CONFIG` |
| Complex Config | ❌ Limited | ✅ Nested structures |
| CI/CD Integration | ⚠️ Multiple env vars | ✅ Single file override |
| Documentation | ❌ External docs | ✅ Self-documenting YAML |

## Troubleshooting

### Problem: "Model not found: test_model"

**Solution:** Configure model path in `test_config.yaml`:
```yaml
llm:
  models_dir: "C:/VCC/themis/build-msvc-ninja-release/models"
  models:
    test_model: "test_model.gguf"
```

Verify file exists:
```bash
ls C:\VCC\themis\build-msvc-ninja-release\models\test_model.gguf
```

### Problem: "LLM tests disabled in test_config.yaml"

**Solution:** Enable LLM tests:
```yaml
llm:
  enabled: true
```

### Problem: "LoRA adapter not found: legal-qa"

**Solution:** Either:
1. Add adapter file and configure path:
```yaml
lora:
  enabled: true
  test_adapters:
    legal-qa: "legal-qa.bin"
```

2. Or disable LoRA tests:
```yaml
lora:
  enabled: false
```

### Problem: Config file not found

**Default paths checked (in order):**
1. `../tests/test_config.yaml`
2. `../../tests/test_config.yaml`
3. `./tests/test_config.yaml`
4. `./test_config.yaml`

**Solution:** Either:
- Move config to expected location
- Set `THEMIS_TEST_CONFIG` environment variable

### Problem: Tests still use old environment variables

**Cause:** Test file not updated to use helper functions.

**Solution:** Update test to use YAML-based helpers:
```cpp
// Old code
const char* models_path = std::getenv("THEMIS_LLM_MODELS_PATH");

// New code
#include "test_helpers_llm.h"
std::string model = getRealModelPathOrSkip();
```

## Advanced Usage

### Per-Test Configuration Override

```cpp
TEST_F(MyTest, CustomModelTest) {
    auto& config = TestConfig::instance();
    
    // Temporarily override configuration
    std::string original_model = config.llm().default_model;
    
    // Use specific model for this test
    std::string model = getRealModelPathOrSkip("llama3.2");
    
    // Test code...
}
```

### Conditional Test Execution

```cpp
TEST_F(MyTest, OptionalGPUTest) {
    auto& config = TestConfig::instance();
    
    if (config.gpu().enabled && config.gpu().isCUDA()) {
        // CUDA-specific test
    } else if (config.gpu().enabled && config.gpu().isHIP()) {
        // HIP-specific test
    } else {
        GTEST_SKIP() << "GPU not available";
    }
}
```

### Configuration Validation

```cpp
#include "test_config.h"

TEST(ConfigTest, ValidateTestConfiguration) {
    auto& config = TestConfig::instance();
    
    EXPECT_TRUE(config.llm().enabled || !config.lora().enabled)
        << "LoRA tests require LLM to be enabled";
    
    if (config.llm().enabled) {
        std::string model_path = config.llm().getModelPath(config.llm().default_model);
        EXPECT_FALSE(model_path.empty()) << "Default model not found";
    }
}
```

## See Also

- [test_config.yaml](test_config.yaml) - Main configuration file
- [test_config.h](test_config.h) - C++ config loader
- [test_helpers_llm.h](test_helpers_llm.h) - Helper functions
- [SETUP.md](../SETUP.md) - General ThemisDB setup guide
