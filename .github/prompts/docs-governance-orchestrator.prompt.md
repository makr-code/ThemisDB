---
name: "Docs Governance Orchestrator"
description: "Create or run documentation synchronization tasks under ThemisDB Level 1-4 governance and SOT domains."
argument-hint: "Describe operation + target files (optional: module/tests/benchmark, milestone)."
agent: "doc-orchestrator"
---

Orchestrate the requested documentation task using DOCUMENTATION_GOVERNANCE.md with inference-first input handling.

Required interpretation of input:

- Operation: create, update, move, rename, delete
- Target paths or module name
- Optional scope tags: module, tests, benchmark, api, security, release
- Optional milestone override

Inference requirements:

- Infer level from target paths.
- Infer SOT domain from target paths and scope tags.
- Infer milestone from level and change type.
- Ask only one clarification if operation or target path is missing.

Execution steps:

1. Identify canonical source files for the given SOT domain.
2. Build a minimal file action plan.
3. Apply actions in upstream-first order.
4. Update references affected by move/rename/delete operations.
5. Add provenance metadata where missing.
6. Run consistency checks for cross-doc claims.

Required response:

1. Actions executed per file
2. Canonical references used
3. Consistency checks performed
4. Any blocking gaps that need docs issues
