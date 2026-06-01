/**
 * @file lora_package.h
 * @brief LoRAPackage and PortableAdapterProduct — artifact lifecycle for LoRA adapters.
 *
 * Defines the two primary LoRA artifact classes used in the retrieval stack:
 *   - LoRAPackage: a versioned, self-describing adapter bundle with provenance.
 *   - PortableAdapterProduct: a deployment-ready, hardware-agnostic adapter artifact.
 *
 * Planned in: docs/EPIC1_LORA_ARTIFACTS.md
 * Sub-issue:   #5416 / #5424
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::retrieval {

/// Serialisation format for the adapter weights.
enum class AdapterFormat {
    SafeTensors, ///< HuggingFace safetensors
    GGUF,        ///< GGUF single-file format
    RawBin,      ///< Raw binary (legacy)
};

/// Provenance record attached to every LoRA artifact.
struct AdapterProvenance {
    std::string  dataset_snapshot_id;  ///< ID of the training DatasetSnapshot
    std::string  base_model_id;        ///< Hash or tag of the base LLM
    std::string  training_run_id;      ///< Unique ID of the training job
    std::string  hash_chain_id;        ///< Receipt from the hash chain layer
    std::chrono::system_clock::time_point created_at;
};

/**
 * @brief A versioned, self-describing LoRA adapter bundle.
 *
 * LoRAPackage is the canonical on-disk / in-flight representation of a
 * trained adapter: weights, metadata, and an immutable provenance record.
 */
struct LoRAPackage {
    std::string         id;            ///< Content-addressable unique ID
    std::string         name;          ///< Human-readable label
    std::string         version;       ///< Semantic version string
    AdapterFormat       format = AdapterFormat::SafeTensors;
    std::vector<std::uint8_t> weights; ///< Raw weight bytes (lazily loaded)
    AdapterProvenance   provenance;
    std::unordered_map<std::string, std::string> metadata;
    bool                is_quantized = false;
    std::string         quantization_scheme; ///< e.g. "int4", "fp8"
};

/**
 * @brief A deployment-ready, hardware-agnostic LoRA adapter product.
 *
 * PortableAdapterProduct wraps a LoRAPackage for a specific target device
 * class and adds compatibility metadata for the model-switch workflow.
 */
struct PortableAdapterProduct {
    std::string   id;                  ///< Derived from source LoRAPackage.id
    std::string   lora_package_id;     ///< Source package reference
    std::string   target_arch;         ///< e.g. "cuda", "cpu", "metal"
    std::string   compatibility_token; ///< Checked by the model-switch layer
    std::vector<std::uint8_t> payload; ///< Arch-specific compiled weights
    bool          ready = false;
};

/**
 * @brief Repository interface for LoRA artifact storage and retrieval.
 */
class ILoRARepository {
public:
    virtual ~ILoRARepository() = default;

    /// Store a LoRAPackage; returns the assigned ID.
    virtual std::string store(LoRAPackage pkg) = 0;

    /// Load a LoRAPackage by ID (weights loaded lazily if lazy=true).
    virtual std::optional<LoRAPackage> load(const std::string& id,
                                             bool lazy = true) = 0;

    /// Convert a LoRAPackage into a PortableAdapterProduct for the given arch.
    virtual PortableAdapterProduct compile(const std::string& package_id,
                                           const std::string& target_arch) = 0;

    /// List all available packages ordered by creation time (newest first).
    virtual std::vector<std::string> listPackageIds(std::size_t limit = 100) = 0;

    /// Delete a package and its compiled products.
    virtual bool purge(const std::string& id) = 0;
};

/// Factory: create a LoRARepository backed by the given storage path.
std::unique_ptr<ILoRARepository> makeLoRARepository(const std::string& storage_path);

} // namespace themis::retrieval
