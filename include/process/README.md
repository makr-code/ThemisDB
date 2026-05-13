> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release --target <target>`

# Process Module – API Reference

**Module Path:** `include/process/` · `src/process/`
**Version:** 1.0.0
**Namespace:** `themis::process`

---

## Usage

### Quick Start

```cpp
#include "process/process_model_manager.h"
#include "process/process_linker.h"
#include "process/process_graph_rag.h"
#include "index/process_graph.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis;
using namespace themis::process;

// One-time setup
RocksDBWrapper      db = /* open DB */;
ProcessGraphManager engine(db);
ProcessModelManager models(db);
ProcessLinker       linker(db);
ProcessGraphRag     rag(db, engine, models, linker);

// --- Import a BPMN process model ---
ProcessModelRecord meta;
meta.id = "bauantrag_standard";
meta.name = "Bauantrag Standard";
meta.domain = ProcessDomain::ADMINISTRATION;
meta.compliance_tags = {"DSGVO", "§34 BauO NRW"};
auto import_res = models.importBpmn("<bpmn:definitions>...</bpmn:definitions>", meta);

// --- Attach a document to a running instance ---
auto [ok, attach_id] = linker.attachObject(
    "inst-42",                 // instance ID
    "doc-7",                   // document ID
    "documents",               // collection
    ProcessLinkType::HAS_DOCUMENT,
    "pruefung",                // node_id (optional)
    {{"doc_type", "Bauzeichnung"}, {"mandatory", true}},
    "sachbearbeiter@amt.de"
);

// --- Graph-RAG: build LLM context for a query ---
ProcessRagConfig cfg;
cfg.language = "de";
cfg.max_similar_cases = 3;

auto ctx = rag.retrieve("inst-42", "Was fehlt noch?", cfg);
// ctx.llm_prompt → ready to send to LLM
// ctx.missing_documents → ["Lageplan"]

// --- Structured summary for the UI ---
auto summary = rag.summarizeVerwaltungsvorgang("inst-42");
// { "state": "RUNNING", "progress_pct": 45.0, "missing_documents": [...] }

// --- Compliance check ---
auto comp = rag.checkCompliance("inst-42");
// { is_compliant: false, violations: ["Fehlende Pflichtunterlage: Lageplan"] }
```

---

## Data Flow

```
VCC-VPB YAML / BPMN XML / EPK Text
        │
        ▼
ProcessModelManager.importBpmn/Epk/VccVpb()
        │  stores proc:def:<id>
        ▼
ProcessModelRecord { id, name, normalized, compliance_tags, embedding }
        │
        │ deployToEngine()
        ▼
ProcessGraphManager { instances, tokens, history }
        │
        │ attachObject()       │ linkProcesses()
        ▼                      ▼
ProcessLinker { proc:attach:, proc:link:, proc:req_doc: }
        │
        │ retrieve(instance_id, query)
        ▼
ProcessGraphRag
  ├── extractSubgraph()        → KnowledgeGraph nodes/edges
  ├── getAttachments()         → attached documents
  ├── getMissingDocuments()    → missing required docs
  ├── findSimilarCases()       → cosine/Jaccard similarity
  └── assemblePrompt_()        → llm_prompt (DE/EN)
        │
        ▼
ProcessRagContext { llm_prompt, subgraph, attachments, missing_documents }
        │
        ▼
