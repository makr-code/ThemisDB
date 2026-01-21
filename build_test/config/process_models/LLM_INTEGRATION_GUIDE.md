# LLM Integration Guide for Process Models
# ==========================================
# Guidelines for LLM-assisted process mining and analysis
#
# Version: 1.0
# Date: 2025-12-24

## Overview

This document defines how Large Language Models (LLMs) should be integrated with ThemisDB process models to provide intelligent process analysis, conformance checking, predictions, and recommendations.

## LLM Prompt Templates

### 1. Process Analysis Template

```
Task: Analyze Process Conformance
Model: {model_id}
Domain: {domain}

Process Trace:
{trace_data}

Expected Process Model:
{model_description}

Required Analysis:
1. Calculate conformance scores (fitness, precision, generalization)
2. Identify all deviations from expected model
3. Check compliance with regulations: {compliance_rules}
4. Assess SLA adherence (max duration: {sla_hours} hours)
5. Provide actionable recommendations

Output Format: JSON
```

### 2. Conformance Checking Template

```
Task: Check Process Conformance
Model: {model_id}

Input Trace: {trace}
Expected Activities: {expected_activities}
Expected Flow: {expected_edges}

Compliance Rules:
{compliance_rules}

Calculate:
- Fitness score (0-1): Fraction of behavior in log that can be replayed
- Precision score (0-1): Fraction of model behavior observed in log  
- List of deviations with severity levels
- Compliance violations

Output Format: JSON with schema validation
```

### 3. Prediction Template

```
Task: Predict Next Activity
Current State: {current_activity}
Completed Activities: {completed_activities}
Process Model: {model_id}

Historical Data Available: {use_historical}
Consider SLAs: {consider_sla}

Provide:
1. Top 3 most likely next activities with probabilities
2. Reasoning for each prediction
3. Estimated time to next activity
4. Risk factors or warnings

Output Format: JSON
```

### 4. Optimization Recommendation Template

```
Task: Process Optimization Analysis
Process: {model_id}
Performance Data: {metrics}

Analyze:
1. Bottlenecks (activities with longest wait times)
2. Parallelization opportunities
3. Automation potential
4. SLA risks

Prioritize recommendations by:
- Time savings
- Implementation effort
- Compliance safety

Output Format: JSON
```

## Expected Output Schemas

### Analysis Output Schema

```json
{
  "type": "object",
  "required": ["conformance_score", "deviations", "compliance_issues", "recommendations"],
  "properties": {
    "conformance_score": {
      "type": "number",
      "minimum": 0,
      "maximum": 1,
      "description": "Overall conformance (0-1)"
    },
    "deviations": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["activity", "issue", "severity"],
        "properties": {
          "activity": {"type": "string"},
          "issue": {"type": "string"},
          "severity": {"enum": ["critical", "major", "minor"]},
          "expected": {"type": "string"},
          "actual": {"type": "string"}
        }
      }
    },
    "compliance_issues": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["rule", "violated", "description"],
        "properties": {
          "rule": {"type": "string"},
          "violated": {"type": "boolean"},
          "description": {"type": "string"},
          "severity": {"enum": ["critical", "major", "minor"]}
        }
      }
    },
    "recommendations": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["type", "priority", "description"],
        "properties": {
          "type": {"enum": ["process_change", "automation", "training", "policy_update"]},
          "priority": {"enum": ["high", "medium", "low"]},
          "description": {"type": "string"},
          "expected_impact": {"type": "string"},
          "implementation_effort": {"enum": ["low", "medium", "high"]}
        }
      }
    }
  }
}
```

### Conformance Check Output Schema

```json
{
  "type": "object",
  "required": ["fitness", "precision", "deviations"],
  "properties": {
    "fitness": {
      "type": "number",
      "minimum": 0,
      "maximum": 1,
      "description": "Replay fitness score"
    },
    "precision": {
      "type": "number",
      "minimum": 0,
      "maximum": 1,
      "description": "Model precision score"
    },
    "generalization": {
      "type": "number",
      "minimum": 0,
      "maximum": 1
    },
    "deviations": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "type": {"enum": ["missing_activity", "unexpected_activity", "wrong_order", "sla_violation"]},
          "severity": {"enum": ["critical", "major", "minor"]},
          "description": {"type": "string"},
          "location": {"type": "string"}
        }
      }
    }
  }
}
```

### Prediction Output Schema

```json
{
  "type": "object",
  "required": ["predictions", "reasoning"],
  "properties": {
    "predictions": {
      "type": "array",
      "minItems": 1,
      "maxItems": 3,
      "items": {
        "type": "object",
        "required": ["activity", "probability"],
        "properties": {
          "activity": {"type": "string"},
          "probability": {"type": "number", "minimum": 0, "maximum": 1},
          "estimated_duration_hours": {"type": "number", "minimum": 0},
          "confidence": {"enum": ["high", "medium", "low"]}
        }
      }
    },
    "reasoning": {
      "type": "string",
      "description": "Explanation for predictions"
    },
    "warnings": {
      "type": "array",
      "items": {"type": "string"}
    },
    "risk_factors": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "risk": {"type": "string"},
          "severity": {"enum": ["high", "medium", "low"]},
          "mitigation": {"type": "string"}
        }
      }
    }
  }
}
```

### Optimization Output Schema

