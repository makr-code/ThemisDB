# Process Modeling Module – Future Enhancements

**Version:** 1.0.0
**Status:** 📋 Planned
**Last Updated:** 2026-03-12
**Module Path:** `src/process/`

---

## Scope

- Graph-RAG retrieval for BPMN 2.0, EPK, and VCC-VPB process models
- Attachment of documents/metadata to process instances with required-document enforcement
- German administrative proceedings (Verwaltungsvorgänge) as primary use-case
- LLM-ready context assembly with German/English prompt generation
- Compliance checking against DSGVO, GWB, BauO, and other German regulations
- SLA monitoring and alert dispatch for active instances
- Multi-notation process import/export: BPMN XML, EPK text/JSON, VCC-VPB YAML

---

## Design Constraints

- `[ ]` BPMN 2.0 XML export must be ISO/IEC 19510 compliant; the export serializer must produce valid BPMN 2.0 that can be imported by BPMN-compliant modelling tools (Camunda, Signavio).
- `[ ]` Process model embedding dimensions must match the configured LLM embedding endpoint (default: 1536 for OpenAI `text-embedding-3-small`); mismatched dimensions must raise a structured error at import time, not silently truncate.
- `[ ]` `ProcessLinker::getMissingDocuments()` must complete in < 10 ms for instances with up to 1,000 attached documents.
- `[ ]` `ProcessGraphRag::retrieve()` total latency must not exceed 200 ms (excluding LLM call) for models with up to 500 nodes.
- `[ ]` The LLM prompt produced by `buildAdminProcessingPrompt()` must not exceed the configured `max_prompt_tokens` budget; content must be trimmed gracefully (subgraph nodes dropped first, then similar cases, then history).
- `[ ]` All YAML, XML, and JSON inputs must be validated before processing; malformed inputs must produce structured errors, not uncaught exceptions.
- `[ ]` No dynamic memory allocation inside hot scan loops; reuse pre-allocated result vectors.
- `[ ]` Thread safety: `ProcessLinker` and `ProcessGraphRag` must be safe for concurrent read access; writes are serialised by `RocksDBWrapper`'s internal locking.

---

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ProcessLinker::attachObject(instance_id, object_id, collection, link_type, node_id, metadata, attached_by)` | Server API / workflow engine | Returns `{bool, attachment_id}` |
| `ProcessLinker::getMissingDocuments(instance_id, node_id, model_id)` | `ProcessGraphRag`, compliance checker | Returns list of missing doc_type strings |
| `ProcessGraphRag::retrieve(instance_id, query, config)` | LLM agent, case management UI | Returns `ProcessRagContext` with `llm_prompt` |
| `ProcessGraphRag::summarizeVerwaltungsvorgang(instance_id)` | Case management UI, REST API | Returns structured JSON summary |
| `ProcessGraphRag::checkCompliance(instance_id)` | Compliance dashboard, CEP engine | Returns `ComplianceCheckResult` |
| `ProcessGraphRag::buildKnowledgeGraph(model_id)` | `KnowledgeGraphRetriever` | Returns `ProcessKnowledgeGraph {nodes, edges}` |
| `ProcessModelManager::findSimilar(embedding, k)` | `ProcessGraphRag::findSimilarCases()` | HNSW nearest-neighbour over model embeddings |
| `LLM embedding endpoint` | `ProcessModelManager::save()` (planned) | REST call to configured embedding API |

---

## Planned Features

### 1. AgenticRAG Integration for Multi-Turn Process Q&A

**Priority:** High  
**Target:** Q3 2026

**Scope:**  
Enable iterative, multi-turn LLM conversations about a Verwaltungsvorgang. The LLM agent can call `ProcessLinker` and `ProcessGraphRag` methods as tools during the conversation loop.

**Current State:**  
`ProcessGraphRag::retrieve()` produces a single-shot context. Multi-turn dialogue is not supported.

**Design Constraints:**  
- Maximum 5 LLM turns per question to bound cost and latency.
- Each tool call result must be < 2,000 tokens to fit within the remaining prompt budget.
- Agent state (conversation history, tool results) must be serialisable for resumption after server restart.

**Required Interfaces:**
```cpp
// In rag/agentic_rag.h (planned)
struct AgentTool {
    std::string name;
    std::string description;
    std::function<json(json)> handler;
};

