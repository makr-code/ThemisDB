# Learnable RoPE Parameters

## Overview

The Learnable RoPE (Rotary Position Embedding) feature enables domain-specific optimization of positional encoding parameters through training. Instead of using fixed theta values from the RoFormer paper, this feature allows adapting rotation frequencies to specific data distributions and use cases.

## Features

- **Trainable Theta Parameters**: Learn optimal rotation frequencies for your domain
- **Multiple Optimizers**: Support for SGD and Adam optimizers
- **Early Stopping**: Automatic training termination based on validation loss
- **Parameter Persistence**: Save and load trained parameters in JSON format
- **Gradient Computation**: Automatic gradient calculation for parameter updates
- **Validation Support**: Built-in train/validation split for model evaluation

## Use Cases

1. **Medical Document Retrieval**: Optimize theta values for clinical note sequencing
2. **Legal Document Analysis**: Adapt rotation for case law chronology
3. **Scientific Paper Search**: Optimize for citation temporal patterns
4. **Time-Series Forecasting**: Learn domain-specific temporal frequencies

## Quick Start

### 1. Basic Setup

```cpp
#include "index/learnable_rope.h"

using namespace themis;

// Configure rotation parameters
RotationConfig config;
config.hidden_dim = 128;
config.num_rotation_pairs = 64;
config.base_theta = 10000.0;
config.computeThetaCache();

// Initialize learnable RoPE
LearnableRotaryEmbedding learnable_rope(config, /*trainable=*/true);
```

### 2. Prepare Training Data

```cpp
// Create training samples
std::vector<TrainingSample> training_samples;

for (size_t i = 0; i < num_documents; ++i) {
    std::vector<float> embedding = getEmbedding(documents[i]);
    size_t position = i;
    float similarity_target = computeTarget(documents[i]);
    
    training_samples.emplace_back(embedding, position, similarity_target);
}
```

### 3. Configure Training

```cpp
TrainingConfig train_config;
train_config.learning_rate = 1e-4f;      // Learning rate
train_config.batch_size = 256;           // Mini-batch size
train_config.max_epochs = 100;           // Maximum epochs
train_config.validation_split = 0.1f;    // 10% validation
train_config.use_adam = true;            // Use Adam optimizer
train_config.early_stop_patience = 10;   // Early stopping patience
```

### 4. Train the Model

```cpp
// Start training
std::vector<float> loss_history = learnable_rope.train(training_samples, train_config);

// Check training progress
std::cout << "Final loss: " << loss_history.back() << std::endl;
```

### 5. Save Trained Parameters

```cpp
// Save to file
learnable_rope.saveParameters("trained_rope_medical.json");
```

### 6. Use in Production

```cpp
// Load trained parameters
LearnableRotaryEmbedding production_rope(config, false);  // Not trainable
production_rope.loadParameters("trained_rope_medical.json");

// Use for inference
production_rope.setTrainingMode(false);
auto rotated = production_rope.rotate(embedding, position);
```

## API Reference

### LearnableRotaryEmbedding Class

#### Constructor

```cpp
LearnableRotaryEmbedding(const RotationConfig& config, bool trainable = true);
```

Creates a new learnable RoPE instance.

**Parameters:**
- `config`: Rotation configuration (must have theta_cache computed)
- `trainable`: If true, parameters can be trained; if false, parameters are frozen

#### Training Methods

##### train()

```cpp
std::vector<float> train(
    const std::vector<TrainingSample>& samples,
    const TrainingConfig& config
);
```

Trains the learnable theta parameters on a dataset using contrastive learning.

**Parameters:**
- `samples`: Training samples with embeddings, positions, and targets
- `config`: Training configuration

**Returns:** Loss history (one value per epoch)

##### computeGradients()

```cpp
std::vector<double> computeGradients(
    const std::vector<float>& embedding,
    float target_similarity,
    size_t position
);
```

Computes gradients for a single sample (used internally during training).

**Returns:** Gradients with respect to theta parameters

##### updateParameters()

```cpp
void updateParameters(
    const std::vector<double>& gradients,
    float learning_rate
);
```

Updates theta parameters using computed gradients.

#### Parameter Management

##### getLearnableTheta()

```cpp
const std::vector<double>& getLearnableTheta() const;
```

Returns current learnable theta values.

##### setLearnableTheta()

