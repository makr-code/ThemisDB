# Process Modeling Module Roadmap

**Version:** 1.0.0
**Status:** 🟡 Beta
**Last Updated:** 2026-04-06
**Module Path:** `src/process/`

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status

Beta-ready for core process modelling (BPMN, EPK, VCC-VPB), process linking, and Graph-RAG retrieval for Verwaltungsvorgänge. Pre-loaded VCC-VPB model library (17 models, 5 domains) operational. Embedding-based similarity search requires pre-computed embeddings (not yet auto-generated).

## Completed ✅

- [x] `ProcessModelManager`: import/export BPMN 2.0, EPK, VCC-VPB YAML
- [x] BPMN 2.0 XML serializer – state-machine tokenizer (namespace-aware, no regex, nested sub-processes, `conditionExpression` child, 10 MiB security guard)
- [x] EPK text and JSON serializer
- [x] `VccVpbImporter`: single model, batch list, and directory import
- [x] `LlmProcessDescriptor`: generate structured JSON descriptor + system-prompt-ready text; conformance-checking prompt builder; multi-model summary
- [x] Base-entity storage layer (`proc:def:` key prefix, versioned revisions)
- [x] `ProcessLinker`: attach/detach documents and metadata to process instances (hard-delete via `db_.del()`, secondary index `proc:obj_idx:` for `findInstancesWithObject()`)
- [x] `ProcessLinker`: process-to-process linking (sub-process, cross-reference, triggers)
- [x] `ProcessLinker`: required document registry per process node
- [x] `ProcessLinker`: missing document detection via cross-reference
- [x] `ProcessGraphRag`: `KnowledgeGraph` population from process models (`buildKnowledgeGraph`)
- [x] `ProcessGraphRag`: instance knowledge graph with attachment and token nodes (`buildInstanceKnowledgeGraph`)
- [x] `ProcessGraphRag`: BFS subgraph extraction (`extractSubgraph`)
- [x] `ProcessGraphRag`: `retrieve()` and `retrieveForNode()` context assembly
- [x] `ProcessGraphRag`: Verwaltungsvorgang summary (`summarizeVerwaltungsvorgang`)
- [x] `ProcessGraphRag`: compliance checking (`checkCompliance`)
- [x] `ProcessGraphRag`: similar case finding via cosine + Jaccard fallback (`findSimilarCases`)
- [x] `ProcessGraphRag`: German and English LLM prompt builder
- [x] 17 pre-loaded VCC-VPB administrative models (5 domains: Verwaltung, IT, Gesundheit, Finanzen, Kundenservice)
- [x] AQL-queryable `_process_definitions` system collection (base-entity layer)
- [x] `ProcessRagConfig` struct with full tuning parameters

## In Progress 🚧

- [~] Pre-computed embedding storage for semantic similarity search
  - `proc:inst_emb:<id>` key scheme implemented; auto-generation from LLM module pending

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Auto-generate and persist process model embeddings via LLM module on import (Target: Q2 2026)
  - Affected: `ProcessModelManager::save()`, `llm_process_descriptor.cpp`, LLM module API
  - Expected: call configured LLM embedding endpoint after each `importBpmn()`/`importEpk()`/`importVccVpb()`; store result in `ProcessModelRecord::embedding`
  - Tests: embedding round-trip + cosine similarity regression test
  - Perf: embedding call must not block import pipeline (async dispatch)

- [ ] Auto-generate and persist process instance embeddings after state change (Target: Q2 2026)
  - Affected: `ProcessGraphManager` (post-execution hook), `ProcessGraphRag::findSimilarCases()`
  - Expected: on instance COMPLETED/FAILED, compute embedding of summary JSON; store under `proc:inst_emb:<id>`
  - Tests: similarity ranking correctness vs ground-truth labelled case pairs

