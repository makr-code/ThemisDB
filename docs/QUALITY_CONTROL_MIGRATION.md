# Migration Guide: Adding Quality Control to Existing RAG Systems

This guide helps you integrate the Quality Control System into an existing RAG implementation.

## Prerequisites

- Existing RAG pipeline with query, retrieval, and generation
- C++20 compiler
- CMake 3.23+
- ThemisDB with `THEMIS_ENABLE_LLM=ON`

## Migration Paths

### Path 1: Minimal Integration (15 minutes)

Add basic quality control without changing existing code.

#### Step 1: Include Headers
```cpp
#include "rag/quality_control_factory.h"
```

#### Step 2: Create Pipeline (once at startup)
```cpp
// In your initialization code
auto qc_pipeline = QualityControlFactory::createBasic();
```

#### Step 3: Add Quality Check (before returning answer)
```cpp
// Your existing RAG code
std::string query = get_user_query();
auto documents = retrieval_system->retrieve(query);
std::string answer = llm->generate(query, documents);

// NEW: Add quality check
auto qc_result = qc_pipeline->runQualityControl(query, documents, answer);

if (qc_result.decision == QCDecision::ACCEPT) {
    return answer;  // Quality passed
} else {
    // Handle quality failure
    logger->warn("Quality check failed", qc_result.explanation);
    return answer;  // Or implement retry/fallback
}
```

**Benefits**: Immediate quality visibility, no architecture changes.

---

### Path 2: Production Integration (1-2 hours)

Full integration with retry logic and continuous learning.

#### Step 1: Setup During Initialization

```cpp
class RAGSystem {
private:
    std::shared_ptr<RetrievalSystem> retrieval;
    std::shared_ptr<LLM> generator;
    std::shared_ptr<QualityControlPipeline> quality_control;  // NEW
    
public:
    RAGSystem() {
        // Existing initialization
        retrieval = std::make_shared<RetrievalSystem>(config);
        generator = std::make_shared<LLM>(model_config);
        
        // NEW: Setup quality control
        QualityControlFactory::SetupConfig qc_config;
        qc_config.nli_model_path = "/models/deberta-v3-large-mnli.onnx";
        qc_config.inference_engine = generator;  // Reuse existing LLM
        qc_config.enable_continuous_learning = true;
        qc_config.cl_endpoint = "http://localhost:8080/metrics";
        
        quality_control = QualityControlFactory::createProduction(qc_config);
    }
};
```

#### Step 2: Modify Generation Method

```cpp
std::string RAGSystem::answer_query(const std::string& query) {
    // Existing retrieval
    auto documents = retrieval->retrieve(query);
    
    // Existing generation with retry logic
    int max_attempts = 3;
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        // Generate answer
        std::string answer = generator->generate(query, documents);
        
        // NEW: Quality check
        auto qc_result = quality_control->runQualityControl(
            query, documents, answer
        );
        
        // Handle decision
        if (qc_result.decision == QCDecision::ACCEPT) {
            return answer;  // Quality passed
        }
        
        if (qc_result.decision == QCDecision::RETRY && attempt < max_attempts - 1) {
            // Adjust generation parameters based on issues
            adjust_parameters_for_retry(qc_result.issues);
            continue;  // Try again
        }
        
        if (qc_result.decision == QCDecision::REJECT) {
            // Log and use fallback
            logger->error("Answer quality rejected", qc_result.explanation);
            return generate_fallback_answer(query);
        }
        
        if (qc_result.decision == QCDecision::WARN) {
            // Log warning but accept
            logger->warn("Answer quality borderline", qc_result.explanation);
            return answer;
        }
    }
    
    // All retries exhausted
    logger->error("Max retries exhausted for query", query);
    return generate_fallback_answer(query);
}

private:
    void adjust_parameters_for_retry(const std::vector<std::string>& issues) {
        // Adjust based on detected issues
        for (const auto& issue : issues) {
            if (issue.find("faithfulness") != std::string::npos) {
                // Retrieve more documents
                retrieval->set_top_k(retrieval->get_top_k() + 2);
            } else if (issue.find("completeness") != std::string::npos) {
                // Increase response length
                generator->set_max_tokens(generator->get_max_tokens() + 100);
            }
        }
    }
```

#### Step 3: Add Continuous Learning Callbacks

```cpp
void RAGSystem::setup_continuous_learning() {
    auto cl_client = quality_control->get_cl_client();
    
    cl_client->setTriggerCallback([this](const OptimizationTrigger& trigger) {
        if (trigger.trigger_type == "low_faithfulness") {
            // Optimize retrieval system
            logger->info("Triggering retrieval optimization");
            retrieval->optimize_ranking_model();
        } 
        else if (trigger.trigger_type == "low_relevance") {
            // Optimize prompts
            logger->info("Triggering prompt optimization");
            prompt_optimizer->run();
        } 
        else if (trigger.trigger_type == "low_overall_quality") {
            // Trigger LoRA fine-tuning
            logger->info("Triggering LoRA fine-tuning");
            lora_trainer->start_training();
        }
    });
}
```

