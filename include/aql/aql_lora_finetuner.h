/**
 * @file aql_lora_finetuner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_config.h"
#include "llm/adapter_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>

namespace themis {
namespace aql {

using json = nlohmann::json;
using ::themis::llm::lora::TrainingData;
using ::themis::llm::lora::TrainingDataSample;
using ::themis::llm::lora::TrainingResult;
using ::themis::llm::lora::LoRAHyperparameters;

// ============================================================================
// AQL Training Sample Categories
// ============================================================================

/// Category of AQL training sample for domain-specific fine-tuning
enum class AQLSampleCategory {
    NL_TO_AQL,           ///< Natural language → AQL translation
    AQL_EXPLANATION,     ///< AQL query → natural language explanation
    AQL_COMPLETION,      ///< Partial AQL → completed query
    AQL_OPTIMISATION,    ///< Sub-optimal AQL → optimised AQL
    AQL_ERROR_FIX,       ///< AQL with error → corrected AQL
    AQL_SCHEMA_AWARE,    ///< Schema-context + NL → AQL
    AQL_LORA_CMD,        ///< LLM LORA command sequences
};

// ============================================================================
// AQL Dataset Builder
// ============================================================================

/**
 * @brief Builds a training dataset of AQL-specific prompt/completion pairs.
 *
 * Provides:
 * - A curated set of built-in ThemisDB AQL examples covering all major
 *   query patterns (relational, graph, vector, geo, timeseries, LLM).
 * - An API for adding custom domain-specific samples.
 * - Serialisation to/from JSON for offline dataset management.
 *
 * Usage:
 * @code
 * AQLDatasetBuilder builder;
 * builder.addBuiltinSamples();
 * builder.addCustomSample("Find all users older than 30",
 *                         "FOR u IN users FILTER u.age > 30 RETURN u",
 *                         AQLSampleCategory::NL_TO_AQL);
 * auto dataset = builder.build("themisdb_aql_v1");
 * @endcode
 */
class AQLDatasetBuilder {
public:
    AQLDatasetBuilder() = default;

    // -------------------------------------------------------------------------
    // Built-in sample sources
    // -------------------------------------------------------------------------

    /// Append the standard set of ThemisDB AQL training samples.
    /// Covers: relational, graph traversal, vector similarity, geo-spatial,
    /// timeseries, LLM extensions (INFER, RAG, EMBED, LORA), and DDL.
    AQLDatasetBuilder& addBuiltinSamples();

    /// Append only samples for a specific category.
    AQLDatasetBuilder& addBuiltinSamplesForCategory(AQLSampleCategory cat);

    // -------------------------------------------------------------------------
    // Custom sample management
    // -------------------------------------------------------------------------

    /// Append a single NL → AQL pair.
    AQLDatasetBuilder& addCustomSample(
        const std::string& nl_input,
        const std::string& aql_output,
        AQLSampleCategory category = AQLSampleCategory::NL_TO_AQL
    );

    /// Append a batch of samples loaded from a JSON file.
    /// Expected format: array of {"input": "...", "output": "..."} objects.
    AQLDatasetBuilder& loadFromJson(const std::string& json_path);

    /// Append samples from a JSON object (in-memory).
    AQLDatasetBuilder& loadFromJsonObject(const json& data);

    // -------------------------------------------------------------------------
    // Dataset construction
    // -------------------------------------------------------------------------

    /// Finalise and return the accumulated dataset.
    TrainingData build(const std::string& dataset_name = "themisdb_aql") const;

    /// Number of samples currently in the builder.
    std::size_t size() const;

    /// Remove all samples.
    AQLDatasetBuilder& clear();

    // -------------------------------------------------------------------------
    // Serialisation helpers
    // -------------------------------------------------------------------------

    /// Export all samples as a JSON array (for offline editing / audit).
    json toJson() const;

private:
    std::vector<TrainingDataSample> samples_;

    // Helpers that populate the built-in sample tables
    void addRelationalSamples();
    void addGraphSamples();
    void addVectorSamples();
    void addGeoSamples();
    void addTimeseriesSamples();
    void addLLMExtensionSamples();
    void addLoraCmdSamples();
    void addDDLSamples();
};

