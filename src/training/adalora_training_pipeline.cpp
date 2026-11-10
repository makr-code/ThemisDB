/**
 * @file adalora_training_pipeline.cpp
 * @brief Native docs-to-TinyLlama AdaLoRA training and GGUF serving handoff.
 */
#include "training/adalora_training_pipeline.h"

#include "llm/adapter_registry.h"
#include "llm/llama_wrapper.h"
#include "llm/training_data_iterator.h"
#include "training/ada_lora_adapter.h"

#include <ggml.h>
#include <gguf.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace themis::training {
namespace {

using GGUFContext = std::unique_ptr<gguf_context, decltype(&gguf_free)>;
using GGMLContext = std::unique_ptr<ggml_context, decltype(&ggml_free)>;

struct PreparedSample {
    std::vector<int> prompt_tokens;
    std::vector<int> target_tokens;
    float weight = 1.0f;
};

void atomicallyReplaceFile(const std::filesystem::path& temporary,
                           const std::filesystem::path& destination) {
#ifdef _WIN32
    const BOOL replaced = std::filesystem::exists(destination)
        ? ReplaceFileW(destination.c_str(), temporary.c_str(), nullptr, REPLACEFILE_WRITE_THROUGH, nullptr, nullptr)
        : MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH);
    if (!replaced) {
        throw std::runtime_error("Failed to atomically publish checkpoint: " +
                                 std::to_string(GetLastError()));
    }
#else
    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
    if (error) {
        throw std::runtime_error("Failed to atomically publish checkpoint: " + error.message());
    }
#endif
}

std::string makePrompt(const llm::TrainingDataIterator::TrainingSample& sample) {
    std::string prompt = sample.instruction;
    if (!sample.input.empty()) {
        if (!prompt.empty()) {
            prompt.push_back('\n');
        }
        prompt += sample.input;
    }
    return prompt;
}

