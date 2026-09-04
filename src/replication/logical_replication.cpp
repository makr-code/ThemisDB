/**
 * @file logical_replication.cpp
 * @brief ThemisDB Logical Replication Implementation
 *
 * Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=10, M=19, L=4
 * @note Status: Production Ready
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <future>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include "replication/logical_replication.h"
#include <fstream>
#include "utils/logger.h"
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace themisdb {
namespace replication {

// ----------------------------------------------------------------------------
// WAVE1-FIX [no_timeout:647,702]: module-local helper mirror of the identical
// template in replication_manager.cpp.  Wraps a blocking callable with a
// configurable millisecond deadline.  Returns true if the callable completed
// within the deadline; false if timed out (an error is already logged by the
// caller). On timeout the worker thread is detached so the caller returns
// immediately; callers MUST use only value captures in the passed lambda to
// avoid dangling references in the detached thread.
// ----------------------------------------------------------------------------
namespace {
template <typename Func>
bool lrm_executeWithTimeout(uint32_t timeout_ms, Func&& op) {
    using Operation = std::decay_t<Func>;
    Operation op_copy(std::forward<Func>(op));

    if (timeout_ms == 0) {
        try { op_copy(); return true; }
        catch (const std::exception& e) {
            THEMIS_ERROR("LogicalReplicationManager: I/O op failed: {}", e.what());
            return false;
        }
    }

    std::packaged_task<void()> task(std::move(op_copy));
    auto fut = task.get_future();

    // std::thread (not std::jthread) so detach on timeout does not block.
    std::thread worker([task = std::move(task)]() mutable {
        task();
    });

    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
        THEMIS_ERROR("LogicalReplicationManager: I/O op timed out after {}ms; "
                     "worker detached", timeout_ms);
        worker.detach();
        return false;
    }

    worker.join();
    try { fut.get(); }
    catch (const std::exception& e) {
        THEMIS_ERROR("LogicalReplicationManager: I/O op threw: {}", e.what());
        return false;
    }
    return true;
}
} // anonymous namespace

// ============================================================================
// Lock Hierarchy Documentation (logical_replication.cpp)
// ============================================================================
//
// This module implements a 2-level lock hierarchy for logical replication
// slot management using std::shared_mutex for reader-writer concurrency.
//
// LOCK HIERARCHY (ordered from outermost to innermost):
//
//   Level 1: LogicalReplicationManager::slots_mutex_
//            - Purpose: Protects slots_ map (slot collection)
//            - Scope: Slot lookup, creation, listing
//            - Hold time: MINIMAL (~microseconds)
//            - Pattern: shared_lock for reads, unique_lock for writes
//            - Mode: Multiple readers (slot access), exclusive writer (slot create)
//            - Typically released before accessing individual slot
//
//   Level 2: SlotRuntime::mutex
//            - Purpose: Protects individual slot state (buffer, metadata)
//            - Scope: Per-slot operations (advance, readChanges, persist)
//            - Hold time: VARIABLE (depends on buffer size)
//            - Pattern: lock_guard for state updates, persist outside if possible
//            - Mode: exclusive access to per-slot state
//
// BLOCKING OPERATIONS:
//   persistSlot() performs file I/O (currently under Level 2 lock)
//   readChanges() may buffer large change sets (under Level 2 lock)
//   Note: These should be refactored to execute lock-free when possible
//
// TIMEOUT SAFETY:
//   SlotRuntime::mutex could be upgraded to unique_lock with timeout
//   in high-contention scenarios (future improvement)
//
// ============================================================================

namespace {
std::string trimCopy(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
      start++;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[static_cast<int>(end - 1)]))) {
      end--;
    }
    return s.substr(start, end - start);
}

bool isSupportedRowFilter(const std::string& expr) {
    const auto trimmed = trimCopy(expr);
    if (trimmed.empty()) {
      return true;
    }
    return trimmed.find("==") != std::string::npos || trimmed.find("!=") != std::string::npos;
}
}  // namespace

LogicalReplicationManager::LogicalReplicationManager(std::shared_ptr<WALManager> wal)
    : LogicalReplicationManager(std::move(wal), Config{}) {}

LogicalReplicationManager::LogicalReplicationManager(std::shared_ptr<WALManager> wal, Config config)
    : wal_(std::move(wal))
    , config_(std::move(config)) {
    if (config_.wal_directory.empty()) {
        THEMIS_WARN("LogicalReplicationManager persistence disabled: wal_directory not configured");
    }
    loadPersistedSlots();
}

LogicalReplicationManager::LogicalReplicationSlot LogicalReplicationManager::createSlot(
    const std::string& slot_name,
    const std::string& output_plugin) {
    return createSlot(slot_name, output_plugin, ReplicationFilter{}, true, {});
}

LogicalReplicationManager::LogicalReplicationSlot LogicalReplicationManager::createSlot(
    const std::string& slot_name,
    const std::string& output_plugin,
    const ReplicationFilter& filter) {
    return createSlot(slot_name, output_plugin, filter, true, {});
}

LogicalReplicationManager::LogicalReplicationSlot LogicalReplicationManager::createSlot(
    const std::string& slot_name,
    const std::string& output_plugin,
    const ReplicationFilter& filter,
    bool perform_initial_sync) {
    return createSlot(slot_name, output_plugin, filter, perform_initial_sync, {});
}

LogicalReplicationManager::LogicalReplicationSlot LogicalReplicationManager::createSlot(
    const std::string& slot_name,
    const std::string& output_plugin,
    const ReplicationFilter& filter,
    bool perform_initial_sync,
    std::vector<LogicalChange> initial_snapshot) {

    if (!filter.row_filter_expression.empty()) {
        const auto expr = trimCopy(filter.row_filter_expression);
        if (!isSupportedRowFilter(expr)) {
            throw std::invalid_argument("Unsupported row filter expression: " + expr);
        }
    }

    auto runtime = std::make_shared<SlotRuntime>();
    runtime->meta.slot_name = slot_name;
    runtime->meta.plugin_name = output_plugin;
    runtime->meta.filter = filter;
    runtime->meta.restart_lsn = wal_ ? wal_->getCurrentSequence() : 0;
    runtime->meta.confirmed_flush_lsn = runtime->meta.restart_lsn;
    runtime->meta.initial_sync_pending = perform_initial_sync && !initial_snapshot.empty();
    runtime->initial_sync_pending = runtime->meta.initial_sync_pending;

    for (auto& snap : initial_snapshot) {
        if (snap.lsn == 0) {
            snap.lsn = runtime->meta.restart_lsn;
        }
        snap.type = LogicalChange::Type::SNAPSHOT;
        snap.schema_version = snap.schema_version.empty() ? config_.target_version : snap.schema_version;
        snap.source_version = config_.source_version;
        snap.target_version = config_.target_version;
        const auto doc_id = documentIdFromChange(snap);
        runtime->buffer.push_back(std::move(snap));
        if (!doc_id.empty()) {
            const auto key = collectionKey(runtime->buffer.back().collection, doc_id);
            runtime->snapshot_keys.insert(key);
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(slots_mutex_);
        if (slots_.count(slot_name)) {
            throw std::runtime_error("logical replication slot already exists: " + slot_name);
        }
        slots_[slot_name] = runtime;
    }
    {
        std::lock_guard<std::mutex> lock(runtime->mutex);
        persistSlot(*runtime);
    }
    return runtime->meta;
}

void LogicalReplicationManager::advanceSlot(const std::string& slot_name, uint64_t lsn) {
    std::shared_ptr<SlotRuntime> runtime;
    {
        std::shared_lock<std::shared_mutex> lock(slots_mutex_);
        auto it = slots_.find(slot_name);
        if (it == slots_.end()) {
          return;
        }
        runtime = it->second;
    }

    {
        std::lock_guard<std::mutex> lock(runtime->mutex);
        if (lsn < runtime->meta.confirmed_flush_lsn) {
            return;
        }
        runtime->meta.confirmed_flush_lsn = lsn;
        if (runtime->initial_sync_pending && lsn >= runtime->meta.restart_lsn) {
            runtime->initial_sync_pending = false;
            runtime->meta.initial_sync_pending = false;
            runtime->snapshot_keys.clear();
        }
        persistSlot(*runtime);
    }
}

std::vector<LogicalReplicationManager::LogicalReplicationSlot>
LogicalReplicationManager::listSlots() const {
    std::vector<LogicalReplicationSlot> out;
    std::shared_lock<std::shared_mutex> lock(slots_mutex_);
    out.reserve(slots_.size());
    for (const auto& kv : slots_) {
        std::lock_guard<std::mutex> g(kv.second->mutex);
        out.push_back(kv.second->meta);
    }
    return out;
}

bool LogicalReplicationManager::hasSlot(const std::string& slot_name) const {
    std::shared_lock<std::shared_mutex> lock(slots_mutex_);
    return slots_.find(slot_name) != slots_.end();
}

std::vector<LogicalChange> LogicalReplicationManager::readChanges(
    const std::string& slot_name, uint32_t max_changes) {
    std::vector<LogicalChange> out;
    std::shared_ptr<SlotRuntime> runtime;
    {
        std::shared_lock<std::shared_mutex> lock(slots_mutex_);
        auto it = slots_.find(slot_name);
        if (it == slots_.end()) {
          return out;
        }
        runtime = it->second;
    }

    std::lock_guard<std::mutex> lock(runtime->mutex);
    const uint32_t count = std::min<uint32_t>(max_changes, static_cast<uint32_t>(runtime->buffer.size()));
    // BATCH B OPTIMIZATION: Reserve space for all changes upfront
    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        out.push_back(std::move(runtime->buffer.front()));
        runtime->buffer.pop_front();
    }
    return out;
}

void LogicalReplicationManager::recordDDLChange(const std::string& ddl_statement,
                                                const std::string& schema_version,
                                                uint64_t lsn) {
    LogicalChange ddl;
    ddl.type = LogicalChange::Type::DDL;
    ddl.schema_version = schema_version.empty() ? config_.target_version : schema_version;
    ddl.source_version = config_.source_version;
    ddl.target_version = config_.target_version;
    ddl.ddl_statement = ddl_statement;
    ddl.lsn = lsn ? lsn : (wal_ ? wal_->getCurrentSequence() : 0);
    ddl.timestamp = std::chrono::system_clock::now();

    std::vector<std::shared_ptr<SlotRuntime>> slots_copy;
    {
        std::shared_lock<std::shared_mutex> lock(slots_mutex_);
        // BATCH B OPTIMIZATION: Reserve space for all slots upfront
        slots_copy.reserve(slots_.size());
        for (auto& kv : slots_) {
            slots_copy.push_back(kv.second);
        }
    }

    for (auto& slot : slots_copy) {
        std::lock_guard<std::mutex> lock(slot->mutex);
        if (!slot->meta.filter.replicate_ddl) {
            continue;
        }
        slot->buffer.push_back(ddl);
        std::lock_guard<std::mutex> slog(stats_mutex_);
        ++stats_.ddl_enqueued;
    }
}

void LogicalReplicationManager::onRoleChange(ReplicationRole, ReplicationRole) {}
void LogicalReplicationManager::onLeaderElected(const std::string&) {}
void LogicalReplicationManager::onReplicaAdded(const ReplicaInfo&) {}
void LogicalReplicationManager::onReplicaRemoved(const std::string&) {}
void LogicalReplicationManager::onConflictDetected(const std::string&) {}
void LogicalReplicationManager::onReplicationLagWarning(int64_t) {}
void LogicalReplicationManager::onReplicaHealthChanged(const std::string&, HealthStatus, HealthStatus) {}
void LogicalReplicationManager::onFailoverStarted(const std::string&, const std::string&) {}
void LogicalReplicationManager::onFailoverCompleted(const std::string&, bool) {}
void LogicalReplicationManager::onNetworkPartitionDetected(const std::vector<std::string>&) {}

void LogicalReplicationManager::onWALEntryApplied(const WALEntry& entry) {
    LogicalChange change = makeLogicalChange(entry);
    if (change.type == LogicalChange::Type::UNKNOWN) {
        return;
    }

    std::vector<std::shared_ptr<SlotRuntime>> slots_copy;
    {
        std::shared_lock<std::shared_mutex> lock(slots_mutex_);
        for (auto& kv : slots_) {
            slots_copy.push_back(kv.second);
        }
    }

    auto process_slot = [this, entry, change](const std::shared_ptr<SlotRuntime>& slot) -> std::pair<uint64_t, uint64_t> {
        auto slot_change = change;
        std::lock_guard<std::mutex> lock(slot->mutex);
        if (!slot->meta.filter.replicate_dml) {
            return {0, 0};
        }

        std::string doc_id = entry.document_id;
        if (doc_id.empty()) {
            doc_id = documentIdFromChange(slot_change);
        }
        if (slot->initial_sync_pending &&
            !doc_id.empty() &&
            slot->snapshot_keys.count(collectionKey(change.collection, doc_id)) &&
            entry.sequence_number <= slot->meta.restart_lsn) {
            return {0, 0};  // conflict-free initial sync: skip duplicates from snapshot
        }

        if (!matchesFilter(slot_change, slot->meta.filter)) {
            return {1u, 0u};
        }

        applyTransform(slot_change);
        slot->buffer.push_back(slot_change);
        return {0u, 1u};
    };

    auto process_slots_sequential = [&] {
        uint64_t total_filtered = 0;
        uint64_t total_enqueued = 0;
        for (auto& slot : slots_copy) {
            auto [f, e] = process_slot(slot);
            total_filtered += f;
            total_enqueued += e;
        }
        if (total_filtered || total_enqueued) {
            std::lock_guard<std::mutex> slog(stats_mutex_);
            stats_.filtered_out += total_filtered;
            stats_.changes_enqueued += total_enqueued;
        }
    };

    if (!config_.parallel_decoding || static_cast<int>(slots_copy.size()) <= 1) {
        process_slots_sequential();
        return;
    }

    const unsigned hw_threads = std::thread::hardware_concurrency();
    if (hw_threads <= 1) {
        THEMIS_WARN("Parallel decoding: hardware concurrency is unavailable or single-core, using sequential processing");
        process_slots_sequential();
        return;
    }
    const size_t worker_count = std::min<size_t>(slots_copy.size(), hw_threads - 1);
    std::exception_ptr worker_error = {};
    std::mutex worker_err_mutex = {};
    std::atomic<size_t> next_index{0};
    std::vector<std::thread> workers;
    workers.reserve(worker_count);
    for (size_t w = 0; w < worker_count; ++w) {
        workers.emplace_back([&next_index, &slots_copy, &process_slot, &worker_error, &worker_err_mutex, this] {
            uint64_t local_filtered = 0;
            uint64_t local_enqueued = 0;
            try {
                while (true) {
                    const size_t idx = next_index.fetch_add(1);
                    if (idx >= static_cast<int>(slots_copy.size())) {
                      break;
                    }
                    auto [f, e] = process_slot(slots_copy[idx]);
                    local_filtered += f;
                    local_enqueued += e;
                }
                if (local_filtered || local_enqueued) {  // avoid contended lock if no updates
                    std::lock_guard<std::mutex> slog(stats_mutex_);
                    stats_.filtered_out += local_filtered;
                    stats_.changes_enqueued += local_enqueued;
                }
            } catch (const std::exception& ex) {
                THEMIS_ERROR("Parallel decoding worker failed: {}", ex.what());
                std::lock_guard<std::mutex> elock(worker_err_mutex);
                if (!worker_error) {
                  worker_error = std::current_exception();
                }
            } catch (...) {
                THEMIS_ERROR("Parallel decoding worker failed with unknown exception");
                std::lock_guard<std::mutex> elock(worker_err_mutex);
                if (!worker_error) {
                  worker_error = std::current_exception();
                }
            }
        });
    }
    for (auto& t : workers) {
        t.join();
    }
    if (worker_error) {
        std::rethrow_exception(worker_error);
    }
}

LogicalChange LogicalReplicationManager::makeLogicalChange(const WALEntry& entry) const {
    LogicalChange change;
    const std::string op = entry.operation;
    if (op == "INSERT") {
      change.type = LogicalChange::Type::INSERT;
    }
    else if (op == "UPDATE") change.type = LogicalChange::Type::UPDATE;
    else if (op == "DELETE") change.type = LogicalChange::Type::DELETE;
    else if (op == "TRUNCATE") change.type = LogicalChange::Type::TRUNCATE;
    else if (op == "DDL") change.type = LogicalChange::Type::DDL;
    else change.type = LogicalChange::Type::UNKNOWN;

    change.collection = entry.collection;
    change.schema_version = config_.target_version;
    change.source_version = config_.source_version;
    change.target_version = config_.target_version;
    if (entry.sequence_number == 0 && wal_ && !missing_seq_warned_.exchange(true)) {
        THEMIS_WARN("Logical replication entry missing sequence_number; using current WAL sequence");
    }
    // Use WAL assigned sequence when available; fall back to current WAL position for notifications
    change.lsn = entry.sequence_number != 0 ? entry.sequence_number : (wal_ ? wal_->getCurrentSequence() : 0);
    change.timestamp = entry.timestamp.time_since_epoch().count() == 0
                           ? std::chrono::system_clock::now()
                           : entry.timestamp;

    auto parsed = nlohmann::json::parse(entry.data, nullptr, false);
    if (!parsed.is_discarded()) {
        if (change.type == LogicalChange::Type::DELETE) {
            change.old_data = parsed;
        } else {
            change.new_data = parsed;
        }
    }
    return change;
}

bool LogicalReplicationManager::matchesFilter(const LogicalChange& change,
                                              const ReplicationFilter& filter) const {
    if (change.type == LogicalChange::Type::UNKNOWN) {
        return false;
    }

    if (change.type == LogicalChange::Type::DDL) {
        return filter.replicate_ddl;
    }

    if (!filter.include_collections.empty()) {
        const bool included = std::find(filter.include_collections.begin(),
                                        filter.include_collections.end(),
                                        change.collection) != filter.include_collections.end();
        if (!included) {
          return false;
        }
    }

    if (std::find(filter.exclude_collections.begin(),
                  filter.exclude_collections.end(),
                  change.collection) != filter.exclude_collections.end()) {
        return false;
    }

    if (!filter.replicate_dml) {
        return false;
    }

    if (!filter.row_filter_expression.empty()) {
        const auto& payload = change.new_data.is_null() ? change.old_data : change.new_data;
        if (!payload.is_object()) {
          return false;
        }
        return evaluateRowFilter(filter.row_filter_expression, payload);
    }

    return true;
}

bool LogicalReplicationManager::evaluateRowFilter(const std::string& expression,
                                                  const nlohmann::json& payload) const {
    const auto eq_pos = expression.find("==");
    const auto ne_pos = expression.find("!=");
    bool equality = true;
    size_t pos = 0;
    if (eq_pos != std::string::npos) {
        pos = eq_pos;
        equality = true;
    } else if (ne_pos != std::string::npos) {
        pos = ne_pos;
        equality = false;
    } else {
        // Fail closed on unsupported expressions to avoid accidental broad replication
        THEMIS_WARN("Unsupported row filter expression rejected: {}", expression);
        return false;
    }

    std::string field = trimCopy(expression.substr(0, pos));
    std::string value = trimCopy(expression.substr(pos + 2));
    if (static_cast<int>(value.size()) >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, static_cast<int>(value.size()) - 2);
    }

    auto it = payload.find(field);
    if (it == payload.end()) {
      return false;
    }

    std::string actual = {};
    if (it->is_string()) {
        actual = it->get<std::string>();
    } else {
        actual = it->dump();
    }

    const bool matches = (actual == value);
    return equality ? matches : !matches;
}

void LogicalReplicationManager::applyTransform(LogicalChange& change) const {
    if (config_.transform) {
        config_.transform(change);
    }
}

std::string LogicalReplicationManager::documentIdFromChange(const LogicalChange& change) const {
    // Production Logic: Extract document ID from change data (new_data first, then old_data).
    // Searches for "document_id" first, then "_id" field in JSON objects.
    // Returns: document ID string if found; empty string ("") if not found or data is not an object.
    // Contract: Caller must check for empty return and treat as "no ID available" condition.
    
    if (change.new_data.is_object()) {
        if (change.new_data.contains("document_id")) {
            const auto& v = change.new_data["document_id"];
            return v.is_string() ? v.get<std::string>() : v.dump();
        }
        if (change.new_data.contains("_id")) {
            const auto& v = change.new_data["_id"];
            return v.is_string() ? v.get<std::string>() : v.dump();
        }
    }
    if (change.old_data.is_object()) {
        if (change.old_data.contains("document_id")) {
            const auto& v = change.old_data["document_id"];
            return v.is_string() ? v.get<std::string>() : v.dump();
        }
        if (change.old_data.contains("_id")) {
            const auto& v = change.old_data["_id"];
            return v.is_string() ? v.get<std::string>() : v.dump();
        }
    }
    THEMIS_DEBUG("LogicalReplicationManager::documentIdFromChange: no document id found in change; returning empty string");
    return {};  // Production behavior: empty string signals "ID not available"
}

void LogicalReplicationManager::loadPersistedSlots() {
    const auto base_path = slotStatePath("");
    if (base_path.empty()) {
        return;
    }

    std::error_code ec = {};
    const fs::path dir(base_path);
    if (!fs::exists(dir, ec)) {
        fs::create_directories(dir, ec);
        if (ec) {
            THEMIS_WARN("Failed to create logical slot directory {}: {}", dir.string(), ec.message());
        }
        fs::permissions(dir, fs::perms::owner_all, fs::perm_options::replace, ec);
        return;
    }

    for (fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator();) {
        const auto& entry = *it;
        std::error_code type_ec = {};
        if (!entry.is_regular_file(type_ec) || type_ec || entry.path().extension() != ".json") {
            it.increment(ec);
            if (ec) {
                THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
            }
            continue;
        }

        std::ifstream in(entry.path());
        if (!in.is_open()) {
            it.increment(ec);
            if (ec) {
                THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
            }
            continue;
        }
        nlohmann::json j;
        try {
            in >> j;
        } catch (const std::exception& ex) {
            THEMIS_WARN("Failed to parse logical slot file {}: {}", entry.path().string(), ex.what());
            it.increment(ec);
            if (ec) {
                THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
            }
            continue;
        }

        auto runtime = std::make_shared<SlotRuntime>();
        runtime->meta.slot_name = j.value("slot_name", entry.path().stem().string());
        runtime->meta.plugin_name = j.value("plugin_name", "json");
        runtime->meta.restart_lsn = j.value("restart_lsn", 0);
        runtime->meta.confirmed_flush_lsn = j.value("confirmed_flush_lsn", runtime->meta.restart_lsn);
        runtime->meta.initial_sync_pending = j.value("initial_sync_pending", false);
        runtime->initial_sync_pending = runtime->meta.initial_sync_pending;

        if (j.contains("filter") && j["filter"].is_object()) {
            const auto& jf = j["filter"];
            runtime->meta.filter.replicate_ddl = jf.value("replicate_ddl", true);
            runtime->meta.filter.replicate_dml = jf.value("replicate_dml", true);
            runtime->meta.filter.row_filter_expression = jf.value("row_filter_expression", "");
            if (jf.contains("include_collections") && jf["include_collections"].is_array()) {
                try {
                    runtime->meta.filter.include_collections =
                        jf["include_collections"].get<std::vector<std::string>>();
                } catch (...) {
                    THEMIS_WARN("Failed to parse include_collections for slot {}", runtime->meta.slot_name);
                    it.increment(ec);
                    if (ec) {
                        THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
                    }
                    continue;
                }
            }
            if (jf.contains("exclude_collections") && jf["exclude_collections"].is_array()) {
                try {
                    runtime->meta.filter.exclude_collections =
                        jf["exclude_collections"].get<std::vector<std::string>>();
                } catch (...) {
                    THEMIS_WARN("Failed to parse exclude_collections for slot {}", runtime->meta.slot_name);
                    it.increment(ec);
                    if (ec) {
                        THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
                    }
                    continue;
                }
            }
        }

        {
            std::lock_guard<std::mutex> slog(stats_mutex_);
            ++stats_.slots_loaded;
        }

        std::unique_lock<std::shared_mutex> lock(slots_mutex_);
        if (!slots_.count(runtime->meta.slot_name)) {
            slots_[runtime->meta.slot_name] = runtime;
        }

        it.increment(ec);
        if (ec) {
            THEMIS_WARN("Error iterating logical slot directory: {}", ec.message());
            break;
        }
    }
}

void LogicalReplicationManager::persistSlot(const SlotRuntime& slot) const {
    const auto state_path = slotStatePath(slot.meta.slot_name);
    if (state_path.empty()) {
        return;
    }

    fs::path base = fs::path(state_path).parent_path();
    std::error_code ec = {};
    fs::create_directories(base, ec);
    if (ec) {
        THEMIS_WARN("Failed to create slot directory {}: {}", base.string(), ec.message());
        return;
    }
    fs::permissions(base, fs::perms::owner_all, fs::perm_options::replace, ec);

    nlohmann::json j;
    j["slot_name"] = slot.meta.slot_name;
    j["plugin_name"] = slot.meta.plugin_name;
    j["restart_lsn"] = slot.meta.restart_lsn;
    j["confirmed_flush_lsn"] = slot.meta.confirmed_flush_lsn;
    j["initial_sync_pending"] = slot.meta.initial_sync_pending;
    j["filter"] = {
        {"include_collections", slot.meta.filter.include_collections},
        {"exclude_collections", slot.meta.filter.exclude_collections},
        {"row_filter_expression", slot.meta.filter.row_filter_expression},
        {"replicate_ddl", slot.meta.filter.replicate_ddl},
        {"replicate_dml", slot.meta.filter.replicate_dml},
    };

    const auto tmp_path = base / (slot.meta.slot_name + ".json.tmp");
    const std::string payload = j.dump(2);

    // WAVE1-FIX [no_timeout:647,702]: wrap all blocking filesystem operations
    // (open/write/fsync/rename/dir-fsync) in a single configurable deadline via
    // lrm_executeWithTimeout.  If the deadline fires the slot state is NOT
    // persisted this cycle — the caller logs the failure and replication
    // continues; the WAL provides a recovery path on restart.
    // All captures are by value so that the lambda is safe to run on a
    // detached thread if lrm_executeWithTimeout times out.
    const bool io_ok = lrm_executeWithTimeout(
        config_.file_io_timeout_ms,
        [tmp_path, payload, state_path, base, slot_name = slot.meta.slot_name]() mutable {
            std::error_code ec = {};
#ifdef _WIN32
            int fd = ::_open(tmp_path.string().c_str(),
                             _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                             _S_IREAD | _S_IWRITE);
#else
            int fd = ::open(tmp_path.c_str(),
                            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
#endif
            if (fd < 0) {
                THEMIS_WARN("Failed to open logical slot state {}: {}",
                            tmp_path.string(), strerror(errno));
                return;
            }

            const auto written =
#ifdef _WIN32
                ::_write(fd, payload.data(),
                         static_cast<unsigned int>(payload.size()));
#else
                ::write(fd, payload.data(),static_cast<int>(payload.size()));
#endif
            if (written < 0 || static_cast<size_t>(written) != payload.size()) {
#ifdef _WIN32
                ::_close(fd);
#else
                ::close(fd);
#endif
                fs::remove(tmp_path, ec);
                if (written < 0) {
                    THEMIS_WARN("Failed to persist logical slot {}: {}",
                                slot_name, strerror(errno));
                } else {
                    THEMIS_WARN("Failed to persist logical slot {}, "
                                "partial/failed write", slot_name);
                }
                return;
            }

#ifdef _WIN32
            if (::_commit(fd) != 0) {
#else
            if (::fsync(fd) != 0) {
#endif
#ifdef _WIN32
                ::_close(fd);
#else
                ::close(fd);
#endif
                fs::remove(tmp_path, ec);
                THEMIS_WARN("Failed to fsync logical slot {}", slot_name);
                return;
            }
#ifdef _WIN32
            ::_close(fd);
#else
            ::close(fd);
#endif

            fs::rename(tmp_path, state_path, ec);
            if (ec) {
                THEMIS_WARN("Failed to rename slot state file {} to {}: {}",
                            tmp_path.string(), state_path, ec.message());
                fs::remove(tmp_path, ec);
            }

#ifndef _WIN32
            int dir_fd = ::open(base.c_str(),
                                O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            if (dir_fd >= 0) {
                if (::fsync(dir_fd) != 0) {
                    THEMIS_WARN("Failed to fsync logical slot directory {}: {}",
                                base.string(), strerror(errno));
                }
                ::close(dir_fd);
            }
#endif
        }); // end lrm_executeWithTimeout

    if (!io_ok) {
        THEMIS_ERROR("LogicalReplicationManager::persistSlot: I/O timed out "
                     "or failed for slot '{}' (timeout={}ms) – slot state not "
                     "persisted this cycle; will retry on next advance",
                     slot.meta.slot_name, config_.file_io_timeout_ms);
    }
}

std::string LogicalReplicationManager::slotStatePath(const std::string& slot_name) const {
    // Production Logic: Generate filesystem path for logical replication slot state.
    // Returns: 
    //   - empty string if WAL directory not configured (prerequisite missing)
    //   - base directory path if slot_name is empty
    //   - full path to slot JSON file (base/slot_name.json) if slot_name provided
    // Contract: Caller must check for empty return when wal_directory is not configured.
    
    fs::path base = config_.wal_directory.empty()
                        ? fs::path()
                        : fs::path(config_.wal_directory) / "logical_slots";
    if (base.empty()) {
        THEMIS_WARN("LogicalReplicationManager::slotStatePath: wal_directory not configured; returning empty path");
        return {};  // Production behavior: empty string signals "configuration incomplete"
    }
    if (slot_name.empty()) {
      return base.string();
    }
    return (base / (slot_name + ".json")).string();
}

LogicalReplicationManager::Stats LogicalReplicationManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

std::string LogicalReplicationManager::collectionKey(const std::string& collection,
                                                     const std::string& document_id) {
    // Production Logic: Create composite key for document identification.
    // Returns: "collection:document_id" on valid inputs; empty string if either parameter is empty.
    // Contract: Caller must validate inputs before calling; empty return signals invalid parameters.
    
    if (collection.empty() || document_id.empty()) {
        THEMIS_WARN("LogicalReplicationManager::collectionKey: empty collection or document_id; returning empty key");
        return {};  // Production behavior: empty string signals "invalid inputs"
    }
    return collection + ":" + document_id;
}

}  // namespace replication
}  // namespace themisdb
