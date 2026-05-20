/*
 * ThemisDB | File: text_normalizer.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
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
