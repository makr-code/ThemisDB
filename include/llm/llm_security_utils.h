/**
 * @file llm_security_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>

namespace themis {
namespace llm {

/**
 * @brief Sanitize (mask) an LLM API key for safe logging and display.
 *
 * Replaces the middle portion of the key with asterisks so that the raw
 * credential never appears in log output or error messages.  At most 4
 * characters at the start and 4 at the end are shown; keys shorter than
 * 9 characters are fully masked.
 *
 * Example: "sk-abcdefghij1234567890xyz" → "sk-a***...***0xyz"
 *
 * @param api_key  The raw API key string (may be empty).
 * @return         A masked representation safe for logging.
 */
std::string sanitizeApiKey(const std::string& api_key);

} // namespace llm
} // namespace themis