class AgenticRag {
public:
    void registerTool(AgentTool tool);
    AgentResponse run(std::string_view query, std::string_view context,
                      int max_turns = 5);
};
```

**Implementation Notes:**  
- Register `getMissingDocuments`, `getLinks`, `findSimilarCases`, `summarizeVerwaltungsvorgang` as `AgentTool` instances.
- The initial context is `ProcessGraphRag::retrieve()` output serialised to JSON.
- The agent loop: LLM response → parse tool calls → execute → append results → next turn.
- Terminate on: final answer detected, max_turns reached, or error.

**Test Strategy:**  
- Unit: mock LLM + known tool responses; verify turn count and tool invocation order.
- Integration: real llama.cpp backend; 10 representative Verwaltungsvorgang scenarios; verify correct missing-document identification.

**Performance Targets:**  
- Total latency ≤ 5 s for 3-turn conversation on local llama.cpp (LLaMA-3 8B Q4).
- Tool call overhead ≤ 10 ms per call.

**Security / Reliability:**  
- Tool arguments must be validated (no path traversal, no injection).
- LLM output must be sanitised before using as RocksDB keys or query parameters.

---

### 2. BPMN-S Security Profile for DSGVO Compliance

**Priority:** Medium  
**Target:** Q4 2026

**Scope:**  
Support annotating BPMN nodes with DSGVO (General Data Protection Regulation) data-handling requirements using the BPMN-S security profile extension.

**Current State:**  
Compliance tags are stored as plain strings in `ProcessModelRecord::compliance_tags`. No structured DSGVO annotation per node.

**Design Constraints:**  
- Must not break existing BPMN 2.0 import/export.
- BPMN-S annotations stored as extension elements in the XML namespace `http://bpmn-s.org/schema`.
- Retain backward compatibility: BPMN files without BPMN-S annotations import normally.

**Required Interfaces:**
```cpp
// New fields on ProcessNodeInfo (planned)
struct DsgvoAnnotation {
    std::string data_category;      // "personal", "sensitive", "anonymised"
    std::string legal_basis;        // "Art. 6(1)(e) DSGVO", etc.
    std::optional<int> retention_days;
    bool requires_consent{false};
};

// ProcessNodeInfo extended field
std::optional<DsgvoAnnotation> dsgvo_annotation;
```

**Implementation Notes:**  
- `BpmnSerializer::importBpmn()` parses `<extensionElements>` for `<bpmns:SecurityAnnotation>` tags.
- `ProcessGraphRag::checkCompliance()` extended: flag nodes with personal data but no legal basis.
- LLM prompt includes DSGVO annotations per active node.

**Test Strategy:**  
- Import a BPMN-S annotated model; verify annotations stored correctly.
- `checkCompliance()` flags missing legal basis; produces violation string referencing specific node.

---

### 3. Real-Time SLA Monitoring via CEP Engine

**Priority:** Medium  
**Target:** Q4 2026

**Scope:**  
Register a CEP rule for each active process instance so that alerts fire automatically when the SLA is at risk or exceeded, without requiring polling.

**Current State:**  
SLA status is computed on-demand in `summarizeVerwaltungsvorgang()` and `checkCompliance()`. No proactive alerting.

**Design Constraints:**  
- CEP rule registration must complete in < 5 ms per instance.
- Alert dispatch failure must not affect process execution.
- Rules must be deregistered on instance completion to avoid rule-set growth.

**Required Interfaces:**
```cpp
// Integration with analytics/cep_engine.h (existing)
// New registration helper in process_graph_rag.cpp (planned):
void registerSlaRule(std::string_view instance_id, int64_t sla_ms,
                     CepEngine& cep);
void deregisterSlaRule(std::string_view instance_id, CepEngine& cep);
```

**Implementation Notes:**  
- On `ProcessGraphManager::startProcess()`, call `registerSlaRule()` if the model has `sla_ms`.
- CEP rule: `SELECT FIRST(instance_id) FROM _process_instances WHERE elapsed_ms > sla_ms * 0.8 WITHIN 60s ACTION alert(...)`.
- On `completeTask()` / `terminateProcess()`, call `deregisterSlaRule()`.
- Alert payload: `{ instance_id, process_name, sla_ms, elapsed_ms, assigned_to }`.

**Test Strategy:**  
- Synthetic test: start instance with SLA = 200 ms; verify CEP alert fires within 300 ms.
- No false positives for instances completed before SLA threshold.

**Performance Targets:**  
- CEP alert latency ≤ 100 ms after threshold crossing.
- Rule registration overhead ≤ 5 ms per instance.

**Security / Reliability:**  
- Alert dispatch uses exponential back-off (max 3 retries, delay 1 s / 2 s / 4 s).
- Failure to dispatch alert is logged at WARN level but does not fail the process operation.

---

### 4. Cross-Case Process Analytics: Bottleneck Detection

**Priority:** Medium  
**Target:** Q4 2026

**Scope:**  
Aggregate token dwell-time across all completed instances to identify systematic bottlenecks in administrative proceedings.

**Current State:**  
`ProcessGraphRag::findSimilarCases()` retrieves individual similar cases. No aggregate cross-case analysis.

**Design Constraints:**  
- Analysis must operate on completed instances only; running instances are excluded.
- Dwell-time aggregation must be incremental (new completions update the aggregate, not recompute from scratch).
- Results must be queryable via AQL `PROCESS_BOTTLENECKS(model_id, top_n)`.

**Implementation Notes:**
```cpp
// Integration with analytics/process_mining.cpp (existing)
struct NodeDwellStats {
    std::string node_id;
    std::string node_name;
    double avg_dwell_ms;
    double p95_dwell_ms;
    size_t sample_count;
};

// New method on ProcessGraphRag (planned):
std::vector<NodeDwellStats> analyzeBottlenecks(
    std::string_view model_id,
    int top_n = 5
) const;
```