- [ ] Full-text inverted index over process model descriptions (Target: Q2 2026)
  - Affected: `ProcessModelManager::search()`, `include/index/inverted_index.h`
  - Expected: TF-IDF ranked search across all process node names, descriptions, and compliance tags
  - Perf: < 50 ms for search over 10,000 models
  - Tests: recall ≥ 0.85 on a 50-query benchmark over the VCC-VPB model library

- [ ] BPMN BPMNDI layout import (Target: Q2 2026)
  - Affected: `bpmn_serializer.cpp`
  - Expected: import graphical `x`/`y` positions from BPMNDI section; store as `node.metadata.layout`
  - Tests: round-trip check on BPMNDI sample files

- [ ] AgenticRAG integration for iterative process question answering (Target: Q3 2026)
  - Affected: `process_graph_rag.cpp`, `rag/agentic_rag.h`
  - Expected: multi-turn conversation loop about a Verwaltungsvorgang; agent can call `getMissingDocuments`, `getLinks`, `findSimilarCases` as tools
  - Tests: conformance checking scenarios with iterative refinement; max 5 LLM turns per question
  - Perf: total latency ≤ 5 s for 3-turn conversation on local llama.cpp

### Long-term (6–12 months)

- [ ] EPK ARIS-XML import (Target: Q3 2026)
  - Affected: `epk_serializer.cpp` + new `aris_xml_parser.cpp`
  - Expected: import `.epk` files exported from ARIS tooling; map ARIS function/event/connector objects to `ProcessNodeInfo`
  - Tests: import round-trip for 5 representative ARIS EPK files; no data loss on node/edge count
  - Errors: unsupported ARIS node types → log warning + skip; malformed XML → structured error

- [ ] BPMN-S (BPMN Security Profile) support (Target: Q4 2026)
  - Affected: `bpmn_serializer.cpp`, `ProcessModelRecord`, `ProcessGraphRag::checkCompliance()`
  - Expected: annotate BPMN nodes with DSGVO data-handling requirements (personal data, retention period, legal basis); expose in compliance check
  - Tests: compliance check correctly flags missing DSGVO annotations; integration test with a BPMN-S sample model
  - Constraints: must not break existing BPMN 2.0 import/export

- [ ] Real-time SLA monitoring and alert dispatch (Target: Q4 2026)
  - Affected: `process_graph_rag.cpp`, scheduler module, CEP engine (`analytics/cep_engine.cpp`)
  - Expected: register SLA CEP rule per active instance; dispatch alert (webhook/Slack/email) when instance is at risk or overdue; deregister on completion
  - Tests: CEP alert fires within 100 ms of SLA threshold crossing; no false positives
  - Errors: alert dispatch failure → log + retry with exponential back-off (max 3 retries)

- [ ] Cross-case graph analytics: identify bottlenecks across all Vorgänge (Target: Q4 2026)
  - Affected: `ProcessGraphRag` + `analytics/process_mining.cpp`
  - Expected: aggregate token dwell-time per node across all completed instances; report top-5 bottleneck nodes; expose via AQL `PROCESS_BOTTLENECKS(model_id)` function
  - Tests: synthetic dataset of 1,000 instances with known bottleneck; detection accuracy ≥ 90 %
  - Perf: analysis ≤ 2 s for 10,000 completed instances

## Implementation Phases

### Phase 1: Core Process Modelling (Status: Completed ✅)

- [x] `ProcessModelManager` CRUD with RocksDB storage
- [x] BPMN 2.0 XML import/export (`BpmnSerializer`)
- [x] EPK text/JSON import/export (`EpkSerializer`)
- [x] VCC-VPB YAML import (`VccVpbImporter`)
- [x] LLM descriptor generation (`LlmProcessDescriptor`)

### Phase 2: Linking & Attachment (Status: Completed ✅)

- [x] `ProcessLinker`: attach documents and metadata to instances
- [x] `ProcessLinker`: process-to-process links (sub-process, cross-reference, triggers)
- [x] Required document registry per process node
- [x] Missing document detection via cross-reference

### Phase 3: Graph-RAG Integration (Status: Completed ✅)

