/**
 * @file rag_quality_monitor.h
 * @brief Per-layer RAG handoff quality monitor with Prometheus gauge emission
 *        and rolling z-score anomaly detection.
 *
 * Metrics tracked per sample:
 *  - ANN Recall\@10
 *  - Tensor routing accuracy
 *  - Graph provenance precision
 *  - LLM ROUGE-L
 *  - Query latency (ms)
 *  - Guardrail deny rate
 *
 * Anomaly detection uses a z-score ≥ 3 criterion over a rolling 5-minute
 * window (300 samples at 1 sample/second) and emits structured root-cause
 * hints via THEMIS_WARN.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ─────────────────────────────────────────────────────────────────────────────
// LayerQualityMetrics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One sample of per-layer RAG quality metrics.
 */
struct LayerQualityMetrics {
    /// ANN Recall\@10 [0.0, 1.0].
    float ann_recall_at_10{0.0f};

    /// Tensor routing accuracy [0.0, 1.0].
    float tensor_routing_accuracy{0.0f};

    /// Graph provenance precision [0.0, 1.0].
    float graph_provenance_precision{0.0f};

    /// LLM ROUGE-L score [0.0, 1.0].
    float llm_rouge_l{0.0f};

    /// End-to-end query latency in milliseconds.
    float query_latency_ms{0.0f};

    /// Fraction of queries denied by the retrieval guardrail [0.0, 1.0].
    float guardrail_deny_rate{0.0f};
};

// ─────────────────────────────────────────────────────────────────────────────
// RagQualityMonitor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe RAG quality monitor with Prometheus gauge emission and
 *        rolling z-score anomaly detection.
 *
 * @code{.cpp}
 * themis::rag::RagQualityMonitor monitor;
 * monitor.recordMetrics({0.85f, 0.92f, 0.88f, 0.74f, 120.0f, 0.01f});
 * monitor.emitPrometheusGauges();
 * auto anomalies = monitor.checkAnomalies();
 * @endcode
 */
class RagQualityMonitor {
public:
    /// Maximum samples retained in the rolling window (≈ 5 min at 1 Hz).
    static constexpr std::size_t kWindowSize = 300;

    RagQualityMonitor() = default;

    /**
     * @brief Records a quality sample into the rolling ring buffer.
     *
     * If the buffer already holds kWindowSize samples the oldest entry is
     * evicted (FIFO).  Thread-safe.
     *
     * @param m Metrics sample to record.
     */
    void recordMetrics(const LayerQualityMetrics& m);

    /**
     * @brief Emits the most-recent sample as Prometheus gauge lines to
     *        THEMIS_INFO.
     *
     * Emits in the standard text exposition format (no external library):
     * @verbatim
     * # HELP rag_ann_recall_at_10 ANN Recall@10 over rolling window
     * # TYPE rag_ann_recall_at_10 gauge
     * rag_ann_recall_at_10 0.85
     * @endverbatim
     *
     * No-op when the ring buffer is empty.  Thread-safe (takes shared lock).
     */
    void emitPrometheusGauges() const;

    /**
     * @brief Computes per-metric rolling z-scores and returns root-cause hints
     *        for any metric whose latest sample deviates by ≥ 3 standard
     *        deviations from the window mean.
     *
     * Root-cause hint mapping:
     *  - @c ann_recall_at_10 low  → @c "low_recall"
     *  - @c query_latency_ms high → @c "high_latency"
     *  - @c guardrail_deny_rate high → @c "guardrail_deny_rate"
     *
     * THEMIS_WARN is emitted for each detected anomaly.  Thread-safe.
     *
     * @return Vector of root-cause hint strings (may be empty).
     */
    std::vector<std::string> checkAnomalies() const;

private:
    mutable std::mutex             mutex_;
    std::deque<LayerQualityMetrics> buffer_;

    // ── Internal helpers ─────────────────────────────────────────────────────

    struct Stats { float mean; float stddev; };

    /// Computes mean and sample stddev for a metric extracted by @p selector.
    template <typename Selector>
    Stats computeStats(Selector selector) const;
};

} // namespace rag
} // namespace themis
