/**
 * @file sampling_strategy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/sampling_strategy.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <limits>

#if __has_include(<llama.h>)
#include <llama.h>
#elif __has_include("llama.h")
#include "llama.h"
#else
extern "C" float* llama_get_logits_ith(struct llama_context* ctx, std::int32_t i);
#endif

namespace themis {
namespace llm {

// ===== GreedySampling =====

llama_token GreedySampling::sample(
    llama_context* ctx,
    const std::vector<llama_token>& /*last_tokens*/,
    int pos) {
    
    // Note: last_tokens parameter is not used in greedy sampling
    // Greedy always selects the highest probability token deterministically
    spdlog::debug("GreedySampling::sample - selecting token with highest probability");
    
    // Get logits for the specified position
    float* logits = llama_get_logits_ith(ctx, pos);
    size_t n_vocab = 32000;  // Standard llama vocabulary size
    
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

    // Fetch logits for current position
    float* logits = llama_get_logits_ith(ctx, pos);
    const size_t n_vocab = 32000; // fallback vocab size

    // Copy logits to vector for manipulation
    std::vector<float> scores(logits, logits + n_vocab);

    // Apply repeat penalty
    if (repeat_penalty_ > 1.0f && !last_tokens.empty()) {
        for (auto t : last_tokens) {
            if (t >= 0 && static_cast<size_t>(t) < scores.size()) {
                scores[t] /= repeat_penalty_;
            }
        }
    }

    // Apply temperature scaling
    if (temperature_ > 0.0f && std::abs(temperature_ - 1.0f) > 1e-6f) {
        for (auto& s : scores) {
          s /= temperature_;
        }
    }

    // Select top-k candidates
    std::vector<int> indices(scores.size());
    std::iota(indices.begin(), indices.end(), 0);
    const int k = std::max(1, std::min(top_k_, static_cast<int>(n_vocab)));
    std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
        [&](int a, int b) { return scores[a] > scores[b]; });

    indices.resize(k);

    // Compute softmax probabilities over top-k
    std::vector<float> probs;
    probs.reserve(indices.size());
    float max_logit = -std::numeric_limits<float>::infinity();
    for (int idx : indices) {
      max_logit = std::max(max_logit, scores[idx]);
    }
    float sum = 0.0f;
    for (int idx : indices) {
        float p = std::exp(scores[idx] - max_logit);
        probs.push_back(p);
        sum += p;
    }
    for (auto& p : probs) {
      p /= (sum > 0.f ? sum : 1.f);
    }

    // Sort by probability desc for nucleus filtering
    std::vector<int> order(indices.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort([[maybe_unused]] order.begin(), order.end(), [&](int a, int b) { return probs[a] > probs[b]; });

    // Keep smallest set with cumulative prob >= top_p_
    float cum = 0.0f;
    std::vector<int> nucleus;
    for (int oi : order) {
        nucleus.push_back(indices[oi]);
        cum += probs[oi];
        if (cum >= top_p_) {
          break;
        }
    }
    if (nucleus.empty()) {
      nucleus.push_back(indices[order.front()]);
    }

    // Renormalize probabilities over nucleus and sample
    std::vector<float> nuc_probs;
    nuc_probs.reserve(nucleus.size());
    float nuc_sum = 0.0f;
    for (int id : nucleus) {
        // find id in indices to get its prob
        auto it = std::find(indices.begin(), indices.end(), id);
        size_t pidx = static_cast<size_t>(std::distance(indices.begin(), it));
        float p = probs[pidx];
        nuc_probs.push_back(p);
        nuc_sum += p;
    }
    for (auto& p : nuc_probs) {
      p /= (nuc_sum > 0.f ? nuc_sum : 1.f);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<int> dist(nuc_probs.begin(), nuc_probs.end());
    int chosen = nucleus[dist(gen)];

    spdlog::debug("NucleusSampling selected token: {}", chosen);
    return static_cast<llama_token>(chosen);
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
    const std::vector<llama_token>& /*last_tokens*/,
    int pos) {
    // Simplified Mirostat: approximate via nucleus with adaptive temperature
    spdlog::debug("MirostatSampling::sample - tau={}, eta={}", tau_, eta_);

    float temp = std::clamp(tau_ / 5.0f, 0.5f, 2.0f);
    const int top_k = 50;
    const float top_p = 0.95f;

    // Fetch logits
    float* logits = llama_get_logits_ith(ctx, pos);
    const size_t n_vocab = 32000;
    std::vector<float> scores(logits, logits + n_vocab);

    // Apply temperature
    for (auto& s : scores) {
      s /= temp;
    }

    // Basic top-k/top-p sampling (reuse logic similar to nucleus)
    std::vector<int> indices(scores.size());
    std::iota(indices.begin(), indices.end(), 0);
    const int k = std::max(1, std::min(top_k, static_cast<int>(n_vocab)));
    std::partial_sort(indices.begin(), indices.begin() + k, indices.end(),
        [&](int a, int b) { return scores[a] > scores[b]; });
    indices.resize(k);

    // Softmax on top-k
    std::vector<float> probs;
    probs.reserve(indices.size());
    float max_logit = -std::numeric_limits<float>::infinity();
    for (int idx : indices) {
      max_logit = std::max(max_logit, scores[idx]);
    }
    float sum = 0.0f;
    for (int idx : indices) { float p = std::exp(scores[idx] - max_logit); probs.push_back(p); sum += p; }
    for (auto& p : probs) {
      p /= (sum > 0.f ? sum : 1.f);
    }

    // Nucleus filter
    std::vector<int> order(indices.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort([[maybe_unused]] order.begin(), order.end(), [&](int a, int b) { return probs[a] > probs[b]; });
    float cum = 0.0f; std::vector<int> nucleus;
    for (int oi : order) { nucleus.push_back(indices[oi]); cum += probs[oi]; if (cum >= top_p) break; }
    if (nucleus.empty()) {
      nucleus.push_back(indices[order.front()]);
    }
    std::vector<float> nuc_probs; nuc_probs.reserve(nucleus.size()); float nuc_sum = 0.0f;
    for (int id : nucleus) { auto it = std::find(indices.begin(), indices.end(), id); size_t pidx = std::distance(indices.begin(), it); float p = probs[pidx]; nuc_probs.push_back(p); nuc_sum += p; }
    for (auto& p : nuc_probs) {
      p /= (nuc_sum > 0.f ? nuc_sum : 1.f);
    }

    std::random_device rd; std::mt19937 gen(rd());
    std::discrete_distribution<int> dist(nuc_probs.begin(), nuc_probs.end());
    int chosen = nucleus[dist(gen)];
    spdlog::debug("MirostatSampling selected token: {}", chosen);
    return static_cast<llama_token>(chosen);
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
        spdlog::info("Creating MirostatSampling strategy with default parameters (tau=5.0, eta=0.1)");
        // Mirostat uses its own default parameters (tau=5.0f, eta=0.1f)
        // For custom Mirostat parameters, instantiate MirostatSampling directly
        return std::make_unique<MirostatSampling>();
        
    } else {
        spdlog::warn("Unknown sampling strategy '{}', using nucleus", strategy_name);
        return std::make_unique<NucleusSampling>(temperature, top_k, top_p);
    }
}

} // namespace llm
} // namespace themis