- [x] `ProcessGraphRag`: `KnowledgeGraph` population from process model
- [x] `ProcessGraphRag`: instance knowledge graph with attachment nodes
- [x] `ProcessGraphRag`: BFS subgraph extraction
- [x] `ProcessGraphRag`: full retrieval context assembly for LLM
- [x] `ProcessGraphRag`: Verwaltungsvorgang JSON summary
- [x] `ProcessGraphRag`: compliance checking (required docs, SLA, state)
- [x] German and English LLM prompt builder

### Phase 4: Semantic Search (Status: In Progress 🚧)

- [~] Embedding storage and retrieval for process models (`proc:inst_emb:`)
- [ ] Auto-generation of embeddings via LLM module on import (Target: Q2 2026)
- [ ] Full-text inverted index integration (Target: Q2 2026)
- [ ] HNSW-based process model similarity search (Target: Q2 2026)
- [ ] AgenticRAG integration for iterative Q&A (Target: Q3 2026)

### Phase 5: Advanced Features (Status: Planned)

- [ ] EPK ARIS-XML import (Target: Q3 2026)
- [ ] BPMN-S security profile for DSGVO compliance (Target: Q4 2026)
- [ ] Real-time SLA monitoring via CEP engine (Target: Q4 2026)
- [ ] Cross-case bottleneck analytics (Target: Q4 2026)

### Phase 7: State-of-the-Art – SotA-Derived Features (Status: Planned)

> Wissenschaftliche Grundlagen: [`docs/de/process/STATE_OF_THE_ART.md`](../../docs/de/process/STATE_OF_THE_ART.md)

- [ ] PPR-basiertes GraphRAG Scoring (HippoRAG-Ansatz, Gutierrez 2024) (Target: Q2 2026)
  - Ersetzt BFS in `ProcessGraphRag::extractSubgraph()` durch Personalized PageRank
  - Multi-Hop-Anfragen werden korrekt bewertet; relevante entfernte Knoten fließen in Kontext
  - Perf: ≤ 20 ms für 500-Knoten-Graph; Tests: 3-Hop-Anfrage korrekt aufgelöst
- [ ] LLM-to-BPMN Generator (ProcessGPT, Busch 2023) (Target: Q2 2026)
  - `ProcessModelGenerator::generateFromDescription()`: Freitext → ProcessModelRecord
  - Max 3 Validierungsrunden (generate → BPMN-check → fix); keine Deadlocks/isolierten Knoten
  - Tests: generiertes Bauantrag-Modell hat ≥ 5 Knoten, ≥ 1 Gateway, deploybar
- [ ] OCEL 2.0 Export (Berti 2023) (Target: Q2 2026)
  - `OcelExporter::exportInstance/exportModel()` → PM4Py/Celonis-kompatibles JSON
  - Event-Objekt-Beziehungen aus ProcessLinker-Anhängen
  - Tests: OCEL 2.0 JSON-Schema-Validierung; Round-trip mit PM4Py
- [ ] Leiden-Community-Detection für Prozesscluster (GraphRAG, Edge 2024) (Target: Q3 2026)
  - `ProcessCommunityDetector::detect()` → thematische Knotengruppen
  - LLM-Community-Reports pro Cluster, gecacht unter `proc:community:`
  - Globale Anfragen ("Beschreibe den Genehmigungsablauf") über Reports statt Knotentraversal
  - Perf: Recompute < 500 ms für 500 Knoten
- [ ] Duales Retrieval Local/Global (LightRAG, Guo 2024) (Target: Q3 2026)
  - `ProcessLightRetriever::retrieve(query, instance_id, mode: LOW|HIGH|AUTO)`
  - Low = Entity-zentriert (Sachbearbeiter-Anfragen), High = Community-zentriert (Bürger-Anfragen)
  - AUTO wählt Modus basierend auf Anfrage-Typ (spezifisch vs. konzeptuell)
