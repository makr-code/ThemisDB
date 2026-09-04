/**
 * @file tensor_deduplication_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/tensor_deduplication_manager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include "index/graph_index.h"
#include "storage/base_entity.h"
#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"
#include "utils/logger.h"

namespace themis {
namespace graph {

using storage::TensorFieldKey;
using storage::TensorTrainConfig;
using storage::TensorTrainDecomposer;
using storage::TTTrain;

// ============================================================================
// Construction
// ============================================================================

TensorDeduplicationManager::TensorDeduplicationManager(std::shared_ptr<storage::TensorNetworkStorageEngine> storage,
                                                       std::shared_ptr<TensorFingerprintGraph> fp_graph,
                                                       std::shared_ptr<TensorTrainDecomposer> decomposer,
                                                       const DeduplicationConfig &cfg)
    : storage_(std::move(storage)), fp_graph_(std::move(fp_graph)), decomposer_(std::move(decomposer)), cfg_(cfg) {
    if (!storage_ || !fp_graph_ || !decomposer_) {
        throw std::invalid_argument("TensorDeduplicationManager: null dependency");
    }

    // Auto-wire per-entry mutation journaling to raw storage metadata so
    // post-snapshot updates rewrite only one tensor-specific journal record.
    // This keeps write amplification bounded while restoreGraph() remains
    // backward-compatible with older blob-only journals.
    setJournalEntryHooks(
        [storage = storage_](std::string_view snapshot_key, std::string_view tensor_id,
                             const std::vector<uint8_t> &payload) {
            if (!storage) {
                return false;
            }
            return storage->putRawMetadata(
                std::string{"__tfgjournal__:"} + std::string{snapshot_key} + ":" + std::string{tensor_id}, payload);
        },
        [storage = storage_](std::string_view snapshot_key, std::string_view tensor_id) {
            if (!storage) {
                return false;
            }
            return storage->deleteRawMetadata(std::string{"__tfgjournal__:"} + std::string{snapshot_key} + ":"
                                              + std::string{tensor_id});
        },
        [storage = storage_](std::string_view snapshot_key,
                             std::function<void(std::string_view, const std::vector<uint8_t> &)> cb) {
            if (!storage || !cb) {
                return;
            }
            const auto prefix = std::string{"__tfgjournal__:"} + std::string{snapshot_key} + ":";
            for (const auto &tensor_id_suffix : storage->listRawMetadataKeys(prefix)) {
                // listRawMetadataKeys(prefix) returns suffixes relative to @p prefix,
                // while getRawMetadata() expects the full logical key.
                const auto payload = storage->getRawMetadata(prefix + tensor_id_suffix);
                if (!payload.has_value()) {
                    continue;
                }
                cb(tensor_id_suffix, *payload);
            }
        },
        [storage = storage_](std::string_view snapshot_key) {
            if (!storage) {
                return false;
            }
            bool ok           = true;
            const auto prefix = std::string{"__tfgjournal__:"} + std::string{snapshot_key} + ":";
            for (const auto &key : storage->listRawMetadataKeys(prefix)) {
                const auto deleted = storage->deleteRawMetadata(prefix + key);
                ok                 = deleted && ok;
            }
            return ok;
        });

    fp_graph_->setTrainLoadFn([storage = storage_](const std::string &, const std::string &tenant,
                                                   const std::string &collection,
                                                   const std::string &field) -> std::optional<TTTrain> {
        if (!storage) {
            return std::nullopt;
        }
        auto qtrain = storage->getCompressed({tenant, collection, field});
        if (!qtrain.has_value()) {
            return std::nullopt;
        }
        storage::TTQuantizer quantizer;
        return quantizer.dequantize(*qtrain);
    });

    storage_->setWriteObserverFn([this](const TensorFieldKey &key, const TTTrain &train) {
        std::string tensor_id = {};
        std::optional<StoredTensorRecord> record;
        std::size_t total_bytes_stored = 0;
        std::size_t bytes_saved        = 0;
        {
            std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
            auto it = key_to_tensor_id_.find(makeKeyIndex(key));
            if (it == key_to_tensor_id_.end()) {
                return;
            }
            tensor_id      = it->second;
            auto record_it = records_.find(tensor_id);
            if (record_it == records_.end()) {
                return;
            }
            record             = record_it->second;
            total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
            bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
        }
        fp_graph_->insert(tensor_id, train, key.tenant, key.collection, key.field);
        persistUpsertJournalEntry(*record, total_bytes_stored, bytes_saved);
    });

    storage_->setDeleteObserverFn([[maybe_unused]] [this](const TensorFieldKey &key) {
        std::string tensor_id = {};
        std::size_t total_bytes_stored  = 0;
        std::size_t bytes_saved         = 0;
        const auto fetchSubAndGetResult = [](std::atomic<std::size_t> &counter, std::size_t value) {
            return counter.fetch_sub(value, std::memory_order_relaxed) - value;
        };
        {
            std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
            auto it = key_to_tensor_id_.find(makeKeyIndex(key));
            if (it == key_to_tensor_id_.end()) {
                return;
            }
            tensor_id            = it->second;
            const auto record_it = records_.find(tensor_id);
            if (record_it != records_.end()) {
                const auto &record = record_it->second;
                total_bytes_stored = fetchSubAndGetResult(total_bytes_stored_, record.compressed_bytes);
                bytes_saved        = fetchSubAndGetResult(bytes_saved_, record.saved_bytes);
            } else {
                total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
                bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
            }
            clearMappingForTensorIdLocked(tensor_id);
            records_.erase(tensor_id);
        }
        fp_graph_->remove(tensor_id);
        persistDeleteJournalEntry(tensor_id, total_bytes_stored, bytes_saved);
    });
}

TensorDeduplicationManager::~TensorDeduplicationManager() {
    if (storage_) {
        storage_->setWriteObserverFn([[maybe_unused]] nullptr);
        storage_->setDeleteObserverFn([[maybe_unused]] nullptr);
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

TensorFieldKey TensorDeduplicationManager::makeKey(const std::string &tenant, const std::string &collection,
                                                   const std::string &field) const {
    return {tenant, collection, field};
}

std::string TensorDeduplicationManager::makeKeyIndex(const TensorFieldKey &key) const {
    // Use ASCII Unit Separator to avoid ambiguity with user-provided key chars.
    constexpr char kSep = '\x1f';
    return key.tenant + kSep + key.collection + kSep + key.field;
}

void TensorDeduplicationManager::clearMappingForTensorIdLocked(const std::string &tensor_id) {
    auto key_it = tensor_id_to_key_.find(tensor_id);
    if (key_it == tensor_id_to_key_.end()) {
        return;
    }
    key_to_tensor_id_.erase(key_it->second);
    tensor_id_to_key_.erase(key_it);
}

TTTrain TensorDeduplicationManager::computeDelta(const TTTrain &ref, const TTTrain &new_train) const {
    // Reconstruct both (in production this would use TT-arithmetic)
    auto ref_dense = ref.reconstruct();
    auto new_dense = new_train.reconstruct();

    if (static_cast<int>(ref_dense.size()) != new_dense.size()) {
        // Incompatible shapes; return new_train unchanged (no delta possible)
        return new_train;
    }

    // Delta = new - ref (element-wise)
    std::vector<float> delta_dense(ref_dense.size());
    for (std::size_t i = 0; i <static_cast<int>(delta_dense.size()); ++i) {
        delta_dense[i] = new_dense[i] - ref_dense[i];
    }

    // Re-compress delta
    TensorTrainConfig delta_cfg;
    delta_cfg.eps      = cfg_.delta_eps;
    delta_cfg.max_rank = cfg_.delta_max_rank;

    auto [delta_train, stats] = decomposer_->decompose(delta_dense, new_train.mode_sizes, delta_cfg);
    return std::move(delta_train);
}

TTTrain TensorDeduplicationManager::addTrains(const TTTrain &a, const TTTrain &b) const {
    auto da = a.reconstruct();
    auto db = b.reconstruct();

    if (static_cast<int>(da.size()) != db.size()) {
        return a; // incompatible
    }

    std::vector<float> sum(da.size());
    for (std::size_t i = 0; i <static_cast<int>(da.size()); ++i) {
        sum[i] = da[i] + db[i];
    }

    TensorTrainConfig cfg;
    cfg.eps         = cfg_.delta_eps;
    auto [t, stats] = decomposer_->decompose(sum, a.mode_sizes, cfg);
    return std::move(t);
}

// ============================================================================
// store
// ============================================================================

StoredTensorRecord TensorDeduplicationManager::store(const std::string &tensor_id, const std::vector<float> &data,
                                                     const std::vector<std::size_t> &mode_sizes,
                                                     const std::string &tenant, const std::string &collection,
                                                     const std::string &field) {
    // Decompose the new tensor
    TensorTrainConfig cfg;
    cfg.eps                 = cfg_.delta_eps * 10.0; // slightly looser for the fingerprint
    auto [new_train, stats] = decomposer_->decompose(data, mode_sizes, cfg);

    // Find similar tensors via fingerprint graph
    auto similar = fp_graph_->findSimilar(new_train, 1);

    StoredTensorRecord record;
    record.tensor_id    = tensor_id;
    record.is_canonical = true;

    std::size_t full_bytes = data.size() * sizeof(float);

    if (!similar.empty() && similar[0].similarity >= cfg_.similarity_threshold) {
        // Found a reference — store delta
        const std::string &ref_id = similar[0].tensor_id;

        // Load reference compressed train for delta computation
        std::string ref_collection = similar[0].collection;
        std::string ref_tenant     = similar[0].tenant;
        std::string ref_field      = similar[0].field;

        auto ref_dense_opt = storage_->get(makeKey(ref_tenant, ref_collection, ref_field));
        if (ref_dense_opt) {
            // Build reference TTTrain (re-decompose the retrieved dense data)
            auto ref_ms = mode_sizes; // assume compatible shapes
            TensorTrainConfig ref_cfg;
            ref_cfg.eps         = cfg_.delta_eps;
            auto [ref_train, _] = decomposer_->decompose(*ref_dense_opt, ref_ms, ref_cfg);

            TTTrain delta = computeDelta(ref_train, new_train);

            // Store delta under a delta field name
            std::string delta_field  = field + "__delta__" + ref_id;
            TensorFieldKey delta_key = makeKey(tenant, collection, delta_field);
            auto delta_dense         = delta.reconstruct();
            storage_->put(delta_key, delta_dense, mode_sizes);

            std::size_t delta_bytes = delta.totalParams() * sizeof(float);

            record.reference_id            = ref_id;
            record.is_canonical            = false;
            record.compressed_bytes        = delta_bytes;
            record.saved_bytes             = (full_bytes > delta_bytes) ? full_bytes - delta_bytes : 0;
            record.similarity_to_reference = similar[0].similarity;
        } else {
            // Reference not loadable — fall back to full storage
            goto store_canonical;
        }
    } else {
    store_canonical:
        // Store as canonical tensor
        TensorFieldKey key = makeKey(tenant, collection, field);
        storage_->put(key, data, mode_sizes);

        std::size_t stored_bytes = new_train.totalParams() * sizeof(float);
        record.compressed_bytes  = stored_bytes;
        record.saved_bytes       = (full_bytes > stored_bytes) ? full_bytes - stored_bytes : 0;
    }

    // Insert into fingerprint graph
    fp_graph_->insert(tensor_id, new_train, tenant, collection, field);

    // Persist key fields so retrieve() can look up the tensor without an extra index
    record.tenant     = tenant;
    record.collection = collection;
    record.field      = field;

    // Store record
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved        = 0;
    {
        // LOCK SCOPE: Acquire rw_mutex_ (Tier 2), release BEFORE callback
        std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
        auto prev = records_.find(tensor_id);
        if (prev != records_.end()) {
            total_bytes_stored_.fetch_sub(prev->second.compressed_bytes, std::memory_order_relaxed);
            bytes_saved_.fetch_sub(prev->second.saved_bytes, std::memory_order_relaxed);
            clearMappingForTensorIdLocked(tensor_id);
        }
        total_bytes_stored_.fetch_add(record.compressed_bytes, std::memory_order_relaxed);
        bytes_saved_.fetch_add(record.saved_bytes, std::memory_order_relaxed);
        records_[tensor_id] = record;
        if (record.is_canonical) {
            const auto idx               = makeKeyIndex(makeKey(record.tenant, record.collection, record.field));
            key_to_tensor_id_[idx]       = tensor_id;
            tensor_id_to_key_[tensor_id] = idx;
        }
        total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
        bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
    }  // rw_mutex_ released HERE

    // SAFE: Call journal callback AFTER releasing rw_mutex_
    persistUpsertJournalEntry(record, total_bytes_stored, bytes_saved);

    return record;
}

// ============================================================================
// retrieve
// ============================================================================

std::optional<std::vector<float>> TensorDeduplicationManager::retrieve(const std::string &tensor_id) const {
    // Copy the record while holding the shared lock so that we do NOT hold
    // the lock while calling storage_->get() — mixing the rw_mutex_ shared
    // lock with the storage engine's own write lock (held during put()) would
    // otherwise create a potential deadlock.
    StoredTensorRecord rec;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        auto it = records_.find(tensor_id);
        if (it == records_.end()) {
            return std::nullopt;
        }
        rec = it->second;
    }

    if (rec.is_canonical) {
        return storage_->get(makeKey(rec.tenant, rec.collection, rec.field));
    }

    // ── Delta path ───────────────────────────────────────────────────────────
    // 1. Load the canonical reference record.
    StoredTensorRecord ref_rec;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        auto ref_it = records_.find(rec.reference_id);
        if (ref_it == records_.end()) {
            return std::nullopt;
        }
        ref_rec = ref_it->second;
    }

    // 2. Load canonical reference dense vector from storage.
    auto ref_opt = storage_->get(makeKey(ref_rec.tenant, ref_rec.collection, ref_rec.field));
    if (!ref_opt) {
        return std::nullopt;
    }

    // 3. Load delta (stored under field + "__delta__" + reference_id).
    const std::string delta_field = rec.field + "__delta__" + rec.reference_id;
    auto delta_opt                = storage_->get(makeKey(rec.tenant, rec.collection, delta_field));
    if (!delta_opt) {
        return std::nullopt;
    }

    if (ref_opt->size() != delta_opt->size()) {
        return std::nullopt;
    }

    // 4. Reconstruct: result = reference + delta (element-wise).
    std::vector<float> result(ref_opt->size());
    for (std::size_t i = 0; i <static_cast<int>(result.size()); ++i) {
        result[i] = (*ref_opt)[i] + (*delta_opt)[i];
    }
    return result;
}

std::optional<StoredTensorRecord> TensorDeduplicationManager::getRecord(const std::string &tensor_id) const {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    auto it = records_.find(tensor_id);
    if (it == records_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// getStats
// ============================================================================

DeduplicationStats TensorDeduplicationManager::getStats() const noexcept {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    DeduplicationStats s;
    s.total_tensors = records_.size();
    for (const auto &kv : records_) {
        if (kv.second.is_canonical) {
            ++s.canonical_tensors;
        } else {
            ++s.delta_tensors;
        }
    }
    s.total_bytes_stored   = total_bytes_stored_.load(std::memory_order_relaxed);
    s.bytes_saved          = bytes_saved_.load(std::memory_order_relaxed);
    std::size_t full_bytes = s.total_bytes_stored + s.bytes_saved;
    s.dedup_ratio          = (s.total_bytes_stored > 0)
                                 ? static_cast<double>(full_bytes) / static_cast<double>(s.total_bytes_stored)
                                 : 1.0;
    return s;
}

// ============================================================================
// Graph snapshot serialization helpers
// ============================================================================
//
// Binary layout (all integers little-endian):
//   uint64_t magic    = 0x504E535F47465400  ("TFG_SNP\0")
//   uint32_t version  = 1
//   uint64_t node_count
//   for each node:
//     uint64_t tensor_id_len  + bytes
//     uint64_t minhash[128]   (128 × uint64_t, raw)
//     uint32_t core_norms_len
//     float    core_norms[core_norms_len]
//     float    total_norm
//     uint64_t order
//     uint64_t max_rank
//     uint64_t tenant_len     + bytes
//     uint64_t collection_len + bytes
//     uint64_t field_len      + bytes
//   uint64_t edge_count
//   for each edge:
//     uint64_t from_len       + bytes
//     uint64_t to_len         + bytes
//     double   similarity

namespace {

constexpr uint64_t kGraphSnapshotMagic          = 0x504E535F47465400ULL; // "TFG_SNP\0"
constexpr uint32_t kGraphSnapshotVersion        = 1;
constexpr uint64_t kDedupSnapshotMagic          = 0x504E535F4D445400ULL; // "TDM_SNP\0"
constexpr uint32_t kDedupSnapshotVersion        = 1;
constexpr uint64_t kMutationJournalMagic        = 0x4A4E4C5F4D445400ULL; // "TDM_JNL\0"
constexpr uint32_t kMutationJournalVersion      = 1;
constexpr char kActiveSnapshotMetaKey[]         = "__tfg_active_snapshot__";
constexpr char kMutationJournalMetaPrefix[]     = "__tfgmeta__:wal:";
constexpr char kPerEntryMutationJournalPrefix[] = "__tfgjournal__:";

enum class JournalLoadStatus { Missing, Loaded, InvalidReset };

enum class MutationJournalEntryType : uint8_t {
    Upsert = 1,
    Delete = 2,
};

struct MutationJournalEntry {
    MutationJournalEntryType type = MutationJournalEntryType::Upsert;
    themis::graph::StoredTensorRecord record;
    themis::graph::PersistedFingerprintNode node;
    std::vector<themis::graph::PersistedFingerprintEdge> edges;
    std::string tensor_id = {};
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved        = 0;
};

// Little-endian write helpers.
template <typename T> static void writeLE(std::vector<uint8_t> &buf, T val) {
    static_assert(std::is_trivially_copyable<T>::value);
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&val);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        buf.push_back(p[i]);
    }
}

static void writeStr(std::vector<uint8_t> &buf, const std::string &s) {
    writeLE<uint64_t>(buf, static_cast<uint64_t>(s.size()));
    for (unsigned char c : s) {
        buf.push_back(c);
    }
}

// Little-endian read helpers.
template <typename T> static bool readLE(const uint8_t *data, std::size_t size, std::size_t &pos, T &out) {
    if (pos + sizeof(T) > size) {
        return false;
    }
    std::memcpy(&out, data + pos, sizeof(T));
    pos += sizeof(T);
    return true;
}

static bool readStr(const uint8_t *data, std::size_t size, std::size_t &pos, std::string &out) {
    uint64_t len = 0;
    if (!readLE(data, size, pos, len)) {
        return false;
    }
    if (pos + len > size) {
        return false;
    }
    out.assign(reinterpret_cast<const char *>(data + pos), static_cast<std::size_t>(len));
    pos += static_cast<std::size_t>(len);
    return true;
}

static std::vector<uint8_t> serializeGraphSnapshot(const themis::graph::PersistedFingerprintGraphSnapshot &snapshot) {
    std::vector<uint8_t> buf;
    buf.reserve(4096);

    writeLE<uint64_t>(buf, kGraphSnapshotMagic);
    writeLE<uint32_t>(buf, kGraphSnapshotVersion);

    writeLE<uint64_t>(buf, static_cast<uint64_t>(snapshot.nodes.size()));
    for (const auto &n : snapshot.nodes) {
        writeStr(buf, n.tensor_id);

        // fingerprint.minhash (128 × uint64_t)
        for (const auto h : n.fingerprint.minhash) {
            writeLE<uint64_t>(buf, h);
        }

        // fingerprint.core_norms
        writeLE<uint32_t>(buf, static_cast<uint32_t>(n.fingerprint.core_norms.size()));
        for (float f : n.fingerprint.core_norms) {
            writeLE<float>(buf, f);
        }

        writeLE<float>(buf, n.fingerprint.total_norm);
        writeLE<uint64_t>(buf, static_cast<uint64_t>(n.fingerprint.order));
        writeLE<uint64_t>(buf, static_cast<uint64_t>(n.fingerprint.max_rank));

        writeStr(buf, n.tenant);
        writeStr(buf, n.collection);
        writeStr(buf, n.field);
    }

    writeLE<uint64_t>(buf, static_cast<uint64_t>(snapshot.edges.size()));
    for (const auto &e : snapshot.edges) {
        writeStr(buf, e.from);
        writeStr(buf, e.to);
        writeLE<double>(buf, e.similarity);
    }

    return buf;
}

static void writeStoredTensorRecord(std::vector<uint8_t> &buf, const themis::graph::StoredTensorRecord &record) {
    writeStr(buf, record.tensor_id);
    writeStr(buf, record.reference_id);
    writeLE<uint8_t>(buf, record.is_canonical ? 1 : 0);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(record.compressed_bytes));
    writeLE<uint64_t>(buf, static_cast<uint64_t>(record.saved_bytes));
    writeLE<double>(buf, record.similarity_to_reference);
    writeStr(buf, record.tenant);
    writeStr(buf, record.collection);
    writeStr(buf, record.field);
}

static void writePersistedFingerprintNode(std::vector<uint8_t> &buf,
                                          const themis::graph::PersistedFingerprintNode &node) {
    writeStr(buf, node.tensor_id);
    for (const auto hash : node.fingerprint.minhash) {
        writeLE<uint64_t>(buf, hash);
    }
    writeLE<uint32_t>(buf, static_cast<uint32_t>(node.fingerprint.core_norms.size()));
    for (const auto core_norm : node.fingerprint.core_norms) {
        writeLE<float>(buf, core_norm);
    }
    writeLE<float>(buf, node.fingerprint.total_norm);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(node.fingerprint.order));
    writeLE<uint64_t>(buf, static_cast<uint64_t>(node.fingerprint.max_rank));
    writeStr(buf, node.tenant);
    writeStr(buf, node.collection);
    writeStr(buf, node.field);
}

static void writePersistedFingerprintEdge(std::vector<uint8_t> &buf,
                                          const themis::graph::PersistedFingerprintEdge &edge) {
    writeStr(buf, edge.from);
    writeStr(buf, edge.to);
    writeLE<double>(buf, edge.similarity);
}

static std::vector<uint8_t> serializeMutationJournal(const std::vector<MutationJournalEntry> &entries) {
    constexpr std::size_t kEstimatedBytesPerEntry = 256;
    constexpr std::size_t kJournalHeaderBytes     = 64;
    std::vector<uint8_t> buf = {};

    buf.reserve(entries.size() * kEstimatedBytesPerEntry + kJournalHeaderBytes);
    writeLE<uint64_t>(buf, kMutationJournalMagic);
    writeLE<uint32_t>(buf, kMutationJournalVersion);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(entries.size()));
    for (const auto &entry : entries) {
        writeLE<uint8_t>(buf, static_cast<uint8_t>(entry.type));
        if (entry.type == MutationJournalEntryType::Upsert) {
            writeStoredTensorRecord(buf, entry.record);
            writePersistedFingerprintNode(buf, entry.node);
            writeLE<uint64_t>(buf, static_cast<uint64_t>(entry.edges.size()));
            for (const auto &edge : entry.edges) {
                writePersistedFingerprintEdge(buf, edge);
            }
        } else {
            writeStr(buf, entry.tensor_id);
        }
        writeLE<uint64_t>(buf, static_cast<uint64_t>(entry.total_bytes_stored));
        writeLE<uint64_t>(buf, static_cast<uint64_t>(entry.bytes_saved));
    }
    return buf;
}

static std::vector<uint8_t> serializeDedupSnapshot(const themis::graph::PersistedFingerprintGraphSnapshot &snapshot,
                                                   const std::vector<themis::graph::StoredTensorRecord> &records,
                                                   std::size_t total_bytes_stored, std::size_t bytes_saved) {
    auto graph_bytes = serializeGraphSnapshot(snapshot);

    std::vector<uint8_t> buf = {};

    buf.reserve(static_cast<int>(graph_bytes.size()) + static_cast<int>(records.size()) * 256 + 64);
    writeLE<uint64_t>(buf, kDedupSnapshotMagic);
    writeLE<uint32_t>(buf, kDedupSnapshotVersion);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(graph_bytes.size()));
    buf.insert(buf.end(), graph_bytes.begin(), graph_bytes.end());

    writeLE<uint64_t>(buf, static_cast<uint64_t>(records.size()));
    for (const auto &record : records) {
        writeStoredTensorRecord(buf, record);
    }

    writeLE<uint64_t>(buf, static_cast<uint64_t>(total_bytes_stored));
    writeLE<uint64_t>(buf, static_cast<uint64_t>(bytes_saved));
    return buf;
}

static bool deserializeGraphSnapshot(const std::vector<uint8_t> &buf,
                                     themis::graph::PersistedFingerprintGraphSnapshot &snapshot) {
    std::size_t pos        = 0;
    const uint8_t *data    = buf.data();
    const std::size_t size = buf.size();

    uint64_t magic = 0;
    uint32_t ver   = 0;
    if (!readLE(data, size, pos, magic)) {
        return false;
    }
    if (magic != kGraphSnapshotMagic) {
        return false;
    }
    if (!readLE(data, size, pos, ver)) {
        return false;
    }
    if (ver != kGraphSnapshotVersion) {
        return false;
    }

    uint64_t node_count = 0;
    if (!readLE(data, size, pos, node_count)) {
        return false;
    }

    snapshot.nodes.clear();
    // Sanity-bound: reject unreasonably large node counts before allocating.
    // This prevents std::length_error ("vector too long") from escaping when
    // a corrupted or adversarial payload encodes a huge node_count value.
    constexpr uint64_t kMaxGraphNodes = 50'000'000;
    if (node_count > kMaxGraphNodes) {
        THEMIS_WARN(
            "[TensorDeduplicationManager] restore snapshot: node_count {} exceeds sanity limit {}; rejecting payload",
            node_count, kMaxGraphNodes);
        return false;
    }
    snapshot.nodes.reserve(static_cast<std::size_t>(node_count));

    for (uint64_t i = 0; i < node_count; ++i) {
        themis::graph::PersistedFingerprintNode n;
        if (!readStr(data, size, pos, n.tensor_id)) {
            return false;
        }

        for (auto &h : n.fingerprint.minhash) {
            if (!readLE(data, size, pos, h)) {
                return false;
            }
        }

        uint32_t core_norms_len = 0;
        if (!readLE(data, size, pos, core_norms_len)) {
            return false;
        }
        // Sanity-bound: reject payloads with unreasonably many core norms.
        constexpr uint32_t kMaxCoreNorms = 500'000;
        if (core_norms_len > kMaxCoreNorms) {
            THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: core_norms_len {} exceeds sanity limit {}; "
                        "rejecting payload",
                        core_norms_len, kMaxCoreNorms);
            return false;
        }
        n.fingerprint.core_norms.resize(core_norms_len);
        for (auto &f : n.fingerprint.core_norms) {
            if (!readLE(data, size, pos, f)) {
                return false;
            }
        }

        if (!readLE(data, size, pos, n.fingerprint.total_norm)) {
            return false;
        }

        uint64_t order = 0, max_rank = 0;
        if (!readLE(data, size, pos, order)) {
            return false;
        }
        if (!readLE(data, size, pos, max_rank)) {
            return false;
        }
        n.fingerprint.order    = static_cast<std::size_t>(order);
        n.fingerprint.max_rank = static_cast<std::size_t>(max_rank);

        if (!readStr(data, size, pos, n.tenant)) {
            return false;
        }
        if (!readStr(data, size, pos, n.collection)) {
            return false;
        }
        if (!readStr(data, size, pos, n.field)) {
            return false;
        }

        snapshot.nodes.push_back(std::move(n));
    }

    uint64_t edge_count = 0;
    if (!readLE(data, size, pos, edge_count)) {
        return false;
    }

    snapshot.edges.clear();
    // Sanity-bound: reject unreasonably large edge counts.
    constexpr uint64_t kMaxGraphEdges = 200'000'000;
    if (edge_count > kMaxGraphEdges) {
        THEMIS_WARN(
            "[TensorDeduplicationManager] restore snapshot: edge_count {} exceeds sanity limit {}; rejecting payload",
            edge_count, kMaxGraphEdges);
        return false;
    }
    snapshot.edges.reserve(static_cast<std::size_t>(edge_count));

    for (uint64_t i = 0; i < edge_count; ++i) {
        themis::graph::PersistedFingerprintEdge e;
        if (!readStr(data, size, pos, e.from)) {
            return false;
        }
        if (!readStr(data, size, pos, e.to)) {
            return false;
        }
        if (!readLE(data, size, pos, e.similarity)) {
            return false;
        }
        snapshot.edges.push_back(std::move(e));
    }

    return true;
}

static bool readStoredTensorRecord(const uint8_t *data, std::size_t size, std::size_t &pos,
                                   themis::graph::StoredTensorRecord &record) {
    if (!readStr(data, size, pos, record.tensor_id)) {
        return false;
    }
    if (!readStr(data, size, pos, record.reference_id)) {
        return false;
    }

    uint8_t is_canonical      = 0;
    uint64_t compressed_bytes = 0;
    uint64_t saved_bytes      = 0;
    if (!readLE(data, size, pos, is_canonical)) {
        return false;
    }
    if (!readLE(data, size, pos, compressed_bytes)) {
        return false;
    }
    if (!readLE(data, size, pos, saved_bytes)) {
        return false;
    }
    if (!readLE(data, size, pos, record.similarity_to_reference)) {
        return false;
    }
    if (!readStr(data, size, pos, record.tenant)) {
        return false;
    }
    if (!readStr(data, size, pos, record.collection)) {
        return false;
    }
    if (!readStr(data, size, pos, record.field)) {
        return false;
    }

    record.is_canonical     = (is_canonical != 0);
    record.compressed_bytes = static_cast<std::size_t>(compressed_bytes);
    record.saved_bytes      = static_cast<std::size_t>(saved_bytes);
    return true;
}

static bool readPersistedFingerprintNode(const uint8_t *data, std::size_t size, std::size_t &pos,
                                         themis::graph::PersistedFingerprintNode &node) {
    if (!readStr(data, size, pos, node.tensor_id)) {
        return false;
    }
    for (auto &hash : node.fingerprint.minhash) {
        if (!readLE(data, size, pos, hash)) {
            return false;
        }
    }

    uint32_t core_norm_count = 0;
    if (!readLE(data, size, pos, core_norm_count)) {
        return false;
    }
    node.fingerprint.core_norms.resize(core_norm_count);
    for (auto &core_norm : node.fingerprint.core_norms) {
        if (!readLE(data, size, pos, core_norm)) {
            return false;
        }
    }

    if (!readLE(data, size, pos, node.fingerprint.total_norm)) {
        return false;
    }

    uint64_t order    = 0;
    uint64_t max_rank = 0;
    if (!readLE(data, size, pos, order)) {
        return false;
    }
    if (!readLE(data, size, pos, max_rank)) {
        return false;
    }
    node.fingerprint.order    = static_cast<std::size_t>(order);
    node.fingerprint.max_rank = static_cast<std::size_t>(max_rank);

    if (!readStr(data, size, pos, node.tenant)) {
        return false;
    }
    if (!readStr(data, size, pos, node.collection)) {
        return false;
    }
    if (!readStr(data, size, pos, node.field)) {
        return false;
    }
    return true;
}

static bool readPersistedFingerprintEdge(const uint8_t *data, std::size_t size, std::size_t &pos,
                                         themis::graph::PersistedFingerprintEdge &edge) {
    if (!readStr(data, size, pos, edge.from)) {
        return false;
    }
    if (!readStr(data, size, pos, edge.to)) {
        return false;
    }
    return readLE(data, size, pos, edge.similarity);
}

static bool deserializeMutationJournal(const std::vector<uint8_t> &buf, std::vector<MutationJournalEntry> &entries) {
    std::size_t pos  = 0;
    const auto *data = buf.data();
    const auto size  = buf.size();

    uint64_t magic   = 0;
    uint32_t version = 0;
    if (!readLE(data, size, pos, magic) || magic != kMutationJournalMagic) {
        return false;
    }
    if (!readLE(data, size, pos, version) || version != kMutationJournalVersion) {
        return false;
    }

    uint64_t entry_count = 0;
    if (!readLE(data, size, pos, entry_count)) {
        return false;
    }

    entries.clear();
    entries.reserve(static_cast<std::size_t>(entry_count));
    for (uint64_t i = 0; i < entry_count; ++i) {
        uint8_t raw_type = 0;
        if (!readLE(data, size, pos, raw_type)) {
            return false;
        }

        MutationJournalEntry entry;
        entry.type = static_cast<MutationJournalEntryType>(raw_type);
        if (entry.type == MutationJournalEntryType::Upsert) {
            if (!readStoredTensorRecord(data, size, pos, entry.record)) {
                return false;
            }
            if (!readPersistedFingerprintNode(data, size, pos, entry.node)) {
                return false;
            }

            uint64_t edge_count = 0;
            if (!readLE(data, size, pos, edge_count)) {
                return false;
            }
            entry.edges.reserve(static_cast<std::size_t>(edge_count));
            for (uint64_t edge_idx = 0; edge_idx < edge_count; ++edge_idx) {
                themis::graph::PersistedFingerprintEdge edge;
                if (!readPersistedFingerprintEdge(data, size, pos, edge)) {
                    return false;
                }
                entry.edges.push_back(std::move(edge));
            }
        } else if (entry.type == MutationJournalEntryType::Delete) {
            if (!readStr(data, size, pos, entry.tensor_id)) {
                return false;
            }
        } else {
            return false;
        }

        uint64_t total_bytes_stored = 0;
        uint64_t bytes_saved        = 0;
        if (!readLE(data, size, pos, total_bytes_stored)) {
            return false;
        }
        if (!readLE(data, size, pos, bytes_saved)) {
            return false;
        }
        entry.total_bytes_stored = static_cast<std::size_t>(total_bytes_stored);
        entry.bytes_saved        = static_cast<std::size_t>(bytes_saved);
        entries.push_back(std::move(entry));
    }

    return pos == size;
}

static void compactMutationJournalEntries(std::vector<MutationJournalEntry> &entries) {
    if (static_cast<int>(entries.size()) < 2) {
        return;
    }

    const auto extractTensorId = [](const MutationJournalEntry &entry) -> const std::string & {
        return (entry.type == MutationJournalEntryType::Upsert) ? entry.record.tensor_id : entry.tensor_id;
    };

    std::unordered_map<std::string, std::size_t> last_index_by_tensor_id = {};

    last_index_by_tensor_id.reserve(entries.size());
    for (std::size_t i = 0; i <static_cast<int>(entries.size()); ++i) {
        last_index_by_tensor_id[extractTensorId(entries[i])] = i;
    }

    std::vector<MutationJournalEntry> compacted = {};

    compacted.reserve(last_index_by_tensor_id.size());
    for (std::size_t i = 0; i <static_cast<int>(entries.size()); ++i) {
        auto &entry                  = entries[i];
        const std::string &tensor_id = extractTensorId(entry);
        if (last_index_by_tensor_id.at(tensor_id) != i) {
            continue;
        }
        compacted.push_back(std::move(entry));
    }

    entries = std::move(compacted);
}

[[nodiscard]] static std::string mutationJournalKeyForSnapshot(const std::string &snapshot_key) {
    std::string key = {};
    key.reserve((sizeof(kMutationJournalMetaPrefix) - 1) + static_cast<int>(snapshot_key.size()) );
    key.append(kMutationJournalMetaPrefix);
    key.append(snapshot_key);
    return key;
}

[[nodiscard]] static std::string legacyMutationJournalKeyForSnapshot(const std::string &snapshot_key) {
    std::string key = {};
    key.reserve(static_cast<int>(snapshot_key.size()) + 5);
    key.append(snapshot_key);
    key.append("::wal");
    return key;
}

// Load mutation journal entries using key fallback order:
//   1) namespaced key "__tfgmeta__:wal:<snapshot>"
//   2) legacy key "<snapshot>::wal"
// Return value:
//   - Loaded => journal payload exists and was deserialized successfully
//   - InvalidReset => a payload existed but was invalid and got cleared
//   - Missing => no journal payload exists under either key
static JournalLoadStatus
loadJournalWithLegacyFallback(const std::shared_ptr<themis::storage::TensorNetworkStorageEngine> &storage,
                              const std::string &snapshot_key, std::vector<MutationJournalEntry> &entries) {
    if (!storage) {
        entries.clear();
        return JournalLoadStatus::Missing;
    }

    const auto try_load_key = [&]([[maybe_unused]] const std::string &key) -> JournalLoadStatus {
        const auto payload = storage->getRawMetadata(key);
        if (!payload || payload->empty()) {
            return JournalLoadStatus::Missing;
        }
        if (deserializeMutationJournal(*payload, entries)) {
            return JournalLoadStatus::Loaded;
        }
        THEMIS_WARN("[TensorDeduplicationManager] mutation journal parse failed for key='{}' ({} bytes); clearing "
                    "in-memory replay entries",
                    key, payload->size());
        if (!storage->putRawMetadata(key, {})) {
            THEMIS_WARN(
                "[TensorDeduplicationManager] failed to reset invalid mutation journal payload for key='{}'; corrupted "
                "payload may persist across restore attempts and manual metadata cleanup may be required",
                key);
        }
        entries.clear();
        return JournalLoadStatus::InvalidReset;
    };

    const auto namespaced_status = try_load_key(mutationJournalKeyForSnapshot(snapshot_key));
    if (namespaced_status == JournalLoadStatus::Loaded) {
        return JournalLoadStatus::Loaded;
    }

    const auto legacy_status = try_load_key(legacyMutationJournalKeyForSnapshot(snapshot_key));
    if (legacy_status == JournalLoadStatus::Loaded) {
        return JournalLoadStatus::Loaded;
    }

    entries.clear();
    if (namespaced_status == JournalLoadStatus::InvalidReset || legacy_status == JournalLoadStatus::InvalidReset) {
        return JournalLoadStatus::InvalidReset;
    }
    return JournalLoadStatus::Missing;
}

static void writeJournalAndClearLegacy(const std::shared_ptr<themis::storage::TensorNetworkStorageEngine> &storage,
                                       const std::string &snapshot_key,
                                       const std::vector<MutationJournalEntry> &entries) {
    if (!storage) {
        return;
    }
    const auto namespaced_key = mutationJournalKeyForSnapshot(snapshot_key);
    const auto legacy_key     = legacyMutationJournalKeyForSnapshot(snapshot_key);
    auto payload              = serializeMutationJournal(entries);

    const auto existing_namespaced = storage->getRawMetadata(namespaced_key);
    // Missing namespaced key and changed payload are both write paths.
    const bool namespaced_changed = !existing_namespaced.has_value() || (*existing_namespaced != payload);
    bool namespaced_ready         = !namespaced_changed;
    if (namespaced_changed) {
        namespaced_ready = storage->putRawMetadata(namespaced_key, payload);
        if (!namespaced_ready) {
            THEMIS_WARN("[TensorDeduplicationManager] failed to persist mutation journal for key='{}'; keeping legacy "
                        "payload to avoid data loss",
                        namespaced_key);
        }
    }
    if (!namespaced_ready) {
        return;
    }

    const auto legacy_payload = storage->getRawMetadata(legacy_key);
    // Skip no-op empty rewrites: legacy key may already exist with an empty
    // payload from previous normalization/reset cycles.
    if (legacy_payload.has_value() && !legacy_payload->empty()) {
        if (!storage->putRawMetadata(legacy_key, {})) {
            THEMIS_WARN("[TensorDeduplicationManager] failed to clear legacy mutation journal key='{}'", legacy_key);
        }
    }
}

static std::optional<std::string>
activeSnapshotKeyOrNullopt(const std::shared_ptr<themis::storage::TensorNetworkStorageEngine> &storage) {
    if (!storage) {
        return std::nullopt;
    }

    const auto raw_key = storage->getRawMetadata(kActiveSnapshotMetaKey);
    if (!raw_key || raw_key->empty()) {
        return std::nullopt;
    }

    return std::string(raw_key->begin(), raw_key->end());
}

static bool deserializeDedupSnapshot(const std::vector<uint8_t> &buf,
                                     themis::graph::PersistedFingerprintGraphSnapshot &snapshot,
                                     std::vector<themis::graph::StoredTensorRecord> &records,
                                     std::size_t &total_bytes_stored, std::size_t &bytes_saved) {
    std::size_t pos  = 0;
    const auto *data = buf.data();
    const auto size  = buf.size();

    uint64_t magic   = 0;
    uint32_t version = 0;
    if (!readLE(data, size, pos, magic)) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: failed to read dedup magic");
        return false;
    }
    if (magic != kDedupSnapshotMagic) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: dedup magic mismatch (expected={}, actual={})",
                     kDedupSnapshotMagic, magic);
        return false;
    }
    if (!readLE(data, size, pos, version)) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: failed to read dedup version");
        return false;
    }
    if (version != kDedupSnapshotVersion) {
        THEMIS_DEBUG(
            "[TensorDeduplicationManager] restore snapshot: unsupported dedup version (expected={}, actual={})",
            kDedupSnapshotVersion, version);
        return false;
    }

    uint64_t graph_bytes_size = 0;
    if (!readLE(data, size, pos, graph_bytes_size)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read graph payload length");
        return false;
    }
    // Guard against unsigned overflow and oversized payload claims.
    if (graph_bytes_size > static_cast<uint64_t>(size - pos)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: graph payload length {} exceeds buffer size {}",
                    graph_bytes_size, size - pos);
        return false;
    }

    const auto graph_size = static_cast<std::size_t>(graph_bytes_size);
    std::vector<uint8_t> graph_bytes(graph_size);
    std::memcpy(graph_bytes.data(), data + pos, graph_size);
    pos += graph_size;
    if (!deserializeGraphSnapshot(graph_bytes, snapshot)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: embedded graph payload is invalid");
        return false;
    }

    uint64_t record_count = 0;
    if (!readLE(data, size, pos, record_count)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read record count");
        return false;
    }
    records.clear();
    // Sanity-bound: reject unreasonably large record counts.
    constexpr uint64_t kMaxDedupRecords = 50'000'000;
    if (record_count > kMaxDedupRecords) {
        THEMIS_WARN(
            "[TensorDeduplicationManager] restore snapshot: record_count {} exceeds sanity limit {}; rejecting payload",
            record_count, kMaxDedupRecords);
        return false;
    }
    records.reserve(static_cast<std::size_t>(record_count));
    for (uint64_t i = 0; i < record_count; ++i) {
        themis::graph::StoredTensorRecord record;
        if (!readStoredTensorRecord(data, size, pos, record)) {
            THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read record {}", i);
            return false;
        }
        records.push_back(std::move(record));
    }

    uint64_t total_bytes_stored_u64 = 0;
    uint64_t bytes_saved_u64        = 0;
    if (!readLE(data, size, pos, total_bytes_stored_u64)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read total_bytes_stored");
        return false;
    }
    if (!readLE(data, size, pos, bytes_saved_u64)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read bytes_saved");
        return false;
    }

    total_bytes_stored = static_cast<std::size_t>(total_bytes_stored_u64);
    bytes_saved        = static_cast<std::size_t>(bytes_saved_u64);
    if (pos != size) {
        const auto trailing_bytes = size - pos;
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: trailing bytes detected ({})", trailing_bytes);
        return false;
    }
    return true;
}

} // namespace

// ============================================================================
// snapshotGraph / restoreGraph
// ============================================================================

bool TensorDeduplicationManager::snapshotGraph(const std::string &snapshot_key) {
    auto snapshot = fp_graph_->exportPersistedGraph();

    std::vector<StoredTensorRecord> records;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved        = 0;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        records.reserve(records_.size());
        for (const auto &[_, record] : records_) {
            records.push_back(record);
        }
        total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
        bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
    }

    auto bytes = serializeDedupSnapshot(snapshot, records, total_bytes_stored, bytes_saved);
    if (!storage_->putRawMetadata(snapshot_key, bytes)) {
        return false;
    }
    clearMutationJournal(snapshot_key);
    activateSnapshotKey(snapshot_key);
    return true;
}

bool TensorDeduplicationManager::restoreGraph(const std::string &snapshot_key) {
    try {
        auto bytes_opt = storage_->getRawMetadata(snapshot_key);
        if (!bytes_opt) {
            return false;
        }

        PersistedFingerprintGraphSnapshot snapshot;
        std::vector<StoredTensorRecord> records;
        std::size_t total_bytes_stored = 0;
        std::size_t bytes_saved        = 0;

        if (!deserializeDedupSnapshot(*bytes_opt, snapshot, records, total_bytes_stored, bytes_saved)) {
            THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: dedup payload parse failed, trying legacy "
                         "graph-only payload");
            if (!deserializeGraphSnapshot(*bytes_opt, snapshot)) {
                THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: both dedup and legacy graph payload "
                            "parsing failed");
                return false;
            }
            THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: legacy graph-only payload restored");
        }

        fp_graph_->importPersistedGraph(snapshot);

        {
            // LOCK SCOPE: Acquire rw_mutex_ (Tier 2), release BEFORE replayMutationJournal
            std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
            records_.clear();
            key_to_tensor_id_.clear();
            tensor_id_to_key_.clear();
            for (const auto &record : records) {
                records_[record.tensor_id] = record;
                if (!record.is_canonical) {
                    continue;
                }

                const auto key                      = makeKey(record.tenant, record.collection, record.field);
                const auto key_index                = makeKeyIndex(key);
                key_to_tensor_id_[key_index]        = record.tensor_id;
                tensor_id_to_key_[record.tensor_id] = key_index;
            }
            total_bytes_stored_.store(total_bytes_stored, std::memory_order_relaxed);
            bytes_saved_.store(bytes_saved, std::memory_order_relaxed);
        }  // rw_mutex_ released HERE
        
        // SAFE: Call replayMutationJournal AFTER releasing rw_mutex_
        if (!replayMutationJournal(snapshot_key)) {
            return false;
        }
        activateSnapshotKey(snapshot_key);
        return true;
    } catch (const std::exception &ex) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: exception while restoring '{}': {}", snapshot_key,
                    ex.what());
        return false;
    } catch (...) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: unknown exception while restoring '{}'",
                    snapshot_key);
        return false;
    }
}

bool TensorDeduplicationManager::replayMutationJournal(const std::string &snapshot_key) {
    std::vector<MutationJournalEntry> entries;

    // Per-entry hook path: enumerate individual per-tensor journal records.
    if (hasJournalEntryHooks()) {
        std::lock_guard<std::mutex> hlk(journal_hooks_mutex_);
        if (journal_entry_enumerate_fn_) {
            try {
                journal_entry_enumerate_fn_(
                    snapshot_key,
                    [&](std::string_view /*tensor_id*/, const std::vector<uint8_t>& payload) {
                        std::vector<MutationJournalEntry> one = {};

                        if (deserializeMutationJournal(payload, one) && !one.empty()) {
                            entries.push_back(std::move(one[0]));
                        }
                    });
            } catch (...) {}
        }
        // Per-entry journals are already compacted (one entry per tensor_id).
        // Compact again to handle duplicate tensor_ids from concurrent writes.
        compactMutationJournalEntries(entries);
        if (entries.empty()) {
            const auto load_status = loadJournalWithLegacyFallback(storage_, snapshot_key, entries);
            if (load_status == JournalLoadStatus::Missing) {
                return true;
            }
            if (entries.empty()) {
                if (load_status == JournalLoadStatus::InvalidReset) {
                    THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: mutation journal reset after parse "
                                "failure; replay skipped");
                }
                return true;
            }
        }
    } else {
        const auto load_status = loadJournalWithLegacyFallback(storage_, snapshot_key, entries);
        if (load_status == JournalLoadStatus::Missing) {
            return true;
        }
        if (entries.empty()) {
            // This path handles both a valid empty journal and an invalid payload
            // that has been reset.
            // Parse failures are treated as no-op replay because the base snapshot
            // payload has already restored a consistent graph/record state.
            if (load_status == JournalLoadStatus::InvalidReset) {
                THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: mutation journal reset after parse "
                            "failure; replay skipped");
            }
            return true;
        }
    }

    if (entries.empty()) {
        return true;
    }

    // replayMutationJournal() runs after restoreGraph() has already reloaded the
    // full snapshot state into records_, mappings, and counters. Replay is
    // single-threaded during restore, so journal entries can safely overwrite
    // the relaxed atomic counters with their absolute snapshots in sequence.
    // Each entry captures the full counter state after its mutation, so the
    // compacted journal remains replay-order-dependent but deterministic.
    for (const auto &entry : entries) {
        if (entry.type == MutationJournalEntryType::Upsert) {
            fp_graph_->upsertPersistedNode(entry.node, entry.edges);

            {
                // LOCK SCOPE: Acquire rw_mutex_ (Tier 2) per entry
                std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
                clearMappingForTensorIdLocked(entry.record.tensor_id);
                records_[entry.record.tensor_id] = entry.record;
                if (entry.record.is_canonical) {
                    const auto key       = makeKey(entry.record.tenant, entry.record.collection, entry.record.field);
                    const auto key_index = makeKeyIndex(key);
                    key_to_tensor_id_[key_index]              = entry.record.tensor_id;
                    tensor_id_to_key_[entry.record.tensor_id] = key_index;
                }
                total_bytes_stored_.store(entry.total_bytes_stored, std::memory_order_relaxed);
                bytes_saved_.store(entry.bytes_saved, std::memory_order_relaxed);
            }  // rw_mutex_ released HERE
            continue;
        }

        {
            // LOCK SCOPE: Acquire rw_mutex_ (Tier 2) for deletion
            std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
            clearMappingForTensorIdLocked(entry.tensor_id);
            records_.erase(entry.tensor_id);
            total_bytes_stored_.store(entry.total_bytes_stored, std::memory_order_relaxed);
            bytes_saved_.store(entry.bytes_saved, std::memory_order_relaxed);
        }  // rw_mutex_ released HERE
        
        // SAFE: Call fp_graph operation AFTER releasing rw_mutex_
        fp_graph_->remove(entry.tensor_id);
    }

    // For blob-based journals: rewrite in compact form as a recovery-time
    // maintenance step so older, pre-compaction payloads are normalized after
    // the first successful replay even if they were persisted by earlier code.
    // Skip for per-entry journals — entries are already individually stored.
    if (!hasJournalEntryHooks()) {
        compactMutationJournalEntries(entries);
        writeJournalAndClearLegacy(storage_, snapshot_key, entries);
    }

    return true;
}

