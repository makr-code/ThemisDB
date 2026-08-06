/**
 * @file epk_serializer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: epk_serializer.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 340
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=11, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    epk_serializer.cpp
 * Module:  src/process/
 * Purpose: EPK (Ereignisgesteuerte Prozesskette) import and export.
 *
 * Supported input formats:
 *  1. Simple text notation (line-based, arrow arrows `->`)
 *  2. JSON array of {type, id, name, …} node and edge objects
 */

#include "process/epk_serializer.h"
#include "utils/logger.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// EPK type helpers
// ---------------------------------------------------------------------------

std::string EpkSerializer::epkNodeTypeToLabel_(EPKNodeType t) {
    switch (t) {
        case EPKNodeType::EVENT:              return "EVENT";
        case EPKNodeType::FUNCTION:           return "FUNCTION";
        case EPKNodeType::AND_CONNECTOR:      return "AND";
        case EPKNodeType::OR_CONNECTOR:       return "OR";
        case EPKNodeType::XOR_CONNECTOR:      return "XOR";
        case EPKNodeType::ORGANIZATIONAL_UNIT:return "ORG_UNIT";
        case EPKNodeType::INFORMATION_OBJECT: return "INFO_OBJ";
        case EPKNodeType::APPLICATION_SYSTEM: return "APP_SYS";
        case EPKNodeType::PROCESS_PATH:       return "PROC_PATH";
        default:                              return "FUNCTION";
    }
}

EPKNodeType EpkSerializer::labelToEpkNodeType_(std::string_view label) {
    if (label == "EVENT"  || label == "Ereignis")       return EPKNodeType::EVENT;
    if (label == "FUNCTION" || label == "Funktion")     return EPKNodeType::FUNCTION;
    if (label == "AND")                                  return EPKNodeType::AND_CONNECTOR;
    if (label == "OR")                                   return EPKNodeType::OR_CONNECTOR;
    if (label == "XOR")                                  return EPKNodeType::XOR_CONNECTOR;
    if (label == "ORG_UNIT" || label == "Organisationseinheit")
                                                         return EPKNodeType::ORGANIZATIONAL_UNIT;
    if (label == "INFO_OBJ" || label == "Informationsobjekt")
                                                         return EPKNodeType::INFORMATION_OBJECT;
    if (label == "APP_SYS"  || label == "Anwendungssystem")
                                                         return EPKNodeType::APPLICATION_SYSTEM;
    if (label == "PROC_PATH"|| label == "Prozesswegweiser")
                                                         return EPKNodeType::PROCESS_PATH;
    return EPKNodeType::FUNCTION;
}

// ---------------------------------------------------------------------------
// importText
// ---------------------------------------------------------------------------

EpkSerializer::ImportResult EpkSerializer::importText(
    std::string_view epk_text,
    std::string_view process_id,
    std::string_view process_name)
{
    ImportResult result;
    result.process_id   = process_id.empty()   ? "epk_process" : std::string(process_id);
    result.process_name = process_name.empty() ? "EPK Process" : std::string(process_name);

    std::istringstream ss{std::string(epk_text)};
    std::string line;
    int node_counter = 0;
    std::string last_node_id;

    // Parse line-by-line
    // Supported line formats:
    //   TYPE: "name" [attr=value, ...]
    //   -> TYPE: "name"
    while (std::getline(ss, line)) {
        // Trim
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;

        bool is_edge = false;
        if (line.substr(0, 2) == "->") {
            is_edge = true;
            line    = line.substr(2);
            line.erase(0, line.find_first_not_of(" \t"));
        }

        // Parse TYPE: "name" or TYPE: name
        std::regex node_re(R"((\w+)\s*:\s*["\']?([^"\'\[\n]+)["\']?\s*(?:\[([^\]]*)\])?)");
        std::smatch m;
        if (!std::regex_search(line, m, node_re)) continue;

        std::string type_str = m[1].str();
        std::string name_str = m[2].str();
        name_str.erase(name_str.find_last_not_of(" \t") + 1);

        // Auto-generate node id
        std::string node_id = "n" + std::to_string(++node_counter);

        ProcessNodeInfo node;
        node.node_id   = node_id;
        node.name      = name_str;
        node.node_type = labelToEpkNodeType_(type_str);

        // Parse optional attributes [role="x", sla=48h]
        if (m[3].matched) {
            std::string attrs = m[3].str();
            std::regex attr_re(R"((\w+)\s*=\s*["\']?([^,"'\]]+)["\']?)");
            auto ai  = std::sregex_iterator(attrs.begin(), attrs.end(), attr_re);
            auto aend = std::sregex_iterator();
            for (; ai != aend; ++ai) {
                std::string key = (*ai)[1].str();
                std::string val = (*ai)[2].str();
                if (key == "role" || key == "responsible_role") {
                    // Store as description for now
                    node.description = "Role: " + val;
                }
            }
        }

        result.nodes.push_back(node);

        // Add edge from last node if this is a flow arrow or sequential
        if (is_edge && !last_node_id.empty()) {
            ProcessEdgeInfo edge;
            edge.edge_id   = "e" + std::to_string(result.edges.size() + 1);
            edge.from_node = last_node_id;
            edge.to_node   = node_id;
            edge.edge_type = ProcessEdgeType::CONTROL_FLOW;
            result.edges.push_back(std::move(edge));
        } else if (!last_node_id.empty()) {
            // Implicit sequential flow between consecutive lines
            ProcessEdgeInfo edge;
            edge.edge_id   = "e" + std::to_string(result.edges.size() + 1);
            edge.from_node = last_node_id;
            edge.to_node   = node_id;
            edge.edge_type = ProcessEdgeType::CONTROL_FLOW;
            result.edges.push_back(std::move(edge));
        }

        last_node_id = node_id;
    }

    if (result.nodes.empty()) {
        result.ok      = false;
        result.message = "No EPK elements found in text input";
        return result;
    }

    result.ok      = true;
    result.message = "OK";
    return result;
}

