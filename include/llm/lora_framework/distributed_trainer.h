/**
 * @file distributed_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_layers.h"
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

enum class DistributedBackend {
    NONE,       // Single GPU/CPU
    NCCL,       // NVIDIA Collective Communications Library
    GLOO,       // Facebook Gloo (CPU/GPU)
    MPI         // Message Passing Interface
};

struct DistributedConfig {
    /**
     * @brief Distributed Config.
     * @return Return value.
     */
    virtual ~DistributedConfig() = default;
    DistributedBackend backend = DistributedBackend::NONE;
    int world_size = 1;                 // Total number of processes
    int rank = 0;                       // Current process rank
    std::string master_addr = "localhost";
    int master_port = 29500;
    bool gradient_as_bucket_view = true;
    int bucket_cap_mb = 25;             // Bucket size for gradient communication
    
    json toJSON() const {
        return json{
            {"backend", static_cast<int>(backend)},
            {"world_size", world_size},
            {"rank", rank},
            {"master_addr", master_addr},
            {"master_port", master_port},
            {"gradient_as_bucket_view", gradient_as_bucket_view},
            {"bucket_cap_mb", bucket_cap_mb}
        };
    }
    
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     * @details Calls: contains().
     */
    static DistributedConfig fromJSON(const json& j) {
        DistributedConfig config = {};
        if (j.contains("backend")) {
          config.backend = static_cast<DistributedBackend>(j["backend"].get<int>());
        }
        if (j.contains("world_size")) {
          config.world_size = j["world_size"];
        }
        if (j.contains("rank")) {
          config.rank = j["rank"];
        }
        if (j.contains("master_addr")) {
          config.master_addr = j["master_addr"];
        }
        if (j.contains("master_port")) {
          config.master_port = j["master_port"];
        }
        if (j.contains("gradient_as_bucket_view")) {
          config.gradient_as_bucket_view = j["gradient_as_bucket_view"];
        }
        if (j.contains("bucket_cap_mb")) {
          config.bucket_cap_mb = j["bucket_cap_mb"];
        }
        return config;
    }
};

struct DistributedStats {
    /**
     * @brief Distributed Stats.
     * @return Return value.
     */
    virtual ~DistributedStats() = default;
    int world_size = 1;
    int rank = 0;
    float communication_time_ms = 0.0f;
    float computation_time_ms = 0.0f;
    size_t bytes_communicated = 0;
    int num_syncs = 0;
    
    json toJSON() const {
        return json{
            {"world_size", world_size},
            {"rank", rank},
            {"communication_time_ms", communication_time_ms},
            {"computation_time_ms", computation_time_ms},
            {"bytes_communicated", bytes_communicated},
            {"num_syncs", num_syncs},
            {"efficiency", computation_time_ms / (computation_time_ms + communication_time_ms + 1e-6f)}
        };
    }
};

class DistributedTrainer {
public:
    using BarrierFn = std::function<void()>;

    using BroadcastFn = std::function<void(std::vector<float>&)>;

    using AllReduceCpuFn = std::function<void(std::vector<float>& data)>;
    /**
     * @brief Distributed Trainer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit DistributedTrainer(const DistributedConfig& config);
    ~DistributedTrainer();
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Finalize.
     */
    void finalize();
    
    /**
     * @brief Is distributed.
     * @return True when the operation succeeds.
     */
    bool is_distributed() const;
    
    /**
     * @brief Is master.
     * @return True when the operation succeeds.
     */
    bool is_master() const;
    
    /**
     * @brief Synchronize gradients.
     * @param[in,out] gradients Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool synchronize_gradients(std::vector<Tensor*>& gradients);
    
    /**
     * @brief Broadcast parameters.
     * @param[in,out] parameters Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool broadcast_parameters(std::vector<Tensor*>& parameters);
    
    /**
     * @brief Barrier.
     */
    void barrier();


    /**
     * @brief Set Barrier Fn.
     * @param[in] fn Input parameter.
     */
    void setBarrierFn(BarrierFn fn);

    /**
     * @brief Set Broadcast Fn.
     * @param[in] fn Input parameter.
     */
    void setBroadcastFn(BroadcastFn fn);

    /**
     * @brief Set All Reduce Cpu Fn.
     * @param[in] fn Input parameter.
     */
    void setAllReduceCpuFn(AllReduceCpuFn fn);
    
    DistributedConfig config() const { return config_; }
    
    /**
     * @brief Stats.
     * @return Return value.
     */
    DistributedStats stats() const;
    
    /**
     * @brief Reset stats.
     */
    void reset_stats();
    
    int world_size() const { return config_.world_size; }
    
    int rank() const { return config_.rank; }
    
    static float scale_learning_rate(float base_lr, int world_size, 
                                     const std::string& strategy = "sqrt");

private:
    DistributedConfig config_;
    bool initialized_ = false;
    
    // Statistics
    DistributedStats stats_;
    
    // Helper methods
    /**
     * @brief Allreduce cpu.
     * @param[in,out] data Input/output parameter.
     */
    void allreduce_cpu(std::vector<float>& data);
    /**
     * @brief Broadcast cpu.
     * @param[in,out] data Input/output parameter.
     */
    void broadcast_cpu(std::vector<float>& data);

    std::optional<BarrierFn>        barrier_fn_;
    std::optional<BroadcastFn>      broadcast_fn_;
    std::optional<AllReduceCpuFn>   allreduce_cpu_fn_;
};

class DistributedScope {
public:
    /**
     * @brief Distributed Scope.
     * @param[in,out] trainer Input/output parameter.
     * @return Return value.
     */
    explicit DistributedScope(DistributedTrainer* trainer)
        : trainer_(trainer) {
        if (trainer_) {
            trainer_->initialize();
        }
    }
    
    ~DistributedScope() {
        if (trainer_) {
            trainer_->finalize();
        }
    }
    
    DistributedTrainer* trainer() const { return trainer_; }

private:
    DistributedTrainer* trainer_;
};

/**
 * @brief Detect available backends.
 * @return Return value.
 */
std::vector<DistributedBackend> detect_available_backends();

/**
 * @brief Is nccl available.
 * @return True when the operation succeeds.
 */
bool is_nccl_available();

/**
 * @brief Is gloo available.
 * @return True when the operation succeeds.
 */
bool is_gloo_available();

/**
 * @brief Is mpi available.
 * @return True when the operation succeeds.
 */
bool is_mpi_available();

} // namespace lora
} // namespace llm
} // namespace themis

