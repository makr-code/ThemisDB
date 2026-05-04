# Training-Modul – Fehlende Implementierungen

**Generiert:** 2026-03-11  
**Validiert gegen:** Commit `c5396a31` (Branch `copilot/wire-find-similar-documents`)  
**Primärquelle:** `src/training/`, `include/training/`

---

## Zusammenfassung

Das Training-Modul hat Phase 3 abgeschlossen und ist im Status **Alpha**.
Alle in der ROADMAP als `[x]` markierten Einträge besitzen korrespondierende
Implementierungsdateien. Ein Bereich erfordert noch weitere Produktivimplementierungen
bevor der Status auf **Beta** angehoben werden kann (FINDING-T-004).
FINDING-T-001, T-002 und T-003 sind vollständig abgeschlossen.

---

## Befunde

### FINDING-T-005: AdaLoRAAdapter + LoRAAdapterMerger — ✅ Implementiert (v1.6.0)

| Feld | Wert |
|------|------|
| **Schweregrad** | Mittel |
| **Status** | ✅ Vollständig abgeschlossen (v1.6.0) |
| **Claim-Quelle** | `src/training/ROADMAP.md` Phase 5 |
| **Feature** | `AdaLoRAAdapter` — Wichtigkeitsbasiertes Rank-Pruning (`updateImportance`, `reallocateRanks`, `forward`). 39 Tests. |
| **Feature** | `LoRAAdapterMerger` — Zusammenführung (`mergeLinear`, `mergeTIES`, `*All`) mit Power-Iteration-SVD. 27 Tests. |
| **Feature** | `IncrementalTrainingConfig::lora_plus_lambda` — LoRA+ Dual-AdamOptimizer (B-Matrix lr×λ, A-Matrix lr). |
| **Tests** | `tests/test_ada_lora_adapter.cpp` (AdaLoRAFocusedTests, 39), `tests/test_lora_adapter_merger.cpp` (LoRAMergerFocusedTests, 27) |

---

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
| **Status** | ✅ Vollständig abgeschlossen (v1.6.0) |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "KnowledgeGraphEnricher" |
| **Erwartet** | `findSimilarDocuments()` führt Cosinus-Ähnlichkeitssuche über Dokumenteinbettungen durch |
| **Beobachtet** | `knowledge_graph_enricher.cpp`: Vektor-Index-Abfrage ist als kommentierte AQL-Vorlage implementiert; gibt leere Liste zurück bis Query-Executor verdrahtet ist |
| **Evidence** | `src/training/knowledge_graph_enricher.cpp` (kommentierte AQL-Templates) |
| **Lösung** | `KnowledgeGraphEnricher` akzeptiert jetzt einen optionalen `VectorIndexManager*`-Parameter via `setVectorIndex()`. Wenn verdrahtet, ruft `findSimilarDocuments()` `getVectorByPk()` zum Laden des Abfragevektors auf und führt dann `searchKnn()` für eine echte Cosinus-Ähnlichkeitssuche durch. Der Self-Doc wird ausgeschlossen, `distance` wird in einen Similarity-Score (`1 − distance`) umgewandelt, und `max_results` wird eingehalten. Ohne `VectorIndexManager` bleibt das bisherige Offline-/Testverhalten erhalten. |
| **Tests** | `tests/test_kge_vector_search.cpp` – 11 Integrationstests decken Offline-Stub, Wired-Modus, Self-Exclusion, `max_results`-Bound, Score-Bereich, Nearest-Neighbor-Ranking, fehlende Embedding-Fallback und nullptr-Reset ab. CTest-Target: `KgeVectorSearchFocusedTests`. |
| **Issue** | [FEATURE] Wire findSimilarDocuments to vector index / embedding store in KnowledgeGraphEnricher |
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

### FINDING-T-004: `deployVersion` Traffic-Splitting ✅ BEHOBEN (2026-03-11)

| Feld | Wert |
|------|------|
| **Schweregrad** | Mittel |
| **Status** | ✅ Behoben (2026-03-11) |
| **Claim-Quelle** | `src/training/README.md`, Abschnitt "IncrementalLoRATrainer" |
| **Erwartet** | `deployVersion(version, traffic_split)` leitet einen konfigurierbaren Anteil des LLM-Traffics auf den neuen Adapter um |
| **Lösung** | `selectAdapterForRequest()` (public API in `include/training/incremental_lora_trainer.h`): gewichtete Zufallsauswahl eines aktiven Adapters basierend auf den `traffic_split`-Werten in `version_registry_`. Thread-lokaler `mt19937`-PRNG; korrekte Fallback-Behandlung bei leerem Registry oder `total == 0`. |
| **Evidence** | `src/training/incremental_lora_trainer.cpp` (`Impl::selectAdapterForRequest()`), `include/training/incremental_lora_trainer.h` |

---

## Vollständig implementierte Bereiche (keine Befunde)

- ✅ `LoRACheckpointManager` — SHA-256-Integrität, atomare Rotation, Rolling-Window, Manifest-JSON
- ✅ `ProvenanceTracker` — append-only Provenanz, queryLineage(), getRecord()
- ✅ `ModalityParser` — ModalityDetector, TextClauseExtractor, TableExtractor, CitationExtractor, OCRExtractor
- ✅ `ConfidenceCalibrator` — isotonische Regression (PAV-Algorithmus), kategoriespezifische Schwellenwerte
- ✅ `LoraDataSelection` — Deduplizierung, Balancierung, Stratifizierung
- ✅ `LegalAutoLabeler::labelDocument()` — vollständig implementiert
- ✅ `LegalAutoLabeler::labelAll()` und `labelQuery()` — DB-Verdrahtung via AQL-Executor implementiert (FINDING-T-001)
- ✅ `KnowledgeGraphEnricher::findSimilarDocuments()` — Cosinus-Ähnlichkeitssuche via `VectorIndexManager` implementiert (FINDING-T-002)
- ✅ Alle ROADMAP `[x]`-Einträge haben korrespondierende Quelldateien (b2342851)
