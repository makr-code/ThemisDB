# Themis – Sicherheits-Audit Checkliste

**Stand:** 6. April 2026  
**Version:** 1.1.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [📋 Überblick](#themis--sicherheits-audit-checkliste)
- [🧱 Architektur & Threat Modeling](#1-architektur--threat-modeling)
- [🔍 Vulnerability-Scan](#2-abhängigkeiten--vulnerability-scan)
- [🛡️ Malware-Schutz](#2a-malware-schutz-bsi-c5-ops-05-)
- [🧰 Build/Compiler-Härtung](#3-buildcompiler-härtung)
- [🔐 Authentifizierung & Autorisierung](#4-authentifizierung--autorisierung)
- [🔐 Datenverschlüsselung](#4a-datenverschlüsselung-bsi-anforderung)
- [🔒 Transport-Sicherheit](#5-transport-sicherheit)
- [🧼 Input-Validierung](#6-input-validierung--serialisierung)
- [⏱️ Ratenbegrenzung](#7-ratenbegrenzung--ressourcen-schutz)
- [📝 Logging & Audit](#8-logging--audit)
- [🔑 Secrets & Konfiguration](#9-secrets--konfiguration)
- [🛡️ Privacy & Compliance](#10-privacy--compliance)
- [🧪 Testen & Review](#11-testen--review)
- [🚀 Release-Gates](#12-release-gates)


Diese Checkliste unterstützt ein wiederholbares Sicherheits-Audit für Themis-Server und Admin-Tools.

> **📋 Vollständige Compliance-Checkliste:** Für eine umfassende Audit-Checkliste nach BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2, HIPAA und DIN-Normen siehe: **[Vollständige Audit-Checkliste](../compliance/compliance_full_checklist.md)**

## 1) Architektur & Threat Modeling
- Datenflüsse und Vertrauensgrenzen dokumentiert (Client ↔ Server ↔ Storage)
- Angriffsflächen identifiziert (HTTP-API, Admin-Tools, Datei-Importe)
- Missbrauchsfälle (Abuse Cases) erfasst (API-Scraping, DoS, unautorisierte Exporte)

## 2) Abhängigkeiten & Vulnerability-Scan
- .NET: `dotnet list package --vulnerable` in allen Tools
- C++/vcpkg: Versionen und CVEs prüfen (vcpkg-Baseline aktuell, Release Notes)
- Container-Images (falls genutzt): Trivy/Grype Scan
- Repo-Scan: `security-scan.ps1` ausführen (C/C++ Risky-APIs, Secret-Pattern, .NET Vulnerabilities)

## 2a) Malware-Schutz (BSI C5 OPS-05) ✅
- Malware-Scanner in Content-Ingestion-Pipeline aktiviert (`MalwareFilterManager`)
- SignatureScanner (built-in): PE/ELF/Mach-O, Doppelendungen, Archive-Bombs, EICAR
- ClamAV-Integration (optional): clamd-Daemon für professionelles AV-Scanning
- Dokumentation: `docs/security/security_malware_scanner.md`

## 3) Build/Compiler-Härtung
- C++: Warnings auf Maximum, Sanitizer im CI-Testlauf (ASAN/UBSAN) aktivierbar
- Windows: ASLR/DEP standardmäßig aktiv, Code Signing für EXEs/Installer
- Release-Builds reproduzierbar (vcpkg Baseline, deterministische Flags)

## 4) Authentifizierung & Autorisierung
- Admin-APIs nur für authentisierte Nutzer (Token/Bearer, mTLS oder Reverse Proxy Auth)
- Rollen/Rechte getrennt (View vs. Export vs. Löschfunktionen)
- Sensitive Aktionen (PII-Delete, Key-Rotation) auditierbar
- **Enterprise-Autorisierung (BSI-Anforderung):** Apache Ranger Integration produktionsreif (`src/server/ranger_adapter.cpp`) — Policies von zentraler Ranger-Instanz synchronisieren. Konfiguration: `docs/de/security/security_policies.md`
- RBAC/ABAC intern via `RbacManager` + optional Apache Ranger als externe Policy-Source

## 4a) Datenverschlüsselung (BSI-Anforderung)
- **Column-Level Encryption (Data-at-Rest):** `FieldEncryption` (AES-256-GCM) und `EncryptedField<T>` produktionsreif (`src/security/field_encryption.cpp`). Konfiguration: `docs/de/security/security_column_encryption.md`
- **Key Management:** `VaultKeyProvider` (HashiCorp Vault, `src/security/vault_key_provider.cpp`) und `HSMProvider PKCS#11` (`src/security/hsm_provider_pkcs11.cpp`) produktionsreif. MockKeyProvider **nur für Entwicklung/Tests**. Konfiguration: `docs/de/security/security_key_management.md`
- Key Rotation: Schlüsselrotation ohne Downtime (Dual-Write, versionierte Keys)

## 5) Transport-Sicherheit
- TLS erzwingen (Reverse Proxy wie Nginx/Traefik vor Themis-Server)
- Sichere Cipher Suites, HSTS, TLS 1.2+
- Interne Admin-Tools: Kommunikation nur über HTTPS

## 6) Input-Validierung & Serialisierung
- Strikte Schema-Validierung für JSON-Inputs
- Limitierung von Eingabegrößen (Body Size, Felder)
- Schutz gegen Path Traversal, SSRF, Open Redirects (URL/Path-Validierung)

## 7) Ratenbegrenzung & Ressourcen-Schutz
- Rate Limiting / Backoff bei teuren Endpoints (Export, Query)
- Timeouts, maximale Parallelität, Queue-Limits
- DoS-Schutz auf Proxy-Ebene

## 8) Logging & Audit
- Security-relevante Events protokolliert (Login, Export, Löschungen, Rotation)
- Log-Integrität (WORM/zentral, manipulationssicher)
- PII-Logging minimieren, keine sensiblen Daten im Klartext

## 9) Secrets & Konfiguration
- Keine Secrets im Repo (API Keys, Zertifikate) – Geheimnis-Scan
- Konfiguration via Umgebung/geschützte Stores (Windows DPAPI/KeyVault)
- Rotationspläne und Notfall-Rollback definiert

## 10) Privacy & Compliance
- Data Minimization, Zweckbindung, Löschkonzepte (Art. 17)
- Export-/Reporting-Pfade DSGVO-konform (Berechtigungen, Pseudonymisierung)
- Auftragsverarbeitung und TOMs dokumentiert

## 11) Testen & Review
- Security Code Review (C++/C#) gegen diese Checkliste
- Fuzzing-Kampagnen (Parser, Query, Import)
- Penetration-Test gegen die bereitgestellte Staging-Umgebung

## 12) Release-Gates
- Build/Lint/Tests PASS
- Vulnerability-Scan: keine kritischen offenen CVEs
- Signierte Artefakte (Code Signing), Hashes veröffentlicht
 - Geheimnis-Scan ohne Treffer (oder Findings adressiert): `security-scan.ps1`
