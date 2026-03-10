# Edition CI Badges

| Badge | Workflow | Edition |
|-------|----------|---------|
| [![MINIMAL](https://github.com/makr-code/ThemisDB/actions/workflows/edition-minimal-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/edition-minimal-ci.yml) | `edition-minimal-ci.yml` | MINIMAL |
| [![COMMUNITY](https://github.com/makr-code/ThemisDB/actions/workflows/edition-community-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/edition-community-ci.yml) | `edition-community-ci.yml` | COMMUNITY |
| [![ENTERPRISE](https://github.com/makr-code/ThemisDB/actions/workflows/edition-enterprise-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/edition-enterprise-ci.yml) | `edition-enterprise-ci.yml` | ENTERPRISE |
| [![HYPERSCALER](https://github.com/makr-code/ThemisDB/actions/workflows/edition-hyperscaler-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/edition-hyperscaler-ci.yml) | `edition-hyperscaler-ci.yml` | HYPERSCALER |
| [![MILITARY](https://github.com/makr-code/ThemisDB/actions/workflows/edition-military-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/edition-military-ci.yml) | `edition-military-ci.yml` | MILITARY |

## What they show

Each badge reflects the result of the most recent CI run for that **ThemisDB edition** on the `develop` branch (Linux, `gcc-12`, `Debug` build). The workflows validate that the edition-specific CMake configuration is accepted and that the core test suite passes when the code is compiled with that edition flag.

All five editions share a single reusable workflow ([`edition-build-ci.yml`](../../../.github/workflows/edition-build-ci.yml)) that is called by each edition-specific wrapper workflow.

## CI Mode and feature flags

Because GitHub-hosted runners do not have heavyweight optional dependencies (gRPC, GPU libraries, llama.cpp), all edition CI builds pass `-DTHEMIS_CI_MODE=ON` to CMake. In CI mode the `EditionMatrix` validation logs REQUIRED features but does not force-enable them, so builds succeed without installing those dependencies.

ENTERPRISE, HYPERSCALER, and MILITARY Release builds would additionally require a valid `THEMIS_LICENSE_FILE`; the CI workflows always use `Debug` builds to avoid this requirement.

## Source of truth

| Resource | Link |
|----------|------|
| Reusable workflow | [`.github/workflows/edition-build-ci.yml`](../../../.github/workflows/edition-build-ci.yml) |
| Edition CMake files | [`cmake/editions/`](../../../cmake/editions/) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions> |
