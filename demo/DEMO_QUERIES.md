# ThemisDB Live Demo - Step by Step (Speaker + PowerShell + Expected Output)

This is one continuous walkthrough.
Each step always follows the same structure:

1. Speaker text (what you say)
2. PowerShell prompt (what you type)
3. Expected result (real, shortened output)

Real output source logs for these snippets:
- ai_working/demo_run_real_output_latest.log
- ai_working/themisctl_real_output_latest.log

Important copy/paste note:
- In PowerShell, do not paste the prompt prefix (`PS C:\...>`). Paste only the command itself.
- Example: use `& $THEMISCTL ...`, not `PS C:\Projects\ThemisDB> & $THEMISCTL ...`.

Unified query command pattern (for recognizability):
- `'{"query":"...",...}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST <endpoint> --stdin --content-type application/json`

## Step 1 - Open workspace and define themisctl

Speaker text:
"I am running the demo live from a Windows PowerShell session in the ThemisDB workspace."

PowerShell prompt:
```powershell
PS C:\> Set-Location C:\Projects\ThemisDB
PS C:\Projects\ThemisDB> $THEMISCTL = ".\build-msvc-windows-release\bin\themisctl.exe"
```

Expected result:
```text
(no output is expected for variable assignment)
```

PowerShell prompt (sanity check for this window):
```powershell
PS C:\Projects\ThemisDB> $THEMISCTL
PS C:\Projects\ThemisDB> Test-Path $THEMISCTL
```

Expected result:
```text
.\build-msvc-windows-release\bin\themisctl.exe
True
```

If `$THEMISCTL` is empty in your current window, run this fallback before Step 3:
```powershell
PS C:\Projects\ThemisDB> $THEMISCTL = ".\build-msvc-windows-release\bin\themisctl.exe"
```

## Step 2 - Start ThemisDB server (separate PowerShell window)

Speaker text:
"In a second PowerShell window, I start the ThemisDB server exactly like the demo script, including the two required compatibility switches."

PowerShell prompt:
```powershell
PS C:\> Set-Location C:\Projects\ThemisDB; $SERVER_DB_PATH = ".\demo\data\themis_db"; New-Item -ItemType Directory -Path $SERVER_DB_PATH -Force | Out-Null; .\build-msvc-windows-release\bin\themis_server.exe --db "$SERVER_DB_PATH" --port 8765 --allow-degraded-build --allow-stub-hsm
```

Required switches from `demo/kickstarter_demo_script.ps1`:
- `--allow-degraded-build`
- `--allow-stub-hsm`

Expected result (real, shortened):
```text
[PRE-FLIGHT] Server laeuft. Lade Demo-Daten...
...
Server is running and responding to queries.
```

Demo-Datenablage (wichtig fuer den Live-Call):
- Physischer DB-Pfad: `./demo/data/themis_db`
- Keys in der Demo-DB folgen dem Schema `<collection>:<prefix><nnnn>`
  - z. B. `demo_articles:art_0001`, `demo_embeddings:vec_0001`, `demo_knowledge_graph:node_0001`
- Import-Payload ist ein Wrapper mit `blob`:
  - `{"blob":"<jsonl-zeile-als-string>"}`

## Step 3 - Verify server reachability

Speaker text:
"First, I verify the server is reachable before running AI and graph endpoints."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 schema
```

Path-safe fallback (works even if variable was not set):
```powershell
PS C:\Projects\ThemisDB> .\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 schema
```

Expected result (real, shortened):
```text
(no output is also valid on success in current themisctl builds)
```

Optional explicit reachability check (with visible output):
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 health
```

Expected result (real):
```text
liveness: healthy
readiness: healthy
```

## Step 4 - Explain resilience behavior (auto model load)

Speaker text:
"If no default LLM plugin is active, the flow auto-loads the configured model so the demo can continue."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> '{"model_id":"default","path":"C:\\Projects\\ThemisDB\\models\\phi4.gguf"}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/models/load --stdin --content-type application/json
```

Path-safe fallback (works even if variable was not set):
```powershell
'{"model_id":"default","path":"C:\\Projects\\ThemisDB\\models\\phi4.gguf"}' | .\build-msvc-windows-release\bin\themisctl.exe --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/models/load --stdin --content-type application/json
```

Expected result (real, shortened):
```text
[PRECHECK] FAIL: Section 5 LLM inference endpoint
  "message": "LLM endpoint failure: No default LLM plugin available"
