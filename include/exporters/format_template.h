/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            format_template.h                                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:53:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     168                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 47062c4ec  2026-02-28  Implement Alpaca, ShareGPT, ChatML, and OpenAI instructio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

} // namespace exporters
} // namespace themis
