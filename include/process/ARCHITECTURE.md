> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/process/ARCHITECTURE.md -->

# Process Mining Module — Public Header Architecture

**Module Path:** `include/process/`  
**Implementation:** `../../src/process/`  
**Canonical architecture doc:** [`../../src/process/ARCHITECTURE.md`](../../src/process/ARCHITECTURE.md)

---

## 1. Overview

`include/process/` defines the **public BPM/BPMN/CMMN/DMN process modelling, EPK, XPDL, OCEL, object-centric tracing, LLM process descriptors, and RAG integration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/process/ARCHITECTURE.md`](../../src/process/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Process Model Serialisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `bpmn_serializer.h` | `BPMNSerializer` | BPMN 2.0 import/export |
| `cmmn_serializer.h` | `CMMNSerializer` | CMMN case model import/export |
| `epk_serializer.h` | `EPKSerializer` | Event-Process Chain serialisation |
| `epk_aris_xml_importer.h` | `EPKARISXMLImporter` | ARIS XML EPK importer |
| `dmn_evaluator.h` | `DMNEvaluator` | DMN decision table evaluation |
| `vcc_vpb_importer.h` | `VCCVPBImporter` | VCC VPB process model importer |
| `xpdl_importer.h` | `XPDLImporter` | XPDL workflow definition importer |
| `fim_importer.h` | `FIMImporter` | FIM process model importer |
### 2.2 Process Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `process_model_manager.h` | `ProcessModelManager` | Process model lifecycle management |
| `process_model_generator.h` | `ProcessModelGenerator` | LLM-assisted process model generation |
| `process_linker.h` | `ProcessLinker` | Cross-model process linking and tracing |
| `process_common.h` | `ProcessCommon` | Shared process domain types |
### 2.3 Process Mining and Analytics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `object_centric_tracer.h` | `ObjectCentricTracer` | Object-centric event log tracing |
| `ocel_exporter.h` | `OCELExporter` | OCEL 2.0 event log export |
| `process_community_detector.h` | `ProcessCommunityDetector` | Community detection in process graphs |
### 2.4 RAG and LLM Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `process_agentic_rag.h` | `ProcessAgenticRAG` | Agentic RAG over process models |
| `process_graph_rag.h` | `ProcessGraphRAG` | Graph-RAG over process data |
| `process_light_retriever.h` | `ProcessLightRetriever` | Lightweight process knowledge retrieval |
| `llm_process_adapter.h` | `LLMProcessAdapter` | LLM adapter for process queries |
| `llm_process_descriptor.h` | `LLMProcessDescriptor` | LLM-generated process descriptions |

---

## 3. Namespace Layout

All public types reside in the `themis::process` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/process/` expose the **stable public API**; internal types live in `src/process/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph/LLM**.
