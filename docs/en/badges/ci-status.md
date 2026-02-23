# CI Status Badge

[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/validate-ai-guardrails.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/validate-ai-guardrails.yml)

## What it shows

The result of the most recent run of the primary CI workflow on the `develop` branch. A green **passing** badge means the last automated run completed without errors. A red **failing** badge means at least one required check failed.

## What it does NOT guarantee

- A passing badge does **not** mean the full test suite passed – only the workflows configured in GitHub Actions have run.
- Badges are cached by GitHub/shields.io and may lag behind the actual workflow state by a few minutes.
- A passing badge on `develop` does not imply the same state on a feature branch or a release candidate.

## Source of truth

| Source | URL |
|--------|-----|
| Workflow file | [`.github/workflows/validate-ai-guardrails.yml`](../../../.github/workflows/validate-ai-guardrails.yml) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions> |

## How contributors can verify

1. Go to the [Actions tab](https://github.com/makr-code/ThemisDB/actions) on GitHub.
2. Select the **validate-ai-guardrails** workflow.
3. Review the most recent run on the target branch for detailed step-by-step logs.
