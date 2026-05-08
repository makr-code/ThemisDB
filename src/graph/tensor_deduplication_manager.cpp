/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_deduplication_manager.cpp                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "graph/tensor_deduplication_manager.h"
#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace themis {
namespace graph {

using storage::TTTrain;
using storage::TensorFieldKey;
using storage::TensorTrainDecomposer;
using storage::TensorTrainConfig;

// ============================================================================
// Construction
// ============================================================================

TensorDeduplicationManager::TensorDeduplicationManager(
    std::shared_ptr<storage::TensorNetworkStorageEngine> storage,
    std::shared_ptr<TensorFingerprintGraph>              fp_graph,
    std::shared_ptr<TensorTrainDecomposer>               decomposer,
    const DeduplicationConfig&                           cfg)
    : storage_(std::move(storage))
    , fp_graph_(std::move(fp_graph))
    , decomposer_(std::move(decomposer))
    , cfg_(cfg)
{
    if (!storage_ || !fp_graph_ || !decomposer_)
        throw std::invalid_argument("TensorDeduplicationManager: null dependency");

    fp_graph_->setTrainLoadFn(
        [storage = storage_](const std::string&,
                             const std::string& tenant,
                             const std::string& collection,
                             const std::string& field)
            -> std::optional<TTTrain> {
            if (!storage) return std::nullopt;
            auto qtrain = storage->getCompressed({tenant, collection, field});
            if (!qtrain.has_value()) return std::nullopt;
            storage::TTQuantizer quantizer;
            return quantizer.dequantize(*qtrain);
        });

    storage_->setWriteObserverFn(
        [this](const TensorFieldKey& key, const TTTrain& train) {
            std::string tensor_id;
            std::optional<StoredTensorRecord> record;
            std::size_t total_bytes_stored = 0;
            std::size_t bytes_saved = 0;
            {
                std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
                auto it = key_to_tensor_id_.find(makeKeyIndex(key));
                if (it == key_to_tensor_id_.end()) return;
                tensor_id = it->second;
                auto record_it = records_.find(tensor_id);
                if (record_it == records_.end()) return;
                record = record_it->second;
                total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
                bytes_saved = bytes_saved_.load(std::memory_order_relaxed);
            }
            fp_graph_->insert(tensor_id, train, key.tenant, key.collection, key.field);
            persistUpsertJournalEntry(*record, total_bytes_stored, bytes_saved);
        });

    storage_->setDeleteObserverFn(
        [this](const TensorFieldKey& key) {
            std::string tensor_id;
            std::size_t total_bytes_stored = 0;
            std::size_t bytes_saved = 0;
            const auto post_subtracted_value =
                [](std::atomic<std::size_t>& counter, std::size_t value) {
                    return counter.fetch_sub(value, std::memory_order_relaxed) - value;
                };
            {
                std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
                auto it = key_to_tensor_id_.find(makeKeyIndex(key));
                if (it == key_to_tensor_id_.end()) return;
                tensor_id = it->second;
                const auto record_it = records_.find(tensor_id);
                if (record_it != records_.end()) {
                    const auto& record = record_it->second;
                    total_bytes_stored =
                        post_subtracted_value(total_bytes_stored_, record.compressed_bytes);
                    bytes_saved =
                        post_subtracted_value(bytes_saved_, record.saved_bytes);
                } else {
                    total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
                    bytes_saved = bytes_saved_.load(std::memory_order_relaxed);
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
        storage_->setWriteObserverFn(nullptr);
        storage_->setDeleteObserverFn(nullptr);
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

TensorFieldKey TensorDeduplicationManager::makeKey(
    const std::string& tenant,
    const std::string& collection,
    const std::string& field) const {
    return {tenant, collection, field};
}

std::string TensorDeduplicationManager::makeKeyIndex(const TensorFieldKey& key) const {
    // Use ASCII Unit Separator to avoid ambiguity with user-provided key chars.
    constexpr char kSep = '\x1f';
    return key.tenant + kSep + key.collection + kSep + key.field;
}

void TensorDeduplicationManager::clearMappingForTensorIdLocked(
    const std::string& tensor_id) {
    auto key_it = tensor_id_to_key_.find(tensor_id);
    if (key_it == tensor_id_to_key_.end()) return;
    key_to_tensor_id_.erase(key_it->second);
    tensor_id_to_key_.erase(key_it);
}

TTTrain TensorDeduplicationManager::computeDelta(
    const TTTrain& ref,
    const TTTrain& new_train) const {

    // Reconstruct both (in production this would use TT-arithmetic)
    auto ref_dense = ref.reconstruct();
    auto new_dense = new_train.reconstruct();

    if (ref_dense.size() != new_dense.size()) {
        // Incompatible shapes; return new_train unchanged (no delta possible)
        return new_train;
    }

    // Delta = new - ref (element-wise)
    std::vector<float> delta_dense(ref_dense.size());
    for (std::size_t i = 0; i < delta_dense.size(); ++i)
        delta_dense[i] = new_dense[i] - ref_dense[i];

    // Re-compress delta
    TensorTrainConfig delta_cfg;
    delta_cfg.eps      = cfg_.delta_eps;
    delta_cfg.max_rank = cfg_.delta_max_rank;

    auto [delta_train, stats] = decomposer_->decompose(delta_dense,
                                                        new_train.mode_sizes,
                                                        delta_cfg);
    return std::move(delta_train);
}

TTTrain TensorDeduplicationManager::addTrains(
    const TTTrain& a, const TTTrain& b) const {

    auto da = a.reconstruct();
    auto db = b.reconstruct();

    if (da.size() != db.size()) return a;  // incompatible

    std::vector<float> sum(da.size());
    for (std::size_t i = 0; i < da.size(); ++i) sum[i] = da[i] + db[i];

    TensorTrainConfig cfg;
    cfg.eps = cfg_.delta_eps;
    auto [t, stats] = decomposer_->decompose(sum, a.mode_sizes, cfg);
    return std::move(t);
}

// ============================================================================
// store
// ============================================================================

StoredTensorRecord TensorDeduplicationManager::store(
    const std::string&              tensor_id,
    const std::vector<float>&       data,
    const std::vector<std::size_t>& mode_sizes,
    const std::string&              tenant,
    const std::string&              collection,
    const std::string&              field)
{
    // Decompose the new tensor
    TensorTrainConfig cfg;
    cfg.eps = cfg_.delta_eps * 10.0;  // slightly looser for the fingerprint
    auto [new_train, stats] = decomposer_->decompose(data, mode_sizes, cfg);

    // Find similar tensors via fingerprint graph
    auto similar = fp_graph_->findSimilar(new_train, 1);

    StoredTensorRecord record;
    record.tensor_id   = tensor_id;
    record.is_canonical = true;

    std::size_t full_bytes = data.size() * sizeof(float);

    if (!similar.empty() && similar[0].similarity >= cfg_.similarity_threshold) {
        // Found a reference — store delta
        const std::string& ref_id = similar[0].tensor_id;

        // Load reference compressed train for delta computation
        std::string ref_collection = similar[0].collection;
        std::string ref_tenant     = similar[0].tenant;
        std::string ref_field      = similar[0].field;

        auto ref_dense_opt = storage_->get(makeKey(ref_tenant, ref_collection, ref_field));
        if (ref_dense_opt) {
            // Build reference TTTrain (re-decompose the retrieved dense data)
            auto ref_ms = mode_sizes;  // assume compatible shapes
            TensorTrainConfig ref_cfg;
            ref_cfg.eps = cfg_.delta_eps;
            auto [ref_train, _] = decomposer_->decompose(*ref_dense_opt, ref_ms, ref_cfg);

            TTTrain delta = computeDelta(ref_train, new_train);

            // Store delta under a delta field name
            std::string delta_field = field + "__delta__" + ref_id;
            TensorFieldKey delta_key = makeKey(tenant, collection, delta_field);
            auto delta_dense = delta.reconstruct();
            storage_->put(delta_key, delta_dense, mode_sizes);

            std::size_t delta_bytes = delta.totalParams() * sizeof(float);

            record.reference_id              = ref_id;
            record.is_canonical              = false;
            record.compressed_bytes          = delta_bytes;
            record.saved_bytes               = (full_bytes > delta_bytes) ? full_bytes - delta_bytes : 0;
            record.similarity_to_reference   = similar[0].similarity;
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
        record.compressed_bytes = stored_bytes;
        record.saved_bytes      = (full_bytes > stored_bytes) ? full_bytes - stored_bytes : 0;
    }

    // Insert into fingerprint graph
    fp_graph_->insert(tensor_id, new_train, tenant, collection, field);

    // Persist key fields so retrieve() can look up the tensor without an extra index
    record.tenant     = tenant;
    record.collection = collection;
    record.field      = field;

    // Store record
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved = 0;
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
        const auto idx = makeKeyIndex(makeKey(record.tenant, record.collection, record.field));
        key_to_tensor_id_[idx] = tensor_id;
        tensor_id_to_key_[tensor_id] = idx;
    }
    total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
    bytes_saved = bytes_saved_.load(std::memory_order_relaxed);
    wlk.unlock();

    persistUpsertJournalEntry(record, total_bytes_stored, bytes_saved);

    return record;
}

// ============================================================================
// retrieve
// ============================================================================

std::optional<std::vector<float>>
TensorDeduplicationManager::retrieve(const std::string& tensor_id) const {
    // Copy the record while holding the shared lock so that we do NOT hold
    // the lock while calling storage_->get() — mixing the rw_mutex_ shared
    // lock with the storage engine's own write lock (held during put()) would
    // otherwise create a potential deadlock.
    StoredTensorRecord rec;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        auto it = records_.find(tensor_id);
        if (it == records_.end()) return std::nullopt;
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
        if (ref_it == records_.end()) return std::nullopt;
        ref_rec = ref_it->second;
    }

    // 2. Load canonical reference dense vector from storage.
    auto ref_opt = storage_->get(makeKey(ref_rec.tenant, ref_rec.collection, ref_rec.field));
    if (!ref_opt) return std::nullopt;

    // 3. Load delta (stored under field + "__delta__" + reference_id).
    const std::string delta_field = rec.field + "__delta__" + rec.reference_id;
    auto delta_opt = storage_->get(makeKey(rec.tenant, rec.collection, delta_field));
    if (!delta_opt) return std::nullopt;

    if (ref_opt->size() != delta_opt->size()) return std::nullopt;

    // 4. Reconstruct: result = reference + delta (element-wise).
    std::vector<float> result(ref_opt->size());
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = (*ref_opt)[i] + (*delta_opt)[i];
    return result;
}

std::optional<StoredTensorRecord>
TensorDeduplicationManager::getRecord(const std::string& tensor_id) const {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    auto it = records_.find(tensor_id);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}

// ============================================================================
// getStats
// ============================================================================

DeduplicationStats TensorDeduplicationManager::getStats() const noexcept {
    std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
    DeduplicationStats s;
    s.total_tensors     = records_.size();
    for (const auto& kv : records_) {
        if (kv.second.is_canonical) ++s.canonical_tensors;
        else                        ++s.delta_tensors;
    }
    s.total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
    s.bytes_saved        = bytes_saved_.load(std::memory_order_relaxed);
    std::size_t full_bytes = s.total_bytes_stored + s.bytes_saved;
    s.dedup_ratio = (s.total_bytes_stored > 0)
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

constexpr uint64_t kGraphSnapshotMagic   = 0x504E535F47465400ULL; // "TFG_SNP\0"
constexpr uint32_t kGraphSnapshotVersion = 1;
constexpr uint64_t kDedupSnapshotMagic   = 0x504E535F4D445400ULL; // "TDM_SNP\0"
constexpr uint32_t kDedupSnapshotVersion = 1;
constexpr uint64_t kMutationJournalMagic = 0x4A4E4C5F4D445400ULL; // "TDM_JNL\0"
constexpr uint32_t kMutationJournalVersion = 1;
constexpr char kActiveSnapshotMetaKey[] = "__tfg_active_snapshot__";

enum class MutationJournalEntryType : uint8_t {
    Upsert = 1,
    Delete = 2,
};

struct MutationJournalEntry {
    MutationJournalEntryType type = MutationJournalEntryType::Upsert;
    themis::graph::StoredTensorRecord record;
    themis::graph::PersistedFingerprintNode node;
    std::vector<themis::graph::PersistedFingerprintEdge> edges;
    std::string tensor_id;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved = 0;
};

// Little-endian write helpers.
template<typename T>
static void writeLE(std::vector<uint8_t>& buf, T val) {
    static_assert(std::is_trivially_copyable<T>::value);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&val);
    for (std::size_t i = 0; i < sizeof(T); ++i) buf.push_back(p[i]);
}

static void writeStr(std::vector<uint8_t>& buf, const std::string& s) {
    writeLE<uint64_t>(buf, static_cast<uint64_t>(s.size()));
    for (unsigned char c : s) buf.push_back(c);
}

// Little-endian read helpers.
template<typename T>
static bool readLE(const uint8_t* data, std::size_t size,
                   std::size_t& pos, T& out) {
    if (pos + sizeof(T) > size) return false;
    std::memcpy(&out, data + pos, sizeof(T));
    pos += sizeof(T);
    return true;
}

static bool readStr(const uint8_t* data, std::size_t size,
                    std::size_t& pos, std::string& out) {
    uint64_t len = 0;
    if (!readLE(data, size, pos, len)) return false;
    if (pos + len > size) return false;
    out.assign(reinterpret_cast<const char*>(data + pos), static_cast<std::size_t>(len));
    pos += static_cast<std::size_t>(len);
    return true;
}

static std::vector<uint8_t> serializeGraphSnapshot(
    const themis::graph::PersistedFingerprintGraphSnapshot& snapshot)
{
    std::vector<uint8_t> buf;
    buf.reserve(4096);

    writeLE<uint64_t>(buf, kGraphSnapshotMagic);
    writeLE<uint32_t>(buf, kGraphSnapshotVersion);

    writeLE<uint64_t>(buf, static_cast<uint64_t>(snapshot.nodes.size()));
    for (const auto& n : snapshot.nodes) {
        writeStr(buf, n.tensor_id);

        // fingerprint.minhash (128 × uint64_t)
        for (const auto h : n.fingerprint.minhash) writeLE<uint64_t>(buf, h);

        // fingerprint.core_norms
        writeLE<uint32_t>(buf, static_cast<uint32_t>(n.fingerprint.core_norms.size()));
        for (float f : n.fingerprint.core_norms) writeLE<float>(buf, f);

        writeLE<float>(buf, n.fingerprint.total_norm);
        writeLE<uint64_t>(buf, static_cast<uint64_t>(n.fingerprint.order));
        writeLE<uint64_t>(buf, static_cast<uint64_t>(n.fingerprint.max_rank));

        writeStr(buf, n.tenant);
        writeStr(buf, n.collection);
        writeStr(buf, n.field);
    }

    writeLE<uint64_t>(buf, static_cast<uint64_t>(snapshot.edges.size()));
    for (const auto& e : snapshot.edges) {
        writeStr(buf, e.from);
        writeStr(buf, e.to);
        writeLE<double>(buf, e.similarity);
    }

    return buf;
}

static void writeStoredTensorRecord(std::vector<uint8_t>& buf,
                                    const themis::graph::StoredTensorRecord& record) {
    writeStr(buf, record.tensor_id);
    writeStr(buf, record.reference_id);
    writeLE<uint8_t>(buf, record.is_canonical ? 1U : 0U);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(record.compressed_bytes));
    writeLE<uint64_t>(buf, static_cast<uint64_t>(record.saved_bytes));
    writeLE<double>(buf, record.similarity_to_reference);
    writeStr(buf, record.tenant);
    writeStr(buf, record.collection);
    writeStr(buf, record.field);
}

static void writePersistedFingerprintNode(
    std::vector<uint8_t>& buf,
    const themis::graph::PersistedFingerprintNode& node) {
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

static void writePersistedFingerprintEdge(
    std::vector<uint8_t>& buf,
    const themis::graph::PersistedFingerprintEdge& edge) {
    writeStr(buf, edge.from);
    writeStr(buf, edge.to);
    writeLE<double>(buf, edge.similarity);
}

static std::vector<uint8_t> serializeMutationJournal(
    const std::vector<MutationJournalEntry>& entries) {
    std::vector<uint8_t> buf;
    buf.reserve(entries.size() * 256U + 64U);
    writeLE<uint64_t>(buf, kMutationJournalMagic);
    writeLE<uint32_t>(buf, kMutationJournalVersion);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(entries.size()));
    for (const auto& entry : entries) {
        writeLE<uint8_t>(buf, static_cast<uint8_t>(entry.type));
        if (entry.type == MutationJournalEntryType::Upsert) {
            writeStoredTensorRecord(buf, entry.record);
            writePersistedFingerprintNode(buf, entry.node);
            writeLE<uint64_t>(buf, static_cast<uint64_t>(entry.edges.size()));
            for (const auto& edge : entry.edges) {
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

static std::vector<uint8_t> serializeDedupSnapshot(
    const themis::graph::PersistedFingerprintGraphSnapshot& snapshot,
    const std::vector<themis::graph::StoredTensorRecord>& records,
    std::size_t total_bytes_stored,
    std::size_t bytes_saved)
{
    auto graph_bytes = serializeGraphSnapshot(snapshot);

    std::vector<uint8_t> buf;
    buf.reserve(graph_bytes.size() + records.size() * 256U + 64U);
    writeLE<uint64_t>(buf, kDedupSnapshotMagic);
    writeLE<uint32_t>(buf, kDedupSnapshotVersion);
    writeLE<uint64_t>(buf, static_cast<uint64_t>(graph_bytes.size()));
    buf.insert(buf.end(), graph_bytes.begin(), graph_bytes.end());

    writeLE<uint64_t>(buf, static_cast<uint64_t>(records.size()));
    for (const auto& record : records) {
        writeStoredTensorRecord(buf, record);
    }

    writeLE<uint64_t>(buf, static_cast<uint64_t>(total_bytes_stored));
    writeLE<uint64_t>(buf, static_cast<uint64_t>(bytes_saved));
    return buf;
}

static bool deserializeGraphSnapshot(
    const std::vector<uint8_t>& buf,
    themis::graph::PersistedFingerprintGraphSnapshot& snapshot)
{
    std::size_t pos = 0;
    const uint8_t* data = buf.data();
    const std::size_t size = buf.size();

    uint64_t magic = 0;
    uint32_t ver   = 0;
    if (!readLE(data, size, pos, magic)) return false;
    if (magic != kGraphSnapshotMagic) return false;
    if (!readLE(data, size, pos, ver)) return false;
    if (ver != kGraphSnapshotVersion) return false;

    uint64_t node_count = 0;
    if (!readLE(data, size, pos, node_count)) return false;

    snapshot.nodes.clear();
    snapshot.nodes.reserve(static_cast<std::size_t>(node_count));

    for (uint64_t i = 0; i < node_count; ++i) {
        themis::graph::PersistedFingerprintNode n;
        if (!readStr(data, size, pos, n.tensor_id)) return false;

        for (auto& h : n.fingerprint.minhash)
            if (!readLE(data, size, pos, h)) return false;

        uint32_t core_norms_len = 0;
        if (!readLE(data, size, pos, core_norms_len)) return false;
        n.fingerprint.core_norms.resize(core_norms_len);
        for (auto& f : n.fingerprint.core_norms)
            if (!readLE(data, size, pos, f)) return false;

        if (!readLE(data, size, pos, n.fingerprint.total_norm)) return false;

        uint64_t order = 0, max_rank = 0;
        if (!readLE(data, size, pos, order)) return false;
        if (!readLE(data, size, pos, max_rank)) return false;
        n.fingerprint.order    = static_cast<std::size_t>(order);
        n.fingerprint.max_rank = static_cast<std::size_t>(max_rank);

        if (!readStr(data, size, pos, n.tenant)) return false;
        if (!readStr(data, size, pos, n.collection)) return false;
        if (!readStr(data, size, pos, n.field)) return false;

        snapshot.nodes.push_back(std::move(n));
    }

    uint64_t edge_count = 0;
    if (!readLE(data, size, pos, edge_count)) return false;

    snapshot.edges.clear();
    snapshot.edges.reserve(static_cast<std::size_t>(edge_count));

    for (uint64_t i = 0; i < edge_count; ++i) {
        themis::graph::PersistedFingerprintEdge e;
        if (!readStr(data, size, pos, e.from)) return false;
        if (!readStr(data, size, pos, e.to)) return false;
        if (!readLE(data, size, pos, e.similarity)) return false;
        snapshot.edges.push_back(std::move(e));
    }

    return true;
}

static bool readStoredTensorRecord(const uint8_t* data,
                                   std::size_t size,
                                   std::size_t& pos,
                                   themis::graph::StoredTensorRecord& record) {
    if (!readStr(data, size, pos, record.tensor_id)) return false;
    if (!readStr(data, size, pos, record.reference_id)) return false;

    uint8_t is_canonical = 0;
    uint64_t compressed_bytes = 0;
    uint64_t saved_bytes = 0;
    if (!readLE(data, size, pos, is_canonical)) return false;
    if (!readLE(data, size, pos, compressed_bytes)) return false;
    if (!readLE(data, size, pos, saved_bytes)) return false;
    if (!readLE(data, size, pos, record.similarity_to_reference)) return false;
    if (!readStr(data, size, pos, record.tenant)) return false;
    if (!readStr(data, size, pos, record.collection)) return false;
    if (!readStr(data, size, pos, record.field)) return false;

    record.is_canonical = (is_canonical != 0);
    record.compressed_bytes = static_cast<std::size_t>(compressed_bytes);
    record.saved_bytes = static_cast<std::size_t>(saved_bytes);
    return true;
}

static bool readPersistedFingerprintNode(
    const uint8_t* data,
    std::size_t size,
    std::size_t& pos,
    themis::graph::PersistedFingerprintNode& node) {
    if (!readStr(data, size, pos, node.tensor_id)) return false;
    for (auto& hash : node.fingerprint.minhash) {
        if (!readLE(data, size, pos, hash)) return false;
    }

    uint32_t core_norm_count = 0;
    if (!readLE(data, size, pos, core_norm_count)) return false;
    node.fingerprint.core_norms.resize(core_norm_count);
    for (auto& core_norm : node.fingerprint.core_norms) {
        if (!readLE(data, size, pos, core_norm)) return false;
    }

    if (!readLE(data, size, pos, node.fingerprint.total_norm)) return false;

    uint64_t order = 0;
    uint64_t max_rank = 0;
    if (!readLE(data, size, pos, order)) return false;
    if (!readLE(data, size, pos, max_rank)) return false;
    node.fingerprint.order = static_cast<std::size_t>(order);
    node.fingerprint.max_rank = static_cast<std::size_t>(max_rank);

    if (!readStr(data, size, pos, node.tenant)) return false;
    if (!readStr(data, size, pos, node.collection)) return false;
    if (!readStr(data, size, pos, node.field)) return false;
    return true;
}

static bool readPersistedFingerprintEdge(
    const uint8_t* data,
    std::size_t size,
    std::size_t& pos,
    themis::graph::PersistedFingerprintEdge& edge) {
    if (!readStr(data, size, pos, edge.from)) return false;
    if (!readStr(data, size, pos, edge.to)) return false;
    return readLE(data, size, pos, edge.similarity);
}

static bool deserializeMutationJournal(
    const std::vector<uint8_t>& buf,
    std::vector<MutationJournalEntry>& entries) {
    std::size_t pos = 0;
    const auto* data = buf.data();
    const auto size = buf.size();

    uint64_t magic = 0;
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
            if (!readStoredTensorRecord(data, size, pos, entry.record)) return false;
            if (!readPersistedFingerprintNode(data, size, pos, entry.node)) return false;

            uint64_t edge_count = 0;
            if (!readLE(data, size, pos, edge_count)) return false;
            entry.edges.reserve(static_cast<std::size_t>(edge_count));
            for (uint64_t edge_idx = 0; edge_idx < edge_count; ++edge_idx) {
                themis::graph::PersistedFingerprintEdge edge;
                if (!readPersistedFingerprintEdge(data, size, pos, edge)) return false;
                entry.edges.push_back(std::move(edge));
            }
        } else if (entry.type == MutationJournalEntryType::Delete) {
            if (!readStr(data, size, pos, entry.tensor_id)) return false;
        } else {
            return false;
        }

        uint64_t total_bytes_stored = 0;
        uint64_t bytes_saved = 0;
        if (!readLE(data, size, pos, total_bytes_stored)) return false;
        if (!readLE(data, size, pos, bytes_saved)) return false;
        entry.total_bytes_stored = static_cast<std::size_t>(total_bytes_stored);
        entry.bytes_saved = static_cast<std::size_t>(bytes_saved);
        entries.push_back(std::move(entry));
    }

    return pos == size;
}

static void loadOrResetJournal(
    const std::shared_ptr<themis::storage::TensorNetworkStorageEngine>& storage,
    const std::string& journal_key,
    std::vector<MutationJournalEntry>& entries) {
    entries.clear();
    if (!storage) {
        return;
    }

    const auto existing = storage->getRawMetadata(journal_key);
    if (!existing || existing->empty()) {
        return;
    }
    if (deserializeMutationJournal(*existing, entries)) {
        return;
    }

    THEMIS_WARN("[TensorDeduplicationManager] mutation journal parse failed for key='{}' ({} bytes); resetting journal payload",
                journal_key,
                existing->size());
    entries.clear();
}

static bool deserializeDedupSnapshot(
    const std::vector<uint8_t>& buf,
    themis::graph::PersistedFingerprintGraphSnapshot& snapshot,
    std::vector<themis::graph::StoredTensorRecord>& records,
    std::size_t& total_bytes_stored,
    std::size_t& bytes_saved)
{
    std::size_t pos = 0;
    const auto* data = buf.data();
    const auto size = buf.size();

    uint64_t magic = 0;
    uint32_t version = 0;
    if (!readLE(data, size, pos, magic)) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: failed to read dedup magic");
        return false;
    }
    if (magic != kDedupSnapshotMagic) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: dedup magic mismatch (expected={}, actual={})",
                     kDedupSnapshotMagic,
                     magic);
        return false;
    }
    if (!readLE(data, size, pos, version)) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: failed to read dedup version");
        return false;
    }
    if (version != kDedupSnapshotVersion) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: unsupported dedup version (expected={}, actual={})",
                     kDedupSnapshotVersion,
                     version);
        return false;
    }

