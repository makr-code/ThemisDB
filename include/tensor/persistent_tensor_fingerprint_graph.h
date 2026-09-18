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
 * a small operation journal for crash-safe recovery of partial writes.
 */
class PersistentTensorFingerprintGraph {
public:
    PersistentTensorFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph,
                                     std::shared_ptr<storage::ITensorStorageBackend> backend,
                                     std::string tenant_id,
                                     std::string domain);

    [[nodiscard]] bool rehydrate();

    [[nodiscard]] bool addAdapter(const std::string& adapter_key,
                                  const storage::TTTrain& train,
                                  const std::string& base_model_id);

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

    [[nodiscard]] bool writeWithJournal(JournalOp op,
                                        const std::string& target_key,
                                        const std::vector<uint8_t>& payload);
    [[nodiscard]] bool recoverJournal();

    /**
     * @brief TBD: Describe serializeEntry.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> serializeEntry(const PersistedEntry& entry);
    /**
     * @brief TBD: Describe deserializeEntry.
     * @param[in] bytes Input parameter.
     * @return Return value.
     */
    static std::optional<PersistedEntry> deserializeEntry(const std::vector<uint8_t>& bytes);

    /**
     * @brief TBD: Describe serializeJournalRecord.
     * @param[in] op Input parameter.
     * @param[in] target_key Input parameter.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> serializeJournalRecord(JournalOp op,
                                                       const std::string& target_key,
                                                       const std::vector<uint8_t>& payload);
    /**
     * @brief TBD: Describe deserializeJournalRecord.
     * @param[in] bytes Input parameter.
     * @param[in,out] op Input/output parameter.
     * @param[in,out] target_key Input/output parameter.
     * @param[in,out] payload Input/output parameter.
     * @return True on success.
     */
    static bool deserializeJournalRecord(const std::vector<uint8_t>& bytes,
                                         JournalOp& op,
                                         std::string& target_key,
                                         std::vector<uint8_t>& payload);

    /**
     * @brief TBD: Describe appendU32.
     * @param[in,out] out Input/output parameter.
     * @param[in] v Input parameter.
     */
    static void appendU32(std::vector<uint8_t>& out, uint32_t v);
    /**
     * @brief TBD: Describe readU32.
     * @param[in] in Input parameter.
     * @param[in,out] off Input/output parameter.
     * @param[in,out] v Input/output parameter.
     * @return True on success.
     */
    static bool readU32(const std::vector<uint8_t>& in, std::size_t& off, uint32_t& v);
    /**
     * @brief TBD: Describe appendString.
     * @param[in,out] out Input/output parameter.
     * @param[in] v Input parameter.
     */
    static void appendString(std::vector<uint8_t>& out, const std::string& v);
    /**
     * @brief TBD: Describe readString.
     * @param[in] in Input parameter.
     * @param[in,out] off Input/output parameter.
     * @param[in,out] v Input/output parameter.
     * @return True on success.
     */
    static bool readString(const std::vector<uint8_t>& in, std::size_t& off, std::string& v);
    /**
     * @brief TBD: Describe appendBytes.
     * @param[in,out] out Input/output parameter.
     * @param[in] v Input parameter.
     */
    static void appendBytes(std::vector<uint8_t>& out, const std::vector<uint8_t>& v);
    /**
     * @brief TBD: Describe readBytes.
     * @param[in] in Input parameter.
     * @param[in,out] off Input/output parameter.
     * @param[in,out] v Input/output parameter.
     * @return True on success.
     */
    static bool readBytes(const std::vector<uint8_t>& in, std::size_t& off, std::vector<uint8_t>& v);

    std::shared_ptr<TensorFingerprintGraph> graph_;
    std::shared_ptr<storage::ITensorStorageBackend> backend_;
    std::string tenant_id_;
    std::string domain_;
};

}  // namespace themis::tensor
