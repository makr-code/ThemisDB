/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audio_preprocessing.cpp                            ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-04-14 11:39:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     469                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • be2946f188  2026-02-28  feat(voice): implement RNNoise deep-learning noise suppre... ║
    • 28a4b23b94  2026-02-23  Refactor tests and update error handling ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file audio_preprocessing.cpp
 * @brief Audio preprocessing pipeline implementation (Phase 1 production readiness)
 *        RNNoise deep-learning noise suppression (Phase 3)
 */

#include "voice/audio_preprocessing.h"
#include <chrono>
#include <cmath>
#include <numbers>
#include <numeric>
#include <algorithm>
#include <stdexcept>

#ifdef THEMIS_ENABLE_RNNOISE
#  include <rnnoise.h>
#endif

namespace themis { namespace voice {

// ============================================================================
// NoiseSuppressor – RNNoise integration (Phase 3)
// ============================================================================

// RNNoise operates at 48 kHz with fixed 480-sample frames (10 ms).
// Samples are expected in the range [-32768, 32767] (int16 scale).
static constexpr int   kRNNoiseRate         = 48000;
static constexpr int   kRNNoiseFrameSamples = 480;  // 10 ms at 48 kHz
static constexpr float kRNNoiseScale        = 32768.0f;

#ifdef THEMIS_ENABLE_RNNOISE
struct NoiseSuppressor::Impl {
    DenoiseState* state = nullptr;
    Impl()  { state = rnnoise_create(nullptr); }
    ~Impl() { if (state) rnnoise_destroy(state); }
};
#else
struct NoiseSuppressor::Impl {};  // placeholder when RNNoise is not available
#endif

NoiseSuppressor::NoiseSuppressor()
    : impl_(std::make_unique<Impl>()) {}

NoiseSuppressor::~NoiseSuppressor() = default;

NoiseSuppressor::NoiseSuppressor(NoiseSuppressor&&) noexcept = default;
NoiseSuppressor& NoiseSuppressor::operator=(NoiseSuppressor&&) noexcept = default;

// static
bool NoiseSuppressor::isRNNoiseEnabled() {
#ifdef THEMIS_ENABLE_RNNOISE
    return true;
#else
    return false;
#endif
}

// Linear-interpolation resampler (same logic used elsewhere in this file).
// static
std::vector<float> NoiseSuppressor::resampleLinear(
    const std::vector<float>& in, int src_rate, int dst_rate)
{
    if (in.empty() || src_rate == dst_rate) return in;
    double ratio = static_cast<double>(dst_rate) / static_cast<double>(src_rate);
    size_t out_size = static_cast<size_t>(static_cast<double>(in.size()) * ratio);
    if (out_size == 0) return {};
    std::vector<float> out(out_size);
    for (size_t i = 0; i < out_size; ++i) {
        double src_pos = static_cast<double>(i) / ratio;
        size_t idx0    = static_cast<size_t>(src_pos);
        size_t idx1    = std::min(idx0 + 1, in.size() - 1);
        float  frac    = static_cast<float>(src_pos - static_cast<double>(idx0));
        out[i] = in[idx0] * (1.0f - frac) + in[idx1] * frac;
    }
    return out;
}

// Process samples_48k (already at 48 kHz, normalised [-1,1]) through
// RNNoise in kRNNoiseFrameSamples-sized chunks.
// Returns the mean VAD probability across all processed frames.
float NoiseSuppressor::processRNNoiseFrames(
    std::vector<float>& samples_48k, float vad_threshold)
{
    if (samples_48k.empty()) return 0.0f;

#ifdef THEMIS_ENABLE_RNNOISE
    // RNNoise expects samples in [-32768, 32767] range.
    std::vector<float> buf(kRNNoiseFrameSamples, 0.0f);
    float vad_sum   = 0.0f;
    int   num_frames = 0;

    for (size_t offset = 0; offset < samples_48k.size(); offset += kRNNoiseFrameSamples) {
        // Fill one 10-ms frame (zero-pad if < 480 samples remain).
        size_t avail = std::min(static_cast<size_t>(kRNNoiseFrameSamples),
                                samples_48k.size() - offset);
        for (size_t i = 0; i < avail; ++i)
            buf[i] = samples_48k[offset + i] * kRNNoiseScale;
        for (size_t i = avail; i < static_cast<size_t>(kRNNoiseFrameSamples); ++i)
            buf[i] = 0.0f;

        float vad_prob = rnnoise_process_frame(impl_->state, buf.data(), buf.data());
        vad_sum += vad_prob;
        ++num_frames;

        // Gate: attenuate frames classified as noise only.
        float gain = (vad_prob >= vad_threshold) ? 1.0f : vad_prob / std::max(vad_threshold, 1e-6f);
        for (size_t i = 0; i < avail; ++i)
            samples_48k[offset + i] = (buf[i] / kRNNoiseScale) * gain;
    }
    return (num_frames > 0) ? (vad_sum / static_cast<float>(num_frames)) : 0.0f;
#else
    // Fallback: spectral-gate noise suppression (no external library).
    // Estimate noise floor from leading samples and attenuate below threshold.
    if (samples_48k.size() < 2) return 0.0f;

    size_t noise_end = std::max<size_t>(1, samples_48k.size() / 10);
    float  sum_sq    = 0.0f;
    for (size_t i = 0; i < noise_end; ++i) sum_sq += samples_48k[i] * samples_48k[i];
    float noise_floor = std::sqrt(sum_sq / static_cast<float>(noise_end));
    float threshold   = noise_floor * (1.0f + vad_threshold);

    float signal_sq = 0.0f, total_sq = 0.0f;
    for (float& s : samples_48k) {
        float abs_s = std::abs(s);
        total_sq   += s * s;
        if (abs_s < threshold) {
            s *= (abs_s / std::max(threshold, 1e-9f)) * (1.0f - vad_threshold);
        } else {
            signal_sq += s * s;
        }
    }
    return (total_sq > 1e-12f) ? std::sqrt(signal_sq / total_sq) : 0.0f;
#endif
}

AudioFrame NoiseSuppressor::suppress(const AudioFrame& frame, float vad_threshold) {
    AudioFrame result = frame;
    ++frames_processed_;

    if (frame.samples.empty()) return result;

    // 1. Resample to 48 kHz (RNNoise requirement).
    std::vector<float> s48k = resampleLinear(frame.samples, frame.sample_rate, kRNNoiseRate);

    // 2. Process through RNNoise (or fallback).
    last_vad_prob_ = processRNNoiseFrames(s48k, vad_threshold);

    // 3. Resample back to original sample rate.
    result.samples = resampleLinear(s48k, kRNNoiseRate, frame.sample_rate);
    return result;
}

// ============================================================================

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
    float rc = 1.0f / (2.0f * std::numbers::pi_v<float> * cutoff_hz);
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

AudioFrame AudioPreprocessingPipeline::applyRNNoiseSuppression(
    const AudioFrame& frame, float vad_threshold)
{
    return noise_suppressor_.suppress(frame, vad_threshold);
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
    if (opts_.enable_rnnoise_suppression) {
        current = applyRNNoiseSuppression(current, opts_.rnnoise_vad_threshold);
        res.rnnoise_vad_probability = noise_suppressor_.lastVadProbability();
    }
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
    res.diagnostics["rnnoise_enabled"] = NoiseSuppressor::isRNNoiseEnabled();
    res.diagnostics["rnnoise_vad_probability"] = res.rnnoise_vad_probability;

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
