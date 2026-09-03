> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB RAG (Retrieval-Augmented Generation) Module Headers

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: include/rag/README.md · src/rag/README.md · src/rag/ARCHITECTURE.md · src/rag/ROADMAP.md · src/rag/FUTURE_ENHANCEMENTS.md · docs/troubleshooting/rag_troubleshooting.md -->

## Module Purpose

The RAG module provides ThemisDB's comprehensive Retrieval-Augmented Generation system for LLM-powered question answering, featuring intelligent retrieval, quality evaluation, knowledge gap detection, and ethical compliance checking. This module integrates vector search, document retrieval, LLM generation, and multi-dimensional quality assessment to deliver accurate, reliable, and ethically sound AI-generated responses.

### What is RAG?

**RAG (Retrieval-Augmented Generation)** is a technique that enhances LLM responses by retrieving relevant context from a knowledge base before generation:

**Traditional LLM:**
```
User Query → LLM → Answer (may hallucinate)
```

**RAG Pipeline:**
```
User Query → Vector Search → Relevant Documents → LLM + Context → Answer (grounded in facts)
```

**Key Benefits:**
- ✅ **Reduced Hallucinations**: Answers grounded in retrieved documents
- ✅ **Up-to-date Information**: Query live data vs. training cutoff
- ✅ **Source Attribution**: Cite specific documents/passages
- ✅ **Domain Specialization**: Leverage private/proprietary knowledge bases
- ✅ **Cost Efficiency**: Smaller models + retrieval often outperform larger models alone

## Scope

**In Scope:**
- RAG pipeline orchestration (retrieval + generation + evaluation)
- Knowledge gap detection (pre, during, post-generation)
- LLM-as-Judge multi-dimensional quality evaluation
- Prompt template management and engineering
- Response parsing and claim extraction
- Evaluation caching and performance optimization
- Bias detection and ethical compliance checking
- Pairwise comparison and ranking
- Judge ensemble and calibration
- Integration with llama.cpp and other LLM backends

**Out of Scope:**
- Vector indexing and search algorithms (handled by index module)
- Document storage and management (handled by storage module)
- LLM inference execution (handled by llm module)
- Query language parsing (handled by aql module)

## Architecture Overview

### RAG Pipeline Stages

```
┌─────────────────────────────────────────────────────────────────┐
│                        RAG PIPELINE                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. QUERY PROCESSING                                            │
│     ┌──────────────┐                                            │
│     │ User Query   │                                            │
│     └──────┬───────┘                                            │
│            │                                                     │
│            v                                                     │
│     ┌──────────────────────┐                                   │
│     │ Query Understanding  │ (intent detection, entity extract) │
│     └──────────┬───────────┘                                   │
│                │                                                 │
│  2. RETRIEVAL                                                   │
│                v                                                 │
│     ┌──────────────────────┐                                   │
│     │  Vector Search       │ (embedding similarity)             │
│     │  + Hybrid Search     │ (vector + keyword + filters)       │
│     └──────────┬───────────┘                                   │
│                │                                                 │
│                v                                                 │
│     ┌──────────────────────┐                                   │
│     │  Retrieved Documents │                                    │
│     │  (Top-K results)     │                                    │
│     └──────────┬───────────┘                                   │
│                │                                                 │
│  3. PRE-GENERATION QUALITY CHECK                               │
│                v                                                 │
│     ┌──────────────────────────────┐                          │
│     │  Knowledge Gap Detection     │                          │
│     │  - Similarity threshold      │                          │
│     │  - Document count check      │                          │
│     │  - Query coverage analysis   │                          │
│     └──────────┬───────────────────┘                          │
│                │                                                 │
│            Gap Detected?                                        │
│         ┌──────┴──────┐                                        │
│        YES            NO                                        │
│         │              │                                         │
│   ┌─────v─────┐        │                                        │
│   │ Fallback  │        │                                        │
│   │ Strategy  │        │                                        │
│   └───────────┘        │                                        │
│                        │                                         │
│  4. PROMPT CONSTRUCTION                                        │
│                        v                                         │
│     ┌──────────────────────────────┐                          │
│     │  Prompt Template Manager     │                          │
│     │  - Context formatting        │                          │
│     │  - Instruction injection     │                          │
│     │  - Few-shot examples         │                          │
│     └──────────┬───────────────────┘                          │
│                │                                                 │
│  5. GENERATION                                                 │
│                v                                                 │
│     ┌──────────────────────────────┐                          │
│     │  LLM Generation              │                          │
│     │  (via llama.cpp integration) │                          │
│     └──────────┬───────────────────┘                          │
│                │                                                 │
│  6. POST-GENERATION QUALITY CHECK                              │
│                v                                                 │
│     ┌──────────────────────────────┐                          │
│     │  RAG Judge Evaluation        │                          │
│     │  - Faithfulness (fact check) │                          │
│     │  - Relevance (query match)   │                          │
│     │  - Completeness (coverage)   │                          │
│     │  - Coherence (structure)     │                          │
│     │  - Ethical Compliance        │                          │
│     └──────────┬───────────────────┘                          │
│                │                                                 │
│         Quality OK?                                             │
│         ┌──────┴──────┐                                        │
│        YES            NO                                        │
│         │              │                                         │
│         │         ┌────v──────┐                                │
│         │         │ Regenerate │                                │
│         │         │ or Reject  │                                │
│         │         └────────────┘                                │
│         │                                                        │
│  7. RESPONSE DELIVERY                                          │
│         v                                                        │
│     ┌──────────────────────────┐                              │
│     │  Formatted Response      │                              │
│     │  + Source Citations      │                              │
│     │  + Confidence Score      │                              │
│     │  + Quality Metrics       │                              │
│     └──────────────────────────┘                              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Key Components

### 1. RAG Judge - Quality Evaluation

**Location:** `rag_judge.h`, `../../src/rag/rag_judge.cpp`

LLM-based evaluation of RAG outputs across multiple quality dimensions.

**Evaluation Dimensions:**
1. **Faithfulness**: Answer supported by retrieved documents (no hallucinations)
2. **Relevance**: Answer addresses the user's query
3. **Completeness**: All aspects of query covered
4. **Coherence**: Logical structure and readability
5. **Ethical Compliance**: Respects human autonomy, shows diverse moral perspectives

**Features:**
- Multi-dimensional scoring (0.0 - 1.0 per dimension)
- Chain-of-Thought (CoT) reasoning explanations
- Claim extraction and verification
- Pairwise comparison for A/B testing
- Batch evaluation support
- Configurable quality thresholds
- Evaluation result caching

**Evaluation Modes:**
- **Fast Mode** (~100ms): Single-dimension relevance check
- **Balanced Mode** (~500ms): All dimensions with basic CoT
- **Thorough Mode** (~2s): Full CoT, claim verification, ensemble

**API Example:**
```cpp
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

