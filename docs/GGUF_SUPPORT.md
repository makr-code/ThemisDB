# GGUF Format Support

## Overview

ThemisDB supports loading pre-quantized models in GGUF (GGML Universal File) format from the llama.cpp ecosystem. This enables QLoRA training on quantized models without requiring re-quantization, saving significant time and computational resources.

**Note**: ThemisDB supports two GGUF-based formats:
1. **Standard GGUF v3** (this document) - For loading base models
2. **GGUF-ST (GGUF + SafeTensors)** - For LoRA adapters with embedded metadata (see `include/llm/gguf_st_adapter.h` and `docs/de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md`)

## Supported Formats

### GGUF Version 3

ThemisDB supports GGUF version 3, the latest stable format specification for base models.

### Quantization Types

The table below reflects the current status as of v1.5.  `GGUFLoader::isFormatSupported()` is the canonical check for whether a format will be accepted at parse time.

| GGUF Type | Block size | Elements/block | Internal Type | Status |
|-----------|------------|----------------|---------------|--------|
| F32       | 4 B/elem   | 1              | FP32          | ✅ Supported — direct copy |
| F16       | 2 B/elem   | 1              | FP32          | ✅ Supported — direct conversion |
| Q4_K_M    | 144 B      | 256            | NF4           | ✅ Supported — dequantize → NF4 |
| Q8_0      | 34 B       | 32             | INT8          | ✅ Supported — dequantize → INT8 |
| Q4_0      | 18 B       | 32             | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q4_1      | 20 B       | 32             | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q5_0      | 22 B       | 32             | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q5_1      | 24 B       | 32             | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q8_1      | 36 B       | 32             | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q5_K      | 176 B      | 256            | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q6_K      | 210 B      | 256            | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q2_K      | varies     | 256            | —             | ❌ Not supported — `parseFile()` returns an explicit error |
| Q3_K      | varies     | 256            | —             | ❌ Not supported — `parseFile()` returns an explicit error |

**Unsupported format behavior (v1.5+):** Loading a GGUF file that contains any tensor in an unsupported format causes `GGUFLoader::parseFile()` to return `false` immediately with a human-readable error available via `GGUFLoader::getLastError()`.  The error message names the format, the tensor, and suggests downloading a Q4_K_M or Q8_0 variant.  Prior to v1.5, unsupported formats silently returned raw quantized bytes, causing downstream numerical corruption.

**Model recommendation:** For production deployments use **Q4_K_M** (best accuracy/size trade-off) or **Q8_0** (highest accuracy).  Both are available for most popular models on HuggingFace and Ollama.

## Architecture

### GGUF File Structure

```
┌─────────────────────────────────────┐
│ Header (24 bytes)                   │
│  - Magic: "GGUF" (4 bytes)         │
│  - Version: 3 (4 bytes)             │
│  - Tensor count (8 bytes)           │
│  - Metadata count (8 bytes)         │
├─────────────────────────────────────┤
│ Metadata Key-Value Pairs            │
│  - Architecture (string)            │
│  - Model config (various types)     │
│  - Tokenizer info                   │
├─────────────────────────────────────┤
│ Tensor Information                  │
│  - Name (string)                    │
│  - Dimensions (uint64[])            │
│  - Type (GGMLType)                  │
│  - Offset (uint64)                  │
├─────────────────────────────────────┤
│ Tensor Data (aligned to 32 bytes)  │
│  - Quantized weights                │
│  - Block-wise format                │
└─────────────────────────────────────┘
```

### Conversion Pipeline

