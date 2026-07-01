# Phase 2.1 Implementation Guide - Test & Code Structure

**Prepared for:** ThemisDB Development Team  
**Focus:** RotatE Knowledge Graph Completion Implementation  
**Test File:** `tests/graph/test_rotate_completion.cpp`  
**Total Tests:** 16 (KGC-01 through KGC-16)

---

## Required Components for Implementation

Based on test file analysis, the following headers/modules must be implemented:

### 1. Header: `graph/rotate_completion.h`

**Required Classes:**

#### RotatEConfig
```cpp
struct RotatEConfig {
    size_t embedding_dim;      // Embedding dimension (e.g., 4)
    size_t neg_samples;        // Negative samples per batch
    size_t epochs;             // Training epochs (e.g., 5)
    float learning_rate;       // Learning rate (e.g., 1e-3)
    size_t batch_size;         // Batch size (e.g., 8)
};
```

#### RotatEModel
```cpp
class RotatEModel {
public:
    explicit RotatEModel(const RotatEConfig& cfg);
    
    // Entity/Relation management
    size_t addEntity(const std::string& name);
    size_t addRelation(const std::string& name);
    size_t entityCount() const;
    size_t relationCount() const;
    
    // Training
    struct TrainResult {
        bool success;
        size_t entities;
        size_t relations;
        size_t triples;
        size_t epochs_run;
    };
    TrainResult train(const std::vector<KGTriple>& triples);
    bool isTrained() const;
    
    // Scoring
    double score(const std::string& head, 
                 const std::string& relation, 
                 const std::string& tail);
    
    // Embeddings
    std::vector<float> entityEmbedding(const std::string& entity);
    std::vector<float> relationPhase(const std::string& relation);
};
```

#### KGTriple
```cpp
struct KGTriple {
    std::string head;
    std::string relation;
    std::string tail;
};
```

#### LinkPredictionHead
```cpp
class LinkPredictionHead {
public:
    explicit LinkPredictionHead(const RotatEModel& model);
    
    struct Prediction {
        std::string entity;
        double score;
        double rank;  // 1-based sequential
    };
    
    std::vector<Prediction> predictTail(
        const std::string& head,
        const std::string& relation,
        size_t top_k);
    
    std::vector<Prediction> predictHead(
        const std::string& relation,
        const std::string& tail,
        size_t top_k);
};
```

#### KGCompletionEngine
```cpp
class KGCompletionEngine {
public:
    explicit KGCompletionEngine(const RotatEConfig& cfg);
    
    // Entity/Relation registration (delegates to model)
    size_t addEntity(const std::string& name);
    size_t addRelation(const std::string& name);
    
    // Training
    RotatEModel::TrainResult train(const std::vector<KGTriple>& triples);
    
    // Prediction with reasoner injection
    std::vector<LinkPredictionHead::Prediction> completeTail(
        const std::string& head,
        const std::string& relation,
        size_t top_k);
    
    std::vector<LinkPredictionHead::Prediction> completeHead(
        const std::string& relation,
        const std::string& tail,
        size_t top_k);
    
    // Reasoner integration
    void setReasoner(KnowledgeGraphReasoner* reasoner, double threshold);
};
```

### 2. Header: `graph/knowledge_graph_reasoner.h`

**Required Class:**

#### KnowledgeGraphReasoner
```cpp
class KnowledgeGraphReasoner {
public:
    // Add inferred facts from link prediction
    void addFact(const KGTriple& triple, double confidence);
    
    // Query interface
    size_t derivedTripleCount() const;
    
    // No exception should be thrown when adding facts
    // (addFact must handle all inputs gracefully)
};
```

---

## Test Execution Flow

### Phase 1: Model Basics (KGC-01 to KGC-03)
```cpp
RotatEModel model(config);
size_t e1 = model.addEntity("alice");
size_t e2 = model.addEntity("bob");
ASSERT_NE(e1, e2);  // Unique indices
ASSERT_EQ(model.addEntity("alice"), e1);  // Same for duplicates
ASSERT_EQ(model.entityCount(), 2);
```