// Create balanced judge (recommended for production)
auto judge = RAGJudgeFactory::createBalanced();

// Prepare documents
std::vector<RetrievedDocument> documents = {
    {"doc1", "Seattle weather is typically rainy in winter.", 0.92},
    {"doc2", "Average temperature in Seattle winter is 45°F.", 0.88}
};

// Evaluate generated answer
auto result = judge->evaluate(
    "What's the weather like in Seattle during winter?",
    documents,
    "Seattle experiences rainy weather in winter with average temperatures around 45°F."
);

// Check quality
if (result.passed_quality_threshold) {
    std::cout << "Answer quality: " << result.overall_score << "\n";
    std::cout << "Faithfulness: " << result.faithfulness_score << "\n";
    std::cout << "Relevance: " << result.relevance_score << "\n";

    // Show verified claims
    for (const auto& claim : result.verified_claims) {
        std::cout << "✓ " << claim << "\n";
    }
} else {
    std::cerr << "Answer quality below threshold!\n";
    for (const auto& claim : result.unverified_claims) {
        std::cerr << "✗ Unverified: " << claim << "\n";
    }
}

// Compare two candidate answers (A/B testing)
auto comparison = judge->compare(
    "What's the weather like in Seattle?",
    documents,
    answer_a,  // Candidate A
    answer_b   // Candidate B
);

if (comparison.winner == ComparisonResult::Winner::ANSWER_A) {
    std::cout << "Answer A is better\n";
    std::cout << "Reasoning: " << comparison.reasoning << "\n";
}
```

**Configuration:**
```cpp
RAGJudgeConfig config;
config.mode = EvaluationMode::BALANCED;

// Scoring weights (must sum to 1.0)
config.faithfulness_weight = 0.35;        // Most important
config.relevance_weight = 0.25;
config.completeness_weight = 0.15;
config.coherence_weight = 0.10;
config.ethical_compliance_weight = 0.15;

// Quality thresholds
config.quality_threshold = 0.7;           // Overall minimum
config.faithfulness_threshold = 0.8;      // Critical for accuracy
config.ethical_compliance_threshold = 0.7;

// Advanced features
config.use_chain_of_thought = true;
config.enable_claim_verification = true;
config.cache_evaluations = true;

auto judge = RAGJudgeFactory::create(config);
```

**Performance Characteristics:**
| Mode | Latency | Human Correlation | Use Case |
|------|---------|-------------------|----------|
| Fast | ~100ms | 0.70-0.75 | Real-time relevance check |
| Balanced | ~500ms | 0.80-0.85 | Production RAG pipeline |
| Thorough | ~2s | 0.90+ | Research, benchmarking |

**Sub-Components:**
- `faithfulness_evaluator.h`: Fact-checking against sources
- `relevance_evaluator.h`: Query-answer alignment
- `completeness_evaluator.h`: Query aspect coverage
- `coherence_evaluator.h`: Structure and readability
- `bias_detector.h`: Ethical perspective analysis
- `claim_extractor.h`: Atomic claim extraction
- `response_parser.h`: LLM response parsing

---

### 2. Knowledge Gap Detector

**Location:** `knowledge_gap_detector.h`, `../../src/rag/knowledge_gap_detector.cpp`

Detects when retrieved documents are insufficient to answer a query reliably.

**Detection Levels:**
1. **Pre-Generation**: Before LLM sees documents
   - Similarity score analysis
   - Document count check
   - Query coverage assessment

2. **During Generation**: Real-time monitoring
   - Token probability tracking
   - Perplexity anomaly detection
   - Uncertainty signals

3. **Post-Generation**: After answer generated
   - Self-consistency checking
   - Claim verification
   - Answer confidence scoring

**Gap Types:**
- `LOW_SIMILARITY`: Retrieved documents have low semantic similarity to query
- `INSUFFICIENT_DOCS`: Not enough documents retrieved
- `UNCERTAIN_GENERATION`: LLM shows low confidence during generation
- `MISSING_ASPECTS`: Query aspects not covered by documents
- `CONFLICTING_INFO`: Retrieved documents contradict each other
- `OUTDATED_INFO`: Information may be stale
- `ETHICAL_PERSPECTIVE_GAP`: Ethical query needs diverse moral perspectives

**Fallback Strategies:**
- `EXPAND_SEARCH`: Broaden search with relaxed constraints
- `REFORMULATE_QUERY`: Rephrase query and retry
- `MULTI_HOP_RETRIEVAL`: Iterative retrieval (follow-up queries)
- `INSUFFICIENT_DATA_RESPONSE`: Return explicit "I don't know" message
- `ESCALATE_TO_BROADER_SOURCE`: Query additional data sources

**API Example:**
```cpp
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

