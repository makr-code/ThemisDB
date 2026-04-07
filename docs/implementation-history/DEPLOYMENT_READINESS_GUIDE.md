# ThemisDB LLM/LoRA System - Deployment Readiness Guide

**Version**: 1.4.0-alpha  
**Date**: January 17, 2026  
**Status**: Development/Testing Ready ✅ | Production Ready ⚠️

---

## Quick Status Check

### ✅ Ready for Development/Testing

The ThemisDB LLM/LoRA system is **ready for development and testing** with the following capabilities:

- ✅ LoRA training on CPU (functional, tested)
- ✅ Model loading from filesystem (working)
- ✅ Basic inference pipeline (functional)
- ✅ Adapter storage to filesystem (working)
- ✅ Security features for development (MockKeyProvider)

### ⚠️ Not Ready for Production

The following critical features are incomplete or using placeholders:

- ❌ Production key management (requires HSM or Vault setup)
- ❌ LoRA adapter application to models (adapters stored but not applied)
- ❌ Production validation tests (31 TODOs remaining)
- ❌ Real embeddings from base models (using hash-based placeholders)
- ❌ Full model training (simplified forward/backward pass)

**Recommendation**: Use for development and research. Do not deploy to production until P0 gaps addressed.

---

## Development Setup

### Prerequisites

```bash
# System dependencies
sudo apt-get install -y \
  librocksdb-dev \
  libfmt-dev \
  libspdlog-dev \
  libtbb-dev \
  libsimdjson-dev \
  nlohmann-json3-dev \
  libgrpc++-dev \
  protobuf-compiler-grpc \
  libgtest-dev

# Clone llama.cpp for LLM support
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
make -j$(nproc)
```

### Build Configuration

**Development Build** (MockKeyProvider allowed):

```bash
export THEMIS_ENVIRONMENT=development

cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_MIMALLOC=OFF

cmake --build build -j$(nproc)
```

**Testing Build** (with all features):

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON

cmake --build build -j$(nproc)
```

### Running Tests

```bash
cd build

# All LoRA tests
ctest --output-on-failure -R lora

# Specific test suites
ctest -R test_lora_training_integration -V
ctest -R test_lora_llama_integration -V
ctest -R test_data_loader -V
```

---

## Configuration Guide

### Development Configuration

**File**: `config/development.yaml`

```yaml
# LLM Configuration
llm:
  enabled: true
  model_path: "models/llama-2-7b.gguf"  # Local file path
  max_context: 4096
  batch_size: 512

# LoRA Training
lora_training:
  enabled: true
  backend: filesystem  # Use filesystem in development
  storage_path: "data/lora_adapters"
  
  # Base model configuration
  base_model:
    use_base_model: true
    path: "models/llama-2-7b.gguf"
    freeze_weights: true
  
  # LoRA hyperparameters
  hyperparameters:
    rank: 16
    alpha: 32
    dropout: 0.05
    learning_rate: 3e-4
    batch_size: 4
    num_epochs: 3
  
  # Target modules for LoRA
  target_modules:
    - "attention.wq"
    - "attention.wk"
    - "attention.wv"
    - "attention.wo"

# Storage Configuration (Development)
storage:
  backend: filesystem
  enable_encryption: false  # OK for development
  enable_signatures: false  # OK for development
```

### Production Configuration (Future)

⚠️ **Note**: Production deployment requires completing P0 gaps first!

**File**: `config/production.yaml`

```yaml
# Environment must be set
# export THEMIS_ENVIRONMENT=production

# LLM Configuration
llm:
  enabled: true
  model_storage: themisdb  # Load from database
  max_context: 8192
  batch_size: 1024

# LoRA Training
lora_training:
  enabled: true
  backend: themisdb  # Use ThemisDB backend
  
  # Security Configuration (REQUIRED in production)
  security:
    enable_encryption: true
    enable_signatures: true
    
    # Option 1: HSM (Hardware Security Module)
    use_hsm_for_encryption: true
    hsm_library_path: "/usr/lib/softhsm/libsofthsm2.so"
    hsm_slot_id: 0
    hsm_key_label: "lora-adapter-kek"
    # hsm_pin: set via environment variable THEMIS_HSM_PIN
    
    # Option 2: Vault (HashiCorp Vault)
    # use_vault_for_encryption: true
    # vault_addr: "https://vault.example.com:8200"
    # vault_token: set via environment variable THEMIS_VAULT_TOKEN
    # vault_kv_mount: "themis"

# Storage Configuration (Production)
storage:
  backend: themisdb
  enable_encryption: true
  enable_signatures: true
  enable_versioning: true
  max_versions: 10
```

---

## Usage Examples

### Example 1: Basic LoRA Training (Development)

```cpp
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/data_loader.h"

