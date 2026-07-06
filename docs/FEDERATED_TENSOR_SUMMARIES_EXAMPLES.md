# Federated Tensor Summaries - Practical Examples

This document provides concrete, working examples of using federated and cross-shard tensor summaries in ThemisDB applications.

## Example 1: Basic Multi-Shard Query

```cpp
#include "tensor/tensor_mid_layer.h"
#include <iostream>

int main() {
    // Create tensor mid-layer with dependencies
    auto mid_layer = std::make_unique<themis::tensor::TensorMidLayer>();
    auto adapter_repo = std::make_shared<themis::tensor::AdapterRepository>();
    auto fp_graph = std::make_shared<themis::tensor::TensorFingerprintGraph>();
    
    mid_layer->setAdapterRepository(adapter_repo);
    mid_layer->setFingerprintGraph(fp_graph);
    
    // Setup federated query context
    themis::tensor::TensorLayerContext context;
    context.tenant_id = "acme_corp";
    context.domain = "legal";
    context.base_model_id = "llama3-70b";
    context.shard_scope_ids = {"shard_eu_west", "shard_us_east", "shard_asia_pacific"};
    context.shard_aware = true;
    context.top_k = 20;
    
    // Execute federated query
    auto federated = mid_layer->summarizeFederatedShards(context);
    
    // Print results
    std::cout << "Federated Summary Results:" << std::endl;
    std::cout << "Shards queried: " << federated.shard_summaries.size() << std::endl;
    std::cout << "Merged adapters: " << federated.merged_similar_adapters.size() << std::endl;
    std::cout << "Routing reason: " << federated.routing_reason << std::endl;
    
    // Process merged results
    for (const auto& adapter : federated.merged_similar_adapters) {
        std::cout << "  - " << adapter.adapter_key 
                  << ": score=" << adapter.score << std::endl;
    }
    
    return 0;
}
```

## Example 2: Shard Health-Aware Selection

```cpp
#include "tensor/tensor_mid_layer.h"
#include <algorithm>

class HealthAwareShardSelector {
private:
    std::shared_ptr<themis::tensor::TensorMidLayer> mid_layer_;
    float health_threshold_ = 0.7f;  // 70% uptime required
    float latency_limit_ms_ = 1000.0f;
    
public:
    std::vector<std::string> selectHealthyShards(
        const themis::tensor::FederatedTensorSummary& federated) {
        
        std::vector<std::string> selected;
        
        for (const auto& shard_summary : federated.shard_summaries) {
            // Check shard health
            if (!shard_summary.shard_healthy) {
                std::cout << "Shard " << shard_summary.scope_key 
                          << " unhealthy, skipping" << std::endl;
                continue;
            }
            
            // Check latency
            if (shard_summary.retrieval_latency_ms > latency_limit_ms_) {
                std::cout << "Shard " << shard_summary.scope_key 
                          << " slow (" << shard_summary.retrieval_latency_ms 
                          << "ms), skipping" << std::endl;
                continue;
            }
            
            // Check relevance
            if (shard_summary.shard_relevance < health_threshold_) {
                std::cout << "Shard " << shard_summary.scope_key 
                          << " low relevance, skipping" << std::endl;
                continue;
            }
            
            selected.push_back(shard_summary.scope_key);
            std::cout << "Shard " << shard_summary.scope_key 
                      << " selected (latency: " << shard_summary.retrieval_latency_ms 
                      << "ms)" << std::endl;
        }
        
        return selected;
    }
};
```

## Example 3: Adaptive Compression Based on Domain

```cpp
#include "tensor/tensor_summary_types.h"

class AdaptiveCompressionSelector {
private:
    // Compression ratios by domain sensitivity
    std::unordered_map<std::string, float> domain_compression_ratios_ = {
        {"legal", 1.5f},           // Critical: low compression
        {"financial", 2.0f},       // Important: moderate compression
        {"medical", 2.0f},         // Important: moderate compression
        {"research", 4.0f},        // Standard: normal compression
        {"marketing", 8.0f},       // Non-critical: aggressive compression
    };
    
public:
    themis::tensor::ShardSummary createOptimizedSummary(
        const std::string& shard_id,
        const std::string& domain,
        std::size_t candidates_before,
        const themis::tensor::CompressionResult& base_result) {
        
        // Adjust compression ratio by domain
        auto it = domain_compression_ratios_.find(domain);
        float compression_ratio = (it != domain_compression_ratios_.end()) 
                                  ? it->second 
                                  : 4.0f;
        
        // Create adjusted compression result
        themis::tensor::CompressionResult adjusted = base_result;
        adjusted.compression_ratio = compression_ratio;
        
        // Log the selection
        std::cout << "Domain: " << domain 
                  << ", Compression Ratio: " << compression_ratio << std::endl;
        
        // Create summary with adjusted compression
        return themis::tensor::SummaryFactory::createShardSummary(
            shard_id,
            candidates_before,
            adjusted
        );
    }
};
```

