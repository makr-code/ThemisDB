/**
 * @file outbox.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/outbox.h"

#include <chrono>
#include <cstdio>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace cdc {

// ============================================================
// OutboxRecord JSON serialisation
// ============================================================

static std::string outboxStateToString(OutboxState s) {
    switch (s) {
        case OutboxState::PENDING:
            return "PENDING";
        case OutboxState::PUBLISHED:
            return "PUBLISHED";
        case OutboxState::FAILED:
            return "FAILED";
    }
    return "PENDING";
}

static OutboxState outboxStateFromString(const std::string &s) {
    if (s == "PUBLISHED") {
        return OutboxState::PUBLISHED;
    }
    if (s == "FAILED") {
        return OutboxState::FAILED;
    }
    return OutboxState::PENDING;
}

static std::string changeEventTypeToString(Changefeed::ChangeEventType t) {
    switch (t) {
        case Changefeed::ChangeEventType::EVENT_PUT:
            return "PUT";
        case Changefeed::ChangeEventType::EVENT_DELETE:
            return "DELETE";
        case Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT:
            return "TRANSACTION_COMMIT";
        case Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK:
            return "TRANSACTION_ROLLBACK";
    }
    return "PUT";
}

static Changefeed::ChangeEventType changeEventTypeFromString(const std::string &s) {
    if (s == "DELETE") {
        return Changefeed::ChangeEventType::EVENT_DELETE;
    }
    if (s == "TRANSACTION_COMMIT") {
        return Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    }
    if (s == "TRANSACTION_ROLLBACK") {
        return Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    }
    return Changefeed::ChangeEventType::EVENT_PUT;
}

nlohmann::json OutboxRecord::toJson() const {
    nlohmann::json j;
    j["outbox_sequence"] = outbox_sequence;
    j["collection"]      = collection;
    j["key"]             = key;
    if (value.has_value()) {
        j["value"] = *value;
    } else {
        j["value"] = nullptr;
    }
    j["event_type"]      = changeEventTypeToString(event_type);
    j["state"]           = outboxStateToString(state);
    j["created_at_ms"]   = created_at_ms;
    j["published_at_ms"] = published_at_ms;
    j["relay_attempts"]  = relay_attempts;
    j["failure_reason"]  = failure_reason;
    j["metadata"]        = metadata;
    return j;
}

OutboxRecord OutboxRecord::fromJson(const nlohmann::json &j) {
    OutboxRecord r;
    r.outbox_sequence = j.value("outbox_sequence", uint64_t{0});
    r.collection      = j.value("collection", std::string{});
    r.key             = j.value("key", std::string{});
    if (j.contains("value") && !j["value"].is_null()) {
        r.value = j["value"].get<std::string>();
    }
    r.event_type      = changeEventTypeFromString(j.value("event_type", std::string{"PUT"}));
    r.state           = outboxStateFromString(j.value("state", std::string{"PENDING"}));
    r.created_at_ms   = j.value("created_at_ms", int64_t{0});
    r.published_at_ms = j.value("published_at_ms", int64_t{0});
    r.relay_attempts  = j.value("relay_attempts", 0);
    r.failure_reason  = j.value("failure_reason", std::string{});
    if (j.contains("metadata")) {
        r.metadata = j["metadata"];
    }
    return r;
}

// ============================================================
// OutboxWriter helpers
// ============================================================

OutboxWriter::OutboxWriter(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf) : db_(db), cf_(cf) {
    if (!db_) {
        throw error::invalidArgument("OutboxWriter: db cannot be null");
    }
}

std::string OutboxWriter::makeKey([[maybe_unused]] uint64_t seq) const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));
    return std::string(buf);
}

uint64_t OutboxWriter::nextSequence() {
    std::lock_guard<std::mutex> lock(sequence_mutex_);

    rocksdb::ReadOptions read_opts;
    std::string seq_value = {};
    rocksdb::Status s;

    if (cf_) {
        s = db_->Get(read_opts, cf_, SEQUENCE_KEY, &seq_value);
    } else {
        s = db_->Get(read_opts, SEQUENCE_KEY, &seq_value);
    }

    uint64_t next = 1;
    if (s.ok()) {
        try {
            next = std::stoull(seq_value) + 1;
        } catch (const std::string&) {
            next = 1;
        } catch (const char*) {
            next = 1;
        } catch (...) {
            next = 1;
        }
    }

    rocksdb::WriteOptions write_opts;
    std::string next_str = std::to_string(next);
    rocksdb::Status ws;
    if (cf_) {
        ws = db_->Put(write_opts, cf_, SEQUENCE_KEY, next_str);
    } else {
        ws = db_->Put(write_opts, SEQUENCE_KEY, next_str);
    }
    if (!ws.ok()) {
        throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::CRITICAL,
                           "OutboxWriter: failed to persist sequence counter", ws.ToString());
    }
    return next;
}

// ============================================================
// OutboxWriter::writeToOutbox
// ============================================================

OutboxRecord &OutboxWriter::writeToOutbox(rocksdb::Transaction *txn, OutboxRecord &rec) {
    if (!txn) {
        throw error::invalidArgument("OutboxWriter::writeToOutbox: txn cannot be null");
    }
    if (rec.key.empty()) {
        throw error::invalidArgument("OutboxWriter::writeToOutbox: record key cannot be empty");
    }

    rec.outbox_sequence = nextSequence();
    rec.state           = OutboxState::PENDING;
    rec.relay_attempts  = 0;
    rec.published_at_ms = 0;

    auto now = std::chrono::system_clock::now();
    rec.created_at_ms
        = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());

    std::string serialised = rec.toJson().dump();
    std::string db_key     = makeKey(rec.outbox_sequence);

    rocksdb::Status s;
    if (cf_) {
        s = txn->Put(cf_, db_key, serialised);
    } else {
        s = txn->Put(db_key, serialised);
    }

    if (!s.ok()) {
        throw CDCException(ErrorCode::DB_WRITE_FAILED, ErrorSeverity::ERROR,
                           "OutboxWriter: failed to write outbox record", s.ToString());
    }

    THEMIS_DEBUG("OutboxWriter: enqueued record seq={} key={}", rec.outbox_sequence, rec.key);
    return rec;
}

// ============================================================
// OutboxRelay helpers
// ============================================================

OutboxRelay::OutboxRelay(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf, Changefeed &changefeed,
                         OutboxRelayConfig config)
    : db_(db), cf_(cf), changefeed_(changefeed), config_(std::move(config)) {
    if (!db_) {
        throw error::invalidArgument("OutboxRelay: db cannot be null");
    }
}

OutboxRelay::~OutboxRelay() {
    stop();
}

std::string OutboxRelay::makeKey([[maybe_unused]] uint64_t seq) const {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%020llu", KEY_PREFIX, static_cast<unsigned long long>(seq));
    return std::string(buf);
}

void OutboxRelay::updateRecord(const OutboxRecord &rec) {
    std::string db_key     = makeKey(rec.outbox_sequence);
    std::string serialised = rec.toJson().dump();

    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    if (cf_) {
        s = db_->Put(write_opts, cf_, db_key, serialised);
    } else {
        s = db_->Put(write_opts, db_key, serialised);
    }
    if (!s.ok()) {
        THEMIS_WARN("OutboxRelay: failed to update record seq={}: {}", rec.outbox_sequence, s.ToString());
    }
}

std::vector<OutboxRecord> OutboxRelay::scanRecords(size_t limit, OutboxState filter_state, bool all_states) const {
    std::vector<OutboxRecord> result;

    rocksdb::ReadOptions read_opts;
    std::unique_ptr<rocksdb::Iterator> it(cf_ ? db_->NewIterator(read_opts, cf_) : db_->NewIterator(read_opts));

    it->Seek(KEY_PREFIX);
    for (; it->Valid(); it->Next()) {
        rocksdb::Slice k = it->key();
        if (!k.starts_with(KEY_PREFIX)) {
            break;
        }

        try {
            OutboxRecord rec = OutboxRecord::fromJson(nlohmann::json::parse(it->value().ToString()));

            if (all_states || rec.state == filter_state) {
                result.push_back(std::move(rec));
                if (limit > 0 && result.size() >= limit) {
                    break;
                }
            }
        } catch (const std::exception &e) {
            THEMIS_WARN("OutboxRelay: failed to parse outbox record at key={}: {}", k.ToString(), e.what());
        }
    }

    return result;
}

// ============================================================
// OutboxRelay::relayOnce
// ============================================================

size_t OutboxRelay::relayOnce() {
    std::vector<OutboxRecord> pending = scanRecords(config_.batch_size, OutboxState::PENDING, /*all_states=*/false);

    size_t published = 0;
    for (OutboxRecord &rec : pending) {
        rec.relay_attempts++;

        try {
            Changefeed::ChangeEvent event;
            event.type     = rec.event_type;
            event.key      = rec.key;
            event.value    = rec.value;
            event.metadata = rec.metadata;
            if (!rec.collection.empty()) {
                event.metadata["collection"] = rec.collection;
            }
            event.timestamp_ms = rec.created_at_ms;

            changefeed_.recordEvent(event);

            rec.state           = OutboxState::PUBLISHED;
            auto now            = std::chrono::system_clock::now();
            rec.published_at_ms = static_cast<int64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());

            updateRecord(rec);
            ++published;
            total_relayed_.fetch_add(1, std::memory_order_relaxed);

            THEMIS_DEBUG("OutboxRelay: published record seq={} key={}", rec.outbox_sequence, rec.key);

        } catch (const std::exception &e) {
            rec.failure_reason = e.what();

            bool exhausted = (config_.max_relay_attempts > 0 && rec.relay_attempts >= config_.max_relay_attempts);
            if (exhausted) {
                rec.state = OutboxState::FAILED;
                total_failed_.fetch_add(1, std::memory_order_relaxed);
                THEMIS_WARN("OutboxRelay: record seq={} failed permanently after {} attempts: {}", rec.outbox_sequence,
                            rec.relay_attempts, e.what());
            } else {
                THEMIS_WARN("OutboxRelay: record seq={} relay attempt {} failed (will retry): {}", rec.outbox_sequence,
                            rec.relay_attempts, e.what());
            }

            updateRecord(rec);
        }
    }

    return published;
}

