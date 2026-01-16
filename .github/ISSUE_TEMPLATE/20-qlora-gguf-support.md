---
name: "📦 GGUF Format Support for QLoRA"
about: Add support for loading GGUF quantized models (Q4_K_M, Q8_0) for QLoRA training
title: "[QLoRA] GGUF Format Support"
labels: priority:P2, type:feature, area:llm, effort:medium, phase:2-formats
assignees: ''

---

## 📋 Description

Add support for loading GGUF format quantized models (Q4_K_M, Q8_0) from llama.cpp ecosystem, enabling QLoRA training on pre-quantized models without re-quantization.

**Prerequisites**: 
- ✅ QLoRA Infrastructure Complete
- ✅ NF4/INT8 quantization working
- ⏳ llama.cpp integration

**Related Documents**: 
- `QLORA_IMPLEMENTATION_SUMMARY.md`
- `docs/QUANTIZATION_FORMATS.md`
- GGUF Spec: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md

## 🎯 Goals

- [ ] Load GGUF Q4_K_M models
- [ ] Load GGUF Q8_0 models
- [ ] Convert GGUF to internal quantization format
- [ ] Support llama.cpp model zoo
- [ ] Avoid re-quantization overhead
- [ ] Compatibility with existing QLoRA infrastructure

## 📝 Tasks

### 1. GGUF Format Parser
- [ ] Read GGUF file header
- [ ] Parse metadata (model config, tokenizer)
- [ ] Extract tensor information
- [ ] Support version 3 format

**Files**:
- `include/llm/lora_framework/gguf_loader.h`
- `src/llm/lora_framework/gguf_loader.cpp`

**Structure**:
```cpp
class GGUFLoader {
public:
    struct GGUFHeader {
        uint32_t magic;      // "GGUF"
        uint32_t version;    // 3
        uint64_t tensor_count;
        uint64_t metadata_kv_count;
    };
    
    struct TensorInfo {
        std::string name;
        std::vector<uint64_t> dimensions;
        GGMLType type;       // Q4_K, Q8_0, etc.
        uint64_t offset;
        uint64_t size;
    };
    
    // Load GGUF file
    bool load(const std::string& path);
    
    // Get tensor data
    const void* getTensorData(const std::string& name) const;
    
    // Get model metadata
    const std::unordered_map<std::string, std::string>& metadata() const;
    
private:
    GGUFHeader header_;
    std::vector<TensorInfo> tensors_;
    std::unordered_map<std::string, std::string> metadata_;
    std::unique_ptr<MappedFile> file_;
};
```

### 2. GGUF Quantization Format Converters
- [ ] Q4_K_M → Internal NF4 converter
- [ ] Q8_0 → Internal INT8 converter
- [ ] Handle block-wise quantization differences
- [ ] Preserve quantization accuracy

**Q4_K_M Format**:
```
Block (256 values):
  - 32 bytes: Quantized data (4 bits per value)
  - 2 bytes: FP16 scales (per 32 values, 8 groups)
  - 2 bytes: FP16 min values
  Total: 36 bytes per 256 values
```

**Q8_0 Format**:
```
Block (32 values):
  - 32 bytes: INT8 quantized data
  - 2 bytes: FP16 scale
  Total: 34 bytes per 32 values
```

**Converter**:
```cpp
class GGUFConverter {
public:
    // Convert Q4_K_M to internal NF4
    QuantizedTensor convertQ4KM(
        const void* gguf_data,
        const std::vector<size_t>& shape,
        const TensorInfo& info
    );
    
    // Convert Q8_0 to internal INT8
    QuantizedTensor convertQ8_0(
        const void* gguf_data,
        const std::vector<size_t>& shape,
        const TensorInfo& info
    );
    
private:
    // Dequantize GGUF format
    std::vector<float> dequantizeGGUF(
        const void* data,
        const TensorInfo& info
    );
    
    // Requantize to internal format
    QuantizedTensor requantize(
        const std::vector<float>& values,
        QuantizationType target_type
    );
};
```

### 3. Model Loading Integration
- [ ] Detect GGUF format from file extension
- [ ] Load quantized weights layer by layer
- [ ] Create QuantizedModel from GGUF
- [ ] Memory-efficient streaming

**Integration**:
```cpp
class QuantizedModelLoader {
public:
    // Load from GGUF file
    static QuantizedModel loadFromGGUF(
        const std::string& gguf_path,
        const QuantizedModelConfig& config
    );
    
    // Load from directory (multiple GGUF files)
    static QuantizedModel loadFromDirectory(
        const std::string& dir_path,
        const QuantizedModelConfig& config
    );
    
private:
    // Stream load large models
    void streamLoad(
        const std::string& path,
        QuantizedModel& model
    );
};
```

### 4. Format Compatibility Matrix
- [ ] Document supported formats
- [ ] Conversion accuracy tests
- [ ] Performance comparison
- [ ] Best practices guide

