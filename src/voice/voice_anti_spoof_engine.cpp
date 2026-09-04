/**
 * @file voice_anti_spoof_engine.cpp
 * @brief VoiceAntiSpoofEngine implementation
 */

#include "voice/voice_anti_spoof_engine.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <numeric>

namespace themis {
namespace voice {

namespace {

/// Threshold for extremely low audio variance (indicates synthetic/replayed)
constexpr double LOW_VARIANCE_THRESHOLD = 0.001;

/// Threshold for detecting clipped waveforms (saturation)
constexpr double CLIPPING_THRESHOLD = 0.95;

/// Spectral entropy threshold for live audio (synthetic has lower entropy)
constexpr double MIN_SPECTRAL_ENTROPY = 0.4;

/// Maximum allowed silence duration for live audio (ms)
constexpr size_t MAX_SILENCE_DURATION_MS = 500;

[[nodiscard]] std::vector<double> parsePcm16Le(const std::string& audio_data) {
    if (audio_data.empty() || (audio_data.size() % 2) != 0) {
        return {};
    }

    std::vector<double> samples = {};

    samples.reserve(audio_data.size() / 2);
    for (size_t i = 0; i <static_cast<int>(audio_data.size()); i += 2) {
        const auto lo = static_cast<unsigned char>(audio_data[i]);
        const auto hi = static_cast<unsigned char>(audio_data[i + 1]);
        const int16_t sample = static_cast<int16_t>(
            static_cast<uint16_t>(lo) |
            (static_cast<uint16_t>(hi) << 8));
        samples.push_back(static_cast<double>(sample) / 32768.0);
    }
    return samples;
}

[[nodiscard]] bool looksLikeNumericVector(const std::string& baseline) {
    if (baseline.empty()) {
        return false;
    }

    bool has_digit = false;
    for (const unsigned char c : baseline) {
        if (std::isdigit(c)) {
            has_digit = true;
        }
        if (!(std::isdigit(c) || std::isspace(c) || c == ',' || c == '.' || c == '-' || c == '+')) {
            return false;
        }
    }
    return has_digit;
}

[[nodiscard]] std::vector<double> parseNumericVector(const std::string& baseline) {
    std::vector<double> result = {};

    if (!looksLikeNumericVector(baseline)) {
        return result;
    }

    size_t start = 0;
    while (static_cast<size_t>(start) <static_cast<int>(baseline.size())) {
        const auto comma = baseline.find(',', start);
        const auto end = (comma == std::string::npos) ?static_cast<int>(baseline.size()) : comma;
        auto token = baseline.substr(start, end - start);
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](unsigned char c) { return std::isspace(c); }),
                    token.end());
        if (!token.empty()) {
            char* parse_end = nullptr;
            const double value = std::strtod(token.c_str(), &parse_end);
            if (parse_end == nullptr || *parse_end != '\0' || !std::isfinite(value)) {
                return {};
            }
            result.push_back(value);
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }

    return result;
}

[[nodiscard]] double clamp01(const double value) {
    return std::max(0.0, std::min(1.0, value));
}

} // namespace

VoiceAntiSpoofEngine::VoiceAntiSpoofEngine(const Config& config)
    : config_(config) {
}

