# Developer LLM Wiki Context for GitHub Copilot

Use this module when GitHub Copilot on GitHub is implementing, refactoring, reviewing, or debugging code in ThemisDB.

## Goal

- Make the curated Developer LLM Wiki a first-class context source for GitHub Copilot coding tasks.
- Reduce ad-hoc repository scanning when relevant wiki syntheses already exist.
- Keep Copilot anchored to source-of-truth precedence instead of treating wiki pages as independent truth.

## Required Read Order

1. [../../AI_WIKI_INTEGRATION_PLAYBOOK.md](../../AI_WIKI_INTEGRATION_PLAYBOOK.md)
2. [../../ai_context/developer_llm_wiki/INDEX.md](../../ai_context/developer_llm_wiki/INDEX.md)
3. One or more task-specific wiki pages:
   - [../../ai_context/developer_llm_wiki/MODULES_AND_APIS.md](../../ai_context/developer_llm_wiki/MODULES_AND_APIS.md)
   - [../../ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md](../../ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md)
   - [../../ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md](../../ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md)
4. [../../ai_context/developer_llm_wiki/WIKI_STATUS.json](../../ai_context/developer_llm_wiki/WIKI_STATUS.json) as freshness/sync evidence

## File Selection Rules

### Module and API work
- Start with `MODULES_AND_APIS.md`
- Cross-check with module-local `src/<module>/ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `README.md`

### Build, test, CI, and operations work
- Start with `BUILD_TEST_CI_AND_OPERATIONS.md`
- Cross-check with `.github/workflows/*.yml`, `scripts/*`, and root build docs

### Governance, roadmap, release, and process work
- Start with `GOVERNANCE_AND_ROADMAP.md`
- Cross-check with `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md`

### General onboarding or multi-area tasks
- Start with `INDEX.md`
- Follow its links before falling back to broad repository search

## Freshness and Trust Rules

- Check `WIKI_STATUS.json` for:
  - `generated_at`
  - `source_count`
  - delta metadata (`added_count`, `removed_count`, `changed_count`)
- Treat the Developer LLM Wiki as compiled guidance, not primary truth.
- If root SOT, module docs, tests, or code disagree with the wiki, prefer the primary source and report wiki drift.
- If the needed wiki page is missing, stale, or irrelevant, state that explicitly in the task output.

## GitHub Copilot Behavior Expectations

- On GitHub coding tasks, cite the consulted wiki page(s) when they materially informed the implementation.
- Do not claim the wiki was used unless the referenced files were actually consulted.
- Use the wiki to narrow search space, identify likely files, and recover module/build/governance context faster.
- Still verify behavior against code, tests, and workflows before making changes.
