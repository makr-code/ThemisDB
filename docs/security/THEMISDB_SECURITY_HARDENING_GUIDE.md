# ThemisDB Security Hardening in der Praxis

**Ein sicherheitsorientierter, praxisnaher Leitfaden für Plattform- und Compliance-Teams**

---

## Vertrauen ist kein Feature — es ist das Fundament

Die Frage, die jede Organisation stellen muss, die KI-Systeme einsetzt: *Woher weiß ich, dass die Antworten meiner KI belastbar, nachvollziehbar und compliance-fähig sind?* Die Antwort liegt nicht in der Magie der Algorithmen, sondern in der Sorgfalt der Infrastruktur, die sie trägt. ThemisDB ist die erste offene Datenbank, die Post-Quantum Cryptography mit FIPS 140-3 Enforcement, ACID Audit Trails und nativem GDPR Support vereint. Doch diese Technologien sind nur so stark wie die Prozesse, die sie umgeben.

Grounding — also die Fähigkeit, jede KI-Antwort auf ihre ursprüngliche Datenquelle zurückzuführen — ist dabei kein Nice-to-have, sondern eine Compliance-Anforderung. Ohne nachvollziehbare Datenherkunft gibt es keine Auditierbarkeit. Ohne Auditierbarkeit gibt es keine regulatorische Akzeptanz. Und ohne regulatorische Akzeptanz gibt es keine Produktionstauglichkeit für kritische Anwendungsfälle.

Dieser Guide zeigt, wie Security, Grounding und Compliance in ThemisDB zusammenwirken, um vertrauenswürdige KI-Systeme zu schaffen.

---

## Warum Post-Quantum-Sicherheit heute schon entscheidend ist

Die größte Bedrohung für die Datensicherheit kommt nicht von heute bekannten Angriffen, sondern von morgen verfügbarer Technologie. Shors Algorithmus auf einem quantenrelevanten Computer wird RSA, ECDSA und ECDH brechen — die Algorithmen, die heute praktisch alle Datenbankverbindungen, digitale Signaturen und Schlüsseltransportmechanismen sichern. NIST schätzt, dass solche Systeme innerhalb von 10–15 Jahren verfügbar sein könnten. Doch die Gefahr beginnt *heute*.

**Harvest-Now-Decrypt-Later-Angriffe** funktionieren bereits: Angreifer zeichnen verschlüsselten Datenverkehr heute auf, um ihn später zu entschlüsseln, sobald Quantencomputer verfügbar sind. Betroffen sind alle Daten mit einer Vertraulichkeitsanforderung von 10+ Jahren — medizinische Unterlagen, Finanztransaktionen, klassifizierte Informationen, persönliche Daten. Die EU schätzt, dass bis zu 80% aller heute gespeicherten Daten diese Kriterien erfüllen.

ThemisDB ist die erste HTAP-Datenbank, die NIST-standardisierte Post-Quantum-Algorithmen integriert:

- **CRYSTALS-Kyber-1024** (FIPS 203) für Schlüsselkapselung mit NIST Security Level 5 (äquivalent zu AES-256)
- **CRYSTALS-Dilithium-5** (FIPS 204) für digitale Signaturen mit derselben Sicherheitsstufe
- **Hybride Verschlüsselung** nach NIST SP 800-227: Jede Verschlüsselungsoperation kombiniert Kyber mit AES-256-GCM — ein Angreifer müsste *beide* brechen

Die Key-Hierarchie ist quantensicher gestaltet:

```
Master Key (Kyber-1024 KEM Paare, in HSM)
  └─→ KEK (AES-256, via Kyber verkapselt)
        └─→ DEK (AES-256-GCM, via KEK umhüllt)
              └─→ Feld-verschlüsselte Nutzdaten
```

Jeder Feldverschlüsselungsvorgang verwendet ein ephemeres KEM-Schlüsselpaar — das schafft Forward Secrecy. Selbst wenn ein Master Key später kompromittiert wird, bleiben früher verschlüsselte Daten geschützt.

**Praktische Implikation**: Post-Quantum-Sicherheit ist kein Feature für die Zukunft, sondern eine Notwendigkeit für alle Daten, die länger als die Lebensdauer klassischer Kryptographie gespeichert werden müssen.

---

## FIPS 140-3 Application-Level Enforcement: Was ThemisDB anders macht

FIPS 140-3 ist der Goldstandard für kryptographische Module — doch die meisten Datenbanken implementieren ihn als *Hardware*-Anforderung (die HSM muss zertifiziert sein), nicht als *Software*-Garantie. ThemisDB geht weiter und erzwingt FIPS-Konformität auf Anwendungsebene.

Der `FipsCryptoMode` (in `include/security/fips_crypto_mode.h`) ist ein Singleton, das nur genehmigte Algorithmen zulässt:

**Genehmigte Algorithmen:**
- Symmetrisch: AES-128/192/256 in CBC, CTR, GCM, CCM, XTS, KW
- Hash: SHA-256, SHA-384, SHA-512 (und SHA3-Varianten)
- MAC: HMAC-SHA-256/384/512, CMAC-AES-128/256
- Asymmetrisch: RSA-2048/3072/4096, ECDSA/ECDH P-256/384/521
- KDF: PBKDF2, HKDF, SP800-108-Varianten
- DRBG: CTR_DRBG, HASH_DRBG, HMAC_DRBG

