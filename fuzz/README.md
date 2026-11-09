---
Author: "ThemisDB Contributors"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Fuzzing Infrastructure

This directory contains the AFL++ fuzzing infrastructure for ThemisDB.

## Overview

```
fuzz/
├── aflplusplus-config.yaml     # AFL++ configuration (YAML)
├── harnesses/                  # Fuzzing harnesses (C++)
│   ├── aql_parser_harness.cpp
│   ├── gguf_loader_harness.cpp
│   ├── grammar_harness.cpp
│   ├── http_parser_harness.cpp
│   ├── jwt_rbac_config_harness.cpp
│   ├── ldap_dn_harness.cpp
│   ├── pii_redaction_harness.cpp
│   ├── postgres_importer_harness.cpp
│   ├── security_input_validator_harness.cpp
│   ├── security_policy_engine_harness.cpp
│   └── wire_protocol_parser_harness.cpp
├── corpus/                     # Seed input corpora
│   ├── aql/                   # AQL query seeds
│   ├── crypto/                # Crypto test vectors
│   ├── importer/              # Postgres importer seeds
│   ├── input_validator/       # Input validator seeds
│   ├── json/                  # JSON document seeds
│   ├── jwt/                   # JWT seeds
│   ├── ldap_dn/               # LDAP DN seeds
│   ├── llm/                   # LLM (gguf + grammar) seeds
│   ├── pii/                   # PII redaction seeds
│   ├── policy_engine/         # Policy engine seeds
│   └── rbac/                  # RBAC seeds
├── dictionaries/              # AFL++ token dictionaries
│   ├── aql.dict
│   ├── crypto.dict
│   ├── http.dict
│   ├── importer.dict
│   ├── json.dict
│   ├── jwt.dict
│   ├── ldap_dn.dict
│   ├── pii.dict
│   ├── policy_engine.dict
│   └── rbac.dict
├── crashes/                   # Crash outputs (generated at runtime)
├── hangs/                     # Hang outputs (generated at runtime)
├── coverage/                  # Coverage reports (generated at runtime)
└── reports/                   # Fuzzing reports (generated at runtime)
```

## Quick Start

### 1. Install AFL++

```bash
# Ubuntu/Debian
sudo apt-get install afl++

# From source
git clone https://github.com/AFLplusplus/AFLplusplus.git
cd AFLplusplus
make distrib
sudo make install
```

### 2. Build ThemisDB with fuzzing instrumentation

```bash
cmake -B build-fuzz \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_FUZZING=ON \
  -DCMAKE_C_COMPILER=afl-clang-lto \
  -DCMAKE_CXX_COMPILER=afl-clang-lto++
cmake --build build-fuzz --target fuzz_targets
```

### 3. Run a fuzzer

```bash
# Single target
afl-fuzz -i fuzz/corpus/aql -o fuzz/output/aql \
  -x fuzz/dictionaries/aql.dict \
  -- ./build-fuzz/fuzz/bin/aql_parser_harness @@

# Parallel fuzzing (8 cores)
afl-fuzz -M main -i fuzz/corpus/aql -o fuzz/output/aql -- ./build-fuzz/fuzz/bin/aql_parser_harness @@
afl-fuzz -S sec1  -i fuzz/corpus/aql -o fuzz/output/aql -- ./build-fuzz/fuzz/bin/aql_parser_harness @@
# ... etc.
```

## Fuzzing Targets

| Harness | Corpus | Dictionary | Priority |
|---------|--------|------------|----------|
| `aql_parser` | `corpus/aql` | `aql.dict` | High |
| `gguf_loader` | `corpus/llm/gguf` | — | High |
| `grammar` | `corpus/llm/grammar` | — | Medium |
| `http_parser` | `corpus/http` | `http.dict` | High |
| `jwt_rbac_config` | `corpus/jwt` | `jwt.dict` + `rbac.dict` | Critical |
| `ldap_dn` | `corpus/ldap_dn` | `ldap_dn.dict` | High |
| `pii_redaction` | `corpus/pii` | `pii.dict` | Critical |
| `postgres_importer` | `corpus/importer` | `importer.dict` | High |
| `wire_protocol_parser` | `corpus/wire_protocol` | — | High |
| `security_input_validator` | `corpus/input_validator` | — | Critical |
| `security_policy_engine` | `corpus/policy_engine` | `policy_engine.dict` | Critical |

## CI/CD Integration

GitHub Actions workflow: `.github/workflows/security-fuzzing.yml`

`.github/workflows/security-fuzzing.yml` additionally runs affected-parser PR
smoke tests using the AQL, HTTP, wire-protocol, and importer corpora. These runs
use sanitizer-instrumented standalone harnesses and execute every seed with a
300-second total budget per parser. The checked-in required seed inventory
detects corpus loss; no AFL findings/cache directory is restored or modified by
this deterministic smoke lane. Missing harnesses/seeds and sanitizer reports
fail the lane. The wire-protocol harness calls the pure frame parser used by
`WireProtocolWebSocketSession::processBinaryFrame`, rather than the separate
TCP packet parser or a copied test implementation.

- **Schedule:** Every Sunday at 00:00 UTC, AFL++ builds/runs the selected
  standalone AQL, importer, and wire-protocol harnesses. Legacy targets not in
  this file-mode subset are explicitly skipped and the evidence is marked
  `PARTIAL (not PASS)`; build errors for selected targets remain failures.
- **Manual trigger:** `workflow_dispatch` with a parser `target` input
- **Artifacts:** Build/CTest logs and the corpus are retained for 30 days

## Crash Analysis

### With CASR

```bash
# Install CASR
cargo install casr

# Analyse a crash
casr-gdb -o crash_report.json -- ./harness crash_input

# Cluster and triage
casr-cluster -i crashes/ -o clustered/
```

### Severity Levels

| Level | Description |
|-------|-------------|
| Exploitable | Definitively exploitable |
| Probably Exploitable | Likely exploitable |
| Probably Not Exploitable | Likely not exploitable |
| Not Exploitable | Not exploitable |

## Best Practices

1. **Sanitizers:** Use AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)
2. **Coverage-guided:** Always use coverage-guided fuzzing
3. **Dictionaries:** Use target-specific dictionaries where available
4. **Persistent mode:** Enable AFL++ persistent mode in harnesses for better throughput
5. **Parallel fuzzing:** Run multiple instances with different power schedules

## Architecture

See [harnesses/ARCHITECTURE.md](harnesses/ARCHITECTURE.md) for harness design and build requirements.

## Roadmap

See [harnesses/ROADMAP.md](harnesses/ROADMAP.md) for planned enhancements.

## References

- [AFL++ Documentation](https://aflplus.plus/)
- [AFL++ GitHub](https://github.com/AFLplusplus/AFLplusplus)
- [CASR Crash Analyzer](https://github.com/ispras/casr)
- [OSS-Fuzz](https://google.github.io/oss-fuzz/)
