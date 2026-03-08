# Documentation Organization Plan

## Current State (as of 2026-03)
- ~387 markdown files in `docs/` root (many are implementation summaries, review reports, and working documents)
- 59 subdirectories with well-organized technical documentation

## Navigation & Hub Files

The following files serve as primary navigation entry points and must be kept consistent:

| File | Purpose |
|------|---------|
| `00_DOCUMENTATION_INDEX.md` | Master index with all major documentation areas |
| `DOCUMENTATION_HUB.md` | User-facing navigation hub (role-based, use-case-based) |
| `CATEGORY_INDEX.md` | Category-organized reference index |
| `DOCS_ORGANIZATION_PLAN.md` | This file — documentation structure overview |

## Established Subdirectory Structure

### Language-specific technical docs
- `de/` — German-language feature docs, architecture, security, guides, APIs
  - `de/aql/` — AQL query language reference
  - `de/architecture/` — Architecture docs (MVCC, multi-model, caching, wire protocol)
  - `de/apis/` — API specifications (REST, GraphQL, gRPC, WebSocket, MCP)
  - `de/deployment/` — Build, Docker, CI/CD deployment docs
  - `de/features/` — Feature documentation (vector ops, transactions, CDC, etc.)
  - `de/guides/` — Operational guides (RBAC, TLS, deployment, build)
  - `de/performance/` — Performance optimization docs
  - `de/security/` — Security implementation docs (encryption, HSM, policies)
  - `de/compliance/` — Compliance guides (GDPR, BCP, risk register)

- `en/` — English-language deployment and operations docs
  - `en/deployment/` — Docker build guides, environment variables
  - `en/gpu/` — GPU vector indexing architecture
  - `en/operations/` — Monitoring, operations runbooks
  - `en/features/` — Feature guides (PITR, etc.)

### User-facing content
- `tutorials/` — Step-by-step hands-on tutorials
- `use-cases/` — Production-ready application guides (E-Commerce, IoT, RAG, SaaS)
- `knowledge-base/` — Troubleshooting, performance tips, migration guides, backup/recovery
- `certification/` — Professional certification programs
- `troubleshooting/` — Per-module troubleshooting guides

### Core reference
- `api/` — API reference overview
- `architecture/` — (legacy path, prefer `de/architecture/`)
- `research/` — Research papers, architecture decisions, best practices
- `security/` — Security policies, HSM, PKCS11, production hardening
- `replication/` — Replication module documentation

### Operations & tooling
- `tools/` — Admin, analysis, development, ingestion, operations tools docs
- `production/` — Production runbooks and checklists
- `benchmarks/` — Benchmark methodology (CHIMERA), hardware specs
- `acceleration/` — GPU capability negotiation, error codes, production readiness
- `sharding/` — Shard RPC client documentation
- `storage/` — Storage backend docs (RocksDB, cloud blob)
- `timeseries/` — Time-series configuration and reference

### Meta / process docs
- `ARCHIVED/` — Historical development documents (GAP analyses, old roadmaps, todos, implementation summaries)
- `archive/` — Older reports and analysis files
- `Audit/` — Architecture and documentation audit reports
- `reviews/` — Code and module review reports
- `reports/` — Release reports, competitive analysis, implementation summaries
- `schemas/` — Data schema documentation
- `training/` — Security awareness training materials

## Recommended Cleanup (Future Work)

The docs root still contains many working documents and implementation summaries
that could be moved to `implementation-history/` or `ARCHIVED/implementation-summaries/`
to keep the root cleaner for navigational files. This includes files matching patterns:
- `*_IMPLEMENTATION_SUMMARY.md`
- `*_COMPLETE*.md`
- `*_REVIEW_*.md`
- `CODE_REVIEW_*.md`
- `tmp_*.md`

The `tmp_*.md` files in docs root are temporary working documents and should be removed
once their content has been incorporated.

## Key Entry Points by Audience

| Audience | Start Here |
|----------|-----------|
| New users | `de/guides/QUICKSTART.md` |
| Developers | `api/API_REFERENCE.md`, `de/aql/aql_syntax.md` |
| Contributors | `../CONTRIBUTING.md`, `de/architecture/ARCHITECTURE_OVERVIEW.md` |
| Operators | `en/deployment/DOCKER_BUILD_GUIDE.md`, `en/operations/MONITORING_SETUP_GUIDE.md` |
| Security team | `de/security/security_implementation.md`, `de/security/README.md` |
| AQL users | `de/aql/aql_syntax.md`, `de/aql/aql_functions_reference.md` |
| MVCC/architecture | `de/architecture/architecture_mvcc.md`, `de/architecture/architecture_multi_model.md` |
