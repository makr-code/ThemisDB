# Copilot ↔ Ollama Router

A VS Code extension that routes Copilot chat requests to a **local Ollama model** for
cost-effective, low-latency code generation while keeping Copilot as the orchestrator
for security reviews, architecture decisions, and complex debugging.

---

## Features

| Task | Destination |
|---|---|
| Boilerplate generation | 🖥️ Local Ollama |
| Test/spec writing | 🖥️ Local Ollama |
| Refactoring | 🖥️ Local Ollama |
| Documentation | 🖥️ Local Ollama |
| Security review | ☁️ Copilot (cloud) |
| Architecture decision | ☁️ Copilot (cloud) |
| Complex debugging | ☁️ Copilot (cloud) |

**ThemisDB-specific rules** (enabled by default):

- C++ code generation → always Ollama (`codellama`)
- Security / audit prompts → always Copilot cloud

---

## Requirements

- VS Code ≥ 1.90
- [Ollama](https://ollama.com) running locally (`ollama serve`)
- At least one Ollama model pulled, e.g. `ollama pull codellama:13b`
- GitHub Copilot extension (for cloud fallback and optional review pass)

---

## Quick Start

```bash
# 1. Install dependencies
cd tools/copilot-ollama-router
npm install

# 2. Compile
npm run compile

# 3. Press F5 in VS Code to launch the Extension Development Host
```

Then in the chat panel:

```
@ollama Write unit tests for the selected function
@ollama /local Implement a move constructor for this class
@ollama /cloud Review the security implications of this code
```

---

## Configuration

| Setting | Default | Description |
|---|---|---|
| `copilotOllamaRouter.endpoint` | `http://localhost:11434` | Ollama API base URL |
| `copilotOllamaRouter.defaultModel` | `codellama:13b` | Model for code tasks |
| `copilotOllamaRouter.reasoningModel` | `llama3` | Model for docs/reasoning |
| `copilotOllamaRouter.delegationMode` | `auto` | `auto` \| `always` \| `never` |
| `copilotOllamaRouter.copilotReviewEnabled` | `true` | Run Copilot quality-check after Ollama |
| `copilotOllamaRouter.requestTimeoutMs` | `60000` | Ollama request timeout (ms) |
| `copilotOllamaRouter.themisDbRules` | `true` | ThemisDB-specific routing overrides |

---

## Commands

| Command | Description |
|---|---|
| `Copilot Ollama Router: Delegate to Ollama` | Send a prompt directly to local Ollama |
| `Copilot Ollama Router: Ask Copilot (Cloud)` | Open Copilot chat |
| `Copilot Ollama Router: Auto-Route` | Classify a prompt and show routing decision |
| `Copilot Ollama Router: Check Ollama Connection` | Verify Ollama is reachable |

---

## Architecture

```
User prompt
     │
     ▼
ContextManager          ← enriches prompt with active file, language,
     │                    selection, and diagnostics
     ▼
DelegationRouter        ← keyword-based classifier
     │
     ├─ destination=ollama ──► OllamaClient.generate() (streaming)
     │                              │
     │                              └─ (optional) CopilotReviewer.review()
     │
     └─ destination=copilot ─► vscode.lm (Copilot Language Model API)
```

---

## Development

```bash
npm run compile    # one-shot TypeScript build
npm run watch      # watch mode
npm run lint       # ESLint
```

---

## License

MIT — see [LICENSE](../../LICENSE).