**Gesperrte Algorithmen** (werfen `FipsPolicyViolation`): MD5, SHA-1 (für neue Signaturen), RC4, DES, 3DES, Blowfish, ChaCha20-Poly1305.

Der entscheidende Unterschied zu anderen Datenbanken:

| Aspekt | Traditionelle DBs | ThemisDB |
|--------|------------------|----------|
| FIPS Scope | Hardware-HSM | Anwendungsebene + Hardware |
| Algorithmus-Durchsetzung | Konfiguration | Code-Level, nicht umgehbar |
| Fehlerverhalten | Graceful Degradation | Warnung + Deaktivierung (kein unsicherer Betrieb) |
| Selbsttests | HSM-intern | Anwendungsweit (`OSSL_PROVIDER_self_test()`) |
| Speicherbereinigung | Standard | `OPENSSL_cleanse()` für alle kryptographischen Puffer |

**Compliance-Vorteil**: Auditoren können nicht nur überprüfen, *dass* ein HSM verwendet wird, sondern auch *dass* die Anwendung nur genehmigte Algorithmen nutzt — selbst wenn das HSM theoretically unsichere Algorithmen unterstützen würde.

---

## GDPR/CCPA Compliance im ACID-Kontext

Die meisten Organisationen behandeln Compliance als separates System: Datenbank hier, DSGVO-Tool dort, Audit-Log irgendwo anders. ThemisDB integriert Compliance in die Transaktionssemantik selbst — weil regulatorische Anforderungen nicht nachträglich aufgesetzt, sondern fundamental in den Datenfluss eingebettet werden müssen.

**ACID + Compliance = AACID (Audit-Aware ACID)**

| ACID-Eigenschaft | Compliance-Relevanz | ThemisDB-Implementierung |
|-----------------|---------------------|--------------------------|
| Atomicity | Alles-oder-nichts für Daten *und* Audit-Eintrag | 2-Phase-Commit mit Audit-Log |
| Consistency | Datenbankregeln + Compliance-Regeln | RBAC/ABAC + Policy-Enforcement |
| Isolation | Lesen kommittierter Daten + Audit-Sichtbarkeit | MVCC mit Versionstokens für Grounding |
| Durability | Daten persistieren + Audit-Eintrag persistieren | WAL + HSM-geschützte Audit-Log-Signaturen |

**Konkrete Umsetzungen:**

**Datenschutz durch Design (GDPR Art. 25):**
- Feldlevel-Verschlüsselung mit einzigartigen DEKs pro Dokument (kein Key-Sharing für Related-Plaintext-Angriffe)
- Granularitäten: DOCUMENT, ARRAY, VRAM (für GPU-Vektoren)
- Automatische Schlüsselrotation: DEKs jährlich, KEKs alle 2 Jahre, Master Keys alle 3 Jahre

**Recht auf Vergessenwerden (GDPR Art. 17):**
- `DELETE`-Operationen in ThemisDB löschen nicht nur Daten, sondern auch alle zugehörigen Audit-Trail-Einträge *und* die verschlüsselten Feldwerte
- Quanten-sichere Löschbestätigung: Jede Löschoperation wird mit Dilithium-5 signiert und im HSM-geschützten Master-Log gespeichert

**Datenportabilität (GDPR Art. 20):**
- Export-Funktionen generieren verschlüsselte Dumps mit eingebetteten Metadaten zur Datenherkunft (Grounding-Informationen)
- Signaturen ermöglichen die Überprüfung, dass exportierte Daten nicht manipuliert wurden

**Vergleich: Separate Systeme vs. ThemisDB ACID-Integration**

| Anforderung | Separate Systeme | ThemisDB |
|-------------|-------------------|----------|
| Audit-Trail-Integrität | Nachträgliche Signatur | Transaktions-integrierte Signatur |
| Löschnachweis | Manuelle Bestätigung | Automatische, signierte Bestätigung |
| Zugriffskontrolle | Externes IAM | RBAC + ABAC + Zero-Trust-Enforcer |
| Performance-Overhead | Hohe Latenz durch Systemgrenzen | Minimal (im Transaktionsflow) |
| Compliance-Nachweis | Multiple Logs korrelieren | Ein integrierter, signierter Trail |

**Praktisches Beispiel**: Eine Bank muss nachweisen, dass ein Kundendatensatz gelöscht wurde. In traditionellen Systemen bedeutet das: Datenbank-Log zeigen, dass DELETE ausgeführt wurde, dann separates Audit-System konsultieren, dann vielleicht ein Ticket-System überprüfen. In ThemisDB: Eine einzelne, Dilithium-signierte Transaktion zeigt die Löschung *und* den zugehörigen Audit-Eintrag *und* die Grounding-Informationen (welche Datenquellen betroffen waren).

---

## Audit-Trail: Was Regulierer wirklich brauchen

Audit-Trails sind das Rückgrat jeder Compliance. Doch die meisten Organisationen speichern zu viel (Noise) oder zu wenig (Lücken). ThemisDB implementiert einen **tamper-evident audit trail** mit folgende Eigenschaften:

**Muss-Inhalte für regulatorische Anforderungen:**

| Regulatorische Anforderung | ThemisDB-Umsetzung | Quelle |
|---------------------------|--------------------|--------|
| **Wer** hat zugegriffen? | User ID + Authentifizierungsmethode | RBAC-Log |
| **Was** wurde gemacht? | Operationstyp + betroffene Ressourcen | Transaktions-Log |
| **Wann** geschah es? | Timestamp mit Mikrosekunden-Genauigkeit | System Clock + HSM |
| **Woher** kam die Anfrage? | Client IP + Geolocation (optional) | Zero-Trust-Context |
| **Warum** war es erlaubt? | Policy-Entscheidung + Rollen | Policy-Enforcer-Log |
| **Datenherkunft** | Grounding-Informationen (Quelldokumente, Versionen) | Version-Tokens |

**Technische Implementierung:**

- **Hash-Chain**: Jeder Audit-Eintrag enthält den Hash des vorherigen Eintrags → Manipulationen sind sofort erkennbar
- **Dilithium-Signaturen**: Jeder Commit-Record in der Audit-Log wird mit CRYSTALS-Dilithium-5 signiert
- **HSM-Integration**: Audit-Log-Signaturschlüssel werden in HSMs gespeichert (FIPS 140-2 Level 3)
- **Unveränderlichkeit**: Einmal geschriebene Audit-Einträge können nicht geändert oder gelöscht werden (außer durch spezielle, auditierte Löschprozesse)

**Speicherstrategie:**
- **Hot Storage**: Aktuelle 30 Tage im schnellen Speicher für schnelle Abfragen
- **Warm Storage**: 1 Jahr in komprimierter Form auf Object Storage
- **Cold Storage**: >1 Jahr in HSM-verschlüsselter Archivform (für Harvest-Now-Decrypt-Later-Schutz)

**Abfragefähigkeit:**
```sql
-- Beispiel: Alle Zugriffe auf Personaldaten in den letzten 30 Tagen
SELECT user_id, action, resource, timestamp, grounding_sources
FROM audit_trail
WHERE resource LIKE '%/hr/personal/%'
  AND timestamp > NOW() - INTERVAL '30 days'
ORDER BY timestamp DESC;
```

---

## Grounding: Der Compliance-Faktor, den viele übersehen

Grounding — die Fähigkeit, jede KI-Antwort auf ihre ursprünglichen Datenquellen zurückzuführen — ist nicht nur eine technische Anforderung, sondern ein zentraler Compliance-Faktor. Ohne Grounding gibt es keine:

- **Nachvollziehbarkeit**: Welche Daten wurden für diese Antwort verwendet?
- **Verantwortlichkeit**: Wer ist für die Datenqualität zuständig?
- **Reproduzierbarkeit**: Kann die Antwort mit den gleichen Daten reproduziert werden?
- **Regulatorische Akzeptanz**: Erfüllt die Antwort die Anforderungen an Transparenz?

**ThemisDB implementiert Grounding auf mehreren Ebenen:**

**1. Versionstokens für Datenkonsistenz**
Jeder Datensatz in ThemisDB hat ein `CrossLayerVersionToken`:
- `storage_version`: Version aus der Storage-Schicht (Paxos/RAID-Paxos)
- `cache_version`: Version aus der KV-Cache-Schicht (Raft)
- `transaction_id`: Assoziierte Transaktions-ID
- `timestamp`: Zeitstempel der letzten Änderung

Diese Tokens ermöglichen es, zu jeder Zeit zu verifizieren, dass die Daten, die für eine KI-Antwort verwendet wurden, konsistent und nicht manipuliert sind.

**2. RAG-spezifisches Grounding**
Für Retrieval-Augmented Generation (RAG) Anwendungsfälle speichert ThemisDB:
- **Chunk-IDs**: Welche Textsegmente wurden Retrieviert
- **Dokumenten-Metadaten**: Ursprüngliche Dokumenten-IDs und Versionen
- **Vektor-Referenzen**: Welche Embeddings wurden verwendet
- **Confidence Scores**: Vertrauenswerte für jeden Retrieval-Schritt

**3. Audit-Trail-Integration**
Jede Grounding-Information wird im Audit-Trail gespeichert:
```json
{
  "request_id": "req_abc123",
  "query": "Was ist die Datenschutzrichtlinie?",
  "response": "...",
  "grounding": {
    "sources": [
      {
        "document_id": "doc_456",
        "version": "v3.2",
        "chunk_id": "chunk_789",
        "confidence": 0.95,
        "retrieval_timestamp": "2026-06-13T10:00:00Z",
        "encryption_status": "AES-256-GCM (Kyber-1024 wrapped)"
      }
    ],
    "consistency_token": "CLVT_v1_storage=4567_cache=4567",
    "signatures": {
      "storage": "Dilithium5_sig_...",
      "cache": "Dilithium5_sig_..."
    }
  }
}
```

**4. Compliance-Relevanz**

