---
name: "RoPE Enhancement: Learned Rotation Parameters"
about: Implement trainable theta values for domain-specific optimization
title: '[RoPE] Learned Rotation Parameters for Domain Adaptation'
labels: 'enhancement, priority:P3, area:vector-index, component:rotary-embeddings, effort:large, research'
assignees: ''
---

## Feature Description

Enable learning and fine-tuning of RoPE rotation parameters (theta values) for domain-specific optimization. Instead of using fixed theta values from the RoFormer paper, this feature allows adapting rotation frequencies to specific data distributions and use cases.

## Problem Statement

Current RoPE implementation uses fixed theta values: `θᵢ = base^(-2i/d)` with base=10000. While effective for general NLP tasks, these values may be suboptimal for:
- Domain-specific embeddings (medical, legal, scientific)
- Time-series data with different temporal patterns
- Knowledge graphs with specific relational structures
- Custom embedding models with unique characteristics

## Proposed Solution

### Core Components

1. **Learnable Theta Layer** (`include/index/learnable_rope.h`)
   ```cpp
   class LearnableRotaryEmbedding : public RotaryEmbedding {
   public:
       // Initialize with learnable parameters
       LearnableRotaryEmbedding(const RotationConfig& config, bool trainable = true);
       
       // Training interface
       void setTrainingMode(bool training);
       std::vector<float> computeGradients(
           const std::vector<float>& embedding,
           const std::vector<float>& target,
           size_t position
       ) const;
       
       void updateParameters(
           const std::vector<float>& gradients,
           float learning_rate
       );
       
       // Serialization for trained parameters
       void saveParameters(const std::string& path) const;
       void loadParameters(const std::string& path);
       
   private:
       std::vector<float> learnable_theta_;  // Trainable theta values
       std::vector<float> theta_gradients_;  // Accumulated gradients
       bool training_mode_ = false;
   };
   ```

2. **Training Pipeline** (`src/index/rope_trainer.cpp`)
   - Contrastive learning objective for positional encoding
   - Gradient computation and backpropagation
   - Mini-batch training with Adam optimizer
   - Early stopping and validation

3. **Integration with Vector Index**
   ```cpp
   // VectorIndexManager extension
   Status trainRotaryEmbeddings(
       const std::vector<TrainingSample>& samples,
       const TrainingConfig& config
   );
   
   struct TrainingSample {
       std::vector<float> embedding;
       size_t position;
       float similarity_target;  // Target similarity for contrastive loss
   };
   ```

## Technical Design

### Training Objective

**Contrastive Loss** (inspired by SimCLR):
```
L = -log(exp(sim(rot(x_i, p_i), rot(x_j, p_j)) / τ) / 
         Σ_k exp(sim(rot(x_i, p_i), rot(x_k, p_k)) / τ))
```

where:
- `rot(x, p)` = rotated embedding at position p
- `sim(a, b)` = cosine similarity
- `τ` = temperature parameter

**Gradient w.r.t. theta:**
```
∂L/∂θᵢ = ∂L/∂rot * ∂rot/∂θᵢ
```

### Initialization Strategies

1. **From Paper**: Initialize with RoFormer values, then fine-tune
2. **Random**: Initialize randomly within reasonable range
3. **Pre-trained**: Load from checkpoint trained on similar domain

### Hyperparameters

```cpp
struct TrainingConfig {
    float learning_rate = 1e-4;
    size_t batch_size = 256;
    size_t max_epochs = 100;
    float temperature = 0.07;
    float weight_decay = 1e-5;
    size_t validation_split = 0.1;
    bool use_adam = true;
};
```

## Implementation Considerations

### Dependencies
- Eigen or similar linear algebra library for gradient computation
- Optional: Integration with PyTorch/TensorFlow for advanced training
- Checkpoint format: HDF5 or custom binary format

### Storage Format
```json
{
    "version": "1.0",
    "hidden_dim": 768,
    "num_rotation_pairs": 384,
    "base_theta": 10000.0,
    "learnable_theta": [1.0, 0.9995, 0.9990, ...],
    "training_history": {
        "epochs": 50,
        "final_loss": 0.123,
        "validation_accuracy": 0.95
    }
}
```

### Training Pipeline
1. **Data Collection**: Gather embeddings with known positional relationships
2. **Training**: Optimize theta values using contrastive loss
3. **Validation**: Test on held-out data
4. **Export**: Save trained parameters to file
5. **Deployment**: Load in production VectorIndexManager

## Performance Targets

| Metric | Before (Fixed) | After (Learned) | Improvement |
|--------|----------------|-----------------|-------------|
| Positional Accuracy | 85% | 95% | +10% |
| Downstream Task Accuracy | 88% | 93% | +5% |
| Training Time | N/A | 2-4 hours | - |

**Benefits:**
- Better positional encoding for domain-specific data
- Improved semantic similarity with position awareness
- Adaptability to different temporal patterns

## Use Cases

1. **Medical Document Retrieval**: Learn theta values optimized for clinical note sequencing
2. **Legal Document Analysis**: Adapt rotation for case law chronology
3. **Scientific Paper Search**: Optimize for citation temporal patterns
4. **Time-Series Forecasting**: Learn domain-specific temporal frequencies

## Example Usage

```cpp
// Initialize with learnable parameters
LearnableRotaryEmbedding learnable_rope(config, /*trainable=*/true);

// Training phase
learnable_rope.setTrainingMode(true);
TrainingConfig train_config;
train_config.learning_rate = 1e-4;
train_config.max_epochs = 50;

std::vector<TrainingSample> training_data = loadTrainingData();
learnable_rope.train(training_data, train_config);

// Save trained parameters
learnable_rope.saveParameters("trained_rope_medical.bin");

// Production use
learnable_rope.setTrainingMode(false);
auto rotated = learnable_rope.rotate(embedding, position);
```

## Alternative Solutions

1. **Hyperparameter Tuning**: Grid search over base_theta values (simpler but less flexible)
2. **Multi-Scale RoPE**: Use multiple fixed theta scales (no training needed)
3. **Adaptive RoPE**: Dynamically adjust theta based on data statistics

## Related Features

- LoRA Training Framework ([#existing_issue])
- Embedding Fine-Tuning ([#existing_issue])
- Knowledge Distillation ([#existing_issue])

## Additional Context

**References:**
- RoFormer Paper: https://arxiv.org/abs/2104.09864
- SimCLR Contrastive Learning: https://arxiv.org/abs/2002.05709
- Learned Positional Encodings: https://arxiv.org/abs/1706.03762

**Scientific Justification:**
- Transformer position encodings benefit from task-specific tuning
- Domain adaptation improves downstream task performance by 5-10%
- Learned parameters can capture domain-specific temporal patterns

**Priority:** P3 (Nice to Have) - Research/experimental feature  
**Effort:** 4-6 weeks  
**Complexity:** High (requires ML expertise)

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have clearly described the problem this feature solves
- [ ] I have provided a detailed description of the proposed solution
- [ ] I have considered the impact on existing functionality
- [ ] I have identified performance targets and evaluation metrics
- [ ] I have reviewed related research and alternatives
