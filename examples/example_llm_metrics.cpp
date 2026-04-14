/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_llm_metrics.cpp                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     185                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_llm_metrics.cpp
 * @brief Simple example demonstrating LLM metrics integration
 * 
 * This example shows how to use the Grafana metrics integration
 * with the LlamaWrapper LLM plugin.
 * 
 * Compile:
 *   g++ -std=c++20 -I../include example_llm_metrics.cpp -o example_llm_metrics
 * 
 * Run:
 *   ./example_llm_metrics
 *   # In another terminal:
 *   curl http://localhost:9091/metrics
 */

#include "llm/llama_wrapper.h"
#include "llm/grafana_metrics.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm;
using namespace themis::llm::monitoring;

int main() {
    std::cout << "=== ThemisDB LLM Metrics Example ===" << std::endl;
    
    // Step 1: Create Prometheus exporter
    std::cout << "\n1. Initializing Prometheus exporter..." << std::endl;
    auto exporter = std::make_unique<PrometheusExporter>();
    
    // Step 2: Create metrics collector
    std::cout << "2. Creating LLM metrics collector..." << std::endl;
    auto metrics_collector = std::make_unique<LLMMetricsCollector>(exporter.get());
    
    // Step 3: Start metrics server (optional)
    std::cout << "3. Starting metrics server on port 9091..." << std::endl;
    MetricsServer::ServerConfig server_config;
    server_config.port = 9091;
    server_config.metrics_path = "/metrics";
    
    MetricsServer metrics_server(server_config, exporter.get());
    bool started = metrics_server.start();
    
    if (started) {
        std::cout << "   ✓ Metrics server started: " << metrics_server.getMetricsURL() << std::endl;
    } else {
        std::cout << "   ✗ Failed to start metrics server" << std::endl;
    }
    
    // Step 4: Configure LlamaWrapper
    std::cout << "\n4. Configuring LlamaWrapper..." << std::endl;
    LlamaWrapper::Config llm_config;
    llm_config.n_gpu_layers = 0;  // CPU only for this example
    llm_config.n_ctx = 2048;
    llm_config.n_batch = 512;
    
    // Note: Response cache is ENABLED BY DEFAULT (enable_response_cache = true)
    // Uncomment to disable: llm_config.enable_response_cache = false;
    
    auto wrapper = std::make_unique<LlamaWrapper>(llm_config);
    
    // Step 5: Connect metrics collector to wrapper
    std::cout << "5. Connecting metrics collector..." << std::endl;
    wrapper->setMetricsCollector(metrics_collector.get());
    std::cout << "   ✓ Metrics collector connected" << std::endl;
    
    // Step 6: Load a model
    std::cout << "\n6. Loading model..." << std::endl;
    bool loaded = wrapper->loadModel("models/example-7b-q4.gguf");
    
    if (loaded) {
        std::cout << "   ✓ Model loaded successfully" << std::endl;
    } else {
        std::cout << "   ✗ Model load failed (this is expected in example)" << std::endl;
        std::cout << "   ℹ Using stub implementation for demonstration" << std::endl;
    }
    
    // Step 7: Perform some inferences
    std::cout << "\n7. Running inference examples..." << std::endl;
    
    const char* prompts[] = {
        "What is machine learning?",
        "Explain quantum computing in simple terms.",
        "What are the benefits of vector databases?"
    };
    
    for (int i = 0; i < 3; ++i) {
        std::cout << "\n   Request " << (i+1) << ": " << prompts[i] << std::endl;
        
        InferenceRequest request;
        request.request_id = "example-" + std::to_string(i);
        request.prompt = prompts[i];
        request.max_tokens = 50;
        request.temperature = 0.7f;
        
        try {
            auto response = wrapper->generate(request);
            
            std::cout << "   ✓ Generated " << response.tokens_generated << " tokens" << std::endl;
            std::cout << "   ✓ Latency: " << response.inference_time_ms << " ms" << std::endl;
            std::cout << "   ✓ Throughput: " << response.tokens_per_second << " tokens/sec" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "   ✗ Inference error: " << e.what() << std::endl;
        }
        
        // Small delay between requests
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Step 8: Export and display metrics
    std::cout << "\n8. Exporting metrics..." << std::endl;
    std::string metrics_output = exporter->exportMetrics();
    
    std::cout << "\n=== PROMETHEUS METRICS ===" << std::endl;
    std::cout << metrics_output << std::endl;
    std::cout << "=== END METRICS ===" << std::endl;
    
    // Step 9: Generate dashboard
    std::cout << "\n9. Generating Grafana dashboard..." << std::endl;
    GrafanaDashboardGenerator::DashboardConfig dashboard_config;
    dashboard_config.title = "Example LLM Monitoring";
    dashboard_config.refresh_interval_sec = 5;
    
    GrafanaDashboardGenerator dashboard_gen(dashboard_config);
    std::string dashboard_json = dashboard_gen.generateDashboard();
    
    std::cout << "   ✓ Dashboard JSON generated (" << dashboard_json.size() << " bytes)" << std::endl;
    
    // Optionally save dashboard
    bool saved = dashboard_gen.saveDashboard("/tmp/example_dashboard.json");
    if (saved) {
        std::cout << "   ✓ Dashboard saved to /tmp/example_dashboard.json" << std::endl;
    }
    
    // Step 10: Summary
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "✓ Metrics collection is active" << std::endl;
    std::cout << "✓ All LLM operations are being tracked" << std::endl;
    std::cout << "✓ Metrics available at: " << metrics_server.getMetricsURL() << std::endl;
    std::cout << "\nTo view metrics:" << std::endl;
    std::cout << "  curl " << metrics_server.getMetricsURL() << std::endl;
    std::cout << "\nTo start Grafana:" << std::endl;
    std::cout << "  cd grafana && docker-compose up -d" << std::endl;
    std::cout << "  open http://localhost:3000" << std::endl;
    
    // Keep server running for a bit
    std::cout << "\nMetrics server will run for 30 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    metrics_server.stop();
    wrapper->unloadModel();
    
    std::cout << "Done!" << std::endl;
    
    return 0;
}
