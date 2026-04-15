/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            phi3_lora_training_example.cpp                     ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file phi3_lora_training_example.cpp
 * @brief Example demonstrating LoRA adapter training on Phi-3-Mini-4k
 * 
 * This example shows:
 * 1. Loading Phi-3 as the base model
 * 2. Preparing training data for ThemisDB help queries
 * 3. Training a LoRA adapter with Phi-3 specific configuration
 * 4. Validating the trained adapter
 * 5. Monitoring training metrics
 * 
 * Usage:
 *   ./phi3_lora_training_example
 */

#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_config.h"
#include "llm/model_downloader.h"
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>

using namespace themis::llm::lora;

// Load Phi-3 LoRA training configuration
LoRATrainingService::Config loadPhi3TrainingConfig() {
    try {
        YAML::Node yaml = YAML::LoadFile("config/phi3_lora_training.yaml");
        auto phi3_config = yaml["phi3_lora_training"];
        
        LoRATrainingService::Config config;
        
        // Base model
        auto base_model = phi3_config["base_model"];
        config.base_model_path = base_model["model_path"].as<std::string>();
        
        // Target modules (Phi-3 specific)
        if (phi3_config["target_modules"]) {
            for (const auto& module : phi3_config["target_modules"]) {
                config.target_modules.push_back(module.as<std::string>());
            }
        }
        
        // Hyperparameters
        auto hyperparams = phi3_config["hyperparameters"];
        config.default_hyperparameters.rank = hyperparams["rank"].as<int>();
        config.default_hyperparameters.alpha = hyperparams["alpha"].as<float>();
        config.default_hyperparameters.dropout = hyperparams["dropout"].as<float>();
        config.default_hyperparameters.learning_rate = hyperparams["learning_rate"].as<float>();
        config.default_hyperparameters.batch_size = hyperparams["batch_size"].as<int>();
        config.default_hyperparameters.num_epochs = hyperparams["num_epochs"].as<int>();
        config.default_hyperparameters.max_seq_length = hyperparams["max_seq_length"].as<int>();
        
        // Training features
        auto training = phi3_config["training"];
        config.enable_checkpointing = training["checkpointing"]["enabled"].as<bool>();
        config.checkpoint_interval_steps = training["checkpointing"]["checkpoint_interval_steps"].as<int>();
        config.checkpoint_dir = training["checkpointing"]["checkpoint_dir"].as<std::string>();
        
        // Mixed precision
        config.mixed_precision.enabled = training["mixed_precision"]["enabled"].as<bool>();
        config.mixed_precision.use_fp16 = training["mixed_precision"]["fp16"].as<bool>();
        
        // Gradient clipping
        config.gradient_clipping.enabled = training["gradient_clipping"]["enabled"].as<bool>();
        config.gradient_clipping.max_grad_norm = training["gradient_clipping"]["max_grad_norm"].as<float>();
        
        spdlog::info("Loaded Phi-3 training configuration:");
        spdlog::info("  Base model: {}", config.base_model_path);
        spdlog::info("  Rank: {}, Alpha: {}", config.default_hyperparameters.rank, config.default_hyperparameters.alpha);
        spdlog::info("  Learning rate: {}", config.default_hyperparameters.learning_rate);
        spdlog::info("  Target modules: {}", fmt::join(config.target_modules, ", "));
        
        return config;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load training config: {}", e.what());
        throw;
    }
}

// Prepare example training data for ThemisDB help
TrainingData prepareThemisHelpData() {
    TrainingData data;
    data.dataset_name = "themis_help_phi3";
    
    // Sample 1: Vector index creation
    TrainingDataSample sample1;
    sample1.input = "How do I create a vector index in ThemisDB?";
    sample1.output = "To create a vector index in ThemisDB, use the following AQL syntax:\n\n"
        "CREATE VECTOR INDEX idx_name ON collection_name(field_name) WITH METRIC 'cosine'\n\n"
        "Supported similarity metrics are: cosine, euclidean, and dot_product. "
        "Vector indexes enable efficient semantic search and similarity queries.";
    data.samples.push_back(sample1);
    
    // Sample 2: Graph queries
    TrainingDataSample sample2;
    sample2.input = "How do I query a graph in ThemisDB?";
    sample2.output = "ThemisDB supports graph queries using AQL with MATCH syntax:\n\n"
        "MATCH (n:Person)-[r:KNOWS]->(m:Person)\n"
        "WHERE n.name = 'Alice'\n"
        "RETURN n, r, m\n\n"
        "This finds all people that Alice knows. You can also use more complex patterns "
        "and traverse multiple hops in the graph.";
    data.samples.push_back(sample2);
    
    // Sample 3: Collection creation
    TrainingDataSample sample3;
    sample3.input = "How do I create a collection?";
    sample3.output = "Create a collection in ThemisDB using:\n\n"
        "CREATE COLLECTION users\n\n"
        "For a collection with schema:\n\n"
        "CREATE COLLECTION users (\n"
        "  name STRING,\n"
        "  email STRING,\n"
        "  age INT\n"
        ")\n\n"
        "Collections can store documents, graphs, vectors, and time-series data.";
    data.samples.push_back(sample3);
    
    // Sample 4: LoRA adapters
    TrainingDataSample sample4;
    sample4.input = "What are LoRA adapters in ThemisDB?";
    sample4.output = "LoRA (Low-Rank Adaptation) adapters in ThemisDB allow you to fine-tune "
        "the LLM for specific use cases without retraining the entire model. "
        "Benefits include:\n\n"
        "1. Efficiency: Only train a small number of parameters (~1% of base model)\n"
        "2. Speed: Training completes in minutes instead of hours\n"
        "3. Flexibility: Switch between adapters for different domains\n"
        "4. Storage: Each adapter is only 15-20 MB\n\n"
        "Use the TRAIN LORA command to create custom adapters for your specific data.";
    data.samples.push_back(sample4);
    
    // Sample 5: Query performance
    TrainingDataSample sample5;
    sample5.input = "How do I optimize query performance?";
    sample5.output = "To optimize query performance in ThemisDB:\n\n"
        "1. Create indexes on frequently queried fields\n"
        "2. Use vector indexes for similarity search\n"
        "3. Leverage query caching for repeated queries\n"
        "4. Partition large datasets using sharding\n"
        "5. Monitor query execution plans with EXPLAIN\n\n"
        "ThemisDB also supports GPU acceleration for vector operations.";
    data.samples.push_back(sample5);
    
    spdlog::info("Prepared {} training samples", data.samples.size());
    
    return data;
}

