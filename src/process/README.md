> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release --target <target>`
> **Test:** `ctest --preset linux-release --output-on-failure -R process`

# ThemisDB Process Modeling Module

**Version:** 2.1.0
**Status:** 🟡 Beta — Core functionality implemented; ARIS-XML import and AgenticRAG operational as of v2.1.0
**Last Updated:** 2026-04-17
**Module Path:** `src/process/`

---

## Module Purpose

The Process Modeling module provides a dedicated, machine-readable management layer for
**BPMN 2.0**, **EPK** (Ereignisgesteuerte Prozesskette), and **VCC-VPB** process definitions
stored in ThemisDB.

It acts as a **layer over base-entities**: every process model is stored as a standard
ThemisDB document in the `_process_definitions` system collection, making it fully
queryable via AQL without any special language extensions.

### Core Capabilities

- **Multi-notation import/export** – BPMN 2.0 XML, EPK text/JSON, VCC-VPB YAML, **ARIS AML v9/v10 (EPK)**
- **LLM-optimised descriptors** – structured JSON + system-prompt generation for RAG
- **AgenticRAG** – iterative multi-hop Q&A over Verwaltungsvorgänge via `ProcessAgenticRag`
- **Base-entity layer** – processes stored as versioned documents with audit trail
- **Vector similarity search** – find semantically similar models via embeddings
- **Execution bridge** – deploy models to `ProcessGraphManager` for live execution
- **Administrative process library** – German public administration processes pre-modelled
- **Compliance tagging** – regulatory frameworks (DSGVO, GWB, BauO, ITIL, etc.)
- **DMN 1.5 decision tables** – FEEL-subset evaluator via `DmnEvaluator`
- **OCEL 2.0 export** – object-centric event log export for PM4Py/Celonis via `OcelExporter`
- **LLM-to-BPMN generator** – natural language to `ProcessModelRecord` via `ProcessModelGenerator`

## Subsystem Scope

**In Scope:**
- Process model definition management (CRUD + versioning)
- BPMN 2.0, EPK, VCC-VPB import and export
- LLM context descriptor generation
- Process model storage as ThemisDB base-entity documents
- Deployment bridge to ProcessGraphManager (execution engine)
- Administrative and business process model templates
- Compliance / regulatory tag management

**Out of Scope:**
- Process execution and token routing (handled by `include/index/process_graph.h`)
- Process mining and conformance checking analytics (handled by `src/analytics/process_mining.cpp`)
- BPMN HTTP API endpoints (handled by `src/server/bpmn_api_handler.cpp`)
- Raw AQL query execution (handled by `src/query/`)

## Relevant Interfaces

| File | Role |
|------|------|
| `process_model_manager.cpp` | High-level process model CRUD + import/export orchestration; `importArisXml()` added v2.1.0 |
| `bpmn_serializer.cpp` | BPMN 2.0 XML ↔ ProcessNodeInfo/EdgeInfo (state-machine tokenizer, no regex) |
| `epk_serializer.cpp` | EPK text/JSON ↔ ProcessNodeInfo/EdgeInfo conversion |
| `epk_aris_xml_importer.cpp` | EPK import from ARIS AML v9/v10 XML (`importAml`, `importAllAml`) |
| `process_agentic_rag.cpp` | `ProcessAgenticRag` — iterative multi-hop Q&A over process instances |
| `process_graph_rag.cpp` | `ProcessGraphRag` — Knowledge Graph build, BFS/PPR subgraph extraction, RAG context assembly |
| `process_linker.cpp` | `ProcessLinker` — attach/detach documents, process-to-process links, required-document registry |
| `process_model_generator.cpp` | `ProcessModelGenerator` — LLM-driven BPMN model generation from natural language |
| `dmn_evaluator.cpp` | `DmnEvaluator` — DMN 1.5 decision table evaluator with FEEL-subset support |
| `ocel_exporter.cpp` | `OcelExporter` — OCEL 2.0 object-centric event log export |
| `llm_process_descriptor.cpp` | LLM-optimised JSON descriptor + system-prompt-ready text; conformance-checking prompt builder |
| `vcc_vpb_importer.cpp` | VCC-VPB YAML → ProcessModelRecord conversion |

## Current Delivery Status

**Maturity:** 🟡 Beta — Core import/export, storage, and LLM descriptor generation are
operational. Advanced semantic search (requires pre-computed embeddings) and full
LLM conformance checking are planned for v1.1.0.

## About This Directory

This directory (`src/process/`) contains **implementation files only**. For API
documentation, see [`../../include/process/`](../../include/process/).

## Configuration Options

### `ProcessRagConfig` (`include/process/process_graph_rag.h`)

- Retrieval depth/shape: `max_subgraph_depth`, `max_similar_cases`
- Context toggles: `include_attachments`, `include_history`, `include_missing_docs`, `include_compliance`
- Ranking/prompt controls: `similarity_threshold`, `max_prompt_tokens`, `language`, `use_ppr`

### `ProcessAgenticConfig` (`include/process/process_agentic_rag.h`)

- Iteration controls: `max_iterations`, `max_total_documents`
- Quality gates: `quality_threshold`, `faithfulness_threshold`
- Embedded Graph-RAG defaults: `rag_config`

### Optional `ProcessModelManager` integrations (`include/process/process_model_manager.h`)

- `setEmbedder(...)`: automatic embedding generation during `save()`
- `setInvertedIndex(...)`: BM25-backed text search for `search(...)`
- `setVectorIndex(...)`: HNSW-backed nearest-neighbor for `findSimilar(...)`

## Implementation Files

### 1. ProcessModelManager (`process_model_manager.cpp`)

**Purpose:** The central orchestrator for all process model operations.

**Key responsibilities:**
- Import from BPMN XML, EPK text, VCC-VPB YAML, and ARIS AML XML (`importArisXml()`) via the respective serializers/importers
- Persist records to RocksDB under the `proc:def:<id>` key prefix
- Keep versioned snapshots under `proc:def:<id>:rev:<n>` for full audit trail
- Provide list, search, and vector-similarity query operations
- Bridge to `ProcessGraphManager` for deploying process models to the execution engine

**Storage contract:**
```
Key:   proc:def:<model_id>          → current revision (JSON)
Key:   proc:def:<model_id>:rev:<n>  → historical revision (JSON)
```

**Thread Safety:** All public methods are thread-safe for separate `ProcessModelManager`
instances backed by the same `RocksDBWrapper` (RocksDB is internally thread-safe).

### 2. BpmnSerializer (`bpmn_serializer.cpp`)

**Purpose:** BPMN 2.0 XML import and export.

**Implementation notes:**
- Uses a hand-written state-machine XML tokenizer (`tokenizeXml` / `parseAttrs` / `stripNs` / `unescapeXml`); no regex, no DOM/SAX library
- Handles `bpmn:` namespace prefixes, nested `subProcess`, `conditionExpression` child text, comments/PIs/CDATA; 10 MiB security guard

### 3. EpkSerializer (`epk_serializer.cpp`)

**Purpose:** EPK text notation and JSON import/export.

**Supported input formats:**
1. Simple line-based text (`TYPE: "name" [attr=value]` with `->` arrows)
2. JSON array of `{type, id, name, …}` node and edge objects

**EPK element support:** All 9 standard EPK types including organizational units,
information objects, and application system references.

### 4. LlmProcessDescriptor (`llm_process_descriptor.cpp`)

**Purpose:** Generate machine-readable, LLM-optimised representations of process models.

**Output includes:**
- Structured JSON with nodes, edges, SLA info, compliance tags
- Natural-language summary (DE/EN)
- `llm_context` field: condensed system-prompt-ready text block (< 2000 tokens)
- Conformance-checking prompt builder

**Usage in RAG:**
```cpp
auto record = manager.load("bauantrag_standard").value();
auto desc   = LlmProcessDescriptor::generate(record);
// Inject desc["llm_context"] into LLM system prompt
```

### 5. VccVpbImporter (`vcc_vpb_importer.cpp`)

**Purpose:** Import VCC-VPB YAML process definitions.

**Capabilities:**
- Single model import from YAML string
- Batch import from YAML files with list keys (e.g. `administrative_models:`)
- Directory scanner for bulk import of all `*.yaml` files

**Pre-loaded model library** (from `config/process_models/`):
- Administrative: Bauantrag, Beschaffung, Personal, Haushalt, Dokumentenfreigabe
- IT Service: Incident Management, Change Management, SDLC Scrum
- Healthcare: Patient Admission, Medication Management, Lab Testing
- Customer Service: Complaint Handling, Order Processing, Returns
- Finance: Invoice Processing, Budget Planning, Audit

## Example Usage

```cpp
// Initialize
RocksDBWrapper db(config);
ProcessModelManager manager(db);

