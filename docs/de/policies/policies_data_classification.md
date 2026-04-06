# ThemisDB Data Classification Policy

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Policies

---


**Version:** 1.0  
**Datum:** 2025-11-27  
**Status:** Aktiv  
**Klassifizierung:** Intern  
**Nächste Überprüfung:** 2026-05-27

---

## 1. Zweck und Geltungsbereich

### 1.1 Zweck

Diese Richtlinie definiert die Klassifizierung, Handhabung und den Schutz von Daten innerhalb der ThemisDB-Infrastruktur. Sie stellt sicher, dass alle Daten entsprechend ihrer Sensitivität behandelt werden.

### 1.2 Geltungsbereich

- Alle in ThemisDB gespeicherten Daten
- Alle Daten in Transit (API, Replikation)
- Backup- und Archivdaten
- Metadaten und Audit-Logs
- Konfigurationsdaten

### 1.3 Compliance-Referenzen

| Standard | Kontrolle | Beschreibung |
|----------|-----------|--------------|
| ISO 27001 | A.8.2 | Information Classification |
| BSI C5 | AM-01/02 | Asset Management |
| DSGVO | Art. 5, 32 | Datenschutzgrundsätze |
| NIST SP 800-53 | RA-2, SC-16 | Security Categorization |
| PCI DSS | 3.1-3.7 | Protect Cardholder Data |

---

## 2. Datenklassifizierungsstufen

### 2.1 Klassifizierungsschema

ThemisDB verwendet ein 4-stufiges Klassifizierungsschema:

```
┌─────────────────────────────────────────────────────────────┐
│                    STRENG VERTRAULICH                        │
│   Höchste Sensitivität - Schwerwiegende Auswirkungen        │
├─────────────────────────────────────────────────────────────┤
│                      VERTRAULICH                             │
│   Hohe Sensitivität - Erhebliche Auswirkungen               │
├─────────────────────────────────────────────────────────────┤
│                        INTERN                                │
│   Moderate Sensitivität - Geringe Auswirkungen              │
├─────────────────────────────────────────────────────────────┤
│                       ÖFFENTLICH                             │
│   Keine Sensitivität - Keine Auswirkungen                   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Stufe 1: STRENG VERTRAULICH (Confidential - Restricted)

**Definition:** Daten, deren Offenlegung schwerwiegende Schäden verursachen würde.

**Beispiele:**
- Kryptographische Schlüssel (Master Keys, KEK)
- Authentifizierungsgeheimnisse (Tokens, Passwörter)
- HSM/Vault-Zugangsdaten
- Root-Zertifikate und Private Keys
- Sicherheitskonfigurationen

**Schutzanforderungen:**
| Aspekt | Anforderung |
|--------|-------------|
| Verschlüsselung at Rest | AES-256-GCM (HSM-geschützt) |
| Verschlüsselung in Transit | TLS 1.3, mTLS |
| Zugriff | Need-to-know, Admin only |
| Audit | Vollständige Protokollierung |
| Backup | Verschlüsselt, HSM-geschützt |
| Aufbewahrung | Nach Ablauf sicher löschen |
| Druck/Export | Verboten |

### 2.3 Stufe 2: VERTRAULICH (Confidential)

**Definition:** Geschäftskritische Daten mit erheblichem Schadenpotenzial.

**Beispiele:**
- Personenbezogene Daten (PII)
- Gesundheitsdaten (PHI)
- Finanzdaten
- Geschäftsgeheimnisse
- Kundendaten
- Audit-Logs mit sensitiven Informationen

**Schutzanforderungen:**
| Aspekt | Anforderung |
|--------|-------------|
| Verschlüsselung at Rest | AES-256-GCM |
| Verschlüsselung in Transit | TLS 1.3 |
| Zugriff | RBAC (Analyst+) |
| Audit | Protokollierung aller Zugriffe |
| Backup | Verschlüsselt |
| Aufbewahrung | Gemäß Retention Policy |
| Druck/Export | Genehmigung erforderlich |

### 2.4 Stufe 3: INTERN (Internal)

**Definition:** Interne Daten ohne erhebliches Schadenpotenzial bei Offenlegung.

**Beispiele:**
- Interne Dokumentation
- Nicht-sensitive Konfigurationen
- Aggregierte Statistiken
- System-Logs (ohne PII)
- Performance-Metriken

**Schutzanforderungen:**
| Aspekt | Anforderung |
|--------|-------------|
| Verschlüsselung at Rest | Optional (empfohlen) |
| Verschlüsselung in Transit | TLS 1.2+ |
| Zugriff | Alle authentifizierten Benutzer |
| Audit | Standard-Logging |
| Backup | Standard |
| Aufbewahrung | Standard |
| Druck/Export | Erlaubt |

### 2.5 Stufe 4: ÖFFENTLICH (Public)

**Definition:** Daten, die öffentlich zugänglich sind oder sein dürfen.

**Beispiele:**
- Öffentliche Dokumentation
- API-Spezifikationen
- Open-Source-Code
- Marketing-Materialien
- Veröffentlichte Release Notes

**Schutzanforderungen:**
| Aspekt | Anforderung |
|--------|-------------|
| Verschlüsselung at Rest | Nicht erforderlich |
| Verschlüsselung in Transit | TLS empfohlen |
| Zugriff | Uneingeschränkt |
| Audit | Optional |
| Backup | Standard |
| Aufbewahrung | Unbegrenzt |
| Druck/Export | Frei |

---

## 3. Datentypen in ThemisDB

### 3.1 Automatische Klassifizierung

ThemisDB unterstützt automatische Datenklassifizierung basierend auf:

```yaml
# Beispiel: Collection-Schema mit Klassifizierung
{
  "collection": "users",
  "classification": "CONFIDENTIAL",
  "fields": {
    "email": {
      "type": "string",
      "classification": "CONFIDENTIAL",
      "pii": true
    },
    "password_hash": {
      "type": "string",
      "classification": "RESTRICTED",
      "sensitive": true
    },
    "username": {
      "type": "string",
      "classification": "INTERNAL"
    }
  }
}
```

### 3.2 Datentyp-Matrix

| Datentyp | Standard-Klassifizierung | Verschlüsselung | Maskierung |
|----------|--------------------------|-----------------|------------|
| Master Keys | STRENG VERTRAULICH | HSM | N/A |
| API Tokens | STRENG VERTRAULICH | AES-256 | Vollständig |
| Passwort-Hashes | STRENG VERTRAULICH | Argon2id | N/A |
| E-Mail-Adressen | VERTRAULICH | AES-256 | Partiell |
| Telefonnummern | VERTRAULICH | AES-256 | Partiell |
| Namen | VERTRAULICH | AES-256 | Optional |
| IP-Adressen | VERTRAULICH | Optional | Optional |
| Audit-Logs | VERTRAULICH | AES-256 | N/A |
| Session-Daten | VERTRAULICH | AES-256 | N/A |
| Konfigurationen | INTERN | Optional | N/A |
| Metriken | INTERN | Nein | N/A |
| Dokumentation | ÖFFENTLICH | Nein | N/A |

### 3.3 Besondere Datenkategorien (DSGVO Art. 9)

Folgende Daten erfordern erhöhten Schutz:

- Gesundheitsdaten → STRENG VERTRAULICH
- Biometrische Daten → STRENG VERTRAULICH
- Genetische Daten → STRENG VERTRAULICH
- Religiöse Überzeugungen → VERTRAULICH
- Politische Meinungen → VERTRAULICH
- Gewerkschaftszugehörigkeit → VERTRAULICH
- Sexuelle Orientierung → STRENG VERTRAULICH

---

## 4. Datenhandhabung

### 4.1 Kennzeichnung (Labeling)

Alle Daten müssen gemäß ihrer Klassifizierung gekennzeichnet werden:

**Dokumente:**
```
[STRENG VERTRAULICH] - Nur für autorisierte Personen
[VERTRAULICH] - Vertraulich behandeln
[INTERN] - Nur für internen Gebrauch
[ÖFFENTLICH] - Öffentlich zugänglich
```

**Datenbank-Dokumente:**
```json
{
  "_meta": {
    "classification": "CONFIDENTIAL",
    "data_owner": "security-team",
    "retention_until": "2027-01-01",
    "pii_fields": ["email", "phone"]
  },
  // ... Daten
}
```

### 4.2 Speicherung

| Klassifizierung | Speicherort | Verschlüsselung | Zugriffskontrolle |
|-----------------|-------------|-----------------|-------------------|
| STRENG VERTRAULICH | HSM/Vault | AES-256-GCM (HSM) | MFA + Approval |
| VERTRAULICH | Encrypted Volume | AES-256-GCM | RBAC (Analyst+) |
| INTERN | Standard Storage | Optional | RBAC (Readonly+) |
| ÖFFENTLICH | Standard Storage | Nein | None |

### 4.3 Übertragung

| Klassifizierung | Internes Netzwerk | Externes Netzwerk | API |
|-----------------|-------------------|-------------------|-----|
| STRENG VERTRAULICH | mTLS + Encryption | Verboten | mTLS + Signed |
| VERTRAULICH | TLS 1.3 | TLS 1.3 + VPN | TLS 1.3 |
| INTERN | TLS 1.2+ | TLS 1.3 | TLS 1.2+ |
| ÖFFENTLICH | Optional | Optional | Optional |

### 4.4 Vernichtung

| Klassifizierung | Methode | Zertifizierung |
|-----------------|---------|----------------|
| STRENG VERTRAULICH | Crypto-Shredding + Secure Erase | Erforderlich |
| VERTRAULICH | Secure Erase (DoD 5220.22-M) | Empfohlen |
| INTERN | Standard Delete | Nicht erforderlich |
| ÖFFENTLICH | Standard Delete | Nicht erforderlich |

---

## 5. Zugriffskontrolle nach Klassifizierung

### 5.1 Rollen-Matrix

| Klassifizierung | Admin | Operator | Analyst | Readonly |
|-----------------|-------|----------|---------|----------|
| STRENG VERTRAULICH | ✅ (mit Approval) | ❌ | ❌ | ❌ |
| VERTRAULICH | ✅ | ✅ (eingeschränkt) | ✅ (lesend) | ❌ |
| INTERN | ✅ | ✅ | ✅ | ✅ |
| ÖFFENTLICH | ✅ | ✅ | ✅ | ✅ |

### 5.2 Need-to-Know-Prinzip

```
┌─────────────────────────────────────────────┐
│           Datenzugriff erlaubt wenn:        │
├─────────────────────────────────────────────┤
│ 1. Benutzer hat entsprechende Rolle         │
│ 2. Benutzer hat Need-to-Know                │
│ 3. Zugriff ist für Aufgabe erforderlich     │
│ 4. Genehmigung vorhanden (bei RESTRICTED)   │
└─────────────────────────────────────────────┘
```

---

## 6. Datenschutz-spezifische Anforderungen

### 6.1 Personenbezogene Daten (PII)

| Anforderung | Umsetzung |
|-------------|-----------|
| Minimierung | Nur erforderliche Daten speichern |
| Zweckbindung | Dokumentierte Verarbeitungszwecke |
| Speicherbegrenzung | Retention Policy |
| Richtigkeit | Regelmäßige Validierung |
| Integrität | Signierte Audit-Logs |
| Vertraulichkeit | Verschlüsselung |

### 6.2 Betroffenenrechte

| Recht | Umsetzung in ThemisDB |
|-------|------------------------|
| Auskunft (Art. 15) | Data Export API |
| Berichtigung (Art. 16) | Standard Update |
| Löschung (Art. 17) | Hard Delete mit Audit |
| Einschränkung (Art. 18) | Status-Flag |
| Datenübertragbarkeit (Art. 20) | JSON/CSV Export |
| Widerspruch (Art. 21) | Consent Management |

---

## 7. Implementierung in ThemisDB

### 7.1 Collection-Level Klassifizierung

```cpp
// C++ Implementation
class Collection {
    DataClassification classification;
    EncryptionPolicy encryption_policy;
    RetentionPolicy retention_policy;
    