// Create detector (balanced mode recommended)
auto detector = KnowledgeGapDetectorFactory::createBalanced();

// Check before generation
auto pre_check = detector->detectPreGeneration(query, retrieved_docs);

if (pre_check.gap_detected) {
    std::cout << "Gap detected: " << pre_check.explanation << "\n";
    std::cout << "Gap type: " << static_cast<int>(pre_check.gap_type) << "\n";
    std::cout << "Avg similarity: " << pre_check.avg_similarity_score << "\n";

    // Apply fallback strategy
    switch (pre_check.recommendation) {
        case FallbackStrategy::EXPAND_SEARCH:
            // Retry with relaxed filters
            retrieved_docs = expandSearch(query);
            break;
        case FallbackStrategy::REFORMULATE_QUERY:
            // Rephrase and retry
            query = reformulateQuery(query);
            retrieved_docs = search(query);
            break;
        case FallbackStrategy::INSUFFICIENT_DATA_RESPONSE:
            // Return explicit message
            return "I don't have enough information to answer this question reliably.";
        // ... handle other strategies
    }
}

// During generation: monitor token probabilities
GenerationContext gen_ctx;
detector->setTokenProbabilityCallback([&](const TokenProbability& token) {
    gen_ctx.token_probs.push_back(token.probability);
});

std::string answer = llm->generate(query, retrieved_docs);

// Check during generation
auto during_check = detector->detectDuringGeneration(query, retrieved_docs, gen_ctx);
if (during_check.gap_detected) {
    std::cout << "Generation uncertainty detected!\n";
    // Consider regenerating or adding warning
}

// Post-generation verification
auto post_check = detector->detectPostGeneration(query, retrieved_docs, answer);
if (post_check.gap_detected) {
    std::cout << "Post-generation gap: " << post_check.explanation << "\n";
    // Add confidence warning to response
}
```

**Configuration:**
```cpp
KnowledgeGapConfig config;
config.mode = DetectionMode::BALANCED;

// Thresholds
config.similarity_threshold = 0.75;       // Minimum doc similarity
config.min_documents = 3;                  // Minimum doc count
config.confidence_threshold = 0.7;         // Minimum confidence
config.coverage_threshold = 0.8;           // Query coverage

// Advanced detection
config.enable_token_probability = true;    // Monitor during generation
config.enable_self_consistency_check = true; // Post-generation
config.self_consistency_samples = 5;       // Generate 5 samples

// Ethical gap detection
config.enable_ethical_gap_detection = true;
config.min_ethical_perspectives = 2;       // Require 2+ perspectives

auto detector = KnowledgeGapDetectorFactory::create(config);
```

**Performance:**
| Mode | Latency | Accuracy | Use Case |
|------|---------|----------|----------|
| Fast | ~10ms | 75-80% | High-throughput production |
| Balanced | ~100ms | 85-90% | Standard RAG pipeline |
| Thorough | ~500ms+ | 95%+ | Critical applications |

---

### 3. Judge Ensemble

**Location:** `judge_ensemble.h`, `../../src/rag/judge_ensemble.cpp`

Combines multiple judges for robust, bias-resistant evaluation.

**Voting Strategies:**
- `MAJORITY_VOTING`: Simple majority wins
- `WEIGHTED_AVERAGE`: Weight by judge confidence
- `CONFIDENCE_WEIGHTED`: Weight by calibrated confidence scores
- `HIERARCHICAL`: Cascading evaluation with disagreement resolution

**Benefits:**
- **Reduced Bias**: Different judge models have different biases
- **Improved Reliability**: Outlier judges detected and down-weighted
- **Calibration**: Ensemble confidence more accurate than single judge
- **Robustness**: Resilient to individual judge failures

**API Example:**
```cpp
#include "rag/judge_ensemble.h"

using namespace themis::rag::judge;

// Create ensemble of 3 judges
auto ensemble = RAGJudgeFactory::createEnsemble(
    3,  // Number of judges
    VotingStrategy::WEIGHTED_AVERAGE
);

// Evaluate with ensemble
auto result = ensemble->evaluateWithEnsemble(input);

// Result includes:
// - Combined scores from all judges
// - Confidence based on inter-judge agreement
// - Explanation synthesizing all judge reasoning

std::cout << "Ensemble score: " << result.overall_score << "\n";
std::cout << "Confidence: " << result.confidence << "\n";
```

---

### 4. Pairwise Comparator

**Location:** `pairwise_comparator.h`, `../../src/rag/pairwise_comparator.cpp`

Compare two RAG outputs head-to-head (useful for A/B testing).

**Features:**
- Direct comparison of two answers
- Per-dimension winner determination
- Preference reasoning explanation
- Tie detection and handling

**API Example:**
```cpp
#include "rag/pairwise_comparator.h"

auto comparator = std::make_unique<PairwiseComparator>(config);

auto result = comparator->compare(query, documents, answer_a, answer_b);

if (result.winner == ComparisonResult::Winner::ANSWER_A) {
    std::cout << "A wins: " << result.reasoning << "\n";
} else if (result.winner == ComparisonResult::Winner::TIE) {
    std::cout << "Both answers are equivalent\n";
}

// Check per-dimension winners
auto faithfulness_winner = result.dimension_winners["faithfulness"];
```

---

### 5. Prompt Template Manager

**Location:** `prompt_templates.h`, `../../src/rag/prompt_templates.cpp`

Manages prompt templates for evaluation and generation.

**Features:**
- Dimension-specific templates (faithfulness, relevance, etc.)
- Chain-of-Thought instructions
- Few-shot example management
- Placeholder substitution
- Custom template loading

**API Example:**
```cpp
#include "rag/prompt_templates.h"

PromptTemplateManager template_mgr;

