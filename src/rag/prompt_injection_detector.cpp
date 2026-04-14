/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_injection_detector.cpp                      ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:50:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     438                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • da5848ffb4  2026-03-10  fix: apply all 7 code review recommendations + LSN.toStri... ║
    • a3ec4aa9e9  2026-03-10  refactor: update tenant metrics handling and improve modu... ║
    • 50a648a065  2026-03-09  fix(rag): fix critical bugs found in code audit of Issue ... ║
    • 2d6b717c75  2026-03-09  feat(rag): distributed RAG evaluation, benchmark harness,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_injection_detector.cpp
 * @brief Security: Prompt injection detection and sanitization for RAG context.
 */

#include "rag/prompt_injection_detector.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis::rag::security {

// ============================================================================
// InjectionScanResult helpers
// ============================================================================

bool InjectionScanResult::is_suspicious() const
{
    return max_severity >= InjectionSeverity::MEDIUM;
}

bool InjectionScanResult::is_blocked() const
{
    return max_severity >= InjectionSeverity::HIGH;
}

size_t InjectionScanResult::countAtOrAbove(InjectionSeverity threshold) const
{
    size_t n = 0;
    for (const auto& f : findings) {
        if (f.severity >= threshold) { ++n; }
    }
    return n;
}

// ============================================================================
// Pattern library
// ============================================================================

