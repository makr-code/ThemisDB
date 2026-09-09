/**
 * @file voice_tts_customizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "voice/voice_tts_customizer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <numeric>
#include <regex>
#include <map>
#include <set>

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
    addProfile("de-male",    "German Male",       "de-DE", "male",    "piper");
    addProfile("de-female",  "German Female",     "de-DE", "female",  "piper");
    addProfile("fr-default", "French Default",    "fr-FR", "neutral", "piper");
    addProfile("fr-male",    "French Male",       "fr-FR", "male",    "piper");
    addProfile("fr-female",  "French Female",     "fr-FR", "female",  "piper");
    addProfile("es-default", "Spanish Default",   "es-ES", "neutral", "piper");
    addProfile("es-male",    "Spanish Male",      "es-ES", "male",    "piper");
    addProfile("es-female",  "Spanish Female",    "es-ES", "female",  "piper");
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
    addLang("de", "German",   "de-default", {"de-default", "de-male", "de-female"});
    addLang("de-DE", "German (Germany)", "de-default", {"de-default", "de-male", "de-female"});
    addLang("fr", "French",   "fr-default", {"fr-default", "fr-male", "fr-female"});
    addLang("fr-FR", "French (France)", "fr-default", {"fr-default", "fr-male", "fr-female"});
    addLang("es", "Spanish",  "es-default", {"es-default", "es-male", "es-female"});
    addLang("es-ES", "Spanish (Spain)", "es-default", {"es-default", "es-male", "es-female"});
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
    if (it == profiles_.end()) {
      return std::nullopt;
    }
    return it->second;
}

std::vector<VoiceProfile> VoiceTTSCustomizer::listProfiles() const {
    std::vector<VoiceProfile> result = {};

    result.reserve(profiles_.size());
    for (const auto& [id, p] : profiles_) {
        result.push_back(p);
    }
    return result;
}

std::vector<VoiceProfile> VoiceTTSCustomizer::getProfilesForLanguage(const std::string& lang) const {
    std::vector<VoiceProfile> result = {};

    for (const auto& [id, p] : profiles_) {
        if (p.language == lang ||
            (static_cast<int>(p.language.size()) >= 2 && static_cast<int>(lang.size()) >= 2 &&
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
    if (overrides.pitch != def.pitch) {
      base.pitch = overrides.pitch;
    }
    if (overrides.speed != def.speed) {
      base.speed = overrides.speed;
    }
    if (overrides.volume != def.volume) {
      base.volume = overrides.volume;
    }
    if (overrides.emphasis != def.emphasis) {
      base.emphasis = overrides.emphasis;
    }
    if (!overrides.style.empty()) {
      base.style = overrides.style;
    }
    if (overrides.enable_ssml) {
      base.enable_ssml = true;
    }

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
        if (end == std::string::npos) {
          break;
        }
        std::string tag = work.substr(pos, end - pos + 1);

        ProsodyConfig seg;
        // Extract rate attribute
        auto extractAttr = [&](const std::string& attr) -> std::string {
            size_t a = tag.find(attr + "=\"");
            if (a == std::string::npos) return {};
            size_t vs = a + static_cast<int>(attr.size()) + 2;
            size_t ve = tag.find('"', vs);
            if (ve == std::string::npos) return {};
            return tag.substr(vs, ve - vs);
        };

        std::string rate_str = extractAttr("rate");
        std::string pitch_str = extractAttr("pitch");
        if (!rate_str.empty()) {
            try {
                seg.speed = std::stof(rate_str);
            } catch (const std::invalid_argument&) {
            } catch (const std::out_of_range&) {
            } catch (const std::string&) {
            } catch (const char*) {
            } catch (...) {
            }
        }
        if (!pitch_str.empty()) {
            try {
                seg.pitch = std::stof(pitch_str);
            } catch (const std::invalid_argument&) {
            } catch (const std::out_of_range&) {
            } catch (const std::string&) {
            } catch (const char*) {
            } catch (...) {
            }
        }
        seg = validateProsody(seg);
        result.segments.push_back(seg);
        pos = end + 1;
    }

    // Strip all XML tags
    std::string plain = {};
    plain.reserve(ssml_text.size());
    bool in_tag = false;
    for (char c : ssml_text) {
        if (c == '<') { in_tag = true; continue; }
        if (c == '>') { in_tag = false; continue; }
        if (!in_tag) {
          plain += c;
        }
    }

    // Collapse whitespace
    std::string collapsed = {};
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
    if (!collapsed.empty() && collapsed.back() == ' ') {
      collapsed.pop_back();
    }

    result.plain_text = std::move(collapsed);
    return result;
}

// ---------------------------------------------------------------------------
// SSML injection sanitization
// ---------------------------------------------------------------------------

namespace {

// Allowlist of permitted SSML 1.1 tags (lowercase)
const std::set<std::string>& ssmlAllowedTags() {
    static const std::set<std::string> kTags = {
        "speak", "prosody", "break", "emphasis", "say-as",
        "p", "s", "phoneme", "sub", "lang", "voice"
    };
    return kTags;
}

// Allowlist of permitted attributes per tag (tag name → set of attr names)
const std::map<std::string, std::set<std::string>>& ssmlAllowedAttrs() {
    static const std::map<std::string, std::set<std::string>> kAttrs = {
        {"speak",    {"version", "xml:lang", "xmlns"}},
        {"prosody",  {"rate", "pitch", "volume", "duration", "range", "contour"}},
        {"break",    {"time", "strength"}},
        {"emphasis", {"level"}},
        {"say-as",   {"interpret-as", "format", "detail"}},
        {"phoneme",  {"alphabet", "ph"}},
        {"sub",      {"alias"}},
        {"lang",     {"xml:lang"}},
        {"voice",    {"name", "gender", "age", "variant", "languages"}},
        {"p",        {}},
        {"s",        {}},
    };
    return kAttrs;
}

// Return true if the attribute value looks safe (no nested tags, no script keywords)
bool isAttrValueSafe(const std::string& value) {
    if (value.find('<') != std::string::npos) {
      return false;
    }
    if (value.find('>') != std::string::npos) {
      return false;
    }
    // Reject common script/entity injection patterns
    if (value.find("script") != std::string::npos) {
      return false;
    }
    if (value.find("javascript") != std::string::npos) {
      return false;
    }
    if (value.find("vbscript") != std::string::npos) {
      return false;
    }
    if (value.find('\0') != std::string::npos) {
      return false;
    }
    return true;
}

// Parse attribute key="value" pairs from a tag's attribute string.
// Strips disallowed attributes; returns sanitized attribute string.
// Sets injection_found if a disallowed attribute or unsafe value is detected.
std::string sanitizeTagAttributes(
    const std::string& tag_name,
    const std::string& attrs_str,
    bool& injection_found)
{
    const auto& allowed_map = ssmlAllowedAttrs();
    auto it = allowed_map.find(tag_name);
    const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;

    std::string output = {};
    size_t ap = 0;

    while (static_cast<size_t>(ap) <static_cast<int>(attrs_str.size())) {
        // Skip leading whitespace
        while (ap < attrs_str.size() && std::isspace(static_cast<unsigned char>(attrs_str[ap]))) {
          ++ap;
        }
        if (ap >= static_cast<int>(attrs_str.size())) {
          break;
        }

        // Read attribute name
        size_t ns = ap;
        while (ap < attrs_str.size() && attrs_str[ap] != '=' &&
               !std::isspace(static_cast<unsigned char>(attrs_str[ap])) &&
               attrs_str[ap] != '/' && attrs_str[ap] != '>')
        {
            ++ap;
        }
        if (ap == ns) { ++ap; continue; } // guard against infinite loop
        std::string attr_name = attrs_str.substr(ns, ap - ns);
        std::string attr_name_lower = attr_name;
        std::transform(attr_name_lower.begin(), attr_name_lower.end(),
                       attr_name_lower.begin(), ::tolower);

        // Expect '='
        while (ap < attrs_str.size() && std::isspace(static_cast<unsigned char>(attrs_str[ap]))) {
          ++ap;
        }
        if (ap >= attrs_str.size() || attrs_str[ap] != '=') {
            // Boolean attribute (no value) – strip it
            injection_found = true;
            continue;
        }
        ++ap; // skip '='

        // Skip whitespace before quote
        while (ap < attrs_str.size() && std::isspace(static_cast<unsigned char>(attrs_str[ap]))) {
          ++ap;
        }

        // Require quoted value
        if (ap >= attrs_str.size() || (attrs_str[ap] != '"' && attrs_str[ap] != '\'')) {
            injection_found = true;
            // Skip unquoted value token
            while (ap < attrs_str.size() && !std::isspace(static_cast<unsigned char>(attrs_str[ap]))) {
              ++ap;
            }
            continue;
        }
        char quote = attrs_str[ap++];
        size_t vs = ap;
        while (ap < attrs_str.size() && attrs_str[ap] != quote) {
          ++ap;
        }
        std::string attr_value = attrs_str.substr(vs, ap - vs);
        if (static_cast<int>(attrs_str.size()) > ap) ++ap; // skip closing quote

        // Validate value safety
        if (!isAttrValueSafe(attr_value)) {
            injection_found = true;
            continue;
        }

        // Check if attribute is allowed for this tag
        bool attr_allowed = (allowed != nullptr) && (allowed->count(attr_name_lower) > 0);
        if (!attr_allowed) {
            injection_found = true;
            continue;
        }

        output += ' ';
        output += attr_name_lower;
        output += '=';
        output += '"';
        output += attr_value;
        output += '"';
    }
    return output;
}

} // anonymous namespace

SSMLSanitizeResult VoiceTTSCustomizer::sanitizeSSML(const std::string& ssml_input) const {
    SSMLSanitizeResult result;
    const auto& allowed_tags = ssmlAllowedTags();

    std::string output = {};
    output.reserve(ssml_input.size());
    size_t pos = 0;

    while (static_cast<size_t>(pos) <static_cast<int>(ssml_input.size())) {
        if (ssml_input[pos] != '<') {
            output += ssml_input[pos++];
            continue;
        }

        // Find closing '>'
        size_t end = ssml_input.find('>', pos);
        if (end == std::string::npos) {
            // Malformed/unclosed tag – treat remainder as injection
            result.had_injection_attempt = true;
            break;
        }

        std::string tag_body = ssml_input.substr(pos + 1, end - pos - 1);
        bool is_closing      = (!tag_body.empty() && tag_body[0] == '/');
        if (is_closing) {
          tag_body = tag_body.substr(1);
        }
        bool is_self_closing = (!tag_body.empty() && tag_body.back() == '/');
        if (is_self_closing) {
          tag_body.pop_back();
        }

        // Extract tag name
        size_t sp = tag_body.find_first_of(" \t\r\n");
        std::string tag_name = (sp != std::string::npos) ? tag_body.substr(0, sp) : tag_body;
        std::string attrs_str = (sp != std::string::npos) ? tag_body.substr(sp + 1) : std::string{};

        std::string tag_name_lower = tag_name;
        std::transform(tag_name_lower.begin(), tag_name_lower.end(),
                       tag_name_lower.begin(), ::tolower);

        if (allowed_tags.count(tag_name_lower)) {
            // Allowed tag: emit with sanitized attributes
            std::string safe_attrs = {};
            if (!is_closing && !attrs_str.empty()) {
                safe_attrs = sanitizeTagAttributes(tag_name_lower, attrs_str,
                                                   result.had_injection_attempt);
            }
            output += '<';
            if (is_closing) {
              output += '/';
            }
            output += tag_name_lower;
            output += safe_attrs;
            if (is_self_closing) {
              output += '/';
            }
            output += '>';
        } else {
            // Disallowed tag: strip and flag
            result.had_injection_attempt = true;
            auto found = std::find(result.rejected_tags.begin(),
                                   result.rejected_tags.end(), tag_name_lower);
            if (found == result.rejected_tags.end()) {
                result.rejected_tags.push_back(tag_name_lower);
            }
        }
        pos = end + 1;
    }

    result.sanitized_text = std::move(output);
    return result;
}

bool VoiceTTSCustomizer::isSSMLSafe(const std::string& ssml_input) const {
    auto res = sanitizeSSML(ssml_input);
    return !res.had_injection_attempt;
}

float VoiceTTSCustomizer::computeSignalEnergy(
    const std::vector<uint8_t>& audio, int /*sample_rate*/) const
{
    if (static_cast<int>(audio.size()) < 2) {
      return 0.0f;
    }
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
    if (static_cast<int>(audio.size()) < 2 || sample_rate <= 0) {
      return 0.5f;
    }
    // Simple rhythm: variance in energy across 100ms windows
    size_t window = static_cast<size_t>(sample_rate * 0.1);
    size_t n_samples = audio.size() / 2;
    if (n_samples < window) {
      return 0.5f;
    }

    std::vector<float> energies = {};

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
    if (energies.empty()) {
      return 0.5f;
    }

    float mean = std::accumulate(energies.begin(), energies.end(), 0.0f)
                 / static_cast<float>(energies.size());
    float var = 0.0f;
    for (float v : energies) {
      var += (v - mean) * (v - mean);
    }
    var /= static_cast<float>(energies.size());

    // Guard against division by zero when mean energy is near zero
    static constexpr float ZERO_GUARD_EPSILON = 1e-6f;
    float norm = std::min(1.0f, var / (mean * mean + ZERO_GUARD_EPSILON));
    return 1.0f - std::abs(norm - 0.3f) / 0.7f;
}

