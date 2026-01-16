# QLoRA (Quantized LoRA) Training Guide

## Overview

QLoRA (Quantized Low-Rank Adaptation) enables memory-efficient fine-tuning of large language models by quantizing base model weights to 4-bit or 8-bit precision while keeping LoRA adapters in full precision. This allows training significantly larger models on consumer GPUs.

**Memory Savings**: 60-80% reduction compared to full-precision LoRA  
**Accuracy**: Within 1-2% of full-precision LoRA  
**Paper**: [QLoRA: Efficient Finetuning of Quantized LLMs](https://arxiv.org/abs/2305.14314)

## Key Concepts

### Quantization Types

#### NF4 (4-bit NormalFloat)
- **Bits per parameter**: 4
- **Memory reduction**: ~81%
- **Optimal for**: Normally distributed neural network weights
- **16 bins**: Non-uniform spacing, denser near zero
- **Recommended**: Yes (default for QLoRA)

```cpp
// NF4 bins (optimized for normal distribution)
[-1.0, -0.6962, -0.5251, -0.3949, -0.2844, -0.1848, -0.0911, 0.0,
  0.0796, 0.1609, 0.2461, 0.3379, 0.4407, 0.5626, 0.7230, 1.0]
```

#### INT8 (8-bit Integer)
- **Bits per parameter**: 8
- **Memory reduction**: ~69%
- **Optimal for**: General purpose, higher accuracy requirements
- **Range**: [-127, 127] (symmetric around 0)
- **Recommended**: When accuracy is critical

### Block-wise Quantization

Weights are quantized in blocks (typically 64-128 elements) with separate scale and zero-point parameters per block. This improves accuracy by adapting to local weight distributions.

```
Block 1: [weights 0-63]    -> scale₁, zero₁
Block 2: [weights 64-127]  -> scale₂, zero₂
...
```

### Double Quantization

Further reduces memory by quantizing the quantization constants (scales and zero-points) to 8-bit. Provides an additional ~2% memory savings.

```
Without: 4-bit weights + 32-bit constants = 4.125 bits/param
With:    4-bit weights + 8-bit constants  = 4.03 bits/param
Savings: ~2% additional reduction
```

## Architecture

### QLoRA Training Flow

```
┌─────────────────────────────────────────────────────────┐
│ 1. Load Base Model (FP32/FP16)                         │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────┐
│ 2. Quantize to NF4/INT8                                 │
│    Memory: 13 GB → 3.5 GB (73% reduction)               │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────┐
│ 3. Initialize LoRA Adapters (FP32, trainable)          │
│    Memory: +50 MB                                        │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────┐
│ 4. Forward Pass                                          │
│    • Dequantize base weights on-demand                  │
│    • Compute: output = base(x) + LoRA(x)                │
│    • Discard dequantized weights                         │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────┐
│ 5. Backward Pass                                         │
│    • Compute gradients ONLY for LoRA                     │
│    • Base model gradients: SKIPPED (frozen)              │
└───────────────┬─────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────┐
│ 6. Optimizer Update                                      │
│    • Update ONLY LoRA parameters                         │
│    • Base model: unchanged                               │
└─────────────────────────────────────────────────────────┘
```

## Usage

### Basic Example

```cpp
#include "llm/lora_framework/quantized_model.h"
#include "llm/lora_framework/quantization.h"

using namespace themis::llm::lora;

// 1. Configure quantization
QuantizedModelConfig config;
config.quantization_type = QuantizationType::NF4;  // or INT8
config.block_size = 64;                             // 64 or 128
config.use_double_quantization = true;              // Extra savings
config.layer_by_layer = true;                       // Memory efficient

// 2. Load and quantize base model
Tensor base_weights = load_model_weights("llama-7b");  // Your loader
auto quantized_weights = std::make_shared<QuantizedLayerWeights>(
    base_weights, config
);

// 3. Create QLoRA layer
QLoRALayer qlora_layer(
    768,                   // input dimension
    768,                   // output dimension
    8,                     // LoRA rank
    quantized_weights,     // Quantized base
    1.0f                   // Scaling factor
);

// 4. Setup optimizer (updates only LoRA, not base)
SGDOptimizer optimizer(0.001f);  // or Adam
optimizer.add_parameters(qlora_layer.parameters());

// 5. Training loop
for (int epoch = 0; epoch < num_epochs; ++epoch) {
    for (auto& batch : dataset) {
        optimizer.zero_grad();
        
        // Forward pass
        Tensor output = qlora_layer.forward(batch.input);
        
        // Compute loss
        Tensor loss = compute_loss(output, batch.target);
        
        // Backward pass (gradients only for LoRA)
        Tensor grad = loss.backward();
        qlora_layer.backward(grad);
        
        // Update LoRA parameters
        optimizer.step();
    }
}

// 6. Save LoRA adapters (not base model)
auto [B, A] = qlora_layer.get_lora_weights();
save_weights("lora_adapter.bin", B, A);
```

### Advanced: Full Model Quantization

```cpp
// Quantize entire model
std::unordered_map<std::string, Tensor> model_weights = {
    {"layer1.weight", load_layer("layer1")},
    {"layer2.weight", load_layer("layer2")},
    // ... more layers
};

QuantizedModel quantized_model = 
    quantized_model_utils::convert_to_quantized(
        model_weights, config
    );

// Get memory savings
size_t original_bytes = calculate_original_size(model_weights);
size_t quantized_bytes = quantized_model.memory_bytes();
float reduction = 1.0f - (float)quantized_bytes / original_bytes;

std::cout << "Memory reduction: " << (reduction * 100) << "%" << std::endl;
// Output: Memory reduction: 73.5%

// Dequantize specific layer when needed
Tensor layer1_weights = quantized_model.dequantize_layer("layer1.weight");
```

## Memory Estimates

### Model Size Comparison

| Model      | Full LoRA | QLoRA (NF4) | Reduction | Max GPU (16GB) |
|------------|-----------|-------------|-----------|----------------|
| Llama-7B   | ~14 GB    | ~5-6 GB     | 60-65%    | ✅ Fits         |
| Llama-13B  | ~26 GB    | ~9-10 GB    | 62-65%    | ✅ Fits         |
| Llama-30B  | ~60 GB    | ~20-22 GB   | 63-67%    | ❌ Needs 24GB   |
| Llama-65B  | ~130 GB   | ~40-45 GB   | 65-69%    | ❌ Needs 48GB   |

### Memory Breakdown (Llama-7B Example)

```
Component              Full LoRA    QLoRA (NF4)   Savings
--------------------------------------------------------------
Base Model Weights     13 GB        3.5 GB        73%
LoRA Adapters          50 MB        50 MB         0%
Optimizer States       50 MB        50 MB*        0%
Activations            2 GB         2 GB          0%
--------------------------------------------------------------
Total                  ~15 GB       ~5.6 GB       63%

* Can be further reduced with paged optimizers
```

## Performance Characteristics

### Accuracy

| Quantization | Reconstruction Error | Training Accuracy | Use Case                    |
|--------------|---------------------|-------------------|------------------------------|
| NF4          | MSE < 0.01          | 98-99% of FP32    | Standard QLoRA training      |
| INT8         | MSE < 0.0001        | 99-99.5% of FP32  | High accuracy requirements   |
| FP16         | MSE < 1e-6          | 99.9% of FP32     | Baseline comparison          |

### Speed

- **Forward pass**: 10-20% slower than full LoRA (due to dequantization)
- **Backward pass**: Same speed (only LoRA gradients)
- **Overall throughput**: 0.8-0.9x of full LoRA
- **Memory bandwidth**: Significantly reduced (4x-8x less data)

### Trade-offs

✅ **Advantages**:
- 60-80% memory reduction
- Train larger models on same hardware
- Minimal accuracy loss (<2%)
- Easy to integrate

⚠️ **Disadvantages**:
- Slightly slower (10-20%)
- Dequantization overhead
- Limited to CPU/GPU (no TPU support yet)

## Configuration Reference

```cpp
struct QuantizedModelConfig {
    // Quantization type
    QuantizationType quantization_type = QuantizationType::NF4;
    
    // Block size for block-wise quantization
    // Smaller = better accuracy, more memory overhead
    // Larger = less accuracy, less memory overhead
    // Typical: 64 or 128
    size_t block_size = 64;
    
    // Double quantization (quantize the quantization constants)
    // Saves additional ~2% memory
    bool use_double_quantization = false;
    
    // Layer-by-layer dequantization
    // Dequantize only active layers during forward pass
    // Reduces peak memory usage
    bool layer_by_layer = true;
};
```

### Choosing Quantization Type

**Use NF4 when**:
- Memory is the primary constraint
- Training general-purpose models
- Weights follow normal distribution (most neural nets)

**Use INT8 when**:
- Accuracy is critical
- You have slightly more memory available
- Weights have outliers or non-normal distribution

### Choosing Block Size

**Smaller blocks (32-64)**:
- ✅ Better accuracy (adapt to local distributions)
- ❌ More memory overhead (more scale/zero parameters)
- Use when: Accuracy is critical

**Larger blocks (128-256)**:
- ❌ Lower accuracy (less adaptive)
- ✅ Less memory overhead
- Use when: Memory is extremely constrained

## Best Practices

### 1. Start with Default Settings

```cpp
QuantizedModelConfig config;
config.quantization_type = QuantizationType::NF4;  // Most efficient
config.block_size = 64;                             // Good balance
config.use_double_quantization = true;              // Free savings
```

### 2. Validate Quantization Error

```cpp
// Check reconstruction error before training
Tensor original = load_weights("model.bin");
QuantizedLayerWeights q_weights(original, config);
Tensor reconstructed = q_weights.dequantize();

float error = compute_mse(original, reconstructed);
std::cout << "Quantization error: " << error << std::endl;

// Expected: < 0.01 for NF4, < 0.0001 for INT8
assert(error < 0.01f);
```

### 3. Monitor Memory Usage

```cpp
// Before training
size_t initial_memory = get_gpu_memory_used();

// During training
size_t current_memory = get_gpu_memory_used();
size_t peak_memory = get_gpu_memory_peak();

std::cout << "Current: " << current_memory << " MB" << std::endl;
std::cout << "Peak: " << peak_memory << " MB" << std::endl;
```

### 4. Layer-by-Layer Quantization

```cpp
// For very large models, quantize layer by layer
QuantizedModel model(config);

for (const auto& [name, weights] : model_layers) {
    model.add_layer(name, weights);
    
    // Optional: free original weights immediately
    weights.free_memory();
}
```

## Troubleshooting

### High Memory Usage

**Problem**: Memory usage higher than expected  
**Solutions**:
1. Enable `layer_by_layer = true`
2. Use `use_double_quantization = true`
3. Increase block size (128 instead of 64)
4. Check for memory leaks in dequantization

### Poor Accuracy

**Problem**: Model accuracy significantly lower after quantization  
**Solutions**:
1. Switch from NF4 to INT8
2. Decrease block size (32 instead of 64)
3. Check quantization error (should be < 0.01)
4. Verify LoRA rank is sufficient (8-16 for most models)

### Slow Training

**Problem**: Training significantly slower than expected  
**Solutions**:
1. Profile dequantization overhead
2. Use larger block sizes to reduce dequantization calls
3. Consider caching dequantized weights (trades memory for speed)
4. Verify layer-by-layer mode is enabled

## API Reference

### Core Classes

```cpp
// Quantized tensor storage
class QuantizedTensor {
    QuantizationType type() const;
    const std::vector<size_t>& shape() const;
    size_t memory_bytes() const;
};

// Quantized layer weights
class QuantizedLayerWeights {
    Tensor dequantize() const;
    size_t memory_bytes() const;
    QuantizationType type() const;
};

// Quantized model
class QuantizedModel {
    void add_layer(const std::string& name, const Tensor& weights);
    Tensor dequantize_layer(const std::string& name) const;
    size_t num_layers() const;
    size_t memory_bytes() const;
};

// QLoRA layer
class QLoRALayer : public ITrainableLayer {
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    std::vector<Tensor*> parameters() override;
    size_t parameter_count() const override;
    size_t memory_bytes() const override;
};
```

### Utility Functions

```cpp
namespace quantization {
    // Quantize to NF4
    void quantize_nf4(const std::vector<float>& input,
                      QuantizedTensor& output,
                      size_t block_size = 64);
    
    // Quantize to INT8
    void quantize_int8(const std::vector<float>& input,
                       QuantizedTensor& output,
                       size_t block_size = 64);
    
    // Dequantize
    void dequantize(const QuantizedTensor& input,
                    std::vector<float>& output);
    
    // Compute quantization error
    float quantization_error(const std::vector<float>& original,
                             const QuantizedTensor& quantized);
}

namespace quantized_model_utils {
    // Estimate memory usage
    size_t estimate_memory_usage(size_t num_parameters,
                                  QuantizationType quant_type,
                                  size_t block_size = 64,
                                  bool use_double_quant = false);
    
    // Calculate memory reduction
    float calculate_memory_reduction(size_t original_bytes,
                                      QuantizationType quant_type);
    
    // Convert model to quantized
    QuantizedModel convert_to_quantized(
        const std::unordered_map<std::string, Tensor>& model_weights,
        const QuantizedModelConfig& config);
}
```

## Examples

See:
- `tests/test_quantization.cpp` - Quantization tests and examples
- `tests/test_qlora.cpp` - QLoRA training examples
- `examples/qlora_finetuning/` - Complete training example (coming soon)

## References

1. **QLoRA Paper**: https://arxiv.org/abs/2305.14314
2. **LoRA Paper**: https://arxiv.org/abs/2106.09685
3. **bitsandbytes**: https://github.com/TimDettmers/bitsandbytes
4. **GPTQ**: https://arxiv.org/abs/2210.17323
5. **llama.cpp quantization**: https://github.com/ggerganov/llama.cpp

## License

MIT License - See LICENSE file for details