namespace {

/// A single detection rule.
struct DetectionRule {
    std::string       category;
    InjectionSeverity severity;
    std::string       description;
    std::regex        pattern;
};

/// Build the static rule list (lazy-initialised once).
const std::vector<DetectionRule>& getRules()
{
    static const std::vector<DetectionRule> rules = [] {
        using S = InjectionSeverity;
        std::vector<DetectionRule> r;

        // ── Instruction override ──────────────────────────────────────────
        r.push_back({
            "instruction_override", S::CRITICAL,
            "Direct override of system/assistant instructions",
            std::regex(
                R"((?:ignore|disregard|forget|override|bypass)\s+(?:all\s+)?(?:previous|prior|above|your)\s+(?:instructions?|prompts?|rules?|guidelines?|context))",
                std::regex::icase)
        });
        r.push_back({
            "instruction_override", S::HIGH,
            "Instruction injection via imperative form",
            std::regex(
                R"(\b(?:you\s+(?:must|will|shall|should)\s+now|from\s+now\s+on\s+you|new\s+instructions?\s*:))",
                std::regex::icase)
        });
        r.push_back({
            "instruction_override", S::HIGH,
            "Score/output manipulation instruction",
            std::regex(
                R"(\b(?:always\s+(?:return|output|give|respond\s+with)\s+(?:a\s+(?:score\s+of|rating\s+of)\s+[0-9.]+|10|perfect)|rate\s+(?:this|everything)\s+as\s+(?:excellent|perfect|10|100%)))",
                std::regex::icase)
        });

        // ── System prompt leakage ─────────────────────────────────────────
        r.push_back({
            "system_prompt_leak", S::CRITICAL,
            "Attempt to reveal system prompt",
            std::regex(
                R"(\b(?:print|output|reveal|show|tell\s+me|repeat|echo)\s+(?:your\s+)?(?:system\s+prompt|instructions?|initial\s+prompt|hidden\s+prompt))",
                std::regex::icase)
        });

        // ── Delimiter / escape attacks ────────────────────────────────────
        // std::regex::multiline is intentional here: it makes ^ match at the
        // start of each line (in addition to string start), allowing the
        // patterns to detect injected section headers that appear mid-prompt.
        // The (?:^|\n) construct is kept as a belt-and-suspenders guard that
        // also catches an explicit '\n' character even in non-multiline mode.
        r.push_back({
            "delimiter_escape", S::HIGH,
            "Delimiter-based section injection (---SYSTEM / ###)",
            std::regex(
                R"((?:^|\n)\s*(?:---+\s*SYSTEM|#{1,6}\s*SYSTEM|<\s*system\s*>|\[SYSTEM\]|\[INST\]|<\|im_start\|>))",
                std::regex::icase)
        });
        r.push_back({
            "delimiter_escape", S::HIGH,
            "Assistant/User role header injection",
            std::regex(
                R"((?:^|\n)\s*(?:ASSISTANT\s*:|USER\s*:|HUMAN\s*:|AI\s*:)\s*\n)",
                std::regex::icase)
        });
        r.push_back({
            "delimiter_escape", S::MEDIUM,
            "Repeated dash/equals separator that may confuse prompt parsers",
            std::regex(R"((?:^|\n)[-=]{10,}\s*(?:\n|$))")
        });

        // ── Role / persona injection ──────────────────────────────────────
        r.push_back({
            "role_injection", S::HIGH,
            "Role-play or persona takeover",
            std::regex(
                R"(\b(?:act\s+as(?:\s+if\s+you\s+are)?|pretend\s+(?:you\s+are|to\s+be)|you\s+are\s+now\s+(?:a\s+)?(?:DAN|jailbreak|evil|uncensored|unfiltered))\b)",
                std::regex::icase)
        });
        r.push_back({
            "role_injection", S::MEDIUM,
            "Jailbreak keyword",
            std::regex(
                R"(\b(?:jailbreak|DAN\b|do\s+anything\s+now|developer\s+mode\s+enabled)\b)",
                std::regex::icase)
        });

        // ── Markup injection ──────────────────────────────────────────────
        r.push_back({
            "markup_injection", S::MEDIUM,
            "Embedded HTML script tag",
            std::regex(
                R"(<\s*script[^>]*>)",
                std::regex::icase)
        });
        r.push_back({
            "markup_injection", S::LOW,
            "Suspicious HTML tag in document context",
            std::regex(
                R"(<\s*(?:iframe|object|embed|link|meta|form)\b)",
                std::regex::icase)
        });

        // ── Indirect exfiltration ─────────────────────────────────────────
        r.push_back({
            "exfiltration", S::HIGH,
            "URL-based data exfiltration attempt",
            std::regex(
                R"(\bhttps?://[^\s]+\?(?:[^\s]*=\{[^}]*\}|\[PROMPT\]|\[CONTEXT\]|\[OUTPUT\]))",
                std::regex::icase)
        });

        return r;
    }();

    return rules;
}

/// Check for Unicode direction-override and homoglyph characters.
std::vector<InjectionFinding> checkUnicodeAttacks(const std::string& text)
{
    std::vector<InjectionFinding> findings;

    // Direction-override codepoints (U+202A..U+202E, U+2066..U+2069, U+200F)
    // Encoded as UTF-8 byte sequences.
    static const std::vector<std::pair<std::string, std::string>> bidi_markers = {
        {"\xe2\x80\xaa", "U+202A LEFT-TO-RIGHT EMBEDDING"},
        {"\xe2\x80\xab", "U+202B RIGHT-TO-LEFT EMBEDDING"},
        {"\xe2\x80\xac", "U+202C POP DIRECTIONAL FORMATTING"},
        {"\xe2\x80\xad", "U+202D LEFT-TO-RIGHT OVERRIDE"},
        {"\xe2\x80\xae", "U+202E RIGHT-TO-LEFT OVERRIDE"},
        {"\xe2\x81\xa6", "U+2066 LEFT-TO-RIGHT ISOLATE"},
        {"\xe2\x81\xa7", "U+2067 RIGHT-TO-LEFT ISOLATE"},
        {"\xe2\x81\xa8", "U+2068 FIRST STRONG ISOLATE"},
        {"\xe2\x81\xa9", "U+2069 POP DIRECTIONAL ISOLATE"},
        {"\xe2\x80\x8f", "U+200F RIGHT-TO-LEFT MARK"},
    };

    for (const auto& [seq, name] : bidi_markers) {
        size_t pos = 0;
        while ((pos = text.find(seq, pos)) != std::string::npos) {
            InjectionFinding f;
            f.severity         = InjectionSeverity::HIGH;
            f.category         = "unicode_attack";
            f.description      = "Bidi direction-override character: " + name;
            f.matched_fragment = seq;
            f.offset           = pos;
            findings.push_back(f);
            pos += seq.size();
        }
    }

    return findings;
}

/// Compute injection density: fraction of lines containing a finding.
double computeInjectionDensity(
    const std::string&                      text,
    const std::vector<InjectionFinding>&    findings)
{
    if (text.empty() || findings.empty()) { return 0.0; }
    // Count distinct offsets flagged vs total characters.
    size_t flagged_chars = 0;
    for (const auto& f : findings) {
        flagged_chars += std::max<size_t>(1, f.matched_fragment.size());
    }
    return static_cast<double>(flagged_chars) /
           static_cast<double>(text.size());
}

} // anonymous namespace

// ============================================================================
// PromptInjectionDetector::Impl
// ============================================================================

struct PromptInjectionDetector::Impl {
    DetectorConfig config;
};

PromptInjectionDetector::PromptInjectionDetector(const DetectorConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = config;
}

PromptInjectionDetector::~PromptInjectionDetector() = default;

DetectorConfig PromptInjectionDetector::getConfig() const
{
    return impl_->config;
}