void TensorDeduplicationManager::activateSnapshotKey(const std::string &snapshot_key) const {
    storage_->putRawMetadata(kActiveSnapshotMetaKey, std::vector<uint8_t>{snapshot_key.begin(), snapshot_key.end()});
}

void TensorDeduplicationManager::clearMutationJournal(const std::string &snapshot_key) const {
    // Per-entry hook path: clear all per-entry records for this snapshot key.
    if (hasJournalEntryHooks()) {
        std::lock_guard<std::mutex> hlk(journal_hooks_mutex_);
        if (journal_entry_clear_fn_) {
            try { journal_entry_clear_fn_(snapshot_key); }
            catch (...) {}
        }
        // Also clear the legacy blob keys so they don't confuse future restores.
        storage_->putRawMetadata(mutationJournalKeyForSnapshot(snapshot_key), {});
        storage_->putRawMetadata(legacyMutationJournalKeyForSnapshot(snapshot_key), {});
        return;
    }
    storage_->putRawMetadata(mutationJournalKeyForSnapshot(snapshot_key), {});
    storage_->putRawMetadata(legacyMutationJournalKeyForSnapshot(snapshot_key), {});
}

void TensorDeduplicationManager::persistUpsertJournalEntry(const StoredTensorRecord &record,
                                                           std::size_t total_bytes_stored,
                                                           std::size_t bytes_saved) const {
    const auto snapshot_key = activeSnapshotKeyOrNullopt(storage_);
    if (!snapshot_key.has_value()) {
        return;
    }
    const auto node_opt = fp_graph_->exportPersistedNode(record.tensor_id);
    if (!node_opt.has_value()) {
        return;
    }

    MutationJournalEntry entry;
    entry.type               = MutationJournalEntryType::Upsert;
    entry.record             = record;
    entry.node               = *node_opt;
    entry.edges              = fp_graph_->exportPersistedEdgesFor(record.tensor_id);
    entry.total_bytes_stored = total_bytes_stored;
    entry.bytes_saved        = bytes_saved;

    // Per-entry hook path: write a single-entry blob for this tensor_id.
    // Overwriting an existing entry for the same tensor_id IS compaction.
    if (hasJournalEntryHooks()) {
        const auto payload = serializeMutationJournal({entry});
        std::lock_guard<std::mutex> hlk(journal_hooks_mutex_);
        if (journal_entry_persist_fn_) {
            try {
                if (!journal_entry_persist_fn_(*snapshot_key, record.tensor_id, payload)) {
                    THEMIS_WARN("[TensorDeduplicationManager] persistUpsertJournalEntry: persist_fn returned false for "
                                "tensor_id={}",
                                record.tensor_id);
                }
            }
            catch (const std::exception& ex) {
                THEMIS_WARN("[TensorDeduplicationManager] persistUpsertJournalEntry: persist_fn threw exception: {}", ex.what());
            }
            catch (...) {
                THEMIS_WARN("[TensorDeduplicationManager] persistUpsertJournalEntry: persist_fn threw unknown exception");
            }
        }
        return;
    }

    std::vector<MutationJournalEntry> entries;
    loadJournalWithLegacyFallback(storage_, *snapshot_key, entries);
    entries.push_back(std::move(entry));
    compactMutationJournalEntries(entries);
    writeJournalAndClearLegacy(storage_, *snapshot_key, entries);
}

