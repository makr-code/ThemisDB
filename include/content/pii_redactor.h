/**
 * @file pii_redactor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace content {

// ---------------------------------------------------------------------------
// PIIType — enumeration of detectable PII entity types
// ---------------------------------------------------------------------------

enum class PIIType {
    EMAIL,
    PHONE,
    SSN,              ///< US Social Security Number.
    CREDIT_CARD,
    IP_ADDRESS,
    PERSON_NAME,
    ADDRESS,
    DATE_OF_BIRTH,
    PASSPORT,
    DRIVER_LICENSE,
    IBAN,
    SWIFT_CODE,
    MEDICAL_RECORD,
    CUSTOM,
};

// ---------------------------------------------------------------------------
// PIIMatch — a detected PII span in the source text
// ---------------------------------------------------------------------------

struct PIIMatch {
    PIIType     type;
    size_t      start_offset = 0;
    size_t      end_offset   = 0;
    std::string original_value;
    float       confidence   = 1.0f;
};

// ---------------------------------------------------------------------------
// RedactionMode — strategy for replacing detected PII
// ---------------------------------------------------------------------------

enum class RedactionMode {
    MASK,      ///< Replace with asterisks (e.g., "*****").
    REPLACE,   ///< Replace with a typed placeholder (e.g., "[EMAIL]").
    HASH,      ///< Replace with a one-way hash for referential integrity.
    TOKENIZE,  ///< Replace with a reversible token (requires a token vault).
    ENCRYPT,   ///< Replace with an encrypted ciphertext.
};

// ---------------------------------------------------------------------------
// PIIRedactionConfig — configuration for a redact() call
// ---------------------------------------------------------------------------

struct PIIRedactionConfig {
    RedactionMode            mode             = RedactionMode::REPLACE;
    std::vector<PIIType>     types_to_redact; ///< Empty = all types.
    float                    min_confidence   = 0.75f;
    bool                     preserve_format  = false;
};

// ---------------------------------------------------------------------------
// PIIRedactionResult — output from a redact() call
// ---------------------------------------------------------------------------

struct PIIRedactionResult {
    std::string          redacted_text;
    std::vector<PIIMatch> detected;
    int                  redaction_count = 0;
};

// ---------------------------------------------------------------------------
// IPIIRedactor — PII detection and redaction interface
// ---------------------------------------------------------------------------

class IPIIRedactor {
public:
    /**
     * @brief IPIIRedactor.
     * @return Return value.
     */
    virtual ~IPIIRedactor() = default;

    [[nodiscard]] virtual PIIRedactionResult redact(
        const std::string&      text,
        const PIIRedactionConfig& config = {}
    ) = 0;

    [[nodiscard]] virtual std::vector<PIIMatch> detect(const std::string& text) = 0;

    [[nodiscard]] virtual bool train(const std::string& custom_model_path) = 0;

    [[nodiscard]] virtual std::vector<PIIType> supportedTypes() const = 0;
};

} // namespace content
} // namespace themis
