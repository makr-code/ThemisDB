# Process Mining in AQL - Practical Examples
# ============================================
# Real-world examples demonstrating process mining capabilities in ThemisDB AQL
#
# Version: 1.0
# Date: 2025-12-24
# Target Audience: Administrators, Analysts, Governance Teams

---

## Example 1: Finding Similar Building Permit Processes

### Scenario
You work for a city administration and want to find all building applications
that follow a pattern similar to the standard approval process.

### Step 1: Load the Standard Model

```aql
-- Load predefined administrative model for building permits
LET ideal_model = PM_LOAD_ADMIN_MODEL("bauantrag_standard")

RETURN ideal_model
```

**Expected Output:**
```json
{
  "id": "bauantrag_standard",
  "name": "Bauantrag (Standard)",
  "activities": ["antragstellung", "vollstaendigkeitspruefung", "fachliche_pruefung", "genehmigung"],
  "edges": [
    {"from": "antragstellung", "to": "vollstaendigkeitspruefung"},
    {"from": "vollstaendigkeitspruefung", "to": "fachliche_pruefung"},
    {"from": "fachliche_pruefung", "to": "genehmigung"}
  ]
}
```

### Step 2: Find Similar Processes

```aql
-- Find all building applications with similar process structure
LET ideal_model = PM_LOAD_ADMIN_MODEL("bauantrag_standard")

LET similar_processes = PM_FIND_SIMILAR(ideal_model, {
  method: "hybrid",          -- Use graph + vector + behavioral similarity
  threshold: 0.75,           -- Minimum 75% similarity
  limit: 50,                 -- Top 50 results
  graph_weight: 0.4,         -- 40% weight on structure
  vector_weight: 0.3,        -- 30% weight on semantics
  behavioral_weight: 0.3     -- 30% weight on execution behavior
})

FOR result IN similar_processes
  SORT result.overall_similarity DESC
  RETURN {
    case_id: result.case_id,
    similarity: result.overall_similarity,
    graph_sim: result.metrics.graph_similarity,
    vector_sim: result.metrics.vector_similarity,
    behavioral_sim: result.metrics.behavioral_similarity,
    matched_activities: result.matched_activities,
    extra_activities: result.extra_activities,
    missing_activities: result.missing_activities
  }
```

**Expected Output:**
```json
[
  {
    "case_id": "V-2024-0123",
    "similarity": 0.92,
    "graph_sim": 0.95,
    "vector_sim": 0.88,
    "behavioral_sim": 0.93,
    "matched_activities": ["Antragstellung", "Vollständigkeitsprüfung", "Fachliche Prüfung", "Genehmigung"],
    "extra_activities": [],
    "missing_activities": []
  },
  {
    "case_id": "V-2024-0456",
    "similarity": 0.85,
    "graph_sim": 0.82,
    "vector_sim": 0.91,
    "behavioral_sim": 0.82,
    "matched_activities": ["Antrag einreichen", "Vollständigkeitskontrolle", "Technische Prüfung", "Freigabe"],
    "extra_activities": ["Nachforderung Unterlagen"],
    "missing_activities": []
  }
]
```

---

## Example 2: Conformance Checking Against Ideal Process

### Scenario
Check if all building applications comply with the standard process.
Identify deviations for quality improvement.

### Query: Find Non-Conformant Processes

```aql
-- Load ideal model
LET ideal = PM_LOAD_ADMIN_MODEL("bauantrag_standard")

-- Check all cases in the bauantraege collection
FOR case IN bauantraege
  LET comparison = PM_COMPARE_IDEAL(case.vorgang_id, ideal)
  
  -- Filter: Only show cases with fitness < 90%
  FILTER comparison.fitness < 0.9
  
  SORT comparison.fitness ASC
  
  RETURN {
    vorgang_id: case.vorgang_id,
    antragsteller: case.antragsteller,
    eingangsdatum: case.eingangsdatum,
    
    -- Conformance metrics
    fitness: comparison.fitness,
    precision: comparison.precision,
    steps_checked: comparison.steps_checked,
    steps_conformant: comparison.steps_conformant,
    
    -- Specific deviations
    deviations: comparison.deviations,
    
    -- Recommended actions
    recommended_action: (
      comparison.fitness < 0.7 ? "Urgent review needed" :
      comparison.fitness < 0.9 ? "Minor adjustments" :
      "OK"
    )
  }
```

