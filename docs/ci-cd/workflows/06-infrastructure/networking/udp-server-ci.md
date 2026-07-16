# UDP Server (Ingestion) CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/06-infrastructure_networking_udp-server-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **UDP Server (Ingestion)**.

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

### `udp-server-tests`
**Anzeigename:** UDP Server (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (COMMUNITY edition)** — `cmake -S . -B build \`
- **Build test_udp_server_focused** — `cmake --build build \`
- **Run UDPServerFocusedTests (CTest)** — `set -o pipefail`
- **Run all UDP Server tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

### `udp-server-docs-gate`
**Anzeigename:** UDP Server — Documentation Gate

**Läuft auf:** `ubuntu-24.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify UDP Server header present** — `test -f include/network/udp_server.h && \`
- **Verify UDP Server implementation present** — `test -f src/network/udp_server.cpp && \`
- **Verify test file present** — `test -f tests/test_udp_server.cpp && \`
- **Verify TM magic bytes declared** — `grep -q "kUdpServerMagic0" include/network/udp_server.h && \`
- **Verify ingestion opcodes declared** — `grep -q "METRIC" include/network/udp_server.h && \`
- **Verify ACK support declared** — `grep -q "enable_acks" include/network/udp_server.h && \`
- **Verify batching support declared** — `grep -q "enable_batching" include/network/udp_server.h && \`
- **Verify rate limiting declared** — `grep -q "max_packets_per_second_per_ip" include/network/udp_server.h && \`
- **Verify deduplication declared** — `grep -q "dedup_window_size" include/network/udp_server.h && \`
- **Verify Stats struct for packet loss monitoring** — `grep -q "duplicate_drops" include/network/udp_server.h && \`
- **Verify UDPServer lifecycle API declared** — `grep -q "void start" include/network/udp_server.h && \`
- **Write job summary** — `echo "## 📡 UDP Ingestion Server (v1.8.0) — Documentation Gate" >> "$GITHUB_STEP_`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_networking_udp-server-ci.yml)
- [Alle Workflows](../README.md)


