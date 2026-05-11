/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            project_bundle.h                                   ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// Project import/export ZIP bundle interface
#include <string>
#include <vector>
#include &lt;map&gt;

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
