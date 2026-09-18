#pragma once

/**
 * @file lora_checkpoint_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 97/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "lora_config.h"
#include "lora_storage_service.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

// ─────────────────────────────────────────────────────────────────────────────
// Data types
// ─────────────────────────────────────────────────────────────────────────────

struct CheckpointMeta {
    /**
     * @brief Checkpoint Meta.
     * @return Return value.
     */
    virtual ~CheckpointMeta() = default;
    std::string adapter_id;          ///< Unique adapter identifier
    uint64_t    step        = 0;     ///< Global training step at checkpoint
    uint32_t    epoch       = 0;     ///< Epoch at checkpoint
    float       train_loss  = 0.0f;  ///< Training loss
    float       val_loss    = std::numeric_limits<float>::infinity(); ///< Validation loss (lower=better)
    double      elapsed_s   = 0.0;   ///< Wall-clock training time in seconds
    std::string created_at;          ///< ISO-8601 UTC timestamp
    std::string weights_sha256;      ///< SHA-256 hex digest of the weight blob

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static CheckpointMeta fromJSON(const json& j);
};

struct CheckpointRef {
    std::string path;          ///< Absolute path to weight file
    CheckpointMeta meta;       ///< Associated metadata
    bool is_best = false;      ///< Whether this is the current best checkpoint
};

// ─────────────────────────────────────────────────────────────────────────────
// LoRACheckpointManager
// ─────────────────────────────────────────────────────────────────────────────

class LoRACheckpointManager {
public:
    struct Config {
        std::string root_dir     = "checkpoints";  ///< Base directory for checkpoint storage
        size_t      keep_last    = 5;              ///< How many recent checkpoints to retain (0 = keep all)
        bool        keep_best    = true;           ///< Always retain the checkpoint with lowest val_loss
        bool        verify_hash  = true;           ///< Verify SHA-256 on load
        bool        enable_compression = true;     ///< Compress weight blobs (zstd)
        std::chrono::seconds auto_save_interval{0}; ///< 0 = disabled; >0 = background auto-save period
    };

    LoRACheckpointManager();
    /**
     * @brief Lo RACheckpoint Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRACheckpointManager(Config config);
    ~LoRACheckpointManager();

    // Non-copyable
    LoRACheckpointManager(const LoRACheckpointManager&)            = delete;
    LoRACheckpointManager& operator=(const LoRACheckpointManager&) = delete;

    /**
     * @brief ── Save / Load ───────────────────────────────────────────────────────────
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] weights Input parameter.
     * @param[in] meta Input parameter.
     * @return Return value.
     */

    std::string save(const std::string&         adapter_id,
                     const std::vector<uint8_t>& weights,
                     CheckpointMeta              meta);

    /**
     * @brief Load Latest.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<CheckpointRef> loadLatest(const std::string& adapter_id) const;

    /**
     * @brief Load Best.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<CheckpointRef> loadBest(const std::string& adapter_id) const;

    /**
     * @brief Load By Step.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] step Input parameter.
     * @return Return value.
     */
    std::optional<CheckpointRef> loadByStep(const std::string& adapter_id,
                                             uint64_t           step) const;

    /**
     * @brief Read Weights.
     * @param[in] ref Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> readWeights(const CheckpointRef& ref) const;

    /**
     * @brief ── Listing / Management ─────────────────────────────────────────────────
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */

    std::vector<CheckpointRef> listCheckpoints(const std::string& adapter_id) const;

    /**
     * @brief Delete Checkpoint.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] step Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteCheckpoint(const std::string& adapter_id, uint64_t step);

    /**
     * @brief Delete All.
     * @param[in] adapter_id Identifier of the adapter.
     */
    void deleteAll(const std::string& adapter_id);

    /**
     * @brief Prune.
     * @param[in] adapter_id Identifier of the adapter.
     */
    void prune(const std::string& adapter_id);

    const Config& config() const noexcept { return config_; }

private:
    Config      config_;
    mutable std::mutex mutex_;

    /**
     * @brief Adapter Dir.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::string adapterDir(const std::string& adapter_id) const;
    /**
     * @brief Weight Path.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] step Input parameter.
     * @return Return value.
     */
    std::string weightPath(const std::string& adapter_id, uint64_t step) const;
    /**
     * @brief Meta Path.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] step Input parameter.
     * @return Return value.
     */
    std::string metaPath(const std::string& adapter_id, uint64_t step) const;

    /**
     * @brief Write Meta.
     * @param[in] path Input parameter.
     * @param[in] meta Input parameter.
     */
    void writeMeta(const std::string& path, const CheckpointMeta& meta) const;
    /**
     * @brief Read Meta.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    CheckpointMeta readMeta(const std::string& path) const;
    /**
     * @brief Update Best Record.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] meta Input parameter.
     */
    void updateBestRecord(const std::string& adapter_id,
                          const CheckpointMeta& meta) const;
    /**
     * @brief Read Best Meta.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<CheckpointMeta> readBestMeta(const std::string& adapter_id) const;
};

} // namespace lora
} // namespace llm
} // namespace themis

