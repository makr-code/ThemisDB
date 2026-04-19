> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md (root) -->

# Security Policy — Prompt Engineering Module

## Supported Versions

| Version | Security Fixes |
|---------|---------------|
| 2.0.x   | ✅ Active      |
| 1.4.x   | ✅ Active      |
| < 1.4   | ❌ EOL         |

## Threat Model

### T1 — Prompt Injection Attacks
- **Risk:** High — malicious user input could hijack LLM instructions
- **Mitigation:** `PromptInjectionDetector` with 10 built-in patterns, keyword/syntax scoring, and `sanitize()` method applied before template rendering
- **Residual risk:** Low for known patterns; novel jailbreak patterns require detector updates

### T2 — Template Variable Injection
- **Risk:** Medium — `{placeholder}` substitution could be abused if values are evaluated
- **Mitigation:** Substitution is purely structural string replacement; no expression evaluation occurs
- **Residual risk:** Negligible — no code execution path from placeholder values

### T3 — Version Control Authorization
- **Risk:** Medium — unauthorized users modifying prompt versions or rolling back to vulnerable templates
- **Mitigation:** RBAC enforced at the server layer; version control operations require authenticated sessions
- **Residual risk:** Low — authorization is not enforced inside the module itself; depends on server layer

### T4 — Feedback Poisoning
- **Risk:** Medium — crafted feedback entries could skew evaluator scores and optimizer decisions
- **Mitigation:** FNV-1a audit checksum on every `FeedbackCollector` entry; tampered entries are detectable
- **Residual risk:** Low — checksums detect tampering; bulk injection from authenticated users is an operational risk

### T5 — A/B Test Manipulation
- **Risk:** Low — inflating sample counts or skewing distributions to force false significance
- **Mitigation:** erfc-based normal CDF p-value gates enforce statistical significance before accepting A/B results
- **Residual risk:** Negligible under honest usage; adversarial data injection falls under T4

## Known Limitations

| ID    | Description                                                              | Target Fix |
|-------|--------------------------------------------------------------------------|------------|
| KL-01 | `ContextWindowBudgetManager` enforces a budget cap but does not use model-specific BPE tokenization; the `CharDivisionCounter` is an approximation | Planned    |
| KL-02 | Injection detector covers 10 known patterns; novel patterns require updates | Ongoing |

## Reporting a Vulnerability

Report via the project's private security disclosure channel (see root `SECURITY.md`).
Do **not** open public issues for security vulnerabilities.
