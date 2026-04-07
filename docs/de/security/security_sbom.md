# SBOM (Software Bill of Materials) - ThemisDB

**Version:** v1.3.0  
**Stand:** 6. April 2026  
**Format:** SPDX 2.3 & CycloneDX 1.5  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [🔧 SBOM-Generierung](#-sbom-generierung)
- [📦 Hauptabhängigkeiten](#-hauptabhängigkeiten)
- [🔍 Vulnerability Scanning](#-vulnerability-scanning)
- [📊 SBOM-Formate](#-sbom-formate)
- [🔐 Signierung (Optional)](#-signierung-optional)
- [📋 Lizenz-Übersicht](#-lizenz-übersicht)

---

## 📋 Übersicht

Ein Software Bill of Materials (SBOM) ist ein detailliertes Inventar aller Komponenten, Bibliotheken und Abhängigkeiten einer Software. Für ThemisDB wird automatisch ein SBOM generiert, um Transparenz über die verwendeten Komponenten zu gewährleisten.

### Compliance-Anforderungen

| Anforderung | Standard | Status |
|-------------|----------|--------|
| SBOM-Generierung | BSI C5 (SSO-02) | ✅ Implementiert |
| Supply Chain Security | NIS2 Art. 21(2)(d) | ✅ Implementiert |
| Software Composition | NIST SSDF | ✅ Implementiert |
| Transparency | Executive Order 14028 | ✅ Implementiert |
| Vulnerability Tracking | NTIA Minimum Elements | ✅ Implementiert |

---

## 🔧 SBOM-Generierung

### Automatische Generierung (CI/CD)

Der SBOM wird automatisch bei jedem Release über GitHub Actions generiert:

```yaml
# .github/workflows/sbom.yml
- Trigger: push to main, tags, releases
- Tools: Syft, Grype
- Outputs: SPDX, CycloneDX
```

### Manuelle Generierung

```bash
# Syft installieren
curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh -s -- -b /usr/local/bin

# SBOM generieren (SPDX)
syft . -o spdx-json=sbom-spdx.json

# SBOM generieren (CycloneDX)
syft . -o cyclonedx-json=sbom-cyclonedx.json

# Übersicht anzeigen
syft . -o table
```

---

## 📦 Hauptabhängigkeiten

### C++ Core (vcpkg)

| Komponente | Version | Lizenz | Beschreibung |
|------------|---------|--------|--------------|
| **rocksdb** | 8.x | Apache-2.0 / GPL-2.0 | LSM-Tree Storage Engine |
| **boost** | 1.83+ | BSL-1.0 | Asio, Beast, Program Options |
| **openssl** | 3.x | Apache-2.0 | TLS, Kryptographie |
| **nlohmann-json** | 3.x | MIT | JSON Parser |
| **simdjson** | 3.x | Apache-2.0 | High-Performance JSON |
| **yaml-cpp** | 0.8+ | MIT | YAML Parser |
| **spdlog** | 1.x | MIT | Logging |
| **gtest** | 1.14+ | BSD-3-Clause | Testing Framework |
| **benchmark** | 1.x | Apache-2.0 | Benchmarking |
| **tbb** | 2021.x | Apache-2.0 | Threading Building Blocks |
| **hnsw** | - | Apache-2.0 | Vector Search |
| **msgpack** | 6.x | BSL-1.0 | Serialization |

### Admin Tools (.NET)

| Komponente | Version | Lizenz | Beschreibung |
|------------|---------|--------|--------------|
| **WPF** | .NET 8.0 | MIT | UI Framework |
| **CommunityToolkit.Mvvm** | 8.x | MIT | MVVM Framework |
| **System.Text.Json** | 8.x | MIT | JSON Serialization |

### Client SDKs

| SDK | Abhängigkeiten | Lizenz |
|-----|----------------|--------|
| **JavaScript/TypeScript** | fetch, typescript | MIT |
| **Python** | requests, httpx | MIT |
| **Rust** | reqwest, serde, tokio | MIT/Apache-2.0 |
| **Go** | net/http, encoding/json | BSD-3-Clause |
| **Java** | Gson, HttpClient | Apache-2.0 |
| **C#** | System.Net.Http, System.Text.Json | MIT |

---

## 🔍 Vulnerability Scanning

### Automatische Scans

```bash
# Grype installieren
curl -sSfL https://raw.githubusercontent.com/anchore/grype/main/install.sh | sh -s -- -b /usr/local/bin

# SBOM scannen
grype sbom:sbom-cyclonedx.json

# JSON-Output für CI/CD
grype sbom:sbom-cyclonedx.json -o json > vulnerabilities.json
```

### Schwellenwerte

| Schweregrad | Aktion |
|-------------|--------|
| **Critical** | Blockiert Release, sofortige Behebung |
| **High** | Warnung, Behebung innerhalb 7 Tagen |
| **Medium** | Dokumentation, Behebung innerhalb 30 Tagen |
| **Low** | Tracking, Behebung bei nächster Gelegenheit |

---

## 📊 SBOM-Formate

### SPDX (Software Package Data Exchange)

```json
{
  "spdxVersion": "SPDX-2.3",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "name": "themisdb",
  "documentNamespace": "https://github.com/makr-code/ThemisDB",
  "packages": [
    {
      "SPDXID": "SPDXRef-Package-rocksdb",
      "name": "rocksdb",
      "versionInfo": "8.x",
      "licenseConcluded": "Apache-2.0"
    }
  ]
}
```

### CycloneDX

```json
{
  "bomFormat": "CycloneDX",
  "specVersion": "1.5",
  "version": 1,
  "metadata": {
    "component": {
      "name": "themisdb",
      "version": "1.0.0"
    }
  },
  "components": [
    {
      "type": "library",
      "name": "rocksdb",
      "version": "8.x",
      "licenses": [{"id": "Apache-2.0"}]
    }
  ]
}
```

---

## 🔐 Signierung (Optional)

### Cosign-Signierung

```bash
# Cosign installieren
go install github.com/sigstore/cosign/v2/cmd/cosign@latest

# SBOM signieren (keyless)
cosign sign-blob --yes sbom-cyclonedx.json > sbom-cyclonedx.sig

# Signatur verifizieren
cosign verify-blob --signature sbom-cyclonedx.sig sbom-cyclonedx.json
```

---

## 📋 Lizenz-Übersicht

| Lizenz | Anzahl Komponenten | Kompatibilität |
|--------|-------------------|----------------|
| **MIT** | ~15 | ✅ Kompatibel |
| **Apache-2.0** | ~10 | ✅ Kompatibel |
| **BSD-3-Clause** | ~5 | ✅ Kompatibel |
| **BSL-1.0** | ~3 | ✅ Kompatibel |
| **GPL-2.0** (RocksDB optional) | 1 | ⚠️ Prüfung erforderlich |

### Lizenzkompatibilitätsmatrix

```
ThemisDB (MIT/Apache-2.0)
├── MIT ✅
├── Apache-2.0 ✅
├── BSD-3-Clause ✅
├── BSL-1.0 ✅
└── GPL-2.0 ⚠️ (nur wenn RocksDB unter Apache lizenziert)
```

---

## 📁 Artefakte

### Generierte Dateien

| Datei | Format | Beschreibung |
|-------|--------|--------------|
| `sbom-spdx.json` | SPDX 2.3 JSON | Vollständiges SBOM |
| `sbom-spdx.spdx` | SPDX Tag-Value | Alternative Format |
| `sbom-cyclonedx.json` | CycloneDX 1.5 JSON | Vollständiges SBOM |
| `sbom-cyclonedx.xml` | CycloneDX 1.5 XML | Alternative Format |
| `sbom-summary.txt` | Text | Übersichtstabelle |
| `vulnerability-report.json` | JSON | Grype Scan-Ergebnisse |

### Download

SBOMs werden automatisch an jedes GitHub Release angehängt:
- https://github.com/makr-code/ThemisDB/releases

---

## 🔗 Referenzen

- [SPDX Specification](https://spdx.github.io/spdx-spec/)
- [CycloneDX Specification](https://cyclonedx.org/specification/overview/)
- [NTIA SBOM Minimum Elements](https://www.ntia.doc.gov/report/2021/minimum-elements-software-bill-materials-sbom)
- [Syft Documentation](https://github.com/anchore/syft)
- [Grype Documentation](https://github.com/anchore/grype)
- [BSI C5 Criteria](https://www.bsi.bund.de/EN/Topics/CloudComputing/Compliance_Criteria_Catalogue/Compliance_Criteria_Catalogue_node.html)

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Security Team
