# Copilot ↔ Ollama Router — ROADMAP

> **Scope**: VS Code extension at `tools/copilot-ollama-router/`.
> Routes Copilot chat requests to a local Ollama model for code-generation tasks
> while keeping Copilot cloud for security reviews, architecture decisions, and
> complex debugging. Fully project-agnostic — no ThemisDB dependency.

---

## Current Status

**v0.2.0 — functional prototype, not yet Marketplace-ready.**

| Component | State |
|---|---|
| `DelegationRouter` — keyword-based classifier | ✅ done |
| `OllamaClient` — streaming HTTP client | ✅ done |
| `ContextManager` — prompt enrichment (file, lang, selection, diagnostics) | ✅ done |
| `CopilotReviewer` — optional quality review via VS Code LM API | ✅ done |
| `ModelSetupManager` — catalog, download wizard, workspace config | ✅ done |
| `@ollama` chat participant | ✅ done |
| 6 command palette commands | ✅ done |
| Idempotent workspace config generation | ✅ done |
| Rename `vscode-ollama-bridge` → `copilot-ollama-router` | ✅ done |
| Unit & integration tests | ❌ missing |
| VS Code Marketplace packaging | ❌ missing |

---

## In Progress

- [~] Stabilisation: full rename completed, compile clean (`v0.2.0`)
- [~] ROADMAP scoping and analysis

---

## Planned Features

### Routing Intelligence
- [ ] Embedding-based semantic classifier (replace keyword regex, Target: Q3 2026)
- [ ] Language-aware routing profiles — Python, TypeScript, Rust, Go, C++ (Target: Q3 2026)
- [ ] Routing decision history and audit log in Output Channel (Target: Q3 2026)
- [ ] Confidence score display in chat response header (Target: Q3 2026)
- [ ] Webview-based routing rule editor (custom keyword sets per project) (Target: Q4 2026)

### Context & UX
- [ ] Smart context-window management (token-budget-aware truncation) (Target: Q3 2026)
- [ ] Multi-file context via import graph / workspace symbol index (Target: Q4 2026)
- [ ] Inline completion provider routed to Ollama (not just chat) (Target: Q4 2026)
- [ ] Streaming token counter in VS Code status bar (Target: Q3 2026)
- [ ] Similarity-based response cache (TTL + max-size configurable) (Target: Q4 2026)

### Model Management
- [ ] Per-model performance metrics (latency P50/P95, quality score) (Target: Q3 2026)
- [ ] HumanEval subset benchmark runner (local, no internet needed) (Target: Q4 2026)
- [ ] Automatic model selection by task type + detected available VRAM (Target: Q4 2026)
- [ ] LoRA adapter management (load/unload, ThemisDB training integration) (Target: Q4 2026)
- [ ] Mid-conversation model switching via `/switch <model>` command (Target: Q4 2026)

### Multi-Provider
- [ ] LM Studio REST API support (OpenAI-compatible) (Target: Q4 2026)
- [ ] LocalAI backend support (Target: Q1 2027)
- [ ] Remote Ollama over SSH tunnel / Docker (Target: Q1 2027)
- [ ] Provider health dashboard (webview panel) (Target: Q1 2027)

### Quality & Distribution
- [ ] Unit tests: `DelegationRouter`, `ContextManager`, `OllamaClient`, `ModelSetupManager` (Target: Q3 2026)
- [ ] Integration tests with mocked Ollama server (Target: Q3 2026)
- [ ] CI workflow (GitHub Actions: compile + lint + test) (Target: Q3 2026)
- [ ] VS Code Marketplace VSIX packaging and publishing (Target: Q1 2027)
- [ ] Opt-in telemetry: routing decisions + latency (Target: Q1 2027)
- [ ] Team-shared routing profiles via workspace settings (Target: Q1 2027)

---

## Implementation Phases

### Phase 1 — Stabilisation & Testing (Q2–Q3 2026)

> Goal: turn the prototype into a testable, CI-gated baseline.

- [x] Rename package to `copilot-ollama-router`, publisher to `makr-code`
- [x] Update all command IDs to `copilotOllamaRouter.*`
- [x] Update config prefix to `copilotOllamaRouter.*`
- [x] Clean TypeScript compile (`v0.2.0`)
- [ ] Write unit tests for `DelegationRouter.classify()` — all branches, edge cases
  - Inputs: prompt text, activeLanguage, mode flags
  - Coverage target: ≥ 90 % branch coverage
  - Test framework: Mocha + `@vscode/test-electron`
- [ ] Write unit tests for `ContextManager.buildPrompt()` — with/without selection, diagnostics
- [ ] Mock `OllamaClient` and test `generate()` streaming + timeout + abort
- [ ] Mock `CopilotReviewer` and test fragment accumulation
- [ ] Mock `ModelSetupManager.generateWorkspaceConfig()` and verify idempotency
- [ ] GitHub Actions CI: `npm run compile && npm run lint && npm run test`
- [ ] Add `npm run test` script to `package.json`
- [ ] Add ESLint config baseline (no-explicit-any, strict-null-checks)