**Expected Output:**
```json
[
  {
    "vorgang_id": "V-2024-0789",
    "antragsteller": "Müller GmbH",
    "eingangsdatum": "2024-10-15",
    "fitness": 0.65,
    "precision": 0.82,
    "steps_checked": 6,
    "steps_conformant": 4,
    "deviations": [
      "Activity 'Vollständigkeitsprüfung' was skipped",
      "Activity 'Genehmigung' occurred before 'Fachliche Prüfung'"
    ],
    "recommended_action": "Urgent review needed"
  }
]
```

---

## Example 3: Pattern-Based Filtering

### Scenario
Find all processes where approval happened before technical review
(potential compliance issue).

### Query: Find Processes with Problematic Pattern

```aql
-- Define problematic pattern: Approval before Review
LET problematic_pattern = {
  activities: ["Genehmigung", "Fachliche Prüfung"],
  edges: [
    {from: "Genehmigung", to: "Fachliche Prüfung"}  -- Wrong order!
  ]
}

-- Find all cases matching this pattern
FOR case IN bauantraege
  -- Check if case has this problematic pattern
  LET has_problem = PM_HAS_PATTERN(
    case.vorgang_id,
    problematic_pattern,
    0.8  -- 80% similarity threshold
  )
  
  FILTER has_problem == true
  
  -- Get full trace for analysis
  LET trace = PM_EXTRACT_TRACE(case.vorgang_id)
  
  RETURN {
    vorgang_id: case.vorgang_id,
    status: case.status,
    activities: trace.activities,
    timestamps: trace.timestamps,
    alert: "⚠️ Approval occurred before technical review!",
    requires_investigation: true
  }
```

---

## Example 4: Procurement Process Discovery

### Scenario
You want to discover the actual procurement process from event logs
and compare it with the ideal process defined by law (GWB).

### Step 1: Extract Event Log

```aql
-- Extract procurement event log from audit data
LET procurement_log = PM_EXTRACT_LOG("procurement_audit", {
  case_id_field: "beschaffung_id",
  activity_field: "activity_name",
  timestamp_field: "timestamp",
  resource_field: "user_id",
  
  -- Optional filters
  start_time: DATE_ISO8601("2024-01-01"),
  end_time: DATE_ISO8601("2024-12-31")
})

RETURN {
  total_events: procurement_log.total_events,
  unique_cases: procurement_log.unique_cases,
  unique_activities: procurement_log.unique_activities,
  unique_variants: procurement_log.unique_variants,
  time_range: {
    start: procurement_log.min_timestamp,
    end: procurement_log.max_timestamp
  }
}
```

### Step 2: Discover Process Model

```aql
LET procurement_log = PM_EXTRACT_LOG("procurement_audit", config)

-- Discover process using Heuristic Miner (robust to noise)
LET discovered_model = PM_DISCOVER_PROCESS(procurement_log, {
  algorithm: "heuristic",
  dependency_threshold: 0.85,
  positive_observations: 5,
  detect_loops: true,
  detect_parallelism: true
})

RETURN {
  model_name: discovered_model.name,
  activities: LENGTH(discovered_model.nodes),
  transitions: LENGTH(discovered_model.edges),
  fitness: discovered_model.fitness,
  precision: discovered_model.precision,
  simplicity: discovered_model.simplicity,
  nodes: discovered_model.nodes,
  edges: discovered_model.edges
}
```

### Step 3: Compare with Legal Requirements

```aql
LET procurement_log = PM_EXTRACT_LOG("procurement_audit", config)
LET discovered_model = PM_DISCOVER_PROCESS(procurement_log, config)
LET legal_model = PM_LOAD_ADMIN_MODEL("beschaffung_vergaberecht")

-- Compare discovered vs. legal model
LET comparison = PM_COMPARE_IDEAL(discovered_model, legal_model)

RETURN {
  compliance_status: comparison.fitness > 0.9 ? "✅ Compliant" : "❌ Non-Compliant",
  fitness_score: comparison.fitness,
  
  -- Missing mandatory steps
  missing_activities: comparison.missing_activities,
  
  -- Extra steps (might be OK, but worth reviewing)
  extra_activities: comparison.extra_activities,
  
  -- Specific violations
  deviations: comparison.deviations,
  
  -- Recommendations
  recommendations: (
    FOR dev IN comparison.deviations
      RETURN {
        issue: dev,
        severity: dev LIKE "%Schwellenwert%" ? "HIGH" : "MEDIUM",
        law_reference: dev LIKE "%GWB%" ? "§119 GWB" : null
      }
  )
}
```