**Benefits**: Automatic quality improvement, retry logic, continuous learning.

---

### Path 3: Advanced Integration (2-4 hours)

Custom evaluation with domain-specific criteria.

#### Step 1: Create Custom Pipeline

```cpp
class CustomQualityPipeline {
private:
    std::shared_ptr<QualityControlPipeline> base_pipeline;
    
public:
    CustomQualityPipeline(const Config& config) {
        // Start with production pipeline
        base_pipeline = QualityControlFactory::createProduction(config);
        
        // Add custom callbacks
        setup_custom_evaluation();
    }
    
    QualityCheckResult evaluate(
        const std::string& query,
        const std::vector<std::string>& documents,
        const std::string& answer
    ) {
        // Run base quality check
        auto result = base_pipeline->runQualityControl(query, documents, answer);
        
        // Add domain-specific checks
        result = add_domain_specific_checks(result, query, answer);
        
        // Add business logic
        result = apply_business_rules(result);
        
        return result;
    }
    
private:
    QualityCheckResult add_domain_specific_checks(
        QualityCheckResult result,
        const std::string& query,
        const std::string& answer
    ) {
        // Example: Check for required keywords in medical domain
        if (is_medical_query(query)) {
            if (!contains_medical_disclaimer(answer)) {
                result.issues.push_back("Missing medical disclaimer");
                result.overall_score *= 0.8;  // Penalize
            }
        }
        
        // Example: Check for citations in legal domain
        if (is_legal_query(query)) {
            if (!has_citations(answer)) {
                result.issues.push_back("Missing legal citations");
                result.overall_score *= 0.7;  // Penalize more
            }
        }
        
        return result;
    }
    
    QualityCheckResult apply_business_rules(QualityCheckResult result) {
        // Example: Higher threshold for sensitive topics
        if (is_sensitive_topic(query)) {
            if (result.overall_score < 0.85) {  // Stricter threshold
                result.decision = QCDecision::REJECT;
                result.recommendations.push_back("Human review required for sensitive topic");
            }
        }
        
        return result;
    }
};
```

#### Step 2: Integrate Custom Pipeline

```cpp
class AdvancedRAGSystem {
private:
    std::shared_ptr<CustomQualityPipeline> quality_control;
    
public:
    std::string answer_query(const std::string& query) {
        auto documents = retrieval->retrieve(query);
        auto answer = generator->generate(query, documents);
        
        // Use custom quality pipeline
        auto qc_result = quality_control->evaluate(query, documents, answer);
        
        return handle_quality_result(qc_result, query, documents);
    }
};
```

**Benefits**: Domain-specific validation, custom business logic, maximum control.

---

## Common Migration Scenarios

### Scenario 1: API Service

**Existing**: RESTful API that returns RAG answers

```cpp
// Before
Response answer_endpoint(const Request& req) {
    auto answer = rag_system->generate(req.query);
    return Response{answer, 200};
}
```

```cpp
// After (with quality control)
Response answer_endpoint(const Request& req) {
    auto documents = retrieval->retrieve(req.query);
    auto answer = llm->generate(req.query, documents);
    
    auto qc_result = quality_control->runQualityControl(
        req.query, documents, answer
    );
    
    if (qc_result.decision == QCDecision::ACCEPT) {
        return Response{
            answer, 
            200, 
            {"X-Quality-Score", std::to_string(qc_result.overall_score)}
        };
    } else {
        // Return with quality warning
        return Response{
            answer,
            200,
            {"X-Quality-Warning", qc_result.explanation}
        };
    }
}
```

### Scenario 2: Batch Processing

**Existing**: Process documents in batch

```cpp
// Before
void process_batch(const std::vector<Item>& items) {
    for (const auto& item : items) {
        auto answer = rag_system->generate(item.query);
        save_result(item.id, answer);
    }
}
```

```cpp
// After (with quality control and filtering)
void process_batch(const std::vector<Item>& items) {
    auto pipeline = QualityControlFactory::createComprehensive();
    
    std::vector<Item> passed_items;
    std::vector<Item> failed_items;
    
    for (const auto& item : items) {
        auto documents = retrieval->retrieve(item.query);
        auto answer = llm->generate(item.query, documents);
        
        auto qc_result = pipeline->runQualityControl(
            item.query, documents, answer
        );
        
        if (qc_result.decision == QCDecision::ACCEPT) {
            save_result(item.id, answer, qc_result.overall_score);
            passed_items.push_back(item);
        } else {
            log_failure(item.id, qc_result.explanation);
            failed_items.push_back(item);
        }
    }
    
    // Retry failed items with different parameters
    retry_failed_items(failed_items);
}
```

### Scenario 3: Streaming Responses

