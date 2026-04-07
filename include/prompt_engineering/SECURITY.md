<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Prompt Engineering Module

## Scope

Covers all public headers in `include/prompt_engineering/`. Implementation hardening in `../../src/prompt_engineering/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Prompt injection via user input | High — LLM manipulation / data exfiltration | `PromptInjectionDetector` screens all user-supplied input; blocks jailbreak, role-override, and indirect injection patterns |
| Token budget exhaustion (DoS) | Medium — cost amplification | `ContextWindowBudgetManager` raises `PromptBudgetExceededError` before LLM API call |
| Reflection loop infinite cost | High — unbounded LLM spend | `ReflectionTuner` enforces max iteration count and total token budget per reflection cycle |
| Prompt template injection (stored) | Medium — malicious template execution | `PromptTemplateValidator` schema-validates templates before storage; template variables are sandboxed |
| Version control history exposure | Low — prompt strategy leakage | `PromptVersionControl` enforces read-permission checks on history and diff operations |
| A/B test result manipulation | Low — optimizer poisoning | `PromptOptimizer` uses cryptographically seeded random assignment; results stored immutably |
| System prompt override via user message | High — persona hijacking | `SystemPromptManager` injects system prompt with separator tokens; `PromptInjectionDetector` verifies no override in user turn |

## Security Controls

1. **Injection screening** — `PromptInjectionDetector` validates all user input before prompt assembly.
2. **Token budget hard limit** — `ContextWindowBudgetManager` enforces `max_tokens` per model; excess raises `PromptBudgetExceededError`.
3. **Reflection iteration cap** — `ReflectionTuner` enforces `max_iterations` (default: 5) and `max_total_tokens` per cycle.
4. **Template sandboxing** — template variables are rendered in a restricted Jinja2-equivalent context; no arbitrary code execution.
5. **RBAC on version history** — `PromptVersionControl` checks read role before exposing prompt history.
6. **System prompt separator** — `SystemPromptManager` uses model-specific separator tokens to prevent user-turn override.

## Known Limitations

- `PromptInjectionDetector` uses heuristic + embedding-similarity detection; novel jailbreak patterns may evade detection until next model update.
- Multi-modal injection (image-embedded instructions) is not yet covered — tracked for Q4 2026.
