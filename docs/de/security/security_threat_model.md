# Threat Model (light)

**Stand:** 6. April 2026  
**Version:** v1.8.0-rc1  
**Kategorie:** 🔒 Security

---

> **📢 WICHTIG:** Dieses Dokument bietet einen Überblick über das Bedrohungsmodell von ThemisDB.  
> Für eine **umfassende Analyse aller Angriffsvektoren** siehe:  
> **[ANGRIFFSVEKTOREN_ANALYSE.md](ANGRIFFSVEKTOREN_ANALYSE.md)** ⭐

---

## 📑 Inhaltsverzeichnis

- [Assets](#assets)
- [Akteure](#akteure)
- [Vertrauensgrenzen](#vertrauensgrenzen)
- [Hauptrisiken (Auszug)](#hauptrisiken-auszug)
- [Gegenmaßnahmen](#gegenmaßnahmen)
- [Beobachtbarkeit](#beobachtbarkeit)
- [Weiterlesen](#weiterlesen)

---


Ziel: Risiken sichtbar machen und mit pragmatischen Kontrollen adressieren.

## Aktualisierung v1.4.0-alpha

**Neue Angriffsflächen seit v1.3.0:**
- 7 Netzwerkprotokolle (HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL Wire, gRPC, MCP)
- 10 Native Client SDKs
- LLM-Integration mit GPU-Zugriff
- Voice Assistant (Enterprise)
- Image Analysis Plugins
- Erweiterte gRPC-RPC-Framework für Sharding

**Siehe umfassende Analyse:**  
→ [ANGRIFFSVEKTOREN_ANALYSE.md](ANGRIFFSVEKTOREN_ANALYSE.md) für detaillierte Bewertung aller 120+ API-Endpunkte, Client-Bibliotheken und Protokolle.

## Assets
- Datenbankinhalte (Dokumente, Entitäten, Vektoren, Zeitreihen, Graphen)
- Schlüsselmaterial (LEK/KEK/DEK, TLS-Zertifikate, HSM-Keys)
- Audit‑Trails (Changefeed, Security Events)
- LLM-Modelle und Embeddings
- GPU-Ressourcen (CUDA)

## Akteure
- Admin/Operator (berechtigt)
- Anwendung/Service (technisch)
- Angreifer extern/intern (unberechtigt/teilberechtigt)

## Vertrauensgrenzen
- **Externe Clients** ↔ **Netzwerkprotokolle** (HTTP/REST, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL, gRPC, MCP)
- **Netzwerkprotokolle** ↔ **TLS/Auth-Layer** (JWT, API-Token, mTLS)
- **Auth-Layer** ↔ **RBAC/ABAC** (Ranger Policy Engine)
- **Themis-Server** ↔ **Storage** (RocksDB mit Encryption)
- **Themis-Server** ↔ **LLM Engine** (Optional: llama.cpp)
- **Themis-Server** ↔ **Externe Services** (Vault, HSM, TSA, Ranger)
- **Shard-to-Shard Communication** ↔ **gRPC mTLS**

## Hauptrisiken (Auszug)

**Externe Angriffe:**
- Unautorisierte Schlüsselrotation/Schlüsselabgriff via `/api/keys/rotate`
- Datenexfiltration über Admin‑APIs (`/admin/backup`, `/admin/restore`)
- AQL/SQL Injection via Query-Endpunkte
- DoS-Angriffe über komplexe Queries oder Batch-Operationen
- PII‑Leakage in Logs/Exports
- LLM Prompt Injection (Datenzugriff via manipulierte Prompts)
- Shard-Spoofing (Fake-Shard via gRPC)

**Interne Angriffe:**
- Manipulation Audit‑Trail via `/changefeed/retention`
- Privilege Escalation (RBAC-Bypass)
- Insider Data Exfiltration (berechtigte Admins)
- Supply Chain Attacks (Dependencies, Client SDKs)

**Infrastruktur:**
- GPU-Memory-Extraktion (LLM-Modelle)
- Container-Escape
- Certificate-Spoofing (mTLS-Bypass)

**Siehe [ANGRIFFSVEKTOREN_ANALYSE.md](ANGRIFFSVEKTOREN_ANALYSE.md) für STRIDE-Analyse und CVSS-Scores.**

### LLM & LoRa-spezifische Risiken (v1.4.0+)
- **Prompt Injection:** Manipulation von System-Prompts oder Benutzer-Eingaben
- **LoRa Model Poisoning:** Vergiftete Adapter mit Backdoors
- **Vector Embedding Manipulation:** Manipulation von HNSW-Indizes für RAG
- **Adapter Weight Extraction:** Seitenkanal-Angriffe auf LoRa-Weights
- **Multi-LoRa Adversarial Switching:** Ausnutzung von Adapter-Batching
- Siehe: [LLM_LORA_ATTACK_VECTORS.md](LLM_LORA_ATTACK_VECTORS.md)

## Gegenmaßnahmen

**Netzwerksicherheit:**
- RBAC/ABAC mit Apache Ranger für alle API-Endpunkte
- Netzwerk‑Kontrollen vor Admin‑APIs (`/keys/rotate`, `/changefeed/retention`, `/admin/*`)
- TLS 1.2/1.3 (empfohlen: 1.3 only), mTLS für gRPC-Shards
- Rate Limiting (Token Bucket: 100 req/min global, 10 req/min für Admin-APIs)
- Input Validation (JSON Schema, Path Traversal Protection, AQL Injection Detection)

**Authentifizierung & Autorisierung:**
- JWT-Validierung (JWKS, Issuer/Audience Check)
- API-Token-based Authentication
- Least‑Privilege, Separation of Duties
- Vier‑Augen‑Prinzip bei kritischen Aktionen (empfohlen)
- MFA für Admin-APIs (geplant für v1.5.0)

**Datenschutz:**
- AES-256-GCM Verschlüsselung (Data-at-Rest)
- Field-Level Encryption (Schema-based)
- Secrets‑Management (Vault/HSM, kein Klartext im Repo/Config)
- PII-Pseudonymisierung in Logs
- Audit Logging: Encrypt-then-Sign, Hash Chain

**LLM-Sicherheit:**
- Prompt Sanitization (in Entwicklung)
- Resource Limits (GPU, Memory, max_tokens)
- Model Access Control (RBAC per Model)
- Output Filtering (PII Detection, Secret Scanning)

**Monitoring & Detection:**
- Umfassende Audit-Logs (65+ Security Event Types)
- SIEM-Integration (Syslog RFC 5424, Splunk HEC)
- Grafana Dashboards mit Security Alerts
- Anomalie-Erkennung (geplant)

**Supply Chain:**
- Dependency Scanning (OSV, npm audit, Snyk)
- SBOM-Generierung
- Gitleaks (Secret Detection in CI/CD)

**Weitere Maßnahmen:**
- Backup/Restore mit Integritätsprüfungen (Signatures)
- Regelmäßige Schlüsselrotation (empfohlen: KEK alle 90 Tage)
- Immutable Audit Logs (WORM-Storage empfohlen)

### LLM & LoRa-Schutzmaßnahmen (v1.4.0+)
- **LoRa Signature Verification:** Signaturvalidierung für Adapter (`LoRASecurityValidator`)
- **Prompt Injection Detection:** Pattern-basierte Erkennung (`PromptInjectionDetector`)
- **Embedding Anomaly Detection:** Statistische Anomalieerkennung (`EmbeddingAnomalyDetector`)
- **Vector Encryption:** At-Rest-Verschlüsselung für HNSW (Phase 1+2 ✅)
- **Adapter Integrity Checks:** Checksum-Validierung und Weight-Anomalie-Detection
- Siehe: Implementierung in `include/llm/lora_security_validator.h`

## Beobachtbarkeit
- **Health/Metrics Endpunkte:** `/health`, `/metrics`, `/api/capabilities`
- **Audit Events:** 65+ Security Event Types (Authentication, Authorization, Data Access, Admin Actions)
- **Alarme:**
  - Schlüsselablauf (Key Expiration)
  - Retention‑Fehler (Changefeed Errors)
  - Anomalien im Changefeed (Unusual Patterns)
  - Failed Authentication Attempts (> 5 in 1min)
  - Admin API After Hours
  - Large Batch Operations
  - Rate Limit Exceeded
- **Monitoring-Stack:** Grafana + Prometheus + OpenTelemetry
- **SIEM:** Syslog RFC 5424, Splunk HEC (optional)

## Weiterlesen

**Umfassende Analyse:**
- **[ANGRIFFSVEKTOREN_ANALYSE.md](ANGRIFFSVEKTOREN_ANALYSE.md)** ⭐ - Vollständige Analyse aller Angriffsvektoren (extern + intern)

**Sicherheits-Dokumentation:**
- [security_overview.md](security_overview.md) - Übersicht aller Security-Features
- [security_hardening.md](security_hardening.md) - Production Hardening Guide
- [security_audit_checklist.md](security_audit_checklist.md) - BSI C5, ISO 27001, DSGVO
- [security_encryption_strategy.md](security_encryption_strategy.md) - Verschlüsselungsstrategie

**Spezielle Themen:**
- [key_management.md](security_key_management.md) - Schlüsselverwaltung (LEK, KEK, DEK)
- [security_policies.md](security_policies.md) - Apache Ranger Policies
- [security_hsm.md](security_hsm.md) - HSM-Integration (PKCS#11)
- [security_pki_architecture.md](security_pki_architecture.md) - PKI-Integration
- [security_pii_detection.md](security_pii_detection.md) - PII-Erkennung
- [security_audit_retention.md](security_audit_retention.md) - Audit & Retention
- [security_incident_response.md](security_incident_response.md) - Incident Response Plan

---

**Letzte Aktualisierung:** 2026-01-07  
**Nächste Review:** 2026-04-07 (Quarterly)
Weiterlesen:
- security/key_management.md, security/audit_and_retention.md
- encryption_strategy.md, security_hardening_guide.md, security_audit_checklist.md
- [GPU/VRAM/CUDA Angriffsvektoren](GPU_VRAM_ANGRIFFSVEKTOREN.md) - Spezifische Analyse von GPU-basierten Bedrohungen
