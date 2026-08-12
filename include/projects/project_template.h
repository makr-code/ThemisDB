/**
 * @file project_template.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "projects/DocumentManager/document_manager.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace projects {

using json = nlohmann::json;

// ─── Built-in templates ───────────────────────────────────────────────────────

/**
 * @brief Enumeration of pre-defined project templates.
 *
 * Templates define a curated set of object types (document collections,
 * table schemas, index configurations) that are created during
 * instantiation.  CUSTOM is not a built-in — it is used as a sentinel
 * when a caller provides an explicit @c template_def JSON document.
 */
enum class BuiltinTemplate {
    EMPTY,           ///< Blank project — no pre-created objects
    WEB_APPLICATION, ///< Web-app backend: users, sessions, posts
    MACHINE_LEARNING,///< ML pipeline: datasets, features, experiments
    ANALYTICS,       ///< Analytics warehouse: facts, dimensions
    TIME_SERIES,     ///< Time-series: sensors, metrics, alerts
    GRAPH_ANALYTICS, ///< Graph database: nodes, edges
    DOCUMENT_STORE,  ///< Document-oriented: collections with schemas
};

/// Human-readable name for a BuiltinTemplate value.
const char* builtinTemplateToString(BuiltinTemplate tmpl) noexcept;

// ─── Options ─────────────────────────────────────────────────────────────────

/**
 * @brief Configuration options for template instantiation.
 */
struct TemplateOptions {
    std::string project_name;       ///< Name for the new project (required)
    std::string description;        ///< Optional project description
    bool        include_sample_data = false; ///< Pre-populate with sample data
    json        extra_config        = json::object(); ///< Template-specific overrides
};

// ─── Result ──────────────────────────────────────────────────────────────────

/**
 * @brief Result of a template instantiation operation.
 */
struct TemplateInstantiationResult {
    bool                     ok = false;  ///< True on success
    std::string              project_id;  ///< UUID of the created project
    std::vector<std::string> objects_created; ///< Names of objects created (e.g. collection names)
    std::string              message;         ///< Status or error message
};

// ─── ProjectTemplate ─────────────────────────────────────────────────────────

/**
 * @brief Factory for creating projects from built-in or custom templates.
 *
 * Template instantiation is atomic with respect to object creation: if
 * schema validation or any object-creation step fails, all previously
 * created objects are rolled back before the error is returned.
 *
 * Template schema validation is always performed before any objects are
 * written to storage.  Instantiation of an invalid template definition
 * returns a @c TemplateInstantiationResult with @c ok=false and a
 * descriptive error message — no partial state is written.
 *
 * Thread-safety: all methods are thread-safe.
 */
class ProjectTemplate {
public:
    explicit ProjectTemplate(std::shared_ptr<RocksDBWrapper> storage);
    ~ProjectTemplate() = default;

    /**
     * @brief Instantiate a project from a built-in template.
     *
     * @param tmpl     The built-in template to use.
     * @param options  Instantiation options (must include a project_name).
     * @return TemplateInstantiationResult describing success or failure.
     */
    TemplateInstantiationResult instantiate(
        BuiltinTemplate          tmpl,
        const TemplateOptions&   options
    );

    /**
     * @brief Instantiate a project from a caller-supplied JSON template definition.
     *
     * The definition is validated against the template schema before any
     * objects are written.
     *
     * @param template_def  JSON template definition (see validateTemplateDefinition).
     * @param options       Instantiation options.
     * @return TemplateInstantiationResult.
     */
    TemplateInstantiationResult instantiateFromDefinition(
        const json&            template_def,
        const TemplateOptions& options
    );

    /**
     * @brief List the names of all available built-in templates.
     */
    static std::vector<std::string> listBuiltinTemplates();

    /**
     * @brief Validate a custom template definition.
     *
     * Required top-level fields: "name" (string), "objects" (array).
     * Each object entry must have: "type" (string), "name" (string).
     *
     * @return Status{true} if the definition is valid, Status{false, reason} otherwise.
     */
    static Status validateTemplateDefinition(const json& template_def);

private:
    std::shared_ptr<RocksDBWrapper> storage_;

    /// Return the JSON schema for a built-in template (object definitions).
    json getBuiltinTemplateSchema(BuiltinTemplate tmpl) const;

    /// Generate a new UUID string.
    std::string generateUuid() const;

    /// Write a single object entry to storage; return object name on success.
    std::optional<std::string> createObjectFromDefinition(
        const std::string& project_id,
        const json&        object_def,
        bool               include_sample_data
    );
};

} // namespace projects
} // namespace themis