    bool requiresEncryption() const {
        return classification >= DataClassification::CONFIDENTIAL;
    }
    
    bool requiresAudit() const {
        return classification >= DataClassification::INTERNAL;
    }
};
```

### 7.2 Field-Level Klassifizierung

```cpp
// Field-Level Encryption for PII
class FieldEncryption {
    static std::string encrypt_pii(
        const std::string& value,
        DataClassification level) {
        
        switch(level) {
            case RESTRICTED:
                return hsm_encrypt(value);
            case CONFIDENTIAL:
                return aes256_encrypt(value);
            default:
                return value;
        }
    }
};
```

### 7.3 Query-Filterung

```cpp
// Classification-aware Query
class QueryExecutor {
    ResultSet execute(Query query, UserContext user) {
        // Filter results based on user's clearance
        auto filtered = filterByClassification(
            results, 
            user.getClearanceLevel()
        );
        
        // Mask sensitive fields
        return maskSensitiveFields(filtered, user);
    }
};
```

---

## 8. Audit und Überwachung

### 8.1 Klassifizierungs-Audit

| Ereignis | Log-Level | Retention |
|----------|-----------|-----------|
| Klassifizierung geändert | CRITICAL | 7 Jahre |
| Zugriff auf RESTRICTED | WARNING | 3 Jahre |
| Zugriff auf CONFIDENTIAL | INFO | 1 Jahr |
| Datenexport | INFO | 3 Jahre |
| Löschung | CRITICAL | 7 Jahre |

### 8.2 Automatische Erkennung

ThemisDB kann PII automatisch erkennen:

```yaml
pii_detection:
  enabled: true
  patterns:
    - name: email
      regex: "^[\\w.-]+@[\\w.-]+\\.\\w+$"
      classification: CONFIDENTIAL
    - name: credit_card
      regex: "^\\d{4}[- ]?\\d{4}[- ]?\\d{4}[- ]?\\d{4}$"
      classification: RESTRICTED
    - name: ssn
      regex: "^\\d{3}-\\d{2}-\\d{4}$"
      classification: RESTRICTED