| Regulatorische Anforderung | Grounding-Beitrag |
|---------------------------|-------------------|
| **GDPR Art. 5(1)(d)** (Datenminimierung) | Nachweis, dass nur relevante Daten verwendet wurden |
| **GDPR Art. 22** (Automatisierte Entscheidungen) | Transparenz über Datenquellen für KI-Entscheidungen |
| **AI Act (EU)** | Dokumentationspflicht für Trainingsdaten |
| **ISO 27001 A.18.1.3** | Schutz vor manipulierten Datenquellen |
| **SOC 2 CC6.1** | Nachvollziehbarkeit von Datenflüssen |

**Praktische Empfehlung**: Jede KI-Anfrage sollte eine Grounding-Signatur zurückgeben, die im Audit-Log gespeichert wird. Diese Signatur ermöglicht es Auditoren, zu verifizieren, dass die Antwort auf den angegebenen Datenquellen basiert und diese nicht manipuliert wurden.

---

## Security-Kontrollen, die Entscheider sehen und messen können

Compliance ist nicht nur eine technische Anforderung, sondern eine Management-Aufgabe. Entscheider brauchen sichtbare, messbare Kontrollen, die Vertrauen schaffen und regulatorische Anforderungen erfüllen.

**Dashboard-Metriken (Echtzeit):**

| Metrik | Zielwert | Bedeutung | Quelle |
|--------|----------|-----------|--------|
| FIPS-Konformitätsrate | 100% | Keine nicht-genehmigten Algorithmen | FipsCryptoMode |
| HSM-Antwortzeit | < 10ms | Kein Performance-Bottleneck | HsmSecurityMetrics |
| Audit-Log-Integrität | 100% | Keine manipulierten Einträge | Hash-Chain-Verifikation |
| Schlüsselrotations-Status | "Grün" | Alle Schlüssel aktuell | KeyRotationStateMachine |
| Zero-Trust-Verifizierungsrate | > 99.9% | Fast alle Requests verifiziert | ZeroTrustPolicyEnforcer |
| Grounding-Vollständigkeit | 100% | Jede Antwort hat Quellen | RAG-Grounding-Service |

**Monatliche Compliance-Reports:**

1. **Algorithmus-Nutzungsstatistik**: Welche kryptographischen Algorithmen wurden verwendet? (Nachweis für FIPS 140-3)
2. **Zugriffsstatistik nach Rolle**: Welche Rollen haben auf welche Daten zugegriffen? (RBAC-Audit)
3. **Schlüsselrotations-Log**: Wann wurden welche Schlüssel rotiert? (Key Management Policy)
4. **Incident-Response-Zeiten**: Wie schnell wurden Sicherheitsvorfälle bearbeitet? (SOC 2)
5. **Grounding-Audit**: Wie viele Antworten hatten vollständige/teilweise/unvollständige Grounding-Informationen?

**Jährliche Penetrationstests:**
- Externer Test der gesamten Security-Kette: TLS 1.3 + mTLS + Zero-Trust + HSM + Audit-Log
- Spezifischer Test: Harvest-Now-Decrypt-Later-Szenario (kann Kyber-verschlüsselte Daten nicht entschlüsselt werden?)
- Social-Engineering-Test: Können Angreifer durch Phishing an Schlüssel gelangen?

---

## Threat Surface: Unified vs. Polyglot Systems

Die Architektur einer Datenbank hat direkten Einfluss auf ihre Angriffsfläche. ThemisDB setzt auf ein **unified architecture model** — alle Security-Funktionen sind in eine einzige, integrierte Plattform eingebettet. Das steht im Kontrast zu **polyglot systems**, die verschiedene Spezialwerkzeuge kombinieren.

**Vergleich der Angriffsflächen:**

| Angriffsvektor | Unified System (ThemisDB) | Polyglot System |
|----------------|----------------------------|------------------|
| **Kommunikationswege** | Weniger (alles intern) | Viele (zwischen Komponenten) |
| **Authentifizierungs-Systems** | Ein System (mTLS + Zero-Trust) | Mehrere (TLS hier, JWT dort, API-Keys woanders) |
| **Audit-Log-Integrität** | Ein log, eine Signatur | Multiple Logs, müssen korreliert werden |
| **Schlüsselmanagement** | Zentral (HSM-integriert) | Verstreut (verschiedene KMS) |
| **Patch-Management** | Ein System aktualisieren | Mehrere Systeme koordinieren |
| **Misconfiguration-Risiko** | Gering (konsistente Konfiguration) | Hoch (jede Komponente anders) |
| **Zero-Day-Impact** | Begrenzt (ein System betroffen) | Breit (mehrere Systeme potenziell betroffen) |
| **Compliance-Nachweis** | Einfacher (alles an einem Ort) | Komplex (mehrere Systeme auditen) |

**Konkrete Beispiele aus ThemisDB:**

**Verringerte Angriffsfläche durch Integration:**
- **TLS 1.3 + mTLS**: Alle internen Kommunikationen verwenden mutual TLS — kein Vertrauen ohne gegenseitige Authentifizierung
- **Zero-Trust-Policy-Enforcer**: Jede Anfrage wird verifiziert, bevor RBAC/ABAC angewendet wird — keine implizite Session-Trust
- **HSM-Integration**: Alle Master Keys sind hardware-geschützt — kein Zugriff ohne physische oder logische HSM-Authentifizierung
- **FIPS-Mode**: Genehmigte Algorithmen nur — keine Backdoors durch schwache Kryptographie