```
┌──────────────┐
│  GGUF File   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ GGUFLoader   │  Parse header, metadata, tensor info
│ Parse v3     │  Memory-map file for efficient access
└──────┬───────┘
       │
       ▼
┌──────────────────┐
│  GGUFConverter   │  Convert formats
│                  │
├──────────────────┤
│ Q4_K_M → NF4    │  Dequantize 4-bit K-means blocks
│                  │  Re-quantize to internal NF4 format
├──────────────────┤
│ Q8_0 → INT8     │  Dequantize 8-bit symmetric blocks
│                  │  Re-quantize to internal INT8 format
├──────────────────┤
│ F16 → FP32      │  IEEE 754 half→single precision
│ F32 → FP32      │  Direct copy
└──────┬───────────┘
       │
       ▼
┌────────────────┐
│ QuantizedModel │  Internal quantized format
│                │  Ready for QLoRA training
└────────────────┘
```

## Usage

### Loading a GGUF Model

```cpp
#include "llm/lora_framework/quantized_model.h"

using namespace themis::llm::lora;

// Load GGUF file (auto-detect quantization type)
QuantizedModel model = quantized_model_utils::load_from_gguf(
    "/path/to/model.gguf"
);

// Or specify custom config
QuantizedModelConfig config;
config.quantization_type = QuantizationType::NF4;
config.block_size = 64;

QuantizedModel model = quantized_model_utils::load_from_gguf(
    "/path/to/model.gguf",
    &config
);
```

### Using with QLoRA Training

```cpp
#include "llm/lora_framework/quantized_model.h"
#include "llm/lora_framework/quantization.h"

// 1. Load pre-quantized GGUF model
auto base_model = quantized_model_utils::load_from_gguf(
    "llama-2-7b-q4_k_m.gguf"
);

// 2. Create QLoRA layers for fine-tuning
std::vector<std::unique_ptr<QLoRALayer>> qlora_layers;

for (const auto& layer_name : base_model.layer_names()) {
    auto base_weights = base_model.get_layer(layer_name);
    
    // Create QLoRA layer with quantized base weights
    auto qlora = std::make_unique<QLoRALayer>(
        in_dim, out_dim, rank,
        std::make_shared<QuantizedLayerWeights>(*base_weights),
        scaling
    );
    
    qlora_layers.push_back(std::move(qlora));
}

// 3. Train with standard optimizer
// Only LoRA adapters are trainable, base weights stay frozen and quantized
```

### Direct GGUF Parsing

```cpp
#include "llm/gguf_loader.h"

using namespace themis::llm;

// Parse GGUF file
GGUFLoader loader;
if (!loader.parseFile("model.gguf")) {
    std::cerr << "Failed to parse GGUF file" << std::endl;
    return;
}

// Access metadata
const auto& metadata = loader.getMetadata();
std::cout << "Version: " << metadata.version << std::endl;
std::cout << "Architecture: " << metadata.architecture << std::endl;
std::cout << "Tensors: " << metadata.tensors.size() << std::endl;

// Access specific tensor
void* tensor_data = loader.mmapTensor("blk.0.attn_q.weight");
```

## Format Details

### Q4_K_M Format

**Block Size**: 256 values per block (144 bytes)

```
Block Structure:
  uint8_t  qs[128];       // Quantized values (4 bits each, packed)
  uint8_t  scales[12];    // Scales and minimums (mixed encoding)
  uint16_t d;             // Delta (FP16)
  uint16_t dmin;          // Minimum (FP16)
```

**Characteristics**:
- 4 bits per value (0.5 bytes)
- K-means clustering for optimal quantization
- Sub-block scales for accuracy
- ~144 / 256 = 0.56 bytes per value
- Memory reduction: ~86% vs FP32

**Conversion to NF4**:
1. Dequantize Q4_K_M blocks to FP32
2. Re-quantize to internal NF4 format (block size 64)
3. Accuracy loss: < 0.5% additional error

### Q8_0 Format

**Block Size**: 32 values per block (34 bytes)

```
Block Structure:
  uint16_t d;             // Scale (FP16)
  int8_t   qs[32];        // Quantized values (INT8)
```

**Characteristics**:
- 8 bits per value (1 byte)
- Symmetric quantization
- Single scale per block
- ~34 / 32 = 1.06 bytes per value
- Memory reduction: ~73% vs FP32

