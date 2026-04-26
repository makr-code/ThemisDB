> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Process Module – Architecture

**Module Path:** `src/process/`
**Version:** 1.0.0
**Last Updated:** 2026-04-06

---

## 1. Overview

The `src/process/` module is the dedicated **process modelling layer** of ThemisDB. It stores, manages, and queries process models expressed in BPMN 2.0, EPK (Ereignisgesteuerte Prozesskette), and VCC-VPB YAML format. Its primary use-case is **Graph-RAG for German administrative proceedings (Verwaltungsvorgänge)**: process models and running instances are exposed as a knowledge graph that large language models can traverse to answer questions about the current state of a case, which documents are still missing, and whether the proceeding is on schedule.

Key capabilities:

| Capability | Component |
|---|---|
| Import / export BPMN 2.0 XML | `BpmnSerializer` (state-machine tokenizer, no regex) |
| Import / export EPK text & JSON | `EpkSerializer` |
| Import EPK from ARIS AML v9/v10 XML | `EpkArisXmlImporter` |
| Import VCC-VPB YAML (single, batch, directory) | `VccVpbImporter` |
| CRUD + versioning for process models | `ProcessModelManager` (incl. `importArisXml()`) |
| LLM descriptor + system-prompt generation | `LlmProcessDescriptor` |
| Attach documents/metadata to instances | `ProcessLinker` |
| Graph-RAG context assembly for LLM | `ProcessGraphRag` |
| Iterative multi-hop agentic Q&A | `ProcessAgenticRag` |
| LLM-driven BPMN model generation | `ProcessModelGenerator` |
| DMN 1.5 decision table evaluation | `DmnEvaluator` |
| OCEL 2.0 event log export | `OcelExporter` |
| Execution engine (tokens, state) | `ProcessGraphManager` (from `index/`) |

---

## 2. Design Principles

### Layer over base-entities
Process models are stored as ordinary ThemisDB base-entity documents in the `_process_definitions` system collection, with the key prefix `proc:def:<id>`. This means they are queryable via AQL without additional joins and benefit from all standard ThemisDB indices (full-text, vector, geo, temporal).

### Graph-RAG first
The process graph *is* the knowledge graph for the LLM. Every node in a BPMN/EPK model becomes a `KGNode`; every sequence flow becomes a `KGEdge`. The `ProcessGraphRag` component converts this into a `KnowledgeGraph` (from `rag/knowledge_graph_retriever.h`) on demand, then extracts a query-relevant subgraph and assembles a structured LLM prompt in German or English.

### Multi-notation (BPMN, EPK, VCC-VPB)
All three notations are first-class citizens. Internally, every model is normalised to the same JSON representation (`ProcessModelRecord::normalized`) regardless of source format. Serialisers/importers handle the conversion.

### Administrative process focus
The module is explicitly designed for German public administration:
- Compliance tags reference German law (DSGVO, GWB, BauO, etc.)
- Default language for LLM prompts is German (`language="de"`)
- SLA monitoring is built into `ProcessGraphRag::summarizeVerwaltungsvorgang()`
- Required-document enforcement uses `ProcessLinker::getMissingDocuments()`

---

## 3. Component Architecture

```
VCC-VPB YAML / BPMN XML / EPK Text
        │
        ▼
┌─────────────────────────┐
│   ProcessModelManager   │  ◄── import / export / CRUD / versioning
│   (process_model_       │      stores to proc:def: key prefix
│    manager.cpp)         │      exposes AQL-queryable collection
└──────────┬──────────────┘
           │ deploy()
           ▼
┌─────────────────────────┐
│  ProcessGraphManager    │  ◄── execution engine (from index/ module)
│  (index/process_graph.h)│      manages instances, tokens, history
└──────────┬──────────────┘
           │ getProcessInstance() / findActiveTasks()
           ▼
┌─────────────────────────┐
│    ProcessLinker        │  ◄── attach documents / metadata to instances
│  (process_linker.cpp)   │      process-to-process links
│                         │      required-document registry
└──────────┬──────────────┘
           │ getAttachments() / getMissingDocuments()
           ▼
┌─────────────────────────┐
│   ProcessGraphRag       │  ◄── build KnowledgeGraph from model
│  (process_graph_rag.cpp)│      extract subgraph + assemble context
│                         │      German/English LLM prompt builder
└──────────┬──────────────┘
           │ ProcessRagContext (nodes, edges, attachments, missing docs)
           ▼
┌─────────────────────────┐
│  KnowledgeGraphRetriever│  ◄── graph traversal + entity linking
│  (rag/knowledge_graph_  │      score fusion (vector + graph signal)
│   retriever.h)          │
└─────────────────────────┘
           │
           ▼
      LLM endpoint
```

