/**
 * @file process_linker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_linker.cpp
 * Module:  src/process/
 * Purpose: Implementation of ProcessLinker – attaching documents/metadata to
 *          process instances and linking process instances to each other.
 */

#include "process/process_linker.h"
#include "process/process_common.h"
#include "process/process_diagnostics.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// toString / fromString helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string_view toString(ProcessLinkType t) {
    switch (t) {
        case ProcessLinkType::HAS_DOCUMENT:      return "HAS_DOCUMENT";
        case ProcessLinkType::HAS_METADATA:      return "HAS_METADATA";
        case ProcessLinkType::REQUIRES_DOCUMENT: return "REQUIRES_DOCUMENT";
        case ProcessLinkType::IS_INSTANCE_OF:    return "IS_INSTANCE_OF";
        case ProcessLinkType::SUB_PROCESS:       return "SUB_PROCESS";
        case ProcessLinkType::CROSS_REFERENCE:   return "CROSS_REFERENCE";
        case ProcessLinkType::TRIGGERS:          return "TRIGGERS";
        case ProcessLinkType::EVIDENCE_FOR:      return "EVIDENCE_FOR";
    }
    return "UNKNOWN";
}

ProcessLinkType processLinkTypeFromString(std::string_view s) {
    if (s == "HAS_DOCUMENT")      return ProcessLinkType::HAS_DOCUMENT;
    if (s == "HAS_METADATA")      return ProcessLinkType::HAS_METADATA;
    if (s == "REQUIRES_DOCUMENT") return ProcessLinkType::REQUIRES_DOCUMENT;
    if (s == "IS_INSTANCE_OF")    return ProcessLinkType::IS_INSTANCE_OF;
    if (s == "SUB_PROCESS")       return ProcessLinkType::SUB_PROCESS;
    if (s == "CROSS_REFERENCE")   return ProcessLinkType::CROSS_REFERENCE;
    if (s == "TRIGGERS")          return ProcessLinkType::TRIGGERS;
    if (s == "EVIDENCE_FOR")      return ProcessLinkType::EVIDENCE_FOR;
    return ProcessLinkType::HAS_DOCUMENT;
}

// ─────────────────────────────────────────────────────────────────────────────
// ProcessAttachment serialisation
// ─────────────────────────────────────────────────────────────────────────────

json ProcessAttachment::toDocument() const {
    json doc;
    doc["id"]                = id;
    doc["instance_id"]       = instance_id;
    doc["object_id"]         = object_id;
    doc["object_collection"] = object_collection;
    doc["link_type"]         = std::string(toString(link_type));
    doc["attached_by"]       = attached_by;
    doc["attached_at_ms"]    = attached_at_ms;
    doc["metadata"]          = metadata;
    if (node_id.has_value()) {
        doc["node_id"] = *node_id;
    }
    return doc;
}

ProcessAttachment ProcessAttachment::fromDocument(const json& doc) {
    ProcessAttachment a;
    a.id                = doc.value("id", "");
    a.instance_id       = doc.value("instance_id", "");
    a.object_id         = doc.value("object_id", "");
    a.object_collection = doc.value("object_collection", "");
    a.link_type         = processLinkTypeFromString(doc.value("link_type", "HAS_DOCUMENT"));
    a.attached_by       = doc.value("attached_by", "");
    a.attached_at_ms    = doc.value("attached_at_ms", int64_t{0});
    a.metadata          = doc.value("metadata", json::object());
    if (doc.contains("node_id") && !doc["node_id"].is_null()) {
        a.node_id = doc["node_id"].get<std::string>();
    }
    return a;
}

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLink serialisation
// ─────────────────────────────────────────────────────────────────────────────

json ProcessLink::toDocument() const {
    json doc;
    doc["link_id"]       = link_id;
    doc["source_id"]     = source_id;
    doc["target_id"]     = target_id;
    doc["link_type"]     = std::string(toString(link_type));
    doc["properties"]    = properties;
    doc["created_at_ms"] = created_at_ms;
    return doc;
}

