/**
 * @file distributed_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Distributed training backend
 */
enum class DistributedBackend {
    NONE,       // Single GPU/CPU
    NCCL,       // NVIDIA Collective Communications Library
    GLOO,       // Facebook Gloo (CPU/GPU)
    MPI         // Message Passing Interface
};

/**
 * @brief Configuration for distributed training
 */
struct DistributedConfig {
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
    
    static DistributedConfig fromJSON(const json& j) {
        DistributedConfig config;
        if (j.contains("backend")) config.backend = static_cast<DistributedBackend>(j["backend"].get<int>());
        if (j.contains("world_size")) config.world_size = j["world_size"];
        if (j.contains("rank")) config.rank = j["rank"];
        if (j.contains("master_addr")) config.master_addr = j["master_addr"];
        if (j.contains("master_port")) config.master_port = j["master_port"];
        if (j.contains("gradient_as_bucket_view")) config.gradient_as_bucket_view = j["gradient_as_bucket_view"];
        if (j.contains("bucket_cap_mb")) config.bucket_cap_mb = j["bucket_cap_mb"];
        return config;
    }
};

/**
 * @brief Statistics for distributed training
 */
struct DistributedStats {
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

/**
 * @brief Distributed trainer for multi-GPU LoRA training
 * 
 * Features:
 * - Data parallelism (each GPU trains on different batch)
 * - Gradient synchronization via AllReduce
 * - Support for NCCL (NVIDIA) and Gloo (generic) backends
 * - Automatic load balancing
 * - Communication overlap with computation
 * 
 * Note: This is a simplified CPU-based implementation for the initial phase.
 * Full GPU support with NCCL/Gloo will be added in future PRs when GPU
 * acceleration is fully integrated.
 */
class DistributedTrainer {
public:
    /**
     * @brief Function type for barrier synchronization.
     *
     * Callers inject a real NCCL/MPI/Gloo barrier via setBarrierFn().
     * The injected function must block until all world_size ranks have
     * called barrier().
     */
    using BarrierFn = std::function<void()>;

    /**
     * @brief Function type for parameter broadcast.
     *
     * Callers MUST inject a real MPI_Bcast / Gloo broadcast via setBroadcastFn()
     * before calling broadcast_parameters() when world_size > 1. This enforces
     * fail-closed: training will not proceed silently without real collective
     * operations.
     *
     * The function receives the rank-0 data vector in-place; non-root ranks
     * are expected to overwrite their copy with rank-0's values.
     * 
     * @throws std::runtime_error if called in distributed mode without callback.
     */
    using BroadcastFn = std::function<void(std::vector<float>&)>;

    /**
     * @brief Function type for CPU gradient all-reduce.
     *
     * Callers MUST inject a real MPI_Allreduce / Gloo allreduce via
     * setAllReduceCpuFn() before calling synchronize_gradients() when
     * world_size > 1. This enforces fail-closed: training will not proceed
     * silently without real collective operations.
     *
     * The injected function receives the local gradient vector and must
     * perform an in-place SUM-then-divide-by-world_size across all ranks.
     *
     * @param data Gradient vector to reduce in-place.
     * @throws std::runtime_error if called in distributed mode without callback.
     */
    using AllReduceCpuFn = std::function<void(std::vector<float>& data)>;
    explicit DistributedTrainer(const DistributedConfig& config);
    ~DistributedTrainer();
    
    /**
     * @brief Initialize distributed training
        *
        * Fail-closed validation:
        * - world_size must be >= 1
        * - rank must satisfy 0 <= rank < world_size
        * - for world_size > 1, collective callbacks must be injected via
        *   setAllReduceCpuFn(), setBroadcastFn(), and setBarrierFn().
        *
        * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Finalize distributed training
     */
    void finalize();
    
    /**
     * @brief Check if distributed training is enabled
     * @return true if world_size > 1
     */
    bool is_distributed() const;
    
    /**
     * @brief Check if this is the master process (rank 0)
     * @return true if rank == 0
     */
    bool is_master() const;
    
    /**
     * @brief Synchronize gradients across all processes (AllReduce)
     * @param gradients Gradients to synchronize
        * @return true if successful
        * @return false when multi-rank synchronization is requested without an
        *         injected AllReduce callback
     */
    bool synchronize_gradients(std::vector<Tensor*>& gradients);
    
