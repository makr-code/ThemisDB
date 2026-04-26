/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            xpdl_importer.h                                    ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
// XPDL 2.2 import/export (WFMC standard)
#include <string>
#include <vector>

namespace themis { namespace process {

struct XpdlPackage {
    std::string package_id;
    std::string name;
    std::string version;
    std::string vendor;
    std::vector<std::string> process_ids;
};

struct XpdlImportResult {
    bool success = false;
    std::string package_id;
    int processes_imported = 0;
    int activities_imported = 0;
    int transitions_imported = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class IXpdlImporter {
public:
    virtual ~IXpdlImporter() = default;
    virtual XpdlImportResult importFromFile(const std::string& xpdl_path) = 0;
    virtual XpdlImportResult importFromString(const std::string& xpdl_xml) = 0;
    virtual std::string exportToXpdl(const std::string& package_id) = 0;
    virtual bool validateXpdl(const std::string& xpdl_xml, std::vector<std::string>& errors) = 0;
    virtual std::string xpdlVersion() const { return "2.2"; }
};

}} // namespace themis::process
