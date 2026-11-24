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
 * a single operation journal for recovery of partial writes. Pending operations
 * are replayed before another write; conflicting journals for one target fail
 * closed. Callers must serialize graph and backend mutations made through these
 * wrappers.
 */
class PersistentTensorFingerprintGraph {
public:
    PersistentTensorFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph,
                                     std::shared_ptr<storage::ITensorStorageBackend> backend,
                                     std::string tenant_id,
                                     std::string domain);

    /**
     * @brief Recover the journal and replace this tenant/domain's graph entries.
     * @return False for invalid, out-of-scope, or ambiguous journal/entry records
     *         or failed writes; unrelated tenant/domain graph entries are
     *         preserved.
     * @throws std::runtime_error when a backend reports a read or scan failure.
     * @note Readers see either the previous or recovered graph contents.
     *       Callers must exclude concurrent graph and backend mutations
     *       throughout recovery. All journal records are validated before
     *       replay; replay may still modify the backend before a later write or
     *       cleanup failure leaves recovery incomplete.
     */
    [[nodiscard]] bool rehydrate();

    /**
     * @brief Persist an adapter and update the graph.
     * @param adapter_key Non-empty graph key, owned by this tenant and domain.
     * @param train Adapter tensor train.
     * @param base_model_id Adapter model identifier.
     * @return True when the entry is written or retained as a replayable
     *         operation; false on invalid input, a conflicting namespace, or
     *         failure before the operation is accepted.
     */
    [[nodiscard]] bool addAdapter(const std::string& adapter_key,
                                  const storage::TTTrain& train,
                                  const std::string& base_model_id);

    /**
     * @brief Remove an adapter belonging to this tenant and domain.
     * @param adapter_key Non-empty graph key.
     * @return True when removal is written or retained as a replayable
     *         operation; false if absent, conflicting, or the operation cannot
     *         be accepted.
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
