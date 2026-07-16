# ThemisDB — Maintainers

This document lists the current maintainers and their areas of responsibility.

> **New contributors:** Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting your first pull request.

---

## Project Lead

| GitHub | Role |
|---|---|
| [@makr-code](https://github.com/makr-code) | Project Lead, Release Manager, Core Architect |

---

## Module Maintainers

The table below maps each source module to its responsible maintainer. All code review requests and issues for a module are routed to the corresponding maintainer via [CODEOWNERS](.github/CODEOWNERS).

| Module | Path | Maintainer |
|---|---|---|
| **Storage & MVCC** | `src/storage/`, `src/transaction/` | [@makr-code](https://github.com/makr-code) |
| **Query Engine & AQL** | `src/query/`, `src/aql/` | [@makr-code](https://github.com/makr-code) |
| **Vector & Index** | `src/index/` | [@makr-code](https://github.com/makr-code) |
| **Graph** | `src/graph/` | [@makr-code](https://github.com/makr-code) |
| **Search** | `src/search/` | [@makr-code](https://github.com/makr-code) |
| **LLM / AI** | `src/llm/`, `src/rag/`, `src/prompt_engineering/`, `src/training/` | [@makr-code](https://github.com/makr-code) |
| **Network & Server** | `src/network/`, `src/server/`, `src/rpc_grpc/` | [@makr-code](https://github.com/makr-code) |
| **Replication & Sharding** | `src/replication/`, `src/sharding/`, `src/cdc/` | [@makr-code](https://github.com/makr-code) |
| **Security & Auth** | `src/security/`, `src/auth/`, `src/governance/` | [@makr-code](https://github.com/makr-code) |
| **Observability** | `src/observability/` | [@makr-code](https://github.com/makr-code) |
| **Analytics & Geo** | `src/analytics/`, `src/geo/`, `src/timeseries/`, `src/temporal/` | [@makr-code](https://github.com/makr-code) |
| **Content & Ingestion** | `src/content/`, `src/ingestion/`, `src/importers/`, `src/exporters/` | [@makr-code](https://github.com/makr-code) |
| **Core & Performance** | `src/core/`, `src/base/`, `src/performance/`, `src/utils/` | [@makr-code](https://github.com/makr-code) |
| **Plugins & Scheduler** | `src/plugins/`, `src/scheduler/`, `src/updates/` | [@makr-code](https://github.com/makr-code) |
| **Build & Infrastructure** | `cmake/`, `.github/`, `CMakeLists.txt` | [@makr-code](https://github.com/makr-code) |
| **Documentation** | `docs/`, `compendium/` | [@makr-code](https://github.com/makr-code) |

---

## Responsibilities

### Project Lead
- Final decision authority on architecture, API stability, and release dates
- Merges PRs to protected release branches such as `community`, `enterprise`, `hyperscaler`, `military`, and `minimal`
- Creates release tags and publishes GitHub Releases
- Manages the security advisory process

### Module Maintainer
- Reviews and approves PRs touching their module within **5 business days**
- Ensures module tests remain green on `develop`
- Keeps module `ROADMAP.md` and `README.md` up to date
- Escalates blockers to the Project Lead

---

## Escalation and Decision Path

To align responsibilities with project governance and contributor guidance:

1. **Normal contribution flow:** follow [CONTRIBUTING.md](CONTRIBUTING.md) and route review to module maintainers via CODEOWNERS.
2. **Cross-module disagreements or blocked decisions:** escalate to the Project Lead as defined in [GOVERNANCE.md](GOVERNANCE.md).
3. **Behavior and conduct issues:** follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
4. **Security vulnerabilities or sensitive incidents:** use [SECURITY.md](SECURITY.md) and [SOP.md](SOP.md#sop-05--security-vulnerability-response).

---

## Becoming a Maintainer

ThemisDB is currently a single-maintainer project. If you are interested in taking on a co-maintainer role for a specific module:

1. Contribute consistently (≥ 3 accepted PRs to the module).
2. Open a GitHub Discussion proposing your co-maintainer candidacy.
3. The Project Lead will review and respond within 2 weeks.

---

## Emeritus Maintainers

*No emeritus maintainers at this time.*

---

## Contact

- **General questions:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Code of Conduct / community behavior reports:** Follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md#project-specific-reporting-channels)
- **Security issues:** [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new) — see [SECURITY.md](SECURITY.md)
- **Bug reports:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---
Zuletzt geprueft (Root-Sync): 2026-06-15