void TensorDeduplicationManager::persistDeleteJournalEntry(const std::string &tensor_id, std::size_t total_bytes_stored,
                                                           std::size_t bytes_saved) const {
    const auto snapshot_key = activeSnapshotKeyOrNullopt(storage_);
    if (!snapshot_key.has_value()) {
        return;
    }

    MutationJournalEntry entry;
    entry.type               = MutationJournalEntryType::Delete;
    entry.tensor_id          = tensor_id;
    entry.total_bytes_stored = total_bytes_stored;
    entry.bytes_saved        = bytes_saved;

    // Per-entry hook path: overwrite the entry for this tensor_id with DELETE.
    if (hasJournalEntryHooks()) {
        const auto payload = serializeMutationJournal({entry});
        std::lock_guard<std::mutex> hlk(journal_hooks_mutex_);
        if (journal_entry_persist_fn_) {
            try { journal_entry_persist_fn_(*snapshot_key, tensor_id, payload); }
            catch (...) {}
        }
        return;
    }

    std::vector<MutationJournalEntry> entries;
    loadJournalWithLegacyFallback(storage_, *snapshot_key, entries);
    entries.push_back(std::move(entry));
    compactMutationJournalEntries(entries);
    writeJournalAndClearLegacy(storage_, *snapshot_key, entries);
}

