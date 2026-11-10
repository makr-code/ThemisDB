/**
 * @file adalora_strategy_config.h
 * @brief Typed loader for config/lora/adalora_optimization_strategy.yaml.
 *
 * Maps the `adalora.rank_budget`, `adalora.importance_pruning`,
 * `adalora.hyperparameters` and `adalora.target_modules` sections onto the
 * training contract types AdaLoRAScheduleParams and LoRAHyperparameters
 * (include/llm/lora_framework/lora_training_backend.h). Other sections of the
 * file (multi_adapter, adapter_merger, slo_gates, memory, storage, federation,
 * database_optimizer_labeler) are owned by their respective subsystems and are
 * intentionally not interpreted here.
 *
 * Reference: Zhang et al., "AdaLoRA: Adaptive Budget Allocation for
 * Parameter-Efficient Fine-Tuning", ICLR 2023, arXiv:2303.10512.
 */
#pragma once

#include "llm/lora_framework/lora_training_backend.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Validated AdaLoRA optimisation strategy.
 *
 * Ownership: plain value type, safe to copy between threads.
 */
struct AdaLoRAStrategyConfig {
    /// AdaLoRA when `importance_pruning.enabled` is true, plain LoRA otherwise.
    AdapterTrainingMethod method = AdapterTrainingMethod::AdaLoRA;
    /// Budget schedule: init_rank/target_rank, t_i/t_f, ΔT, β1/β2, γ.
    AdaLoRAScheduleParams schedule;
    /// Optimiser/training defaults. `rank` = target rank, `alpha` = alpha_multiplier × target rank.
    LoRAHyperparameters hyperparameters;
    /// Effective batch = hyperparameters.batch_size × gradient_accumulation_steps.
    int gradient_accumulation_steps = 1;
    /// Hard per-module rank cap (`rank_budget.max_rank`); 0 = no cap configured.
    int max_rank = 0;
    /// Non-fatal diagnostics (e.g. init_rank deviating from the paper's ≈1.5× target).
    std::vector<std::string> warnings;
    /// Source path or label used in diagnostics.
    std::string source;
};

/**
 * @brief Thrown when the strategy file is missing, malformed or out of range.
 *
 * what() lists every validation error found (separated by "; "), prefixed with the source.
 */
class AdaLoRAStrategyConfigError : public std::runtime_error {
public:
    /**
     * @brief Construct with the aggregated error list.
     * @param[in] source Path or label of the offending document.
     * @param[in] errors Individual validation errors (at least one).
     */
    AdaLoRAStrategyConfigError(const std::string& source, std::vector<std::string> errors);

    /// @brief Individual validation errors.
    [[nodiscard]] const std::vector<std::string>& errors() const noexcept { return errors_; }

private:
    std::vector<std::string> errors_;
};

/**
 * @brief Load and validate an AdaLoRA optimisation-strategy YAML file.
 *
 * Validation (all violations are collected before throwing):
 *  - `rank_budget.initial_rank` ≥ `rank_budget.target_rank` ≥ 1 and, if set,
 *    `initial_rank` ≤ `max_rank`;
 *  - `importance_pruning.warmup_steps` (t_i) ≥ 0, `final_steps` (t_f) ≥ 0,
 *    `pruning_interval_steps` (ΔT) ≥ 1;
 *  - 0 < `beta1`, `beta2` < 1 (sensitivity/uncertainty EMA);
 *  - `orth_reg_weight` (γ) ≥ 0;
 *  - `metric` must be "gradient_sensitivity" (the |w·∇w| score of the paper);
 *  - hyperparameters: alpha_multiplier > 0, 0 ≤ dropout < 1, learning_rate > 0,
 *    batch_size/gradient_accumulation_steps/max_seq_length/num_epochs ≥ 1,
 *    optimizer ∈ {sgd, adam, adamw}, 0 ≤ Adam β < 1, eps > 0, weight_decay ≥ 0,
 *    lr_scheduler ∈ {constant, linear_warmup, cosine, cosine_warmup, step, exponential},
 *    lr_scheduler.warmup_steps ≥ 0;
 *  - `target_modules` non-empty list of non-empty strings.
 * Missing optional keys keep the defaults of AdaLoRAScheduleParams / LoRAHyperparameters.
 *
 * @param[in] path Filesystem path of the YAML document.
 * @return Validated strategy.
 * @throws AdaLoRAStrategyConfigError on unreadable/malformed input or any range violation.
 */
[[nodiscard]] AdaLoRAStrategyConfig loadAdaLoRAStrategy(const std::string& path);

/**
 * @brief Same as loadAdaLoRAStrategy() but parses YAML text already in memory.
 * @param[in] yaml_text YAML document.
 * @param[in] source_label Label used in diagnostics (e.g. "<inline>").
 * @return Validated strategy.
 * @throws AdaLoRAStrategyConfigError on malformed input or any range violation.
 */
[[nodiscard]] AdaLoRAStrategyConfig loadAdaLoRAStrategyFromString(const std::string& yaml_text,
                                                                  const std::string& source_label = "<inline>");

}  // namespace lora
}  // namespace llm
}  // namespace themis
