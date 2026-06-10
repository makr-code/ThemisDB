/**
 * @file text_normalizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#pragma once
std::string normalizeText(std::string_view text);

} // namespace toolbox
} // namespace themis
