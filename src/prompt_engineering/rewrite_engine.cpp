/**
 * @file rewrite_engine.cpp
 * @brief Core RewriteEngine implementation (Phase 2 delivery).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 core engine (Q4 2026)
 *
 * Main orchestration logic for deterministic rewrite operation:
 * - Phase ordering enforcement
 * - Priority-based rule execution
 * - Max-steps loop prevention
 * - Trace collection
 * - Metrics and observability
 * - Thread-safe rule lookup
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "prompt_engineering/rewrite_engine.h"
#include "prompt_engineering/rewrite_rule.h"
#include "prompt_engineering/rewrite_rule_loader.h"
#include <algorithm>
#include <chrono>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <shared_mutex>

namespace themis {
namespace prompt_engineering {

/**
 * @class RewriteEngine
 * @brief Production implementation of IRewriteEngine.
 */
class RewriteEngine : public IRewriteEngine {
public:
    RewriteEngine() : rule_lock_(), rules_(), stats_() {
        reset_stats();
    }

    ~RewriteEngine() override = default;

    bool register_rule(std::shared_ptr<IRewriteRule> rule) override {
        if (!rule) {
            auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");
            logger->warn("Attempted to register null rule");
            return false;
        }

        std::unique_lock lock(rule_lock_);

        // Check for duplicate rule ID
        for (const auto& existing : rules_) {
            if (existing->rule_id() == rule->rule_id()) {
                auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");
                logger->warn("Rule {} already registered, skipping", rule->rule_id());
                return false;
            }
        }

        rules_.push_back(rule);
        stats_.total_rules_registered++;

        auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");
        logger->debug("Registered rule: {} (phase={}, priority={})", 
                      rule->rule_id(), 
                      static_cast<int>(rule->execution_phase()),
                      static_cast<int>(rule->priority()));

        return true;
    }

