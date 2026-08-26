# ThemisDB AI-Guardrails and Development Setup

This directory contains modular Copilot instructions and development tooling for ThemisDB.

## Directory Structure

```text
.github/
├── copilot-instructions.md          # Main entry point
├── copilot/                         # Modular instruction files
│   ├── BRANCHING_GUIDE.md
│   ├── BUILD_GUIDE.md
│   ├── BUILD_PERF_AGENT.md
│   ├── CODE_STANDARDS.md
│   ├── CUDA_OPTIMIZATION.md
│   ├── CROSS_COMPILATION_CONTEXT.md
│   ├── MODERNIZE_AGENT.md
│   ├── MVCC_CONCURRENCY.md
│   ├── PERFORMANCE_PROFILING.md
│   ├── TESTING_GUIDE.md
│   ├── VSCODE_CONTEXT.md
│   └── PROMPT_ENGINEERING.md
├── scripts/
│   └── validate_copilot_refs.py     # Validate file references
└── workflows/
    └── validate-ai-guardrails.yml   # CI validation workflow
```

## Purpose

The modular AI-Guardrails architecture provides:

1. **Predictable Copilot behavior**: Clear, focused instructions per domain.
2. **Maintainability**: Easier updates to individual modules without conflicts.
3. **Discoverability**: Logical organization by topic.
4. **Validation**: Automated checks for broken references and structure drift.

## Installation

No installation is required beyond cloning the repository. To apply the recommended developer setup locally:

```bash
cp -r .vscode.example .vscode
```

## Quick Start

### For Contributors

1. Read the main instructions in [../copilot-instructions.md](../copilot-instructions.md).
2. Read the team workflow in [AI_CUSTOMIZATION_WORKFLOW.md](AI_CUSTOMIZATION_WORKFLOW.md).
3. Follow links into the relevant module guides.
4. Use [VSCODE_CONTEXT.md](VSCODE_CONTEXT.md) for local environment setup.
5. Use [AI_WIKI_CONTEXT.md](AI_WIKI_CONTEXT.md) when GitHub Copilot on GitHub should ground coding work in the Developer LLM Wiki.

## AI Workflow Quick Links

- Team workflow playbook: [AI_CUSTOMIZATION_WORKFLOW.md](AI_CUSTOMIZATION_WORKFLOW.md)
- PR AI report template: [PR_AI_REPORT_TEMPLATE.md](PR_AI_REPORT_TEMPLATE.md)
- Review severity policy: [REVIEW_SEVERITY_POLICY.md](REVIEW_SEVERITY_POLICY.md)
- Developer wiki context for GitHub Copilot: [AI_WIKI_CONTEXT.md](AI_WIKI_CONTEXT.md)
- Agents: [../agents/themisdb-implementer.agent.md](../agents/themisdb-implementer.agent.md), [../agents/themisdb-reviewer.agent.md](../agents/themisdb-reviewer.agent.md)
- Prompts: [../prompts/roadmap-to-production.prompt.md](../prompts/roadmap-to-production.prompt.md), [../prompts/build-triage-windows-release.prompt.md](../prompts/build-triage-windows-release.prompt.md), [../prompts/pr-diff-findings-review.prompt.md](../prompts/pr-diff-findings-review.prompt.md), [../prompts/security-hardening-review.prompt.md](../prompts/security-hardening-review.prompt.md), [../prompts/api-change-impact-review.prompt.md](../prompts/api-change-impact-review.prompt.md), [../prompts/release-readiness-check.prompt.md](../prompts/release-readiness-check.prompt.md), [../prompts/compose-ai-pr-report.prompt.md](../prompts/compose-ai-pr-report.prompt.md), [../prompts/verify-high-exception-record.prompt.md](../prompts/verify-high-exception-record.prompt.md)

### For Maintainers

1. Validate changes:

   ```bash
   python .github/scripts/validate_copilot_refs.py
   ```

2. Update the affected module file, for example `copilot/BUILD_GUIDE.md`.
3. Keep the main file compact and focused.

## Usage

- Read `../copilot-instructions.md` first.
- Use the module docs in this directory for task-specific guidance.
- Re-run `.github/scripts/validate_copilot_refs.py` after editing these docs.

## Module Overview

### BRANCHING_GUIDE.md

- Git Flow branching strategy.
- PR creation workflow.
- Merge strategies.
- Branch naming conventions.

### BUILD_GUIDE.md

- CMake presets for supported platforms.
- vcpkg offline-first architecture.
- Quick start commands for Windows, Linux, Docker, and ARM.
- Edition-specific build paths.
- Troubleshooting guidance.

### CODE_STANDARDS.md

- C++ coding style for C++17 and C++20.
- Naming conventions.
- Code quality tools.
- Thread-safety patterns.
- Error-handling guidelines.

### TESTING_GUIDE.md

- Google Test usage.
- Running all, targeted, and critical tests.
- Unit, integration, and benchmark categories.
- Coverage targets.
- Mock and fixture patterns.

### CROSS_COMPILATION_CONTEXT.md

- Platform support matrix.
- Platform-specific compiler flags.
- SIMD optimization notes for x86 and ARM.
- vcpkg triplet configuration.
- Docker multi-architecture builds.

### CUDA_OPTIMIZATION.md

- GPU architecture basics.
- Memory hierarchy and access patterns.
- Kernel design guidelines.
- SIMD fallback strategy for x86 and ARM.
- GPU atomics and lock-free patterns.
- Annotated examples.
- Common pitfalls.

