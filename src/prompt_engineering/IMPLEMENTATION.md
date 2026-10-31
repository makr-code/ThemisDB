---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering Implementation Notes

## Scope and ownership

The public contracts are under [`include/prompt_engineering/`](../../include/prompt_engineering/); implementation lives under `src/prompt_engineering/`; focused module tests live under [`tests/prompt_engineering/`](../../tests/prompt_engineering/). The module owns prompt templates, version history, prompt-quality workflows, feedback, and rewrite-rule execution. It does not own model serving or transport.

## Template lifecycle

`PromptManager` can be constructed in memory or with non-owning `RocksDBWrapper*` and optional column-family handles. Keep the database and handle alive for the manager's lifetime. Its concurrent map stores templates in memory. When a database is configured, reads and listing query RocksDB; writes persist JSON under the module's prompt-template key prefix.

Before creating or loading a template, `PromptManager::validateTemplate()` requires non-empty name, version, and content; image entries need non-empty alt text; metadata must be an object or null. An empty description is a warning. A validation failure from `createTemplate()` returns a default template with an empty ID rather than throwing.

Do not conflate this field validation with `PromptTemplateValidator`: its JSON-string overload parses JSON and scans content for a limited set of injection patterns, turning any detected warning into an invalid result. The manager's struct validation does not perform that scan. Neither validator substitutes for caller-owned context authorization or output policy.

`injectContext()` replaces literal `{key}` occurrences for keys present in the supplied map. It does not report missing placeholders, escape replacement values, or recursively expand newly inserted placeholders. `getPromptWithContext()` returns no value for an absent or inactive template; callers must decide how to handle that result before model dispatch.

Persistence errors from template create/update are logged, but the methods can still report success from the in-memory operation. Confirm durability through storage-level checks when persistence is an acceptance requirement.

## Versioning, evaluation, and feedback

- `PromptVersionControl::commit()` records the prompt ID, content, parent version, author, message, branch, timestamp, and metadata; it can also persist through a non-owning RocksDB wrapper. Rollback, branching, diff, and merge operate on this version history.
- `PromptOptimizer` uses caller-supplied evaluation and improvement functions, with an iteration limit, target score, and minimum-improvement threshold in `OptimizationConfig`.
- `FeedbackCollector` clamps recorded severity to `[0,1]`, computes an FNV-1a 64-bit audit checksum, and retains query/response text in entries. With a database configured, those entries are persisted; the checksum is not a cryptographic integrity guarantee.
- `PromptEngineeringMetrics` exports text metrics and supports JSON snapshot/restore. Callback alerting runs on the thread that records the triggering metric.

Use the contracts in [`API.md`](./API.md); consult [`TRUST.md`](./TRUST.md) before persisting or exporting prompt data.

## Rewrite engine

Create an engine with `create_rewrite_engine()`. Rules are registered by unique ID; null and duplicate registrations are rejected. Full rewrites run four phases in order and sort rules by priority, keeping registration order for ties. Each phase repeats while a transformation occurs, up to `RewriteContext::max_steps` (zero selects the implementation default of 1000). A terminal block or rule error stops processing. Trace collection is bounded by `max_trace_entries`.

Only regex and dictionary rules are accepted from YAML. Validate the candidate configuration and exercise it in tests before deploying it. Loading is all-or-nothing at the loader boundary; the engine skips IDs already registered rather than replacing their definitions. Policy and semantic rules must be registered as C++ implementations.

The core is compiled into `themis_core`, but the integration façade does not currently invoke it automatically. Pipeline wiring and behavior-level rule regression coverage remain open in [`ROADMAP.md`](./ROADMAP.md); check the actual caller before assuming a prompt is rewritten.

## Implementation and test workflow

1. Update the public header contract first for interface changes, then keep implementation and module docs aligned.
2. Add focused tests for success, invalid input, missing state, and the relevant concurrency or failure boundary.
3. Run the focused test target and performance cases mapped in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
4. Keep changes to persisted JSON, version behavior, or rule configuration backward compatible; document migration requirements before changing those formats.

The test target and CTest labels are registered in [`tests/prompt_engineering/CMakeLists.txt`](../../tests/prompt_engineering/CMakeLists.txt). See [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for reproducible commands.

Exact model backend not verifiable from final document text alone.
