/**
 * @file voice_batch_processor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_batch_processor.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <random>
#include <iomanip>

namespace themis { namespace voice {

// ---- Free functions ----

std::string batchJobStatusToString(BatchJobStatus s) {
    switch (s) {
        case BatchJobStatus::PENDING:   return "pending";
        case BatchJobStatus::RUNNING:   return "running";
        case BatchJobStatus::COMPLETED: return "completed";
        case BatchJobStatus::FAILED:    return "failed";
        default:                        return "unknown";
    }
}

// ---- VoiceBatchProcessor ----

VoiceBatchProcessor::VoiceBatchProcessor(const BatchProcessorConfig& config)
    : config_(config) {}

std::string VoiceBatchProcessor::generateJobId() {
    static std::atomic<uint64_t> counter{0};
    uint64_t id = ++counter;
    std::ostringstream ss;
    ss << "batch-job-" << std::setfill('0') << std::setw(8) << id;
    return ss.str();
}

std::string VoiceBatchProcessor::submitBatch(
    const std::vector<BatchAudioItem>& items,
    BatchProgressCallback progress_cb)
{
    std::string job_id = generateJobId();
    ++jobs_submitted_;

    BatchSummary summary;
    summary.job_id = job_id;
    summary.status = BatchJobStatus::RUNNING;
    summary.total_items = items.size();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        job_summaries_[job_id] = summary;
    }

    // Process synchronously (async dispatch can be added as a future enhancement)
    auto results = processBatchSync(items, progress_cb);

    float total_wer = 0.0f;
    float total_pesq = 0.0f;
    int wer_count = 0;
    int pesq_count = 0;

    for (const auto& r : results) {
        if (r.success) {
            ++summary.completed_items;
        } else {
            ++summary.failed_items;
        }
        if (r.wer_score >= 0.0f) {
            total_wer += r.wer_score;
            ++wer_count;
        }
        if (r.pesq_score >= 0.0f) {
            total_pesq += r.pesq_score;
            ++pesq_count;
        }
        summary.total_processing_ms += r.processing_time_ms;
    }

    summary.avg_wer  = wer_count  > 0 ? total_wer  / static_cast<float>(wer_count)  : -1.0f;
    summary.avg_pesq = pesq_count > 0 ? total_pesq / static_cast<float>(pesq_count) : -1.0f;
    summary.status = summary.failed_items == items.size() ? BatchJobStatus::FAILED : BatchJobStatus::COMPLETED;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        job_summaries_[job_id] = summary;
    }

    return job_id;
}

std::vector<BatchItemResult> VoiceBatchProcessor::processBatchSync(
    const std::vector<BatchAudioItem>& items,
    BatchProgressCallback progress_cb)
{
    std::string job_id = "sync";
    std::vector<BatchItemResult> results = {};

    results.reserve(items.size());

    size_t batch_size = config_.default_batch_size > 0 ? config_.default_batch_size : 1;

    for (size_t i = 0; i < items.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, items.size());
        for (size_t j = i; j < end; ++j) {
            results.push_back(processItem(items[j]));
        }
        if (progress_cb) {
            progress_cb(job_id, end, items.size());
        }
    }

    return results;
}

void VoiceBatchProcessor::setSTTProcessor(std::shared_ptr<content::STTProcessor> stt) {
    std::lock_guard<std::mutex> lock(mutex_);
    stt_processor_ = std::move(stt);
}

BatchItemResult VoiceBatchProcessor::processItem(const BatchAudioItem& item) {
    using namespace std::chrono;
    auto start = steady_clock::now();

    BatchItemResult result;
    result.item_id = item.item_id;

    // Capture a local copy of the STT processor to avoid holding the lock
    // during potentially slow transcription.
    std::shared_ptr<content::STTProcessor> stt;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stt = stt_processor_;
    }

    try {
        // Preprocess audio
        if (!item.audio_data.empty()) {
            result.preprocessing = preprocessor_.process(item.audio_data, item.sample_rate);
        }

        // Real-time streaming STT: transcribe word-by-word using a sliding
        // window and accumulate the segments into a single transcript string.
        if (stt && !item.audio_data.empty()) {
            std::string transcript;
            bool ok = stt->streamTranscribe(
                item.audio_data,
                [&transcript](const content::TranscriptionSegment& seg) {
                    if (!transcript.empty()) {
                        transcript += ' ';
                    }
                    transcript += seg.text;
                }
            );
            if (ok) {
                result.transcript = std::move(transcript);
            }
        }

        // Compute quality metrics
        if (config_.compute_quality_metrics && !item.audio_data.empty()) {
            auto metrics = computeQualityMetrics(item.audio_data, item.sample_rate);
            result.pesq_score = metrics.pesq_mos;
        }

        // Compute WER if reference provided
        if (config_.compute_wer && !item.transcript_reference.empty() && !result.transcript.empty()) {
            result.wer_score = computeWER(item.transcript_reference, result.transcript);
        }

        result.success = true;
        ++items_processed_;
    } catch (const std::exception& ex) {
        result.success = false;
        result.error_message = ex.what();
        ++items_failed_;
    }

    auto elapsed = steady_clock::now() - start;
    result.processing_time_ms = duration_cast<milliseconds>(elapsed).count();
    return result;
}

AudioQualityMetrics VoiceBatchProcessor::computeQualityMetrics(
    const std::vector<uint8_t>& audio_data,
    int sample_rate) const
{
    AudioQualityMetrics metrics;

    if (audio_data.empty()) {
        metrics.quality_label = "poor";
        return metrics;
    }

    auto samples = rawToFloat(audio_data);
    if (samples.empty()) {
        metrics.quality_label = "poor";
        return metrics;
    }

    // RMS energy
    metrics.rms_energy = computeRMS(samples);

    // Clipping ratio: fraction of samples at or near full scale (±1.0)
    size_t clipped = 0;
    for (float s : samples) {
        if (std::fabs(s) >= 0.999f) {
          ++clipped;
        }
    }
    metrics.clipping_ratio = static_cast<float>(clipped) / static_cast<float>(samples.size());

    // SNR
    metrics.snr_db = estimateSNR(audio_data, sample_rate);

    // PESQ MOS estimate
    metrics.pesq_mos = estimatePESQ(metrics.snr_db);

    // Quality label
    if (metrics.pesq_mos < 2.0f) {
      metrics.quality_label = "poor";
    }
    else if (metrics.pesq_mos < 3.0f) metrics.quality_label = "fair";
    else if (metrics.pesq_mos < 4.0f) metrics.quality_label = "good";
    else                               metrics.quality_label = "excellent";

    return metrics;
}

float VoiceBatchProcessor::computeWER(
    const std::string& reference,
    const std::string& hypothesis) const
{
    auto ref_tokens = tokenize(reference);
    auto hyp_tokens = tokenize(hypothesis);

    if (ref_tokens.empty()) {
      return 0.0f;
    }

    size_t R = ref_tokens.size();
    size_t H = hyp_tokens.size();

    // Levenshtein distance via DP
    std::vector<std::vector<size_t>> dp(R + 1, std::vector<size_t>(H + 1, 0));
    for (size_t i = 0; i <= R; ++i) {
      dp[i][0] = i;
    }
    for (size_t j = 0; j <= H; ++j) {
      dp[0][j] = j;
    }

    for (size_t i = 1; i <= R; ++i) {
        for (size_t j = 1; j <= H; ++j) {
            if (ref_tokens[i - 1] == hyp_tokens[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return static_cast<float>(dp[R][H]) / static_cast<float>(R);
}

float VoiceBatchProcessor::estimatePESQ([[maybe_unused]] float snr_db) const {
    // Linear approximation: clamp(1.0 + snr_db * 0.07, 1.0, 5.0)
    // SNR 0dB→1.0, SNR 28.57dB→3.0, SNR 57.14dB→5.0
    float pesq = 1.0f + snr_db * 0.07f;
    if (pesq < 1.0f) {
      pesq = 1.0f;
    }
    if (pesq > 5.0f) {
      pesq = 5.0f;
    }
    return pesq;
}

float VoiceBatchProcessor::estimateSNR(
    const std::vector<uint8_t>& audio_data,
    int /*sample_rate*/) const
{
    if (audio_data.empty()) {
      return 0.0f;
    }

    auto samples = rawToFloat(audio_data);
    if (samples.empty()) {
      return 0.0f;
    }

    float signal_rms = computeRMS(samples);
    float noise_floor = computeNoiseFloor(samples);

    if (noise_floor <= 0.0f || signal_rms <= 0.0f) {
      return 0.0f;
    }

    return 20.0f * std::log10(signal_rms / noise_floor);
}

