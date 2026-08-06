/**
 * @file bpmn_serializer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: bpmn_serializer.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "index/process_graph.h"
#include "process/process_common.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief BPMN 2.0 serializer — import and export between BPMN XML and the
 *        ThemisDB internal process graph representation.
 *
 * ## BPMN 2.0 coverage
 *
 * | Element class         | Import | Export |
 * |-----------------------|--------|--------|
 * | Events (start/end/intermediate) | ✅ | ✅ |
 * | Tasks (user/service/script/send/receive/manual/business-rule) | ✅ | ✅ |
 * | Sub-processes & call activities | ✅ | ✅ |
 * | Gateways (XOR/AND/OR/event-based/complex) | ✅ | ✅ |
 * | Sequence flows (with conditions) | ✅ | ✅ |
 * | Message flows | ✅ | ✅ |
 * | Pools & lanes | ✅ | ✅ |
 * | Data objects & data stores | ✅ | ✅ |
 * | Annotations & associations | ✅ | ✅ |
 * | BPMNDI layout hints | ✅ (BPMNShape x/y/width/height → node.metadata.layout) | ❌ (not emitted) |
 *
 * ## Compliance
 *
 * The XML output conforms to the BPMN 2.0 schema at
 * http://www.omg.org/spec/BPMN/20100524/MODEL (ISO/IEC 19510:2013).
 *
 * ## Error Handling (Phase 3)
 *
 * All import errors are explicit and actionable:
 * - Empty input → EMPTY_INPUT
 * - Oversized input (>10 MiB) → INPUT_TOO_LARGE
 * - Malformed XML → MALFORMED_INPUT
 * - Missing required elements → MISSING_REQUIRED_ELEMENT
 * - Unsupported BPMN constructs → UNSUPPORTED_ELEMENT
 * - Broken element references → BROKEN_REFERENCE
 * - File I/O errors → FILE_READ_ERROR
 *
 * No silent failures or missing error codes.
 */
class BpmnSerializer {
public:
    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    /**
     * @brief Result of a BPMN import operation.
     *
     * When @c ok is false:
     * - @c error_code is set to a ProcessErrorCode indicating the failure category.
     * - @c message contains an actionable diagnostic message for operator triage.
     * - All other fields are empty/default.
     *
     * When @c ok is true:
     * - @c error_code is ignored (not checked by callers).
     * - @c nodes and @c edges contain the parsed process graph.
     */
    struct ImportResult {
        bool ok{false};
        ProcessErrorCode error_code{ProcessErrorCode::INTERNAL_ERROR};
        std::string message;
        std::string process_id;     ///< Extracted from the BPMN <process id=…>
        std::string process_name;
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
        
        /**
         * @brief Create a successful import result.
         */
        static ImportResult success(
            std::string_view pid,
            std::string_view pname,
            std::vector<ProcessNodeInfo> n,
            std::vector<ProcessEdgeInfo> e
        );
        
        /**
         * @brief Create a failure result with error code and diagnostic message.
         */
        static ImportResult failure(
            ProcessErrorCode code,
            std::string_view context,
            std::string_view detail = ""
        );
    };

    /**
     * @brief Parse a BPMN 2.0 XML string into ProcessNodeInfo / ProcessEdgeInfo
     *        objects that can be registered with ProcessGraphManager.
     *
     * All errors are explicit. There are no silent skipped elements or missing
     * error codes. If unsupported elements are encountered, UNSUPPORTED_ELEMENT
     * is returned with a diagnostic message listing the element name and line number.
     *
     * @param bpmn_xml  Full BPMN 2.0 XML document.
     * @return ImportResult with error_code set on failure.
     */
    static ImportResult importXml(std::string_view bpmn_xml);

    /**
     * @brief Import a BPMN 2.0 XML file from disk.
     *
     * Errors include FILE_READ_ERROR if file cannot be opened, plus all
     * errors from importXml() for the file content.
     *
     * @param file_path Path to BPMN XML file.
     * @return ImportResult with error_code set on failure.
     */
    static ImportResult importFile(std::string_view file_path);

    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Export a list of process nodes and edges to BPMN 2.0 XML.
     *
     * @param process_id    The BPMN <process id=…> attribute value.
     * @param process_name  The BPMN <process name=…> attribute value.
     * @param nodes         Ordered list of process nodes.
     * @param edges         List of connecting edges.
     * @return Well-formed BPMN 2.0 XML string.
     *
     * @throws std::bad_alloc if memory allocation fails.
     */
    static std::string exportXml(
        std::string_view                          process_id,
        std::string_view                          process_name,
        const std::vector<ProcessNodeInfo>&       nodes,
        const std::vector<ProcessEdgeInfo>&       edges
    );

    /**
     * @brief Export from an intermediate normalised JSON graph.
     *
     * Accepts the `normalized` JSON field of a ProcessModelRecord.
     *
     * @param normalized_graph JSON structure with "nodes" and "edges" arrays.
     * @return Well-formed BPMN 2.0 XML string.
     *
     * @throws std::invalid_argument if normalized_graph is malformed.
     * @throws std::bad_alloc if memory allocation fails.
     */
    static std::string exportFromJson(const nlohmann::json& normalized_graph);

    // -------------------------------------------------------------------------
    // Validation and Hardening
    // -------------------------------------------------------------------------

    /**
     * @brief Validate BPMN structure and bounds before processing.
     *
     * Checks:
     * - Node and edge counts within limits
     * - All referenced nodes exist
     * - Deterministic element ordering
     * - No malformed attributes
     *
     * @param nodes The nodes to validate
     * @param edges The edges to validate
     * @return Error message if invalid; empty string if valid
     */
    [[nodiscard]] static std::string validateStructure(
        const std::vector<ProcessNodeInfo>& nodes,
        const std::vector<ProcessEdgeInfo>& edges
    );

private:
    // XML helpers
    static std::string escapeXml_(std::string_view s);
    static std::string nodeTypeToXmlTag_(BPMNNodeType t);
    static BPMNNodeType xmlTagToNodeType_(std::string_view tag);
};

} // namespace process
} // namespace themis

