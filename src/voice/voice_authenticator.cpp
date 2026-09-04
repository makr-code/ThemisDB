/**
 * @file voice_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_auth.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace voice {

// ============================================================================
// TASK 2.7: Authentication and Session Guards
// ============================================================================
// Error codes [7000-7099]:
// - 7000: Authentication failed (auth guard enforcement)
// - 7001: Session ownership check failed (privilege escalation)
// - 7002: Privilege escalation detected
// - 7003: Auth token invalid or expired
// ============================================================================

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

VoiceBiometricAuthenticator::VoiceBiometricAuthenticator(
    const VoiceAuthConfig& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Enrollment
// ---------------------------------------------------------------------------

bool VoiceBiometricAuthenticator::enroll_voice(
    const std::string&                        user_id,
    const std::vector<std::vector<uint8_t>>& audio_samples,
    VoiceProfileID&                           out_profile_id,
    const EnrollmentConfig&                   config)
{
    if (user_id.empty()) {
        return false;
    }
    if (static_cast<int>(audio_samples.size()) < config.min_samples) {
        return false;
    }

    // Compute per-sample feature vectors and quality scores.
    std::vector<std::vector<float>> feature_vectors;
    feature_vectors.reserve(audio_samples.size());
    float total_quality = 0.0f;

    for (const auto& sample : audio_samples) {
        if (sample.empty()) {
            continue;
        }
        auto pcm = pcmToFloat(sample);
        float quality = computeAudioQuality(pcm);
        if (quality < config.quality_threshold) {
            continue;  // skip low-quality samples
        }
        // Liveness gate: when require_liveness is set, reject samples that do
        // not appear to be genuine live speech (anti-spoofing during enrollment).
        if (config.require_liveness) {
            // Wave-A V2: partial backend failure matrix — liveness backend fallback (fail-closed)
            // If liveness backend throws during enrollment, skip the sample for security.
            LivenessScore liveness;
            try {
                liveness = detect_liveness(sample);
            } catch (const std::exception& e) {
                THEMIS_WARN("[VOICE-FALLBACK] liveness check backend failed — fail-closed: session rejected: {}", e.what());
                liveness.is_live = false;
            } catch (...) {
                THEMIS_WARN("[VOICE-FALLBACK] liveness check backend failed — fail-closed: session rejected (unknown exception)");
                liveness.is_live = false;
            }
            if (!liveness.is_live) {
                continue;  // skip replay / synthetic samples
            }
        }
        auto fv = extractFeatures(sample);
        if (fv.empty()) {
            continue;
        }
        feature_vectors.push_back(std::move(fv));
        total_quality += quality;
    }

    if (static_cast<int>(feature_vectors.size()) < config.min_samples) {
        return false;  // not enough high-quality samples
    }

    // Average the feature vectors.
    const size_t dim = feature_vectors[0].size();
    std::vector<float> mean_fv(dim, 0.0f);
    for (const auto& fv : feature_vectors) {
        for (size_t i = 0; i < dim; ++i) {
            mean_fv[i] += fv[i];
        }
    }
    const float n = static_cast<float>(feature_vectors.size());
    for (float& v : mean_fv) {
        v /= n;
    }
    l2Normalize(mean_fv);

    std::lock_guard<std::mutex> lock(mutex_);

    // Only one profile per user_id is supported.
    if (user_to_profile_.count(user_id)) {
        return false;
    }

    VoiceProfile profile;
    profile.id             = generateProfileId(user_id);
    profile.user_id        = user_id;
    profile.feature_vector = std::move(mean_fv);
    profile.quality_score  = total_quality / n;
    profile.created_at_ms  = nowMs();
    profile.num_samples    = static_cast<int>(feature_vectors.size());

    out_profile_id               = profile.id;
    profiles_[profile.id]        = std::move(profile);
    user_to_profile_[user_id]    = out_profile_id;
    ++total_enrollments_;

    return true;
}

// ---------------------------------------------------------------------------
// Verification (1:1)
// ---------------------------------------------------------------------------

VerificationResult VoiceBiometricAuthenticator::verify_speaker(
    const VoiceProfileID&        profile_id,
    const std::vector<uint8_t>& audio_sample)
{
    VerificationResult result;
    result.threshold = config_.verification_threshold;

    if (audio_sample.empty()) {
        result.decision_reason = "empty_audio";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = profiles_.find(profile_id);
    if (it == profiles_.end()) {
        result.decision_reason = "profile_not_found";
        return result;
    }

    auto probe_fv = extractFeatures(audio_sample);
    l2Normalize(probe_fv);

    result.match_score = cosineSimilarity(probe_fv, it->second.feature_vector);
    result.verified    = result.match_score >= config_.verification_threshold;
    result.decision_reason = result.verified ? "match" : "score_below_threshold";

    ++total_verifications_;

    return result;
}

// ---------------------------------------------------------------------------
// Identification (1:N)
// ---------------------------------------------------------------------------

IdentificationResult VoiceBiometricAuthenticator::identify_speaker(
    const std::vector<VoiceProfileID>& candidate_profiles,
    const std::vector<uint8_t>&        audio_sample)
{
    IdentificationResult result = {};

    if (audio_sample.empty() || candidate_profiles.empty()) {
        return result;
    }

    auto probe_fv = extractFeatures(audio_sample);
    l2Normalize(probe_fv);

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& pid : candidate_profiles) {
        auto it = profiles_.find(pid);
        if (it == profiles_.end()) {
            continue;
        }
        float score = cosineSimilarity(probe_fv, it->second.feature_vector);
        if (score >= config_.identification_threshold) {
            SpeakerMatch m;
            m.profile_id  = pid;
            m.user_id     = it->second.user_id;
            m.match_score = score;
            result.matches.push_back(m);
        }
    }

    // Sort by score descending and assign ranks.
    std::sort(result.matches.begin(), result.matches.end(),
              [](const SpeakerMatch& a, const SpeakerMatch& b) {
                  return a.match_score > b.match_score;
              });
    for (size_t i = 0; i < static_cast<int>(result.matches.size()); ++i) {
        result.matches[static_cast<size_t>(i)].rank = i + 1;
    }

    if (!result.matches.empty()) {
        result.identified     = true;
        result.top_match_id   = result.matches[0].profile_id;
        result.top_match_score = result.matches[0].match_score;
    }

    ++total_identifications_;

    return result;
}

// ---------------------------------------------------------------------------
// Liveness detection
// ---------------------------------------------------------------------------

LivenessScore VoiceBiometricAuthenticator::detect_liveness(
    const std::vector<uint8_t>& audio_sample)
{
    LivenessScore result = {};

    if (audio_sample.empty()) {
        result.reason = "empty_audio";
        return result;
    }
    if ((audio_sample.size() % 2) != 0 || static_cast<int>(audio_sample.size()) < 320) {
        result.reason = "insufficient_audio";
        return result;
    }

    auto samples = pcmToFloat(audio_sample);
    if (samples.empty()) {
        result.reason = "no_samples";
        return result;
    }
    if (static_cast<int>(samples.size()) < 1600) {
        result.reason = "insufficient_audio";
        return result;
    }

    float mean_abs = 0.0f;
    float mean_abs_delta = 0.0f;
    size_t clipping_count = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        const float a = std::abs(samples[i]);
        mean_abs += a;
        if (a >= 0.98f) {
            ++clipping_count;
        }
        if (i > 0) {
            mean_abs_delta += std::abs(samples[i] - samples[static_cast<int>(i - 1)]);
        }
    }
    mean_abs /= static_cast<float>(samples.size());
    mean_abs_delta /= static_cast<float>(std::max<size_t>(1, static_cast<int>(samples.size()) - 1));

    const float clipping_ratio =
        static_cast<float>(clipping_count) / static_cast<float>(samples.size());
    if (clipping_ratio > 0.20f) {
        result.score = 0.0f;
        result.reason = "clipping_detected";
        return result;
    }
    if (mean_abs < 0.01f) {
        result.score = 0.0f;
        result.reason = "low_energy";
        return result;
    }

    // --- Feature 1: crest factor -------------------------------------------
    // Live speech: crest factor typically 4–10 (= 12–20 dB).
    // Replay / synthesis: often flatter (< 3) or spiky (> 15).
    float rms = 0.0f;
    float peak = 0.0f;
    for (float s : samples) {
        float a = std::abs(s);
        rms  += s * s;
        if (a > peak) {
          peak = a;
        }
    }
    rms = std::sqrt(rms / static_cast<float>(samples.size()));

    float crest = (rms > 1e-9f) ? (peak / rms) : 0.0f;
    // Score peaks at crest ~ 6: live speech is characterized by transients.
    float crest_score = std::max(0.0f, 1.0f - std::abs(crest - 6.0f) / 8.0f);

    // --- Feature 2: spectral flatness (Wiener entropy) ----------------------
    // Live speech has structured harmonics → low flatness.
    // White noise / replay artefacts → flatness closer to 1.
    const size_t n = samples.size();
    float log_sum = 0.0f;
    float arith_sum = 0.0f;
    int   nonzero = 0;
    for (float s : samples) {
        float mag = std::abs(s) + 1e-9f;
        log_sum   += std::log(mag);
        arith_sum += mag;
        ++nonzero;
    }
    float flatness = 1.0f;
    if (nonzero > 0 && arith_sum > 0.0f) {
        float geo_mean   = std::exp(log_sum / static_cast<float>(nonzero));
        float arith_mean = arith_sum / static_cast<float>(nonzero);
        flatness = geo_mean / (arith_mean + 1e-9f);
    }
    // Live speech flatness is low (< 0.5); replay/noise is higher.
    float flatness_score = std::max(0.0f, 1.0f - flatness);

    // --- Feature 3: zero-crossing rate variability --------------------------
    // Live speech has variable ZCR across sub-bands; flat synthesis does not.
    const size_t half = n / 2;
    size_t zcr1 = 0, zcr2 = 0;
    for (size_t i = 1; i < half; ++i) {
        if ((samples[i] >= 0.0f) != (samples[static_cast<int>(i - 1)] >= 0.0f)) {
            ++zcr1;
        }
    }
    for (size_t i = half + 1; i < n; ++i) {
        if ((samples[i] >= 0.0f) != (samples[static_cast<int>(i - 1)] >= 0.0f)) {
            ++zcr2;
        }
    }
    float zcr_ratio = 1.0f;
    if (zcr1 + zcr2 > 0) {
        float z1 = static_cast<float>(zcr1);
        float z2 = static_cast<float>(zcr2);
        // Variability: how different are the two halves?
        zcr_ratio = std::abs(z1 - z2) / (z1 + z2 + 1.0f);
    }
    // Higher variability → more likely live speech.
    float zcr_score = std::min(1.0f, zcr_ratio * 2.0f);

    // --- Feature 4: temporal variability -----------------------------------
    // Live speech exhibits more short-horizon variation than looped replay
    // snippets or adversarially flattened samples.
    float variability_score = std::min(1.0f, mean_abs_delta / 0.12f);

    // --- Feature 5: repeated-frame similarity -------------------------------
    // Replayed or looped audio often repeats adjacent 20 ms windows nearly
    // exactly. Penalize those cases fail-closed.
    constexpr size_t kReplayFrameSamples = 320;  // 20 ms at 16 kHz
    size_t frame_pairs = 0;
    size_t repeated_pairs = 0;
    if (static_cast<int>(samples.size()) >= (2 * kReplayFrameSamples)) {
        for (size_t offset = kReplayFrameSamples;
             offset + kReplayFrameSamples <= samples.size();
             offset += kReplayFrameSamples) {
            float diff_sum = 0.0f;
            float base_sum = 0.0f;
            for (size_t i = 0; i < kReplayFrameSamples; ++i) {
                const float prev = samples[offset - kReplayFrameSamples + i];
                const float curr = samples[offset + i];
                diff_sum += std::abs(curr - prev);
                base_sum += std::abs(prev);
            }
            const float normalized_diff =
                diff_sum / (base_sum + static_cast<float>(kReplayFrameSamples) * 1e-6f);
            if (normalized_diff < 0.08f) {
                ++repeated_pairs;
            }
            ++frame_pairs;
        }
    }
    const float replay_score =
        (frame_pairs == 0)
            ? 1.0f
            : 1.0f - (static_cast<float>(repeated_pairs) / static_cast<float>(frame_pairs));
    if (frame_pairs >= 3 && repeated_pairs * 4 >= frame_pairs * 3) {
        result.score = std::min(0.20f, replay_score);
        result.reason = "replay_like_repetition";
        return result;
    }

    // --- Combine ------------------------------------------------------------
    result.score = 0.25f * crest_score +
                   0.25f * flatness_score +
                   0.15f * zcr_score +
                   0.15f * variability_score +
                   0.20f * replay_score;
    result.score = std::min(1.0f, std::max(0.0f, result.score));
    result.is_live = result.score >= config_.liveness_threshold;
    if (result.is_live) {
        result.reason = "live_speech";
    } else if (replay_score < 0.35f) {
        result.reason = "replay_like_repetition";
    } else if (variability_score < 0.20f) {
        result.reason = "low_variability";
    } else {
        result.reason = "suspected_replay";
    }

    return result;
}

// ---------------------------------------------------------------------------
// Full authentication
// ---------------------------------------------------------------------------

VoiceAuthResult VoiceBiometricAuthenticator::authenticate(
    const std::string&          user_id,
    const std::vector<uint8_t>& audio_sample)
{
    VoiceAuthResult result;
    result.threshold    = config_.verification_threshold;
    result.timestamp_ms = nowMs();

    if (user_id.empty()) {
        result.decision_reason = "empty_user_id";
        emitAuthAuditEvent(user_id, result);
        return result;
    }
    if (audio_sample.empty()) {
        result.decision_reason = "empty_audio";
        emitAuthAuditEvent(user_id, result);
        return result;
    }

    // 1. Liveness check
    // Wave-A V2: partial backend failure matrix — liveness backend fallback (fail-closed)
    // If liveness backend throws, reject the session for security.
    LivenessScore liveness;
    try {
        liveness = detect_liveness(audio_sample);
    } catch (const std::exception& e) {
        THEMIS_WARN("[VOICE-FALLBACK] liveness check backend failed — fail-closed: session rejected: {}", e.what());
        liveness.is_live = false;
        liveness.reason = "liveness_backend_exception";
    } catch (...) {
        THEMIS_WARN("[VOICE-FALLBACK] liveness check backend failed — fail-closed: session rejected (unknown exception)");
        liveness.is_live = false;
        liveness.reason = "liveness_backend_unknown_exception";
    }
    if (!liveness.is_live) {
        result.decision_reason = "liveness_failed: " + liveness.reason;
        emitAuthAuditEvent(user_id, result);
        return result;
    }

    // 2. Look up profile
    VoiceProfileID pid;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = user_to_profile_.find(user_id);
        if (it != user_to_profile_.end()) {
            pid = it->second;
        }
    }
    if (pid.empty()) {
        result.decision_reason = "profile_not_found";
        emitAuthAuditEvent(user_id, result);
        return result;
    }

    // 3. Verify speaker (acquires lock internally)
    auto verification = verify_speaker(pid, audio_sample);
    result.confidence_score = verification.match_score;

    if (!verification.verified) {
        result.decision_reason =
            "verification_failed: " + verification.decision_reason;
        emitAuthAuditEvent(user_id, result);
        return result;
    }

    result.authenticated   = true;
    result.user_id         = user_id;
    result.decision_reason = "authenticated";

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++successful_authentications_;
    }

    emitAuthAuditEvent(user_id, result);

    return result;
}

// ---------------------------------------------------------------------------
// Profile management
// ---------------------------------------------------------------------------

bool VoiceBiometricAuthenticator::delete_profile(
    const VoiceProfileID& profile_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = profiles_.find(profile_id);
    if (it == profiles_.end()) {
        return false;
    }
    user_to_profile_.erase(it->second.user_id);
    profiles_.erase(it);
    return true;
}

bool VoiceBiometricAuthenticator::has_profile(
    const VoiceProfileID& profile_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return profiles_.count(profile_id) > 0;
}

std::vector<VoiceProfileID> VoiceBiometricAuthenticator::list_profiles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<VoiceProfileID> ids = {};

    ids.reserve(profiles_.size());
    for (const auto& kv : profiles_) {
        ids.push_back(kv.first);
    }
    return ids;
}

std::optional<std::string> VoiceBiometricAuthenticator::get_user_id(
    const VoiceProfileID& profile_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = profiles_.find(profile_id);
    if (it == profiles_.end()) {
        return std::nullopt;
    }
    return it->second.user_id;
}

// ---------------------------------------------------------------------------
// Configuration & statistics
// ---------------------------------------------------------------------------

void VoiceBiometricAuthenticator::set_config(const VoiceAuthConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void VoiceBiometricAuthenticator::setAuthAuditCallback(
    std::function<void(const std::string&, const VoiceAuthResult&)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auth_audit_callback_ = std::move(callback);
}

VoiceAuthConfig VoiceBiometricAuthenticator::get_config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

json VoiceBiometricAuthenticator::get_statistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    json stats;
    stats["enrolled_profiles"]        = profiles_.size();
    stats["total_enrollments"]         = total_enrollments_;
    stats["total_verifications"]       = total_verifications_;
    stats["total_identifications"]     = total_identifications_;
    stats["successful_authentications"]= successful_authentications_;
    stats["total_auth_audit_events"]   = total_auth_audit_events_;
    return stats;
}

void VoiceBiometricAuthenticator::emitAuthAuditEvent(
    const std::string& claimed_user_id,
    const VoiceAuthResult& result)
{
    std::function<void(const std::string&, const VoiceAuthResult&)> callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++total_auth_audit_events_;
        callback = auth_audit_callback_;
    }

    if (callback) {
        try {
            callback(claimed_user_id, result);
        } catch (...) {
            // Audit callbacks must never affect authentication results.
        }
    }
}

// ---------------------------------------------------------------------------
// Private: feature extraction
// ---------------------------------------------------------------------------

/**
 * Compute a 32-dimensional acoustic feature vector from raw PCM audio.
 *
 * Sub-band layout (8 equal-width bands):
 *   Indices 0–7:   Sub-band RMS energy (normalised by overall RMS)
 *   Indices 8–15:  Sub-band zero-crossing rate (normalised)
 *   Index  16:     Spectral centroid (normalised to [0,1])
 *   Index  17:     Spectral spread (normalised)
 *   Index  18:     Spectral skewness
 *   Index  19:     Spectral kurtosis
 *   Index  20:     Global RMS energy
 *   Index  21:     Crest factor (normalised to [0,1])
 *   Index  22:     Peak amplitude
 *   Index  23:     Spectral flatness
 *   Indices 24–31: Delta sub-band RMS (band[i+1] – band[i], padded)
 */