...
[PRECHECK] OK: model auto-load succeeded.
```

## Step 5 - LLM inference

Speaker text:
"Now I run direct model inference and show latency plus token count from the live response."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> '{"prompt":"Summarize the impact of ACID transactions for distributed databases in two sentences.","max_tokens":64,"temperature":0.2}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/inference --stdin --content-type application/json
```

PowerShell prompt (KPI overlay for model identity and inference validity):
```powershell
PS C:\Projects\ThemisDB> $inferRaw = '{"prompt":"Summarize the impact of ACID transactions for distributed databases in two sentences.","max_tokens":64,"temperature":0.2}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/inference --stdin --content-type application/json; $infer = $inferRaw | ConvertFrom-Json; [pscustomobject]@{ model_alias = $infer.model; model_path = 'C:\Projects\ThemisDB\models\phi4.gguf'; tokens_generated = [int]$infer.tokens_generated; inference_time_ms = [math]::Round([double]$infer.inference_time_ms,2); tokens_per_sec = [math]::Round(([double]$infer.tokens_generated * 1000.0) / [Math]::Max([double]$infer.inference_time_ms,1.0),2); chars_generated = [int]$infer.text.Length; chars_per_token = [math]::Round(([double]$infer.text.Length) / [Math]::Max([double]$infer.tokens_generated,1.0),2); hit_max_tokens = ([int]$infer.tokens_generated -ge 64); non_empty_text = ([string]::IsNullOrWhiteSpace($infer.text) -eq $false) } | Format-List
```

Expected result (real, shortened):
```json
{
  "generated_length": 395,
  "hit_max_tokens_limit": true,
  "inference_time_ms": 7563.83984375,
  "max_tokens_requested": 64,
  "model": "default",
  "ms_per_token": 118.18499755859375,
  "non_empty_text": true,
  "prompt_length": 85,
  "text": "assistantACID transactions ensure data integrity ...",
  "tokens_generated": 64,
  "tokens_per_second": 8.461310831810273
}
```

Expected KPI overlay (real example, shortened):
```text
model_alias       : default
model_path        : C:\Projects\ThemisDB\models\phi4.gguf
tokens_generated  : 64
inference_time_ms : 7388.06
tokens_per_sec    : 8.66
chars_generated   : 395
chars_per_token   : 6.17
hit_max_tokens    : True
non_empty_text    : True
```

Expected server console line (INFO, shortened):
```text
... [info] LLMApiHandler::handleInference success: model='default' prompt_len=85 tokens_generated=64 inference_time_ms=7388.06 lora='<none>'
```

## Step 6 - Graph query planner explain

Speaker text:
"This call shows how the graph planner selects an algorithm and reports estimated execution cost."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> '{"query_type":"k_hop","start_vertex":"demo_knowledge_graph:node_0001","max_depth":1}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/graph/query/explain --stdin --content-type application/json
```

Expected result (real, shortened):
```json
{
  "algorithm": "BFS",
  "estimated_time_ms": 0.11199999999999999,
  "pattern": "K-Hop Neighborhood",
  "use_index": true,
  "use_cache": true
}
```

## Step 7 - RAG endpoint

Speaker text:
"Now we combine retrieval and generation. The response includes retrieved document count and inference timing."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> '{"query":"What are the latest papers on quantum computing by MIT?","collection":"demo_articles","top_k":3,"max_tokens":96,"temperature":0.2}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/rag --stdin --content-type application/json
```

Expected result (real, shortened):
```json
{
  "documents_retrieved": 3,
  "inference_time_ms": 11044.544921875,
  "model": "default",
  "text": "assistantAs a large language model, I can't provide real-time updates ...",
  "tokens_generated": 96
}
```

Expected server console line (INFO, shortened):
```text
... [info] LLMApiHandler::handleRAG success: query_len=57 collection='demo_articles' top_k=3 docs_retrieved=3 tokens_generated=96 inference_time_ms=11044.54 cache_hit=false rag_mode='enhanced' lora='<none>'
```

## Step 7b - Optional LoRA query endpoint (observability proof)

Speaker text:
"If an adapter is available, I run a LoRA-specific query to verify adapter-level request logging on the server console."

PowerShell prompt (optional):
```powershell
PS C:\Projects\ThemisDB> '{"model_id":"default","adapter_id":"demo_adapter","prompt":"Summarize why adapter-based fine-tuning helps domain adaptation.","max_tokens":64,"temperature":0.2}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /api/v1/llm/lora/query --stdin --content-type application/json
```

Expected result (target, shortened):
```json
{
  "adapter_id": "demo_adapter",
  "inference_time_ms": 7000.0,
  "model_id": "default",
  "response": "...",
  "tokens_used": 64
}
```

