# Quality Control Pipeline for RAG Systems

## Overview

The Quality Control Pipeline provides comprehensive post-generation quality assessment for RAG (Retrieval-Augmented Generation) systems. It implements state-of-the-art evaluation techniques including LLM-as-Judge, G-Eval, and NLI verification.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Quality Control Pipeline                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Stage 1: Fast Screening (<50ms)                            │
│  ┌────────────────────────────────────────────────────┐     │
│  │ • Quick faithfulness check (LLM Judge)             │     │
│  │ • Early rejection of bad answers                   │     │
│  └────────────────────────────────────────────────────┘     │
│                          ↓                                    │
│  Stage 2: Balanced Evaluation (<500ms)                      │
│  ┌────────────────────────────────────────────────────┐     │
│  │ • Multi-dimension LLM judging                      │     │
│  │   - Faithfulness, Relevance, Completeness          │     │
│  │ • G-Eval probabilistic scoring                     │     │
│  │ • Aggregate quality assessment                     │     │
│  └────────────────────────────────────────────────────┘     │
│                          ↓                                    │
│  Stage 3: Thorough Verification (<2s)                       │
│  ┌────────────────────────────────────────────────────┐     │
│  │ • NLI faithfulness verification                    │     │
│  │ • Claim-level entailment checking                  │     │
│  │ • Citation verification                            │     │
│  └────────────────────────────────────────────────────┘     │
│                          ↓                                    │
│  Stage 4: Continuous Learning                               │
│  ┌────────────────────────────────────────────────────┐     │
│  │ • Feedback to learning orchestrator                │     │
│  │ • Quality trend tracking                           │     │
│  │ • Model improvement loop                           │     │
│  └────────────────────────────────────────────────────┘     │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Components

### 1. LLM Judge Client

Connects evaluation prompts to the InferenceEngineEnhanced for efficient LLM-based evaluation.

**Features:**
- Integration with InferenceEngineEnhanced
- Context caching for repeated evaluations
- Batch processing support
- Configurable temperature and sampling

**Usage:**
```cpp
#include "rag/llm_judge_client.h"

LLMJudgeClient::Config config;
config.model_name = "gpt-4";
config.temperature = 0.3;
config.enable_caching = true;

auto client = std::make_shared<LLMJudgeClient>(config);

auto response = client->evaluateDimension(
    query, answer, documents, "faithfulness");
```

### 2. G-Eval Evaluator

State-of-the-art probabilistic scoring using token probabilities from the LLM.

**Features:**
- Token probability extraction
- Continuous scoring (0-1 instead of discrete levels)
- Multiple sample aggregation
- Confidence estimation

**Usage:**
```cpp
#include "rag/geval_evaluator.h"

GEvalEvaluator::Config config;
config.num_samples = 3;
config.aggregation = AggregationMethod::MEAN;

auto geval = std::make_shared<GEvalEvaluator>(config);

auto result = geval->evaluate(
    query, answer, documents, "faithfulness");

std::cout << "Score: " << result.geval_score << "\n";
std::cout << "Confidence: " << result.confidence << "\n";
```

### 3. NLI Faithfulness Verifier

Natural Language Inference for claim-level faithfulness verification.

**Features:**
- Claim extraction from answers
- Entailment checking using NLI models
- Support for RoBERTa-large-MNLI, DeBERTa
- Contradiction detection

**Usage:**
```cpp
#include "rag/nli_faithfulness_verifier.h"

NLIFaithfulnessVerifier::Config config;
config.entailment_threshold = 0.7;
config.max_claims = 10;

auto verifier = std::make_shared<NLIFaithfulnessVerifier>(config);

auto result = verifier->verify(answer, documents);

std::cout << "Faithfulness: " << result.faithfulness_score << "\n";
std::cout << "Supported claims: " << result.supported_claims << "/"
          << result.total_claims << "\n";
```

### 4. Quality Control Pipeline

Multi-stage orchestration with automatic retry and learning feedback.

**Features:**
- Configurable stages (fast/balanced/thorough)
- Quality gates with thresholds
- Automatic retry on failure
- Learning feedback integration
- Performance monitoring

**Usage:**
```cpp
#include "rag/quality_control_pipeline.h"

// Create production pipeline
auto pipeline = QualityPipelineFactory::createProduction();

// Set up callbacks
pipeline->setFailureCallback([](const QualityCheckResult& result) {
    // Handle failure
});

pipeline->setLearningCallback([](const std::string& query, 
                                const QualityCheckResult& result) {
    // Send to learning system
});

// Run quality control
auto result = pipeline->runQualityControl(query, answer, documents);

if (result.status == QualityGateStatus::PASSED) {
    // Answer passed quality checks
} else {
    // Handle failure, retry, or escalate
}
```

## Performance Targets

