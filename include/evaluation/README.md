> **Build:** `cmake --preset release && cmake --build build/release`

# Evaluation Module — Public Headers

**Module Path:** `include/evaluation/`
**Implementation:** `../../src/evaluation/`

## Purpose

Public interfaces and declarations for ThemisDB's evaluation and quality assessment subsystem, providing tools for model evaluation, metric computation, and result validation.

## Canonical Module Documentation

`include/evaluation/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/evaluation/`:

- [`../../src/evaluation/README.md`](../../src/evaluation/README.md)
- [`../../src/evaluation/ARCHITECTURE.md`](../../src/evaluation/ARCHITECTURE.md)
- [`../../src/evaluation/ROADMAP.md`](../../src/evaluation/ROADMAP.md)
- [`../../src/evaluation/FUTURE_ENHANCEMENTS.md`](../../src/evaluation/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `evaluation.h` | `Evaluator` — core evaluation framework and metric computation |
| `metric_registry.h` | `MetricRegistry` — metric collection and aggregation |
| `result_validator.h` | `ResultValidator` — output validation and quality checks |
| `evaluation_context.h` | `EvaluationContext` — evaluation session management |
| `quality_gates.h` | `QualityGate` — acceptance criteria and threshold enforcement |

## Usage

```cpp
#include "evaluation/evaluation.h"

auto evaluator = themis::evaluation::createEvaluator({
    .metrics = {"accuracy", "precision", "recall"},
    .validation_enabled = true
});

auto results = evaluator->evaluate(model, test_data);
auto quality = evaluator->computeQuality(results);
```

For full runtime usage examples (metrics, validation, quality gates), see [`../../src/evaluation/README.md`](../../src/evaluation/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `evaluation.h` (`Evaluator::Config` for metric selection)
- `metric_registry.h` (custom metric registration)
- `result_validator.h` (`ResultValidator::Config` for validation rules)
- `quality_gates.h` (`QualityGate` threshold configuration)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-evaluation
```

## See Also

- [`../../src/evaluation/README.md`](../../src/evaluation/README.md) — implementation details
- [`../../src/training/README.md`](../../src/training/README.md) — model training integration

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