**Existing**: Stream tokens as they're generated

```cpp
// Before
void stream_answer(const std::string& query, StreamCallback callback) {
    retrieval->retrieve(query);
    llm->stream_generate(query, callback);
}
```

```cpp
// After (buffer and check quality)
void stream_answer(const std::string& query, StreamCallback callback) {
    auto documents = retrieval->retrieve(query);
    
    // Buffer the complete response
    std::string full_answer;
    llm->stream_generate(query, [&](const std::string& token) {
        full_answer += token;
    });
    
    // Quality check complete answer
    auto qc_result = quality_control->runQualityControl(
        query, documents, full_answer
    );
    
    if (qc_result.decision == QCDecision::ACCEPT) {
        // Stream to client
        for (const auto& token : tokenize(full_answer)) {
            callback(token);
        }
    } else {
        // Regenerate or send error
        callback("[Quality check failed. Regenerating...]");
        auto new_answer = regenerate_with_adjustments(query, documents);
        for (const auto& token : tokenize(new_answer)) {
            callback(token);
        }
    }
}
```

## Configuration Migration

### From Simple Thresholds

```cpp
// Before: Manual threshold checking
if (similarity_score < 0.7) {
    return "No good answer found";
}
```

```cpp
// After: Comprehensive quality control
QualityControlPipeline::Config config;
config.faithfulness_threshold = 0.75;
config.relevance_threshold = 0.70;
config.completeness_threshold = 0.70;
config.coherence_threshold = 0.65;
// Multi-dimensional assessment
```

### From Custom Evaluation

```cpp
// Before: Custom evaluation function
bool is_answer_good(const std::string& answer) {
    return answer.length() > 50 && 
           has_keywords(answer) && 
           !is_generic(answer);
}
```

```cpp
// After: Structured evaluation
auto qc_result = quality_control->runQualityControl(query, docs, answer);
// Returns structured result with scores, explanation, recommendations
```

## Testing Migration

Add tests to verify quality control integration:

```cpp
TEST(RAGSystemTest, QualityControlIntegration) {
    auto rag = std::make_shared<RAGSystem>();
    
    // Test high-quality answer
    auto result1 = rag->answer_query("What is the capital of France?");
    EXPECT_TRUE(result1.quality_passed);
    
    // Test low-quality answer (should retry)
    auto result2 = rag->answer_query("Complex ambiguous query");
    EXPECT_TRUE(result2.was_retried);
    
    // Test rejection case
    auto result3 = rag->answer_query("Impossible query");
    EXPECT_TRUE(result3.used_fallback);
}
```

## Monitoring Migration

Add metrics to track quality:

```cpp
// Track quality metrics
void log_quality_metrics(const QualityCheckResult& result) {
    metrics->gauge("rag.quality.overall", result.overall_score);
    metrics->gauge("rag.quality.faithfulness", result.faithfulness_score);
    metrics->gauge("rag.quality.relevance", result.relevance_score);
    metrics->counter("rag.quality.decision." + to_string(result.decision));
    metrics->histogram("rag.quality.latency", result.latency_ms);
}
```

## Rollout Strategy

### Phase 1: Shadow Mode (Week 1)
- Run quality control but don't act on results
- Log all decisions for analysis
- Tune thresholds based on data

### Phase 2: Warning Mode (Week 2-3)
- Log warnings for low-quality answers
- Continue serving all answers
- Monitor false positive rate

### Phase 3: Retry Mode (Week 4)
- Enable retry for RETRY decisions
- Still serve all answers (after retry)
- Monitor improvement rate

### Phase 4: Full Enforcement (Week 5+)
- Enable all decisions (including REJECT)
- Use fallback for rejected answers
- Monitor user satisfaction

## Rollback Plan

Keep feature flag for easy rollback:

```cpp
class RAGSystem {
    bool quality_control_enabled = true;  // Feature flag
    
    std::string answer_query(const std::string& query) {
        auto answer = generate_answer(query, documents);
        
        if (!quality_control_enabled) {
            return answer;  // Skip QC
        }
        
        // Run quality control
        auto qc_result = quality_control->runQualityControl(...);
        return handle_qc_result(qc_result, answer);
    }
};
```

## Getting Help

- Documentation: `docs/QUALITY_CONTROL_SYSTEM.md`
- Quick Reference: `docs/QUALITY_CONTROL_QUICK_REF.md`
- Examples: `examples/quality_control_demo.cpp`
- Tests: `tests/test_quality_control_pipeline.cpp`

## Summary

1. **Start Simple**: Use `createBasic()` for immediate integration
2. **Add Retry**: Implement retry logic for better quality
3. **Enable CL**: Turn on continuous learning for automatic improvement
4. **Customize**: Add domain-specific checks as needed
5. **Monitor**: Track metrics and adjust thresholds
6. **Iterate**: Gradually increase enforcement based on data