**Conversion to INT8**:
1. Dequantize Q8_0 blocks to FP32
2. Re-quantize to internal INT8 format (block size 64)
3. Accuracy loss: < 0.1% additional error

### FP16 Format

**Direct Conversion**:
- IEEE 754 half-precision (16 bits)
- Sign: 1 bit, Exponent: 5 bits, Mantissa: 10 bits
- Range: ±65,504 (normalized)
- Precision: ~3-4 decimal digits
- Converted to FP32 for internal use

## Performance

### Load Times

| Model Size | File Size (Q4_K_M) | Load Time | Peak Memory |
|------------|-------------------|-----------|-------------|
| Llama-2-7B | ~4 GB            | 5-10 sec  | ~4.5 GB     |
| Llama-2-13B| ~7 GB            | 10-20 sec | ~8 GB       |
| Llama-2-70B| ~36 GB           | 60-90 sec | ~40 GB      |

### Conversion Accuracy

| Format | Original → GGUF | GGUF → Internal | Total Error |
|--------|-----------------|-----------------|-------------|
| Q4_K_M | < 1%           | < 0.5%          | < 1.5%      |
| Q8_0   | < 0.1%         | < 0.1%          | < 0.2%      |

### Training Performance

Compared to re-quantizing from scratch:

- **Time Saved**: 80-90% (no quantization step needed)
- **Memory**: Same as native quantization
- **Training Speed**: < 5% slower (conversion overhead)
- **Final Accuracy**: 98-99% of native quantization

## Model Sources

### HuggingFace Models

Popular GGUF models on HuggingFace:

```
TheBloke/Llama-2-7B-GGUF
TheBloke/Llama-2-13B-GGUF
TheBloke/Mistral-7B-v0.1-GGUF
TheBloke/Mixtral-8x7B-v0.1-GGUF
```

### Download Example

```bash
# Install HuggingFace CLI
pip install huggingface-hub

# Download Q4_K_M quantized model
huggingface-cli download \
  TheBloke/Llama-2-7B-GGUF \
  llama-2-7b.Q4_K_M.gguf \
  --local-dir ./models
```

## Limitations

### Current Limitations

1. **Format Support**: Only Q4_K_M and Q8_0 (most common formats)
2. **Conversion Overhead**: 5-15% slower than direct use
3. **Memory Peak**: Temporary memory spike during conversion
4. **One-Time Cost**: Re-quantization happens on load

### Not Supported

- Q4_0, Q4_1, Q5_0, Q5_1 (legacy formats)
- Q5_K, Q6_K (less common)
- Q2_K (extreme compression)
- Custom quantization parameters

## Troubleshooting

### "Failed to parse GGUF file"

**Cause**: Invalid file format or unsupported version

**Solution**:
- Verify file is valid GGUF format
- Check version is 3 (run `file model.gguf`)
- Re-download if corrupted

### "Unsupported quantization type"

**Cause**: Model uses Q4_0, Q5_K, etc.

**Solution**:
- Download Q4_K_M or Q8_0 variant instead
- Use llama.cpp to convert: `quantize model.gguf model-q4km.gguf Q4_K_M`

### "Out of memory during load"

**Cause**: Large model + conversion overhead

**Solution**:
- Close other applications
- Use Q8_0 instead of Q4_K_M (less overhead)
- Enable swap/page file
- Use smaller model variant

### "Conversion accuracy too low"

**Cause**: Accumulated quantization errors

**Solution**:
- Use Q8_0 instead of Q4_K_M (higher accuracy)
- Consider re-quantizing from original FP16 weights
- Verify GGUF file quality

## API Reference

### GGUFLoader

```cpp
class GGUFLoader {
public:
    // Parse GGUF file header and metadata
    bool parseFile(const std::string& filepath);
    
    // Get parsed metadata
    const GGUFMetadata& getMetadata() const;
    
    // Memory-mapped tensor access
    void* mmapTensor(const std::string& tensor_name);
    
    // Extract tensor data
    std::vector<uint8_t> getTensorData(const std::string& tensor_name);
};
```