LLM endpoint (via KnowledgeGraphRetriever or direct)
```

---

## ProcessModelManager API

**Header:** `include/process/process_model_manager.h`

| Method | Description |
|---|---|
| `importBpmn(xml, meta)` | Import BPMN 2.0 XML; returns `ProcessModelResult` |
| `importEpk(text, meta)` | Import EPK text or JSON notation |
| `importVccVpb(yaml, meta)` | Import VCC-VPB YAML |
| `importArisXml(aml_xml, meta)` | Import ARIS AML XML |
| `save(record)` | Store/update a `ProcessModelRecord` (versioned) |
| `load(id)` | Return `optional<ProcessModelRecord>` |
| `remove(id)` | Soft-delete (sets state to ARCHIVED) |
| `list(domain, state, limit)` | Filtered list of models |
| `search(query, limit)` | Keyword/BM25 search (if `InvertedIndex` integration enabled) |
| `findSimilar(embedding, k, min_similarity)` | Nearest-neighbour search (linear or HNSW if configured) |
| `deployToEngine(model_id, engine)` | Register model with `ProcessGraphManager` |
| `exportBpmn(id)` | Export model as BPMN 2.0 XML string |
| `exportEpk(id)` | Export model as EPK text |
| `generateLlmDescriptor(id)` | Return `LlmProcessDescriptor` output JSON |

---

## ProcessLinker API

**Header:** `include/process/process_linker.h`

| Method | Description |
|---|---|
| `attachObject(instance_id, object_id, collection, link_type, node_id, metadata, attached_by)` | Attach a data object to an instance; returns `{bool, attachment_id}` |
| `detachObject(attachment_id)` | Soft-delete an attachment |
| `getAttachments(instance_id, filter_type)` | All attachments for an instance (optional type filter) |
| `getNodeAttachments(instance_id, node_id)` | Attachments scoped to a specific process node |
| `findInstancesWithObject(object_id, collection)` | Reverse lookup: which instances have this object? |
| `linkProcesses(source_id, target_id, link_type, properties)` | Create typed process-to-process link |
| `getLinks(process_id, filter_type)` | All outgoing links from a process entity |
| `registerRequiredDocument(model_id, node_id, doc_type, mandatory, schema)` | Register a required document type for a model node |
| `getRequiredDocuments(model_id, node_id)` | List required documents for a node |
| `getMissingDocuments(instance_id, node_id, model_id)` | Which mandatory documents are missing? |

### ProcessLinkType values

| Value | Meaning |
|---|---|
| `HAS_DOCUMENT` | Instance has an attached document |
| `HAS_METADATA` | Instance has structured metadata |
| `REQUIRES_DOCUMENT` | Model node requires a document |
| `IS_INSTANCE_OF` | Instance was created from this model |
| `SUB_PROCESS` | Instance is a sub-process of another |
| `CROSS_REFERENCE` | References another administrative case |
| `TRIGGERS` | Completion of this process triggers another |
| `EVIDENCE_FOR` | Document is evidence for a decision |

---

## ProcessGraphRag API

**Header:** `include/process/process_graph_rag.h`

| Method | Description |
|---|---|
| `retrieve(instance_id, query, config)` | Full Graph-RAG context for an instance + query |
| `retrieveForNode(instance_id, node_id, query, config)` | Node-scoped context variant |
| `buildKnowledgeGraph(model_id)` | Convert model to `{KGNode[], KGEdge[]}` |
| `buildInstanceKnowledgeGraph(instance_id, config)` | Instance + model + attachments as KG |
| `extractSubgraph(model_id, seeds, depth)` | BFS subgraph around seed nodes |
| `buildAdminProcessingPrompt(ctx)` | German administrative LLM prompt |
| `buildQueryPrompt(ctx)` | Query-specific prompt variant |
| `summarizeVerwaltungsvorgang(instance_id)` | Structured JSON summary for UI/API |
| `checkCompliance(instance_id)` | Compliance check (docs, SLA, state) |
| `findSimilarCases(instance_id, k, min_similarity)` | Similar historical cases |

## Public Header Entry Points

| Header | Role |
|---|---|
| `process_model_manager.h` | Core CRUD/import/export manager for process definitions |
| `process_linker.h` | Attachments, process-to-process links, required-document registry |
| `process_graph_rag.h` | Graph-RAG context assembly, compliance checks, similar case retrieval |
| `process_agentic_rag.h` | Iterative AgenticRAG loop over process contexts |
| `llm_process_descriptor.h` | LLM-oriented descriptor generation contracts |
| `bpmn_serializer.h` / `epk_serializer.h` / `epk_aris_xml_importer.h` / `vcc_vpb_importer.h` | Format import/export surfaces |
| `dmn_evaluator.h` / `cmmn_serializer.h` / `ocel_exporter.h` / `object_centric_tracer.h` | Decision, case modeling, and process-mining integrations |
| `process_model_generator.h` / `process_light_retriever.h` / `process_community_detector.h` | Advanced retrieval and generation components |

### ProcessRagConfig fields

| Field | Type | Default | Description |
|---|---|---|---|
| `max_subgraph_depth` | int | 3 | BFS hops from active nodes |
| `max_similar_cases` | int | 5 | Max similar cases included |
| `include_attachments` | bool | true | Include attached documents |
| `include_history` | bool | true | Include token traversal history |
| `include_missing_docs` | bool | true | Check missing required documents |
| `include_compliance` | bool | true | Include compliance tags |
| `similarity_threshold` | float | 0.7 | Minimum similarity for similar cases |
| `max_prompt_tokens` | size_t | 3000 | Approximate LLM token budget |
| `language` | string | "de" | "de" (German) or "en" (English) |

---

## AQL Integration Examples

### Query all active administrative process models

```aql
FOR model IN _process_definitions
  FILTER model.domain == "ADMINISTRATION"
  FILTER model.state == "ACTIVE"
  SORT model.name ASC
  RETURN { id: model.id, name: model.name, version: model.version,
           compliance_tags: model.compliance_tags }
