# Process Models Directory

This directory contains predefined process models for various business domains that can be used with ThemisDB's Process Mining functionality, **enhanced with LLM (Large Language Model) support** for intelligent process analysis.

## Key Features

✅ **17 Process Models** across 5 domains  
✅ **LLM Integration** - AI-powered process analysis, conformance checking, predictions  
✅ **Automated Testing** - Validate LLM behavior with test cases  
✅ **Benchmarks** - Measure LLM accuracy and performance  

## Available Process Model Files

### 1. Administrative Processes (`administrative_process_models.yaml`)
**Domain:** Public Administration (Verwaltung)

**Models:**
- **bauantrag_standard** - Building Permit Process (§34 BauO)
- **beschaffung_vergaberecht** - Procurement (GWB, VOB/A compliant)
- **personal_einstellung** - HR Recruitment (AGG, DSGVO)
- **haushaltsplanung_jaehrlich** - Budget Planning (Annual cycle)
- **dokumenten_freigabe** - Document Approval (Multi-stage)

**Compliance Frameworks:** §34 BauO, GWB, VOB/A, AGG, DSGVO, Haushaltsrecht

### 2. IT Service Processes (`it_service_processes.yaml`)
**Domain:** IT Service Management

**Models:**
- **incident_management_standard** - ITIL v4 Incident Management
- **change_management_standard** - ITIL v4 Change Management
- **sdlc_agile_scrum** - Agile Development (Scrum Framework)

**Frameworks:** ITIL v4, ISO 20000, Scrum Guide

### 3. Healthcare Processes (`healthcare_processes.yaml`)
**Domain:** Healthcare (Gesundheitswesen)

**Models:**
- **patient_admission_standard** - Patient Admission Process
- **medication_management_standard** - Medication Management (5R Rule)
- **lab_testing_standard** - Laboratory Testing Process

**Regulations:** Patientenrechtegesetz, AMG, RiliBÄK, ISO 15189, DSGVO Art. 9

### 4. Customer Service Processes (`customer_service_processes.yaml`)
**Domain:** Customer Service & E-Commerce

**Models:**
- **complaint_handling_standard** - Complaint Management (ISO 10002)
- **order_processing_ecommerce** - E-Commerce Order Processing
- **return_refund_process** - Return and Refund Process

**Standards:** ISO 10002, BGB (Consumer Rights), Widerrufsrecht

### 5. Financial Processes (`financial_processes.yaml`)
**Domain:** Finance & Accounting

**Models:**
- **invoice_processing_ap** - Accounts Payable Invoice Processing
- **expense_claim_processing** - Travel Expense Claims
- **month_end_closing** - Month-End Financial Closing

**Regulations:** HGB, GoBD, UStG, EStG

---

## Usage in AQL

### Loading a Specific Model

```aql
-- Load a specific process model
LET model = PM_LOAD_ADMIN_MODEL("incident_management_standard")
RETURN model
```

### List All Available Models

```aql
-- List all available process models
LET models = PM_LIST_ADMIN_MODELS()
FOR model IN models
  RETURN {
    id: model.id,
    name: model.name,
    domain: model.domain
  }
```

### Find Similar Processes

```aql
-- Load ideal model and find similar processes
LET ideal = PM_LOAD_ADMIN_MODEL("complaint_handling_standard")

LET similar = PM_FIND_SIMILAR(ideal, {
  method: "hybrid",
  threshold: 0.75,
  limit: 20
})

FOR result IN similar
  RETURN {
    case_id: result.case_id,
    similarity: result.overall_similarity,
    matched_activities: result.matched_activities
  }
```

### Conformance Checking

```aql
-- Check conformance against ideal process
FOR case IN my_processes
  LET comparison = PM_COMPARE_IDEAL(
    case.id,
    PM_LOAD_ADMIN_MODEL("patient_admission_standard")
  )
  
  FILTER comparison.fitness < 0.9
  
  RETURN {
    case_id: case.id,
    fitness: comparison.fitness,
    deviations: comparison.deviations
  }
```

---

## Model Structure

Each YAML file contains:

```yaml
<domain>_models:
  - id: "unique_model_id"
    name: "Process Name (German)"
    name_en: "Process Name (English)"
    domain: "Domain"
    description: "Process description"
    
    activities:
      - id: "activity_id"
        name: "Activity Name"
        type: "start|task|end"
        sla_hours: 24
        responsible_role: "Role"
    
    edges:
      - from: "source_activity"
        to: "target_activity"
        type: "sequence|conditional|loop"
    
    compliance:
      - rule: "Regulation Name"
        description: "Description"
        check_type: "type"
    
    semantic_tags:
      - "tag1"
      - "tag2"

metadata:
  version: "1.0"
  categories: [...]
  frameworks: [...]
```