// ============================================================================
// Per-entry journal hooks
// ============================================================================

void TensorDeduplicationManager::setJournalEntryHooks(JournalEntryPersistFn persist_fn, JournalEntryDeleteFn delete_fn,
                                                      JournalEntryEnumerateFn enumerate_fn,
                                                      JournalEntryClearFn clear_fn) {
    std::lock_guard<std::mutex> lk(journal_hooks_mutex_);
    journal_entry_persist_fn_   = std::move(persist_fn);
    journal_entry_delete_fn_    = std::move(delete_fn);
    journal_entry_enumerate_fn_ = std::move(enumerate_fn);
    journal_entry_clear_fn_     = std::move(clear_fn);
}

bool TensorDeduplicationManager::hasJournalEntryHooks() const noexcept {
    // Hooks are mutable and may be reconfigured at runtime (e.g., tests
    // switching between per-entry and blob journaling), so the composite
    // readiness check must be guarded with the mutex.
    std::lock_guard<std::mutex> lk(journal_hooks_mutex_);
    return static_cast<bool>(journal_entry_persist_fn_) && static_cast<bool>(journal_entry_enumerate_fn_)
           && static_cast<bool>(journal_entry_clear_fn_);
}

} // namespace graph
} // namespace themis

namespace themis {
namespace graph {

namespace {
constexpr std::string_view kJournalEdgePrefix   = "__tfgjournal__";
constexpr std::string_view kJournalPayloadField = "__tfgjournal_payload_hex";
constexpr std::string_view kJournalType         = "__tfgjournal__";

inline std::string toHex(std::string_view text) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out = {};
    out.resize(text.size() * 2);
    for (std::size_t i = 0; i <static_cast<int>(text.size()); ++i) {
        const uint8_t byte = static_cast<uint8_t>(text[i]);
        out[(i * 2)]      = kHex[(byte >> 4) & 0x0FU];
        out[(i * 2) + 1] = kHex[byte & 0x0FU];
    }
    return out;
}

// Virtual source node in the graph from which all journal edges originate.
// Must remain colon-free because GraphIndex legacy out-key parsing splits on
// ':' and would otherwise reconstruct the wrong source node after topology
// rebuild.
inline std::string makeAnchorId(std::string_view snapshot_key) {
    return std::string("__tfgj_anchor__") + "_" + toHex(snapshot_key);
}

// Graph edge primary key (also used as edgeId in GraphIndexManager).
inline std::string makeEdgeId(std::string_view snapshot_key, std::string_view tensor_id) {
    return std::string(kJournalEdgePrefix) + "_" + toHex(snapshot_key) + "_" + toHex(tensor_id);
}

inline std::string toHex(const std::vector<uint8_t> &data) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out = {};
    out.resize(data.size() * 2);
    for (std::size_t i = 0; i <static_cast<int>(data.size()); ++i) {
        const uint8_t byte = data[i];
        out[(i * 2)]      = kHex[(byte >> 4) & 0x0FU];
        out[(i * 2) + 1] = kHex[byte & 0x0FU];
    }
    return out;
}

inline uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(10 + (c - 'a'));
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(10 + (c - 'A'));
    }
    return 0xFFU;
}

