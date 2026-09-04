/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_gpu_example.cpp                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     285                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_gpu_example.cpp
 * @brief Example usage of Multi-GPU Resource Management
 * 
 * This example demonstrates how to use the multi-GPU distribution,
 * load balancing, and health monitoring features in ThemisDB.
 */

#include "llm/llama_resource_manager.h"
#include "llm/gpu_memory_manager.h"
#include "llm/adapter_load_balancer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace themis::llm;

int main() {
    std::cout << "=== ThemisDB Multi-GPU Resource Management Example ===" << std::endl;
    
    // ================================================================
    // Step 1: Initialize GPU Memory Manager
    // ================================================================
    std::cout << "\n[Step 1] Initializing GPU Memory Manager..." << std::endl;
    
    GPUMemoryManager::Config mem_config;
    mem_config.enable_multi_gpu = true;
    mem_config.gpu_devices = {0, 1, 2, 3};  // Use 4 GPUs
    mem_config.enable_peer_access = true;
    mem_config.max_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24 GB per GPU
    
    auto memory_manager = std::make_shared<GPUMemoryManager>(mem_config);
    std::cout << "  ✓ GPU Memory Manager initialized with " 
              << mem_config.gpu_devices.size() << " GPUs" << std::endl;
    
    // ================================================================
    // Step 2: Initialize Adapter Load Balancer
    // ================================================================
    std::cout << "\n[Step 2] Initializing Adapter Load Balancer..." << std::endl;
    
    AdapterLoadBalancer::Config lb_config;
    lb_config.enable_dynamic_balancing = true;
    lb_config.enable_jit_eviction = true;
    lb_config.max_adapters_per_gpu = 10;
    lb_config.rebalance_threshold = 0.8f;
    lb_config.eviction_threshold = 0.9f;
    
    auto load_balancer = std::make_shared<AdapterLoadBalancer>(
        memory_manager, lb_config);
    std::cout << "  ✓ Adapter Load Balancer initialized" << std::endl;
    
    // ================================================================
    // Step 3: Configure Multi-GPU Backend
    // ================================================================
    std::cout << "\n[Step 3] Configuring Multi-GPU Backend..." << std::endl;
    
    GPUBackendConfig gpu_config;
    gpu_config.primary_gpu_id = 0;
    gpu_config.secondary_gpus = {1, 2, 3};
    
    // Tensor Parallelism
    gpu_config.tensor_parallel_mode = GPUBackendConfig::TensorParallelismMode::HYBRID;
    gpu_config.tensor_split_ratio = 0.5f;
    
    // Dynamic Load Balancing
    gpu_config.enable_dynamic_load_balancing = true;
    gpu_config.load_balance_threshold = 0.8f;
    gpu_config.load_balance_interval_ms = 5000;
    
    // Health Monitoring
    gpu_config.enable_health_checks = true;
    gpu_config.health_check_interval_ms = 10000;
    gpu_config.max_gpu_temperature_celsius = 85.0f;
    gpu_config.auto_failover_on_error = true;
    
    // Persistent Pinning
    gpu_config.enable_persistent_pinning = true;
    gpu_config.pinned_adapter_ids = {"legal-qa-v1", "medical-v1"};
    
    std::cout << "  ✓ Multi-GPU backend configured" << std::endl;
    std::cout << "    - Tensor Parallelism: HYBRID mode" << std::endl;
    std::cout << "    - Load Balancing: Enabled" << std::endl;
    std::cout << "    - Health Checks: Enabled" << std::endl;
    std::cout << "    - Pinned Adapters: " << gpu_config.pinned_adapter_ids.size() << std::endl;
    
    // ================================================================
    // Step 4: Place LoRA Adapters Across GPUs
    // ================================================================
    std::cout << "\n[Step 4] Placing LoRA Adapters..." << std::endl;
    
    std::vector<std::string> adapter_ids = {
        "legal-qa-v1",        // High priority - will be pinned
        "medical-v1",         // High priority - will be pinned
        "code-gen-v1",        // Medium priority
        "chat-v1",            // Medium priority
        "translation-v1",     // Low priority
        "summarization-v1"    // Low priority
    };
    
    for (size_t i = 0; i < adapter_ids.size(); ++i) {
        const auto& adapter_id = adapter_ids[i];
        size_t vram_bytes = 256 * 1024 * 1024;  // 256 MB per adapter
        int priority = (i < 2) ? 10 : (i < 4) ? 5 : 2;  // High/Medium/Low
        bool should_pin = (i < 2);  // Pin first two adapters
        
        // Select optimal GPU
        int gpu_id = load_balancer->selectGPUForAdapter(adapter_id, vram_bytes, priority);
        
        if (gpu_id >= 0) {
            // Place adapter
            bool success = load_balancer->placeAdapter(
                adapter_id, gpu_id, vram_bytes, priority, should_pin);
            
            if (success) {
                std::cout << "  ✓ " << adapter_id << " placed on GPU " << gpu_id 
                          << " (priority: " << priority 
                          << ", pinned: " << (should_pin ? "yes" : "no") << ")" << std::endl;
            }
        } else {
            std::cerr << "  ✗ Failed to place adapter: " << adapter_id << std::endl;
        }
    }
    
    // ================================================================
    // Step 5: Display GPU Statistics
    // ================================================================
    std::cout << "\n[Step 5] GPU Statistics:" << std::endl;
    
    auto all_stats = memory_manager->getAllGPUStats();
    for (const auto& stats : all_stats) {
        std::cout << "\nGPU " << stats.device_id << ":" << std::endl;
        std::cout << "  VRAM: " 
                  << (stats.used_vram_bytes / (1024.0 * 1024.0)) << " / "
                  << (stats.total_vram_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
        std::cout << "  Utilization: " << stats.utilization_percent << "%" << std::endl;
        std::cout << "  Temperature: " << stats.temperature_celsius << "°C" << std::endl;
        std::cout << "  Health: " << (stats.is_healthy ? "Healthy ✓" : "Unhealthy ✗") << std::endl;
        std::cout << "  Loaded Adapters: " << stats.loaded_adapters.size() << std::endl;
        
        if (!stats.loaded_adapters.empty()) {
            std::cout << "    ";
            for (size_t i = 0; i < stats.loaded_adapters.size(); ++i) {
                if (i > 0) {
                  std::cout << ", ";
                }
                std::cout << stats.loaded_adapters[i];
            }
            std::cout << std::endl;
        }
    }
    
    // ================================================================
    // Step 6: Check Load Balancing Status
    // ================================================================
    std::cout << "\n[Step 6] Load Balancing Status:" << std::endl;
    
    auto lb_stats = load_balancer->getStats();
    std::cout << "  Total Adapters: " << lb_stats.num_adapters << std::endl;
    std::cout << "  Active GPUs: " << lb_stats.num_gpus << std::endl;
    std::cout << "  Average Load: " << (lb_stats.average_gpu_load * 100) << "%" << std::endl;
    std::cout << "  Max Load: " << (lb_stats.max_gpu_load * 100) << "%" << std::endl;
    std::cout << "  Min Load: " << (lb_stats.min_gpu_load * 100) << "%" << std::endl;
    std::cout << "  Total Migrations: " << lb_stats.num_migrations << std::endl;
    std::cout << "  Total Evictions: " << lb_stats.num_evictions << std::endl;
    
    // Check if rebalancing is needed
    if (memory_manager->needsLoadRebalancing(0.3f)) {
        std::cout << "\n  ⚠ Load imbalance detected (>30% difference)" << std::endl;
        std::cout << "  Triggering rebalancing..." << std::endl;
        
        bool rebalanced = load_balancer->rebalance();
        if (rebalanced) {
            std::cout << "  ✓ Load rebalancing completed" << std::endl;
        }
    } else {
        std::cout << "  ✓ Load is well balanced across GPUs" << std::endl;
    }
    
    // ================================================================
    // Step 7: Simulate GPU Health Check
    // ================================================================
    std::cout << "\n[Step 7] GPU Health Check:" << std::endl;
    
    auto all_health = memory_manager->getAllGPUHealth();
    for (const auto& health : all_health) {
        std::cout << "\nGPU " << health.device_id << ":" << std::endl;
        std::cout << "  Available: " << (health.is_available ? "Yes" : "No") << std::endl;
        std::cout << "  Healthy: " << (health.is_healthy ? "Yes ✓" : "No ✗") << std::endl;
        std::cout << "  Temperature: " << health.temperature_celsius << "°C" << std::endl;
        std::cout << "  Utilization: " << health.utilization_percent << "%" << std::endl;
        std::cout << "  Error Count: " << health.error_count << std::endl;
        
        if (!health.last_error.empty()) {
            std::cout << "  Last Error: " << health.last_error << std::endl;
        }
    }
    
    // ================================================================
    // Step 8: Demonstrate Adapter Migration
    // ================================================================
    std::cout << "\n[Step 8] Demonstrating Adapter Migration:" << std::endl;
    
    // Get an adapter on GPU 0
    auto gpu0_adapters = load_balancer->getGPUAdapters(0);
    if (!gpu0_adapters.empty()) {
        std::string adapter_to_migrate = gpu0_adapters[0];
        
        // Check if it's pinned
        if (!load_balancer->isAdapterPinned(adapter_to_migrate)) {
            std::cout << "  Migrating adapter: " << adapter_to_migrate << std::endl;
            std::cout << "  Source GPU: 0" << std::endl;
            std::cout << "  Target GPU: 1" << std::endl;
            
            bool success = load_balancer->migrateAdapter(adapter_to_migrate, 1);
            if (success) {
                std::cout << "  ✓ Migration successful" << std::endl;
                
                // Verify new location
                int new_gpu = load_balancer->getAdapterGPU(adapter_to_migrate);
                std::cout << "  Verified: adapter now on GPU " << new_gpu << std::endl;
            } else {
                std::cout << "  ✗ Migration failed" << std::endl;
            }
        } else {
            std::cout << "  Adapter " << adapter_to_migrate << " is pinned - skipping migration" << std::endl;
        }
    }
    
    // ================================================================
    // Step 9: List Healthy GPUs
    // ================================================================
    std::cout << "\n[Step 9] Healthy GPU List:" << std::endl;
    
    auto healthy_gpus = memory_manager->getHealthyGPUs();
    std::cout << "  Healthy GPUs: ";
    for (size_t i = 0; i < healthy_gpus.size(); ++i) {
        if (i > 0) {
          std::cout << ", ";
        }
        std::cout << healthy_gpus[i];
    }
    std::cout << std::endl;
    
    // Get least loaded GPU
    int least_loaded = memory_manager->getLeastLoadedGPU();
    if (least_loaded >= 0) {
        std::cout << "  Least Loaded GPU: " << least_loaded << std::endl;
    }
    
    // ================================================================
    // Summary
    // ================================================================
    std::cout << "\n=== Example Complete ===" << std::endl;
    std::cout << "\nKey Features Demonstrated:" << std::endl;
    std::cout << "  ✓ Multi-GPU initialization with 4 GPUs" << std::endl;
    std::cout << "  ✓ Dynamic adapter placement with load balancing" << std::endl;
    std::cout << "  ✓ Persistent pinning for critical adapters" << std::endl;
    std::cout << "  ✓ GPU health monitoring with temperature and utilization" << std::endl;
    std::cout << "  ✓ Adapter migration between GPUs" << std::endl;
    std::cout << "  ✓ Load rebalancing detection and execution" << std::endl;
    std::cout << "  ✓ Per-GPU statistics and monitoring" << std::endl;
    
    std::cout << "\nFor more information, see:" << std::endl;
    std::cout << "  docs/llm/MULTI_GPU_RESOURCE_MANAGEMENT.md" << std::endl;
    
    return 0;
}