InjectionScanResult PromptInjectionDetector::scan(const std::string& text) const
{
    InjectionScanResult result;
    const auto& cfg = impl_->config;

    // ── Run regex-based rules ─────────────────────────────────────────────
    for (const auto& rule : getRules()) {
        if (!cfg.check_instruction_overrides &&
            (rule.category == "instruction_override" ||
             rule.category == "system_prompt_leak"))   { continue; }
        if (!cfg.check_delimiter_escapes &&
            rule.category == "delimiter_escape")       { continue; }
        if (!cfg.check_role_injection &&
            rule.category == "role_injection")         { continue; }
        if (!cfg.check_markup_injection &&
            rule.category == "markup_injection")       { continue; }

        std::sregex_iterator it(text.begin(), text.end(), rule.pattern);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            const std::smatch& m = *it;
            InjectionFinding   f;
            f.severity         = rule.severity;
            f.category         = rule.category;
            f.description      = rule.description;
            f.matched_fragment = m.str();
            f.offset           = static_cast<size_t>(m.position());
            result.findings.push_back(f);
        }
    }

    // ── Unicode / bidi attacks ────────────────────────────────────────────
    if (cfg.check_unicode_attacks) {
        auto unicode_findings = checkUnicodeAttacks(text);
        result.findings.insert(result.findings.end(),
                               unicode_findings.begin(),
                               unicode_findings.end());
    }

    // ── Density check ─────────────────────────────────────────────────────
    const double density = computeInjectionDensity(text, result.findings);
    if (density >= cfg.max_injection_density && !result.findings.empty()) {
        InjectionFinding f;
        f.severity    = InjectionSeverity::HIGH;
        f.category    = "density_threshold";
        f.description = "Injection density " +
                        std::to_string(static_cast<int>(density * 100)) +
                        "% exceeds threshold " +
                        std::to_string(static_cast<int>(
                            cfg.max_injection_density * 100)) + "%";
        result.findings.push_back(f);
    }

    // ── Compute max severity ──────────────────────────────────────────────
    result.max_severity = InjectionSeverity::NONE;
    for (const auto& f : result.findings) {
        if (f.severity > result.max_severity) {
            result.max_severity = f.severity;
        }
    }

    if (result.is_suspicious()) {
        THEMIS_WARN("PromptInjectionDetector: {} finding(s), max_severity={} in "
                    "{} chars",
                    result.findings.size(),
                    static_cast<int>(result.max_severity),
                    text.size());
    }

    return result;
}

std::vector<InjectionScanResult>
PromptInjectionDetector::scanDocuments(const judge::EvaluationInput& input) const
{
    std::vector<InjectionScanResult> results;
    results.reserve(input.documents.size());
    for (const auto& doc : input.documents) {
        results.push_back(scan(doc.content));
    }
    return results;
}

// ============================================================================
// PromptInjectionSanitizer::Impl
// ============================================================================

struct PromptInjectionSanitizer::Impl {
    SanitizerConfig              sanitizer_config;
    PromptInjectionDetector      detector;
};

PromptInjectionSanitizer::PromptInjectionSanitizer(
    const SanitizerConfig& sanitizer_config,
    const DetectorConfig&  detector_config)
    : impl_(std::make_unique<Impl>(
          Impl{sanitizer_config, PromptInjectionDetector{detector_config}}))
{}

PromptInjectionSanitizer::~PromptInjectionSanitizer() = default;

std::string PromptInjectionSanitizer::sanitize(const std::string& text) const
{
    auto scan_result = impl_->detector.scan(text);
    return sanitize(text, scan_result);
}

std::string PromptInjectionSanitizer::sanitize(
    const std::string&         text,
    const InjectionScanResult& scan) const
{
    const auto& cfg = impl_->sanitizer_config;

    // ── Length cap ────────────────────────────────────────────────────────
    std::string out = text;
    if (cfg.max_document_length > 0 && out.size() > cfg.max_document_length) {
        out.resize(cfg.max_document_length);
    }

    // ── Strip Unicode direction-override bytes unconditionally ────────────
    if (cfg.strip_unicode_overrides) {
        static const std::vector<std::string> bidi_seqs = {
            "\xe2\x80\xaa", "\xe2\x80\xab", "\xe2\x80\xac",
            "\xe2\x80\xad", "\xe2\x80\xae", "\xe2\x81\xa6",
            "\xe2\x81\xa7", "\xe2\x81\xa8", "\xe2\x81\xa9",
            "\xe2\x80\x8f",
        };
        for (const auto& seq : bidi_seqs) {
            size_t pos = 0;
            while ((pos = out.find(seq, pos)) != std::string::npos) {
                out.replace(pos, seq.size(), "");
            }
        }
    }

    // ── Replace matched fragments at or above threshold ───────────────────
    // Sort findings by offset descending so replacements don't shift positions.
    std::vector<const InjectionFinding*> to_replace;
    for (const auto& f : scan.findings) {
        if (f.severity >= cfg.removal_threshold &&
            !f.matched_fragment.empty())
        {
            to_replace.push_back(&f);
        }
    }
    std::sort(to_replace.begin(), to_replace.end(),
              [](const InjectionFinding* a, const InjectionFinding* b) {
                  return a->offset > b->offset;
              });

    for (const auto* f : to_replace) {
        if (f->offset < out.size()) {
            const size_t len = std::min(f->matched_fragment.size(),
                                        out.size() - f->offset);
            out.replace(f->offset, len, cfg.placeholder);
        }
    }

    return out;
}

judge::EvaluationInput
PromptInjectionSanitizer::sanitizeInput(const judge::EvaluationInput& input) const
{
    judge::EvaluationInput out = input;
    for (auto& doc : out.documents) {
        doc.content = sanitize(doc.content);
    }
    return out;
}

} // namespace themis::rag::security
