/**
 * @file distributed_trainer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/distributed_trainer.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

DistributedTrainer::DistributedTrainer(const DistributedConfig& config)
    : config_(config), initialized_(false) {
    
    stats_.world_size = config_.world_size;
    stats_.rank = config_.rank;
    
    spdlog::info("DistributedTrainer created:");
    spdlog::info("  Backend: {}", static_cast<int>(config_.backend));
    spdlog::info("  World size: {}", config_.world_size);
    spdlog::info("  Rank: {}", config_.rank);
}

DistributedTrainer::~DistributedTrainer() {
    if (initialized_) {
        finalize();
    }
}

bool DistributedTrainer::initialize() {
    if (initialized_) {
        spdlog::warn("DistributedTrainer already initialized");
        return true;
    }

    if (config_.world_size < 1) {
        spdlog::error("DistributedTrainer initialization failed: invalid world_size={} (must be >= 1)",
                      config_.world_size);
        return false;
    }

    if (config_.rank < 0 || config_.rank >= config_.world_size) {
        spdlog::error("Invalid rank: {} (world_size={})", config_.rank, config_.world_size);
        return false;
    }
    
    if (!is_distributed()) {
        spdlog::info("Single process mode (world_size=1), skipping collective operation validation");
        initialized_ = true;
        return true;
    }
    
    // FAIL-CLOSED VALIDATION (Batch 2 - Remove Pseudo-AllReduce)
    // For multi-rank distributed training, all collective operations (AllReduce, Broadcast, Barrier)
    // MUST be wired via real implementations (NCCL/MPI/Gloo).
    // No synthetic or placeholder fallbacks are permitted.
    spdlog::info("Initializing distributed training (fail-closed mode):");
    spdlog::info("  Backend: {}", static_cast<int>(config_.backend));
    spdlog::info("  Master: {}:{}", config_.master_addr, config_.master_port);
    spdlog::info("  Validating required collective operation callbacks...");
    
    if (!allreduce_cpu_fn_) {
        spdlog::error("FAIL-CLOSED: AllReduceFn bridge not installed. Distributed training requires "
                      "real collective operations. Call setAllReduceCpuFn() with NCCL/MPI/Gloo before initialize().");
        return false;
    }
    if (!broadcast_fn_) {
        spdlog::error("FAIL-CLOSED: BroadcastFn bridge not installed. Distributed training requires "
                      "real collective operations. Call setBroadcastFn() with NCCL/MPI/Gloo before initialize().");
        return false;
    }
    if (!barrier_fn_) {
        spdlog::error("FAIL-CLOSED: BarrierFn bridge not installed. Distributed training requires "
                      "real collective operations. Call setBarrierFn() with NCCL/MPI/Gloo before initialize().");
        return false;
    }
    
    initialized_ = true;
    spdlog::info("Distributed training initialized successfully (all collective operations verified)");
    return true;
}

void DistributedTrainer::finalize() {
    if (!initialized_) {
        return;
    }
    
    if (is_distributed()) {
        spdlog::info("Finalizing distributed training (rank {})", config_.rank);
        // Real implementation would call NCCL/Gloo/MPI cleanup here
    }
    
    initialized_ = false;
}

bool DistributedTrainer::is_distributed() const {
    return config_.world_size > 1;
}

bool DistributedTrainer::is_master() const {
    return config_.rank == 0;
}

bool DistributedTrainer::synchronize_gradients(std::vector<Tensor*>& gradients) {
    if (!is_distributed()) {
        return true;  // No synchronization needed
    }

    if (!allreduce_cpu_fn_) {
        spdlog::error("DistributedTrainer::synchronize_gradients failed: missing AllReduceCpuFn "
                      "for world_size={}", config_.world_size);
        return false;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // CPU-based implementation using simple averaging
    // Real GPU implementation would use NCCL AllReduce for efficiency
    for (auto* grad_ptr : gradients) {
        if (!grad_ptr) continue;
        
        auto& data = grad_ptr->data();
        
        // AllReduce: sum gradients across all processes, then divide by world_size
        allreduce_cpu(data);
        
        stats_.bytes_communicated += data.size() * sizeof(float);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    float elapsed_ms = std::chrono::duration<float, std::milli>(end - start).count();
    stats_.communication_time_ms += elapsed_ms;
    stats_.num_syncs++;
    
    return true;
}

bool DistributedTrainer::broadcast_parameters(std::vector<Tensor*>& parameters) {
    if (!is_distributed()) {
        return true;  // No broadcast needed
    }

    if (!broadcast_fn_) {
        spdlog::error("DistributedTrainer::broadcast_parameters failed: missing BroadcastFn "
                      "for world_size={}", config_.world_size);
        return false;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Broadcast from master (rank 0) to all processes
    for (auto* param_ptr : parameters) {
        if (!param_ptr) continue;
        
        auto& data = param_ptr->data();
        broadcast_cpu(data);
        
        stats_.bytes_communicated += data.size() * sizeof(float);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    float elapsed_ms = std::chrono::duration<float, std::milli>(end - start).count();
    stats_.communication_time_ms += elapsed_ms;
    
    return true;
}

void DistributedTrainer::barrier() {
    if (!is_distributed()) {
        return;
    }

    if (!barrier_fn_) {
        spdlog::error("DistributedTrainer::barrier() called without BarrierFn bridge. "
                      "Multi-rank training requires real collective operations (NCCL/MPI/Gloo). "
                      "Call setBarrierFn() before training begins.");
        throw std::runtime_error("BarrierFn not wired: cannot synchronize distributed training.");
    }

    (*barrier_fn_)();
}

void DistributedTrainer::setBarrierFn(BarrierFn fn) {
    barrier_fn_ = std::move(fn);
}

void DistributedTrainer::setBroadcastFn(BroadcastFn fn) {
    broadcast_fn_ = std::move(fn);
}

void DistributedTrainer::setAllReduceCpuFn(AllReduceCpuFn fn) {
    allreduce_cpu_fn_ = std::move(fn);
}

DistributedStats DistributedTrainer::stats() const {
    return stats_;
}

void DistributedTrainer::reset_stats() {
    stats_.communication_time_ms = 0.0f;
    stats_.computation_time_ms = 0.0f;
    stats_.bytes_communicated = 0;
    stats_.num_syncs = 0;
}

float DistributedTrainer::scale_learning_rate(
    float base_lr, int world_size, const std::string& strategy
) {
    if (world_size <= 1) {
        return base_lr;
    }
    
    if (strategy == "linear") {
        // Linear scaling: lr = base_lr * world_size
        return base_lr * static_cast<float>(world_size);
    } else if (strategy == "sqrt") {
        // Square root scaling: lr = base_lr * sqrt(world_size)
        return base_lr * std::sqrt(static_cast<float>(world_size));
    } else {
        spdlog::warn("Unknown LR scaling strategy: {}, using sqrt", strategy);
        return base_lr * std::sqrt(static_cast<float>(world_size));
    }
}

// allreduce_cpu: delegates to the injected AllReduceCpuFn when available
// (MPI_Allreduce / Gloo allreduce must be injected via setAllReduceCpuFn()
// before training starts when world_size > 1).  Falls back to local scale for
// single-process builds (world_size == 1) where no peer exchange is needed.
void DistributedTrainer::allreduce_cpu(std::vector<float>& data) {
    if (!allreduce_cpu_fn_) {
        if (config_.world_size > 1) {
            spdlog::error("DistributedTrainer::allreduce_cpu() called without AllReduceCpuFn bridge. "
                         "Multi-rank training (world_size={}) requires real collective operations (NCCL/MPI/Gloo). "
                         "Call setAllReduceCpuFn() before training begins.", config_.world_size);
            throw std::runtime_error("AllReduceCpuFn not wired: cannot synchronize distributed gradients.");
        }
        
        // Single-process fallback is safe: no peer communication needed
        const float scale = 1.0f / static_cast<float>(config_.world_size);
        for (float& val : data) {
            val *= scale;
        }
        return;
    }

    (*allreduce_cpu_fn_)(data);
}

// CPU-based Broadcast (simplified)
void DistributedTrainer::broadcast_cpu(std::vector<float>& data) {
    if (!broadcast_fn_) {
        if (config_.world_size > 1) {
            spdlog::error("DistributedTrainer::broadcast_cpu() called without BroadcastFn bridge. "
                         "Multi-rank training (world_size={}) requires real collective operations (NCCL/MPI/Gloo). "
                         "Call setBroadcastFn() before training begins.", config_.world_size);
            throw std::runtime_error("BroadcastFn not wired: cannot synchronize distributed parameters.");
        }
        return;
    }

    (*broadcast_fn_)(data);
}

// ============================================================================
// Backend Detection
// ============================================================================

std::vector<DistributedBackend> detect_available_backends() {
    std::vector<DistributedBackend> backends;
    
    // Always available: single process
    backends.push_back(DistributedBackend::NONE);
    
    // Check for NCCL
    if (is_nccl_available()) {
        backends.push_back(DistributedBackend::NCCL);
    }
    
    // Check for Gloo
    if (is_gloo_available()) {
        backends.push_back(DistributedBackend::GLOO);
    }
    
    // Check for MPI
    if (is_mpi_available()) {
        backends.push_back(DistributedBackend::MPI);
    }
    
    return backends;
}

bool is_nccl_available() {
    // Check if NCCL library is available
    // This would typically check for libnccl.so or nccl.h
    
    #ifdef WITH_NCCL
    return true;
    #else
    return false;
    #endif
}

bool is_gloo_available() {
    // Check if Gloo library is available
    
    #ifdef WITH_GLOO
    return true;
    #else
    return false;
    #endif
}

bool is_mpi_available() {
    // Check if MPI library is available
    
    #ifdef WITH_MPI
    return true;
    #else
    return false;
    #endif
}

} // namespace lora
} // namespace llm
} // namespace themis

