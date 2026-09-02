/**
 * @file prompt_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Manages prompt templates in-memory with optional RocksDB persistence.
 */

class PromptManager {
public:
    /**
     * @brief Persistent representation of a prompt template.
     */

    struct PromptTemplate {
        std::string id;           // generated id
        std::string name;         // human readable name
        std::string version;      // version string, e.g. "v1", "2.3"
        std::string content;      // template body
        std::string description;  // description of the prompt
        nlohmann::json metadata;  // arbitrary metadata (experiment flags etc.)
        bool active = true;

        /**
         * @brief Serializes this prompt template into JSON.
         * @return JSON object containing all prompt template fields.
         */

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

    /**
     * @brief Constructs an in-memory prompt manager.
     */
    PromptManager();

    /**
     * @brief Constructs a prompt manager backed by RocksDB handles.
     * @param db Non-owning pointer to the RocksDB wrapper.
     * @param cf Non-owning pointer to the column family used for prompt records.
     */
    PromptManager(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);

    /**
     * @brief Destroys the prompt manager.
     */
    ~PromptManager() = default;

    /**
     * @brief Creates a prompt template entry.
     * @param t Template to store; an id is generated when empty.
     * @return Stored prompt template including generated fields.
     */
    PromptTemplate createTemplate(PromptTemplate t);

    /**
     * @brief Retrieves a template by id.
     * @param id Template id to look up.
     * @return Found template or std::nullopt when no template exists for id.
     */
    std::optional<PromptTemplate> getTemplate(const std::string& id) const;

    /**
     * @brief Lists all known templates.
     * @return Snapshot vector of all stored templates.
     */
    std::vector<PromptTemplate> listTemplates() const;

    /**
     * @brief Updates metadata and active flag for an existing template.
     * @param id Template id to update.
     * @param metadata Metadata payload to store.
     * @param active New active flag value.
     * @return true when the template exists and was updated, otherwise false.
     */
    bool updateTemplate(const std::string& id, const nlohmann::json& metadata, bool active);

    /**
     * @brief Assigns an experiment id to a template.
     * @param id Template id to update.
     * @param experiment_id Experiment identifier to store in metadata.
     * @return true when the template exists and was updated, otherwise false.
     */
    bool assignExperiment(const std::string& id, const std::string& experiment_id);

    /**
     * @brief Loads prompt templates from a YAML configuration file.
     * @param yaml_path Path to the YAML file.
     * @return Number of templates loaded successfully.
     */
    size_t loadFromYAML(const std::string& yaml_path);

    /**
     * @brief Injects context variables into a template string.
     * @param template_str Template source text containing {variable} placeholders.
     * @param context Mapping from placeholder key to replacement value.
     * @return Prompt text with placeholder substitutions applied.
     */
    std::string injectContext(const std::string& template_str, 
                             const std::unordered_map<std::string, std::string>& context) const;

    /**
     * @brief Retrieves a template and returns context-injected prompt text.
     * @param id Template id to render.
     * @param context Mapping from placeholder key to replacement value.
     * @return Rendered prompt text or std::nullopt when the template is absent.
     */
    std::optional<std::string> getPromptWithContext(
        const std::string& id,
        const std::unordered_map<std::string, std::string>& context) const;

    /**
     * @brief Builds standard prompt context variables from schema metadata.
     * @param schema_mgr Schema manager used to derive schema-dependent variables.
     * @param edition Product edition label used in context fields.
     * @param version Product version string used in context fields.
     * @return Context map containing canonical keys such as version and schema data.
     */
    static std::unordered_map<std::string, std::string> buildContextFromSchema(
        SchemaManager* schema_mgr,
        const std::string& edition = "Community",
        const std::string& version = "1.5.0");

private:
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