// ============================================================
// OutboxRelay background thread
// ============================================================

void OutboxRelay::relayThreadFunc() {
    while (running_.load(std::memory_order_acquire)) {
        relayOnce();

        std::unique_lock<std::mutex> lock(cv_mutex_);
        cv_.wait_for(lock, config_.poll_interval, [this] { return !running_.load(std::memory_order_acquire); });
    }
}

void OutboxRelay::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return; // already running
    }
    relay_thread_ = std::thread(&OutboxRelay::relayThreadFunc, this);
    THEMIS_INFO("OutboxRelay: started (poll_interval={}ms)", config_.poll_interval.count());
}

void OutboxRelay::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return; // was not running
    }
    cv_.notify_all();
    if (relay_thread_.joinable()) {
        relay_thread_.join();
    }
    THEMIS_INFO("OutboxRelay: stopped");
}

// ============================================================
// OutboxRelay query / maintenance helpers
// ============================================================

std::vector<OutboxRecord> OutboxRelay::listRecords(OutboxState state, size_t limit) const {
    return scanRecords(limit, state, /*all_states=*/false);
}

std::vector<OutboxRecord> OutboxRelay::listAllRecords([[maybe_unused]] size_t limit) const {
    return scanRecords(limit, OutboxState::PENDING, /*all_states=*/true);
}

bool OutboxRelay::removeRecord([[maybe_unused]] uint64_t outbox_sequence) {
    std::string db_key = makeKey(outbox_sequence);
    rocksdb::WriteOptions write_opts;
    rocksdb::Status s;
    if (cf_) {
        s = db_->Delete(write_opts, cf_, db_key);
    } else {
        s = db_->Delete(write_opts, db_key);
    }
    return s.ok();
}

size_t OutboxRelay::purgePublished() {
    auto published = scanRecords(0, OutboxState::PUBLISHED, /*all_states=*/false);
    size_t removed = 0;
    for (const auto &rec : published) {
        if (removeRecord(rec.outbox_sequence)) {
            ++removed;
        }
    }
    return removed;
}

uint64_t OutboxRelay::totalRelayed() const {
    return total_relayed_.load(std::memory_order_relaxed);
}

uint64_t OutboxRelay::totalFailed() const {
    return total_failed_.load(std::memory_order_relaxed);
}

} // namespace cdc
} // namespace themis

