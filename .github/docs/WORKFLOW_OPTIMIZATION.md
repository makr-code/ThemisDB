# Workflow Optimization Best Practices

This document describes how CI workflows in this repository are structured to
minimise unnecessary runs while ensuring all relevant code changes are validated
before merging.

---

## 1. PR Event Types

Every `pull_request:` trigger **must** include an explicit `types:` filter:

```yaml
on:
  pull_request:
    types: [opened, synchronize, reopened]
```

This prevents workflows from running on events such as `edited`, `labeled`,
`ready_for_review`, `assigned`, or `milestoned` that do not change the
branch's file tree.

---

## 2. Using the CI Scope Classifier

The reusable workflow `.github/workflows/ci-scope-classifier.yml` analyses the
changed files for a push or pull request and exports boolean outputs that
downstream jobs use to decide whether to run.

### Available outputs

| Output | True when … |
|--------|-------------|
| `has_code_changes` | C++, CUDA, or Python source files changed |
| `has_security_changes` | `src/security/**` or `src/auth/**` changed |
| `has_gpu_changes` | GPU module or geo-GPU backend files changed |
| `has_llm_changes` | LLM / RAG / CUDA kernel files changed |
| `has_doc_only_changes` | **Only** documentation changed (no code) |
| `has_config_changes` | CMake / config / workflow files changed |
| `has_grafana_changes` | Grafana dashboard JSON files changed |

### How to call the classifier

```yaml
jobs:
  ci-scope-classifier:
    permissions: {}
    uses: ./.github/workflows/ci-scope-classifier.yml

  my-job:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_code_changes == 'true'
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      # …
```

---

## 3. Language-specific filter patterns

### C++ / CUDA workflows

```yaml
jobs:
  ci-scope-classifier:
    permissions: {}
    uses: ./.github/workflows/ci-scope-classifier.yml

  build-cpp:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_code_changes == 'true'
```

When a finer check is needed (e.g. a job that only cares about CUDA files):

```yaml
  cuda-compile:
    needs: ci-scope-classifier
    if: >
      needs.ci-scope-classifier.outputs.has_llm_changes == 'true' ||
      needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'
```

### Security workflows

```yaml
  security-tests:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_security_changes == 'true'
```

### GPU workflows

```yaml
  gpu-tests:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_gpu_changes == 'true'
```

### LLM / CUDA kernel workflows

```yaml
  llm-tests:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_llm_changes == 'true'
```

### Documentation-only workflows

```yaml
  docs-lint:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_doc_only_changes == 'true'
```

### Grafana dashboard workflows

```yaml
  grafana-validate:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_grafana_changes == 'true'
```

### Config / infrastructure workflows

```yaml
  config-validate:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_config_changes == 'true'
```

---

## 4. Correct workflow trigger template

A complete, well-optimised workflow skeleton:

```yaml
name: My Module CI

on:
  push:
    branches:
      - main
      - develop
    paths:
      - 'src/my-module/**'
      - 'include/my-module/**'
      - 'tests/test_my_module*.cpp'
      - '.github/workflows/my-module-ci.yml'
  pull_request:
    types: [opened, synchronize, reopened]   # ← required
    paths:
      - 'src/my-module/**'
      - 'include/my-module/**'
      - 'tests/test_my_module*.cpp'
      - '.github/workflows/my-module-ci.yml'
  workflow_dispatch:

concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  ci-scope-classifier:
    permissions: {}
    uses: ./.github/workflows/ci-scope-classifier.yml

  build-and-test:
    needs: ci-scope-classifier
    if: needs.ci-scope-classifier.outputs.has_code_changes == 'true'
    runs-on: ubuntu-latest
    permissions:
      contents: read
    steps:
      - uses: actions/checkout@v4
      # …
```

---

## 5. Common mistakes to avoid

| Anti-pattern | Correct pattern |
|---|---|
| `pull_request:` with no `types:` | `pull_request:\n  types: [opened, synchronize, reopened]` |
| `pull_request:` with no `paths:` (broad trigger) | Add a `paths:` filter matching the module's sources |
| Job with no `if:` condition after `ci-scope-classifier` | Add `if: needs.ci-scope-classifier.outputs.<scope> == 'true'` |
| Running heavy GPU jobs on doc-only PRs | Gate with `has_gpu_changes` or `has_llm_changes` |

---

## 6. See also

- `.github/ci-scope-config.yaml` — path-pattern → scope mappings
- `.github/scripts/classify_ci_scope.py` — classifier implementation
- `.github/workflows/ci-scope-classifier.yml` — reusable classifier workflow
