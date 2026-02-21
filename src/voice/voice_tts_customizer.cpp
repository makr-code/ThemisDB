/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_tts_customizer.cpp                           ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 14:17:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     381                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 6632e4e56  2026-02-21  Add License Portal and Renewal Reminder classes for ThemisDB ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "voice/voice_tts_customizer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <numeric>
#include <regex>

namespace themis { namespace voice {

VoiceTTSCustomizer::VoiceTTSCustomizer(const TTSCustomizerConfig& config)
    : config_(config)
{
    loadDefaultProfiles();
    loadDefaultLanguageVoices();
    // Register any profiles provided in config
    for (const auto& p : config_.profiles) {
        profiles_.emplace(p.id, p);
    }
}

void VoiceTTSCustomizer::loadDefaultProfiles() {
    auto addProfile = [&](const std::string& id, const std::string& name,
                          const std::string& lang, const std::string& gender,
                          const std::string& engine) {
        VoiceProfile p;
        p.id = id;
        p.name = name;
        p.language = lang;
        p.gender = gender;
        p.engine = engine;
        p.supported_styles = {"neutral", "professional", "conversational"};
        p.supported_languages = {lang};
        profiles_.emplace(id, std::move(p));
    };
    addProfile("en-default", "English Default",  "en-US", "neutral", "piper");
    addProfile("en-male",    "English Male",      "en-US", "male",    "piper");
    addProfile("en-female",  "English Female",    "en-US", "female",  "piper");
    addProfile("de-default", "German Default",    "de-DE", "neutral", "piper");
    addProfile("fr-default", "French Default",    "fr-FR", "neutral", "piper");
}

void VoiceTTSCustomizer::loadDefaultLanguageVoices() {
    auto addLang = [&](const std::string& code, const std::string& name,
                       const std::string& default_id,
                       const std::vector<std::string>& ids) {
        LanguageVoice lv;
        lv.language_code = code;
        lv.language_name = name;
        lv.default_voice_id = default_id;
        lv.voice_ids = ids;
        language_voices_.emplace(code, std::move(lv));
    };
    addLang("en", "English",  "en-default", {"en-default", "en-male", "en-female"});
    addLang("en-US", "English (US)", "en-default", {"en-default", "en-male", "en-female"});
    addLang("de", "German",   "de-default", {"de-default"});
    addLang("de-DE", "German (Germany)", "de-default", {"de-default"});
    addLang("fr", "French",   "fr-default", {"fr-default"});
    addLang("fr-FR", "French (France)", "fr-default", {"fr-default"});
    addLang("es", "Spanish",  "",           {});
    addLang("it", "Italian",  "",           {});
    addLang("zh", "Chinese",  "",           {});
    addLang("ja", "Japanese", "",           {});
}

bool VoiceTTSCustomizer::registerVoiceProfile(const VoiceProfile& profile) {
    if (profiles_.count(profile.id)) {
        return false; // already exists
    }
    profiles_.emplace(profile.id, profile);
    return true;
}

bool VoiceTTSCustomizer::hasProfile(const std::string& voice_id) const {
    return profiles_.count(voice_id) > 0;
}

std::optional<VoiceProfile> VoiceTTSCustomizer::getProfile(const std::string& voice_id) const {
    auto it = profiles_.find(voice_id);
    if (it == profiles_.end()) return std::nullopt;
    return it->second;
}

std::vector<VoiceProfile> VoiceTTSCustomizer::listProfiles() const {
    std::vector<VoiceProfile> result;
    result.reserve(profiles_.size());
    for (const auto& [id, p] : profiles_) {
        result.push_back(p);
    }
    return result;
}

std::vector<VoiceProfile> VoiceTTSCustomizer::getProfilesForLanguage(const std::string& lang) const {
    std::vector<VoiceProfile> result;
    for (const auto& [id, p] : profiles_) {
        if (p.language == lang ||
            (p.language.size() >= 2 && lang.size() >= 2 &&
             p.language.substr(0, 2) == lang.substr(0, 2))) {
            result.push_back(p);
        }
    }
    return result;
}

ProsodyConfig VoiceTTSCustomizer::buildProsody(
    const std::string& voice_id,
    const ProsodyConfig& overrides) const
{
    // Start with global default
    ProsodyConfig base = config_.default_prosody;

    // Overlay profile default if found
    auto it = profiles_.find(voice_id);
    if (it != profiles_.end()) {
        base = it->second.default_prosody;
    }

    // Apply overrides: non-default values win
    ProsodyConfig def{};
    if (overrides.pitch != def.pitch)       base.pitch = overrides.pitch;
    if (overrides.speed != def.speed)       base.speed = overrides.speed;
    if (overrides.volume != def.volume)     base.volume = overrides.volume;
    if (overrides.emphasis != def.emphasis) base.emphasis = overrides.emphasis;
    if (!overrides.style.empty())           base.style = overrides.style;
    if (overrides.enable_ssml)              base.enable_ssml = true;

    return validateProsody(base);
}

ProsodyConfig VoiceTTSCustomizer::validateProsody(const ProsodyConfig& p) const {
    ProsodyConfig out = p;
    out.pitch    = std::clamp(out.pitch,    0.5f, 2.0f);
    out.speed    = std::clamp(out.speed,    0.25f, 4.0f);
    out.volume   = std::clamp(out.volume,   0.0f, 2.0f);
    out.emphasis = std::clamp(out.emphasis, 0.5f, 2.0f);
    return out;
}

SSMLResult VoiceTTSCustomizer::parseSSML(const std::string& ssml_text) const {
    SSMLResult result;

    // Detect breaks and emphasis before stripping
    result.has_breaks   = (ssml_text.find("<break") != std::string::npos);
    result.has_emphasis = (ssml_text.find("<emphasis") != std::string::npos);

    // Extract <prosody rate="X" pitch="X"> attributes into a segment
    // Simple manual extraction to avoid heavy regex overhead
    std::string work = ssml_text;
    size_t pos = 0;
    while ((pos = work.find("<prosody", pos)) != std::string::npos) {
        size_t end = work.find('>', pos);
        if (end == std::string::npos) break;
        std::string tag = work.substr(pos, end - pos + 1);

        ProsodyConfig seg;
        // Extract rate attribute
        auto extractAttr = [&](const std::string& attr) -> std::string {
            size_t a = tag.find(attr + "=\"");
            if (a == std::string::npos) return {};
            size_t vs = a + attr.size() + 2;
            size_t ve = tag.find('"', vs);
            if (ve == std::string::npos) return {};
            return tag.substr(vs, ve - vs);
        };

        std::string rate_str = extractAttr("rate");
        std::string pitch_str = extractAttr("pitch");
        if (!rate_str.empty()) {
            try { seg.speed = std::stof(rate_str); } catch (...) {}
        }
        if (!pitch_str.empty()) {
            try { seg.pitch = std::stof(pitch_str); } catch (...) {}
        }
        seg = validateProsody(seg);
        result.segments.push_back(seg);
        pos = end + 1;
    }

    // Strip all XML tags
    std::string plain;
    plain.reserve(ssml_text.size());
    bool in_tag = false;
    for (char c : ssml_text) {
        if (c == '<') { in_tag = true; continue; }
        if (c == '>') { in_tag = false; continue; }
        if (!in_tag) plain += c;
    }

    // Collapse whitespace
    std::string collapsed;
    bool last_space = true;
    for (char c : plain) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_space) { collapsed += ' '; last_space = true; }
        } else {
            collapsed += c;
            last_space = false;
        }
    }
    // Trim trailing space
    if (!collapsed.empty() && collapsed.back() == ' ') collapsed.pop_back();

    result.plain_text = std::move(collapsed);
    return result;
}

