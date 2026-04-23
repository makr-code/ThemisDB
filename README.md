<div align="center">

# ThemisDB

**High-performance multi-model database with native AI/LLM integration**

[![Version](https://img.shields.io/badge/version-1.8.2--rc-blue)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Security](https://img.shields.io/badge/security-A+-green)](SECURITY.md)
[![Docs](https://img.shields.io/badge/docs-online-blue)](https://makr-code.github.io/ThemisDB/)
[![Contributing](https://img.shields.io/badge/contributions-welcome-brightgreen)](CONTRIBUTING.md)

[📚 Documentation](docs/Home.md) · [🚀 Quick Start](QUICKSTART.md) · [❓ FAQ](docs/FAQ.md) · [Release Notes](CHANGELOG.md) · [Roadmap](roadmap.md)

</div>

---

## What is ThemisDB?

ThemisDB is an enterprise-grade, multi-model database engine that combines relational, graph, vector, document, geospatial, and time-series storage in a single system — with native AI/LLM integration built in. It is designed for workloads that require strong transactional guarantees, distributed operation, and advanced analytical capabilities alongside modern machine-learning workflows.

ThemisDB has **comprehensive documentation for all 50 modules** with production-ready standards:

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

## Quick Start

### Docker (fastest)

```bash
docker pull 
docker run
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

## Architecture

ThemisDB is organised into 55 source modules under `src/`, grouped into four logical layers:

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
→ Module list and status: [roadmap.md](roadmap.md)

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
