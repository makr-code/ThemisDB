# RAG Ethics Integration - Implementation Guide

## Overview

The RAG Ethics Integration adds ethical compliance evaluation and ethical perspective gap detection to ThemisDB's RAG (Retrieval-Augmented Generation) system. This ensures that AI-generated responses respect human autonomy, show moral diversity, and maintain proper citations for ethical claims.

## Components

### 1. LLMMetaAnalyzer Base Class

**Location:** `include/rag/llm_meta_analyzer.h`, `src/rag/llm_meta_analyzer.cpp`

Provides shared infrastructure for LLM-based meta-analysis, including:
- Prompt building and formatting
- Response parsing
- Score extraction
- Caching mechanisms

### 2. Ethical Compliance Evaluation

**Location:** `include/rag/rag_judge.h`, `src/rag/rag_judge.cpp`

Adds `ETHICAL_COMPLIANCE` as a 5th evaluation dimension to RAG Judge with three sub-components:

#### 2.1 Autonomy Respect (40% weight)
- **Patronizing Language Detection**: Identifies condescending phrases
- **Choice Preservation Check**: Ensures answers don't force opinions
- **Balanced Perspectives**: Requires at least 2 moral viewpoints

#### 2.2 Moral Diversity (30% weight)
- **Multi-Perspective Representation**: Detects various ethical frameworks
- **Bias Detection**: Identifies absolute statements without nuance
- **Framework Recognition**: Utilitarian, deontological, virtue ethics, rights-based, etc.

#### 2.3 Citation Quality (30% weight)
- **Source Attribution**: Verifies ethical claims cite sources
- **Authority Check**: Evaluates source credibility
- **Completeness**: Ensures all moral claims are properly cited

### 3. VETO Mechanism

If ethical compliance score falls below **0.70**, the response is automatically rejected:

```cpp
if (ethical_compliance_score < ethical_compliance_threshold) {
    result.passed_quality_threshold = false;
    // Log VETO decision with reasoning
}
```

### 4. Ethical Perspective Gap Detection

**Location:** `include/rag/knowledge_gap_detector.h`, `src/rag/knowledge_gap_detector.cpp`

Adds `ETHICAL_PERSPECTIVE_GAP` type to detect when:
- Query has ethical context (moral questions)
- Retrieved documents lack diverse ethical perspectives
- Fewer than 2 distinct moral frameworks are represented

## Configuration

### RAG Judge Configuration

```cpp
RAGJudgeConfig config;
config.enable_ethical_evaluation = true;
config.ethical_veto_power = true;
config.ethical_compliance_threshold = 0.7;

// Sub-dimension weights
config.autonomy_respect_weight = 0.40;
config.moral_diversity_weight = 0.30;
config.citation_quality_weight = 0.30;
```

### Knowledge Gap Detector Configuration

```cpp
KnowledgeGapConfig gap_config;
gap_config.enable_ethical_gap_detection = true;
gap_config.min_ethical_perspectives = 2;
gap_config.ethical_diversity_threshold = 0.6;
```

## Usage Examples

### Example 1: Evaluating Ethical Compliance

```cpp
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

// Create judge with ethical evaluation enabled
RAGJudgeConfig config;
config.enable_ethical_evaluation = true;
config.ethical_veto_power = true;
RAGJudge judge(config);

// Prepare input
EvaluationInput input;
input.query = "Is it ethical to use AI for surveillance?";
input.generated_answer = "From a utilitarian perspective, AI surveillance "
                        "can maximize public safety. However, rights-based "
                        "approaches emphasize privacy concerns. According to "
                        "Article 12 of the Universal Declaration of Human Rights, "
                        "individuals have the right to privacy.";

// Evaluate
auto result = judge.evaluate(input);

std::cout << "Ethical Compliance Score: " << result.ethical_compliance_score << "\n";
std::cout << "Respects Autonomy: " << result.respects_human_autonomy << "\n";
std::cout << "Shows Diversity: " << result.shows_moral_diversity << "\n";
std::cout << "Has Citations: " << result.has_ethical_citations << "\n";
std::cout << "Passed Threshold: " << result.passed_quality_threshold << "\n";
```

### Example 2: Detecting Ethical Perspective Gaps

```cpp
#include "rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

// Create detector
KnowledgeGapConfig config;
config.enable_ethical_gap_detection = true;
config.min_ethical_perspectives = 2;
KnowledgeGapDetector detector(config);

// Prepare query and documents
std::string query = "What are the ethical considerations of genetic engineering?";
std::vector<RetrievedDocument> documents = /* ... */;

// Detect gaps
auto result = detector.detectEthicalPerspectiveGap(query, documents);

if (result.gap_detected && result.gap_type == GapType::ETHICAL_PERSPECTIVE_GAP) {
    std::cout << "Ethical perspective gap detected!\n";
    std::cout << "Explanation: " << result.explanation << "\n";
    std::cout << "Recommendation: Expand search for diverse perspectives\n";
}
```

## Detected Ethical Frameworks

The system recognizes the following moral frameworks:

1. **Utilitarian/Consequentialist**: Focuses on outcomes and overall welfare
2. **Deontological**: Emphasizes duties, rules, and categorical imperatives
3. **Virtue Ethics**: Considers character traits and virtues
4. **Rights-Based**: Focuses on human rights and natural rights
5. **Care Ethics**: Emphasizes relationships and care
6. **Religious/Faith-Based**: Moral principles from religious traditions
7. **Cultural Relativism**: Context-dependent ethical considerations

## Patronizing Language Patterns

The system detects the following patronizing patterns:

- "you should know"
- "obviously"
- "clearly"
- "it's simple"
- "just do"
- "anyone can"
- "even you"
- "you must understand"

## Testing

Comprehensive test suite with 24 test cases:

```bash
# Build tests
cmake --build build --target test_rag_ethics

# Run tests
./build/tests/test_rag_ethics
```

### Test Categories

1. **Autonomy Respect Tests** (10 tests)
   - Patronizing language detection
   - Choice preservation
   - Balanced perspectives

2. **Moral Diversity Tests** (4 tests)
   - Perspective counting
   - Bias detection

3. **Citation Quality Tests** (2 tests)
   - Citation presence
   - Citation for ethical claims

4. **VETO Mechanism Tests** (2 tests)
   - VETO trigger
   - VETO bypass

5. **Ethical Gap Detection Tests** (5 tests)
   - Ethical query classification
   - Perspective gap detection
   - Diversity calculation

6. **Integration Tests** (1 test)
   - End-to-end ethical evaluation

## Performance Targets

- **Autonomy Assessment**: < 200ms
- **Moral Diversity Check**: < 300ms
- **Citation Quality**: < 200ms
- **Perspective Gap Detection**: < 100ms
- **Total Ethical Evaluation**: < 800ms

## Future Enhancements

1. **Audit Logging**: Comprehensive logging for all VETO decisions
2. **LLM Integration**: Use actual LLM calls for more sophisticated detection
3. **Custom Frameworks**: Support for user-defined ethical frameworks
4. **Multi-language Support**: Detection patterns in multiple languages
5. **Learning from Feedback**: Improve detection based on user corrections

## References

- Issue: [RAG-ETHICS] RAG Ethics Integration
- Documentation: `docs/de/llm/RAG_ETHICS_INTEGRATION_ANALYSIS.md`
- UN Declaration of Human Rights (UDHR)
- Constitutional AI (Anthropic, 2022)
- EU AI Act (2024)

## License

This implementation is part of ThemisDB and follows the same MIT license.
