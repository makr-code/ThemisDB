# RAG Module Implementation - Future Enhancements

This document outlines planned implementation improvements for the RAG module source files.

## Performance Optimizations

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
