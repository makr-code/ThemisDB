/**
 * @file format_template.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/format_template.h"

#include <nlohmann/json.hpp>
#include <set>

using json = nlohmann::json;

namespace themis {
namespace exporters {

// ---------------------------------------------------------------------------
// AlpacaTemplate
// ---------------------------------------------------------------------------

bool AlpacaTemplate::validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping,
                                    std::vector<std::string> *missing_fields) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.instruction_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.instruction_field);
        }
    }
    if (!entity.getFieldAsString(mapping.output_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.output_field);
        }
    }
    return ok;
}

std::string AlpacaTemplate::render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const {
    auto instruction = entity.getFieldAsString(mapping.instruction_field);
    auto output      = entity.getFieldAsString(mapping.output_field);

    if (!instruction || !output) {
        return {};
    }

    json j;
    j["instruction"] = *instruction;

    auto input = entity.getFieldAsString(mapping.input_field);
    if (input && !input->empty()) {
        j["input"] = *input;
    } else {
        j["input"] = "";
    }

    j["output"] = *output;
    return j.dump();
}

// ---------------------------------------------------------------------------
// ShareGPTTemplate
// ---------------------------------------------------------------------------

bool ShareGPTTemplate::validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping,
                                      std::vector<std::string> *missing_fields) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.user_field);
        }
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.assistant_field);
        }
    }
    return ok;
}

std::string ShareGPTTemplate::render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const {
    auto user      = entity.getFieldAsString(mapping.user_field);
    auto assistant = entity.getFieldAsString(mapping.assistant_field);

    if (!user || !assistant) {
        return {};
    }

    json conversations = json::array();

    auto system = entity.getFieldAsString(mapping.system_field);
    if (system && !system->empty()) {
        conversations.push_back({{"from", "system"}, {"value", *system}});
    }

    conversations.push_back({{"from", "human"}, {"value", *user}});
    conversations.push_back({{"from", "gpt"}, {"value", *assistant}});

    json j;
    j["conversations"] = conversations;
    return j.dump();
}

// ---------------------------------------------------------------------------
// ChatMLTemplate
// ---------------------------------------------------------------------------

bool ChatMLTemplate::validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping,
                                    std::vector<std::string> *missing_fields) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.user_field);
        }
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.assistant_field);
        }
    }
    return ok;
}

std::string ChatMLTemplate::render(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping) const {
    auto user      = entity.getFieldAsString(mapping.user_field);
    auto assistant = entity.getFieldAsString(mapping.assistant_field);

    if (!user || !assistant) {
        return {};
    }

    json messages = json::array();

    auto system = entity.getFieldAsString(mapping.system_field);
    if (system && !system->empty()) {
        messages.push_back({{"role", "system"}, {"content", *system}});
    }

    messages.push_back({{"role", "user"}, {"content", *user}});
    messages.push_back({{"role", "assistant"}, {"content", *assistant}});

    json j;
    j["messages"] = messages;
    return j.dump();
}

// ---------------------------------------------------------------------------
// OpenAIFineTuningTemplate
// ---------------------------------------------------------------------------

bool OpenAIFineTuningTemplate::validateFields(const BaseEntity &entity, const FormatTemplateFieldMapping &mapping,
                                              std::vector<std::string> *missing_fields) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.user_field);
        }
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) {
            missing_fields->push_back(mapping.assistant_field);
        }
    }
    return ok;
}

std::string OpenAIFineTuningTemplate::render(const BaseEntity &entity,
                                             const FormatTemplateFieldMapping &mapping) const {
    auto user      = entity.getFieldAsString(mapping.user_field);
    auto assistant = entity.getFieldAsString(mapping.assistant_field);

    if (!user || !assistant) {
        return {};
    }

    json messages = json::array();

    auto system = entity.getFieldAsString(mapping.system_field);
    if (system && !system->empty()) {
        messages.push_back({{"role", "system"}, {"content", *system}});
    }

    messages.push_back({{"role", "user"}, {"content", *user}});
    messages.push_back({{"role", "assistant"}, {"content", *assistant}});

    json j;
    j["messages"] = messages;
    return j.dump();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<IFormatTemplate> makeFormatTemplate(FormatTemplateType type) {
    switch (type) {
        case FormatTemplateType::ALPACA:
            return std::make_unique<AlpacaTemplate>();
        case FormatTemplateType::SHAREGPT:
            return std::make_unique<ShareGPTTemplate>();
        case FormatTemplateType::CHATML:
            return std::make_unique<ChatMLTemplate>();
        case FormatTemplateType::OPENAI_FINETUNING:
            return std::make_unique<OpenAIFineTuningTemplate>();
        default:
            return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Dry-run / preflight validation
// ---------------------------------------------------------------------------

TemplateValidationResult validateTemplate(FormatTemplateType type, const FormatTemplateFieldMapping &mapping,
                                          const std::vector<BaseEntity> &sample) {
    TemplateValidationResult result;

    // No template active — nothing to validate.
    if (type == FormatTemplateType::NONE) {
        result.valid = true;
        return result;
    }

    auto tpl = makeFormatTemplate(type);
    if (!tpl) {
        // Unknown type treated as no template.
        result.valid = true;
        return result;
    }

    std::set<std::string> missing_set;

    for (const auto &entity : sample) {
        ++result.entities_checked;
        std::vector<std::string> entity_missing = {};

        if (!tpl->validateFields(entity, mapping, &entity_missing)) {
            ++result.entities_failed;
            for (const auto &f : entity_missing) {
                missing_set.insert(f);
            }
        }
    }

    result.valid = (result.entities_failed == 0);
    result.missing_fields.assign(missing_set.begin(), missing_set.end());
    return result;
}

} // namespace exporters
} // namespace themis
