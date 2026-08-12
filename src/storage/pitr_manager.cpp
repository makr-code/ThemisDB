/**
 * @file pitr_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/pitr_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "transaction/snapshot_manager.h"
#include "utils/logger.h"
#include <chrono>
#include <algorithm>
#include <set>

namespace themis {

int64_t PITRManager::RestoreProgress::getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
}

PITRManager::PITRManager(RocksDBWrapper* db,
                        Changefeed* changefeed,
                        transaction::SnapshotManager* snapshot_mgr)
    : db_(db), changefeed_(changefeed), snapshot_mgr_(snapshot_mgr) {
    // uncaught_exception scanner alerts (lines 33, 36, 39): constructor throws
    // std::invalid_argument for null dependencies; callers must provide valid objects —
    // intentional API contract enforcement — false positives.
    if (!db_) {
        throw std::invalid_argument("PITRManager: db cannot be null");
    }
    if (!changefeed_) {
        throw std::invalid_argument("PITRManager: changefeed cannot be null");
    }
    if (!snapshot_mgr_) {
        throw std::invalid_argument("PITRManager: snapshot_mgr cannot be null");
    }
}

PITRManager::Status PITRManager::restoreToSequence(uint64_t target_sequence,
                                                   const RestoreOptions& options) {
    // Check if already in progress
    if (isRestoreInProgress()) {
        return Status::Error("Restore already in progress");
    }

    // Initialize progress
    progress_ = RestoreProgress{};
    progress_.phase = RestoreProgress::Phase::VALIDATING;
    progress_.start_time_ms = RestoreProgress::getCurrentTimeMs();

    uint64_t current_sequence = changefeed_->getLatestSequence();

    // Validate
    auto validate_status = validate(target_sequence, current_sequence);
    if (!validate_status.ok) {
        progress_.phase = RestoreProgress::Phase::FAILED;
        progress_.last_error = validate_status.message;
        return validate_status;
    }

    THEMIS_INFO("PITR restore initiated: target={}, current={}, dry_run={}",
                target_sequence, current_sequence, options.dry_run);

    // Create backup if enabled
    if (options.create_backup && !options.dry_run) {
        progress_.phase = RestoreProgress::Phase::CREATING_BACKUP;
        auto backup_status = createAutoBackup(options);
        if (!backup_status.ok) {
            progress_.phase = RestoreProgress::Phase::FAILED;
            progress_.last_error = backup_status.message;
            return backup_status;
        }
    }

    // Replay events backward
    progress_.phase = RestoreProgress::Phase::REPLAYING_EVENTS;
    auto replay_status = replayBackward(current_sequence, target_sequence, options);
    
    if (!replay_status.ok) {
        progress_.phase = RestoreProgress::Phase::FAILED;
        progress_.last_error = replay_status.message;
        THEMIS_ERROR("PITR restore failed: {}", replay_status.message);
        return replay_status;
    }

    // Success
    progress_.phase = RestoreProgress::Phase::COMPLETED;
    progress_.end_time_ms = RestoreProgress::getCurrentTimeMs();
    
    THEMIS_INFO("PITR restore completed: events_processed={}, elapsed={}ms",
                progress_.events_processed, progress_.getElapsedMs());

    return Status::WithProgress(progress_);
}

PITRManager::Status PITRManager::restoreToTag(const std::string& tag_name,
                                              const RestoreOptions& options) {
    // Get snapshot by tag
    // data_race scanner alert: snapshot_mgr_ is immutable after construction (set in
    // the PITRManager constructor and never reassigned).  SnapshotManager::getTag() is
    // internally thread-safe (uses its own lock).  The scanner incorrectly treats the
    // call-through-pointer as unsynchronised shared access.
    auto snapshot = snapshot_mgr_->getTag(tag_name);
    if (!snapshot.has_value()) {
        return Status::Error("Tag not found: " + tag_name);
    }

    THEMIS_INFO("PITR restore to tag: name={}, sequence={}",
                tag_name, snapshot->sequence_number);

    // Restore to the sequence from the snapshot
    return restoreToSequence(snapshot->sequence_number, options);
}

PITRManager::Status PITRManager::restoreToTimestamp(int64_t timestamp_ms,
                                                    const RestoreOptions& options) {
    // Find sequence for timestamp
    auto sequence = findSequenceForTimestamp(timestamp_ms);
    if (!sequence.has_value()) {
        return Status::Error("No events found at or before the given timestamp");
    }

    THEMIS_INFO("PITR restore to timestamp: time={}, sequence={}",
                timestamp_ms, sequence.value());

    // Restore to the found sequence
    return restoreToSequence(sequence.value(), options);
}

PITRManager::RestorePreview PITRManager::previewRestore(uint64_t target_sequence,
                                                        const RestoreOptions& options) const {
    RestorePreview preview{};
    preview.target_sequence = target_sequence;
    preview.current_sequence = changefeed_->getLatestSequence();
    
    if (target_sequence >= preview.current_sequence) {
        preview.events_to_replay = 0;
        preview.estimated_duration_sec = 0;
        return preview;
    }

    // Get events to replay
    Changefeed::ListOptions list_opts;
    list_opts.from_sequence = target_sequence;
    list_opts.limit = preview.current_sequence - target_sequence;

    auto events = changefeed_->listEvents(list_opts);
    preview.events_to_replay = events.size();

    // Analyze events
    std::set<std::string> tables;
    size_t total_size = 0;
    size_t keys_collected = 0;

    for (const auto& event : events) {
        // Extract table from key (assuming format: table:pk)
        auto colon_pos = event.key.find(':');
        if (colon_pos != std::string::npos) {
            std::string table = event.key.substr(0, colon_pos);
            
            // Apply table filter if provided
            // repeated_search scanner alerts (lines 167, 289): std::find on options.tables
            // is intentional; the list is typically empty or contains very few entries and
            // is not mutated during iteration — the O(n) linear scan is negligible and
            // caching is not warranted — false positives.
            if (options.tables.empty() || 
                std::find(options.tables.begin(), options.tables.end(), table) != options.tables.end()) {
                tables.insert(table);
                
                // Collect sample keys (first 100)
                if (keys_collected < 100) {
                    preview.affected_keys.push_back(event.key);
                    keys_collected++;
                }
            }
        }

        // Estimate size
        total_size += event.key.size();
        if (event.value.has_value()) {
            total_size += event.value->size();
        }
    }

    preview.affected_tables.assign(tables.begin(), tables.end());
    preview.estimated_size_bytes = total_size;
    
    // Estimate duration: ~1000 events/second (conservative estimate)
    preview.estimated_duration_sec = std::max<int64_t>(1, preview.events_to_replay / 1000);

    return preview;
}

std::optional<PITRManager::RestoreProgress> PITRManager::getProgress() const {
    if (progress_.phase == RestoreProgress::Phase::NOT_STARTED) {
        return std::nullopt;
    }
    return progress_;
}

bool PITRManager::isRestoreInProgress() const {
    return progress_.phase != RestoreProgress::Phase::NOT_STARTED &&
           progress_.phase != RestoreProgress::Phase::COMPLETED &&
           progress_.phase != RestoreProgress::Phase::FAILED &&
           progress_.phase != RestoreProgress::Phase::ROLLED_BACK;
}

std::optional<uint64_t> PITRManager::getSequenceForTag(const std::string& tag_name) const {
    // data_race scanner alert: same rationale as restoreToTag — snapshot_mgr_ is
    // immutable after construction and SnapshotManager::getTag() is thread-safe.
    auto snapshot = snapshot_mgr_->getTag(tag_name);
    if (snapshot.has_value()) {
        return snapshot->sequence_number;
    }
    return std::nullopt;
}

std::optional<uint64_t> PITRManager::getSequenceForTimestamp(int64_t timestamp_ms) const {
    return findSequenceForTimestamp(timestamp_ms);
}

std::optional<uint64_t> PITRManager::findSequenceForTimestamp(int64_t timestamp_ms) const {
    // Get all events
    auto events = changefeed_->listEvents();
    
    // Find the latest event with timestamp <= target timestamp
    uint64_t best_sequence = 0;
    bool found = false;
    
    for (const auto& event : events) {
        if (event.timestamp_ms <= timestamp_ms && event.sequence > best_sequence) {
            best_sequence = event.sequence;
            found = true;
        }
    }

    if (found) {
        return best_sequence;
    }
    return std::nullopt;
}

PITRManager::Status PITRManager::replayBackward(uint64_t from_sequence, uint64_t to_sequence,
                                                const RestoreOptions& options) {
    if (from_sequence <= to_sequence) {
        return Status::Error("Invalid replay range: from_sequence must be greater than to_sequence");
    }

    // Get events in range
    Changefeed::ListOptions list_opts;
    list_opts.from_sequence = to_sequence;
    list_opts.limit = from_sequence - to_sequence;
    if (options.max_events_to_replay > 0) {
        list_opts.limit = std::min(list_opts.limit, options.max_events_to_replay);
    }

    auto events = changefeed_->listEvents(list_opts);

    // Fail closed: PITR must not silently proceed with truncated WAL coverage
    // unless the caller explicitly requested a replay cap.
    if (options.max_events_to_replay == 0) {
        const auto expected_events = from_sequence - to_sequence;
        if (events.size() < expected_events) {
            return Status::Error(
                "WAL replay coverage incomplete: expected " +
                std::to_string(expected_events) +
                " event(s), found " + std::to_string(events.size()));
        }
    }

    progress_.total_events = events.size();
    progress_.events_processed = 0;

    THEMIS_INFO("Replaying {} events backward (from {} to {})",
                events.size(), from_sequence, to_sequence);

    // Reverse the events (replay backward)
    std::reverse(events.begin(), events.end());

    // Apply each event in reverse
    uint64_t replay_errors = 0;
    std::string first_replay_error;
    for (const auto& event : events) {
        // Apply table filter
        if (!options.tables.empty()) {
            auto colon_pos = event.key.find(':');
            if (colon_pos != std::string::npos) {
                std::string table = event.key.substr(0, colon_pos);
                if (std::find(options.tables.begin(), options.tables.end(), table) == options.tables.end()) {
                    progress_.events_processed++;
                    continue; // Skip this event
                }
            }
        }

        // Update current table in progress
        auto colon_pos = event.key.find(':');
        if (colon_pos != std::string::npos) {
            progress_.current_table = event.key.substr(0, colon_pos);
        }

        // Apply event in reverse (only if not dry-run)
        if (!options.dry_run) {
            auto status = applyEventReverse(event);
            if (!status.ok) {
                replay_errors++;
                if (first_replay_error.empty()) {
                    first_replay_error = status.message;
                }
                if (options.abort_on_first_error) {
                    THEMIS_ERROR("Failed to apply event {}: {}", event.sequence, status.message);
                    return status;
                } else {
                    THEMIS_WARN("Failed to apply event {} (continuing): {}", 
                               event.sequence, status.message);
                }
            }
        }

        progress_.events_processed++;
    }

    if (replay_errors > 0) {
        return Status::Error(
            "WAL replay encountered " + std::to_string(replay_errors) +
            " error(s); first error: " + first_replay_error);
    }

    return Status::OK();
}

PITRManager::Status PITRManager::applyEventReverse(const Changefeed::ChangeEvent& event) {
    // Reverse the operation
    switch (event.type) {
        case Changefeed::ChangeEventType::EVENT_PUT:
            // PUT → DELETE (remove the key)
            // delete_no_nullptr scanner alert (line 338): db_->del() is a method call on
            // RocksDBWrapper, not the delete operator; there is no raw pointer being
            // deleted here — false positive.
            // RocksDBWrapper::del() returns bool - true on success
            if (!db_->del(event.key)) {
                return Status::Error("Failed to delete key: " + event.key);
            }
            break;

        case Changefeed::ChangeEventType::EVENT_DELETE:
            // DELETE → PUT (restore the value)
            // Enforce strict recoverability: the previous value must be present
            // either in value or in before_snapshot.
            if (event.value.has_value()) {
                if (!db_->put(event.key, *event.value)) {
                    return Status::Error("Failed to restore deleted key from event value: " + event.key);
                }
                break;
            }
            if (event.before_snapshot.has_value()) {
                if (!db_->put(event.key, *event.before_snapshot)) {
                    return Status::Error("Failed to restore deleted key from before_snapshot: " + event.key);
                }
                break;
            }
            return Status::Error(
                "Cannot reverse DELETE without previous value (key=" + event.key + ")");
            break;

        case Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT:
        case Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK:
            // Metadata only, skip
            break;
    }

    return Status::OK();
}

PITRManager::Status PITRManager::createAutoBackup(const RestoreOptions& options) {
    // Create a snapshot tag for the current state
    auto snapshot = snapshot_mgr_->createTag(
        options.backup_tag,
        "Auto-backup before PITR restore",
        "pitr_manager"
    );

    if (!snapshot.has_value()) {
        return Status::Error("Failed to create auto-backup");
    }

    THEMIS_INFO("Auto-backup created: tag={}", options.backup_tag);
    return Status::OK();
}

PITRManager::Status PITRManager::validate(uint64_t target_sequence, 
                                         uint64_t current_sequence) const {
    if (target_sequence >= current_sequence) {
        return Status::Error("Target sequence must be less than current sequence");
    }

    if (target_sequence == 0 && current_sequence > 0) {
        // Restoring to sequence 0 means empty database
        THEMIS_WARN("Restoring to sequence 0 will result in an empty database");
    }

    return Status::OK();
}

void PITRManager::updateProgress(RestoreProgress::Phase phase, const std::string& message) {
    progress_.phase = phase;
    if (!message.empty()) {
        progress_.last_error = message;
    }
}

// Overloaded functions with default options
namespace {
    // Static default options to avoid repeated construction
    static const PITRManager::RestoreOptions kDefaultRestoreOptions{};
}

PITRManager::Status PITRManager::restoreToSequence(uint64_t target_sequence) {
    return restoreToSequence(target_sequence, kDefaultRestoreOptions);
}

PITRManager::Status PITRManager::restoreToTag(const std::string& tag_name) {
    return restoreToTag(tag_name, kDefaultRestoreOptions);
}

PITRManager::Status PITRManager::restoreToTimestamp(int64_t timestamp_ms) {
    return restoreToTimestamp(timestamp_ms, kDefaultRestoreOptions);
}

PITRManager::RestorePreview PITRManager::previewRestore(uint64_t target_sequence) const {
    return previewRestore(target_sequence, kDefaultRestoreOptions);
}

} // namespace themis
