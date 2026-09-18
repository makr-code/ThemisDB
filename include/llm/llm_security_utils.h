/**
 * @file llm_security_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>

namespace themis {
namespace llm {

/**
 * @brief Sanitize Api Key.
 * @param[in] api_key Input parameter.
 * @return Return value.
 */
std::string sanitizeApiKey(const std::string& api_key);

} // namespace llm
} // namespace themis
