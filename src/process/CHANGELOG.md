> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

# Process Modeling Module – Changelog

All notable changes to this module are documented here.
Format: [Semantic Versioning](https://semver.org/), newest first.

---

## [2.1.0] – 2026-04-17

### Added

- **`EpkArisXmlImporter`** (`include/process/epk_aris_xml_importer.h`,
  `src/process/epk_aris_xml_importer.cpp`):
  EPK import from ARIS Markup Language (AML) XML produced by ARIS Designer 9.x/10.x.
  - `importAml(aml_xml)` — imports the first EPK model in an AML document.
  - `importAllAml(aml_xml)` — imports all EPK models.
  - `typeNumToEpkNodeType(type_num)` — maps ARIS TypeNum to `EPKNodeType`.
  - `typeNumToLabel(type_num)` — returns German ARIS element label.
  - Supports: Funktion (1), Ereignis (14), AND/OR/XOR connectors (13/12/11),
    Organisationseinheit (18), Informationsobjekt (15), Anwendungssystem (40),
    Prozesswegweiser (16).
  - Hand-written XML tokenizer (same strategy as BpmnSerializer; no regex, no external
    XML library). 10 MiB document guard. Numeric character reference decoding (`&#N;`).
  - 10 tests: EAX-01..EAX-10.

- **`ProcessModelManager::importArisXml()`**
  (`include/process/process_model_manager.h`,
  `src/process/process_model_manager.cpp`):
  High-level convenience method that calls `EpkArisXmlImporter::importAml()`, builds
  the normalized graph via `buildNormalizedGraph_()`, and calls `save()` (including
  automatic FTS indexing and embedding generation when those are wired).

- **`ProcessAgenticRag`** (`include/process/process_agentic_rag.h`,
  `src/process/process_agentic_rag.cpp`):
  Iterative agentic Q&A façade bridging `ProcessGraphRag` and
  `rag::agentic::AgenticRAG`.
  - `iterativeQuery(instance_id, question)` — multi-hop Q&A with gap-driven
    query reformulation.
  - `iterativeQueryForNode(instance_id, node_id, question)` — node-scoped entry point.
  - `ProcessAgenticConfig` — tuning parameters (max_iterations, quality_threshold,
    faithfulness_threshold, max_total_documents, rag_config).
  - `ProcessAgenticResult` — final context, LLM prompt, quality_satisfied flag,
    iteration history.
  - `encodeContext()` / `mergeDocuments()` — bridge between `ProcessRagContext` and
    `std::vector<rag::judge::RetrievedDocument>`.
  - 6 façade tests: PAR-01..PAR-06 (in `tests/test_process_aris_xml.cpp`).

---

## [2.0.0] – 2026-04-15

### Added

- **`ProcessModelGenerator`** (`src/process/process_model_generator.cpp`,
  `include/process/process_model_generator.h`):
  LLM-to-BPMN generator. Converts natural language process descriptions into
  `ProcessModelRecord` objects via iterative LLM calls (up to `Config::max_retries`
  validate-and-fix cycles). Based on ProcessGPT (Busch 2023).
  - `generateFromDescription(description, cfg)` — builds a BPMN model from text.
  - `refine(existing, feedback, cfg)` — iteratively refines an existing model.
  - `validate(normalized_graph)` — BPMN semantic checks (startEvent, endEvent,
    no isolated nodes, gateway outgoing edges).
  - `fromLlmJson(llm_json, domain)` — converts LLM JSON response to `ProcessModelRecord`.
  - 7 tests: PMG-01..PMG-07.

- **`OcelExporter`** (`src/process/ocel_exporter.cpp`,
  `include/process/ocel_exporter.h`):
  OCEL 2.0 export for process mining tools (PM4Py, Celonis, ProM). Based on
  OCEL 2.0 spec (Berti 2023, doi:10.5281/zenodo.8428111).
  - `exportInstance(instance_id)` — single instance to OCEL 2.0 JSON.
  - `exportModel(model_id)` — all instances of a model to OCEL 2.0 JSON.
  - `exportFiltered(model_id, from_ms, to_ms)` — time-window export.
  - Objects from `ProcessLinker` attachments; events from `ProcessToken` visit log.
  - 4 tests: OCEL-01..OCEL-04.

- **PPR-based GraphRAG Scoring** (`src/process/process_graph_rag.cpp`,
  `include/process/process_graph_rag.h`):
  Personalized PageRank as an alternative to BFS in `ProcessGraphRag::retrieve()`.
  Based on HippoRAG (Gutierrez 2024, NeurIPS 2024).
  - `computePpr(normalized_graph, seeds, config)` — power-iteration PPR.
  - `PprConfig` struct with `damping`, `max_iterations`, `convergence_epsilon`, `top_k_nodes`.
  - `ProcessRagConfig::use_ppr` — enables PPR path in `retrieve()`.
  - 5 tests: PPR-01..PPR-05.

- **`DmnEvaluator`** (`src/process/dmn_evaluator.cpp`,
  `include/process/dmn_evaluator.h`):
  DMN 1.5 decision table evaluator. Based on OMG DMN 1.5 spec.
  - `loadFromJson(dmn_json)` — loads from JSON schema.
  - `loadFromXml(dmn_xml)` — simplified DMN 1.5 XML parser (state-machine tokenizer).
  - `evaluate(decision_id, context)` — evaluates decision with UNIQUE/FIRST/COLLECT
    hit policy support.
  - `evaluateFeel(expr, value)` — FEEL subset evaluator: numeric comparisons,
    ranges `[a..b]`, string equality, null/boolean, wildcard `-`.
  - 10 tests: DMN-01..DMN-10.

---

## [1.0.0] – 2026-03-12

### Added

- **`ProcessModelManager`** (`src/process/process_model_manager.cpp`):
  Import BPMN 2.0 XML, EPK text/JSON, VCC-VPB YAML; CRUD with RocksDB versioned
  storage (`proc:def:<id>`, `proc:def:<id>:rev:<n>`); list/search/findSimilar queries;
  deploy/undeploy to `ProcessGraphManager`.

- **`BpmnSerializer`** (`src/process/bpmn_serializer.cpp`):
  BPMN 2.0 XML import (regex-based, lenient; ignores BPMNDI graphical data) and
  export (ISO/IEC 19510). Supports start/end events, user tasks, service tasks,
  script tasks, gateways (exclusive, inclusive, parallel, event-based), and all
  sequence/message/association flow types.

- **`EpkSerializer`** (`src/process/epk_serializer.cpp`):
  EPK text notation and EPK JSON import/export. Supports events, functions, and
  AND/OR/XOR connectors.

- **`VccVpbImporter`** (`src/process/vcc_vpb_importer.cpp`):
  Single model, batch list, and directory import from VCC-VPB YAML files.
  Validates required fields; reports per-model errors without aborting batch.

- **`LlmProcessDescriptor`** (`src/process/llm_process_descriptor.cpp`):
  Generate structured JSON descriptor and system-prompt-ready text for a process
  model; conformance-checking prompt builder; multi-model summary table.

- **`ProcessLinker`** (`src/process/process_linker.cpp`,
  `include/process/process_linker.h`):
  - Attach/detach documents and metadata to process instances with typed
    `ProcessLinkType` semantics (8 types).
  - Process-to-process linking (sub-process, cross-reference, triggers, etc.).
  - Required document registry per process node
    (`proc:req_doc:<model_id>:<node_id>:<doc_type>` key prefix).
  - Missing document detection via cross-reference of registry vs attachments.
  - `getNodeAttachments()` for node-scoped attachment retrieval.
  - `findInstancesWithObject()` for reverse lookup.

- **`ProcessGraphRag`** (`src/process/process_graph_rag.cpp`,
  `include/process/process_graph_rag.h`):
  - `buildKnowledgeGraph(model_id)`: converts normalised process nodes/edges to
    `KGNode`/`KGEdge` objects for `KnowledgeGraphRetriever`.
  - `buildInstanceKnowledgeGraph(instance_id)`: adds instance-state, token, and
    attachment nodes to the model knowledge graph.
  - `extractSubgraph(model_id, seeds, depth)`: BFS subgraph extraction up to
    configurable depth.
  - `retrieve(instance_id, query, config)`: full Graph-RAG context assembly –
    instance state, subgraph, attachments, missing docs, similar cases, LLM prompt.
  - `retrieveForNode(instance_id, node_id, query)`: node-scoped context variant.
  - `summarizeVerwaltungsvorgang(instance_id)`: structured JSON summary with
    state, current tasks, progress %, missing documents, compliance status, SLA
    status, attachment count, and variables.
  - `checkCompliance(instance_id)`: checks required-document presence, SLA
    adherence, and non-FAILED instance state; returns score and violation list.
  - `findSimilarCases(instance_id, k, min_similarity)`: cosine similarity over
    stored instance embeddings (`proc:inst_emb:<id>`); falls back to variable-key
    Jaccard similarity when no embeddings are available.
  - `buildAdminProcessingPrompt(ctx)`: German administrative prompt in structured
    Verwaltungsvorgang format.
  - `buildQueryPrompt(ctx)`: appends user query to the admin prompt.
  - `ProcessRagConfig`: full tuning struct (depth, similar-case count, language,
    token budget, feature flags).

- **17 pre-loaded VCC-VPB administrative models** in `config/process_models/`,
  spanning 5 domains: Verwaltung, IT, Gesundheit, Finanzen, Kundenservice.

- **AQL-queryable `_process_definitions` system collection** (base-entity layer):
  process models are stored as standard ThemisDB documents and can be queried
  with full AQL syntax including `FILTER`, `SORT`, `LIMIT`, and graph traversals.

- **German and English LLM output support** via `ProcessRagConfig::language`
  (`"de"` produces German prompts with German section headers; `"en"` produces
  English output).

### Architecture

- Process models stored as base-entity documents under `proc:def:` key prefix.
- Versioned revision snapshots under `proc:def:<id>:rev:<n>`.
- Attachments stored under `proc:attach:<instance_id>:<object_id>`.
- Process-to-process links stored under `proc:link:<source_id>:<target_id>:<link_type>`.
- Required document entries under `proc:req_doc:<model_id>:<node_id>:<doc_type>`.
- Instance embeddings (optional) under `proc:inst_emb:<instance_id>`.
- Soft-delete via tombstone pattern for attachments (`{"deleted": true}`).

### Known Limitations

- BPMN parser is regex-based, not a full XML DOM parser; deeply nested BPMN
  structures may require manual post-processing.
- Embedding-based similarity search requires pre-computed embeddings; auto-
  generation via the LLM module is planned for Q2 2026.
- VCC-VPB YAML parser covers the VCC-VPB subset; YAML 1.2 anchors and custom
  tags are not supported.
- EPK import uses implicit sequential flow for non-arrow lines; complex EPK
  branching may need explicit edge specification.
