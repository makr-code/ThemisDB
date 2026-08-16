/**
 * @file voice_anti_spoof_engine.cpp
 * @brief VoiceAntiSpoofEngine implementation
 */

#include "voice/voice_anti_spoof_engine.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace voice {

VoiceAntiSpoofEngine::VoiceAntiSpoofEngine(const Config& config)
    : config_(config) {
}

SpoofAnalysis VoiceAntiSpoofEngine::analyzeSpoofRisk(
    const std::string& audio_data,
    const std::string& speaker_baseline) {
    
    SpoofAnalysis result;
    
    if (audio_data.empty() || speaker_baseline.empty()) {
        result.is_likely_spoofed = true;
        result.spoof_probability = 1.0;
        result.reason = "Invalid audio or baseline data";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Analyze three orthogonal features
        result.audio_freshness_score = analyzeAudioFreshness(audio_data);
        result.speaker_match_score = analyzeSpeakerMatch(audio_data, speaker_baseline);
        result.noise_consistency_score = analyzeNoisePattern(audio_data);

        // Compute composite verdict
        if (config_.require_all_checks) {
            // Fail-closed: ALL checks must pass
            bool freshness_ok = result.audio_freshness_score >= config_.freshness_threshold;
            bool speaker_ok = result.speaker_match_score >= config_.speaker_match_threshold;
            bool noise_ok = result.noise_consistency_score >= config_.noise_consistency_threshold;

            result.is_likely_spoofed = !(freshness_ok && speaker_ok && noise_ok);

            if (!freshness_ok) {
                result.reason = "Audio freshness check failed (likely synthetic/recorded)";
            } else if (!speaker_ok) {
                result.reason = "Speaker verification failed (voice mismatch)";
            } else if (!noise_ok) {
                result.reason = "Noise consistency check failed (likely edited/spliced)";
            } else {
                result.reason = "Audio passed all anti-spoofing checks";
            }
        } else {
            // Majority voting: 2 out of 3 must pass
            int passed_checks = 0;
            if (result.audio_freshness_score >= config_.freshness_threshold) passed_checks++;
            if (result.speaker_match_score >= config_.speaker_match_threshold) passed_checks++;
            if (result.noise_consistency_score >= config_.noise_consistency_threshold) passed_checks++;

            result.is_likely_spoofed = (passed_checks < 2);
            
            if (result.is_likely_spoofed) {
                result.reason = "Failed majority of anti-spoofing checks";
            } else {
                result.reason = "Passed majority anti-spoofing checks";
            }
        }

        // Compute spoof probability (inverse of how many checks passed)
        int passed = 0;
        if (result.audio_freshness_score >= config_.freshness_threshold) passed++;
        if (result.speaker_match_score >= config_.speaker_match_threshold) passed++;
        if (result.noise_consistency_score >= config_.noise_consistency_threshold) passed++;
        
        result.spoof_probability = (3.0 - passed) / 3.0;
        result.overall_confidence = (result.audio_freshness_score + 
                                    result.speaker_match_score + 
                                    result.noise_consistency_score) / 3.0;

    } catch (const std::exception& e) {
        result.is_likely_spoofed = true;  // Fail-closed
        result.spoof_probability = 1.0;
        result.reason = std::string("Analysis error: ") + e.what();
    }

    return result;
}

double VoiceAntiSpoofEngine::analyzeAudioFreshness(const std::string& audio_data) {
    // Extract spectral features to detect synthetic/recorded audio
    auto features = extractSpectralFeatures(audio_data);
    
    if (features.empty()) {
        return 0.0;  // Fail-closed: unable to analyze = likely spoofed
    }

    // Heuristic: Compute spectral variation
    // Live audio has natural spectral variation; synthetic is too regular
    double mean = std::accumulate(features.begin(), features.end(), 0.0) / features.size();
    double variance = 0.0;
    
    for (double f : features) {
        variance += (f - mean) * (f - mean);
    }
    variance /= features.size();
    
    double std_dev = std::sqrt(variance);
    
    // Normalize to [0, 1] scale
    // Live audio typically has std_dev in range [0.1, 0.5]
    // Synthetic audio has std_dev < 0.05 or > 0.6
    double freshness = std::min(1.0, std_dev / 0.35);
    
    // Clamp to avoid floating point issues
    return std::max(0.0, std::min(1.0, freshness));
}

