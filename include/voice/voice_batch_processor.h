/**
 * @file voice_batch_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Batch audio processing + quality metrics – Phase 10 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <future>
#include <atomic>
#include <mutex>
#include <memory>
#include <nlohmann/json.hpp>
#include "voice/audio_preprocessing.h"
#include "content/stt_processor.h"

namespace themis { namespace voice {
using json = nlohmann::json;

// Batch job item
struct BatchAudioItem {
    std::string item_id;
    std::vector<uint8_t> audio_data;
    int sample_rate = 16000;
    std::string transcript_reference;  // Reference text for WER calculation
    json metadata;
};

// Batch job result
struct BatchItemResult {
    std::string item_id;
    bool success = false;
    std::string error_message;
    std::string transcript;            // Produced transcript (empty if no STT)
    float wer_score = -1.0f;          // Word Error Rate (0–1), -1 if no reference
    float pesq_score = -1.0f;         // PESQ-like score (1–5), -1 if not computed
    int64_t processing_time_ms = 0;
    PreprocessingResult preprocessing;
};

// Batch job
struct BatchJob {
    std::string job_id;
    std::vector<BatchAudioItem> items;
    size_t batch_size = 8;            // Items per parallel batch
};

// Batch job status
enum class BatchJobStatus {
    PENDING, RUNNING, COMPLETED, FAILED
};
std::string batchJobStatusToString(BatchJobStatus s);

// Batch summary
struct BatchSummary {
    std::string job_id;
    BatchJobStatus status = BatchJobStatus::PENDING;
    size_t total_items = 0;
    size_t completed_items = 0;
    size_t failed_items = 0;
    float avg_wer = -1.0f;
    float avg_pesq = -1.0f;
    int64_t total_processing_ms = 0;
    int64_t elapsed_ms = 0;
};

// Audio quality metrics (PESQ-inspired approximation, no external lib)
struct AudioQualityMetrics {
    float pesq_mos = -1.0f;          // Estimated MOS from PESQ (1–5)
    float wer = -1.0f;               // Word Error Rate (0–1)
    float snr_db = 0.0f;             // Signal-to-Noise Ratio in dB
    float rms_energy = 0.0f;
    float clipping_ratio = 0.0f;     // Fraction of clipped samples
    std::string quality_label;       // "poor"/"fair"/"good"/"excellent"
};

// Batch processor config
struct BatchProcessorConfig {
    size_t max_concurrent_jobs = 4;
    size_t default_batch_size = 8;
    bool compute_quality_metrics = true;
    bool compute_wer = false;        // WER only if reference text provided
    int64_t item_timeout_ms = 30000;
};

// Progress callback: called with (job_id, completed, total)
using BatchProgressCallback = std::function<void(const std::string&, size_t, size_t)>;

// VoiceBatchProcessor: Phase 10 production component
/** @brief VoiceBatchProcessor: Phase 10 production component. */
class VoiceBatchProcessor {
public:
    explicit VoiceBatchProcessor(const BatchProcessorConfig& config = {});
    ~VoiceBatchProcessor() = default;

    // Submit a batch job, returns job ID
    std::string submitBatch(
        const std::vector<BatchAudioItem>& items,
        BatchProgressCallback progress_cb = nullptr
    );

    // Process batch synchronously and return all results
    std::vector<BatchItemResult> processBatchSync(
        const std::vector<BatchAudioItem>& items,
        BatchProgressCallback progress_cb = nullptr
    );

    // Process a single item (used internally)
    BatchItemResult processItem(const BatchAudioItem& item);

    // Quality metrics
    AudioQualityMetrics computeQualityMetrics(
        const std::vector<uint8_t>& audio_data,
        int sample_rate = 16000
    ) const;

    // WER computation (pure text comparison)
    float computeWER(const std::string& reference, const std::string& hypothesis) const;

    // PESQ-like estimation from SNR (approximation, no external lib required)
    float estimatePESQ(float snr_db) const;

    // SNR estimation from audio
    float estimateSNR(const std::vector<uint8_t>& audio_data, int sample_rate) const;

    // Load test helper: simulate N concurrent requests
    BatchSummary runLoadTest(
        size_t num_concurrent,
        const BatchAudioItem& template_item
    );

    // Statistics
    json getStatistics() const;
    BatchSummary getJobSummary(const std::string& job_id) const;

    /**
     * @brief Attach an STT processor for real-time streaming transcription.
     *
     * When set, @p processItem will call @c STTProcessor::streamTranscribe on
     * each audio item and populate @c BatchItemResult::transcript with the
     * word-by-word result.  Pass @c nullptr to detach.
     *
     * @param stt  Initialized STTProcessor instance, or nullptr to disable.
     */
    void setSTTProcessor(std::shared_ptr<content::STTProcessor> stt);

private:
    BatchProcessorConfig config_;
    AudioPreprocessingPipeline preprocessor_;
    std::shared_ptr<content::STTProcessor> stt_processor_;
    mutable std::mutex mutex_;

    std::atomic<uint64_t> jobs_submitted_{0};
    std::atomic<uint64_t> items_processed_{0};
    std::atomic<uint64_t> items_failed_{0};

    std::map<std::string, BatchSummary> job_summaries_;

    static std::string generateJobId();
    std::vector<std::string> tokenize(const std::string& text) const;
    float computeRMS(const std::vector<float>& samples) const;
    float computeNoiseFloor(const std::vector<float>& samples, size_t window_size = 800) const;
    std::vector<float> rawToFloat(const std::vector<uint8_t>& data) const;
};

}} // namespace themis::voice