float VoiceTTSCustomizer::computeSignalEnergy(
    const std::vector<uint8_t>& audio, int /*sample_rate*/) const
{
    if (audio.size() < 2) return 0.0f;
    size_t n_samples = audio.size() / 2;
    double energy = 0.0;
    for (size_t i = 0; i < n_samples; ++i) {
        int16_t s = static_cast<int16_t>(
            static_cast<uint16_t>(audio[2*i]) |
            (static_cast<uint16_t>(audio[2*i+1]) << 8));
        energy += static_cast<double>(s) * static_cast<double>(s);
    }
    return static_cast<float>(std::sqrt(energy / static_cast<double>(n_samples)));
}

float VoiceTTSCustomizer::computeSpeechRhythm(
    const std::vector<uint8_t>& audio, int sample_rate) const
{
    if (audio.size() < 2 || sample_rate <= 0) return 0.5f;
    // Simple rhythm: variance in energy across 100ms windows
    size_t window = static_cast<size_t>(sample_rate * 0.1);
    size_t n_samples = audio.size() / 2;
    if (n_samples < window) return 0.5f;

    std::vector<float> energies;
    for (size_t off = 0; off + window <= n_samples; off += window) {
        double e = 0.0;
        for (size_t i = off; i < off + window; ++i) {
            int16_t s = static_cast<int16_t>(
                static_cast<uint16_t>(audio[2*i]) |
                (static_cast<uint16_t>(audio[2*i+1]) << 8));
            e += static_cast<double>(s) * static_cast<double>(s);
        }
        energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
    }
    if (energies.empty()) return 0.5f;

    float mean = std::accumulate(energies.begin(), energies.end(), 0.0f)
                 / static_cast<float>(energies.size());
    float var = 0.0f;
    for (float v : energies) var += (v - mean) * (v - mean);
    var /= static_cast<float>(energies.size());

    // Guard against division by zero when mean energy is near zero
    static constexpr float ZERO_GUARD_EPSILON = 1e-6f;
    float norm = std::min(1.0f, var / (mean * mean + ZERO_GUARD_EPSILON));
    return 1.0f - std::abs(norm - 0.3f) / 0.7f;
}