**Polyglot-Fallen (die ThemisDB vermeidet):**
- **"Best-of-Breed" Paradox**: Jede Komponente ist vielleicht die beste in ihrem Bereich, aber die Integration schafft neue Schwachstellen
- **Konfigurations-Drift**: Unterschiedliche Systeme entwickeln sich auseinander — Security-Policies werden inkonsistent
- **Responsibility Gaps**: Wenn etwas schiefgeht, zeigt jeder auf den anderen
- **Performance-Overhead**: Jede Systemgrenze bedeutet Serialisierung, Deserialisierung, Netzwerk-Latenz

**Praktische Empfehlung für Entscheider**:

| Entscheidung | Unified (ThemisDB) | Polyglot |
|--------------|--------------------|----------|
| **Anschaffungskosten** | Höher (ein System) | Geringer (Open Source Komponenten) |
| **Betriebskosten** | Geringer (weniger Integration) | Höher (mehr Maintenance) |
| **Security-Kosten** | Geringer (integriert) | Höher (mehr Audits) |
| **Compliance-Kosten** | Geringer (ein Audit) | Höher (mehrere Audits) |
| **Flexibilität** | Geringer (fest integriert) | Höher (austauschbare Komponenten) |
| **Time-to-Market** | Schneller (alles da) | Langsamer (Integration nötig) |

Für die meisten Unternehmen — besonders im regulierten Umfeld — überwiegen die Vorteile des Unified-Ansatzes bei weitem.

---

## 90-Tage-Hardening-Plan: Von Default zu Production-Ready

Dieser Plan führt Sie in 90 Tagen von einer Standard-ThemisDB-Installation zu einer production-hardened, compliance-fähigen Plattform.

### Phase 1: Quick Wins (Tage 1–30)

**Ziel:** Grundlegende Security-Kontrollen aktivieren

- [ ] **TLS 1.3 aktivieren** für alle Client-Verbindungen
  - Zertifikate generieren und in `config/tls.yaml` konfigurieren
  - TLS 1.2 und älter explizit deaktivieren
  - Zertifikatsrotation alle 90 Tage einplanen

- [ ] **mTLS für interne Kommunikation** aktivieren
  - Node-Zertifikate für jeden Cluster-Knoten generieren
  - `mtls.enabled: true` in `config/cluster.yaml` setzen
  - Zertifikats-CRL (Certificate Revocation List) einrichten

- [ ] **Audit-Logging aktivieren** mit voller Detailtiefe
  - `audit_logging.level: "full"` in `config/security.yaml`
  - Log-Rotation konfigurieren (z. B. täglich, 30 Tage Aufbewahrung)
  - SIEM-Integration einrichten (Splunk, ELK, oder Cloud SIEM)

- [ ] **MFA für alle Admin-Zugriffe** erzwingen
  - TOTP oder Hardware-Tokens für alle Benutzer mit Admin-Rechten
  - Recovery-Codes sicher speichern (HSM oder Offline)
  - `mfa.enabled: true` in `config/auth.yaml`

- [ ] **Standard-Passwörter ändern** und komplexe Richtlinien erzwingen
  - Mindestens 12 Zeichen, Groß-/Kleinschreibung, Zahlen, Sonderzeichen
  - Passwort-History (letzte 5 Passwörter dürfen nicht wiederverwendet werden)
  - Account-Lockout nach 5 fehlgeschlagenen Versuchen

- [ ] **VRAM Secure Clear aktivieren** (für GPU-Deployments)
  - `gpu_security.vram_secure_clear.enabled: true`
  - 3-Pass-Override für Compliance (GDPR, HIPAA, SOC 2)

- [ ] **FIPS-Mode testen** (wenn verfügbar)
  - `fips_mode.enabled: true` in `config/security.yaml`
  - Verifizieren, dass alle Anwendungen mit genehmigten Algorithmen funktionieren

### Phase 2: Mid-Term Hardening (Tage 31–60)

**Ziel:** Fortgeschrittene Security-Kontrollen implementieren

- [ ] **HSM-Integration** für Master Keys
  - HSM-Anbieter auswählen (Thales Luna, Utimaco, AWS CloudHSM, etc.)
  - PKCS#11-Bibliothek installieren und konfigurieren
  - Master Key in HSM generieren und speichern
  - `hsm.provider: "pkcs11"` und `hsm.library_path` in `config/security.yaml`

- [ ] **Schlüsselrotations-Policy implementieren**
  - Key-Hierarchie definieren: Master → KEK → DEK
  - Rotationsintervalle festlegen (Master: 3 Jahre, KEK: 2 Jahre, DEK: 1 Jahr)
  - Automatisierte Rotationsskripte erstellen und testen
  - `key_rotation.automatic: true` in `config/security.yaml`

- [ ] **Zero-Trust-Policy-Enforcer konfigurieren**
  - Network Policies für alle Benutzer und Rollen definieren
  - IP-CIDR-Bereiche für jeden Zugriffstyp festlegen
  - Trust-Score-Schwellenwerte konfigurieren (z. B. Minimum 0.8 für Admin-Zugriffe)
  - `zero_trust.default_deny: true` (Deny-by-Default)

