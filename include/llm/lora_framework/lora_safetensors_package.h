/**
 * @file lora_safetensors_package.h
 * @brief Native validation of distributable LoRA/AdaLoRA safetensors packages.
 *
 * ThemisDB distributes adapters independently of the base model as a
 * safetensors package (`adapter_model.safetensors` + `adapter_config.json`).
 * safetensors is chosen because, unlike pickle checkpoints (.bin/.pt/.pkl/.ckpt),
 * loading it cannot execute code. The serving format (GGUF) is produced from
 * this package; see LoRAOrchestrator::importAdapter.
 *
 * File layout (safetensors spec):
 *   [u64 little-endian N][N bytes UTF-8 JSON header][tensor data region]
 * The header maps tensor names to {dtype, shape, data_offsets:[begin,end]}
 * (offsets relative to the data region) and may contain a string→string map
 * `__metadata__`, which carries the `themis.*` adapter metadata.
 *
 * Integrity convention: `themis.adapter.sha256_tensors` is the lowercase hex
 * SHA-256 of the complete tensor data region (all bytes after the header).
 * The header itself cannot be covered because it contains the digest.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

/// @brief One tensor entry of a validated safetensors header.
struct SafetensorsTensorInfo {
    std::string name;                 ///< Tensor name (e.g. "base_model.model...lora_A.weight").
    std::string dtype;                ///< One of F16, BF16, F32.
    std::vector<std::uint64_t> shape; ///< Tensor shape.
    std::uint64_t begin = 0;          ///< Start offset within the data region.
    std::uint64_t end = 0;            ///< End offset (exclusive) within the data region.
};

/// @brief Result of inspecting a safetensors file.
struct SafetensorsInspection {
    bool valid = false;                          ///< True iff every structural check passed.
    std::string error;                           ///< Non-empty iff !valid.
    std::uint64_t header_length = 0;             ///< N from the 8-byte prefix.
    std::uint64_t data_offset = 0;               ///< 8 + N.
    std::uint64_t data_length = 0;               ///< File size - data_offset.
    std::vector<SafetensorsTensorInfo> tensors;  ///< Sorted by begin offset.
    std::map<std::string, std::string> metadata; ///< `__metadata__` (empty if absent).
};

/// @brief Resolved paths of a distributable adapter package.
struct AdapterPackagePaths {
    bool valid = false;                    ///< True iff the package layout is acceptable.
    std::string error;                     ///< Non-empty iff !valid.
    std::filesystem::path package_dir;     ///< Directory containing the package files.
    std::filesystem::path safetensors;     ///< adapter_model.safetensors (or the given file).
    std::filesystem::path adapter_config;  ///< adapter_config.json (required).
};

/**
 * @brief Stateless validator for safetensors adapter packages.
 *
 * All functions are thread-safe and never throw for malformed input; errors
 * are reported through the returned structures.
 */
class SafetensorsPackageValidator {
public:
    /// Upper bound for the JSON header size (bytes).
    static constexpr std::uint64_t kMaxHeaderBytes = 100ULL * 1024ULL * 1024ULL;
    /// Upper bound for adapter_config.json (bytes).
    static constexpr std::uint64_t kMaxAdapterConfigBytes = 1024ULL * 1024ULL;

    /**
     * @brief Whether @p path has a pickle-based checkpoint extension
     *        (.bin, .pt, .pth, .pkl, .pickle, .ckpt); such files are rejected.
     */
    [[nodiscard]] static bool isForbiddenCheckpointExtension(const std::filesystem::path& path);

    /**
     * @brief Resolve and validate the package layout.
     * @param source Either a package directory or a `.safetensors` file inside it.
     * @return Paths of the safetensors file and adapter_config.json, or an error
     *         if a pickle checkpoint is given/present, files are missing, or
     *         adapter_config.json is not a plain-LoRA PEFT config.
     */
    [[nodiscard]] static AdapterPackagePaths resolvePackage(const std::filesystem::path& source);

    /**
     * @brief Structurally validate a safetensors file without loading tensors.
     *
     * Checks: 8-byte LE header length bounded by kMaxHeaderBytes and the file
     * size; JSON object header; dtype whitelist F16/BF16/F32; non-negative
     * integer shapes; data_offsets within the data region; byte size equal to
     * product(shape)·sizeof(dtype); offsets contiguous from 0, non-overlapping
     * and covering the entire data region; `__metadata__` is string→string.
     */
    [[nodiscard]] static SafetensorsInspection inspectFile(const std::filesystem::path& path);

    /**
     * @brief SHA-256 (lowercase hex) of the tensor data region of @p path.
     * @return Empty string on I/O failure.
     */
    [[nodiscard]] static std::string sha256OfTensorData(const std::filesystem::path& path,
                                                        const SafetensorsInspection& inspection);

    /// @brief SHA-256 (lowercase hex) of a whole file; empty string on I/O failure.
    [[nodiscard]] static std::string sha256OfFile(const std::filesystem::path& path);
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
