/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            text_normalizer.h                                  ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-20                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file text_normalizer.h
 * @brief Toolbox façade for Unicode / umlaut text normalisation.
 *
 * `TextNormalizer` wraps `utils::Normalizer` so that modules outside `utils/`
 * can call `themis::toolbox::normalizeText()` without depending on the
 * internal `utils/normalizer.h` header directly.
 *
 * ## Free function (simplest usage)
 * @code
 * std::string clean = themis::toolbox::normalizeText("Ärger über Öl");
 * // -> "Arger uber Ol"
 * @endcode
 */

#include <string>
#include <string_view>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// TextNormalizer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thin wrapper over `utils::Normalizer` for Unicode / umlaut
 *        normalisation.
 *
 * Currently supports:
 *  - German umlaut folding: ä→a, ö→o, ü→u, Ä→A, Ö→O, Ü→U, ß→ss
 *
 * Input must be valid UTF-8.  Output is UTF-8 / ASCII-clean.
 *
 * Thread-safety: all methods are stateless; an instance may be shared
 * across threads without synchronisation.
 */
class TextNormalizer {
public:
    TextNormalizer()  = default;
    ~TextNormalizer() = default;

    TextNormalizer(const TextNormalizer&)            = default;
    TextNormalizer& operator=(const TextNormalizer&) = default;

    /**
     * @brief Normalise German umlauts and ß in @p text.
     *
     * @param text UTF-8 input text.
     * @return Normalised UTF-8 string.
     */
    std::string normalize(std::string_view text) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Normalise German umlauts and ß in @p text.
 *
 * Convenience free function that delegates to `utils::Normalizer::normalizeUmlauts`.
 *
 * @param text UTF-8 input text.
 * @return Normalised UTF-8 string (umlaut-folded to ASCII equivalents).
 */
std::string normalizeText(std::string_view text);

} // namespace toolbox
} // namespace themis
