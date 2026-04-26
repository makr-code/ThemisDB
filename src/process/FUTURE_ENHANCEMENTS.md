> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# Process Modeling Module – Future Enhancements

**Version:** 1.0.0
**Status:** 📋 Planned
**Last Updated:** 2026-04-06
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

---

## Stand der Wissenschaft und Technik

> Vollständige Literaturanalyse: [`docs/de/process/STATE_OF_THE_ART.md`](../../docs/de/process/STATE_OF_THE_ART.md)

Die folgende Tabelle ordnet geplante Features ihren wissenschaftlichen Quellen zu und
gibt eine Einschätzung zu Impact und Implementierungsaufwand.

| # | Feature | Wissenschaftliche Quelle | Impact | Effort | Target |
|---|---------|--------------------------|--------|--------|--------|
| P1 | LLM-to-BPMN Generator | ProcessGPT, Busch 2023 | Hoch | M | Q2 2026 |
| P2 | PPR-basiertes GraphRAG Scoring | HippoRAG, Gutierrez 2024 | Hoch | M | Q2 2026 |
| P3 | OCEL 2.0 Export | OCEL 2.0 Spec, Berti 2023 | Mittel | S | Q2 2026 |
| P4 | Leiden-Community-Detection | GraphRAG, Edge 2024 | Hoch | M | Q3 2026 |
| P5 | Duales Retrieval Local/Global | LightRAG, Guo 2024 | Hoch | M | Q3 2026 |
| P6 | Object-Centric Process Mining | van der Aalst 2022 | Sehr Hoch | L | Q3 2026 |
| P7 | DMN 1.5 Entscheidungstabellen | OMG DMN 1.5, 2023 | Hoch | M | Q3 2026 |
| P8 | FIM-Prozessbibliothek-Import | FITKO FIM, 2024 | Sehr Hoch | M | Q4 2026 |
| P9 | CMMN 1.1 Fallmodellierung | OMG CMMN 1.1, 2016 | Mittel | L | Q4 2026 |
| P10 | ProcessTransformer Vorhersage | Bukhsh et al. 2021 | Hoch | L | Q1 2027 |

---

## Planned Features

### 0. PPR-basiertes GraphRAG Scoring (HippoRAG-Ansatz)

**Priority:** High
**Target:** Q2 2026
**Wissenschaftliche Basis:** Gutierrez et al. (2024). *HippoRAG: Neurobiologically Inspired
Long-Term Memory for Large Language Models.* NeurIPS 2024. arXiv:2405.14831.

**Scope:**
Replace the current BFS-based subgraph extraction in `ProcessGraphRag::retrieve()` with
Personalized PageRank (PPR) scoring. PPR naturally handles multi-hop queries that span
several process steps — e.g. "Which documents were attached after the completeness check?"
(3 hops: completeness node → tokens → attachments → documents).

**Current State:**
`ProcessGraphRag::extractSubgraph()` uses BFS from seed nodes with a fixed depth limit.
All nodes within the BFS radius are returned with equal weight. This misses long-distance
but highly relevant nodes.

**Design Constraints:**
- PPR must terminate in < 20 ms for graphs with up to 500 nodes.
- Damping factor α = 0.85 (same as standard PageRank).
- Power iteration converges when `||r_new - r_old||_1 < 1e-6`.
- API must remain backward-compatible: BFS is kept as fallback when graph is a tree.

**Required Interfaces:**
```cpp
// Replaces BFS in process_graph_rag.cpp
struct PprConfig {
    float damping{0.85f};
    int   max_iterations{50};
    float convergence_epsilon{1e-6f};
    int   top_k_nodes{20};
};

// Returns top-k nodes by PPR score from seed_node_ids
std::vector<std::pair<std::string, float>> computePpr(
    const nlohmann::json& normalized_graph,
    const std::vector<std::string>& seed_node_ids,
    const PprConfig& config = {}
) const;
```