BatchSummary VoiceBatchProcessor::runLoadTest(
    size_t num_concurrent,
    const BatchAudioItem& template_item)
{
    std::vector<BatchAudioItem> items;
    items.reserve(num_concurrent);
    for (size_t i = 0; i < num_concurrent; ++i) {
        BatchAudioItem item = template_item;
        item.item_id = template_item.item_id + "-" + std::to_string(i);
        items.push_back(std::move(item));
    }

    using namespace std::chrono;
    auto t0 = steady_clock::now();
    std::string job_id = submitBatch(items);
    auto elapsed = duration_cast<milliseconds>(steady_clock::now() - t0).count();

    BatchSummary summary;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = job_summaries_.find(job_id);
        if (it != job_summaries_.end()) {
            summary = it->second;
        }
    }
    summary.elapsed_ms = elapsed;
    return summary;
}

json VoiceBatchProcessor::getStatistics() const {
    json stats;
    stats["jobs_submitted"] = jobs_submitted_.load();
    stats["items_processed"] = items_processed_.load();
    stats["items_failed"] = items_failed_.load();
    return stats;
}

BatchSummary VoiceBatchProcessor::getJobSummary(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = job_summaries_.find(job_id);
    if (it == job_summaries_.end()) {
        BatchSummary empty;
        empty.job_id = job_id;
        empty.status = BatchJobStatus::FAILED;
        return empty;
    }
    return it->second;
}

