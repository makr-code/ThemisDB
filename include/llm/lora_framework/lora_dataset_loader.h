/**
 * @file lora_dataset_loader.h
 * @brief Loads LoRA training samples from a ThemisDB collection or inline JSON.
 *
 * A dataset_id names a database collection whose documents carry
 * `input` (prompt) and `output` (target completion) strings and an optional
 * `context` (string or array of strings, e.g. retrieved passages for
 * RAFT-style training). The loader does not depend on the query module: the
 * caller injects an AQL executor (the server binds it to `executeAql`).
 *
 * Security: dataset_id is interpolated into AQL as a collection identifier,
 * which cannot be bound as a parameter; it is therefore validated against
 * `^[A-Za-z_][A-Za-z0-9_]{0,127}$` before any query is built.
 */
#pragma once

#include "llm/lora_framework/lora_training_service.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace llm {
namespace lora {

/// @brief Outcome of executing one AQL statement through the injected executor.
struct AqlRowsResult {
    bool ok = false;        ///< False if the query failed.
    std::string error;      ///< Non-empty iff !ok.
    nlohmann::json rows;    ///< Array of documents, or an object with a "results" array.
};

/// @brief Outcome of a dataset load.
struct DatasetLoadResult {
    bool success = false;          ///< True iff at least min_samples valid samples were loaded.
    std::string error;             ///< Non-empty iff !success.
    TrainingData data;             ///< Loaded samples (metadata.context holds context strings).
    std::size_t skipped_rows = 0;  ///< Rows without string input/output.
};

/**
 * @brief Converts DB documents / inline JSON samples into TrainingData.
 * @note Thread-safe if the injected executor is thread-safe.
 */
class LoRADatasetLoader {
public:
    /// Executes a complete AQL statement and returns its rows.
    using AqlExecutor = std::function<AqlRowsResult(const std::string& aql)>;

    /// Minimum number of samples required (train + holdout split).
    static constexpr std::size_t kMinSamples = 2;
    /// Hard upper bound for samples per dataset.
    static constexpr std::size_t kMaxSamples = 100000;

    /// @param executor AQL executor; may be empty, in which case load() fails.
    explicit LoRADatasetLoader(AqlExecutor executor);

    /// @brief True iff @p name matches ^[A-Za-z_][A-Za-z0-9_]{0,127}$.
    [[nodiscard]] static bool isValidCollectionName(std::string_view name);

    /**
     * @brief Build the dataset AQL for a validated collection.
     * @param collection Collection identifier (validated).
     * @param limit Row limit, clamped to [1, kMaxSamples].
     * @return `FOR d IN <collection> LIMIT <limit> RETURN d`, or std::nullopt if
     *         the identifier is invalid. Rows without input/output are filtered
     *         client-side because the conjunctive AQL path cannot express
     *         `!= null` predicates without an index.
     */
    [[nodiscard]] static std::optional<std::string> buildDatasetQuery(std::string_view collection,
                                                                      std::size_t limit);

    /**
     * @brief Convert rows (array or {"results": [...]}) into TrainingData.
     * @param rows Documents with string `input`/`output` and optional `context`.
     * @param dataset_name Stored as TrainingData::dataset_name.
     * @param limit Maximum number of samples taken (clamped to kMaxSamples).
     */
    [[nodiscard]] static DatasetLoadResult fromRows(const nlohmann::json& rows,
                                                    const std::string& dataset_name,
                                                    std::size_t limit);

    /**
     * @brief Load a dataset from the collection named @p dataset_id.
     * @return success=false with an error for invalid ids, executor failure,
     *         unavailable executor, or fewer than kMinSamples valid samples.
     */
    [[nodiscard]] DatasetLoadResult load(const std::string& dataset_id, std::size_t limit) const;

    /// @brief Whether an executor is configured.
    [[nodiscard]] bool hasExecutor() const noexcept { return static_cast<bool>(executor_); }

private:
    AqlExecutor executor_;
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
