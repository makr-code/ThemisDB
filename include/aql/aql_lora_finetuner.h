/**
 * @file aql_lora_finetuner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class AQLDatasetBuilder {
public:
    AQLDatasetBuilder() = default;

    // -------------------------------------------------------------------------
    // Built-in sample sources
    // -------------------------------------------------------------------------

    /**
     * @brief Add Builtin Samples.
     * @return Return value.
     */
    AQLDatasetBuilder& addBuiltinSamples();

    /**
     * @brief Add Builtin Samples For Category.
     * @param[in] cat Input parameter.
     * @return Return value.
     */
    AQLDatasetBuilder& addBuiltinSamplesForCategory(AQLSampleCategory cat);

    // -------------------------------------------------------------------------
    // Custom sample management
    // -------------------------------------------------------------------------

    AQLDatasetBuilder& addCustomSample(
        const std::string& nl_input,
        const std::string& aql_output,
        AQLSampleCategory category = AQLSampleCategory::NL_TO_AQL
    );

    /**
     * @brief Load From Json.
     * @param[in] json_path Path to the json.
     * @return Return value.
     */
    AQLDatasetBuilder& loadFromJson(const std::string& json_path);

    /**
     * @brief Load From Json Object.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    AQLDatasetBuilder& loadFromJsonObject(const json& data);

    // -------------------------------------------------------------------------
    // Dataset construction
    // -------------------------------------------------------------------------

    TrainingData build(const std::string& dataset_name = "themisdb_aql") const;

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    /**
     * @brief Clear.
     * @return Return value.
     */
    AQLDatasetBuilder& clear();

    // -------------------------------------------------------------------------
    // Serialisation helpers
    // -------------------------------------------------------------------------

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;

private:
    std::vector<TrainingDataSample> samples_;

    /**
     * @brief Helpers that populate the built-in sample tables
     */
    void addRelationalSamples();
    /**
     * @brief Add Graph Samples.
     */
    void addGraphSamples();
    /**
     * @brief Add Vector Samples.
     */
    void addVectorSamples();
    /**
     * @brief Add Geo Samples.
     */
    void addGeoSamples();
    /**
     * @brief Add Timeseries Samples.
     */
    void addTimeseriesSamples();
    /**
     * @brief Add LLMExtension Samples.
     */
    void addLLMExtensionSamples();
    /**
     * @brief Add Lora Cmd Samples.
     */
    void addLoraCmdSamples();
    /**
     * @brief Add DDLSamples.
     */
    void addDDLSamples();
};

// ============================================================================
// AQL LoRA Fine-tuner
// ============================================================================

class AQLLoRAFinetuner {
public:
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    struct Config {
        // ---------------------------------------------------------------
        // AQL-optimised LoRA default hyperparameters (named constants)
        // ---------------------------------------------------------------

        static constexpr int kDefaultRank          = 8;
        static constexpr float kDefaultAlpha       = 16.0f;
        static constexpr float kDefaultDropout     = 0.05f;
        static constexpr float kDefaultLearningRate = 3e-4f;
        static constexpr int kDefaultBatchSize     = 4;
        static constexpr int kDefaultEpochs        = 3;
        static constexpr int kDefaultMaxSeqLength  = 512;
        static constexpr int kDefaultWarmupSteps   = 10;

        // ---------------------------------------------------------------
        // Fields
        // ---------------------------------------------------------------

        std::string adapter_id = "themisdb-aql-adapter";

        std::string base_model = "mistral-7b";

        LoRAHyperparameters hyperparameters;

        bool include_builtin_samples = true;

        std::string extra_dataset_path;

        std::string output_dir = "data/lora_adapters/aql";

        std::size_t min_training_samples = 10;

        std::function<void(int epoch, double loss)> epoch_callback;

        Config();

        static Config fromOptions(
            const std::unordered_map<std::string, std::string>& options
        );
    };

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    explicit AQLLoRAFinetuner(const Config& config = Config{});

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
     * @brief Train.
     * @return Return value.
     */
    TrainingResult train();

    void addCustomSample(
        const std::string& nl_input,
        const std::string& aql_output,
        AQLSampleCategory category = AQLSampleCategory::NL_TO_AQL
    );

    /**
     * @brief Load Extra Dataset.
     * @param[in] json_path Path to the json.
     */
    void loadExtraDataset(const std::string& json_path);

    // -------------------------------------------------------------------------
    // Adapter resolution (inference time)
    // -------------------------------------------------------------------------

    /**
     * @brief Get Adapter ID.
     * @return Return value.
     */
    std::string getAdapterID() const;

    /**
     * @brief Is Trained.
     * @return True when the operation succeeds.
     */
    bool isTrained() const;

    // -------------------------------------------------------------------------
    // Dataset introspection
    // -------------------------------------------------------------------------

    /**
     * @brief Build Dataset.
     * @return Return value.
     */
    TrainingData buildDataset() const;

    /**
     * @brief Export Dataset Json.
     * @return Return value.
     */
    json exportDatasetJson() const;

    // -------------------------------------------------------------------------
    // Registry integration (optional)
    // -------------------------------------------------------------------------

    /**
     * @brief Set Adapter Registry.
     * @param[in] registry Input parameter.
     */
    void setAdapterRegistry(std::shared_ptr<::themis::llm::AdapterRegistry> registry);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