### GGUFConverter

```cpp
class GGUFConverter {
public:
    // Convert Q4_K_M to NF4
    static QuantizedTensor convertQ4KM(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    // Convert Q8_0 to INT8
    static QuantizedTensor convertQ8_0(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    // Convert F16 to FP32
    static std::vector<float> convertF16(
        const void* gguf_data,
        const TensorMetadata& tensor_info
    );
    
    // Check if type is supported
    static bool isSupported(GGMLType type);
};
```

### quantized_model_utils

```cpp
namespace quantized_model_utils {
    // Load from GGUF file
    QuantizedModel load_from_gguf(
        const std::string& gguf_path,
        const QuantizedModelConfig* config = nullptr
    );
}
```

## References

- [GGUF Specification](https://github.com/ggerganov/ggml/blob/master/docs/gguf.md)
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [GGML Quantization](https://github.com/ggerganov/ggml/blob/master/docs/quantization.md)
- [QLoRA Paper](https://arxiv.org/abs/2305.14314)

## Related Formats

### GGUF-ST (GGUF + SafeTensors Hybrid)

ThemisDB also supports **GGUF-ST**, a hybrid format that combines GGUF with embedded SafeTensors data, primarily used for LoRA adapters.

**Key Differences:**
- **Standard GGUF** (this document): Base model loading with quantized weights
- **GGUF-ST**: LoRA adapter format with optional SafeTensors, signatures, and manifests

**GGUF-ST Structure:**
```
┌─────────────────────────────────────┐
│ GGUF Header + Metadata + Tensors    │  (Standard GGUF v3)
├─────────────────────────────────────┤
│ [OPTIONAL] SafeTensors Section      │  (FP16/FP32 weights)
│   Header: "STNS"                    │
├─────────────────────────────────────┤
│ ThemisDB Signature Section          │  (Cryptographic signature)
│   Header: "TSGN"                    │
├─────────────────────────────────────┤
│ ThemisDB Manifest Section           │  (Adapter metadata)
│   Header: "TMFT"                    │
└─────────────────────────────────────┘
```

**Use Cases:**
- LoRA adapter storage with verification
- Compatibility: llama.cpp can read GGUF-ST (ignores ThemisDB extensions)
- Security: Embedded cryptographic signatures for adapter verification
- Metadata: Training config, base model info, version tracking

**Implementation:**
- Header file: `include/llm/gguf_st_adapter.h`
- Loader class: `GGUFSTAdapter`
- Documentation: `docs/de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md`

**Loading GGUF-ST Adapters:**
```cpp
#include "llm/gguf_st_adapter.h"

// Load LoRA adapter in GGUF-ST format
GGUFSTAdapter adapter(storage, config);
auto components = adapter.readAdapter(blob_ref);

// Verify adapter integrity
auto verification = adapter.verifyAdapter(blob_ref);
if (verification.valid && verification.signature_valid) {
    // Use adapter...
}
```

**Format Modes:**
- `FULL`: GGUF + SafeTensors + Signature + Manifest (~12-20MB)
- `COMPACT`: GGUF + Signature + Manifest (~8-16MB, default)
- `ULTRA_COMPACT`: GGUF + minimal metadata (~8MB)
- `SIGNATURE_ONLY`: Registry metadata only (~100KB)

## See Also

- [QUANTIZATION_FORMATS.md](performance/QUANTIZATION_FORMATS.md) - Quantization format comparison
- [QLORA_GUIDE.md](./QLORA_GUIDE.md) - QLoRA training guide
- [QLORA_IMPLEMENTATION_SUMMARY.md](../QLORA_IMPLEMENTATION_SUMMARY.md) - Implementation details
- [LORA_TRAINING_FRAMEWORK_INTEGRATION.md](./de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md) - GGUF-ST format specification (German)
