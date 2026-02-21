/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_manager.h                                   ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <tbb/concurrent_hash_map.h> // v1.1.0: TBB Concurrent Hash Map
#include <nlohmann/json.hpp>
// Forward declaration
namespace rocksdb { class ColumnFamilyHandle; }

namespace themis {
class RocksDBWrapper;
class SchemaManager;

namespace prompt_engineering {

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

    // In-memory only manager
    PromptManager();

    // RocksDB-backed manager (does not take ownership of db or cf)
    PromptManager(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);
    ~PromptManager() = default;

    // Create a template; if template.id empty one is generated
    PromptTemplate createTemplate(PromptTemplate t);

    // Retrieve template by id
    std::optional<PromptTemplate> getTemplate(const std::string& id) const;

    // List all templates
    std::vector<PromptTemplate> listTemplates() const;

    // Update metadata/active flag of template; returns false if not found
    bool updateTemplate(const std::string& id, const nlohmann::json& metadata, bool active);

    // Assign an experiment id to a template (stores in metadata["experiment_id"])
    bool assignExperiment(const std::string& id, const std::string& experiment_id);

    // Validation result for a prompt template
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;   ///< List of validation errors
        std::vector<std::string> warnings; ///< Non-fatal warnings
    };

    // Validate a template; returns ValidationResult with detailed error reporting
    static ValidationResult validateTemplate(const PromptTemplate& t);

    // Load prompts from YAML configuration file
    // Returns number of prompts loaded successfully
    size_t loadFromYAML(const std::string& yaml_path);

    // Inject context variables into a prompt template
    // Replaces {variable} with values from context map
    // Example: "{version}" -> "1.5.0", "{table_count}" -> "5"
    std::string injectContext(const std::string& template_str, 
                             const std::unordered_map<std::string, std::string>& context) const;

    // Get a prompt with context injection
    // Retrieves template by id and injects context variables
    std::optional<std::string> getPromptWithContext(
        const std::string& id,
        const std::unordered_map<std::string, std::string>& context) const;

    // Build context map from SchemaManager
    // Creates standard context variables: {version}, {table_count}, {schema}, etc.
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

} // namespace prompt_engineering
} // namespace themis
