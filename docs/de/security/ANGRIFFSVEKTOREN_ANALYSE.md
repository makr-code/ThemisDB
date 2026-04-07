# Angriffsvektoren-Analyse ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.8.0-rc1  
**Kategorie:** 🔒 Security Analysis  
**Status:** Comprehensive Review

---

## 📑 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [Änderungshistorie](#änderungshistorie)
- [Systemübersicht](#systemübersicht)
- [Externe Angriffsvektoren](#externe-angriffsvektoren)
- [Interne Angriffsvektoren](#interne-angriffsvektoren)
- [Schnittstellen-Matrix](#schnittstellen-matrix)
- [Risikobewertung](#risikobewertung)
- [Gegenmaßnahmen](#gegenmaßnahmen)
- [Monitoring & Detection](#monitoring--detection)
- [Handlungsempfehlungen](#handlungsempfehlungen)

---

## Executive Summary

Diese Analyse untersucht systematisch alle Angriffsvektoren auf ThemisDB v1.4.0-alpha, sowohl externe als auch interne. Die Analyse aktualisiert und erweitert das bestehende Threat Model (security_threat_model.md) um die seit v1.3.0 hinzugefügten Komponenten.

### Wichtigste Erkenntnisse

**🔴 Kritische Angriffsflächen:**
- 7 Netzwerkprotokolle mit unterschiedlichen Sicherheitsprofilen
- 120+ REST-API-Endpunkte (inkl. Admin-APIs)
- 10 Native Client SDKs
- LLM-Integration mit GPU-Zugriff
- gRPC-basiertes RPC-Framework für Shard-Kommunikation

**🟡 Erweiterte Angriffsfläche seit v1.3.0:**
- +85% mehr Netzwerkschnittstellen (HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL Wire, MCP)
- +200% mehr Client-Bibliotheken
- Neue LLM-Komponenten mit Dateisystemzugriff
- Voice Assistant mit Audio-Processing
- Image Analysis Plugins

**🟢 Bestehende Schutzmaßnahmen:**
- RBAC/ABAC mit Apache Ranger
- TLS 1.2/1.3 mit mTLS-Option
- AES-256-GCM Verschlüsselung
- Rate Limiting & DoS-Schutz
- Audit Logging mit Signierung
- Input Validation

---

## Änderungshistorie

| Datum | Version | Änderung | Autor |
|-------|---------|----------|-------|
| 2026-01-07 | 1.0 | Initiale umfassende Analyse für v1.4.0-alpha | Security Team |
| 2025-12-05 | 0.1 | Basis Threat Model für v1.3.0 | Security Team |

---

## Systemübersicht

### Komponenten-Architektur (v1.4.0-alpha)

```
┌─────────────────────────────────────────────────────────────────┐
│                    EXTERNE SCHNITTSTELLEN                        │
├─────────────────────────────────────────────────────────────────┤
│ HTTP/REST (8765) │ HTTP/2 │ HTTP/3 │ WebSocket │ MQTT (1883)   │
│ PostgreSQL Wire (5432) │ gRPC (50051) │ MCP Server (stdio/SSE) │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    SICHERHEITSSCHICHT                            │
├─────────────────────────────────────────────────────────────────┤
│ TLS/mTLS │ JWT/API Token Auth │ RBAC/ABAC (Ranger)             │
│ Rate Limiter │ Input Validator │ Audit Logger                   │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    ANWENDUNGSSCHICHT                             │
├─────────────────────────────────────────────────────────────────┤
│ HTTP Server │ Query Engine │ Transaction Manager                │
│ LLM Engine │ Image Analysis │ Voice Assistant (Enterprise)      │
│ Content Processor │ CDC/Changefeed │ Sharding Manager           │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    DATENSCHICHT                                  │
├─────────────────────────────────────────────────────────────────┤
│ RocksDB │ Vector Index (HNSW) │ Graph Index │ Spatial Index    │
│ Encryption (AES-256-GCM) │ Key Management (Vault/HSM)           │
└─────────────────────────────────────────────────────────────────┘
```

### Kritische Assets

1. **Daten:**
   - Dokumente, Entitäten, Vektoren, Zeitreihen, Graphen
   - PII (Personally Identifiable Information)
   - Audit-Trails und Changefeed
   - LLM-Modelle und Embeddings

2. **Schlüsselmaterial:**
   - LEK (Local Encryption Keys)
   - KEK (Key Encryption Keys)
   - DEK (Data Encryption Keys)
   - TLS-Zertifikate und Private Keys
   - HSM-Keys und PKI-Zertifikate

3. **Konfiguration:**
   - Zugangsdaten (Vault, Ranger, HSM)
   - Netzwerk-Konfiguration
   - Shard-Topologie
   - LLM-Modellpfade

4. **Infrastruktur:**
   - GPU-Ressourcen (CUDA)
   - Dateisystem-Zugriff
   - Netzwerk-Ports
   - Docker/Container-Umgebung

---

## Externe Angriffsvektoren

### 1. Netzwerkschnittstellen

#### 1.1 HTTP/REST API (Port 8765)

**Angriffsfläche:** 120+ Endpunkte

**Kritische Endpunkte:**

| Endpunkt | Methode | Risiko | Beschreibung |
|----------|---------|--------|--------------|
| `/admin/backup` | POST | 🔴 Kritisch | Backup-Erstellung, Directory Traversal |
| `/admin/restore` | POST | 🔴 Kritisch | Restore-Funktion, Code Injection |
| `/api/v1/wal/apply` | POST | 🔴 Kritisch | WAL-Replay, Privilege Escalation |
| `/api/pki/hsm/sign` | POST | 🔴 Kritisch | HSM-Signierung, Key Abuse |
| `/api/keys/rotate` | POST | 🔴 Kritisch | Schlüsselrotation, DoS |
| `/entities/:key` | GET/PUT/DELETE | 🟡 Hoch | CRUD-Operationen, SQL Injection |
| `/query/aql` | POST | 🟡 Hoch | AQL-Queries, Injection |
| `/api/pii/reveal` | GET | 🟡 Hoch | PII-Offenlegung |
| `/changefeed/retention` | POST | 🟡 Hoch | Audit-Trail Manipulation |

**Angriffsvektoren:**

1. **Injection-Angriffe:**
   - **AQL Injection:** Manipulierte Query-Parameter
   - **NoSQL Injection:** Document-Key Manipulation
   - **Path Traversal:** In Content-Import/Backup-Pfaden
   - **Command Injection:** In Admin-Funktionen

2. **Authentifizierungs-/Autorisierungsangriffe:**
   - **Token Theft:** Abfangen von API-Tokens
   - **JWT Bypass:** Schwache Signatur-Algorithmen
   - **Privilege Escalation:** Role-Manipulation
   - **Session Hijacking:** WebSocket/SSE-Sessions

3. **DoS-Angriffe:**
   - **Rate Limit Bypass:** Verteilte Requests
   - **Resource Exhaustion:** Große Batch-Operationen
   - **Slowloris:** Langanhaltende Connections
   - **Query Complexity:** Komplexe AQL-Queries

4. **Datenexfiltration:**
   - **Bulk Export:** Massen-Download via `/api/audit/export/csv`
   - **Changefeed Streaming:** Kontinuierliche Datenextraktion
   - **PII Leakage:** Nicht-pseudonymisierte Logs
   - **Backup Theft:** Unverschlüsselte Backups

#### 1.2 HTTP/2 mit Server Push

**Angriffsfläche:** CDC/Changefeed Push

**Spezifische Risiken:**
- **Server Push Abuse:** Unkontrollierte Push-Streams
- **Stream Multiplexing DoS:** Ressourcen-Erschöpfung
- **Header Compression Attacks:** HPACK Bombs
- **Stream Priority Manipulation:** QoS-Bypass

#### 1.3 HTTP/3 (QUIC)

**Angriffsfläche:** Experimentelles Protokoll

**Spezifische Risiken:**
- **QUIC Implementation Bugs:** Neue Codebasis
- **UDP Amplification:** DDoS-Vektor
- **Connection Migration Attacks:** Session-Hijacking
- **0-RTT Replay Attacks:** Nonce-Wiederverwendung

#### 1.4 WebSocket (Port 8765)

**Angriffsfläche:** CDC Streaming, Echtzeit-Kommunikation

**Spezifische Risiken:**
- **WebSocket Hijacking:** Fehlende Origin-Prüfung
- **Message Injection:** Manipulierte Frames
- **Ping/Pong Flooding:** DoS
- **Connection Exhaustion:** Offene Connections

#### 1.5 MQTT Broker (Port 1883)

**Angriffsfläche:** IoT-Geräte, Pub/Sub

**Spezifische Risiken:**
- **Topic Hijacking:** Unzureichende ACLs
- **Retained Message Poisoning:** Persistente Angriffe
- **Will Message Abuse:** Fake Disconnect-Nachrichten
- **QoS 2 DoS:** Acknowledgment-Flooding

#### 1.6 PostgreSQL Wire Protocol (Port 5432)

**Angriffsfläche:** BI-Tools, SQL-Clients

**Spezifische Risiken:**
- **SQL-to-AQL Translation Bypass:** Injection
- **Protocol Confusion:** Fehlinterpretation
- **Authentication Downgrade:** Plain-Text Passwords
- **Copy Command Abuse:** Dateisystem-Zugriff

#### 1.7 gRPC (Port 50051)

**Angriffsfläche:** Shard-Kommunikation, LLM-Service

**Spezifische Risiken:**
- **gRPC Metadata Injection:** Header-Manipulation
- **Stream Flooding:** Bidirektionale Streams
- **Protobuf Deserialization:** Memory Exhaustion
- **mTLS Certificate Spoofing:** Fake Shards

**Dienste:**
- `themis_core.proto`: Core Themis Operations
- `llm_service.proto`: LLM Inference Requests
- `shard_rpc.proto`: Inter-Shard Communication

#### 1.8 MCP Server (Model Context Protocol)

**Angriffsfläche:** AI-Agent Integration

**Transporte:**
- **stdio:** Lokaler Prozess
- **SSE (Server-Sent Events):** HTTP-basiert
- **WebSocket:** Bidirektional

**Spezifische Risiken:**
- **Prompt Injection:** Manipulierte Context
- **Tool Abuse:** Unkontrollierte Tool-Aufrufe
- **Resource Access:** Dateisystem-Zugriff via MCP
- **Context Poisoning:** Manipulierte Conversation History

### 2. Client SDKs & Bibliotheken

**Angriffsfläche:** 10 Native Client-Bibliotheken

| SDK | Sprache | Risiken |
|-----|---------|---------|
| `clients/csharp` | C# | Deserialization, Reflection |
| `clients/go` | Go | Memory Safety (Cgo) |
| `clients/java` | Java | Deserialization, RCE |
| `clients/javascript` | JS/Node | Prototype Pollution, XSS |
| `clients/php` | PHP | Code Injection |
| `clients/python` | Python | Code Injection, Pickle |
| `clients/ruby` | Ruby | YAML Deserialization |
| `clients/rust` | Rust | Unsafe Code Blocks |
| `clients/swift` | Swift | Memory Corruption |
| `clients/typescript` | TS | Type Confusion |

**Gemeinsame Risiken:**
- **Dependency Vulnerabilities:** Third-Party Libraries
- **Hardcoded Credentials:** API-Tokens im Code
- **Insufficient TLS Validation:** MITM-Angriffe
- **Logging Sensitive Data:** Credentials in Logs

### 3. Adapter & Integrationen

**Angriffsfläche:** 4 Adapter-Komponenten

| Adapter | Zweck | Risiken |
|---------|-------|---------|
| `adapters/covina_fastapi_ingestion` | FastAPI Data Ingestion | API Abuse, Rate Limit |
| `adapters/vcc_base` | Base VCC Adapter | N/A |
| `adapters/vcc_clara_ingestion` | Clara Ingestion | Data Validation |
| `adapters/vcc_veritas` | Veritas Integration | Auth Bypass |

---

## Interne Angriffsvektoren

### 1. Insider-Bedrohungen

#### 1.1 Berechtigte Administratoren

**Angriffsszenario:** Böswilliger oder kompromittierter Admin

**Mögliche Angriffe:**
1. **Datenexfiltration:**
   - Bulk-Export über `/admin/backup`
   - Changefeed-Streaming ohne Audit
   - Direct RocksDB-Zugriff (Dateisystem)

2. **Datenmanipulation:**
   - WAL-Replay mit manipulierten Logs
   - Direct Database-Änderungen
   - Audit-Trail-Manipulation via `/changefeed/retention`

3. **Schlüssel-Diebstahl:**
   - KEK-Extraktion via `/api/keys/list`
   - HSM-Key-Missbrauch via `/api/pki/hsm/sign`
   - Vault-Token-Diebstahl

4. **Denial-of-Service:**
   - Index-Rebuild ohne Reason
   - Unkontrollierte Shard-Migration
   - Resource Exhaustion (LLM-Modelle laden)

#### 1.2 Entwickler mit Code-Zugriff

**Angriffsszenario:** Böswilliger Entwickler committet Backdoor

**Mögliche Angriffe:**
1. **Code Injection:**
   - Backdoor in C++-Code
   - Malicious Dependencies (vcpkg, npm)
   - Supply Chain Attack (llama.cpp)

2. **Credential Leakage:**
   - Hardcoded API-Tokens
   - Vault-Credentials in Config
   - Secrets in Git History

3. **Logic Bombs:**
   - Zeitgesteuerte Malware
   - Trigger-basierte Datenexfiltration

#### 1.3 Betriebspersonal (DevOps/SRE)

**Angriffsszenario:** Kompromittierter Operator

**Mögliche Angriffe:**
1. **Infrastruktur-Manipulation:**
   - Docker-Container-Manipulation
   - Kubernetes-Secret-Diebstahl
   - Netzwerk-Traffic-Sniffing

2. **Monitoring-Bypass:**
   - Grafana-Dashboard-Manipulation
   - Log-Deletion
   - Alert-Suppression

### 2. Privilege Escalation

#### 2.1 Horizontale Privilege Escalation

**Szenario:** Normaler User greift auf Daten anderer User zu

**Vektoren:**
- **Entity-Key-Guessing:** Vorhersagbare Keys
- **Changefeed-Filter-Bypass:** Fehlende Row-Level Security
- **Query-Injection:** Cross-Tenant-Zugriff

#### 2.2 Vertikale Privilege Escalation

**Szenario:** User erlangt Admin-Rechte

**Vektoren:**
- **RBAC-Bypass:** Fehler in Policy-Engine
- **JWT-Role-Injection:** Token-Manipulation
- **Ranger-Policy-Manipulation:** Admin-API-Missbrauch
- **Session-Hijacking:** Admin-Session-Token-Diebstahl

### 3. Seitenkanalangriffe

#### 3.1 Timing-Angriffe

**Ziel:** Schlüssel oder Daten erraten

**Vektoren:**
- **Authentication Timing:** User-Enumeration
- **Query Timing:** Datenstruktur-Inferenz
- **Encryption Timing:** Key-Recovery (bei fehlender Constant-Time Crypto)

#### 3.2 Cache-Angriffe

**Ziel:** Sensitive Daten aus Caches extrahieren

**Vektoren:**
- **Semantic Cache Poisoning:** Manipulierte Embeddings
- **Query Cache Timing:** Daten-Leakage
- **LLM Prefix Cache:** Prompt-Reconstruction

### 4. LLM-spezifische Angriffe

#### 4.1 Prompt Injection

**Szenario:** Manipulierte Prompts an LLM-Engine

**Vektoren:**
- **Direct Injection:** In User-Queries
- **Indirect Injection:** Via gespeicherte Dokumente
- **Jailbreaking:** Sicherheits-Guard-Bypass

**Auswirkungen:**
- Unberechtigter Datenzugriff
- Code-Ausführung (bei Agent-Features)
- PII-Leakage

#### 4.2 Model Extraction

**Szenario:** Diebstahl des LLM-Modells

**Vektoren:**
- **Filesystem-Zugriff:** Direct Model-File-Read
- **API-Queries:** Modell-Reverse-Engineering
- **Memory Dumps:** GPU-Memory-Extraktion

#### 4.3 Adversarial Inputs

**Szenario:** Speziell gestaltete Inputs

**Vektoren:**
- **Embedding Manipulation:** Fake Vector-Similarity
- **Image Analysis Poisoning:** Adversarial Images
- **Voice Assistant Attacks:** Audio-Adversarial-Examples

### 5. Sharding & Replication Angriffe

#### 5.1 Split-Brain-Szenarien

**Szenario:** Netzwerkpartitionierung führt zu inkonsistentem Zustand

**Risiken:**
- Daten-Divergenz zwischen Shards
- Konflikt-Resolution-Exploits
- Write-Concern-Bypass

#### 5.2 Shard-Spoofing

**Szenario:** Fake-Shard registriert sich beim Coordinator

**Vektoren:**
- **gRPC mTLS-Bypass:** Certificate-Spoofing
- **Health-Check-Manipulation:** Fake Metrics
- **Shard-Metadata-Injection:** Topology-Manipulation

---

## Schnittstellen-Matrix

### Netzwerk-Endpunkte (Übersicht)

| Protokoll | Port | TLS | Auth | Rate Limit | Audit | Risiko |
|-----------|------|-----|------|------------|-------|--------|
| HTTP/REST | 8765 | ✅ Opt | ✅ JWT/Token | ✅ Token Bucket | ✅ | 🟡 Hoch |
| HTTP/2 | 8765 | ✅ Req | ✅ JWT/Token | ✅ | ✅ | 🟡 Hoch |
| HTTP/3 | 8765 | ✅ Req | ✅ JWT/Token | ⚠️ Partial | ⚠️ | 🔴 Kritisch |
| WebSocket | 8765 | ✅ Opt | ✅ JWT/Token | ⚠️ | ✅ | 🟡 Hoch |
| MQTT | 1883 | ✅ Opt | ✅ | ✅ | ⚠️ | 🟡 Hoch |
| PostgreSQL | 5432 | ✅ Opt | ✅ | ⚠️ | ✅ | 🟡 Hoch |
| gRPC | 50051 | ✅ mTLS | ✅ mTLS | ✅ | ✅ | 🟢 Mittel |
| MCP (stdio) | N/A | N/A | ⚠️ | N/A | ⚠️ | 🟡 Hoch |
| MCP (SSE) | 8765 | ✅ | ✅ | ✅ | ⚠️ | 🟡 Hoch |
| MCP (WS) | 8765 | ✅ | ✅ | ✅ | ⚠️ | 🟡 Hoch |

**Legende:**
- ✅ = Implementiert/Verfügbar
- ⚠️ = Teilweise/Optional
- ❌ = Nicht implementiert
- N/A = Nicht anwendbar

### Admin-API-Endpunkte (Kritisch)

| Endpunkt | Funktion | Ranger-Resource | Risiko | Gegenmaßnahme |
|----------|----------|-----------------|--------|---------------|
| `/admin/backup` | Backup erstellen | `admin.backup` | 🔴 | Directory Traversal Protection |
| `/admin/restore` | Restore durchführen | `admin.restore` | 🔴 | Signature-Verification |
| `/api/v1/wal/apply` | WAL-Replay | `wal.apply` | 🔴 | Strong Authentication |
| `/api/keys/rotate` | Schlüssel rotieren | `keys.rotate` | 🔴 | Multi-Factor Auth |
| `/api/pki/hsm/sign` | HSM-Signierung | `pki.hsm.sign` | 🔴 | Audit + Rate Limit |
| `/changefeed/retention` | Audit-Retention ändern | `cdc.admin` | 🔴 | Immutable Logs |
| `/api/pii/reveal` | PII offenlegen | `pii.reveal` | 🔴 | Purpose Limitation |
| `/entities/batch` | Batch-Operation | `entities.batch` | 🟡 | Size Limits |

### Client-Library-Risiken

| Client | Language | CVE History | Supply Chain Risk | Empfehlung |
|--------|----------|-------------|-------------------|------------|
| csharp | C# | ⚠️ Mittel | 🟢 NuGet | SBOM + Dependency Scanning |
| go | Go | 🟢 Niedrig | 🟢 Go Modules | Minimal Dependencies |
| java | Java | 🔴 Hoch | 🔴 Maven Central | Snyk/Dependabot |
| javascript | JS | 🔴 Hoch | 🔴 npm | npm audit + SRI |
| php | PHP | 🔴 Hoch | 🟡 Composer | Roave Security |
| python | Python | 🟡 Mittel | 🟡 PyPI | pip-audit + Safety |
| ruby | Ruby | 🟡 Mittel | 🟡 RubyGems | Bundle Audit |
| rust | Rust | 🟢 Niedrig | 🟢 Cargo | Cargo Audit |
| swift | Swift | 🟢 Niedrig | 🟢 SPM | Minimal |
| typescript | TS | 🔴 Hoch | 🔴 npm | npm audit + SRI |

---

## Risikobewertung

### STRIDE-Analyse

| Kategorie | Bedrohung | Beispiel | Likelihood | Impact | Risk Score |
|-----------|-----------|----------|------------|--------|------------|
| **S**poofing | Shard-Spoofing | Fake gRPC Shard | 🟡 Mittel | 🔴 Hoch | 🔴 8/10 |
| **T**ampering | Audit-Trail-Manipulation | Changefeed-Deletion | 🟢 Niedrig | 🔴 Hoch | �� 6/10 |
| **R**epudiation | Non-Audited Actions | Fehlende Signierung | 🟡 Mittel | 🟡 Mittel | 🟡 5/10 |
| **I**nfo Disclosure | PII-Leakage | Unverschlüsselte Logs | 🟡 Mittel | 🔴 Hoch | 🔴 7/10 |
| **D**enial of Service | Query Complexity DoS | AQL Injection | 🔴 Hoch | 🟡 Mittel | 🔴 7/10 |
| **E**levation of Privilege | RBAC-Bypass | Policy-Engine-Bug | 🟡 Mittel | 🔴 Hoch | 🔴 8/10 |

### CVSS v3.1 Risk Ratings

**Top 10 Risiken:**

1. **Ungeschützte Admin-APIs (CVSS 9.1 - Critical)**
   - Vector: AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:N
   - Ohne Authentifizierung: `/admin/backup`, `/admin/restore`

2. **LLM Prompt Injection (CVSS 8.6 - High)**
   - Vector: AV:N/AC:L/PR:L/UI:N/S:C/C:H/I:L/A:N
   - Zugriff auf sensitive Daten via Prompt-Manipulation

3. **Shard-Spoofing via gRPC (CVSS 8.3 - High)**
   - Vector: AV:N/AC:H/PR:N/UI:N/S:C/C:H/I:H/A:N
   - Fake-Shard registriert sich im Cluster

4. **AQL Injection (CVSS 8.1 - High)**
   - Vector: AV:N/AC:L/PR:L/UI:N/S:U/C:H/I:H/A:N
   - Code-Injection in Query-Engine

5. **HSM-Key-Missbrauch (CVSS 7.8 - High)**
   - Vector: AV:N/AC:L/PR:H/UI:N/S:U/C:H/I:H/A:H
   - Signierung beliebiger Daten

6. **JWT-Secret-Leak (CVSS 7.5 - High)**
   - Vector: AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:N/A:N
   - Token-Fälschung

7. **WebSocket-Hijacking (CVSS 7.4 - High)**
   - Vector: AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:L/A:N
   - Session-Takeover

8. **Dependency Vulnerabilities (CVSS 7.3 - High)**
   - Vector: AV:N/AC:L/PR:N/UI:N/S:U/C:L/I:L/A:L
   - Third-Party Library CVEs

9. **Backup-Theft (CVSS 7.1 - High)**
   - Vector: AV:L/AC:L/PR:L/UI:N/S:U/C:H/I:H/A:N
   - Unverschlüsselte Backups

10. **Insider Data Exfiltration (CVSS 6.9 - Medium)**
    - Vector: AV:N/AC:L/PR:H/UI:N/S:U/C:H/I:H/A:N
    - Berechtigter Admin missbraucht Zugriff

---

## Gegenmaßnahmen

(Content continues...)

### 1. Netzwerksicherheit

#### 1.1 TLS/mTLS-Härtung

**Maßnahmen:**
```yaml
# config/tls.yaml
tls:
  min_version: "1.3"
  cipher_suites:
    - TLS_AES_256_GCM_SHA384
    - TLS_CHACHA20_POLY1305_SHA256
  enable_mtls: true
  client_cert_required: true  # Für gRPC
  cert_pinning: true          # Für HSM/TSA
```

**Implementierung:**
- ✅ TLS 1.2/1.3 Support (OpenSSL)
- ✅ mTLS für gRPC
- ⚠️ Certificate Pinning (nur HSM/TSA)
- ❌ Mutual TLS für HTTP/REST (optional)

#### 1.2 Rate Limiting & DoS-Schutz

**Maßnahmen:**
```yaml
# config/rate_limits.yaml
rate_limits:
  global:
    requests_per_minute: 100
    burst_size: 20
  per_ip:
    enabled: true
    requests_per_minute: 50
  per_user:
    enabled: true
    requests_per_minute: 100
  endpoints:
    - path: /admin/*
      requests_per_minute: 10
    - path: /api/keys/rotate
      requests_per_minute: 1
    - path: /query/aql
      requests_per_minute: 30
```

**Implementierung:**
- ✅ Token Bucket Algorithm (rate_limiter.cpp)
- ✅ Per-IP Rate Limiting
- ✅ Per-User Rate Limiting
- ⚠️ Adaptive Rate Limiting (planned)

#### 1.3 Input Validation

**Maßnahmen:**
```cpp
// src/utils/input_validator.cpp
class InputValidator {
public:
    // Path Traversal Protection
    static bool validatePath(const std::string& path);
    
    // AQL Injection Protection
    static bool validateAQL(const std::string& query);
    
    // Entity Key Validation
    static bool validateKey(const std::string& key);
    
    // JSON Schema Validation
    static bool validateJson(const json& data, const json& schema);
};
```

**Implementierung:**
- ✅ JSON Schema Validation
- ✅ Path Traversal Protection
- ⚠️ AQL Injection Detection (basic)
- ❌ Advanced SQL Injection Detection

### 2. Authentifizierung & Autorisierung

#### 2.1 Multi-Factor Authentication (MFA)

**Empfehlung:** MFA für Admin-APIs

**Implementierung (geplant):**
```yaml
# config/auth.yaml
authentication:
  mfa:
    enabled: true
    required_for:
      - /admin/*
      - /api/keys/rotate
      - /api/pki/hsm/*
    methods:
      - totp
      - webauthn
```

**Status:** ❌ Nicht implementiert (Empfehlung für v1.5.0)

#### 2.2 RBAC-Härtung

**Maßnahmen:**
1. **Principle of Least Privilege:**
   - Minimale Berechtigungen für Service-Accounts
   - Zeitlich begrenzte Admin-Tokens

2. **Policy-Review:**
   - Regelmäßige Audit der Ranger-Policies
   - Removal of unused Permissions

3. **Separation of Duties:**
   - Backup-Operator ≠ Restore-Operator
   - Key-Rotation-Operator ≠ Key-Usage-Operator

**Implementierung:**
```yaml
# config/policies.yaml
policies:
  - name: "Backup Operator"
    subjects: ["backup-service"]
    actions: ["backup:create"]
    resources: ["/admin/backup"]
    effect: allow
    
  - name: "Key Rotation Operator"
    subjects: ["key-rotation-service"]
    actions: ["keys:rotate"]
    resources: ["/api/keys/rotate"]
    effect: allow
    conditions:
      - type: time_window
        start: "02:00"
        end: "04:00"
```

### 3. Audit & Monitoring

#### 3.1 Umfassendes Audit-Logging

**Maßnahmen:**
```cpp
// 65+ Security Event Types
enum class AuditEventType {
    // Authentication
    AUTH_LOGIN_SUCCESS,
    AUTH_LOGIN_FAILURE,
    AUTH_LOGOUT,
    AUTH_TOKEN_CREATED,
    AUTH_TOKEN_REVOKED,
    
    // Authorization
    AUTHZ_POLICY_CHECKED,
    AUTHZ_ACCESS_DENIED,
    
    // Data Access
    DATA_READ,
    DATA_WRITE,
    DATA_DELETE,
    DATA_EXPORT,
    
    // Admin Actions
    ADMIN_BACKUP_CREATED,
    ADMIN_RESTORE_PERFORMED,
    ADMIN_KEY_ROTATED,
    ADMIN_CONFIG_CHANGED,
    
    // Security Events
    SECURITY_INJECTION_ATTEMPT,
    SECURITY_RATE_LIMIT_EXCEEDED,
    SECURITY_INVALID_TOKEN,
    SECURITY_PRIVILEGE_ESCALATION_ATTEMPT
};
```

**Features:**
- ✅ Encrypt-then-Sign
- ✅ Hash Chain (Tamper Detection)
- ✅ SIEM Integration (Syslog RFC 5424, Splunk HEC)
- ⚠️ Real-Time Alerting (via Grafana)

#### 3.2 Anomalie-Erkennung

**Empfohlene Metriken:**
```yaml
# Grafana Alerts
alerts:
  - name: "Unusual API Call Pattern"
    query: rate(http_requests_total[5m]) > 1000
    severity: warning
    
  - name: "Multiple Failed Auth Attempts"
    query: increase(auth_failures_total[1m]) > 10
    severity: critical
    
  - name: "Admin API After Hours"
    query: http_requests_total{path="/admin/*", hour!~"02-04"}
    severity: warning
    
  - name: "Large Batch Operations"
    query: histogram_quantile(0.99, batch_size_bucket) > 10000
    severity: warning
```

### 4. LLM-Sicherheit

#### 4.1 Prompt Injection Schutz

**Maßnahmen:**
1. **Input Sanitization:**
   ```cpp
   // src/llm/prompt_sanitizer.cpp
   class PromptSanitizer {
   public:
       static std::string sanitize(const std::string& user_input) {
           // Remove instruction keywords
           // Escape special tokens
           // Limit length
       }
   };
   ```

2. **Output Filtering:**
   - PII-Detection in LLM-Antworten
   - Secret-Scanning (API-Keys, Passwords)

3. **Sandboxing:**
   - LLM in separater Docker-Container
   - Eingeschränkte Filesystem-Zugriffe
   - Resource Limits (GPU, Memory)

#### 4.2 Model Access Control

**Maßnahmen:**
```yaml
# config/llm_security.yaml
llm:
  model_access:
    - model: "llama-3-70b"
      allowed_users: ["premium-tier"]
      max_tokens: 4096
      
    - model: "phi-3-mini"
      allowed_users: ["*"]
      max_tokens: 2048
      
  guardrails:
    - type: "pii_detection"
      enabled: true
    - type: "toxicity_filter"
      enabled: true
      threshold: 0.7
```

### 5. Supply Chain Security

#### 5.1 Dependency Scanning

**Tools:**
- **C++ Dependencies:** vcpkg + OSV Scanner
- **JavaScript/TypeScript:** npm audit + Snyk
- **Python:** pip-audit + Safety
- **Java:** OWASP Dependency-Check

**CI/CD Integration:**
```yaml
# .github/workflows/security.yml
name: Security Scan
on: [push, pull_request]
jobs:
  dependency-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: OSV Scanner (C++)
        run: osv-scanner --lockfile=vcpkg.json
      - name: npm audit (JS)
        run: cd clients/javascript && npm audit
      - name: Snyk (All)
        run: snyk test --all-projects
```

#### 5.2 SBOM Generation

**Maßnahmen:**
```bash
# Generate Software Bill of Materials
syft packages dir:. -o cyclonedx-json > sbom.json
grype sbom:sbom.json
```

**Implementierung:**
- ✅ SBOM-Dokumentation (docs/security/security_sbom.md)
- ❌ Automatische SBOM-Generierung (geplant)

### 6. Sharding & Replication Security

#### 6.1 mTLS für Shard-Kommunikation

**Maßnahmen:**
```yaml
# config/sharding.yaml
sharding:
  rpc:
    tls:
      enabled: true
      mutual_tls: true
      client_cert: /certs/shard-client.crt
      client_key: /certs/shard-client.key
      ca_cert: /certs/ca.crt
      verify_client: true
```

**Implementierung:**
- ✅ gRPC mit mTLS
- ✅ Certificate-based Authentication
- ⚠️ Certificate Rotation (manuell)

#### 6.2 Write-Concern Enforcement

**Maßnahmen:**
```cpp
// src/sharding/write_concern.h
enum class WriteConcern {
    W1,           // Primary only
    W_MAJORITY,   // Majority of replicas
    W_ALL         // All replicas
};

// Enforce for sensitive operations
if (is_admin_operation) {
    write_concern = WriteConcern::W_ALL;
}
```

---

## Monitoring & Detection

### 1. Security Event Monitoring

**Kritische Events zum Überwachen:**

| Event | Schwellwert | Aktion |
|-------|-------------|--------|
| `AUTH_LOGIN_FAILURE` | > 5 in 1 min | Block IP |
| `AUTHZ_ACCESS_DENIED` | > 10 in 5 min | Alert SOC |
| `SECURITY_INJECTION_ATTEMPT` | > 0 | Alert + Block |
| `ADMIN_KEY_ROTATED` | > 1 per day | Alert + Review |
| `DATA_EXPORT` (Bulk) | > 10k rows | Alert + Review |
| `SECURITY_RATE_LIMIT_EXCEEDED` | > 100 in 1 min | Temporary Ban |

### 2. Grafana Dashboards

**Empfohlene Dashboards:**

1. **Security Overview:**
   - Authentication Success/Failure Rate
   - Authorization Denials
   - Rate Limit Hits
   - Active Sessions

2. **Admin Activity:**
   - Backup/Restore Operations
   - Key Rotations
   - Config Changes
   - User Management

3. **Data Access:**
   - Entity CRUD Operations
   - Query Patterns
   - Export Operations
   - PII Access

4. **Network Traffic:**
   - Requests per Protocol
   - Error Rates
   - Latency Percentiles
   - Connection Count

### 3. SIEM Integration

**Syslog RFC 5424:**
```cpp
// src/utils/audit_logger.cpp
void AuditLogger::logToSyslog(const AuditEvent& event) {
    // <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD MSG
    std::string syslog_msg = fmt::format(
        "<{}>1 {} {} themisdb {} {} [security] {}",
        facility * 8 + severity,
        iso8601_timestamp(),
        hostname(),
        event.type,
        event.id,
        event.toJson()
    );
    sendto(syslog_socket_, syslog_msg);
}
```

**Splunk HEC:**
```cpp
void AuditLogger::logToSplunk(const AuditEvent& event) {
    json hec_event = {
        {"time", event.timestamp},
        {"host", hostname()},
        {"source", "themisdb"},
        {"sourcetype", "security_audit"},
        {"event", event.toJson()}
    };
    httpClient_.post(splunk_hec_url_, hec_event);
}
```

---

## Handlungsempfehlungen

### Sofortmaßnahmen (P0)

1. **✅ Admin-API-Schutz:**
   - [ ] MFA für `/admin/*` Endpunkte aktivieren
   - [ ] IP-Whitelisting für Admin-APIs
   - [ ] Separate Admin-Port (z.B. 8766) mit restricted Access

2. **✅ TLS-Enforcement:**
   - [ ] TLS 1.3 als Minimum festlegen
   - [ ] Weak Cipher Suites deaktivieren
   - [ ] mTLS für alle gRPC-Verbindungen

3. **✅ Rate Limiting:**
   - [ ] Aggressive Rate Limits für `/api/keys/rotate`
   - [ ] Query Complexity Limits für AQL
   - [ ] Batch Size Limits für `/entities/batch`

4. **✅ Audit-Log-Schutz:**
   - [ ] Immutable Audit Logs (WORM-Storage)
   - [ ] Real-Time Backup von Audit-Trails
   - [ ] Alert bei Changefeed-Retention-Änderungen

### Kurzfristig (1-3 Monate)

5. **⚠️ LLM-Sicherheit:**
   - [ ] Prompt Injection Detection implementieren
   - [ ] LLM Output Filtering (PII, Secrets)
   - [ ] Resource Limits für LLM-Inferenz

6. **⚠️ Input Validation:**
   - [ ] Advanced AQL Injection Detection
   - [ ] Path Traversal Protection für alle File-Operations
   - [ ] JSON Schema Validation für alle API-Requests

7. **⚠️ Dependency Management:**
   - [ ] Automatische Dependency Scanning in CI/CD
   - [ ] Vulnerability Alerting (GitHub Dependabot)
   - [ ] SBOM-Generierung automatisieren

8. **⚠️ Client-Library-Sicherheit:**
   - [ ] Security Guidelines für Client-Entwickler
   - [ ] Code-Signing für Release-Artifacts
   - [ ] Vulnerability Disclosure für Clients

### Mittelfristig (3-6 Monate)

9. **❌ Zero-Trust-Architektur:**
   - [ ] mTLS für alle Protokolle (nicht nur gRPC)
   - [ ] Service Mesh Integration (Istio/Linkerd)
   - [ ] Network Segmentation (VLAN/Firewall)

10. **❌ Advanced Threat Detection:**
    - [ ] Machine Learning-basierte Anomalie-Erkennung
    - [ ] Behavioral Analytics (UEBA)
    - [ ] Automated Incident Response

11. **❌ Compliance & Certification:**
    - [ ] SOC 2 Type II Audit
    - [ ] ISO 27001 Zertifizierung
    - [ ] BSI C5 Attestierung

12. **❌ Penetration Testing:**
    - [ ] External Pentest (jährlich)
    - [ ] Internal Pentest (halbjährlich)
    - [ ] Bug Bounty Program

### Langfristig (6-12 Monate)

13. **❌ Security Hardening:**
    - [ ] AppArmor/SELinux Profiles
    - [ ] Kernel Security Modules
    - [ ] Secure Boot + TPM

14. **❌ Post-Quantum Cryptography:**
    - [ ] PQC-Ready Key Exchange (Kyber)
    - [ ] Hybrid TLS (Classic + PQC)
    - [ ] Migration Plan für bestehende Keys

15. **❌ Distributed Tracing Security:**
    - [ ] OpenTelemetry mit Security Context
    - [ ] Trace-basierte Anomalie-Erkennung
    - [ ] Automated Attack Path Analysis

---

## Anhang

### A. Referenzen

**Interne Dokumentation:**
- [security_overview.md](security_overview.md)
- [security_threat_model.md](security_threat_model.md)
- [security_hardening.md](security_hardening.md)
- [security_audit_checklist.md](security_audit_checklist.md)

**Standards & Frameworks:**
- OWASP Top 10 (2021)
- NIST Cybersecurity Framework
- CIS Benchmarks
- MITRE ATT&CK

**CVE-Datenbanken:**
- NVD (National Vulnerability Database)
- OSV (Open Source Vulnerabilities)
- GitHub Advisory Database

### B. Glossar

| Begriff | Definition |
|---------|------------|
| **AQL** | Analytics Query Language (ThemisDB-eigene Query-Sprache) |
| **CDC** | Change Data Capture (Changefeed) |
| **HSM** | Hardware Security Module |
| **KEK** | Key Encryption Key |
| **LEK** | Local Encryption Key |
| **mTLS** | Mutual TLS (bidirektionale Zertifikat-Authentifizierung) |
| **PII** | Personally Identifiable Information |
| **RBAC** | Role-Based Access Control |
| **WAL** | Write-Ahead Log |

### C. Kontakt

**Security Team:**
- E-Mail: security@themisdb.example.com
- GitHub Security Advisories: https://github.com/makr-code/ThemisDB/security/advisories
- Response Time: < 24h

**Vulnerability Disclosure:**
Siehe [SECURITY.md](../../SECURITY.md) für Details zum Responsible Disclosure Process.

---

**Dokumenten-Ende**

*Letzte Aktualisierung: 2026-01-07*  
*Nächste Review: 2026-04-07 (Quarterly)*  
*Verantwortlich: Security Team*
