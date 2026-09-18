/**
 * @file ethics_base_entity_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include <nlohmann/json.hpp>
#include "utils/logger.h"

namespace themis {
namespace plugins {
namespace ethics {

class EthicsBaseEntityAdapter {
public:
    /**
     * @brief ========== Ethical Argument Conversion ==========
     * @param[in] argument Input parameter.
     * @return Return value.
     * @details Calls: argumentTypeToString(), argumentStrengthToString(), time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().
     */
    
    static BaseEntity toBaseEntity(const EthicalArgument& argument) {
        BaseEntity::FieldMap fields;
        
        fields["philosophy_school"] = argument.philosophy_school;
        fields["argument_type"] = argumentTypeToString(argument.argument_type);
        fields["content"] = argument.content;
        fields["strength"] = argumentStrengthToString(argument.strength);
        fields["created_at"] = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                argument.created_at.time_since_epoch()
            ).count()
        );
        
        // Store arrays as JSON strings (ThemisDB Value doesn't support nested arrays yet)
        if (!argument.principle_basis.empty()) {
            nlohmann::json j = argument.principle_basis;
            fields["principle_basis"] = j.dump();
        }
        
        if (!argument.counterarguments.empty()) {
            nlohmann::json j = argument.counterarguments;
            fields["counterarguments"] = j.dump();
        }
        
        if (!argument.supports.empty()) {
            nlohmann::json j = argument.supports;
            fields["supports"] = j.dump();
        }
        
        return BaseEntity::fromFields(argument.id, fields);
    }
    
    /**
     * @brief From Base Entity.
     * @param[in] entity Input parameter.
     * @return Return value.
     * @details Calls: getPrimaryKey(), getFieldAsString(), value_or(), stringToArgumentType(), stringToArgumentStrength(), getFieldAsInt(), std::chrono::system_clock::from_time_t(), nlohmann::json::parse().
     */
    static EthicalArgument fromBaseEntity(const BaseEntity& entity) {
        EthicalArgument argument;
        argument.id = entity.getPrimaryKey();
        
        argument.philosophy_school = entity.getFieldAsString("philosophy_school").value_or("");
        
        auto arg_type_str = entity.getFieldAsString("argument_type").value_or("pro");
        argument.argument_type = stringToArgumentType(arg_type_str);
        
        argument.content = entity.getFieldAsString("content").value_or("");
        
        auto strength_str = entity.getFieldAsString("strength").value_or("moderate");
        argument.strength = stringToArgumentStrength(strength_str);
        
        auto created_at_int = entity.getFieldAsInt("created_at").value_or(0);
        argument.created_at = std::chrono::system_clock::from_time_t(created_at_int);
        
        // Parse JSON arrays
        auto principle_json = entity.getFieldAsString("principle_basis");
        if (principle_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*principle_json);
                argument.principle_basis = j.get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse principle_basis JSON: {}", ex.what());
            }
        }
        
        auto counter_json = entity.getFieldAsString("counterarguments");
        if (counter_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*counter_json);
                argument.counterarguments = j.get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse counterarguments JSON: {}", ex.what());
            }
        }
        
        auto supports_json = entity.getFieldAsString("supports");
        if (supports_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*supports_json);
                argument.supports = j.get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse supports JSON: {}", ex.what());
            }
        }
        
        return argument;
    }
    
    /**
     * @brief Make Argument Key.
     * @param[in] argument_id Identifier of the argument.
     * @return Return value.
     * @details Implements makeArgumentKey without additional internal calls.
     */
    static std::string makeArgumentKey(const std::string& argument_id) {
        return "entity:ethics_arguments:" + argument_id;
    }
    
    /**
     * @brief ========== Ethical Decision Conversion ==========
     * @param[in] decision Input parameter.
     * @return Return value.
     * @details Calls: time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().
     */
    
    static BaseEntity toBaseEntity(const EthicalDecision& decision) {
        BaseEntity::FieldMap fields;
        
        fields["dilemma_id"] = decision.dilemma_id;
        fields["decision_text"] = decision.decision_text;
        fields["primary_philosophy"] = decision.primary_philosophy;
        fields["confidence"] = decision.confidence;
        fields["consensus_level"] = decision.consensus_level;
        fields["created_at"] = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                decision.created_at.time_since_epoch()
            ).count()
        );
        
        // Store arrays as JSON
        if (!decision.supporting_philosophies.empty()) {
            nlohmann::json j = decision.supporting_philosophies;
            fields["supporting_philosophies"] = j.dump();
        }
        
        if (!decision.argument_chain_ids.empty()) {
            nlohmann::json j = decision.argument_chain_ids;
            fields["argument_chain_ids"] = j.dump();
        }
        
        return BaseEntity::fromFields(decision.decision_id, fields);
    }
    
    /**
     * @brief From Base Entity.
     * @param[in] entity Input parameter.
     * @param[in] is_decision Input parameter.
     * @return Return value.
     * @details Calls: getPrimaryKey(), getFieldAsString(), value_or(), getFieldAsDouble(), getFieldAsInt(), std::chrono::system_clock::from_time_t(), nlohmann::json::parse(), THEMIS_WARN().
     */
    static EthicalDecision fromBaseEntity(const BaseEntity& entity, bool is_decision) {
        EthicalDecision decision;
        decision.decision_id = entity.getPrimaryKey();
        
        decision.dilemma_id = entity.getFieldAsString("dilemma_id").value_or("");
        decision.decision_text = entity.getFieldAsString("decision_text").value_or("");
        decision.primary_philosophy = entity.getFieldAsString("primary_philosophy").value_or("");
        decision.confidence = entity.getFieldAsDouble("confidence").value_or(0.0);
        decision.consensus_level = entity.getFieldAsDouble("consensus_level").value_or(0.0);
        
        auto created_at_int = entity.getFieldAsInt("created_at").value_or(0);
        decision.created_at = std::chrono::system_clock::from_time_t(created_at_int);
        
        // Parse JSON arrays
        auto supporting_json = entity.getFieldAsString("supporting_philosophies");
        if (supporting_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*supporting_json);
                decision.supporting_philosophies = j.get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse supporting_philosophies JSON: {}", ex.what());
            }
        }
        
        auto chain_json = entity.getFieldAsString("argument_chain_ids");
        if (chain_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*chain_json);
                decision.argument_chain_ids = j.get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse argument_chain_ids JSON: {}", ex.what());
            }
        }
        
        return decision;
    }
    
    /**
     * @brief Make Decision Key.
     * @param[in] decision_id Identifier of the decision.
     * @return Return value.
     * @details Implements makeDecisionKey without additional internal calls.
     */
    static std::string makeDecisionKey(const std::string& decision_id) {
        return "entity:ethics_decisions:" + decision_id;
    }
    
    /**
     * @brief ========== Philosophy Profile Conversion ==========
     * @param[in] profile Input parameter.
     * @return Return value.
     * @details Calls: dump(), BaseEntity::fromFields().
     */
    
    static BaseEntity toBaseEntity(const PhilosophyProfile& profile) {
        BaseEntity::FieldMap fields;
        
        fields["school_id"] = profile.school_id;
        fields["name"] = profile.name;
        
        // Serialize vector<string> fields as JSON
        {
            nlohmann::json j = profile.main_theses;
            fields["main_theses"] = j.dump();
        }
        {
            nlohmann::json j = profile.secondary_theses;
            fields["secondary_theses"] = j.dump();
        }
        {
            nlohmann::json j = profile.strengths;
            fields["strengths"] = j.dump();
        }
        {
            nlohmann::json j = profile.weaknesses;
            fields["weaknesses"] = j.dump();
        }
        
        // Serialize map<string,string> fields as JSON
        {
            nlohmann::json j = profile.decision_framework;
            fields["decision_framework"] = j.dump();
        }
        {
            nlohmann::json j = profile.internal_debate;
            fields["internal_debate"] = j.dump();
        }
        {
            nlohmann::json j = profile.philosophical_positioning;
            fields["philosophical_positioning"] = j.dump();
        }
        
        return BaseEntity::fromFields(profile.school_id, fields);
    }
    
    /**
     * @brief From Base Entity To Profile.
     * @param[in] entity Input parameter.
     * @return Return value.
     * @details Calls: getPrimaryKey(), getFieldAsString(), value_or(), nlohmann::json::parse(), THEMIS_WARN(), what(), parse_string_vec(), parse_string_map().
     */
    static PhilosophyProfile fromBaseEntityToProfile(const BaseEntity& entity) {
        PhilosophyProfile profile;
        profile.school_id = entity.getPrimaryKey();
        profile.name = entity.getFieldAsString("name").value_or("");
        
        // Parse JSON arrays
        auto parse_string_vec = [&](const char* field) -> std::vector<std::string> {
            auto s = entity.getFieldAsString(field);
            if (!s) return {};
            try {
                return nlohmann::json::parse(*s).get<std::vector<std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse JSON array field '{}': {}", field, ex.what());
                return {};
            }
        };
        auto parse_string_map = [&](const char* field) -> std::map<std::string, std::string> {
            auto s = entity.getFieldAsString(field);
            if (!s) return {};
            try {
                return nlohmann::json::parse(*s).get<std::map<std::string, std::string>>();
            } catch (const nlohmann::json::exception& ex) {
                THEMIS_WARN("Failed to parse JSON map field '{}': {}", field, ex.what());
                return {};
            }
        };
        
        profile.main_theses = parse_string_vec("main_theses");
        profile.secondary_theses = parse_string_vec("secondary_theses");
        profile.strengths = parse_string_vec("strengths");
        profile.weaknesses = parse_string_vec("weaknesses");
        profile.decision_framework = parse_string_map("decision_framework");
        profile.internal_debate = parse_string_map("internal_debate");
        profile.philosophical_positioning = parse_string_map("philosophical_positioning");
        
        return profile;
    }
    
    /**
     * @brief Make Profile Key.
     * @param[in] school Input parameter.
     * @return Return value.
     * @details Implements makeProfileKey without additional internal calls.
     */
    static std::string makeProfileKey(const std::string& school) {
        return "entity:ethics_profiles:" + school;
    }
    
    /**
     * @brief ========== Debate Initialization Conversion ==========
     * @param[in] debate Input parameter.
     * @return Return value.
     * @details Calls: time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().
     */
    
    static BaseEntity toBaseEntity(const DebateInitialization& debate) {
        BaseEntity::FieldMap fields;
        
        fields["debate_id"] = debate.debate_id;
        fields["dilemma_description"] = debate.dilemma_description;
        fields["category"] = debate.category;
        fields["created_at"] = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                debate.created_at.time_since_epoch()
            ).count()
        );
        
        // Store arrays as JSON
        if (!debate.philosophy_schools.empty()) {
            nlohmann::json j = debate.philosophy_schools;
            fields["philosophy_schools"] = j.dump();
        }
        
        return BaseEntity::fromFields(debate.debate_id, fields);
    }
    
    /**
     * @brief Make Debate Key.
     * @param[in] debate_id Identifier of the debate.
     * @return Return value.
     * @details Implements makeDebateKey without additional internal calls.
     */
    static std::string makeDebateKey(const std::string& debate_id) {
        return "entity:ethics_debates:" + debate_id;
    }
};

} // namespace ethics
} // namespace plugins
} // namespace themis
