/**
 * @file format_template.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/base_entity.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace exporters {

/// Named instruction-tuning format schemas supported by the JSONL exporter.
enum class FormatTemplateType {
    NONE,               ///< No template — use JSONLFormat::Style directly
    ALPACA,             ///< {"instruction":…,"input":…,"output":…}
    SHAREGPT,           ///< {"conversations":[{"from":"human","value":…},{"from":"gpt","value":…}]}
    CHATML,             ///< {"messages":[{"role":"system","content":…},…]}
    OPENAI_FINETUNING   ///< OpenAI fine-tuning JSONL (same shape as ChatML)
};

/// Field-name configuration shared by all templates.
/// Each template uses only the subset of fields it requires.
struct FormatTemplateFieldMapping {
    std::string instruction_field  = "question";
    std::string input_field        = "context";
    std::string output_field       = "answer";
    std::string system_field       = "system_prompt";
    std::string user_field         = "user_message";
    std::string assistant_field    = "assistant_response";
};

/// Abstract base for a single named instruction-tuning template.
class IFormatTemplate {
public:
    virtual ~IFormatTemplate() = default;

    /// Return the canonical name of this template (e.g. "alpaca").
    virtual std::string name() const = 0;

    /// Validate that all required fields are present in the entity.
    /// Returns true when the entity can be rendered; false when mandatory
    /// fields are missing.  Populates \p missing_fields on failure.
    virtual bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const = 0;

    /// Render the entity as a single JSON object string (no trailing newline).
    /// Returns an empty string when required fields are absent.
    virtual std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const = 0;
};

// ---------------------------------------------------------------------------
// Concrete templates
// ---------------------------------------------------------------------------

/// Alpaca instruction-tuning format.
/// Schema: {"instruction":…, "input":…, "output":…}
/// "input" is present but empty when the input field is absent or empty
/// (follows the original Alpaca specification which always includes the key).
class AlpacaTemplate : public IFormatTemplate {
public:
    std::string name() const override { return "alpaca"; }

    bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const override;

    std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const override;
};

/// ShareGPT conversation format.
/// Schema: {"conversations":[{"from":"human","value":…},{"from":"gpt","value":…}]}
/// An optional "system" turn is prepended when the system_field is present and non-empty.
class ShareGPTTemplate : public IFormatTemplate {
public:
    std::string name() const override { return "sharegpt"; }

    bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const override;

    std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const override;
};

/// ChatML message-list format.
/// Schema: {"messages":[{"role":"system","content":…},{"role":"user","content":…},{"role":"assistant","content":…}]}
/// The "system" message is omitted when the system_field is absent or empty.
class ChatMLTemplate : public IFormatTemplate {
public:
    std::string name() const override { return "chatml"; }

    bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const override;

    std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const override;
};

/// OpenAI fine-tuning JSONL format.
/// Structurally identical to ChatML; exposed as a separate type so callers
/// can distinguish intent without losing the explicit schema identity.
class OpenAIFineTuningTemplate : public IFormatTemplate {
public:
    std::string name() const override { return "openai_finetuning"; }

    bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const override;

    std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const override;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/// Create an IFormatTemplate for the given type.
/// Returns nullptr for FormatTemplateType::NONE.
std::unique_ptr<IFormatTemplate> makeFormatTemplate(FormatTemplateType type);

// ---------------------------------------------------------------------------
// Dry-run / preflight validation
// ---------------------------------------------------------------------------

/// Result of a collection-level template dry-run validation.
struct TemplateValidationResult {
    /// True when every entity in the sample satisfies the template's required
    /// fields (or when no template is active, i.e. type == NONE).
    bool valid = true;

    /// Sorted, deduplicated list of field names that were absent in at least
    /// one entity of the sample.  Empty when \p valid is true.
    std::vector<std::string> missing_fields;

    /// Number of entities that were examined.
    size_t entities_checked = 0;

    /// Number of entities that failed validation (i.e. had at least one
    /// missing required field).
    size_t entities_failed = 0;
};

/// Validate that all entities in \p sample provide the required fields for
/// the selected template type.
///
/// \param type     The format template to check against.  When NONE the
///                 result is always valid (no template fields are required).
/// \param mapping  Field-name overrides forwarded to the template.
/// \param sample   Representative collection of entities to inspect.  At
///                 least one entity is recommended for meaningful results;
///                 an empty sample yields a valid result with
///                 entities_checked == 0.
///
/// \returns A TemplateValidationResult summarising the outcome.  The
///          \p missing_fields list is deterministic (sorted) so callers can
///          rely on its order for automated comparisons.
TemplateValidationResult validateTemplate(
    FormatTemplateType type,
    const FormatTemplateFieldMapping& mapping,
    const std::vector<BaseEntity>& sample
);

} // namespace exporters
} // namespace themis
