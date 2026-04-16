# Security Hardening CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_security_security-hardening-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Security Hardening**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (28 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (28 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `security-negative-tests`
**Anzeigename:** Security negative tests (${{ matrix.edition }} / ${{ matrix.compiler }})

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_security_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build (${{ matrix.edition }})** — `./.github/actions/configure-themis`
- **Run JWT negative tests** — `cd build`
- **Run RBAC negative tests** — `cd build`
- **Run Vault negative tests** — `cd build`
- **Run HSM stub gating and security tests** — `cd build`
- **Run USB volume hardening tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔐 Security Hardening CI – Negative Tests" >> "$GITHUB_STEP_SUMMARY"`

### `sanitizer-tests`
**Anzeigename:** Sanitizer tests (ASan + UBSan)

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_security_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build (ASan + UBSan)** — `./.github/actions/configure-themis`
- **Run security tests under sanitizers** — `SECURITY_FILTER="\`
- **Upload sanitizer results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🧪 Security Hardening CI – Sanitizer Tests (ASan + UBSan)" >> "$GITHUB_S`

### `edition-build-check`
**Anzeigename:** Edition compile check (${{ matrix.edition }})

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_security_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build themis_core (${{ matrix.edition }})** — `./.github/actions/configure-themis`
- **Write job summary** — `echo "## 🏗️ Security Hardening CI – Edition Compile Check" >> "$GITHUB_STEP_SUMM`

### `static-analysis`
**Anzeigename:** Static analysis (clang-tidy)

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_security_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (generate compile_commands.json)** — `./.github/actions/configure-themis`
- **Run clang-tidy on security sources** — `set -o pipefail`
- **Upload clang-tidy report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔍 Security Hardening CI – Static Analysis (clang-tidy)" >> "$GITHUB_STE`

### `fuzz-smoke-test`
**Anzeigename:** Fuzz smoke test (JWT + RBAC config)

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_security_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install dependencies** — `sudo apt-get update -qq`
- **Build JWT/RBAC fuzz harness with libFuzzer** — `clang++-15 -std=c++20 \`
- **Run JWT fuzz corpus smoke-test (30 s)** — `# -max_total_time limits each run to 30 seconds – enough to catch`
- **Run RBAC fuzz corpus smoke-test (30 s)** — `./fuzz_jwt_rbac \`
- **Upload fuzz artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔥 Security Hardening CI – Fuzz Smoke Test" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_security_security-hardening-ci.yml)
- [Alle Workflows](../README.md)