**Test Strategy:**  
- Synthetic dataset: 1,000 completed instances; inject artificial delay at nodes 3 and 7.
- Bottleneck detection accuracy ≥ 90 % (nodes 3 and 7 appear in top-5).

**Performance Targets:**  
- Analysis ≤ 2 s for 10,000 completed instances.
- Incremental update ≤ 50 ms per new completion.

---

### 5. EPK ARIS-XML Import

**Priority:** Low  
**Target:** Q3 2026

**Scope:**  
Import Event-driven Process Chain (EPK) models exported from the ARIS toolset in ARIS-XML format.

**Current State:**  
`EpkSerializer` supports EPK text notation and EPK JSON. ARIS-XML (`.epk` binary or XML export) is not supported.

**Design Constraints:**  
- No external XML parsing library; hand-written parser or regex-based (consistent with existing code).
- Unsupported ARIS node types → log at WARN level and skip.
- Malformed XML → return structured error, not exception.
- Must not break existing EPK text/JSON import.

**Required Interfaces:**  
New file `src/process/aris_xml_parser.cpp`:
```cpp
namespace themis::process {
// Returns a ProcessModelRecord with notation=EPK
ProcessModelRecord importArisXml(std::string_view xml_content);
}
```

**Implementation Notes:**  
- Map ARIS `<ObjDef Class="EVT">` → EPK event node.
- Map ARIS `<ObjDef Class="FUNC">` → EPK function node.
- Map ARIS `<ObjDef Class="RULE">` → EPK connector (AND/OR/XOR based on `TypeNum`).
- Map ARIS `<CxnDef>` → EPK edge.
- Layout data from ARIS `<Pos>` elements stored in `node.metadata.layout`.

**Test Strategy:**  
- Round-trip: import ARIS-XML; verify node and edge count matches ARIS source.
- 5 representative real-world ARIS EPK files (sanitised).

---

### 6. Full-Text Inverted Index Integration

**Priority:** High  
**Target:** Q2 2026

**Scope:**  
Integrate process model names, descriptions, node names, and compliance tags into the ThemisDB inverted index for TF-IDF ranked search.

**Current State:**  
`ProcessModelManager::search()` performs in-memory substring matching over scanned records. No inverted index.

**Design Constraints:**  
- Inverted index must be populated incrementally on `save()` / `importBpmn()` etc.
- Index must support German morphological normalisation (compound splitting, lemmatisation) via the existing NLP text analyzer in `analytics/`.
- Search latency < 50 ms for 10,000 models.

**Implementation Notes:**
```cpp
// Integration with include/index/inverted_index.h (planned path)
// ProcessModelManager::save() extended:
inverted_index_.index(record.id, {
    record.name, record.description,
    record.long_description,
    join(record.compliance_tags, " ")
});

// ProcessModelManager::search() extended:
auto hits = inverted_index_.search(query, top_k);
```

**Test Strategy:**  
- Recall ≥ 0.85 on a 50-query benchmark over the VCC-VPB model library.
- Latency regression test: < 50 ms at 10,000 models on a single-core laptop.

**Performance Targets:**  
- Incremental index update ≤ 5 ms per model save.
- Search < 50 ms at 10,000 models.

---

### 7. Streaming Process Graph Updates via CDC Module

**Priority:** Low  
**Target:** Q4 2026

**Scope:**  
Use the ThemisDB CDC (Change-Data-Capture) module to stream process graph mutations (node added, edge added, instance state changed) to downstream consumers (dashboards, audit log, CEP engine).

**Current State:**  
Process graph mutations are written directly to RocksDB with no outbound event stream.

**Design Constraints:**  
- CDC events must be ordered (monotonically increasing sequence number per model).
- Downstream consumers must receive at-least-once delivery; deduplication is the consumer's responsibility.
- CDC event payload must not exceed 64 KB.

**Required Interfaces:**
```cpp
// Integration with analytics/cep_engine.h and CDC module (planned)
struct ProcessChangeEvent {
    enum class Type { NODE_ADDED, EDGE_ADDED, INSTANCE_STARTED,
                      INSTANCE_COMPLETED, INSTANCE_FAILED, ATTACHMENT_ADDED };
    Type type;
    std::string model_or_instance_id;
    nlohmann::json payload;
    int64_t sequence_number;
    int64_t timestamp_ms;
};
```

**Implementation Notes:**  
- Wrap `ProcessModelManager::save()`, `ProcessLinker::attachObject()`, and `ProcessGraphManager` execution hooks to publish `ProcessChangeEvent` to a CDC channel.
- CEP engine subscribes to this channel to trigger SLA and compliance rules.
- Dashboard WebSocket subscribers receive events in real time.

**Test Strategy:**  
- Unit: verify event published on each mutation; verify payload schema.
- Integration: CEP engine receives `INSTANCE_STARTED` event; SLA rule is registered within 100 ms.