inline std::optional<std::vector<uint8_t>> fromHex(std::string_view hex) {
    if ((hex.size() % 2) != 0) {
        return std::nullopt;
    }
    std::vector<uint8_t> out = {};

    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i <static_cast<int>(hex.size()); i += 2) {
        const uint8_t hi = hexNibble(hex[i]);
        const uint8_t lo = hexNibble(hex[i + 1]);
        if (hi == 0xFFU || lo == 0xFFU) {
            return std::nullopt;
        }
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}
} // anonymous namespace

void wireGraphIndexJournalHooks(TensorDeduplicationManager &tdm, GraphIndexManager &graph_idx,
                                const std::string &snapshot_key) {
    const auto default_snapshot_key = snapshot_key;

    // ── persist_fn: store one journal entry ──────────────────────────────
    TensorDeduplicationManager::JournalEntryPersistFn persist_fn
        = [&graph_idx, default_snapshot_key](std::string_view snap, std::string_view tensor_id,
                                             const std::vector<uint8_t> &payload) -> bool {
        const auto effective_snap = snap.empty() ? std::string_view(default_snapshot_key) : snap;
        const auto edge_id        = makeEdgeId(effective_snap, tensor_id);
        const auto anchor         = makeAnchorId(effective_snap);

        // Represent the journal entry as a directed edge anchor → tensor_id
        // in GraphIndexManager (enables outAdjacency-based enumeration).
        BaseEntity edge(edge_id);
        edge.setField("id", themis::Value{std::string(edge_id)});
        edge.setField("_from", themis::Value{std::string(anchor)});
        edge.setField("_to", themis::Value{std::string(tensor_id)});
        edge.setField("_graph", themis::Value{std::string(effective_snap)});
        edge.setField("_type", themis::Value{std::string(kJournalType)});
        edge.setField(std::string(kJournalPayloadField), themis::Value{toHex(payload)});
        // Idempotent: deleteEdge before addEdge in case an entry already exists.
        (void)graph_idx.deleteEdge(edge_id);
        const auto status = graph_idx.addEdge(edge);
        return status.ok;
    };

    // ── delete_fn: remove one journal entry ──────────────────────────────
    TensorDeduplicationManager::JournalEntryDeleteFn delete_fn
        = [&graph_idx, default_snapshot_key](std::string_view snap, std::string_view tensor_id) -> bool {
        const auto effective_snap = snap.empty() ? std::string_view(default_snapshot_key) : snap;
        const auto edge_id        = makeEdgeId(effective_snap, tensor_id);
        const auto gstatus        = graph_idx.deleteEdge(edge_id);
        return gstatus.ok;
    };

    // ── enumerate_fn: iterate all journal entries for a snapshot ─────────
    TensorDeduplicationManager::JournalEntryEnumerateFn enumerate_fn
        = [&graph_idx, default_snapshot_key](std::string_view snap,
                                             std::function<void(std::string_view, const std::vector<uint8_t> &)> cb) {
              if (!cb) {
                  return;
              }
              const auto effective_snap = snap.empty() ? std::string_view(default_snapshot_key) : snap;
              const auto anchor         = makeAnchorId(effective_snap);
              auto [status, adj]        = graph_idx.outAdjacency(anchor);
              if (!status.ok) {
                  return;
              }
              for (const auto &info : adj) {
                  const auto payload_hex = graph_idx.getEdgeField(info.edgeId, kJournalPayloadField);
                  if (!payload_hex.has_value()) {
                      continue;
                  }
                  const auto payload = fromHex(*payload_hex);
                  if (!payload.has_value()) {
                      continue;
                  }
                  cb(info.targetPk, *payload);
              }
          };

    // ── clear_fn: delete all journal entries for a snapshot ──────────────
    TensorDeduplicationManager::JournalEntryClearFn clear_fn
        = [&graph_idx, default_snapshot_key](std::string_view snap) -> bool {
        const auto effective_snap = snap.empty() ? std::string_view(default_snapshot_key) : snap;
        const auto anchor         = makeAnchorId(effective_snap);
        auto [status, adj]        = graph_idx.outAdjacency(anchor);
        bool all_ok               = status.ok;
        for (const auto &info : adj) {
            const auto gs = graph_idx.deleteEdge(info.edgeId);
            all_ok        = all_ok && gs.ok;
        }
        return all_ok;
    };

    tdm.setJournalEntryHooks(std::move(persist_fn), std::move(delete_fn), std::move(enumerate_fn), std::move(clear_fn));
}

} // namespace graph
} // namespace themis

