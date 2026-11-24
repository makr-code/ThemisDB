/**
 * @file persistent_tensor_fingerprint_graph.h
 * @brief Persistent fingerprint graph for tensor deduplication.
 *
 * Maintains a durable graph of content-addressed tensor fingerprints
 * enabling cache-hit detection and deduplication across training runs.
 */

#pragma once

#include "storage/tensor_network_storage_engine.h"
#include "tensor/tensor_fingerprint_graph.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::tensor {

/**
 * @brief Durable wrapper for TensorFingerprintGraph using ITensorStorageBackend.
 *
 * Persists each fingerprint graph entry under tenant/domain scoped keys and uses
 * a small operation journal for recovery of partial writes. Callers must
 * serialize graph and backend mutations made through these wrappers.
 */
class PersistentTensorFingerprintGraph {
public:
    PersistentTensorFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph,
                                     std::shared_ptr<storage::ITensorStorageBackend> backend,
                                     std::string tenant_id,
                                     std::string domain);

    /**
     * @brief Recover the journal and replace this tenant/domain's graph entries.
     * @return False for invalid or out-of-scope journal/entry records or failed
     *         writes; unrelated tenant/domain graph entries are preserved.
     * @throws std::runtime_error when a backend reports a read or scan failure.
     * @note Callers must exclude concurrent graph reads and mutations, and
     *       backend mutations, throughout recovery. Recovered entries are
     *       validated before the existing graph is modified, but replacement
     *       is not atomic. Journal replay may modify the backend even if
     *       validation later fails.
     */
    [[nodiscard]] bool rehydrate();

    /**
     * @brief Persist an adapter and update the graph.
     * @param adapter_key Non-empty graph key, owned by this tenant and domain.
     * @param train Adapter tensor train.
     * @param base_model_id Adapter model identifier.
     * @return False on invalid input, a conflicting namespace, or a failed write.
     */
    [[nodiscard]] bool addAdapter(const std::string& adapter_key,
                                  const storage::TTTrain& train,
                                  const std::string& base_model_id);

    /**
     * @brief Remove an adapter belonging to this tenant and domain.
     * @param adapter_key Non-empty graph key.
     * @return False if absent, conflicting, or a storage operation fails.
     */
    [[nodiscard]] bool removeAdapter(const std::string& adapter_key);

    [[nodiscard]] std::shared_ptr<TensorFingerprintGraph> graph() const noexcept { return graph_; }

private:
    struct PersistedEntry {
        std::string adapter_key;
        std::string tenant_id;
        std::string domain;
        std::string base_model_id;
        std::vector<uint8_t> serialized_train;
    };

    enum class JournalOp : uint8_t { Put = 1, Delete = 2 };

    [[nodiscard]] std::string entryPrefix() const;
    [[nodiscard]] std::string entryKeyFor(const std::string& adapter_key) const;
    [[nodiscard]] std::string journalPrefix() const;

    [[nodiscard]] bool writeEntry(const PersistedEntry& entry);
    [[nodiscard]] bool deleteEntry(const std::string& adapter_key);
    [[nodiscard]] std::optional<PersistedEntry> readEntry(const std::string& key) const;
    [[nodiscard]] bool entryMatchesKey(const PersistedEntry& entry,
                                       const std::string& key) const;

    [[nodiscard]] bool writeWithJournal(JournalOp op,
                                        const std::string& target_key,
                                        const std::vector<uint8_t>& payload);
    [[nodiscard]] bool recoverJournal();

    static std::vector<uint8_t> serializeEntry(const PersistedEntry& entry);
    static std::optional<PersistedEntry> deserializeEntry(const std::vector<uint8_t>& bytes);

    static std::vector<uint8_t> serializeJournalRecord(JournalOp op,
                                                       const std::string& target_key,
                                                       const std::vector<uint8_t>& payload);
    static bool deserializeJournalRecord(const std::vector<uint8_t>& bytes,
                                         JournalOp& op,
                                         std::string& target_key,
                                         std::vector<uint8_t>& payload);

    static void appendU32(std::vector<uint8_t>& out, uint32_t v);
    static bool readU32(const std::vector<uint8_t>& in, std::size_t& off, uint32_t& v);
    static void appendString(std::vector<uint8_t>& out, const std::string& v);
    static bool readString(const std::vector<uint8_t>& in, std::size_t& off, std::string& v);
    static void appendBytes(std::vector<uint8_t>& out, const std::vector<uint8_t>& v);
    static bool readBytes(const std::vector<uint8_t>& in, std::size_t& off, std::vector<uint8_t>& v);

    std::shared_ptr<TensorFingerprintGraph> graph_;
    std::shared_ptr<storage::ITensorStorageBackend> backend_;
    std::string tenant_id_;
    std::string domain_;
};

}  // namespace themis::tensor