// Import BPMN from file
std::ifstream f("bauantrag.bpmn");
std::string xml((std::istreambuf_iterator<char>(f)), {});
auto result = manager.importBpmn(xml, {.domain = ProcessDomain::ADMINISTRATION});
ASSERT_TRUE(result.ok);

// List all administrative models
auto models = manager.list(ProcessDomain::ADMINISTRATION, ProcessModelState::ACTIVE);

// Generate LLM descriptor
auto desc = manager.generateLlmDescriptor("bauantrag_standard");
std::string prompt = desc["llm_context"].get<std::string>();

// Deploy to execution engine
ProcessGraphManager engine(db);
manager.deployToEngine("bauantrag_standard", engine);
engine.startProcess("bauantrag_standard", {{"antragsteller", "Max Mustermann"}});
```

## AQL Integration

Because process models are stored as standard ThemisDB documents, they are queryable
via AQL without any special extensions:

```aql
-- List all active administrative process models
FOR m IN _process_definitions
  FILTER m.domain == "ADMINISTRATION"
  FILTER m.state  == "ACTIVE"
  SORT m.name ASC
  RETURN {
    id:          m.id,
    name:        m.name,
    compliance:  m.compliance_tags,
    node_count:  LENGTH(m.normalized.nodes),
    edge_count:  LENGTH(m.normalized.edges)
  }

