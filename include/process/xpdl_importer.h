/**
 * @file xpdl_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/** @brief I xpdl importer component. */
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
