/**
 * @file lr_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Learning rate scheduler type
 */
enum class SchedulerType {
    CONSTANT,               // Constant learning rate
    LINEAR,                 // Linear decay
    COSINE,                 // Cosine annealing
    COSINE_WITH_RESTARTS,   // Cosine with warm restarts
    POLYNOMIAL,             // Polynomial decay
    STEP,                   // Step decay
    EXPONENTIAL,            // Exponential decay
    WARMUP_CONSTANT,        // Linear warmup then constant
    WARMUP_COSINE,          // Linear warmup then cosine
    WARMUP_LINEAR,          // Linear warmup then linear decay
    CYCLIC,                 // Triangular / cyclic schedule
    ONE_CYCLE               // OneCycle policy (Smith 2018)
};

/**
 * @brief Configuration for learning rate scheduler
 */
struct LRSchedulerConfig {
    virtual ~LRSchedulerConfig() = default;
    SchedulerType type = SchedulerType::CONSTANT;
    float base_lr = 1e-4f;              // Base learning rate
    float min_lr = 1e-6f;               // Minimum learning rate
    float max_lr = 1e-3f;               // Maximum learning rate (for warmup)
    int warmup_steps = 0;               // Number of warmup steps
    int total_steps = 1000;             // Total training steps
    float decay_power = 1.0f;           // Power for polynomial decay
    float step_size = 100;              // Step size for step decay
    float gamma = 0.1f;                 // Multiplicative factor for step/exp decay
    int num_cycles = 1;                 // Number of cycles for cosine with restarts
    int step_size_up = 200;             // Steps for increasing LR (cyclic/one-cycle)
    int step_size_down = 200;           // Steps for decreasing LR (cyclic/one-cycle)
    float pct_start = 0.3f;             // Fraction of steps to increase LR (one-cycle)
    float final_div_factor = 25.0f;     // Final LR = max_lr/final_div_factor (one-cycle)
    
    json toJSON() const {
        return json{
            {"type", static_cast<int>(type)},
            {"base_lr", base_lr},
            {"min_lr", min_lr},
            {"max_lr", max_lr},
            {"warmup_steps", warmup_steps},
            {"total_steps", total_steps},
            {"decay_power", decay_power},
            {"step_size", step_size},
            {"gamma", gamma},
            {"num_cycles", num_cycles},
            {"step_size_up", step_size_up},
            {"step_size_down", step_size_down},
            {"pct_start", pct_start},
            {"final_div_factor", final_div_factor}
        };
    }
    
    static LRSchedulerConfig fromJSON(const json& j) {
        LRSchedulerConfig config = {};
        if (j.contains("type")) {
          config.type = static_cast<SchedulerType>(j["type"].get<int>());
        }
        if (j.contains("base_lr")) {
          config.base_lr = j["base_lr"];
        }
        if (j.contains("min_lr")) {
          config.min_lr = j["min_lr"];
        }
        if (j.contains("max_lr")) {
          config.max_lr = j["max_lr"];
        }
        if (j.contains("warmup_steps")) {
          config.warmup_steps = j["warmup_steps"];
        }
        if (j.contains("total_steps")) {
          config.total_steps = j["total_steps"];
        }
        if (j.contains("decay_power")) {
          config.decay_power = j["decay_power"];
        }
        if (j.contains("step_size")) {
          config.step_size = j["step_size"];
        }
        if (j.contains("gamma")) {
          config.gamma = j["gamma"];
        }
        if (j.contains("num_cycles")) {
          config.num_cycles = j["num_cycles"];
        }
        if (j.contains("step_size_up")) {
          config.step_size_up = j["step_size_up"];
        }
        if (j.contains("step_size_down")) {
          config.step_size_down = j["step_size_down"];
        }
        if (j.contains("pct_start")) {
          config.pct_start = j["pct_start"];
        }
        if (j.contains("final_div_factor")) {
          config.final_div_factor = j["final_div_factor"];
        }
        return config;
    }
};

/**
 * @brief Base class for learning rate schedulers
 */
class LRScheduler {
public:
    virtual ~LRScheduler() = default;
    
    /**
     * @brief Get learning rate for current step
     * @param step Current training step
     * @return Learning rate
     */
    virtual float get_lr(int step) const = 0;
    
    /**
     * @brief Get scheduler type
     * @return Scheduler type
     */
    virtual SchedulerType type() const = 0;
    
    /**
     * @brief Get configuration
     * @return Scheduler configuration
     */
    virtual LRSchedulerConfig config() const = 0;
};

/**
 * @brief Constant learning rate scheduler
 */
class ConstantLR : public LRScheduler {
public:
    explicit ConstantLR(float lr) : lr_(lr) {}
    ~ConstantLR() override = default;

    float get_lr(int /*step*/) const override { return lr_; }
    SchedulerType type() const override { return SchedulerType::CONSTANT; }
    LRSchedulerConfig config() const override {
        LRSchedulerConfig cfg;
        cfg.type = SchedulerType::CONSTANT;
        cfg.base_lr = lr_;
        return cfg;
    }

private:
    float lr_ = 0.0f;
};

/**
 * @brief Linear decay scheduler
 */
class LinearLR : public LRScheduler {
public:
    LinearLR(float start_lr, float end_lr, int total_steps)
        : start_lr_(start_lr), end_lr_(end_lr), total_steps_(total_steps) {}
    ~LinearLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::LINEAR; }
    LRSchedulerConfig config() const override;

private:
    float start_lr_ = 0.0f;
    float end_lr_ = 0.0f;
    int total_steps_ = 0;
};

/**
 * @brief Cosine annealing scheduler
 */