std::string VoiceTTSCustomizer::classifyMOS(float mos) const {
    if (mos >= 4.0f) {
      return "excellent";
    }
    if (mos >= 3.0f) {
      return "good";
    }
    if (mos >= 2.0f) {
      return "fair";
    }
    return "poor";
}

MOSMetrics VoiceTTSCustomizer::estimateMOS(
    const std::vector<uint8_t>& audio_data, int sample_rate) const
{
    MOSMetrics m = {};
    if (static_cast<int>(audio_data.size()) < 2) {
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
    m.details = {{"input_len",static_cast<int>(input_text.size())}, {"output_len", out_len}};
    return m;
}

void VoiceTTSCustomizer::registerLanguageVoice(const LanguageVoice& lv) {
    language_voices_[lv.language_code] = lv;
}

std::vector<LanguageVoice> VoiceTTSCustomizer::getSupportedLanguages() const {
    std::vector<LanguageVoice> result = {};

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
    if (lang_code.empty() || static_cast<int>(lang_code.size()) < 2) return {};
    std::string prefix = lang_code.substr(0, 2);
    for (const auto& [code, lv] : language_voices_) {
        if (static_cast<int>(code.size()) >= 2 && code.substr(0, 2) == prefix && !lv.default_voice_id.empty()) {
            return lv.default_voice_id;
        }
    }
    return {};
}

bool VoiceTTSCustomizer::supportsLanguage(const std::string& lang_code) const {
    if (language_voices_.count(lang_code)) {
      return true;
    }
    // Check profile languages
    for (const auto& [id, p] : profiles_) {
        if (p.language == lang_code ||
            (static_cast<int>(p.language.size()) >= 2 && static_cast<int>(lang_code.size()) >= 2 &&
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

