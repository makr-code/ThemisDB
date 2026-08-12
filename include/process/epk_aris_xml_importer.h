/**
 * @file epk_aris_xml_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    epk_aris_xml_importer.h
 * Module:  include/process/
 * Purpose: EPK import from ARIS Markup Language (AML) XML files.
 *
 * ARIS (Architecture of Integrated Information Systems) is a methodology
 * and toolset by Software AG.  AML is the native export format of the
 * ARIS Designer/Architect suite.
 *
 * ## ARIS EPK TypeNum mapping
 *
 * | ARIS TypeNum | ARIS Name (de)                | EPKNodeType             |
 * |-------------:|-------------------------------|-------------------------|
 * |            1 | Funktion                      | FUNCTION                |
 * |           14 | Ereignis                      | EVENT                   |
 * |           13 | UND-Verknüpfungsoperator      | AND_CONNECTOR           |
 * |           12 | ODER-Verknüpfungsoperator     | OR_CONNECTOR            |
 * |           11 | XOR-Verknüpfungsoperator      | XOR_CONNECTOR           |
 * |           18 | Organisationseinheit          | ORGANIZATIONAL_UNIT     |
 * |           15 | Informationsobjekt            | INFORMATION_OBJECT      |
 * |           40 | Anwendungssystem              | APPLICATION_SYSTEM      |
 * |           16 | Prozesswegweiser              | PROCESS_PATH            |
 *
 * ## Supported AML structure
 *
 * The importer handles the following subset of AML v9 and v10:
 *
 * ```xml
 * <AML>
 *   <Group Group.ID="g-001">
 *     <Model Model.ID="m-001" Model.Type="EPK">
 *       <Model.Name LocaleId="19">Process Name</Model.Name>
 *       <ObjOcc ObjOcc.ID="occ-001" SymbolNum="14" ObjDef.IdRef="obj-001"/>
 *       <ObjOcc ObjOcc.ID="occ-002" SymbolNum="1"  ObjDef.IdRef="obj-002"/>
 *       <CxnOcc CxnOcc.ID="cx-001" CxnDef.IdRef="cd-001"
 *               FromObjOcc.IdRef="occ-001" ToObjOcc.IdRef="occ-002"/>
 *     </Model>
 *     <ObjDef ObjDef.ID="obj-001" TypeNum="14">
 *       <ObjDef.Name>Antrag eingegangen</ObjDef.Name>
 *     </ObjDef>
 *     <ObjDef ObjDef.ID="obj-002" TypeNum="1">
 *       <ObjDef.Name>Vollständigkeit prüfen</ObjDef.Name>
 *     </ObjDef>
 *   </Group>
 * </AML>
 * ```
 *
 * Multiple `<Group>` levels are supported; the first EPK model encountered is
 * imported.  To import all models from a multi-model AML file call
 * `importAllAml()`.
 *
 * ## Thread safety
 *
 * All methods are static; the class has no instance state.
 */

#include "index/process_graph.h"
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief Imports EPK process models from ARIS Markup Language (AML) XML.
 *
 * Handles the `<AML>` / `<Group>` / `<Model>` hierarchy produced by ARIS
 * Designer 9.x and 10.x export.  Uses the same hand-written, regex-free
 * XML tokenizer strategy as `BpmnSerializer`.
 */
class EpkArisXmlImporter {
public:
    // -----------------------------------------------------------------------
    // Result type
    // -----------------------------------------------------------------------

    struct ImportResult {
        bool ok{false};
        std::string message;
        std::string process_id;   ///< Model.ID from the AML file
        std::string process_name; ///< Model.Name text from the AML file
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
    };

    // -----------------------------------------------------------------------
    // Import
    // -----------------------------------------------------------------------

    /**
     * @brief Import the first EPK model found in an AML XML document.
     *
     * Scans `<Group>` hierarchy depth-first for the first `<Model>` whose
     * `Model.Type` attribute equals "EPK" (case-insensitive).
     *
     * @param aml_xml  Full AML XML string.
     * @return ImportResult.  `ok` is true on success; `message` contains an
     *         error description on failure.
     */
    static ImportResult importAml(std::string_view aml_xml);

    /**
     * @brief Import all EPK models from an AML document.
     *
     * Each entry in the returned vector corresponds to one `<Model>` of type
     * EPK in the AML file.
     *
     * @param aml_xml  Full AML XML string.
     * @return Vector of ImportResult (one per EPK model).  Empty on
     *         parse error or when no EPK model is present.
     */
    static std::vector<ImportResult> importAllAml(std::string_view aml_xml);

    // -----------------------------------------------------------------------
    // TypeNum helpers (public for unit testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Map an ARIS TypeNum to the corresponding EPKNodeType.
     *
     * Returns EPKNodeType::FUNCTION for any unrecognised TypeNum.
     */
    static EPKNodeType typeNumToEpkNodeType(int type_num);

    /**
     * @brief Return a human-readable German label for an ARIS TypeNum.
     */
    static std::string_view typeNumToLabel(int type_num);

private:
    /// Maximum AML document size accepted (10 MiB security guard).
    static constexpr size_t kMaxAmlBytes = 10u * 1024u * 1024u;
};

} // namespace process
} // namespace themis
