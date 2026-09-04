/**
 * @file project_diff.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "projects/project_diff.h"
#include <stdexcept>

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <string>

namespace themis {
namespace projects {

// ─── DeltaEntry serialisation ─────────────────────────────────────────────────

json DeltaEntry::toJson() const {
    const char* type_str =
        type == DeltaType::ADDED    ? "added" :
        type == DeltaType::REMOVED  ? "removed" : "modified";
    return json{
        {"field_path", field_path},
        {"type",       type_str},
        {"old_value",  old_value},
        {"new_value",  new_value},
    };
}

DeltaEntry DeltaEntry::fromJson(const json& j) {
    DeltaEntry e;
    e.field_path = j.value("field_path", std::string{});
    const std::string t = j.value("type", std::string{"modified"});
    e.type = (t == "added")   ? DeltaType::ADDED  :
             (t == "removed") ? DeltaType::REMOVED : DeltaType::MODIFIED;
    e.old_value = j.value("old_value", json{});
    e.new_value = j.value("new_value", json{});
    return e;
}

// ─── DeltaSet serialisation ───────────────────────────────────────────────────

json DeltaSet::toJson() const {
    json arr = json::array();
    for (const auto& e : entries) {
      arr.push_back(e.toJson());
    }
    return json{{"entries", arr}};
}

DeltaSet DeltaSet::fromJson(const json& j) {
    DeltaSet ds;
    if (j.contains("entries") && j["entries"].is_array()) {
        for (const auto& e : j["entries"])
            ds.entries.push_back(DeltaEntry::fromJson(e));
    }
    return ds;
}

// ─── ProjectDiff ─────────────────────────────────────────────────────────────

ProjectDiff::ProjectDiff(std::shared_ptr<RocksDBWrapper> storage)
    : storage_(std::move(storage)) {}

void ProjectDiff::setMetrics(std::shared_ptr<ProjectMetrics> metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = std::move(metrics);
}

void ProjectDiff::diffRecursive(
    const std::string&       path,
    const json&              from,
    const json&              to,
    std::vector<DeltaEntry>& out) const
{
    if (from == to) {
      return;
    }

    if (from.is_object() && to.is_object()) {
        // Fields removed in `to`
        for (const auto& [key, val] : from.items()) {
            if (!to.contains(key)) {
                out.push_back({path + "/" + key, DeltaType::REMOVED, val, nullptr});
            }
        }
        // Fields added or modified in `to`
        for (const auto& [key, val] : to.items()) {
            const std::string child_path = path + "/" + key;
            if (!from.contains(key)) {
                out.push_back({child_path, DeltaType::ADDED, nullptr, val});
            } else {
                diffRecursive(child_path, from.at(key), val, out);
            }
        }
    } else {
        // Scalar or mismatched-type change
        out.push_back({path, DeltaType::MODIFIED, from, to});
    }
}

DeltaSet ProjectDiff::diffDocuments(const json& from, const json& to) const {
    DeltaSet ds;
    diffRecursive("", from, to, ds.entries);
    return ds;
}

DeltaSet ProjectDiff::diff(
    const SnapshotId& from_snap,
    const SnapshotId& to_snap) const
{
    const auto t0 = std::chrono::steady_clock::now();

    DeltaSet ds;

    // Load content of both snapshots
    auto loadContent = [this](const SnapshotId& snap_id) -> json {
        const auto snap_uuid = snap_id.substr(5); // strip "snap:"
        std::string content_str;
        if (!storage_->get("snap_data:" + snap_uuid, content_str))
            return json::array();
        try { return json::parse(content_str); }
        catch (const nlohmann::json::exception &) { return json::array(); }
        catch (const std::exception &) { return json::array(); }
        catch (const std::string &) { return json::array(); }
        catch (const char *) { return json::array(); }
    };

    const json from_docs = loadContent(from_snap);
    const json to_docs   = loadContent(to_snap);

    // Build lookup maps: doc_id → doc_json
    auto buildMap = [](const json& docs) {
        std::unordered_map<std::string, json> m;
        if (!docs.is_array()) {
          return m;
        }
        for (const auto& doc : docs) {
            const std::string id = doc.value("id", std::string{});
            if (!id.empty()) {
              m[id] = doc;
            }
        }
        return m;
    };

    auto from_map = buildMap(from_docs);
    auto to_map   = buildMap(to_docs);

    // Documents removed
    for (const auto& [id, doc] : from_map) {
        if (to_map.find(id) == to_map.end()) {
            ds.entries.push_back({"/" + id, DeltaType::REMOVED, doc, nullptr});
        }
    }

    // Documents added or modified
    for (const auto& [id, doc] : to_map) {
        if (from_map.find(id) == from_map.end()) {
            ds.entries.push_back({"/" + id, DeltaType::ADDED, nullptr, doc});
        } else {
            diffRecursive("/" + id, from_map.at(id), doc, ds.entries);
        }
    }

    // Record latency in metrics sink (if set)
    {
        const auto t1     = std::chrono::steady_clock::now();
        const auto dur_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        if (metrics_)
            metrics_->recordDiff(dur_ms);
    }

    return ds;
}

// ─── ProjectMerge ─────────────────────────────────────────────────────────────

ProjectMerge::ProjectMerge(std::shared_ptr<RocksDBWrapper> storage)
    : storage_(storage), diff_(storage) {}

MergeResult ProjectMerge::merge(
    const SnapshotId& ancestor_snap,
    const SnapshotId& ours_snap,
    const SnapshotId& theirs_snap) const
{
    MergeResult result;

    const DeltaSet ours_delta   = diff_.diff(ancestor_snap, ours_snap);
    const DeltaSet theirs_delta = diff_.diff(ancestor_snap, theirs_snap);

    // Build index of our changes by field path for fast conflict detection
    std::unordered_map<std::string, const DeltaEntry*> ours_index;
    for (const auto& e : ours_delta.entries)
        ours_index[e.field_path] = &e;

    for (const auto& their_entry : theirs_delta.entries) {
        auto it = ours_index.find(their_entry.field_path);
        if (it == ours_index.end()) {
            // No conflict — accept their change
            result.applied.entries.push_back(their_entry);
        } else {
            // Same field modified in both branches — conflict
            result.conflicts.entries.push_back(their_entry);
        }
    }

    result.ok = result.conflicts.empty();
    if (!result.ok) {
        result.message = "Merge completed with " +
                         std::to_string(result.conflicts.totalChanges()) +
                         " conflict(s)";
    } else {
        result.message = "Merge completed successfully";
    }
    return result;
}

} // namespace projects
} // namespace themis
