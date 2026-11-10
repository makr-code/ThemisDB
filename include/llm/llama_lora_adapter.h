/**
 * @file llama_lora_adapter.h
 * @brief GGUF key/value metadata contract for standalone LoRA/AdaLoRA adapter files
 *        served through the llama.cpp (pinned tag b7974) runtime adapter API.
 *
 * Adapters are distributed independently of the base model and hot-loaded at runtime
 * as standalone GGUF LoRA files (never merged into base weights).  All training and
 * base-model identity metadata is embedded in the adapter GGUF KV store; this header
 * exposes the reader and the base-model compatibility check that MultiLoRAManager runs
 * before calling llama_adapter_lora_init().
 *
 * Implementation note: the functions are implemented in src/llm/multi_lora_manager.cpp
 * (the only consumer, always compiled together with the llama.cpp-enabled LLM sources).
 */

#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

struct llama_model;

namespace themis {
namespace llm {
namespace lora_gguf {

/**
 * @brief Adapter metadata read from the GGUF key/value store.
 *
 * Standard keys: general.architecture, general.type, adapter.type, adapter.lora.alpha.
 * ThemisDB keys (namespace `themis.`): adapter id/version/method/rank/rank_pattern,
 * base_model.{name,gguf,sha256} and training.* metrics.
 */
struct AdapterGgufMetadata {
    bool ok = false;                 ///< File parsed as GGUF
    std::string error;               ///< Parse error when ok == false
    std::string adapter_type;        ///< adapter.type (must be "lora")
    std::string architecture;        ///< general.architecture
    std::string base_model_name;     ///< themis.base_model.name (optional)
    std::string base_model_gguf;     ///< themis.base_model.gguf (optional, file name)
    std::string base_model_sha256;   ///< themis.base_model.sha256 (optional, hex)
    nlohmann::json kv = nlohmann::json::object(); ///< All scalar general.*, adapter.*, themis.* KVs
};

/**
 * @brief Read GGUF KV metadata of a LoRA adapter without loading tensor data.
 * @param path Adapter GGUF path.
 * @return Parsed metadata; ok == false with error set when the file is missing or
 *         not a valid GGUF file.  Array-typed values are recorded as
 *         {"type":"array","n":<count>} (string arrays are expanded).
 * @note Uses ggml's gguf_init_from_file(no_alloc=true); no tensor data is read.
 */
[[nodiscard]] AdapterGgufMetadata readAdapterGgufMetadata(const std::string& path);

/**
 * @brief Identity of the currently loaded base model used for compatibility checks.
 */
struct BaseModelIdentity {
    std::string gguf_file_name;  ///< File name (no directory) of the loaded base GGUF; empty = unknown
    std::string sha256;          ///< SHA-256 hex of the base GGUF if known; empty = unknown
    /// Load generation of the llama_model instance (e.g. CachedModel::loaded_at ticks).
    /// Used together with the model pointer to detect base-model reloads; 0 = unknown.
    std::uint64_t generation = 0;
};

/**
 * @brief Verify that adapter metadata is compatible with a loaded base model.
 * @param meta     Metadata from readAdapterGgufMetadata(); must have ok == true.
 * @param model    Loaded base model; must be non-null.
 * @param identity Known identity of the base model (fields may be empty).
 * @param[out] error Human-readable reason on rejection.
 * @return true when: adapter.type == "lora"; general.architecture is present and equals
 *         the model's general.architecture; and, when both sides are known,
 *         themis.base_model.gguf equals identity.gguf_file_name and
 *         themis.base_model.sha256 equals identity.sha256 (case-insensitive).
 * @note Tensor shape compatibility is additionally enforced by llama_adapter_lora_init.
 */
[[nodiscard]] bool checkBaseModelCompatibility(const AdapterGgufMetadata& meta,
                                               const llama_model* model,
                                               const BaseModelIdentity& identity,
                                               std::string& error);

} // namespace lora_gguf
} // namespace llm
} // namespace themis