std::string VoiceTTSCustomizer::classifyMOS(float mos) const {
    if (mos >= 4.0f) return "excellent";
    if (mos >= 3.0f) return "good";
    if (mos >= 2.0f) return "fair";
    return "poor";
}

MOSMetrics VoiceTTSCustomizer::estimateMOS(
    const std::vector<uint8_t>& audio_data, int sample_rate) const
{
    MOSMetrics m;
    if (audio_data.size() < 2) {
        m.mos_score = 1.0f;
        m.quality_label = classifyMOS(1.0f);
        return m;
    }

    float energy = computeSignalEnergy(audio_data, sample_rate);
    // Normalise energy: assumes 16-bit signed PCM (max amplitude = 32768)
    static constexpr float PCM_16BIT_MAX = 32768.0f;
    float norm_energy = std::min(1.0f, energy / PCM_16BIT_MAX);
    m.naturalness = std::min(1.0f, norm_energy * 2.0f);
    m.rhythm_score = computeSpeechRhythm(audio_data, sample_rate);
    m.intelligibility = std::min(1.0f, (m.naturalness + m.rhythm_score) * 0.5f);
    m.mos_score = 1.0f + 4.0f * m.naturalness;
    m.mos_score = std::clamp(m.mos_score, 1.0f, 5.0f);
    m.quality_label = classifyMOS(m.mos_score);
    m.details = {{"energy", energy}, {"norm_energy", norm_energy}};
    return m;
}

MOSMetrics VoiceTTSCustomizer::estimateMOSFromText(
    const std::string& input_text, const std::string& output_text) const
{
    MOSMetrics m;
    // Longer sentences slightly lower intelligibility
    size_t out_len = output_text.size();
    float intel = (out_len == 0) ? 1.0f : std::max(0.5f, 1.0f - static_cast<float>(out_len) / 2000.0f);
    m.intelligibility = intel;
    m.naturalness = 0.85f;
    m.rhythm_score = 0.8f;
    // MOS in range 3.5–4.5
    m.mos_score = 3.5f + 1.0f * intel;
    m.mos_score = std::clamp(m.mos_score, 3.5f, 4.5f);
    m.quality_label = classifyMOS(m.mos_score);
    m.details = {{"input_len", input_text.size()}, {"output_len", out_len}};
    return m;
}

void VoiceTTSCustomizer::registerLanguageVoice(const LanguageVoice& lv) {
    language_voices_[lv.language_code] = lv;
}

std::vector<LanguageVoice> VoiceTTSCustomizer::getSupportedLanguages() const {
    std::vector<LanguageVoice> result;
    result.reserve(language_voices_.size());
    for (const auto& [code, lv] : language_voices_) {
        result.push_back(lv);
    }
    return result;
}

std::string VoiceTTSCustomizer::getBestVoiceForLanguage(const std::string& lang_code) const {
    auto it = language_voices_.find(lang_code);
    if (it != language_voices_.end()) {
        return it->second.default_voice_id;
    }
    // Try prefix match (e.g. "en" matches "en-US")
    if (lang_code.empty() || lang_code.size() < 2) return {};
    std::string prefix = lang_code.substr(0, 2);
    for (const auto& [code, lv] : language_voices_) {
        if (code.size() >= 2 && code.substr(0, 2) == prefix && !lv.default_voice_id.empty()) {
            return lv.default_voice_id;
        }
    }
    return {};
}

bool VoiceTTSCustomizer::supportsLanguage(const std::string& lang_code) const {
    if (language_voices_.count(lang_code)) return true;
    // Check profile languages
    for (const auto& [id, p] : profiles_) {
        if (p.language == lang_code ||
            (p.language.size() >= 2 && lang_code.size() >= 2 &&
             p.language.substr(0, 2) == lang_code.substr(0, 2))) {
            return true;
        }
    }
    return false;
}

json VoiceTTSCustomizer::getStatistics() const {
    json stats;
    stats["profile_count"] = profiles_.size();
    stats["language_count"] = language_voices_.size();
    stats["default_engine"] = config_.default_engine;
    return stats;
}

}} // namespace themis::voice
