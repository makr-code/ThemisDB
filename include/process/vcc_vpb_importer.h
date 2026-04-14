/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vcc_vpb_importer.h                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:41:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     155                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f7a272409  2026-03-12  feat(process): add ProcessLinker, ProcessGraphRag, and mo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "process/process_model_manager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief Importer for VCC-VPB (Visual Change Control – Visual Process Builder)
 *        YAML process definitions.
 *
 * ## VCC-VPB Format Overview
 *
 * VCC-VPB produces YAML process definitions that look like:
 *
 * ```yaml
 * id: bauantrag_standard
 * name: "Bauantrag (Standard)"
 * domain: Bauwesen
 * description: "Standard-Bauantragsverfahren nach §34 BauO"
 * compliance: ["§34 BauO", "DSGVO", "VwVfG"]
 *
 * activities:
 *   - id: antragstellung
 *     name: "Antragstellung"
 *     type: start
 *     description: "Eingang des Bauantrags"
 *     sla_hours: 0
 *
 *   - id: vollstaendigkeitspruefung
 *     name: "Vollständigkeitsprüfung"
 *     type: task
 *     description: "Prüfung der Vollständigkeit der Unterlagen"
 *     responsible_role: "Sachbearbeiter"
 *     sla_hours: 48
 *
 * edges:
 *   - from: antragstellung
 *     to: vollstaendigkeitspruefung
 *     type: sequence
 *
 *   - from: vollstaendigkeitspruefung
 *     to: nachforderung
 *     type: conditional
 *     condition: incomplete
 * ```
 *
 * ## Domain mapping
 *
 * | VCC-VPB domain string | ThemisDB ProcessDomain        |
 * |-----------------------|-------------------------------|
 * | Bauwesen              | ADMINISTRATION                |
 * | Beschaffung           | ADMINISTRATION                |
 * | Personal              | ADMINISTRATION                |
 * | Haushalt              | ADMINISTRATION                |
 * | IT                    | IT_SERVICE                    |
 * | Gesundheit            | HEALTHCARE                    |
 * | Finanzen              | FINANCE                       |
 * | Kundenservice         | CUSTOMER_SERVICE              |
 * | (anything else)       | CUSTOM                        |
 */
class VccVpbImporter {
public:
    struct ImportResult {
        bool ok{false};
        std::string message;
        ProcessModelRecord record; ///< Fully populated record ready to save
    };

    /**
     * @brief Parse a VCC-VPB YAML string and produce a ProcessModelRecord.
     *
     * @param yaml_text  Raw YAML content (UTF-8).
     * @param meta       Optional metadata overrides (owner, version, state).
     * @return ImportResult with a fully populated record on success.
     */
    static ImportResult importYaml(
        std::string_view      yaml_text,
        const ProcessModelRecord& meta = {}
    );

    /**
     * @brief Batch-import multiple VCC-VPB models from a single YAML file that
     *        uses a top-level list key (e.g. `administrative_models:`).
     *
     * @param yaml_text     Raw YAML content with a top-level list.
     * @param list_key      The top-level YAML key that holds the array of models.
     * @param meta_defaults Metadata defaults applied to every imported model.
     * @return Vector of ImportResult — one per model in the list.
     */
    static std::vector<ImportResult> importYamlList(
        std::string_view          yaml_text,
        std::string_view          list_key      = "administrative_models",
        const ProcessModelRecord& meta_defaults = {}
    );

    /**
     * @brief Import all *.yaml files from a directory.
     *
     * Recursively discovers YAML files and imports them using the standard
     * VCC-VPB schema.  Errors per file are collected and returned individually.
     *
     * @param directory_path  Filesystem path to the directory.
     * @param meta_defaults   Metadata defaults applied to every imported model.
     */
    static std::vector<ImportResult> importDirectory(
        std::string_view          directory_path,
        const ProcessModelRecord& meta_defaults = {}
    );

private:
    // Internal: convert a single YAML node-map to ProcessModelRecord
    static ImportResult parseModelNode_(
        const nlohmann::json& yaml_as_json,
        const ProcessModelRecord& meta_defaults
    );

    // Internal: map VCC-VPB activity type string to BPMNNodeType
    static BPMNNodeType activityTypeToNodeType_(std::string_view type_str);

    // Internal: map VCC-VPB edge type string to ProcessEdgeType
    static ProcessEdgeType edgeTypeToProcessEdgeType_(std::string_view type_str);

    // Internal: map VCC-VPB domain string to ProcessDomain
    static ProcessDomain domainStringToEnum_(std::string_view domain);
};

} // namespace process
} // namespace themis