-- Full-text search
FOR m IN _process_definitions
  FILTER CONTAINS(LOWER(m.name), "bauantrag")
  RETURN m
```

## Runtime Behavior, Error Cases, and Limits

- **Error signaling:** import/save/link operations report failure via result objects (`ProcessModelResult` or `{bool, message}` pairs).
- **Parser safety limits:** BPMN and ARIS XML imports use bounded parser paths including size guards (10 MiB guard documented for tokenizer-based XML imports).
- **Retrieval limits:** prompt assembly is constrained by `ProcessRagConfig::max_prompt_tokens`; subgraph scope is constrained by depth/PPR settings.
- **Fallback paths:** semantic similarity uses embeddings when available; fallback heuristics keep retrieval functional when embeddings are missing.
- **Storage lifecycle:** model deletes are archival (`ARCHIVED`) to preserve revision history and auditability.

## Troubleshooting

| Symptom | Likely cause | Recommended action |
|---|---|---|
| `importBpmn`/`importArisXml` fails immediately | malformed or oversized XML payload | validate XML and retry with standards-compliant, reduced payload |
| `findSimilar`/`findSimilarCases` returns weak matches | embedding pipeline not configured or empty vectors | wire `setEmbedder(...)` and validate `proc:inst_emb:` population |
| `retrieve()` misses relevant nodes | subgraph depth too low or restrictive thresholds | raise `max_subgraph_depth` / lower `similarity_threshold` / enable `use_ppr` |
| compliance warnings seem unexpected | required-document mappings incomplete | verify `registerRequiredDocument(...)` coverage and attachment metadata |

## Documentation Links

- Public API headers: [`../../include/process/README.md`](../../include/process/README.md)
- Architecture: [`./ARCHITECTURE.md`](./ARCHITECTURE.md)
- Roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Security notes: [`./SECURITY.md`](./SECURITY.md)
- Primary sources index (DE): [`../../docs/de/process/PRIMARY_SOURCES.md`](../../docs/de/process/PRIMARY_SOURCES.md)
- Primary sources index (EN): [`../../docs/en/process/PRIMARY_SOURCES.md`](../../docs/en/process/PRIMARY_SOURCES.md)

## Wissenschaftliche Grundlagen

The following peer-reviewed sources and standards form the scientific foundation of the Process module.

### Workflow and Process Modelling

1. **van der Aalst, W. M. P. (2016).**
   *Process Mining: Data Science in Action (2nd ed.).*
   Springer. ISBN: 978-3-662-49851-4.
   DOI: [10.1007/978-3-662-49851-4](https://doi.org/10.1007/978-3-662-49851-4)
   > Foundational reference for process discovery, conformance checking and enhancement.
   > Directly motivates the `ProcessGraphManager` conformance pipeline and the BPMN/PNML
   > import/export layer in `bpmn_adapter.cpp` and `pnml_adapter.cpp`.

2. **Object Management Group. (2013).**
   *Business Process Model and Notation (BPMN) 2.0.2.*
   OMG Document Number: formal/2013-12-09.
   URL: https://www.omg.org/spec/BPMN/2.0.2
   > Normative specification for the BPMN 2.0 interchange format.
   > Governs the `BPMNExporter` and `BPMNImporter` round-trip fidelity contract and the
   > compliance_tags vocabulary stored in `_process_definitions`.

3. **ISO/IEC. (2013).**
   *Information Technology – Process Assessment – Part 2: Performing an Assessment.*
   ISO/IEC 33002:2015.
   URL: https://www.iso.org/standard/54449.html
   > ISO 19510:2013 BPMN standard underpinning the XML serialisation schema validated by
   > `ProcessSchemaValidator` at import time.

### LLM-Augmented Process Automation

4. **Busch, M., Brunk, J., & Becker, J. (2023).**
   *ProcessGPT: Transforming Business Process Management with Generative Artificial Intelligence.*
   arXiv preprint arXiv:2304.07510.
   URL: https://arxiv.org/abs/2304.07510
   > Proposes using GPT-class models to draft, analyse and refactor process models from
   > natural language descriptions. Target integration for the planned `NLProcessDrafter`
   > in `src/process/` (Q2 2026 roadmap item).

5. **Bukhsh, Z. A., Saeed, A., & Dijkman, R. M. (2021).**
   *ProcessTransformer: Predictive Business Process Monitoring with Transformer Networks.*
   arXiv preprint arXiv:2104.00721.
   URL: https://arxiv.org/abs/2104.00721
   > Introduces a Transformer-based next-activity predictor for process event logs.
   > Planned basis for the conformance-prediction extension of `ProcessGraphManager`
   > (Q1 2027 roadmap item).

### Public Administration Standards

6. **FITKO / Föderale IT-Kooperation. (2024).**
   *Föderales Informationsmanagement (FIM) – Technischer Leitfaden.*
   URL: https://www.fitko.de/fim
   > Defines the canonical FIM data model for German administrative processes (Leistungs-,
   > Formular- und Prozessbaukasten). Governs the `FIMProcessAdapter` schema mapping in
   > `fim_adapter.cpp` and the `compliance_tags` vocabulary (`FIM`, `OZG`).

7. **Bundesministerium des Innern (BMI). (2017).**
   *XÖV-Rahmenwerk: Rahmenbedingungen für die Entwicklung von XÖV-Standards.*
   URL: https://www.xoev.de
   > XÖV exchange standard family used by `XoevProcessSerializer` for interoperability
   > with German public-sector IT systems (Bund, Länder, Kommunen).

## Scientific References

1. van der Aalst, W. M. P. (2016). **Process Mining: Data Science in Action (2nd ed.)**. Springer. https://doi.org/10.1007/978-3-662-49851-4

2. Object Management Group. (2013). **Business Process Model and Notation (BPMN) 2.0.2**. OMG formal/2013-12-09. https://www.omg.org/spec/BPMN/2.0.2

3. Busch, M., Brunk, J., & Becker, J. (2023). **ProcessGPT: Transforming Business Process Management with Generative Artificial Intelligence**. arXiv:2304.07510. https://arxiv.org/abs/2304.07510

4. Bukhsh, Z. A., Saeed, A., & Dijkman, R. M. (2021). **ProcessTransformer: Predictive Business Process Monitoring with Transformer Networks**. arXiv:2104.00721. https://arxiv.org/abs/2104.00721

5. FITKO / Föderale IT-Kooperation. (2024). **Föderales Informationsmanagement (FIM) – Technischer Leitfaden**. https://www.fitko.de/fim

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