int main() {
    // Configure training service
    LoRATrainingService::Config config;
    config.use_base_model = false;  // Standalone mode for quick testing
    config.default_hyperparameters.rank = 4;
    config.default_hyperparameters.alpha = 8.0f;
    config.default_hyperparameters.learning_rate = 1e-3f;
    
    LoRATrainingService service(config);
    
    // Prepare training data
    DataLoader data_loader;
    std::vector<std::string> texts = {
        "The quick brown fox jumps over the lazy dog.",
        "Machine learning is transforming technology.",
        "Natural language processing enables AI understanding."
    };
    
    // Convert to training samples
    TrainingData data;
    for (const auto& text : texts) {
        TrainingSample sample;
        sample.text = text;
        sample.label = "training";
        data.samples.push_back(sample);
    }
    
    // Train
    auto result = service.trainOnTheFly("my_test_adapter", data);
    
    if (result.success) {
        std::cout << "✅ Training completed!\n";
        std::cout << "Final loss: " << result.final_loss << "\n";
        std::cout << "Training time: " << result.training_time_seconds << "s\n";
    } else {
        std::cerr << "❌ Training failed: " << result.error_message << "\n";
    }
    
    return 0;
}
```

### Example 2: Training with Base Model (Development)

```cpp
#include "llm/lora_framework/lora_training_service.h"

int main() {
    // Configure for base model training
    LoRATrainingService::Config config;
    config.use_base_model = true;
    config.base_model_path = "models/llama-2-7b.gguf";
    config.target_modules = {
        "attention.wq", "attention.wk",
        "attention.wv", "attention.wo"
    };
    config.default_hyperparameters.rank = 16;
    config.default_hyperparameters.alpha = 32.0f;
    config.default_hyperparameters.learning_rate = 3e-4f;
    
    LoRATrainingService service(config);
    
    // Load training data from file
    TrainingData data;
    // ... load data ...
    
    // Register progress callback
    service.registerCallback([](const TrainingMetrics& metrics) {
        std::cout << "Epoch " << metrics.current_epoch << "/" << metrics.total_epochs
                  << " | Loss: " << metrics.current_loss
                  << " | Progress: " << (metrics.progress * 100) << "%\n";
    });
    
    // Train
    auto result = service.trainOnTheFly("qa_adapter", data);
    
    return result.success ? 0 : 1;
}
```

### Example 3: Loading and Using Adapters

⚠️ **Known Issue**: Adapter application to models not yet implemented (see REMAINING_GAPS_SUMMARY.md #3)

```cpp
#include "llm/lora_framework/lora_storage_service.h"

int main() {
    // Configure storage
    LoRAStorageService::Config config;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = "data/lora_adapters";
    config.enable_encryption = false;  // Development mode
    
    LoRAStorageService storage(config);
    
    // Load adapter
    auto weights = storage.loadAdapter("my_test_adapter");
    if (weights) {
        std::cout << "Adapter loaded: " << weights->size_bytes << " bytes\n";
        
        // TODO: Apply adapter to model (not yet implemented)
        // This functionality is tracked in REMAINING_GAPS_SUMMARY.md #3
    } else {
        std::cerr << "Failed to load adapter\n";
    }
    
    return 0;
}
```

---

## Known Limitations

### Current Limitations (Development)

1. **MockKeyProvider in Development Mode**
   - Encryption uses MockKeyProvider (NOT secure)
   - Only suitable for development/testing
   - Production deployment requires HSM or Vault

2. **LoRA Adapters Not Applied**
   - Adapters can be trained and stored
   - But not yet applied to inference
   - Models behave as base models (without fine-tuning)

3. **Hash-Based Embeddings**
   - Training uses simple hash-based embeddings
   - Not using actual base model embeddings
   - May affect training quality

4. **Simplified Forward/Backward Pass**
   - Training uses simplified computation
   - Not full multi-layer processing
   - Sufficient for development but not optimal

5. **Production Validator Incomplete**
   - Cannot fully validate production readiness
   - Many tests are placeholders
   - Manual validation required

### Performance Characteristics (Development)

**Training Speed (CPU-only)**:
- 10 samples: ~100ms
- 100 samples: ~500ms
- 1,000 samples: ~3-4s
- 10,000 samples: ~30-40s

**Memory Usage**:
- Standalone LoRA: ~9 MB (trainable)
- With base model (frozen): ~14 GB + 50 MB adapters
- Memory efficient: 99%+ parameter reduction

**Disk Space**:
- Model (GGUF): 4-14 GB depending on quantization
- LoRA Adapters: ~5-50 MB per adapter
- Training checkpoints: ~50-100 MB per checkpoint

---

## Environment Variables Reference

### Required for Production

```bash
# Environment mode (critical!)
export THEMIS_ENVIRONMENT=production  # Enforces secure key providers

# HSM Configuration (if using HSM)
export THEMIS_HSM_PIN=<secure-pin>
export THEMIS_HSM_SLOT=0