class CosineAnnealingLR : public LRScheduler {
public:
    CosineAnnealingLR(float max_lr, float min_lr, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), total_steps_(total_steps) {}
    ~CosineAnnealingLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::COSINE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_ = 0.0f;
    float min_lr_ = 0.0f;
    int total_steps_ = 0;
};

/**
 * @brief Cosine annealing with warm restarts
 */
class CosineAnnealingWarmRestartsLR : public LRScheduler {
public:
    CosineAnnealingWarmRestartsLR(float max_lr, float min_lr, int period, int num_cycles = 1)
        : max_lr_(max_lr), min_lr_(min_lr), period_(period), num_cycles_(num_cycles) {}
    ~CosineAnnealingWarmRestartsLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::COSINE_WITH_RESTARTS; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_ = 0.0f;
    float min_lr_ = 0.0f;
    int period_ = 0;
    int num_cycles_ = 0;
};

/**
 * @brief Polynomial decay scheduler
 */
class PolynomialLR : public LRScheduler {
public:
    PolynomialLR(float start_lr, float end_lr, int total_steps, float power = 1.0f)
        : start_lr_(start_lr), end_lr_(end_lr), total_steps_(total_steps), power_(power) {}
    ~PolynomialLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::POLYNOMIAL; }
    LRSchedulerConfig config() const override;

private:
    float start_lr_ = 0.0f;
    float end_lr_ = 0.0f;
    int total_steps_ = 0;
    float power_ = 0.0f;
};

/**
 * @brief Step decay scheduler
 */
class StepLR : public LRScheduler {
public:
    StepLR(float initial_lr, int step_size, float gamma = 0.1f)
        : initial_lr_(initial_lr), step_size_(step_size), gamma_(gamma) {}
    ~StepLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::STEP; }
    LRSchedulerConfig config() const override;

private:
    float initial_lr_ = 0.0f;
    int step_size_ = 0;
    float gamma_ = 0.0f;
};

/**
 * @brief Exponential decay scheduler
 */
class ExponentialLR : public LRScheduler {
public:
    ExponentialLR(float initial_lr, float gamma = 0.95f)
        : initial_lr_(initial_lr), gamma_(gamma) {}
    ~ExponentialLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::EXPONENTIAL; }
    LRSchedulerConfig config() const override;

private:
    float initial_lr_ = 0.0f;
    float gamma_ = 0.0f;
};

/**
 * @brief Warmup with constant learning rate
 */
class WarmupConstantLR : public LRScheduler {
public:
    WarmupConstantLR(float target_lr, int warmup_steps)
        : target_lr_(target_lr), warmup_steps_(warmup_steps) {}
    ~WarmupConstantLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_CONSTANT; }
    LRSchedulerConfig config() const override;

private:
    float target_lr_ = 0.0f;
    int warmup_steps_ = 0;
};

/**
 * @brief Warmup with cosine annealing
 */
class WarmupCosineLR : public LRScheduler {
public:
    WarmupCosineLR(float max_lr, float min_lr, int warmup_steps, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), 
          warmup_steps_(warmup_steps), total_steps_(total_steps) {}
    ~WarmupCosineLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_COSINE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_ = 0.0f;
    float min_lr_ = 0.0f;
    int warmup_steps_ = 0;
    int total_steps_ = 0;

};

/**
 * @brief Cyclic learning rate scheduler (triangular)
 */
class CyclicLR : public LRScheduler {
public:
    CyclicLR(float base_lr, float max_lr, int step_size_up, int step_size_down)
        : base_lr_(base_lr), max_lr_(max_lr), step_size_up_(step_size_up), step_size_down_(step_size_down) {}
    ~CyclicLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::CYCLIC; }
    LRSchedulerConfig config() const override;

private:
    float base_lr_ = 0.0f;
    float max_lr_ = 0.0f;
    int step_size_up_ = 0;
    int step_size_down_ = 0;
};

/**
 * @brief OneCycle learning rate scheduler
 */
class OneCycleLR : public LRScheduler {
public:
    OneCycleLR(float max_lr, float base_lr, float final_div_factor,
               int total_steps, float pct_start)
        : max_lr_(max_lr), base_lr_(base_lr), final_div_factor_(final_div_factor),
          total_steps_(total_steps), pct_start_(pct_start) {}
    ~OneCycleLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::ONE_CYCLE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_ = 0.0f;
    float base_lr_ = 0.0f;
    float final_div_factor_ = 0.0f;
    int total_steps_ = 0;
    float pct_start_ = 0.0f;
};

/**
 * @brief Warmup with linear decay
 */
class WarmupLinearLR : public LRScheduler {
public:
    WarmupLinearLR(float max_lr, float min_lr, int warmup_steps, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), 
          warmup_steps_(warmup_steps), total_steps_(total_steps) {}
    ~WarmupLinearLR() override = default;

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_LINEAR; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_ = 0.0f;
    float min_lr_ = 0.0f;
    int warmup_steps_ = 0;
    int total_steps_ = 0;
};

/**
 * @brief Factory for creating learning rate schedulers
 */
class LRSchedulerFactory {
public:
    /**
     * @brief Create scheduler from configuration
     * @param config Scheduler configuration
     * @return Unique pointer to scheduler
     */
    static std::unique_ptr<LRScheduler> create(const LRSchedulerConfig& config);
    
    /**
     * @brief Create common scheduler presets
     */
    static std::unique_ptr<LRScheduler> createConstant(float lr);
    static std::unique_ptr<LRScheduler> createLinearDecay(float start_lr, float end_lr, int steps);
    static std::unique_ptr<LRScheduler> createCosineAnnealing(float max_lr, float min_lr, int steps);
    static std::unique_ptr<LRScheduler> createWarmupCosine(float max_lr, float min_lr, 
                                                            int warmup_steps, int total_steps);
};

} // namespace lora
} // namespace llm
} // namespace themis