```

---

## 9. Ausnahmen und Eskalation

### 9.1 Ausnahmeprozess

Ausnahmen von dieser Richtlinie erfordern:

1. **Antrag:** Schriftliche Begründung
2. **Risikobewertung:** Security-Team-Review
3. **Genehmigung:** CISO/DPO-Freigabe
4. **Dokumentation:** Ausnahme-Register
5. **Befristung:** Max. 12 Monate
6. **Review:** Regelmäßige Überprüfung

### 9.2 Eskalation bei Verstößen

| Schweregrad | Verstoß | Eskalation |
|-------------|---------|------------|
| Kritisch | Offenlegung RESTRICTED | CISO + Management sofort |
| Hoch | Offenlegung CONFIDENTIAL | Security-Team sofort |
| Mittel | Falsche Klassifizierung | Data Owner binnen 24h |
| Niedrig | Fehlende Kennzeichnung | IT-Team binnen 7 Tage |

---

## 10. Schulung und Awareness

### 10.1 Pflichtschulungen

| Zielgruppe | Schulung | Häufigkeit |
|------------|----------|------------|
| Alle Mitarbeiter | Grundlagen Datenklassifizierung | Jährlich |
| Entwickler | Secure Coding & Data Handling | Halbjährlich |
| Admins | Advanced Classification | Halbjährlich |
| Management | Executive Awareness | Jährlich |

### 10.2 Schulungsinhalte

- Klassifizierungsschema verstehen
- Richtige Kennzeichnung
- Handhabungsregeln
- Meldepflichten bei Verstößen
- Praktische Übungen

---

## 11. Review und Aktualisierung

### 11.1 Review-Zyklus

| Aktivität | Häufigkeit | Verantwortlich |
|-----------|------------|----------------|
| Richtlinien-Review | Halbjährlich | CISO |
| Klassifizierungs-Audit | Quartalsweise | Data Governance |
| Datenbestandsprüfung | Jährlich | Data Owners |
| Compliance-Assessment | Jährlich | Compliance Team |

### 11.2 Änderungshistorie

| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| 1.0 | 2025-11-27 | Erstversion | Security Team |

---

## 12. Anhänge

### Anhang A: Quick Reference Card

```
╔═══════════════════════════════════════════════════════════════╗
║          DATENKLASSIFIZIERUNG - KURZREFERENZ                  ║
╠═══════════════════════════════════════════════════════════════╣
║ 🔴 STRENG VERTRAULICH                                         ║
║    → Schlüssel, Passwörter, Gesundheitsdaten                  ║
║    → HSM-Verschlüsselung, Admin-only, kein Export             ║
╠═══════════════════════════════════════════════════════════════╣
║ 🟠 VERTRAULICH                                                 ║
║    → PII, Finanzdaten, Kundendaten                            ║
║    → AES-256 Verschlüsselung, RBAC, Audit-Logs                ║
╠═══════════════════════════════════════════════════════════════╣
║ 🟡 INTERN                                                      ║
║    → Interne Dokumente, Configs, Metriken                     ║
║    → TLS in Transit, Standard-Zugriff                         ║
╠═══════════════════════════════════════════════════════════════╣
║ 🟢 ÖFFENTLICH                                                  ║
║    → Dokumentation, Marketing, Open Source                    ║
║    → Keine Einschränkungen                                    ║
╚═══════════════════════════════════════════════════════════════╝
```

### Anhang B: Klassifizierungs-Entscheidungsbaum

```
                    ┌─────────────────────┐
                    │ Würde Offenlegung   │
                    │ schweren Schaden    │
                    │ verursachen?        │
                    └─────────┬───────────┘
                              │
              ┌───────────────┴───────────────┐
              │ JA                            │ NEIN
              ▼                               ▼
    ┌─────────────────┐           ┌─────────────────┐
    │ Enthält Master  │           │ Enthält PII,    │
    │ Keys, Secrets?  │           │ Finanzdaten?    │
    └────────┬────────┘           └────────┬────────┘
             │                              │
     ┌───────┴───────┐              ┌───────┴───────┐
     │ JA            │ NEIN         │ JA            │ NEIN
     ▼               ▼              ▼               ▼
┌─────────┐   ┌─────────┐    ┌─────────┐    ┌─────────────┐
│STRENG   │   │STRENG   │    │VERTRAU- │    │ Nur für     │
│VERTRAU- │   │VERTRAU- │    │LICH     │    │ internen    │
│LICH     │   │LICH     │    │         │    │ Gebrauch?   │
└─────────┘   └─────────┘    └─────────┘    └──────┬──────┘
                                                    │
                                            ┌───────┴───────┐
                                            │ JA            │ NEIN
                                            ▼               ▼
                                      ┌─────────┐    ┌───────────┐
                                      │ INTERN  │    │ ÖFFENTLICH│
                                      └─────────┘    └───────────┘
```

---

**Genehmigt von:**

| Rolle | Name | Datum | Unterschrift |
|-------|------|-------|--------------|
| CISO | _________________ | __________ | _____________ |
| DPO | _________________ | __________ | _____________ |
| CTO | _________________ | __________ | _____________ |