```json
{
  "type": "object",
  "required": ["bottlenecks", "recommendations"],
  "properties": {
    "bottlenecks": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["activity", "avg_duration_hours", "impact"],
        "properties": {
          "activity": {"type": "string"},
          "avg_duration_hours": {"type": "number"},
          "impact": {"enum": ["high", "medium", "low"]},
          "frequency": {"type": "integer"},
          "wait_time_contribution_percent": {"type": "number"}
        }
      }
    },
    "parallelization_opportunities": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "activities": {"type": "array", "items": {"type": "string"}},
          "potential_time_savings_percent": {"type": "number"}
        }
      }
    },
    "automation_potential": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "activity": {"type": "string"},
          "automation_feasibility": {"enum": ["high", "medium", "low"]},
          "expected_time_savings_percent": {"type": "number"}
        }
      }
    },
    "recommendations": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["title", "description", "priority"],
        "properties": {
          "title": {"type": "string"},
          "description": {"type": "string"},
          "priority": {"enum": ["high", "medium", "low"]},
          "category": {"enum": ["efficiency", "compliance", "quality", "cost"]},
          "time_savings_percent": {"type": "number"},
          "implementation_effort": {"enum": ["low", "medium", "high"]},
          "roi_estimate": {"type": "string"}
        }
      }
    }
  }
}
```

## Validation Criteria

### General Validation Rules

1. **Score Validation**
   - All scores (fitness, precision, conformance) must be in range [0, 1]
   - Probabilities must sum to approximately 1.0 (tolerance: ±0.05)

2. **Completeness Checks**
   - All required fields must be present
   - Arrays must not be empty when issues exist
   - Descriptions must be meaningful (min 10 characters)

3. **Consistency Checks**
   - Severity levels must match issue types
   - Predictions must reference valid activities from model
   - Recommendations must be actionable

4. **Compliance Coverage**
   - All compliance rules from model must be checked
   - Critical violations must be flagged as "critical" severity
   - SLA violations must be detected and reported

### Domain-Specific Validations

#### Administrative Processes
- Four-eyes principle must be validated
- SLA checks must reference specific regulations (e.g., §34 BauO)
- Documentation requirements must be verified

#### Healthcare Processes
- Patient safety checks (e.g., 5R Rule for medications)
- Regulatory compliance (RiliBÄK, ISO 15189)
- Privacy/GDPR compliance for patient data

#### IT Service
- ITIL compliance for incident/change management
- Priority levels must be valid (Critical, High, Medium, Low)
- SLA based on priority must be checked

#### Financial Processes
- Separation of duties (Vier-Augen-Prinzip)
- Regulatory compliance (HGB, GoBD, UStG)
- Approval hierarchies based on amount thresholds

## LLM Test Framework

### Test Case Structure

```yaml
llm_test_cases:
  - name: "Test case description"
    category: "conformance|prediction|optimization"
    input_data:
      trace: [activities]
      context: {additional_data}
    expected_behavior:
      conformance_score: "operator value"  # e.g., ">= 0.95"
      deviations_count: "operator value"   # e.g., "== 0"
      specific_checks:
        - check: "description"
          condition: "boolean expression"
    pass_criteria:
      - "All expected behaviors met"
      - "Output schema valid"
      - "Response time < 5s"
```

### Benchmark Metrics

1. **Accuracy Metrics**
   - Conformance score accuracy (±0.05 tolerance)
   - Deviation detection rate (recall > 0.95)
   - False positive rate (< 0.05)

2. **Performance Metrics**
   - Response time (< 5 seconds for analysis)
   - Throughput (processes per minute)

3. **Quality Metrics**
   - Recommendation relevance score
   - Prediction accuracy (top-1, top-3)
   - Explanation quality (human evaluation)

### Test Scenarios

#### 1. Perfect Conformance
- Input: Process trace matching model exactly
- Expected: conformance_score >= 0.98, deviations_count == 0

#### 2. Minor Deviations
- Input: Process with optional activity skipped
- Expected: 0.80 <= conformance_score < 0.95, minor deviations only

#### 3. Critical Violations
- Input: Process violating compliance rules
- Expected: Critical severity flagged, specific rule identified

#### 4. SLA Violations
- Input: Process exceeding time limits
- Expected: SLA violation detected, regulation referenced

#### 5. Prediction Accuracy
- Input: Partial process trace
- Expected: Correct next activity in top-3 predictions

## Integration with ThemisDB

### AQL Function Integration

```aql
-- Use LLM for process analysis
LET analysis = LLM_ANALYZE_PROCESS(
  trace: case.trace,
  model: PM_LOAD_ADMIN_MODEL("bauantrag_standard"),
  task: "analyze_process"
)

RETURN {
  case_id: case.id,
  conformance: analysis.conformance_score,
  issues: analysis.deviations,
  recommendations: analysis.recommendations
}
```

### Benchmark Execution

```aql
-- Run LLM benchmarks
FOR test_case IN llm_test_cases
  LET result = LLM_RUN_TEST(
    model: test_case.model_id,
    input: test_case.input_data,
    expected: test_case.expected_behavior
  )
  
  RETURN {
    test: test_case.name,
    passed: result.all_checks_passed,
    accuracy: result.accuracy_score,
    response_time_ms: result.duration
  }
```

## Best Practices

1. **Prompt Engineering**
   - Be specific about expected output format
   - Provide clear examples
   - Include relevant context
   - Specify constraints explicitly

2. **Error Handling**
   - Validate LLM output against schema
   - Provide fallback for invalid responses
   - Log anomalies for review

3. **Performance Optimization**
   - Cache frequent queries
   - Batch similar requests
   - Use streaming for long responses

4. **Quality Assurance**
   - Regular benchmark testing
   - Human review of edge cases
   - Continuous improvement based on feedback

## Version History

- **v1.0** (2025-12-24) - Initial LLM integration framework

## References

- Process Mining: Data Science in Action (van der Aalst, 2016)
- LLM Prompting Best Practices
- JSON Schema Specification
- ThemisDB AQL Documentation
