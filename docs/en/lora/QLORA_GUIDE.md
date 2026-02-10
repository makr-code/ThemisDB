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

### Training Service Integration

```cpp
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_config.h"

using namespace themis::llm::lora;

// 1. Configure training service with QLoRA
LoRATrainingService::Config service_config;

// Set base model
service_config.base_model_path = "models/llama-7b.gguf";
service_config.use_base_model = true;

// Configure hyperparameters
service_config.default_hyperparameters.rank = 8;
service_config.default_hyperparameters.alpha = 16.0f;
service_config.default_hyperparameters.learning_rate = 0.0001f;
service_config.default_hyperparameters.num_epochs = 3;
service_config.default_hyperparameters.batch_size = 4;

// Enable QLoRA
service_config.qlora.enabled = true;
service_config.qlora.quantization_type = "nf4";  // or "int8"
service_config.qlora.block_size = 64;
service_config.qlora.use_double_quantization = true;
service_config.qlora.layer_by_layer = true;

// 2. Create training service
LoRATrainingService service(service_config);

// 3. Prepare training data
TrainingData training_data;
training_data.dataset_name = "alpaca-100";
for (auto& sample : load_dataset("alpaca-100.json")) {
    TrainingDataSample ts;
    ts.input = sample.instruction;
    ts.output = sample.response;
    training_data.samples.push_back(ts);
}

// 4. Train with quantization
auto result = service.trainWithQuantization(
    "my_adapter",
    training_data
);

// 5. Check results
if (result.success) {
    std::cout << "Training completed!" << std::endl;
    std::cout << "Final loss: " << result.final_loss << std::endl;
    std::cout << "Epochs: " << result.epochs_completed << std::endl;
    
    // Check memory usage
    if (result.metrics.contains("memory_bytes")) {
        size_t memory_mb = result.metrics["memory_bytes"].get<size_t>() / (1024 * 1024);
        std::cout << "Memory used: " << memory_mb << " MB" << std::endl;
    }
    
    // Check quantization type used
    std::cout << "Quantization: " << 
        result.metrics["quantization_type"].get<std::string>() << std::endl;
} else {
    std::cerr << "Training failed: " << result.error_message << std::endl;
}
```

### JSON Configuration

You can also configure QLoRA training via JSON:

```json
{
  "adapter_id": "customer_support_qlora",
  "base_model": "llama-7b",
  "training": {
    "rank": 8,
    "alpha": 16,
    "learning_rate": 0.0001,
    "num_epochs": 3,
    "batch_size": 4,
    "optimizer": "adamw"
  },
  "qlora": {
    "enabled": true,
    "quantization_type": "nf4",
    "block_size": 64,
    "use_double_quantization": true,
    "layer_by_layer": true
  },
  "dataset": "data/customer_support.jsonl"
}
```

### Memory Estimation

Before training, estimate memory requirements:

```cpp
// Estimate memory for QLoRA training
QLoRAConfig qlora_config;
qlora_config.enabled = true;
qlora_config.quantization_type = "nf4";
qlora_config.block_size = 64;
qlora_config.use_double_quantization = true;

// For a 7B parameter model
size_t num_params = 7'000'000'000;
size_t estimated_bytes = quantized_model_utils::estimate_memory_usage(
    num_params,
    QuantizationType::NF4,
    qlora_config.block_size,
    qlora_config.use_double_quantization
);

float estimated_gb = estimated_bytes / (1024.0f * 1024.0f * 1024.0f);
std::cout << "Estimated memory: " << estimated_gb << " GB" << std::endl;
// Output: Estimated memory: ~4.2 GB

// Check if it fits in available GPU memory
size_t available_memory = get_available_gpu_memory();
if (estimated_bytes < available_memory) {
    std::cout << "✓ Model will fit in GPU memory" << std::endl;
} else {
    std::cout << "✗ Insufficient GPU memory" << std::endl;
    std::cout << "  Consider: smaller model, different quantization, or CPU offload" << std::endl;
}
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

## Troubleshooting

### Out of Memory Errors

**Problem**: Training crashes with OOM (Out of Memory) error

**Solutions**:
1. **Use smaller block size**: Try `block_size = 32` instead of 64
2. **Enable double quantization**: Set `use_double_quantization = true`
3. **Reduce batch size**: Lower `batch_size` from 4 to 2 or 1
4. **Enable layer-by-layer mode**: Set `layer_by_layer = true`
5. **Try INT8 instead of NF4**: `quantization_type = "int8"` (uses more memory but faster)

```cpp
// Memory-constrained configuration
service_config.qlora.quantization_type = "nf4";
service_config.qlora.block_size = 32;  // Smaller blocks
service_config.qlora.use_double_quantization = true;
service_config.qlora.layer_by_layer = true;
service_config.default_hyperparameters.batch_size = 1;  // Minimum
```

### High Quantization Error

**Problem**: Model accuracy significantly degraded after quantization

**Solutions**:
1. **Use INT8 instead of NF4**: Higher precision, less degradation
2. **Increase block size**: Try `block_size = 128` for better accuracy
3. **Check input data range**: Ensure weights are roughly normalized
4. **Validate quantization error**: Use `quantization_error()` to measure

```cpp
// Check quantization error
auto q_weights = QuantizedLayerWeights(weights, config);
auto reconstructed = q_weights.dequantize();