ProcessLink ProcessLink::fromDocument(const json& doc) {
    ProcessLink l;
    l.link_id       = doc.value("link_id", "");
    l.source_id     = doc.value("source_id", "");
    l.target_id     = doc.value("target_id", "");
    l.link_type     = processLinkTypeFromString(doc.value("link_type", "CROSS_REFERENCE"));
    l.properties    = doc.value("properties", json::object());
    l.created_at_ms = doc.value("created_at_ms", int64_t{0});
    return l;
}

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLinker – construction
// ─────────────────────────────────────────────────────────────────────────────

ProcessLinker::ProcessLinker(RocksDBWrapper& db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
// Internal key builders
// ─────────────────────────────────────────────────────────────────────────────

std::string ProcessLinker::makeAttachKey_(std::string_view instance_id,
                                          std::string_view object_id) const {
    return "proc:attach:" + std::string(instance_id) + ":" + std::string(object_id);
}

std::string ProcessLinker::makeObjIdxKey_(std::string_view object_id,
                                           std::string_view collection,
                                           std::string_view instance_id) const {
    return "proc:obj_idx:" + std::string(object_id) + ":" +
           std::string(collection) + ":" + std::string(instance_id);
}

std::string ProcessLinker::makeLinkKey_(std::string_view source_id,
                                         std::string_view target_id,
                                         ProcessLinkType  link_type) const {
    return "proc:link:" + std::string(source_id) + ":" +
           std::string(target_id) + ":" + std::string(toString(link_type));
}

std::string ProcessLinker::makeReqDocKey_(std::string_view model_id,
                                           std::string_view node_id,
                                           std::string_view doc_type) const {
    return "proc:req_doc:" + std::string(model_id) + ":" +
           std::string(node_id) + ":" + std::string(doc_type);
}

// ─────────────────────────────────────────────────────────────────────────────
// attachObject
// ─────────────────────────────────────────────────────────────────────────────

std::pair<bool, std::string> ProcessLinker::attachObject(
    std::string_view instance_id,
    std::string_view object_id,
    std::string_view object_collection,
    ProcessLinkType  link_type,
    std::optional<std::string_view> node_id,
    json             metadata,
    std::string_view attached_by)
{
    if (instance_id.empty() || object_id.empty()) {
        return {false, "instance_id and object_id must not be empty"};
    }

    ProcessAttachment att;
    att.id                = "attach:" + std::string(instance_id) + ":" + std::string(object_id);
    att.instance_id       = std::string(instance_id);
    att.object_id         = std::string(object_id);
    att.object_collection = std::string(object_collection);
    att.link_type         = link_type;
    att.attached_by       = std::string(attached_by);
    att.attached_at_ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    att.metadata          = std::move(metadata);
    if (node_id.has_value()) {
        att.node_id = std::string(*node_id);
    }

    const std::string key = makeAttachKey_(instance_id, object_id);
    const std::string val = att.toDocument().dump();

    if (!db_.put(key, val)) {
        SPDLOG_WARN("[process_linker] failed to persist attachment '{}' for instance '{}'",
                    att.id, instance_id);
        return {false, "storage write failed"};
    }

    // Write reverse-lookup secondary index for findInstancesWithObject().
    const std::string idx_key =
        makeObjIdxKey_(object_id, object_collection, instance_id);
    db_.put(idx_key, "1");

    SPDLOG_INFO("[process_linker] attached '{}' ({}) to instance '{}'",
                object_id, std::string(toString(link_type)), instance_id);
    return {true, att.id};
}

// ─────────────────────────────────────────────────────────────────────────────
// detachObject
// ─────────────────────────────────────────────────────────────────────────────

bool ProcessLinker::detachObject(std::string_view attachment_id) {
    // attachment_id format: "attach:<instance_id>:<object_id>"
    // Reconstruct the RocksDB primary key: "proc:attach:<instance_id>:<object_id>"
    std::string sid(attachment_id);
    if (sid.size() > 7 && sid.substr(0, 7) == "attach:") {
        sid = "proc:" + sid;
    }

    // Read the attachment document to extract fields needed for index cleanup.
    std::string existing;
    if (!db_.get(sid, existing)) {
        SPDLOG_WARN("[process_linker] detachObject: attachment key '{}' not found", sid);
        return false;
    }

    // Remove the secondary reverse-lookup index entry.
    try {
        auto doc = json::parse(existing);
        if (!doc.value("deleted", false)) {
            const std::string obj_id  = doc.value("object_id", "");
            const std::string col     = doc.value("object_collection", "");
            const std::string inst_id = doc.value("instance_id", "");
            if (!obj_id.empty() && !inst_id.empty()) {
                db_.del(makeObjIdxKey_(obj_id, col, inst_id));
            }
        }
    } catch (const std::exception& ex) {
        SPDLOG_WARN("[process_linker] detachObject: failed to parse attachment '{}': {}",
                    sid, ex.what());
    }

    // Hard delete the primary attachment record.
    if (!db_.del(sid)) {
        SPDLOG_WARN("[process_linker] detachObject: del failed for '{}'", sid);
        return false;
    }

    SPDLOG_INFO("[process_linker] detached attachment '{}'", attachment_id);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// getAttachments
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ProcessAttachment> ProcessLinker::getAttachments(
    std::string_view instance_id,
    std::optional<ProcessLinkType> filter_type) const
{
    std::vector<ProcessAttachment> results;
    const std::string prefix = "proc:attach:" + std::string(instance_id) + ":";

    db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(value);
            if (doc.value("deleted", false)) return true;
            auto att = ProcessAttachment::fromDocument(doc);
            if (!filter_type.has_value() || att.link_type == *filter_type) {
                results.push_back(std::move(att));
            }
        } catch (const std::exception& ex) {
            SPDLOG_WARN("[process_linker] getAttachments parse error: {}", ex.what());
        }
        return true;
    });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// getNodeAttachments
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ProcessAttachment> ProcessLinker::getNodeAttachments(
    std::string_view instance_id,
    std::string_view node_id) const
{
    auto all = getAttachments(instance_id);
    std::vector<ProcessAttachment> result;
    for (auto& att : all) {
        if (att.node_id.has_value() && *att.node_id == node_id) {
            result.push_back(std::move(att));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// findInstancesWithObject
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> ProcessLinker::findInstancesWithObject(
    std::string_view object_id,
    std::string_view object_collection) const
{
    std::vector<std::string> instances;

    // Use the secondary reverse-lookup index for O(prefix-scan) lookup
    // instead of a full scan over all attachments.
    // Key scheme: proc:obj_idx:<object_id>:<collection>:<instance_id>
    const std::string prefix =
        "proc:obj_idx:" + std::string(object_id) + ":" +
        std::string(object_collection) + ":";

    db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) -> bool {
        // The instance_id is the suffix after the prefix.
        if (key.size() > prefix.size()) {
            instances.emplace_back(key.substr(prefix.size()));
        }
        return true;
    });

    // Index entries are unique per (object_id, collection, instance_id) triple,
    // but sort for stable ordering.
    std::sort(instances.begin(), instances.end());
    return instances;
}

// ─────────────────────────────────────────────────────────────────────────────
// linkProcesses
// ─────────────────────────────────────────────────────────────────────────────

std::pair<bool, std::string> ProcessLinker::linkProcesses(
    std::string_view source_id,
    std::string_view target_id,
    ProcessLinkType  link_type,
    json             properties)
{
    if (source_id.empty() || target_id.empty()) {
        return {false, "source_id and target_id must not be empty"};
    }

    // Phase 3: Validate link target exists (stale link detection)
    if (!isLinkTargetValid(target_id)) {
        auto incident = ProcessDiagnostics::createMissingTargetIncident(
            ProcError::kInvalidTransition,
            target_id,
            "Cannot create link: target does not exist"
        );
        SPDLOG_WARN("[process_linker] {}", incident.toFormattedMessage());
        return {false, "target does not exist (stale reference protection)"};
    }

    // Phase 3: Detect cyclic dependencies
    if (wouldCreateCycle(source_id, target_id)) {
        auto incident = ProcessDiagnostics::createCycleIncident(
            ProcError::kInvalidTransition,
            source_id,
            "Creating link would introduce a cycle: " + std::string(source_id) + " → " + std::string(target_id)
        );
        SPDLOG_WARN("[process_linker] {}", incident.toFormattedMessage());
        return {false, "would create a cycle"};
    }

    ProcessLink lnk;
    lnk.link_id = "link:" + std::string(source_id) + ":" +
                  std::string(target_id) + ":" + std::string(toString(link_type));
    lnk.source_id     = std::string(source_id);
    lnk.target_id     = std::string(target_id);
    lnk.link_type     = link_type;
    lnk.properties    = std::move(properties);
    lnk.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

    const std::string key = makeLinkKey_(source_id, target_id, link_type);
    const std::string val = lnk.toDocument().dump();

    if (!db_.put(key, val)) {
        SPDLOG_WARN("[process_linker] linkProcesses: storage write failed for '{}'", lnk.link_id);
        return {false, "storage write failed"};
    }

    SPDLOG_INFO("[process_linker] linked '{}' → '{}' ({})",
                source_id, target_id, std::string(toString(link_type)));
    return {true, lnk.link_id};
}

// ─────────────────────────────────────────────────────────────────────────────
// getLinks
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ProcessLink> ProcessLinker::getLinks(
    std::string_view process_id,
    std::optional<ProcessLinkType> filter_type) const
{
    std::vector<ProcessLink> results;
    const std::string prefix = "proc:link:" + std::string(process_id) + ":";

    db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(value);
            if (doc.value("deleted", false)) return true;
            auto lnk = ProcessLink::fromDocument(doc);
            if (!filter_type.has_value() || lnk.link_type == *filter_type) {
                results.push_back(std::move(lnk));
            }
        } catch (const std::exception& ex) {
            SPDLOG_WARN("[process_linker] getLinks parse error: {}", ex.what());
        }
        return true;
    });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// registerRequiredDocument
