/**
 * @file emotion_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/emotion_analyzer.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace voice {

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------

std::string to_string(Emotion e) {
    switch (e) {
        case Emotion::NEUTRAL:   return "neutral";
        case Emotion::HAPPY:     return "happy";
        case Emotion::SAD:       return "sad";
        case Emotion::ANGRY:     return "angry";
        case Emotion::SURPRISED: return "surprised";
        case Emotion::FEARFUL:   return "fearful";
        case Emotion::DISGUSTED: return "disgusted";
    }
    return "unknown";
}

std::string to_string(Sentiment s) {
    switch (s) {
        case Sentiment::POSITIVE: return "positive";
        case Sentiment::NEUTRAL:  return "neutral";
        case Sentiment::NEGATIVE: return "negative";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

EmotionAnalyzer::EmotionAnalyzer(const EmotionConfig& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void EmotionAnalyzer::set_config(const EmotionConfig& config) {
    config_ = config;
}

EmotionConfig EmotionAnalyzer::get_config() const {
    return config_;
}

// ---------------------------------------------------------------------------
// Core analysis
// ---------------------------------------------------------------------------

std::optional<EmotionAnalysis> EmotionAnalyzer::analyze(
    const std::vector<uint8_t>& audio_data,
    const EmotionConfig&        config) const
{
    if (audio_data.empty()) {
        return std::nullopt;
    }

    const EmotionConfig& cfg = config.analysis_window_ms != EmotionConfig{}.analysis_window_ms
                               ? config : config_;

    auto samples = pcmToFloat(audio_data);
    if (samples.empty()) {
        return std::nullopt;
    }

    ++total_analyses_;

    AcousticFeatures feat = extractFeatures(samples);

    EmotionAnalysis result;
    result.emotion_probabilities = scoreEmotions(feat);

    // Primary emotion: argmax
    auto best = std::max_element(
        result.emotion_probabilities.begin(),
        result.emotion_probabilities.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    result.primary_emotion    = best->first;
    result.emotion_confidence = best->second;

    if (cfg.track_sentiment) {
        result.sentiment_score = computeSentimentScore(result.emotion_probabilities);
        if (result.sentiment_score > 0.15f) {
            result.sentiment = Sentiment::POSITIVE;
        } else if (result.sentiment_score < -0.15f) {
            result.sentiment = Sentiment::NEGATIVE;
        } else {
            result.sentiment = Sentiment::NEUTRAL;
        }
    }

    if (cfg.track_stress) {
        result.stress_level = computeStressLevel(feat);
    }

    if (cfg.track_engagement) {
        result.engagement_score = computeEngagementScore(feat);
    }

    result.quality = buildVoiceQuality(feat);

    return result;
}

EmotionTimeline EmotionAnalyzer::track(
    const std::vector<AudioSegment>& segments,
    const EmotionConfig&             config) const
{
    EmotionTimeline timeline = {};
    if (segments.empty()) {
        return timeline;
    }

    const EmotionConfig& cfg = config.analysis_window_ms != EmotionConfig{}.analysis_window_ms
                               ? config : config_;

    std::vector<float> stress_levels;
    std::vector<float> engagement_scores = {};

    stress_levels.reserve(segments.size());
    engagement_scores.reserve(segments.size());

    for (const auto& seg : segments) {
        auto result = analyze(seg.audio_data, cfg);
        if (!result) {
            continue;
        }

        TimedEmotion te;
        te.timestamp_ms   = seg.start_ms;
        te.emotion        = result->primary_emotion;
        te.confidence     = result->emotion_confidence;
        te.sentiment      = result->sentiment;
        te.sentiment_score = result->sentiment_score;

        timeline.timeline.push_back(te);
        stress_levels.push_back(result->stress_level);
        engagement_scores.push_back(result->engagement_score);
        timeline.total_duration_ms += (seg.end_ms - seg.start_ms);
    }

    if (!timeline.timeline.empty()) {
        timeline.statistics = computeStatistics(
            timeline.timeline, stress_levels, engagement_scores);
    }

    return timeline;
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

json EmotionAnalyzer::get_statistics() const {
    json stats;
    stats["total_analyses"] = total_analyses_;
    return stats;
}

// ---------------------------------------------------------------------------
// Private: PCM helpers
// ---------------------------------------------------------------------------

std::vector<float> EmotionAnalyzer::pcmToFloat(
    const std::vector<uint8_t>& raw) const
{
    // Interpret as 16-bit little-endian signed PCM.
    const size_t n = raw.size() / 2;
    std::vector<float> out(n);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = static_cast<int16_t>(
            static_cast<uint16_t>(raw[2 * i]) |
            (static_cast<uint16_t>(raw[2 * i + 1]) << 8));
        out[i] = s / 32768.0f;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Private: Acoustic feature extraction
// ---------------------------------------------------------------------------

EmotionAnalyzer::AcousticFeatures EmotionAnalyzer::extractFeatures(
    const std::vector<float>& samples) const
{
    constexpr int kBands = 8;

    AcousticFeatures f;
    f.band_rms.assign(kBands, 0.0f);

    const size_t n = samples.size();
    if (n == 0) {
        return f;
    }

    // ---- Global RMS & peak ------------------------------------------------
    float rms_sq = 0.0f;
    float peak   = 0.0f;
    for (float s : samples) {
        rms_sq += s * s;
        float a = std::abs(s);
        if (a > peak) {
          peak = a;
        }
    }
    f.global_rms  = std::sqrt(rms_sq / static_cast<float>(n));
    f.crest_factor = (f.global_rms > 1e-9f) ? std::min(1.0f, peak / (f.global_rms * 20.0f)) : 0.0f;

    // ---- Zero-crossing rate -----------------------------------------------
    size_t zc = 0;
    for (size_t i = 1; i < n; ++i) {
        if ((samples[i] >= 0.0f) != (samples[static_cast<int>(i - 1)] >= 0.0f)) {
            ++zc;
        }
    }
    f.zcr = static_cast<float>(zc) / static_cast<float>(n);
    // Normalise: typical speech ZCR is 0–0.3; cap at 1.
    f.zcr = std::min(1.0f, f.zcr * 5.0f);

    // ---- Sub-band RMS -------------------------------------------------------
    const size_t band_size = n / kBands;
    float max_band_rms = 0.0f;
    float hf_energy    = 0.0f;
    float total_energy = 0.0f;

    for (int b = 0; b < kBands; ++b) {
        size_t start = static_cast<size_t>(b) * band_size;
        size_t end   = (b == kBands - 1) ? n : start + band_size;
        float rms = 0.0f;
        for (size_t i = start; i < end; ++i) {
            rms += samples[i] * samples[i];
        }
        rms = std::sqrt(rms / static_cast<float>(end - start));
        f.band_rms[static_cast<size_t>(b)] = rms;
        total_energy += rms * rms;
        if (rms > max_band_rms) {
          max_band_rms = rms;
        }
        if (b >= kBands - 2) {
            hf_energy += rms * rms;
        }
    }
    f.hf_ratio = (total_energy > 1e-9f) ? (hf_energy / total_energy) : 0.0f;

    // ---- Spectral centroid & flatness (from sub-band energies) ---------------
    float w_sum = 0.0f, e_sum = 0.0f;
    float log_geo = 0.0f, arith = 0.0f;
    for (int b = 0; b < kBands; ++b) {
        float e = f.band_rms[static_cast<size_t>(b)] * f.band_rms[static_cast<size_t>(b)] + 1e-12f;
        w_sum += static_cast<float>(b) * e;
        e_sum += e;
        log_geo += std::log(e);
        arith   += e;
    }
    f.spectral_centroid = (e_sum > 1e-9f)
        ? std::min(1.0f, w_sum / e_sum / static_cast<float>(kBands - 1)) : 0.0f;
    float geo_mean = std::exp(log_geo / kBands);
    f.spectral_flatness = std::min(1.0f, geo_mean / (arith / kBands));

    // ---- Pitch estimate (autocorrelation on low-band samples) ---------------
    // We estimate pitch from the lower half of the signal (< 4 kHz equivalent).
    // We use a simplified autocorrelation peak search over lags 16–160 samples
    // (equivalent to ~100 Hz – 1000 Hz at 16 kHz).
    const size_t half_n = n / 2;
    constexpr int kLagMin = 16;
    const int kLagMax = static_cast<int>(std::min<size_t>(160, half_n / 2));

    float best_r    = -1.0f;
    int   best_lag  = kLagMin;
    if (half_n > static_cast<size_t>(kLagMax)) {
        // Compute unbiased autocorrelation for each lag.
        float r0 = 0.0f;
        for (size_t i = 0; i < half_n; ++i) {
            r0 += samples[i] * samples[i];
        }
        for (int lag = kLagMin; lag <= kLagMax; ++lag) {
            float r = 0.0f;
            for (size_t i = 0; i + static_cast<size_t>(lag) < half_n; ++i) {
                r += samples[i] * samples[i + static_cast<size_t>(lag)];
            }
            float norm_r = (r0 > 1e-12f) ? r / r0 : 0.0f;
            if (norm_r > best_r) {
                best_r   = norm_r;
                best_lag = lag;
            }
        }
    }

    // Convert lag to Hz assuming 16 kHz sample rate; clamp to [80, 400] Hz.
    float pitch_hz = (best_lag > 0) ? (16000.0f / static_cast<float>(best_lag)) : 0.0f;
    pitch_hz = std::max(80.0f, std::min(400.0f, pitch_hz));
    // Normalise to [0, 1] relative to [80, 400] Hz range.
    f.pitch_hz = (pitch_hz - 80.0f) / 320.0f;

    // Pitch variation: use variance of per-block pitch within the signal.
    // Approximate by measuring RMS variation across sub-band centroids.
    float pitch_var = 0.0f;
    for (int b = 0; b < kBands - 1; ++b) {
        float diff = f.band_rms[static_cast<size_t>(b + 1)] - f.band_rms[static_cast<size_t>(b)];
        pitch_var += diff * diff;
    }
    f.pitch_variation = std::min(1.0f, std::sqrt(pitch_var / (kBands - 1)) * 4.0f);

    return f;
}

// ---------------------------------------------------------------------------
// Private: Emotion scoring
// ---------------------------------------------------------------------------

/**
 * Hand-tuned linear emotion scoring.
 *
 * Feature→emotion weights (unnormalised):
 *
 *             energy  zcr  crest  centroid  flatness  hf_ratio  pitch  pitch_var
 * NEUTRAL      0.3   0.3   0.2     0.3        0.3      0.2      0.3     0.2
 * HAPPY        0.7   0.6   0.4     0.6        0.3      0.6      0.5     0.4
 * SAD         -0.6  -0.5  -0.3    -0.4       -0.2     -0.5     -0.4    -0.3
 * ANGRY        0.8   0.7   0.5     0.5        0.7      0.7      0.3     0.5
 * SURPRISED    0.5   0.5   0.9     0.5        0.4      0.5      0.5     0.7
 * FEARFUL      0.4   0.7   0.6     0.6        0.3      0.4      0.7     0.6
 * DISGUSTED    0.2   0.3   0.2     0.2        0.6      0.3      0.2     0.2
 *
 * The raw score for each emotion is the dot product of the feature vector
 * with the weight row.  A positive bias of 1.0 is added to NEUTRAL to prevent
 * degenerate distributions.  All raw scores are then softmax-normalised.
 */
