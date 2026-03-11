# Legal Text Ingestion Guide

This guide explains how to configure and use ThemisDB's **LLM-driven semantic
extraction pipeline** to ingest German administrative law documents
(Verwaltungsrecht) such as the BImSchG, StGB, DSGVO, and EU directives.

## Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Components](#components)
  - [DeonticExtractor](#deonticextractor)
  - [SemanticValidator](#semanticvalidator)
  - [AgenticReferenceValidator](#agenticreferencevalidator)
- [Configuration](#configuration)
- [Quality Gates](#quality-gates)
- [Integration with IngestionManager](#integration-with-ingestionmanager)
- [Phase 2: LLM Integration](#phase-2-llm-integration)

---

## Overview

German administrative law texts have a well-defined hierarchical structure
(`§ → Abs → Nr`) and use a restricted vocabulary of **deontic modalities**
(obligation, permission, prohibition). ThemisDB exploits this structure to
extract structured legal knowledge at ingestion time.

### Phase 1 (Current): Regex-Based Extraction

All components use regex patterns covering standard German legal language.
This provides deterministic, fast extraction with no external dependencies.

### Phase 2 (Planned): LLM + NER

- **Deontic Extraction**: Mistral 7B + LoRA adapter trained on BImSchG / DSGVO
- **Entity Recognition**: SpaCy `de_legal_ner` custom model
- **Agentic Verification**: ReAct-style agents for reference resolution

---

## Architecture

```
Raw Legal Text
      │
      ▼
┌─────────────────────┐
│   DeonticExtractor  │  ← Extracts deontic categories + entities + obligations
│   (regex / LLM)     │
└─────────┬───────────┘
          │ DeonticExtraction
          ▼
┌─────────────────────┐
│  SemanticValidator  │  ← Applies quality gates; produces semantic score
│  + Quality Gates    │
└─────────┬───────────┘
          │ SemanticValidationResult
          ▼
┌──────────────────────────┐
│ AgenticReferenceValidator│  ← Extracts + validates cross-references
│ (regex / knowledge base) │
└─────────┬────────────────┘
          │ ReferenceValidationReport
          ▼
┌─────────────────────────┐
│   LegalExtractionResult │  ← Aggregated result per document
│   (provisions + score)  │
└─────────────────────────┘
```

---

## Quick Start

### 1. Register a legal source

```cpp
#include "ingestion/ingestion_manager.h"
using namespace themis::ingestion;

LegalIngestionConfig legal_cfg;
legal_cfg.enabled              = true;
legal_cfg.confidence_threshold = 0.75;
legal_cfg.validate_references  = true;

auto mgr = IngestionBuilder("themis_db")
    .withFilesystemSource("bimschg", "/data/laws/bimschg",
                          {{"recursive", "true"}})
    .withLegalIngestionConfig("bimschg", legal_cfg)
    .withTargetCollection("legal_documents")
    .build();

auto report = mgr->ingestAll();
```

### 2. Extract a single document

```cpp
IngestionManager mgr("themis_db");
LegalIngestionConfig cfg;
cfg.enabled = true;

auto result = mgr.runLegalExtraction("BImSchG_2024", bimschg_text, cfg);
std::cout << "Quality: " << result.quality_score << "\n";
```

---

## Components

### DeonticExtractor

`#include "ingestion/deontic_extractor.h"`

Extracts deontic modalities from German legal text.

#### Supported Categories

| Category     | German Patterns                                           |
|--------------|-----------------------------------------------------------|
| `obligation` | muss, müssen, bedarf, bedürfen, ist verpflichtet          |
| `permission` | darf, dürfen, kann, können, ist berechtigt                |
| `prohibition`| darf nicht, ist verboten, ist unzulässig, ist untersagt   |
| `definition` | im Sinne dieses Gesetzes, gilt als, Begriffsbestimmung    |
| `condition`  | wenn, falls, sofern, soweit, vorbehaltlich                |
| `exception`  | ausgenommen, außer, gilt nicht für, es sei denn           |
| `reference`  | gemäß, nach § , entsprechend § , nach Maßgabe             |

#### Entity Types

| Type              | Examples                                               |
|-------------------|--------------------------------------------------------|
| `law_reference`   | § 4 Abs. 1, BImSchG, Richtlinie 2010/75/EU            |
| `person_role`     | Antragsteller, Betreiber, Genehmigungsinhaber          |
| `organization`    | Umweltbundesamt, Bundesministerium, Behörde            |
| `temporal`        | 14 Tage, 15. März 1974, innerhalb von 3 Monaten       |
| `threshold_value` | 500 kW, 10 mg, mehr als 1000 m³                       |

#### Usage

```cpp
DeonticExtractor extractor;
extractor.setConfidenceThreshold(0.75);

auto result = extractor.extract("Wer Anlagen betreiben will, bedarf einer Genehmigung.");
// result.primaryCategory() == DeonticCategory::OBLIGATION
// result.overall_confidence >= 0.75

auto entities = extractor.extractEntities("Gemäß BImSchG § 4 Abs. 1...");
// entities[0].type == "law_reference", entities[0].value == "BImSchG"
```

#### Injectable Extractor (Phase 2 / Testing)

```cpp
extractor.setExtractorFn([&llm](const std::string& text) -> DeonticExtraction {
    // Call LLM adapter or mock for testing
    return llm.extractDeontic(text);
});
```

---

### SemanticValidator

`#include "ingestion/semantic_validator.h"`

Applies quality gates to a `DeonticExtraction` and produces a semantic score.

#### Quality Gates

| Gate                | Default Threshold | Required | Fail Action  |
|---------------------|-------------------|----------|--------------|
| `min_confidence`    | 0.80              | No       | warn         |
| `deontic_confidence`| 0.75              | No       | warn         |
| `section_hierarchy` | -                 | Yes      | flag_for_review |
| `temporal_present`  | -                 | No       | warn         |
| `no_dangling_refs`  | -                 | No       | warn         |

#### Document-Level Extraction

```cpp
SemanticValidator validator;
auto result = validator.extractDocument("BImSchG_2024", full_bimschg_text);

for (const auto& prov : result.provisions) {
    std::cout << prov.section_ref << ": "
              << deonticCategoryToString(prov.deontic_category) << "\n";
}
std::cout << "Overall quality: " << result.quality_score << "\n";
```

#### Semantic Score Composition

The semantic score is a weighted average of:
- Overall extraction confidence (weight 0.4)
- Deontic category identified (weight 0.2)
- Entities extracted (weight 0.2, capped at 5 entities)
- Temporal expression present (weight 0.1)
- Section structure detected (weight 0.1)

---

### AgenticReferenceValidator

`#include "ingestion/agentic_reference_validator.h"`

Extracts and validates cross-references to other legal provisions.

#### Extracted Reference Types

- Same-document: `§ 4 Abs. 1`, `§ 4a`
- Inter-law: `BImSchG`, `StGB`, `DSGVO`
- EU directives: `Richtlinie 2010/75/EU`
- Articles: `Art. 20 Abs. 3 GG`

#### Knowledge Base

The validator maintains an in-memory knowledge base of known law identifiers
and section numbers. Pre-loaded identifiers: BImSchG, StGB, DSGVO, GG, BGB,
HGB, VwVfG, UmwG, KrWG (and more).

```cpp
AgenticReferenceValidator validator;

// Add domain-specific sections
validator.addKnownSection("BImSchG", "4");
validator.addKnownSection("BImSchG", "5");
validator.addKnownSection("BImSchG", "6");

auto report = validator.validate(legal_text);
std::cout << "Dangling references: " << report.dangling_count << "\n";
for (const auto& w : report.warnings) {
    std::cout << "Warning: " << w << "\n";
}
```

---

## Configuration

### legal-ingestion-schema.yaml

The full pipeline is configured via `config/ingestion/legal-ingestion-schema.yaml`.
See the file for the complete reference. Key sections:

```yaml
ingestion_rules:
  deontic_extraction:
    enabled: true
    confidence_threshold: 0.75
    categories: [obligation, permission, prohibition, definition, condition, exception, reference]

  quality_gates:
    min_extraction_confidence: 0.80
    min_deontic_extraction_confidence: 0.75
    require_temporal_analysis: true
```

### LegalIngestionConfig

```cpp
LegalIngestionConfig cfg;
cfg.enabled               = true;   // Enable the pipeline
cfg.confidence_threshold  = 0.75;   // Minimum deontic confidence
cfg.validate_references   = true;   // Run AgenticReferenceValidator
cfg.require_section_struct = false; // Reject docs without § structure
cfg.flag_low_confidence   = true;   // Warn in lineage on low confidence
```

---

## Quality Gates

Quality gates are evaluated per document. Results appear in:
- `LegalExtractionResult::validation.gate_results`
- `LegalExtractionResult::validation.is_valid` (false if a required gate fails)
- The ingestion lineage as transformation steps

| Gate                | Behavior on Failure                                    |
|---------------------|--------------------------------------------------------|
| `min_confidence`    | Warning added; document not rejected                   |
| `deontic_confidence`| Warning added; document not rejected                   |
| `section_hierarchy` | `is_valid = false`; flagged for human review (required)|
| `temporal_present`  | Warning added; document not rejected                   |
| `no_dangling_refs`  | Warning added; dangling refs listed                    |

---

## Integration with IngestionManager

### Set configuration per source

```cpp
LegalIngestionConfig cfg;
cfg.enabled = true;
mgr.setLegalIngestionConfig("source_id", cfg);
```

### Retrieve configuration

```cpp
LegalIngestionConfig out;
bool found = mgr.getLegalIngestionConfig("source_id", out);
```

### Run extraction on a document

```cpp
auto result = mgr.runLegalExtraction("doc_id", text, cfg);
```

### Lineage tracking

When `enableLineageTracking(true)` is set and a legal ingestion config is
registered for a source, the following transformation steps are recorded in
lineage:

- `deontic_extraction`
- `semantic_validation`
- `reference_validation` (when `validate_references = true`)

---

## Phase 2: LLM Integration

To activate LLM-based extraction, update the YAML schema:

```yaml
deontic_extraction:
  model: "legal-lora-adapter"
  llm_config:
    base_model: "mistral-7b"
    adapter: "legal-lora-adapter"
    adapter_rank: 32
    temperature: 0.1
    enabled: true
```

And wire the LoRA adapter into `DeonticExtractor`:

```cpp
// Example: Mistral 7B + LoRA via llama.cpp
DeonticExtractor extractor;
extractor.setExtractorFn([&inference_engine](const std::string& text) {
    auto response = inference_engine.infer(
        "Classify the deontic category of the following German legal text:\n"
        + text
    );
    return parseDeonticResponse(response);
});
```

For entity recognition, wire `DeonticExtractor::extractEntities()` to SpaCy:

```python
# Python bridge (via pybind11 or subprocess)
import spacy
nlp = spacy.load("de_legal_ner")
doc = nlp(text)
entities = [{"type": ent.label_, "value": ent.text} for ent in doc.ents]
```

---

## Testing

The legal extraction pipeline has dedicated unit tests in
`tests/test_legal_extraction.cpp` covering:

- All 7 deontic categories with German legal samples
- Entity extraction (law references, person roles, temporal, threshold values)
- Confidence threshold behavior
- Injectable extractor and validator functions
- SemanticValidator quality gates
- Document-level extraction
- AgenticReferenceValidator knowledge base and dangling reference detection
- IngestionManager and IngestionBuilder integration

Run only the legal extraction tests:

```bash
ctest -R LegalExtractionFocusedTests --output-on-failure
```