### Phase 2 — Smarter Routing (Q3 2026)

> Goal: reduce false-positive routing decisions by ≥ 40 % vs. keyword baseline.

- [ ] Design routing classifier interface: `classify(prompt, context): RoutingDecision`
  - Must be synchronous (< 5 ms) to avoid latency in chat flow
  - Must be replaceable (strategy pattern)
- [ ] Implement TF-IDF scoring on keyword sets as improved baseline
  - Inputs: tokenised prompt, weighted keyword bags
  - Output: score per destination + confidence float [0.0, 1.0]
  - Threshold: configurable (`copilotOllamaRouter.routingConfidenceThreshold`, default 0.6)
- [ ] Language-aware routing profiles
  - Detected languages: cpp, python, typescript, rust, go, java
  - Config: `copilotOllamaRouter.languageProfiles` (JSON map lang → preferred model)
- [ ] Routing audit log (Output Channel `Copilot Ollama Router`)
  - Log: timestamp, prompt preview (first 80 chars), decision, confidence, model
  - Configurable verbosity: `off | info | verbose`
- [ ] Confidence display in chat response header (`🖥️ Ollama (codellama:13b) — confidence 87%`)
- [ ] Validate routing accuracy on 100-item labelled prompt dataset
  - Baseline keyword: accuracy target ≥ 75 %
  - TF-IDF target: accuracy ≥ 88 %

### Phase 3 — Context & UX (Q3–Q4 2026)

> Goal: reduce prompt round-trips and improve response relevance.

- [ ] Token-budget-aware context truncation
  - Config: `copilotOllamaRouter.contextTokenBudget` (default 2048)
  - Priority: selection > diagnostics > cursor window > file path
  - Implementation: character-based estimate (1 token ≈ 4 chars), not tiktoken
- [ ] Multi-file context via `vscode.workspace.findFiles` + symbol index
  - Adds: imported module signatures, parent class definitions
  - Max files: 3, max lines per file: 50
- [ ] Inline completion provider (`vscode.languages.registerInlineCompletionItemProvider`)
  - Trigger: debounced 400 ms after keystroke in supported languages
  - Model: use `copilotOllamaRouter.inlineModel` (default same as `defaultModel`)
  - Toggle: `copilotOllamaRouter.inlineCompletionEnabled` (default false)
- [ ] Status bar item: streaming token counter (`⚡ Ollama: 142 tok`)
  - Shows spinner during active request, clears after 3 s on completion
- [ ] Response cache (in-memory, session-scoped)
  - Key: SHA-256 of (model + normalised prompt)
  - TTL: 300 s, max entries: 50
  - Configurable: `copilotOllamaRouter.responseCacheEnabled` (default true)

### Phase 4 — Model Management (Q3–Q4 2026)

> Goal: give developers real data about local model quality and cost.

- [ ] Per-model metrics store (`Map<string, ModelMetrics>`, persisted in `ExtensionContext.globalState`)
  - Fields: requestCount, totalLatencyMs, errorCount, lastUsed
  - Display: `Ollama Router: List Installed Models` command shows metrics table
- [ ] HumanEval benchmark runner
  - Input: bundled 20-item HumanEval subset (permissively licensed)
  - Output: pass@1 score + latency P50 per installed model
  - Command: `Copilot Ollama Router: Benchmark Installed Models`
  - Runtime: ≤ 5 min for 7B model on mid-range GPU
- [ ] VRAM-aware auto model selection
  - Reads available VRAM via `nvidia-smi --query-gpu=memory.free --format=csv,noheader`
    on Linux/Windows; `system_profiler SPDisplaysDataType` on macOS
  - Falls back gracefully if GPU tools unavailable
  - Selection: largest model whose `vramGb ≤ availableVram × 0.85`
- [ ] LoRA adapter panel (ThemisDB-specific, behind `copilotOllamaRouter.themisDbRules`)
  - Lists adapters from `THEMIS_MODEL_ROOT/adapters/`
  - Load/unload via Ollama modelfile API
  - Status indicator in status bar

### Phase 5 — Multi-Provider (Q4 2026 – Q1 2027)

> Goal: support any OpenAI-compatible local inference backend.

- [ ] Provider abstraction interface
  ```
  interface InferenceProvider {
    generate(opts: GenerateOptions): Promise<string>;
    health(): Promise<HealthResult>;
    listModels(): Promise<string[]>;
  }
  ```
  - `OllamaClient` implements `InferenceProvider`
  - New providers: `LmStudioClient`, `LocalAiClient`, `OpenAiCompatClient`
