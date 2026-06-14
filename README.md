<div align="center">

# ThemisDB

**High-performance multi-model database with native AI/LLM integration**

[![Version](https://img.shields.io/badge/version-1.9.0--beta-blue)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Status](https://img.shields.io/badge/status-ACTIVE_DEVELOPMENT-orange)](ROADMAP.md)
[![Maturity](https://img.shields.io/badge/maturity-66_sync__15_PC__45_H__2_E__4_T-orange)](ROADMAP.md)
[![Contributing](https://img.shields.io/badge/contributions-welcome-brightgreen)](CONTRIBUTING.md)

[📚 Documentation](docs/Home.md) · [🚀 Quick Start](QUICKSTART.md) · [🛠️ Setup](SETUP.md) · [⚠️ Status](ROADMAP.md) · [🆘 Support](SUPPORT.md) · [Release Notes](CHANGELOG.md)

</div>

---

## ⚠️ IMPORTANT: Module Status Snapshot (66/66)

**This is an active development project.** Current synchronized status snapshot (source-based):
- ✅ **15 modules** are `PRODUCTION_CANDIDATE`
- 🟡 **45 modules** are `HARDENING`
- 🔴 **2 modules** are `EXPERIMENTAL` (`llama_cpp`, `stable_diffusion`)
- ⚪ **4 modules** are `THIN/PLACEHOLDER` (`ai_working`, `distributed_tensor`, `evaluation`, `retrieval`)

**See [ROADMAP.md](ROADMAP.md) for the full 66-module table.**

Evidence artifacts:
- [logs/module_status_66_refined.csv](logs/module_status_66_refined.csv)
- [logs/module_test_include_refs_66.csv](logs/module_test_include_refs_66.csv)
- [logs/module_status_66_classified_v2.csv](logs/module_status_66_classified_v2.csv)

## Documentation Sync (2026-05-26)

- Root-level markdown documentation was reviewed and synchronized.
- Current wire/themis verification baseline:
  - `cmake --build --preset windows-release --target themis_tests --parallel 16`
  - `themis_tests --gtest_filter=WireProtocolServer.SingleThreadedIoContextPrunesSessionsAfterDisconnect`
  - `ctest --preset windows-release -R ThemisWireProtocolV1Tests --output-on-failure`
- Recent technical hardening reflected in docs/changelog:
  - fail-closed wire bootstrap behaviour retained for deprecated bridge-only setup
  - single-threaded wire server session pruning regression covered by dedicated test
  - `multi_lora_manager` opaque adapter handle consistency fix (`void*`)

### Scanner Baseline Update (2026-06-11)

- Aktueller Gap-Scan-Stand wird ueber die Worklist gepflegt:
  - `ai_working/gap_scan_report_ollama_gemma4.md`
  - `ai_working/gap_scan_report_ollama_gemma4.smoke.md`
- Scope-Regel: `themis_core` actionable, `third_party` nur informativ.
- Aktives Tracking-Issue fuer den aktuellen Baseline-Scope:
  - `#5475` (`[P0-HIGH] INCLUDE Module - Current Gap Worklist Tracking (2026-06-11)`)
- Konsolidierungsstatus GitHub-Issues:
  - Historische v3-P0- und Cross-Module-Tracker wurden geschlossen (superseded by `#5475`).
  - Duplikat-Tracker `#5474` wurde geschlossen.
  - Bewusst offen bleiben die Legacy-Umstellungs-Issues `#5363` bis `#5366`.

---

## What is ThemisDB?

ThemisDB is a **high-performance multi-model database engine in active development** that aims to combine relational, graph, vector, document, geospatial, and time-series storage in a single system with native AI/LLM integration.

**Current Status (2026-06-14, source-evidence based):** 66 modules are tracked in `src`; 15 are `PRODUCTION_CANDIDATE`, 45 are `HARDENING`, 2 are `EXPERIMENTAL`, and 4 are `THIN/PLACEHOLDER`. See [ROADMAP.md](ROADMAP.md) for detailed per-module status.

**Key capabilities at a glance:**

| Capability | Details |
|---|---|
| **Multi-model storage** | Relational · Graph · Vector (HNSW/FAISS) · Document · Geospatial · Time-series |
| **ACID transactions** | MVCC, SSI, 2PC, SAGA orchestration, HLC-based global ordering |
| **Distributed** | Raft consensus, mTLS replication, consistent-hash sharding, auto-failover |
| **AI/LLM native** | Embedded LLM inference (llama.cpp, ONNX), RAG pipeline, prompt engineering, LoRA fine-tuning |
| **Full-text search** | BM25 + vector hybrid search (RRF), faceted, conversational, multi-modal |
| **Observability** | Prometheus metrics, OpenTelemetry tracing, PagerDuty/Slack alerting |
| **Security** | AES-256-GCM field encryption, RLS, Zero-Trust policy, eIDAS timestamping, HSM/Vault |
| **Editions** | MINIMAL · COMMUNITY · ENTERPRISE · MILITARY · HYPERSCALER |

---

## Canonical Onboarding Path

For a consistent onboarding flow, use these pages in order:

1. [QUICKSTART.md](QUICKSTART.md) — install + first successful run
2. [SETUP.md](SETUP.md) — complete local development environment
3. [SUPPORT.md](SUPPORT.md) — support and escalation paths
4. [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) — release lanes and version lifecycle
5. [INDEX.md](INDEX.md) — full root navigation map

---

## Installation

### Docker (fastest)

```bash
docker pull ghcr.io/makr-code/themisdb:latest
docker run -d --name themisdb -p 8765:8765 -p 8766:8766 ghcr.io/makr-code/themisdb:latest
```

Connect via the wire protocol on port `8766` or the REST/HTTP API on port `8765`.

### Build from source

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
# Install dependencies and configure build environment
./scripts/setup-pre-commit.sh          # Linux/macOS
# pwsh ./scripts/setup-third-party.ps1  # all platforms (vcpkg, llama.cpp, ffmpeg)

cmake --preset linux-release        # Linux x64
cmake --build --preset linux-release

# Windows (run from VS Developer Command Prompt):
# cmake --preset windows-release && cmake --build --preset windows-release
```

See [QUICKSTART.md](QUICKSTART.md) for a step-by-step guide, and [SETUP.md](SETUP.md) for a full development-environment walkthrough.

---

## Editions

ThemisDB is available in five editions, selected at CMake build time:

| Edition | Use case | Branch | Build flag |
|---|---|---|---|
| **MINIMAL** | Embedded / resource-constrained | `main` | `-DTHEMIS_EDITION=MINIMAL` |
| **COMMUNITY** | Open-source, self-hosted | `main` | `-DTHEMIS_EDITION=COMMUNITY` |
| **ENTERPRISE** | Commercial, SLA-backed | `enterprise` | `-DTHEMIS_EDITION=ENTERPRISE` |
| **MILITARY** | Hardened / air-gapped | `enterprise` | `-DTHEMIS_EDITION=MILITARY` |
| **HYPERSCALER** | Cloud/OEM, Kubernetes operator | `hyperscaler` | `-DTHEMIS_EDITION=HYPERSCALER` |

Feature sets are nested: MINIMAL ⊂ COMMUNITY ⊂ ENTERPRISE ⊂ HYPERSCALER.

See [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) for the full feature comparison and edition matrix.

---

## Usage

After startup, verify health and run a first query:

```bash
curl http://localhost:8765/health
curl -X POST http://localhost:8765/v2/query \
  -H 'Content-Type: application/json' \
  -d '{"query":"SELECT 1 AS hello"}'
```

---

## Architecture

ThemisDB is organised into tracked source modules under `src/`, grouped into four logical layers:

```
┌─────────────────────────────────────────────────────┐
│  API Layer       REST · GraphQL · gRPC · Wire V2     │
├─────────────────────────────────────────────────────┤
│  Query Layer     AQL · Optimizer · Planner · Cache   │
├─────────────────────────────────────────────────────┤
│  Storage Layer   RocksDB · MVCC · WAL · Sharding     │
├─────────────────────────────────────────────────────┤
│  Distributed     Raft · Replication · Failover · CDC │
└─────────────────────────────────────────────────────┘
```

→ Full architecture reference: [ARCHITECTURE.md](ARCHITECTURE.md)  
→ Module list and status: [ROADMAP.md](ROADMAP.md)

---

## Documentation

| Document | Description |
|---|---|
| [QUICKSTART.md](QUICKSTART.md) | Get running in minutes |
| [SETUP.md](SETUP.md) | Full development environment setup |
| [ARCHITECTURE.md](ARCHITECTURE.md) | System design and module overview |
| [VERSIONING.md](VERSIONING.md) | Versioning policy and release cadence |
| [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) | Branch model, edition matrix, CI/CD pipeline |
| [CHANGELOG.md](CHANGELOG.md) | Release notes (Keep a Changelog format) |
| [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) | Benchmarks and performance targets |
| [SOP.md](SOP.md) | Standard operating procedures (release, hotfix, incident) |
| [GOVERNANCE.md](GOVERNANCE.md) | Project governance: roles, decision-making, contribution policy |
| [MAINTAINERS.md](MAINTAINERS.md) | Maintainer roster and module ownership |
| [SECURITY.md](SECURITY.md) | Security policy and vulnerability reporting |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) | Community guidelines |
| [SUPPORT.md](SUPPORT.md) | Where to get help |
| [INDEX.md](INDEX.md) | Full project structure index |
| [docs/](docs/) | Extended documentation (API reference, guides, research) |
| [compendium/](compendium/docs/) | In-depth technical compendium |

---

## Versioning

ThemisDB follows [Semantic Versioning 2.0.0](https://semver.org/). The current version is stored in the [`VERSION`](VERSION) file and in [`CHANGELOG.md`](CHANGELOG.md). Pre-release identifiers use the form `-rcN` (release candidate) or `-alphaN` / `-betaN`.

See [VERSIONING.md](VERSIONING.md) for the full versioning policy.

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a pull request. All participants are expected to follow our [Code of Conduct](CODE_OF_CONDUCT.md).

Good first issues are tagged [`good first issue`](https://github.com/makr-code/ThemisDB/issues?q=is%3Aopen+label%3A%22good+first+issue%22) in the issue tracker.

---

## Security

To report a security vulnerability, **do not** open a public issue. Follow the responsible disclosure process in [SECURITY.md](SECURITY.md) or use [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new).

---

## License

ThemisDB is released under the [MIT License](LICENSE).

---

## Module Documentation

> Per-module documentation lives in `src/<module>/README.md` and `include/<module>/`. This section is a navigation index.

---
Zuletzt geprueft (Root-Sync): 2026-05-26

