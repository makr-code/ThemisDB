/**
 * @file adapter_load_balancer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/adapter_load_balancer.h"
#include "llm/gpu_memory_manager.h"
#include "llm/decision_record_yaml_processor.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>

namespace themis {
namespace llm {

AdapterLoadBalancer::AdapterLoadBalancer(
    std::shared_ptr<GPUMemoryManager> memory_manager,
    const Config& config)
    : memory_manager_(memory_manager)
    , config_(config) {
    
    spdlog::info("Adapter Load Balancer initialized:");
    spdlog::info("  Dynamic balancing: {}", config_.enable_dynamic_balancing ? "enabled" : "disabled");
    spdlog::info("  JIT eviction: {}", config_.enable_jit_eviction ? "enabled" : "disabled");
    spdlog::info("  Max adapters per GPU: {}", config_.max_adapters_per_gpu);
    spdlog::info("  Rebalance threshold: {:.1f}%", config_.rebalance_threshold * 100.0f);
}

AdapterLoadBalancer::~AdapterLoadBalancer() {
    spdlog::info("Adapter Load Balancer shutting down");
    spdlog::info("  Total migrations: {}", total_migrations_);
    spdlog::info("  Total evictions: {}", total_evictions_);
}

int AdapterLoadBalancer::selectGPUForAdapter(
    const std::string& adapter_id, size_t vram_bytes, int priority) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get list of healthy GPUs
    auto healthy_gpus = memory_manager_->getHealthyGPUs();
    
    if (healthy_gpus.empty()) {
        spdlog::error("No healthy GPUs available for adapter placement");
        return -1;
    }
    
    // Find GPU with most free VRAM that can accommodate the adapter
    int best_gpu = -1;
    size_t max_free_vram = 0;
    
    for (int gpu_id : healthy_gpus) {
        // Check if GPU has enough free VRAM
        size_t free_vram = memory_manager_->getFreeGPUVRAM(gpu_id);
        
        if (free_vram >= vram_bytes && free_vram > max_free_vram) {
            // Check adapter count limit
            auto& adapters = gpu_to_adapters_[gpu_id];
            if (adapters.size() < config_.max_adapters_per_gpu) {
                max_free_vram = free_vram;
                best_gpu = gpu_id;
            }
        }
    }
    
    if (best_gpu == -1) {
        spdlog::warn("No GPU found with sufficient free VRAM for adapter {}", adapter_id);
        
        // Try JIT eviction if enabled
        if (config_.enable_jit_eviction) {
            for (int gpu_id : healthy_gpus) {
                auto evicted = evictLRUAdapters(gpu_id, vram_bytes);
                if (!evicted.empty()) {
                    spdlog::info("Evicted {} adapters from GPU {} to make space", 
                                evicted.size(), gpu_id);
                    best_gpu = gpu_id;
                    break;
                }
            }
        }
    }
    
    if (best_gpu != -1) {
        spdlog::info("Selected GPU {} for adapter {} (priority: {})", 
                    best_gpu, adapter_id, priority);
    }
    
    return best_gpu;
}

bool AdapterLoadBalancer::placeAdapter(
    const std::string& adapter_id, int gpu_device_id,
    size_t vram_bytes, int priority, bool pinned) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if adapter already exists
    if (placements_.find(adapter_id) != placements_.end()) {
        spdlog::warn("Adapter {} already placed, removing old placement", adapter_id);
        
        // Remove inline without calling removeAdapter to avoid deadlock
        auto it = placements_.find(adapter_id);
        int old_gpu_id = it->second.gpu_device_id;
        placements_.erase(it);
        
        auto& adapters = gpu_to_adapters_[old_gpu_id];
        adapters.erase(std::remove(adapters.begin(), adapters.end(), adapter_id), adapters.end());
    }
    
    // Verify GPU can accommodate adapter
    if (!canPlaceOnGPU(gpu_device_id, vram_bytes)) {
        spdlog::error("Cannot place adapter {} on GPU {}: insufficient resources", 
                     adapter_id, gpu_device_id);
        return false;
    }
    
    // Create placement record
    AdapterPlacement placement;
    placement.adapter_id = adapter_id;
    placement.gpu_device_id = gpu_device_id;
    placement.vram_bytes = vram_bytes;
    placement.priority = priority;
    placement.is_pinned = pinned;
    placement.last_access_time_ms = getCurrentTimeMs();
    placement.access_count = 0;
    
    placements_[adapter_id] = placement;
    gpu_to_adapters_[gpu_device_id].push_back(adapter_id);
    
    spdlog::info("Adapter {} placed on GPU {} (pinned: {}, priority: {})", 
                adapter_id, gpu_device_id, pinned, priority);
    
    return true;
}

bool AdapterLoadBalancer::removeAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        return false;
    }
    
    int gpu_id = it->second.gpu_device_id;
    placements_.erase(it);
    
    // Remove from GPU's adapter list
    auto& adapters = gpu_to_adapters_[gpu_id];
    adapters.erase(std::remove(adapters.begin(), adapters.end(), adapter_id), adapters.end());
    
    spdlog::info("Adapter {} removed from GPU {}", adapter_id, gpu_id);
    
    return true;
}

int AdapterLoadBalancer::getAdapterGPU(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it != placements_.end()) {
        return it->second.gpu_device_id;
    }
    
    return -1;
}

std::vector<std::string> AdapterLoadBalancer::getGPUAdapters([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = gpu_to_adapters_.find(gpu_device_id);
    if (it != gpu_to_adapters_.end()) {
        return it->second;
    }
    
    return {};
}

AdapterLoadBalancer::AdapterPlacement AdapterLoadBalancer::getAdapterPlacement(
    const std::string& adapter_id) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it != placements_.end()) {
        return it->second;
    }
    
    return AdapterPlacement{};
}

bool AdapterLoadBalancer::isAdapterLoaded(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return placements_.find(adapter_id) != placements_.end();
}

bool AdapterLoadBalancer::pinAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        spdlog::error("Cannot pin adapter {}: not loaded", adapter_id);
        return false;
    }
    
    it->second.is_pinned = true;
    spdlog::info("Adapter {} pinned on GPU {}", adapter_id, it->second.gpu_device_id);
    
    return true;
}

bool AdapterLoadBalancer::unpinAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        return false;
    }
    
    it->second.is_pinned = false;
    spdlog::info("Adapter {} unpinned from GPU {}", adapter_id, it->second.gpu_device_id);
    
    return true;
}

bool AdapterLoadBalancer::isAdapterPinned(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it != placements_.end()) {
        return it->second.is_pinned;
    }
    
    return false;
}

bool AdapterLoadBalancer::rebalance() {
    if (!config_.enable_dynamic_balancing) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!shouldRebalance()) {
        return false;
    }
    
    spdlog::info("Starting adapter load rebalancing");
    
    auto healthy_gpus = memory_manager_->getHealthyGPUs();
    if (healthy_gpus.size() < 2) {
        return false;  // No rebalancing needed with single GPU
    }
    
    // Calculate average load
    float avg_load = 0.0f;
    for (int gpu_id : healthy_gpus) {
        avg_load += calculateGPULoad(gpu_id);
    }
    avg_load /= healthy_gpus.size();
    
    // Find overloaded and underloaded GPUs
    std::vector<int> overloaded_gpus;
    std::vector<int> underloaded_gpus;
    
    for (int gpu_id : healthy_gpus) {
        float load = calculateGPULoad(gpu_id);
        if (load > avg_load + config_.migration_threshold) {
            overloaded_gpus.push_back(gpu_id);
        } else if (load < avg_load - config_.migration_threshold) {
            underloaded_gpus.push_back(gpu_id);
        }
    }
    
    // Migrate adapters from overloaded to underloaded GPUs
    int migrations = 0;
    for (int overloaded_gpu : overloaded_gpus) {
        if (underloaded_gpus.empty()) {
          break;
        }
        
        auto& adapters = gpu_to_adapters_[overloaded_gpu];
        
        // Sort by priority (migrate lower priority first) and pinning status
        std::vector<std::string> candidates = {};

        for (const auto& adapter_id : adapters) {
            auto it = placements_.find(adapter_id);
            if (it != placements_.end() && !it->second.is_pinned) {
                candidates.push_back(adapter_id);
            }
        }
        
        // Sort by priority (lowest first)
        std::sort(candidates.begin(), candidates.end(), 
            [this](const std::string& a, const std::string& b) {
                return placements_[a].priority < placements_[b].priority;
            });
        
        // Migrate candidates
        for (const auto& adapter_id : candidates) {
            if (underloaded_gpus.empty()) {
              break;
            }
            
            int target_gpu = underloaded_gpus[0];
            
            // Perform migration inline (mutex already locked)
            auto it = placements_.find(adapter_id);
            if (it != placements_.end() && !it->second.is_pinned) {
                int source_gpu = it->second.gpu_device_id;
                
                bool success = performMigration(adapter_id, source_gpu, target_gpu);
                
                if (success) {
                    migrations++;
                    total_migrations_++;
                    
                    // Check if target GPU is still underloaded
                    float new_load = calculateGPULoad(target_gpu);
                    if (new_load >= avg_load) {
                        underloaded_gpus.erase(underloaded_gpus.begin());
                    }
                }
            }
        }
    }
    
    last_rebalance_time_ = getCurrentTimeMs();
    
    spdlog::info("Load rebalancing completed: {} migrations", migrations);

    emitRebalanceRecord(migrations, static_cast<int>(healthy_gpus.size()), avg_load);

    return migrations > 0;
}

bool AdapterLoadBalancer::migrateAdapter(
    const std::string& adapter_id, int target_gpu_id) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        spdlog::error("Cannot migrate adapter {}: not found", adapter_id);
        return false;
    }
    
    if (it->second.is_pinned && config_.respect_pinning) {
        spdlog::warn("Cannot migrate pinned adapter {}", adapter_id);
        return false;
    }
    
    int source_gpu = it->second.gpu_device_id;
    if (source_gpu == target_gpu_id) {
        return true;  // Already on target GPU
    }
    
    spdlog::info("Migrating adapter {} from GPU {} to GPU {}", 
                adapter_id, source_gpu, target_gpu_id);
    
    // Perform actual migration
    bool success = performMigration(adapter_id, source_gpu, target_gpu_id);
    
    if (success) {
        total_migrations_++;
    }
    
    return success;
}

std::vector<std::string> AdapterLoadBalancer::evictLRUAdapters(
    int gpu_device_id, size_t required_bytes) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> evicted;
    
    if (!config_.enable_jit_eviction) {
        return evicted;
    }
    
    auto candidates = selectAdaptersForEviction(gpu_device_id, required_bytes);
    
    for (const auto& adapter_id : candidates) {
        auto it = placements_.find(adapter_id);
        if (it != placements_.end() && !it->second.is_pinned) {
            // Perform eviction inline (mutex already locked)
            bool success = performEviction(adapter_id);
            
            if (success) {
                evicted.push_back(adapter_id);
                total_evictions_++;
            }
        }
    }
    
    return evicted;
}

void AdapterLoadBalancer::recordAccess(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = placements_.find(adapter_id);
    if (it != placements_.end()) {
        it->second.last_access_time_ms = getCurrentTimeMs();
        it->second.access_count++;
    }
}

AdapterLoadBalancer::LoadBalanceStats AdapterLoadBalancer::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LoadBalanceStats stats = {};
    stats.num_adapters = placements_.size();
    
    auto healthy_gpus = memory_manager_->getHealthyGPUs();
    stats.num_gpus = healthy_gpus.size();
    
    if (!healthy_gpus.empty()) {
        float total_load = 0.0f;
        float max_load = 0.0f;
        float min_load = 1.0f;
        
        for (int gpu_id : healthy_gpus) {
            float load = calculateGPULoad(gpu_id);
            total_load += load;
            max_load = std::max(max_load, load);
            min_load = std::min(min_load, load);
        }
        
        stats.average_gpu_load = total_load / healthy_gpus.size();
        stats.max_gpu_load = max_load;
        stats.min_gpu_load = min_load;
    }
    
    stats.num_migrations = total_migrations_;
    stats.num_evictions = total_evictions_;
    stats.last_balance_time_ms = last_rebalance_time_;
    
    return stats;
}

float AdapterLoadBalancer::getGPULoad([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculateGPULoad(gpu_device_id);
}

void AdapterLoadBalancer::markGPUUnhealthy([[maybe_unused]] int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    memory_manager_->markGPUUnhealthy(gpu_device_id, "Marked unhealthy by load balancer");
    
    spdlog::warn("GPU {} marked unhealthy, considering adapter migration", gpu_device_id);
}

void AdapterLoadBalancer::markGPUHealthy([[maybe_unused]] int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    memory_manager_->markGPUHealthy(gpu_device_id);
    
    spdlog::info("GPU {} marked healthy", gpu_device_id);
}

bool AdapterLoadBalancer::shouldMigrateFromGPU([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return !memory_manager_->isGPUHealthy(gpu_device_id);
}

// Private helper methods

bool AdapterLoadBalancer::canPlaceOnGPU(int gpu_device_id, size_t vram_bytes) const {
    size_t free_vram = memory_manager_->getFreeGPUVRAM(gpu_device_id);
    return free_vram >= vram_bytes;
}

std::vector<std::string> AdapterLoadBalancer::selectAdaptersForEviction(
    int gpu_device_id, size_t required_bytes) const {
    
    auto& adapters = gpu_to_adapters_.at(gpu_device_id);
    
    // Build list of candidates (non-pinned adapters)
    std::vector<std::pair<std::string, AdapterPlacement>> candidates;
    for (const auto& adapter_id : adapters) {
        auto it = placements_.find(adapter_id);
        if (it != placements_.end() && !it->second.is_pinned) {
            candidates.push_back({adapter_id, it->second});
        }
    }
    
    // Sort by LRU (least recently used first)
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
            return a.second.last_access_time_ms < b.second.last_access_time_ms;
        });
    
    // Select adapters until we have enough space
    std::vector<std::string> to_evict;
    size_t freed_bytes = 0;
    
    for (const auto& [adapter_id, placement] : candidates) {
        to_evict.push_back(adapter_id);
        freed_bytes += placement.vram_bytes;
        
        if (freed_bytes >= required_bytes) {
            break;
        }
    }
    
    return to_evict;
}

bool AdapterLoadBalancer::shouldRebalance() const {
    // Check if enough time has passed since last rebalance
    int64_t now = getCurrentTimeMs();
    if (now - last_rebalance_time_ < config_.rebalance_interval_ms) {
        return false;
    }
    
    // Check if load imbalance exceeds threshold
    return memory_manager_->needsLoadRebalancing(config_.migration_threshold);
}

float AdapterLoadBalancer::calculateGPULoad([[maybe_unused]] int gpu_device_id) const {
    auto stats = memory_manager_->getGPUStats(gpu_device_id);
    
    if (stats.total_vram_bytes == 0) {
        return 0.0f;
    }
    
    return static_cast<float>(stats.used_vram_bytes) / stats.total_vram_bytes;
}

int AdapterLoadBalancer::findLeastLoadedHealthyGPU() const {
    auto healthy_gpus = memory_manager_->getHealthyGPUs();
    
    if (healthy_gpus.empty()) {
        return -1;
    }
    
    int best_gpu = healthy_gpus[0];
    float min_load = calculateGPULoad(best_gpu);
    
    for (size_t i = 1; i < healthy_gpus.size(); ++i) {
        int gpu_id = healthy_gpus[i];
        float load = calculateGPULoad(gpu_id);
        if (load < min_load) {
            min_load = load;
            best_gpu = gpu_id;
        }
    }
    
    return best_gpu;
}

bool AdapterLoadBalancer::performMigration(
    const std::string& adapter_id, int source_gpu, int target_gpu) {
    
    // Update placement record
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        return false;
    }
    
    // Remove from source GPU list
    auto& source_adapters = gpu_to_adapters_[source_gpu];
    source_adapters.erase(
        std::remove(source_adapters.begin(), source_adapters.end(), adapter_id),
        source_adapters.end());
    
    // Add to target GPU list
    gpu_to_adapters_[target_gpu].push_back(adapter_id);
    
    // Update placement
    it->second.gpu_device_id = target_gpu;
    
    spdlog::info("Adapter {} migrated from GPU {} to GPU {}", 
                adapter_id, source_gpu, target_gpu);
    
    return true;
}

bool AdapterLoadBalancer::performEviction(const std::string& adapter_id) {
    auto it = placements_.find(adapter_id);
    if (it == placements_.end()) {
        return false;
    }
    
    int gpu_id = it->second.gpu_device_id;
    
    // Remove from tracking
    placements_.erase(it);
    
    auto& adapters = gpu_to_adapters_[gpu_id];
    adapters.erase(std::remove(adapters.begin(), adapters.end(), adapter_id), adapters.end());
    
    spdlog::info("Adapter {} evicted from GPU {}", adapter_id, gpu_id);
    
    return true;
}

int64_t AdapterLoadBalancer::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ============================================================================
// Hot-load in-progress tracking
// ============================================================================

void AdapterLoadBalancer::beginHotLoad(const std::string& adapter_id,
                                        const std::string& fallback_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    hot_loading_adapters_[adapter_id] = fallback_id;
    spdlog::info("AdapterLoadBalancer: hot-load started for '{}' (fallback='{}')",
                 adapter_id, fallback_id.empty() ? "<none>" : fallback_id);
}

void AdapterLoadBalancer::endHotLoad(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    hot_loading_adapters_.erase(adapter_id);
    spdlog::info("AdapterLoadBalancer: hot-load finished for '{}'", adapter_id);
}

bool AdapterLoadBalancer::isHotLoadInProgress(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hot_loading_adapters_.count(adapter_id) > 0;
}

std::string AdapterLoadBalancer::resolveAdapter(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hot_loading_adapters_.find(adapter_id);
    if (it != hot_loading_adapters_.end()) {
        // Hot-load in progress: route to fallback (may be empty = caller uses base model)
        spdlog::debug("AdapterLoadBalancer: '{}' loading, routing to fallback '{}'",
                      adapter_id, it->second.empty() ? "<base>" : it->second);
        return it->second;
    }
    return adapter_id;
}

void AdapterLoadBalancer::setDecisionRecordProcessor(
    std::shared_ptr<DecisionRecordYamlProcessor> processor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dr_processor_ = std::move(processor);
}

void AdapterLoadBalancer::emitRebalanceRecord(
    int migrations, int num_gpus, float avg_load) const
{
    // dr_processor_ is checked under the caller's mutex_ context
    if (!dr_processor_) {
        return;
    }

    DecisionRecord rec;
    rec.decision_type = "LORA_RANK_ADJUSTMENT";
    rec.component     = "AdapterLoadBalancer";
    rec.outcome       = migrations > 0 ? "SUCCESS" : "SKIPPED_BUDGET";

    rec.parameters["migrations"]        = std::to_string(migrations);
    rec.parameters["num_gpus"]          = std::to_string(num_gpus);
    rec.parameters["avg_gpu_load"]      = std::to_string(avg_load);
    rec.parameters["total_migrations"]  = std::to_string(total_migrations_);
    rec.parameters["total_evictions"]   = std::to_string(total_evictions_);

    // submit() is non-blocking — the processor's background thread handles I/O
    dr_processor_->submit(std::move(rec));
}

} // namespace llm
} // namespace themis
