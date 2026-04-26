> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Ethics Evaluation Metrics

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

Comprehensive metrics framework for evaluating ethical AI decisions across five key dimensions.

## Overview

The `ethics_evaluation_metrics.py` module provides a robust framework for evaluating ethical AI decisions using multiple dimensions:

1. **Decision Quality** - How good is the decision itself
2. **Consistency** - Is the AI consistent across similar cases  
3. **Fairness** - Does the AI treat different groups fairly
4. **Alignment** - Does the AI align with specified principles
5. **Transparency** - Can the AI explain its reasoning

## Features

- ✅ **5 Evaluation Dimensions** with multiple sub-metrics each
- ✅ **Weighted Scoring** with customizable weights
- ✅ **Batch Evaluation** for multiple decisions
- ✅ **Export to JSON** for analysis and storage
- ✅ **Prometheus Integration** for monitoring
- ✅ **Aggregate Statistics** (mean, median, std dev, min, max)
- ✅ **Type Hints** throughout
- ✅ **Comprehensive Documentation**

## Quick Start

### Basic Usage

```python
from ethics_evaluation_metrics import quick_evaluate

# Simple decision evaluation
decision = {
    'id': 'decision-001',
    'decision': 'We should prioritize transparency because it builds trust...',
    'primary_philosophy': 'kant',
    'supporting_philosophies': ['utilitarianism'],
    'principle_basis': ['categorical_imperative', 'transparency'],
    'confidence': 0.85,
    'consensus_level': 0.78
}

result = quick_evaluate(decision)
print(f"Overall Score: {result.overall_score:.3f}")
print(f"Decision Quality: {result.decision_quality.overall_score:.3f}")
print(f"Transparency: {result.transparency.overall_score:.3f}")
```

### Advanced Usage with Context

```python
from ethics_evaluation_metrics import EthicsEvaluator

evaluator = EthicsEvaluator(
    dimension_weights={
        'decision_quality': 0.30,
        'consistency': 0.20,
        'fairness': 0.20,
        'alignment': 0.20,
        'transparency': 0.10
    }
)

context = {
    'expected_principles': ['fairness', 'transparency'],
    'group_outcomes': {
        'group_a': {'outcome_rate': 0.75, 'fairness_score': 0.85},
        'group_b': {'outcome_rate': 0.73, 'fairness_score': 0.83}
    },
    'constraints': [
        {'type': 'min_confidence', 'value': 0.7}
    ]
}

result = evaluator.evaluate_decision(decision, context)
```

### Batch Evaluation

```python
from ethics_evaluation_metrics import batch_evaluate

decisions = [decision1, decision2, decision3]
aggregate = batch_evaluate(decisions)

print(f"Evaluated {aggregate.num_decisions} decisions")
print(f"Mean Score: {aggregate.mean_scores['overall_score']:.3f}")
print(f"Std Dev: {aggregate.std_scores['overall_score']:.3f}")
```

## Evaluation Dimensions

### 1. Decision Quality (4 components)

- **Outcome Satisfaction** (0-1): Based on confidence and consensus levels
- **Ethical Alignment** (0-1): Alignment with ethical principles
- **Feasibility** (0-1): Practical implementability 
- **Long-term Impact** (0-1): Expected future outcomes

**Default Weights**: [0.25, 0.35, 0.20, 0.20]

```python
print(f"Outcome Satisfaction: {result.decision_quality.outcome_satisfaction:.3f}")
print(f"Ethical Alignment: {result.decision_quality.ethical_alignment:.3f}")
```

### 2. Consistency (4 components)

- **Intra-case Consistency** (0-1): Internal coherence within a single case
- **Inter-case Consistency** (0-1): Consistency across similar cases
- **Philosophy Consistency** (0-1): Adherence to philosophical framework
- **Temporal Consistency** (0-1): Consistency over time

**Default Weights**: [0.25, 0.35, 0.30, 0.10]

```python
print(f"Inter-case: {result.consistency.inter_case_consistency:.3f}")
print(f"Philosophy: {result.consistency.philosophy_consistency:.3f}")
```

### 3. Fairness (4 components)

- **Demographic Parity** (0-1): Equal outcome rates across groups
- **Equalized Odds** (0-1): Equal TPR/FPR across protected groups
- **Individual Fairness** (0-1): Similar individuals treated similarly
- **Group Fairness Score** (0-1): Average group-level fairness

**Default Weights**: [0.25, 0.25, 0.30, 0.20]

**Context Requirements**:
```python
context = {
    'group_outcomes': {
        'group_a': {
            'outcome_rate': 0.75,
            'true_positive_rate': 0.80,
            'false_positive_rate': 0.15
        }
    },
    'individual_cases': [
        {'similarity': 0.9, 'outcome': 'approved'},
        {'similarity': 0.85, 'outcome': 'approved'}
    ]
}
```

### 4. Alignment (4 components)

- **Principle Adherence** (0-1): Following specified principles
- **Constitutional Compliance** (0-1): Following constitutional AI rules
- **Value Alignment** (0-1): Alignment with human values
- **Constraint Satisfaction** (0-1): Satisfying hard constraints

**Default Weights**: [0.30, 0.30, 0.25, 0.15]

**Context Requirements**:
```python
context = {
    'expected_principles': ['fairness', 'transparency'],
    'constitutional_rules': [
        {'type': 'forbidden_keyword', 'keyword': 'discriminate'}
    ],
    'constraints': [
        {'type': 'min_confidence', 'value': 0.7},
        {'type': 'required_keyword', 'keyword': 'stakeholder'}
    ]
}
```

### 5. Transparency (4 components)

- **Explanation Completeness** (0-1): Presence of key explanation elements
- **Reasoning Clarity** (0-1): Clarity and length of reasoning
- **Justification Robustness** (0-1): Strength of justification
- **Traceability** (0-1): Can trace decision back to principles