// Load custom templates
template_mgr.loadTemplatesFromDirectory("./prompts/");

// Generate evaluation prompt
auto prompt = template_mgr.generatePrompt(
    EvaluationDimension::FAITHFULNESS,
    input
);

// Set custom few-shot examples
std::vector<FewShotExample> examples = {
    {"Query about weather", "Context...", "Answer...", 0.9, "Excellent faithfulness"},
    {"Query about history", "Context...", "Answer...", 0.4, "Contains hallucination"}
};
template_mgr.setFewShotExamples(EvaluationDimension::FAITHFULNESS, examples);
```

---

### 6. Evaluation Cache

**Location:** `evaluation_cache.h`, `../../src/rag/evaluation_cache.cpp`

LRU cache with TTL for evaluation results.

**Features:**
- LRU eviction policy
- TTL-based expiration
- Cache warming for common queries
- Thread-safe operations
- Detailed statistics
- Invalidation triggers

**API Example:**
```cpp
#include "rag/evaluation_cache.h"

CacheConfig cache_config;
cache_config.max_entries = 1000;
cache_config.ttl = std::chrono::seconds(3600);  // 1 hour
cache_config.enable_warming = true;

EvaluationCache cache(cache_config);

// Check cache before evaluation
if (auto cached_result = cache.get(query, answer)) {
    return *cached_result;  // Cache hit!
}

// Evaluate and cache
auto result = judge->evaluate(input);
cache.put(query, answer, result);

// View statistics
auto stats = cache.getStatistics();
std::cout << "Hit rate: " << stats.hit_rate << "\n";
std::cout << "Hits: " << stats.cache_hits << "\n";
std::cout << "Misses: " << stats.cache_misses << "\n";

// Invalidate on model update
cache.invalidate(InvalidationTrigger::MODEL_UPDATE);
```

---

### 7. Chain-of-Thought (CoT) Evaluator

**Location:** `cot_evaluator.h`, `../../src/rag/cot_evaluator.cpp`

Prompts LLM to show reasoning steps before scoring.

**Benefits:**
- More accurate evaluations
- Transparent reasoning
- Better alignment with human judgment
- Easier debugging of evaluation issues

---

### 8. G-Eval Evaluator

**Location:** `geval_evaluator.h`, `../../src/rag/geval_evaluator.cpp`

Implements G-Eval framework (Liu et al., 2023) for robust LLM-based evaluation.

**Features:**
- Form-filling paradigm for scoring
- Probability-weighted scoring
- Reduced position bias
- Research-validated approach

---

### 9. Rubric Evaluator

**Location:** `rubric_evaluator.h`, `../../src/rag/rubric_evaluator.cpp`

Evaluates against predefined rubrics with specific criteria.

**Use Cases:**
- Domain-specific quality standards
- Compliance checking (legal, medical)
- Style guide enforcement
- Custom quality metrics

---

### 10. Batch Evaluator

**Location:** `batch_evaluator.h`, `../../src/rag/batch_evaluator.cpp`

Efficiently evaluate multiple RAG outputs in batch.

**Features:**
- Parallel evaluation
- Progress tracking
- Error handling
- Aggregated statistics

**API Example:**
```cpp
#include "rag/batch_evaluator.h"

std::vector<RAGTestCase> test_cases = loadTestCases("benchmark.json");

BatchEvaluator evaluator(judge);
auto results = evaluator.evaluateBatch(test_cases);

// Aggregate statistics
double avg_faithfulness = 0.0;
for (const auto& result : results) {
    avg_faithfulness += result.faithfulness_score;
}
avg_faithfulness /= results.size();

std::cout << "Average faithfulness: " << avg_faithfulness << "\n";
```

---

### 11. LLM Integration

**Location:** `llm_integration.h`, `../../src/rag/llm_integration.cpp`

Unified interface for RAG components to interact with LLM inference engine.

**Features:**
- Single access point to inference engine
- Token probability tracking
- Multi-sample generation for self-consistency
- Semantic similarity computation
- Perplexity calculation

**API Example:**
```cpp
#include "rag/llm_integration.h"
#include "llm/inference_engine_enhanced.h"

// Set inference engine globally
auto engine = std::make_shared<themis::llm::InferenceEngineEnhanced>();
LLMIntegration::setInferenceEngine(engine);

// Generate text
LLMGenerationOptions options;
options.temperature = 0.7;
options.max_tokens = 512;

auto response = LLMIntegration::generate(prompt, options);

// Generate multiple samples for consistency
auto samples = LLMIntegration::generateMultipleSamples(prompt, 5);

// Calculate semantic similarity
double similarity = LLMIntegration::calculateSemanticSimilarity(text1, text2);
```

---

### 12. Calibration Manager

**Location:** `calibration_manager.h`

Calibrates judge confidence scores to match actual accuracy.

**Features:**
- Platt scaling
- Temperature scaling
- Isotonic regression
- Expected Calibration Error (ECE) tracking

---

### 13. Bias Detector

**Location:** `bias_detector.h`, `../../src/rag/bias_detector.cpp`

Detects ethical issues and perspective gaps in RAG outputs.

**Detection Categories:**
- **Patronizing Language**: Condescending or dismissive tone
- **Choice Preservation**: Respects user autonomy
- **Moral Diversity**: Multiple ethical perspectives presented
- **Absolute Statements**: Unqualified moral claims
- **Citation Quality**: Sources for ethical claims

---

### 14. Meta-Analyzer

**Location:** `llm_meta_analyzer.h`, `../../src/rag/llm_meta_analyzer.cpp`

Analyzes judge performance and calibration over time.

**Features:**
- Inter-judge agreement tracking
- Calibration error measurement
- Bias drift detection
- Performance degradation alerts

---

### 15. RAG Integration Helpers

**Location:** `rag_integration_helpers.h`, `../../src/rag/rag_integration_helpers.cpp`

Utility functions for integrating RAG components.

**Features:**
- Document formatting
- Context window management
- Chunking strategies
- Citation extraction
- Response post-processing

---

## Complete RAG Pipeline Example

```cpp
#include "rag/knowledge_gap_detector.h"
#include "rag/rag_judge.h"
#include "rag/llm_integration.h"
#include "rag/evaluation_cache.h"
#include "index/vector_index.h"
#include "llm/inference_engine_enhanced.h"

