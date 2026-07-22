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
