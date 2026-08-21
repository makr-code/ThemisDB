/**
 * @file audio_preprocessing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/audio_preprocessing.h"
#include <chrono>
#include <cmath>
#include <numbers>
#include <numeric>
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_RNNOISE
#  include <rnnoise.h>
#endif

namespace themis { namespace voice {

namespace {
std::mutex                      s_noise_suppressor_bridge_mutex;
NoiseSuppressor::ProcessFramesFn s_process_frames_fn;
}

void NoiseSuppressor::setProcessFramesFn(ProcessFramesFn fn) {
    std::lock_guard<std::mutex> lk(s_noise_suppressor_bridge_mutex);
    s_process_frames_fn = std::move(fn);
}

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
// STUB/SIMULATION NOTE:
// Purpose: Empty Impl when RNNoise is not compiled in. NoiseSuppressor still
//          constructs successfully; suppress() is a no-op (audio passes through).
// Activation: Compiled when THEMIS_ENABLE_RNNOISE is NOT defined (default builds).
//             Build with -DTHEMIS_ENABLE_RNNOISE=ON and link rnnoise (vcpkg) for real.
// Production Delta: No noise reduction is applied to audio frames.
// Removal Plan: Not removed — kept as compile-time fallback alongside the real path.
// Roadmap ref: src/voice/ROADMAP.md § "Phase 2: RNNoise integration"
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
    ProcessFramesFn fn;
    {
        std::lock_guard<std::mutex> lk(s_noise_suppressor_bridge_mutex);
        fn = s_process_frames_fn;
    }
    if (fn) {
        try {
            return fn(samples_48k, vad_threshold);
        } catch (const std::string&) {
        } catch (const char*) {
        } catch (...) {
        }
    }

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
    // TASK 2.2: Audio preprocessing and validation
    // Error codes [6700-6799]:
    // - 6700: Input size validation failed (> 512KB)
    // - 6701: Codec validation failed
    // - 6702: Preprocessing pipeline error
    
    auto t0 = std::chrono::steady_clock::now();

    PreprocessingResult res;
    res.success = false;

    // TASK 2.2: Bounded chunk handling — reject frames > 512KB immediately
    // Error code 6700: Input size validation failed
    if (raw_audio.size() > kMaxAudioFrameSizeBytes) {
        res.error_message = "Audio frame exceeds maximum size (512KB) - error 6700";
        spdlog::error("AudioPreprocessingPipeline::process: input size {} bytes exceeds limit (error 6700)",
                      raw_audio.size());
        return res;  // Fail-closed: reject oversized frames
    }

    // TASK 2.2: Handle empty input gracefully
    if (raw_audio.empty()) {
        res.success = true;
        res.error_message = "empty input";
        return res;
    }

    // TASK 2.2: Validate sample rate (supported range: 8kHz - 48kHz)
    if (source_sample_rate < 8000 || source_sample_rate > 48000) {
        res.error_message = "Sample rate out of supported range (8000-48000 Hz) - error 6701";
        spdlog::error("AudioPreprocessingPipeline::process: invalid sample rate {} Hz (error 6701)",
                      source_sample_rate);
        return res;  // Fail-closed
    }

    // TASK 2.2: Convert raw bytes to audio frame
    AudioFrame frame;
    try {
        frame.samples = convertRawToFloat(raw_audio);
    } catch (const std::exception& e) {
        res.error_message = std::string("Audio conversion failed - error 6702: ") + e.what();
        spdlog::error("AudioPreprocessingPipeline::process: conversion failed (error 6702): {}", e.what());
        return res;  // Fail-closed
    }

    if (frame.samples.empty()) {
        res.error_message = "Conversion resulted in empty samples - error 6702";
        spdlog::error("AudioPreprocessingPipeline::process: empty samples after conversion (error 6702)");
        return res;
    }

    frame.sample_rate = source_sample_rate;
    frame.channels = 1;

    // TASK 2.2: Process through pipeline (with graceful fallback for optional models)
    res = processFrame(frame);

    auto t1 = std::chrono::steady_clock::now();
    res.processing_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    
    return res;
}

PreprocessingResult AudioPreprocessingPipeline::processFrame(const AudioFrame& frame) {
    // TASK 2.2: Preprocessing chain with graceful fallback
    // Chain: normalize → resample → enhance (RNNoise) → filter (noise reduction)
    // Graceful degradation: if optional model unavailable, skip it and continue
    
    auto t0 = std::chrono::steady_clock::now();

    PreprocessingResult res;
    AudioFrame current = frame;

    // TASK 2.2: Preprocessing chain stages (order matters for signal integrity)
    
    // Stage 1: Deep-learning noise suppression (RNNoise if available)
    if (opts_.enable_rnnoise_suppression) {
        try {
            current = applyRNNoiseSuppression(current, opts_.rnnoise_vad_threshold);
            res.rnnoise_vad_probability = noise_suppressor_.lastVadProbability();
        } catch (const std::exception& e) {
            // Graceful fallback: RNNoise model unavailable
            spdlog::debug("AudioPreprocessingPipeline::processFrame: RNNoise suppression failed (fallback): {}",
                         e.what());
            res.rnnoise_vad_probability = 0.0f;  // Safe fallback
        }
    }

    // Stage 2: Spectral-gate noise reduction (always available)
    if (opts_.enable_noise_reduction) {
        try {
            current = applyNoiseReduction(current, opts_.noise_reduction_strength);
        } catch (const std::exception& e) {
            spdlog::debug("AudioPreprocessingPipeline::processFrame: noise reduction failed: {}", e.what());
        }
    }

    // Stage 3: Echo cancellation (requires reference frame; no-op here)
    if (opts_.enable_echo_cancellation) {
        // No reference frame; echo cancellation would require dual-channel input
        // This stage is a no-op in streaming context
    }

    // Stage 4: Voice Activity Detection (VAD)
    if (opts_.enable_vad) {
        try {
            res.voice_activity_ratio = detectVoiceActivity(current);
        } catch (const std::exception& e) {
            spdlog::debug("AudioPreprocessingPipeline::processFrame: VAD failed: {}", e.what());
            res.voice_activity_ratio = 1.0f;  // Safe fallback: assume all voice
        }
    }

    // Stage 5: Audio normalization (critical for downstream processing)
    if (opts_.enable_normalization) {
        try {
            current = normalize(current, opts_.target_rms);
        } catch (const std::exception& e) {
            spdlog::debug("AudioPreprocessingPipeline::processFrame: normalization failed: {}", e.what());
        }
    }

    // Stage 6: Sample rate conversion (critical for compatibility)
    if (current.sample_rate != opts_.target_sample_rate) {
        try {
            current = resample(current, opts_.target_sample_rate);
        } catch (const std::exception& e) {
            spdlog::error("AudioPreprocessingPipeline::processFrame: resampling failed: {}", e.what());
            // Don't fail; return with original sample rate
        }
    }

    // TASK 2.2: Compute diagnostics
    res.detected_noise_level = computeNoiseFloor(frame.samples);
    res.processed_audio = current;
    res.success = true;

    auto t1 = std::chrono::steady_clock::now();
    res.processing_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    ++frames_processed_;
    total_processing_time_ms_ += static_cast<uint64_t>(res.processing_time_ms);

    // TASK 2.2: Logging for all validation failures (non-sensitive)
    res.diagnostics["frames_processed"] = frames_processed_;
    res.diagnostics["voice_activity_ratio"] = res.voice_activity_ratio;
    res.diagnostics["noise_level"] = res.detected_noise_level;
    res.diagnostics["rnnoise_enabled"] = NoiseSuppressor::isRNNoiseEnabled();
    res.diagnostics["rnnoise_vad_probability"] = res.rnnoise_vad_probability;
    res.diagnostics["sample_rate_original"] = frame.sample_rate;
    res.diagnostics["sample_rate_output"] = current.sample_rate;

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

// ============================================================================
// Phase 3: Input Validation Hardening
// ============================================================================

AudioValidationResult AudioPreprocessingPipeline::validateAudioPayload(
    const std::vector<uint8_t>& raw_audio,
    int declared_sample_rate,
    int declared_channels,
    int declared_bits_per_sample)
{
    AudioValidationResult result;
    result.detected_sample_rate = declared_sample_rate;
    result.detected_channels = declared_channels;
    result.detected_bits_per_sample = declared_bits_per_sample;
    
    // Phase 3.1: Size validation (fail-closed)
    if (raw_audio.empty() || raw_audio.size() < MIN_AUDIO_SIZE_BYTES) {
        result.valid = false;
        result.error_message = "Audio payload too small (min: " + 
            std::to_string(MIN_AUDIO_SIZE_BYTES) + " bytes)";
        ++validation_errors_;
        return result;
    }
    
    if (raw_audio.size() > MAX_AUDIO_SIZE_BYTES) {
        result.valid = false;
        result.error_message = "Audio payload too large (max: " + 
            std::to_string(MAX_AUDIO_SIZE_BYTES) + " bytes)";
        ++overflow_attempts_;
        result.is_overflow_attempt = true;
        return result;
    }
    
    // Phase 3.2: Sample rate validation (fail-closed)
    if (declared_sample_rate < MIN_SAMPLE_RATE || declared_sample_rate > MAX_SAMPLE_RATE) {
        result.valid = false;
        result.error_message = "Unsupported sample rate: " + std::to_string(declared_sample_rate) +
            " Hz (supported: " + std::to_string(MIN_SAMPLE_RATE) + "-" + 
            std::to_string(MAX_SAMPLE_RATE) + " Hz)";
        ++validation_errors_;
        return result;
    }
    
    // Phase 3.2: Channel validation (fail-closed)
    if (declared_channels < MIN_CHANNELS || declared_channels > MAX_CHANNELS) {
        result.valid = false;
        result.error_message = "Unsupported channel count: " + std::to_string(declared_channels);
        ++validation_errors_;
        return result;
    }
    
    // Phase 3.2: Bits per sample validation (fail-closed)
    if (declared_bits_per_sample < MIN_BITS_PER_SAMPLE || 
        declared_bits_per_sample > MAX_BITS_PER_SAMPLE) {
        result.valid = false;
        result.error_message = "Unsupported bits per sample: " + 
            std::to_string(declared_bits_per_sample);
        ++validation_errors_;
        return result;
    }
    
    // Phase 3.3: Detect codec from header
    result.detected_codec = detectCodecFromHeader(raw_audio);
    
    // Phase 3.3: Codec validation (whitelist-based)
    if (!isCodecSupported(result.detected_codec)) {
        result.valid = false;
        result.error_message = "Unsupported or unrecognized audio codec";
        ++validation_errors_;
        return result;
    }
    
    // Phase 3: Frame header validation (fuzzing-aware)
    if (!validateFrameHeader(raw_audio)) {
        result.valid = false;
        result.error_message = "Malformed audio frame header";
        result.is_malformed_frame_header = true;
        ++malformed_frames_;
        return result;
    }
    
    // Phase 3: Overflow detection (fuzzing-aware)
    if (detectOverflowAttempt(raw_audio)) {
        result.valid = false;
        result.error_message = "Potential buffer overflow detected in payload";
        result.is_overflow_attempt = true;
        ++overflow_attempts_;
        return result;
    }
    
    // All validations passed
    result.valid = true;
    result.error_message = "";
    return result;
}

bool AudioPreprocessingPipeline::isCodecSupported(DetectedAudioCodec codec) const {
    switch (codec) {
        case DetectedAudioCodec::PCM16:
        case DetectedAudioCodec::PCM32:
        case DetectedAudioCodec::OPUS:
        case DetectedAudioCodec::AAC:
        case DetectedAudioCodec::FLAC:
            return true;
        case DetectedAudioCodec::UNKNOWN:
        default:
            return false;
    }
}

DetectedAudioCodec AudioPreprocessingPipeline::detectCodecFromHeader(
    const std::vector<uint8_t>& raw_audio) const
{
    if (raw_audio.size() < 4) return DetectedAudioCodec::UNKNOWN;
    
    // Check for OPUS header (0xFF, 0x4F)
    if (raw_audio[0] == 0xFF && raw_audio[1] == 0x4F) {
        return DetectedAudioCodec::OPUS;
    }
    
    // Check for FLAC header ('fLaC' = 0x66 0x4C 0x61 0x43)
    if (raw_audio.size() >= 4 && raw_audio[0] == 0x66 && raw_audio[1] == 0x4C && 
        raw_audio[2] == 0x61 && raw_audio[3] == 0x43) {
        return DetectedAudioCodec::FLAC;
    }
    
    // Check for AAC header (0xFF 0xF1 or 0xFF 0xF9)
    if (raw_audio[0] == 0xFF && (raw_audio[1] == 0xF1 || raw_audio[1] == 0xF9)) {
        return DetectedAudioCodec::AAC;
    }
    
    // Default to PCM16 for raw PCM data
    return DetectedAudioCodec::PCM16;
}

bool AudioPreprocessingPipeline::validateFrameHeader(
    const std::vector<uint8_t>& raw_audio) const
{
    // Minimum frame size: check for truncation
    if (raw_audio.size() < 4) {
        return false;
    }
    
    // Check for valid RIFF header for WAV
    if (raw_audio.size() >= 12) {
        if (raw_audio[0] == 'R' && raw_audio[1] == 'I' && 
            raw_audio[2] == 'F' && raw_audio[3] == 'F') {
            // Valid RIFF header, check size field matches payload
            uint32_t size = *reinterpret_cast<const uint32_t*>(raw_audio.data() + 4);
            if (size > raw_audio.size()) {
                return false; // Truncated
            }
        }
    }
    
    // Frame is not obviously malformed
    return true;
}

bool AudioPreprocessingPipeline::detectOverflowAttempt(
    const std::vector<uint8_t>& raw_audio) const
{
    // Detect suspicious patterns that might indicate buffer overflow attempt
    
    // Pattern 1: All bytes same (likely fuzz-generated)
    if (raw_audio.size() > 10) {
        bool all_same = true;
        uint8_t first = raw_audio[0];
        for (size_t i = 1; i < std::min(raw_audio.size(), size_t(100)); ++i) {
            if (raw_audio[i] != first) {
                all_same = false;
                break;
            }
        }
        // If all bytes are identical for more than 10 bytes, suspicious
        if (all_same && raw_audio.size() > 10) {
            return true;
        }
    }
    
    // Pattern 2: Check for obvious integer overflow attempts in size fields
    if (raw_audio.size() >= 8) {
        uint32_t size_field = *reinterpret_cast<const uint32_t*>(raw_audio.data() + 4);
        if (size_field > MAX_AUDIO_SIZE_BYTES) {
            return true;
        }
    }
    
    return false;
}

}} // namespace themis::voice