// ============================================================================
// AQL LoRA Fine-tuner
// ============================================================================

/**
 * @brief Manages the complete lifecycle of a ThemisDB-AQL–specific LoRA adapter.
 *
 * Responsibilities:
 * - Building an AQL training dataset (via AQLDatasetBuilder).
 * - Driving the LoRATrainingService with AQL-optimised hyperparameters.
 * - Registering the resulting adapter in the AdapterRegistry.
 * - Providing inference-time integration: resolving the adapter ID for a
 *   given base model so callers can pass it to LLMAQLHandler::executeInfer().
 *
 * Recommended hyperparameters (defaults):
 *  rank=8, alpha=16, dropout=0.05, lr=3e-4, epochs=3, batch_size=4,
 *  max_seq_length=512, target_modules=[q_proj, v_proj, k_proj, o_proj]
 *
 * @note The fine-tuner does NOT embed a real LLM backend – it delegates weight
 *       updates to LoRATrainingService.  Provide a properly configured
 *       LoRATrainingService when actual GPU training is desired; in test
 *       environments the service runs in simulation mode.
 *
 * @note **Compile guards:** The LoRA framework has no dedicated
 *       `THEMIS_ENABLE_LORA` flag.  CPU-only components
 *       (`lora_training_service`, `aql_lora_finetuner`, `llama_lora_adapter`,
 *       etc.) are always compiled as part of `THEMIS_CORE_SOURCES`.
 *       GPU-accelerated components (`gpu_lora_layers`, `vram_allocator`,
 *       `multi_gpu_trainer`, `multi_gpu_lora_layer`, etc.) are additionally
 *       compiled only when `THEMIS_ENABLE_GPU=ON` (default `ON`).
 *       Use `cmake … -DTHEMIS_ENABLE_GPU=OFF` to build a CPU-only ThemisDB
 *       image that still includes the full AQL LoRA fine-tuning API but
 *       delegates to the simulation mode of LoRATrainingService.
 *
 * Thread safety: all public methods are thread-safe.
 */
class AQLLoRAFinetuner {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    struct Config {
        // ---------------------------------------------------------------
        // AQL-optimised LoRA default hyperparameters (named constants)
        // ---------------------------------------------------------------

        /// LoRA rank: 8 is a well-validated starting point for 7B–13B models.
        static constexpr int kDefaultRank          = 8;
        /// LoRA alpha: typically set to 2 × rank for stable training.
        static constexpr float kDefaultAlpha       = 16.0f;
        /// Dropout: small value reduces over-fitting on the AQL dataset.
        static constexpr float kDefaultDropout     = 0.05f;
        /// Learning rate: 3e-4 works well with AdamW on AQL translation tasks.
        static constexpr float kDefaultLearningRate = 3e-4f;
        /// Batch size: 4 balances GPU memory and gradient quality.
        static constexpr int kDefaultBatchSize     = 4;
        /// Epochs: 3 is sufficient for the built-in AQL sample set.
        static constexpr int kDefaultEpochs        = 3;
        /// Max sequence length: 512 covers virtually all AQL query+NL pairs.
        static constexpr int kDefaultMaxSeqLength  = 512;
        /// Warmup steps: 10 steps give a smooth LR ramp-up.
        static constexpr int kDefaultWarmupSteps   = 10;

        // ---------------------------------------------------------------
        // Fields
        // ---------------------------------------------------------------

        /// Unique identifier for the AQL adapter family (versioned at train time)
        std::string adapter_id = "themisdb-aql-adapter";

        /// Base LLM to fine-tune (e.g., "mistral-7b", "llama-3-8b")
        std::string base_model = "mistral-7b";

        /// LoRA training hyperparameters (AQL-optimised defaults)
        LoRAHyperparameters hyperparameters;

        /// Include built-in ThemisDB AQL samples automatically
        bool include_builtin_samples = true;

        /// Optional path to an extra JSON dataset to merge with built-in samples
        std::string extra_dataset_path;

        /// Adapter output directory (passed to LoRATrainingService)
        std::string output_dir = "data/lora_adapters/aql";

        /// Minimum number of training samples required before training proceeds.
        /// Prevents trivially small datasets from producing degraded adapters.
        std::size_t min_training_samples = 10;

