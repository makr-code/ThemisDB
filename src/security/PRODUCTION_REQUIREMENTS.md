> **Status:** 2026-08-17 – mit aktuellem Security-Code, Wave-C Validierung (`tests/security/test_security_wavec_production_validation_focused.cpp`) und Rest-Gap-Status aus `MODULE_GAPS.md` abgeglichen.

# ThemisDB Security Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Security-Moduls.
Es definiert verbindliche Anforderungen für Zugriffskontrolle, Verschlüsselung, Key-Provider-Konfiguration, Injektionserkennung und Audit.

## Dokumentabgrenzung (Canonical Split)

- **`src/security/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/security/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/security/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/security/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Sicherheitsanforderungen

### 1) Key-Provider und Verschlüsselung

- **MUST:** `field_encryption.cpp` mit `VAULT`- oder `HSM`-Key-Provider betreiben (kein `LOCAL`-Key-Provider in Produktion).
- **MUST:** `vault_key_provider.cpp` mit gültiger Vault-Adresse und Token konfiguriert; leere Konfiguration wird abgewiesen.
- **MUST:** Bei HSM-Einsatz: `THEMIS_HSM_ENABLED=1`, `library_path` auf existierende PKCS#11-Datei gesetzt, `slot_id` numerisch gültig.
- **MUST NOT:** `LOCAL`-Key-Provider in Produktionsdeployments verwenden.
- **MUST NOT:** Vault/HSM-Fehler ignorieren; Verschlüsselungsfehler führen zu Fail-Closed-Verhalten.

### 2) Zugriffskontrolle (RBAC/ABAC/RLS)

- **MUST:** `rbac.cpp` und `access_control_manager.cpp` aktiv; kein Bypass von Policy-Enforcement-Pfaden.
- **MUST:** `row_level_security.cpp` aktiviert, wenn Tenant-Isolation erforderlich.
- **MUST:** Policy-Enforcement-Fehler führen zu explizitem `ERR_ACCESS_DENIED`, nicht zu Silent-Permit.
- **MUST NOT:** `access_control_manager` mit leerem Policy-Set in Produktion betreiben.

### 3) Injektionserkennung und Detection-Controls

- **MUST:** `aql_injection_detector.cpp` aktiv für alle Query-Eingabepfade.
- **MUST:** `behavioral_anomaly_detector.cpp` in Produktionsdeployments konfiguriert.
- **MUST:** Security-Evidence-Collector (`security_evidence_collector.cpp`) aktiv; Audit-Trail muss persistiert werden.

### 4) Timestamp-Authority Transport (RFC 3161)

- **MUST:** Für produktive TSA-Anbindung den OpenSSL/CURL-Pfad (`timestamp_authority_openssl.cpp`) mit HTTPS-Endpunkt betreiben.
- **MUST:** TSA-Transport auf TLS-gesicherte Verbindungen beschränken (kein HTTP-Fallback, keine Nicht-HTTPS-Redirects).
- **MUST:** Verbindungs- und Gesamt-Timeouts gesetzt halten; unbegrenzte TSA-Requests sind unzulässig.
- **MUST NOT:** Unsichere TSA-Transportpfade als produktiven Standard verwenden.

## Betriebsgrenzen (aktuelles Security-Verhalten)

- Encryption-Key-Rotation über `vault_key_provider` ist operativ; Key-Rotation ohne Ausfallzeit erfordert konfigurierte Rotation-Policy.
- `pki_key_provider.cpp` für TLS-Zertifikatsverwaltung; PKI-Zertifikat-Ablauf führt zu Verbindungsabbrüchen ohne rechtzeitige Rotation.
- `zero_trust_policy_enforcer.cpp`, `behavioral_anomaly_detector.cpp` und `security_evidence_collector.cpp` bilden die aktuell dokumentierten produktionsrelevanten Policy-/Detection-/Evidence-Surfaces; deren Konfiguration muss vor Produktionsstart validiert werden.
- Dieses Dokument beschreibt produktive Mindestanforderungen, ersetzt aber **keine** finale Gap-Abnahme: Solange `MODULE_GAPS.md` offene Rescan-/Residualpunkte führt, ist die Modul-Sign-off noch nicht vollstaendig abgeschlossen.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Key-Provider: `VAULT` oder `HSM` (kein `LOCAL`)
- [ ] Vault/HSM-Konfiguration vollständig und validiert
- [ ] RBAC/ABAC Policy-Set nicht leer und aktiv
- [ ] RLS aktiviert wenn Tenant-Isolation erforderlich
- [ ] AQL-Injection-Detektor aktiv
- [ ] Behavioral-Anomaly-Detektor konfiguriert
- [ ] Security-Evidence-Collector persistiert Audit-Trail
- [ ] TSA-Endpunkt nutzt HTTPS; RFC-3161 Transport wird über den OpenSSL/CURL-Pfad abgesichert
- [ ] `THEMIS_ALLOW_HSM_STUB` ist in Produktion nicht gesetzt
- [ ] `THEMIS_ALLOW_TSA_STUB` ist in Produktion nicht gesetzt
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/security/PRODUCTION_REQUIREMENTS.md`
- `src/security/rbac.cpp`
- `src/security/access_control_manager.cpp`
- `src/security/row_level_security.cpp`
- `src/security/field_encryption.cpp`
- `src/security/vault_key_provider.cpp`
- `src/security/hsm_provider_pkcs11.cpp`
- `src/security/pki_key_provider.cpp`
- `src/security/aql_injection_detector.cpp`
- `src/security/behavioral_anomaly_detector.cpp`
- `src/security/security_evidence_collector.cpp`
- `src/security/zero_trust_policy_enforcer.cpp`
- `src/security/hsm_provider.cpp`
- `src/security/timestamp_authority.cpp`
- `src/security/timestamp_authority_openssl.cpp`