---

## 4. Data Model

### System collections

| Collection | Description |
|---|---|
| `_process_definitions` | Process model records (base-entity documents) |
| `_process_instances` | Running / completed process instances |
| `_process_tokens` | Execution tokens (current node, state, variables) |
| `_process_history` | Audit log of all node transitions |

### Document schemas

**`_process_definitions` document (abbreviated)**
```json
{
  "_id": "proc:def:bauantrag_standard",
  "id": "bauantrag_standard",
  "name": "Bauantragsverfahren Standard",
  "version": "1.0.0",
  "revision": 3,
  "notation": "BPMN_2_0",
  "domain": "ADMINISTRATION",
  "state": "ACTIVE",
  "compliance_tags": ["§34 BauO", "DSGVO"],
  "normalized": {
    "nodes": [ { "id": "start", "name": "Antrag einreichen", ... } ],
    "edges": [ { "from_node": "start", "to_node": "pruefung", ... } ],
    "metadata": { "sla_ms": 2592000000 }
  },
  "embedding": [0.12, 0.34, ...]
}
```

**`proc:attach:<instance_id>:<object_id>` document**
```json
{
  "id": "attach:inst-42:doc-7",
  "instance_id": "inst-42",
  "object_id": "doc-7",
  "object_collection": "documents",
  "link_type": "HAS_DOCUMENT",
  "node_id": "pruefung",
  "attached_by": "sachbearbeiter@bürgeramt.de",
  "attached_at_ms": 1741737600000,
  "metadata": { "doc_type": "Bauzeichnung", "pages": 4 }
}
```

**`proc:link:<source_id>:<target_id>:<link_type>` document**
```json
{
  "link_id": "link:inst-42:inst-99:SUB_PROCESS",
  "source_id": "inst-42",
  "target_id": "inst-99",
  "link_type": "SUB_PROCESS",
  "properties": { "spawned_by_node": "teilgenehmigung" },
  "created_at_ms": 1741737600000
}
```

**`proc:req_doc:<model_id>:<node_id>:<doc_type>` document**
```json
{
  "model_id": "bauantrag_standard",
  "node_id": "pruefung",
  "doc_type": "Bauzeichnung",
  "mandatory": true,
  "schema": { "type": "object", "required": ["format"] }
}
```

### Key prefix scheme

| Prefix | Content |
|---|---|
| `proc:def:<id>` | Process model record (current revision) |
| `proc:def:<id>:rev:<n>` | Versioned revision snapshot |
| `proc:attach:<instance_id>:<object_id>` | Attachment descriptor |
| `proc:link:<source_id>:<target_id>:<link_type>` | Process-to-process link |
| `proc:req_doc:<model_id>:<node_id>:<doc_type>` | Required document entry |
| `proc:inst_emb:<instance_id>` | Instance embedding (float array JSON) |
| `proc:inst:<instance_id>` | Serialised instance document (fallback) |

---

## 5. Indexing Strategy

### Full-text index
Model names, descriptions, compliance tags, and node descriptions are tokenised and stored in the ThemisDB inverted index. `ProcessModelManager::search()` performs TF-IDF ranked retrieval across all process models.

### Vector / HNSW index
`ProcessModelRecord::embedding` stores a pre-computed text embedding of the `long_description`. `ProcessModelManager::findSimilar()` performs approximate nearest-neighbour search using the HNSW index.  Instance embeddings are stored separately under `proc:inst_emb:<id>` and used by `ProcessGraphRag::findSimilarCases()` for cosine similarity retrieval.

### Graph index
The process model normalised graph (`ProcessModelRecord::normalized`) is loaded into the `KnowledgeGraph` class on demand for BFS subgraph extraction.

### Secondary / compound index
Models are filterable by `domain`, `state`, and `notation` through `ProcessModelManager::list()`, which scans the `proc:def:` prefix and filters in memory.

