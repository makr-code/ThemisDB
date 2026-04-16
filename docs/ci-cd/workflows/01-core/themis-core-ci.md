# Themis Core Framework CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/01-core_themis-core-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Themis Core Framework**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (64 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (64 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `core-tests`
**Anzeigename:** Core tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run build-info tests** — `set -o pipefail`
- **Run wire protocol V2 tests** — `set -o pipefail`
- **Run wire protocol performance tests** — `set -o pipefail`
- **Run module sandbox / ABI checker tests** — `set -o pipefail`
- **Run module loader tests** — `set -o pipefail`
- **Run license client tests** — `set -o pipefail`
- **Run fuzz-style core tests** — `set -o pipefail`
- **Run JWT negative tests (required)** — `set -o pipefail`
- **Run RBAC negative tests (required)** — `set -o pipefail`
- **Run Vault negative tests (required)** — `set -o pipefail`
- **Run wire performance benchmarks** — `set -o pipefail`
- **Run concerns context and feature flags tests** — `set -o pipefail`
- **Run plugin health monitor tests** — `set -o pipefail`
- **Run CCPA/CPRA data subject rights tests** — `set -o pipefail`
- **Export build manifest** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🧪 Themis Core CI – Core Tests" >> "$GITHUB_STEP_SUMMARY"`

### `coverage`
**Anzeigename:** Coverage report (ubuntu-22.04 / gcc-12)

**Läuft auf:** `ubuntu-22.04`
**Abhängigkeiten:** `core-tests`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build (coverage)** — `./.github/actions/configure-themis`
- **Run all core-framework tests for coverage** — `# Core framework tests`
- **Capture lcov data** — `lcov --capture \`
- **Generate HTML report** — `genhtml coverage_filtered.info \`
- **Upload coverage report** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📊 Themis Core CI – Coverage Report" >> "$GITHUB_STEP_SUMMARY"`

### `docs-lint`
**Anzeigename:** Docs lint

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true' || needs.ci-scope-classifier.outputs.ha`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify developer guide exists** — `if [ ! -f docs/architecture/THEMIS_CORE_GUIDE.md ]; then`
- **Verify all referenced headers exist** — `declare -a headers=(`
- **Check for TODO/FIXME in new source files** — `count=$(grep -rn "TODO\|FIXME\|HACK\|XXX" \`
- **Write job summary** — `echo "## 📝 Themis Core CI – Docs Lint" >> "$GITHUB_STEP_SUMMARY"`

### `license-server-tests`
**Anzeigename:** License server tests (Python / FastAPI)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install dependencies** — `pip install -r requirements.txt pytest`
- **Run license server tests** — `python -m pytest test_license_server.py -v --tb=short`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔑 Themis Core CI – License Server Tests" >> "$GITHUB_STEP_SUMMARY"`

### `windows-compile-check`
**Anzeigename:** Windows compile-check (MSVC / x64)

**Läuft auf:** `windows-2022`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Enable Developer Command Prompt (MSVC)** — `ilammy/msvc-dev-cmd@v1`
- **Install Ninja** — `choco install ninja --no-progress -y`
- **Cache vcpkg packages** — `actions/cache@v4`
- **Bootstrap vcpkg** — `cd vcpkg`
- **Configure (CMake – modular build, COMMUNITY, Debug)** — `cmake -S cmake -B build-windows-check ``
- **Build themis_base (DLL export check)** — `cmake --build build-windows-check --target themis_base --config Debug -- -j2`
- **Write job summary** — `$summary = @"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/01-core_themis-core-ci.yml)
- [Alle Workflows](../README.md)


