#include "llm/sampling_strategy.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace themis {
namespace llm {

// ===== GreedySampling =====

llama_token GreedySampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    spdlog::debug("GreedySampling::sample - selecting token with highest probability");
    
    // Get logits for the specified position
    float* logits = llama_get_logits_ith(ctx, pos);
    size_t n_vocab = llama_n_vocab(llama_get_model(ctx));
    
    // Find token with highest probability (greedy selection)
    auto max_it = std::max_element(logits, logits + n_vocab);
    llama_token result = static_cast<llama_token>(std::distance(logits, max_it));
    
    spdlog::debug("GreedySampling selected token: {}", result);
    return result;
}

// ===== NucleusSampling =====

NucleusSampling::NucleusSampling(float temperature,
                                 int top_k,
                                 float top_p,
                                 float repeat_penalty)
    : temperature_(temperature)
    , top_k_(top_k)
    , top_p_(top_p)
    , repeat_penalty_(repeat_penalty) {
    
    spdlog::debug("NucleusSampling created: temp={}, top_k={}, top_p={}, penalty={}",
                 temperature_, top_k_, top_p_, repeat_penalty_);
}

llama_token NucleusSampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    spdlog::debug("NucleusSampling::sample - temp={}, top_k={}, top_p={}, repeat_penalty={}",
                  temperature_, top_k_, top_p_, repeat_penalty_);
    
    llama_model* model = llama_get_model(ctx);
    size_t n_vocab = llama_n_vocab(model);
    
    // Create sampler chain with default parameters
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    
    // Add repeat penalty sampler
    llama_sampler_chain_add(
        sampler,
        llama_sampler_init_penalties(
            n_vocab,
            llama_token_eos(model),
            llama_token_nl(model),
            0,                     // penalty_last_n (0 = disabled)
            repeat_penalty_,       // repeat penalty
            0.0f,                  // frequency penalty
            0.0f,                  // presence penalty
            false                  // penalize_nl
        )
    );
    
    // Add top-k filtering
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(top_k_));
    
    // Add top-p (nucleus) filtering
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(top_p_, 1));
    
    // Add temperature scaling
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(temperature_));
    
    // Add distribution sampler (final sampling step)
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(0)); // seed=0 for random
    
    // Sample token
    llama_token result = llama_sampler_sample(sampler, ctx, pos);
    
    // Cleanup sampler chain
    llama_sampler_free(sampler);
    
    spdlog::debug("NucleusSampling selected token: {}", result);
    return result;
}

// ===== MirostatSampling =====

MirostatSampling::MirostatSampling(float tau, float eta)
    : tau_(tau)
    , eta_(eta)
    , mu_(2.0f * tau) {
    
    spdlog::debug("MirostatSampling created: tau={}, eta={}", tau_, eta_);
}

llama_token MirostatSampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& last_tokens,
    int pos) {
    
    spdlog::debug("MirostatSampling::sample - tau={}, eta={}, mu={}", tau_, eta_, mu_);
    
    llama_model* model = llama_get_model(ctx);
    
    // Create sampler chain with default parameters
    llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    
    // Add Mirostat v2 sampler
    // The seed parameter (0) means use random seed
    llama_sampler_chain_add(
        sampler,
        llama_sampler_init_mirostat_v2(0, tau_, eta_)
    );
    
    // Sample token
    llama_token result = llama_sampler_sample(sampler, ctx, pos);
    
    // Cleanup sampler chain
    llama_sampler_free(sampler);
    
    spdlog::debug("MirostatSampling selected token: {}", result);
    return result;
}

// ===== Factory =====

std::unique_ptr<ISamplingStrategy> SamplingStrategyFactory::create(
    const std::string& strategy_name,
    float temperature,
    int top_k,
    float top_p) {
    
    if (strategy_name == "greedy") {
        spdlog::info("Creating GreedySampling strategy");
        return std::make_unique<GreedySampling>();
        
    } else if (strategy_name == "nucleus" || strategy_name == "top_p") {
        spdlog::info("Creating NucleusSampling strategy");
        return std::make_unique<NucleusSampling>(temperature, top_k, top_p);
        
    } else if (strategy_name == "mirostat") {
        spdlog::info("Creating MirostatSampling strategy");
        return std::make_unique<MirostatSampling>();
        
    } else {
        spdlog::warn("Unknown sampling strategy '{}', using nucleus", strategy_name);
        return std::make_unique<NucleusSampling>(temperature, top_k, top_p);
    }
}

} // namespace llm
} // namespace themis