---

## 6. Graph-RAG Pattern for Verwaltungsvorgänge

### Query flow

```
User query: "Was fehlt noch für den Bauantrag inst-42?"
                  │
  ProcessGraphRag::retrieve("inst-42", query)
                  │
    1. engine_.getProcessInstance("inst-42")
       → {state: RUNNING, tokens: [{current_node: "pruefung"}]}
                  │
    2. extractSubgraph("bauantrag_standard", ["pruefung"], depth=3)
       → {nodes: [...], edges: [...]}
                  │
    3. linker_.getAttachments("inst-42")
       → [{doc-7, HAS_DOCUMENT, ...}]
                  │
    4. linker_.getMissingDocuments("inst-42", "pruefung", "bauantrag_standard")
       → ["Bauzeichnung", "Lageplan"]
                  │
    5. findSimilarCases("inst-42", k=5)
       → [{inst-38, similarity=0.91, COMPLETED}, ...]
                  │
    6. assemblePrompt_(ctx, config)
       → German LLM prompt
                  │
  ProcessRagContext { llm_prompt, subgraph, attachments, missing_documents, ... }
```

### Process graph as knowledge graph
`ProcessGraphRag::buildKnowledgeGraph()` converts:
- Each process node → `KGNode` (type: `CONCEPT`, properties: description, node_type)
- Each sequence flow → `KGEdge` (relation: `CAUSES`, weight from edge metadata)
- Conditional flows → `KGEdge` (relation: `CAUSES`)
- Associations → `KGEdge` (relation: `RELATED_TO`)

### Attachment enrichment
`ProcessGraphRag::buildInstanceKnowledgeGraph()` adds:
- Instance-state node → `KGNode` (type: `EVENT`)
- Active token nodes → `KGNode` + edges to their current process node
- Attachment nodes → `KGNode` (type: `PRODUCT`) + edges from instance node

### Similar case retrieval
1. If `proc:inst_emb:<id>` keys exist: cosine similarity over embeddings.
2. Fallback: Jaccard similarity over variable key sets for instances with the same `process_definition_id`.

---

## 7. Process Linking Model

### ProcessLinkType semantics

| Type | Source → Target | Semantics |
|---|---|---|
| `HAS_DOCUMENT` | instance → document | Instance owns an attached document |
| `HAS_METADATA` | instance → metadata | Instance has structured metadata |
| `REQUIRES_DOCUMENT` | model node → doc type | Model prescribes a required document |
| `IS_INSTANCE_OF` | instance → model | Instance was created from this model |
| `SUB_PROCESS` | parent instance → child | Child is a sub-process of parent |
| `CROSS_REFERENCE` | instance A → instance B | A references another case |
| `TRIGGERS` | instance A → instance B | Completion of A starts B |
| `EVIDENCE_FOR` | document → instance | Document is evidence for a decision |

### Document attachment to nodes
Attachments carry an optional `node_id` field so that a document can be scoped to a specific process node (e.g., "Bauzeichnung required at the Prüfung node"). `ProcessLinker::getNodeAttachments()` filters by this field; `getMissingDocuments()` cross-references the required-document registry for that node against actual node-scoped attachments.

### Required document enforcement
1. `registerRequiredDocument(model_id, node_id, doc_type, mandatory=true)` stores the requirement under `proc:req_doc:`.
2. When an instance reaches a node, `getMissingDocuments(instance_id, node_id, model_id)` returns the list of missing mandatory document types.
3. `summarizeVerwaltungsvorgang()` and `checkCompliance()` call this automatically.

---

## 8. Abstraction and Template Model

### Model → Instance relationship
A process model (`ProcessModelRecord`) is the template. `ProcessModelManager::deployToEngine()` registers the model with `ProcessGraphManager`, which can then start instances via `startProcess()`. Instances reference their origin model via `process_definition_id`.

### Sub-process linking
When a `CALL_ACTIVITY` node spawns a child instance, `ProcessLinker::linkProcesses(parent_id, child_id, SUB_PROCESS)` records the parent–child relationship. `getLinks(parent_id, SUB_PROCESS)` retrieves all child instances.