int main(int argc, char** argv) {
    // Configure logging
    spdlog::set_level(spdlog::level::info);
    spdlog::info("=== Phi-3 LoRA Training Example ===\n");
    
    try {
        // Step 1: Load training configuration
        spdlog::info("Step 1: Loading Phi-3 training configuration...");
        auto config = loadPhi3TrainingConfig();
        
        // Step 2: Verify base model exists
        spdlog::info("\nStep 2: Checking base model availability...");
        if (!ModelDownloader::isModelAvailable(config.base_model_path)) {
            spdlog::error("Base model not found: {}", config.base_model_path);
            spdlog::info("\nPlease run phi3_query_example first to download the model,");
            spdlog::info("or ensure the model exists at the configured path.");
            return 1;
        }
        spdlog::info("✓ Base model found: {}", config.base_model_path);
        
        // Step 3: Initialize training service
        spdlog::info("\nStep 3: Initializing LoRA training service...");
        LoRATrainingService training_service(config);
        spdlog::info("✓ Training service initialized");
        
        // Step 4: Prepare training data
        spdlog::info("\nStep 4: Preparing training data...");
        auto training_data = prepareThemisHelpData();
        spdlog::info("✓ Training data ready");
        
        // Step 5: Register progress callback
        training_service.registerCallback([](const TrainingMetrics& metrics) {
            if (metrics.current_step % 10 == 0) {  // Log every 10 steps
                spdlog::info("  Epoch {}/{} | Step {}/{} | Loss: {:.4f} | LR: {:.6f}",
                    metrics.current_epoch, metrics.total_epochs,
                    metrics.current_step, metrics.total_steps,
                    metrics.current_loss, metrics.learning_rate);
            }
        });
        
        // Step 6: Train adapter
        spdlog::info("\nStep 6: Training LoRA adapter on Phi-3...");
        spdlog::info("This may take a few minutes depending on your hardware.\n");
        
        auto start = std::chrono::steady_clock::now();
        
        auto result = training_service.trainOnTheFly(
            "themis_help_phi3_v1",  // Adapter ID
            training_data
        );
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        
        // Step 7: Check results
        spdlog::info("\n=== Training Results ===");
        
        if (result.success) {
            spdlog::info("✓ Training completed successfully!");
            spdlog::info("  Adapter ID: {}", result.adapter_id);
            spdlog::info("  Version: {}", result.version);
            spdlog::info("  Final loss: {:.6f}", result.final_loss);
            spdlog::info("  Validation accuracy: {:.2f}%", result.validation_accuracy * 100);
            spdlog::info("  Epochs completed: {}", result.epochs_completed);
            spdlog::info("  Training time: {}s", duration.count());
            
            // Display metrics
            if (!result.metrics.empty()) {
                spdlog::info("\n  Additional metrics:");
                for (const auto& [key, value] : result.metrics.items()) {
                    spdlog::info("    {}: {}", key, value.dump());
                }
            }
            
            spdlog::info("\n✓ Adapter saved and ready for use");
            spdlog::info("  Load this adapter using: LOAD LORA \"{}\"", result.adapter_id);
            
        } else {
            spdlog::error("✗ Training failed: {}", result.error_message);
            return 1;
        }
        
        // Step 8: Validation suggestions
        spdlog::info("\n=== Next Steps ===");
        spdlog::info("1. Test the adapter with sample queries");
        spdlog::info("2. Compare responses with and without the adapter");
        spdlog::info("3. Monitor adapter performance in production");
        spdlog::info("4. Collect feedback and retrain as needed");
        
    } catch (const std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        return 1;
    }
    
    spdlog::info("\n=== Example completed ===");
    return 0;
}