**Implementation Notes:**
- Build sparse adjacency matrix from `normalized.edges`.
- Personalisation vector: uniform over `seed_node_ids`, zero elsewhere.
- Power iteration: `r = α * A^T * r + (1-α) * personalization`.
- Return top-k nodes sorted by converged PPR score.
- Integrate into `ProcessGraphRag::retrieve()` by replacing `extractSubgraph()` call.

**Test Strategy:**
- Unit: known 10-node graph; verify PPR assigns highest scores to directly-connected seeds.
- Integration: 3-hop query on Bauantrag model; verify target node appears in top-3.
- Regression: BFS-equivalent result on a linear chain (PPR degrades to BFS on trees).

**Performance Targets:**
- ≤ 20 ms for 500-node graph, 50 iterations.

---

### 0b. LLM-to-BPMN Generator (ProcessGPT-Ansatz)

**Priority:** High
**Target:** Q2 2026
**Wissenschaftliche Basis:** Busch, K. et al. (2023). *ProcessGPT: Transforming Business
Process Management with Generative AI.* IEEE Big Data 2023.

**Scope:**
Allow users to generate a `ProcessModelRecord` from a free-text natural language description.
The LLM generates the process structure; ThemisDB validates the BPMN semantics and stores
the result.

**Current State:**
Process models can only be imported from existing BPMN/EPK/VCC-VPB files. There is no
path from a natural language description to a process model.

**Design Constraints:**
- Maximum 3 LLM validation retries (generate → validate → fix loop).
- Must produce a valid `ProcessModelRecord` with `normalized` graph that passes
  `ProcessGraphManager::registerProcess()` without errors.
- LLM backend is pluggable (OpenAI, local llama.cpp, Ollama).
- BPMN validity rules: (1) exactly one start event, (2) at least one end event,
  (3) all gateways have at least one outgoing edge, (4) no isolated nodes.

**Required Interfaces:**
```cpp
// New file: include/process/process_model_generator.h
namespace themis::process {

class ProcessModelGenerator {
public:
    struct Config {
        std::string llm_endpoint;    // REST endpoint URL
        std::string llm_model;       // e.g. "gpt-4o", "llama-3.1-70b"
        int         max_retries{3};
        std::string language{"de"};
        ProcessDomain domain{ProcessDomain::BUSINESS};
    };

    // Generate a ProcessModelRecord from natural language description
    // Calls LLM, validates BPMN semantics, retries on validation failure
    std::pair<bool, ProcessModelRecord> generateFromDescription(
        std::string_view description,
        const Config& cfg = {}
    );

    // Refine an existing model based on feedback
    std::pair<bool, ProcessModelRecord> refine(
        const ProcessModelRecord& existing,
        std::string_view feedback,
        const Config& cfg = {}
    );
};

} // namespace themis::process
```

**Implementation Notes:**
- Prompt template (DE): system = "Du bist BPMN 2.0 Experte. Erstelle ein Prozessmodell.
  Ausgabe: JSON mit {id, name, domain, activities:[{id,name,type,sla_hours}], edges:[{from,to,type}]}"
- After LLM response: parse as JSON → `VccVpbImporter::importYaml()` style conversion.
- Validation: check start/end events, gateway balance, no isolated nodes.
- On failure: send validation errors back to LLM with "Korrigiere folgende Fehler: ...".

**Test Strategy:**
- Unit: mock LLM with known response; verify ProcessModelRecord produced correctly.
- Integration: generate "Bauantrag" model; verify ≥ 5 nodes, ≥ 1 gateway, ≥ 1 start/end.
- Regression: generated model deployable to `ProcessGraphManager` without error.

---

### 0c. OCEL 2.0 Export (Object-Centric Event Log)

**Priority:** Medium
**Target:** Q2 2026
**Wissenschaftliche Basis:** Berti, A. et al. (2023). *OCEL 2.0 Specification.*
Process Mining Group, RWTH Aachen. doi:10.5281/zenodo.8428111.

**Scope:**
Export process instances with all attached objects (documents, metadata) as an OCEL 2.0
JSON log. This enables import into external process mining tools (PM4Py, Celonis, ProM)
for advanced analytics — object-centric discovery, conformance checking, enhancement.

**Current State:**
No event log export exists. Process execution data is queryable via AQL but not exportable
in a process mining standard format.

