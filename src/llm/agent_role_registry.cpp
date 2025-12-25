#include "llm/agent_role_registry.h"
#include <rocksdb/db.h>
#include <rocksdb/transaction_db.h>

namespace themis {
namespace llm {

// RoleDefinition methods
nlohmann::json AgentRoleRegistry::RoleDefinition::toJson() const {
    return nlohmann::json{
        {"role_id", role_id},
        {"role_name", role_name},
        {"description", description},
        {"capabilities", capabilities},
        {"lora_adapter_id", lora_adapter_id},
        {"lora_adapter_path", lora_adapter_path},
        {"system_prompt_template", system_prompt_template},
        {"default_parameters", default_parameters},
        {"metadata", metadata}
    };
}

AgentRoleRegistry::RoleDefinition AgentRoleRegistry::RoleDefinition::fromJson(const nlohmann::json& j) {
    RoleDefinition role;
    role.role_id = j.value("role_id", "");
    role.role_name = j.value("role_name", "");
    role.description = j.value("description", "");
    role.capabilities = j.value("capabilities", std::vector<std::string>{});
    role.lora_adapter_id = j.value("lora_adapter_id", "");
    role.lora_adapter_path = j.value("lora_adapter_path", "");
    role.system_prompt_template = j.value("system_prompt_template", "");
    role.default_parameters = j.value("default_parameters", nlohmann::json::object());
    role.metadata = j.value("metadata", nlohmann::json::object());
    return role;
}

// Constructor
AgentRoleRegistry::AgentRoleRegistry(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf
) : db_(db), cf_(cf) {
    // Initialize cache
    rebuildCache();
}

// Register role
bool AgentRoleRegistry::registerRole(const RoleDefinition& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store in cache
    role_cache_[role.role_id] = role;
    
    // Persist to DB
    return persistRole(role);
}

// Get role
std::optional<AgentRoleRegistry::RoleDefinition> AgentRoleRegistry::getRole(
    const std::string& role_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cache first
    auto it = role_cache_.find(role_id);
    if (it != role_cache_.end()) {
        return it->second;
    }
    
    // Load from DB
    return loadRole(role_id);
}

// Find roles for task
std::vector<AgentRoleRegistry::RoleDefinition> AgentRoleRegistry::findRolesForTask(
    const std::string& task_description
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RoleDefinition> matching_roles;
    
    // Simple keyword matching (in production, use NLP/embeddings)
    std::string task_lower = task_description;
    std::transform(task_lower.begin(), task_lower.end(), task_lower.begin(), ::tolower);
    
    for (const auto& [role_id, role] : role_cache_) {
        // Check if any capability matches task
        for (const auto& capability : role.capabilities) {
            std::string cap_lower = capability;
            std::transform(cap_lower.begin(), cap_lower.end(), cap_lower.begin(), ::tolower);
            
            if (task_lower.find(cap_lower) != std::string::npos) {
                matching_roles.push_back(role);
                break;
            }
        }
        
        // Also check description
        std::string desc_lower = role.description;
        std::transform(desc_lower.begin(), desc_lower.end(), desc_lower.begin(), ::tolower);
        
        if (task_lower.find(desc_lower.substr(0, 20)) != std::string::npos) {
            matching_roles.push_back(role);
        }
    }
    
    return matching_roles;
}

// List all roles
std::vector<std::string> AgentRoleRegistry::listAllRoles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> role_ids;
    for (const auto& [role_id, _] : role_cache_) {
        role_ids.push_back(role_id);
    }
    return role_ids;
}

// Update role
bool AgentRoleRegistry::updateRole(
    const std::string& role_id,
    const RoleDefinition& role
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update cache
    role_cache_[role_id] = role;
    
    // Persist to DB
    return persistRole(role);
}

// Delete role
bool AgentRoleRegistry::deleteRole(const std::string& role_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from cache
    role_cache_.erase(role_id);
    
    // Delete from DB
    std::string key = makeKey(role_id);
    rocksdb::WriteOptions write_options;
    auto status = db_->Delete(write_options, cf_, key);
    
    return status.ok();
}

// Get roles by capability
std::vector<AgentRoleRegistry::RoleDefinition> AgentRoleRegistry::getRolesByCapability(
    const std::string& capability
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RoleDefinition> matching_roles;
    
    for (const auto& [role_id, role] : role_cache_) {
        for (const auto& cap : role.capabilities) {
            if (cap == capability) {
                matching_roles.push_back(role);
                break;
            }
        }
    }
    
    return matching_roles;
}

// Load default roles
size_t AgentRoleRegistry::loadDefaultRoles() {
    auto builtin_roles = getBuiltinRoles();
    
    for (const auto& role : builtin_roles) {
        registerRole(role);
    }
    
    return builtin_roles.size();
}

// Private helper methods
std::string AgentRoleRegistry::makeKey(const std::string& role_id) const {
    return "agent_role:" + role_id;
}

bool AgentRoleRegistry::persistRole(const RoleDefinition& role) {
    std::string key = makeKey(role.role_id);
    std::string value = role.toJson().dump();
    
    rocksdb::WriteOptions write_options;
    auto status = db_->Put(write_options, cf_, key, value);
    
    return status.ok();
}

std::optional<AgentRoleRegistry::RoleDefinition> AgentRoleRegistry::loadRole(
    const std::string& role_id
) const {
    std::string key = makeKey(role_id);
    std::string value;
    
    rocksdb::ReadOptions read_options;
    auto status = db_->Get(read_options, cf_, key, &value);
    
    if (status.ok()) {
        try {
            auto json = nlohmann::json::parse(value);
            return RoleDefinition::fromJson(json);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    return std::nullopt;
}

void AgentRoleRegistry::rebuildCache() {
    role_cache_.clear();
    
    // Iterate through all agent_role:* keys
    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options, cf_));
    
    std::string prefix = "agent_role:";
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        try {
            auto json = nlohmann::json::parse(it->value().ToString());
            auto role = RoleDefinition::fromJson(json);
            role_cache_[role.role_id] = role;
        } catch (...) {
            // Skip invalid entries
        }
    }
    
