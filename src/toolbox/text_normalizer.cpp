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
