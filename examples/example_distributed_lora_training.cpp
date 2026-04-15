/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_distributed_lora_training.cpp              ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     263                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_distributed_lora_training.cpp
 * @brief Example demonstrating distributed LoRA training with ShardRouter/ShardTopology
 * 
 * This example shows how to:
 * 1. Set up shard infrastructure (ShardRouter and ShardTopology)
 * 2. Register them with TrainingServiceRegistry
 * 3. Configure LoRATrainingService for distributed training
 * 4. Execute distributed training with real inter-shard communication
 * 
 * Build: cmake -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON ..
 * Run: ./examples/example_distributed_lora_training
 */

#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/training_service_registry.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>

using namespace themis::llm::lora;
using namespace themis::sharding;

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::info);
    
    std::cout << "====================================================\n";
    std::cout << "Distributed LoRA Training Example\n";
    std::cout << "====================================================\n\n";
    
    try {
        // ============================================================================
        // Step 1: Create Shard Infrastructure (ShardTopology and ShardRouter)
        // ============================================================================
        
        std::cout << "[1/5] Setting up shard infrastructure...\n";
        
        // Create ShardTopology
        ShardTopology::Config topology_config;
        topology_config.cluster_name = "lora-training-cluster";
        topology_config.metadata_endpoint = "http://localhost:2379";  // etcd
        topology_config.refresh_interval_sec = 30;
        topology_config.enable_health_checks = true;
        
        auto shard_topology = std::make_shared<ShardTopology>(topology_config);
        
        // Add example shards to topology
        ShardInfo shard1;
        shard1.shard_id = "shard-1";
        shard1.primary_endpoint = "localhost:8081";
        shard1.datacenter = "dc1";
        shard1.is_healthy = true;
        shard_topology->addShard(shard1);
        
        ShardInfo shard2;
        shard2.shard_id = "shard-2";
        shard2.primary_endpoint = "localhost:8082";
        shard2.datacenter = "dc1";
        shard2.is_healthy = true;
        shard_topology->addShard(shard2);
        
        ShardInfo shard3;
        shard3.shard_id = "shard-3";
        shard3.primary_endpoint = "localhost:8083";
        shard3.datacenter = "dc2";
        shard3.is_healthy = true;
        shard_topology->addShard(shard3);
        
        std::cout << "  ✓ ShardTopology created with " << shard_topology->getShardCount() << " shards\n";
        
        // Create ShardRouter
        // Note: In production, URNResolver and RemoteExecutor would be properly configured
        auto urn_resolver = std::make_shared<URNResolver>(shard_topology, URNResolver::Config{});
        auto remote_executor = std::make_shared<RemoteExecutor>();
        
        ShardRouter::Config router_config;
        router_config.local_shard_id = "shard-1";  // This node is shard-1
        router_config.scatter_timeout_ms = 30000;
        router_config.max_concurrent_shards = 10;
        router_config.enable_query_pushdown = true;
        
        auto shard_router = std::make_shared<ShardRouter>(
            urn_resolver,
            remote_executor,
            router_config
        );
        
        std::cout << "  ✓ ShardRouter created (local shard: " << router_config.local_shard_id << ")\n\n";
        
        // ============================================================================
        // Step 2: Register Infrastructure with TrainingServiceRegistry
        // ============================================================================
        
        std::cout << "[2/5] Registering shard infrastructure...\n";
        
        auto& registry = TrainingServiceRegistry::getInstance();
        registry.registerShardRouter(shard_router);
        registry.registerShardTopology(shard_topology);
        
        if (registry.hasShardInfrastructure()) {
            std::cout << "  ✓ ShardRouter and ShardTopology registered successfully\n\n";
        } else {
            std::cerr << "  ✗ Failed to register shard infrastructure\n";
            return 1;
        }
        
        // ============================================================================
        // Step 3: Configure LoRATrainingService for Distributed Training
        // ============================================================================
        
        std::cout << "[3/5] Configuring LoRATrainingService...\n";
        
        LoRATrainingService::Config training_config;
        
        // Basic training configuration
        training_config.base_model_path = "models/llama-2-7b.gguf";
        training_config.max_concurrent_training = 1;
        training_config.enable_checkpointing = true;
        training_config.checkpoint_interval_steps = 100;
        training_config.checkpoint_dir = "/tmp/lora_checkpoints";
        
        // Distributed training configuration
        training_config.enable_distributed_training = true;
        training_config.coordinator_shard = "shard-1";  // This shard coordinates
        training_config.participant_shards = {"shard-1", "shard-2", "shard-3"};
        
        // Option 1: Inject directly via config (preferred)
        training_config.shard_router = shard_router;
        training_config.shard_topology = shard_topology;
        training_config.auto_discover_shards = true;
        
        // LoRA hyperparameters
        training_config.default_hyperparameters.rank = 8;
        training_config.default_hyperparameters.alpha = 16.0f;
        training_config.default_hyperparameters.learning_rate = 3e-4f;
        training_config.default_hyperparameters.num_epochs = 3;
        training_config.default_hyperparameters.batch_size = 4;
        training_config.default_hyperparameters.dropout = 0.05f;
        
        // Production features
        training_config.mixed_precision.enabled = true;
        training_config.mixed_precision.dtype = PrecisionType::FP16;
        training_config.gradient_clipping.enabled = true;
        training_config.gradient_clipping.max_norm = 1.0f;
        training_config.gradient_accumulation.accumulation_steps = 4;
        
        std::cout << "  ✓ Training configuration created\n";
        std::cout << "    - Coordinator: " << training_config.coordinator_shard << "\n";
        std::cout << "    - Participants: " << training_config.participant_shards.size() << " shards\n";
        std::cout << "    - LoRA rank: " << training_config.default_hyperparameters.rank << "\n";
        std::cout << "    - Mixed precision: " << (training_config.mixed_precision.enabled ? "enabled" : "disabled") << "\n\n";
        
        // ============================================================================
        // Step 4: Create LoRATrainingService
        // ============================================================================
        
        std::cout << "[4/5] Creating LoRATrainingService...\n";
        
        // When creating the service, it will automatically register the shard infrastructure
        // from the config with the registry (if not already registered)
        LoRATrainingService training_service(training_config);
        
        std::cout << "  ✓ LoRATrainingService created\n";
        std::cout << "  ✓ Shard infrastructure is available for distributed training\n\n";
        
        // ============================================================================
        // Step 5: Execute Distributed Training
        // ============================================================================
        
        std::cout << "[5/5] Executing distributed training...\n\n";
        
        // Prepare training data
        TrainingData training_data;
        training_data.dataset_name = "example-dataset";
        
        // Add some example training samples
        for (int i = 0; i < 100; ++i) {
            TrainingDataSample sample;
            sample.input = "Translate to French: Hello, how are you?";
            sample.output = "Bonjour, comment allez-vous ?";
            training_data.samples.push_back(sample);
        }
        
        std::cout << "Training data prepared: " << training_data.size() << " samples\n\n";
        
        // Execute distributed training
        // This will use ShardRouter for gradient collection/broadcasting
        // and ShardTopology for shard discovery and health monitoring
        auto result = training_service.trainDistributed(
            "french-translation-adapter",
            training_data
        );
        
        // ============================================================================
        // Display Results
        // ============================================================================
        
        std::cout << "\n====================================================\n";
        std::cout << "Training Results\n";
        std::cout << "====================================================\n";
        std::cout << "Success: " << (result.success ? "YES" : "NO") << "\n";
        std::cout << "Adapter ID: " << result.adapter_id << "\n";
        std::cout << "Final Loss: " << result.final_loss << "\n";
        std::cout << "Epochs Completed: " << result.epochs_completed << "\n";
        std::cout << "Training Time: " << result.training_time.count() << " seconds\n";
        
        if (result.metrics.contains("distributed_mode")) {
            std::cout << "\nDistributed Training Metrics:\n";
            std::cout << "  Mode: Distributed\n";
            std::cout << "  Total Steps: " << result.metrics["total_steps"] << "\n";
            std::cout << "  Successful Steps: " << result.metrics["successful_steps"] << "\n";
            std::cout << "  Gradient Syncs: " << result.metrics["gradient_syncs"] << "\n";
            std::cout << "  Avg Sync Time: " << result.metrics["avg_sync_time_ms"] << " ms\n";
            std::cout << "  Active Shards: " << result.metrics["active_shards"] << "/" << result.metrics["total_shards"] << "\n";
            std::cout << "  Effective Speedup: " << result.metrics["effective_speedup"] << "x\n";
            std::cout << "  Communication Overhead: " << result.metrics["communication_overhead_pct"] << "%\n";
        }
        
        if (!result.success) {
            std::cout << "\nError: " << result.error_message << "\n";
        }
        
        std::cout << "\n====================================================\n";
        std::cout << "Example completed successfully!\n";
        std::cout << "====================================================\n";
        
        // Cleanup
        registry.clear();
        
        return result.success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
