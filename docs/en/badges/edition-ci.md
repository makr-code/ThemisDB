# Edition CI Badges

| Badge | Workflow | Edition |
|-------|----------|---------|
| [![MINIMAL](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-minimal-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-minimal-ci.yml) | `03-editions/edition-minimal-ci.yml` | MINIMAL |
| [![COMMUNITY](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-community-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-community-ci.yml) | `03-editions/edition-community-ci.yml` | COMMUNITY |
| [![ENTERPRISE](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-enterprise-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-enterprise-ci.yml) | `03-editions/edition-enterprise-ci.yml` | ENTERPRISE |
| [![HYPERSCALER](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-hyperscaler-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-hyperscaler-ci.yml) | `03-editions/edition-hyperscaler-ci.yml` | HYPERSCALER |
| [![MILITARY](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-military-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/03-editions_edition-military-ci.yml) | `03-editions/edition-military-ci.yml` | MILITARY |

## What they show

Each badge reflects the result of the most recent CI run for that **ThemisDB edition** on the `develop` branch (Linux, `gcc-12`, `Debug` build). The workflows validate that the edition-specific CMake configuration is accepted and that the core test suite passes when the code is compiled with that edition flag.

All five editions share a single reusable workflow ([`03-editions/edition-build-ci.yml`](../../../.github/workflows/03-editions_edition-build-ci.yml)) that is called by each edition-specific wrapper workflow.

## CI Mode and feature flags

Because GitHub-hosted runners do not have heavyweight optional dependencies (gRPC, GPU libraries, llama.cpp), all edition CI builds pass `-DTHEMIS_CI_MODE=ON` to CMake. In CI mode the edition cmake files skip the `FORCE`-enable of heavyweight dependencies (LLM, gRPC, GPU, Tracing, Distributed Training), while `EditionMatrix` logs REQUIRED features without enforcing them — so builds succeed without installing those dependencies.

ENTERPRISE, HYPERSCALER, and MILITARY Release builds would additionally require a valid `THEMIS_LICENSE_FILE`; the CI workflows always use `Debug` builds to avoid this requirement.

## Branch Protection (required admin setup)

To enforce that all five edition checks must be green before any PR can be merged into `main`, a repository administrator must configure branch protection rules in **Settings → Branches → Branch protection rules** for the `main` branch. The required status checks to add are the job IDs of the edition builds:

| Status check name | Workflow |
|-------------------|----------|
| `MINIMAL · Linux · Debug` | `03-editions/edition-minimal-ci.yml` / `03-editions/edition-build-ci.yml` |
| `COMMUNITY · Linux · Debug` | `03-editions/edition-community-ci.yml` / `03-editions/edition-build-ci.yml` |
| `ENTERPRISE · Linux · Debug` | `03-editions/edition-enterprise-ci.yml` / `03-editions/edition-build-ci.yml` |
| `HYPERSCALER · Linux · Debug` | `03-editions/edition-hyperscaler-ci.yml` / `03-editions/edition-build-ci.yml` |
| `MILITARY · Linux · Debug` | `03-editions/edition-military-ci.yml` / `03-editions/edition-build-ci.yml` |

The job `name` in `03-editions/edition-build-ci.yml` is `${{ inputs.edition }} · Linux · ${{ inputs.build-type }}`, which renders to e.g. `MINIMAL · Linux · Debug`.

**Steps to configure (GitHub UI):**

1. Go to **Settings → Branches** in the ThemisDB repository.
2. Click **Add classic branch protection rule** (or edit the existing rule for `main`).
3. Set **Branch name pattern**: `main`.
4. Enable **Require status checks to pass before merging**.
5. Enable **Require branches to be up to date before merging**.
6. In the search box, add each of the five status check names listed above.
7. Save changes.

After this configuration, GitHub will block PRs targeting `main` unless all five edition build jobs have completed successfully.

## Source of truth

| Resource | Link |
|----------|------|
| Reusable workflow | [`.github/workflows/03-editions_edition-build-ci.yml`](../../../.github/workflows/03-editions_edition-build-ci.yml) |
| Edition CMake files | [`cmake/editions/`](../../../cmake/editions/) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions> |