// ---------------------------------------------------------------------------
// importJson
// ---------------------------------------------------------------------------

EpkSerializer::ImportResult EpkSerializer::importJson(const json& epk_json) {
    ImportResult result;

    if (epk_json.contains("process_id")) {
        result.process_id = epk_json.value("process_id", "epk_process");
    }
    if (epk_json.contains("name")) {
        result.process_name = epk_json.value("name", "EPK Process");
    }

    if (epk_json.contains("nodes")) {
        for (const auto& jn : epk_json["nodes"]) {
            ProcessNodeInfo n;
            n.node_id     = jn.value("id",   "");
            n.name        = jn.value("name", "");
            n.description = jn.value("description", "");
            n.node_type   = labelToEpkNodeType_(jn.value("type", "FUNCTION"));
            result.nodes.push_back(std::move(n));
        }
    }

    if (epk_json.contains("edges")) {
        for (const auto& je : epk_json["edges"]) {
            ProcessEdgeInfo e;
            e.edge_id   = je.value("id",   "");
            e.from_node = je.value("from", "");
            e.to_node   = je.value("to",   "");
            e.edge_type = ProcessEdgeType::CONTROL_FLOW;
            result.edges.push_back(std::move(e));
        }
    }

    result.ok      = !result.nodes.empty();
    result.message = result.ok ? "OK" : "Empty EPK JSON";
    return result;
}

// ---------------------------------------------------------------------------
// exportText
// ---------------------------------------------------------------------------

std::string EpkSerializer::exportText(
    std::string_view                    process_name,
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    std::ostringstream out;
    out << "# EPK: " << process_name << "\n\n";

    // Build adjacency for ordered traversal
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& e : edges) {
        adj[e.from_node].push_back(e.to_node);
    }

    // Build id→node map
    std::map<std::string, const ProcessNodeInfo*> node_map;
    for (const auto& n : nodes) {
        node_map[n.node_id] = &n;
    }

    // Find start nodes (no incoming edges)
    std::set<std::string> has_incoming;
    for (const auto& e : edges) {
        has_incoming.insert(e.to_node);
    }

    // Emit nodes in BFS order
    std::set<std::string> visited;
    std::queue<std::string> q;
    for (const auto& n : nodes) {
        if (has_incoming.find(n.node_id) == has_incoming.end()) {
            q.push(n.node_id);
        }
    }
    if (q.empty() && !nodes.empty()) {
        q.push(nodes.front().node_id);
    }

    bool first = true;
    while (!q.empty()) {
        auto id = q.front(); q.pop();
        if (visited.count(id)) continue;
        visited.insert(id);

        auto it = node_map.find(id);
        if (it == node_map.end()) continue;
        const auto& n = *it->second;

        std::string type_label;
        if (std::holds_alternative<EPKNodeType>(n.node_type)) {
            type_label = epkNodeTypeToLabel_(std::get<EPKNodeType>(n.node_type));
        } else {
            type_label = "FUNCTION";
        }

        if (!first) out << "-> ";
        out << type_label << ": \"" << n.name << "\"";
        if (!n.description.empty()) {
            out << " # " << n.description;
        }
        out << "\n";
        first = false;

        auto ait = adj.find(id);
        if (ait != adj.end()) {
            for (const auto& next : ait->second) {
                q.push(next);
            }
        }
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// exportJson
// ---------------------------------------------------------------------------

json EpkSerializer::exportJson(
    std::string_view                    process_id,
    std::string_view                    process_name,
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    json g;
    g["process_id"] = std::string(process_id);
    g["name"]       = std::string(process_name);
    g["notation"]   = "EPK";

    json jnodes = json::array();
    for (const auto& n : nodes) {
        json jn;
        jn["id"]   = n.node_id;
        jn["name"] = n.name;
        jn["description"] = n.description;
        if (std::holds_alternative<EPKNodeType>(n.node_type)) {
            jn["type"] = epkNodeTypeToLabel_(std::get<EPKNodeType>(n.node_type));
        } else {
            jn["type"] = "FUNCTION";
        }
        jnodes.push_back(std::move(jn));
    }
    g["nodes"] = std::move(jnodes);

    json jedges = json::array();
    for (const auto& e : edges) {
        json je;
        je["id"]   = e.edge_id;
        je["from"] = e.from_node;
        je["to"]   = e.to_node;
        je["type"] = "CONTROL_FLOW";
        jedges.push_back(std::move(je));
    }
    g["edges"] = std::move(jedges);

    return g;
}

} // namespace process
} // namespace themis
