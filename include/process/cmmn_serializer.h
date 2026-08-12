/**
 * @file cmmn_serializer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    cmmn_serializer.h
 * Module:  include/process/
 * Purpose: CMMN 1.1 (Case Management Model and Notation) import and export.
 *
 * Converts CMMN 1.1 XML case definitions to ThemisDB ProcessNodeInfo /
 * ProcessEdgeInfo objects so that case management models can be stored and
 * retrieved alongside BPMN and EPK process models.
 */

#pragma once

#include "index/process_graph.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief CMMN 1.1 serializer — import and limited export between CMMN XML
 *        and the ThemisDB internal process graph representation.
 *
 * ## CMMN 1.1 element coverage
 *
 * | Element               | Import | Export |
 * |-----------------------|--------|--------|
 * | casePlanModel         | ✅     | ✅     |
 * | humanTask             | ✅     | ✅     |
 * | processTask           | ✅     | ✅     |
 * | caseTask              | ✅     | ✅     |
 * | stage                 | ✅     | ✅     |
 * | milestone             | ✅     | ✅     |
 * | sentry / onPart       | ✅ (→ SEQUENCE_FLOW edge) | ❌ |
 * | entryCriterion / exitCriterion | ✅ (→ edge target) | ❌ |
 *
 * ## Mapping to ProcessNodeInfo
 *
 * | CMMN element    | node_type                      | subtype        |
 * |-----------------|--------------------------------|----------------|
 * | casePlanModel   | BPMNNodeType::SUBPROCESS       | "CASE_PLAN"    |
 * | humanTask       | BPMNNodeType::TASK             | "HUMAN_TASK"   |
 * | processTask     | BPMNNodeType::TASK             | "PROCESS_TASK" |
 * | caseTask        | BPMNNodeType::TASK             | "CASE_TASK"    |
 * | stage           | BPMNNodeType::SUBPROCESS       | "STAGE"        |
 * | milestone       | BPMNNodeType::INTERMEDIATE_EVENT | "MILESTONE"  |
 *
 * Sentry `<onPart sourceRef="X"/>` inside an `<entryCriterion sentryRef="s"/>`
 * or `<exitCriterion sentryRef="s"/>` on element Y creates a SEQUENCE_FLOW
 * edge from X to Y.
 */
class CmmnSerializer {
public:
    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    struct ImportResult {
        bool ok{false};
        std::string message;
        std::string case_id;      ///< Extracted from <case id=…>
        std::string case_name;
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
    };

    /**
     * @brief Parse a CMMN 1.1 XML string into ProcessNodeInfo / ProcessEdgeInfo.
     *
     * @param cmmn_xml  Full CMMN 1.1 XML document.
     * @return ImportResult with nodes and edges on success.
     */
    static ImportResult importXml(std::string_view cmmn_xml);

    /**
     * @brief Import a CMMN 1.1 XML file from disk.
     */
    static ImportResult importFile(std::string_view file_path);

    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Export nodes and edges to a minimal CMMN 1.1 XML document.
     *
     * Only nodes with CMMN-compatible subtypes (HUMAN_TASK, PROCESS_TASK,
     * CASE_TASK, STAGE, MILESTONE, CASE_PLAN) are exported.  EPK nodes and
     * pure BPMN gateways are skipped.
     *
     * @param case_id    The CMMN <case id=…> attribute value.
     * @param case_name  The CMMN <case name=…> attribute value.
     * @param nodes      Process nodes to export.
     * @param edges      Edges (used to emit sentry/onPart where applicable).
     * @return Well-formed CMMN 1.1 XML string.
     */
    static std::string exportXml(
        std::string_view                    case_id,
        std::string_view                    case_name,
        const std::vector<ProcessNodeInfo>& nodes,
        const std::vector<ProcessEdgeInfo>& edges);

private:
    static std::string escapeXml_(std::string_view s);
};

} // namespace process
} // namespace themis

