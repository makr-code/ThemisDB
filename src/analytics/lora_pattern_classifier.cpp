/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            analytics/lora_pattern_classifier.cpp              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "analytics/lora_pattern_classifier.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <sstream>
#include <stdexcept>

namespace themisdb {
namespace analytics {

// ──────────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────────

LoRAPatternClassifier::LoRAPatternClassifier(Config cfg) : cfg_(cfg) {}

// ──────────────────────────────────────────────────────────────────────────────
// Injection setters
// ──────────────────────────────────────────────────────────────────────────────

void LoRAPatternClassifier::setInferenceFn(InferenceFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    inference_fn_ = std::move(fn);
}

void LoRAPatternClassifier::setEmbeddingFn(EmbeddingFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    embedding_fn_ = std::move(fn);
}

void LoRAPatternClassifier::registerAdapterDomain(AdapterDomain domain) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Overwrite existing entry with same adapter_id.
    auto it = std::find_if(domains_.begin(), domains_.end(),
                            [&](const AdapterDomain& d) {
                                return d.adapter_id == domain.adapter_id;
                            });
    if (it != domains_.end()) *it = std::move(domain);
    else domains_.push_back(std::move(domain));
}

// ──────────────────────────────────────────────────────────────────────────────
// buildPrompt
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ std::string LoRAPatternClassifier::buildPrompt(
    const std::vector<DataPoint>& events,
    const std::string& adapter_id) {

    std::ostringstream oss;
    oss << "Classify the following events for pattern detection.\n";
    if (!adapter_id.empty())
        oss << "Adapter: " << adapter_id << "\n";
    oss << "Events:\n";

    const std::size_t max_events = 10;
    const std::size_t n = std::min(events.size(), max_events);
    for (std::size_t i = 0; i < n; ++i) {
        const auto& dp = events[i];
        oss << "- [" << dp.id << "] ";
        bool first = true;
        for (const auto& [k, v] : dp.fields) {
            if (!first) oss << ", ";
            oss << k << "=";
            std::visit([&](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    oss << "null";
                } else {
                    oss << val;
                }
            }, v);
            first = false;
        }
        oss << "\n";
    }
    oss << "Respond with JSON: {\"label\":\"<label>\",\"confidence\":<0-1>}\n";
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// parseInferenceResponse
// ──────────────────────────────────────────────────────────────────────────────

PatternResult LoRAPatternClassifier::parseInferenceResponse(
    const std::string& json,
    const std::string& adapter_id) const {

    PatternResult result;
    result.adapter_id = adapter_id;

    // Extract label.
    const std::string label_key = "\"label\":\"";
    auto pos = json.find(label_key);
    if (pos != std::string::npos) {
        pos += label_key.size();
        const auto end = json.find('"', pos);
        if (end != std::string::npos)
            result.label = json.substr(pos, end - pos);
    }

    // Extract confidence.
    const std::string conf_key = "\"confidence\":";
    pos = json.find(conf_key);
    if (pos != std::string::npos) {
        pos += conf_key.size();
        try {
            std::size_t consumed = 0;
            result.confidence = std::stod(json.substr(pos), &consumed);
            // Clamp to [0, 1].
            result.confidence = std::max(0.0, std::min(1.0, result.confidence));
        } catch (...) {
            result.confidence = 0.0;
        }
    }

    if (result.label.empty()) {
        result.label     = "unknown";
        result.confidence = 0.0;
    }
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// automlFallback
// STUB/SIMULATION NOTE:
// Purpose:          AutoML fallback when no LoRA inference fn is injected.
// Activation:       inference_fn_ is null.
// Production Delta: Returns constant confidence; real impl trains AutoML on
//                   labelled events and calls AutoML::predict().
// Removal Plan:     Q3 2027 — wire AutoML::train()+predict() pipeline with
//                   labelled CEP event dataset.
// ──────────────────────────────────────────────────────────────────────────────

PatternResult LoRAPatternClassifier::automlFallback(
    const std::vector<DataPoint>& /*events*/,
    const std::string& adapter_id) const {

    PatternResult result;
    result.label        = "unknown";
    result.confidence   = cfg_.fallback_confidence;
    result.adapter_id   = adapter_id;
    result.used_fallback = true;
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// cosineSimilarity
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ double LoRAPatternClassifier::cosineSimilarity(
    const std::vector<double>& a,
    const std::vector<double>& b) {

    if (a.empty() || a.size() != b.size()) return 0.0;

    double dot = 0.0, na = 0.0, nb = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    if (na <= 0.0 || nb <= 0.0) return 0.0;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

// ──────────────────────────────────────────────────────────────────────────────
// selectAdapter
// ──────────────────────────────────────────────────────────────────────────────

std::string LoRAPatternClassifier::selectAdapter(const std::string& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (domains_.empty()) return "";
    if (!embedding_fn_)   return domains_.front().adapter_id;

    std::vector<double> ctx_emb;
    try { ctx_emb = embedding_fn_(context); }
    catch (...) { return domains_.front().adapter_id; }

    std::string best_id;
    double      best_sim = -1.0;
    for (const auto& d : domains_) {
        const double sim = cosineSimilarity(ctx_emb, d.embedding);
        if (sim > best_sim) {
            best_sim = sim;
            best_id  = d.adapter_id;
        }
    }
    return best_id;
}

// ──────────────────────────────────────────────────────────────────────────────
// classify
// ──────────────────────────────────────────────────────────────────────────────

PatternResult LoRAPatternClassifier::classify(const std::vector<DataPoint>& events,
                                               const std::string& adapter_id) {
    // Determine adapter (unlocked for selectAdapter to take its own lock).
    std::string aid = adapter_id;
    if (aid.empty()) aid = selectAdapter(buildPrompt(events, ""));

    std::lock_guard<std::mutex> lock(mutex_);

    if (!inference_fn_)
        return automlFallback(events, aid);

    const std::string prompt = buildPrompt(events, aid);
    std::string response;
    try {
        response = inference_fn_(aid, prompt);
    } catch (...) {
        return automlFallback(events, aid);
    }
    return parseInferenceResponse(response, aid);
}

// ──────────────────────────────────────────────────────────────────────────────
// batchClassify
// ──────────────────────────────────────────────────────────────────────────────

std::vector<PatternResult> LoRAPatternClassifier::batchClassify(
    const std::vector<DataPoint>& events) {

    if (events.empty()) return {};

    const std::size_t n       = events.size();
    const std::size_t workers = std::max<std::size_t>(1, cfg_.max_parallel_workers);

    std::vector<PatternResult>                   results(n);
    std::vector<std::future<PatternResult>>      futures;
    futures.reserve(workers);

    std::size_t i = 0;
    while (i < n) {
        // Dispatch up to `workers` tasks.
        futures.clear();
        const std::size_t batch_end = std::min(i + workers, n);
        for (std::size_t j = i; j < batch_end; ++j) {
            const DataPoint& ev = events[j];
            futures.push_back(
                std::async(std::launch::async, [this, &ev]() -> PatternResult {
                    return classify({ev});
                }));
        }
        // Collect results in order.
        for (std::size_t j = i; j < batch_end; ++j)
            results[j] = futures[j - i].get();
        i = batch_end;
    }
    return results;
}

// ──────────────────────────────────────────────────────────────────────────────
// State queries
// ──────────────────────────────────────────────────────────────────────────────

std::size_t LoRAPatternClassifier::registeredAdapterCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return domains_.size();
}

bool LoRAPatternClassifier::hasInferenceFn() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(inference_fn_);
}

} // namespace analytics
} // namespace themisdb
