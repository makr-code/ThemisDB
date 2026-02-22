# ThemisDB LLM Orchestration – Architecture & Quickstart

## Overview

ThemisDB 1.5+ ships an **AI Orchestration Layer** that enables declarative,
YAML-configurable LLM request pipelines.  A single configuration file –
the **ThemisModePack** – defines the available modes, the tool registry, model
assignments, retrieval parameters, output constraints, budgets, and
observability settings.

```
┌────────────────────────────────────────────────────────────────────┐
│                        themis_server                               │
│                                                                    │
│   HTTP/AQL Request ──► AIOrchestrator::run()                      │
│                              │                                     │
│            ┌─────────────────▼──────────────────┐                 │
│            │         ModePack YAML               │                 │
│            │  modes: ask / edit / rag / agentic  │                 │
│            │         ethics / multi_agent        │                 │
│            └──────────┬────────────┬─────────────┘                │
│                       │            │                               │
│               ┌───────▼──┐  ┌──────▼───────┐                      │
│               │  Ask /   │  │  RAG Pipeline│                      │
│               │  Edit    │  │  (retrieval  │                      │
│               │ pipeline │  │   + LLM)     │                      │
│               └───────┬──┘  └──────┬───────┘                      │
│                       │            │                               │
│                ┌──────▼────────────▼──────┐                       │
│                │   LLMPluginInterface     │                       │
│                │   (llama.cpp / vLLM / …) │                       │
│                └──────────────────────────┘                       │
│                                                                    │
│   ToolRegistry ──► docs_search / aql_execute / schema_inspect …   │
└────────────────────────────────────────────────────────────────────┘
```

---

## Quickstart

### 1. Write a Mode Pack YAML

Create `config/ai_ml/llm/modes/my_modes.yaml`:

```yaml
apiVersion: themis.ai/v1
kind: ThemisModePack

metadata:
  name: my-modes
  version: "1.0.0"

default_mode: ask

models:
  - id: default
    path: /models/mistral-7b-instruct-v0.2.Q4_K_M.gguf
    gpu_layers: 32
    n_ctx: 4096

tools:
  - name: docs_search
    description: "Search documentation database"
    timeout_ms: 5000
    schema:
      type: object
      properties:
        query: { type: string }
        top_k:  { type: integer }
      required: [query]

modes:
  - id: ask
    description: "Plain Q&A without retrieval"
    model: default
    tools_allowed: []
    budgets:
      max_tokens: 512
      timeout_ms: 30000
      temperature: 0.7

  - id: rag
    description: "RAG with documentation retrieval"
    model: default
    tools_allowed: [docs_search]
    retrieval:
      enabled: true
      strategy: hybrid
      top_k: 5
      threshold: 0.4
      rerank: true
    budgets:
      max_tokens: 1024
      timeout_ms: 60000
      temperature: 0.5
    observability:
      log_requests: true
      metrics: true
```

### 2. Load and use in C++

```cpp
#include "llm/ai_orchestrator.h"
using namespace themis::llm;

// Load the mode pack
ValidationResult res;
ModePack pack = ModeSpecLoader::loadFromFile(
    "config/ai_ml/llm/modes/my_modes.yaml", &res);

if (!res) {
    for (const auto& e : res.errors) {
        spdlog::error("Mode spec error: {}", e);
    }
    return;
}

// Build the orchestrator
AIOrchestrator orch(pack);
orch.setLLMPlugin(my_llm_plugin);

// Register a tool handler
ToolSpec spec;
spec.name = "docs_search";
orch.toolRegistry().registerTool(spec, [&](const json& args, const ModeSpec&) {
    auto results = docsDb->search(args["query"], args.value("top_k", 5));
    json docs = json::array();
    for (auto& r : results) {
        docs.push_back({
            {"content",         r.text},
            {"source",          r.path},
            {"relevance_score", r.score},
        });
    }
    return json{{"documents", docs}};
});

// Execute a request
OrchestratorContext ctx;
ctx.query    = "How do I configure sharding in ThemisDB?";
ctx.mode_id  = "rag";
ctx.request_id = "req-001";

OrchestratorResult result = orch.run(ctx);
if (result.success) {
    std::cout << result.text << "\n";
    std::cout << "Tokens: " << result.metadata.tokens_generated << "\n";
    std::cout << "Latency: " << result.metadata.latency.total_ms << " ms\n";
}
```