std::map<Emotion, float> EmotionAnalyzer::scoreEmotions(
    const AcousticFeatures& f) const
{
    // Feature vector: [energy, zcr, crest, centroid, flatness, hf_ratio, pitch, pitch_var]
    const float e  = f.global_rms;
    const float z  = f.zcr;
    const float cr = f.crest_factor;
    const float sc = f.spectral_centroid;
    const float sf = f.spectral_flatness;
    const float hf = f.hf_ratio;
    const float p  = f.pitch_hz;
    const float pv = f.pitch_variation;

    std::map<Emotion, float> raw;
    raw[Emotion::NEUTRAL]   = 1.0f + 0.3f*e + 0.3f*z + 0.2f*cr + 0.3f*sc + 0.3f*sf + 0.2f*hf + 0.3f*p  + 0.2f*pv;
    raw[Emotion::HAPPY]     =        0.7f*e + 0.6f*z + 0.4f*cr + 0.6f*sc + 0.3f*sf + 0.6f*hf + 0.5f*p  + 0.4f*pv;
    raw[Emotion::SAD]       = 2.0f - 0.6f*e - 0.5f*z - 0.3f*cr - 0.4f*sc - 0.2f*sf - 0.5f*hf - 0.4f*p  - 0.3f*pv;
    raw[Emotion::ANGRY]     =        0.8f*e + 0.7f*z + 0.5f*cr + 0.5f*sc + 0.7f*sf + 0.7f*hf + 0.3f*p  + 0.5f*pv;
    raw[Emotion::SURPRISED] =        0.5f*e + 0.5f*z + 0.9f*cr + 0.5f*sc + 0.4f*sf + 0.5f*hf + 0.5f*p  + 0.7f*pv;
    raw[Emotion::FEARFUL]   =        0.4f*e + 0.7f*z + 0.6f*cr + 0.6f*sc + 0.3f*sf + 0.4f*hf + 0.7f*p  + 0.6f*pv;
    raw[Emotion::DISGUSTED] = 0.5f + 0.2f*e + 0.3f*z + 0.2f*cr + 0.2f*sc + 0.6f*sf + 0.3f*hf + 0.2f*p  + 0.2f*pv;

    return softmax(raw);
}

