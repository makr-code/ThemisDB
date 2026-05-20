/*
 * ThemisDB | File: distributed_trainer.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:59
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 83/100 | Lines: 331
 * Open Issues: TODOs=2, Stubs=7, Gaps=14, Unimpl=0, Mock=1, Sim=4, Debt=0
 * Gap Correlation: internal=14 | external_v3=59 | delta=45 | status=divergent
 * External Severity (v3): C=0, H=44, M=15
 * PR: #570 [LoRA Phase 10] Add readiness status document for GPU optimization ... (2026-03-11T21:38:02Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    
    if (!is_distributed()) {
        spdlog::info("Single process mode (world_size=1), skipping initialization");
        initialized_ = true;
        return true;
    }
    
    spdlog::info("Initializing distributed training:");
    spdlog::info("  Backend: {}", static_cast<int>(config_.backend));
    spdlog::info("  Master: {}:{}", config_.master_addr, config_.master_port);
    
    // NOTE: This is a placeholder implementation for Phase 3
    // Real implementation would initialize NCCL/Gloo/MPI here
    // For now, we just validate the configuration
    
    if (config_.rank < 0 || config_.rank >= config_.world_size) {
        spdlog::error("Invalid rank: {} (world_size={})", config_.rank, config_.world_size);
        return false;
    }
    
    initialized_ = true;
    spdlog::info("Distributed training initialized successfully");
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

    if (barrier_fn_) {
        (*barrier_fn_)();
        return;
    }

    // STUB/SIMULATION NOTE:
    // Purpose: No-op barrier that lets distributed training loops compile and
    //          run on single-node / non-NCCL / non-MPI builds.
    // Activation: Always when is_distributed() is true and no barrier function
    //             has been injected via setBarrierFn() (default build without
    //             NCCL/MPI).
    // Production Delta: No actual synchronization occurs; ranks are not held
    //                   until all peers reach the same point.  In true multi-GPU
    //                   or multi-node training this leads to gradient staleness
    //                   and divergent model weights.
    // Removal Plan: Inject a real NCCL/MPI barrier via setBarrierFn() at startup.
    //               See src/llm/FUTURE_ENHANCEMENTS.md §Distributed Trainer Barrier.
    spdlog::debug("Barrier synchronization (rank {}) — no-op (inject via setBarrierFn)", config_.rank);
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

// CPU-based AllReduce — stub #290 resolved via AllReduceCpuFn injection.
void DistributedTrainer::allreduce_cpu(std::vector<float>& data) {
    // Delegate to the injected AllReduce implementation when available.
    // This enables MPI_Allreduce / Gloo allreduce to be wired at startup
    // without modifying this function (resolves stub #290).
    if (allreduce_cpu_fn_.has_value()) {
        (*allreduce_cpu_fn_)(data);
        return;
    }

    // Fallback: local scale-only path for single-process builds (world_size == 1).
    // This is mathematically correct only when world_size == 1; for multi-rank
    // deployments inject a real AllReduceCpuFn via setAllReduceCpuFn().
    if (config_.world_size > 1) {
        spdlog::warn("DistributedTrainer::allreduce_cpu: no AllReduceCpuFn injected "
                     "and world_size={} > 1; gradients will not be exchanged with peers. "
                     "Call setAllReduceCpuFn() to enable true multi-rank training.",
                     config_.world_size);
    }
    const float scale = 1.0f / static_cast<float>(config_.world_size);
    for (float& val : data) {
        val *= scale;
    }
}

// CPU-based Broadcast (simplified)
void DistributedTrainer::broadcast_cpu(std::vector<float>& data) {
    if (broadcast_fn_) {
        (*broadcast_fn_)(data);
        return;
    }

    // STUB/SIMULATION NOTE:
    // Purpose: Allow multi-rank training to proceed past the broadcast call in
    //          single-process CPU mode where actual inter-process communication
    //          is not needed (all "ranks" share the same address space).
    // Activation: Called whenever NCCL/RCCL/Gloo are absent and no BroadcastFn
    //             has been injected via setBroadcastFn() (default build without
    //             MPI/Gloo).
    // Production Delta: No data is sent to any rank.  In a true multi-process
    //                   setup (e.g. mpirun with world_size > 1) all non-master
    //                   ranks will continue with stale parameters; training
    //                   diverges immediately.  Single-process builds are unaffected.
    // Removal Plan: Inject a real MPI_Bcast/Gloo broadcast via setBroadcastFn() at
    //               startup.  See src/llm/FUTURE_ENHANCEMENTS.md §DistributedTrainer BroadcastCPU.
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

