/**
 * @file format_template.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class FormatTemplateType {
    NONE,               ///< No template — use JSONLFormat::Style directly
    ALPACA,             ///< {"instruction":…,"input":…,"output":…}
    SHAREGPT,           ///< {"conversations":[{"from":"human","value":…},{"from":"gpt","value":…}]}
    CHATML,             ///< {"messages":[{"role":"system","content":…},…]}
    OPENAI_FINETUNING   ///< OpenAI fine-tuning JSONL (same shape as ChatML)
};

struct FormatTemplateFieldMapping {
    std::string instruction_field  = "question";
    std::string input_field        = "context";
    std::string output_field       = "answer";
    std::string system_field       = "system_prompt";
    std::string user_field         = "user_message";
    std::string assistant_field    = "assistant_response";
};

class IFormatTemplate {
public:
    /**
     * @brief IFormat Template.
     * @return Return value.
     */
    virtual ~IFormatTemplate() = default;

    /**
     * @brief Name.
     * @return Return value.
     */
    virtual std::string name() const = 0;

    virtual bool validateFields(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping,
        std::vector<std::string>* missing_fields = nullptr
    ) const = 0;

    /**
     * @brief Render.
     * @param[in] entity Input parameter.
     * @param[in] mapping Input parameter.
     * @return Return value.
     */
    virtual std::string render(
        const BaseEntity& entity,
        const FormatTemplateFieldMapping& mapping
    ) const = 0;
};

// ---------------------------------------------------------------------------
// Concrete templates
// ---------------------------------------------------------------------------

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

/**
 * @brief Make Format Template.
 * @param[in] type Input parameter.
 * @return Return value.
 */
std::unique_ptr<IFormatTemplate> makeFormatTemplate(FormatTemplateType type);

// ---------------------------------------------------------------------------
// Dry-run / preflight validation
// ---------------------------------------------------------------------------

struct TemplateValidationResult {
    bool valid = true;

    std::vector<std::string> missing_fields;

    size_t entities_checked = 0;

    size_t entities_failed = 0;
};

/**
 * @brief Validate Template.
 * @param[in] type Input parameter.
 * @param[in] mapping Input parameter.
 * @param[in] sample Input parameter.
 * @return Return value.
 */
TemplateValidationResult validateTemplate(
    FormatTemplateType type,
    const FormatTemplateFieldMapping& mapping,
    const std::vector<BaseEntity>& sample
);

} // namespace exporters
} // namespace themis
