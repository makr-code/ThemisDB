# ThemisDB Release Strategy

> **Version:** 1.0 · **Status:** Active  
> This document is the single source of truth for the ThemisDB release model,
> branch responsibilities, edition matrix, PR policies, and release process.

---

## Table of Contents

1. [Branch Model](#1-branch-model)
2. [Edition Matrix](#2-edition-matrix)
3. [Path Policies (PR Rules)](#3-path-policies-pr-rules)
4. [Release Process](#4-release-process)
5. [CI/CD Pipeline Architecture](#5-cicd-pipeline-architecture)
6. [Private Publishing via Environments & Secrets](#6-private-publishing-via-environments--secrets)
7. [Rollback Plan](#7-rollback-plan)
8. [Onboarding: Local Builds per Edition](#8-onboarding-local-builds-per-edition)
9. [Branch Creation (One-Time Setup)](#9-branch-creation-one-time-setup)
10. [Migration Guide (Docker Path Reorganisation)](#10-migration-guide-docker-path-reorganisation)
11. [Known Limitations](#11-known-limitations)

---

## 1. Branch Model

ThemisDB uses **four permanent (long-lived) branches** plus optional short-lived
release branches:

```
develop          ← Single Source of Truth (all development)
   │
   ├── main           ← Community release lane    (public, v*)
   ├── enterprise     ← Enterprise release lane   (commercial, enterprise-v*)
   └── hyperscaler    ← Hyperscaler release lane  (OEM/cloud, hyperscaler-v*)
```

### Branch responsibilities

| Branch        | Role                    | Source of Truth | Who merges     | Tags            |
|---------------|-------------------------|-----------------|----------------|-----------------|
| `develop`     | Integration branch      | **Yes**         | All devs (PR)  | none            |
| `main`        | Community release lane  | No              | Release Manager| `v*`            |
| `enterprise`  | Enterprise release lane | No              | Release Manager| `enterprise-v*` |
| `hyperscaler` | Hyperscaler release lane| No              | Release Manager| `hyperscaler-v*`|

### Key rules

- **`develop` is the Single Source of Truth.** All feature work, bugfixes, and
  integration happens in `develop` (or short-lived feature branches from `develop`).
- **Release lanes (`main`, `enterprise`, `hyperscaler`) are NOT development
  branches.** They receive content via deliberate PRs from `develop` (or from
  short-lived `release/*` branches). Never commit directly to them.
- **Military Edition** has no dedicated branch. It is built as a build-profile
  variant of the `enterprise` lane (same branch, different CMake flags).
- **Sourcecode is always public.** Only build/binary/publish artefacts for
  Enterprise and Hyperscaler are kept private via CI environment gates.

---

## 2. Edition Matrix

ThemisDB supports five editions, built as CMake feature compositions:

| Edition        | Branch       | Build Flag                      | Docker               | Helm | Operator |
|----------------|--------------|---------------------------------|----------------------|------|----------|
| `MINIMAL`      | `main`       | `-DTHEMIS_EDITION=MINIMAL`      | —                    | —    | —        |
| `COMMUNITY`    | `main`       | `-DTHEMIS_EDITION=COMMUNITY`    | `docker/community/`  | —    | —        |
| `ENTERPRISE`   | `enterprise` | `-DTHEMIS_EDITION=ENTERPRISE`   | `docker/enterprise/` | —    | —        |
| `MILITARY`     | `enterprise` | `-DTHEMIS_EDITION=MILITARY`     | `docker/enterprise/` | —    | —        |
| `HYPERSCALER`  | `hyperscaler`| `-DTHEMIS_EDITION=HYPERSCALER`  | `docker/hyperscaler/`| `helm/`| `operator/`|

> **Military** is Variant A (hardening/defaults/feature-composition only).
> It uses the `enterprise` branch and is produced by the same CI jobs as Enterprise.

### Feature flag hierarchy

```
MINIMAL  ⊂  COMMUNITY  ⊂  ENTERPRISE  ⊂  HYPERSCALER
                            MILITARY   ⊂  ENTERPRISE (hardened profile)
```

Each higher edition includes all features of the editions below it.

---

## 3. Path Policies (PR Rules)

### 3.1 PR → `main` (Community lane)

**Enforced by:** `.github/workflows/pr-path-gate-main.yml`

The following paths are **blocked** in PRs targeting `main`:

| Blocked path prefix       | Reason                                        |
|---------------------------|-----------------------------------------------|
| `docker/hyperscaler/**`   | Hyperscaler Docker images                     |
| `docker/enterprise/**`    | Enterprise Docker images                      |
| `helm/**`                 | Helm charts (Hyperscaler/Enterprise only)     |
| `operator/**`             | Kubernetes Operator (Hyperscaler only)        |
| `deploy/**`               | Higher-edition deployment descriptors         |
| `packaging/enterprise/**` | Enterprise packaging                          |
| `packaging/hyperscaler/**`| Hyperscaler packaging                         |

**Always allowed in `main`:**
- `docker/community/**` — Community Docker images
- Root-level Community Dockerfiles (`Dockerfile`, `Dockerfile.community-simple`, etc.)
- `docs/**` — All documentation, including files mentioning enterprise/hyperscaler
- `src/**`, `include/**`, `cmake/**` — Sourcecode (public)
- `scripts/**`, `tests/**`, `benchmarks/**`

### 3.2 PR → `enterprise` (Enterprise/Military lane)

**Enforced by:** `.github/workflows/pr-path-gate-enterprise.yml`

The following paths are **blocked** in PRs targeting `enterprise`:

| Blocked path prefix         | Reason                          |
|-----------------------------|---------------------------------|
| `docker/hyperscaler/**`     | Hyperscaler-only Docker images  |
| `packaging/hyperscaler/**`  | Hyperscaler-only packaging      |
| `helm/**`                   | Helm is Hyperscaler-only        |
| `operator/**`               | Operator is Hyperscaler-only    |
| `deploy/hyperscaler/**`     | Hyperscaler deploy descriptors  |

**Allowed in `enterprise`:**
- `docker/enterprise/**`, `docker/community/**`
- `packaging/enterprise/**`
- `deploy/enterprise/**`
- All sourcecode, docs, tests

### 3.3 PR → `hyperscaler` (Hyperscaler lane)

**Enforced by:** `.github/workflows/pr-path-gate-hyperscaler.yml`

No paths are blocked in the Hyperscaler lane. Instead, **mandatory validations** run:
- Helm charts in `helm/**` must include a `Chart.yaml`
- Kubernetes manifests in `operator/**` must be valid YAML

### 3.4 Path organisation summary

```
docker/
  community/       ← Community Docker images (also root-level Dockerfiles)
  enterprise/      ← Enterprise Docker images
  hyperscaler/     ← Hyperscaler Docker images (Dockerfile, Dockerfile.lite)
helm/              ← Helm charts (Hyperscaler only)
operator/          ← Kubernetes Operator (Hyperscaler only)
deploy/
  enterprise/      ← Enterprise deployment descriptors
  hyperscaler/     ← Hyperscaler deployment descriptors
packaging/
  enterprise/      ← Enterprise packaging (deb, rpm, msi, …)
  hyperscaler/     ← Hyperscaler packaging
docs/              ← All documentation (allowed in all lanes)
src/               ← Sourcecode (public, allowed in all lanes)
```

---

## 4. Release Process

### 4.1 Community Release (`main`)

```
1. Create release branch from develop:
   git checkout develop && git checkout -b release/community/vX.Y.Z

2. Include only Community-relevant commits:
   - Cherry-pick specific commits, OR
   - Remove higher-edition artefacts via git rm / revert

3. Open PR: release/community/vX.Y.Z → main

4. CI gates must pass:
   - pr-path-gate-main.yml  (no higher-edition artefacts)
   - edition-community-ci.yml (COMMUNITY build green)
   - pr-quick-checks.yml (lint + CMake validate + audit)

5. Merge (squash or merge commit) → main

6. Tag: git tag -s v X.Y.Z  (on main)
   git push origin vX.Y.Z

7. GitHub Release is created automatically
   → dockerhub-publish-on-release.yml publishes Community Docker to DockerHub
```

### 4.2 Enterprise Release (`enterprise`)

```
1. Create release branch from develop:
   git checkout develop && git checkout -b release/enterprise/enterprise-vX.Y.Z

2. Ensure only Enterprise/Military-relevant content (no Hyperscaler-only artefacts)

3. Open PR: release/enterprise/enterprise-vX.Y.Z → enterprise

4. CI gates must pass:
   - pr-path-gate-enterprise.yml (no Hyperscaler-only artefacts)
   - edition-enterprise-ci.yml (ENTERPRISE build green on enterprise branch)
   - edition-military-ci.yml (MILITARY build green on enterprise branch)

5. Merge → enterprise

6. Tag: enterprise-vX.Y.Z  (on enterprise branch)
   git push origin enterprise-vX.Y.Z

7. publish-enterprise.yml is triggered by the tag:
   - Requires approval from 'enterprise-prod' environment reviewers
   - Builds ENTERPRISE + MILITARY artefacts
   - Signs artefacts with GPG
   - Publishes to private Enterprise registry
```

### 4.3 Hyperscaler Release (`hyperscaler`)

```
1. Create release branch from develop:
   git checkout develop && git checkout -b release/hyperscaler/hyperscaler-vX.Y.Z

2. Include all Hyperscaler artefacts (Docker, Helm, Operator)

3. Open PR: release/hyperscaler/hyperscaler-vX.Y.Z → hyperscaler

4. CI gates must pass:
   - pr-path-gate-hyperscaler.yml (Helm + Operator validation)
   - edition-hyperscaler-ci.yml (HYPERSCALER build green)

5. Merge → hyperscaler

6. Tag: hyperscaler-vX.Y.Z  (on hyperscaler branch)
   git push origin hyperscaler-vX.Y.Z

7. publish-hyperscaler.yml is triggered by the tag:
   - Requires approval from 'hyperscaler-prod' environment reviewers
   - Builds HYPERSCALER artefacts, Docker image (multi-arch), Helm charts
   - Publishes Docker image to Hyperscaler private registry
   - Pushes Helm charts to Hyperscaler Helm repo
```

### 4.4 Tag naming convention

| Edition       | Tag format             | Example                   |
|---------------|------------------------|---------------------------|
| Community     | `v{major}.{minor}.{patch}` | `v2.1.0`              |
| Enterprise    | `enterprise-v{major}.{minor}.{patch}` | `enterprise-v2.1.0` |
| Hyperscaler   | `hyperscaler-v{major}.{minor}.{patch}`| `hyperscaler-v3.0.0`|

### 4.5 Release artefact naming convention

All release assets (GitHub release files and Docker tags) must use:

`themisdb-{version}-{edition}-{sourcecode|binary}-{arm|x86|x64}`

Examples:

- `themisdb-1.9.0-community-sourcecode-x64.zip`
- `themisdb-1.9.0-community-binary-x64.zip`
- `themisdb-1.9.0-community-binary-arm` (Docker tag)
- `themisdb-1.9.0-enterprise-binary-x64.tar.gz`
- `themisdb-1.9.0-hyperscaler-binary-arm` (Docker tag)

Rules:

- `edition` is always lowercase (`community`, `enterprise`, `hyperscaler`, `military`, `minimal`).
- `sourcecode` is used for source archives, `binary` for executables/packages/containers.
- `arm` and `x64` are the canonical architecture tokens for Docker multi-arch publication.
- `latest` may exist as a compatibility alias, but versioned tags above are canonical.

### 4.6 Cleanup of legacy names (GitHub + DockerHub)

To clean up historical, non-canonical names:

1. List release assets and flag non-matching names:

  ```bash
  gh release view <tag> --json assets --jq '.assets[].name' \
    | grep -Ev '^themisdb-[0-9]+\.[0-9]+\.[0-9]+(-[A-Za-z0-9.-]+)?-[a-z0-9-]+-(sourcecode|binary)-(arm|x86|x64)(\..+)?$'
  ```

2. Delete legacy GitHub assets after replacement upload:

  ```bash
  gh release delete-asset <tag> <asset-name> --yes
  ```

3. For DockerHub, delete legacy tags in the repository UI/API and keep only:
  - canonical tags (`themisdb-{version}-{edition}-binary-{arm|x64}`)
  - `latest` compatibility alias (stable releases only)

---

## 5. CI/CD Pipeline Architecture

### 5.1 On `develop` (integration)

| Workflow                  | Trigger              | Editions           |
|---------------------------|----------------------|--------------------|
| `edition-community-ci.yml`| push to develop      | COMMUNITY          |
| `edition-enterprise-ci.yml`| push to develop     | ENTERPRISE         |
| `edition-hyperscaler-ci.yml`| push to develop    | HYPERSCALER        |
| `edition-military-ci.yml` | push to develop      | MILITARY           |
| `edition-minimal-ci.yml`  | push to develop      | MINIMAL            |
| `pr-quick-checks.yml`     | PR (all branches)    | static analysis    |

### 5.2 On `main` (Community release lane)

| Workflow                          | Trigger              | Action                          |
|-----------------------------------|----------------------|---------------------------------|
| `edition-community-ci.yml`        | push to main         | Build + test COMMUNITY          |
| `edition-minimal-ci.yml`          | push to main         | Build + test MINIMAL            |
| `pr-path-gate-main.yml`           | PR → main            | Block higher-edition artefacts  |
| `dockerhub-publish-on-release.yml`| GitHub Release       | Publish Community Docker        |
| `repair-github-release-assets.yml`| workflow_dispatch    | Normalize legacy release assets |
| `repair-dockerhub-tags.yml`       | workflow_dispatch    | Detect/cleanup legacy Docker tags |

### 5.3 On `enterprise` (Enterprise release lane)

| Workflow                         | Trigger              | Action                          |
|----------------------------------|----------------------|---------------------------------|
| `edition-enterprise-ci.yml`      | push to enterprise   | Build + test ENTERPRISE         |
| `edition-military-ci.yml`        | push to enterprise   | Build + test MILITARY           |
| `pr-path-gate-enterprise.yml`    | PR → enterprise      | Block Hyperscaler-only artefacts|
| `publish-enterprise.yml`         | tag `enterprise-v*`  | Publish (environment-gated)     |

### 5.4 On `hyperscaler` (Hyperscaler release lane)

| Workflow                         | Trigger              | Action                          |
|----------------------------------|----------------------|---------------------------------|
| `edition-hyperscaler-ci.yml`     | push to hyperscaler  | Build + test HYPERSCALER        |
| `pr-path-gate-hyperscaler.yml`   | PR → hyperscaler     | Validate Helm/Operator          |
| `publish-hyperscaler.yml`        | tag `hyperscaler-v*` | Publish Docker+Helm (env-gated) |

---

## 6. Private Publishing via Environments & Secrets

Enterprise and Hyperscaler artefacts are private. Publishing is protected by
GitHub Environments with required reviewer approvals.

### 6.1 Required Environments

Configure these in **Repository Settings → Environments**:

| Environment         | Reviewers (suggested)          | Branch filter          |
|---------------------|--------------------------------|------------------------|
| `enterprise-prod`   | Release Manager + Security Lead| `enterprise` branch    |
| `hyperscaler-prod`  | Release Manager + Infra Lead   | `hyperscaler` branch   |

### 6.2 Required Secrets

Configure these in the corresponding environment (not repo-level):

#### `enterprise-prod` environment

| Secret                       | Description                          |
|------------------------------|--------------------------------------|
| `ENTERPRISE_REGISTRY_URL`    | OCI registry URL                     |
| `ENTERPRISE_REGISTRY_USER`   | Registry username                    |
| `ENTERPRISE_REGISTRY_TOKEN`  | Registry push token                  |
| `ENTERPRISE_SIGNING_KEY`     | GPG private key (armored)            |
| `ENTERPRISE_SIGNING_PASSPHRASE` | GPG key passphrase               |

#### `hyperscaler-prod` environment

| Secret                          | Description                       |
|---------------------------------|-----------------------------------|
| `HYPERSCALER_REGISTRY_URL`      | OCI/Docker registry URL           |
| `HYPERSCALER_REGISTRY_USER`     | Registry username                 |
| `HYPERSCALER_REGISTRY_TOKEN`    | Registry push token               |
| `HYPERSCALER_HELM_REPO_URL`     | Helm repository URL               |
| `HYPERSCALER_HELM_REPO_USER`    | Helm repo username                |
| `HYPERSCALER_HELM_REPO_TOKEN`   | Helm repo push token              |
| `HYPERSCALER_SIGNING_KEY`       | GPG private key (armored)         |
| `HYPERSCALER_SIGNING_PASSPHRASE`| GPG key passphrase                |

#### Community (existing)

| Secret               | Environment / Location | Description          |
|----------------------|------------------------|----------------------|
| `DOCKERHUB_USERNAME` | Repository level       | DockerHub username   |
| `DOCKERHUB_TOKEN`    | Repository level       | DockerHub push token |

### 6.3 Security guarantees

- Secrets in `enterprise-prod` / `hyperscaler-prod` environments are **never**
  accessible on fork PRs or during build jobs (only in the environment-gated
  `publish` jobs after reviewer approval).
- The `build` job (compile + test) always runs **without any secrets**.
- Publishing only starts after:
  1. The `build` job succeeds.
  2. The environment's required reviewers approve the deployment.
  3. The branch/tag matches the expected pattern (`enterprise-v*` / `hyperscaler-v*`).

---

## 7. Rollback Plan

### 7.1 Community rollback

```bash
# Revert the last merge commit on main
git checkout main
git revert <merge-commit-sha> --no-commit
git commit -m "revert: roll back vX.Y.Z"
git push origin main

# Create a patch release tag
git tag v X.Y.(Z+1)
git push origin vX.Y.(Z+1)
```

### 7.2 Enterprise rollback

```bash
git checkout enterprise
git revert <merge-commit-sha> --no-commit
git commit -m "revert: roll back enterprise-vX.Y.Z"
git push origin enterprise
git tag enterprise-vX.Y.(Z+1)
git push origin enterprise-vX.Y.(Z+1)
# → triggers publish-enterprise.yml (requires environment approval)
```

### 7.3 Hyperscaler rollback

```bash
git checkout hyperscaler
git revert <merge-commit-sha> --no-commit
git commit -m "revert: roll back hyperscaler-vX.Y.Z"
git push origin hyperscaler
git tag hyperscaler-vX.Y.(Z+1)
git push origin hyperscaler-vX.Y.(Z+1)
# → triggers publish-hyperscaler.yml (requires environment approval)
```

### 7.4 Docker rollback

If a bad Docker image was pushed to DockerHub (Community):
- Unpublish the tag in DockerHub UI (or via API).
- Re-trigger `dockerhub-publish-on-release.yml` via `workflow_dispatch` with
  the previous stable tag.

---

## 8. Onboarding: Local Builds per Edition

### Prerequisites

```bash
# Ubuntu 22.04
sudo apt-get install -y cmake ninja-build gcc-12 g++-12 \
    libssl-dev libcurl4-openssl-dev libboost-all-dev librocksdb-dev \
    libfmt-dev libtbb-dev libspdlog-dev nlohmann-json3-dev
```

### MINIMAL Edition (embedded, no LLM)

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=MINIMAL \
  -DCMAKE_C_COMPILER=gcc-12 \
  -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
```

### COMMUNITY Edition (default, LLM enabled)

```bash
git submodule update --init --depth=1 llama.cpp

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_C_COMPILER=gcc-12 \
  -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -- -j$(nproc)
```

### ENTERPRISE Edition

```bash
git submodule update --init --depth=1 llama.cpp

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_C_COMPILER=gcc-12 \
  -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -- -j$(nproc)
```

### MILITARY Edition (Enterprise + hardening)

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=MILITARY \
  -DCMAKE_C_COMPILER=gcc-12 \
  -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -- -j$(nproc)
```

### HYPERSCALER Edition

```bash
git submodule update --init --depth=1 llama.cpp

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DCMAKE_C_COMPILER=gcc-12 \
  -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -- -j$(nproc)
```

### Docker builds per edition

```bash
# Community (production image)
docker build -f Dockerfile -t themisdb:community .

# Community (simplified/faster build)
docker build -f Dockerfile.community-simple -t themisdb:community-simple .

# Hyperscaler
docker build -f docker/hyperscaler/Dockerfile \
  -t themisdb:hyperscaler \
  --build-arg THEMIS_VERSION=local \
  --build-arg ENABLE_LLM=ON \
  .

# Hyperscaler Lite (faster, for local testing)
docker build -f docker/hyperscaler/Dockerfile.lite \
  -t themisdb:hyperscaler-lite \
  .
```

---

## 9. Branch Creation (One-Time Setup)

The `enterprise` and `hyperscaler` branches must be created once from `develop`.

### Option A: Automated (recommended) — bootstrap workflow

The repository includes a dedicated workflow that creates the branches for you:

1. Merge the release strategy PR into `develop`.
2. Go to **Actions → Bootstrap Release Branches → Run workflow**.
3. Leave "Source branch" as `develop` (default).
4. Set "Dry-run" to `true` first to see what would be created.
5. Re-run with "Dry-run" set to `false` to actually create the branches.

The workflow is idempotent — running it multiple times is safe; it skips
branches that already exist.

### Option B: Using GitHub CLI

```bash
# Ensure develop is up-to-date locally
git fetch origin develop
git checkout develop
git pull --ff-only origin develop

# Create enterprise branch
git checkout -b enterprise
git push origin enterprise

# Create hyperscaler branch
git checkout develop
git checkout -b hyperscaler
git push origin hyperscaler
```

### Set branch protection rules

In **GitHub Repository Settings → Branches**, configure protection rules for:

| Branch        | Required status checks                              | Require PR    |
|---------------|-----------------------------------------------------|---------------|
| `main`        | `pr-path-gate-main / Community-only path policy`    | Yes           |
| `enterprise`  | `pr-path-gate-enterprise / Enterprise-lane path policy` | Yes       |
| `hyperscaler` | `pr-path-gate-hyperscaler / Hyperscaler-lane path policy` | Yes     |
| `develop`     | `pr-quick-checks` (lint, cmake-validate, audit)     | Yes           |

---

## 10. Migration Guide (Docker Path Reorganisation)

### What changed

Hyperscaler Docker artefacts have been moved into edition-specific subdirectories
to enable clean path-based gating in CI:

| Old path                              | New path                                          |
|---------------------------------------|---------------------------------------------------|
| `docker/Dockerfile.hyperscaler`       | `docker/hyperscaler/Dockerfile`                   |
| `docker/Dockerfile.hyperscaler-lite`  | `docker/hyperscaler/Dockerfile.lite`              |
| `docker/docker-compose.hyperscaler-sharding.yml` | `docker/hyperscaler/docker-compose.hyperscaler-sharding.yml` |

New directories were added to reserve edition-specific paths:
- `docker/community/` — Community Docker images (actual files at root for backwards compatibility)
- `docker/enterprise/` — Enterprise Docker images (planned; README only for now)
- `docker/hyperscaler/` — Hyperscaler Docker images and compose files

The scripts referencing these files have been updated accordingly:
- `scripts/build-hyperscaler-docker.sh`
- `scripts/start-hyperscaler-docker.sh`

### If you have local scripts referencing old paths

Update any local scripts or CI configuration that uses:
- `docker/Dockerfile.hyperscaler` → use `docker/hyperscaler/Dockerfile`
- `docker/Dockerfile.hyperscaler-lite` → use `docker/hyperscaler/Dockerfile.lite`
- `docker/docker-compose.hyperscaler-sharding.yml` → use `docker/hyperscaler/docker-compose.hyperscaler-sharding.yml`

### Community Docker (unchanged)

Community Docker files remain at the repo root and are allowed in `main`:
- `Dockerfile` (production, multi-stage)
- `Dockerfile.community-simple` (simplified, faster build)
- `docker-compose.yml`

---

## 11. Known Limitations

- **Enterprise and Hyperscaler sourcecode is public.** The branch model only
  protects *build/runtime artefacts*. If proprietary enterprise code needs to be
  private, a separate private repository would be required for those modules.
- **Branch creation via bootstrap workflow.** The `enterprise` and `hyperscaler`
  branches need to be created once via the `bootstrap-release-branches.yml`
  workflow (see §9). They are not automatically created on merge.
- **Path-gate is not retroactive.** Existing content on `main` that violates the
  policy is not automatically removed. The gate only applies to new PRs.
- **Military artefacts.** Military builds produce artefacts signed and published
  via the `enterprise-prod` environment. A dedicated `military-prod` environment
  with stricter access can be added if required.
- **Enterprise Docker images.** `docker/enterprise/` is reserved but currently
  contains only a README. Enterprise-specific Dockerfiles will be added when the
  Enterprise Docker build pipeline is activated.
