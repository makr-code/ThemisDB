/**
 * @file example_layered_retrieval_integration.cpp
 * @brief Example showing end-to-end usage of LayeredRetrievalOrchestrator.
 *
 * This example demonstrates:
 * - Creating and configuring an orchestrator
 * - Executing a complete retrieval pipeline
 * - Handling results and errors
 * - Inspecting layer-by-layer decisions
 * - Using observability features
 */

#include "search/layered_retrieval_orchestrator.h"
#include "index/ann_frontdoor.h"
#include "tensor/tensor_mid_layer.h"
#include "rag/graph_truth_validator.h"
#include "llm/final_layer_orchestrator.h"
#include "utils/logger.h"

#include <iostream>
#include <iomanip>

namespace themis {
namespace examples {

/**
 * @brief Complete example of LayeredRetrievalOrchestrator usage.
 */
void exampleLayeredRetrieval() {
    using namespace search;

    // ========================================================================
    // 1. Create Orchestrator
    // ========================================================================

    THEMIS_INFO("=== Layered Retrieval Orchestrator Example ===");
    
    auto orchestrator = std::make_shared<LayeredRetrievalOrchestrator>();

    // ========================================================================
    // 2. Inject Layer Components
    // ========================================================================
    // In production, these would be your actual layer implementations:
    
    // auto ann_frontdoor = createAnnFrontdoor();
    // auto tensor_layer = createTensorMidLayer();
    // auto graph_validator = createGraphTruthValidator();
    // auto llm_orchestrator = createFinalLayerOrchestrator();
    //
    // orchestrator->setAnnFrontdoor(ann_frontdoor);
    // orchestrator->setTensorMidLayer(tensor_layer);
    // orchestrator->setGraphTruthValidator(graph_validator);
    // orchestrator->setFinalLayerOrchestrator(llm_orchestrator);

    // ========================================================================
    // 3. Verify Health
    // ========================================================================

    THEMIS_INFO("\n--- Orchestrator Status ---");
    std::cout << orchestrator->statusReport() << std::endl;

    if (!orchestrator->isHealthy()) {
        THEMIS_WARN("Warning: Some layers are not configured. "
                    "In production, all layers should be injected.");
    }

    // ========================================================================
    // 4. Configure Behavior
    // ========================================================================

    THEMIS_INFO("\n--- Configuration ---");

    LayeredRetrievalConfig config;
    config.enable_ann_layer = true;
    config.enable_tensor_layer = true;
    config.enable_graph_layer = true;
    config.enable_llm_layer = true;

    config.ann_k = 100;           // Initial candidate count from ANN
    config.tensor_top_k = 50;     // Refined by tensor layer
    config.graph_top_k = 20;      // Validated by graph layer
    config.max_candidates = 10000;
    config.timeout_ms = 5000;
    config.fail_closed_on_graph_error = false;  // Graceful degradation
    config.allow_layer_fallback = true;

    orchestrator->setConfig(config);

    THEMIS_INFO("Configuration set:");
    THEMIS_INFO("  ANN k: {}", config.ann_k);
    THEMIS_INFO("  Tensor top_k: {}", config.tensor_top_k);
    THEMIS_INFO("  Graph top_k: {}", config.graph_top_k);
    THEMIS_INFO("  Fail-closed mode: {}", config.fail_closed_on_graph_error);

    // ========================================================================
    // 5. Prepare Query
    // ========================================================================

    THEMIS_INFO("\n--- Query Preparation ---");

    // Create embedding (in practice, from embedding model)
    std::vector<float> query_embedding(128);
    for (size_t i = 0; i < query_embedding.size(); ++i) {
        query_embedding[i] = 0.1f + (i % 10) * 0.01f;
    }

    std::string query_text = "What are the key features of ThemisDB?";
    THEMIS_INFO("Query: {}", query_text);
    THEMIS_INFO("Query embedding dimension: {}", query_embedding.size());

    // ========================================================================
    // 6. Create Retrieval Context
    // ========================================================================

    THEMIS_INFO("\n--- Retrieval Context ---");

    LayeredRetrievalContext context;
    context.query_text = query_text;
    context.correlation_id = "example-trace-001";
    context.tenant_id = "org-acme";
    context.principal = "user@acme.com";
    context.domain = "documentation";
    context.base_model_id = "llama-2-7b";
    context.requested_package_id = "themis-docs-package";
    context.trace_enabled = true;

    THEMIS_INFO("Correlation ID: {}", context.correlation_id);
    THEMIS_INFO("Tenant: {}", context.tenant_id);
    THEMIS_INFO("Principal: {}", context.principal);
    THEMIS_INFO("Domain: {}", context.domain);

    // ========================================================================
    // 7. Execute Retrieval Pipeline
    // ========================================================================

    THEMIS_INFO("\n--- Executing Retrieval Pipeline ---");

    auto result = orchestrator->execute(
        query_embedding.data(),
        query_embedding.size(),
        query_text,
        context
    );

    // ========================================================================
    // 8. Process Results
    // ========================================================================

    THEMIS_INFO("\n--- Retrieval Result ---");

    std::cout << std::string(60, '=') << std::endl;
    std::cout << "SUCCESS: " << (result.success ? "✓" : "✗") << std::endl;
    std::cout << "CORRELATION ID: " << result.correlation_id << std::endl;
    std::cout << "TOTAL LATENCY: " << result.total_latency_ms.count() << "ms" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // ========================================================================
    // 9. Inspect Layer-by-Layer Decisions
    // ========================================================================

    THEMIS_INFO("\n--- Layer-by-Layer Decisions ---");

    for (size_t i = 0; i < result.layer_decisions.size(); ++i) {
        const auto& decision = result.layer_decisions[i];

        std::cout << std::string(60, '-') << std::endl;
        std::cout << "Layer " << (i + 1) << ": " << decision.layer_name << std::endl;
        std::cout << "  Status: " << (decision.success ? "✓ SUCCESS" : "✗ FAILED") << std::endl;
        std::cout << "  Latency: " << decision.elapsed_ms.count() << "ms" << std::endl;
        std::cout << "  Reason: " << decision.routing_reason << std::endl;
        std::cout << "  Reason Code: " << decision.routing_reason_code << std::endl;

        if (!decision.warnings.empty()) {
            std::cout << "  Warnings:" << std::endl;
            for (const auto& warning : decision.warnings) {
                std::cout << "    - " << warning << std::endl;
            }
        }

        if (!decision.errors.empty()) {
            std::cout << "  Errors:" << std::endl;
            for (const auto& error : decision.errors) {
                std::cout << "    - " << error << std::endl;
            }
        }
    }

    // ========================================================================
    // 10. Inspect Evidence Bundle
    // ========================================================================

    THEMIS_INFO("\n--- Evidence Bundle ---");

    if (result.evidence_bundle.empty()) {
        std::cout << "No evidence retrieved." << std::endl;
    } else {
        std::cout << "Retrieved " << result.evidence_bundle.size() << " evidence items:" << std::endl;

        for (size_t i = 0; i < result.evidence_bundle.size(); ++i) {
            const auto& evidence = result.evidence_bundle[i];

            std::cout << "\nEvidence #" << (i + 1) << ":" << std::endl;
            std::cout << "  ID: " << evidence.candidate_id << std::endl;
            std::cout << std::fixed << std::setprecision(4);
            std::cout << "  Graph Score: " << evidence.graph_score << std::endl;
            std::cout << "  Tensor Score: " << evidence.tensor_score << std::endl;
            std::cout << "  Validated: " << (evidence.validated ? "✓" : "✗") << std::endl;
            std::cout << "  Supporting Nodes: " << evidence.supporting_nodes.size() << std::endl;

            if (!evidence.reasoning_chain.empty()) {
                std::cout << "  Reasoning: " << evidence.reasoning_chain << std::endl;
            }
        }
    }

    // ========================================================================
    // 11. Inspect Provenance Trail
    // ========================================================================

    THEMIS_INFO("\n--- Provenance Trail ---");

    if (result.provenance_trail.empty()) {
        std::cout << "No provenance records." << std::endl;
    } else {
        std::cout << "Provenance records for " << result.provenance_trail.size()
                  << " evidence items:" << std::endl;

        for (const auto& prov : result.provenance_trail) {
            std::cout << "\nEvidence: " << prov.evidence_id << std::endl;
            std::cout << "  Source Layer: " << prov.source_layer << std::endl;
            std::cout << "  Path: ";

            for (size_t i = 0; i < prov.layer_decisions.size(); ++i) {
                std::cout << prov.layer_decisions[i];
                if (i < prov.layer_decisions.size() - 1) {
                    std::cout << " → ";
                }
            }
            std::cout << std::endl;

            if (!prov.reasoning_chain.empty()) {
                std::cout << "  Reasoning: " << prov.reasoning_chain << std::endl;
            }
        }
    }

    // ========================================================================
    // 12. Display Final Answer
    // ========================================================================

    THEMIS_INFO("\n--- Generated Answer ---");

    std::cout << std::string(60, '=') << std::endl;
    if (result.success) {
        std::cout << "ANSWER:" << std::endl;
        std::cout << result.final_answer << std::endl;
    } else {
        std::cout << "ERROR:" << std::endl;
        std::cout << result.final_answer << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;

    // ========================================================================
    // 13. Error Handling Example
    // ========================================================================

    THEMIS_INFO("\n--- Error Handling ---");

    if (!result.success) {
        THEMIS_WARN("Retrieval failed. Checking layer errors:");

        for (const auto& decision : result.layer_decisions) {
            if (!decision.success) {
                THEMIS_ERROR("  Layer {} failed: {}", decision.layer_name,
                            decision.routing_reason);

                for (const auto& error : decision.errors) {
                    THEMIS_ERROR("    - {}", error);
                }
            }
        }
    }

    // ========================================================================
    // 14. Configuration Modification Example
    // ========================================================================

    THEMIS_INFO("\n--- Configuration Modification ---");

    // Modify configuration for next retrieval
    LayeredRetrievalConfig modified_config = orchestrator->config();
    modified_config.enable_tensor_layer = false;  // Disable tensor layer
    modified_config.fail_closed_on_graph_error = true;  // Strict mode
    orchestrator->setConfig(modified_config);

    THEMIS_INFO("Updated configuration - Tensor layer disabled, fail-closed enabled");

    THEMIS_INFO("\n=== Example Complete ===");
}

} // namespace examples
} // namespace themis

// Main entry point (for demonstration)
// Uncomment to compile and run standalone
/*
int main() {
    themis::examples::exampleLayeredRetrieval();
    return 0;
}
*/
