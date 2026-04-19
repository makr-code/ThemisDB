> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# RAG Module Implementation - Future Enhancements

- Multi-judge evaluation pipeline (faithfulness, relevance, completeness, coherence, bias) with Fast/Balanced/Thorough modes
- Streaming retrieval with incremental context-window filling for low first-token latency
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Cross-encoder re-ranking with ONNX model integration for precision improvement
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Hybrid BM25 + vector retrieval fused via Reciprocal Rank Fusion (RRF) with configurable weights
- Citation highlighting mapping each answer sentence to its source document chunk
- Knowledge-graph augmented retrieval with entity linking and graph traversal
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Agentic RAG with multi-hop iterative retrieval loops and tool-use orchestration
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Online Bayesian optimization of `top_k` and `similarity_threshold` from evaluation feedback
<!-- TODO: add measurable target, interface spec, and test strategy -->

## Design Constraints

- [x] Fast evaluation mode must complete end-to-end in ≤ 100 ms at p99 (single judge, no LLM call)
- [x] Balanced mode must complete in ≤ 500 ms at p99; Thorough mode ≤ 2 s at p99
- [x] All evaluator scores must be normalised to the [0.0, 1.0] float range; no breaking changes to scoring API
- [x] `StreamingRetriever` must fill the context window incrementally without blocking the generator thread
- [x] Cross-encoder re-ranker must degrade gracefully to heuristic scoring when the ONNX model is not loaded
- [x] `HybridRetriever` RRF weights must be runtime-configurable without index rebuild
- [x] `ClaimExtractor` must fall back to sentence-boundary heuristics when the LLM judge is unavailable
- [x] Agentic RAG loop must enforce a configurable maximum iteration count (default: 5) to prevent infinite loops

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `RAGJudge::evaluate(query, answer, docs)` | API handler, evaluation CLI | Returns `EvaluationReport`; mode selectable (FAST/BALANCED/THOROUGH) |
| `HybridRetriever::retrieve(query, top_k)` | Generator pipeline | Returns ranked `RetrievedChunk` list; RRF weights configurable |
| `CrossEncoderReranker::rerank(query, chunks)` | Retrieval post-processor | ONNX model path configurable; heuristic fallback when model absent |
| `StreamingRetriever::stream(query, cb)` | LLM streaming handler | Invokes `cb` for each chunk as retrieved; non-blocking |
| `CitationHighlighter::highlight(answer, chunks)` | Response formatter | Returns `AnnotatedAnswer` with per-sentence source references |
| `KnowledgeGraphRetriever::retrieve(entities)` | Hybrid retrieval pipeline | Entity linking + graph traversal; falls back to dense retrieval |
| `OnlineLearner::getOptimizedParams()` | Retrieval config layer | Returns `{top_k, similarity_threshold}` from Bayesian optimizer |
| `AgenticRAG::run(query, tools)` | Orchestration layer | Max iterations configurable; returns final answer + retrieval trace |

This document outlines planned implementation improvements for the RAG module source files.

## Source Code Audit Findings (2026-03-12)

### `LLMIntegration` and `LLMJudgeIntegration`: Replace Stub/Mock Mode with Real Engine
**Priority:** High
**Target Version:** v1.8.0

Both RAG LLM integration points fall back to stubs when no real inference engine is configured:
- `llm_integration.cpp` line 109: "No inference engine configured, using stub"
- `llm_judge_integration.cpp` line 45/173: "initialized in MOCK MODE - evaluations will use stub responses"

In mock mode, all evaluations produce fixed scores, making the RAG evaluation pipeline useless in production.

**Implementation Notes:**
- `[x]` `LLMIntegration::generate()`: wire to `LLMPluginManager::instance().generate()` via the same `THEMIS_ENABLE_LLM` guard used by `ingestion/llm_adapter.cpp`; fail fast with a clear error (not a stub) when `THEMIS_ENABLE_LLM=OFF` and a real engine is required.
- `[x]` `LLMJudgeIntegration`: accept an `ILLMInferenceEngine*` injection in the constructor; throw `std::invalid_argument` at construction time when `nullptr` is passed if mock mode is disabled via `config_.allow_mock = false`.
- `[x]` Add a configuration key `rag.llm_judge.allow_mock` (default `false` in production, `true` in tests) so mock mode is explicitly opt-in.
- `[x]` Add unit tests that verify the judge returns real score variance (not constant scores) when a mock engine returning random values is injected.

