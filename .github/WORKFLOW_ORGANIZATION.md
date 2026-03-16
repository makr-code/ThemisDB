# GitHub Workflows Organization Plan

## Overview

ThemisDB uses **9 functional categories** to organize its 138+ GitHub Actions workflows.
Each category maps to a numbered subdirectory under `.github/workflows/`.

## Category Definitions

| # | Directory | Purpose | Count |
|---|-----------|---------|-------|
| 1 | `01-core/` | Foundation CI shared by all other workflows | 2 |
| 2 | `02-feature-modules/` | Per-feature CI for individual ThemisDB modules | 62 |
| 3 | `03-editions/` | Edition-specific build and test workflows | 6 |
| 4 | `04-release/` | Release automation and deployment pipelines | 6 |
| 5 | `05-quality/` | Quality gates: security, build checks, validation | 16 |
| 6 | `06-infrastructure/` | Infrastructure-level CI: GPU, distributed, networking | 19 |
| 7 | `07-data-pipelines/` | Data ingestion, streaming, and export CI | 9 |
| 8 | `08-maintenance/` | Housekeeping: labels, docs sync, audits | 12 |
| 9 | `09-pr-gates/` | PR-level scope gates and quick checks | 4 |
| — | `docs/` | Documentation generation pipelines | 2 |

**Total: 138 workflows**

## 02-feature-modules Subcategories

| Subdirectory | Domain | Count |
|--------------|--------|-------|
| `acceleration/` | Hardware acceleration and benchmarking | 2 |
| `adaptive-query/` | Query compilation, JIT, spatial/temporal | 9 |
| `chimera/` | Chimera platform integration | 3 |
| `config/` | Configuration management | 3 |
| `llm/` | LLM integration and inference | 3 |
| `replication/` | Data replication strategies | 5 |
| `resilience/` | Retry, backoff, circuit breakers | 4 |
| `security/` | Feature-level auth and user management | 2 |
| `storage/` | Storage engine features | 16 |
| `transactions/` | Transaction management and isolation | 6 |
| *(root)* | Cross-cutting feature modules | 9 |

## 05-quality Subcategories

| Subdirectory | Domain | Count |
|--------------|--------|-------|
| `security/` | Encryption, scanning, PKI, PII | 7 |
| `build/` | Build reproducibility, performance regression | 3 |
| `validation/` | Config, AI, Grafana, roadmap validation | 6 |

## 06-infrastructure Subcategories

| Subdirectory | Domain | Count |
|--------------|--------|-------|
| `gpu/` | CUDA, Vulkan, OpenCL pipelines | 4 |
| `distributed/` | Sharding, cluster, Raft coordination | 6 |
| `observability/` | Metrics, tracing, diagnostics | 4 |
| `networking/` | Wire protocol, QoS, API gateway | 5 |

## Reusable Workflow Dependencies

Two workflows are called by many others via `workflow_call`:

- **`01-core/ci-scope-classifier.yml`** — analyses changed files and gates downstream jobs.
  Referenced by 107 workflows.
- **`03-editions/edition-build-ci.yml`** — reusable edition build logic.
  Referenced by all 5 per-edition wrapper workflows.

## Naming Convention

```
<category-dir>/[subcategory/]<feature-name>-<type>.yml
```

Types:
- `-ci.yml` — continuous integration workflow
- `.yml` — event-driven or maintenance workflow (no CI suffix)

## Migration History

All workflows were migrated from a flat structure in `.github/workflows/` to
this hierarchical layout in a single atomic commit. All internal `uses:` references
were updated in the same commit to ensure no broken dependencies.
