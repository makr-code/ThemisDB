/*
 * ThemisDB | File: text_normalizer.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 32
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=3 | delta=0 | status=aligned
 * External Severity (v3): C=0, H=3, M=0
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/text_normalizer.h"
#include "utils/normalizer.h"

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// TextNormalizer
// ─────────────────────────────────────────────────────────────────────────────

std::string TextNormalizer::normalize(std::string_view text) const {
    return utils::Normalizer::normalizeUmlauts(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function
// ─────────────────────────────────────────────────────────────────────────────

std::string normalizeText(std::string_view text) {
    return utils::Normalizer::normalizeUmlauts(text);
}

} // namespace toolbox
} // namespace themis
