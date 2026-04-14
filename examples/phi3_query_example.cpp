/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            phi3_query_example.cpp                             ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:36:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file phi3_query_example.cpp
 * @brief Example demonstrating Phi-3-Mini-4k model usage for queries
 * 
 * This example shows:
 * 1. Automatic model download on first use
 * 2. Basic query/inference with Phi-3
 * 3. Using Phi-3 for ThemisDB-specific queries
 * 4. Performance monitoring
 * 
 * Usage:
 *   ./phi3_query_example
 *   ./phi3_query_example "How do I create a collection?"
 */

#include "llm/embedded_llm.h"
#include "llm/llama_wrapper.h"
#include "llm/model_downloader.h"
#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

using namespace themis::llm;

// Load Phi-3 configuration from default_model_config.yaml
bool loadPhi3Config(EmbeddedLLM::Config& config) {
    try {
        YAML::Node yaml = YAML::LoadFile("config/default_model_config.yaml");
        
        if (!yaml["default_llm"]) {
            spdlog::error("No default_llm section in config");
            return false;
        }
        
        auto llm = yaml["default_llm"];
        
        // Model paths
        auto storage = llm["storage"];
        std::string model_dir = storage["model_dir"].as<std::string>();
        std::string model_filename = storage["model_filename"].as<std::string>();
        config.model_path = model_dir + "/" + model_filename;
        
        // Model identification
        config.model_id = llm["model_id"].as<std::string>();
        
        // Runtime settings
        auto runtime = llm["runtime"];
        config.n_ctx = runtime["context_size"].as<int>();
        config.n_gpu_layers = runtime["n_gpu_layers"].as<int>();
        config.n_threads = runtime["threads"].as<int>();
        config.n_batch = runtime["batch_size"].as<int>();
        
        // Inference settings
        auto inference = llm["inference"];
        config.temperature = inference["temperature"].as<float>();
        config.top_p = inference["top_p"].as<float>();
        config.top_k = inference["top_k"].as<int>();
        config.repeat_penalty = inference["repeat_penalty"].as<float>();
        
        spdlog::info("Loaded Phi-3 configuration:");
        spdlog::info("  Model: {}", config.model_id);
        spdlog::info("  Path: {}", config.model_path);
        spdlog::info("  Context: {} tokens", config.n_ctx);
        spdlog::info("  GPU layers: {}", config.n_gpu_layers);
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load Phi-3 config: {}", e.what());
        return false;
    }
}

// Ensure Phi-3 model is downloaded
bool ensurePhi3ModelAvailable(const std::string& model_path) {
    // Check if model already exists
    if (ModelDownloader::isModelAvailable(model_path)) {
        spdlog::info("✓ Phi-3 model found: {}", model_path);
        return true;
    }
    
    spdlog::info("Phi-3 model not found. Starting auto-download...");
    
    try {
        // Load download configuration
        YAML::Node yaml = YAML::LoadFile("config/default_model_config.yaml");
        auto llm = yaml["default_llm"];
        
        bool auto_download = llm["auto_download"].as<bool>();
        if (!auto_download) {
            spdlog::error("Auto-download is disabled in configuration");
            return false;
        }
        
        // Configure download
        ModelDownloadConfig dl_config;
        dl_config.model_name = llm["ollama"]["model_name"].as<std::string>();
        dl_config.ollama_url = llm["ollama"]["endpoint"].as<std::string>();
        
        // Extract directory from model path
        size_t last_slash = model_path.find_last_of("/\\");
        dl_config.download_dir = model_path.substr(0, last_slash);
        
        // Progress callback
        dl_config.progress_callback = [](size_t downloaded, size_t total, const std::string& status) {
            if (total > 0) {
                float percent = 100.0f * downloaded / total;
                float mb_downloaded = downloaded / (1024.0f * 1024.0f);
                float mb_total = total / (1024.0f * 1024.0f);
                spdlog::info("Downloading: {:.1f}% ({:.1f} / {:.1f} MB)", 
                    percent, mb_downloaded, mb_total);
            }
        };
        
        // Download model
        ModelDownloader downloader;
        auto result = downloader.downloadFromOllama(dl_config);
        
        if (result.success) {
            spdlog::info("✓ Phi-3 model downloaded successfully");
            spdlog::info("  Path: {}", result.model_path);
            spdlog::info("  Size: {:.1f} MB", result.file_size_bytes / (1024.0f * 1024.0f));
            spdlog::info("  Time: {:.1f}s", result.download_time_seconds);
            return true;
        } else {
            spdlog::error("Failed to download Phi-3 model: {}", result.error_message);
            return false;
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Exception during model download: {}", e.what());
        return false;
    }
}

int main(int argc, char** argv) {
    // Configure logging
    spdlog::set_level(spdlog::level::info);
    spdlog::info("=== Phi-3-Mini-4k Query Example ===\n");
    
    // Step 1: Load Phi-3 configuration
    EmbeddedLLM::Config config;
    if (!loadPhi3Config(config)) {
        spdlog::error("Failed to load Phi-3 configuration");
        return 1;
    }
    
    // Step 2: Ensure model is available (auto-download if needed)
    if (!ensurePhi3ModelAvailable(config.model_path)) {
        spdlog::error("Phi-3 model not available and download failed");
        spdlog::info("\nPlease ensure:");
        spdlog::info("1. Ollama is running (http://localhost:11434)");
        spdlog::info("2. Network connectivity is available");
        spdlog::info("3. Sufficient disk space (~2.3 GB)");
        return 1;
    }
    
    // Step 3: Initialize LLM
    spdlog::info("\nInitializing Phi-3 model...");
    
    try {
        // Initialize embedded LLM manager
        EmbeddedLLMManager::instance().initialize(config);
        spdlog::info("✓ Phi-3 model loaded and ready\n");
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize Phi-3: {}", e.what());
        return 1;
    }
    
    // Step 4: Run queries
    std::vector<std::string> queries;
    
    if (argc > 1) {
        // Use command-line argument
        queries.push_back(argv[1]);
    } else {
        // Use example ThemisDB queries
        queries = {
            "What is ThemisDB?",
            "How do I create a vector index in ThemisDB?",
            "Explain graph queries in ThemisDB using AQL.",
            "What are the benefits of using LoRA adapters?"
        };
    }
    
    spdlog::info("Running {} {}", queries.size(), queries.size() > 1 ? "queries" : "query");
    
    for (size_t i = 0; i < queries.size(); ++i) {
        const auto& prompt = queries[i];
        
        spdlog::info("\n--- Query {} ---", i + 1);
        spdlog::info("Prompt: {}", prompt);
        
        auto start = std::chrono::steady_clock::now();
        
        try {
            // Generate response using Phi-3
            std::string response = THEMIS_LLM_GENERATE(prompt);
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            spdlog::info("\nResponse:\n{}", response);
            spdlog::info("\nGeneration time: {} ms", duration.count());
            
            // Estimate tokens per second (rough approximation)
            size_t approx_tokens = response.length() / 4;  // ~4 chars per token
            float tokens_per_sec = approx_tokens * 1000.0f / duration.count();
            spdlog::info("Estimated speed: {:.1f} tokens/sec", tokens_per_sec);
            
        } catch (const std::exception& e) {
            spdlog::error("Query failed: {}", e.what());
        }
    }
    
    spdlog::info("\n=== Example completed ===");
    
    return 0;
}