    /**
     * @brief Broadcast model parameters from master to all processes
     * @param parameters Model parameters to broadcast
        * @return true if successful
        * @return false when multi-rank broadcast is requested without an
        *         injected broadcast callback
     */
    bool broadcast_parameters(std::vector<Tensor*>& parameters);
    
    /**
     * @brief Barrier synchronization (wait for all processes).
     * 
     * @throws std::runtime_error if called in distributed mode (world_size > 1)
     *         without a barrier function injected via setBarrierFn().
     *         This enforces fail-closed: no silent synchronization skips.
     */
    void barrier();

    /**
     * @brief Function type for CPU AllReduce across training ranks.
     *
     * The callable receives the gradient vector in-place and must perform the
     * collective reduction (sum + divide by world_size) across all ranks.
     * A real implementation uses MPI_Allreduce, Gloo allreduce, or a shared-
     * memory ring-reduce.
     */
    using AllReduceCpuFn = std::function<void(std::vector<float>&)>;

    /**
     * @brief Inject a real barrier implementation (NCCL/MPI/Gloo).
     *
     * When set, barrier() delegates to this function instead of the
     * no-op fallback.  Call before the first training step.
     * @param fn Callable that performs the actual collective barrier.
     */
    void setBarrierFn(BarrierFn fn);

    /**
     * @brief Inject a real broadcast implementation (MPI/Gloo).
     *
     * When set, broadcast_cpu() delegates to this function so that
     * non-master ranks receive the master's parameter values.
     * @param fn Callable that broadcasts data in-place from rank 0.
     */
    void setBroadcastFn(BroadcastFn fn);

    /**
     * @brief Inject a real CPU all-reduce implementation (MPI/Gloo).
     *
     * When set, allreduce_cpu() delegates to this function so that gradients
     * are summed across all ranks and divided by world_size before the
     * optimizer step.  Must be called before the first training step when
     * world_size > 1.
     * @param fn Callable that performs the collective sum-reduce in-place.
     */
    void setAllReduceCpuFn(AllReduceCpuFn fn);
    
    /**
     * @brief Get distributed configuration
     * @return Configuration
     */
    DistributedConfig config() const { return config_; }
    
    /**
     * @brief Get distributed statistics
     * @return Statistics
     */
    DistributedStats stats() const;
    
    /**
     * @brief Reset statistics
     */
    void reset_stats();
    
    /**
     * @brief Get world size (number of processes)
     * @return World size
     */
    int world_size() const { return config_.world_size; }
    
    /**
     * @brief Get current process rank
     * @return Rank
     */
    int rank() const { return config_.rank; }
    
    /**
     * @brief Scale learning rate for distributed training
     * 
     * When using data parallelism, the effective batch size increases
     * by world_size, so learning rate should be scaled accordingly.
     * 
     * Common scaling strategies:
     * - Linear: lr_new = lr_base * world_size
     * - Square root: lr_new = lr_base * sqrt(world_size)
     * 
     * @param base_lr Base learning rate
     * @param strategy "linear" or "sqrt"
     * @return Scaled learning rate
     */
    static float scale_learning_rate(float base_lr, int world_size, 
                                     const std::string& strategy = "sqrt");

private:
    DistributedConfig config_;
    bool initialized_ = false;
    
    // Statistics
    DistributedStats stats_;
    
    // Helper methods
    void allreduce_cpu(std::vector<float>& data);
    void broadcast_cpu(std::vector<float>& data);

    std::optional<BarrierFn>        barrier_fn_;
    std::optional<BroadcastFn>      broadcast_fn_;
    std::optional<AllReduceCpuFn>   allreduce_cpu_fn_;
};

/**
 * @brief RAII wrapper for distributed training scope
 */
class DistributedScope {
public:
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
 * @brief Detect available distributed backends
 * 
 * @return Vector of available backends
 */
std::vector<DistributedBackend> detect_available_backends();

/**
 * @brief Check if NCCL is available
 * @return true if NCCL library is found
 */
bool is_nccl_available();

/**
 * @brief Check if Gloo is available
 * @return true if Gloo library is found
 */
bool is_gloo_available();

/**
 * @brief Check if MPI is available
 * @return true if MPI library is found
 */
bool is_mpi_available();

} // namespace lora
} // namespace llm
} // namespace themis