```cpp
void setLearnableTheta(const std::vector<double>& theta);
```

Sets learnable theta values (e.g., from loaded checkpoint).

##### resetToBase()

```cpp
void resetToBase();
```

Resets learnable theta to base configuration values.

#### Serialization

##### saveParameters()

```cpp
bool saveParameters(const std::string& path) const;
```

Saves trained parameters to file in JSON format.

**Returns:** True if successful, false otherwise

##### loadParameters()

```cpp
bool loadParameters(const std::string& path);
```

Loads trained parameters from file.

**Returns:** True if successful, false otherwise

#### Mode Control

##### setTrainingMode()

```cpp
void setTrainingMode(bool training);
```

Sets training mode (affects gradient computation).

##### isTraining()

```cpp
bool isTraining() const;
```

Checks if in training mode.

##### isTrainable()

```cpp
bool isTrainable() const;
```

Checks if parameters are trainable.

### TrainingConfig Structure

```cpp
struct TrainingConfig {
    float learning_rate = 1e-4f;     // Learning rate for gradient descent
    size_t batch_size = 256;         // Mini-batch size
    size_t max_epochs = 100;         // Maximum training epochs
    float temperature = 0.07f;       // Temperature for contrastive loss
    float weight_decay = 1e-5f;      // L2 regularization strength
    float validation_split = 0.1f;   // Fraction of data for validation
    bool use_adam = false;           // Use Adam optimizer (if true), else SGD
    float early_stop_patience = 10;  // Stop if no improvement for N epochs
    
    // Adam optimizer parameters (only used if use_adam = true)
    float adam_beta1 = 0.9f;
    float adam_beta2 = 0.999f;
    float adam_epsilon = 1e-8f;
};
```

### TrainingSample Structure

```cpp
struct TrainingSample {
    std::vector<float> embedding;     // Input embedding vector
    size_t position;                  // Position index
    float similarity_target;          // Target similarity for contrastive loss
};
```

## File Format

Trained parameters are saved in JSON format:

```json
{
  "version": "1.0",
  "hidden_dim": 768,
  "num_rotation_pairs": 384,
  "base_theta": 10000.0,
  "learnable_theta": [1.0, 0.9995, 0.9990, ...]
}
```

## Performance Considerations

### Training Time

- **Small datasets** (< 1000 samples): Minutes
- **Medium datasets** (1000-10000 samples): 1-2 hours
- **Large datasets** (> 10000 samples): 2-4 hours

Training time depends on:
- Dataset size
- Number of epochs
- Batch size
- Hidden dimension

### Memory Requirements

Memory usage scales with:
- Batch size × hidden dimension (for gradients)
- 2 × num_rotation_pairs (for Adam optimizer state)

For typical configurations (dim=768, batch=256):
- Base memory: ~10 MB
- With Adam: ~15 MB

### Optimization Tips

1. **Start with larger learning rates** (1e-3) and decrease if unstable
2. **Use validation split** to detect overfitting early
3. **Enable early stopping** to avoid unnecessary training
4. **Use Adam optimizer** for faster convergence
5. **Adjust batch size** based on available memory

## Best Practices

### Data Preparation

1. **Normalize embeddings**: Ensure embeddings have similar scales
2. **Balance positions**: Include samples from various positions
3. **Quality targets**: Provide meaningful similarity targets

### Training Strategy

1. **Start with base values**: Initialize from RoFormer defaults
2. **Use validation**: Always split data for validation
3. **Monitor loss**: Check for convergence or overfitting
4. **Save checkpoints**: Regularly save parameters during training

### Production Deployment

1. **Freeze parameters**: Set `trainable=false` in production
2. **Disable training mode**: Call `setTrainingMode(false)`
3. **Validate loaded parameters**: Verify theta values after loading
4. **Test on validation set**: Ensure performance before deployment

## Examples

See `examples/learnable_rope_example.cpp` for a complete working example.

## References

- RoFormer Paper: [https://arxiv.org/abs/2104.09864](https://arxiv.org/abs/2104.09864)
- SimCLR Contrastive Learning: [https://arxiv.org/abs/2002.05709](https://arxiv.org/abs/2002.05709)
- Learned Positional Encodings: [https://arxiv.org/abs/1706.03762](https://arxiv.org/abs/1706.03762)

## License

This feature is part of ThemisDB and follows the same license terms.
