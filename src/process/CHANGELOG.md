# Process Modeling Module – Changelog

All notable changes to this module are documented here.
Format: [Semantic Versioning](https://semver.org/), newest first.

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