- [ ] **Feldlevel-Verschlüsselung aktivieren** für sensible Daten
  - PII-Felder identifizieren (Name, Adresse, E-Mail, Telefon, etc.)
  - Verschlüsselungsmodus auswählen (DOCUMENT, ARRAY, oder VRAM)
  - DEKs pro Dokument oder Feldgruppe generieren
  - `encryption.field_level.enabled: true` in `config/encryption.yaml`

- [ ] **RBAC/ABAC-Policies verfeinern**
  - Rollenhierarchie definieren (Admin, Power User, User, Guest)
  - Attribute-based Access Control für feinere Granularität
  - Prinzip des geringsten Privilegs (Least Privilege) anwenden
  - Regelmäßige Policy-Reviews einplanen (quartalsweise)

- [ ] **Post-Quantum-Cryptography testen**
  - Kyber-1024 für Schlüsselkapselung aktivieren
  - Dilithium-5 für Signaturen testen
  - Performance-Impact messen (Ziel: < 5% Overhead)
  - `pqc.enabled: true` in `config/security.yaml`

- [ ] **Backup- und Disaster-Recovery-Policy definieren**
  - Verschlüsselte Backups (mit HSM-geschützten Keys)
  - Backup-Frequenz: Täglich für kritische Daten, wöchentlich für weniger kritische
  - Backup-Validation: Regelmäßig Test-Restores durchführen
  - Geo-Redundanz für Backups (mindestens 2 Standorte)

### Phase 3: Governance & Continuous Compliance (Tage 61–90)

**Ziel:** Nachhaltige Security-Prozesse etablieren

- [ ] **Security-Awareness-Training** für alle Mitarbeiter
  - Phishing-Erkennung und -Prävention
  - Social-Engineering-Szenarien
  - Passwort-Hygiene
  - Incident-Reporting-Prozesse

- [ ] **Compliance-Framework auswählen und implementieren**
  - ISO 27001, SOC 2, HIPAA, oder PCI DSS (je nach Branche)
  - Gap-Analyse durchführen (was ist bereits implementiert, was fehlt?)
  - Remediation-Plan erstellen und umsetzen
  - Externe Audits vorbereiten

- [ ] **Incident-Response-Plan erstellen und testen**
  - Rollen und Verantwortlichkeiten definieren
  - Eskalationspfade festlegen
  - Kommunikationstemplates vorbereiten (intern und extern)
  - Regelmäßige Tabletop-Exercises durchführen (quartalsweise)

- [ ] **Continuous Security-Monitoring** einrichten
  - SIEM-Regeln für ThemisDB-spezifische Ereignisse
  - Alerts für verdächtige Aktivitäten (multiple fehlgeschlagene Logins, ungewöhnliche Zugriffe, etc.)
  - Automatisierte Response-Aktionen (z. B. Account-Lockout nach 5 fehlgeschlagenen Versuchen)
  - Regelmäßige Log-Reviews (täglich für kritische, wöchentlich für andere)

- [ ] **Vulnerability-Management-Prozess** etablieren
  - Regelmäßige Scans (wöchentlich für kritische Systeme, monatlich für andere)
  - Patch-Management-Prozess (Test → Staging → Production)
  - CVE-Monitoring für alle Abhängigkeiten
  - Zero-Day-Response-Plan

- [ ] **Third-Party-Risk-Management** implementieren
  - Vendor-Security-Assessments durchführen
  - Vertragliche Security-Anforderungen definieren
  - Continuity-of-Operations-Plan für kritische Vendor
  - Regelmäßige Vendor-Reviews (jährlich)

- [ ] **Business-Continuity- und Disaster-Recovery-Plan** finalisieren
  - RTO (Recovery Time Objective) und RPO (Recovery Point Objective) definieren
  - Notfallkommunikationsplan erstellen
  - Backup- und Restore-Prozesse testen (mindestens halbjährlich)
  - Failover- und Failback-Prozeduren dokumentieren

---

## Konfigurationsleitlinien nach Bedrohungsmodell

Nicht alle Organisationen haben die gleichen Security-Anforderungen. Passen Sie Ihre ThemisDB-Konfiguration an Ihr spezifisches Bedrohungsmodell an.

### Bedrohungsmodell 1: Standard-Unternehmen (Basis-Schutz)

**Charakteristika:**
- Keine spezifischen regulatorischen Anforderungen
- Moderne IT-Umgebung
- Moderates Risikoprofil

**Empfohlene Konfiguration:**

```yaml
# config/security.yaml
security_level: "standard"

tls:
  enabled: true
  version: "1.3"
  cipher_suites: "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256"

mtls:
  enabled: true

audit_logging:
  enabled: true
  level: "standard"
  retention_days: 90

encryption:
  at_rest: true
  algorithm: "AES-256-GCM"
  field_level: false

mfa:
  enabled: true
  totp:
    time_step_seconds: 30

fips_mode:
  enabled: false

pqc:
  enabled: false
```

**Security Score:** ~75/100

---

### Bedrohungsmodell 2: Reguliertes Unternehmen (GDPR, SOC 2, HIPAA)

**Charakteristika:**
- Compliance-Anforderungen (GDPR, SOC 2, HIPAA, etc.)
- Sensible Daten (PII, PHI, Finanzdaten)
- Externe Audits