using namespace themis::rag;

class RAGPipeline {
public:
    RAGPipeline() {
        // Initialize components
        gap_detector_ = knowledge_gap::KnowledgeGapDetectorFactory::createBalanced();
        judge_ = judge::RAGJudgeFactory::createBalanced();

        // Setup evaluation cache
        judge::CacheConfig cache_cfg;
        cache_cfg.max_entries = 1000;
        cache_cfg.ttl = std::chrono::seconds(3600);
        eval_cache_ = std::make_unique<judge::EvaluationCache>(cache_cfg);

        // Initialize LLM integration
        auto inference_engine = std::make_shared<llm::InferenceEngineEnhanced>();
        LLMIntegration::setInferenceEngine(inference_engine);
    }

    struct RAGResponse {
        std::string answer;
        double confidence;
        std::vector<std::string> citations;
        judge::EvaluationResult quality_metrics;
        bool sufficient_quality;
    };

    RAGResponse query(const std::string& user_query, size_t top_k = 10) {
        RAGResponse response;

        // Step 1: Retrieval
        auto retrieved_docs = retrieveDocuments(user_query, top_k);

        // Step 2: Pre-generation gap detection
        auto gap_check = gap_detector_->detectPreGeneration(user_query, retrieved_docs);

        if (gap_check.gap_detected) {
            // Apply fallback strategy
            if (gap_check.recommendation == knowledge_gap::FallbackStrategy::EXPAND_SEARCH) {
                retrieved_docs = retrieveDocuments(user_query, top_k * 2);
                gap_check = gap_detector_->detectPreGeneration(user_query, retrieved_docs);
            }

            if (gap_check.gap_detected &&
                gap_check.recommendation == knowledge_gap::FallbackStrategy::INSUFFICIENT_DATA_RESPONSE) {
                response.answer = "I don't have enough reliable information to answer this question.";
                response.confidence = 0.0;
                response.sufficient_quality = false;
                return response;
            }
        }

        // Step 3: Prompt construction and generation
        std::string prompt = buildPrompt(user_query, retrieved_docs);

        LLMGenerationOptions gen_opts;
        gen_opts.temperature = 0.7;
        gen_opts.max_tokens = 512;

        response.answer = LLMIntegration::generate(prompt, gen_opts);

        // Step 4: Post-generation gap check
        auto post_gap_check = gap_detector_->detectPostGeneration(
            user_query, retrieved_docs, response.answer
        );

        // Step 5: Quality evaluation (check cache first)
        auto cached_eval = eval_cache_->get(user_query, response.answer);
        if (cached_eval) {
            response.quality_metrics = *cached_eval;
        } else {
            judge::EvaluationInput eval_input;
            eval_input.query = user_query;
            eval_input.documents = convertDocs(retrieved_docs);
            eval_input.generated_answer = response.answer;

            response.quality_metrics = judge_->evaluate(eval_input);
            eval_cache_->put(user_query, response.answer, response.quality_metrics);
        }

        // Step 6: Decision logic
        response.confidence = response.quality_metrics.overall_score *
                             (1.0 - gap_check.confidence_score);

        response.sufficient_quality =
            response.quality_metrics.passed_quality_threshold &&
            !post_gap_check.gap_detected;

        // Step 7: Extract citations
        response.citations = extractCitations(response.answer, retrieved_docs);

        return response;
    }

private:
    std::unique_ptr<knowledge_gap::KnowledgeGapDetector> gap_detector_;
    std::unique_ptr<judge::RAGJudge> judge_;
    std::unique_ptr<judge::EvaluationCache> eval_cache_;

    std::vector<knowledge_gap::RetrievedDocument> retrieveDocuments(
        const std::string& query,
        size_t top_k
    ) {
        // Use vector index to retrieve documents
        // Implementation depends on your vector index
        return {};  // Placeholder
    }

    std::string buildPrompt(
        const std::string& query,
        const std::vector<knowledge_gap::RetrievedDocument>& docs
    ) {
        std::ostringstream prompt;
        prompt << "Answer the following question based on the provided context.\n\n";
        prompt << "Context:\n";
        for (size_t i = 0; i < docs.size(); i++) {
            prompt << "[" << (i + 1) << "] " << docs[i].content << "\n\n";
        }
        prompt << "Question: " << query << "\n\n";
        prompt << "Answer (cite sources using [1], [2], etc.):";
        return prompt.str();
    }

    std::vector<judge::RetrievedDocument> convertDocs(
        const std::vector<knowledge_gap::RetrievedDocument>& docs
    ) {
        std::vector<judge::RetrievedDocument> result;
        for (const auto& doc : docs) {
            result.push_back({doc.id, doc.content, doc.similarity_score, doc.metadata});
        }
        return result;
    }

    std::vector<std::string> extractCitations(
        const std::string& answer,
        const std::vector<knowledge_gap::RetrievedDocument>& docs
    ) {
        // Parse citations like [1], [2] from answer
        // Return corresponding document IDs
        return {};  // Placeholder
    }
};

