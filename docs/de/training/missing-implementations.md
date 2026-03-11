# Training-Modul – Fehlende Implementierungen

**Generiert:** 2026-03-11  
**Validiert gegen:** Commit `b2342851` (Branch `copilot/update-module-documentation-ad046901-1b9a-4382-b7ae-e0f74f440802`)  
**Primärquelle:** `src/training/`, `include/training/`

---

## Zusammenfassung

Das Training-Modul hat Phase 3 abgeschlossen und ist im Status **Alpha**.
Alle in der ROADMAP als `[x]` markierten Einträge besitzen korrespondierende
Implementierungsdateien. Vier Bereiche erfordern jedoch weitere Produktivimplementierungen
bevor der Status auf **Beta** angehoben werden kann.

---

## Befunde

### FINDING-T-001: AQL-Executor-Anbindung fehlt in `LegalAutoLabeler`

| Feld | Wert |
|------|------|
| **Schweregrad** | Hoch |
| **Status** | ✅ Vollständig abgeschlossen (v1.6.0) |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "Production Readiness" |
| **Erwartet** | `labelAll()` und `labelQuery()` lesen Dokumente über den AQL-Query-Executor |
| **Beobachtet** | `auto_labeler.cpp`: Datenbankzugriff ist als `// TODO`-Stub markiert; `labelDocument()` ist vollständig implementiert |
| **Evidence** | `src/training/auto_labeler.cpp` (TODO-Kommentare im Datenbankzugriff-Pfad) |
| **Lösung** | `LegalAutoLabeler`-Konstruktor akzeptiert jetzt einen optionalen `QueryEngine*`-Parameter. Wenn eine Engine vorhanden ist, rufen `labelAll()` und `labelQuery()` `executeAql()` auf, um Dokument-IDs aus der Datenbank abzurufen. Ohne Engine bleibt das bisherige Offline-/Testverhalten erhalten. |
| **Tests** | `tests/test_auto_labeler_db_fetch.cpp` – 13 Integrationstests decken DB-Fetch-Pfad, Offline-Fallback, Callback-Auslösung und Statistikakkumulation ab. CTest-Target: `AutoLabelerDbFetchFocusedTests`. |
| **Issue** | [FEATURE] Wire LegalAutoLabeler DB fetch to AQL query executor |
| **Label-Vorschläge** | `module:training`, `priority:high`, `type:stub` |

---

### FINDING-T-002: Vektor-Ähnlichkeitssuche in `KnowledgeGraphEnricher` ist Stub

| Feld | Wert |
|------|------|
| **Schweregrad** | Mittel |
| **Status** | Offen |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "KnowledgeGraphEnricher" |
| **Erwartet** | `findSimilarDocuments()` führt Cosinus-Ähnlichkeitssuche über Dokumenteinbettungen durch |
| **Beobachtet** | `knowledge_graph_enricher.cpp`: Vektor-Index-Abfrage ist als kommentierte AQL-Vorlage implementiert; gibt leere Liste zurück bis Query-Executor verdrahtet ist |
| **Evidence** | `src/training/knowledge_graph_enricher.cpp` (kommentierte AQL-Templates) |
| **Issue-Titelvorschlag** | `feat(training): wire findSimilarDocuments to vector index / embedding store` |
| **Label-Vorschläge** | `module:training`, `priority:medium`, `type:stub` |

---

### FINDING-T-003: LoRA-Modellgewichts-Manipulation ist simuliert

| Feld | Wert |
|------|------|
| **Schweregrad** | Hoch |
| **Status** | ✅ Behoben (PR: feat(training): implement LoRA model weight manipulation) |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "Production Readiness" |
| **Erwartet** | `IncrementalLoRATrainer` manipuliert echte LoRA-Modellgewichte und serialisiert Checkpoint-Daten |
| **Beobachtet** | `IncrementalLoRATrainer` nutzt jetzt `llm::lora::LoRALayer` + `AdamOptimizer` für echte Gewichts-Updates. Trainingsloop führt echte Forward-/Backward-Pässe und Adam-Optimizer-Schritte durch. CUDA/HIP-Beschleunigung via `GPULoRALayer` + `GPUSGDOptimizer` optional. Checkpoint-Serialisierung schreibt B- und A-Tensoren als Binärdaten. |
| **Evidence** | `src/training/incremental_lora_trainer.cpp` (`initLoRAComponents`, `runCPUTrainingStep`, `runGPUTrainingStep`, `serializeWeightTensors`, `loadCheckpointWeights`) |
| **Issue-Titelvorschlag** | `feat(training): integrate real LoRA weight manipulation via llama.cpp / libtorch` |
| **Label-Vorschläge** | `module:training`, `priority:high`, `type:stub`, `depends:llm` |

---

### FINDING-T-004: `deployVersion` Traffic-Splitting ist Konfigurations-Placeholder

| Feld | Wert |
|------|------|
| **Schweregrad** | Mittel |
| **Status** | Offen |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "IncrementalLoRATrainer" |
| **Erwartet** | `deployVersion(version, traffic_split)` leitet einen konfigurierbaren Anteil des LLM-Traffics auf den neuen Adapter um |
| **Beobachtet** | Traffic-Splitting ist ein Konfigurationsplatzhalter; Produktiveinsatz erfordert ein Routing-Layer-Update im LLM-Integrationsmodul |
| **Evidence** | `src/training/incremental_lora_trainer.cpp` (deployVersion-Implementierung) |
| **Issue-Titelvorschlag** | `feat(training/llm): wire deployVersion traffic-split to LLM routing layer` |
| **Label-Vorschläge** | `module:training`, `module:llm`, `priority:medium`, `type:integration` |

---

## Vollständig implementierte Bereiche (keine Befunde)

- ✅ `LoRACheckpointManager` — SHA-256-Integrität, atomare Rotation, Rolling-Window, Manifest-JSON
- ✅ `ProvenanceTracker` — append-only Provenanz, queryLineage(), getRecord()
- ✅ `ModalityParser` — ModalityDetector, TextClauseExtractor, TableExtractor, CitationExtractor, OCRExtractor
- ✅ `ConfidenceCalibrator` — isotonische Regression (PAV-Algorithmus), kategoriespezifische Schwellenwerte
- ✅ `LoraDataSelection` — Deduplizierung, Balancierung, Stratifizierung
- ✅ `LegalAutoLabeler::labelDocument()` — vollständig implementiert
- ✅ `LegalAutoLabeler::labelAll()` und `labelQuery()` — DB-Verdrahtung via AQL-Executor implementiert (FINDING-T-001)
- ✅ Alle ROADMAP `[x]`-Einträge haben korrespondierende Quelldateien (b2342851)