- [ ] Object-Centric Process Mining / OCPM (van der Aalst 2022) (Target: Q3 2026)
  - `ObjectCentricTracer`: OCEL 2.0 Log aus Instanz + Anhängen; DFG pro Objekttyp
  - Konvergenz/Divergenz-Analyse für Verwaltungsvorgänge (Antragsteller, Dokument, Prüfer)
  - Perf: DFG-Berechnung ≤ 5 s für 10.000 Events
- [ ] DMN 1.5 Entscheidungstabellen (OMG 2023) (Target: Q3 2026)
  - `DmnEvaluator::loadFromXml/Json()`, `evaluate()`, `evaluateFeel()`
  - FEEL-Subset: numerische Vergleiche, Bereiche `[a..b]`, String-Gleichheit
  - Integration in `checkCompliance()`: DMN-referenzierende Knoten werden zur Laufzeit ausgewertet
- [ ] FIM-Prozessbibliothek-Import (FITKO 2024) (Target: Q4 2026)
  - `FimImporter::importFimXml()`, `importFimCatalogue()`, `importFromFitkoApi()`
  - 5.000+ standardisierte Verwaltungsprozesse aus dem Bundesportal importierbar
  - FIM-Leistungscode in `compliance_tags` erhalten
- [ ] CMMN 1.1 Case Management Support (OMG 2016) (Target: Q4 2026)
  - `CmmnSerializer::importXml/exportXml()` für adaptive Fallmodelle
  - Discretionary Tasks: Sachbearbeiter entscheidet Reihenfolge zur Laufzeit
  - `ProcessNotation::CMMN_1_1` als neuer Notation-Typ
- [ ] ProcessTransformer Vorhersage (Bukhsh 2021) (Target: Q1 2027)
  - `ProcessPredictor::predict()`: nächste Aktivität, Outcome, verbleibende Zeit, Bearbeiter
  - Training auf abgeschlossenen Instanzen via `updateModel()`
  - SHAP-Aktivitäts-Wichtigkeit für Erklärbarkeit (Verwaltungs-Transparenzgebot)
  - Accuracy-Ziel: ≥ 85 % Next-Activity-Prediction auf BPIC-Benchmarks

- [ ] Unit test coverage > 90 % for all components (Target: Q2 2026)
- [ ] Integration tests with real VCC-VPB model library (Target: Q2 2026)
- [ ] Performance benchmarks: import time, retrieval latency, prompt size (Target: Q2 2026)
- [ ] Security audit: input validation for BPMN/EPK/YAML parsers (Target: Q2 2026)
- [ ] Documentation complete (Target: Q2 2026)

## Production Readiness Checklist

- [x] Core import/export operational (BPMN, EPK, VCC-VPB)
- [x] Graph-RAG retrieval operational
- [x] ProcessLinker attachment and required-document enforcement
- [x] German LLM prompt builder
- [x] Compliance and SLA checking
- [ ] Unit test coverage > 90 % (Target: Q2 2026)
- [ ] Integration tests with LLM embedding endpoint (Target: Q2 2026)
- [ ] Performance benchmarks documented (Target: Q2 2026)
- [ ] Security audit of BPMN/EPK/YAML parsers (Target: Q2 2026)
- [ ] Documentation complete (Target: Q2 2026)

## Known Issues & Limitations

- Embedding-based similarity search in `findSimilarCases()` requires pre-computed embeddings stored under `proc:inst_emb:<id>`; auto-generation is not yet implemented.
- VCC-VPB YAML parser handles the VCC-VPB subset; full YAML 1.2 constructs (anchors, custom tags) are not supported.
- EPK import uses implicit sequential flow for lines without explicit arrows; complex EPK models with branches may require manual edge specification.
- `findInstancesWithObject()` now uses a secondary reverse-lookup index (`proc:obj_idx:`) for O(prefix-scan) performance; the index is maintained by `attachObject()`/`detachObject()`. Attachments created before this change (that lack index entries) will not appear in the index scan but will still appear in `getAttachments()` direct scans.
