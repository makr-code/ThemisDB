/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lr_scheduler.h                                     ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:39:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     393                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        LRSchedulerConfig config;
        if (j.contains("type")) config.type = static_cast<SchedulerType>(j["type"].get<int>());
        if (j.contains("base_lr")) config.base_lr = j["base_lr"];
        if (j.contains("min_lr")) config.min_lr = j["min_lr"];
        if (j.contains("max_lr")) config.max_lr = j["max_lr"];
        if (j.contains("warmup_steps")) config.warmup_steps = j["warmup_steps"];
        if (j.contains("total_steps")) config.total_steps = j["total_steps"];
        if (j.contains("decay_power")) config.decay_power = j["decay_power"];
        if (j.contains("step_size")) config.step_size = j["step_size"];
        if (j.contains("gamma")) config.gamma = j["gamma"];
        if (j.contains("num_cycles")) config.num_cycles = j["num_cycles"];
        if (j.contains("step_size_up")) config.step_size_up = j["step_size_up"];
        if (j.contains("step_size_down")) config.step_size_down = j["step_size_down"];
        if (j.contains("pct_start")) config.pct_start = j["pct_start"];
        if (j.contains("final_div_factor")) config.final_div_factor = j["final_div_factor"];
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
    
    float get_lr(int /*step*/) const override { return lr_; }
    SchedulerType type() const override { return SchedulerType::CONSTANT; }
    LRSchedulerConfig config() const override {
        LRSchedulerConfig cfg;
        cfg.type = SchedulerType::CONSTANT;
        cfg.base_lr = lr_;
        return cfg;
    }

private:
    float lr_;
};

/**
 * @brief Linear decay scheduler
 */
class LinearLR : public LRScheduler {
public:
    LinearLR(float start_lr, float end_lr, int total_steps)
        : start_lr_(start_lr), end_lr_(end_lr), total_steps_(total_steps) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::LINEAR; }
    LRSchedulerConfig config() const override;

private:
    float start_lr_;
    float end_lr_;
    int total_steps_;
};

/**
 * @brief Cosine annealing scheduler
 */
class CosineAnnealingLR : public LRScheduler {
public:
    CosineAnnealingLR(float max_lr, float min_lr, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), total_steps_(total_steps) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::COSINE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_;
    float min_lr_;
    int total_steps_;
};

/**
 * @brief Cosine annealing with warm restarts
 */
class CosineAnnealingWarmRestartsLR : public LRScheduler {
public:
    CosineAnnealingWarmRestartsLR(float max_lr, float min_lr, int period, int num_cycles = 1)
        : max_lr_(max_lr), min_lr_(min_lr), period_(period), num_cycles_(num_cycles) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::COSINE_WITH_RESTARTS; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_;
    float min_lr_;
    int period_;
    int num_cycles_;
};

/**
 * @brief Polynomial decay scheduler
 */
class PolynomialLR : public LRScheduler {
public:
    PolynomialLR(float start_lr, float end_lr, int total_steps, float power = 1.0f)
        : start_lr_(start_lr), end_lr_(end_lr), total_steps_(total_steps), power_(power) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::POLYNOMIAL; }
    LRSchedulerConfig config() const override;

private:
    float start_lr_;
    float end_lr_;
    int total_steps_;
    float power_;
};

/**
 * @brief Step decay scheduler
 */
class StepLR : public LRScheduler {
public:
    StepLR(float initial_lr, int step_size, float gamma = 0.1f)
        : initial_lr_(initial_lr), step_size_(step_size), gamma_(gamma) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::STEP; }
    LRSchedulerConfig config() const override;

private:
    float initial_lr_;
    int step_size_;
    float gamma_;
};

/**
 * @brief Exponential decay scheduler
 */
class ExponentialLR : public LRScheduler {
public:
    ExponentialLR(float initial_lr, float gamma = 0.95f)
        : initial_lr_(initial_lr), gamma_(gamma) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::EXPONENTIAL; }
    LRSchedulerConfig config() const override;

private:
    float initial_lr_;
    float gamma_;
};

/**
 * @brief Warmup with constant learning rate
 */
class WarmupConstantLR : public LRScheduler {
public:
    WarmupConstantLR(float target_lr, int warmup_steps)
        : target_lr_(target_lr), warmup_steps_(warmup_steps) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_CONSTANT; }
    LRSchedulerConfig config() const override;

private:
    float target_lr_;
    int warmup_steps_;
};

/**
 * @brief Warmup with cosine annealing
 */
class WarmupCosineLR : public LRScheduler {
public:
    WarmupCosineLR(float max_lr, float min_lr, int warmup_steps, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), 
          warmup_steps_(warmup_steps), total_steps_(total_steps) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_COSINE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_;
    float min_lr_;
    int warmup_steps_;
    int total_steps_;

};

/**
 * @brief Cyclic learning rate scheduler (triangular)
 */
class CyclicLR : public LRScheduler {
public:
    CyclicLR(float base_lr, float max_lr, int step_size_up, int step_size_down)
        : base_lr_(base_lr), max_lr_(max_lr), step_size_up_(step_size_up), step_size_down_(step_size_down) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::CYCLIC; }
    LRSchedulerConfig config() const override;

private:
    float base_lr_;
    float max_lr_;
    int step_size_up_;
    int step_size_down_;
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

    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::ONE_CYCLE; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_;
    float base_lr_;
    float final_div_factor_;
    int total_steps_;
    float pct_start_;
};

/**
 * @brief Warmup with linear decay
 */
class WarmupLinearLR : public LRScheduler {
public:
    WarmupLinearLR(float max_lr, float min_lr, int warmup_steps, int total_steps)
        : max_lr_(max_lr), min_lr_(min_lr), 
          warmup_steps_(warmup_steps), total_steps_(total_steps) {}
    
    float get_lr(int step) const override;
    SchedulerType type() const override { return SchedulerType::WARMUP_LINEAR; }
    LRSchedulerConfig config() const override;

private:
    float max_lr_;
    float min_lr_;
    int warmup_steps_;
    int total_steps_;
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