// ─────────────────────────────────────────────────────────────────────────────

bool ProcessLinker::registerRequiredDocument(
    std::string_view model_id,
    std::string_view node_id,
    std::string_view doc_type,
    bool             mandatory,
    json             schema)
{
    if (model_id.empty() || node_id.empty() || doc_type.empty()) {
        SPDLOG_WARN("[process_linker] registerRequiredDocument: empty parameter");
        return false;
    }

    json doc;
    doc["model_id"]   = std::string(model_id);
    doc["node_id"]    = std::string(node_id);
    doc["doc_type"]   = std::string(doc_type);
    doc["mandatory"]  = mandatory;
    doc["schema"]     = std::move(schema);

    const std::string key = makeReqDocKey_(model_id, node_id, doc_type);
    if (!db_.put(key, doc.dump())) {
        SPDLOG_WARN("[process_linker] registerRequiredDocument: write failed for key '{}'", key);
        return false;
    }

    SPDLOG_INFO("[process_linker] registered required doc '{}' for node '{}' in model '{}'",
                doc_type, node_id, model_id);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// getRequiredDocuments
// ─────────────────────────────────────────────────────────────────────────────

std::vector<json> ProcessLinker::getRequiredDocuments(
    std::string_view model_id,
    std::string_view node_id) const
{
    std::vector<json> results;
    const std::string prefix =
        "proc:req_doc:" + std::string(model_id) + ":" + std::string(node_id) + ":";

    db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            results.push_back(json::parse(value));
        } catch (const std::exception& ex) {
            SPDLOG_WARN("[process_linker] getRequiredDocuments parse error: {}", ex.what());
        }
        return true;
    });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// getMissingDocuments
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> ProcessLinker::getMissingDocuments(
    std::string_view instance_id,
    std::string_view node_id,
    std::string_view model_id) const
{
    // Collect required doc types for this node in the model
    auto required = getRequiredDocuments(model_id, node_id);

    // Collect already-attached doc types for this instance+node
    auto node_atts = getNodeAttachments(instance_id, node_id);
    std::set<std::string> present_types;
    for (const auto& att : node_atts) {
        // The attached_by convention: metadata["doc_type"] carries the type
        if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {
            present_types.insert(att.metadata["doc_type"].get<std::string>());
        }
    }

    // Cross-reference
    std::vector<std::string> missing;
    for (const auto& req : required) {
        if (!req.value("mandatory", false)) continue;
        std::string dtype = req.value("doc_type", "");
        if (!dtype.empty() && present_types.find(dtype) == present_types.end()) {
            missing.push_back(dtype);
        }
    }

    return missing;
}

