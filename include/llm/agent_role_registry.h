#ifndef THEMIS_AGENT_ROLE_REGISTRY_H
#define THEMIS_AGENT_ROLE_REGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace llm {

/**
 * @brief AgentRoleRegistry - Manages agent roles and their configurations
 * 
 * Features:
 * - Role definitions with capabilities
 * - LoRA adapter associations
 * - System prompt templates
 * - Role discovery and matching
 * 
 * Persists role definitions in RocksDB for durability.
 */
class AgentRoleRegistry {
public:
    struct RoleDefinition {
        std::string role_id;
        std::string role_name;
        std::string description;
        std::vector<std::string> capabilities;  // "contract_analysis", "code_review", etc.
        std::string lora_adapter_id;
        std::string lora_adapter_path;
        std::string system_prompt_template;
        nlohmann::json default_parameters;
        nlohmann::json metadata;
        
        nlohmann::json toJson() const;
        static RoleDefinition fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Construct AgentRoleRegistry
     * @param db RocksDB TransactionDB instance
     * @param cf Optional column family handle
     */
    explicit AgentRoleRegistry(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr
    );

    ~AgentRoleRegistry() = default;

    /**
     * @brief Register a new role
     * @param role Role definition
     * @return True if successfully registered
     */
    bool registerRole(const RoleDefinition& role);

    /**
     * @brief Get role by ID
     * @param role_id Role identifier
     * @return Role definition if found
     */
    std::optional<RoleDefinition> getRole(const std::string& role_id) const;

    /**
     * @brief Find roles suitable for task
     * @param task_description Task description
     * @return Vector of matching roles
     */
    std::vector<RoleDefinition> findRolesForTask(const std::string& task_description) const;

    /**
     * @brief List all registered roles
     * @return Vector of role IDs
     */
    std::vector<std::string> listAllRoles() const;

    /**
     * @brief Update role definition
     * @param role_id Role identifier
     * @param role Updated role definition
     * @return True if successfully updated
     */
    bool updateRole(const std::string& role_id, const RoleDefinition& role);

    /**
     * @brief Delete role
     * @param role_id Role identifier
     * @return True if successfully deleted
     */
    bool deleteRole(const std::string& role_id);

    /**
     * @brief Get roles by capability
     * @param capability Capability identifier
     * @return Vector of roles with matching capability
     */
    std::vector<RoleDefinition> getRolesByCapability(const std::string& capability) const;

    /**
     * @brief Load default roles (legal, technical, business, etc.)
     * @return Number of roles loaded
     */
    size_t loadDefaultRoles();

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    
    // Thread safety mutex
    mutable std::mutex mutex_;
    
    // In-memory cache: role_id -> RoleDefinition
    std::map<std::string, RoleDefinition> role_cache_;
    
    // Helper methods
    std::string makeKey(const std::string& role_id) const;
    bool persistRole(const RoleDefinition& role);
    std::optional<RoleDefinition> loadRole(const std::string& role_id) const;
    void rebuildCache();
    
    // Default role definitions
    static std::vector<RoleDefinition> getBuiltinRoles();
};

} // namespace llm
} // namespace themis

#endif // THEMIS_AGENT_ROLE_REGISTRY_H