double VoiceAntiSpoofEngine::analyzeSpeakerMatch(
    const std::string& audio_data,
    const std::string& baseline) {
    
    // Extract speaker embedding from current audio
    auto current_embedding = extractSpeakerEmbedding(audio_data);
    
    // Parse baseline embedding (assumed to be comma-separated doubles)
    std::vector<double> baseline_embedding;
    if (!baseline.empty()) {
        // In production, this would deserialize the baseline
        // For now, stub implementation
        baseline_embedding = extractSpeakerEmbedding(baseline);
    }
    
    if (current_embedding.empty() || baseline_embedding.empty()) {
        return 0.0;  // Fail-closed
    }

    // Compute cosine similarity
    double match_score = cosineSimilarity(current_embedding, baseline_embedding);
    
    // Clamp to [0, 1]
    return std::max(0.0, std::min(1.0, match_score));
}

double VoiceAntiSpoofEngine::analyzeNoisePattern(const std::string& audio_data) {
    // Extract background noise profile
    auto noise_profile = extractNoiseProfile(audio_data);
    
    if (noise_profile.empty()) {
        return 0.0;  // Fail-closed
    }

    // Heuristic: Compute noise consistency
    // Consistent background noise = natural/continuous recording
    // Discontinuous noise = edited/spliced audio
    
    double mean = std::accumulate(noise_profile.begin(), noise_profile.end(), 0.0) / noise_profile.size();
    double variance = 0.0;
    
    for (double n : noise_profile) {
        variance += (n - mean) * (n - mean);
    }
    variance /= noise_profile.size();
    
    // High variance in noise = inconsistent = likely edited
    double consistency = 1.0 - std::min(1.0, variance / 10.0);
    
    return std::max(0.0, std::min(1.0, consistency));
}

std::vector<double> VoiceAntiSpoofEngine::extractSpectralFeatures(const std::string& audio) {
    // Stub implementation: return dummy features
    // In production, this would:
    // - Parse audio format (WAV, PCM, etc.)
    // - Compute FFT or MFCC
    // - Return spectral coefficients
    
    if (audio.empty()) {
        return {};
    }

    // Generate synthetic features based on audio content hash
    // This is deterministic but not real spectral analysis
    std::vector<double> features;
    for (size_t i = 0; i < 10; ++i) {
        // Deterministic pseudo-random based on input
        uint32_t hash = i * 31 + audio.size() * 7;
        features.push_back(static_cast<double>(hash % 100) / 100.0);
    }
    return features;
}

std::vector<double> VoiceAntiSpoofEngine::extractSpeakerEmbedding(const std::string& audio) {
    // Stub implementation: return dummy embedding
    // In production, this would:
    // - Use a pre-trained speaker recognition model (e.g., x-vector, i-vector)
    // - Extract speaker embedding from audio
    // - Return 256+ dimensional vector
    
    if (audio.empty()) {
        return {};
    }

    // Generate synthetic embedding based on audio content
    std::vector<double> embedding;
    for (size_t i = 0; i < 128; ++i) {
        uint32_t hash = (i * 17 + audio.size() * 13) % 1000;
        embedding.push_back(static_cast<double>(hash) / 1000.0 - 0.5);
    }
    return embedding;
}

std::vector<double> VoiceAntiSpoofEngine::extractNoiseProfile(const std::string& audio) {
    // Stub implementation: return dummy noise profile
    // In production, this would:
    // - Detect background noise/silence
    // - Extract noise characteristics (frequency content)
    // - Return noise vector for continuity checking
    
    if (audio.empty()) {
        return {};
    }

    std::vector<double> noise;
    for (size_t i = 0; i < 20; ++i) {
        uint32_t hash = (i * 23 + audio.size() * 11) % 100;
        noise.push_back(static_cast<double>(hash) / 100.0);
    }
    return noise;
}

double VoiceAntiSpoofEngine::cosineSimilarity(
    const std::vector<double>& v1,
    const std::vector<double>& v2) const {
    
    if (v1.empty() || v2.empty()) {
        return 0.0;
    }

    // Pad to same size
    size_t size = std::min(v1.size(), v2.size());
    
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
        return 0.0;  // Degenerate case
    }
    
    return dot_product / (norm1 * norm2);
}

std::vector<double> VoiceAntiSpoofEngine::normalizeVector(const std::vector<double>& vec) const {
    double norm = 0.0;
    for (double v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    
    std::vector<double> result = vec;
    if (norm > 1e-10) {
        for (auto& v : result) {
            v /= norm;
        }
    }
    return result;
}

}} // namespace themis::voice
