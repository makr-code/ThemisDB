/**
 * @file import_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "importers/importer_interfaces.h"
#include "importers/s3_importer.h"

#include <memory>
#include <string>
#include <mutex>

// Forward-declare httplib types so <httplib.h> stays out of this header.
namespace httplib {
struct Request;
struct Response;
class  Server;
}

namespace themis {
namespace server {

class ImportApiHandler {
public:
    explicit ImportApiHandler(
        std::shared_ptr<importers::ImportJobRegistry> registry,
        std::shared_ptr<importers::IImporter>        importer,
        std::shared_ptr<importers::IImporter>        s3_importer = nullptr
    );

    ~ImportApiHandler() = default;

    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);

private:
    // Route handlers
    /**
     * @brief Handle Start Import.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleStartImport      (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Start My SQLImport.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleStartMySQLImport (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Start S3 Import.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleStartS3Import    (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Job Status.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleJobStatus        (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Cancel Job.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleCancelJob        (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle List Jobs.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleListJobs         (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Metrics.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMetrics          (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Import Wizard.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleImportWizard     (const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Schema.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetSchema        (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Validate Schema.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleValidateSchema   (const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Update Relationships.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleUpdateRelationships(const httplib::Request& req, httplib::Response& res);

    // Helpers
    /**
     * @brief Parse Request Body.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    static nlohmann::json parseRequestBody(const std::string& body);
    /**
     * @brief Options From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static importers::ImportOptions optionsFromJson(const nlohmann::json& j);
    /**
     * @brief Json Ok.
     * @param[in,out] res Input/output parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    static httplib::Response& jsonOk(httplib::Response& res, const nlohmann::json& body);
    /**
     * @brief Json Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    static httplib::Response& jsonError(httplib::Response& res, int status,
                                        const std::string& message);

    std::shared_ptr<importers::ImportJobRegistry> registry_;
    std::shared_ptr<importers::IImporter>         importer_;
    std::shared_ptr<importers::IImporter>         s3_importer_;

    // v2.0: per-job custom relationship override storage
    // key = job_id, value = JSON array of RelationshipMapping objects
    std::mutex                                    rel_mutex_;
    std::map<std::string, nlohmann::json>         relationship_overrides_;

    // ─── Schema inspection bridges (stub #294) ───────────────────────────────

    using SchemaInspectorFn = std::function<nlohmann::json(const std::string& source_path)>;

    using SchemaValidatorFn = std::function<nlohmann::json(const std::string& source_path,
                                                            const nlohmann::json& overrides)>;

    /**
     * @brief Set Schema Inspector Fn.
     * @param[in] fn Input parameter.
     */
    void setSchemaInspectorFn(SchemaInspectorFn fn);

    /**
     * @brief Clear Schema Inspector Fn.
     */
    void clearSchemaInspectorFn();

    /**
     * @brief Set Schema Validator Fn.
     * @param[in] fn Input parameter.
     */
    void setSchemaValidatorFn(SchemaValidatorFn fn);

    /**
     * @brief Clear Schema Validator Fn.
     */
    void clearSchemaValidatorFn();

    SchemaInspectorFn schemaInspectorFn_;
    SchemaValidatorFn schemaValidatorFn_;
};

} // namespace server
} // namespace themis