---



### 1. SIMD-Accelerated Similarity Computation
**Priority:** High
**Target Version:** v1.16.0
**File:** `faithfulness_evaluator.cpp`, `relevance_evaluator.cpp`

Accelerate semantic similarity calculations using SIMD instructions.

**Current:**
```cpp
double calculateSemanticSimilarity(const std::string& text1, const std::string& text2) {
    auto emb1 = getEmbedding(text1);
    auto emb2 = getEmbedding(text2);

    double dot = 0.0;
    for (size_t i = 0; i < emb1.size(); i++) {
        dot += emb1[i] * emb2[i];  // Scalar operations
    }
    return dot;
}
```

**Optimized with AVX2:**
```cpp
#include <immintrin.h>

double calculateSemanticSimilarity(const std::string& text1, const std::string& text2) {
    auto emb1 = getEmbedding(text1);
    auto emb2 = getEmbedding(text2);

    __m256 sum = _mm256_setzero_ps();

    size_t i = 0;
    for (; i + 8 <= emb1.size(); i += 8) {
        __m256 a = _mm256_loadu_ps(&emb1[i]);
        __m256 b = _mm256_loadu_ps(&emb2[i]);
        __m256 prod = _mm256_mul_ps(a, b);
        sum = _mm256_add_ps(sum, prod);
    }

    // Horizontal sum
    float result[8];
    _mm256_storeu_ps(result, sum);
    double dot = 0.0;
    for (int j = 0; j < 8; j++) {
        dot += result[j];
    }

    // Handle remainder
    for (; i < emb1.size(); i++) {
        dot += emb1[i] * emb2[i];
    }

    return dot;
}
```

**Expected Speedup:** 3-5x for similarity computations

---

### 2. Parallel Claim Verification
**Priority:** High
**Target Version:** v1.16.0
**File:** `faithfulness_evaluator.cpp`

Verify multiple claims in parallel instead of sequentially.

**Current:**
```cpp
for (const auto& claim : claims) {
    bool verified = false;
    for (const auto& doc : documents) {
        if (verifyClaim(claim, doc)) {
            verified = true;
            break;
        }
    }
    if (verified) verified_count++;
}
```

**Parallel:**
```cpp
#include <execution>

std::atomic<size_t> verified_count{0};

std::for_each(std::execution::par, claims.begin(), claims.end(),
    [&](const std::string& claim) {
        for (const auto& doc : documents) {
            if (verifyClaim(claim, doc)) {
                verified_count++;
                break;
            }
        }
    }
);
```

**Expected Speedup:** 2-4x on multi-core systems

---

### 3. Persistent Evaluation Cache with Disk Backing
**Priority:** Medium
**Target Version:** v1.17.0
**File:** New file `evaluation_cache.cpp`

Move from header-only in-memory cache to persistent disk-backed cache.

**Benefits:**
- Cache survives restarts
- Larger cache capacity (GB vs MB)
- Shared across processes

**Implementation:**
```cpp
#include "rocksdb/db.h"

class PersistentEvaluationCache {
public:
    PersistentEvaluationCache(const std::string& db_path) {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::DB::Open(options, db_path, &db_);
    }

    std::optional<EvaluationResult> get(const std::string& key) {
        std::string value;
        auto status = db_->Get(rocksdb::ReadOptions(), key, &value);

        if (status.ok()) {
            return deserialize(value);
        }
        return std::nullopt;
    }

    void put(const std::string& key, const EvaluationResult& result) {
        std::string value = serialize(result);
        db_->Put(rocksdb::WriteOptions(), key, value);
    }

private:
    rocksdb::DB* db_;
};
```

---

### 4. Batched LLM Calls
**Priority:** High
**Target Version:** v1.16.0
**File:** `llm_integration.cpp`

Batch multiple evaluation prompts into single LLM call.

**Current:** Sequential calls (high latency)
```cpp
for (auto& dim : dimensions) {
    auto prompt = generatePrompt(dim, input);
    auto score = llm->generate(prompt);  // Separate call
    scores[dim] = score;
}
```