**Required Interfaces:**
```cpp
// New file: include/process/ocel_exporter.h
namespace themis::process {

class OcelExporter {
public:
    // Export a single instance as OCEL 2.0 JSON
    nlohmann::json exportInstance(std::string_view instance_id) const;

    // Export all instances of a model as OCEL 2.0 JSON
    nlohmann::json exportModel(std::string_view model_id) const;

    // Export with date range filter
    nlohmann::json exportFiltered(
        std::string_view model_id,
        int64_t from_ms,
        int64_t to_ms
    ) const;
};

} // namespace themis::process
```

**Implementation Notes:**
- OCEL 2.0 JSON schema: `{objectTypes, eventTypes, objects, events}`.
- Object types: derive from `ProcessLinker` attachment collections.
- Events: derive from `ProcessToken::visited_nodes` + timestamps.
- Each event's `relationships` list: all attachments active at that token timestamp.
- Output validated against OCEL 2.0 JSON schema (schema available from RWTH).

---

### 0d. Leiden-Community-Detection für Prozesscluster (GraphRAG-Ansatz)

**Priority:** High
**Target:** Q3 2026
**Wissenschaftliche Basis:** Edge, D. et al. (2024). *From Local to Global: A Graph RAG
Approach to Query-Focused Summarization.* Microsoft Research. arXiv:2404.16130.

**Scope:**
Group process nodes into semantic communities using the Leiden algorithm. Pre-generate
LLM "community reports" per cluster. Global queries (e.g. "Describe the approval workflow")
are answered using community reports rather than traversing individual nodes.

**Current State:**
No community structure in the process graph. All graph context is assembled by BFS/PPR
from the current node, which can miss high-level conceptual groups.

**Design Constraints:**
- Community detection must be recomputable on model update in < 500 ms for 500-node graphs.
- Community reports must be cached as `proc:community:<model_id>:<community_id>` keys.
- Report generation requires a configured LLM endpoint.

**Required Interfaces:**
```cpp
// New file: include/process/process_community_detector.h
namespace themis::process {

struct ProcessCommunity {
    std::string community_id;
    std::vector<std::string> node_ids;
    std::string label;          // Short label, LLM-generated
    std::string report;         // Full LLM-generated summary of this community
    float modularity_score;
};

class ProcessCommunityDetector {
public:
    // Run Leiden algorithm on a process model graph
    // Returns detected communities sorted by size desc
    std::vector<ProcessCommunity> detect(
        std::string_view model_id,
        float resolution = 1.0f  // Leiden resolution parameter
    ) const;

    // Generate LLM community report for a detected community
    std::string generateReport(
        const ProcessCommunity& community,
        std::string_view model_id,
        std::string_view llm_endpoint,
        std::string_view language = "de"
    ) const;

    // Store communities in DB (proc:community: prefix)
    bool persistCommunities(
        std::string_view model_id,
        const std::vector<ProcessCommunity>& communities
    );

    // Retrieve stored communities
    std::vector<ProcessCommunity> loadCommunities(std::string_view model_id) const;
};

} // namespace themis::process
```

---

### 0e. Object-Centric Process Mining (OCPM)

**Priority:** Very High
**Target:** Q3 2026
**Wissenschaftliche Basis:** van der Aalst, W.M.P. (2022). *Object-Centric Process Mining:
Dealing with Divergence and Convergence in Event Data.* LNCS 12551.

**Scope:**
Track multiple object types (Antragsteller, Dokument, Prüfer, Grundstück) simultaneously
through a Verwaltungsvorgang. This is essential for German administrative proceedings
which inherently involve many interconnected real-world objects.

**Current State:**
`ProcessLinker` attaches documents to instances, but there is no unified view across object
types. Process mining is case-centric (one case ID = one instance).

**Design Constraints:**
- Object types are declared at model level (in `ProcessModelRecord::normalized`).
- Object instances are linked via `ProcessLinker::attachObject()`.
- OCPM analysis must be exportable as OCEL 2.0 (see feature 0c above).
- Object-centric DFG computation: ≤ 5 s for 10,000 events.

