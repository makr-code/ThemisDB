/**
 * @file dead_letter_queue.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/dead_letter_queue.h"
#include "cdc/cdc_error.h"
#include "utils/logger.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <chrono>
#include <cstdio>

namespace themis {
namespace cdc {

// ===== DLQEntry JSON Serialization =====

nlohmann::json DLQEntry::toJson() const {
    return {
        {"dlq_sequence",    dlq_sequence},
        {"event",           event.toJson()},
        {"failure_reason",  failure_reason},
        {"attempt_count",   attempt_count},
        {"enqueued_at_ms",  enqueued_at_ms}
    };
}

DLQEntry DLQEntry::fromJson(const nlohmann::json& j) {
    DLQEntry entry;
    entry.dlq_sequence    = j.value("dlq_sequence",   uint64_t(0));
    entry.failure_reason  = j.value("failure_reason", std::string{});
    entry.attempt_count   = j.value("attempt_count",  0);
    entry.enqueued_at_ms  = j.value("enqueued_at_ms", int64_t(0));
    if (j.contains("event")) {
        entry.event = Changefeed::ChangeEvent::fromJson(j["event"]);
    }
    return entry;
}

// ===== DeadLetterQueue Implementation =====

DeadLetterQueue::DeadLetterQueue(rocksdb::TransactionDB* db,
                                 rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {
    if (!db_) {
        throw std::invalid_argument("DeadLetterQueue: db cannot be null");
    }
}

std::string DeadLetterQueue::makeKey([[maybe_unused]] uint64_t dlq_sequence) const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%020llu",
                  KEY_PREFIX, static_cast<unsigned long long>(dlq_sequence));
    return std::string(buf);
}

uint64_t DeadLetterQueue::nextSequence() {
    std::lock_guard<std::mutex> lock(sequence_mutex_);

    std::string seq_value = {};
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
    } else {
        s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
    }

    uint64_t next_seq = 1;
    if (s.ok() && !seq_value.empty()) {
        next_seq = std::stoull(seq_value) + 1;
    }

    rocksdb::WriteOptions write_opts;
    const std::string next_seq_str = std::to_string(next_seq);
    rocksdb::Status write_status;

    if (cf_) {
        write_status = db_->Put(write_opts, cf_, SEQUENCE_KEY, next_seq_str);
    } else {
        write_status = db_->Put(write_opts, SEQUENCE_KEY, next_seq_str);
    }

    if (!write_status.ok()) {
        throw error::sequenceGenerationFailed(write_status.ToString());
    }

    return next_seq;
}

DLQEntry DeadLetterQueue::enqueue(const Changefeed::ChangeEvent& event,
                                  const std::string& failure_reason,
                                  int attempt_count) {
    DLQEntry entry;
    entry.dlq_sequence   = nextSequence();
    entry.event          = event;
    entry.failure_reason = failure_reason;
    entry.attempt_count  = attempt_count;
    entry.enqueued_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    const std::string key   = makeKey(entry.dlq_sequence);
    const std::string value = entry.toJson().dump();

    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Put(write_opts, cf_, key, value);
    } else {
        s = db_->Put(write_opts, key, value);
    }

    if (!s.ok()) {
        THEMIS_ERROR("DeadLetterQueue: failed to enqueue event (key={}): {}",
                     event.key, s.ToString());
        throw error::dbOperationFailed("DLQ enqueue", s.ToString());
    }

    THEMIS_WARN("DeadLetterQueue: enqueued event (key={}, dlq_seq={}, attempts={}): {}",
                event.key, entry.dlq_sequence, attempt_count, failure_reason);
    return entry;
}

DLQEntry DeadLetterQueue::getEntry([[maybe_unused]] uint64_t dlq_sequence) const {
    const std::string key = makeKey(dlq_sequence);
    std::string value = {};
    rocksdb::ReadOptions read_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(read_opts, cf_, key, &value);
    } else {
        s = db_->Get(read_opts, key, &value);
    }

    if (!s.ok()) {
        throw error::invalidArgument("dlq_sequence",
                                     "DLQ entry not found: " + std::to_string(dlq_sequence));
    }

    return DLQEntry::fromJson(nlohmann::json::parse(value));
}

Changefeed::ChangeEvent DeadLetterQueue::replay(uint64_t dlq_sequence,
                                                 Changefeed& changefeed) {
    DLQEntry entry = getEntry(dlq_sequence);

    // Reset sequence so the Changefeed assigns a fresh one
    entry.event.sequence = 0;

    Changefeed::ChangeEvent recorded = changefeed.recordEvent(entry.event);

    // Remove from DLQ on successful replay
    if (!remove(dlq_sequence)) {
        THEMIS_WARN("DeadLetterQueue: replay succeeded but remove failed for dlq_seq={}",
                    dlq_sequence);
    }

    THEMIS_INFO("DeadLetterQueue: replayed dlq_seq={} → new seq={}",
                dlq_sequence, recorded.sequence);
    return recorded;
}

bool DeadLetterQueue::remove([[maybe_unused]] uint64_t dlq_sequence) {
    const std::string key = makeKey(dlq_sequence);
    rocksdb::ReadOptions read_opts;
    std::string existing = {};
    rocksdb::Status read_status;

    if (cf_) {
        read_status = db_->Get(read_opts, cf_, key, &existing);
    } else {
        read_status = db_->Get(read_opts, key, &existing);
    }

    if (read_status.IsNotFound()) {
        return false;
    }
    if (!read_status.ok()) {
        THEMIS_ERROR("DeadLetterQueue: failed to read dlq_seq={} before remove: {}",
                     dlq_sequence, read_status.ToString());
        return false;
    }

    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;

    if (cf_) {
        s = db_->Delete(write_opts, cf_, key);
    } else {
        s = db_->Delete(write_opts, key);
    }

    if (s.ok()) {
        THEMIS_DEBUG("DeadLetterQueue: removed dlq_seq={}", dlq_sequence);
        return true;
    }

    THEMIS_ERROR("DeadLetterQueue: failed to remove dlq_seq={}: {}",
                 dlq_sequence, s.ToString());
    return false;
}

size_t DeadLetterQueue::drain() {
    rocksdb::ReadOptions  read_opts;
    rocksdb::WriteOptions write_opts;

    std::unique_ptr<rocksdb::Iterator> it = {};

    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }

    const std::string prefix_start = makeKey(0);
    it->Seek(prefix_start);

    size_t deleted = 0;
    while (it->Valid()) {
        const std::string key = it->key().ToString();
        if (key.compare(0, std::strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }

        rocksdb::Status s;
        if (cf_) {
            s = db_->Delete(write_opts, cf_, key);
        } else {
            s = db_->Delete(write_opts, key);
        }

        if (s.ok()) {
            ++deleted;
        } else {
            THEMIS_WARN("DeadLetterQueue: drain failed to delete key={}: {}",
                        key, s.ToString());
        }

        it->Next();
    }

    THEMIS_INFO("DeadLetterQueue: drained {} entries", deleted);
    return deleted;
}

std::vector<DLQEntry> DeadLetterQueue::listEntries([[maybe_unused]] size_t limit) const {
    std::vector<DLQEntry> results;

    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;

    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }

    const std::string start = makeKey(0);
    it->Seek(start);

    while (it->Valid()) {
        const std::string key = it->key().ToString();
        if (key.compare(0, std::strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }

        try {
            DLQEntry entry = DLQEntry::fromJson(
                nlohmann::json::parse(it->value().ToString()));
            results.push_back(std::move(entry));
        } catch (const std::exception& e) {
            THEMIS_WARN("DeadLetterQueue: failed to parse entry at key={}: {}",
                        key, e.what());
        }

        if (limit > 0 && results.size() >= limit) {
            break;
        }
        it->Next();
    }

    return results;
}

size_t DeadLetterQueue::size() const {
    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it;

    if (cf_) {
        it.reset(db_->NewIterator(read_opts, cf_));
    } else {
        it.reset(db_->NewIterator(read_opts));
    }

    const std::string start = makeKey(0);
    it->Seek(start);

    size_t count = 0;
    while (it->Valid()) {
        const std::string key = it->key().ToString();
        if (key.compare(0, std::strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            break;
        }
        ++count;
        it->Next();
    }

    return count;
}

} // namespace cdc
} // namespace themis