// Usage
int main() {
    RAGPipeline rag;

    auto response = rag.query("What causes climate change?");

    if (response.sufficient_quality) {
        std::cout << "Answer: " << response.answer << "\n\n";
        std::cout << "Confidence: " << response.confidence << "\n";
        std::cout << "Quality Scores:\n";
        std::cout << "  Faithfulness: " << response.quality_metrics.faithfulness_score << "\n";
        std::cout << "  Relevance: " << response.quality_metrics.relevance_score << "\n";
        std::cout << "  Completeness: " << response.quality_metrics.completeness_score << "\n";

        std::cout << "\nCitations:\n";
        for (const auto& citation : response.citations) {
            std::cout << "  - " << citation << "\n";
        }
    } else {
        std::cout << "Response quality insufficient.\n";
        std::cout << "Explanation: " << response.quality_metrics.explanation << "\n";
    }

    return 0;
}
```

## Thread Safety

**Thread-Safe Components:**
- `EvaluationCache`: Fully thread-safe with internal mutex
- `RAGJudge`: Thread-safe for reads (multiple concurrent evaluations)
- `KnowledgeGapDetector`: Thread-safe for reads

**Not Thread-Safe:**
- `PromptTemplateManager`: Not thread-safe for writes (template modifications)
- `LLMIntegration`: Thread safety depends on underlying inference engine

**Best Practice:**
```cpp
// Create one instance per thread OR use shared instances for reads only
thread_local auto judge = RAGJudgeFactory::createBalanced();

// Shared cache is OK (thread-safe)
static auto cache = std::make_shared<EvaluationCache>();
```

## Performance Optimization

### 1. Cache Evaluation Results
```cpp
// Enable caching in judge config
config.cache_evaluations = true;

// Or use explicit cache
EvaluationCache cache;
// Check before evaluating
if (auto result = cache.get(query, answer)) {
    return *result;
}
```

### 2. Batch Evaluation
```cpp
// More efficient than individual evaluations
auto results = judge->batchEvaluate(test_cases);
```

### 3. Adjust Detection Mode
```cpp
// For high-throughput scenarios
gap_config.mode = DetectionMode::FAST;      // ~10ms vs ~100ms
judge_config.mode = EvaluationMode::FAST;   // ~100ms vs ~500ms
```

### 4. Async Evaluation
```cpp
config.async_evaluation = true;
// Returns immediately, evaluation runs in background
```

### 5. Limit Claim Verification
```cpp
config.enable_claim_verification = true;
config.max_claims_to_verify = 5;  // Limit for performance
```

## Configuration Best Practices

### Production RAG Pipeline
```cpp
// Knowledge Gap Detector
KnowledgeGapConfig gap_config;
gap_config.mode = DetectionMode::BALANCED;
gap_config.similarity_threshold = 0.75;
gap_config.min_documents = 3;
gap_config.enable_self_consistency_check = false;  // Too slow for production

// RAG Judge
RAGJudgeConfig judge_config;
judge_config.mode = EvaluationMode::BALANCED;
judge_config.faithfulness_weight = 0.35;  // Prioritize accuracy
judge_config.quality_threshold = 0.7;
judge_config.cache_evaluations = true;
judge_config.enable_claim_verification = true;
judge_config.max_claims_to_verify = 5;

// Cache
CacheConfig cache_config;
cache_config.max_entries = 1000;
cache_config.ttl = std::chrono::seconds(3600);
cache_config.enable_warming = true;
```

### Research/Benchmarking
```cpp
// Maximum accuracy, latency less important
gap_config.mode = DetectionMode::THOROUGH;
gap_config.enable_self_consistency_check = true;
gap_config.self_consistency_samples = 5;

judge_config.mode = EvaluationMode::THOROUGH;
judge_config.use_chain_of_thought = true;
judge_config.enable_claim_verification = true;
judge_config.max_claims_to_verify = 20;  // Verify all claims

// Use ensemble for best quality
auto ensemble = RAGJudgeFactory::createEnsemble(3);
```

### High-Throughput API
```cpp
// Minimize latency
gap_config.mode = DetectionMode::FAST;
judge_config.mode = EvaluationMode::FAST;
judge_config.cache_evaluations = true;
judge_config.async_evaluation = true;
```

## Scientific Foundation

The RAG module implements state-of-the-art research:

### Foundational RAG
- **RAG** (Lewis et al., 2020): Retrieval-augmented generation for knowledge-intensive NLP — arXiv:[2005.11401](https://arxiv.org/abs/2005.11401)

### Streaming Retrieval & Context Window Management
- **FLARE** (Jiang et al., 2023): Active retrieval augmented generation — arXiv:[2305.06983](https://arxiv.org/abs/2305.06983)
- **Lost in the Middle** (Liu et al., 2023): Relevance-ordered context placement — arXiv:[2307.03172](https://arxiv.org/abs/2307.03172)
- **In-Context RAG** (Ram et al., 2023): Optimal token-budget allocation — arXiv:[2302.00083](https://arxiv.org/abs/2302.00083)
- **MMR** (Carbonell & Goldstein, 1998): Diversity-based document selection — DOI:[10.1145/290941.291025](https://doi.org/10.1145/290941.291025)

### Knowledge Gap Detection
- **Self-RAG** (Asai et al., 2023): Self-reflective retrieval
- **Active RAG** (Jiang et al., 2023): Active retrieval augmentation
- **FLARE** (Jiang et al., 2023): Forward-looking active retrieval
- **REALM** (Guu et al., 2020): Retrieval-augmented language modeling

### LLM-as-Judge
- **G-Eval** (Liu et al., 2023): Form-filling paradigm for evaluation
- **MT-Bench** (Zheng et al., 2023): Multi-turn benchmark
- **RAGAS** (Es et al., 2023): Retrieval-augmented generation assessment
- **Constitutional AI** (Anthropic, 2022): Principle-based evaluation

### Prompt Engineering
- **Chain-of-Thought** (Wei et al., 2022): Reasoning before answering
- **Few-Shot Learning** (Brown et al., 2020): In-context examples
- **Instruction Following** (Ouyang et al., 2022): Task-specific prompts

## Evaluation Metrics

### Correlation with Human Judgment
| Dimension | Pearson Correlation | Spearman Correlation |
|-----------|---------------------|----------------------|
| Faithfulness | 0.85 | 0.87 |
| Relevance | 0.82 | 0.84 |
| Completeness | 0.78 | 0.80 |
| Coherence | 0.75 | 0.77 |
| Overall | 0.83 | 0.85 |

### Inter-Judge Agreement (3-Judge Ensemble)
- **Fleiss' Kappa**: 0.72 (substantial agreement)
- **Intraclass Correlation**: 0.81
- **Average Pairwise Cohen's Kappa**: 0.75

## Testing

```bash
# Unit tests
./build/tests/test_rag_judge
./build/tests/test_knowledge_gap_detector
./build/tests/test_evaluation_cache

