/**
 * @file ocel_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Process Modeling Module
 *
 * File:    ocel_exporter.cpp
 * Module:  src/process/
 * Purpose: OCEL 2.0 export implementation.
 */

#include "process/ocel_exporter.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis::process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

OcelExporter::OcelExporter(
    RocksDBWrapper&      db,
    ProcessGraphManager& engine,
    ProcessModelManager& models,
    ProcessLinker&       linker)
    : db_(db), engine_(engine), models_(models), linker_(linker)
{}

// ─────────────────────────────────────────────────────────────────────────────
// toIso8601_
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string OcelExporter::toIso8601_(int64_t epoch_ms) {
    const auto sec = static_cast<std::time_t>(epoch_ms / 1000);
    const auto milliseconds = static_cast<int>(epoch_ms % 1000);
    std::tm tm_val{};
#ifdef _WIN32
    gmtime_s(&tm_val, &sec);
#else
    gmtime_r(&sec, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << milliseconds << 'Z';
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildObjects_
// ─────────────────────────────────────────────────────────────────────────────

json OcelExporter::buildObjects_(std::string_view instance_id) const {
    json objects = json::array();
    const auto attachments = linker_.getAttachments(instance_id);

    for (const auto& att : attachments) {
        json obj;
        obj["id"]   = att.object_id;
        obj["type"] = att.object_collection;

        // Attributes: metadata from attachment + attached_by, link_type
        json attrs = json::array();
        if (!att.attached_by.empty()) {
            attrs.push_back({{"name", "attached_by"}, {"time", toIso8601_(0)},
                             {"value", att.attached_by}});
        }
        if (!att.metadata.empty() && att.metadata.is_object()) {
            for (const auto& [k, v] : att.metadata.items()) {
                attrs.push_back({{"name", k},
                                 {"time", toIso8601_(0)},
                                 {"value", v.dump()}});
            }
        }
        obj["attributes"] = attrs;
        objects.push_back(obj);
    }
    return objects;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildEvents_
// ─────────────────────────────────────────────────────────────────────────────

json OcelExporter::buildEvents_(const ProcessInstance& inst) const {
    json events = json::array();

    // Collect all attachments for relationship building
    const auto attachments = linker_.getAttachments(inst.instance_id);

    // Build an ordered list of (activity_name, timestamp_ms) from all tokens
    struct EventEntry {
        std::string node_id;
        int64_t     timestamp_ms;
    };
    std::vector<EventEntry> entries = {};

    entries.reserve(inst.tokens.size() * 16);  // Estimate based on typical token path lengths
    std::unordered_set<std::string> added_nodes;  // Track already-added nodes for O(1) lookup

    for (const auto& tok : inst.tokens) {
        // Walk visited_nodes in order, using visit_timestamps where available
        for (const auto& nid : tok.visited_nodes) {
            int64_t ts = inst.started_at_ms;
            auto vt_it = tok.visit_timestamps.find(nid);
            if (vt_it != tok.visit_timestamps.end()) {
                // Convert system_clock::time_point to epoch milliseconds
                ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                         vt_it->second.time_since_epoch()).count();
            }
            entries.push_back(EventEntry{.node_id = nid, .timestamp_ms = ts});
            added_nodes.insert(nid);
        }
        // Add current node as final event if not already in visited_nodes
        if (!tok.current_node.empty() && added_nodes.find(tok.current_node) == added_nodes.end()) {
            int64_t ts = tok.started_at_ms.value_or(inst.started_at_ms);
            entries.push_back(EventEntry{.node_id = tok.current_node, .timestamp_ms = ts});
            added_nodes.insert(tok.current_node);
        }
    }

    // Sort by timestamp
    std::ranges::sort(entries,
                      [](const EventEntry& a, const EventEntry& b) {
                          return a.timestamp_ms < b.timestamp_ms;
                      });

    // Try to load the process model for node names
    std::map<std::string, std::string> node_names;
    auto model_opt = models_.load(inst.process_definition_id);
    if (model_opt.has_value() &&
        model_opt->normalized.contains("nodes")) {
        for (const auto& n : model_opt->normalized["nodes"]) {
            std::string nid = n.value("id", "");
            if (!nid.empty()) {
                node_names[nid] = n.value("name", nid);
            }
        }
    }

    // Generate event objects
    int seq = 0;
    for (const auto& entry : entries) {
        json evt;
        evt["id"]   = inst.instance_id + "-ev-" + std::to_string(seq++);
        evt["type"] = node_names.contains(entry.node_id) ?
                          node_names[entry.node_id] : entry.node_id;
        evt["time"] = toIso8601_(entry.timestamp_ms);

        // Instance-level attributes
        json attrs = json::array();
        attrs.push_back({{"name", "node_id"}, {"value", entry.node_id}});
        attrs.push_back({{"name", "process_instance"}, {"value", inst.instance_id}});
        evt["attributes"] = attrs;

        // Relationships: link all attachments that were present at this timestamp
        json rels = json::array();
        for (const auto& att : attachments) {
            json rel;
            rel["objectId"]   = att.object_id;
            rel["qualifier"]  = att.object_collection;
            rels.push_back(rel);
        }
        evt["relationships"] = rels;
        events.push_back(evt);
    }

    return events;
}

// ─────────────────────────────────────────────────────────────────────────────
// deriveObjectTypes_
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ json OcelExporter::deriveObjectTypes_(const json& objects) {
    std::set<std::string> seen;
    json types = json::array();
    for (const auto& obj : objects) {
        std::string t = obj.value("type", "");
        if (!t.empty() && seen.insert(t).second) {
            types.push_back({{"name", t}, {"attributes", json::array()}});
        }
    }
    return types;
}

// ─────────────────────────────────────────────────────────────────────────────
// deriveEventTypes_
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ json OcelExporter::deriveEventTypes_(const json& events) {
    std::set<std::string> seen;
    json types = json::array();
    for (const auto& evt : events) {
        std::string t = evt.value("type", "");
        if (!t.empty() && seen.insert(t).second) {
            types.push_back({{"name", t}, {"attributes", json::array()}});
        }
    }
    return types;
}

// ─────────────────────────────────────────────────────────────────────────────
// exportInstance
// ─────────────────────────────────────────────────────────────────────────────

json OcelExporter::exportInstance(std::string_view instance_id) const {
    auto [status, inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        SPDLOG_WARN("[OcelExporter] Instance '{}' not found", instance_id);
        return {};
    }

    json objects = buildObjects_(instance_id);
    json events  = buildEvents_(inst);

    return {
        {"ocel:version",     "2.0"},
        {"objectTypes",      deriveObjectTypes_(objects)},
        {"eventTypes",       deriveEventTypes_(events)},
        {"objects",          objects},
        {"events",           events}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// exportModel
// ─────────────────────────────────────────────────────────────────────────────

json OcelExporter::exportModel(std::string_view model_id) const {
    return exportFiltered(model_id,
                          std::numeric_limits<int64_t>::min(),
                          std::numeric_limits<int64_t>::max());
}

// ─────────────────────────────────────────────────────────────────────────────
// exportFiltered
// ─────────────────────────────────────────────────────────────────────────────

json OcelExporter::exportFiltered(std::string_view model_id,
                                   int64_t          from_ms,
                                   int64_t          to_ms) const {
    // Scan all instances stored under "process:instance:" prefix
    // and filter by process_definition_id matching model_id
    const std::string instance_prefix = "process:instance:";
    const std::string model_str = std::string(model_id);

    json all_objects = json::array();
    json all_events  = json::array();

    db_.scanPrefix(instance_prefix, [&](std::string_view key, std::string_view) -> bool {
        // Extract instance_id from key
        const std::string full_key = std::string(key);
        if (full_key.size() <= instance_prefix.size()) {
            return true;
        }
        const std::string iid = full_key.substr(instance_prefix.size());

        auto [status, inst] = engine_.getProcessInstance(iid);
        if (!status.ok) {
            return true;
        }

        if (inst.process_definition_id != model_str) {
            return true;
        }
        if (inst.started_at_ms < from_ms || inst.started_at_ms > to_ms) {
            return true;
        }

        json obj = buildObjects_(iid);
        json evt = buildEvents_(inst);
        for (auto& o : obj) {
            all_objects.push_back(o);
        }
        for (auto& e : evt) {
            all_events.push_back(e);
        }
        return true;
    });

    if (all_events.empty()) {
        return {};
    }

    return {
        {"ocel:version", "2.0"},
        {"objectTypes",  deriveObjectTypes_(all_objects)},
        {"eventTypes",   deriveEventTypes_(all_events)},
        {"objects",      all_objects},
        {"events",       all_events}
    };
}

} // namespace themis::process