    uint64_t graph_bytes_size = 0;
    if (!readLE(data, size, pos, graph_bytes_size)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read graph payload length");
        return false;
    }
    if (pos + graph_bytes_size > size) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: graph payload length {} exceeds buffer size {}",
                    graph_bytes_size,
                    size - pos);
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
    uint64_t bytes_saved_u64 = 0;
    if (!readLE(data, size, pos, total_bytes_stored_u64)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read total_bytes_stored");
        return false;
    }
    if (!readLE(data, size, pos, bytes_saved_u64)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: failed to read bytes_saved");
        return false;
    }

    total_bytes_stored = static_cast<std::size_t>(total_bytes_stored_u64);
    bytes_saved = static_cast<std::size_t>(bytes_saved_u64);
    if (pos != size) {
        const auto trailing_bytes = size - pos;
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: trailing bytes detected ({})",
                    trailing_bytes);
        return false;
    }
    return true;
}

} // namespace

// ============================================================================
// snapshotGraph / restoreGraph
// ============================================================================

bool TensorDeduplicationManager::snapshotGraph(const std::string& snapshot_key) {
    auto snapshot = fp_graph_->exportPersistedGraph();

    std::vector<StoredTensorRecord> records;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved = 0;
    {
        std::shared_lock<std::shared_mutex> rlk(rw_mutex_);
        records.reserve(records_.size());
        for (const auto& [_, record] : records_) {
            records.push_back(record);
        }
        total_bytes_stored = total_bytes_stored_.load(std::memory_order_relaxed);
        bytes_saved = bytes_saved_.load(std::memory_order_relaxed);
    }

    auto bytes = serializeDedupSnapshot(snapshot,
                                         records,
                                         total_bytes_stored,
                                         bytes_saved);
    if (!storage_->putRawMetadata(snapshot_key, bytes)) {
        return false;
    }
    clearMutationJournal(snapshot_key);
    activateSnapshotKey(snapshot_key);
    return true;
}

