/**
 * @file rag_quality_monitor.cpp
 * @brief Per-layer RAG handoff quality monitor — implementation.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "rag/rag_quality_monitor.h"
#include "utils/logger.h"

#include <cmath>
#include <sstream>

namespace themis {
namespace rag {

// ─────────────────────────────────────────────────────────────────────────────
// recordMetrics
// ─────────────────────────────────────────────────────────────────────────────

void RagQualityMonitor::recordMetrics(const LayerQualityMetrics& m)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (buffer_.size() >= kWindowSize) {
        buffer_.pop_front();
    }
    buffer_.push_back(m);
}

// ─────────────────────────────────────────────────────────────────────────────
// emitPrometheusGauges
// ─────────────────────────────────────────────────────────────────────────────

void RagQualityMonitor::emitPrometheusGauges() const
{
    LayerQualityMetrics latest{};
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (buffer_.empty()) {
            return;
        }
        latest = buffer_.back();
    }

    // Helper lambda — emits one gauge family (HELP + TYPE + value line).
    auto emit = [](const char* name, const char* help, float value) {
        THEMIS_INFO("# HELP {} {}", name, help);
        THEMIS_INFO("# TYPE {} gauge", name);
        THEMIS_INFO("{} {}", name, value);
    };

    emit("rag_ann_recall_at_10",
         "ANN Recall@10 over rolling window",
         latest.ann_recall_at_10);

    emit("rag_tensor_routing_accuracy",
         "Tensor routing accuracy over rolling window",
         latest.tensor_routing_accuracy);

    emit("rag_graph_provenance_precision",
         "Graph provenance precision over rolling window",
         latest.graph_provenance_precision);

    emit("rag_llm_rouge_l",
         "LLM ROUGE-L score over rolling window",
         latest.llm_rouge_l);

    emit("rag_query_latency_ms",
         "End-to-end query latency (ms) over rolling window",
         latest.query_latency_ms);

    emit("rag_guardrail_deny_rate",
         "Fraction of queries denied by retrieval guardrail",
         latest.guardrail_deny_rate);
}

// ─────────────────────────────────────────────────────────────────────────────
// checkAnomalies
// ─────────────────────────────────────────────────────────────────────────────

template <typename Selector>
RagQualityMonitor::Stats RagQualityMonitor::computeStats(Selector selector) const
{
    // Caller must hold mutex_.
    if (buffer_.empty()) {
        return {0.0f, 0.0f};
    }

    double sum = 0.0;
    for (const auto& s : buffer_) {
        sum += static_cast<double>(selector(s));
    }
    const double mean = sum / static_cast<double>(buffer_.size());

    double sq_sum = 0.0;
    for (const auto& s : buffer_) {
        const double diff = static_cast<double>(selector(s)) - mean;
        sq_sum += diff * diff;
    }
    const double variance = sq_sum / static_cast<double>(buffer_.size());
    const double stddev   = std::sqrt(variance);

    return {static_cast<float>(mean), static_cast<float>(stddev)};
}

std::vector<std::string> RagQualityMonitor::checkAnomalies() const
{
    std::vector<std::string> hints;

    std::lock_guard<std::mutex> lk(mutex_);
    if (buffer_.size() < 2) {
        return hints;
    }

    const LayerQualityMetrics& latest = buffer_.back();
    constexpr float kZThreshold = 3.0f;

    // ── ann_recall_at_10 (low value is anomalous) ────────────────────────────
    {
        auto stats = computeStats([](const LayerQualityMetrics& m) {
            return m.ann_recall_at_10;
        });
        if (stats.stddev > 0.0f) {
            const float z = (latest.ann_recall_at_10 - stats.mean) / stats.stddev;
            if (z <= -kZThreshold) {
                hints.emplace_back("low_recall");
                THEMIS_WARN("[QUALITY] Anomaly detected: low_recall");
            }
        }
    }

    // ── query_latency_ms (high value is anomalous) ───────────────────────────
    {
        auto stats = computeStats([](const LayerQualityMetrics& m) {
            return m.query_latency_ms;
        });
        if (stats.stddev > 0.0f) {
            const float z = (latest.query_latency_ms - stats.mean) / stats.stddev;
            if (z >= kZThreshold) {
                hints.emplace_back("high_latency");
                THEMIS_WARN("[QUALITY] Anomaly detected: high_latency");
            }
        }
    }

    // ── guardrail_deny_rate (high value is anomalous) ────────────────────────
    {
        auto stats = computeStats([](const LayerQualityMetrics& m) {
            return m.guardrail_deny_rate;
        });
        if (stats.stddev > 0.0f) {
            const float z = (latest.guardrail_deny_rate - stats.mean) / stats.stddev;
            if (z >= kZThreshold) {
                hints.emplace_back("guardrail_deny_rate");
                THEMIS_WARN("[QUALITY] Anomaly detected: guardrail_deny_rate");
            }
        }
    }

    return hints;
}

} // namespace rag
} // namespace themis
