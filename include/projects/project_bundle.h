/**
 * @file project_bundle.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// Project import/export ZIP bundle interface
#include <string>
#include <vector>
#include <map>

namespace themis { namespace projects {

struct ProjectBundleManifest {
    std::string project_id;
    std::string name;
    std::string version;
    std::string created_at;
    std::vector<std::string> included_collections;
    std::map<std::string, std::string> metadata;
};

struct BundleExportOptions {
    bool include_data = true;
    bool include_schema = true;
    bool include_indexes = true;
    bool include_permissions = false;
    std::string encryption_key;
    std::string compression_level = "fast";
};

struct BundleImportResult {
    bool success = false;
    std::string project_id;
    int collections_imported = 0;
    int documents_imported = 0;
    std::vector<std::string> warnings;
};

class IProjectBundleManager {
public:
    virtual ~IProjectBundleManager() = default;
    [[nodiscard]] virtual bool exportToZip(const std::string& project_id,
                              const std::string& output_path,
                              const BundleExportOptions& options = {}) = 0;
    [[nodiscard]] virtual BundleImportResult importFromZip(const std::string& bundle_path,
                                              const std::string& target_project_id = "") = 0;
    [[nodiscard]] virtual ProjectBundleManifest readManifest(const std::string& bundle_path) = 0;
    [[nodiscard]] virtual bool validateBundle(const std::string& bundle_path, std::vector<std::string>& errors) = 0;
};

}} // namespace themis::projects