Expected server console line (INFO, shortened):
```text
... [info] LoRAApiHandler::handleLoRAQuery success: model_id='default' adapter_id='demo_adapter' prompt_len=66 tokens_used=64 inference_time_ms=7000
```

## Step 8 - AQL showcase (query endpoint)

Speaker text:
"This AQL showcase runs a direct query against the AQL endpoint and returns matching records with metadata."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> '{"query":"FOR d IN demo_articles LIMIT 1 RETURN d"}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /query/aql --stdin --content-type application/json
```

Expected result (target, shortened):
```json
{
  "result": [
    {
      "id": "demo_articles:article_001",
      "blob": "..."
    }
  ],
  "execution_time_ms": 0.0,
  "count": 1
}
```

Observed fallback in some current builds (known limitation):
```json
{
  "error": true,
  "status_code": 400,
  "message": "Query execution failed: ... Optimized entity execution failed for table 'demo_articles' ..."
}
```

## Step 9 - GraphQL showcase (schema + query)

Speaker text:
"Now I run a richer GraphQL operation with variables, aliases, and multiple root fields to show schema and graph access in one call."

PowerShell prompt (schema):
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 api GET /graphql/schema
```

Expected result (schema, shortened SDL):
```graphql
schema {
  query: Query
  mutation: Mutation
  subscription: Subscription
}

type Query {
  document(collection: String!, id: ID!): Document
  documents(collection: String!, limit: Int, offset: Int): [Document!]!
  aql(query: String!, variables: JSON): JSON
  graphTraversal(startNode: ID!, depth: Int, direction: String): [Node!]!
}
```

PowerShell prompt (advanced query via stdin):
```powershell
PS C:\Projects\ThemisDB> '{"query":"query GraphDashboard($start: ID!, $depth: Int!) { apiVersion schemaVersion kHop: graphTraversal(startNode: $start, depth: $depth, direction: \"out\") { id labels properties } }","variables":{"start":"demo_knowledge_graph:node_0001","depth":1}}' | & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 api POST /graphql --stdin --content-type application/json
```

Expected result (shortened):
```json
{
  "data": {
    "apiVersion": "1.8.0-rc1",
    "kHop": null,
    "schemaVersion": "2.0.0"
  }
}
```

Optional interpretation line for live demo:
```text
If graph resolvers are active in the current runtime profile, `kHop` returns node rows instead of null.
```

## Step 10 - Documentation-aware help

Speaker text:
"This is docs-aware help mode. It answers an operational question using ThemisDB documentation context."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --timeout 180 --host 127.0.0.1 --port 8765 help --mode lora "How do I configure sharding and RAG safely in ThemisDB?"
```

Expected result (real, shortened):
```text
themis-help (lora):
assistantTo configure sharding and Retrieval-Augmented Generation (RAG) safely in ThemisDB, you can follow these general guidelines...
```

## Step 11 - CRUD consistency probe

Speaker text:
"I now write and read back a probe entity to show that core data operations remain consistent during AI traffic."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 put demo_articles:runtime_probe '{"blob":"{\"title\":\"Runtime Probe\",\"content\":\"Compatibility mode\"}"}'
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 get demo_articles:runtime_probe
```

Expected result (real, shortened):
```text
[OK] Entity 'demo_articles:runtime_probe' stored.
(then JSON entity on readback)
```

## Step 12 - Index recommendation

Speaker text:
"Finally, I check optimization guidance from the system recommender."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> & $THEMISCTL --host 127.0.0.1 --port 8765 index recommend demo_articles
```

Expected result (real):
```text
(no recommendations for demo_articles)
```

## Step 13 - Full scripted run (optional, end-to-end proof)

Speaker text:
"As a final proof, I can run the complete scripted flow and capture everything to a logfile."

PowerShell prompt:
```powershell
PS C:\Projects\ThemisDB> $env:THEMIS_DEMO_NO_PAUSE = '1'
PS C:\Projects\ThemisDB> pwsh -NoProfile -ExecutionPolicy Bypass -File .\demo\kickstarter_demo_script.ps1 2>&1 | Tee-Object -FilePath .\ai_working\demo_run_real_output_latest.log
```

Expected result (real, shortened):
```text
Demo Complete!
...
ThemisDB demo checks passed. System appears operational for this scenario.
```

## 60-90s Fast Pitch Sequence

If you need a very short live version, run Steps 3, 5, 6, 8, 9, and 10 in order.
