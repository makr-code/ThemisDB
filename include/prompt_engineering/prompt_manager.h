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

namespace prompt_engineering {

/** @brief Prompt manager component. */
class PromptManager {
public:
    /// @brief Describes a single image attached to a multi-modal prompt.
    struct ImageDescription {
        std::string url;          ///< Optional URL or base64 data URI for the image
        std::string alt_text;     ///< Short descriptive text (required for multi-modal prompts)
        std::string description;  ///< Optional longer human-readable description (text fallback)
        std::string mime_type;    ///< MIME type, e.g. "image/jpeg" (defaults to "image/jpeg")

        nlohmann::json toJson() const {
            nlohmann::json j;
            j["url"]         = url;
            j["alt_text"]    = alt_text;
            j["description"] = description;
            j["mime_type"]   = mime_type.empty() ? "image/jpeg" : mime_type;
            return j;
        }

        static ImageDescription fromJson(const nlohmann::json& j) {
            ImageDescription img;
            img.url         = j.value("url", "");
            img.alt_text    = j.value("alt_text", "");
            img.description = j.value("description", "");
            img.mime_type   = j.value("mime_type", "image/jpeg");
            return img;
        }
    };

    struct PromptTemplate {
        std::string id;           // generated id
        std::string name;         // human readable name
        std::string version;      // version string, e.g. "v1", "2.3"
        std::string content;      // template body
        std::string description;  // description of the prompt
        nlohmann::json metadata;  // arbitrary metadata (experiment flags etc.)
        bool active = true;
        std::vector<ImageDescription> images; // optional multi-modal image descriptions

        nlohmann::json toJson() const {
            nlohmann::json j;
            j["id"] = id;
            j["name"] = name;
            j["version"] = version;
            j["content"] = content;
            j["description"] = description;
            j["metadata"] = metadata;
            j["active"] = active;
            nlohmann::json imgs = nlohmann::json::array();
            for (const auto& img : images) {
                imgs.push_back(img.toJson());
            }
            j["images"] = imgs;
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

    // Build a multi-modal prompt string from a template.
    // Injects context variables into the text content and appends a structured
    // image-description block when the template contains ImageDescription entries.
    // The returned string is suitable for dispatch to a multi-modal LLM.
    static std::string buildMultiModalPrompt(
        const PromptTemplate& t,
        const std::unordered_map<std::string, std::string>& context = {});

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