void writeAdapterGGUF(const AdaLoRAAdapter& adapter,
                      const AdaLoRATrainingConfig& config,
                      size_t hidden_size,
                      size_t vocabulary_size,
                      const std::string& model_fingerprint) {
    namespace fs = std::filesystem;
    const fs::path output_path(config.adapter_output_path);
    if (output_path.empty()) {
        throw std::invalid_argument("AdaLoRA adapter output path is empty");
    }
    if (fs::exists(output_path)) {
        throw std::runtime_error("Refusing to overwrite an existing AdaLoRA adapter: " + output_path.string());
    }
    if (!output_path.parent_path().empty()) {
        fs::create_directories(output_path.parent_path());
    }

    const auto stats = adapter.getLayerStats();
    if (stats.empty()) {
        throw std::runtime_error("Cannot export an AdaLoRA adapter without output shards");
    }
    size_t total_rank = 0;
    for (const auto& layer : stats) {
        if (layer.active_rank == 0 || total_rank > std::numeric_limits<size_t>::max() - layer.active_rank) {
            throw std::runtime_error("Invalid active AdaLoRA rank during GGUF export");
        }
        total_rank += layer.active_rank;
    }
    if (hidden_size > static_cast<size_t>(std::numeric_limits<int64_t>::max()) ||
        vocabulary_size > static_cast<size_t>(std::numeric_limits<int64_t>::max()) ||
        total_rank > static_cast<size_t>(std::numeric_limits<int64_t>::max()) ||
        hidden_size > std::numeric_limits<size_t>::max() / total_rank ||
        vocabulary_size > std::numeric_limits<size_t>::max() / total_rank) {
        throw std::runtime_error("AdaLoRA GGUF tensor dimensions overflow");
    }

    std::vector<float> lora_a(total_rank * hidden_size, 0.0f);
    std::vector<float> lora_b(vocabulary_size * total_rank, 0.0f);
    size_t rank_offset = 0;
    size_t vocabulary_offset = 0;
    for (const auto& layer_stats : stats) {
        const auto [b_internal, a_internal] = adapter.getWeights(layer_stats.layer_name);
        if (layer_stats.max_rank == 0 ||
            b_internal.size() != hidden_size * layer_stats.max_rank ||
            a_internal.size() % layer_stats.max_rank != 0) {
            throw std::runtime_error("AdaLoRA output-shard weight shape does not match model dimensions");
        }
        const size_t shard_vocabulary = a_internal.size() / layer_stats.max_rank;
        if (shard_vocabulary == 0 || shard_vocabulary > vocabulary_size ||
            vocabulary_offset > vocabulary_size - shard_vocabulary) {
            throw std::runtime_error("AdaLoRA output shard is outside the model vocabulary");
        }
        for (size_t component = 0; component < layer_stats.active_rank; ++component) {
            for (size_t hidden = 0; hidden < hidden_size; ++hidden) {
                lora_a[(rank_offset + component) * hidden_size + hidden] =
                    b_internal[hidden * layer_stats.max_rank + component];
            }
            for (size_t token = 0; token < shard_vocabulary; ++token) {
                lora_b[(vocabulary_offset + token) * total_rank + rank_offset + component] =
                    a_internal[component * shard_vocabulary + token];
            }
        }
        rank_offset += layer_stats.active_rank;
        vocabulary_offset += shard_vocabulary;
    }
    if (vocabulary_offset != vocabulary_size || rank_offset != total_rank) {
        throw std::runtime_error("AdaLoRA output shards do not cover the full model vocabulary");
    }

    const size_t metadata_bytes = 16 * ggml_tensor_overhead() + 4096;
    GGMLContext tensor_context(ggml_init({metadata_bytes, nullptr, true}), &ggml_free);
    GGUFContext gguf(gguf_init_empty(), &gguf_free);
    if (!tensor_context || !gguf) {
        throw std::runtime_error("Failed to initialize GGUF adapter writer contexts");
    }

    gguf_set_val_str(gguf.get(), "general.type", "adapter");
    gguf_set_val_str(gguf.get(), "general.architecture", config.model_architecture.c_str());
    gguf_set_val_str(gguf.get(), "general.name", config.adapter_id.c_str());
    gguf_set_val_str(gguf.get(), "adapter.type", "lora");
    const float export_alpha = config.alpha / static_cast<float>(config.initial_rank) *
                               static_cast<float>(total_rank);
    gguf_set_val_f32(gguf.get(), "adapter.lora.alpha", export_alpha);
    gguf_set_val_str(gguf.get(), "themis.base_model.sha256", model_fingerprint.c_str());

    auto* tensor_a = ggml_new_tensor_2d(tensor_context.get(), GGML_TYPE_F32,
        static_cast<int64_t>(hidden_size), static_cast<int64_t>(total_rank));
    auto* tensor_b = ggml_new_tensor_2d(tensor_context.get(), GGML_TYPE_F32,
        static_cast<int64_t>(total_rank), static_cast<int64_t>(vocabulary_size));
    if (!tensor_a || !tensor_b) {
        throw std::runtime_error("Failed to allocate GGUF LoRA tensor metadata");
    }
    ggml_set_name(tensor_a, "output.weight.lora_a");
    ggml_set_name(tensor_b, "output.weight.lora_b");
    gguf_add_tensor(gguf.get(), tensor_a);
    gguf_add_tensor(gguf.get(), tensor_b);
    gguf_set_tensor_data(gguf.get(), "output.weight.lora_a", lora_a.data());
    gguf_set_tensor_data(gguf.get(), "output.weight.lora_b", lora_b.data());

    fs::path temporary_path = output_path;
    temporary_path += ".tmp";
    std::error_code remove_error;
    fs::remove(temporary_path, remove_error);
    if (!gguf_write_to_file(gguf.get(), temporary_path.string().c_str(), false)) {
        fs::remove(temporary_path, remove_error);
        throw std::runtime_error("llama.cpp failed to write the AdaLoRA GGUF adapter");
    }
    std::error_code rename_error;
    fs::rename(temporary_path, output_path, rename_error);
    if (rename_error) {
        fs::remove(temporary_path, remove_error);
        throw std::runtime_error("Failed to publish AdaLoRA GGUF adapter: " + rename_error.message());
    }
}

} // namespace

