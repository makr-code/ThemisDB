> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — AQL Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The AQL module provides LLM-assisted natural language to AQL query translation, agent-based query reasoning, and documentation assistance. Security concerns focus on: prompt injection prevention, LLM response validation before query execution, rate limiting of LLM API calls, and safe handling of schema metadata sent to external providers.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Prompt injection via user-supplied query strings | User input is inserted into structured prompt templates with clearly labeled context sections; LLM outputs are parsed as structured AQL, not executed as instructions |
| Generated AQL executing unauthorized operations | All generated AQL passes through `AQLQueryValidator` and the query module's authorization layer before execution; schema-restricted by `AQLSchemaProvider` |
| Schema exfiltration via LLM provider | `AQLSchemaProvider` sends only collection names and field types; no document content or PII is included in LLM prompts |
| LLM API key exposure in logs | LLM API keys are injected via environment variables or secrets manager; never logged or included in AQL responses |
| Unbounded LLM token consumption | `LLM metrics collector` tracks token usage per request; rate limits enforced at the auth/API layer |
| ReActAgent tool execution injection | `AgentTool` executors are registered by the application; user-supplied strings are passed as tool inputs (not executors); tool errors are captured as JSON observations |
| AQL migration assistant generating unsafe queries | Migration output always passes through `AQLQueryValidator` linting before being returned to the caller |
| Low-confidence query execution | `AQLConfidenceScorer` returns confidence score; callers are responsible for rejecting queries below threshold |

## Security Controls

### LLM Prompt Construction
- All LLM prompts use structured templates from `AQLQueryTemplateLibrary` with explicit context/instruction separation.
- User natural language input is always placed in a clearly delimited "USER INPUT" section of the prompt.
- Schema metadata sent to LLM is restricted to collection names and field types — no sample documents.

### AQL Validation Before Execution
- All generated AQL queries pass through `AQLQueryValidator` for syntax and semantic validation before being executed.
- `AQLQueryBuilder` programmatic construction does not allow arbitrary string injection — uses typed builder methods.

### Agent Framework Security
- `ReActAgent` tool registry is application-controlled; external callers cannot register new tools at runtime.
- Tool input is passed as a string parameter; the executor function is never derived from user input.
- Max iterations limit prevents infinite agent loops.
- Tool execution errors are caught and returned as observations — they never propagate as exceptions.

### Conversation Context
- Multi-turn conversation context (`AQLConversationContext`) is per-session and does not persist across user sessions by default.
- Context does not accumulate raw document content.

## Data Handling

- No document content is sent to external LLM providers; only schema metadata (collection names, field types) and user query text.
- LLM API responses are parsed and validated before being returned to callers; raw LLM output is never executed.
- Conversation history is held in memory per-session only; not persisted to disk by default.
- LoRA fine-tuning (`AQLLoraFinetuner`) operates on local model files; training data selection must comply with data governance policies.

## Known Limitations

- Confidence scoring is advisory only; the caller determines whether to execute a low-confidence generated query.
- The migration assistant may generate syntactically valid but semantically incorrect queries for complex ArangoDB-specific patterns; manual review is recommended.
- `IAsyncLLMBackend` (planned v1.8.0) will introduce async inference; thread-safety review will be required at that time.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| OpenAI API | LLM inference | API key via env/secrets; TLS connection |
| Anthropic API | LLM inference | API key via env/secrets; TLS connection |
| Azure OpenAI | LLM inference | Managed identity or API key; TLS |
| llama.cpp | Local LLM inference | Model files from trusted storage only |