**Empfohlene Konfiguration:**

```yaml
# config/security.yaml
security_level: "compliance"

tls:
  enabled: true
  version: "1.3"
  cipher_suites: "TLS_AES_256_GCM_SHA384"
  cert_rotation_days: 90

mtls:
  enabled: true
  cert_rotation_days: 60

audit_logging:
  enabled: true
  level: "full"
  retention_days: 365
  hsm_signing: true

encryption:
  at_rest: true
  algorithm: "AES-256-GCM"
  field_level: true
  key_rotation:
    dek_days: 365
    kek_years: 2
    master_years: 3

mfa:
  enabled: true
  totp:
    time_step_seconds: 30
  hardware_tokens: true

fips_mode:
  enabled: true

hsm:
  provider: "pkcs11"
  library_path: "/opt/thales/luna/client/libCryptoki.so"
  slot_id: 0

zero_trust:
  enabled: true
  default_deny: true
  trust_score_threshold: 0.8

pqc:
  enabled: true
  kyber_level: 1024
  dilithium_level: 5
```

**Security Score:** ~90/100

---

### Bedrohungsmodell 3: Hochsensible Umgebung (Finanzdienstleistungen, Regierung, Gesundheitswesen)

**Charakteristika:**
- Extrem hohe Compliance-Anforderungen (PCI DSS, BSI C5, FedRAMP, etc.)
- Hochsensible Daten (Finanztransaktionen, klassifizierte Informationen, Patientendaten)
- Ziel von gezielten Angriffen (APT, Nation-State)

**Empfohlene Konfiguration:**

```yaml
# config/security.yaml
security_level: "maximum"

tls:
  enabled: true
  version: "1.3"
  cipher_suites: "TLS_AES_256_GCM_SHA384"
  cert_rotation_days: 30
  ocsp_stapling: true

mtls:
  enabled: true
  cert_rotation_days: 30
  crl_check: true

audit_logging:
  enabled: true
  level: "full"
  retention_days: 730  # 2 Jahre
  hsm_signing: true
  tamper_evident: true

encryption:
  at_rest: true
  algorithm: "AES-256-GCM"
  field_level: true
  per_document_dek: true
  key_rotation:
    dek_days: 180
    kek_years: 1
    master_years: 2

mfa:
  enabled: true
  totp:
    time_step_seconds: 30
  hardware_tokens: true
  require_both: true  # TOTP + Hardware Token

fips_mode:
  enabled: true
  strict: true  # Kein Graceful Degradation

hsm:
  provider: "pkcs11"
  library_path: "/opt/ncipher/nfast/km/lib/libnCipherKM.so"
  slot_id: 0
  require_fips_140_3: true

zero_trust:
  enabled: true
  default_deny: true
  trust_score_threshold: 0.9
  ip_lockdown: true

pqc:
  enabled: true
  kyber_level: 1024
  dilithium_level: 5

gpu_security:
  vram_secure_clear:
    enabled: true
    num_passes: 7
    verify_clear: true
```

**Security Score:** ~95+/100

---

## Häufige Fehlannahmen in Audits

Auditoren sehen immer wieder dieselben Probleme. Hier sind die häufigsten Fehlannahmen — und wie Sie sie in ThemisDB vermeiden.

### Fehlannahme 1: "Wir sind compliance-fähig, weil wir ein HSM verwenden"

**Realität:** Ein HSM allein garantiert keine Compliance. Wichtig ist:
- **Wie** das HSM verwendet wird (FIPS 140-2 Level 2 minimum, Level 3 empfohlen)
- **Was** im HSM gespeichert wird (Master Keys müssen im HSM sein, DEKs können verschlüsselt außerhalb gespeichert werden)
- **Wer** Zugriff auf das HSM hat (strikte Access Controls, Multi-Person Approval für Admin-Operationen)
- **Wie** Schlüssel verwaltet werden (Rotation, Backup, Recovery)

**ThemisDB-Lösung:** HSM-Integration mit Key-Rotation-State-Machine und FIPS-Mode-Enforcement.

---

### Fehlannahme 2: "Audit-Logs sind nur für die IT-Abteilung"

**Realität:** Audit-Logs sind rechtliche Dokumente. Sie müssen:
- **Unveränderlich** sein (Hash-Chain, Signaturen)
- **Vollständig** sein (alle relevanten Ereignisse)
- **Verständlich** sein (nicht nur für Techniker, sondern auch für Auditoren und Juristen)
- **Verfügbar** sein (für die gesamte Aufbewahrungsfrist)

**ThemisDB-Lösung:** Tamper-evident Audit-Trail mit Dilithium-Signaturen und langfristiger Archivierung.

---

### Fehlannahme 3: "Verschlüsselung ist gleich Verschlüsselung"

**Realität:** Nicht alle Verschlüsselung ist gleich. Wichtig ist:
- **Algorithmus**: AES-256-GCM ist sicher, DES ist es nicht
- **Schlüssellänge**: 256 Bit ist sicher, 56 Bit (DES) ist gebrochen
- **Schlüsselmanagement**: Der Schlüssel muss genauso gut geschützt sein wie die Daten
- **Implementierung**: Eine schwache Implementierung kann einen starken Algorithmus unsicher machen