AdaLoRATrainingResult AdaLoRATrainingPipeline::train(
    llm::TrainingDataIterator& data_iterator,
    llm::LlamaWrapper& model,
    const AdaLoRATrainingConfig& config) {
    AdaLoRATrainingResult result;
    result.adapter_path = config.adapter_output_path;
    try {
        if (config.adapter_id.empty() || config.base_model_path.empty() || config.aql_query.empty() ||
            config.adapter_output_path.empty() || config.checkpoint_path.empty() ||
            config.model_architecture != "llama" || config.max_samples == 0 ||
            config.max_sequence_length < 2 || config.output_shard_count == 0 ||
            config.initial_rank == 0 || config.rank_budget < config.output_shard_count ||
            config.epochs <= 0 || !std::isfinite(config.alpha) || config.alpha <= 0.0f ||
            !std::isfinite(config.learning_rate) || config.learning_rate <= 0.0f ||
            !std::isfinite(config.max_gradient_norm) || config.max_gradient_norm <= 0.0f ||
            config.reallocate_every_steps == 0) {
            throw std::invalid_argument("Invalid AdaLoRA training pipeline configuration");
        }
        if (config.output_shard_count > std::numeric_limits<size_t>::max() / config.initial_rank ||
            config.rank_budget > config.output_shard_count * config.initial_rank) {
            throw std::invalid_argument("AdaLoRA rank budget exceeds output-shard capacity");
        }
        if (!std::filesystem::is_regular_file(config.base_model_path)) {
            throw std::invalid_argument("Base model GGUF is not a regular file: " + config.base_model_path);
        }

        llm::AdapterMetadata metadata;
        metadata.adapter_id = config.adapter_id;
        metadata.base_model_name = std::filesystem::path(config.base_model_path).stem().string();
        metadata.architecture = config.model_architecture;
        metadata.task_type = "causal-language-modeling";
        metadata.domain = "documentation";
        metadata.training_config.lora_rank = static_cast<int>(config.initial_rank);
        metadata.training_config.lora_alpha = config.alpha;
        metadata.training_config.learning_rate = config.learning_rate;
        metadata.training_config.epochs = config.epochs;
        metadata.training_config.max_seq_length = static_cast<int>(config.max_sequence_length);
        metadata.training_config.target_modules = {"output"};
        metadata.provenance.dataset_name = config.adapter_id + "_documentation";
        metadata.provenance.training_query = config.aql_query;
        if (!data_iterator.initialize(config.aql_query, metadata)) {
            throw std::runtime_error("AQL training dataset initialization failed: " + data_iterator.lastError());
        }

        std::vector<llm::TrainingDataIterator::TrainingSample> samples;
        samples.reserve(std::min(config.max_samples, data_iterator.getTotalSamples()));
        while (data_iterator.hasNext() && samples.size() < config.max_samples) {
            auto batch = data_iterator.getNextBatch();
            if (!batch) {
                break;
            }
            for (auto& sample : batch->samples) {
                if (samples.size() == config.max_samples) {
                    break;
                }
                if (!sample.output.empty() && std::isfinite(sample.weight) && sample.weight > 0.0) {
                    samples.push_back(std::move(sample));
                }
            }
        }
        result.samples_read = samples.size();
        if (samples.empty()) {
            throw std::runtime_error("AQL training query returned no usable instruction/completion samples");
        }

        if (!model.isModelLoaded() && !model.loadModel(config.base_model_path)) {
            throw std::runtime_error("LlamaWrapper could not load base model " + config.base_model_path);
        }
        result.model_fingerprint = llm::LlamaWrapper::calculateModelChecksum(config.base_model_path);
        if (result.model_fingerprint.empty()) {
            throw std::runtime_error("Could not calculate the base model SHA-256 fingerprint");
        }

        std::vector<PreparedSample> prepared;
        prepared.reserve(samples.size());
        size_t hidden_size = 0;
        size_t vocabulary_size = 0;
        for (const auto& sample : samples) {
            const std::string prompt = makePrompt(sample);
            if (prompt.empty()) {
                continue;
            }
            auto prompt_tokens = model.tokenizeForBridge(prompt, true);
            auto target_tokens = model.tokenizeForBridge(sample.output, false);
            if (prompt_tokens.empty() || target_tokens.empty()) {
                continue;
            }
            const size_t max_target = config.max_sequence_length - 1;
            if (target_tokens.size() > max_target) {
                target_tokens.resize(max_target);
            }
            const size_t max_prompt = config.max_sequence_length - target_tokens.size();
            if (prompt_tokens.size() > max_prompt) {
                prompt_tokens.erase(prompt_tokens.begin(), prompt_tokens.end() - static_cast<std::ptrdiff_t>(max_prompt));
            }
            PreparedSample item;
            item.prompt_tokens = std::move(prompt_tokens);
            item.target_tokens = std::move(target_tokens);
            item.weight = static_cast<float>(sample.weight);
            std::vector<int> probe = item.prompt_tokens;
            probe.insert(probe.end(), item.target_tokens.begin(), item.target_tokens.end());
            const auto forward = model.forwardCausalBatchForTraining(probe);
            hidden_size = forward.hidden_size;
            vocabulary_size = forward.vocabulary_size;
            prepared.push_back(std::move(item));
        }
        result.samples_read = prepared.size();
        if (prepared.empty() || hidden_size == 0 || vocabulary_size == 0) {
            throw std::runtime_error("No usable model-tokenized documentation samples");
        }

        const size_t shard_count = std::min(config.output_shard_count, vocabulary_size);
        if (config.rank_budget < shard_count || vocabulary_size < shard_count) {
            throw std::invalid_argument("AdaLoRA rank budget must provide at least rank one per output shard");
        }
        AdaLoRAAdapter adapter(config.initial_rank, config.alpha, config.rank_budget);
        size_t vocabulary_offset = 0;
        for (size_t shard = 0; shard < shard_count; ++shard) {
            const size_t shard_size = vocabulary_size / shard_count + (shard < vocabulary_size % shard_count ? 1 : 0);
            adapter.addLayer("output_shard_" + std::to_string(shard), hidden_size,
                             shard_size, config.initial_rank, config.alpha);
            vocabulary_offset += shard_size;
        }
        if (vocabulary_offset != vocabulary_size) {
            throw std::runtime_error("AdaLoRA output-shard partition does not cover the model vocabulary");
        }

        if (std::filesystem::exists(config.checkpoint_path)) {
            if (!AdaLoRAAdapter::isCacheValid(config.checkpoint_path, result.model_fingerprint)) {
                throw std::runtime_error("Existing AdaLoRA checkpoint belongs to a different base model");
            }
            const auto checkpoint_fingerprint = adapter.loadFromFile(config.checkpoint_path);
            if (checkpoint_fingerprint != result.model_fingerprint) {
                throw std::runtime_error("AdaLoRA checkpoint fingerprint verification failed");
            }
            if (adapter.layerCount() != shard_count) {
                throw std::runtime_error("AdaLoRA checkpoint shard count does not match current model");
            }
        }
        const auto initial_allocation = adapter.reallocateRanks(config.rank_budget);
        if (!initial_allocation.success || initial_allocation.total_active_rank != config.rank_budget) {
            throw std::runtime_error("Could not apply the configured initial AdaLoRA rank budget");
        }

        float loss_sum = 0.0f;
        size_t reallocation_step = 0;
        for (int epoch = 0; epoch < config.epochs; ++epoch) {
            for (const auto& sample : prepared) {
                std::vector<int> tokens = sample.prompt_tokens;
                const size_t target_begin = tokens.size();
                tokens.insert(tokens.end(), sample.target_tokens.begin(), sample.target_tokens.end());
                if (tokens.size() <= target_begin) {
                    continue;
                }
                const auto forward = model.forwardCausalBatchForTraining(tokens);
                const size_t target_rows = tokens.size() - target_begin;
                std::vector<float> features(target_rows * hidden_size);
                std::vector<float> base_logits(target_rows * vocabulary_size);
                std::vector<size_t> targets(target_rows);
                for (size_t row = 0; row < target_rows; ++row) {
                    const size_t source_position = target_begin + row - 1;
                    std::copy_n(forward.hidden_states.data() + source_position * hidden_size,
                                hidden_size, features.data() + row * hidden_size);
                    std::copy_n(forward.logits.data() + source_position * vocabulary_size,
                                vocabulary_size, base_logits.data() + row * vocabulary_size);
                    targets[row] = static_cast<size_t>(tokens[target_begin + row]);
                }
                const auto step = adapter.trainCausalLmBatch(
                    features, base_logits, targets,
                    std::vector<float>(target_rows, sample.weight), target_rows,
                    config.learning_rate, config.max_gradient_norm);
                loss_sum += step.loss * static_cast<float>(target_rows);
                result.target_tokens_trained += target_rows;
                ++result.samples_trained;
                ++result.optimizer_steps;
                if (++reallocation_step % config.reallocate_every_steps == 0) {
                    const auto allocation = adapter.reallocateRanks(config.rank_budget);
                    if (!allocation.success || allocation.total_active_rank != config.rank_budget) {
                        throw std::runtime_error("AdaLoRA rank reallocation did not meet the configured budget");
                    }
                }
            }
            if (!config.checkpoint_path.empty()) {
                const std::filesystem::path checkpoint_path(config.checkpoint_path);
                const auto checkpoint_parent = checkpoint_path.parent_path();
                if (!checkpoint_parent.empty()) {
                    std::filesystem::create_directories(checkpoint_parent);
                }
                auto temporary_checkpoint = checkpoint_path;
                temporary_checkpoint += ".tmp";
                adapter.saveToFile(temporary_checkpoint.string(), result.model_fingerprint);
                atomicallyReplaceFile(temporary_checkpoint, checkpoint_path);
            }
        }
        if (result.target_tokens_trained == 0) {
            throw std::runtime_error("No next-token targets were processed");
        }
        result.mean_loss = loss_sum / static_cast<float>(result.target_tokens_trained);
        result.final_rank = adapter.reallocateRanks(config.rank_budget).total_active_rank;
        writeAdapterGGUF(adapter, config, hidden_size, vocabulary_size, result.model_fingerprint);
        if (config.load_for_inference && !model.loadLoRA(config.adapter_id, config.adapter_output_path, 1.0f)) {
            throw std::runtime_error("Training succeeded but the serving LoRA manager rejected the exported GGUF adapter");
        }
        result.success = true;
    } catch (const std::exception& exception) {
        result.error_message = exception.what();
    }
    return result;
}

} // namespace themis::training