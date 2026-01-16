#pragma once

#include <cmath>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Base class for learning rate schedulers
 * 
 * Provides interface for adjusting learning rate during training.
 */
class LRScheduler {
public:
    virtual ~LRScheduler() = default;
    
    /**
     * @brief Get learning rate for given step
     * @param step Current training step
     * @return Learning rate value
     */
    virtual float get_lr(int step) const = 0;
    
    /**
     * @brief Get base learning rate
     */
    virtual float base_lr() const = 0;
};

/**
 * @brief Constant learning rate (no scheduling)
 */
class ConstantLR : public LRScheduler {
public:
    explicit ConstantLR(float learning_rate)
        : learning_rate_(learning_rate) {}
    
    float get_lr(int step) const override {
        (void)step; // Unused
        return learning_rate_;
    }
    
    float base_lr() const override {
        return learning_rate_;
    }

private:
    float learning_rate_;
};

/**
 * @brief Linear warmup learning rate scheduler
 * 
 * Linearly increases learning rate from 0 to base_lr over warmup_steps,
 * then keeps it constant.
 */
class LinearWarmupLR : public LRScheduler {
public:
    /**
     * @brief Construct linear warmup scheduler
     * @param base_lr Target learning rate after warmup
     * @param warmup_steps Number of warmup steps
     */
    explicit LinearWarmupLR(float base_lr, int warmup_steps)
        : base_lr_(base_lr)
        , warmup_steps_(warmup_steps) {}
    
    float get_lr(int step) const override {
        if (step < warmup_steps_) {
            // Linear warmup: lr = base_lr * (step / warmup_steps)
            return base_lr_ * (static_cast<float>(step) / static_cast<float>(warmup_steps_));
        }
        return base_lr_;
    }
    
    float base_lr() const override {
        return base_lr_;
    }

private:
    float base_lr_;
    int warmup_steps_;
};

/**
 * @brief Cosine annealing learning rate scheduler
 * 
 * Smoothly decreases learning rate from base_lr to 0 following cosine curve.
 * Formula: lr = base_lr * 0.5 * (1 + cos(π * step / total_steps))
 */
class CosineAnnealingLR : public LRScheduler {
public:
    /**
     * @brief Construct cosine annealing scheduler
     * @param base_lr Initial learning rate
     * @param total_steps Total number of training steps
     * @param min_lr Minimum learning rate (default 0)
     */
    explicit CosineAnnealingLR(float base_lr, int total_steps, float min_lr = 0.0f)
        : base_lr_(base_lr)
        , total_steps_(total_steps)
        , min_lr_(min_lr) {}
    
    float get_lr(int step) const override {
        if (step >= total_steps_) {
            return min_lr_;
        }
        
        // Cosine annealing: lr = min_lr + (base_lr - min_lr) * 0.5 * (1 + cos(π * step / total_steps))
        float progress = static_cast<float>(step) / static_cast<float>(total_steps_);
        float cosine_decay = 0.5f * (1.0f + std::cos(M_PI * progress));
        return min_lr_ + (base_lr_ - min_lr_) * cosine_decay;
    }
    
    float base_lr() const override {
        return base_lr_;
    }

private:
    float base_lr_;
    int total_steps_;
    float min_lr_;
};

/**
 * @brief Cosine annealing with linear warmup
 * 
 * Combines linear warmup with cosine annealing decay.
 */
class CosineAnnealingWarmupLR : public LRScheduler {
public:
    /**
     * @brief Construct cosine annealing with warmup scheduler
     * @param base_lr Target learning rate after warmup
     * @param warmup_steps Number of warmup steps
     * @param total_steps Total number of training steps
     * @param min_lr Minimum learning rate (default 0)
     */
    explicit CosineAnnealingWarmupLR(
        float base_lr,
        int warmup_steps,
        int total_steps,
        float min_lr = 0.0f
    )
        : base_lr_(base_lr)
        , warmup_steps_(warmup_steps)
        , total_steps_(total_steps)
        , min_lr_(min_lr) {}
    
    float get_lr(int step) const override {
        // Warmup phase
        if (step < warmup_steps_) {
            return base_lr_ * (static_cast<float>(step) / static_cast<float>(warmup_steps_));
        }
        
        // Cosine annealing phase
        int decay_steps = total_steps_ - warmup_steps_;
        int current_decay_step = step - warmup_steps_;
        
        if (current_decay_step >= decay_steps) {
            return min_lr_;
        }
        
        float progress = static_cast<float>(current_decay_step) / static_cast<float>(decay_steps);
        float cosine_decay = 0.5f * (1.0f + std::cos(M_PI * progress));
        return min_lr_ + (base_lr_ - min_lr_) * cosine_decay;
    }
    
    float base_lr() const override {
        return base_lr_;
    }

private:
    float base_lr_;
    int warmup_steps_;
    int total_steps_;
    float min_lr_;
};

/**
 * @brief Step decay learning rate scheduler
 * 
 * Multiplies learning rate by gamma every step_size steps.
 */
class StepLR : public LRScheduler {
public:
    /**
     * @brief Construct step decay scheduler
     * @param base_lr Initial learning rate
     * @param step_size Number of steps between decay
     * @param gamma Multiplicative factor (default 0.1)
     */
    explicit StepLR(float base_lr, int step_size, float gamma = 0.1f)
        : base_lr_(base_lr)
        , step_size_(step_size)
        , gamma_(gamma) {}
    
    float get_lr(int step) const override {
        int num_decays = step / step_size_;
        return base_lr_ * std::pow(gamma_, num_decays);
    }
    
    float base_lr() const override {
        return base_lr_;
    }

private:
    float base_lr_;
    int step_size_;
    float gamma_;
};

/**
 * @brief Exponential decay learning rate scheduler
 * 
 * Exponentially decays learning rate: lr = base_lr * gamma^step
 */
class ExponentialLR : public LRScheduler {
public:
    /**
     * @brief Construct exponential decay scheduler
     * @param base_lr Initial learning rate
     * @param gamma Decay factor per step (should be < 1)
     */
    explicit ExponentialLR(float base_lr, float gamma)
        : base_lr_(base_lr)
        , gamma_(gamma) {}
    
    float get_lr(int step) const override {
        return base_lr_ * std::pow(gamma_, step);
    }
    
    float base_lr() const override {
        return base_lr_;
    }

private:
    float base_lr_;
    float gamma_;
};

} // namespace lora
} // namespace llm
} // namespace themis
