/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_auth.h                                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:10:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     339                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file voice_auth.h
 * @brief Voice biometric authentication — speaker verification and identification.
 *
 * Provides enrollment, 1:1 speaker verification, 1:N identification, liveness
 * detection, and a combined authenticate() helper.  The implementation is
 * intentionally model-free: voice profiles are built from spectral sub-band
 * feature vectors that can be computed without external model files, while the
 * public API is designed so that a future neural i-vector/x-vector backend can
 * be plugged in without breaking callers.
 *
 * Typical usage:
 * @code
 *   themis::voice::VoiceBiometricAuthenticator auth;
 *
 *   // Enroll
 *   std::vector<std::vector<uint8_t>> samples = { s1, s2, s3 };
 *   themis::voice::VoiceProfileID pid;
 *   auth.enroll_voice("alice", samples, pid);
 *
 *   // Verify
 *   auto r = auth.verify_speaker(pid, probe_audio);
 *   if (r.verified) { ... }
 *
 *   // Full auth
 *   auto a = auth.authenticate("alice", probe_audio);
 *   if (a.authenticated) { ... }
 * @endcode
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------

/** Opaque identifier for a stored voice profile. */
using VoiceProfileID = std::string;

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the enrollment process.
 */
struct EnrollmentConfig {
    int   min_samples        = 3;     ///< Minimum number of audio samples required
    int   sample_duration_ms = 3000;  ///< Minimum duration of each sample (ms)
    float quality_threshold  = 0.60f; ///< Minimum audio quality score [0, 1]
    bool  require_liveness   = true;  ///< Reject synthetic/replayed audio
};

/**
 * @brief Result of a 1:1 speaker verification attempt.
 */
struct VerificationResult {
    bool        verified        = false; ///< True when the sample matches the profile
    float       match_score     = 0.0f;  ///< Cosine similarity [0, 1]
    float       threshold       = 0.0f;  ///< Decision threshold used
    std::string decision_reason;         ///< Human-readable explanation
};

/**
 * @brief A single ranked match from an identification search.
 */
struct SpeakerMatch {
    VoiceProfileID profile_id;          ///< Matched profile
    std::string    user_id;             ///< Associated user identifier
    float          match_score = 0.0f;  ///< Similarity score [0, 1]
    int            rank        = 0;     ///< 1-based rank (1 = best)
};

/**
 * @brief Result of a 1:N speaker identification attempt.
 */
struct IdentificationResult {
    std::vector<SpeakerMatch> matches;          ///< All candidates, sorted by score desc
    bool                      identified = false; ///< True if any match exceeds threshold
    std::string               top_match_id;      ///< Profile ID of the best match
    float                     top_match_score = 0.0f; ///< Best match score
};

/**
 * @brief Liveness detection result (anti-spoofing).
 */
struct LivenessScore {
    bool        is_live = false; ///< True when input appears to be live speech
    float       score   = 0.0f; ///< Liveness confidence [0, 1]
    std::string reason;          ///< Explanation (e.g. "replay_detected")
};

/**
 * @brief Combined authentication result (liveness + verification).
 */
struct VoiceAuthResult {
    bool        authenticated  = false; ///< True when fully authenticated
    float       confidence_score = 0.0f; ///< Verification similarity [0, 1]
    float       threshold      = 0.0f;  ///< Threshold used for decision
    std::string user_id;                ///< Authenticated user (on success)
    std::string decision_reason;        ///< Human-readable explanation
    int64_t     timestamp_ms   = 0;     ///< Wall-clock ms at authentication time
};

/**
 * @brief Runtime configuration for VoiceBiometricAuthenticator.
 */
struct VoiceAuthConfig {
    float verification_threshold   = 0.72f; ///< Min score to accept 1:1 verification
    float identification_threshold = 0.68f; ///< Min score to include a 1:N candidate
    float liveness_threshold       = 0.55f; ///< Min score to accept as live speech
    int   feature_vector_size      = 32;    ///< Dimension of internal feature vectors
};

// ---------------------------------------------------------------------------
// VoiceBiometricAuthenticator
// ---------------------------------------------------------------------------

/**
 * @brief Voice biometric authentication: enrollment, verification, identification.
 *
 * Thread-safe.  All public methods acquire an internal mutex.
 *
 * Feature extraction
 * ------------------
 * Each audio sample is converted to 16-bit PCM floats, split into 8 equal
 * sub-bands, and the following per-band statistics are computed:
 *  - RMS energy (8 values)
 *  - Zero-crossing rate (8 values)
 * Plus four global spectral-moment features (centroid, spread, skewness,
 * kurtosis) and four temporal features (global RMS, crest factor, peak energy,
 * spectral flatness), totalling 24 base features.  Eight delta features
 * (consecutive sub-band RMS differences) are appended to reach 32 dimensions.
 *
 * Profiles are stored as the L2-normalised average over all enrollment
 * samples.  Verification uses cosine similarity between the normalised probe
 * vector and the stored profile vector.
 */
class VoiceBiometricAuthenticator {
public:
    /**
     * @brief Construct an authenticator with the given configuration.
     * @param config  Runtime parameters (thresholds, feature dimension).
     */
    explicit VoiceBiometricAuthenticator(const VoiceAuthConfig& config = {});
    ~VoiceBiometricAuthenticator() = default;

