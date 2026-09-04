/**
 * @file persistent_tensor_fingerprint_graph.cpp
 * @brief Persistent tensor fingerprint graph implementation.
 *
 * Implements fingerprint insertion, lookup, and RocksDB-backed
 * persistence for the tensor deduplication graph.
 */

#include "tensor/persistent_tensor_fingerprint_graph.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace themis::tensor {

namespace {

constexpr uint32_t kEntryFormatVersion = 1;
constexpr uint32_t kJournalFormatVersion = 1;

std::string makeJournalKey(const std::string& prefix) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return prefix + std::to_string(now);
}

bool deleteIfPresent(storage::ITensorStorageBackend& backend, const std::string& key) {
    if (backend.del(key)) {
        return true;
    }
    return !backend.get(key).has_value();
}

}  // namespace

PersistentTensorFingerprintGraph::PersistentTensorFingerprintGraph(
    std::shared_ptr<TensorFingerprintGraph> graph,
    std::shared_ptr<storage::ITensorStorageBackend> backend,
    std::string tenant_id,
    std::string domain)
    : graph_(std::move(graph)),
      backend_(std::move(backend)),
      tenant_id_(std::move(tenant_id)),
      domain_(std::move(domain)) {}

bool PersistentTensorFingerprintGraph::rehydrate() {
    if (!graph_ || !backend_) {
        return false;
    }

    if (!recoverJournal()) {
        return false;
    }

    for (const auto& key : graph_->adapterKeys()) {
        graph_->removeAdapter(key);
    }

    const auto keys = backend_->listKeys(entryPrefix());
    for (const auto& key : keys) {
        const auto persisted = readEntry(key);
        if (!persisted) {
            return false;
        }

        const auto train_opt = storage::TTTrain::deserialize(persisted->serialized_train);
        if (!train_opt) {
            return false;
        }

        if (!graph_->addAdapter(persisted->adapter_key,
                                *train_opt,
                                persisted->domain,
                                persisted->base_model_id,
                                persisted->tenant_id)) {
            return false;
        }
    }

    return true;
}

bool PersistentTensorFingerprintGraph::addAdapter(const std::string& adapter_key,
                                                  const storage::TTTrain& train,
                                                  const std::string& base_model_id) {
    if (!graph_ || !backend_ || adapter_key.empty()) {
        return false;
    }

    if (!graph_->addAdapter(adapter_key, train, domain_, base_model_id, tenant_id_)) {
        return false;
    }

    PersistedEntry entry;
    entry.adapter_key = adapter_key;
    entry.tenant_id = tenant_id_;
    entry.domain = domain_;
    entry.base_model_id = base_model_id;
    entry.serialized_train = train.serialize();

    if (!writeEntry(entry)) {
        graph_->removeAdapter(adapter_key);
        return false;
    }

    return true;
}

bool PersistentTensorFingerprintGraph::removeAdapter(const std::string& adapter_key) {
    if (!graph_ || !backend_ || adapter_key.empty()) {
        return false;
    }

    const auto existing = graph_->entry(adapter_key);
    if (!existing) {
        return false;
    }

    if (!graph_->removeAdapter(adapter_key)) {
        return false;
    }

    if (!deleteEntry(adapter_key)) {
        const auto train = existing->exact_train;
        graph_->addAdapter(adapter_key, train, existing->domain, existing->base_model_id, existing->tenant_id);
        return false;
    }

    return true;
}

std::string PersistentTensorFingerprintGraph::entryPrefix() const {
    return "__tfgp__:" + tenant_id_ + ":" + domain_ + ":entry:";
}

std::string PersistentTensorFingerprintGraph::entryKeyFor(const std::string& adapter_key) const {
    return entryPrefix() + adapter_key;
}

std::string PersistentTensorFingerprintGraph::journalPrefix() const {
    return "__tfgp__:" + tenant_id_ + ":" + domain_ + ":txn:";
}

bool PersistentTensorFingerprintGraph::writeEntry(const PersistedEntry& entry) {
    const auto key = entryKeyFor(entry.adapter_key);
    return writeWithJournal(JournalOp::Put, key, serializeEntry(entry));
}

bool PersistentTensorFingerprintGraph::deleteEntry(const std::string& adapter_key) {
    const auto key = entryKeyFor(adapter_key);
    return writeWithJournal(JournalOp::Delete, key, {});
}

std::optional<PersistentTensorFingerprintGraph::PersistedEntry>
PersistentTensorFingerprintGraph::readEntry(const std::string& key) const {
    const auto bytes = backend_->get(key);
    if (!bytes) {
        return std::nullopt;
    }
    return deserializeEntry(*bytes);
}

bool PersistentTensorFingerprintGraph::writeWithJournal(JournalOp op,
                                                        const std::string& target_key,
                                                        const std::vector<uint8_t>& payload) {
    const auto journal_key = makeJournalKey(journalPrefix());
    const auto journal_record = serializeJournalRecord(op, target_key, payload);
    if (!backend_->put(journal_key, journal_record)) {
        return false;
    }

    bool applied = false;
    if (op == JournalOp::Put) {
        applied = backend_->put(target_key, payload);
    } else {
        applied = deleteIfPresent(*backend_, target_key);
    }

    if (!applied) {
        return false;
    }

    return backend_->del(journal_key);
}

