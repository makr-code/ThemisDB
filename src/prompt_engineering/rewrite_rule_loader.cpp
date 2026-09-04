/**
 * @file rewrite_rule_loader.cpp
 * @brief YAML configuration loader for rewrite rules (Phase 2 delivery).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 YAML loading (Q4 2026)
 *
 * Loads and validates rewrite rules from YAML configuration files.
 * Only lexical rules (regex, dictionary) are permitted from YAML.
 * Semantic and policy rules must be registered programmatically in C++.
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "prompt_engineering/rewrite_rule_loader.h"
#include "prompt_engineering/rewrite_rule.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace themis {
namespace prompt_engineering {

RewriteRuleLoader::RewriteRuleLoader() : last_error_("") {
}

RewriteRuleLoader::~RewriteRuleLoader() = default;

bool RewriteRuleLoader::load_rules_from_yaml(
    const std::string& yaml_path,
    std::vector<std::shared_ptr<IRewriteRule>>& rules
) {
    auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

    last_error_ = "";

    // Read YAML file
    std::ifstream file(yaml_path);
    if (!file.is_open()) {
        last_error_ = "Failed to open file: " + yaml_path;
        logger->error("{}", last_error_);
        return false;
    }

    std::stringstream buffer = {};
    buffer << file.rdbuf();
    file.close();

    std::vector<std::shared_ptr<IRewriteRule>> temp_rules;

    try {
        YAML::Node config = YAML::Load(buffer.str());

        if (!config["rules"] || !config["rules"].IsSequence()) {
            last_error_ = "YAML must contain 'rules' sequence";
            logger->error("{}", last_error_);
            return false;
        }

        for (const auto& rule_node : config["rules"]) {
            // Extract required fields
            if (!rule_node["id"]) {
                last_error_ = "Rule missing required 'id' field";
                logger->error("{}", last_error_);
                return false;
            }

            std::string rule_id = rule_node["id"].as<std::string>();
            std::string rule_type = rule_node["type"].as<std::string>("regex");
            std::string phase_str = rule_node["phase"].as<std::string>("input");
            uint8_t priority = rule_node["priority"].as<uint8_t>(100);
            std::string description = rule_node["description"].as<std::string>("");

            logger->debug("Loading rule: {} (type={})", rule_id, rule_type);

            // Only lexical rules are allowed from YAML
            if (rule_type != "regex" && rule_type != "dictionary") {
                last_error_ = "YAML can only load 'regex' or 'dictionary' rules, not: " + rule_type;
                logger->error("{}", last_error_);
                return false;
            }

            // Convert phase string to enum
            RewritePhase phase = RewritePhase::PHASE_1_INPUT_NORMALIZATION;
            if (phase_str == "input") {
                phase = RewritePhase::PHASE_1_INPUT_NORMALIZATION;
            } else if (phase_str == "policy") {
                phase = RewritePhase::PHASE_2_POLICY_ENFORCEMENT;
            } else if (phase_str == "nl_aql") {
                phase = RewritePhase::PHASE_3_NL_AQL_PREPROCESSING;
            } else if (phase_str == "post_gen") {
                phase = RewritePhase::PHASE_4_POST_GENERATION;
            } else {
                last_error_ = "Invalid phase: " + phase_str;
                logger->error("{}", last_error_);
                return false;
            }

            nlohmann::json rule_config;
            
            try {
                // Convert YAML node to JSON for easier handling
                std::stringstream yaml_dump = {};
                yaml_dump << rule_node;
                rule_config = nlohmann::json::parse(yaml_dump.str());
            } catch (...) {
                // Continue with empty config
            }

            auto rule = parse_rule_definition(
                rule_id, rule_type, phase_str, priority, description, rule_config
            );

            if (!rule) {
                last_error_ = "Failed to create rule: " + rule_id;
                logger->error("{}", last_error_);
                return false;
            }

            temp_rules.push_back(rule);
        }

    } catch (const YAML::Exception& e) {
        last_error_ = std::string("YAML parse error: ") + e.what();
        logger->error("{}", last_error_);
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Unexpected error loading YAML: ") + e.what();
        logger->error("{}", last_error_);
        return false;
    }

    // All-or-nothing semantics: only add to output if all loaded successfully
    for (auto& rule : temp_rules) {
        rules.push_back(rule);
    }

    logger->info("Successfully loaded {} rules from {}", temp_rules.size(), yaml_path);
    return true;
}

std::string RewriteRuleLoader::last_error() const {
    return last_error_;
}

bool RewriteRuleLoader::validate_yaml_rules(const std::string& yaml_content) {
    auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

    last_error_ = "";

    try {
        YAML::Node config = YAML::Load(yaml_content);

        if (!config["rules"] || !config["rules"].IsSequence()) {
            last_error_ = "YAML must contain 'rules' sequence";
            return false;
        }

        for (const auto& rule_node : config["rules"]) {
            if (!rule_node["id"]) {
                last_error_ = "Rule missing required 'id' field";
                return false;
            }

            std::string rule_type = rule_node["type"].as<std::string>("regex");

            // Validate rule type
            if (rule_type != "regex" && rule_type != "dictionary") {
                last_error_ = "Invalid rule type (must be 'regex' or 'dictionary'): " + rule_type;
                return false;
            }

            // For regex rules, validate pattern compiles
            if (rule_type == "regex") {
                if (!rule_node["pattern"]) {
                    last_error_ = "Regex rule missing 'pattern' field";
                    return false;
                }

                std::string pattern = rule_node["pattern"].as<std::string>();
                try {
                    std::regex test_pattern(pattern);
                } catch (const std::regex_error& e) {
                    last_error_ = std::string("Invalid regex pattern: ") + e.what();
                    return false;
                }
            }

            // For dictionary rules, validate mappings
            if (rule_type == "dictionary") {
                if (!rule_node["mappings"]) {
                    last_error_ = "Dictionary rule missing 'mappings' field";
                    return false;
                }
            }

            // Validate phase
            std::string phase_str = rule_node["phase"].as<std::string>("input");
            if (phase_str != "input" && phase_str != "policy" && 
                phase_str != "nl_aql" && phase_str != "post_gen") {
                last_error_ = "Invalid phase: " + phase_str;
                return false;
            }
        }

    } catch (const YAML::Exception& e) {
        last_error_ = std::string("YAML parse error: ") + e.what();
        return false;
    }

    return true;
}

std::shared_ptr<IRewriteRule> RewriteRuleLoader::parse_rule_definition(
    const std::string& rule_id,
    const std::string& rule_type,
    const std::string& phase_str,
    uint8_t priority,
    const std::string& description,
    const nlohmann::json& rule_config
) {
    auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

    RewritePhase phase = RewritePhase::PHASE_1_INPUT_NORMALIZATION;
    if (phase_str == "policy") {
        phase = RewritePhase::PHASE_2_POLICY_ENFORCEMENT;
    } else if (phase_str == "nl_aql") {
        phase = RewritePhase::PHASE_3_NL_AQL_PREPROCESSING;
    } else if (phase_str == "post_gen") {
        phase = RewritePhase::PHASE_4_POST_GENERATION;
    }

    try {
        if (rule_type == "regex") {
            if (rule_config.empty() || !rule_config.contains("pattern")) {
                logger->error("Regex rule {} missing pattern", rule_id);
                return nullptr;
            }

            std::string pattern = rule_config["pattern"].get<std::string>();
            std::string replacement = rule_config.contains("replacement") 
                ? rule_config["replacement"].get<std::string>()
                : "$0";

            uint32_t max_replacements = rule_config.contains("max_replacements")
                ? rule_config["max_replacements"].get<uint32_t>()
                : 0;

            return std::make_shared<RegexRewriteRule>(
                rule_id, priority, phase, pattern, replacement, description, max_replacements
            );

        } else if (rule_type == "dictionary") {
            if (rule_config.empty() || !rule_config.contains("mappings")) {
                logger->error("Dictionary rule {} missing mappings", rule_id);
                return nullptr;
            }

            std::unordered_map<std::string, std::string> mappings;
            auto mappings_obj = rule_config["mappings"].get<nlohmann::json>();

            for (auto& [key, value] : mappings_obj.items()) {
                mappings[key] = value.get<std::string>();
            }

            bool case_sensitive = rule_config.contains("case_sensitive")
                ? rule_config["case_sensitive"].get<bool>()
                : false;

            uint32_t max_replacements = rule_config.contains("max_replacements")
                ? rule_config["max_replacements"].get<uint32_t>()
                : 0;

            return std::make_shared<DictionaryRewriteRule>(
                rule_id, priority, phase, mappings, description, case_sensitive, max_replacements
            );
        }

    } catch (const std::exception& e) {
        logger->error("Failed to parse rule {}: {}", rule_id, e.what());
        return nullptr;
    }

    return nullptr;
}

} // namespace prompt_engineering
} // namespace themis
