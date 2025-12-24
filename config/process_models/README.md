# Process Models Directory

This directory contains predefined process models for various business domains that can be used with ThemisDB's Process Mining functionality.

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

## Total Statistics

- **Process Models:** 17 predefined models
- **Domains:** 5 (Administrative, IT, Healthcare, Customer Service, Finance)
- **Activities:** 150+ predefined activities
- **Compliance Frameworks:** 25+ regulations and standards
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
