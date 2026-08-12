/**
 * @file lora_pattern_classifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/lora_pattern_classifier.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <numeric>
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
                           [&](const AdapterDomain &d) { return d.adapter_id == domain.adapter_id; });
    if (it != domains_.end()) {
        *it = std::move(domain);
    } else {
        domains_.push_back(std::move(domain));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// buildPrompt
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ std::string LoRAPatternClassifier::buildPrompt(const std::vector<DataPoint> &events,
                                                          const std::string &adapter_id) {
    std::ostringstream oss;
    oss << "Classify the following events for pattern detection.\n";
    if (!adapter_id.empty()) {
        oss << "Adapter: " << adapter_id << "\n";
    }
    oss << "Events:\n";

    const std::size_t max_events = 10;
    const std::size_t n          = std::min(events.size(), max_events);
    for (std::size_t i = 0; i < n; ++i) {
        const auto &dp = events[i];
        oss << "- [" << dp.id << "] ";
        bool first = true;
        for (const auto &[k, v] : dp.fields) {
            if (!first) {
                oss << ", ";
            }
            oss << k << "=";
            std::visit(
                [&](const auto &val) {
                    using T = std::decay_t<decltype(val)>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        oss << "null";
                    } else {
                        oss << val;
                    }
                },
                v);
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

PatternResult LoRAPatternClassifier::parseInferenceResponse(const std::string &json,
                                                            const std::string &adapter_id) const {
    PatternResult result;
    result.adapter_id = adapter_id;

    // Extract label.
    const std::string label_key = "\"label\":\"";
    auto pos                    = json.find(label_key);
    if (pos != std::string::npos) {
        pos += label_key.size();
        const auto end = json.find('"', pos);
        if (end != std::string::npos) {
            result.label = json.substr(pos, end - pos);
        }
    }

    // Extract confidence.
    const std::string conf_key = "\"confidence\":";
    pos                        = json.find(conf_key);
    if (pos != std::string::npos) {
        pos += conf_key.size();
        try {
            std::size_t consumed = 0;
            result.confidence    = std::stod(json.substr(pos), &consumed);
            // Clamp to [0, 1].
            result.confidence = std::max(0.0, std::min(1.0, result.confidence));
        } catch (...) {
            result.confidence = 0.0;
        }
    }

    if (result.label.empty()) {
        result.label      = "unknown";
        result.confidence = 0.0;
    }
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// automlFallback
// ──────────────────────────────────────────────────────────────────────────────

PatternResult LoRAPatternClassifier::automlFallback(const std::vector<DataPoint> &events,
                                                    const std::string &adapter_id) const {
    // Reserve for a typical compact event payload (~8 mixed fields/event).
    constexpr std::size_t kExpectedFieldsPerEvent = 8;
    constexpr double kHighVarianceThreshold       = 0.60;
    constexpr double kTrendThreshold              = 0.80;
    // Weighted blend tuned to keep fallback stable while still reacting to
    // signal quality. Coverage gets the highest weight to prefer richer events.
    constexpr double kPriorWeight      = 0.20;
    constexpr double kCoverageWeight   = 0.25;
    constexpr double kTemporalWeight   = 0.20;
    constexpr double kDispersionWeight = 0.20;
    constexpr double kMagnitudeWeight  = 0.15;
    // Keep confidence bounded away from hard 0/1 for conservative fallback use.
    constexpr double kMinFallbackConfidence = 0.05;
    constexpr double kMaxFallbackConfidence = 0.99;

    const auto clamp01 = [](double v) { return std::max(0.0, std::min(1.0, v)); };

    // Derive fallback label from registered adapter domain whenever possible.
    std::string inferred_label = "unknown";
    if (!adapter_id.empty()) {
        const auto it = std::find_if(domains_.begin(), domains_.end(),
                                     [&](const AdapterDomain &d) { return d.adapter_id == adapter_id; });
        if (it != domains_.end() && !it->domain.empty()) {
            inferred_label = it->domain;
        }
    }

    std::size_t total_fields   = 0;
    std::size_t numeric_fields = 0;
    std::vector<double> numeric_values;
    numeric_values.reserve(events.size() * kExpectedFieldsPerEvent);
    int monotonic_steps = 0;
    int step_count      = 0;
    for (std::size_t i = 0; i < events.size(); ++i) {
        const auto &ev = events[i];
        total_fields += ev.fields.size();

        if (i > 0) {
            ++step_count;
            if (events[i - 1].timestamp_ms <= ev.timestamp_ms) {
                ++monotonic_steps;
            }
        }

        for (const auto &[_, value] : ev.fields) {
            if (const auto *d = std::get_if<double>(&value)) {
                ++numeric_fields;
                numeric_values.push_back(*d);
            } else if (const auto *iv = std::get_if<int64_t>(&value)) {
                ++numeric_fields;
                numeric_values.push_back(static_cast<double>(*iv));
            } else if (const auto *bv = std::get_if<bool>(&value)) {
                ++numeric_fields;
                numeric_values.push_back(*bv ? 1.0 : 0.0);
            }
        }
    }

    const double prior_conf = clamp01(cfg_.fallback_confidence);
    const double coverage_score
        = (total_fields == 0) ? 0.0 : clamp01(static_cast<double>(numeric_fields) / static_cast<double>(total_fields));
    const double temporal_score
        = (step_count == 0) ? 0.5 : clamp01(static_cast<double>(monotonic_steps) / static_cast<double>(step_count));

    double magnitude_score  = 0.0;
    double dispersion_score = 0.0;
    if (!numeric_values.empty()) {
        const double sum  = std::accumulate(numeric_values.begin(), numeric_values.end(), 0.0);
        const double mean = sum / static_cast<double>(numeric_values.size());
        double var        = 0.0;
        for (double v : numeric_values) {
            const double d = v - mean;
            var += d * d;
        }
        var /= static_cast<double>(numeric_values.size());
        const double stddev = std::sqrt(var);
        magnitude_score     = clamp01(std::abs(mean) / (1.0 + std::abs(mean)));
        dispersion_score    = clamp01(stddev / (1.0 + stddev));

        if (inferred_label == "unknown") {
            if (dispersion_score >= kHighVarianceThreshold) {
                inferred_label = "high_variance_pattern";
            } else if (temporal_score >= kTrendThreshold) {
                inferred_label = "trend_pattern";
            } else {
                inferred_label = "stable_pattern";
            }
        }
    }

    double confidence = kPriorWeight * prior_conf + kCoverageWeight * coverage_score + kTemporalWeight * temporal_score
                        + kDispersionWeight * dispersion_score + kMagnitudeWeight * magnitude_score;
    confidence        = std::max(kMinFallbackConfidence, std::min(kMaxFallbackConfidence, confidence));

    PatternResult result;
    result.label         = inferred_label;
    result.confidence    = confidence;
    result.adapter_id    = adapter_id;
    result.used_fallback = true;
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// cosineSimilarity
// ──────────────────────────────────────────────────────────────────────────────

/*static*/ double LoRAPatternClassifier::cosineSimilarity(const std::vector<double> &a, const std::vector<double> &b) {
    if (a.empty() || a.size() != b.size()) {
        return 0.0;
    }

    double dot = 0.0, na = 0.0, nb = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if (na <= 0.0 || nb <= 0.0) {
        return 0.0;
    }
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

// ──────────────────────────────────────────────────────────────────────────────
// selectAdapter
// ──────────────────────────────────────────────────────────────────────────────

std::string LoRAPatternClassifier::selectAdapter(const std::string &context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (domains_.empty()) {
        return "";
    }
    if (!embedding_fn_) {
        return domains_.front().adapter_id;
    }

    std::vector<double> ctx_emb;
    try { ctx_emb = embedding_fn_(context); }
    catch (...) { return domains_.front().adapter_id; }

    std::string best_id;
    double best_sim = -1.0;
    for (const auto &d : domains_) {
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

PatternResult LoRAPatternClassifier::classify(const std::vector<DataPoint> &events, const std::string &adapter_id) {
    // Determine adapter (unlocked for selectAdapter to take its own lock).
    std::string aid = adapter_id;
    if (aid.empty()) {
        aid = selectAdapter(buildPrompt(events, ""));
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!inference_fn_) {
        return automlFallback(events, aid);
    }

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

std::vector<PatternResult> LoRAPatternClassifier::batchClassify(const std::vector<DataPoint> &events) {
    if (events.empty()) {
        return {};
    }

    const std::size_t n       = events.size();
    const std::size_t workers = std::max<std::size_t>(1, cfg_.max_parallel_workers);

    std::vector<PatternResult> results(n);
    std::vector<std::future<PatternResult>> futures;
    futures.reserve(workers);

    std::size_t i = 0;
    while (i < n) {
        // Dispatch up to `workers` tasks.
        futures.clear();
        const std::size_t batch_end = std::min(i + workers, n);
        for (std::size_t j = i; j < batch_end; ++j) {
            const DataPoint &ev = events[j];
            futures.push_back(
                std::async(std::launch::async, [this, &ev]() -> PatternResult { return classify({ev}); }));
        }
        // Collect results in order.
        for (std::size_t j = i; j < batch_end; ++j) {
            results[j] = futures[j - i].get();
        }
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