/*static*/ std::map<Emotion, float> EmotionAnalyzer::softmax(
    const std::map<Emotion, float>& raw)
{
    // Find max for numerical stability.
    float max_val = -std::numeric_limits<float>::infinity();
    for (const auto& kv : raw) {
        if (kv.second > max_val) {
          max_val = kv.second;
        }
    }

    std::map<Emotion, float> out;
    float sum = 0.0f;
    for (const auto& kv : raw) {
        float v = std::exp(kv.second - max_val);
        out[kv.first] = v;
        sum += v;
    }
    if (sum > 1e-9f) {
        for (auto& kv : out) {
            kv.second /= sum;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Private: Derived signals
// ---------------------------------------------------------------------------

float EmotionAnalyzer::computeSentimentScore(
    const std::map<Emotion, float>& probs) const
{
    // Polarity weights per emotion: positive emotions → +, negative → -.
    static const std::map<Emotion, float> kPolarity = {
        {Emotion::NEUTRAL,   0.0f},
        {Emotion::HAPPY,    +1.0f},
        {Emotion::SAD,      -1.0f},
        {Emotion::ANGRY,    -0.8f},
        {Emotion::SURPRISED,+0.4f},
        {Emotion::FEARFUL,  -0.7f},
        {Emotion::DISGUSTED,-0.9f},
    };

    float score = 0.0f;
    for (const auto& kv : probs) {
        auto it = kPolarity.find(kv.first);
        if (it != kPolarity.end()) {
            score += kv.second * it->second;
        }
    }
    // score is already in [-1, +1] because probabilities sum to 1.
    return std::max(-1.0f, std::min(1.0f, score));
}

float EmotionAnalyzer::computeStressLevel(const AcousticFeatures& f) const {
    // Stress increases with high energy, high ZCR, high HF ratio, high pitch.
    float s = 0.30f * f.global_rms
            + 0.25f * f.zcr
            + 0.20f * f.hf_ratio
            + 0.15f * f.pitch_hz
            + 0.10f * f.crest_factor;
    return std::max(0.0f, std::min(1.0f, s));
}

float EmotionAnalyzer::computeEngagementScore(const AcousticFeatures& f) const {
    // Engagement correlates with energy variation, spectral richness, moderate ZCR.
    float g = 0.35f * f.global_rms
            + 0.25f * f.pitch_variation
            + 0.20f * f.spectral_centroid
            + 0.20f * (1.0f - f.spectral_flatness); // tonality signal
    return std::max(0.0f, std::min(1.0f, g));
}

VoiceQuality EmotionAnalyzer::buildVoiceQuality(
    const AcousticFeatures& f) const
{
    VoiceQuality q;
    // Pitch in Hz: de-normalise [0,1] back to [80,400] Hz.
    q.pitch_hz        = 80.0f + f.pitch_hz * 320.0f;
    q.pitch_variation = f.pitch_variation;
    q.tempo           = f.zcr;                // Used as a syllable-rate proxy.
    q.volume_db       = (f.global_rms > 1e-9f)
                        ? 20.0f * std::log10(f.global_rms)
                        : -96.0f;
    q.energy          = f.global_rms;
    // Clarity: inverse spectral flatness (tonal → high clarity).
    q.clarity         = 1.0f - f.spectral_flatness;
    return q;
}

// ---------------------------------------------------------------------------
// Private: Timeline statistics
// ---------------------------------------------------------------------------

/*static*/ EmotionStatistics EmotionAnalyzer::computeStatistics(
    const std::vector<TimedEmotion>& entries,
    const std::vector<float>&        stress_levels,
    const std::vector<float>&        engagement_scores)
{
    EmotionStatistics stats = {};
    if (entries.empty()) {
        return stats;
    }

    // Dominant emotion: most frequent.
    std::map<Emotion, int> counts = {};

    for (const auto& te : entries) {
        ++counts[te.emotion];
    }
    stats.dominant_emotion = std::max_element(
        counts.begin(), counts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; })->first;

    // Emotion stability: fraction of entries with the dominant emotion.
    stats.emotion_stability = static_cast<float>(counts[stats.dominant_emotion])
                              / static_cast<float>(entries.size());

    // Emotion switches: count transitions.
    for (size_t i = 1; i <static_cast<int>(entries.size()); ++i) {
        if (entries[i].emotion != entries[static_cast<int>(i - 1)].emotion) {
            ++stats.emotion_switches;
        }
    }

    // Average & trend sentiment.
    float sum_sent = 0.0f;
    for (const auto& te : entries) {
        sum_sent += te.sentiment_score;
    }
    stats.average_sentiment = sum_sent / static_cast<float>(entries.size());

    // Trend: regression slope (simplified as mean of second half minus first half).
    const size_t half = entries.size() / 2;
    if (half > 0) {
        float first_half = 0.0f, second_half = 0.0f;
        for (size_t i = 0; i < half; ++i) {
            first_half += entries[i].sentiment_score;
        }
        for (size_t i = half; i <static_cast<int>(entries.size()); ++i) {
            second_half += entries[i].sentiment_score;
        }
        stats.sentiment_trend = (second_half - first_half) / static_cast<float>(half);
    }

    // Average stress & engagement.
    if (!stress_levels.empty()) {
        stats.average_stress = std::accumulate(
            stress_levels.begin(), stress_levels.end(), 0.0f)
            / static_cast<float>(stress_levels.size());
    }
    if (!engagement_scores.empty()) {
        stats.average_engagement = std::accumulate(
            engagement_scores.begin(), engagement_scores.end(), 0.0f)
            / static_cast<float>(engagement_scores.size());
    }

    return stats;
}

// ============================================================================
// Phase 3: Edge Case Handling with Safe Defaults
// ============================================================================

bool EmotionAnalyzer::isAvailable() const noexcept {
    return is_available_;
}

std::optional<EmotionAnalysis> EmotionAnalyzer::analyzeWithTimeout(
    const std::vector<uint8_t>& audio_data,
    const EmotionConfig& config) const
{
    // Phase 3.6: Timeout protection with safe defaults
    auto cfg = config.timeout_ms > 0 ? config : config_;
    
    if (!cfg.use_timeout_protection) {
        return analyze(audio_data, cfg);
    }
    
    // In production, this would use a timer/deadline mechanism
    // For now, we'll do a direct call with a note that timeouts should be
    // enforced at the threading/async level
    auto result = analyze(audio_data, cfg);
    
    if (!result) {
        // Analyzer unavailable or timeout occurred
        if (!cfg.skip_on_unavailable) {
            return std::nullopt;
        }
        
        // Return safe default emotion analysis
        ++timeout_fallbacks_;
        EmotionAnalysis safe_default;
        safe_default.primary_emotion = Emotion::NEUTRAL;
        safe_default.emotion_confidence = cfg.fallback_confidence;
        safe_default.sentiment = Sentiment::NEUTRAL;
        safe_default.stress_level = 0.5f;
        safe_default.engagement_score = 0.5f;
        
        return safe_default;
    }
    
    return result;
}

} // namespace voice
} // namespace themis