---

## Example 5: Process Variant Analysis

### Scenario
Analyze how many different variants of the HR recruitment process exist
and identify the most common ones.

### Query: Analyze Recruitment Variants

```aql
-- Extract event log for HR recruitment
LET hr_log = PM_EXTRACT_LOG("hr_recruitment", {
  case_id_field: "application_id",
  activity_field: "step_name",
  timestamp_field: "completed_at"
})

-- Analyze variants
LET variants = PM_VARIANTS(hr_log, 20)  -- Top 20 variants

FOR variant IN variants
  RETURN {
    variant_id: variant.variant_id,
    frequency: variant.frequency,
    percentage: variant.percentage,
    avg_duration_days: variant.avg_duration_ms / (1000 * 60 * 60 * 24),
    
    -- Activity sequence
    process_path: CONCAT_SEPARATOR(" → ", variant.activities),
    
    -- Sample cases
    sample_cases: variant.case_ids[0..2],
    
    -- Classification
    category: (
      variant.percentage > 50 ? "Standard Path" :
      variant.percentage > 10 ? "Common Variation" :
      "Rare Case"
    )
  }
```

**Expected Output:**
```json
[
  {
    "variant_id": 1,
    "frequency": 156,
    "percentage": 62.4,
    "avg_duration_days": 28.5,
    "process_path": "Stellenausschreibung → Bewerbungseingang → Vorauswahl → Vorstellungsgespräch → Vertragsangebot → Vertragsunterzeichnung",
    "sample_cases": ["HR-2024-001", "HR-2024-003", "HR-2024-007"],
    "category": "Standard Path"
  },
  {
    "variant_id": 2,
    "frequency": 42,
    "percentage": 16.8,
    "avg_duration_days": 35.2,
    "process_path": "Stellenausschreibung → Bewerbungseingang → Vorauswahl → Vorstellungsgespräch → Assessment Center → Vertragsangebot → Vertragsunterzeichnung",
    "sample_cases": ["HR-2024-015", "HR-2024-023"],
    "category": "Common Variation"
  }
]
```

---

## Example 6: Bottleneck Detection

### Scenario
Identify which activities in the budget planning process take the longest
and cause delays.

### Query: Detect Bottlenecks

```aql
-- Extract budget planning log
LET budget_log = PM_EXTRACT_LOG("budget_planning_audit", {
  case_id_field: "budget_year",
  activity_field: "stage",
  timestamp_field: "completed_at"
})

-- Enhance with performance data
LET enhanced_model = PM_DISCOVER_PROCESS(budget_log, {algorithm: "heuristic"})

-- Detect bottlenecks (activities in 90th percentile of duration)
LET bottlenecks = PM_BOTTLENECKS(budget_log, 0.9)

FOR bottleneck IN bottlenecks
  RETURN {
    activity: bottleneck,
    avg_duration_days: bottleneck.avg_duration_ms / (1000 * 60 * 60 * 24),
    max_duration_days: bottleneck.max_duration_ms / (1000 * 60 * 60 * 24),
    frequency: bottleneck.frequency,
    
    -- Impact analysis
    impact: (
      bottleneck.avg_duration_ms > 14 * 24 * 60 * 60 * 1000 ? "🔴 Critical" :
      bottleneck.avg_duration_ms > 7 * 24 * 60 * 60 * 1000 ? "🟡 High" :
      "🟢 Normal"
    ),
    
    -- Recommendations
    recommendation: (
      bottleneck.activity == "Budgetverhandlungen" ? "Consider parallel negotiations" :
      bottleneck.activity == "Konsolidierung" ? "Automate data consolidation" :
      "Review process"
    )
  }
```

---

## Example 7: Predictive Analytics

### Scenario
Predict when an ongoing building permit application will be completed.

### Query: Predict Process End