bool TensorDeduplicationManager::restoreGraph(const std::string& snapshot_key) {
    auto bytes_opt = storage_->getRawMetadata(snapshot_key);
    if (!bytes_opt) return false;

    PersistedFingerprintGraphSnapshot snapshot;
    std::vector<StoredTensorRecord> records;
    std::size_t total_bytes_stored = 0;
    std::size_t bytes_saved = 0;

    if (!deserializeDedupSnapshot(*bytes_opt,
                                  snapshot,
                                  records,
                                  total_bytes_stored,
                                  bytes_saved)) {
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: dedup payload parse failed, trying legacy graph-only payload");
        if (!deserializeGraphSnapshot(*bytes_opt, snapshot)) {
            THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: both dedup and legacy graph payload parsing failed");
            return false;
        }
        THEMIS_DEBUG("[TensorDeduplicationManager] restore snapshot: legacy graph-only payload restored");
    }

    fp_graph_->importPersistedGraph(snapshot);

    std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
    records_.clear();
    key_to_tensor_id_.clear();
    tensor_id_to_key_.clear();
    for (const auto& record : records) {
        records_[record.tensor_id] = record;
        if (!record.is_canonical) {
            continue;
        }

        const auto key =
            makeKey(record.tenant, record.collection, record.field);
        const auto key_index = makeKeyIndex(key);
        key_to_tensor_id_[key_index] = record.tensor_id;
        tensor_id_to_key_[record.tensor_id] = key_index;
    }
    total_bytes_stored_.store(total_bytes_stored, std::memory_order_relaxed);
    bytes_saved_.store(bytes_saved, std::memory_order_relaxed);
    if (!replayMutationJournal(snapshot_key)) {
        return false;
    }
    activateSnapshotKey(snapshot_key);
    return true;
}