**Batched:** Single call (low latency)
```cpp
std::string batch_prompt = R"(
Evaluate the following answer on multiple dimensions.

[Dimension 1: Faithfulness]
...

[Dimension 2: Relevance]
...

Provide scores in JSON format:
{"faithfulness": 0.9, "relevance": 0.85, ...}
)";

auto response = llm->generate(batch_prompt);
auto scores = parseJSONScores(response);
```

**Expected Speedup:** 3-5x (5 sequential calls → 1 batched call)

---

## Accuracy Improvements

### 5. Fine-Tuned NLI Model for Claim Verification
**Priority:** High
**Target Version:** v1.17.0
**File:** `faithfulness_evaluator.cpp`

Replace generic NLI model with RAG-specific fine-tuned model.

**Current:** Use generic NLI (DebertaV3-large)
- Trained on general NLI tasks (SNLI, MultiNLI)
- Not optimized for RAG claim verification

**Improved:** Fine-tune on RAG-specific data
- Train on: (claim, document) → entailment/contradiction/neutral
- Dataset: FEVER, fact-checking datasets, synthetic RAG data

**Implementation:**
```cpp
class RAGSpecificNLI {
public:
    RAGSpecificNLI(const std::string& model_path) {
        // Load fine-tuned model
        model_ = loadONNXModel(model_path);
    }

    double score(const std::string& premise, const std::string& hypothesis) {
        auto inputs = tokenize(premise, hypothesis);
        auto outputs = model_->forward(inputs);

        // [entailment, neutral, contradiction]
        return outputs[0];  // Entailment probability
    }

private:
    std::unique_ptr<ONNXModel> model_;
};
```

**Expected Improvement:** +5-10% faithfulness accuracy

---

### 6. Learned Query Aspect Extraction
**Priority:** Medium
**Target Version:** v1.17.0
**File:** `completeness_evaluator.cpp`

Replace rule-based aspect extraction with ML model.

**Current:** Regex patterns for "who", "what", "when", etc.
**Improved:** Semantic parsing model

```cpp
class QueryAspectExtractor {
public:
    std::vector<Aspect> extract(const std::string& query) {
        // Use semantic role labeling (SRL) or question parsing model
        auto parse = srl_model_->parse(query);

        std::vector<Aspect> aspects;
        for (const auto& role : parse.roles) {
            if (role.label == "ARG0") {
                aspects.push_back({AspectType::AGENT, role.span});
            } else if (role.label == "ARGM-TMP") {
                aspects.push_back({AspectType::TIME, role.span});
            }
            // ... other roles
        }

        return aspects;
    }

private:
    std::unique_ptr<SRLModel> srl_model_;
};
```

---

### 7. Calibration with Temperature Scaling
**Priority:** Medium
**Target Version:** v1.17.0
**File:** New file `calibration_manager.cpp`

Calibrate judge confidence scores using learned temperature parameter.

**Algorithm:**
```cpp
class CalibrationManager {
public:
    void calibrate(const std::vector<EvaluationResult>& predictions,
                   const std::vector<double>& ground_truth) {
        // Learn temperature T that minimizes calibration error
        // Calibrated score = softmax(logit / T)

        double best_temp = 1.0;
        double best_ece = 1.0;

        for (double temp = 0.1; temp <= 5.0; temp += 0.1) {
            double ece = computeECE(predictions, ground_truth, temp);
            if (ece < best_ece) {
                best_ece = ece;
                best_temp = temp;
            }
        }

        temperature_ = best_temp;
    }

    double calibrateScore(double raw_score) {
        // Apply temperature scaling
        double logit = std::log(raw_score / (1 - raw_score));
        double calibrated_logit = logit / temperature_;
        return 1.0 / (1.0 + std::exp(-calibrated_logit));
    }

private:
    double temperature_ = 1.0;
};
```

---

## Feature Additions

### 8. Hierarchical Claim Extraction
**Priority:** Medium
**Target Version:** v1.17.0
**File:** `claim_extractor.cpp`

Extract claims at multiple granularities (sentence → sub-claim → atomic).

**Current:** Flat claim list
**Improved:** Hierarchical claim tree