- [ ] Provider registry and switcher
  - Config: `copilotOllamaRouter.providers` (array of provider configs)
  - Active provider: `copilotOllamaRouter.activeProvider` (default `ollama`)
  - Command: `Copilot Ollama Router: Switch Provider`
- [ ] Remote Ollama
  - SSH tunnel: `copilotOllamaRouter.sshTunnel` (host, port, key)
  - Auth token: `copilotOllamaRouter.apiKey` (stored in `vscode.SecretStorage`)
- [ ] Provider health dashboard (webview)
  - Shows: provider name, endpoint, status, active model, latency, error rate
  - Auto-refreshes every 30 s

### Phase 6 — Distribution & Enterprise (Q1 2027)

> Goal: Marketplace-ready, team-deployable.

- [ ] VSIX packaging: `vsce package`
  - Bundle size target: ≤ 500 KB (no bundled models)
  - Icon: 128×128 PNG (`media/icon.png`)
- [ ] VS Code Marketplace listing
  - Categories: `AI`, `Other`
  - Keywords: `ollama`, `copilot`, `local-ai`, `routing`, `llm`
- [ ] Opt-in telemetry (`vscode-extension-telemetry`)
  - Events: `route.ollama`, `route.copilot`, `model.pull`, `benchmark.complete`
  - No prompt content — only: destination, model tag, latency, error type
- [ ] Team-shared routing profiles
  - Config: `.vscode/ollama-router-profiles.json` (committed to repo)
  - Format: `{ "profiles": [{ "name": "security", "rules": [...] }] }`
- [ ] Copilot Enterprise integration
  - Use `vscode.lm.selectChatModels({ vendor: "copilot", family: "gpt-4o-enterprise" })`
  - Config: `copilotOllamaRouter.copilotFamily` (default `gpt-4o`)
- [ ] VSIX signing for enterprise deployment

---

## Production Readiness Checklist

- [ ] All Phase 1–3 tasks complete
- [ ] ≥ 90 % unit test branch coverage (`npm run test`)
- [ ] 0 ESLint errors, 0 TypeScript strict errors
- [ ] Routing accuracy ≥ 88 % on 100-item labelled dataset
- [ ] Context truncation respects token budget in all code paths
- [ ] `generateWorkspaceConfig()` idempotency verified by integration test
- [ ] Extension activates in < 500 ms (measured in Extension Development Host)
- [ ] All `vscode.SecretStorage` used for API keys (no plaintext in settings)
- [ ] `CHANGELOG.md` up to date
- [ ] `README.md` matches current command IDs and config keys
- [ ] `vsce ls` shows no unexpected bundled files

---

## Known Issues & Limitations

| # | Issue | Severity | Workaround |
|---|---|---|---|
| KI-1 | Keyword routing produces false positives on ambiguous prompts (e.g. "document the security model") | Medium | Use `/local` or `/cloud` slash command to force destination |
| KI-2 | Context window: no token counting — long files may exceed model limit | Medium | Reduce `contextTokenBudget` setting |
| KI-3 | `CopilotReviewer` silently skips if no `gpt-4o` model available | Low | Check Copilot subscription |
| KI-4 | `generateWorkspaceConfig()` cannot parse JSONC with trailing commas | Low | Remove trailing commas from existing settings files |
| KI-5 | No test suite — routing regressions undetected | High | Phase 1 milestone (Q3 2026) |
| KI-6 | Inline completion not yet implemented | Info | Use `@ollama` chat participant |
| KI-7 | VRAM detection only works on NVIDIA GPUs with `nvidia-smi` | Low | Auto-selection falls back to smallest model |

---

## Breaking Changes

### v0.2.0

- **Command IDs renamed**: `ollamaBridge.*` → `copilotOllamaRouter.*`
  - Update any keybindings in `keybindings.json`.
- **Config prefix renamed**: `ollamaBridge.*` → `copilotOllamaRouter.*`
  - Existing `settings.json` entries with `ollamaBridge.*` are ignored.
  - Run `Copilot Ollama Router: Set Up / Download Coding Models` to regenerate.
- **Extension ID changed**: `themisdb.vscode-ollama-bridge` → `makr-code.copilot-ollama-router`
  - Update `.vscode/extensions.json` recommendations.
- **Publisher changed**: `themisdb` → `makr-code`
- **Directory moved**: `tools/vscode-ollama-bridge/` → `tools/copilot-ollama-router/`

### Migration

```bash
# 1. Remove old extension
code --uninstall-extension themisdb.vscode-ollama-bridge

# 2. Install new extension (after Marketplace publish or local VSIX)
code --install-extension makr-code.copilot-ollama-router

# 3. In settings.json: replace ollamaBridge.* with copilotOllamaRouter.*
#    or re-run the setup command to regenerate workspace config
```