### 3. Select mode per HTTP request

```http
POST /api/v1/llm/chat
Content-Type: application/json

{
  "query": "Explain ThemisDB replication",
  "mode": "rag"
}
```

---

## YAML Schema Reference

### Top-level fields

| Field          | Type   | Required | Description                                    |
|----------------|--------|----------|------------------------------------------------|
| `apiVersion`   | string | ✓        | Must be `themis.ai/v1`                         |
| `kind`         | string | ✓        | `ThemisModePack` or `ThemisAIPolicy`           |
| `metadata.name`| string |          | Human-readable pack name                       |
| `metadata.version` | string |      | Semantic version of the pack                   |
| `default_mode` | string |          | Mode used when none specified in request       |
| `models`       | array  |          | Model registry entries                         |
| `tools`        | array  |          | MCP-style tool definitions                     |
| `modes`        | array  | ✓        | Mode specifications (at least 1 required)      |

### `modes[]` fields

| Field              | Type    | Default    | Description                                |
|--------------------|---------|------------|--------------------------------------------|
| `id`               | string  | –          | Unique mode identifier (**required**)      |
| `description`      | string  | `""`       | Human-readable description                 |
| `model`            | string  | `"default"`| Reference to a model entry id              |
| `lora_adapter`     | string  | `""`       | LoRA adapter id (empty = none)             |
| `tools_allowed`    | string[]| `[]`       | Allowed tool names; `["*"]` = all          |
| `tools_denied`     | string[]| `[]`       | Explicitly denied tools (overrides allow)  |
| `retrieval`        | object  |            | See retrieval sub-spec                     |
| `output`           | object  |            | See output sub-spec                        |
| `budgets`          | object  |            | See budgets sub-spec                       |
| `observability`    | object  |            | See observability sub-spec                 |
| `judge`            | object  |            | LLM-as-judge quality evaluation            |
| `safety`           | object  |            | Ethics / safety guardrails                 |

### `retrieval` sub-spec

| Field              | Type    | Default    | Description                                |
|--------------------|---------|------------|--------------------------------------------|
| `enabled`          | bool    | `false`    | Enable RAG retrieval                       |
| `strategy`         | string  | `"hybrid"` | `"vector"`, `"fulltext"`, `"hybrid"`       |
| `top_k`            | int     | `5`        | Maximum documents to retrieve              |
| `threshold`        | float   | `0.5`      | Minimum relevance score                    |
| `rerank`           | bool    | `false`    | Apply cross-encoder reranking              |
| `chunking.size`    | int     | `512`      | Chunk size in tokens                       |
| `chunking.overlap` | int     | `64`       | Chunk overlap in tokens                    |
| `locality`         | string  | `""`       | Cluster/shard locality hint                |
| `read_ts_semantics`| string  | `"latest"` | `"latest"` or `"snapshot:<ts>"`            |

### `budgets` sub-spec

| Field           | Type  | Default | Description                    |
|-----------------|-------|---------|--------------------------------|
| `max_tokens`    | int   | `512`   | Max generation tokens          |
| `timeout_ms`    | int   | `30000` | Request timeout in milliseconds|
| `max_retries`   | int   | `1`     | Retry count on failure         |
| `temperature`   | float | `0.7`   | Sampling temperature `[0, 2]`  |
| `top_p`         | float | `0.9`   | Nucleus sampling p             |
| `top_k`         | int   | `40`    | Top-k sampling                 |

### `output` sub-spec