---

## LLM Integration 🤖

All process models now include **LLM (Large Language Model) support** for intelligent analysis:

### LLM Capabilities

1. **Process Analysis** - Automated conformance checking and deviation detection
2. **Predictions** - Predict next activities, durations, and risks
3. **Recommendations** - Suggest optimizations and improvements
4. **Compliance Verification** - Automated regulatory compliance checking
5. **Anomaly Detection** - Identify fraud, safety issues, and violations

### LLM Prompts

Each process model includes:
- **Task-specific prompts** for different analysis types
- **Expected output schemas** (JSON with validation)
- **Test cases** to validate LLM behavior
- **Benchmark criteria** for accuracy and performance

### Example: LLM-Assisted Analysis

```aql
-- Use LLM to analyze building permit process
LET model = PM_LOAD_ADMIN_MODEL("bauantrag_standard")

-- Get LLM analysis
LET analysis = LLM_ANALYZE_PROCESS({
  trace: case.activities,
  model: model,
  task: "analyze_process"
})

-- Filter cases with compliance issues
FILTER analysis.compliance_issues_count > 0

RETURN {
  case_id: case.id,
  conformance_score: analysis.conformance_score,
  violations: analysis.compliance_issues,
  recommendations: analysis.recommendations
}
```

### LLM Testing & Benchmarks

Each model includes test cases to validate LLM behavior:

```yaml
llm_test_cases:
  - name: "Standard conformant process"
    input_trace: ["activity1", "activity2", "activity3"]
    expected_behavior:
      conformance_score: ">= 0.95"
      deviations_count: "== 0"
      compliance_issues_count: "== 0"
  
  - name: "Process with SLA violation"
    input_trace: ["activity1", "activity2"]
    duration_days: 120
    expected_behavior:
      sla_violation_detected: true
      violated_rule: "§34 BauO"
```

### Benchmark Requirements

Performance targets for LLM integration:

| Domain | Accuracy | Response Time | False Positive Rate |
|--------|----------|---------------|---------------------|
| **Administrative** | ≥ 90% | < 5s | < 10% |
| **IT Service** | ≥ 85% | < 5s | < 10% |
| **Healthcare** | ≥ 98% | < 3s | < 2% |
| **Customer Service** | ≥ 85% | < 5s | < 10% |
| **Financial** | ≥ 95% | < 5s | < 5% |

*Healthcare has highest requirements due to patient safety criticality*

### LLM Integration Guide

For detailed information on LLM integration framework, see:
📖 **[LLM_INTEGRATION_GUIDE.md](LLM_INTEGRATION_GUIDE.md)**

---

## Total Statistics

- **Process Models:** 17 predefined models
- **Domains:** 5 (Administrative, IT, Healthcare, Customer Service, Finance)
- **Activities:** 150+ predefined activities
- **Compliance Frameworks:** 25+ regulations and standards
- **LLM Prompts:** 30+ task-specific prompts
- **LLM Test Cases:** 20+ automated test scenarios
- **Languages:** German (primary), English (secondary)

---

## Adding Custom Models

To add your own process models:

1. Create a new YAML file in this directory
2. Follow the structure shown above
3. Use meaningful `id` values (lowercase, underscores)
4. Include compliance rules relevant to your domain
5. Add semantic tags for searchability

Example filename: `your_domain_processes.yaml`

---

## Compliance & Standards Coverage

### Legal/Regulatory
- German: BauO, GWB, VOB/A, AGG, DSGVO, BGB, HGB, UStG, EStG, GoBD, AMG
- European: GDPR (DSGVO)
- Healthcare: Patientenrechtegesetz, RiliBÄK

### Standards
- ISO: 10002, 15189, 20000
- ITIL: v4 Framework
- Healthcare: 5R Rule, Patient Safety
- Agile: Scrum Guide

---

## Version History

- **v1.1** (2025-12-24) - Added LLM integration with prompts, test cases, and benchmarks
- **v1.0** (2025-12-24) - Initial release with 17 process models across 5 domains

---

## License

MIT License - See main repository LICENSE file

---

## Support

For questions or issues:
- Documentation: `/docs/de/analytics/PROCESS_MINING_AQL_EXAMPLES.md`
- Research: `/docs/de/analytics/PROCESS_MINING_RESEARCH_AND_ROADMAP.md`
- Repository: [ThemisDB GitHub](https://github.com/makr-code/ThemisDB)