// ─────────────────────────────────────────────────────────────────────────────
// Cyclic dependency detection (Phase 3)
// ─────────────────────────────────────────────────────────────────────────────

bool ProcessLinker::wouldCreateCycle(
    std::string_view source_id,
    std::string_view target_id,
    int32_t max_depth
) const {
    if (source_id == target_id) {
        // Self-loop is trivially a cycle
        return true;
    }

    if (max_depth <= 0) {
        max_depth = kMaxRetrievalDepth;
    }

    // Use DFS to check if there is a path from target → source
    // If yes, adding source → target would create a cycle
    std::set<std::string> visited;
    return hasCyclePath_(source_id, target_id, visited, 0, max_depth);
}

bool ProcessLinker::isLinkTargetValid(std::string_view target_id) const {
    if (target_id.empty()) {
        return false;
    }

    bool exists = false;
    const std::string target = std::string(target_id);
    const std::string link_prefix = "proc:link:" + target + ":";
    db_.scanPrefix(link_prefix, [&](std::string_view /*key*/, std::string_view /*value*/) -> bool {
        exists = true;
        return false;
    });

    if (exists) {
        return true;
    }

    const std::string attach_prefix = "proc:attach:" + target + ":";
    db_.scanPrefix(attach_prefix, [&](std::string_view /*key*/, std::string_view /*value*/) -> bool {
        exists = true;
        return false;
    });

    return exists;
}