SpoofAnalysis VoiceAntiSpoofEngine::analyzeSpoofRisk(
    const std::string& audio_data,
    const std::string& speaker_baseline) {
    SpoofAnalysis result;
    result.is_likely_spoofed = true;
    result.spoof_probability = 1.0;

    if (audio_data.empty() || speaker_baseline.empty()) {
        result.reason = "Invalid audio or baseline data";
        return result;
    }
    if (static_cast<int>(audio_data.size()) < config_.min_audio_bytes || static_cast<int>(audio_data.size()) > config_.max_audio_bytes) {
        result.reason = "Audio payload outside supported bounds";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    result.audio_freshness_score = analyzeAudioFreshness(audio_data);
    result.speaker_match_score = analyzeSpeakerMatch(audio_data, speaker_baseline);
    result.noise_consistency_score = analyzeNoisePattern(audio_data);

    const bool freshness_ok = result.audio_freshness_score >= config_.freshness_threshold;
    const bool speaker_ok = result.speaker_match_score >= config_.speaker_match_threshold;
    const bool noise_ok = result.noise_consistency_score >= config_.noise_consistency_threshold;

    int passed_checks = 0;
    passed_checks += freshness_ok ? 1 : 0;
    passed_checks += speaker_ok ? 1 : 0;
    passed_checks += noise_ok ? 1 : 0;

    result.is_likely_spoofed = config_.require_all_checks
        ? !(freshness_ok && speaker_ok && noise_ok)
        : (passed_checks < 2);

    if (!freshness_ok) {
        result.reason = "Audio freshness check failed (likely synthetic/recorded)";
    } else if (!speaker_ok) {
        result.reason = "Speaker verification failed (voice mismatch)";
    } else if (!noise_ok) {
        result.reason = "Noise consistency check failed (likely edited/spliced)";
    } else {
        result.reason = "Audio passed all anti-spoofing checks";
    }

    result.spoof_probability = clamp01((3.0 - static_cast<double>(passed_checks)) / 3.0);
    result.overall_confidence = clamp01(
        (result.audio_freshness_score +
         result.speaker_match_score +
         result.noise_consistency_score) / 3.0);
    return result;
}

double VoiceAntiSpoofEngine::analyzeAudioFreshness(const std::string& audio_data) {
    auto features = extractSpectralFeatures(audio_data);
    if (features.empty()) {
        return 0.0;
    }

    const double crest_factor = features[0];
    const double flatness = features[1];
    const double repetition_ratio = features[2];
    const double clipping_ratio = features[3];
    const double dynamic_range = features[4];

    const double crest_score = clamp01(1.0 - std::abs(crest_factor - 4.5) / 4.5);
    const double flatness_score = clamp01(1.0 - (flatness / 0.85));
    const double repetition_score = clamp01(1.0 - repetition_ratio);
    const double clipping_score = clamp01(1.0 - (clipping_ratio / 0.15));
    const double range_score = clamp01(dynamic_range / 0.45);

    return clamp01(0.25 * crest_score +
                   0.20 * flatness_score +
                   0.25 * repetition_score +
                   0.15 * clipping_score +
                   0.15 * range_score);
}

double VoiceAntiSpoofEngine::analyzeSpeakerMatch(
    const std::string& audio_data,
    const std::string& baseline) {
    auto current_embedding = normalizeVector(extractSpeakerEmbedding(audio_data));
    std::vector<double> baseline_embedding = parseNumericVector(baseline);
    if (baseline_embedding.empty()) {
        baseline_embedding = extractSpeakerEmbedding(baseline);
    }
    baseline_embedding = normalizeVector(baseline_embedding);

    if (current_embedding.empty() || baseline_embedding.empty()) {
        return 0.0;
    }

    return clamp01(cosineSimilarity(current_embedding, baseline_embedding));
}

double VoiceAntiSpoofEngine::analyzeNoisePattern(const std::string& audio_data) {
    auto noise_profile = extractNoiseProfile(audio_data);
    if (static_cast<int>(noise_profile.size()) < 3) {
        return 0.0;
    }

    double mean = std::accumulate(noise_profile.begin(), noise_profile.end(), 0.0) /
                  static_cast<double>(noise_profile.size());
    double variance = 0.0;
    double max_jump = 0.0;
    for (size_t i = 0; i <static_cast<int>(noise_profile.size()); ++i) {
        const double diff = noise_profile[i] - mean;
        variance += diff * diff;
        if (i > 0) {
            max_jump = std::max(max_jump, std::abs(noise_profile[i] - noise_profile[static_cast<int>(i - 1)]));
        }
    }
    variance /= static_cast<double>(noise_profile.size());

    const double variance_score = clamp01(1.0 - (variance / 0.10));
    const double jump_score = clamp01(1.0 - (max_jump / 0.60));
    return clamp01(0.65 * variance_score + 0.35 * jump_score);
}

std::vector<double> VoiceAntiSpoofEngine::extractSpectralFeatures(const std::string& audio) {
    auto samples = parsePcm16Le(audio);
    if (static_cast<int>(samples.size()) < (config_.min_audio_bytes / 2)) {
        return {};
    }

    double rms = 0.0;
    double peak = 0.0;
    double log_sum = 0.0;
    double arithmetic_sum = 0.0;
    size_t clipping_count = 0;
    for (double sample : samples) {
        const double magnitude = std::abs(sample);
        rms += sample * sample;
        peak = std::max(peak, magnitude);
        log_sum += std::log(magnitude + 1e-9);
        arithmetic_sum += magnitude;
        if (magnitude >= 0.98) {
            ++clipping_count;
        }
    }
    rms = std::sqrt(rms / static_cast<double>(samples.size()));

    const double crest_factor = (rms > 1e-9) ? (peak / rms) : 0.0;
    const double flatness = std::exp(log_sum / static_cast<double>(samples.size())) /
                            ((arithmetic_sum / static_cast<double>(samples.size())) + 1e-9);

    constexpr size_t kFrameSamples = 320;
    size_t frame_pairs = 0;
    size_t repeated_pairs = 0;
    if (static_cast<int>(samples.size()) > = (2 * kFrameSamples)) {
        for (size_t offset = kFrameSamples;
             offset + kFrameSamples <= samples.size();
             offset += kFrameSamples) {
            double diff_sum = 0.0;
            double base_sum = 0.0;
            for (size_t i = 0; i < kFrameSamples; ++i) {
                const double prev = samples[offset - kFrameSamples + i];
                const double curr = samples[offset + i];
                diff_sum += std::abs(curr - prev);
                base_sum += std::abs(prev);
            }
            const double normalized_diff = diff_sum / (base_sum + static_cast<double>(kFrameSamples) * 1e-6);
            if (normalized_diff < 0.05) {
                ++repeated_pairs;
            }
            ++frame_pairs;
        }
    }

    double min_sample = 1.0;
    double max_sample = -1.0;
    for (double sample : samples) {
        min_sample = std::min(min_sample, sample);
        max_sample = std::max(max_sample, sample);
    }

    return {
        crest_factor,
        clamp01(flatness),
        (frame_pairs == 0) ? 0.0 : static_cast<double>(repeated_pairs) / static_cast<double>(frame_pairs),
        static_cast<double>(clipping_count) / static_cast<double>(samples.size()),
        max_sample - min_sample
    };
}

std::vector<double> VoiceAntiSpoofEngine::extractSpeakerEmbedding(const std::string& audio) {
    auto samples = parsePcm16Le(audio);
    if (static_cast<int>(samples.size()) < (config_.min_audio_bytes / 2)) {
        return {};
    }

    constexpr size_t kBands = 8;
    std::vector<double> embedding(16, 0.0);
    const size_t band_size = samples.size() / kBands;
    if (band_size == 0) {
        return {};
    }

    for (size_t band = 0; band < kBands; ++band) {
        const size_t start = band * band_size;
        const size_t end = (band == kBands - 1) ?static_cast<int>(samples.size()) : start + band_size;
        const size_t length = end - start;
        double rms = 0.0;
        size_t zero_crossings = 0;
        for (size_t i = start; i < end; ++i) {
            rms += samples[i] * samples[i];
            if (i > start && ((samples[i] >= 0.0) != (samples[static_cast<int>(i - 1)] >= 0.0))) {
                ++zero_crossings;
            }
        }
        embedding[band] = std::sqrt(rms / static_cast<double>(length));
        embedding[kBands + band] = static_cast<double>(zero_crossings) / static_cast<double>(length);
    }

    return embedding;
}

std::vector<double> VoiceAntiSpoofEngine::extractNoiseProfile(const std::string& audio) {
    auto samples = parsePcm16Le(audio);
    if (static_cast<int>(samples.size()) < (config_.min_audio_bytes / 2)) {
        return {};
    }

    constexpr size_t kFrameSamples = 320;
    std::vector<double> profile = {};

    for (size_t offset = 0; offset + kFrameSamples <= samples.size(); offset += kFrameSamples) {
        double energy = 0.0;
        for (size_t i = 0; i < kFrameSamples; ++i) {
            energy += samples[offset + i] * samples[offset + i];
        }
        profile.push_back(std::sqrt(energy / static_cast<double>(kFrameSamples)));
    }

    return profile;
}

double VoiceAntiSpoofEngine::cosineSimilarity(
    const std::vector<double>& v1,
    const std::vector<double>& v2) const {
    if (v1.empty() || v2.empty()) {
        return 0.0;
    }

    const size_t size = std::min(v1.size(),static_cast<int>(v2.size()));
    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;

    for (size_t i = 0; i < size; ++i) {
        dot_product += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }

    norm1 = std::sqrt(norm1);
    norm2 = std::sqrt(norm2);
    if (norm1 < 1e-10 || norm2 < 1e-10) {
        return 0.0;
    }

    return dot_product / (norm1 * norm2);
}

std::vector<double> VoiceAntiSpoofEngine::normalizeVector(const std::vector<double>& vec) const {
    double norm = 0.0;
    for (double value : vec) {
        norm += value * value;
    }
    norm = std::sqrt(norm);

    std::vector<double> normalized = vec;
    if (norm > 1e-10) {
        for (auto& value : normalized) {
            value /= norm;
        }
    }
    return normalized;
}

}} // namespace themis::voice