```aql
-- For a specific ongoing case
LET case_id = "V-2024-1234"

-- Get prediction
LET prediction = PM_PREDICT_END(case_id)

-- Get current status
LET status = MILESTONE_STATUS(case_id)
LET trace = PM_EXTRACT_TRACE(case_id)

RETURN {
  case_id: case_id,
  
  -- Current state
  current_activity: trace.activities[-1],
  progress_percent: status.progress_percent,
  
  -- Prediction
  predicted_end_date: DATE_ISO8601(prediction.predicted_end),
  remaining_hours: prediction.remaining_hours,
  confidence: prediction.confidence,
  
  -- Scenarios
  optimistic_end: DATE_ISO8601(prediction.predicted_end - prediction.optimistic_hours * 60 * 60 * 1000),
  pessimistic_end: DATE_ISO8601(prediction.predicted_end + prediction.pessimistic_hours * 60 * 60 * 1000),
  
  -- Risk assessment
  risk: (
    prediction.remaining_hours > 2160 ? "⚠️ SLA at risk" :
    prediction.remaining_hours > 1440 ? "🟡 Monitor closely" :
    "✅ On track"
  )
}
```

---

## Example 8: List Available Administrative Models

### Query: Browse Model Library

```aql
-- List all available predefined models
LET models = PM_LIST_ADMIN_MODELS()

FOR model IN models
  RETURN {
    id: model.id,
    name: model.name,
    domain: model.domain,
    description: model.description,
    activity_count: LENGTH(model.activities),
    compliance_frameworks: model.compliance[*].rule,
    sla_defined: LENGTH(model.milestones) > 0,
    tags: model.semantic_tags
  }
```

**Expected Output:**
```json
[
  {
    "id": "bauantrag_standard",
    "name": "Bauantrag (Standard)",
    "domain": "Bauwesen",
    "description": "Standard-Bauantragsverfahren nach §34 BauO",
    "activity_count": 7,
    "compliance_frameworks": ["§34 BauO", "Vier-Augen-Prinzip", "Dokumentationspflicht"],
    "sla_defined": true,
    "tags": ["öffentliche Verwaltung", "Baugenehmigung", "Baurecht"]
  },
  {
    "id": "beschaffung_vergaberecht",
    "name": "Beschaffung (Vergaberecht konform)",
    "domain": "Beschaffung",
    "description": "Beschaffungsprozess nach GWB und VOB/A",
    "activity_count": 8,
    "compliance_frameworks": ["GWB §119 Schwellenwerte", "VOB/A Dokumentation"],
    "sla_defined": false,
    "tags": ["Beschaffung", "Vergaberecht", "GWB", "VOB/A"]
  }
]
```

---

## Example 9: Export Discovered Process as BPMN

### Scenario
Export a discovered process model as BPMN 2.0 for use in
process modeling tools (Camunda, Signavio, etc.).

### Query: Discover and Export

```aql
-- Extract log and discover process
LET log = PM_EXTRACT_LOG("document_approval", {
  case_id_field: "document_id",
  activity_field: "approval_stage",
  timestamp_field: "timestamp"
})

LET model = PM_DISCOVER_PROCESS(log, {algorithm: "heuristic"})

-- Export as BPMN 2.0 XML
LET bpmn_xml = PM_EXPORT_BPMN(model)

RETURN {
  model_name: model.name,
  bpmn_xml: bpmn_xml,
  
  -- Save to file (pseudo-code)
  export_command: CONCAT(
    "Save to: document_approval_discovered_",
    DATE_FORMAT(NOW(), "%Y%m%d"),
    ".bpmn"
  )
}
```

---

## Example 10: Complex Analysis - Multi-Model Comparison

### Scenario
Compare multiple department's document approval processes
to identify best practices.

### Query: Cross-Department Analysis