std::vector<float> VoiceBiometricAuthenticator::extractFeatures(
    const std::vector<uint8_t>& audio) const
{
    constexpr int kBands = 8;
    constexpr int kDim   = 32;

    auto samples = pcmToFloat(audio);
    std::vector<float> fv(kDim, 0.0f);

    if (samples.empty()) {
        return fv;
    }

    const size_t n = samples.size();

    // --- Global RMS ----------------------------------------------------------
    float global_rms = 0.0f;
    float peak       = 0.0f;
    for (float s : samples) {
        global_rms += s * s;
        float a = std::abs(s);
        if (a > peak) {
          peak = a;
        }
    }
    global_rms = std::sqrt(global_rms / static_cast<float>(n));

    // --- Sub-band features --------------------------------------------------
    const size_t band_size = n / kBands;
    if (band_size == 0) {
        // Too few samples: fill with global RMS only.
        fv[20] = global_rms;
        return fv;
    }

    float max_band_rms = 0.0f;
    float max_zcr      = 0.0f;
    std::vector<float> band_rms(kBands, 0.0f);
    std::vector<float> band_zcr(kBands, 0.0f);

    for (int b = 0; b < kBands; ++b) {
        size_t start = static_cast<size_t>(b) * band_size;
        size_t end   = (b == kBands - 1) ? n : start + band_size;
        size_t len   = end - start;

        float rms = 0.0f;
        size_t zc = 0;
        for (size_t i = start; i < end; ++i) {
            rms += samples[i] * samples[i];
        }
        rms = std::sqrt(rms / static_cast<float>(len));

        for (size_t i = start + 1; i < end; ++i) {
            if ((samples[i] >= 0.0f) != (samples[static_cast<int>(i - 1)] >= 0.0f)) {
                ++zc;
            }
        }

        band_rms[b] = rms;
        band_zcr[b] = static_cast<float>(zc) / static_cast<float>(len);

        if (rms > max_band_rms) {
          max_band_rms = rms;
        }
        if (band_zcr[b] > max_zcr) {
          max_zcr = band_zcr[b];
        }
    }

    // Normalise sub-band features (avoid division by zero).
    for (int b = 0; b < kBands; ++b) {
        fv[b]         = (max_band_rms > 1e-9f) ? (band_rms[b] / max_band_rms) : 0.0f;
        fv[kBands + b] = (max_zcr > 1e-9f) ? (band_zcr[b] / max_zcr) : 0.0f;
    }

    // --- Spectral moments ---------------------------------------------------
    // Use sub-band index as a proxy for "frequency bin".
    float w_sum = 0.0f, w_sq_sum = 0.0f, energy_sum = 0.0f;
    for (int b = 0; b < kBands; ++b) {
        float e = band_rms[b] * band_rms[b];
        float fb = static_cast<float>(b);
        w_sum    += fb * e;
        w_sq_sum += fb * fb * e;
        energy_sum += e;
    }
    float centroid = 0.0f;
    float spread   = 0.0f;
    if (energy_sum > 1e-9f) {
        centroid = w_sum / energy_sum / static_cast<float>(kBands - 1); // [0,1]
        float raw_spread = w_sq_sum / energy_sum - (w_sum / energy_sum) * (w_sum / energy_sum);
        spread = std::sqrt(std::max(0.0f, raw_spread)) / static_cast<float>(kBands - 1);
    }
    // Skewness and kurtosis of sub-band energies (mean-normalised).
    float mean_e = energy_sum / kBands;
    float var_e  = 0.0f, skew_e = 0.0f, kurt_e = 0.0f;
    for (int b = 0; b < kBands; ++b) {
        float d = band_rms[b] * band_rms[b] - mean_e;
        var_e  += d * d;
        skew_e += d * d * d;
        kurt_e += d * d * d * d;
    }
    var_e /= kBands;
    float std_e = std::sqrt(std::max(1e-18f, var_e));
    skew_e = (std_e > 1e-9f) ? (skew_e / (kBands * std_e * std_e * std_e)) : 0.0f;
    kurt_e = (var_e > 1e-9f) ? (kurt_e / (kBands * var_e * var_e)) : 0.0f;

    fv[16] = centroid;
    fv[17] = std::min(1.0f, spread);
    fv[18] = std::tanh(skew_e);         // bound to (-1,1)
    fv[19] = std::tanh(kurt_e - 3.0f);  // excess kurtosis, bounded

    // --- Global temporal features ------------------------------------------
    fv[20] = std::min(1.0f, global_rms);
    float crest = (global_rms > 1e-9f) ? (peak / global_rms) : 0.0f;
    fv[21] = std::min(1.0f, crest / 20.0f); // normalise (typical max ~20)
    fv[22] = std::min(1.0f, peak);

    // Spectral flatness (Wiener entropy over sub-band energies).
    float log_geo = 0.0f, arith_e = 0.0f;
    for (int b = 0; b < kBands; ++b) {
        float e = band_rms[b] + 1e-9f;
        log_geo  += std::log(e);
        arith_e  += e;
    }
    float geo_e = std::exp(log_geo / kBands);
    float flatness = (arith_e > 0.0f) ? (geo_e / (arith_e / kBands)) : 0.0f;
    fv[23] = std::min(1.0f, flatness);

    // --- Delta sub-band RMS features ----------------------------------------
    for (int b = 0; b < kBands - 1; ++b) {
        fv[24 + b] = std::tanh(band_rms[b + 1] - band_rms[b]);
    }
    fv[31] = 0.0f; // padding for last delta

    return fv;
}

