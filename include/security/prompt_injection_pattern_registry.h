/**
 * @file prompt_injection_pattern_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
