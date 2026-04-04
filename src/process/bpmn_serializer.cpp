/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bpmn_serializer.cpp                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-30 04:18:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     434                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3fea6d6b5  2026-03-12  refactor: clean up includes and remove unused transaction... ║
    • 7f7a27240  2026-03-12  feat(process): add ProcessLinker, ProcessGraphRag, and mo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    bpmn_serializer.cpp
 * Module:  src/process/
 * Purpose: BPMN 2.0 XML import and export for process model definitions.
 *
 * Implementation note: we use a minimal hand-written XML parser for import
 * (no external XML library dependency) and produce standards-compliant XML on
 * export.  BPMNDI (diagram interchange) data is intentionally ignored on import
 * and not emitted on export because ThemisDB stores processes as graph data,
 * not as graphical diagrams.
 */

#include "process/bpmn_serializer.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>

namespace themis {
namespace process {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Internal XML mini-parser helpers
// ---------------------------------------------------------------------------

namespace {

/// Extract value of attribute `name` from an XML tag string.
/// Returns empty string when the attribute is not found.
std::string extractAttr(const std::string& tag, const std::string& name) {
    // Match: name="value" or name='value'
    std::regex re(name + R"(\s*=\s*["']([^"']*)["'])");
    std::smatch m;
    if (std::regex_search(tag, m, re)) {
        return m[1].str();
    }
    return {};
}

/// Strip XML/HTML character entities and leading/trailing whitespace.
std::string cleanText(const std::string& s) {
    std::string out = s;
    // Basic entity unescaping
    auto replace_all = [&](const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace_all("&amp;",  "&");
    replace_all("&lt;",   "<");
    replace_all("&gt;",   ">");
    replace_all("&quot;", "\"");
    replace_all("&apos;", "'");

    // Trim
    out.erase(0, out.find_first_not_of(" \t\n\r"));
    out.erase(out.find_last_not_of(" \t\n\r") + 1);
    return out;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// BpmnSerializer::escapeXml_
// ---------------------------------------------------------------------------

std::string BpmnSerializer::escapeXml_(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;        break;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// BpmnSerializer::nodeTypeToXmlTag_
// ---------------------------------------------------------------------------

std::string BpmnSerializer::nodeTypeToXmlTag_(BPMNNodeType t) {
    switch (t) {
        case BPMNNodeType::START_EVENT:         return "startEvent";
        case BPMNNodeType::END_EVENT:           return "endEvent";
        case BPMNNodeType::INTERMEDIATE_EVENT:  return "intermediateCatchEvent";
        case BPMNNodeType::BOUNDARY_EVENT:      return "boundaryEvent";
        case BPMNNodeType::TASK:                return "task";
        case BPMNNodeType::SUBPROCESS:          return "subProcess";
        case BPMNNodeType::CALL_ACTIVITY:       return "callActivity";
        case BPMNNodeType::EXCLUSIVE_GATEWAY:   return "exclusiveGateway";
        case BPMNNodeType::PARALLEL_GATEWAY:    return "parallelGateway";
        case BPMNNodeType::INCLUSIVE_GATEWAY:   return "inclusiveGateway";
        case BPMNNodeType::EVENT_BASED_GATEWAY: return "eventBasedGateway";
        case BPMNNodeType::COMPLEX_GATEWAY:     return "complexGateway";
        case BPMNNodeType::POOL:                return "participant";
        case BPMNNodeType::LANE:                return "lane";
        case BPMNNodeType::DATA_OBJECT:         return "dataObjectReference";
        case BPMNNodeType::DATA_STORE:          return "dataStoreReference";
        case BPMNNodeType::GROUP:               return "group";
        case BPMNNodeType::ANNOTATION:          return "textAnnotation";
        default:                                return "task";
    }
}

// ---------------------------------------------------------------------------
// BpmnSerializer::xmlTagToNodeType_
// ---------------------------------------------------------------------------

BPMNNodeType BpmnSerializer::xmlTagToNodeType_(std::string_view tag) {
    if (tag == "startEvent")              return BPMNNodeType::START_EVENT;
    if (tag == "endEvent")                return BPMNNodeType::END_EVENT;
    if (tag == "intermediateCatchEvent" ||
        tag == "intermediateThrowEvent") return BPMNNodeType::INTERMEDIATE_EVENT;
    if (tag == "boundaryEvent")          return BPMNNodeType::BOUNDARY_EVENT;
    if (tag == "subProcess")             return BPMNNodeType::SUBPROCESS;
    if (tag == "callActivity")           return BPMNNodeType::CALL_ACTIVITY;
    if (tag == "exclusiveGateway")       return BPMNNodeType::EXCLUSIVE_GATEWAY;
    if (tag == "parallelGateway")        return BPMNNodeType::PARALLEL_GATEWAY;
    if (tag == "inclusiveGateway")       return BPMNNodeType::INCLUSIVE_GATEWAY;
    if (tag == "eventBasedGateway")      return BPMNNodeType::EVENT_BASED_GATEWAY;
    if (tag == "complexGateway")         return BPMNNodeType::COMPLEX_GATEWAY;
    if (tag == "participant")            return BPMNNodeType::POOL;
    if (tag == "lane")                   return BPMNNodeType::LANE;
    if (tag == "dataObjectReference")    return BPMNNodeType::DATA_OBJECT;
    if (tag == "dataStoreReference")     return BPMNNodeType::DATA_STORE;
    if (tag == "group")                  return BPMNNodeType::GROUP;
    if (tag == "textAnnotation")         return BPMNNodeType::ANNOTATION;
    // All task variants → TASK
    if (tag.find("Task") != std::string_view::npos ||
        tag == "task")                   return BPMNNodeType::TASK;
    return BPMNNodeType::TASK;
}

// ---------------------------------------------------------------------------
// BpmnSerializer::importXml
// ---------------------------------------------------------------------------

BpmnSerializer::ImportResult BpmnSerializer::importXml(std::string_view bpmn_xml) {
    ImportResult result;

    // Minimal token-based XML scan (no full DOM) — sufficient for BPMN 2.0
    std::string xml(bpmn_xml);

    // Extract process id and name
    {
        std::regex proc_re(R"(<process[^>]+\bid\s*=\s*["']([^"']*)["'][^>]*>)", std::regex::icase);
        std::smatch m;
        if (std::regex_search(xml, m, proc_re)) {
            result.process_id = m[1].str();
            // Try to extract name attribute from the same tag match
            std::string tag_str = m[0].str();
            result.process_name = extractAttr(tag_str, "name");
        } else {
            result.process_id   = "imported_process";
            result.process_name = "Imported Process";
        }
    }

    // Known flow node tags
    static const std::vector<std::string> node_tags = {
        "startEvent", "endEvent", "intermediateCatchEvent", "intermediateThrowEvent",
        "boundaryEvent", "task", "userTask", "serviceTask", "scriptTask",
        "sendTask", "receiveTask", "manualTask", "businessRuleTask",
        "subProcess", "callActivity",
        "exclusiveGateway", "parallelGateway", "inclusiveGateway",
        "eventBasedGateway", "complexGateway",
        "dataObjectReference", "dataStoreReference", "textAnnotation"
    };

    // Parse flow nodes
    for (const auto& tag : node_tags) {
        // Match self-closing and opening tags
        std::regex tag_re("<" + tag + R"([^>]*(?:id\s*=\s*["'][^"']*["'])[^>]*(?:/>|>))",
                          std::regex::icase);
        auto it  = std::sregex_iterator(xml.begin(), xml.end(), tag_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tag_str = it->str();
            std::string id   = extractAttr(tag_str, "id");
            std::string name = extractAttr(tag_str, "name");

            if (id.empty()) continue;

            ProcessNodeInfo node;
            node.node_id  = id;
            node.name     = name.empty() ? id : cleanText(name);
            node.node_type = xmlTagToNodeType_(tag);

            // Determine subtype for task variants
            if (tag == "userTask")         node.subtype = "USER_TASK";
            else if (tag == "serviceTask") node.subtype = "SERVICE_TASK";
            else if (tag == "scriptTask")  node.subtype = "SCRIPT_TASK";
            else if (tag == "sendTask")    node.subtype = "SEND_TASK";
            else if (tag == "receiveTask") node.subtype = "RECEIVE_TASK";
            else if (tag == "manualTask")  node.subtype = "MANUAL_TASK";
            else if (tag == "businessRuleTask") node.subtype = "BUSINESS_RULE_TASK";

            result.nodes.push_back(std::move(node));
        }
    }

    // Parse sequenceFlow
    {
        std::regex sf_re(R"(<sequenceFlow[^>]+>)", std::regex::icase);
        auto it  = std::sregex_iterator(xml.begin(), xml.end(), sf_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tag_str = it->str();
            ProcessEdgeInfo edge;
            edge.edge_id   = extractAttr(tag_str, "id");
            edge.from_node = extractAttr(tag_str, "sourceRef");
            edge.to_node   = extractAttr(tag_str, "targetRef");
            edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;

            // Extract inline condition expression if present
            // (conditionExpression child element — simplistic extraction)
            std::string cond_name = extractAttr(tag_str, "name");
            if (!cond_name.empty()) {
                edge.condition_expression = cond_name;
            }

            if (!edge.from_node.empty() && !edge.to_node.empty()) {
                result.edges.push_back(std::move(edge));
            }
        }
    }

    // Parse messageFlow
    {
        std::regex mf_re(R"(<messageFlow[^>]+>)", std::regex::icase);
        auto it  = std::sregex_iterator(xml.begin(), xml.end(), mf_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tag_str = it->str();
            ProcessEdgeInfo edge;
            edge.edge_id   = extractAttr(tag_str, "id");
            edge.from_node = extractAttr(tag_str, "sourceRef");
            edge.to_node   = extractAttr(tag_str, "targetRef");
            edge.edge_type = ProcessEdgeType::MESSAGE_FLOW;

            if (!edge.from_node.empty() && !edge.to_node.empty()) {
                result.edges.push_back(std::move(edge));
            }
        }
    }

    if (result.nodes.empty() && result.edges.empty()) {
        result.ok      = false;
        result.message = "No BPMN flow elements found in XML";
        return result;
    }

    result.ok      = true;
    result.message = "OK";
    return result;
}

// ---------------------------------------------------------------------------
// BpmnSerializer::importFile
// ---------------------------------------------------------------------------

BpmnSerializer::ImportResult BpmnSerializer::importFile(std::string_view file_path) {
    std::ifstream f{std::string(file_path)};
    if (!f.is_open()) {
        ImportResult r;
        r.ok      = false;
        r.message = "Cannot open file: " + std::string(file_path);
        return r;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return importXml(content);
}

// ---------------------------------------------------------------------------
// BpmnSerializer::exportXml
// ---------------------------------------------------------------------------

std::string BpmnSerializer::exportXml(
    std::string_view                    process_id,
    std::string_view                    process_name,
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    std::ostringstream xml;

    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
    xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
    xml << R"(             targetNamespace="http://themis.db/process")" << "\n";
    xml << R"(             id="definitions_)" << escapeXml_(process_id) << R"(">)" << "\n\n";

    xml << R"(  <process id=")" << escapeXml_(process_id)
        << R"(" name=")" << escapeXml_(process_name)
        << R"(" isExecutable="true">)" << "\n";

    // Nodes
    for (const auto& n : nodes) {
        if (!std::holds_alternative<BPMNNodeType>(n.node_type)) {
            continue; // Skip EPK nodes in BPMN export
        }
        auto btype = std::get<BPMNNodeType>(n.node_type);

        // Determine actual tag (subtype overrides for tasks)
        std::string tag = nodeTypeToXmlTag_(btype);
        if (btype == BPMNNodeType::TASK && !n.subtype.empty()) {
            if      (n.subtype == "USER_TASK")         tag = "userTask";
            else if (n.subtype == "SERVICE_TASK")      tag = "serviceTask";
            else if (n.subtype == "SCRIPT_TASK")       tag = "scriptTask";
            else if (n.subtype == "SEND_TASK")         tag = "sendTask";
            else if (n.subtype == "RECEIVE_TASK")      tag = "receiveTask";
            else if (n.subtype == "MANUAL_TASK")       tag = "manualTask";
            else if (n.subtype == "BUSINESS_RULE_TASK")tag = "businessRuleTask";
        }

        xml << "    <" << tag
            << " id=\"" << escapeXml_(n.node_id) << "\""
            << " name=\"" << escapeXml_(n.name) << "\"";

        if (n.is_async) {
            xml << " themis:isAsync=\"true\"";
        }
        xml << "/>\n";
    }

    // Edges
    for (const auto& e : edges) {
        std::string tag;
        switch (e.edge_type) {
            case ProcessEdgeType::MESSAGE_FLOW:    tag = "messageFlow";    break;
            case ProcessEdgeType::ASSOCIATION:     tag = "association";    break;
            case ProcessEdgeType::DATA_ASSOCIATION:tag = "dataInputAssociation"; break;
            default:                               tag = "sequenceFlow";   break;
        }

        xml << "    <" << tag
            << " id=\"" << escapeXml_(e.edge_id) << "\""
            << " sourceRef=\"" << escapeXml_(e.from_node) << "\""
            << " targetRef=\"" << escapeXml_(e.to_node) << "\"";

        if (e.condition_expression) {
            xml << " name=\"" << escapeXml_(*e.condition_expression) << "\"";
        }
        xml << "/>\n";
    }

    xml << "  </process>\n";
    xml << "</definitions>\n";

    return xml.str();
}

// ---------------------------------------------------------------------------
// BpmnSerializer::exportFromJson
// ---------------------------------------------------------------------------

std::string BpmnSerializer::exportFromJson(const json& g) {
    if (g.is_null()) return {};

    std::string pid  = g.value("process_id", "process");
    std::string name = g.value("name", pid);

    std::vector<ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;

    if (g.contains("nodes")) {
        for (const auto& jn : g["nodes"]) {
            ProcessNodeInfo n;
            n.node_id  = jn.value("id",   "");
            n.name     = jn.value("name", "");
            n.subtype  = jn.value("subtype", "");

            std::string type_str = jn.value("type", "TASK");
            n.node_type = xmlTagToNodeType_(type_str);
            nodes.push_back(std::move(n));
        }
    }

    if (g.contains("edges")) {
        for (const auto& je : g["edges"]) {
            ProcessEdgeInfo e;
            e.edge_id   = je.value("id",   "");
            e.from_node = je.value("from", "");
            e.to_node   = je.value("to",   "");
            std::string cond = je.value("condition", "");
            if (!cond.empty()) e.condition_expression = cond;
            e.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
            edges.push_back(std::move(e));
        }
    }

    return exportXml(pid, name, nodes, edges);
}

} // namespace process
} // namespace themis
