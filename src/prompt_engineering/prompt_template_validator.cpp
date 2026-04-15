/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_template_validator.cpp                      ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:09:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 240a2c1d8b  2026-04-12  feat(prompt_engineering): Typed Template DSL — PromptTemp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "prompt_engineering/prompt_template_validator.h"

#include <stdexcept>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// PromptTemplateValidator
// ============================================================================

PromptTemplateValidator::PromptTemplateValidator(bool require_id)
    : require_id_(require_id)
{}

TemplateValidationResult
PromptTemplateValidator::validate(const nlohmann::json& j) const {
    TemplateValidationResult result;

    // Must be an object at the top level
    if (!j.is_object()) {
        result.errors.push_back("Template JSON must be an object");
        result.valid = false;
        return result;
    }

    // --- id ---
    if (require_id_) {
        if (!j.contains("id") || !j["id"].is_string()) {
            result.errors.push_back("Field 'id' must be a non-empty string");
        } else if (j["id"].get<std::string>().empty()) {
            result.errors.push_back("Field 'id' must not be empty");
        }
    }

    // --- name ---
    if (!j.contains("name") || !j["name"].is_string()) {
        result.errors.push_back("Field 'name' must be a non-empty string");
    } else if (j["name"].get<std::string>().empty()) {
        result.errors.push_back("Field 'name' must not be empty");
    }

    // --- version ---
    if (!j.contains("version") || !j["version"].is_string()) {
        result.errors.push_back("Field 'version' must be a non-empty string");
    } else if (j["version"].get<std::string>().empty()) {
        result.errors.push_back("Field 'version' must not be empty");
    }

    // --- content ---
    if (!j.contains("content") || !j["content"].is_string()) {
        result.errors.push_back("Field 'content' must be a non-empty string");
    } else if (j["content"].get<std::string>().empty()) {
        result.errors.push_back("Field 'content' must not be empty");
    }

    // --- description (advisory) ---
    if (!j.contains("description") || !j["description"].is_string()) {
        result.errors.push_back("Field 'description' must be a string");
    } else if (j["description"].get<std::string>().empty()) {
        result.warnings.push_back("Field 'description' is empty – consider adding one");
    }

    // --- active ---
    if (!j.contains("active") || !j["active"].is_boolean()) {
        result.errors.push_back("Field 'active' must be a boolean");
    }

    // --- metadata ---
    if (j.contains("metadata")) {
        if (!j["metadata"].is_object() && !j["metadata"].is_null()) {
            result.errors.push_back("Field 'metadata' must be a JSON object or null");
        }
    }

    // --- images ---
    if (j.contains("images")) {
        if (!j["images"].is_array()) {
            result.errors.push_back("Field 'images' must be an array");
        } else {
            const auto& images = j["images"];
            for (std::size_t i = 0; i < images.size(); ++i) {
                const auto& img = images[i];
                if (!img.is_object()) {
                    result.errors.push_back(
                        "images[" + std::to_string(i) + "] must be an object");
                    continue;
                }
                if (!img.contains("alt_text") || !img["alt_text"].is_string() ||
                    img["alt_text"].get<std::string>().empty()) {
                    result.errors.push_back(
                        "images[" + std::to_string(i) +
                        "] 'alt_text' must be a non-empty string");
                }
            }
        }
    }

    result.valid = result.errors.empty();
    return result;
}

TemplateValidationResult
PromptTemplateValidator::validate(const std::string& json_str) const {
    TemplateValidationResult result;
    try {
        auto j = nlohmann::json::parse(json_str);
        return validate(j);
    } catch (const nlohmann::json::parse_error& e) {
        result.valid = false;
        result.errors.push_back(std::string("JSON parse error: ") + e.what());
        return result;
    }
}

} // namespace prompt_engineering
} // namespace themis