### Phase 2: Error Handling (KGC-04 to KGC-05)
```cpp
// Scenario 1: Unknown entity
EXPECT_THROW(model.score("unknown", "knows", "bob"), std::out_of_range);

// Scenario 2: Untrained model
RotatEModel fresh(config);
fresh.addEntity("alice");
EXPECT_THROW(fresh.score("alice", "knows", "bob"), std::runtime_error);
```

### Phase 3: Training (KGC-06 to KGC-07)
```cpp
std::vector<KGTriple> triples = {
    {"alice", "knows", "bob"},
    {"bob", "knows", "carol"}
};
auto result = model.train(triples);
ASSERT_TRUE(result.success);
ASSERT_TRUE(model.isTrained());
ASSERT_EQ(result.epochs_run, config.epochs);
```

### Phase 4: Scoring (KGC-08 to KGC-09)
```cpp
double s1 = model.score("alice", "knows", "bob");
double s2 = model.score("alice", "knows", "bob");
EXPECT_TRUE(std::isfinite(s1));
EXPECT_GE(s1, 0.0);
EXPECT_DOUBLE_EQ(s1, s2);  // Deterministic
```

### Phase 5: Embeddings (KGC-10 to KGC-11)
```cpp
auto emb = model.entityEmbedding("alice");
ASSERT_EQ(emb.size(), 2 * config.embedding_dim);  // RotatE uses 2*dim for complex numbers

auto phase = model.relationPhase("knows");
ASSERT_EQ(phase.size(), config.embedding_dim);
```

### Phase 6: Link Prediction (KGC-12 to KGC-13)
```cpp
LinkPredictionHead predictor(model);
auto preds = predictor.predictTail("alice", "knows", 3);

// Verify sorting and ranking
for (size_t i = 1; i < preds.size(); ++i) {
    EXPECT_LE(preds[i-1].score, preds[i].score);  // Ascending by score
}
for (size_t i = 0; i < preds.size(); ++i) {
    EXPECT_EQ(preds[i].rank, i + 1);  // 1-based rank
}
```

### Phase 7: Integration (KGC-14 to KGC-16)
```cpp
KGCompletionEngine engine(config);
// ... populate and train ...

KnowledgeGraphReasoner reasoner;
engine.setReasoner(&reasoner, 1e9);  // High threshold: all predictions injected

auto preds = engine.completeTail("alice", "knows", 3);
// Predictions should have been injected into reasoner without exception
```

---

## Critical Implementation Notes

### 1. RotatE Model Theory

**Representation:** Each entity as complex vector: `(a, b)` where embedding_dim determines dimensions

**Relation:** Phase rotation in complex space

**Score Function:** Distance/dissimilarity in rotated space (lower = better)

**Key Invariant:** Score must always be non-negative and finite

### 2. Test Assertions to Watch

| Assertion Type | What It Tests | Implementation Requirement |
|---|---|---|
| `EXPECT_NE(i0, i1)` | Unique indices | Entity/relation IDs must be distinct |
| `EXPECT_EQ(first, second)` | Deduplication | Maintain entity/relation lookup table |
| `EXPECT_THROW(..., exception_type)` | Error handling | Proper exception propagation |
| `EXPECT_TRUE(std::isfinite(s))` | Numerical stability | No NaN/Inf in scores |
| `EXPECT_DOUBLE_EQ(s1, s2)` | Determinism | Same random seed or no randomness in scoring |
| `EXPECT_LE(preds[i].score, preds[i+1].score)` | Sorting | Results ranked by ascending score |
| `EXPECT_EQ(preds[i].rank, i+1)` | Sequential ranking | Rank = 1-based position |
| `EXPECT_GT(fabs(short_score - long_score), 1e-6)` | Training effect | Different epochs → different scores |

### 3. Configuration Expectations