| Mode      | Target   | Components                        |
|-----------|----------|-----------------------------------|
| Fast      | <50ms    | LLM Judge (faithfulness only)     |
| Balanced  | <500ms   | LLM Judge + G-Eval                |
| Thorough  | <2s      | LLM Judge + G-Eval + NLI          |

## Quality Dimensions

1. **Faithfulness** (35%)
   - Are claims supported by retrieved documents?
   - Detected via LLM judge, G-Eval, and NLI

2. **Relevance** (25%)
   - Does the answer address the query?
   - Detected via LLM judge and G-Eval

3. **Completeness** (15%)
   - Are all aspects of the query covered?
   - Detected via LLM judge

4. **Coherence** (15%)
   - Is the answer well-structured and logical?
   - Detected via LLM judge

5. **Ethical Compliance** (10%)
   - Does it respect human autonomy?
   - Shows diverse perspectives?
   - Cites sources for ethical claims?

## Integration with Continuous Learning

The quality control pipeline integrates with the continuous learning orchestrator:

1. Quality scores are sent as feedback
2. Low-quality patterns are identified
3. Training data is updated
4. Models are retrained periodically
5. Quality improves over time

## Configuration

### Fast Mode (Screening)
```cpp
QualityControlPipeline::Config config;
config.enable_fast_stage = true;
config.enable_balanced_stage = false;
config.enable_thorough_stage = false;
config.fast_stage_threshold = 0.6;
```

### Balanced Mode
```cpp
QualityControlPipeline::Config config;
config.enable_fast_stage = true;
config.enable_balanced_stage = true;
config.enable_thorough_stage = false;
config.balanced_stage_threshold = 0.7;
```

### Thorough Mode
```cpp
QualityControlPipeline::Config config;
config.enable_fast_stage = true;
config.enable_balanced_stage = true;
config.enable_thorough_stage = true;
config.thorough_stage_threshold = 0.8;
```

### Production Mode
```cpp
auto pipeline = QualityPipelineFactory::createProduction();
// Enables all stages + learning feedback
```

## Quality Gates

Quality gates determine the status of an answer:

- **PASSED**: Meets all thresholds, ready for production
- **FAILED**: Fails quality checks, reject immediately
- **RETRY_NEEDED**: Below threshold but above retry threshold
- **ESCALATE**: Needs human review

## Automatic Retry

Configure automatic retry for low-quality answers:

```cpp
QualityControlPipeline::Config config;
config.enable_auto_retry = true;
config.max_retries = 2;
config.retry_threshold = 0.5;
```

## Example: Complete Integration

```cpp
#include "rag/quality_control_pipeline.h"

// Create pipeline
auto pipeline = QualityPipelineFactory::createProduction();

// Configure thresholds
QualityControlPipeline::Config config = pipeline->getConfig();
config.fast_stage_threshold = 0.65;
config.balanced_stage_threshold = 0.75;
config.thorough_stage_threshold = 0.85;
pipeline->setConfig(config);

// Set up learning feedback
pipeline->setLearningCallback([](const std::string& query,
                                const QualityCheckResult& result) {
    // Send to continuous learning orchestrator
    sendToLearningSystem(query, result);
});

// Main RAG loop
while (true) {
    auto query = getNextQuery();
    auto documents = retrieveDocuments(query);
    auto answer = generateAnswer(query, documents);
    
    // Quality control
    auto qc_result = pipeline->runQualityControl(query, answer, documents);
    
    if (qc_result.status == QualityGateStatus::PASSED) {
        returnToUser(answer);
    } else if (qc_result.status == QualityGateStatus::RETRY_NEEDED) {
        // Retry with better prompt
        answer = generateAnswer(query, documents, improved_prompt);
        qc_result = pipeline->runQualityControl(query, answer, documents);
        returnToUser(answer);
    } else {
        // Escalate or return fallback
        handleFailure(qc_result);
    }
}
```

## Testing

Run the test suite:
```bash
./tests/test_quality_control_pipeline
```

Run the demo:
```bash
./examples/quality_control_demo
```

## References

- **G-Eval**: Liu et al., "G-Eval: NLG Evaluation using GPT-4 with Better Human Alignment" (2023)
- **NLI for Faithfulness**: Honovich et al., "TRUE: Re-evaluating Factual Consistency" (2022)
- **LLM-as-Judge**: Zheng et al., "Judging LLM-as-a-Judge" (2023)

## Future Enhancements

1. **Advanced NLI Models**
   - Integration with RoBERTa-large-MNLI
   - Support for DeBERTa-v3
   - Custom fine-tuned models

2. **Calibration**
   - Temperature scaling for better confidence
   - Platt scaling for probability calibration

3. **Multi-Model Ensemble**
   - Use multiple judge models
   - Aggregate with voting strategies

4. **Active Learning**
   - Identify uncertain cases for human annotation
   - Improve models with human feedback

## License

See main ThemisDB LICENSE file.