```cpp
struct ClaimNode {
    std::string text;
    ClaimGranularity level;  // SENTENCE, SUB_CLAIM, ATOMIC
    std::vector<std::shared_ptr<ClaimNode>> children;
    std::vector<std::string> supporting_documents;
};

class HierarchicalClaimExtractor {
public:
    std::shared_ptr<ClaimNode> extractHierarchical(const std::string& answer) {
        auto root = std::make_shared<ClaimNode>();
        root->level = ClaimGranularity::DOCUMENT;

        // Level 1: Split into sentences
        auto sentences = splitSentences(answer);
        for (const auto& sent : sentences) {
            auto sent_node = std::make_shared<ClaimNode>();
            sent_node->text = sent;
            sent_node->level = ClaimGranularity::SENTENCE;

            // Level 2: Extract sub-claims
            auto sub_claims = extractSubClaims(sent);
            for (const auto& sub : sub_claims) {
                auto sub_node = std::make_shared<ClaimNode>();
                sub_node->text = sub;
                sub_node->level = ClaimGranularity::SUB_CLAIM;

                // Level 3: Atomic claims
                auto atomic = extractAtomicClaims(sub);
                for (const auto& atom : atomic) {
                    auto atom_node = std::make_shared<ClaimNode>();
                    atom_node->text = atom;
                    atom_node->level = ClaimGranularity::ATOMIC;
                    sub_node->children.push_back(atom_node);
                }

                sent_node->children.push_back(sub_node);
            }

            root->children.push_back(sent_node);
        }

        return root;
    }
};
```

**Benefit:** More precise verification at atomic level

---

### 9. Contrastive Explanation Generation
**Priority:** Low
**Target Version:** v1.18.0
**File:** `response_parser.cpp`

Generate explanations showing what would improve score.

```cpp
struct ContrastiveExplanation {
    std::string actual_reasoning;
    std::string what_was_good;
    std::string what_was_bad;
    std::string how_to_improve;
    double score_if_improved;
};

ContrastiveExplanation generateContrastive(const EvaluationResult& result) {
    ContrastiveExplanation expl;

    expl.actual_reasoning = result.explanation;

    // Analyze what worked
    if (!result.verified_claims.empty()) {
        expl.what_was_good = "The following claims were well-supported: " +
                             join(result.verified_claims, ", ");
    }

    // Analyze what didn't work
    if (!result.unverified_claims.empty()) {
        expl.what_was_bad = "The following claims lacked support: " +
                           join(result.unverified_claims, ", ");
    }

    // Suggest improvements
    expl.how_to_improve = "To improve the score:\n";
    for (const auto& claim : result.unverified_claims) {
        expl.how_to_improve += "- Add citation for: " + claim + "\n";
    }

    // Estimate improved score
    expl.score_if_improved = estimateImprovedScore(result);

    return expl;
}
```

---

### 10. Adversarial Robustness Testing
**Priority:** Low
**Target Version:** v1.18.0
**File:** New file `adversarial_tester.cpp`

Systematically test RAG robustness against adversarial inputs.

```cpp
class AdversarialTester {
public:
    struct RobustnessReport {
        double robustness_score;
        std::vector<std::string> vulnerabilities;
        std::vector<AdversarialExample> failing_examples;
    };

    RobustnessReport testRobustness(RAGJudge& judge) {
        RobustnessReport report;

        // Test 1: Query perturbations
        auto perturbed = generatePerturbedQueries(base_queries_);
        testQueryPerturbations(judge, perturbed, report);

        // Test 2: Document poisoning
        auto poisoned = generatePoisonedDocuments(base_documents_);
        testDocumentPoisoning(judge, poisoned, report);

        // Test 3: Prompt injection attempts
        testPromptInjection(judge, report);

        // Test 4: Context overflow
        testContextOverflow(judge, report);

        return report;
    }

private:
    void testQueryPerturbations(RAGJudge& judge,
                                 const std::vector<Query>& perturbed,
                                 RobustnessReport& report) {
        for (const auto& query : perturbed) {
            auto original_result = judge.evaluate(query.original);
            auto perturbed_result = judge.evaluate(query.perturbed);

            double score_diff = std::abs(
                original_result.overall_score - perturbed_result.overall_score
            );

            if (score_diff > 0.3) {
                report.vulnerabilities.push_back(
                    "Large score change for minor query perturbation"
                );
            }
        }
    }
};
```

---

## Code Quality Improvements

### 11. Comprehensive Error Handling
**Priority:** High
**Target Version:** v1.16.0
**Files:** All files

Add proper error handling instead of silent failures.

**Current:**
```cpp
auto embedding = getEmbedding(text);  // May fail silently
double sim = cosineSimilarity(emb1, embedding);
```