    bool unregister_rule(const std::string& rule_id) override {
        std::unique_lock lock(rule_lock_);

        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&rule_id](const auto& r) { return r->rule_id() == rule_id; });

        if (it == rules_.end()) {
            return false;
        }

        rules_.erase(it);
        return true;
    }

    std::shared_ptr<const IRewriteRule> get_rule(const std::string& rule_id) const override {
        std::shared_lock lock(rule_lock_);

        for (const auto& rule : rules_) {
            if (rule->rule_id() == rule_id) {
                return rule;
            }
        }

        return nullptr;
    }

    std::vector<std::string> list_rules() const override {
        std::shared_lock lock(rule_lock_);

        std::vector<std::string> rule_ids = {};

        for (const auto& rule : rules_) {
            rule_ids.push_back(rule->rule_id());
        }

        return rule_ids;
    }

    bool load_rules_from_yaml(const std::string& yaml_path) override {
        auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

        RewriteRuleLoader loader;
        std::vector<std::shared_ptr<IRewriteRule>> new_rules;

        if (!loader.load_rules_from_yaml(yaml_path, new_rules)) {
            logger->error("Failed to load YAML rules from {}: {}", yaml_path, loader.last_error());
            return false;
        }

        std::unique_lock lock(rule_lock_);
        for (auto& rule : new_rules) {
            bool duplicate = false;
            for (const auto& existing : rules_) {
                if (existing->rule_id() == rule->rule_id()) {
                    logger->warn("Skipping duplicate rule {} from YAML", rule->rule_id());
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                rules_.push_back(rule);
                stats_.total_rules_registered++;
            }
        }

        logger->info("Loaded {} YAML rules from {}",static_cast<int>(new_rules.size()), yaml_path);
        return true;
    }

    RewriteResult rewrite(RewriteDocument& doc, const RewriteContext& ctx) override {
        auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

        auto start_time = std::chrono::high_resolution_clock::now();

        RewriteResult result;
        result.success = true;
        result.error_code = PromptEngineeringErrorCode::TEMPLATE_INVALID_ID; // Default, will be overwritten
        result.was_blocked = false;
        result.rules_matched = 0;
        result.total_transformations = 0;

        // Execute phases in order
        for (int phase = 1; phase <= 4; ++phase) {
            RewritePhase current_phase = static_cast<RewritePhase>(phase);

            result = rewrite_phase(current_phase, doc, ctx);

            if (!result.success) {
                logger->error("Rewrite failed in phase {}: {}", phase, result.error_message);
                break;
            }

            if (result.was_blocked) {
                logger->info("Rewrite terminated by policy in phase {}", phase);
                break;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        result.total_latency_micros = latency;

        stats_.total_rewrites++;
        stats_.total_latency_micros += latency;

        return result;
    }

    RewriteResult rewrite_phase(
        RewritePhase phase,
        RewriteDocument& doc,
        const RewriteContext& ctx
    ) override {
        auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

        RewriteResult result;
        result.success = true;
        result.error_code = PromptEngineeringErrorCode::TEMPLATE_INVALID_ID;
        result.was_blocked = false;
        result.rules_matched = 0;
        result.total_transformations = 0;

        std::shared_lock lock(rule_lock_);

        // Get rules for this phase, sorted by priority then registration order
        auto phase_rules = get_rules_for_phase(phase);

        // Execute rules until fixpoint or max steps
        uint32_t step_count = 0;
        const uint32_t max_steps = ctx.max_steps > 0 ? ctx.max_steps : 1000;

        bool any_changed = false;

        do {
            any_changed = false;

            for (const auto& rule : phase_rules) {
                step_count++;

                if (step_count > max_steps) {
                    result.success = false;
                    result.error_code = PromptEngineeringErrorCode::REWRITE_MAX_STEPS_EXCEEDED;
                    result.error_message = "Max rewrite steps exceeded";
                    logger->error("Rewrite phase {} exceeded max steps ({})", 
                                  static_cast<int>(phase), max_steps);
                    stats_.max_steps_exceeded++;
                    return result;
                }

                if (!rule->matches(doc, ctx)) {
                    continue;
                }

                RewriteTrace trace;
                trace.phase = phase;
                trace.rule_id = rule->rule_id();

                auto apply_start = std::chrono::high_resolution_clock::now();

                auto rule_result = rule->apply(doc, ctx, trace);

                auto apply_end = std::chrono::high_resolution_clock::now();
                trace.rule_latency_micros = std::chrono::duration_cast<std::chrono::microseconds>(apply_end - apply_start).count();

                stats_.total_rules_evaluated++;
                stats_.total_rule_latency_micros += trace.rule_latency_micros;

                if (!rule_result.success) {
                    result.success = false;
                    result.error_code = rule_result.error_code;
                    result.error_message = rule_result.error_message;
                    logger->error("Rule {} failed: {}", rule->rule_id(), result.error_message);
                    return result;
                }

                if (rule_result.was_blocked) {
                    result.was_blocked = true;
                    result.traces.push_back(trace);
                    result.rules_matched++;
                    logger->info("Rule {} blocked output", rule->rule_id());
                    return result;
                }

                if (trace.transformation_applied) {
                    any_changed = true;
                    result.total_transformations++;
                    result.rules_matched++;
                    stats_.total_rules_applied++;

                    if (ctx.trace_enabled && static_cast<int>(result.traces.size()) < ctx.max_trace_entries) {
                        result.traces.push_back(trace);
                    }

                    logger->debug("Rule {} applied transformation", rule->rule_id());
                }
            }

        } while (any_changed && step_count < max_steps);

        result.transformed_text = doc.content;
        return result;
    }

    std::string get_stats_json() const override {
        std::shared_lock lock(rule_lock_);

        nlohmann::json stats_obj;
        stats_obj["total_rules_registered"] = stats_.total_rules_registered;
        stats_obj["total_rewrites"] = stats_.total_rewrites;
        stats_obj["total_rules_evaluated"] = stats_.total_rules_evaluated;
        stats_obj["total_rules_applied"] = stats_.total_rules_applied;
        stats_obj["max_steps_exceeded"] = stats_.max_steps_exceeded;
        stats_obj["total_latency_micros"] = stats_.total_latency_micros;
        stats_obj["avg_latency_micros"] = stats_.total_rewrites > 0 
            ? stats_.total_latency_micros / stats_.total_rewrites 
            : 0;

        return stats_obj.dump();
    }

    void reset_stats() override {
        std::unique_lock lock(rule_lock_);
        stats_ = Stats{};
    }

private:
    struct Stats {
        uint64_t total_rules_registered = 0;
        uint64_t total_rewrites = 0;
        uint64_t total_rules_evaluated = 0;
        uint64_t total_rules_applied = 0;
        uint64_t max_steps_exceeded = 0;
        uint64_t total_latency_micros = 0;
        uint64_t total_rule_latency_micros = 0;
    };

    mutable std::shared_mutex rule_lock_;
    std::vector<std::shared_ptr<IRewriteRule>> rules_;
    mutable Stats stats_;

    std::vector<std::shared_ptr<IRewriteRule>> get_rules_for_phase(RewritePhase phase) const {
        std::vector<std::shared_ptr<IRewriteRule>> phase_rules;

        for (const auto& rule : rules_) {
            if (rule->execution_phase() == phase) {
                phase_rules.push_back(rule);
            }
        }

        // Sort by priority (lower first), then maintain registration order
        std::stable_sort(phase_rules.begin(), phase_rules.end(),
                        [](const auto& a, const auto& b) {
                            return a->priority() < b->priority();
                        });

        return phase_rules;
    }
};

// Factory function
std::unique_ptr<IRewriteEngine> create_rewrite_engine() {
    return std::make_unique<RewriteEngine>();
}

} // namespace prompt_engineering
} // namespace themis
