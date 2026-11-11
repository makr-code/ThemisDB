#pragma once

#include "training/training_sample_provider.h"

#include <cstddef>
#include <memory>
#include <string>

namespace themis::training {

/**
 * @brief Read-only provider for documentation records stored in a generated RocksDB docs.db.
 *
 * New databases are read from complete `chunk_` records. Older databases without
 * chunk records fall back to `doc_` preview records. The provider never creates,
 * mutates, or deletes the source database.
 */
class DocsDbTrainingSampleProvider final : public ITrainingSampleProvider {
public:
    /** @brief Resource bounds and location for one docs.db source. */
    struct Config {
        std::string database_path;       ///< Existing RocksDB directory; never created or modified.
        std::size_t max_record_bytes = 1024 * 1024; ///< Maximum serialized record accepted.
        std::size_t max_records_to_scan = 65536;    ///< Hard scan limit, including skipped records.
    };

    /** @brief Construct a provider using the given database path and resource bounds. */
    explicit DocsDbTrainingSampleProvider(Config config);
    ~DocsDbTrainingSampleProvider() override;

    DocsDbTrainingSampleProvider(const DocsDbTrainingSampleProvider&) = delete;
    DocsDbTrainingSampleProvider& operator=(const DocsDbTrainingSampleProvider&) = delete;
    DocsDbTrainingSampleProvider(DocsDbTrainingSampleProvider&&) noexcept;
    DocsDbTrainingSampleProvider& operator=(DocsDbTrainingSampleProvider&&) noexcept;

    /**
     * @brief Open the existing RocksDB read-only and select chunk or legacy document records.
     * @param max_samples Maximum accepted records; must be positive.
     * @param batch_size Maximum records returned by each nextBatch() call; must be positive.
     * @throws std::invalid_argument for a missing/non-directory path or zero bounds.
     * @throws std::runtime_error if the DB cannot be opened or has no supported key prefix.
     */
    void initialize(size_t max_samples, size_t batch_size) override;

    /**
     * @brief Check whether the bounded source has more usable records.
     * @return True when another batch may be read.
     */
    [[nodiscard]] bool hasNext() const noexcept override;

    /**
     * @brief Read one bounded batch, skipping short documentation fragments.
     * @return Normalized samples; empty only when the key prefix is exhausted.
     * @throws std::runtime_error for oversized, malformed, or unreadable records.
     */
    [[nodiscard]] std::vector<TrainingSampleRecord> nextBatch() override;

    /** @brief Return the stable provenance source identifier `docs.db`. */
    [[nodiscard]] std::string_view sourceName() const noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::training