| Field         | Type   | Default  | Description                              |
|---------------|--------|----------|------------------------------------------|
| `format`      | string | `"text"` | `"text"`, `"json"`, `"markdown"`         |
| `json_schema` | string |          | JSON Schema string for structured output |
| `grammar`     | string |          | EBNF grammar name (e.g. `"react_agent"`) |

### `observability` sub-spec

| Field           | Type | Default | Description                     |
|-----------------|------|---------|---------------------------------|
| `log_requests`  | bool | `true`  | Log request metadata            |
| `log_responses` | bool | `false` | Log full responses (privacy)    |
| `metrics`       | bool | `true`  | Emit Prometheus metrics         |
| `trace`         | bool | `false` | Emit OpenTelemetry spans        |

---

## Predefined Modes

| Mode         | Description                               | Retrieval | Tools          |
|--------------|-------------------------------------------|-----------|----------------|
| `ask`        | Plain Q&A, fast direct answers            | ✗         | none           |
| `edit`       | Instruction-following / text editing      | ✗         | none           |
| `rag`        | Retrieval-Augmented Generation            | ✓         | docs_search    |
| `agentic`    | Single-agent with ReAct tool use          | ✓         | docs_search, aql_execute, schema_inspect |
| `multi_agent`| Multi-agent coordination (extensible)    | ✗         | docs_search    |
| `ethics`     | Constitutional AI / ethics guardrails     | ✓         | docs_search    |

---

## Integration with ThemisDB Modules

| Module              | Integration point                                                |
|---------------------|------------------------------------------------------------------|
| DocsAssistant       | RAG retrieval via `docs_search` tool                            |
| EmbeddedLLM         | Provides `LLMPluginInterface` to `AIOrchestrator::setLLMPlugin` |
| EthicalGuidelinesManager | Invoked by `ethics` mode system-prompt injection          |
| RAG Judge           | Enabled per-mode via `judge.enabled = true`                     |
| Prometheus metrics  | Hooks via `observability.metrics = true`                        |
| OpenTelemetry       | Hooks via `observability.trace = true`                          |

---

## Server Configuration

Add to your `config/config.yaml` or `config/config-minimal.yaml`:

```yaml
llm:
  orchestrator:
    enabled: true
    mode_spec_path: config/ai_ml/llm/modes/default.yaml
    # Override default mode for all requests without an explicit mode
    default_mode: ask
```

Or pass at startup:

```bash
themis_server --mode-spec config/ai_ml/llm/modes/default.yaml
```

---

## Extending with Custom Modes

Any string id not matching a built-in mode falls back to the **ask pipeline**.
To create a domain-specific mode:

```yaml
modes:
  - id: legal_rag
    description: "RAG with legal document retrieval"
    model: legal-lora
    lora_adapter: legal-qa-v1
    tools_allowed: [docs_search]
    retrieval:
      enabled: true
      strategy: vector
      top_k: 8
      threshold: 0.6
    budgets:
      max_tokens: 2048
      temperature: 0.3
    safety:
      enabled: true
      ethics_profile: config/compliance/legal_ethics.yaml
```

---

## Error Handling

`ModeSpecLoader` emits structured validation errors with key-path context:

```
[AIOrchestrator] Validation error: Mode 'rag': retrieval.top_k must be > 0 when retrieval is enabled
[AIOrchestrator] Validation error: default_mode 'xyz' is not defined in modes[]
[AIOrchestrator] Validation error: Duplicate mode id 'ask'
```

The `ValidationResult` struct captures all errors and warnings:

```cpp
ValidationResult res;
auto pack = ModeSpecLoader::loadFromFile("bad.yaml", &res);
if (!res.ok) {
    for (const auto& e : res.errors) std::cerr << "ERROR: " << e << "\n";
}
for (const auto& w : res.warnings) std::cerr << "WARN: " << w << "\n";
```

---

## Example YAML Files

| File | Description |
|------|-------------|
| `config/ai_ml/llm/modes/default.yaml` | Full mode pack with all 6 built-in modes |
| `config/ai_ml/llm/modes/minimal.yaml` | Minimal pack for CI/embedded deployments |