## Example 4: Summary-First with Fallback

```cpp
#include "tensor/tensor_mid_layer.h"

class FallbackRetrievalFlow {
private:
    std::shared_ptr<themis::tensor::TensorMidLayer> mid_layer_;
    float confidence_threshold_ = 0.8f;
    std::size_t min_results_ = 5;
    
public:
    std::vector<themis::tensor::SimilarityResult> executeWithFallback(
        const std::vector<std::string>& shard_ids,
        const std::string& domain,
        const std::string& base_model_id) {
        
        // Phase 1: Try summary-first retrieval
        themis::tensor::TensorLayerContext context;
        context.shard_scope_ids = shard_ids;
        context.domain = domain;
        context.base_model_id = base_model_id;
        context.shard_aware = true;
        context.top_k = 20;
        
        auto federated = mid_layer_->summarizeFederatedShards(context);
        
        std::cout << "Phase 1: Summary-first retrieval" << std::endl;
        std::cout << "  Results: " << federated.merged_similar_adapters.size() << std::endl;
        
        // Phase 2: Evaluate confidence
        float confidence = evaluateConfidence(federated);
        std::cout << "  Confidence: " << confidence << std::endl;
        
        // Phase 3: Decide fallback
        if (federated.merged_similar_adapters.size() >= min_results_ && 
            confidence >= confidence_threshold_) {
            std::cout << "✓ Summary-first results sufficient" << std::endl;
            return federated.merged_similar_adapters;
        }
        
        // Phase 4: Fallback to full query
        std::cout << "⚠ Falling back to full query..." << std::endl;
        return executeFullQuery(shard_ids, domain, base_model_id);
    }
    
private:
    float evaluateConfidence(
        const themis::tensor::FederatedTensorSummary& federated) {
        
        // Calculate confidence based on:
        // - Number of healthy shards
        // - Consistency of results
        // - Compression ratio used
        
        float healthy_count = 0;
        for (const auto& s : federated.shard_summaries) {
            if (s.shard_healthy) healthy_count++;
        }
        
        float shard_health_score = healthy_count / federated.shard_summaries.size();
        float result_diversity = 
            std::min(1.0f, federated.merged_similar_adapters.size() / 20.0f);
        
        // Simple averaging; can be improved with ML
        return (shard_health_score + result_diversity) / 2.0f;
    }
    
    std::vector<themis::tensor::SimilarityResult> executeFullQuery(
        const std::vector<std::string>& shard_ids,
        const std::string& domain,
        const std::string& base_model_id) {
        
        // Implementation: Query all shards without compression
        // For this example, return empty vector
        return std::vector<themis::tensor::SimilarityResult>();
    }
};
```

## Example 5: Monitoring and Observability

```cpp
#include "tensor/tensor_mid_layer.h"
#include <chrono>

class FederatedSummaryMetrics {
public:
    struct Metrics {
        float generation_time_ms;
        float merge_time_ms;
        float total_time_ms;
        float average_compression_ratio;
        std::size_t total_candidates_before;
        std::size_t total_candidates_after;
        std::size_t healthy_shards;
        std::size_t total_shards;
        float false_negative_risk;  // 0.0 = none, 1.0 = high
    };
    
    Metrics measureFederatedSummary(
        const themis::tensor::FederatedTensorSummary& federated) {
        
        Metrics m;
        m.total_shards = federated.shard_summaries.size();
        m.healthy_shards = 0;
        m.total_candidates_before = 0;
        m.total_candidates_after = 0;
        m.average_compression_ratio = 0.0f;
        m.false_negative_risk = 0.0f;
        
        for (const auto& shard : federated.shard_summaries) {
            if (shard.shard_healthy) m.healthy_shards++;
            m.total_candidates_before += shard.candidates_before_compression;
            m.total_candidates_after += shard.candidates_after_compression;
            
            // Calculate compression ratio for this shard
            if (shard.candidates_before_compression > 0) {
                float ratio = static_cast<float>(shard.candidates_before_compression) /
                             shard.candidates_after_compression;
                m.average_compression_ratio += ratio;
            }
        }
        
        m.average_compression_ratio /= m.total_shards;
        
        // Estimate false-negative risk based on compression
        // Higher compression = higher risk
        m.false_negative_risk = estimateFalseNegativeRisk(m.average_compression_ratio);
        
        return m;
    }
    
    void printMetrics(const Metrics& m) {
        std::cout << "=== Federated Summary Metrics ===" << std::endl;
        std::cout << "Shards: " << m.healthy_shards << "/" << m.total_shards << std::endl;
        std::cout << "Candidates before: " << m.total_candidates_before << std::endl;
        std::cout << "Candidates after: " << m.total_candidates_after << std::endl;
        std::cout << "Avg compression ratio: " << m.average_compression_ratio << "x" << std::endl;
        std::cout << "Estimated FN risk: " << (m.false_negative_risk * 100.0f) << "%" << std::endl;
    }
    
private:
    float estimateFalseNegativeRisk(float compression_ratio) {
        // Linear approximation: higher compression = higher risk
        // At 1x (no compression) = 0% risk
        // At 10x = ~10% risk
        // Can be refined with domain-specific data
        return std::min(1.0f, (compression_ratio - 1.0f) / 10.0f);
    }
};
```

