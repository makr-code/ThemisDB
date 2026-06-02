/*
 * ThemisDB | File: pii_redactor.h | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 170
 * Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file pii_redactor.h
 * @brief Content-level PII detection and redaction interface.
 *
 * IPIIRedactor detects and redacts Personally Identifiable Information (PII)
 * before content is stored or indexed, supporting multiple redaction strategies
 * from simple masking to reversible tokenisation.
 *
 * Compliance: GDPR Art. 25 (data minimisation by design), CCPA, HIPAA §164.514.
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

/**
 * @brief Recognized PII entity types.
 *
 * CUSTOM covers user-defined patterns registered via `train()`.
 */
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

/**
 * @brief A single PII entity detected in the source text.
 *
 * `start_offset` and `end_offset` are byte offsets (not character offsets)
 * into the original UTF-8 string passed to `detect()` or `redact()`.
 */
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

/**
 * @brief Strategy used to replace detected PII in the output text.
 */
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

/**
 * @brief Configuration that governs a single redact() invocation.
 *
 * An empty `types_to_redact` means "redact all detected PII types".
 * `preserve_format` is only relevant for REPLACE mode and emits a
 * format-aware placeholder such as "[PHONE: xxx-xxx-xxxx]".
 */
struct PIIRedactionConfig {
    RedactionMode            mode             = RedactionMode::REPLACE;
    std::vector<PIIType>     types_to_redact; ///< Empty = all types.
    float                    min_confidence   = 0.75f;
    bool                     preserve_format  = false;
};

// ---------------------------------------------------------------------------
// PIIRedactionResult — output from a redact() call
// ---------------------------------------------------------------------------

/**
 * @brief Result of a redact() invocation.
 */
struct PIIRedactionResult {
    std::string          redacted_text;
    std::vector<PIIMatch> detected;
    int                  redaction_count = 0;
};

// ---------------------------------------------------------------------------
// IPIIRedactor — PII detection and redaction interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for content-level PII detection and redaction.
 *
 * Implementations wrap NER models (spaCy, Presidio, AWS Comprehend, custom regex).
 *
 * ### Thread safety
 * `redact()` and `detect()` must be safe to call concurrently.
 * `train()` may hold an exclusive lock while loading the model.
 */
class IPIIRedactor {
public:
    virtual ~IPIIRedactor() = default;

    /**
     * @brief Detect and redact PII in @p text.
     *
     * @param text    Input text (UTF-8).
     * @param config  Redaction configuration; defaults are applied if omitted.
     * @return PIIRedactionResult with the redacted text and all detected matches.
     */
    [[nodiscard]] virtual PIIRedactionResult redact(
        const std::string&      text,
        const PIIRedactionConfig& config = {}
    ) = 0;

    /**
     * @brief Detect PII in @p text without modifying it.
     *
     * Useful for pre-flight inspection before deciding whether to store content.
     */
    [[nodiscard]] virtual std::vector<PIIMatch> detect(const std::string& text) = 0;

    /**
     * @brief Load or fine-tune a custom NER model for CUSTOM entity detection.
     *
     * @param custom_model_path  Path to the model artefact.
     * @return `true` if the model loaded successfully.
     */
    [[nodiscard]] virtual bool train(const std::string& custom_model_path) = 0;

    /// Return the PIIType values this redactor can detect.
    [[nodiscard]] virtual std::vector<PIIType> supportedTypes() const = 0;
};

} // namespace content
} // namespace themis
