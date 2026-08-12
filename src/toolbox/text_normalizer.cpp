/**
 * @file text_normalizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/text_normalizer.h"
#include "utils/normalizer.h"

#include <atomic>
#include <sstream>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Helper metrics tracking
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::atomic<uint64_t> g_text_normalizer_errors_total(0);  ///< Helper error counter
}

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

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Metrics export for helper diagnostics
// ─────────────────────────────────────────────────────────────────────────────

std::string getTextNormalizerMetrics() {
    const uint64_t errors = g_text_normalizer_errors_total.load(std::memory_order_relaxed);
    if (errors == 0) return "";
    
    std::ostringstream out;
    out << "# HELP toolbox_text_normalizer_errors_total Text normalizer helper errors.\n";
    out << "# TYPE toolbox_text_normalizer_errors_total counter\n";
    out << "toolbox_text_normalizer_errors_total " << errors << "\n";
    return out.str();
}

} // namespace toolbox
} // namespace themis