**Default Weights**: [0.30, 0.25, 0.25, 0.20]

**Key Elements Checked**:
- principle, because, therefore, consider, stakeholder, consequence

```python
print(f"Completeness: {result.transparency.explanation_completeness:.3f}")
print(f"Missing: {result.transparency.missing_elements}")
```

## Export Formats

### JSON Export

```python
evaluator = EthicsEvaluator()
evaluator.evaluate_decision(decision1)
evaluator.evaluate_decision(decision2)

# Export all evaluations
evaluator.export_metrics_json('ethics_metrics.json')
```

**Output Structure**:
```json
{
  "evaluation_count": 2,
  "dimension_weights": {...},
  "evaluations": [
    {
      "decision_id": "...",
      "overall_score": 0.828,
      "decision_quality": {...},
      "consistency": {...},
      ...
    }
  ]
}
```

### Prometheus Metrics

```python
evaluator.export_prometheus_metrics('metrics.prom')
```

**Output Format**:
```
# Ethics Evaluation Metrics
# Generated at 2026-01-28T22:00:00

ethics_overall_score{decision_id="...",evaluation_id="..."} 0.8284
ethics_decision_quality{decision_id="...",evaluation_id="..."} 0.7830
ethics_consistency{decision_id="...",evaluation_id="..."} 0.8100
ethics_fairness{decision_id="...",evaluation_id="..."} 0.7875
ethics_alignment{decision_id="...",evaluation_id="..."} 0.8890
ethics_transparency{decision_id="...",evaluation_id="..."} 0.9020
```

## Data Classes

### EthicsEvaluationResult

Main result object containing all metrics:

```python
@dataclass
class EthicsEvaluationResult:
    decision_id: str
    evaluation_id: str
    timestamp: datetime
    decision_quality: DecisionQualityMetrics
    consistency: ConsistencyMetrics
    fairness: FairnessMetrics
    alignment: AlignmentMetrics
    transparency: TransparencyMetrics
    overall_score: float
    dimension_weights: Dict[str, float]
    metadata: Dict[str, Any]
```

### AggregateEvaluationResult

Aggregate statistics across multiple evaluations:

```python
@dataclass
class AggregateEvaluationResult:
    evaluation_set_id: str
    num_decisions: int
    mean_scores: Dict[str, float]
    median_scores: Dict[str, float]
    std_scores: Dict[str, float]
    min_scores: Dict[str, float]
    max_scores: Dict[str, float]
```

## Custom Weights

All metrics support custom weighting:

```python
# Custom dimension weights
evaluator = EthicsEvaluator(
    dimension_weights={
        'decision_quality': 0.40,  # Emphasize quality
        'consistency': 0.15,
        'fairness': 0.20,
        'alignment': 0.15,
        'transparency': 0.10
    }
)

# Custom component weights
evaluator = EthicsEvaluator(
    component_weights={
        'decision_quality': [0.30, 0.40, 0.15, 0.15],  # Custom quality weights
        'fairness': [0.40, 0.30, 0.20, 0.10]  # Emphasize demographic parity
    }
)
```

## Integration with ThemisDB

The module integrates seamlessly with ThemisDB's moral philosophy framework:

```python
from argument_models import EthicalDecision
from ethics_evaluation_metrics import EthicsEvaluator

# Evaluate an EthicalDecision object
ethical_decision = EthicalDecision(
    id="decision-001",
    decision="...",
    primary_philosophy="kant",
    supporting_philosophies=["utilitarianism"],
    principle_basis=["categorical_imperative"],
    confidence=0.85
)

evaluator = EthicsEvaluator()
result = evaluator.evaluate_decision(ethical_decision)
```

## Testing

Run the test suite:

```bash
python3 test_ethics_evaluation_metrics.py
```

Tests cover:
- ✅ Single decision evaluation
- ✅ Batch evaluation
- ✅ Fairness with group outcomes
- ✅ Alignment with constraints
- ✅ Export functionality
- ✅ Component metrics

## Example Output

```
Evaluation Result for Decision sample-001
--------------------------------------------------
Overall Score: 0.828

Dimension Scores:
  Decision Quality: 0.783
  Consistency:      0.810
  Fairness:         0.788
  Alignment:        0.889
  Transparency:     0.902

Decision Quality Components:
  Outcome Satisfaction: 0.822
  Ethical Alignment:    0.950
  Feasibility:          0.600
  Long-term Impact:     0.625

Transparency Analysis:
  Explanation Completeness: 1.000
  Reasoning Clarity:        0.750
  Justification Robustness: 0.900
  Missing Elements:         []
```

## Best Practices

1. **Provide Context**: Include group outcomes, constraints, and expected principles when available
2. **Use Reference Decisions**: Pass previous decisions for inter-case consistency evaluation
3. **Customize Weights**: Adjust dimension and component weights based on your use case
4. **Batch Evaluate**: Use batch evaluation for aggregate statistics
5. **Export Regularly**: Export metrics to JSON/Prometheus for monitoring and analysis

## API Reference

### Main Classes

- `EthicsEvaluator`: Main evaluator class
- `EthicsEvaluationResult`: Single evaluation result
- `AggregateEvaluationResult`: Aggregate statistics

### Metric Classes

- `DecisionQualityMetrics`
- `ConsistencyMetrics`
- `FairnessMetrics`
- `AlignmentMetrics`
- `TransparencyMetrics`

### Convenience Functions

- `quick_evaluate(decision, context)`: Quick single evaluation
- `batch_evaluate(decisions, contexts)`: Quick batch evaluation

## License

MIT License - Part of ThemisDB Ethics AI Framework

## Author

ThemisDB Ethics AI Framework Team