// ---- Private helpers ----

std::vector<std::string> VoiceBatchProcessor::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        // Lowercase
        std::transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        // Strip leading/trailing punctuation
        while (!word.empty() && std::ispunct(static_cast<unsigned char>(word.front()))) {
            word.erase(word.begin());
        }
        while (!word.empty() && std::ispunct(static_cast<unsigned char>(word.back()))) {
            word.pop_back();
        }
        if (!word.empty()) {
          tokens.push_back(word);
        }
    }
    return tokens;
}

float VoiceBatchProcessor::computeRMS(const std::vector<float>& samples) const {
    if (samples.empty()) {
      return 0.0f;
    }
    float sum = 0.0f;
    for (float s : samples) {
      sum += s * s;
    }
    return std::sqrt(sum / static_cast<float>(samples.size()));
}

float VoiceBatchProcessor::computeNoiseFloor(
    const std::vector<float>& samples,
    size_t window_size) const
{
    if (samples.empty()) {
      return 0.0f;
    }
    if (window_size == 0) {
      window_size = 800;
    }

    // Compute RMS energy per window frame
    std::vector<float> frame_rms = {};

    for (size_t i = 0; i + window_size <= samples.size(); i += window_size) {
        float sum = 0.0f;
        for (size_t j = i; j < i + window_size; ++j) {
            sum += samples[j] * samples[j];
        }
        frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
    }
    if (frame_rms.empty()) {
      return 0.0f;
    }

    // Take the quietest 10% of frames as noise floor estimate
    std::sort(frame_rms.begin(), frame_rms.end());
    size_t n = std::max<size_t>(1, frame_rms.size() / 10);
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
      sum += frame_rms[i];
    }
    float floor = sum / static_cast<float>(n);
    static constexpr float MIN_NOISE_FLOOR = 1e-7f; // Minimum value to avoid divide-by-zero in SNR
    return (floor > 0.0f) ? floor : MIN_NOISE_FLOOR;
}

std::vector<float> VoiceBatchProcessor::rawToFloat(const std::vector<uint8_t>& data) const {
    // Treat pairs of bytes as little-endian int16, convert to [-1, 1] float
    if (data.size() < 2) {
        // Fallback: treat each byte as unsigned 8-bit sample
        std::vector<float> samples = {};

        samples.reserve(data.size());
        for (uint8_t b : data) {
            samples.push_back((static_cast<float>(b) - 128.0f) / 128.0f);
        }
        return samples;
    }

    std::vector<float> samples = {};

    samples.reserve(data.size() / 2);
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        int16_t s = static_cast<int16_t>(
            static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i + 1]) << 8)
        );
        samples.push_back(static_cast<float>(s) / 32768.0f);
    }
    return samples;
}

}} // namespace themis::voice