bool TensorDeduplicationManager::replayMutationJournal(const std::string& snapshot_key) {
    const auto journal_key = snapshot_key + "::wal";
    const auto bytes_opt = storage_->getRawMetadata(journal_key);
    if (!bytes_opt || bytes_opt->empty()) {
        return true;
    }

    std::vector<MutationJournalEntry> entries;
    if (!deserializeMutationJournal(*bytes_opt, entries)) {
        THEMIS_WARN("[TensorDeduplicationManager] restore snapshot: mutation journal is invalid");
        return false;
    }

    for (const auto& entry : entries) {
        if (entry.type == MutationJournalEntryType::Upsert) {
            fp_graph_->upsertPersistedNode(entry.node, entry.edges);

            std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
            clearMappingForTensorIdLocked(entry.record.tensor_id);
            records_[entry.record.tensor_id] = entry.record;
            if (entry.record.is_canonical) {
                const auto key = makeKey(entry.record.tenant,
                                         entry.record.collection,
                                         entry.record.field);
                const auto key_index = makeKeyIndex(key);
                key_to_tensor_id_[key_index] = entry.record.tensor_id;
                tensor_id_to_key_[entry.record.tensor_id] = key_index;
            }
            total_bytes_stored_.store(entry.total_bytes_stored, std::memory_order_relaxed);
            bytes_saved_.store(entry.bytes_saved, std::memory_order_relaxed);
            continue;
        }

        {
            std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
            clearMappingForTensorIdLocked(entry.tensor_id);
            records_.erase(entry.tensor_id);
            total_bytes_stored_.store(entry.total_bytes_stored, std::memory_order_relaxed);
            bytes_saved_.store(entry.bytes_saved, std::memory_order_relaxed);
        }
        fp_graph_->remove(entry.tensor_id);
    }

    return true;
}

