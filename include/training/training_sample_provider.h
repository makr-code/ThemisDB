#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace themis::training {

/**
 * @brief One normalized, provenance-bearing sample passed from a provider to a trainer.
 */
struct TrainingSampleRecord {
    std::string sample_id;   ///< Stable opaque identifier within the source snapshot.
    std::string source_uri;  ///< Source location or record key; must not contain raw content.
    std::string instruction; ///< Training instruction; may be empty when input is sufficient.
    std::string input;       ///< Optional context or prompt prefix.
    std::string output;      ///< Required completion target; empty targets are rejected by consumers.
    double weight = 1.0;     ///< Finite positive contribution weight.
};

/**
 * @brief Bounded streaming source of normalized training samples.
 *
 * Providers own source-specific parsing and resource lifetime. Consumers receive
 * only validated value records and do not depend on RocksDB, AQL, or file formats.
 * Implementations are single-consumer objects and are not safe for concurrent calls.
 */
class ITrainingSampleProvider {
public:
    virtual ~ITrainingSampleProvider() = default;

    /**
     * @brief Initialize the source and enforce per-run sample and batch bounds.
    * @param max_samples Maximum accepted samples; must be positive.
    * @param batch_size Maximum records returned by each nextBatch() call; must be positive.
     * @throws std::invalid_argument for zero bounds or invalid source configuration.
     * @throws std::runtime_error when the source cannot be opened or validated.
     */
    virtual void initialize(size_t max_samples, size_t batch_size) = 0;

    /**
     * @brief Whether another bounded batch can be read.
     * @return True when nextBatch() can return records; false after exhaustion.
     */
    [[nodiscard]] virtual bool hasNext() const noexcept = 0;

    /**
     * @brief Read the next batch; returns an empty vector only when the source is exhausted.
     * @throws std::runtime_error for malformed records or source I/O errors.
     */
    [[nodiscard]] virtual std::vector<TrainingSampleRecord> nextBatch() = 0;

    /**
     * @brief Stable, non-content identifier for source provenance metadata.
     * @return A view that remains valid for the provider lifetime.
     */
    [[nodiscard]] virtual std::string_view sourceName() const noexcept = 0;
};

} // namespace themis::training