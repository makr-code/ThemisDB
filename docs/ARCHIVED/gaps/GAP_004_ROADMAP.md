# GAP-004: Security & Governance - Roadmap und nächste Schritte

**Dokument:** Roadmap und Implementierungsplan  
**Version:** 1.0  
**Datum:** 4. Februar 2026  
**Status:** ✅ Phase 1 abgeschlossen

---

## 📋 Übersicht

Dieses Dokument beschreibt die Roadmap für die vollständige Implementierung von Security & Governance Features in ThemisDB, aufbauend auf der in GAP-004 geschaffenen Basisstruktur.

---

## ✅ Phase 1: Basisstruktur (Abgeschlossen)

**Zeitraum:** Februar 2026  
**Status:** ✅ Vollständig implementiert

### Deliverables

1. **PolicyManager & PolicyRule**
   - ✅ Struktur für Governance-Regeln
   - ✅ Ressourcen- und Aktionsmuster mit Wildcards
   - ✅ Rollenbasierte Filterung
   - ✅ Policy-Evaluation mit Regel-Aggregation
   - ✅ Persistierung (JSON)
   - ✅ Validierung und Statistiken
   - ✅ Unit-Tests (15+ Tests)

2. **ProfileManager & Profile**
   - ✅ Datenstruktur für Benutzer-/Entitätsprofile
   - ✅ Klassifizierungslevel
   - ✅ Rollenzuweisung
   - ✅ Ressourcen-Whitelist/Blacklist
   - ✅ Persistierung
   - ✅ Unit-Tests (5+ Tests)

3. **PKI & Signatur Stubs**
   - ✅ PKIManager Stub-Interface
   - ✅ SignatureManager Stub-Interface
   - ✅ Factory Pattern
   - ✅ Logging-Warnungen für Stub-Nutzung
   - ✅ Unit-Tests (13+ Tests)

4. **Dokumentation**
   - ✅ Implementierungsübersicht
   - ✅ API-Dokumentation
   - ✅ Beispiele
   - ✅ Update GAP-Analyse-Dokument

---

## 🔄 Phase 2: Integration (In Planung)

**Zeitraum:** Q2 2026 (April-Juni)  
**Dauer:** 3-4 Wochen  
**Status:** 🔄 Geplant

### Ziele

Integration der neuen Governance-Komponenten in bestehende ThemisDB-Systeme.

### Aufgaben

1. **PolicyEngine Integration**
   - [ ] PolicyManager in PolicyEngine integrieren
   - [ ] Unified Policy Evaluation API
   - [ ] Header-basierte Policy-Entscheidungen
   - [ ] Backward Compatibility sicherstellen
   - **Aufwand:** 1 Woche

2. **RBAC Integration**
   - [ ] PolicyManager mit RBAC-System verbinden
   - [ ] User-Role-Store Integration
   - [ ] Permission Checks in API-Endpunkten
   - [ ] Audit-Logging für Zugriffsentscheidungen
   - **Aufwand:** 1 Woche

3. **API-Endpunkt Integration**
   - [ ] HTTP-Handler für Policy-Management
   - [ ] REST-API für Regel-CRUD-Operationen
   - [ ] Profile-Management-Endpunkte
   - [ ] Admin-Dashboard-Integration
   - **Aufwand:** 1 Woche

4. **Konfiguration**
   - [ ] YAML-Konfigurationsdateien für Policies
   - [ ] Beispiel-Konfigurationen
   - [ ] Policy-Templates
   - [ ] Hot-Reload-Unterstützung
   - **Aufwand:** 3-5 Tage

5. **Testing**
   - [ ] Integrationstests für Policy-Enforcement
   - [ ] E2E-Tests mit verschiedenen Rollen
   - [ ] Performance-Tests
   - [ ] Regressionstests
   - **Aufwand:** 3-5 Tage

### Deliverables

- Vollständig integrierte Policy-Management-API
- HTTP-Endpunkte für Policy-CRUD
- YAML-Konfigurationsunterstützung
- Integrationstests
- Aktualisierte Dokumentation

### Erfolgskriterien

- Alle Unit-Tests und Integrationstests bestanden
- API-Dokumentation vollständig
- Performance: Policy-Evaluation < 1ms
- Backward Compatibility: 100%

---

## 🔐 Phase 3: PKI-Implementierung (Geplant)

**Zeitraum:** Q3 2026 (Juli-September)  
**Dauer:** 4-6 Wochen  
**Status:** 🔄 Geplant

### Ziele

Ersetzung der PKI-Stubs durch echte PKI-Funktionalität mit OpenSSL und HSM-Integration.

### Aufgaben

1. **OpenSSL-Integration**
   - [ ] PKIManager mit OpenSSL-Implementierung
   - [ ] Key-Pair-Generierung (RSA, ECDSA)
   - [ ] CSR-Generierung
   - [ ] X.509-Zertifikatshandling
   - [ ] Certificate Chain Validation
   - **Aufwand:** 2 Wochen