**Required Interfaces:**
```cpp
// New file: include/process/object_centric_tracer.h
namespace themis::process {

struct OcelEvent {
    std::string event_id;
    std::string activity;
    int64_t timestamp_ms;
    // {object_type → [object_ids]}
    std::unordered_map<std::string, std::vector<std::string>> object_refs;
    nlohmann::json attributes;
};

class ObjectCentricTracer {
public:
    // Build OCEL 2.0 log from instance + all attachments
    nlohmann::json buildOcelLog(std::string_view instance_id) const;

    // Compute Directly-Follows Multigraph for a specific object type
    nlohmann::json computeDfmg(
        std::string_view model_id,
        std::string_view object_type
    ) const;

    // Find convergence (many→one) and divergence (one→many) nodes
    struct ConvergenceDivergenceResult {
        std::vector<std::string> convergence_nodes;
        std::vector<std::string> divergence_nodes;
    };
    ConvergenceDivergenceResult analyze(std::string_view model_id) const;
};

} // namespace themis::process
```

---

### 0f. DMN 1.5 Entscheidungstabellen

**Priority:** High
**Target:** Q3 2026
**Wissenschaftliche Basis:** OMG (2023). *Decision Model and Notation (DMN) 1.5.*
Object Management Group Specification.

**Scope:**
Support DMN 1.5 decision tables embedded in or linked to BPMN process nodes. This enables
rule-based administrative decisions (Verwaltungsentscheidungen) to be modelled transparently,
versioned alongside the process, and evaluated at runtime.

**Current State:**
Gateway conditions are stored as free-text strings. No structured decision modelling.

**Design Constraints:**
- DMN tables are stored as JSON in `ProcessNodeInfo::metadata["dmn_table"]`.
- FEEL (Friendly Enough Expression Language) subset: numeric comparisons, string equality,
  range expressions `[a..b]`, boolean `and`/`or`. Full FEEL S-expressions out of scope.
- Hit policies: UNIQUE, FIRST, COLLECT supported. RULE_ORDER, OUTPUT_ORDER planned.

**Required Interfaces:**
```cpp
// New file: include/process/dmn_evaluator.h
namespace themis::process {

struct DmnRule {
    std::vector<std::string> input_expressions; // FEEL expressions
    nlohmann::json output_values;
};

struct DecisionTable {
    std::string id;
    std::string name;
    std::vector<std::string> input_columns;
    std::vector<std::string> output_columns;
    std::vector<DmnRule> rules;
    std::string hit_policy; // "UNIQUE", "FIRST", "COLLECT"
};

class DmnEvaluator {
public:
    bool loadFromXml(std::string_view dmn_xml);
    bool loadFromJson(const nlohmann::json& dmn_json);

    // Evaluate decision table with input context
    nlohmann::json evaluate(
        std::string_view decision_id,
        const nlohmann::json& input_context
    ) const;

    // Evaluate a FEEL expression against a value
    bool evaluateFeel(std::string_view feel_expr, const nlohmann::json& value) const;
};

} // namespace themis::process
```

**Integration with ProcessGraphRag::checkCompliance():**
- If a process node references a DMN decision table (`node.metadata.dmn_ref`),
  `DmnEvaluator::evaluate()` is called with current instance variables.
- Result determines whether the node's compliance condition is satisfied.

---

### 0g. FIM-Prozessbibliothek-Import (Föderales Informationsmanagement)

**Priority:** Very High (German public administration)
**Target:** Q4 2026
**Wissenschaftliche Basis:** FITKO (2024). *Föderales Informationsmanagement –
Handbuch Version 3.* Berlin: FITKO.

**Scope:**
Import process models directly from the German Federal Information Management (FIM)
standardised process library, which contains ~5,000 administrative process blueprints
used across all German federal states.

**Current State:**
VCC-VPB YAML covers a subset of administrative processes. FIM provides the authoritative
source with legal basis, responsible authorities, and required documents per process.