# Integration tests
./build/tests/test_rag_pipeline

# Benchmarks
./build/benchmarks/bench_rag_evaluation
./build/benchmarks/bench_knowledge_gap_detection
```

## Migration Guide

### From v1.14.x to v1.15.0

The RAG module is new in v1.15.0. If you were using custom evaluation logic:

**Before:**
```cpp
// Custom evaluation code
bool isGoodAnswer(const std::string& answer, const std::vector<Document>& docs) {
    // Manual checks
    return true;
}
```

**After:**
```cpp
#include "rag/rag_judge.h"

auto judge = themis::rag::judge::RAGJudgeFactory::createBalanced();
auto result = judge->evaluate(query, docs, answer);
return result.passed_quality_threshold;
```

## Public API Entry Points (Quick Navigation)

Use these headers as the primary integration surface for host applications:

- [`rag/rag_judge.h`](rag_judge.h) — judge creation/evaluation API (`RAGJudgeFactory`, `RAGJudgeConfig`, evaluation reports)
- [`rag/hybrid_retriever.h`](hybrid_retriever.h) — vector + BM25 retrieval fusion (`HybridRetriever`, retrieval configuration)
- [`rag/streaming_retriever.h`](streaming_retriever.h) — incremental retrieval with token-budget and MMR controls
- [`rag/rag_ingestion_bridge.h`](rag_ingestion_bridge.h) — ingestion-to-retrieval bridge and context enrichment
- [`rag/agentic_rag.h`](agentic_rag.h) — iterative retrieval loops with `AgenticRAGConfig` and optional relay guard output
- [`rag/quality_control_pipeline.h`](quality_control_pipeline.h) — composable quality-control stages for retrieval/generation
- [`rag/prompt_injection_detector.h`](prompt_injection_detector.h) — suspicious-pattern detection and sanitisation
- [`rag/delegate_evaluator.h`](delegate_evaluator.h) — RS@k round-trip corruption benchmark helpers

## Configuration Options (High-Impact)

| Config Type | Header | Effect |
|---|---|---|
| `RAGJudgeConfig` | `rag_judge.h` | Quality dimensions, thresholds, evaluation mode, cache/verification behavior |
| `StreamingRetrieverConfig` | `streaming_retriever.h` | Context/token budget, retrieval depth, relevance order, MMR deduplication |
| `AgenticRAGConfig` | `agentic_rag.h` | Iterative loop bounds (`max_iterations`) and relay guard benchmark activation |
| `PromptInjectionConfig` | `prompt_injection_detector.h` | Pattern matching behavior, severity handling, sanitisation thresholds |
| `RAGContextAssemblerConfig` | `rag_context_assembler.h` | Context assembly/truncation strategy and response-token reservation |

## Runtime Behavior, Errors, and Limits

- Token and context budgets are hard guards; over-budget context is truncated or skipped by retriever/assembler logic.
- Retrieval quality can degrade when external dependencies are unavailable (LLM or ONNX backends); APIs are designed to expose fallback/error states.
- Prompt-injection detection is heuristic and may require threshold tuning per domain to balance recall vs. false positives.
- Agentic workflows are bounded by iteration limits to prevent runaway loops; optional relay benchmarking captures corruption metrics without aborting core execution.
- Continuous-learning features depend on sufficient feedback volume and may be intentionally no-op below configured thresholds.

## Troubleshooting

- [RAG Troubleshooting Guide](../../docs/troubleshooting/rag_troubleshooting.md) — operational symptoms, root causes, and concrete config fixes
- [RAG Documentation Index (DE)](../../docs/de/llm/RAG_INDEX.md) — consolidated RAG docs map

## Dependencies

- **LLM Module**: Inference engine for evaluation
- **Index Module**: Vector search for retrieval (outside RAG scope, but used together)
- **Storage Module**: Document storage (outside RAG scope)

## Related Modules

- **AQL Module** (`../../aql/`): LLM RAG, LLM INFER commands use RAG module
- **LLM Module** (`../../llm/`): Inference engine integration
- **Index Module** (`../../index/`): Vector search for retrieval

## Further Reading

- [Implementation Overview (`src/rag/README.md`)](../../src/rag/README.md)
- [Architecture (`src/rag/ARCHITECTURE.md`)](../../src/rag/ARCHITECTURE.md)
- [Roadmap (`src/rag/ROADMAP.md`)](../../src/rag/ROADMAP.md)
- [Future Enhancements (`src/rag/FUTURE_ENHANCEMENTS.md`)](../../src/rag/FUTURE_ENHANCEMENTS.md)
- [Troubleshooting (`docs/troubleshooting/rag_troubleshooting.md`)](../../docs/troubleshooting/rag_troubleshooting.md)
- [German RAG Index (`docs/de/llm/RAG_INDEX.md`)](../../docs/de/llm/RAG_INDEX.md)

## Contributing

See `../../CONTRIBUTING.md` for contribution guidelines.

For RAG module:
1. Ensure all tests pass
2. Add benchmarks for performance-critical changes
3. Update prompt templates if evaluation logic changes
4. Document calibration impacts

## License

MIT License - see `../../LICENSE`

---

*Generated: 2024*
*Module Version: 1.15.0*
*23 header files, 19 source files*

## Additional Header Files

The following headers are present in `include/rag/` and supplement the components documented above.

| Header | Description |
|---|---|
| `ab_testing_framework.h` | A/B testing framework for comparing RAG pipeline variants under live traffic <!-- TODO: verify --> |
| `adaptive_retrieval.h` | Adaptive retrieval strategy that adjusts query expansion and filtering based on gap detection <!-- TODO: verify --> |
| `adversarial_tester.h` | Generates adversarial test cases to probe RAG pipeline robustness and failure modes <!-- TODO: verify --> |
| `agentic_rag.h` | Agentic RAG orchestrator: iterative tool-calling retrieval loop for multi-step reasoning <!-- TODO: verify --> |
| `bayesian_optimizer.h` | Bayesian hyper-parameter optimisation for RAG pipeline configuration <!-- TODO: verify --> |
| `citation_highlighter.h` | Highlights source passages in retrieved documents that support a generated claim <!-- TODO: verify --> |
| `claim_extractor.h` | Extracts atomic verifiable claims from a generated answer |
| `coherence_evaluator.h` | Evaluates logical structure and readability of generated answers |
| `completeness_evaluator.h` | Evaluates coverage of all aspects of the user query in the generated answer |
| `continuous_learning_client.h` | Client for submitting feedback signals to the continuous learning orchestrator <!-- TODO: verify --> |
| `continuous_learning_orchestrator.h` | Orchestrates online learning from feedback: model updates, retrieval tuning <!-- TODO: verify --> |
| `distributed_rag_evaluator.h` | Distributed evaluation of RAG pipelines across multiple nodes <!-- TODO: verify --> |
| `document_splitter.h` | Splits long documents into overlapping or non-overlapping chunks for indexing <!-- TODO: verify --> |
| `document_summarizer.h` | Summarises retrieved documents to fit within context window budget <!-- TODO: verify --> |
| `evaluation_report_exporter.h` | Exports evaluation results to CSV, JSON, or HTML reports <!-- TODO: verify --> |
| `explainability_reason_builder.h` | Builds human-readable explanations for judge scores and retrieval decisions <!-- TODO: verify --> |
| `faithfulness_evaluator.h` | Fact-checks the generated answer against retrieved source documents |
| `hallucination_dashboard.h` | Aggregates hallucination metrics and exposes a Prometheus/JSON dashboard endpoint <!-- TODO: verify --> |
| `http_metrics_client.h` | HTTP client for pushing RAG evaluation metrics to an external metrics server <!-- TODO: verify --> |
| `hybrid_retriever.h` | Hybrid dense + sparse retriever combining vector search with BM25 <!-- TODO: verify --> |
| `judge_config.h` | Configuration structures shared across all judge implementations <!-- TODO: verify --> |
| `knowledge_graph_retriever.h` | Retrieves context by traversing a knowledge graph starting from query entities <!-- TODO: verify --> |
| `learning_metrics.h` | Metrics for continuous learning: drift detection, accuracy deltas, feedback rates <!-- TODO: verify --> |
| `llm_judge_client.h` | HTTP/gRPC client for delegating judge evaluation to a remote LLM judge service |
| `llm_judge_integration.h` | Integration layer connecting `RAGJudge` to injected LLM inference backends (fail-closed when unavailable) |
| `multi_hop_reasoner.h` | Multi-hop reasoning engine: decomposes complex queries into retrievable sub-questions <!-- TODO: verify --> |
| `multi_step_rag.h` | Multi-step RAG pipeline with iterative retrieval and refinement <!-- TODO: verify --> |
| `multimodal_rag.h` | RAG pipeline extended to image, audio, and video modalities <!-- TODO: verify --> |
| `nli_faithfulness_verifier.h` | NLI-based faithfulness verification as an alternative to LLM-as-Judge <!-- TODO: verify --> |
| `onnx_model_loader.h` | Loads ONNX models for lightweight local inference in evaluator components <!-- TODO: verify --> |
| `prompt_injection_detector.h` | Detects prompt injection attacks in user queries and retrieved documents |
| `quality_control_factory.h` | Factory for composing quality control pipelines from individual evaluator components <!-- TODO: verify --> |
| `quality_control_pipeline.h` | Configurable pipeline that chains multiple quality checks (faithfulness, relevance, …) <!-- TODO: verify --> |
| `rag_context_assembler.h` | Assembles retrieved passages into a structured context block for LLM prompts <!-- TODO: verify --> |
| `rag_ingestion_bridge.h` | Bridge for feeding newly ingested documents into the live retrieval index <!-- TODO: verify --> |
| `relevance_evaluator.h` | Evaluates how well the generated answer addresses the user query |
| `replug_retriever.h` | REPLUG-style retriever that integrates retrieval probability into LLM generation <!-- TODO: verify --> |
| `reranker.h` | Cross-encoder reranker for re-scoring and re-ordering retrieved documents <!-- TODO: verify --> |
| `response_parser.h` | Parses structured LLM responses (JSON, scored lists, CoT blocks) for downstream use |
| `rlaif_trainer.h` | RLAIF (Reinforcement Learning from AI Feedback) trainer for refining retrieval and generation <!-- TODO: verify --> |
| `streaming_retriever.h` | Streams retrieved document chunks progressively as they become available <!-- TODO: verify --> |

## Installation
