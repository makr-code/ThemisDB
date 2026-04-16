# Wire Protocol V2 CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_networking_wire-protocol-v2-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Wire Protocol V2**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `wire-protocol-v2-tests`
**Anzeigename:** Wire Protocol V2 (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (COMMUNITY edition)** — `cmake -S . -B build \`
- **Build test_wire_protocol_v2_focused** — `cmake --build build \`
- **Run WireProtocolV2FocusedTests (CTest)** — `set -o pipefail`
- **Run all V2 tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔌 Wire Protocol V2 CI (${{ matrix.compiler }})" >> "$GITHUB_STEP_SUMMAR`

### `wire-protocol-v2-docs-gate`
**Anzeigename:** Wire Protocol V2 — Documentation gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify protocol header is present** — `test -f include/themis/network/wire_protocol_v2.hpp && \`
- **Verify implementation source is present** — `test -f src/network/wire_protocol_v2.cpp && \`
- **Verify test file is present** — `test -f tests/test_wire_protocol_v2.cpp && \`
- **Verify multiplexing — V2Stream and stream_id field declared** — `grep -q "struct V2Stream" include/themis/network/wire_protocol_v2.hpp && \`
- **Verify server push — push_promise and push_to_client declared** — `grep -q "push_promise" include/themis/network/wire_protocol_v2.hpp && \`
- **Verify flow control — WINDOW_UPDATE frame type and update_connection_window declared** — `grep -q "WINDOW_UPDATE" include/themis/network/wire_protocol_v2.hpp && \`
- **Verify max_concurrent_streams config field declared** — `grep -q "max_concurrent_streams" include/themis/network/wire_protocol_v2.hpp && `
- **Verify priority and dependency management — PRIORITY frame + stream_dependency** — `grep -q "PRIORITY" include/themis/network/wire_protocol_v2.hpp && \`
- **Verify V2Server lifecycle API declared** — `grep -q "void start" include/themis/network/wire_protocol_v2.hpp && \`
- **Verify LZ4 and Zstd compression flags declared** — `grep -q "COMPRESSED" include/themis/network/wire_protocol_v2.hpp && \`
- **Write job summary** — `echo "## 📚 Wire Protocol V2 — Documentation Gate" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_networking_wire-protocol-v2-ci.yml)
- [Alle Workflows](../README.md)


