# CI Status Badge

[![CI develop](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml?query=branch%3Adevelop)
[![CI community](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg?branch=community)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml?query=branch%3Acommunity)

## What it shows

The result of the most recent run of the **Themis Core Framework CI** workflow on the `develop` or `community` branch. This workflow builds and tests the core framework sources (storage engine, network layer, module loader, license validation) on every push to `develop` / `community`.

## What it does NOT guarantee

- A passing badge does **not** mean the full test suite passed – only the workflows configured in GitHub Actions have run.
- Badges are cached by GitHub/shields.io and may lag behind the actual workflow state by a few minutes.
- A passing badge on `develop` does not imply the same state on a feature branch or a release candidate.

## Source of truth

| Source | URL |
|--------|-----|
| Workflow file | [`.github/workflows/01-core_themis-core-ci.yml`](../../../.github/workflows/01-core_themis-core-ci.yml) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions> |

## How contributors can verify

1. Go to the [Actions tab](https://github.com/makr-code/ThemisDB/actions) on GitHub.
2. Select the **Themis Core Framework CI** workflow.
3. Review the most recent run on the target branch for detailed step-by-step logs.

## Migration note

The historical branch name `main` is no longer the canonical Community release lane.
Use `community` for new release-lane references and checks.