bool PersistentTensorFingerprintGraph::recoverJournal() {
    const auto journals = backend_->listKeys(journalPrefix());
    for (const auto& journal_key : journals) {
        const auto payload = backend_->get(journal_key);
        if (!payload) {
            return false;
        }

        JournalOp op = JournalOp::Put;
        std::string target_key = {};
        std::vector<uint8_t> target_payload = {};

        if (!deserializeJournalRecord(*payload, op, target_key, target_payload)) {
            return false;
        }

        bool applied = false;
        if (op == JournalOp::Put) {
            applied = backend_->put(target_key, target_payload);
        } else {
            applied = deleteIfPresent(*backend_, target_key);
        }

        if (!applied) {
            return false;
        }

        if (!backend_->del(journal_key)) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t>
PersistentTensorFingerprintGraph::serializeEntry(const PersistedEntry& entry) {
    std::vector<uint8_t> out;
    appendU32(out, kEntryFormatVersion);
    appendString(out, entry.adapter_key);
    appendString(out, entry.tenant_id);
    appendString(out, entry.domain);
    appendString(out, entry.base_model_id);
    appendBytes(out, entry.serialized_train);
    return out;
}

std::optional<PersistentTensorFingerprintGraph::PersistedEntry>
PersistentTensorFingerprintGraph::deserializeEntry(const std::vector<uint8_t>& bytes) {
    std::size_t off = 0;
    uint32_t version = 0;
    if (!readU32(bytes, off, version) || version != kEntryFormatVersion) {
        return std::nullopt;
    }

    PersistedEntry out = {};
    if (!readString(bytes, off, out.adapter_key) ||
        !readString(bytes, off, out.tenant_id) ||
        !readString(bytes, off, out.domain) ||
        !readString(bytes, off, out.base_model_id) ||
        !readBytes(bytes, off, out.serialized_train)) {
        return std::nullopt;
    }

    if (off != bytes.size()) {
        return std::nullopt;
    }
    return out;
}

std::vector<uint8_t>
PersistentTensorFingerprintGraph::serializeJournalRecord(JournalOp op,
                                                         const std::string& target_key,
                                                         const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out;
    appendU32(out, kJournalFormatVersion);
    out.push_back(static_cast<uint8_t>(op));
    appendString(out, target_key);
    appendBytes(out, payload);
    return out;
}

bool PersistentTensorFingerprintGraph::deserializeJournalRecord(const std::vector<uint8_t>& bytes,
                                                                JournalOp& op,
                                                                std::string& target_key,
                                                                std::vector<uint8_t>& payload) {
    std::size_t off = 0;
    uint32_t version = 0;
    if (!readU32(bytes, off, version) || version != kJournalFormatVersion) {
        return false;
    }
    if (off >= static_cast<int>(bytes.size())) {
        return false;
    }

    const auto op_raw = bytes[off++];
    if (op_raw != static_cast<uint8_t>(JournalOp::Put) && op_raw != static_cast<uint8_t>(JournalOp::Delete)) {
        return false;
    }
    op = static_cast<JournalOp>(op_raw);

    if (!readString(bytes, off, target_key) || !readBytes(bytes, off, payload)) {
        return false;
    }

    return off == bytes.size();
}

void PersistentTensorFingerprintGraph::appendU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<uint8_t>((v >> 24u) & 0xffu));
}

bool PersistentTensorFingerprintGraph::readU32(const std::vector<uint8_t>& in, std::size_t& off, uint32_t& v) {
    if (off + 4 > in.size()) {
        return false;
    }
    v = static_cast<uint32_t>(in[off]) |
        (static_cast<uint32_t>(in[off + 1]) << 8u) |
        (static_cast<uint32_t>(in[off + 2]) << 16u) |
        (static_cast<uint32_t>(in[off + 3]) << 24u);
    off += 4;
    return true;
}

void PersistentTensorFingerprintGraph::appendString(std::vector<uint8_t>& out, const std::string& v) {
    appendU32(out, static_cast<uint32_t>(v.size()));
    out.insert(out.end(), v.begin(), v.end());
}

bool PersistentTensorFingerprintGraph::readString(const std::vector<uint8_t>& in,
                                                  std::size_t& off,
                                                  std::string& v) {
    uint32_t size = 0;
    if (!readU32(in, off, size)) {
        return false;
    }
    if (off + size > in.size()) {
        return false;
    }
    v.assign(reinterpret_cast<const char*>(in.data() + off), size);
    off += size;
    return true;
}

void PersistentTensorFingerprintGraph::appendBytes(std::vector<uint8_t>& out, const std::vector<uint8_t>& v) {
    appendU32(out, static_cast<uint32_t>(v.size()));
    out.insert(out.end(), v.begin(), v.end());
}

bool PersistentTensorFingerprintGraph::readBytes(const std::vector<uint8_t>& in,
                                                 std::size_t& off,
                                                 std::vector<uint8_t>& v) {
    uint32_t size = 0;
    if (!readU32(in, off, size)) {
        return false;
    }
    if (off + size > in.size()) {
        return false;
    }
    v.assign(in.begin() + static_cast<std::ptrdiff_t>(off),
             in.begin() + static_cast<std::ptrdiff_t>(off + size));
    off += size;
    return true;
}

}  // namespace themis::tensor