**Supported Formats**:
```
GGUF Format | Internal Format | Accuracy Loss | Notes
------------|-----------------|---------------|-------
Q4_K_M      | NF4            | < 0.5%        | K-means better than uniform
Q8_0        | INT8           | < 0.1%        | Nearly lossless
F16         | FP16           | ~0%           | Direct copy
F32         | FP32           | 0%            | No conversion
```

### 5. llama.cpp Model Zoo Support
- [ ] Download models from HuggingFace
- [ ] Verify checksums
- [ ] Cache downloaded models
- [ ] Support popular model families

**Supported Models**:
```
Model Family     | Sizes           | GGUF Support
-----------------|-----------------|-------------
Llama-2          | 7B, 13B, 70B   | ✅
Llama-3          | 8B, 70B        | ✅
Mistral          | 7B             | ✅
Mixtral          | 8x7B           | ✅
Phi-2            | 2.7B           | ✅
Gemma            | 2B, 7B         | ✅
```

**Helper**:
```cpp
class ModelZoo {
public:
    // Download from HuggingFace
    static std::string downloadModel(
        const std::string& repo_id,
        const std::string& filename,
        const std::string& cache_dir = "~/.cache/themisdb/models"
    );
    
    // List available models
    static std::vector<std::string> listModels();
};
```

### 6. Testing & Validation
- [ ] Load various GGUF models
- [ ] Verify conversion accuracy
- [ ] Compare training results
- [ ] Performance benchmarks

**Test Cases**:
1. Load Llama-2-7B Q4_K_M
2. Load Llama-2-7B Q8_0
3. Compare with native quantization
4. Training convergence validation
5. Memory usage verification

**Files**:
- `tests/test_gguf_loader.cpp`
- `tests/test_gguf_conversion.cpp`

### 7. Documentation
- [ ] GGUF format guide
- [ ] Conversion process explanation
- [ ] Model zoo usage
- [ ] Troubleshooting

**Files**:
- `docs/GGUF_SUPPORT.md` (new)
- `docs/QUANTIZATION_FORMATS.md` (update)

## ✅ Acceptance Criteria

- [ ] Can load GGUF Q4_K_M and Q8_0 models
- [ ] Conversion accuracy within 0.5%
- [ ] Training works with loaded models
- [ ] Memory efficiency maintained
- [ ] Popular models from HuggingFace supported
- [ ] All tests passing
- [ ] Documentation complete

## 🔗 Dependencies

- ✅ QLoRA Infrastructure
- ⏳ llama.cpp library (optional, for reference)
- ⏳ GGUF format spec
- ⏳ HuggingFace model hub access

## 📊 Estimated Effort

**Time**: 2-3 weeks  
**Priority**: 🟢 Medium (P2 - ecosystem compatibility)  
**Complexity**: Medium (format parsing, conversion)

## 🧪 Test Strategy

1. **Format Parsing**: Correctly read GGUF files
2. **Conversion**: Accuracy within tolerance
3. **Training**: Models train successfully
4. **Memory**: No memory leaks or excess usage
5. **Compatibility**: Works with various GGUF versions

### Accuracy Targets

```
Metric              | Q4_K_M → NF4 | Q8_0 → INT8
--------------------|--------------|-------------
Reconstruction MSE  | < 0.005      | < 0.0001
Training Accuracy   | 99%+         | 99.5%+
Memory Overhead     | < 5%         | < 2%
```

## 📚 References

- GGUF Spec: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md
- llama.cpp: https://github.com/ggerganov/llama.cpp
- HuggingFace Model Hub: https://huggingface.co/models
- GGML Quantization: https://github.com/ggerganov/ggml/blob/master/docs/quantization.md

## 💡 Implementation Notes

### Why GGUF Support?

**Advantages**:
- ✅ Huge ecosystem of pre-quantized models
- ✅ No need to quantize from scratch
- ✅ Standardized format
- ✅ llama.cpp compatibility

**Use Cases**:
1. Train on popular HuggingFace models
2. Avoid quantization step (save time)
3. Use community-optimized quantizations
4. Interoperability with llama.cpp

### Conversion Strategy

**Option 1: Direct Mapping** (Preferred)
- Convert GGUF blocks to internal format
- Preserve quantization parameters
- Minimal accuracy loss

**Option 2: Dequantize + Requantize**
- Dequantize GGUF to FP32
- Quantize to internal format
- More flexible but slower

**Recommendation**: Use Option 1 for Q4_K_M and Q8_0

### Model Loading Performance

```
Model Size | Load Time (Direct) | Load Time (Convert)
-----------|-------------------|--------------------
Llama-7B   | 5-10 seconds     | 30-60 seconds
Llama-13B  | 10-20 seconds    | 60-120 seconds
Llama-70B  | 60-120 seconds   | 300-600 seconds
```

## 🏁 Definition of Done

- [ ] GGUF parser implemented
- [ ] Q4_K_M and Q8_0 converters working
- [ ] Model loading integration complete
- [ ] All tests passing
- [ ] Documentation ready
- [ ] Code reviewed and merged
- [ ] Popular models validated
