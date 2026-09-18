/**
 * @file normalizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace utils {

/** @brief Normalizer. */
class Normalizer {
public:
    /**
     * @brief Normalize German umlauts and ß to ASCII equivalents.
     * @param[in] text Input parameter.
     * @return Return value.
     * @details ä->a, ö->o, ü->u, Ä->A, Ö->O, Ü->U, ß->ss Input is expected to be UTF-8; returns normalized UTF-8 string.
     */
    static std::string normalizeUmlauts(std::string_view text);
};

} // namespace utils
} // namespace themis