## Example 6: Shard Scope Prefix Handling

```cpp
#include "tensor/tensor_mid_layer.h"

class ShardScopeRouter {
public:
    bool isSingleShardQuery(const std::string& scope_id) {
        // Single shard if scope_id starts with "shard:"
        return scope_id.rfind("shard:", 0) == 0;
    }
    
    bool isPackageQuery(const std::string& scope_id) {
        // Package query if scope_id starts with "pkg:" or "package:"
        return (scope_id.rfind("pkg:", 0) == 0 || 
                scope_id.rfind("package:", 0) == 0);
    }
    
    themis::tensor::TensorLayerKind classifyScope(
        const std::string& scope_id,
        bool shard_aware) {
        
        if (isSingleShardQuery(scope_id) || shard_aware) {
            return themis::tensor::TensorLayerKind::ShardSummary;
        }
        
        if (isPackageQuery(scope_id)) {
            return themis::tensor::TensorLayerKind::Package;
        }
        
        return themis::tensor::TensorLayerKind::Adapter;
    }
    
    void printRouting(const std::string& scope_id, bool shard_aware) {
        auto kind = classifyScope(scope_id, shard_aware);
        
        std::string kind_str;
        switch (kind) {
            case themis::tensor::TensorLayerKind::ShardSummary:
                kind_str = "ShardSummary (distributed)"; break;
            case themis::tensor::TensorLayerKind::Package:
                kind_str = "Package"; break;
            case themis::tensor::TensorLayerKind::Adapter:
                kind_str = "Adapter"; break;
            default:
                kind_str = "Unknown"; break;
        }
        
        std::cout << "Scope: " << scope_id << std::endl;
        std::cout << "Shard-aware: " << (shard_aware ? "yes" : "no") << std::endl;
        std::cout << "Routed to: " << kind_str << std::endl;
    }
};
```

## Example 7: Testing and Validation

```cpp
#include "tensor/tensor_mid_layer.h"
#include "tensor/tensor_summary_types.h"
#include <cassert>

class FederatedSummaryValidator {
public:
    void validateFederatedSummary(
        const themis::tensor::FederatedTensorSummary& federated,
        std::size_t expected_shards,
        std::size_t min_merged_results) {
        
        // Validation 1: Correct number of shards
        assert(federated.shard_summaries.size() == expected_shards);
        std::cout << "✓ Shard count correct" << std::endl;
        
        // Validation 2: Merged results exist
        assert(federated.merged_similar_adapters.size() >= min_merged_results);
        std::cout << "✓ Merged results present" << std::endl;
        
        // Validation 3: Results are sorted by score
        for (std::size_t i = 1; i < federated.merged_similar_adapters.size(); ++i) {
            assert(federated.merged_similar_adapters[i-1].score >= 
                   federated.merged_similar_adapters[i].score);
        }
        std::cout << "✓ Results sorted by score" << std::endl;
        
        // Validation 4: Routing reason is non-empty
        assert(!federated.routing_reason.empty());
        std::cout << "✓ Routing reason provided" << std::endl;
        
        // Validation 5: No duplicate adapters in results
        std::set<std::string> seen;
        for (const auto& adapter : federated.merged_similar_adapters) {
            assert(seen.find(adapter.adapter_key) == seen.end());
            seen.insert(adapter.adapter_key);
        }
        std::cout << "✓ No duplicate results" << std::endl;
        
        std::cout << "All validations passed!" << std::endl;
    }
};
```

## References

- `docs/FEDERATED_TENSOR_SUMMARIES.md`: Complete API reference and user guide
- `docs/adr/adr-e1-006-federated-tensor-summaries.md`: Architecture decision record
- `tests/tensor/test_federated_tensor_summaries.cpp`: Test suite with 15 test cases
- `include/tensor/tensor_mid_layer.h`: TensorMidLayer interface
- `include/tensor/tensor_summary_types.h`: Summary type definitions
