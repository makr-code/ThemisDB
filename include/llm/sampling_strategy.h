/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sampling_strategy.h                                ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:25:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     131                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <llama.h>
#include <vector>
#include <memory>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief Abstract Strategy for Token Sampling
 * 
 * Design Pattern: Strategy Pattern
 * Allows different sampling algorithms without changing client code
 */
class ISamplingStrategy {
public:
    virtual ~ISamplingStrategy() = default;
    
    virtual llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) = 0;
    
    virtual std::string name() const = 0;
};

/**
 * @brief Greedy Sampling (always highest probability)
 */
class GreedySampling : public ISamplingStrategy {
public:
    GreedySampling() = default;
    ~GreedySampling() override = default;
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "greedy"; }
};

/**
 * @brief Top-K + Top-P (Nucleus) Sampling
 */
class NucleusSampling : public ISamplingStrategy {
public:
    explicit NucleusSampling(float temperature = 0.8f,
                            int top_k = 40,
                            float top_p = 0.9f,
                            float repeat_penalty = 1.1f);
    ~NucleusSampling() override = default;
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "nucleus"; }

private:
    float temperature_;
    int top_k_;
    float top_p_;
    float repeat_penalty_;
};

/**
 * @brief Mirostat Sampling (adaptive, better quality)
 */
class MirostatSampling : public ISamplingStrategy {
public:
    explicit MirostatSampling(float tau = 5.0f, float eta = 0.1f);
    ~MirostatSampling() override = default;
    
    llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) override;
    
    std::string name() const override { return "mirostat"; }

private:
    float tau_;    // Target entropy
    float eta_;    // Learning rate
    float mu_;     // Current mu value (adaptive)
};

/**
 * @brief Factory for Sampling Strategies
 */
class SamplingStrategyFactory {
public:
    static std::unique_ptr<ISamplingStrategy> create(
        const std::string& strategy_name,
        float temperature = 0.8f,
        int top_k = 40,
        float top_p = 0.9f
    );
};

} // namespace llm
} // namespace themis
