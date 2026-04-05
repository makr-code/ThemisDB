# Index Compression CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_storage_index-compression-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Index Compression**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `index-compression-unit-tests`
**Anzeigename:** Index Compression tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run IndexCompressionFocusedTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🗜️ Index Compression CI (${{ matrix.compiler }})" >> "$GITHUB_STEP_SUMM`

### `index-compression-docs-gate`
**Anzeigename:** Documentation gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify implementation header is present** — `test -f include/index/index_compression.h && \`
- **Verify implementation source is present** — `test -f src/index/index_compression.cpp && \`
- **Verify test file is present** — `test -f tests/index/test_index_compression.cpp && \`
- **Verify CompressionAlgorithm enum is defined** — `grep -q "enum class CompressionAlgorithm" \`
- **Verify IndexCompressionCodec class is defined** — `grep -q "class IndexCompressionCodec" \`
- **Verify BloomFilter class is defined** — `grep -q "class BloomFilter" \`
- **Verify DictionaryCodec class is defined** — `grep -q "class DictionaryCodec" \`
- **Verify PrefixCompressor class is defined** — `grep -q "class PrefixCompressor" \`
- **Verify DeltaEncoder class is defined** — `grep -q "class DeltaEncoder" \`
- **Verify RunLengthEncoder class is defined** — `grep -q "class RunLengthEncoder" \`
- **Verify SecondaryIndexManager::Config has compression fields** — `grep -q "enable_compression" include/index/secondary_index.h && \`
- **Verify implementation is referenced in FUTURE_ENHANCEMENTS** — `grep -q "index_compression" src/index/FUTURE_ENHANCEMENTS.md && \`
- **Write job summary** — `echo "## 📚 Index Compression — Documentation Gate" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_storage_index-compression-ci.yml)
- [Alle Workflows](../README.md)


