/*
 * ThemisDB | File: llm_security_utils.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
