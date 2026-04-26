> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Legal Ingestion Example: BImSchG (Bundes-Immissionsschutzgesetz)

This example demonstrates how to configure and use ThemisDB's LLM-driven legal
ingestion pipeline to ingest the **Bundes-Immissionsschutzgesetz (BImSchG)**
into a Verwaltungs-Rechts-Datenbank.

## Overview

The BImSchG is a German federal law regulating protection against harmful
environmental effects. It contains:

- **~80 paragraphs** with hierarchical structure (§ → Abs → Nr)
- **Deontic provisions**: obligations (bedarf, muss), permissions (darf, kann),
  prohibitions (darf nicht, ist verboten)
- **Cross-references** to other laws (BNatSchG, KrWG, UmwG)
- **Temporal validity** with amendment history since 1974

---

## Step 1: Configure the Ingestion Schema

The `config/ingestion/legal-ingestion-schema.yaml` file defines the extraction
rules for German legal texts. The key sections for BImSchG are:

```yaml
ingestion_rules:
  deontic_extraction:
    enabled: true
    categories: [obligation, permission, prohibition, definition, condition, exception]
    confidence_threshold: 0.75

  entity_extraction:
    enabled: true
    entity_types: [law_reference, person_role, organization, temporal, threshold_value]

  temporal_analysis:
    enabled: true

  quality_gates:
    min_extraction_confidence: 0.80
    min_deontic_extraction_confidence: 0.75
```

---

## Step 2: Register the Source

```cpp
#include "ingestion/ingestion_manager.h"
using namespace themis::ingestion;

// Option A: Filesystem source (pre-downloaded BImSchG XML)
auto mgr = IngestionBuilder("themis_legal_db")
    .withFilesystemSource("bimschg",
        "/data/laws/bimschg",
        {{"format", "auto"}, {"recursive", "true"}, {"ocr_enabled", "false"}},
        10 /* high priority */)
    .withLegalIngestionConfig("bimschg", [] {
        LegalIngestionConfig cfg;
        cfg.enabled              = true;
        cfg.confidence_threshold = 0.75;
        cfg.validate_references  = true;
        return cfg;
    }())
    .withTargetCollection("legal_documents")
    .build();

// Option B: Web crawler (live from gesetze-im-internet.de)
auto mgr2 = IngestionBuilder("themis_legal_db")
    .withWebCrawlerSource("bimschg_live",
        "https://www.gesetze-im-internet.de/bimschg/",
        {{"max_depth", "2"}, {"same_domain_only", "true"}},
        8)
    .withLegalIngestionConfig("bimschg_live", [] {
        LegalIngestionConfig cfg;
        cfg.enabled             = true;
        cfg.validate_references = true;
        return cfg;
    }())
    .build();
```

---

## Step 3: Run Extraction on a Single Document

```cpp
IngestionManager mgr("themis_legal_db");
LegalIngestionConfig cfg;
cfg.enabled              = true;
cfg.confidence_threshold = 0.75;
cfg.validate_references  = true;

std::string bimschg_text = R"(
§ 4 Genehmigung

(1) Die Errichtung und der Betrieb von Anlagen, die auf Grund ihrer
Beschaffenheit oder ihres Betriebs in besonderem Maße geeignet sind,
schädliche Umwelteinwirkungen herbeizuführen oder in anderer Weise
die Allgemeinheit oder die Nachbarschaft zu gefährden, erheblich zu
benachteiligen oder erheblich zu belästigen, sowie von ortsfesten
Abfallentsorgungsanlagen zur Lagerung oder Behandlung von Abfällen
bedürfen einer Genehmigung.

§ 5 Pflichten der Betreiber genehmigungsbedürftiger Anlagen

(1) Genehmigungsbedürftige Anlagen sind so zu errichten und zu
betreiben, dass
1. schädliche Umwelteinwirkungen und sonstige Gefahren nicht
   hervorgerufen werden können;
2. Vorsorge gegen schädliche Umwelteinwirkungen getroffen wird,
   insbesondere durch die dem Stand der Technik entsprechenden
   Maßnahmen zur Emissionsbegrenzung.
)";

auto result = mgr.runLegalExtraction("BImSchG_2024", bimschg_text, cfg);
```

---

## Step 4: Inspect the Extraction Result