bool ProcessLinker::hasCyclePath_(
    std::string_view source,
    std::string_view target,
    std::set<std::string>& visited,
    int32_t depth,
    int32_t max_depth
) const {
    if (depth >= max_depth) {
        // Depth limit reached – fail closed so deep cycles are not admitted.
        return true;
    }

    if (visited.count(std::string(target)) > 0) {
        // Already visited in this path – avoid infinite recursion
        return false;
    }

    visited.insert(std::string(target));

    // Get all outgoing links from target
    auto outgoing = getLinks(target);

    for (const auto& link : outgoing) {
        if (link.target_id == source) {
            // Found a path back to source – would create a cycle
            return true;
        }

        // Recurse on the target of this link
        if (hasCyclePath_(source, link.target_id, visited, depth + 1, max_depth)) {
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Phase 2: Conflict Detection and Rollback Implementation
// ---------------------------------------------------------------------------

ProcessLinker::LinkOperationGuard::~LinkOperationGuard() {
    if (failed_) {
        linker_.rollbackLinkOperation_(operation_id_);
        return;
    }

    std::unique_lock<std::shared_mutex> lock(linker_.link_state_lock_);
    linker_.rollback_records_.erase(operation_id_);
}

void ProcessLinker::LinkOperationGuard::recordModification(std::string_view key) {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string prior_value;
    const bool existed_before = linker_.db_.get(key, prior_value);

    ConflictRecord record{
        operation_id_,
        std::string(key),
        now_ms,
        1,
        existed_before,
        existed_before ? std::move(prior_value) : std::string{}
    };

    modifications_.push_back(record);

    std::unique_lock<std::shared_mutex> lock(linker_.link_state_lock_);
    linker_.rollback_records_[operation_id_].push_back(std::move(record));
}

bool ProcessLinker::detectLinkingConflict_(
    std::string_view key,
    std::optional<uint64_t> expected_version) const
{
    std::shared_lock<std::shared_mutex> lock(link_state_lock_);
    
    // Try to read the current value to detect if it's been modified
    std::string current_value;
    if (!db_.get(key, current_value)) {
        // Key doesn't exist; conflict only if we expected a version
        return expected_version.has_value();
    }

    // If we have an expected version, verify it matches (simple versioning)
    if (expected_version.has_value()) {
        try {
            auto doc = json::parse(current_value);
            uint64_t current_version = doc.value("_version", uint64_t{0});
            return current_version != *expected_version;
        } catch (...) {
            // Parse error – assume conflict
            return true;
        }
    }
    
    // No conflict if we're not checking version
    return false;
}

void ProcessLinker::rollbackLinkOperation_(uint64_t operation_id) {
    std::unique_lock<std::shared_mutex> lock(link_state_lock_);

    auto it = rollback_records_.find(operation_id);
    if (it == rollback_records_.end()) {
        SPDLOG_WARN("[process] rollback requested for unknown link operation {}", operation_id);
        return;
    }

    for (auto record_it = it->second.rbegin(); record_it != it->second.rend(); ++record_it) {
        const auto& record = *record_it;
        bool restored = false;
        if (record.existed_before) {
            restored = db_.put(record.affected_key, record.previous_value);
        } else {
            restored = db_.del(record.affected_key);
        }

        if (!restored) {
            SPDLOG_WARN("[process] failed to roll back key '{}' for link operation {}",
                        record.affected_key, operation_id);
        }
    }

    rollback_records_.erase(it);
    SPDLOG_WARN("[process] Rolled back link operation: {}", operation_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stale Link Detection & Cleanup (Phase 3)
// ─────────────────────────────────────────────────────────────────────────────

DiagnosticRecord ProcessLinker::detectStaleLinkAtReadTime(
    std::string_view link_id
) const {
    // Parse link_id format: "link:<source>:<target>:<type>"
    std::string link_str(link_id);
    
    // Retrieve the link document from database
    std::string link_key = "proc:link:";  // Will be constructed from parsed data
    std::string link_doc_str;
    
    // Scan for the link with this ID (expensive, but link_id should be stored as well)
    bool found = false;
    ProcessLink found_link;
    
    db_.scanPrefix("proc:link:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(value);
            std::string stored_id = doc.value("link_id", "");
            if (stored_id == link_str) {
                found_link = ProcessLink::fromDocument(doc);
                found = true;
                return false;  // Stop scanning
            }
        } catch (const std::exception& e) {
            // Log the corruption and continue scanning other links.
            // Callers that need structured diagnostics for corrupted documents
            // should invoke findStaleLinkReferences() which emits per-link records.
            SPDLOG_WARN("[process_linker] detectStaleLinkAtReadTime: JSON parse error while scanning links: {}", e.what());
        }
        return true;
    });
    
    if (!found) {
        return ProcessDiagnostics::createMissingTargetIncident(
            ProcError::kInvalidTransition,
            link_id,
            "Link not found in database"
        );
    }
    
    // Now check if the target exists
    if (!isLinkTargetValid(found_link.target_id)) {
        DiagnosticContext ctx;
        ctx.recordResourceMetric("source_id", 0);
        ctx.recordResourceMetric("target_id", 0);
        ctx.setRemediationSuggestion(
            "Target '" + found_link.target_id + "' does not exist. "
            "Link is stale and should be removed via cleanupOrphanedLinks()."
        );
        
        auto incident = ProcessDiagnostics::createMissingTargetIncident(
            ProcError::kInvalidTransition,
            link_id,
            "Link target '" + found_link.target_id + "' is missing (stale reference)"
        );
        return incident;
    }
    
    // Link is valid
    return DiagnosticRecord(
        DiagnosticIncidentType::LINKING_INCIDENT,
        ProcError::kSuccess,
        "verify_link_valid",
        link_id,
        "Link target is valid"
    );
}

std::vector<std::string> ProcessLinker::findStaleLinkReferences() const {
    std::vector<std::string> stale_links;
    
    db_.scanPrefix("proc:link:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(value);
            auto link = ProcessLink::fromDocument(doc);
            
            // Check if target exists
            if (!isLinkTargetValid(link.target_id)) {
                stale_links.push_back(link.link_id);
                SPDLOG_WARN("[process_linker] Found stale link: {} → {} (target missing)",
                           link.source_id, link.target_id);
            }
        } catch (const std::exception& ex) {
            SPDLOG_WARN("[process_linker] Error parsing link document: {}", ex.what());
        }
        return true;
    });
    
    return stale_links;
}

std::pair<int32_t, std::string> ProcessLinker::cleanupOrphanedLinks(
    const std::vector<std::string>& link_ids
) {
    int32_t count_removed = 0;
    
    for (const auto& link_id : link_ids) {
        // Parse link_id and construct the key to delete
        // link_id format: "link:<source>:<target>:<type>"
        
        // Find the link first to get its components
        bool found = false;
        std::string link_key_to_delete;
        
        db_.scanPrefix("proc:link:", [&](std::string_view key, std::string_view value) -> bool {
            try {
                auto doc = json::parse(value);
                std::string stored_id = doc.value("link_id", "");
                if (stored_id == link_id) {
                    auto link = ProcessLink::fromDocument(doc);
                    link_key_to_delete = makeLinkKey_(link.source_id, link.target_id, link.link_type);
                    found = true;
                    return false;  // Stop scanning
                }
            } catch (const std::exception& e) {
                // Log and create diagnostic for corrupted link document
                SPDLOG_WARN("[process_linker] cleanupOrphanedLinks: JSON parse error while scanning: {}", e.what());
                DiagnosticContext ctx;
                ctx.recordResourceMetric("target_link_id", link_id.size());
                ctx.setRemediationSuggestion("A stored link document could not be parsed as JSON during cleanup. "
                                            "This indicates data corruption. Check database integrity.");
                auto incident = ProcessDiagnostics::createLinkingIncident(
                    ProcError::kLinkingStateInvalid,
                    link_id,
                    "Corrupted link document during cleanup scan: " + std::string(e.what())
                );
                // Continue scanning other links despite this error
            }
            return true;
        });
        
        if (found) {
            if (db_.del(link_key_to_delete)) {
                count_removed++;
                SPDLOG_INFO("[process_linker] Removed orphaned link: {}", link_id);
            } else {
                SPDLOG_WARN("[process_linker] Failed to remove orphaned link: {}", link_id);
            }
        } else {
            SPDLOG_WARN("[process_linker] Link not found for cleanup: {}", link_id);
        }
    }
    
    std::string error_msg = (count_removed == static_cast<int32_t>(link_ids.size())) 
        ? "" 
        : "Some links could not be removed";
    
    return {count_removed, error_msg};
}

std::pair<int32_t, int32_t> ProcessLinker::verifyLinkIntegrity() const {
    int32_t total_links = 0;
    int32_t links_with_issues = 0;
    
    db_.scanPrefix("proc:link:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(value);
            auto link = ProcessLink::fromDocument(doc);
            
            total_links++;
            
            // Check if both source and target exist
            if (!isLinkTargetValid(link.source_id) || !isLinkTargetValid(link.target_id)) {
                links_with_issues++;
                SPDLOG_WARN("[process_linker] Link integrity issue detected: {} (source valid: {}, target valid: {})",
                           link.link_id, isLinkTargetValid(link.source_id), isLinkTargetValid(link.target_id));
            }
        } catch (const std::exception& ex) {
            links_with_issues++;
            SPDLOG_WARN("[process_linker] Error parsing link for integrity check: {}", ex.what());
        }
        return true;
    });
    
    SPDLOG_INFO("[process_linker] Link integrity verification: {} total links, {} with issues",
               total_links, links_with_issues);
    
    return {total_links, links_with_issues};
}

} // namespace process
} // namespace themis