**Improved:**
```cpp
auto embedding_result = getEmbedding(text);
if (!embedding_result) {
    throw RAGException("Failed to compute embedding: " +
                      embedding_result.error());
}

auto sim_result = cosineSimilarity(emb1, *embedding_result);
if (!sim_result) {
    throw RAGException("Failed to compute similarity: " +
                      sim_result.error());
}
```

---

### 12. Logging and Observability
**Priority:** High
**Target Version:** v1.16.0
**Files:** All files

Add structured logging for debugging and monitoring.

```cpp
THEMIS_INFO("RAG evaluation started", {
    {"query", query},
    {"num_documents", documents.size()},
    {"mode", modeToString(config_.mode)}
});

auto start = std::chrono::steady_clock::now();
auto result = evaluateInternal(input);
auto duration = std::chrono::steady_clock::now() - start;

THEMIS_INFO("RAG evaluation completed", {
    {"duration_ms", durationMs(duration)},
    {"overall_score", result.overall_score},
    {"passed", result.passed_quality_threshold}
});

if (!result.passed_quality_threshold) {
    THEMIS_WARN("Quality threshold not met", {
        {"threshold", config_.quality_threshold},
        {"actual", result.overall_score},
        {"unverified_claims", result.unverified_claims.size()}
    });
}
```

---

### 13. Memory Pool for Frequent Allocations
**Priority:** Medium
**Target Version:** v1.17.0
**Files:** `claim_extractor.cpp`, `response_parser.cpp`

Reduce allocation overhead for string-heavy operations.

```cpp
class MemoryPool {
public:
    template<typename T>
    T* allocate() {
        if (free_list_.empty()) {
            return new T();
        }
        auto ptr = free_list_.back();
        free_list_.pop_back();
        return static_cast<T*>(ptr);
    }

    template<typename T>
    void deallocate(T* ptr) {
        free_list_.push_back(ptr);
    }

private:
    std::vector<void*> free_list_;
};
```

---

## Documentation Improvements

### 14. Inline Documentation for Complex Algorithms
**Priority:** Medium
**Target Version:** v1.16.0

Add detailed comments explaining non-obvious algorithms.

**Example:**
```cpp
/**
 * Compute self-consistency score using majority voting.
 *
 * Algorithm:
 * 1. Generate N samples with varying temperature
 * 2. Extract key claims from each sample
 * 3. Cluster similar claims (cosine similarity > 0.9)
 * 4. Consistency = (size of largest cluster) / N
 *
 * References:
 * - Wang et al., "Self-Consistency Improves Chain of Thought Reasoning"
 *   (ICLR 2023)
 *
 * @param samples Vector of N generated answers
 * @return Consistency score in [0, 1]
 */
double computeSelfConsistency(const std::vector<std::string>& samples);
```

---

## Testing Improvements

### 15. Property-Based Testing
**Priority:** Medium
**Target Version:** v1.17.0

Add property-based tests to catch edge cases.

```cpp
#include <rapidcheck.h>

TEST_CASE("Faithfulness score properties") {
    rc::check([](const std::string& query,
                 const std::vector<std::string>& docs,
                 const std::string& answer) {
        auto judge = RAGJudgeFactory::createBalanced();
        auto result = judge->evaluate(query, docs, answer);

        // Property 1: Score in [0, 1]
        RC_ASSERT(result.faithfulness_score >= 0.0);
        RC_ASSERT(result.faithfulness_score <= 1.0);

        // Property 2: Empty answer → 1.0 (no claims)
        if (answer.empty()) {
            RC_ASSERT(result.faithfulness_score == 1.0);
        }

        // Property 3: Adding more documents shouldn't decrease score
        auto more_docs = docs;
        more_docs.push_back("Additional context");
        auto result2 = judge->evaluate(query, more_docs, answer);
        RC_ASSERT(result2.faithfulness_score >= result.faithfulness_score);
    });
}
```

---

## Deployment Improvements

### 16. Model Quantization for Faster Inference
**Priority:** High
**Target Version:** v1.16.0

Quantize evaluation models to INT8 for 2-4x speedup.

```cpp
class QuantizedNLIModel {
public:
    QuantizedNLIModel(const std::string& model_path) {
        // Load INT8 quantized ONNX model
        session_options_.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL
        );
        session_options_.SetIntraOpNumThreads(4);

        session_ = std::make_unique<Ort::Session>(
            env_, model_path.c_str(), session_options_
        );
    }

    // 2-4x faster than FP32 with minimal accuracy loss
};
```