**Design Constraints:**
- FIM uses its own XML schema (different from BPMN and EPK).
- Import must preserve FIM `Leistungscode` (service code) in `compliance_tags`.
- FIM processes are read-only references; local copies can be customised but must track
  the source FIM version.
- Network access for FITKO API is optional; local FIM XML files must also work.

**Required Interfaces:**
```cpp
// New file: include/process/fim_importer.h
namespace themis::process {

class FimImporter {
public:
    // Import from FIM-XML file (downloaded from FITKO portal)
    ProcessModelResult importFimXml(std::string_view fim_xml);

    // Import all models from a FIM XML catalogue file
    std::vector<ProcessModelResult> importFimCatalogue(
        std::string_view catalogue_xml
    );

    // Load from FITKO REST API (requires network + API key)
    std::vector<ProcessModelResult> importFromFitkoApi(
        std::string_view api_url,
        std::string_view api_key,
        std::optional<std::string_view> leistungsbereich = std::nullopt
    );

    // Map FIM Leistungsbereich to ThemisDB ProcessDomain
    static ProcessDomain mapFimDomain(std::string_view fim_leistungsbereich);
};

} // namespace themis::process
```

---

### 0h. CMMN 1.1 – Case Management Model and Notation

**Priority:** Medium
**Target:** Q4 2026
**Wissenschaftliche Basis:** OMG (2016). *Case Management Model and Notation (CMMN) 1.1.*

**Scope:**
Support CMMN 1.1 case models for adaptive, ad-hoc administrative proceedings where the
officer decides the order and applicability of tasks (Discretionary Tasks). Examples:
complex building permits, social welfare cases, legal appeals.

**Current State:**
Only BPMN 2.0 (structured) and EPK (semi-structured) are supported. Unstructured /
adaptive cases cannot be modelled.

**Required Interfaces:**
```cpp
// Extension to process_model_manager.h:
// + CMMN_1_1 to ProcessNotation enum

// New file: include/process/cmmn_serializer.h
namespace themis::process {

enum class CmmnNodeType {
    CASE, STAGE, HUMAN_TASK, PROCESS_TASK, CASE_TASK,
    MILESTONE, EVENT_LISTENER, DISCRETIONARY_ITEM
};

class CmmnSerializer {
public:
    struct ImportResult {
        bool ok;
        std::string message;
        std::vector<ProcessNodeInfo> nodes;
        std::vector<ProcessEdgeInfo> edges;
    };
    static ImportResult importXml(std::string_view cmmn_xml);
    static std::string  exportXml(
        std::string_view case_id,
        std::string_view case_name,
        const std::vector<ProcessNodeInfo>& nodes,
        const std::vector<ProcessEdgeInfo>& edges
    );
};

} // namespace themis::process
```

---

### 0i. ProcessTransformer – Predictive Process Monitoring

**Priority:** High
**Target:** Q1 2027
**Wissenschaftliche Basis:** Bukhsh, Z.A. et al. (2021). *ProcessTransformer: Predictive
Business Process Monitoring with Transformer Network.* arXiv:2104.00721.

**Scope:**
Predict the next activity, outcome, remaining time, and responsible officer for a running
process instance. Uses a Transformer model trained on historical completed instances.

**Current State:**
`ProcessGraphRag::findSimilarCases()` retrieves similar past cases but does not predict
future behaviour.

**Required Interfaces:**
```cpp
// New file: include/process/process_predictor.h
namespace themis::process {

class ProcessPredictor {
public:
    struct Prediction {
        struct NextActivity {
            std::string node_id;
            std::string name;
            float       probability;
        };
        std::vector<NextActivity> next_activities; // top-3
        float  completion_probability;
        double estimated_remaining_ms;
        std::string predicted_assignee;
        // SHAP-style importance of past activities
        std::vector<std::pair<std::string, float>> activity_importance;
    };

    Prediction predict(
        std::string_view instance_id,
        std::string_view model_id
    ) const;

    // Train / update predictor on completed instances
    bool updateModel(
        std::string_view process_definition_id,
        const std::vector<std::string>& completed_instance_ids
    );
};

} // namespace themis::process
```

---

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
New ARIS XML parser implementation file (planned):
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