        /// Callback invoked after each completed training epoch with the epoch
        /// index (0-based) and the average loss for that epoch.
        std::function<void(int epoch, double loss)> epoch_callback;

        Config();

        /**
         * @brief Construct a Config from an AQL @c WITH options map.
         *
         * Recognised keys (all optional; unrecognised keys are silently ignored):
         *  - @c rank          (int, 1–256)
         *  - @c alpha         (float, > 0)
         *  - @c dropout       (float, [0, 1))
         *  - @c learning_rate (float, > 0)
         *  - @c batch_size    (int, > 0)
         *  - @c epochs        (int, > 0)
         *  - @c max_seq_length (int, > 0)
         *
         * @param options  Key/value map from the AQL WITH clause.
         * @return Config with overridden values; fields absent from @p options
         *         keep their default values.
         * @throws std::invalid_argument if any supplied value is out of range.
         */
        static Config fromOptions(
            const std::unordered_map<std::string, std::string>& options
        );
    };

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Construct fine-tuner with default configuration.
     * Creates an internal LoRATrainingService instance.
     */
    explicit AQLLoRAFinetuner(const Config& config = Config{});

    /**
     * @brief Construct fine-tuner with an externally supplied training service.
     * Useful for dependency injection in tests or custom deployments.
     */
    AQLLoRAFinetuner(
        const Config& config,
        std::shared_ptr<::themis::llm::lora::LoRATrainingService> training_service
    );

    ~AQLLoRAFinetuner();

    // Disable copy; allow move
    AQLLoRAFinetuner(const AQLLoRAFinetuner&) = delete;
    AQLLoRAFinetuner& operator=(const AQLLoRAFinetuner&) = delete;
    AQLLoRAFinetuner(AQLLoRAFinetuner&&) noexcept;
    AQLLoRAFinetuner& operator=(AQLLoRAFinetuner&&) noexcept;

    // -------------------------------------------------------------------------
    // Training
    // -------------------------------------------------------------------------

    /**
     * @brief Build the dataset and train the AQL LoRA adapter.
     *
     * Steps:
     *  1. Assemble training data (built-in + optional extra).
     *  2. Validate dataset size against config.min_training_samples.
     *  3. Delegate to LoRATrainingService::train().
     *  4. On success, register adapter metadata in the optional registry.
     *
     * @return TrainingResult from the underlying service.
     * @throws std::runtime_error if the dataset is too small or training fails.
     */
    TrainingResult train();

    /**
     * @brief Add additional NL→AQL samples before calling train().
     * Allows callers to extend the built-in dataset with project-specific pairs.
     */
    void addCustomSample(
        const std::string& nl_input,
        const std::string& aql_output,
        AQLSampleCategory category = AQLSampleCategory::NL_TO_AQL
    );

    /**
     * @brief Load extra samples from a JSON file before calling train().
     * @throws std::runtime_error if the file cannot be parsed.
     */
    void loadExtraDataset(const std::string& json_path);

    // -------------------------------------------------------------------------
    // Adapter resolution (inference time)
    // -------------------------------------------------------------------------

    /**
     * @brief Return the adapter ID to use with LLMAQLHandler::executeInfer().
     *
     * After a successful train() call this returns the versioned adapter ID.
     * Before training it returns the base adapter_id from the configuration.
     */
    std::string getAdapterID() const;

    /**
     * @brief True if train() has been called successfully at least once.
     */
    bool isTrained() const;

    // -------------------------------------------------------------------------
    // Dataset introspection
    // -------------------------------------------------------------------------

    /**
     * @brief Return the dataset that would be used by the next train() call.
     * Useful for auditing or exporting the training data.
     */
    TrainingData buildDataset() const;

    /**
     * @brief Export current dataset samples as JSON (for review/editing).
     */
    json exportDatasetJson() const;

    // -------------------------------------------------------------------------
    // Registry integration (optional)
    // -------------------------------------------------------------------------

    /**
     * @brief Attach an adapter registry.
     * When set, successful train() calls register the resulting adapter metadata.
     */
    void setAdapterRegistry(std::shared_ptr<::themis::llm::AdapterRegistry> registry);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
