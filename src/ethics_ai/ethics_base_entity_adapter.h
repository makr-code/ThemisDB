/**
 * @file ethics_base_entity_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Adapter between Ethics AI types and ThemisDB BaseEntity
 * 
 * Converts between our domain types (EthicalArgument, EthicalDecision, etc.)
 * and ThemisDB's unified BaseEntity storage format.
 * 
 * No duplicate storage structures - directly uses ThemisDB's BaseEntity.
 */
class EthicsBaseEntityAdapter {
public:
    // ========== Ethical Argument Conversion ==========
    
    /**
     * @brief Convert EthicalArgument to BaseEntity
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
     * @brief Convert BaseEntity to EthicalArgument
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
            } catch (...) {}
        }
        
        auto counter_json = entity.getFieldAsString("counterarguments");
        if (counter_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*counter_json);
                argument.counterarguments = j.get<std::vector<std::string>>();
            } catch (...) {}
        }
        
        auto supports_json = entity.getFieldAsString("supports");
        if (supports_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*supports_json);
                argument.supports = j.get<std::vector<std::string>>();
            } catch (...) {}
        }
        
        return argument;
    }
    
    /**
     * @brief Get collection/table key for argument
     */
    static std::string makeArgumentKey(const std::string& argument_id) {
        return "entity:ethics_arguments:" + argument_id;
    }
    
    // ========== Ethical Decision Conversion ==========
    
    /**
     * @brief Convert EthicalDecision to BaseEntity
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
     * @brief Convert BaseEntity to EthicalDecision
     */
    static EthicalDecision fromBaseEntity(const BaseEntity& entity, [[maybe_unused]] bool is_decision) {
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
            } catch (...) {}
        }
        
        auto chain_json = entity.getFieldAsString("argument_chain_ids");
        if (chain_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*chain_json);
                decision.argument_chain_ids = j.get<std::vector<std::string>>();
            } catch (...) {}
        }
        
        return decision;
    }
    
    /**
     * @brief Get collection/table key for decision
     */
    static std::string makeDecisionKey(const std::string& decision_id) {
        return "entity:ethics_decisions:" + decision_id;
    }
    
    // ========== Philosophy Profile Conversion ==========
    
    /**
     * @brief Convert PhilosophyProfile to BaseEntity
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
     * @brief Convert BaseEntity to PhilosophyProfile
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
            } catch (...) { return {}; }
        };
        auto parse_string_map = [&](const char* field) -> std::map<std::string, std::string> {
            auto s = entity.getFieldAsString(field);
            if (!s) return {};
            try {
                return nlohmann::json::parse(*s).get<std::map<std::string, std::string>>();
            } catch (...) { return {}; }
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
     * @brief Get collection/table key for philosophy profile
     */
    static std::string makeProfileKey(const std::string& school) {
        return "entity:ethics_profiles:" + school;
    }
    
    // ========== Debate Initialization Conversion ==========
    
    /**
     * @brief Convert DebateInitialization to BaseEntity
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
     * @brief Get collection/table key for debate
     */
    static std::string makeDebateKey(const std::string& debate_id) {
        return "entity:ethics_debates:" + debate_id;
    }
};

} // namespace ethics
} // namespace plugins
} // namespace themis
