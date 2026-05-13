<!-- Status: current | validated: 2026-05-13 | Source: src/llm/gguf_loader.cpp v0.0.47 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md · AUDIT.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# GGUF Quantization Loading

> **Scope:** This document covers `gguf_loader.cpp` / `include/llm/gguf_loader.h` only.
> For LoRA adapter loading see [LLAMA_LORA_ADAPTER_README.md](LLAMA_LORA_ADAPTER_README.md).
> For the high-level inference framework see [README.md](README.md) and [ARCHITECTURE.md](ARCHITECTURE.md).

## Overview

The GGUF loader implements **real quantization** instead of synthetic weights. Quantized GGUF models are loaded with direct format conversion, avoiding precision loss from dequantization/requantization cycles.

## GGUF File Format Requirements

- **Magic bytes:** `GGUF` (4 bytes at offset 0)
- **Version:** only GGUF **v3** is accepted; other versions cause `parseFile()` to return `false`
- **Metadata:** key-value pairs follow the header; the `general.architecture` key is extracted automatically

## Supported Quantization Formats

`GGUFLoader::isFormatSupported()` accepts the following types. Tensors with any other
type cause `parseFile()` to return `false` and set `getLastError()` with a descriptive
message (e.g. `"Unsupported quantization format Q6_K … Download a Q4_K_M or Q8_0 variant"`).

| GGUF Format | Internal Format | Block Size | Elements/Block | Conversion Method |
|-------------|-----------------|------------|----------------|-------------------|
| **F32** | FP32 | 4 bytes | 1 | Direct copy |
| **F16** | FP32 | 2 bytes | 1 | FP16→FP32 conversion |
| **Q4_K_M** | NF4 | 144 bytes | 256 | **Direct** Q4_K→NF4 |
| **Q8_0** | INT8 | 34 bytes | 32 | **Direct** Q8_0→INT8 |
| Q4_0 | - | 18 bytes | 32 | Not yet supported |
| Q4_1 | - | 20 bytes | 32 | Not yet supported |
| Q5_0 | - | 22 bytes | 32 | Not yet supported |
| Q5_1 | - | 24 bytes | 32 | Not yet supported |
| Q8_1 | - | 36 bytes | 32 | Not yet supported |
| Q5_K | - | 176 bytes | 256 | Not yet supported |
| Q6_K | - | 210 bytes | 256 | Not yet supported |

## Direct Quantization Conversion

### Previous Implementation (Wasteful)
```
GGUF Q4_K → FP32 (dequantize) → NF4 (requantize)
         ↑ Precision loss       ↑ Memory overhead
```

### New Implementation (Efficient)
```
GGUF Q4_K → NF4 (direct block copy)
         ↑ Preserves quality
```

## Key Features

### 1. Direct Format Conversion
- **`convertQ4KM_direct()`**: Q4_K_M → NF4 without FP32 intermediate
- **`convertQ8_0_direct()`**: Q8_0 → INT8 without FP32 intermediate
- Preserves original quantization quality
- Reduces memory overhead by 4-8x during conversion

### 2. Quantization Metadata Validation
- **`validateQuantizationMetadata()`**: Validates tensor integrity
  - Checks block sizes match expected format
  - Verifies tensor data size is consistent with dimensions
  - Detects corrupted quantization data
  - Validates tensor offsets within file bounds

