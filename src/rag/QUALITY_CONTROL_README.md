> **Status:** 2026-04-19 – Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos ggf. korrigiert.

# RAG Quality Control Implementation

This directory contains the implementation of the post-generation quality control system for RAG (Retrieval-Augmented Generation).

## Components

### Core Files

#### LLM Judge Client
- **Header**: `include/rag/llm_judge_client.h`
- **Source**: `src/rag/llm_judge_client.cpp`
- **Purpose**: Connects evaluation prompts to InferenceEngineEnhanced
- **Features**: Caching, batching, multi-model support

#### G-Eval Evaluator
- **Header**: `include/rag/geval_evaluator.h`
- **Source**: `src/rag/geval_evaluator.cpp`
- **Purpose**: Probabilistic scoring using token probabilities
- **Features**: Continuous scores, confidence estimation

#### NLI Faithfulness Verifier
- **Header**: `include/rag/nli_faithfulness_verifier.h`
- **Source**: `src/rag/nli_faithfulness_verifier.cpp`
- **Purpose**: Claim-level faithfulness verification using NLI
- **Features**: Entailment checking, contradiction detection

#### Quality Control Pipeline
- **Header**: `include/rag/quality_control_pipeline.h`
- **Source**: `src/rag/quality_control_pipeline.cpp`
- **Purpose**: Multi-stage orchestration of quality checks
- **Features**: Fast/Balanced/Thorough modes, auto-retry, learning feedback

### Modified Files

#### RAG Judge
- **File**: `src/rag/rag_judge.cpp`
- **Changes**: Added LLM Judge Client integration

#### Faithfulness Evaluator
- **File**: `src/rag/faithfulness_evaluator.cpp`
- **Changes**: Added NLI verifier support

## Architecture

```
Quality Control Pipeline
├── Stage 1: Fast Screening (<50ms)
│   └── LLM Judge (faithfulness only)
├── Stage 2: Balanced Evaluation (<500ms)
│   ├── LLM Judge (multi-dimension)
│   └── G-Eval (probabilistic scoring)
├── Stage 3: Thorough Verification (<2s)
│   ├── NLI Faithfulness Verifier
│   └── Claim-level analysis
└── Stage 4: Continuous Learning
    └── Feedback to learning orchestrator
```

## Usage

### Quick Start

```cpp
#include "rag/quality_control_pipeline.h"

// Create production pipeline
auto pipeline = QualityPipelineFactory::createProduction();

// Run quality control
auto result = pipeline->runQualityControl(query, answer, documents);

if (result.status == QualityGateStatus::PASSED) {
    // Answer passed quality checks
}
```

### Custom Configuration

```cpp
QualityControlPipeline::Config config;
config.enable_fast_stage = true;
config.enable_balanced_stage = true;
config.enable_thorough_stage = true;
config.fast_stage_threshold = 0.6;
config.balanced_stage_threshold = 0.7;
config.thorough_stage_threshold = 0.8;

auto pipeline = std::make_unique<QualityControlPipeline>(config);
```

## Testing

### Unit Tests
```bash
./tests/test_quality_control_pipeline
```

### Demo Example
```bash
./examples/quality_control_demo
```

## Performance

| Mode      | Target   | Typical   | Components                        |
|-----------|----------|-----------|-----------------------------------|
| Fast      | <50ms    | ~30ms     | LLM Judge (faithfulness)          |
| Balanced  | <500ms   | ~300ms    | LLM Judge + G-Eval                |
| Thorough  | <2s      | ~1.2s     | LLM Judge + G-Eval + NLI          |

## Quality Dimensions

1. **Faithfulness** (35%): Claims supported by documents
2. **Relevance** (25%): Answer addresses query
3. **Completeness** (15%): All query aspects covered
4. **Coherence** (15%): Well-structured and logical
5. **Ethical Compliance** (10%): Respects human autonomy

## Integration Points

### With InferenceEngineEnhanced
- LLM Judge Client uses InferenceEngineEnhanced for LLM calls
- Supports context caching and batch processing
- Load balancing across multiple models

### With Continuous Learning
- Quality scores sent as feedback
- Low-quality patterns identified
- Training data updated
- Models retrained periodically

### With RAG Judge
- Extends existing RAG Judge functionality
- Adds real LLM integration
- Enhances faithfulness evaluation with NLI

## Configuration Files

Configuration can be provided via:
- C++ Config structs
- JSON configuration files
- Environment variables

Example JSON config:
```json
{
  "quality_pipeline": {
    "enable_fast_stage": true,
    "enable_balanced_stage": true,
    "enable_thorough_stage": true,
    "fast_stage_threshold": 0.6,
    "balanced_stage_threshold": 0.7,
    "thorough_stage_threshold": 0.8,
    "enable_auto_retry": true,
    "max_retries": 2
  }
}
```

## Dependencies

- InferenceEngineEnhanced (for LLM calls)
- llama.cpp (for token probabilities)
- nlohmann/json (for JSON parsing)
- spdlog (for logging)

## Future Enhancements

1. **Advanced NLI Models**
   - RoBERTa-large-MNLI integration
   - DeBERTa-v3 support
   - Custom fine-tuned models

2. **Calibration**
   - Temperature scaling
   - Platt scaling

3. **Multi-Model Ensemble**
   - Multiple judge models
   - Voting strategies

4. **Active Learning**
   - Uncertain case identification
   - Human feedback loop

## Documentation

- Main documentation: `docs/quality_control_pipeline.md`
- API reference: Auto-generated from headers
- Examples: `examples/quality_control_demo.cpp`

## License

See main ThemisDB LICENSE file.