```aql
-- Define departments to analyze
LET departments = ["IT", "HR", "Finance", "Legal"]

FOR dept IN departments
  -- Extract log for this department
  LET log = PM_EXTRACT_LOG("document_approvals", {
    case_id_field: "document_id",
    activity_field: "stage",
    timestamp_field: "timestamp",
    
    -- Filter by department
    include_activities: CONCAT(dept, "_")
  })
  
  -- Discover process
  LET discovered = PM_DISCOVER_PROCESS(log, {algorithm: "heuristic"})
  
  -- Compare with ideal
  LET ideal = PM_LOAD_ADMIN_MODEL("dokumenten_freigabe")
  LET comparison = PM_COMPARE_IDEAL(discovered, ideal)
  
  -- Analyze performance
  LET bottlenecks = PM_BOTTLENECKS(log, 0.85)
  
  -- Calculate metrics
  LET variants = PM_VARIANTS(log, 5)
  
  RETURN {
    department: dept,
    
    -- Model characteristics
    complexity: LENGTH(discovered.nodes),
    variants: LENGTH(variants),
    
    -- Conformance
    fitness: comparison.fitness,
    precision: comparison.precision,
    
    -- Performance
    avg_duration_days: (
      SUM(log.traces[*].duration_ms) / LENGTH(log.traces) / (1000 * 60 * 60 * 24)
    ),
    bottleneck_count: LENGTH(bottlenecks),
    
    -- Best/Worst case
    fastest_case: MIN(log.traces[*].duration_ms) / (1000 * 60 * 60 * 24),
    slowest_case: MAX(log.traces[*].duration_ms) / (1000 * 60 * 60 * 24),
    
    -- Recommendations
    grade: (
      comparison.fitness > 0.95 && LENGTH(bottlenecks) == 0 ? "A - Exemplary" :
      comparison.fitness > 0.85 && LENGTH(bottlenecks) <= 1 ? "B - Good" :
      comparison.fitness > 0.75 ? "C - Needs Improvement" :
      "D - Requires Intervention"
    )
  }
```

---

## Tips & Best Practices

### 1. Performance Optimization
```aql
-- Use thresholds to limit results
LET similar = PM_FIND_SIMILAR(pattern, {threshold: 0.8})  -- ✅ Good
LET similar = PM_FIND_SIMILAR(pattern, {threshold: 0.0})  -- ❌ Too many results

-- Limit result count
LET similar = PM_FIND_SIMILAR(pattern, {limit: 50})       -- ✅ Reasonable
LET similar = PM_FIND_SIMILAR(pattern, {limit: 10000})    -- ❌ Too many

-- Use indexed queries when possible
FILTER PM_HAS_PATTERN(case.id, pattern, 0.8)              -- ✅ Uses index
```

### 2. Choosing Similarity Methods
```aql
-- Graph: When structure matters most (workflows with strict sequences)
{method: "graph"}

-- Vector: When semantics matter (similar activities with different names)
{method: "vector"}

-- Behavioral: When execution order matters
{method: "behavioral"}

-- Hybrid: Best overall (default, recommended)
{method: "hybrid", graph_weight: 0.4, vector_weight: 0.3, behavioral_weight: 0.3}
```

### 3. Working with Administrative Models
```aql
-- Always list available models first
LET models = PM_LIST_ADMIN_MODELS()

-- Load specific model
LET model = PM_LOAD_ADMIN_MODEL("bauantrag_standard")

-- Check model structure before use
RETURN {
  activities: model.activities,
  compliance_rules: model.compliance
}
```

---

## Troubleshooting

### Issue: "Pattern not found"
**Solution**: Lower the threshold or check activity names
```aql
-- Too strict
LET result = PM_HAS_PATTERN(case_id, pattern, 0.99)  -- ❌

-- More tolerant
LET result = PM_HAS_PATTERN(case_id, pattern, 0.7)   -- ✅
```

### Issue: "No administrative model found"
**Solution**: Check available models
```aql
LET models = PM_LIST_ADMIN_MODELS()
RETURN models[*].id  -- Shows all available IDs
```

### Issue: "Event log extraction failed"
**Solution**: Verify field names
```aql
-- Check sample document structure first
FOR doc IN my_collection LIMIT 1
  RETURN KEYS(doc)

-- Then use correct field names
LET log = PM_EXTRACT_LOG("my_collection", {
  case_id_field: "id",      -- ✅ Correct field name
  activity_field: "action",
  timestamp_field: "ts"
})
```

---

## Further Reading

- [Process Mining Research & Roadmap](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md)
- [Process Mining Guide](./process_mining_guide.md)
- [AQL Functions Reference](../aql/aql_functions_reference.md)
- [Administrative Models Configuration](../../config/process_models/administrative_process_models.yaml)

---

**Version**: 1.0  
**Last Updated**: 2026-04-06  
**Status**: Examples & Best Practices
