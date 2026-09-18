/**
 * @file sampling_strategy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

struct llama_context;
using llama_token = std::int32_t;

namespace themis {
namespace llm {

class ISamplingStrategy {
public:
    /**
     * @brief ISampling Strategy.
     * @return Return value.
     */
    virtual ~ISamplingStrategy() = default;
    
    [[nodiscard]] virtual llama_token sample(
        llama_context* ctx,
        const std::vector<llama_token>& last_tokens,
        int pos
    ) = 0;
    
    [[nodiscard]] virtual std::string name() const = 0;
};

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
    float temperature_ = 0.0f;
    int top_k_ = 0;
    float top_p_ = 0.0f;
    float repeat_penalty_ = 0.0f;
};

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
    float tau_ = 0.0f;    // Target entropy
    float eta_ = 0.0f;    // Learning rate
    float mu_ = 0.0f;     // Current mu value (adaptive)
};

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
