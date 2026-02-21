/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audio_preprocessing.cpp                            ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 19:14:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     319                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file audio_preprocessing.cpp
 * @brief Audio preprocessing pipeline implementation (Phase 1 production readiness)
 */

#include "voice/audio_preprocessing.h"
#include <chrono>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>

namespace themis { namespace voice {

AudioPreprocessingPipeline::AudioPreprocessingPipeline(const PreprocessingOptions& opts)
    : opts_(opts) {}

// Convert raw bytes (16-bit PCM) to normalized float [-1, 1]
std::vector<float> AudioPreprocessingPipeline::convertRawToFloat(
    const std::vector<uint8_t>& raw, int bits_per_sample) const
{
    if (raw.empty()) return {};
    if (bits_per_sample == 16) {
        size_t num_samples = raw.size() / 2;
        std::vector<float> result(num_samples);
        for (size_t i = 0; i < num_samples; ++i) {
            int16_t sample = static_cast<int16_t>(raw[2 * i] | (raw[2 * i + 1] << 8));
            result[i] = sample / 32768.0f;
        }
        return result;
    }
    // Fallback: treat bytes as unsigned 8-bit
    std::vector<float> result(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        result[i] = (raw[i] / 128.0f) - 1.0f;
    }
    return result;
}

float AudioPreprocessingPipeline::computeRMS(const std::vector<float>& samples) const {
    if (samples.empty()) return 0.0f;
    float sum_sq = 0.0f;
    for (float s : samples) sum_sq += s * s;
    return std::sqrt(sum_sq / static_cast<float>(samples.size()));
}

float AudioPreprocessingPipeline::computeNoiseFloor(const std::vector<float>& samples) const {
    if (samples.empty()) return 0.0f;
    // Estimate noise from the first 100ms-equivalent samples (first 10% of buffer)
    size_t noise_end = std::max<size_t>(1, samples.size() / 10);
    float sum_sq = 0.0f;
    for (size_t i = 0; i < noise_end; ++i) sum_sq += samples[i] * samples[i];
    return std::sqrt(sum_sq / static_cast<float>(noise_end));
}

std::vector<float> AudioPreprocessingPipeline::applyHighPassFilter(
    const std::vector<float>& samples, float cutoff_hz, int sample_rate) const
{
    if (samples.empty()) return {};
    // Simple first-order high-pass IIR filter: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
    float rc = 1.0f / (2.0f * static_cast<float>(M_PI) * cutoff_hz);
    float dt = 1.0f / static_cast<float>(sample_rate);
    float alpha = rc / (rc + dt);

    std::vector<float> output(samples.size());
    output[0] = samples[0];
    for (size_t i = 1; i < samples.size(); ++i) {
        output[i] = alpha * (output[i - 1] + samples[i] - samples[i - 1]);
    }
    return output;
}

AudioFrame AudioPreprocessingPipeline::applyNoiseReduction(const AudioFrame& frame, float strength) {
    AudioFrame result = frame;
    if (frame.samples.empty()) return result;

    float noise_floor = computeNoiseFloor(frame.samples);
    float threshold = noise_floor * strength;

    // Spectral subtraction approximation: attenuate samples near noise floor
    for (float& s : result.samples) {
        float abs_s = std::abs(s);
        if (abs_s < threshold) {
            s *= (abs_s / threshold) * (1.0f - strength);
        }
    }
    return result;
}

AudioFrame AudioPreprocessingPipeline::applyEchoCancellation(
    const AudioFrame& input, const AudioFrame& reference)
{
    AudioFrame result = input;
    if (input.samples.empty() || reference.samples.empty()) return result;

    // Simple subtraction-based echo cancellation
    size_t len = std::min(input.samples.size(), reference.samples.size());
    float ref_rms = computeRMS(reference.samples);
    float input_rms = computeRMS(input.samples);
    float scale = (ref_rms > 1e-6f) ? (input_rms / ref_rms) * 0.3f : 0.0f;

    for (size_t i = 0; i < len; ++i) {
        result.samples[i] = input.samples[i] - scale * reference.samples[i];
        result.samples[i] = std::clamp(result.samples[i], -1.0f, 1.0f);
    }
    return result;
}

float AudioPreprocessingPipeline::detectVoiceActivity(const AudioFrame& frame) {
    if (frame.samples.empty()) return 0.0f;

    // Chunk audio into 10ms windows and check RMS against threshold
    int window_samples = std::max(1, frame.sample_rate / 100);
    size_t num_windows = frame.samples.size() / window_samples;
    if (num_windows == 0) {
        float rms = computeRMS(frame.samples);
        return (rms > opts_.vad_threshold * 0.1f) ? 1.0f : 0.0f;
    }

    float noise_floor = computeNoiseFloor(frame.samples);
    float dynamic_threshold = std::max(opts_.vad_threshold * 0.05f, noise_floor * 3.0f);

    size_t active_windows = 0;
    for (size_t w = 0; w < num_windows; ++w) {
        float sum_sq = 0.0f;
        size_t start = w * window_samples;
        for (int s = 0; s < window_samples; ++s) {
            float v = frame.samples[start + s];
            sum_sq += v * v;
        }
        float window_rms = std::sqrt(sum_sq / window_samples);
        if (window_rms > dynamic_threshold) ++active_windows;
    }
    return static_cast<float>(active_windows) / static_cast<float>(num_windows);
}

AudioFrame AudioPreprocessingPipeline::normalize(const AudioFrame& frame, float target_rms) {
    AudioFrame result = frame;
    if (frame.samples.empty()) return result;

    float current_rms = computeRMS(frame.samples);
    if (current_rms < 1e-9f) return result;

    float gain = target_rms / current_rms;
    for (float& s : result.samples) {
        s = std::clamp(s * gain, -1.0f, 1.0f);
    }
    return result;
}

AudioFrame AudioPreprocessingPipeline::resample(const AudioFrame& frame, int target_sample_rate) {
    AudioFrame result = frame;
    result.sample_rate = target_sample_rate;

    if (frame.sample_rate == target_sample_rate || frame.samples.empty()) return result;

    double ratio = static_cast<double>(target_sample_rate) / static_cast<double>(frame.sample_rate);
    size_t out_size = static_cast<size_t>(frame.samples.size() * ratio);
    result.samples.resize(out_size);

    // Linear interpolation resampling
    for (size_t i = 0; i < out_size; ++i) {
        double src_pos = i / ratio;
        size_t idx0 = static_cast<size_t>(src_pos);
        size_t idx1 = std::min(idx0 + 1, frame.samples.size() - 1);
        float frac = static_cast<float>(src_pos - idx0);
        result.samples[i] = frame.samples[idx0] * (1.0f - frac) + frame.samples[idx1] * frac;
    }
    return result;
}

ConfidenceScore AudioPreprocessingPipeline::scoreConfidence(const AudioFrame& frame) {
    ConfidenceScore score;
    if (frame.samples.empty()) {
        score.quality_level = "low";
        return score;
    }

    float rms = computeRMS(frame.samples);
    float noise_floor = computeNoiseFloor(frame.samples);
    float snr = (noise_floor > 1e-9f) ? (rms / noise_floor) : 100.0f;

    // Map SNR to confidence
    score.acoustic = std::clamp(std::log10(snr + 1.0f) / 3.0f, 0.0f, 1.0f);
    score.language_model = 0.75f;  // Constant prior without LM
    score.overall = (score.acoustic * 0.7f + score.language_model * 0.3f);

    if (score.overall >= 0.75f) score.quality_level = "high";
    else if (score.overall >= 0.45f) score.quality_level = "medium";
    else score.quality_level = "low";

    return score;
}

LanguageDetectionResult AudioPreprocessingPipeline::detectLanguage(
    const AudioFrame& /*frame*/, const std::string& hint)
{
    // Language detection from raw audio requires a full ASR model (e.g. Whisper).
    // Return the hint if provided, otherwise default to English.
    LanguageDetectionResult result;
    if (hint == "auto" || hint.empty()) {
        result.detected_language = "en";
        result.confidence = 0.5f;
    } else {
        result.detected_language = hint;
        result.confidence = 0.9f;
    }
    result.alternatives = {{"en", 0.5f}};
    return result;
}

PreprocessingResult AudioPreprocessingPipeline::process(
    const std::vector<uint8_t>& raw_audio, int source_sample_rate)
{
    auto t0 = std::chrono::steady_clock::now();

    PreprocessingResult res;
    if (raw_audio.empty()) {
        res.success = true;
        res.error_message = "empty input";
        return res;
    }

    AudioFrame frame;
    frame.samples = convertRawToFloat(raw_audio);
    frame.sample_rate = source_sample_rate;
    frame.channels = 1;

    res = processFrame(frame);

    auto t1 = std::chrono::steady_clock::now();
    res.processing_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    return res;
}

PreprocessingResult AudioPreprocessingPipeline::processFrame(const AudioFrame& frame) {
    auto t0 = std::chrono::steady_clock::now();

    PreprocessingResult res;
    AudioFrame current = frame;

    // Apply pipeline stages
    if (opts_.enable_noise_reduction) {
        current = applyNoiseReduction(current, opts_.noise_reduction_strength);
    }
    if (opts_.enable_echo_cancellation) {
        // No reference frame here; echo cancellation is a no-op without reference
    }
    if (opts_.enable_vad) {
        res.voice_activity_ratio = detectVoiceActivity(current);
    }
    if (opts_.enable_normalization) {
        current = normalize(current, opts_.target_rms);
    }
    if (current.sample_rate != opts_.target_sample_rate) {
        current = resample(current, opts_.target_sample_rate);
    }

    res.detected_noise_level = computeNoiseFloor(frame.samples);
    res.processed_audio = current;
    res.success = true;

    auto t1 = std::chrono::steady_clock::now();
    res.processing_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    ++frames_processed_;
    total_processing_time_ms_ += static_cast<uint64_t>(res.processing_time_ms);

    res.diagnostics["frames_processed"] = frames_processed_;
    res.diagnostics["voice_activity_ratio"] = res.voice_activity_ratio;
    res.diagnostics["noise_level"] = res.detected_noise_level;

    return res;
}

json AudioPreprocessingPipeline::getStatistics() const {
    json stats;
    stats["frames_processed"] = frames_processed_;
    stats["total_processing_time_ms"] = total_processing_time_ms_;
    stats["avg_processing_time_ms"] = frames_processed_ > 0
        ? static_cast<double>(total_processing_time_ms_) / frames_processed_
        : 0.0;
    return stats;
}

void AudioPreprocessingPipeline::resetStatistics() {
    frames_processed_ = 0;
    total_processing_time_ms_ = 0;
}

}} // namespace themis::voice
