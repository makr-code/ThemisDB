# CLAUDE.md

Project: PROJECT_NAME

## Core workflow

1. Explore
- Read headers, architecture docs, and roadmap sections first.
- Use semantic symbol navigation for C/C++ impact checks.

2. Plan
- Write a short plan in `ai_working/plan.md` for non-trivial changes.
- Include files, acceptance criteria, and test scope.

3. Execute
- Apply minimal safe edits.
- Keep interfaces const-correct and ownership-safe.
- Update API docs for all changed public behavior.

4. Commit and PR
- Explain what changed and why.
- Open AI-generated PRs as Draft by default.
- Require human review before merge.

## Build, test, docs

- Configure: `cmake --preset windows-release`
- Build: `cmake --build --preset windows-release --parallel 4`
- Test: `ctest --preset windows-release --output-on-failure -j 1 --timeout 60`
- Docs: `doxygen Doxyfile_xml`

## Rules

- Prefer RAII and avoid raw pointers in public APIs.
- Prefer simple, readable code over clever abstractions.
- Use symbol-first refactoring for signature and rename changes.
