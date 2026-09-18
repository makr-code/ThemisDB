/**
 * @file prompt_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include "utils/tbb_compat.h" // Fallback for environments without Intel TBB
#include <nlohmann/json.hpp>
// Forward declaration
namespace rocksdb { class ColumnFamilyHandle; }

namespace themis {
class RocksDBWrapper;
class SchemaManager;


class PromptManager {
public:

    struct PromptTemplate {
        std::string id;           // generated id
        std::string name;         // human readable name
        std::string version;      // version string, e.g. "v1", "2.3"
        std::string content;      // template body
        std::string description;  // description of the prompt
        nlohmann::json metadata;  // arbitrary metadata (experiment flags etc.)
        bool active = true;


        nlohmann::json toJson() const {
            nlohmann::json j;
            j["id"] = id;
            j["name"] = name;
            j["version"] = version;
            j["content"] = content;
            j["description"] = description;
            j["metadata"] = metadata;
            j["active"] = active;
            return j;
        }
    };

    PromptManager();

    PromptManager(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~PromptManager() = default;

    /**
     * @brief Create Template.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    PromptTemplate createTemplate(PromptTemplate t);

    /**
     * @brief Get Template.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<PromptTemplate> getTemplate(const std::string& id) const;

    /**
     * @brief List Templates.
     * @return Return value.
     */
    std::vector<PromptTemplate> listTemplates() const;

    /**
     * @brief Update Template.
     * @param[in] id Input parameter.
     * @param[in] metadata Input parameter.
     * @param[in] active Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateTemplate(const std::string& id, const nlohmann::json& metadata, bool active);

    /**
     * @brief Assign Experiment.
     * @param[in] id Input parameter.
     * @param[in] experiment_id Identifier of the experiment.
     * @return True when the operation succeeds.
     */
    bool assignExperiment(const std::string& id, const std::string& experiment_id);

    /**
     * @brief Load From YAML.
     * @param[in] yaml_path Path to the yaml.
     * @return Return value.
     */
    size_t loadFromYAML(const std::string& yaml_path);

    std::string injectContext(const std::string& template_str, 
                             const std::unordered_map<std::string, std::string>& context) const;

    std::optional<std::string> getPromptWithContext(
        const std::string& id,
        const std::unordered_map<std::string, std::string>& context) const;

    static std::unordered_map<std::string, std::string> buildContextFromSchema(
        SchemaManager* schema_mgr,
        const std::string& edition = "Community",
        const std::string& version = "1.5.0");

private:
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    std::string generateId() const;

    // v1.1.0: Lock-free concurrent hash map (2-3x throughput)
    using StoreType = tbb::concurrent_hash_map<std::string, PromptTemplate>;
    mutable StoreType store_;

    // Optional persistence
    RocksDBWrapper* db_ = nullptr; // not owned
    rocksdb::ColumnFamilyHandle* cf_ = nullptr; // not owned

    static constexpr const char* KEY_PREFIX = "prompt_template:";
};

} // namespace themis