### MVCC_CONCURRENCY.md

- MVCC fundamentals.
- Lock strategies.
- Thread-safety patterns.
- Deadlock prevention.
- Concurrency testing.
- Transaction lifecycle.
- Version garbage collection and common pitfalls.

### PERFORMANCE_PROFILING.md

- GPU and CPU profiling workflows.
- Key performance metrics and thresholds.
- Benchmark best practices.
- Google Benchmark integration.
- Regression detection in CI.
- ThemisDB-specific performance patterns.

### VSCODE_CONTEXT.md

- VS Code setup and extensions.
- Remote development options.
- CMake integration.
- IntelliSense configuration.
- Debugging workflows.
- Formatting support.

### AI_WIKI_CONTEXT.md

- Required Developer LLM Wiki read order for GitHub Copilot on GitHub.
- Which curated wiki files to use for module/API, CI/build, governance, and C/C++ tasks.
- Finer API routing for transport, storage/index, auth/identity, LLM/model, and transaction/consistency work.
- Which C++ workflow profile to use for public API, internal core, concurrency/performance, and plugin-boundary work.
- Freshness checks via `WIKI_STATUS.json`.
- Source-of-truth fallback rules when wiki synthesis drifts.

### PROMPT_ENGINEERING.md

- Step-by-step decomposition for implementation prompts.
- Acceptance-criteria-first prompt structure.
- Checkpoint strategy for long-running autonomous tasks.
- Canonical prompt patterns for concurrency/networking/rate-limiting tasks.

### MODERNIZE_AGENT.md

- Assessment-plan-execution structure for C++ modernization tasks.
- Trigger scenarios for legacy toolchains and C++11/14 migration.
- Validation and staging requirements for safe modernization.

### BUILD_PERF_AGENT.md

- ETL trace-based build bottleneck analysis.
- Build-time hotspot categories (headers, templates, function generation).
- Existing CMake-target integration and measurement-first workflow.

## Validation

### Automated Checks

The CI pipeline validates:

- All file references exist.
- The main file stays under 300 lines.
- Required modules are present.
- Markdown linting runs on the relevant instruction files.

### Manual Validation

```bash
python .github/scripts/validate_copilot_refs.py
wc -l .github/copilot-instructions.md
```

## Editing Guidelines

### When to Create a New Module

Create a new module when:

- The topic is self-contained and large enough to justify isolation.
- Multiple sections belong to the same domain.
- The content will be referenced frequently.
- The split reduces complexity in the main file.

### When to Update an Existing Module

Update a module when:

- Instructions change for that domain.
- New tools or processes are introduced.
- New troubleshooting knowledge is discovered.
- Examples need clarification.

### Main File Expectations

The main `copilot-instructions.md` file should:

- Provide a high-level overview.
- Link to detailed modules.
- Offer quick reference examples.
- Stay under 300 lines, with a target around 200.

### Style Guidelines

1. Use clear headers.
2. Prefer short, concrete examples.
3. Use tables only when comparison adds value.
4. Keep links current.
5. Use visual flourishes sparingly.

## Update Process

1. Edit the relevant module or main file.
2. Run the validation script.
3. Verify cross-references.
4. Commit with a conventional message.
5. Confirm CI checks pass.

## Tooling

### Validation Script

` .github/scripts/validate_copilot_refs.py ` checks markdown links in Copilot instruction files, validates file existence, and reports broken references.

### CI Workflow

` .github/workflows/validate-ai-guardrails.yml ` validates references, checks file size, lints markdown, and verifies module structure.

### Pre-commit Hooks

` .pre-commit-config.yaml ` can be used to run markdown linting, YAML validation, secret detection, and custom validation hooks before commit.

## Metrics

### Before Refactoring

- Single file with 501 lines.
- Difficult to navigate.
- No validation.
- Conflicting instructions.

### After Refactoring

- Main file reduced to 264 lines.
- Nine focused modules.
- Automated validation.
- Clearer organization.

## Contributing

When contributing to AI-Guardrails:

1. Keep modules focused.
2. Run the validation script.
3. Maintain cross-references.
4. Update this README when structure changes.

## Related Documentation

- [CONTRIBUTING.md](../../CONTRIBUTING.md): Contribution guidelines.
- [ARCHITECTURE.md](../../ARCHITECTURE.md): System architecture.
- [.devcontainer/](../../.devcontainer/devcontainer.json): Dev Container setup.
- [.pre-commit-config.yaml](../../.pre-commit-config.yaml): Pre-commit hooks.

## FAQ

### Why split into modules?

The previous monolithic file was hard to maintain, difficult to navigate, and impossible to validate reliably. The modular structure provides better separation of concerns, simpler maintenance, clearer organization, and automatic validation.

### How often should modules be updated?

Update modules when build processes change, coding standards evolve, new tools are adopted, or better practices are discovered.

### Can I add new modules?

Yes. Use this sequence:

1. Create the module in `.github/copilot/`.
2. Add the reference in the main file.
3. Extend validation if required.
4. Document the new module in this README.

### What if validation fails?

Common causes are:

- Broken links that need to be updated.
- An oversized main file that should be split.
- Missing required modules.

## Support

For questions or issues:

- Open an issue with label `area:documentation`.
- Consult [CONTRIBUTING.md](../../CONTRIBUTING.md).
- Use the team discussion channels.

---

Version: 1.0
Last Updated: 2026-04-14
Maintainer: ThemisDB Team
