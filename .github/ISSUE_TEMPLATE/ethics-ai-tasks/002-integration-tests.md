---
name: "[Ethics AI] Add Integration Tests"
about: Create comprehensive integration tests for Ethics AI Plugin
title: "[Ethics AI] Add integration tests for AQL functions and REST API"
labels: ethics-ai, testing, high-priority
assignees: ''
---

## 🎯 Objective

Create comprehensive integration tests to validate Ethics AI Plugin functionality end-to-end.

## 📋 Background

Currently only unit tests exist (44 tests for individual components). Need integration tests that validate:
- AQL function execution via QueryEngine
- REST API endpoints via HTTP server
- BaseEntity storage/retrieval
- Composability with other ThemisDB features

## 🔧 Tasks

### Test File Creation

- [ ] Create `tests/test_ethics_integration.cpp`
- [ ] Set up test fixtures with ThemisDB components
- [ ] Add to CMake test configuration

### AQL Function Tests

- [ ] Test ETHICS_MAKE_DECISION execution
- [ ] Test ETHICS_EVALUATE execution
- [ ] Test ETHICS_GET_ARGUMENTS with filters
- [ ] Test ETHICS_FIND_SIMILAR_DILEMMAS vector search
- [ ] Test ETHICS_TRAVERSE_CHAIN graph traversal
- [ ] Test ETHICS_LOAD_PROFILE profile loading
- [ ] Test ETHICS_BUILD_CONTEXT RAG context
- [ ] Test ETHICS_STATS statistics queries
- [ ] Test ETHICS_METRICS metrics generation

### REST API Tests

- [ ] Test POST /ethics/debate/init
- [ ] Test POST /ethics/decision/make
- [ ] Test POST /ethics/evaluation
- [ ] Test GET /ethics/arguments with query params
- [ ] Test POST /ethics/arguments/search
- [ ] Test GET /ethics/philosophies
- [ ] Test POST /ethics/rag/context
- [ ] Test GET /ethics/metrics

### Storage Tests

- [ ] Test argument storage in BaseEntity
- [ ] Test decision storage and retrieval
- [ ] Test philosophy profile persistence
- [ ] Test vector embedding storage (when integrated)
- [ ] Test graph edge creation (when integrated)

### Composability Tests

- [ ] Test ethics + process mining AQL composition
- [ ] Test ethics + LoRA function combination
- [ ] Test complex multi-feature queries

### Error Handling Tests

- [ ] Test invalid function parameters
- [ ] Test missing philosophy schools
- [ ] Test malformed REST requests
- [ ] Test storage failures
- [ ] Test timeout scenarios

## ✅ Acceptance Criteria

- [ ] All integration tests pass
- [ ] Code coverage > 80% for integration paths
- [ ] Tests run in CI/CD pipeline
- [ ] Test execution time < 5 minutes
- [ ] Clear test failure messages
- [ ] Documentation for running tests

## 🧪 Test Structure Example

```cpp
TEST(EthicsIntegration, MakeDecisionViaAQL) {
    // Setup
    auto storage = createTestStorage();
    auto query_engine = createQueryEngine(storage);
    
    // Execute AQL
    std::string aql = R"(
        RETURN ETHICS_MAKE_DECISION(
            "Test dilemma",
            ["kant", "utilitarianism"],
            "test_category",
            true
        )
    )";
    auto result = query_engine->execute(aql);
    
    // Verify
    ASSERT_TRUE(result["decision_text"].is_string());
    ASSERT_GT(result["confidence"].get<double>(), 0.0);
}
```

## 📚 References

- Unit tests: `tests/test_ethics_ai_types.cpp`, etc.
- Integration patterns: Other ThemisDB integration tests
- Component docs: `plugins/ethics_ai/README.md`

## ⏱️ Estimated Effort

**Total:** 4-6 hours

- Test infrastructure setup: 1 hour
- AQL function tests: 2 hours
- REST API tests: 1 hour
- Storage/composability tests: 1-2 hours

## 🏷️ Labels

- `ethics-ai`: Ethics AI Plugin feature
- `testing`: Test infrastructure
- `high-priority`: Important for quality assurance