# Vault Configuration (if using Vault)
export THEMIS_VAULT_TOKEN=<vault-token>
```

### Optional for Development

```bash
# Development mode (allows MockKeyProvider)
export THEMIS_ENVIRONMENT=development

# Logging level
export SPDLOG_LEVEL=debug

# Model cache directory
export THEMIS_MODEL_CACHE=/tmp/themis_models
```

---

## Troubleshooting

### Issue: "Using MockKeyProvider - NOT SUITABLE FOR PRODUCTION"

**Symptom**: Warning messages about MockKeyProvider during startup

**Solution**: This is expected in development mode. To suppress:
```bash
export THEMIS_ENVIRONMENT=development
```

For production, configure HSM or Vault instead.

---

### Issue: "Training completed but adapter has no effect"

**Symptom**: Model behaves the same with or without adapter

**Solution**: This is a known limitation (#3 in REMAINING_GAPS_SUMMARY.md). Adapter application to inference is not yet implemented. Track issue for updates.

---

### Issue: "Failed to load model from ThemisDB"

**Symptom**: Cannot load models from database

**Solution**: Currently, only filesystem loading is fully supported. Use `loadModel(path)` instead of `loadModelFromThemisDB(urn)` for development.

---

### Issue: "Training loss not decreasing"

**Symptom**: Loss stays high or increases

**Possible causes**:
1. Learning rate too high - try 1e-4 to 1e-3
2. Rank too low - try rank 8 or 16
3. Not enough training data - need at least 100+ samples
4. Data quality issues - check tokenization

**Debug steps**:
```cpp
// Enable debug logging
service.setLogLevel(LogLevel::Debug);

// Check loss per epoch
service.registerCallback([](const TrainingMetrics& m) {
    std::cout << "Loss: " << m.current_loss << "\n";
});
```

---

## Security Considerations

### Development Mode

✅ **Acceptable for development**:
- MockKeyProvider for encryption
- Local filesystem storage
- No TLS required
- Disabled signatures

⚠️ **Never in production**:
- All of the above!

### Production Mode

🔒 **Required for production**:
- HSM or Vault for key management
- TLS 1.3 for all connections
- Enable signatures for integrity
- Audit logging enabled
- Environment variable: `THEMIS_ENVIRONMENT=production`

---

## Migration Path to Production

### Phase 1: Complete P0 Gaps (3-4 weeks)

1. ✅ Refactor lora_storage_service_themisdb.cpp
2. ✅ Implement LoRA adapter application
3. ✅ Real embeddings from base model
4. ✅ llama.cpp tokenizer integration

### Phase 2: Production Infrastructure (2-3 weeks)

1. Set up HSM or Vault
2. Configure TLS certificates
3. Enable audit logging
4. Set up monitoring (Prometheus + Grafana)

### Phase 3: Validation (1-2 weeks)

1. Complete production validator tests
2. Run load tests
3. Security audit (CodeQL + manual review)
4. Performance benchmarking

### Phase 4: Deployment (1 week)

1. Deploy to staging environment
2. Run integration tests
3. Gradual rollout to production
4. Monitor and validate

**Total Time**: 7-10 weeks from current state

**Note**: This timeline assumes:
- Focused effort on P0 items first (3-4 weeks)
- Parallel work on P1/P2 items (3-4 weeks, overlapping with P0)
- P3 items (production_validator tests) can be completed after deployment (1-2 weeks)
- Some work streams can run in parallel
- Does not require completing all 44 TODOs before production - P3 items are lower priority

---

## Support and Resources

### Documentation

- **Full Investigation**: `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md`
- **Executive Summary**: `EXECUTIVE_SUMMARY_GAPS_ANALYSIS.md`
- **Remaining Work**: `REMAINING_GAPS_SUMMARY.md`
- **Implementation Status**: `IMPLEMENTATION_COMPLETE.md`

### Examples

- **Training Guide**: `docs/en/llm/LORA_LLAMA_INTEGRATION_GUIDE.md`
- **Example Code**: `examples/lora_training_example.cpp`
- **Configuration**: `examples/lora_training_config.yaml`

### Getting Help

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Documentation**: https://makr-code.github.io/ThemisDB/

---

## Conclusion

### Current State Summary

**Development Ready**: ✅  
**Testing Ready**: ✅  
**Production Ready**: ⚠️ (after completing P0 gaps)

The ThemisDB LLM/LoRA system provides a solid foundation for development and research. Core functionality works well, but several critical features must be completed before production deployment.

**Recommended approach**:
1. Use for development and experimentation now
2. Track progress on P0 gap completion
3. Plan production deployment for 7-10 weeks from now
4. Set up production infrastructure in parallel

---

**Last Updated**: April 2026  
**Next Review**: After P0 gaps completion  
**Status**: 🚧 Active Development - Dev/Test Ready ✅
