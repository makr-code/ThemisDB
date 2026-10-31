---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

<div align="center">

# ThemisDB

**A multi-model database with native AI/LLM integration**

[![Status](https://img.shields.io/badge/status-ACTIVE_DEVELOPMENT-orange)](ROADMAP.md)
[![Build Workflow](https://github.com/makr-code/ThemisDB/actions/workflows/build-mainline.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/build-mainline.yml)
[![Release Workflow](https://github.com/makr-code/ThemisDB/actions/workflows/release-mainline.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/release-mainline.yml)

[📚 Documentation](docs/README.md) · [🚀 Quick Start](QUICKSTART.md) · [🛠️ Development Setup](SETUP.md) · [📋 Roadmap](ROADMAP.md) · [🆘 Support](SUPPORT.md)

</div>

Exact model backend not verifiable from final document text alone.

## What is ThemisDB?

ThemisDB is an actively developed database project that brings multiple data
models and AI/LLM capabilities together. For current implementation and release
readiness, consult the source-backed [project roadmap](ROADMAP.md); this README
is an overview, not a status source.

### Capabilities

| Area | Overview |
|---|---|
| Multi-model data | Relational, document, graph, vector, geospatial, and time-series workloads |
| Query and retrieval | AQL, full-text and vector search, and hybrid retrieval |
| Transactions and distribution | ACID transactions, replication, sharding, and recovery workflows |
| AI/LLM | Inference and retrieval-augmented generation integrations |
| Operations | Metrics, tracing, security controls, and deployment tooling |

Availability and maturity vary by module and edition. Check the linked module
documentation and release gates before relying on a capability in production.

## Get started

1. [Quick Start](QUICKSTART.md) — installation and first query.
2. [Development Setup](SETUP.md) — build environment and canonical CMake presets.
3. [Documentation index](docs/README.md) — user, operator, and developer guides.

For module-specific build requirements and test commands, see the
[developer entry points](INDEX.md#developer-area-entry-points).

## Project status and architecture

- [ROADMAP.md](ROADMAP.md) is the source of truth for module status, blockers,
  release readiness, and active planning.
- [CHANGELOG.md](CHANGELOG.md) records delivered changes.
- [ARCHITECTURE.md](ARCHITECTURE.md) is the generated module/dependency map;
  [TARGET_ARCHITECTURE.md](TARGET_ARCHITECTURE.md) describes the target design.
- [MODULE_INDEX.md](MODULE_INDEX.md) provides module navigation.
- [DOCUMENTATION_GOVERNANCE.md](DOCUMENTATION_GOVERNANCE.md) defines documentation
  source precedence and synchronization rules.

## Editions and releases

The canonical edition branches and branch-to-edition rules are maintained in
[BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md). Release lanes, artifacts, and
promotion gates are defined in [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md). Do not
infer production readiness from an edition name or a feature overview alone;
use the roadmap and applicable release evidence.

## Build from source

Use the repository's canonical setup guide rather than copying platform-specific
build instructions here:

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
```

Continue with [SETUP.md](SETUP.md) for dependency bootstrap, preset selection,
build, and test commands. Preset definitions are in
[CMakePresets.json](CMakePresets.json).

## Documentation map

- [Quick Start](QUICKSTART.md) and [Development Setup](SETUP.md)
- [Documentation hub](docs/README.md) and [language selector](docs/Home.md)
- [API reference](docs/api/API_REFERENCE.md)
- [Project roadmap](ROADMAP.md) and [changelog](CHANGELOG.md)
- [Root documentation index](INDEX.md)

Per-module implementation and API details belong with their code in `src/` and
`include/`; root documentation is an overview and navigation layer.

## Contributing and support

- Read [CONTRIBUTING.md](CONTRIBUTING.md) and follow the
  [Code of Conduct](CODE_OF_CONDUCT.md) before contributing.
- Ask questions in [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
  or browse [open issues](https://github.com/makr-code/ThemisDB/issues).
- Report security vulnerabilities using the process in [SECURITY.md](SECURITY.md);
  do not disclose them in public issues.
- See [SUPPORT.md](SUPPORT.md) for help and escalation paths.

## License

The licensing terms are documented in the
[license file](docs/de/legal/license.md).