### Parameterisation via variables
Instance variables (`ProcessInstance::variables`) carry parameters specific to this run (e.g., applicant ID, property address). The LLM prompt includes variables so the model can give personalised answers.

---

## 9. Administrative Process Configuration

### Pre-loaded VCC-VPB models
The `config/process_models/` directory contains 17 pre-loaded VCC-VPB YAML models across 5 domains:

| Domain | Examples |
|---|---|
| Verwaltung | Bauantrag Standard, Gewerbeummeldung, Führungszeugnis |
| IT | Incident Management, Change Request |
| Gesundheit | Krankenhausaufnahme, Pflegebedürftigkeitsprüfung |
| Finanzen | Haushaltsmittelanforderung, Reisekostenabrechnung |
| Kundenservice | Beschwerdeverfahren, Rückgabe |

Models are imported with `VccVpbImporter::importDirectory("config/process_models/")` during server startup.

### Compliance framework integration
`ProcessModelRecord::compliance_tags` lists applicable regulations (e.g., `["DSGVO Art. 5", "§34 BauO NRW", "GWB §97"]`). `ProcessGraphRag::checkCompliance()` includes these in the compliance check, and the LLM prompt lists them explicitly so the model can reference them.

### SLA monitoring
The normalised process model may carry an `sla_ms` value in `normalized.metadata`. `summarizeVerwaltungsvorgang()` compares elapsed time since `started_at_ms` against this value and reports `"on_time"`, `"at_risk"` (>80% consumed), or `"overdue"`. `checkCompliance()` raises a violation when SLA is exceeded.

---

## 10. Integration Points

| Module | Integration | File |
|---|---|---|
| Execution engine | `ProcessGraphManager::getProcessInstance()`, `findActiveTasks()` | `include/index/process_graph.h` |
| Process mining | Token replay and conformance checking | `src/analytics/process_mining.cpp` |
| Graph-RAG retrieval | `KnowledgeGraph`, `KGNode`, `KGEdge` | `include/rag/knowledge_graph_retriever.h` |
| HTTP API | BPMN import, instance management | `src/server/bpmn_api_handler.cpp` |
| VCC-VPB model library | Pre-loaded models on startup | `config/process_models/` |
| Storage | `RocksDBWrapper::scanPrefix()`, `get()`, `put()` | `include/storage/rocksdb_wrapper.h` |
| LLM descriptors | `LlmProcessDescriptor::generateSystemPrompt()` | `src/process/llm_process_descriptor.cpp` |

---

## 11. Stand der Wissenschaft und Technik

Diese Architektur berücksichtigt und integriert den aktuellen Stand der Forschung.
Eine vollständige Literaturanalyse mit konkreten Implementierungsableitungen findet sich in:

> **[`docs/de/process/STATE_OF_THE_ART.md`](../../docs/de/process/STATE_OF_THE_ART.md)**

Schlüsselreferenzen, die die aktuelle Architektur begründen:

| Forschungsbereich | Quelle | ThemisDB-Implementierung |
|-------------------|--------|--------------------------|
| Graph-RAG | Edge et al. (2024) – GraphRAG | `ProcessGraphRag::buildKnowledgeGraph()` |
| Graph-RAG Scoring | Gutierrez et al. (2024) – HippoRAG | BFS-Subgraph → PPR (geplant Q2 2026) |
| Duales Retrieval | Guo et al. (2024) – LightRAG | `ProcessRagConfig::mode` (geplant Q3 2026) |
| Process Mining LLM | Busch et al. (2023) – ProcessGPT | `LlmProcessDescriptor::buildConformancePrompt()` |
| OCPM | van der Aalst (2022) | `ObjectCentricTracer` (geplant Q3 2026) |
| Event Log Standard | Berti et al. (2023) – OCEL 2.0 | `OcelExporter` (geplant Q2 2026) |
| Verwaltungsdigitalisierung | FITKO FIM (2024) | `FimImporter` (geplant Q4 2026) |
| Case Management | OMG CMMN 1.1 (2016) | `CmmnSerializer` (geplant Q4 2026) |
| Decision Tables | OMG DMN 1.5 (2023) | `DmnEvaluator` (geplant Q3 2026) |
| Predictive Monitoring | Bukhsh et al. (2021) – ProcessTransformer | `ProcessPredictor` (geplant Q1 2027) |
