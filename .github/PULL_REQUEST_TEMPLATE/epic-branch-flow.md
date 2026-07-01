# Epic Branch Flow — Pull Request

<!-- Use this template for all PRs targeting an EPIC branch (e.g. epic/gap-wave1-5475). -->
<!-- Standard feature PRs should use the default pull_request_template.md instead.      -->

## Description

<!-- Describe the changes in this PR and how they contribute to the epic. -->

## Linked Epic & Issues

<!-- Mandatory: Reference the parent epic and any sub-issues addressed by this PR. -->
- **Epic:** <!-- e.g. #5475 epic/gap-wave1-5475 -->
- **Sub-issues:** <!-- e.g. Closes #5482, Relates to #5483 -->

## Type of Change

- [ ] Bug fix (`fix/5475-<kurzname>`)
- [ ] Feature implementation (`feature/5475-<kurzname>`)
- [ ] Infra / CI / Documentation (`chore/5475-<kurzname>`)
- [ ] Refactoring (non-breaking)
- [ ] Other:

## Merge Order (Mandatory)

> PRs within an epic **must** follow the two-step merge order:
>
> 1. Feature/fix/chore branch → EPIC branch (this PR)
> 2. Periodic integration PR: EPIC branch → `develop`
>
> **Direct merges from feature branches to `develop` are prohibited** for this epic scope.

- [ ] This PR targets the EPIC branch, **not** `develop` directly.
- [ ] I have confirmed the epic integration PR cadence with the maintainer.

## Epic PR Checklist (Mandatory)

- [ ] Issue reference in PR title or description (`#<epic>` + sub-issue number)
- [ ] Build/test evidence attached (commands + outcomes)
- [ ] Risk and rollback note documented below
- [ ] No direct merge from this branch to `develop`

## Build & Test Evidence

<!-- Attach or paste build and test results relevant to this PR. -->

```
# Example:
# cmake --build --preset windows-release --parallel 16
# ctest --preset windows-release --output-on-failure -j 1 --timeout 60
```

- Build result:
- Test result:
- Scanner delta (baseline vs. current):

## Risk & Rollback Note

<!-- Describe any risks introduced by this PR and how to revert if needed. -->

- **Risk:**
- **Rollback:** <!-- e.g. revert commit SHA, feature flag, or re-apply previous artifact -->

## AI-Generated Code (if applicable)

<!-- Tool reference: see `.github/instructions/cpp-language-service-tools.instructions.md` -->

- [ ] Symbol references checked with `GetSymbolReferences_CppTools`
- [ ] No raw pointers or `new`/`delete` without explicit review justification
- [ ] RAII and exception-safety verified for new/changed paths
- [ ] No unnecessary AI-generated abstractions introduced
- [ ] Performance metrics reviewed if a hot path is affected

## Gap Scanner Gates

- [ ] No new `critical` findings in categories: `security`, `input_validation`, `query_correctness`, `distributed_consistency`, `concurrency`, `memory`
- [ ] No new `high` findings in the same categories (or explicitly approved with rationale)
- [ ] Gap Scanner delta report attached (baseline vs. current), not only totals

## Security & Quality Gates

- [ ] IntelliSense/Compiler: no new errors in changed files
- [ ] clang-tidy / cppcheck: no new high-risk findings in changed files
- [ ] No new warnings introduced
- [ ] Documentation updated for any changed public C++ API (Doxygen comments)
- [ ] CHANGELOG.md updated under `[Unreleased]` (if user-visible behavior changes)

## Residual Risks & Follow-ups

- Risk:
- Follow-up action / tracking issue:
