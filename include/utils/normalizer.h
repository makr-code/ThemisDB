/**
 * @file normalizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace utils {

class Normalizer {
public:
    // Normalize German umlauts and ß to ASCII equivalents.
    // ä->a, ö->o, ü->u, Ä->A, Ö->O, Ü->U, ß->ss
    // Input is expected to be UTF-8; returns normalized UTF-8 string.
    static std::string normalizeUmlauts(std::string_view text);
};

} // namespace utils
} // namespace themis
