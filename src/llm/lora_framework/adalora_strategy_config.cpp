/**
 * @file adalora_strategy_config.cpp
 * @brief Typed, validating loader for config/lora/adalora_optimization_strategy.yaml.
 */

#include "llm/lora_framework/adalora_strategy_config.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace themis {
namespace llm {
namespace lora {

namespace {

[[nodiscard]] std::string joinErrors(const std::string& source, const std::vector<std::string>& errors) {
    std::string msg = "Invalid AdaLoRA strategy '" + source + "': ";
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i != 0) {
            msg += "; ";
        }
        msg += errors[i];
    }
    return msg;
}

/// Collects errors while reading typed scalars from optional YAML keys.
class FieldReader {
public:
    explicit FieldReader(std::vector<std::string>& errors) : errors_(errors) {}

    template <typename T>
    void read(const YAML::Node& parent, const char* key, const std::string& path, T& target) {
        if (!parent || !parent.IsMap()) {
            return;
        }
        const YAML::Node node = parent[key];
        if (!node || node.IsNull()) {
            return;  // optional: keep default
        }
        if (!node.IsScalar()) {
            errors_.push_back(path + " must be a scalar");
            return;
        }
        try {
            target = node.as<T>();
        } catch (const YAML::Exception&) {
            errors_.push_back(path + " has an invalid value '" + node.Scalar() + "'");
        }
    }

private:
    std::vector<std::string>& errors_;
};

[[nodiscard]] bool isFiniteInOpenUnit(double v) { return std::isfinite(v) && v > 0.0 && v < 1.0; }

[[nodiscard]] AdaLoRAStrategyConfig parseStrategy(const YAML::Node& root, const std::string& source) {
    std::vector<std::string> errors;
    AdaLoRAStrategyConfig cfg;
    cfg.source = source;

    if (!root || !root.IsMap() || !root["adalora"] || !root["adalora"].IsMap()) {
        throw AdaLoRAStrategyConfigError(source, {"top-level mapping 'adalora' is missing"});
    }
    const YAML::Node ada = root["adalora"];
    FieldReader r(errors);

    // ── Rank budget: b(0) = initial_rank, b(T) = target_rank (per module) ───────
    const YAML::Node budget = ada["rank_budget"];
    r.read(budget, "initial_rank", "rank_budget.initial_rank", cfg.schedule.init_rank);
    r.read(budget, "target_rank", "rank_budget.target_rank", cfg.schedule.target_rank);
    r.read(budget, "max_rank", "rank_budget.max_rank", cfg.max_rank);

    // ── Importance pruning / budget schedule ──────────────────────────────────
    const YAML::Node pruning = ada["importance_pruning"];
    bool pruning_enabled = true;
    std::string metric = "gradient_sensitivity";
    r.read(pruning, "enabled", "importance_pruning.enabled", pruning_enabled);
    r.read(pruning, "metric", "importance_pruning.metric", metric);
    r.read(pruning, "warmup_steps", "importance_pruning.warmup_steps", cfg.schedule.warmup_steps);
    r.read(pruning, "final_steps", "importance_pruning.final_steps", cfg.schedule.final_steps);
    r.read(pruning, "pruning_interval_steps", "importance_pruning.pruning_interval_steps", cfg.schedule.delta_t);
    r.read(pruning, "beta1", "importance_pruning.beta1", cfg.schedule.beta1);
    r.read(pruning, "beta2", "importance_pruning.beta2", cfg.schedule.beta2);
    r.read(pruning, "orth_reg_weight", "importance_pruning.orth_reg_weight", cfg.schedule.orth_reg_weight);
    cfg.method = pruning_enabled ? AdapterTrainingMethod::AdaLoRA : AdapterTrainingMethod::LoRA;

    // ── LoRA / optimiser hyperparameters ──────────────────────────────────────
    const YAML::Node hp = ada["hyperparameters"];
    double alpha_multiplier = 2.0;
    double dropout = cfg.hyperparameters.dropout;
    double learning_rate = cfg.hyperparameters.learning_rate;
    r.read(hp, "alpha_multiplier", "hyperparameters.alpha_multiplier", alpha_multiplier);
    r.read(hp, "dropout", "hyperparameters.dropout", dropout);
    r.read(hp, "learning_rate", "hyperparameters.learning_rate", learning_rate);
    r.read(hp, "batch_size", "hyperparameters.batch_size", cfg.hyperparameters.batch_size);
    r.read(hp, "gradient_accumulation_steps", "hyperparameters.gradient_accumulation_steps",
           cfg.gradient_accumulation_steps);
    r.read(hp, "max_seq_length", "hyperparameters.max_seq_length", cfg.hyperparameters.max_seq_length);
    r.read(hp, "num_epochs", "hyperparameters.num_epochs", cfg.hyperparameters.num_epochs);

    const YAML::Node opt = hp ? hp["optimizer"] : YAML::Node();
    double adam_beta1 = cfg.hyperparameters.beta1;
    double adam_beta2 = cfg.hyperparameters.beta2;
    double adam_eps = cfg.hyperparameters.epsilon;
    double weight_decay = cfg.hyperparameters.weight_decay;
    r.read(opt, "type", "hyperparameters.optimizer.type", cfg.hyperparameters.optimizer);
    r.read(opt, "beta1", "hyperparameters.optimizer.beta1", adam_beta1);
    r.read(opt, "beta2", "hyperparameters.optimizer.beta2", adam_beta2);
    r.read(opt, "eps", "hyperparameters.optimizer.eps", adam_eps);
    r.read(opt, "weight_decay", "hyperparameters.optimizer.weight_decay", weight_decay);

    const YAML::Node sched = hp ? hp["lr_scheduler"] : YAML::Node();
    r.read(sched, "type", "hyperparameters.lr_scheduler.type", cfg.hyperparameters.lr_scheduler);
    r.read(sched, "warmup_steps", "hyperparameters.lr_scheduler.warmup_steps", cfg.hyperparameters.warmup_steps);

    // ── Target modules ────────────────────────────────────────────────────────
    if (const YAML::Node mods = ada["target_modules"]; mods) {
        if (!mods.IsSequence() || mods.size() == 0) {
            errors.emplace_back("target_modules must be a non-empty list");
        } else {
            std::vector<std::string> modules;
            for (const auto& m : mods) {
                if (!m.IsScalar() || m.Scalar().empty()) {
                    errors.emplace_back("target_modules entries must be non-empty strings");
                    break;
                }
                modules.push_back(m.Scalar());
            }
            if (!modules.empty()) {
                cfg.hyperparameters.target_modules = std::move(modules);
            }
        }
    }

    // ── Range validation (paper constraints + training contract) ──────────────
    const auto& s = cfg.schedule;
    if (s.target_rank < 1) {
        errors.emplace_back("rank_budget.target_rank must be >= 1");
    }
    if (s.init_rank < s.target_rank) {
        errors.emplace_back("rank_budget.initial_rank must be >= rank_budget.target_rank");
    }
    if (cfg.max_rank < 0) {
        errors.emplace_back("rank_budget.max_rank must be >= 0 (0 = no cap)");
    } else if (cfg.max_rank > 0 && s.init_rank > cfg.max_rank) {
        errors.emplace_back("rank_budget.initial_rank must be <= rank_budget.max_rank");
    }
    if (s.warmup_steps < 0) {
        errors.emplace_back("importance_pruning.warmup_steps (t_i) must be >= 0");
    }
    if (s.final_steps < 0) {
        errors.emplace_back("importance_pruning.final_steps (t_f) must be >= 0");
    }
    if (s.delta_t < 1) {
        errors.emplace_back("importance_pruning.pruning_interval_steps (delta_T) must be >= 1");
    }
    if (!isFiniteInOpenUnit(s.beta1)) {
        errors.emplace_back("importance_pruning.beta1 must satisfy 0 < beta1 < 1");
    }
    if (!isFiniteInOpenUnit(s.beta2)) {
        errors.emplace_back("importance_pruning.beta2 must satisfy 0 < beta2 < 1");
    }
    if (!std::isfinite(s.orth_reg_weight) || s.orth_reg_weight < 0.0) {
        errors.emplace_back("importance_pruning.orth_reg_weight (gamma) must be >= 0");
    }
    if (metric != "gradient_sensitivity") {
        errors.emplace_back("importance_pruning.metric '" + metric +
                            "' is not supported (only 'gradient_sensitivity', the AdaLoRA |w*grad(w)| score)");
    }

    if (!std::isfinite(alpha_multiplier) || alpha_multiplier <= 0.0) {
        errors.emplace_back("hyperparameters.alpha_multiplier must be > 0");
    }
    if (!std::isfinite(dropout) || dropout < 0.0 || dropout >= 1.0) {
        errors.emplace_back("hyperparameters.dropout must satisfy 0 <= dropout < 1");
    }
    if (!std::isfinite(learning_rate) || learning_rate <= 0.0) {
        errors.emplace_back("hyperparameters.learning_rate must be > 0");
    }
    if (cfg.hyperparameters.batch_size < 1) {
        errors.emplace_back("hyperparameters.batch_size must be >= 1");
    }
    if (cfg.gradient_accumulation_steps < 1) {
        errors.emplace_back("hyperparameters.gradient_accumulation_steps must be >= 1");
    }
    if (cfg.hyperparameters.max_seq_length < 1) {
        errors.emplace_back("hyperparameters.max_seq_length must be >= 1");
    }
    if (cfg.hyperparameters.num_epochs < 1) {
        errors.emplace_back("hyperparameters.num_epochs must be >= 1");
    }
    static constexpr std::array<const char*, 3> kOptimizers{"sgd", "adam", "adamw"};
    if (std::none_of(kOptimizers.begin(), kOptimizers.end(),
                     [&](const char* o) { return cfg.hyperparameters.optimizer == o; })) {
        errors.emplace_back("hyperparameters.optimizer.type '" + cfg.hyperparameters.optimizer +
                            "' is not one of sgd|adam|adamw");
    }
    if (!std::isfinite(adam_beta1) || adam_beta1 < 0.0 || adam_beta1 >= 1.0) {
        errors.emplace_back("hyperparameters.optimizer.beta1 must satisfy 0 <= beta1 < 1");
    }
    if (!std::isfinite(adam_beta2) || adam_beta2 < 0.0 || adam_beta2 >= 1.0) {
        errors.emplace_back("hyperparameters.optimizer.beta2 must satisfy 0 <= beta2 < 1");
    }
    if (!std::isfinite(adam_eps) || adam_eps <= 0.0) {
        errors.emplace_back("hyperparameters.optimizer.eps must be > 0");
    }
    if (!std::isfinite(weight_decay) || weight_decay < 0.0) {
        errors.emplace_back("hyperparameters.optimizer.weight_decay must be >= 0");
    }
    static constexpr std::array<const char*, 6> kSchedulers{
        "constant", "linear_warmup", "cosine", "cosine_warmup", "step", "exponential"};
    if (std::none_of(kSchedulers.begin(), kSchedulers.end(),
                     [&](const char* n) { return cfg.hyperparameters.lr_scheduler == n; })) {
        errors.emplace_back("hyperparameters.lr_scheduler.type '" + cfg.hyperparameters.lr_scheduler +
                            "' is not one of constant|linear_warmup|cosine|cosine_warmup|step|exponential");
    }
    if (cfg.hyperparameters.warmup_steps < 0) {
        errors.emplace_back("hyperparameters.lr_scheduler.warmup_steps must be >= 0");
    }

    if (!errors.empty()) {
        throw AdaLoRAStrategyConfigError(source, std::move(errors));
    }

    // ── Derived values ────────────────────────────────────────────────────────
    cfg.hyperparameters.rank = s.target_rank;
    cfg.hyperparameters.alpha = static_cast<float>(alpha_multiplier * s.target_rank);
    cfg.hyperparameters.dropout = static_cast<float>(dropout);
    cfg.hyperparameters.learning_rate = static_cast<float>(learning_rate);
    cfg.hyperparameters.beta1 = static_cast<float>(adam_beta1);
    cfg.hyperparameters.beta2 = static_cast<float>(adam_beta2);
    cfg.hyperparameters.epsilon = static_cast<float>(adam_eps);
    cfg.hyperparameters.weight_decay = static_cast<float>(weight_decay);

    // ── Paper-guidance warnings (non-fatal) ───────────────────────────────────
    if (cfg.method == AdapterTrainingMethod::AdaLoRA) {
        const double ratio = static_cast<double>(s.init_rank) / static_cast<double>(s.target_rank);
        if (ratio < 1.25 || ratio > 2.0) {
            cfg.warnings.push_back("initial_rank/target_rank = " + std::to_string(ratio) +
                                   " deviates from the AdaLoRA guidance b(0) ~ 1.5 x b(T)");
        }
        if (s.orth_reg_weight < 0.1 || s.orth_reg_weight > 0.5) {
            cfg.warnings.push_back("orth_reg_weight outside the AdaLoRA paper range [0.1, 0.5]");
        }
    }
    return cfg;
}

}  // namespace

AdaLoRAStrategyConfigError::AdaLoRAStrategyConfigError(const std::string& source,
                                                       std::vector<std::string> errors)
    : std::runtime_error(joinErrors(source, errors)), errors_(std::move(errors)) {}

AdaLoRAStrategyConfig loadAdaLoRAStrategy(const std::string& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::BadFile&) {
        throw AdaLoRAStrategyConfigError(path, {"file cannot be opened"});
    } catch (const YAML::Exception& e) {
        throw AdaLoRAStrategyConfigError(path, {std::string("YAML parse error: ") + e.what()});
    }
    return parseStrategy(root, path);
}

AdaLoRAStrategyConfig loadAdaLoRAStrategyFromString(const std::string& yaml_text,
                                                    const std::string& source_label) {
    YAML::Node root;
    try {
        root = YAML::Load(yaml_text);
    } catch (const YAML::Exception& e) {
        throw AdaLoRAStrategyConfigError(source_label, {std::string("YAML parse error: ") + e.what()});
    }
    return parseStrategy(root, source_label);
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