    // Non-copyable, movable
    VoiceBiometricAuthenticator(const VoiceBiometricAuthenticator&)            = delete;
    VoiceBiometricAuthenticator& operator=(const VoiceBiometricAuthenticator&) = delete;
    VoiceBiometricAuthenticator(VoiceBiometricAuthenticator&&)                 = default;
    VoiceBiometricAuthenticator& operator=(VoiceBiometricAuthenticator&&)      = default;

    // -----------------------------------------------------------------------
    // Enrollment
    // -----------------------------------------------------------------------

    /**
     * @brief Enroll a new speaker from multiple audio samples.
     *
     * Builds an average feature vector over all provided samples and stores a
     * voice profile keyed by the generated @p out_profile_id.
     *
     * @param user_id         Identifier of the person being enrolled.
     * @param audio_samples   At least EnrollmentConfig::min_samples raw PCM buffers.
     * @param out_profile_id  Set to the new profile ID on success.
     * @param config          Enrollment parameters.
     * @return true on success; false if too few samples, quality too low, or
     *         a profile already exists for @p user_id.
     */
    bool enroll_voice(
        const std::string&                        user_id,
        const std::vector<std::vector<uint8_t>>& audio_samples,
        VoiceProfileID&                           out_profile_id,
        const EnrollmentConfig&                   config = {});

    // -----------------------------------------------------------------------
    // Verification & identification
    // -----------------------------------------------------------------------

    /**
     * @brief Verify that @p audio_sample matches the given profile (1:1).
     *
     * @param profile_id   Previously enrolled profile identifier.
     * @param audio_sample Raw PCM probe audio.
     * @return VerificationResult with verified==true when match_score ≥ threshold.
     */
    VerificationResult verify_speaker(
        const VoiceProfileID&          profile_id,
        const std::vector<uint8_t>&   audio_sample);

    /**
     * @brief Search for the speaker among a set of candidate profiles (1:N).
     *
     * @param candidate_profiles  Profiles to compare against.
     * @param audio_sample        Raw PCM probe audio.
     * @return IdentificationResult with all matches above the identification
     *         threshold, sorted by score descending.
     */
    IdentificationResult identify_speaker(
        const std::vector<VoiceProfileID>& candidate_profiles,
        const std::vector<uint8_t>&        audio_sample);

    /**
     * @brief Detect liveness: distinguish live speech from replay/synthesis.
     *
     * Uses crest factor and spectral flatness heuristics.  A future neural
     * anti-spoofing model can replace this method without API changes.
     *
     * @param audio_sample  Raw PCM audio.
     * @return LivenessScore with is_live==true when the sample appears genuine.
     */
    LivenessScore detect_liveness(const std::vector<uint8_t>& audio_sample);

    /**
     * @brief Full biometric authentication: liveness check + 1:1 verification.
     *
     * Looks up the profile associated with @p user_id, runs liveness detection,
     * then verifies the speaker.  Returns authenticated==true only if both
     * checks pass.
     *
     * @param user_id       User claiming identity.
     * @param audio_sample  Raw PCM probe audio.
     * @return VoiceAuthResult with authenticated==true on full success.
     */
    VoiceAuthResult authenticate(
        const std::string&          user_id,
        const std::vector<uint8_t>& audio_sample);

    // -----------------------------------------------------------------------
    // Profile management
    // -----------------------------------------------------------------------

    /**
     * @brief Delete a voice profile.
     * @return true if the profile existed and was removed; false otherwise.
     */
    bool delete_profile(const VoiceProfileID& profile_id);

    /** @brief Return true if the given profile ID exists. */
    bool has_profile(const VoiceProfileID& profile_id) const;

    /** @brief List all registered profile IDs. */
    std::vector<VoiceProfileID> list_profiles() const;

    /** @brief Return the user_id associated with a profile, if it exists. */
    std::optional<std::string> get_user_id(const VoiceProfileID& profile_id) const;

    // -----------------------------------------------------------------------
    // Configuration & statistics
    // -----------------------------------------------------------------------

    /** @brief Update runtime configuration (thread-safe). */
    void set_config(const VoiceAuthConfig& config);

    /** @brief Return a copy of the current configuration. */
    VoiceAuthConfig get_config() const;

    /** @brief Return JSON statistics (profiles enrolled, verifications, etc.). */
    json get_statistics() const;

private:
    // Stored voice profile
    struct VoiceProfile {
        VoiceProfileID      id;
        std::string         user_id;
        std::vector<float>  feature_vector; ///< L2-normalised mean enrollment vector
        float               quality_score  = 0.0f;
        int64_t             created_at_ms  = 0;
        int                 num_samples    = 0;
    };

    VoiceAuthConfig config_;
    mutable std::mutex mutex_;

    std::map<VoiceProfileID, VoiceProfile> profiles_;
    std::map<std::string, VoiceProfileID>  user_to_profile_; ///< user_id → profile_id

    uint64_t total_enrollments_    = 0;
    uint64_t total_verifications_  = 0;
    uint64_t total_identifications_= 0;
    uint64_t successful_authentications_ = 0;

    // Feature extraction pipeline
    std::vector<float> extractFeatures(const std::vector<uint8_t>& audio) const;
    std::vector<float> pcmToFloat(const std::vector<uint8_t>& raw) const;
    void               l2Normalize(std::vector<float>& vec) const;

    // Similarity & quality
    float cosineSimilarity(const std::vector<float>& a,
                           const std::vector<float>& b) const;
    float computeAudioQuality(const std::vector<float>& samples) const;

    // Utilities
    int64_t     nowMs() const;
    std::string generateProfileId(const std::string& user_id) const;
};

} // namespace voice
} // namespace themis
