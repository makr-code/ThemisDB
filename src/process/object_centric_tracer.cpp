/**
 * @file object_centric_tracer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    object_centric_tracer.cpp
 * Module:  src/process/
 * Purpose: Object-Centric Process Mining (OCPM) — OCEL 2.0 log builder,
 *          Directly-Follows Multigraph, and convergence/divergence analysis.
 *          P6 implementation (van der Aalst 2022).
 */

#include "process/object_centric_tracer.h"
#include "utils/logger.h"

#include <algorithm>
#include <map>
#include <set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// ObjectCentricTracer
// ─────────────────────────────────────────────────────────────────────────────

ObjectCentricTracer::ObjectCentricTracer(
    ProcessLinker&       linker,
    ProcessModelManager& model_manager)
    : linker_(linker)
    , model_manager_(model_manager)
{}

// ─────────────────────────────────────────────────────────────────────────────
// buildOcelLog
// ─────────────────────────────────────────────────────────────────────────────

json ObjectCentricTracer::buildOcelLog(std::string_view instance_id) const {
    const auto attachments = linker_.getAttachments(instance_id);

    // Collect distinct object types (collections)
    std::set<std::string> object_types_set;
    for (const auto& att : attachments) {
        object_types_set.insert(att.object_collection);
    }

    json result;

    // ocel:global-log
    json global_log;
    global_log["ocel:attribute-names"] = json::array();
    json obj_types_arr = json::array();
    for (const auto& ot : object_types_set) obj_types_arr.push_back(ot);
    global_log["ocel:object-types"] = std::move(obj_types_arr);
    result["ocel:global-log"] = std::move(global_log);

    // ocel:events
    json events_arr = json::array();
    for (const auto& att : attachments) {
        json ev;
        ev["ocel:id"]        = att.id;
        ev["ocel:activity"]  = std::string(toString(att.link_type));
        ev["ocel:timestamp"] = att.attached_at_ms;

        // ocel:omap — object map: {collection: [object_id]}
        json omap;
        omap[att.object_collection] = json::array({att.object_id});
        ev["ocel:omap"] = std::move(omap);

        // ocel:vmap — attribute map (empty in base form; attach metadata if present)
        ev["ocel:vmap"] = att.metadata.is_null() ? json::object() : att.metadata;

        events_arr.push_back(std::move(ev));
    }
    result["ocel:events"] = std::move(events_arr);

    // ocel:objects — {object_id: {ocel:type: collection}}
    json objects_map = json::object();
    for (const auto& att : attachments) {
        if (!objects_map.contains(att.object_id)) {
            objects_map[att.object_id] = {{"ocel:type", att.object_collection}};
        }
    }
    result["ocel:objects"] = std::move(objects_map);

    THEMIS_INFO("ObjectCentricTracer::buildOcelLog: {} events for instance '{}'",
                attachments.size(), instance_id);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// computeDfmg
// ─────────────────────────────────────────────────────────────────────────────

json ObjectCentricTracer::computeDfmg(
    std::string_view model_id,
    std::string_view object_type) const
{
    json result;
    result["object_type"] = std::string(object_type);
    result["nodes"]       = json::array();
    result["arcs"]        = json::array();

    auto opt = model_manager_.load(model_id);
    if (!opt.has_value()) {
        THEMIS_WARN("ObjectCentricTracer::computeDfmg: model '{}' not found", model_id);
        return result;
    }

    const auto& normalized = opt->normalized;
    if (!normalized.contains("nodes") || !normalized.contains("edges")) {
        return result;
    }

    // Collect node IDs
    std::set<std::string> node_set;
    for (const auto& n : normalized["nodes"]) {
        const std::string id = n.value("id", "");
        if (!id.empty()) node_set.insert(id);
    }

    // Build arc frequency map: (from→to) → frequency
    // An arc is counted for each edge whose label / metadata references object_type,
    // or (when object_type is non-empty) for all edges as a baseline DFMG approach.
    // O(n) over edges — meets the performance target.
    std::map<std::string, std::map<std::string, int>> freq;
    const std::string obj_type_str(object_type);

    for (const auto& e : normalized["edges"]) {
        const std::string from = e.value("from", e.value("source", ""));
        const std::string to   = e.value("to",   e.value("target", ""));
        if (from.empty() || to.empty()) continue;

        // Include arc if object_type is referenced in the edge metadata / label,
        // or if object_type is empty (include all).
        bool matches = obj_type_str.empty();
        if (!matches) {
            // Check label, type, and metadata fields
            const std::string label = e.value("label", e.value("name", ""));
            if (label.find(obj_type_str) != std::string::npos) {
                matches = true;
            } else if (e.contains("metadata")) {
                const auto& meta = e["metadata"];
                if (meta.contains("object_type") &&
                    meta["object_type"].get<std::string>() == obj_type_str) {
                    matches = true;
                }
            }
        }

        if (matches) {
            freq[from][to]++;
            node_set.insert(from);
            node_set.insert(to);
        }
    }

    // Build nodes array
    json nodes_arr = json::array();
    for (const auto& nid : node_set) nodes_arr.push_back(nid);
    result["nodes"] = std::move(nodes_arr);

    // Build arcs array (unique arcs only)
    json arcs_arr = json::array();
    for (const auto& [from, to_map] : freq) {
        for (const auto& [to, cnt] : to_map) {
            arcs_arr.push_back({{"from", from}, {"to", to}, {"frequency", cnt}});
        }
    }
    result["arcs"] = std::move(arcs_arr);

    THEMIS_INFO("ObjectCentricTracer::computeDfmg: {} arcs for model '{}' / type '{}'",
                result["arcs"].size(), model_id, object_type);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// analyze
// ─────────────────────────────────────────────────────────────────────────────

ObjectCentricTracer::ConvergenceDivergenceResult
ObjectCentricTracer::analyze(std::string_view model_id) const
{
    ConvergenceDivergenceResult result;

    auto opt = model_manager_.load(model_id);
    if (!opt.has_value()) {
        THEMIS_WARN("ObjectCentricTracer::analyze: model '{}' not found", model_id);
        return result;
    }

    const auto& normalized = opt->normalized;
    if (!normalized.contains("edges")) return result;

    // in_degree_by_type[node][obj_type]  and  out_degree_by_type[node][obj_type]
    std::map<std::string, std::map<std::string, int>> in_deg;
    std::map<std::string, std::map<std::string, int>> out_deg;

    for (const auto& e : normalized["edges"]) {
        const std::string from = e.value("from", e.value("source", ""));
        const std::string to   = e.value("to",   e.value("target", ""));
        if (from.empty() || to.empty()) continue;

        // Determine object type from edge metadata or default
        std::string obj_type = "default";
        if (e.contains("metadata") && e["metadata"].contains("object_type")) {
            obj_type = e["metadata"]["object_type"].get<std::string>();
        } else if (e.contains("label") && !e["label"].get<std::string>().empty()) {
            obj_type = e["label"].get<std::string>();
        }

        out_deg[from][obj_type]++;
        in_deg[to][obj_type]++;
    }

    std::set<std::string> conv_set;
    std::set<std::string> div_set;

    for (const auto& [node, type_map] : in_deg) {
        for (const auto& [type, cnt] : type_map) {
            if (cnt > 1) {
                conv_set.insert(node);
                break;
            }
        }
    }
    for (const auto& [node, type_map] : out_deg) {
        for (const auto& [type, cnt] : type_map) {
            if (cnt > 1) {
                div_set.insert(node);
                break;
            }
        }
    }

    result.convergence_nodes = std::vector<std::string>(conv_set.begin(), conv_set.end());
    result.divergence_nodes  = std::vector<std::string>(div_set.begin(), div_set.end());

    THEMIS_INFO("ObjectCentricTracer::analyze: model '{}' → {} convergence, {} divergence nodes",
                model_id,
                result.convergence_nodes.size(),
                result.divergence_nodes.size());
    return result;
}

} // namespace process
} // namespace themis
