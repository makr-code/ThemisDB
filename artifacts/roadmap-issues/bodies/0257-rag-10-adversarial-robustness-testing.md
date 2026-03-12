### Context

This issue implements the roadmap item 'Adversarial Robustness Testing' for the rag domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.18.0.

Primary detail section: 10. Adversarial Robustness Testing

### Goal

Deliver the scoped changes for Adversarial Robustness Testing in src/rag/ and complete the linked detail section in a release-ready state for v1.18.0.

### Detailed Scope

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

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #257 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/rag/FUTURE_ENHANCEMENTS.md#10-adversarial-robustness-testing
- Source key: roadmap:257:rag:v1.18.0:10-adversarial-robustness-testing

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:257:rag:v1.18.0:10-adversarial-robustness-testing -->
<!-- roadmap-ref: row=257;module=rag;target=v1.18.0 -->
<!-- roadmap-detail: src/rag/FUTURE_ENHANCEMENTS.md#10-adversarial-robustness-testing -->