---

*Last Updated: 2024*
*Implementation Roadmap for v1.16-v1.18*

---

## Test Strategy

- Unit test coverage ≥ 80% for all evaluator components (`FaithfulnessEvaluator`, `RelevanceEvaluator`, `CoherenceEvaluator`, `CompletenessEvaluator`, `BiasDetector`)
- `StreamingRetriever`: 28+ test cases covering incremental chunk delivery, context-window overflow, and empty-result streams
- `CrossEncoderReranker`: 30+ test cases including ONNX model path and heuristic fallback when model is absent
- `HybridRetriever`: 31+ test cases covering RRF weight extremes (0.0/1.0), BM25-only, vector-only, and tie-breaking
- End-to-end pipeline integration test: retrieve → generate → evaluate cycle with a 100-document corpus; assert faithfulness ≥ 0.8 and relevance ≥ 0.75
- Agentic RAG loop test: inject a tool that requires 3 hops; assert correct final answer and that the max-iteration guard fires at the configured limit
- Hallucination dashboard rolling-window test: inject 1,000 evaluation events and assert rolling rate matches ground-truth within ± 0.5%
- Citation highlighter tests: assert each answer sentence maps to ≥ 1 source chunk with token-overlap ≥ 0.3

## Performance Targets

- Fast evaluation mode: ≤ 100 ms end-to-end at p99 (single judge, no LLM call, heuristic fallback)
- Balanced evaluation mode: ≤ 500 ms end-to-end at p99 (multi-judge with one LLM call)
- Thorough evaluation mode: ≤ 2,000 ms end-to-end at p99 (full multi-judge + CoT + G-Eval)
- `HybridRetriever` recall@10: ≥ 85% on the BEIR benchmark NQ split
- `CrossEncoderReranker` MRR@10 improvement: ≥ +10% relative vs. BM25-only baseline
- `StreamingRetriever` first-chunk latency: ≤ 50 ms from query submission to first retrieved chunk
- Bayesian optimizer convergence: reach ≥ 90% of optimal retrieval F1 within 200 feedback events
- `ClaimExtractor` throughput: ≤ 500 ms for a 1,000-character answer with LLM-first path; ≤ 50 ms with heuristic fallback

## Security / Reliability

- Prompt injection in retrieved context: all retrieved text chunks must be sanitized to remove instruction-format delimiters (e.g., `###`, `<|system|>`) before injection into judge prompts
- LLM judge availability: all evaluators must degrade gracefully to heuristic scoring when the LLM judge endpoint returns an error or exceeds the 2 s timeout; no evaluation call may throw an unhandled exception
- Citation data must never include raw file-system paths or internal document IDs in API responses; only chunk references are exposed
- Agentic RAG tool-use must validate all tool inputs against a JSON schema before invocation; malformed inputs are rejected and logged, not silently ignored
- `HallucinationDashboard` rolling-window data must not be persisted to disk in unencrypted form; in-memory only by default
- Evaluator score tampering: evaluation reports must include an HMAC over the score payload when transmitted across service boundaries
- Knowledge-graph entity linking must enforce a maximum graph traversal depth of 5 hops to prevent query amplification attacks

---

## Paper 1+2 — Loop Orchestration, Explainability & Federated RAG (IMPL-A2, IMPL-A3, IMPL-B9)

> Full papers: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md` · `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md`
> See also: `include/rag/FUTURE_ENHANCEMENTS.md` §Paper 1+2

### Loop 1–4 Explicit Orchestration (IMPL-A2)
- `ContinuousLearningOrchestrator` gains `triggerLoop1…4()` with loop-interference cooldown guard
- `RAGIngestionBridge` indexes optimizer-log documents for Loop 4 dataset enrichment

### Federated RLAIF (IMPL-A3)
- `FEDERATED_ROUND_START` event fires after Loop 4 (24 h guard)
- `RLAIFTrainer` preference dataset propagated via `CrossShardFeedbackSync`

### ExplainabilityReasonBuilder (IMPL-B9)
- `CausalChain` generated for every autonomous loop decision (rule-based, ≤ 20 ms)
- Written to `AIDecisionAuditor`; GDPR guard enforces no PII in chain
