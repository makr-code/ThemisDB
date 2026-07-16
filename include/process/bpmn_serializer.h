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
 */
class BpmnSerializer {
public:
    // -------------------------------------------------------------------------
    // Import
    // -------------------------------------------------------------------------

    struct ImportResult {
        bool ok{false};
        std::string message;
        std::string process_id;     ///< Extracted from the BPMN <process id=…>
        std::string process_name;
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
    };

    /**
     * @brief Parse a BPMN 2.0 XML string into ProcessNodeInfo / ProcessEdgeInfo
     *        objects that can be registered with ProcessGraphManager.
     *
     * The parser is lenient: unknown elements are silently skipped so that
     * BPMN files produced by different tools (Camunda, Flowable, Signavio,
     * VCC-VPB) are handled without errors.
     *
     * @param bpmn_xml  Full BPMN 2.0 XML document.
     * @return ImportResult with nodes and edges on success.
     */
    static ImportResult importXml(std::string_view bpmn_xml);

    /**
     * @brief Import a BPMN 2.0 XML file from disk.
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
     */
    static std::string exportFromJson(const nlohmann::json& normalized_graph);

private:
    // XML helpers
    static std::string escapeXml_(std::string_view s);
    static std::string nodeTypeToXmlTag_(BPMNNodeType t);
    static BPMNNodeType xmlTagToNodeType_(std::string_view tag);
};

} // namespace process
} // namespace themis