    // Check iterator status
    if (!it->status().ok()) {
        // Log error but don't throw - allow partial cache rebuild
    }
}

// Built-in roles
std::vector<AgentRoleRegistry::RoleDefinition> AgentRoleRegistry::getBuiltinRoles() {
    return {
        // Legal Expert
        {
            "legal_expert",
            "Legal Expert",
            "Expert in contract law and legal compliance",
            {"contract_analysis", "legal_risk", "compliance"},
            "legal_contracts_v2",
            "",
            "You are a legal expert specializing in contract law. "
            "Analyze contracts for legal risks, compliance issues, and liability clauses.",
            nlohmann::json{{"temperature", 0.7}, {"max_tokens", 2048}},
            nlohmann::json::object()
        },
        
        // Technical Analyst
        {
            "technical_analyst",
            "Technical Analyst",
            "Expert in technical architecture and code review",
            {"code_review", "architecture", "performance"},
            "tech_analysis_v1",
            "",
            "You are a technical analyst with expertise in software architecture. "
            "Review code for technical quality, performance, and maintainability.",
            nlohmann::json{{"temperature", 0.6}, {"max_tokens", 2048}},
            nlohmann::json::object()
        },
        
        // Business Strategist
        {
            "business_strategist",
            "Business Strategist",
            "Expert in business strategy and financial analysis",
            {"business_analysis", "financial", "strategic"},
            "business_analysis_v1",
            "",
            "You are a business strategist with expertise in commercial analysis. "
            "Evaluate business implications, ROI, and strategic alignment.",
            nlohmann::json{{"temperature", 0.7}, {"max_tokens", 2048}},
            nlohmann::json::object()
        },
        
        // Security Analyst
        {
            "security_analyst",
            "Security Analyst",
            "Expert in cybersecurity and secure coding",
            {"security", "vulnerability", "code_security"},
            "security_patterns_v3",
            "",
            "You are a security expert specializing in application security. "
            "Identify vulnerabilities, security risks, and recommend mitigations.",
            nlohmann::json{{"temperature", 0.5}, {"max_tokens", 2048}},
            nlohmann::json::object()
        },
        
        // General Analyst
        {
            "general_analyst",
            "General Analyst",
            "General-purpose analytical agent",
            {"general_analysis"},
            "",
            "",
            "You are a general-purpose analyst. "
            "Provide comprehensive analysis of the given task.",
            nlohmann::json{{"temperature", 0.7}, {"max_tokens", 2048}},
            nlohmann::json::object()
        }
    };
}

} // namespace llm
} // namespace themis
