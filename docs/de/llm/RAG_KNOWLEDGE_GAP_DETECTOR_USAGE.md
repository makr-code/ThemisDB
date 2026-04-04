# Knowledge Gap Detector - Usage Examples

This document provides practical examples for using the Knowledge Gap Detector in ThemisDB's RAG pipeline.

## Table of Contents
- [Basic Usage](#basic-usage)
- [Integration with VectorIndexManager](#integration-with-vectorindexmanager)
- [Configuration](#configuration)
- [Advanced Usage](#advanced-usage)
- [Performance Optimization](#performance-optimization)

## Basic Usage

### Simple Gap Detection

```cpp
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

// Create detector with default configuration
auto detector = KnowledgeGapDetectorFactory::createBalanced();

// Prepare documents (from your retrieval system)
std::vector<RetrievedDocument> documents;

RetrievedDocument doc1;
doc1.id = "doc1";
doc1.content = "Machine learning is a subset of artificial intelligence.";
doc1.similarity_score = 0.85;
documents.push_back(doc1);

RetrievedDocument doc2;
doc2.id = "doc2";
doc2.content = "Deep learning uses neural networks with multiple layers.";
doc2.similarity_score = 0.80;
documents.push_back(doc2);

// Check for gaps before generation
auto result = detector->detectPreGeneration(
    "What is machine learning?",
    documents
);

if (result.gap_detected) {
    std::cout << "Gap detected: " << result.explanation << std::endl;
    std::cout << "Recommendation: " << static_cast<int>(result.recommendation) << std::endl;
} else {
    std::cout << "Documents sufficient for generation" << std::endl;
}
```

## Integration with VectorIndexManager

### Complete RAG Pipeline

```cpp
#include "rag/knowledge_gap_detector.h"
#include "rag/rag_integration_helpers.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis;
using namespace themis::rag;
using namespace themis::rag::knowledge_gap;

// Initialize database and vector index
RocksDBWrapper::Config db_config;
db_config.db_path = "/path/to/db";
RocksDBWrapper db(db_config);
db.open();

VectorIndexManager vector_mgr(db);
vector_mgr.init("documents", 768, VectorIndexManager::Metric::COSINE);

// Initialize gap detector
auto gap_detector = KnowledgeGapDetectorFactory::createBalanced();

// User query
std::string query = "Explain machine learning algorithms";

// 1. Get query embedding (from your embedding model)
std::vector<float> query_embedding = getEmbedding(query); // Your embedding function

// 2. Search for similar documents
auto [status, search_results] = vector_mgr.searchKnn(query_embedding, 10);

if (!status.ok) {
    std::cerr << "Search failed: " << status.message << std::endl;
    return;
}

// 3. Convert to RetrievedDocuments format
auto documents = convertToRetrievedDocuments(
    search_results,
    db,
    VectorIndexManager::Metric::COSINE
);

// 4. Check for knowledge gaps
auto gap_result = gap_detector->detectPreGeneration(query, documents);

// 5. Handle result
if (gap_result.gap_detected) {
    switch (gap_result.recommendation) {
        case FallbackStrategy::EXPAND_SEARCH:
            // Retry with more documents
            std::tie(status, search_results) = vector_mgr.searchKnn(query_embedding, 20);
            documents = convertToRetrievedDocuments(search_results, db, 
                                                   VectorIndexManager::Metric::COSINE);
            break;
            
        case FallbackStrategy::REFORMULATE_QUERY:
            // Try alternative query formulation
            query = reformulateQuery(query); // Your reformulation logic
            break;
            
        case FallbackStrategy::INSUFFICIENT_DATA_RESPONSE:
            // Return explicit message
            std::cout << "Insufficient information to answer reliably." << std::endl;
            if (!gap_result.missing_aspects.empty()) {
                std::cout << "Missing aspects: ";
                for (const auto& aspect : gap_result.missing_aspects) {
                    std::cout << aspect << " ";
                }
                std::cout << std::endl;
            }
            return;
    }
}

// 6. Proceed with LLM generation (documents are sufficient)
std::string answer = generateAnswer(query, documents); // Your LLM generation
std::cout << "Answer: " << answer << std::endl;
```

## Configuration

### Creating Custom Detectors

```cpp
// Fast detector - for high-throughput production
auto fast_detector = KnowledgeGapDetectorFactory::createFast();
// - Only pre-generation checks
// - ~10ms latency
// - Suitable for real-time APIs

// Balanced detector - recommended default
auto balanced_detector = KnowledgeGapDetectorFactory::createBalanced();
// - Pre-generation + during-generation checks
// - ~100ms latency
// - Good balance of accuracy and speed

// Thorough detector - for critical applications
auto thorough_detector = KnowledgeGapDetectorFactory::createThorough();
// - All detection levels
// - ~500ms+ latency
// - Maximum accuracy
```

### Custom Configuration

```cpp
KnowledgeGapConfig config;

// Adjust thresholds
config.similarity_threshold = 0.80;  // Higher threshold = more strict
config.min_documents = 5;            // Require more documents
config.confidence_threshold = 0.75;  // LLM confidence threshold
config.coverage_threshold = 0.85;    // Query coverage threshold

// Enable/disable features
config.enable_query_aspect_analysis = true;
config.enable_self_consistency_check = true;  // Phase 2
config.enable_claim_verification = true;      // Phase 2

// Set detection mode
config.mode = DetectionMode::BALANCED;

// Create detector with custom config
auto detector = std::make_unique<KnowledgeGapDetector>(config);

// Update config at runtime
KnowledgeGapConfig new_config = detector->getConfig();
new_config.similarity_threshold = 0.70;
detector->setConfig(new_config);
```

## Advanced Usage

### Multi-Level Detection

```cpp
// Pre-generation check
auto pre_result = detector->detectPreGeneration(query, documents);

if (pre_result.gap_detected) {
    // Handle pre-generation gap
    return;
}

// During generation (if available)
GenerationContext context;
context.generation_started = true;
context.token_probability_avg = 0.75;  // From LLM
context.perplexity = 45.0;              // From LLM

auto during_result = detector->detectDuringGeneration(query, documents, context);

if (during_result.gap_detected) {
    // Handle during-generation gap
    return;
}

// After generation
std::string answer = llm->generate(query, documents);

auto post_result = detector->detectPostGeneration(query, documents, answer);

if (post_result.gap_detected) {
    // Handle post-generation gap (e.g., unverified claims)
    std::cout << "Warning: Answer contains unverified claims" << std::endl;
}
```

### Gap Detection Callback

```cpp
// Set up callback for monitoring
detector->setGapDetectionCallback([](const DetectionResult& result) {
    // Log to monitoring system
    if (result.gap_detected) {
        logMetric("gap_detected", 1.0);
        logMetric("gap_type", static_cast<int>(result.gap_type));
        logMetric("confidence_score", result.confidence_score);
    }
});

// Callback is invoked automatically during detection
auto result = detector->detectGap(query, documents, answer);
```

### Handling Different Gap Types

```cpp
auto result = detector->detectPreGeneration(query, documents);

if (result.gap_detected) {
    switch (result.gap_type) {
        case GapType::LOW_SIMILARITY:
            std::cout << "Documents not semantically similar to query" << std::endl;
            std::cout << "Avg similarity: " << result.avg_similarity_score << std::endl;
            break;
            
        case GapType::INSUFFICIENT_DOCS:
            std::cout << "Not enough documents retrieved" << std::endl;
            std::cout << "Found: " << result.num_retrieved_docs << std::endl;
            break;
            
        case GapType::MISSING_ASPECTS:
            std::cout << "Query aspects not covered by documents" << std::endl;
            std::cout << "Coverage: " << result.coverage_score << std::endl;
            for (const auto& aspect : result.missing_aspects) {
                std::cout << "  - Missing: " << aspect << std::endl;
            }
            break;
            
        case GapType::OUTDATED_INFO:
            std::cout << "Documents contain outdated information" << std::endl;
            break;
            
        case GapType::UNCERTAIN_GENERATION:
            std::cout << "LLM indicates low confidence" << std::endl;
            break;
            
        case GapType::CONFLICTING_INFO:
            std::cout << "Documents contain contradictory information" << std::endl;
            break;
    }
}
```

## Performance Optimization

### Caching Detection Results

```cpp
// The detector internally caches some results
// For external caching, use query fingerprinting:

#include <functional>
#include <unordered_map>

std::unordered_map<size_t, DetectionResult> result_cache;

size_t query_hash = std::hash<std::string>{}(query);

auto cached = result_cache.find(query_hash);
if (cached != result_cache.end()) {
    // Use cached result
    return cached->second;
}

// Perform detection
auto result = detector->detectPreGeneration(query, documents);

// Cache for future use
result_cache[query_hash] = result;
```

### Batch Processing

```cpp
#include "rag/rag_integration_helpers.h"

std::vector<std::string> queries = {
    "What is machine learning?",
    "Explain neural networks",
    "What is deep learning?"
};

// Batch convert documents
auto all_documents = batchConvertToRetrievedDocuments(
    queries,
    vector_mgr,
    db,
    10  // k results per query
);

// Process each query
std::vector<DetectionResult> results;
for (size_t i = 0; i < queries.size(); ++i) {
    auto result = detector->detectPreGeneration(queries[i], all_documents[i]);
    results.push_back(result);
}
```

### Async Processing (Future Enhancement)

```cpp
// Future: Async gap detection for non-blocking pipelines
#include <future>

auto future_result = std::async(std::launch::async, [&]() {
    return detector->detectGap(query, documents, answer);
});

// Do other work...

// Get result when needed
auto result = future_result.get();
```

## Metrics and Monitoring

```cpp
// Extract metrics for monitoring
auto result = detector->detectPreGeneration(query, documents);

// Export to Prometheus/Grafana
exportMetric("rag.gap_detected", result.gap_detected ? 1.0 : 0.0);
exportMetric("rag.similarity_avg", result.avg_similarity_score);
exportMetric("rag.coverage_score", result.coverage_score);
exportMetric("rag.num_docs", static_cast<double>(result.num_retrieved_docs));
exportMetric("rag.confidence", result.confidence_score);

if (result.gap_detected) {
    exportMetric("rag.gap_type", static_cast<double>(result.gap_type));
    exportLabel("gap_explanation", result.explanation);
}
```

## Error Handling

```cpp
try {
    auto result = detector->detectPreGeneration(query, documents);
    
    // Check confidence score
    if (result.confidence_score < 0.5) {
        // Low confidence in detection itself
        std::cerr << "Warning: Low confidence in gap detection" << std::endl;
    }
    
} catch (const std::exception& e) {
    std::cerr << "Error during gap detection: " << e.what() << std::endl;
    // Fall back to proceeding without gap detection
}
```

## Testing Your Integration

```cpp
#include <gtest/gtest.h>

TEST(RAGPipeline, KnowledgeGapDetection) {
    // Set up test environment
    auto detector = KnowledgeGapDetectorFactory::createFast();
    
    // Test with insufficient documents
    std::vector<RetrievedDocument> few_docs;
    RetrievedDocument doc;
    doc.id = "test1";
    doc.content = "Test content";
    doc.similarity_score = 0.9;
    few_docs.push_back(doc);
    
    auto result = detector->detectPreGeneration("test query", few_docs);
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::INSUFFICIENT_DOCS);
    
    // Test with sufficient high-quality documents
    std::vector<RetrievedDocument> good_docs;
    for (int i = 0; i < 5; ++i) {
        RetrievedDocument d;
        d.id = "doc" + std::to_string(i);
        d.content = "High quality relevant content";
        d.similarity_score = 0.9;
        good_docs.push_back(d);
    }
    
    result = detector->detectPreGeneration("test query", good_docs);
    EXPECT_FALSE(result.gap_detected);
}
```

## Phase 2: Advanced LLM-Based Features

### Token Probability Tracking

Phase 2 adds real-time token probability tracking with perplexity calculation for fine-grained confidence monitoring during generation.

```cpp
#include "rag/knowledge_gap_detector.h"

// Enable Phase 2 features
KnowledgeGapConfig config;
config.mode = DetectionMode::BALANCED;
config.enable_token_probability = true;
config.perplexity_threshold = 100.0;
config.perplexity_window_size = 10;
config.outlier_zscore_threshold = 3.0;

auto detector = std::make_unique<KnowledgeGapDetector>(config);

// During generation, collect token probabilities
GenerationContext gen_context;
gen_context.token_probs = {0.9, 0.85, 0.88, 0.92, 0.87}; // From LLM
gen_context.generation_started = true;

// Detect gaps during generation
auto result = detector->detectDuringGeneration(
    "What is machine learning?",
    documents,
    gen_context
);

if (result.gap_detected) {
    std::cout << "Low confidence detected during generation" << std::endl;
    std::cout << "Explanation: " << result.explanation << std::endl;
}
```

**Features:**
- **Perplexity calculation**: Exponential of negative average log probability
- **Sliding window analysis**: Detects local regions of high uncertainty
- **Outlier removal**: Filters anomalous tokens using z-score (threshold: 3.0)
- **Anomaly detection**: Triggers when perplexity > threshold (default: 100)

### Self-Consistency Check

Multi-sampling approach to verify answer consistency across different generation runs.

```cpp
// Enable self-consistency checking
KnowledgeGapConfig config;
config.enable_self_consistency_check = true;
config.self_consistency_samples = 5;
config.temperature_range = {0.7, 0.8, 0.9};
config.consistency_threshold = 0.6;
config.consistency_timeout_ms = 10000; // 10 seconds max

auto detector = std::make_unique<KnowledgeGapDetector>(config);

// After generation
auto result = detector->detectPostGeneration(
    "What is machine learning?",
    documents,
    "Machine learning is a subset of AI that enables systems to learn..."
);

if (result.gap_detected && result.gap_type == GapType::CONFLICTING_INFO) {
    std::cout << "Inconsistent answers detected!" << std::endl;
    std::cout << "Confidence: " << result.confidence_score << std::endl;
}
```

**Features:**
- **Multiple sampling**: Generates 3-5 answers with different seeds/temperatures
- **Semantic similarity**: Measures consistency using Jaccard similarity
- **Contradiction detection**: Identifies conflicting statements with negation analysis
- **Configurable threshold**: Adjustable consistency requirements (0.0-1.0)

### FLARE-Style Active Retrieval

Forward-looking active retrieval that iteratively enhances document set based on generation confidence.

```cpp
// Enable FLARE
KnowledgeGapConfig config;
config.enable_flare = true;
config.max_retrieval_rounds = 3;
config.flare_confidence_threshold = 0.5;

auto detector = std::make_unique<KnowledgeGapDetector>(config);

// Initial documents (may be insufficient)
std::vector<RetrievedDocument> documents = getInitialDocuments(query);

// FLARE will iteratively enhance the document set
auto result = detector->detectWithActiveRetrieval(query, documents);

if (result.gap_detected) {
    std::cout << "Gap persists after " << config.max_retrieval_rounds 
              << " retrieval rounds" << std::endl;
    std::cout << "Final coverage: " << result.coverage_score << std::endl;
} else {
    std::cout << "Sufficient information after active retrieval" << std::endl;
    std::cout << "Retrieved " << documents.size() << " documents total" << std::endl;
    // documents vector now contains enhanced document set
}
```

**Features:**
- **Sentence-by-sentence generation**: Monitors confidence at fine granularity
- **Dynamic re-retrieval**: Automatically fetches more documents when confidence drops
- **Query reformulation**: Creates new queries based on missing aspects
- **Document deduplication**: Prevents duplicate documents in enhanced set
- **Max rounds limit**: Prevents infinite retrieval loops (default: 3)

### Factory Presets with Phase 2

```cpp
// Fast mode: Phase 2 features disabled for minimum latency
auto fast_detector = KnowledgeGapDetectorFactory::createFast();
// - enable_token_probability: false
// - enable_self_consistency_check: false
// - enable_flare: false
// Expected latency: ~10ms

// Balanced mode: Token probability enabled
auto balanced_detector = KnowledgeGapDetectorFactory::createBalanced();
// - enable_token_probability: true
// - enable_self_consistency_check: false
// - enable_flare: false
// Expected latency: ~100ms

// Thorough mode: All Phase 2 features enabled
auto thorough_detector = KnowledgeGapDetectorFactory::createThorough();
// - enable_token_probability: true
// - enable_self_consistency_check: true
// - enable_claim_verification: true
// - enable_flare: false (VectorIndexManager integration required)
// Expected latency: ~500ms+
```

### Complete Phase 2 Configuration

```cpp
KnowledgeGapConfig config;

// Detection mode
config.mode = DetectionMode::THOROUGH;

// Basic thresholds (Phase 1)
config.similarity_threshold = 0.75;
config.min_documents = 3;
config.confidence_threshold = 0.7;
config.coverage_threshold = 0.8;

// Phase 2: Token Probability Tracking
config.enable_token_probability = true;
config.perplexity_threshold = 100.0;        // Anomaly detection threshold
config.perplexity_window_size = 10;         // Sliding window size
config.outlier_zscore_threshold = 3.0;      // Outlier detection sensitivity

// Phase 2: Self-Consistency Check
config.enable_self_consistency_check = true;
config.self_consistency_samples = 5;        // Number of samples to generate
config.temperature_range = {0.7, 0.8, 0.9}; // Temperature variations
config.consistency_threshold = 0.6;         // Minimum consistency score
config.consistency_timeout_ms = 10000;      // Max 10s per sample

// Phase 2: FLARE Active Retrieval
config.enable_flare = false;                // Requires VectorIndexManager integration
config.max_retrieval_rounds = 3;            // Max re-retrieval iterations
config.flare_confidence_threshold = 0.5;    // Trigger re-retrieval below this

// Other features
config.enable_claim_verification = true;
config.enable_query_aspect_analysis = true;

auto detector = std::make_unique<KnowledgeGapDetector>(config);
```

### Performance Guidelines

**Phase 2 Performance Targets:**
- Token probability tracking: < 10ms overhead ✅
- Self-consistency check: < 2s for 5 samples
- FLARE re-retrieval: < 500ms per round
- Total overhead: < 3s for complex queries

**Optimization Tips:**
1. Disable self-consistency for real-time applications
2. Reduce self_consistency_samples to 3 for faster checking
3. Increase consistency_threshold for stricter validation
4. Limit max_retrieval_rounds to prevent latency spikes
5. Use Fast or Balanced mode for most applications

### Integration with LLM Inference

To fully leverage Phase 2 features, integrate with your LLM inference engine:

```cpp
// Example: Collect token probabilities during generation
InferenceRequest request;
request.prompt = formatted_prompt;
request.max_tokens = 512;

// Enable logprobs collection
auto response = llm_wrapper->generate(request);

// Build GenerationContext from response
GenerationContext context;
context.token_probs = response.logprobs;  // Per-token probabilities
context.generation_started = true;

// Calculate perplexity from token probs
context.perplexity = calculatePerplexity(context.token_probs);
context.token_probability_avg = 
    std::accumulate(context.token_probs.begin(), 
                   context.token_probs.end(), 0.0) / context.token_probs.size();

// Detect gaps during generation
auto result = detector->detectDuringGeneration(query, documents, context);
```

## See Also

- [RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md](RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md) - Scientific background
- [RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md](RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md) - Implementation roadmap
- [RAG_IMPLEMENTATION_GUIDE.md](RAG_IMPLEMENTATION_GUIDE.md) - General RAG guide
- API Reference: `include/rag/knowledge_gap_detector.h`

---

*Created: 2026-01-18*  
*Version: 1.0*  
*Status: Phase 1 Complete*