**Häufige Fehler:**
- Verwendung von veralteten Algorithmen (DES, 3DES, RC4)
- Zu kurze Schlüssel (AES-128 statt AES-256)
- Schlüssel in Konfigurationsdateien speichern
- Keine Schlüsselrotation

**ThemisDB-Lösung:** FIPS 140-3 Mode erzwingt genehmigte Algorithmen, HSM-Integration schützt Schlüssel.

---

### Fehlannahme 4: "Zero Trust ist ein Produkt, das man kauft"

**Realität:** Zero Trust ist ein **Prinzip** (never trust, always verify), kein Produkt. Implementierung erfordert:
- **Identitätsverifizierung** für jeden Request (nicht nur beim Login)
- **Netzwerk-Segmentierung** und Mikro-Segmentierung
- **Least Privilege Access** (jeder Benutzer hat nur die minimalen Rechte, die er braucht)
- **Continuous Monitoring** und Anomalie-Erkennung

**Häufige Fehler:**
- Zero Trust als "Produkt" betrachten, das man installiert
- Nur die Netzwerk-Ebene betrachten, nicht die Anwendungsebene
- Existing Trust Relationships nicht überprüfen

**ThemisDB-Lösung:** Zero-Trust-Policy-Enforcer mit per-Request-Verifizierung und Trust-Score.

---

### Fehlannahme 5: "Post-Quantum-Cryptography brauchen wir erst in 10 Jahren"

**Realität:** Harvest-Now-Decrypt-Later-Angriffe sind **heute** bereits ein Risiko. Jede Organisation, die Daten mit einer Vertraulichkeitsanforderung von > 10 Jahren speichert, muss heute handeln.

**Betroffene Daten:**
- Medizinische Unterlagen (Aufbewahrungspflicht: 10–30 Jahre)
- Finanztransaktionen (7–10 Jahre)
- Personalakten (bis zu 100 Jahre nach Beschäftigungsende)
- Klassifizierte Informationen (unbegrenzte Aufbewahrung)

**ThemisDB-Lösung:** CRYSTALS-Kyber-1024 und Dilithium-5 Integration mit hybridem Verschlüsselungsansatz.

---

### Fehlannahme 6: "Compliance ist ein einmaliger Prozess"

**Realität:** Compliance ist ein **kontinuierlicher Prozess**. Regulatorische Anforderungen ändern sich, Bedrohungen entwickeln sich weiter, Technologien reifen.

**Wichtige Zyklen:**
- **Täglich:** Log-Reviews, Security-Alerts
- **Wöchentlich:** Vulnerability-Scans, Patch-Management
- **Monatlich:** Compliance-Reports, Access-Reviews
- **Quartalsweise:** Incident-Response-Tests, Policy-Reviews
- **Jährlich:** Externe Audits, Penetrationstests, Risikoassessments

**ThemisDB-Lösung:** Automatisierte Security-Metriken und integrierte Audit-Funktionen.

---

### Fehlannahme 7: "KI-Systeme brauchen keine besondere Security"

**Realität:** KI-Systeme haben **einzigartige Security-Herausforderungen**:
- **Training Data Poisoning**: Manipulation von Trainingsdaten, um das Modell zu kompromittieren
- **Model Extraction**: Angreifer versuchen, das Modell durch Abfragen zu rekonstruieren
- **Prompt Injection**: Manipulation von Prompts, um schädliche Ausgaben zu erzeugen
- **Supply Chain Attacks**: Kompromittierung von Abhängigkeiten (LLM-Bibliotheken, Embedding-Modelle)
- **Grounding Manipulation**: Fälschung von Datenquellen, um falsche Antworten zu erzeugen

**ThemisDB-Lösung:**
- Grounding mit Versionstokens und Signaturen
- Feldlevel-Verschlüsselung für Trainingsdaten
- Zero-Trust für alle KI-Anfragen
- Audit-Trail für alle KI-Interaktionen

---

## Fazit: Security als Wettbewerbsvorteil

In einer Welt, in der Daten das neue Öl sind, ist Security das neue Vertrauen. Organisationen, die ihre KI-Systeme von Anfang an mit Security, Grounding und Compliance im Kern aufbauen, werden nicht nur regulatorische Anforderungen erfüllen — sie werden einen echten Wettbewerbsvorteil haben.

ThemisDB zeigt, wie das geht: Durch die Integration von Post-Quantum-Cryptography, FIPS 140-3 Enforcement, ACID Audit Trails und nativem GDPR Support in eine unified Architecture schafft es eine Plattform, die nicht nur sicher, sondern auch nachvollziehbar und compliance-fähig ist.

**Die drei Säulen des Vertrauens:**

1. **Security**: Schutz vor Bedrohungen von heute und morgen
2. **Grounding**: Nachvollziehbarkeit und Transparenz aller Datenflüsse
3. **Compliance**: Erfüllung aller regulatorischen Anforderungen

Erst wenn alle drei Säulen stark sind, können Organisationen ihre KI-Systeme mit dem Vertrauen einsetzen, das sie für kritische Anwendungsfälle benötigen.

---

*Letzte Aktualisierung: Juni 2026*
*Version: 1.0*
*Zielgruppe: Plattform-Teams, Compliance-Officers, Security-Architekten*