```

### Find models by compliance tag

```aql
FOR model IN _process_definitions
  FILTER "DSGVO" IN model.compliance_tags
  RETURN model.id
```

### Find all running instances for a model

```aql
FOR inst IN _process_instances
  FILTER inst.process_definition_id == "bauantrag_standard"
  FILTER inst._state == "RUNNING"
  RETURN { id: inst._key, name: inst.name, started: inst._created_at }
```

### Find instances with a specific document attached

```aql
// Use ProcessLinker::findInstancesWithObject() via the HTTP API, or:
FOR inst IN _process_instances
  LET attachments = (
    FOR att IN _process_attachments
      FILTER att.instance_id == inst._key
      FILTER att.object_id == "doc-7"
      RETURN att
  )
  FILTER LENGTH(attachments) > 0
  RETURN inst._key
```

### Multi-model query: process + vector + geo

```aql
FOR task IN _process_tokens
  FOR node IN _process_nodes
    FILTER task.current_node == node.id
    FILTER node._type == "USER_TASK"
  LET applicant = DOCUMENT("citizens", task.variables.applicant_id)
  LET similar = SIMILARITY(task._embedding, applicant._embedding, 3)
  RETURN { task: task._key, node: node.name, applicant: applicant.name, similar }
```

---

## Key Prefixes (RocksDB)

| Prefix | Content |
|---|---|
| `proc:def:<id>` | Current revision of process model |
| `proc:def:<id>:rev:<n>` | Historical revision snapshot |
| `proc:attach:<inst_id>:<obj_id>` | Attachment descriptor |
| `proc:link:<src_id>:<tgt_id>:<type>` | Process-to-process link |
| `proc:req_doc:<model_id>:<node_id>:<doc_type>` | Required document entry |
| `proc:inst_emb:<instance_id>` | Instance embedding (float array JSON) |

## Runtime Behavior, Error Cases, and Limits

- **Storage contract:** keys use `proc:def:`, `proc:attach:`, `proc:link:`, `proc:req_doc:`, `proc:inst_emb:`.
- **BPMN/AML parser guards:** importer paths are guarded by bounded input handling (notably the 10 MiB XML guard for BPMN/ARIS flows).
- **Similarity behavior:** `findSimilarCases()` prefers stored embeddings (`proc:inst_emb:<id>`) and falls back to Jaccard-style heuristics when embeddings are unavailable.
- **Retrieval shape controls:** `ProcessRagConfig` limits depth, similar-case count, and prompt size budget (`max_prompt_tokens`).
- **Lifecycle behavior:** `remove(model_id)` archives model definitions (`ProcessModelState::ARCHIVED`) instead of deleting revision history.

## Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| `importBpmn` / `importArisXml` returns failure | Invalid XML or unsupported structure | Validate input BPMN/AML and retry with a reduced, schema-conform payload |
| `retrieve()` returns little context | Query too broad or depth too low | Increase `max_subgraph_depth`, `max_similar_cases`, or enable attachments/history in `ProcessRagConfig` |
| Similar-case output is weak | Missing instance embeddings | Ensure embedding generation pipeline populates `proc:inst_emb:<instance_id>` |
| Missing required documents are always reported | Required docs registered on a different `model_id`/`node_id` | Verify `registerRequiredDocument(...)` and attachment node/model IDs |

## Documentation Links

- Module implementation overview: [`../../src/process/README.md`](../../src/process/README.md)
- Module architecture: [`../../src/process/ARCHITECTURE.md`](../../src/process/ARCHITECTURE.md)
- Module roadmap: [`../../src/process/ROADMAP.md`](../../src/process/ROADMAP.md)
- Future enhancements: [`../../src/process/FUTURE_ENHANCEMENTS.md`](../../src/process/FUTURE_ENHANCEMENTS.md)
- Security notes: [`../../src/process/SECURITY.md`](../../src/process/SECURITY.md)
- Primary source index (DE): [`../../docs/de/process/PRIMARY_SOURCES.md`](../../docs/de/process/PRIMARY_SOURCES.md)
- Primary source index (EN): [`../../docs/en/process/PRIMARY_SOURCES.md`](../../docs/en/process/PRIMARY_SOURCES.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