float error = 0.0f;
for (size_t i = 0; i < weights.size(); ++i) {
    float diff = weights[i] - reconstructed[i];
    error += diff * diff;
}
error /= weights.size();

if (error > 0.01f) {
    std::cerr << "Warning: High quantization error: " << error << std::endl;
    std::cerr << "Consider using INT8 or larger block size" << std::endl;
}
```

### Slow Training

**Problem**: QLoRA training is much slower than expected

**Causes & Solutions**:
1. **Dequantization overhead**: Normal 10-20% slowdown
   - **Solution**: Use INT8 for faster dequantization
2. **Small block size**: More blocks = more overhead
   - **Solution**: Increase block size to 128 or 256
3. **Layer-by-layer mode**: Can be slower but saves memory
   - **Solution**: Disable if memory allows: `layer_by_layer = false`
4. **Optimizer state**: Using large optimizer state
   - **Solution**: Use SGD instead of Adam for less memory

```cpp
// Speed-optimized configuration (requires more memory)
service_config.qlora.quantization_type = "int8";  // Faster dequantization
service_config.qlora.block_size = 128;  // Fewer blocks
service_config.qlora.layer_by_layer = false;  // Keep in memory
service_config.default_hyperparameters.optimizer = "sgd";  // Smaller state
```

### Training Not Converging

**Problem**: Loss not decreasing, model not learning

**Solutions**:
1. **Check learning rate**: May be too high or too low
   - **Try**: `learning_rate = 0.0001` to `0.001`
2. **Verify gradients**: Ensure backward pass is working
3. **Check data quality**: Ensure training data is valid
4. **Increase LoRA rank**: Try `rank = 16` or `32` for more capacity

```cpp
// Debugging non-convergence
service.registerCallback([](const TrainingMetrics& metrics) {
    std::cout << "Step " << metrics.current_step 
              << " Loss: " << metrics.current_loss << std::endl;
    
    if (std::isnan(metrics.current_loss)) {
        std::cerr << "ERROR: NaN loss detected!" << std::endl;
        std::cerr << "  - Learning rate may be too high" << std::endl;
        std::cerr << "  - Try: learning_rate = 0.0001" << std::endl;
    }
});
```

### Quantization Type Errors

**Problem**: Invalid quantization type or unsupported configuration

**Solution**: Verify supported quantization types

```cpp
// Supported types
std::vector<std::string> supported = {"nf4", "int8", "none"};

if (std::find(supported.begin(), supported.end(), 
              config.quantization_type) == supported.end()) {
    std::cerr << "Unsupported quantization type: " 
              << config.quantization_type << std::endl;
    std::cerr << "Supported types: nf4, int8, none" << std::endl;
    config.quantization_type = "nf4";  // Default fallback
}
```

### Memory Estimation Mismatch

**Problem**: Actual memory usage differs from estimation

**Explanation**: Estimation includes only model weights, not:
- Optimizer states (can be 2x model size)
- Activations (depends on batch size)
- Gradients (equal to trainable parameters)

**Solution**: Add buffer to estimates

```cpp
// More realistic memory estimation
size_t model_memory = estimate_memory_usage(...);
size_t optimizer_memory = model_memory * 2;  // Adam states
size_t activation_memory = batch_size * seq_length * hidden_dim * sizeof(float) * num_layers;
size_t total_estimated = model_memory + optimizer_memory + activation_memory;

// Add 20% safety buffer
total_estimated = static_cast<size_t>(total_estimated * 1.2f);
```

## Examples

See:
- `tests/test_quantization.cpp` - Quantization tests and examples
- `tests/test_qlora.cpp` - QLoRA layer and model tests
- `tests/test_qlora_training_integration.cpp` - End-to-end training service integration tests
- Training service API examples above

## Best Practices

1. **Start with defaults**: NF4, block_size=64, double_quantization=true
2. **Monitor memory**: Use callbacks to track memory usage during training
3. **Validate accuracy**: Compare QLoRA results with full LoRA on small dataset
4. **Tune hyperparameters**: Adjust learning rate and rank based on task
5. **Use checkpointing**: Save adapters regularly to prevent data loss
6. **Test configurations**: Try both NF4 and INT8 to find best trade-off

## References

1. **QLoRA Paper**: https://arxiv.org/abs/2305.14314
2. **LoRA Paper**: https://arxiv.org/abs/2106.09685
3. **bitsandbytes**: https://github.com/TimDettmers/bitsandbytes
4. **GPTQ**: https://arxiv.org/abs/2210.17323
5. **llama.cpp quantization**: https://github.com/ggerganov/llama.cpp

## License

MIT License - See LICENSE file for details