```cpp
// Document-level quality
std::cout << "Quality score: " << result.quality_score << "\n";
std::cout << "Provisions extracted: " << result.provisions.size() << "\n";

// Per-provision results
for (const auto& prov : result.provisions) {
    std::cout << "Section: " << prov.section_ref << "\n";
    std::cout << "  Deontic: "
              << deonticCategoryToString(prov.deontic_category)
              << " (conf: " << prov.category_confidence << ")\n";
    std::cout << "  Entities: " << prov.entities.size() << "\n";
    for (const auto& ent : prov.entities) {
        std::cout << "    [" << ent.type << "] " << ent.value << "\n";
    }
    for (const auto& obl : prov.obligations) {
        std::cout << "  Obligation: actor='" << obl.actor
                  << "' action='" << obl.action << "'\n";
    }
}

// Quality gates
for (const auto& gate : result.validation.gate_results) {
    std::cout << "Gate '" << gate.name << "': "
              << (gate.passed ? "PASSED" : "FAILED");
    if (!gate.passed) std::cout << " (" << gate.reason << ")";
    std::cout << "\n";
}

// Warnings
for (const auto& w : result.warnings) {
    std::cout << "Warning: " << w << "\n";
}
```

---

## Expected Output

```
Quality score: 0.87
Provisions extracted: 2

Section: §4
  Deontic: obligation (conf: 0.95)
  Entities: 2
    [law_reference] §4
    [person_role] Betreiber
  Obligation: actor='Betreiber' action='einer Genehmigung einholen'

Section: §5
  Deontic: obligation (conf: 0.95)
  Entities: 1
    [threshold_value] Stand der Technik

Gate 'min_confidence': PASSED
Gate 'deontic_confidence': PASSED
Gate 'section_hierarchy': PASSED
Gate 'temporal_present': FAILED (no temporal expression found)
Gate 'no_dangling_refs': PASSED
```

---

## Expected JSON Output Format

The full extraction result maps to the following JSON structure:

```json
{
  "document_id": "BImSchG_2024",
  "quality_score": 0.87,
  "provisions": [
    {
      "provision_id": "BImSchG_2024_§4",
      "section_ref": "§4",
      "deontic_category": "obligation",
      "category_confidence": 0.95,
      "text": "Die Errichtung und der Betrieb von Anlagen ... bedürfen einer Genehmigung.",
      "entities": [
        {"type": "law_reference", "value": "§4", "confidence": 0.85},
        {"type": "person_role", "value": "Betreiber", "confidence": 0.85}
      ],
      "obligations": [
        {"actor": "Betreiber", "action": "einer Genehmigung einholen", "confidence": 0.75}
      ]
    },
    {
      "provision_id": "BImSchG_2024_§5",
      "section_ref": "§5",
      "deontic_category": "obligation",
      "category_confidence": 0.95,
      "text": "Genehmigungsbedürftige Anlagen sind so zu errichten und zu betreiben...",
      "entities": [
        {"type": "threshold_value", "value": "Stand der Technik", "confidence": 0.85}
      ]
    }
  ],
  "extraction_metadata": {
    "gates_passed": ["min_confidence", "deontic_confidence", "section_hierarchy", "no_dangling_refs"],
    "gates_failed": ["temporal_present"],
    "warnings": []
  }
}
```

---

## Lineage Tracking

When lineage tracking is enabled, the ingestion pipeline records the
transformation steps applied to each document:

```cpp
mgr.enableLineageTracking(true);
auto stats = mgr.ingestSource("bimschg");

// Inspect lineage
for (const auto& record : mgr.getLineageRecords("bimschg")) {
    for (const auto& step : record.transformation_steps) {
        std::cout << "Step: " << step << "\n";
    }
}
// Output:
// Step: deontic_extraction
// Step: semantic_validation
// Step: reference_validation
```

---

## Phase 2 (Future): LLM-Enhanced Extraction

Once the LoRA adapter is trained on German legal texts, enable LLM-based
extraction by updating `legal-ingestion-schema.yaml`:

```yaml
deontic_extraction:
  model: "legal-lora-adapter"
  llm_config:
    base_model: "mistral-7b"
    adapter: "legal-lora-adapter"
    enabled: true
```

The `DeonticExtractor` supports a pluggable extraction function that can be
wired to any LLM backend:

```cpp
DeonticExtractor extractor;
extractor.setExtractorFn([&llm_client](const std::string& text) {
    auto response = llm_client.complete(
        "Extract deontic categories from: " + text);
    return parseLlmResponse(response);
});
```
