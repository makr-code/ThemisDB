/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            format_template.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:57:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     216                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 47062c4ec  2026-02-28  Implement Alpaca, ShareGPT, ChatML, and OpenAI instructio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/format_template.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace exporters {

// ---------------------------------------------------------------------------
// AlpacaTemplate
// ---------------------------------------------------------------------------

bool AlpacaTemplate::validateFields(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping,
    std::vector<std::string>* missing_fields
) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.instruction_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.instruction_field);
    }
    if (!entity.getFieldAsString(mapping.output_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.output_field);
    }
    return ok;
}

std::string AlpacaTemplate::render(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping
) const {
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

bool ShareGPTTemplate::validateFields(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping,
    std::vector<std::string>* missing_fields
) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.user_field);
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.assistant_field);
    }
    return ok;
}

std::string ShareGPTTemplate::render(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping
) const {
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
    conversations.push_back({{"from", "gpt"},   {"value", *assistant}});

    json j;
    j["conversations"] = conversations;
    return j.dump();
}

// ---------------------------------------------------------------------------
// ChatMLTemplate
// ---------------------------------------------------------------------------

bool ChatMLTemplate::validateFields(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping,
    std::vector<std::string>* missing_fields
) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.user_field);
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.assistant_field);
    }
    return ok;
}

std::string ChatMLTemplate::render(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping
) const {
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

    messages.push_back({{"role", "user"},      {"content", *user}});
    messages.push_back({{"role", "assistant"}, {"content", *assistant}});

    json j;
    j["messages"] = messages;
    return j.dump();
}

// ---------------------------------------------------------------------------
// OpenAIFineTuningTemplate
// ---------------------------------------------------------------------------

bool OpenAIFineTuningTemplate::validateFields(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping,
    std::vector<std::string>* missing_fields
) const {
    bool ok = true;
    if (!entity.getFieldAsString(mapping.user_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.user_field);
    }
    if (!entity.getFieldAsString(mapping.assistant_field)) {
        ok = false;
        if (missing_fields) missing_fields->push_back(mapping.assistant_field);
    }
    return ok;
}

std::string OpenAIFineTuningTemplate::render(
    const BaseEntity& entity,
    const FormatTemplateFieldMapping& mapping
) const {
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

    messages.push_back({{"role", "user"},      {"content", *user}});
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

} // namespace exporters
} // namespace themis