2. **Certificate Management**
   - [ ] Certificate Store
   - [ ] CRL (Certificate Revocation List) Support
   - [ ] OCSP (Online Certificate Status Protocol)
   - [ ] Certificate Expiry Monitoring
   - [ ] Auto-Renewal für Zertifikate
   - **Aufwand:** 1-2 Wochen

3. **HSM-Integration**
   - [ ] PKCS#11-Integration für PKI-Operationen
   - [ ] HSM Key Storage
   - [ ] HSM-basierte Signatur-Operationen
   - [ ] Failover zu Software-Keys
   - **Aufwand:** 1-2 Wochen

4. **CA-Integration**
   - [ ] Externe CA-Anbindung (Let's Encrypt, etc.)
   - [ ] ACME-Protokoll-Unterstützung
   - [ ] Self-Signed CA für Test/Dev
   - [ ] Multi-CA-Unterstützung
   - **Aufwand:** 1 Woche

5. **Certificate Pinning**
   - [ ] Public Key Pinning
   - [ ] Certificate Pinning für Shard-Kommunikation
   - [ ] Trust-on-First-Use (TOFU)
   - **Aufwand:** 3-5 Tage

6. **Testing & Security Audit**
   - [ ] Unit-Tests für alle PKI-Funktionen
   - [ ] Security Audit der PKI-Implementierung
   - [ ] Penetration Testing
   - [ ] Dokumentation
   - **Aufwand:** 1 Woche

### Deliverables

- Vollständige PKI-Implementierung mit OpenSSL
- HSM-Integration
- CA-Anbindung (ACME)
- Certificate Management System
- Security Audit Report
- Produktionsreife Dokumentation

### Erfolgskriterien

- Alle Security-Tests bestanden
- HSM-Integration funktionsfähig
- ACME-Protokoll vollständig implementiert
- Externe Security Audit bestanden
- Performance: Zertifikatsvalidierung < 10ms

---

## ✍️ Phase 4: Signatur-Implementierung (Geplant)

**Zeitraum:** Q3 2026 (September-Oktober)  
**Dauer:** 3-4 Wochen  
**Status:** 🔄 Geplant

### Ziele

Ersetzung der Signatur-Stubs durch echte Signatur-Funktionalität mit RFC 3161 Timestamp Authority.

### Aufgaben

1. **Digital Signatures**
   - [ ] SignatureManager mit OpenSSL-Implementierung
   - [ ] RSA-Signaturen (PKCS#1, PSS)
   - [ ] ECDSA-Signaturen
   - [ ] EdDSA-Signaturen
   - [ ] Detached und Embedded Signatures
   - **Aufwand:** 1-2 Wochen

2. **Timestamp Authority (TSA)**
   - [ ] RFC 3161 TSA-Client-Implementierung
   - [ ] Externe TSA-Anbindung
   - [ ] TSA-Response-Validierung
   - [ ] Eingebettete TSA für Dev/Test
   - **Aufwand:** 1 Woche

3. **Langzeit-Signaturen**
   - [ ] XAdES (XML Advanced Electronic Signatures)
   - [ ] CAdES (CMS Advanced Electronic Signatures)
   - [ ] PAdES (PDF Advanced Electronic Signatures)
   - [ ] Archivale Signaturen (LTA)
   - **Aufwand:** 1-2 Wochen

4. **Multi-Signatur**
   - [ ] Multi-Party Signatures
   - [ ] Threshold Signatures
   - [ ] Counter-Signatures
   - **Aufwand:** 1 Woche

5. **eIDAS-Compliance**
   - [ ] Qualified Electronic Signatures (QES)
   - [ ] eIDAS-Baseline-Profile
   - [ ] Remote Signature API
   - **Aufwand:** 1-2 Wochen (Optional)

6. **Testing & Compliance**
   - [ ] Unit-Tests für alle Signatur-Funktionen
   - [ ] Compliance-Tests (eIDAS, etc.)
   - [ ] Performance-Tests
   - [ ] Dokumentation
   - **Aufwand:** 3-5 Tage

### Deliverables

- Vollständige Signatur-Implementierung
- RFC 3161 TSA-Integration
- Langzeit-Signaturen (XAdES, CAdES, PAdES)
- Multi-Signatur-Unterstützung
- eIDAS-Compliance (Optional)
- Compliance Report

### Erfolgskriterien

- Alle Signatur-Tests bestanden
- TSA-Integration funktionsfähig
- Langzeit-Signaturen validierbar
- Performance: Signatur < 50ms, Verifikation < 10ms
- eIDAS-Compliance (falls implementiert)

---

## 🏢 Phase 5: Enterprise Features (Geplant)

**Zeitraum:** Q4 2026 (Oktober-Dezember)  
**Dauer:** 4-6 Wochen  
**Status:** 🔄 Geplant

### Ziele

Enterprise-Features für Governance und Compliance.

### Aufgaben

1. **Policy-Versionierung**
   - [ ] Policy-Version-Management
   - [ ] Policy-History
   - [ ] Policy-Rollback
   - [ ] Policy-Diff
   - **Aufwand:** 1 Woche

2. **Policy-Templates**
   - [ ] Vordefinierte Policy-Templates
   - [ ] Template-Bibliothek
   - [ ] Custom Templates
   - [ ] Template-Marketplace (Optional)
   - **Aufwand:** 1 Woche

3. **Compliance-Reports**
   - [ ] Compliance-Dashboard
   - [ ] Automated Compliance Checks
   - [ ] Compliance-Reports (PDF, HTML)
   - [ ] Audit-Trail-Visualisierung
   - **Aufwand:** 2 Wochen

4. **Policy-Konflikt-Auflösung**
   - [ ] Automatische Konflikt-Erkennung
   - [ ] Konflikt-Auflösungs-Strategien
   - [ ] Policy-Simulation
   - [ ] What-If-Analyse
   - **Aufwand:** 1-2 Wochen

5. **Advanced RBAC**
   - [ ] Attribute-Based Access Control (ABAC)
   - [ ] Context-Aware Policies
   - [ ] Dynamic Role Assignment
   - [ ] Delegation und Impersonation
   - **Aufwand:** 2 Wochen

6. **Integration & Testing**
   - [ ] Integrationstests
   - [ ] Performance-Tests
   - [ ] User Acceptance Testing
   - [ ] Dokumentation
   - **Aufwand:** 1 Woche

### Deliverables

- Policy-Versionierung und -Rollback
- Policy-Template-Bibliothek
- Compliance-Dashboard
- Advanced RBAC (ABAC)
- Enterprise-Dokumentation

### Erfolgskriterien

- Policy-Versionierung funktionsfähig
- Template-Bibliothek mit 10+ Templates
- Compliance-Dashboard einsatzbereit
- ABAC vollständig implementiert
- User Acceptance Testing bestanden

---

## 📊 Gesamtübersicht

### Timeline

```
Q1 2026: Phase 1 ✅ Basisstruktur (Abgeschlossen)
Q2 2026: Phase 2 🔄 Integration (3-4 Wochen)
Q3 2026: Phase 3 🔄 PKI-Implementierung (4-6 Wochen)
Q3 2026: Phase 4 🔄 Signatur-Implementierung (3-4 Wochen)
Q4 2026: Phase 5 🔄 Enterprise Features (4-6 Wochen)
```

### Gesamtaufwand

- **Phase 1:** ✅ 1-2 Wochen (Abgeschlossen)
- **Phase 2:** 3-4 Wochen
- **Phase 3:** 4-6 Wochen
- **Phase 4:** 3-4 Wochen
- **Phase 5:** 4-6 Wochen

**Total:** 15-22 Wochen (ca. 4-6 Monate)

### Prioritäten

1. **HOCH:** Phase 2 (Integration) - Erforderlich für Produktionsnutzung
2. **HOCH:** Phase 3 (PKI) - Security-kritisch
3. **MITTEL:** Phase 4 (Signaturen) - Compliance-relevant
4. **MITTEL:** Phase 5 (Enterprise) - Feature-Enhancement
5. **NIEDRIG:** eIDAS-Compliance - Nischenanwendung

---

## 🎯 Abhängigkeiten

### Technisch

- **OpenSSL 3.0+** für PKI und Signaturen
- **PKCS#11** für HSM-Integration
- **yaml-cpp** für Konfiguration (bereits vorhanden)
- **nlohmann/json** für Persistierung (bereits vorhanden)

### Organisatorisch

- **Security Audit** (Phase 3): Externer Security-Experte erforderlich
- **Penetration Testing** (Phase 3): Security-Team oder externe Firma
- **eIDAS-Compliance** (Phase 4): Rechtliche Beratung erforderlich
- **User Acceptance Testing** (Phase 5): Kunden-Feedback-Runde

---

## 📝 Risiken und Mitigationen

### Technische Risiken

1. **OpenSSL-Komplexität**
   - **Risiko:** OpenSSL-API ist komplex und fehleranfällig
   - **Mitigation:** Wrapper-Klassen, umfassende Tests, Code-Review

2. **HSM-Integration**
   - **Risiko:** Verschiedene HSM-Hersteller, unterschiedliche APIs
   - **Mitigation:** PKCS#11-Standard nutzen, Abstraktionslayer

3. **Performance**
   - **Risiko:** Signatur-/Verifikationsoperationen können langsam sein
   - **Mitigation:** Caching, Async-Operationen, Hardware-Beschleunigung

### Zeitliche Risiken

1. **Security Audit**
   - **Risiko:** Externes Audit kann Verzögerungen verursachen
   - **Mitigation:** Frühzeitig planen, mehrere Anbieter evaluieren

2. **Scope Creep**
   - **Risiko:** Zusätzliche Features verzögern Implementierung
   - **Mitigation:** Strikte Priorisierung, MVP-first Approach

---

## 🔗 Verwandte Dokumente

- [GAP-004 Implementierungsübersicht](GAP_004_SECURITY_GOVERNANCE.md)
- [GAPS_STUBS_SUMMARY.md](../development/GAPS_STUBS_SUMMARY.md)
- [Security Overview](security_overview.md)
- [RBAC Dokumentation](../../../include/security/rbac.h)

---

**Erstellt:** 4. Februar 2026  
**Nächste Review:** April 2026 (Start Phase 2)  
**Verantwortlich:** ThemisDB Security Team
