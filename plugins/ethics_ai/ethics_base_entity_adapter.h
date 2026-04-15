/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_base_entity_adapter.h                       ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:39:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     312                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 7053becfae  2026-02-23  fix(ci): fix 10 error-handling audit violations to bring ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
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
            } catch (...) { THEMIS_WARN("ethics: failed to parse principle_basis json for argument"); }
        }
        
        auto counter_json = entity.getFieldAsString("counterarguments");
        if (counter_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*counter_json);
                argument.counterarguments = j.get<std::vector<std::string>>();
            } catch (...) { THEMIS_WARN("ethics: failed to parse counterarguments json for argument"); }
        }
        
        auto supports_json = entity.getFieldAsString("supports");
        if (supports_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*supports_json);
                argument.supports = j.get<std::vector<std::string>>();
            } catch (...) { THEMIS_WARN("ethics: failed to parse supports json for argument"); }
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
            } catch (...) { THEMIS_WARN("ethics: failed to parse supporting_philosophies json for decision"); }
        }
        
        auto chain_json = entity.getFieldAsString("argument_chain_ids");
        if (chain_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*chain_json);
                decision.argument_chain_ids = j.get<std::vector<std::string>>();
            } catch (...) { THEMIS_WARN("ethics: failed to parse argument_chain_ids json for decision"); }
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
        
        fields["school"] = profile.school;
        fields["name"] = profile.name;
        fields["founder"] = profile.founder;
        fields["historical_context"] = profile.historical_context;
        fields["main_thesis"] = profile.main_thesis;
        fields["secondary_thesis"] = profile.secondary_thesis;
        fields["decision_framework"] = profile.decision_framework;
        fields["strengths"] = profile.strengths;
        fields["weaknesses"] = profile.weaknesses;
        
        // Store principles as JSON
        if (!profile.principles.empty()) {
            nlohmann::json j = profile.principles;
            fields["principles"] = j.dump();
        }
        
        return BaseEntity::fromFields(profile.school, fields);
    }
    
    /**
     * @brief Convert BaseEntity to PhilosophyProfile
     */
    static PhilosophyProfile fromBaseEntityToProfile(const BaseEntity& entity) {
        PhilosophyProfile profile;
        profile.school = entity.getPrimaryKey();
        
        profile.name = entity.getFieldAsString("name").value_or("");
        profile.founder = entity.getFieldAsString("founder").value_or("");
        profile.historical_context = entity.getFieldAsString("historical_context").value_or("");
        profile.main_thesis = entity.getFieldAsString("main_thesis").value_or("");
        profile.secondary_thesis = entity.getFieldAsString("secondary_thesis").value_or("");
        profile.decision_framework = entity.getFieldAsString("decision_framework").value_or("");
        profile.strengths = entity.getFieldAsString("strengths").value_or("");
        profile.weaknesses = entity.getFieldAsString("weaknesses").value_or("");
        
        // Parse JSON array
        auto principles_json = entity.getFieldAsString("principles");
        if (principles_json) {
            try {
                nlohmann::json j = nlohmann::json::parse(*principles_json);
                profile.principles = j.get<std::vector<std::string>>();
            } catch (...) { THEMIS_WARN("ethics: failed to parse principles json for profile"); }
        }
        
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
