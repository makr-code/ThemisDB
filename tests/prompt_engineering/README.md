# tests/prompt_engineering

Focused module test folder for `src/prompt_engineering`.

## Focused Test Suite (PE-FT-001..PE-FT-015)

The focused test suite provides production readiness validation for the prompt_engineering module:

- **test_prompt_engineering_focused.cpp** (15 test cases)
  - Template lifecycle management (create, get, list, list templates)
  - Context injection and validation
  - Version control commit and history operations
  - Feedback collection and statistics
  - Optimizer basic optimization loop
  - Evaluator structural evaluation
  - Metrics recording and retrieval
  - Error handling for missing templates and invalid context
  - Concurrency sanity checks
  - Injection validation edge cases
  - Consistency checks across multiple invocations

### Build and Test

```bash
# Configure with tests enabled
cmake --preset=community-release-allow-missing-rocksdb \
  -DTHEMIS_BUILD_TESTS=ON

# Build focused test
cmake --build build-community-release-allow-missing-rocksdb \
  --target module_prompt_engineering_test_prompt_engineering_focused_focused

# Run focused test
ctest --preset=community-release-allow-missing-rocksdb \
  -L prompt_engineering --output-on-failure
```

### Test Results

All 15 PE-FT test cases validate core prompt_engineering surfaces:

| Surface | Test ID | Coverage |
|---------|---------|----------|
| PromptManager | PE-FT-001, PE-FT-002, PE-FT-003 | Template lifecycle, context injection, validation |
| PromptVersionControl | PE-FT-004, PE-FT-013 | Commit, history, consistency |
| FeedbackCollector | PE-FT-005 | Recording and statistics |
| PromptOptimizer | PE-FT-006, PE-FT-014 | Optimization loop, diagnostics |
| PromptEvaluator | PE-FT-007, PE-FT-015 | Structural evaluation, consistency |
| PromptEngineeringMetrics | PE-FT-008 | Metrics recording |
| Error Handling | PE-FT-009, PE-FT-010, PE-FT-012 | Missing templates, invalid context, edge cases |
| Concurrency | PE-FT-011 | Basic concurrent access sanity |