The test uses `smallCfg()`:
```cpp
RotatEConfig cfg;
cfg.embedding_dim = 4;    // Small for test speed
cfg.neg_samples = 4;      // Negative sampling count
cfg.epochs = 5;           // 5 epochs for training
cfg.learning_rate = 1e-3; // 0.001 gradient step
cfg.batch_size = 8;       // Process 8 triples per batch
```

**Test Data:**
- 3 entities: alice, bob, carol
- 1 relation: knows
- 2 triples: (alice, knows, bob), (bob, knows, carol)

### 4. Potential Edge Cases

1. **Unregistered Entities Before Training:**
   - addEntity/addRelation creates indices BEFORE training
   - score() must throw if entity not registered

2. **Training State Transitions:**
   - Freshly created model: isTrained() = false
   - After train(): isTrained() = true
   - score() must check this state

3. **Embedding Dimensions:**
   - Entity embeddings: ALWAYS 2 * embedding_dim (complex representation)
   - Relation embeddings: ALWAYS embedding_dim

4. **Prediction Sorting:**
   - Results must be sorted ascending by score
   - Rank must be 1-based sequential (1, 2, 3, ...)
   - Request for k=3 entities but only 3 total → returns all 3

5. **Reasoner Integration:**
   - No exception should be thrown by addFact()
   - Threshold parameter controls which predictions are injected
   - High threshold (1e9) = inject all; zero = inject none

---

## Build & Test Commands (Once RocksDB is Available)

```bash
# Configure
cmake --preset community-release

# Build tests only
cmake --build build-community-release --target rotate_completion

# Run all rotating_completion tests
ctest --preset community-release -R rotate_completion --output-on-failure

# Run specific test
ctest --preset community-release -R "RotatEModelTest.KGC_01" --output-on-failure

# Verbose output
ctest --preset community-release -R rotate_completion -V
```

---

## Dependency Resolution Path

```
ThemisDB Build
├── CMake (v3.31.6) ✅
├── Ninja (v1.13.2) ✅
├── GCC 13.3.0 ✅
└── Dependencies
    ├── OpenSSL 3.0.13 ✅
    ├── zlib1g-dev ✅
    ├── RocksDB ❌ NEEDED: librocksdb-dev
    ├── gtest ✅ (will be built by CMake)
    └── simdjson, TBB (optional)
```

**Blocker:** `librocksdb-dev` (see main baseline report for installation options)

---

## Success Criteria for Phase 2.1

All 16 tests passing:
- ✅ KGC-01: Unique entity/relation indices
- ✅ KGC-02: Deduplication on repeated registration
- ✅ KGC-03: Count tracking
- ✅ KGC-04: Error on unknown entity
- ✅ KGC-05: Error when untrained
- ✅ KGC-06: Training succeeds
- ✅ KGC-07: Training reports correct metadata
- ✅ KGC-08: Scores are finite and non-negative
- ✅ KGC-09: Scoring is deterministic
- ✅ KGC-10: Entity embedding dimensions (2*dim)
- ✅ KGC-11: Relation embedding dimensions (dim)
- ✅ KGC-12: Tail prediction sorted by score
- ✅ KGC-13: Head prediction sorted by score
- ✅ KGC-14: Reasoner receives predictions
- ✅ KGC-15: completeHead delegates correctly
- ✅ KGC-16: Epoch count affects learned scores

**Gate:** All 16 tests must pass without exceptions or assertion failures

---

## Reference Files Location

- **Test File:** `tests/graph/test_rotate_completion.cpp` (453 lines)
- **Include Path:** `include/graph/` (for header files to be created)
- **Source Path:** `src/graph/` (for implementation files)
- **Build Config:** `cmake/Dependencies.cmake`, `CMakeLists.txt`

---

**Prepared By:** Build & Test Integration Specialist  
**Status:** Ready for implementation team handoff  
**Next Step:** Resolve RocksDB dependency, then implement components in test order
