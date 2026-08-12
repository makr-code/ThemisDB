/**
 * @file voice_tts_customizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// TTS Customization API – Phase 2 production readiness
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Prosody control
struct ProsodyConfig {
    float pitch = 1.0f;        // 0.5–2.0 (1.0 = neutral)
    float speed = 1.0f;        // 0.5–2.0 (1.0 = normal)
    float volume = 1.0f;       // 0.0–2.0 (1.0 = normal)
    float emphasis = 1.0f;     // 0.5–2.0 (stress/emphasis)
    std::string style;         // "neutral", "excited", "sad", "professional", "conversational"
    bool enable_ssml = false;  // parse SSML tags in text
};

// Voice profile
struct VoiceProfile {
    std::string id;
    std::string name;
    std::string language;          // BCP-47 code: "en-US", "de-DE", etc.
    std::string gender;            // "male", "female", "neutral"
    std::string engine;            // "piper", "gtts", "espeak", "custom"
    std::string model_path;
    ProsodyConfig default_prosody;
    std::vector<std::string> supported_styles;
    std::vector<std::string> supported_languages;
};

// SSML processing result
struct SSMLResult {
    std::string plain_text;               // Text with SSML tags stripped
    std::vector<ProsodyConfig> segments;  // Per-segment prosody
    bool has_breaks = false;
    bool has_emphasis = false;
};

// SSML sanitization result (injection prevention)
struct SSMLSanitizeResult {
    std::string sanitized_text;              // SSML with only allowlisted tags retained
    std::vector<std::string> rejected_tags;  // Tag names that were stripped (lowercased)
    bool had_injection_attempt = false;      // True if any disallowed content was found
};

// MOS (Mean Opinion Score) quality metrics
struct MOSMetrics {
    float mos_score = 0.0f;        // 1.0–5.0 (5.0 = excellent)
    float naturalness = 0.0f;      // 0–1: naturalness of speech
    float intelligibility = 0.0f;  // 0–1: clarity
    float rhythm_score = 0.0f;     // 0–1: prosody rhythm quality
    std::string quality_label;     // "poor"/"fair"/"good"/"excellent"
    json details;
};

// Multi-language voice registry entry
struct LanguageVoice {
    std::string language_code;     // BCP-47 code
    std::string language_name;     // Human-readable name
    std::vector<std::string> voice_ids;
    std::string default_voice_id;
};

// TTS customization config
struct TTSCustomizerConfig {
    ProsodyConfig default_prosody;
    std::string default_engine = "piper";
    bool auto_detect_language = false;
    bool enable_quality_metrics = true;
    std::vector<VoiceProfile> profiles;
};

// VoiceTTSCustomizer: Phase 2 production component
/** @brief VoiceTTSCustomizer: Phase 2 production component. */
class VoiceTTSCustomizer {
public:
    explicit VoiceTTSCustomizer(const TTSCustomizerConfig& config = {});
    ~VoiceTTSCustomizer() = default;

    // Voice profile management
    bool registerVoiceProfile(const VoiceProfile& profile);
    bool hasProfile(const std::string& voice_id) const;
    std::optional<VoiceProfile> getProfile(const std::string& voice_id) const;
    std::vector<VoiceProfile> listProfiles() const;
    std::vector<VoiceProfile> getProfilesForLanguage(const std::string& lang) const;

    // Prosody control – merge custom prosody on top of profile defaults
    ProsodyConfig buildProsody(
        const std::string& voice_id,
        const ProsodyConfig& overrides = {}
    ) const;

    // Clamp all prosody values to valid ranges
    ProsodyConfig validateProsody(const ProsodyConfig& p) const;

    // SSML processing: strip tags, extract prosody hints
    SSMLResult parseSSML(const std::string& ssml_text) const;

    // SSML injection prevention: validate tags against allowlist and strip
    // disallowed content; returns the sanitized SSML and a flag indicating
    // whether an injection attempt was detected.
    SSMLSanitizeResult sanitizeSSML(const std::string& ssml_input) const;

    // Returns true if ssml_input contains only allowlisted tags/attributes.
    bool isSSMLSafe(const std::string& ssml_input) const;

    // Quality metrics
    MOSMetrics estimateMOS(const std::vector<uint8_t>& audio_data, int sample_rate = 22050) const;
    MOSMetrics estimateMOSFromText(const std::string& input_text, const std::string& output_text) const;

    // Multi-language synthesis support
    void registerLanguageVoice(const LanguageVoice& lv);
    std::vector<LanguageVoice> getSupportedLanguages() const;
    std::string getBestVoiceForLanguage(const std::string& lang_code) const;
    bool supportsLanguage(const std::string& lang_code) const;

    // Statistics
    json getStatistics() const;

private:
    TTSCustomizerConfig config_;
    std::map<std::string, VoiceProfile> profiles_;
    std::map<std::string, LanguageVoice> language_voices_;

    void loadDefaultProfiles();
    void loadDefaultLanguageVoices();

    float computeSignalEnergy(const std::vector<uint8_t>& audio, int sample_rate) const;
    float computeSpeechRhythm(const std::vector<uint8_t>& audio, int sample_rate) const;
    std::string classifyMOS(float mos) const;
};

}} // namespace themis::voice
