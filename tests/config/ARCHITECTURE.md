---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Test Architecture

## Purpose and data flow

The tests under `tests/config/` exercise public config interfaces and
implementation behavior using temporary files/directories, in-memory JSON/YAML,
test-local store instances, and assertions. They do not consume a shared
`policies.test.yaml`; no such file exists in the directory inventory.

Focused executables are registered in the root `tests/CMakeLists.txt`; the
per-directory `tests/config/CMakeLists.txt` has a separate target-generation
pattern. For the actual root target and CTest names, see
[`README.md`](README.md).

## Test responsibility map

| Test group | Inputs and observable behavior |
|---|---|
| Resolver | Paths/files, mapping, cache, environment and audit/metrics state |
| Schema validator | JSON/YAML strings/files, schema results and parse diagnostics |
| Watcher | Temporary directories, event delivery, callback and lifecycle |
| Encrypted store | In-memory values, rotation, serialization and crypto errors |
| Hardening | Selected path, schema, watcher and store boundary cases |
| Metrics/coverage | Export output, counters, cache, audit and supporting types |
| Migration scanner | Temporary text files, legacy references and scan/fix helpers |

## Limits

Contract-oriented test names and comments are descriptions, not proof that each
contract statement is covered. In particular, the hardening watcher suite
contains checks of declared timing constants as well as runtime behavior.
Cross-platform event delivery and consumer-level reload/apply are not
established by these test sources alone.

Sources: `tests/config/*.cpp`, `tests/CMakeLists.txt`,
`include/config/config_contract.h`, and `src/config/*.cpp`.
