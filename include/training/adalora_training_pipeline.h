/**
 * @file adalora_training_pipeline.h
 * @brief Native AdaLoRA fine-tuning from ThemisDB AQL samples into llama.cpp GGUF adapters.
 */
#pragma once

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
    size_t max_sequence_length = 512;
    size_t output_shard_count = 16;
    size_t initial_rank = 4;
    size_t rank_budget = 32;
    size_t reallocate_every_steps = 8;
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
     * @param model Dedicated LlamaWrapper for the configured frozen GGUF base model.
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
};

} // namespace themis::training