### 3. Memory-Efficient Loading
- **Linux/macOS**: memory-mapped file access (`mmap`/`MAP_PRIVATE`) for zero-copy tensor loading
- **Windows**: entire file is read into `file_buffer_` at `parseFile()` time; `mmapTensor()` returns a pointer into that buffer
- Lazy tensor loading (only load what's needed)
- Chunk-based storage to RocksDB for persistence

## Security & Path Validation

> **AUDIT.md F1-1 / F2-1 (fixed 2026-04-21):** Model file paths received from API callers or storage
> metadata **must** be validated against a configured trusted directory before being passed to
> `GGUFLoader::parseFile()`.  The loader itself does **not** enforce this boundary — path validation
> is the responsibility of the calling layer (`GGUFLoader` configuration surface in `README.md`).
> See [AUDIT.md](AUDIT.md) and [SECURITY.md](SECURITY.md) for the full threat model and mitigations.

## Failure Modes

| Condition | `parseFile()` result | `getLastError()` message |
|-----------|---------------------|--------------------------|
| File not found / unreadable | `false` | *(empty; OS error via errno)* |
| File too small (< 24 bytes) | `false` | *(empty)* |
| Wrong magic bytes | `false` | *(empty)* |
| GGUF version ≠ 3 | `false` | *(empty)* |
| Unreasonable tensor/kv count (> 100 000 / > 10 000) | `false` | *(empty)* |
| Unsupported quantization type in any tensor | `false` | `"Unsupported quantization format <type> in tensor '<name>'. Supported formats: F32, F16, Q4_K_M, Q8_0. Download a Q4_K_M or Q8_0 variant of this model."` |
| Tensor offset out of file bounds | `false` | set via `validateQuantizationMetadata()` |
| Corrupt block size / dimension mismatch | `false` | set via `validateQuantizationMetadata()` |

Use `GGUFLoader::getLastError()` to retrieve a human-readable description after a failed call.

## Usage Examples

### Loading Quantized Model
```cpp
#include "llm/gguf_loader.h"
#include "llm/lora_framework/gguf_converter.h"

// Parse GGUF file (trusted path only — see SECURITY.md / AUDIT.md F1-1)
GGUFLoader loader;
if (!loader.parseFile("/trusted/models/model.gguf")) {
    spdlog::error("GGUF load failed: {}", loader.getLastError());
    return;
}

// Access tensor data (zero-copy via mmap on Linux; via file_buffer_ on Windows)
void* tensor_data = loader.mmapTensor("model.embed_tokens.weight");

// Direct conversion to internal format
TensorMetadata tensor_info = loader.getMetadata().tensors[0];
QuantizedTensor quantized = GGUFConverter::convertQ4KM_direct(
    tensor_data, tensor_info
);

// Validate quantization integrity
bool valid = loader.validateQuantizationMetadata("model.embed_tokens.weight");
```

### Integration with QLoRA Training
```cpp
#include "llm/lora_framework/quantized_model.h"

// Load quantized base model
QuantizedModel base_model = QuantizedModel::from_gguf(
    "model.gguf", 
    QuantizationType::NF4
);

// Model tensors are stored in quantized format
// Only dequantized during forward pass (layer-by-layer)
```

## Quantization Quality Comparison

### Q4_K_M (4-bit)
- **Memory**: ~4GB for 7B model (vs. 28GB FP32)
- **Accuracy**: ≥99% vs. full precision
- **Block structure**: 256 values/block with sub-block scales

### Q8_0 (8-bit)
- **Memory**: ~8GB for 7B model (vs. 28GB FP32)
- **Accuracy**: ≥99.5% vs. full precision
- **Block structure**: 32 values/block with FP16 scale

## Technical Details

### Q4_K_M Block Structure
```
struct Q4KBlock {
    uint8_t qs[128];        // Quantized values (4 bits each, packed)
    uint8_t scales[12];     // Scales and mins (mixed)
    uint16_t d;             // Delta (FP16)
    uint16_t dmin;          // Min (FP16)
}; // Total: 144 bytes for 256 values
```

### Q8_0 Block Structure
```
struct Q8_0Block {
    uint16_t d;             // Scale (FP16)
    int8_t qs[32];          // Quantized values (INT8)
}; // Total: 34 bytes for 32 values
```

## Performance Benefits

1. **Memory Efficiency**
   - Avoids temporary FP32 buffer allocation
   - Direct block-to-block copy
   - 4-8x less memory during conversion

2. **Accuracy Preservation**
   - No precision loss from dequant/requant
   - Original GGUF quantization quality maintained
   - Suitable for production inference

3. **Speed**
   - Faster loading (no FP32 intermediate)
   - Reduced memory bandwidth requirements
   - Zero-copy access via mmap

## Security & Safety

### Overflow Prevention
- **INT8 Conversion**: Clamped to [0, 255] range to prevent overflow
- **Scale Validation**: Block scales initialized to 0.0f for proper tracking
- **Zero Point**: Correctly set to 128.0f for symmetric signed-to-unsigned conversion

### Data Integrity
- Metadata validation checks tensor sizes and block counts
- Bounds checking for tensor offsets within file
- Format validation for all quantization types

### Path Security
See the **Security & Path Validation** section above and [SECURITY.md](SECURITY.md) / [AUDIT.md](AUDIT.md).

## Testing

See `tests/test_gguf_loader.cpp` for comprehensive test coverage:
- `DirectQuantizedLoading`: Q4_K_M direct conversion test
- `DirectQ8_0Loading`: Q8_0 direct conversion test
- `QuantizationMetadataValidation`: Integrity validation test
- `QuantizedModelIntegration`: End-to-end integration test

## References

- GGUF Specification: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md
- QLoRA Paper: https://arxiv.org/abs/2305.14314
- NF4 Quantization: 4-bit NormalFloat for neural network weights

## Core LLM Documentation

| Document | Description |
|----------|-------------|
| [README.md](README.md) | Module overview, engine descriptions, configuration surfaces |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Component diagrams, KV-cache and batching internals |
| [SECURITY.md](SECURITY.md) | Threat model, path-injection mitigations |
| [AUDIT.md](AUDIT.md) | S0/S1/S2 findings (F1-1, F2-1 — model path validation) |
| [ROADMAP.md](ROADMAP.md) | Implementation status and planned work |
| [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) | Long-horizon backlog |
| [LLAMA_LORA_ADAPTER_README.md](LLAMA_LORA_ADAPTER_README.md) | LoRA adapter runtime loading |

## Review / Audit Trail

| Date | Reviewer | Scope | Result |
|------|----------|-------|--------|
| 2026-04-21 | Copilot | `gguf_loader.cpp` path injection (F1-1, F2-1) | Fixed — trusted-directory guard at call site |
| 2026-05-13 | Copilot | Documentation sync with `gguf_loader.cpp` v0.0.47 | Updated — scope boundary, failure modes, Windows behavior, cross-references added |