std::vector<float> VoiceBiometricAuthenticator::pcmToFloat(
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

void VoiceBiometricAuthenticator::l2Normalize(std::vector<float>& vec) const {
    float norm = 0.0f;
    for (float v : vec) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm < 1e-9f) {
        return;
    }
    for (float& v : vec) {
        v /= norm;
    }
}

float VoiceBiometricAuthenticator::cosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) const
{
    if (static_cast<int>(a.size()) != static_cast<int>(b.size()) || a.empty()) {
        return 0.0f;
    }
    float dot = 0.0f;
    float na  = 0.0f;
    float nb  = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    if (denom < 1e-9f) {
        return 0.0f;
    }
    // Map cosine similarity from [-1, 1] to [0, 1].
    return (dot / denom + 1.0f) * 0.5f;
}

float VoiceBiometricAuthenticator::computeAudioQuality(
    const std::vector<float>& samples) const
{
    if (samples.empty()) {
        return 0.0f;
    }
    // Quality is a combination of:
    // 1. Sufficient RMS energy (too quiet → low quality).
    float rms = 0.0f;
    for (float s : samples) {
        rms += s * s;
    }
    rms = std::sqrt(rms / static_cast<float>(samples.size()));
    float energy_score = std::min(1.0f, rms / 0.05f); // saturates at 5% full-scale

    // 2. No clipping (peak < 0.98).
    float peak = 0.0f;
    for (float s : samples) {
        float a = std::abs(s);
        if (a > peak) {
          peak = a;
        }
    }
    float clip_score = (peak < 0.98f) ? 1.0f : 0.5f;

    // 3. Sufficient duration: at least 1 second of 16 kHz audio (16000 samples).
    float dur_score = std::min(1.0f, static_cast<float>(samples.size()) / 16000.0f);

    return 0.4f * energy_score + 0.3f * clip_score + 0.3f * dur_score;
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

int64_t VoiceBiometricAuthenticator::nowMs() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}

std::string VoiceBiometricAuthenticator::generateProfileId(
    const std::string& user_id) const
{
    // Combine user_id with a timestamp hash to produce a unique ID.
    int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss = {};
    oss << "vp_" << user_id << "_" << ts;
    return oss.str();
}

} // namespace voice
} // namespace themis

