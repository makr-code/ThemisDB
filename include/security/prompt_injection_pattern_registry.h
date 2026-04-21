/**
 * @file prompt_injection_pattern_registry.h
 * @brief Shared canonical pattern registry for PromptInjectionDetector (Gap 5).
 *
 * **Purpose (Gap 5 — AI_ML_IMPACT_ASSESSMENT.md §7, Severity: Medium/S2)**
 *
 * Two independent `PromptInjectionDetector` implementations existed:
 *   - `src/rag/prompt_injection_detector.cpp` (RAG context, InjectionSeverity)
 *   - `src/prompt_engineering/prompt_injection_detector.cpp` (PE context, risk_score)
 *
 * A new attack pattern added to one would silently be absent from the other,
 * creating divergent security postures across the RAG and prompt-engineering
 * pipelines.
 *
 * This header provides a single shared registry that is loaded by both detectors
 * at construction.  Each detector may still add domain-specific patterns on top.
 *
 * **Usage:**
 * @code
 * const auto& reg = PromptInjectionPatternRegistry::defaultRegistry();
 * for (const auto& e : reg.patterns()) {
 *     // compile and use e.pattern_str with e.severity_hint
 * }
 * assert(reg.patternCount() == SHARED_INJECTION_PATTERN_COUNT);
 * @endcode
 *
 * **Version semantics:** `version()` returns a monotonic integer starting at 1.
 * It increments on each `addPattern()` or `addKeyword()` call, allowing callers
 * to detect registry updates without re-scanning the full pattern list.
 *
 * **Thread safety:** The default registry is initialised once (Meyers singleton)
 * and is read-only after construction — safe for concurrent reads.
 * Custom registries built by tests are single-threaded.
 *
 * @see src/rag/prompt_injection_detector.cpp — consumes this registry in getRules()
 * @see src/prompt_engineering/prompt_injection_detector.cpp — consumes in initializePatterns()
 * @see src/rag/FUTURE_ENHANCEMENTS.md §Gap 5
 * @see src/prompt_engineering/FUTURE_ENHANCEMENTS.md §Gap 5
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace themis::security {

// ---------------------------------------------------------------------------
// Severity hint for a shared pattern entry.
// Numeric values intentionally match themis::rag::security::InjectionSeverity.
// ---------------------------------------------------------------------------
enum class SharedPatternSeverity : int {
    LOW      = 0,
    MEDIUM   = 1,
    HIGH     = 2,
    CRITICAL = 3,
};

// ---------------------------------------------------------------------------
// A single shared pattern entry.
// ---------------------------------------------------------------------------
struct SharedPatternEntry {
    /// Short tag identifying the attack category (e.g. "instruction_override").
    std::string label;
    /// Raw ECMAScript regex string (case-insensitive flag applied by each detector).
    std::string pattern_str;
    /// Human-readable description used in log messages / findings.
    std::string description;
    /// Severity hint; detectors may override for domain-specific reasons.
    SharedPatternSeverity severity = SharedPatternSeverity::HIGH;
};

// ---------------------------------------------------------------------------
// PromptInjectionPatternRegistry
// ---------------------------------------------------------------------------

/**
 * @brief Canonical shared registry of prompt injection detection patterns.
 *
 * Both `themis::rag::security::PromptInjectionDetector` and
 * `themis::prompt_engineering::PromptInjectionDetector` load the default
 * registry at construction time to ensure pattern parity.
 *
 * Compile-time enforcement: each detector's constructor asserts
 * `defaultRegistry().patternCount() == SHARED_INJECTION_PATTERN_COUNT`
 * (defined below) so that a mismatch triggers a startup failure rather than
 * a silent security regression.
 */
class PromptInjectionPatternRegistry {
public:
    PromptInjectionPatternRegistry() = default;

    // ── Accessors ─────────────────────────────────────────────────────────
    const std::vector<SharedPatternEntry>& patterns() const { return patterns_; }
    const std::vector<std::string>&        keywords() const { return keywords_; }

    /// Returns number of regex patterns in the registry.
    size_t patternCount() const { return patterns_.size(); }
    /// Returns number of keywords in the registry.
    size_t keywordCount() const { return keywords_.size(); }
    /// Monotonic version counter — increments on each addPattern()/addKeyword().
    uint32_t version() const { return version_; }

    // ── Mutators (for custom / test-only registries) ──────────────────────
    void addPattern(SharedPatternEntry entry) {
        patterns_.push_back(std::move(entry));
        ++version_;
    }
    void addKeyword(std::string keyword) {
        keywords_.push_back(std::move(keyword));
        ++version_;
    }

    // ── Default (canonical) registry ──────────────────────────────────────
    /**
     * @brief Returns the canonical, lazily-initialised shared registry.
     *
     * Contains SHARED_INJECTION_PATTERN_COUNT patterns (see below) and
     * SHARED_INJECTION_KEYWORD_COUNT keywords.  Thread-safe after first call.
     */
    static const PromptInjectionPatternRegistry& defaultRegistry();

private:
    std::vector<SharedPatternEntry> patterns_;
    std::vector<std::string>        keywords_;
    uint32_t                        version_ = 1u;
};

// ---------------------------------------------------------------------------
// Compile-time constants used by each detector's constructor assertion.
// Update these when patterns are added to defaultRegistry().
// ---------------------------------------------------------------------------

/// Number of base patterns in PromptInjectionPatternRegistry::defaultRegistry().
inline constexpr size_t SHARED_INJECTION_PATTERN_COUNT = 11u;

/// Number of base keywords in PromptInjectionPatternRegistry::defaultRegistry().
inline constexpr size_t SHARED_INJECTION_KEYWORD_COUNT = 11u;

} // namespace themis::security