void TensorDeduplicationManager::activateSnapshotKey(
    const std::string& snapshot_key) const {
    storage_->putRawMetadata(kActiveSnapshotMetaKey,
                             std::vector<uint8_t>(snapshot_key.begin(), snapshot_key.end()));
}

void TensorDeduplicationManager::clearMutationJournal(
    const std::string& snapshot_key) const {
    storage_->putRawMetadata(snapshot_key + "::wal", {});
}

void TensorDeduplicationManager::persistUpsertJournalEntry(
    const StoredTensorRecord& record,
    std::size_t total_bytes_stored,
    std::size_t bytes_saved) const {
    const auto active_snapshot_opt = storage_->getRawMetadata(kActiveSnapshotMetaKey);
    if (!active_snapshot_opt || active_snapshot_opt->empty()) {
        return;
    }

    const std::string snapshot_key(active_snapshot_opt->begin(), active_snapshot_opt->end());
    const auto node_opt = fp_graph_->exportPersistedNode(record.tensor_id);
    if (!node_opt.has_value()) {
        return;
    }

    MutationJournalEntry entry;
    entry.type = MutationJournalEntryType::Upsert;
    entry.record = record;
    entry.node = *node_opt;
    entry.edges = fp_graph_->exportPersistedEdgesFor(record.tensor_id);
    entry.total_bytes_stored = total_bytes_stored;
    entry.bytes_saved = bytes_saved;

    std::vector<MutationJournalEntry> entries;
    const auto journal_key = snapshot_key + "::wal";
    loadOrResetJournal(storage_, journal_key, entries);
    entries.push_back(std::move(entry));
    storage_->putRawMetadata(journal_key, serializeMutationJournal(entries));
}

void TensorDeduplicationManager::persistDeleteJournalEntry(
    const std::string& tensor_id,
    std::size_t total_bytes_stored,
    std::size_t bytes_saved) const {
    const auto active_snapshot_opt = storage_->getRawMetadata(kActiveSnapshotMetaKey);
    if (!active_snapshot_opt || active_snapshot_opt->empty()) {
        return;
    }

    MutationJournalEntry entry;
    entry.type = MutationJournalEntryType::Delete;
    entry.tensor_id = tensor_id;
    entry.total_bytes_stored = total_bytes_stored;
    entry.bytes_saved = bytes_saved;

    const std::string snapshot_key(active_snapshot_opt->begin(), active_snapshot_opt->end());
    std::vector<MutationJournalEntry> entries;
    const auto journal_key = snapshot_key + "::wal";
    loadOrResetJournal(storage_, journal_key, entries);
    entries.push_back(std::move(entry));
    storage_->putRawMetadata(journal_key, serializeMutationJournal(entries));
}

} // namespace graph
} // namespace themis
