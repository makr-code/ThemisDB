/**
 * @file adalora_training_pipeline.h
 * @brief Native AdaLoRA fine-tuning from ThemisDB AQL samples into llama.cpp GGUF adapters.
 */
#pragma once

#include "training/training_sample_provider.h"

#include <cstddef>
#include <string>

namespace themis::llm {
class LlamaWrapper;
class TrainingDataIterator;
}

namespace themis::training {

struct AdaLoRATrainingConfig {
    std::string adapter_id;
    std::string base_model_path;
    std::string aql_query;
    std::string adapter_output_path;
    std::string checkpoint_path;
    std::string model_architecture = "llama";
    size_t max_samples = 1024;
    size_t max_sample_bytes = 256 * 1024;
    size_t max_training_data_bytes = 64 * 1024 * 1024;
    size_t max_sequence_length = 512;
    size_t output_shard_count = 16;
    size_t initial_rank = 4;
    size_t rank_budget = 32;
    size_t reallocate_every_steps = 8;
    size_t provider_batch_size = 16; ///< Maximum records per provider batch; must be positive.
    int epochs = 1;
    float alpha = 8.0f;
    float learning_rate = 1e-4f;
    float max_gradient_norm = 1.0f;
    bool load_for_inference = true;
};

struct AdaLoRATrainingResult {
    bool success = false;
    std::string error_message;
    std::string adapter_path;
    size_t samples_read = 0;
    size_t samples_trained = 0;
    size_t target_tokens_trained = 0;
    size_t optimizer_steps = 0;
    size_t final_rank = 0;
    float mean_loss = 0.0f;
    std::string model_fingerprint;
};

/**
 * @brief Trains adaptive-rank LoRA factors from validated ThemisDB AQL samples.
 *
 * The frozen llama.cpp base model supplies final hidden states and base logits;
 * AdaLoRA applies exact next-token cross-entropy gradients to a block-sharded
 * `output.weight` adapter. The resulting standard GGUF adapter is loaded by the
 * existing MultiLoRAManager so callers can immediately serve it.
 */
class AdaLoRATrainingPipeline final {
public:
    /**
     * @brief Run one bounded training job.
     * @param data_iterator Open, configured iterator whose QueryEngine outlives the call.
     * @param model Dedicated LlamaWrapper; its loaded base-model SHA-256 must match
     *        the configured GGUF file, or this call fails before training/checkpoint reuse.
     * @param config Model, AQL, rank, optimizer, checkpoint, and output settings.
     * @return Counts, measured loss, fingerprint, and adapter path; failures are fail-closed.
     * @throws std::invalid_argument if required configuration or query is invalid.
     * @note Do not share @p model with concurrent inference during training; the wrapper
     *       serializes its context, and adapter loading changes that context's lifecycle.
     */
    [[nodiscard]] static AdaLoRATrainingResult train(
        llm::TrainingDataIterator& data_iterator,
        llm::LlamaWrapper& model,
        const AdaLoRATrainingConfig& config);

    /**
     * @brief Consume normalized batches from an injected source and train one adapter.
    * @param provider Source-specific provider; initialized by this call and not used concurrently.
     * @param model Dedicated LlamaWrapper; its loaded base-model SHA-256 must match
     *        the configured GGUF file, or this call fails before training/checkpoint reuse.
     * @param config Model, rank, optimizer, checkpoint, and output settings.
     * @return Training result; provider, model, checkpoint, and export failures are explicit.
    * @throws std::bad_alloc if bounded sample or model buffers cannot be allocated.
     * @note The caller remains responsible for authorizing the provider's source before use.
     */
    [[nodiscard]] static AdaLoRATrainingResult trainFromProvider(
        ITrainingSampleProvider& provider,
        llm::LlamaWrapper& model,
        const AdaLoRATrainingConfig& config);

    /**
     * @brief Train an AdaLoRA adapter directly from a generated documentation RocksDB.
     * @param docs_db_path Path to the docs.db RocksDB directory produced by the
     *        repository documentation importer.
     * @param model Dedicated LlamaWrapper; its loaded base-model SHA-256 must match
     *        the configured GGUF file, or this call fails before training/checkpoint reuse.
     * @param config Model, rank, optimizer, checkpoint, and output settings; aql_query
     *        is not used by this source path.
     * @return Counts, measured loss, fingerprint, and adapter path; malformed or empty
     *         databases and invalid model paths fail closed.
     * @throws std::invalid_argument if the path or training configuration is invalid.
     * @note The database is opened read-only. Complete `chunk_` records are preferred;
     *       older databases fall back to `doc_` records containing a text preview.
     *       The caller must ensure the documentation database is authorized for training.
     *       This method is synchronous; schedule it outside latency-sensitive request threads.
     */
    [[nodiscard]] static AdaLoRATrainingResult trainFromDocsDatabase(
        const std::string& docs_db_path,
        llm::LlamaWrapper& model,
        const AdaLoRATrainingConfig& config);
};

} // namespace themis::training