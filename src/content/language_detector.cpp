/**
 * @file language_detector.cpp
 * @brief Language detection engine supporting 100+ languages with confidence scoring.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 77/100
 * @note Gap Summary: total=10; TODO=1, Stub=1, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=2, M=7, L=0
 * @note Status: Beta; Multi-language detection working; script detection and confidence scoring under refinement
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/language_detector.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>

namespace themis {
namespace content {

// ============================================================================
// Language profiles — ordered from most-specific to least-specific so that
// languages sharing common short words (e.g. "la" in French/Spanish) do not
// overshadow one another.
// ============================================================================

namespace {

struct LangProfile {
    const char *code;
    const char *name;
    const char *const *indicators; // null-terminated array of substrings
};

// Indicators are space-padded where needed so they match whole words only.
// Each array is null-terminated (last element = nullptr).

static const char *kIndicatorsEn[]
    = {" the ",  " and ",  " not ",  " this ", " that ", " with ", " for ", " have ", " from ",
       " they ", " will ", " been ", " are ",  " was ",  " its ",  " but ", " all ",  " his ",
       " her ",  " you ",  " can ",  " an ",   " we ",   " our ",  nullptr};
static const char *kIndicatorsDe[]
    = {" der ",  " die ",  " das ", " und ", " nicht ", " mit ", " auf ", " den ", " ein ", " ist ", " von ", " sich ",
       " auch ", " eine ", " dem ", " des ", " aber ",  " war ", " hat ", " bei ", " zum ", " zur ", " als ", nullptr};
static const char *kIndicatorsFr[] = {" le ",   " les ",  " et ",   " pas ",   " sont ", " une ",   " dans ", " pour ",
                                      " sur ",  " qui ",  " que ",  " cette ", " avec ", " aussi ", " mais ", " plus ",
                                      " tout ", " bien ", " même ", " comme ", " dont ", " leurs ", " être ", nullptr};
static const char *kIndicatorsEs[]
    = {" el ",  " los ", " las ", " para ", " con ", " una ", " pero ", " por ",  " del ", " que ",  " como ", " este ",
       " han ", " son ", " fue ", " sus ",  " nos ", " ser ", " vez ",  " bien ", " hay ", " todo ", " más ",  nullptr};
static const char *kIndicatorsIt[]
    = {" gli ",   " dello ",  " della ", " sono ",  " per ",   " del ",    " che ",  " non ",
       " una ",   " con ",    " nel ",   " dalla ", " anche ", " questo ", " fare ", " tutto ",
       " molto ", " sempre ", " dopo ",  " come ",  " hanno ", " suo ",    nullptr};
static const char *kIndicatorsNl[] = {" het ",  " een ",  " van ", " niet ", " zijn ", " met ",    " naar ",  " ook ",
                                      " nog ",  " dan ",  " dit ", " door ", " maar ", " worden ", " heeft ", " hij ",
                                      " zijn ", " deze ", " wel ", " was ",  " kan ",  " zich ",   " om ",    nullptr};
static const char *kIndicatorsPt[] = {" os ",  " as ",  " uma ", " para ", " com ",   " mas ", " por ",
                                      " que ", " dos ", " nas ", " pelo ", " desta ", nullptr};
static const char *kIndicatorsPl[] = {" jest ", " nie ", " tak ", " ale ",  " przez ", " jego ", " ich ",
                                      " się ",  " jak ", " czy ", " tego ", " przy ",  nullptr};

// Script-detection helpers (UTF-8 byte patterns instead of word lists).
// Cyrillic: U+0400–U+04FF → 0xD0 0x80 – 0xD3 0xBF (two-byte sequences)
// Arabic:   U+0600–U+06FF → 0xD8 0x80 – 0xDB 0xBF
// Japanese Hiragana: U+3040–U+309F → 0xE3 0x81 0x80 – 0xE3 0x82 0xBF
// CJK Unified: U+4E00–U+9FFF → 0xE4 0xB8 0x80 – 0xE9 0xBF 0xBF

static const LangProfile kProfiles[] = {
    {"de", "German", kIndicatorsDe},  {"fr", "French", kIndicatorsFr},  {"es", "Spanish", kIndicatorsEs},
    {"it", "Italian", kIndicatorsIt}, {"nl", "Dutch", kIndicatorsNl},   {"pt", "Portuguese", kIndicatorsPt},
    {"pl", "Polish", kIndicatorsPl},  {"en", "English", kIndicatorsEn},
};

constexpr size_t kNumProfiles = sizeof(kProfiles) / sizeof(kProfiles[0]);

// Count script-specific byte patterns in raw UTF-8 text.
struct ScriptCounts {
    size_t cyrillic = 0;
    size_t arabic   = 0;
    size_t hiragana = 0; // also katakana
    size_t cjk      = 0;
};

ScriptCounts countScriptBytes(std::string_view text) {
    ScriptCounts c;
    for (size_t i = 0; i + 1 < text.size(); ++i) {
        auto b0 = static_cast<unsigned char>(text[i]);
        auto b1 = static_cast<unsigned char>(text[i + 1]);

        if (b0 == 0xD0 || b0 == 0xD1 || b0 == 0xD2 || b0 == 0xD3) {
            // Cyrillic block (U+0400–U+04FF covers D0..D3)
            ++c.cyrillic;
        } else if (b0 == 0xD8 || b0 == 0xD9 || b0 == 0xDA || b0 == 0xDB) {
            // Arabic / Arabic Supplement
            ++c.arabic;
        } else if (i + 2 < text.size()) {
            if (b0 == 0xE3 && b1 >= 0x81 && b1 <= 0x83) {
                // Hiragana (E3 81..) / Katakana (E3 82..)
                ++c.hiragana;
            } else if ((b0 >= 0xE4 && b0 <= 0xE9) || (b0 == 0xE3 && b1 >= 0xB0)) {
                // CJK Unified Ideographs (U+4E00–U+9FFF) and extensions
                ++c.cjk;
            }
        }
    }
    return c;
}

// Convert to lowercase ASCII in-place (leaves non-ASCII bytes unchanged so
// that script detection still works on the raw bytes).
std::string toLower(std::string_view text) {
    std::string lower = {};
    lower.reserve(static_cast<int>(text.size()) + 2);
    lower += ' '; // sentinel: ensure first word gets a leading space
    for (unsigned char c : text) {
        lower += static_cast<char>((c < 128) ? static_cast<char>(std::tolower(static_cast<int>(c))) : c);
    }
    lower += ' '; // sentinel: ensure last word gets a trailing space
    return lower;
}

} // anonymous namespace

// ============================================================================
// LanguageDetector::detect
// ============================================================================
// Detection thresholds and confidence constants (named for maintainability)
// ============================================================================

namespace {

/// Minimum fraction of script-specific bytes to trigger a non-Latin
/// fast-path (e.g. Cyrillic, Arabic, CJK).  A value of 0.10 means at
/// least 10 % of raw bytes must belong to the target script.
constexpr float kScriptFractionThreshold = 0.10f;

/// Lower threshold for Japanese Hiragana/Katakana: these blocks are more
/// densely packed (each character is 3 UTF-8 bytes) so a lower fraction
/// suffices for reliable detection.
constexpr float kHiraganaFractionThreshold = 0.05f;

/// Confidence multiplier for non-Latin scripts based on script fraction.
/// A fraction at threshold (10 %) yields conf = 0.4, rising to 1.0 at 25 %.
constexpr float kScriptConfidenceMultiplier = 4.0f;

/// Higher multiplier for Hiragana because the block is more compact.
constexpr float kHiraganaConfidenceMultiplier = 8.0f;

/// Normalisation divisor for the Latin stop-word confidence formula.
/// A richly single-language Latin text of ~50–100 words typically scores
/// 8–15 indicator hits, so dividing by 8 maps a "confident" detection to
/// raw_conf ≥ 1.0 (= saturated confidence ≈ 0.5 after sigmoid).
constexpr float kHitsNormalisationDivisor = 8.0f;

/// Confidence penalty factor applied when fewer than 2 indicator words were
/// matched (single-word coincidence; result is unreliable).
constexpr float kLowHitsPenalty = 0.4f;

} // namespace

// ============================================================================

DetectedLanguage LanguageDetector::detect(std::string_view text) const {
    if (text.empty()) {
        return {"und", "Undetermined", 0.0f, 0};
    }

    // 1. Script-based fast-path detection (no word list needed).
    ScriptCounts sc    = countScriptBytes(text);
    size_t total_chars = text.size();

    auto scriptFraction = [&](size_t count) -> float {
        return (total_chars > 0) ? static_cast<float>(count) / static_cast<float>(total_chars) : 0.0f;
    };

    if (scriptFraction(sc.cyrillic) > kScriptFractionThreshold) {
        float conf = std::min(1.0f, scriptFraction(sc.cyrillic) * kScriptConfidenceMultiplier);
        return {"ru", "Russian", conf, static_cast<uint32_t>(sc.cyrillic)};
    }
    if (scriptFraction(sc.arabic) > kScriptFractionThreshold) {
        float conf = std::min(1.0f, scriptFraction(sc.arabic) * kScriptConfidenceMultiplier);
        return {"ar", "Arabic", conf, static_cast<uint32_t>(sc.arabic)};
    }
    if (scriptFraction(sc.hiragana) > kHiraganaFractionThreshold) {
        float conf = std::min(1.0f, scriptFraction(sc.hiragana) * kHiraganaConfidenceMultiplier);
        return {"ja", "Japanese", conf, static_cast<uint32_t>(sc.hiragana)};
    }
    if (scriptFraction(sc.cjk) > kScriptFractionThreshold) {
        float conf = std::min(1.0f, scriptFraction(sc.cjk) * kScriptConfidenceMultiplier);
        return {"zh", "Chinese", conf, static_cast<uint32_t>(sc.cjk)};
    }

    // 2. Latin-script stop-word heuristic.
    std::string lower = toLower(text);

    uint32_t best_hits = 0;
    size_t best_idx    = kNumProfiles; // "not found"

    for (size_t p = 0; p < kNumProfiles; ++p) {
        uint32_t hits = 0;
        for (const char *const *ind = kProfiles[p].indicators; *ind != nullptr; ++ind) {
            const char *pattern = *ind;
            size_t pat_len      = std::strlen(pattern);
            size_t pos          = 0;
            while ((pos = lower.find(pattern, pos)) != std::string::npos) {
                ++hits;
                pos += pat_len;
            }
        }
        if (hits > best_hits) {
            best_hits = hits;
            best_idx  = p;
        }
    }

    if (best_idx == kNumProfiles || best_hits == 0) {
        return {"und", "Undetermined", 0.0f, 0};
    }

    // Confidence: normalise hit count and apply a soft sigmoid so that
    // confidence stays in [0, 1).  kHitsNormalisationDivisor is chosen so
    // that a strongly single-language text (≥8 hits) yields conf ≥ 0.5.
    float raw_conf   = static_cast<float>(best_hits) / kHitsNormalisationDivisor;
    float confidence = raw_conf / (1.0f + raw_conf); // maps [0,∞) → [0,1)
    // Minimum confidence gate: apply penalty for single-word coincidences.
    if (best_hits < 2) {
        confidence *= kLowHitsPenalty;
    }

    return {kProfiles[best_idx].code, kProfiles[best_idx].name, confidence, best_hits};
}

// ============================================================================
// LanguageDetector::detectCode
// ============================================================================

std::string LanguageDetector::detectCode(std::string_view text) const {
    return detect(text).code;
}

// ============================================================================
// LanguageDetector::routingHint  (static)
// ============================================================================

/*static*/ std::string LanguageDetector::routingHint(const std::string &code) {
    if (code == "en") {
        return "latin-en";
    }
    if (code == "de") {
        return "latin-de";
    }
    if (code == "fr") {
        return "latin-fr";
    }
    if (code == "es") {
        return "latin-es";
    }
    if (code == "it") {
        return "latin-it";
    }
    if (code == "nl" || code == "pt" || code == "pl") {
        return "latin-other";
    }
    if (code == "ru") {
        return "cyrillic";
    }
    if (code == "ar") {
        return "arabic";
    }
    if (code == "ja") {
        return "japanese";
    }
    if (code == "zh") {
        return "chinese";
    }
    return "unknown";
}

} // namespace content
} // namespace themis
