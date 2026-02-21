/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_tts_customizer.h                             ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 19:14:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     148                                            ║
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
