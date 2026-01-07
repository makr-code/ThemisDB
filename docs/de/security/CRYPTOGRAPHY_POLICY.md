# Kryptographie-Policy für ThemisDB

**Version:** 1.0  
**Datum:** 15. Dezember 2025  
**Status:** Gültig  
**Genehmigt durch:** ThemisDB Security Team  
**Nächste Überprüfung:** 15. Juni 2026

---

## 1. Zweck und Geltungsbereich

### 1.1 Zweck

Diese Kryptographie-Policy definiert die Standards und Anforderungen für den Einsatz kryptographischer Verfahren in ThemisDB. Sie stellt sicher, dass alle kryptographischen Operationen den aktuellen Best Practices und Compliance-Anforderungen (BSI C5, ISO 27001, DSGVO) entsprechen.

### 1.2 Geltungsbereich

Diese Policy gilt für:
- Alle kryptographischen Operationen in ThemisDB
- Verschlüsselung von Daten at-rest und in-transit
- Schlüsselmanagement und -verwaltung
- Entwicklung, Betrieb und Wartung

### 1.3 Referenzen

- **BSI C5 (2020):** Cloud Computing Compliance Criteria Catalogue, Kontrollen CRY-01 bis CRY-06
- **BSI TR-02102-1:** Kryptographische Verfahren: Empfehlungen und Schlüssellängen (2024-01)
- **ISO/IEC 27001:2013:** Informationssicherheit, Anhang A.10 (Kryptographie)
- **NIST SP 800-38D:** Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM)
- **NIST SP 800-57:** Recommendation for Key Management

---

## 2. Kryptographische Standards

### 2.1 Zugelassene Algorithmen

#### 2.1.1 Symmetrische Verschlüsselung

**Primär:**
```yaml
Algorithmus:     AES-256-GCM (Advanced Encryption Standard)
Modus:          Galois/Counter Mode (AEAD - Authenticated Encryption with Associated Data)
Schlüssellänge: 256 bit (32 bytes)
IV-Länge:       96 bit (12 bytes) - zufällig generiert pro Operation
Tag-Länge:      128 bit (16 bytes) - für Authentifizierung
```

**Begründung:**
- AES-256 ist BSI-empfohlen für "VS-NfD" (Verschlusssache - Nur für den Dienstgebrauch)
- GCM bietet sowohl Vertraulichkeit als auch Integrität (AEAD)
- Keine bekannten praktischen Angriffe auf AES-256-GCM

**Alternative (falls erforderlich):**
```yaml
Algorithmus:     ChaCha20-Poly1305
Schlüssellänge: 256 bit
Nonce-Länge:    96 bit
Tag-Länge:      128 bit
```

**NICHT zugelassen:**
- ❌ AES-ECB (keine Authentifizierung, Pattern Leakage)
- ❌ DES, 3DES (veraltet, zu kurze Schlüssel)
- ❌ RC4 (Bias in Keystream)
- ❌ AES-CBC ohne HMAC (keine Authentifizierung)

#### 2.1.2 Asymmetrische Verschlüsselung

**RSA:**
```yaml
Schlüssellänge: Minimum 2048 bit, empfohlen 4096 bit
Padding:        OAEP (Optimal Asymmetric Encryption Padding) mit SHA-256
Verwendung:     Schlüsselaustausch, Zertifikate
```

**Elliptic Curve (ECDH, ECDSA):**
```yaml
Kurven:         P-256 (NIST), P-384, Curve25519
Verwendung:     TLS, Schlüsselaustausch
```

**NICHT zugelassen:**
- ❌ RSA < 2048 bit
- ❌ RSA ohne Padding
- ❌ Schwache Kurven (< 256 bit)

#### 2.1.3 Hashing und Message Authentication

**Hash-Funktionen:**
```yaml
Primär:     SHA-256, SHA-384, SHA-512
Verwendung: Integritätsprüfung, Signaturen, HMAC
```

**Message Authentication Codes (MAC):**
```yaml
Algorithmus: HMAC-SHA256
Verwendung:  Nachrichtenauthentifizierung
```

**Key Derivation:**
```yaml
Algorithmus: HKDF-SHA256 (HMAC-based Key Derivation Function)
Verwendung:  Content-based metadata, Schlüsselableitung
```

**NICHT zugelassen:**
- ❌ MD5 (Kollisionen gefunden)
- ❌ SHA-1 (schwach, Sunset 2020)

#### 2.1.4 Digitale Signaturen

```yaml
RSA:        RSA-SHA256, Minimum 2048 bit
ECDSA:      P-256 oder P-384
EdDSA:      Ed25519 (für moderne Anwendungen)
```

### 2.2 Transport Layer Security (TLS)

```yaml
Protokoll:      TLS 1.3 (primär), TLS 1.2 (fallback)
Cipher Suites:  
  - TLS_AES_256_GCM_SHA384
  - TLS_CHACHA20_POLY1305_SHA256
  - ECDHE-RSA-AES256-GCM-SHA384
  - ECDHE-RSA-AES128-GCM-SHA256

NICHT zugelassen:
  - SSLv2, SSLv3 (unsicher)
  - TLS 1.0, TLS 1.1 (veraltet)
  - Cipher Suites mit RC4, DES, MD5
  - NULL-Cipher
  - Export-Grade Cipher
```

---

## 3. Schlüsselmanagement

### 3.1 Schlüsselhierarchie

ThemisDB verwendet eine zweistufige Schlüsselhierarchie:

```
┌────────────────────────────────────────┐
│ Key Encryption Key (KEK)               │
│ - Gespeichert: Vault/KMS/HSM           │
│ - Rotation: Jährlich                   │
│ - Länge: 256 bit                       │
└──────────────┬─────────────────────────┘
               │ verschlüsselt
               ▼
┌────────────────────────────────────────┐
│ Data Encryption Keys (DEK)             │
│ - Gespeichert: Verschlüsselt im Cache  │
│ - Rotation: Vierteljährlich            │
│ - Länge: 256 bit                       │
│ - TTL: 1 Stunde (Cache)                │
└────────────────────────────────────────┘
```

### 3.2 Schlüsselerzeugung

**Anforderungen:**
- Verwendung kryptographisch sicherer Zufallszahlengeneratoren (CSPRNG)
- OpenSSL `RAND_bytes()` oder `/dev/urandom` (Linux)
- Mindest-Entropie: 256 bit für symmetrische Schlüssel

**Verfahren:**
```cpp
// Korrekt:
std::vector<uint8_t> key(32);  // 256 bit
if (RAND_bytes(key.data(), key.size()) != 1) {
    throw KeyGenerationException("Insufficient entropy");
}

// NICHT zulässig:
std::srand(time(NULL));  // ❌ std::rand() ist NICHT kryptographisch sicher
```

### 3.3 Schlüsselspeicherung

**KEK (Key Encryption Keys):**
- **MUSS** in externem Key Management System gespeichert werden:
  - HashiCorp Vault (empfohlen)
  - AWS KMS, Azure Key Vault, GCP KMS
  - Hardware Security Module (HSM) mit PKCS#11
- **DARF NICHT** im Klartext auf Festplatte gespeichert werden
- Zugriff nur über authentifizierte Service-Principals

**DEK (Data Encryption Keys):**
- Verschlüsselt mit KEK
- Cache: Max. 1000 Keys, 1 Stunde TTL
- LRU-Eviction bei Cache-Überlauf
- Im Memory nur für aktive Operationen

**Verboten:**
- ❌ Hardcoded Keys im Quellcode
- ❌ Keys in Konfigurationsdateien (ohne Verschlüsselung)
- ❌ Keys in Versionskontrolle (Git)
- ❌ Keys in Logdateien

### 3.4 Schlüsselrotation

**Rotationsfrequenzen:**

| Schlüsseltyp | Reguläre Rotation | Notfall-Rotation |
|--------------|-------------------|------------------|
| KEK (Master) | Jährlich | < 24 Stunden |
| DEK (Data)   | Vierteljährlich | < 7 Tage |
| TLS-Zertifikate | Jährlich (90 Tage empfohlen) | < 24 Stunden |

**Rotationsprozess (Vier-Phasen-Modell):**

1. **Phase 1: Dual-Write** (Woche 1-2)
   - Neuer Schlüssel wird erstellt
   - Neue Daten mit neuem Schlüssel verschlüsselt
   - Alte Daten mit altem Schlüssel entschlüsselbar

2. **Phase 2: Re-Encryption** (Woche 3-4)
   - Hintergrundjob verschlüsselt alte Daten neu
   - Progress Tracking und Monitoring
   - Idempotente Operation (Wiederholbar)

3. **Phase 3: Deprecation** (Woche 5)
   - Alter Schlüssel wird als DEPRECATED markiert
   - Monitoring für unerwartete Zugriffe
   - 30 Tage Übergangszeit

4. **Phase 4: Deletion** (Woche 8+)
   - Alter Schlüssel wird gelöscht
   - Audit-Log-Eintrag
   - Bestätigung durch Security Team

**Notfall-Rotation (bei Kompromittierung):**
- Sofortige Deaktivierung des kompromittierten Schlüssels
- Erstellung eines neuen Schlüssels innerhalb 1 Stunde
- Re-Encryption mit hoher Priorität (24-48 Stunden)
- Incident Report und Root-Cause-Analysis

### 3.5 Schlüsselvernichtung

**Verfahren:**
- Sicheres Überschreiben (mindestens 1x mit Zufallsdaten)
- Verwendung von `memset_s()` oder `explicit_bzero()`
- Loggen der Vernichtung im Audit-Log
- Aufbewahrung von Metadaten (wann, wer, warum) für 7 Jahre

```cpp
// Korrekt:
void secureZero(std::vector<uint8_t>& key) {
    #ifdef _WIN32
        SecureZeroMemory(key.data(), key.size());
    #else
        explicit_bzero(key.data(), key.size());
    #endif
}

// NICHT ausreichend:
key.clear();  // ❌ Compiler könnte optimieren
```

---

## 4. Implementierungsrichtlinien

### 4.1 Initialization Vector (IV) / Nonce

**Anforderungen:**
- **MUSS** für jede Verschlüsselung einzigartig sein
- **MUSS** kryptographisch zufällig generiert werden (CSPRNG)
- **DARF NICHT** wiederverwendet werden (für denselben Schlüssel)
- Länge: 96 bit (12 bytes) für AES-GCM

**Implementierung:**
```cpp
// Korrekt:
std::vector<uint8_t> iv(12);
RAND_bytes(iv.data(), iv.size());

// NICHT zulässig:
std::vector<uint8_t> iv(12, 0);  // ❌ Null-IV
iv = last_iv;  // ❌ IV-Wiederverwendung
iv = counter++;  // ❌ Vorhersagbar
```

### 4.2 Authentication Tags

**Anforderungen:**
- **MUSS** bei AEAD-Modi (GCM, CCM, ChaCha20-Poly1305) verwendet werden
- Tag-Länge: Minimum 96 bit, empfohlen 128 bit
- **MUSS** vor Entschlüsselung verifiziert werden
- Bei Verifikationsfehler: Sofort abbrechen, keine Information preisgeben

**Implementierung:**
```cpp
// Entschlüsselung mit Tag-Verifikation
int ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
if (ret <= 0) {
    // Tag-Verifikation fehlgeschlagen
    throw DecryptionException("Authentication failed");
    // ❌ NICHT: Details über den Fehler preisgeben
}
```

### 4.3 Padding (falls erforderlich)

Bei Modi die Padding erfordern (CBC):
- **MUSS** PKCS#7 Padding verwendet werden
- Padding Oracle-Angriffe vermeiden: Konstante Zeit für Fehler
- **Besser:** Verwende AEAD-Modi (kein Padding erforderlich)

### 4.4 Fehlerbehandlung

**Anforderungen:**
- Kryptographische Fehler **MÜSSEN** geloggt werden (ohne sensitive Daten)
- Keine Details über Fehler an Angreifer zurückgeben
- Konstante Zeit für Fehlerbehandlung (Timing-Angriffe vermeiden)

```cpp
// Gut:
try {
    decrypt(ciphertext);
} catch (const DecryptionException& e) {
    AuditLogger::log("Decryption failed", {
        {"key_id", key_id},
        {"error_type", "auth_failed"}
        // ❌ NICHT loggen: plaintext, key, iv
    });
    throw;  // Generic error nach außen
}
```

---

## 5. Compliance und Auditing

### 5.1 Audit-Logging

**Zu loggende Ereignisse:**

```yaml
Schlüsseloperationen:
  - Schlüsselerzeugung (key_id, algorithm, creator)
  - Schlüsselabruf (key_id, version, timestamp)
  - Schlüsselrotation (key_id, old_version, new_version)
  - Schlüsseldeaktivierung (key_id, reason)
  - Schlüsselvernichtung (key_id, timestamp, operator)

Verschlüsselungsoperationen:
  - Verschlüsselung (key_id, key_version, success)
  - Entschlüsselung (key_id, key_version, success/failure)
  - Authentifizierungsfehler (key_id, timestamp)

Administrative Operationen:
  - Policy-Änderungen (what, who, when)
  - Konfigurationsänderungen (parameter, old_value, new_value)
  - Notfall-Rotationen (reason, initiated_by)
```

**NICHT loggen:**
- ❌ Plaintext-Daten
- ❌ Kryptographische Schlüssel
- ❌ IVs oder Nonces (außer als Hash)
- ❌ Vollständige Ciphertexte

### 5.2 Monitoring

**Metriken:**
```yaml
Performance:
  - encryption_operations_total{operation, key_id}
  - encryption_duration_seconds{operation, quantile}
  - key_cache_hit_rate{key_id}

Sicherheit:
  - key_rotation_age_days{key_id}
  - deprecated_key_access_total{key_id}
  - encryption_errors_total{error_type}
  - authentication_failures_total{key_id}
```

**Alerts:**
```yaml
Critical:
  - Deprecated key access > 0
  - Authentication failure rate > 1%
  - Key rotation overdue > 30 days

Warning:
  - Cache hit rate < 90%
  - Encryption error rate > 0.1%
  - Key age > 80% of rotation interval
```

### 5.3 Compliance-Nachweise

**Erforderliche Dokumentation:**
- ✅ Kryptographie-Policy (dieses Dokument)
- ✅ Schlüsselmanagement-Verfahren
- ✅ Rotations-Runbooks
- ✅ Incident-Response-Playbook
- ✅ Audit-Logs (7 Jahre Aufbewahrung)

**Regelmäßige Überprüfungen:**
- Jährliche Policy-Review
- Vierteljährliche Compliance-Audits
- Monatliche Schlüsselrotations-Checks

---

## 6. Rollen und Verantwortlichkeiten

### 6.1 Security Officer

**Verantwortlichkeiten:**
- Genehmigung von Policy-Änderungen
- Überwachung der Compliance
- Incident-Response bei Kompromittierung
- Jährliche Policy-Review

### 6.2 Development Team

**Verantwortlichkeiten:**
- Implementierung gemäß Policy
- Code-Reviews für kryptographische Operationen
- Unit-Tests für Verschlüsselung
- Dokumentation von Änderungen

### 6.3 Operations Team

**Verantwortlichkeiten:**
- Schlüsselrotation durchführen
- KMS/Vault-Administration
- Monitoring und Alerting
- Backup und Recovery

### 6.4 Compliance Team

**Verantwortlichkeiten:**
- Audit-Überprüfungen
- Compliance-Berichte
- Externe Audits koordinieren

---

## 7. Ausnahmen und Genehmigungsprozess

### 7.1 Ausnahmeregelung

Ausnahmen von dieser Policy sind nur in begründeten Fällen zulässig und erfordern:

1. **Schriftliche Begründung** mit technischer Rechtfertigung
2. **Risk Assessment** durch Security Team
3. **Formale Genehmigung** durch Security Officer
4. **Befristete Gültigkeit** (max. 12 Monate)
5. **Kompensatorische Kontrollen** definieren
6. **Audit-Trail** der Entscheidung

### 7.2 Legacy-Systeme

Für bestehende Implementierungen vor dieser Policy:
- **Migration Plan** innerhalb 12 Monate
- **Temporäre Ausnahme** mit Risikobewertung
- **Monitoring** der Legacy-Verwendung
- **Deprecation Timeline** festlegen

---

## 8. Policy-Wartung

### 8.1 Änderungsmanagement

**Versionierung:**
- Semantic Versioning (MAJOR.MINOR.PATCH)
- MAJOR: Grundlegende Änderungen (neue Algorithmen)
- MINOR: Ergänzungen (neue Abschnitte)
- PATCH: Korrekturen, Klarstellungen

**Änderungsprozess:**
1. Vorschlag durch Security Team oder Stakeholder
2. Impact Analysis
3. Review durch Development, Ops, Compliance
4. Genehmigung durch Security Officer
5. Kommunikation an alle Teams
6. Implementation Grace Period (30 Tage)

### 8.2 Review-Zyklus

- **Regulär:** Jährlich (15. Dezember)
- **Ad-hoc:** Bei Sicherheitsvorfällen oder neuen Bedrohungen
- **Trigger:** Neue BSI/NIST-Empfehlungen

### 8.3 Schulung

- Alle Entwickler **MÜSSEN** diese Policy vor Implementierung kryptographischer Funktionen lesen
- Jährliche Security-Awareness-Schulung
- Spezielle Schulung bei Policy-Updates

---

## 9. Anhänge

### 9.1 Referenz-Implementierung

Siehe:
- `docs/security/security_column_encryption.md` (Design)
- `include/security/encryption.h` (API)
- `src/security/field_encryption.cpp` (Implementierung)
- `tests/test_encryption.cpp` (Tests)

### 9.2 Compliance-Mapping

| BSI C5 | ISO 27001 | NIST | Diese Policy |
|--------|-----------|------|--------------|
| CRY-01 | A.10.1.1 | SC-12 | Abschnitt 2 |
| CRY-02 | A.10.1.2 | SC-12, SC-13 | Abschnitt 3 |
| CRY-03 | A.10.1.1 | SC-28 | Abschnitt 2 |
| CRY-04 | A.13.1.1 | SC-8 | Abschnitt 2.2 |
| CRY-05 | A.10.1.2 | SC-12 | Abschnitt 3.4 |
| CRY-06 | A.10.1.1 | SC-13 | Abschnitt 4.2 |

### 9.3 Verbotene Praktiken

**Häufige Fehler (NICHT tun):**

```cpp
// ❌ Schwache Schlüssel
std::vector<uint8_t> key = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};

// ❌ IV-Wiederverwendung
static std::vector<uint8_t> iv(12, 0);  // Konstante IV

// ❌ Keine Authentifizierung
// CBC-Modus ohne HMAC

// ❌ Hardcoded Keys
const char* KEY = "super_secret_key_1234567890";

// ❌ Unsichere Zufallszahlen
std::srand(time(NULL));
int iv = std::rand();

// ❌ Fehler ignorieren
EVP_EncryptFinal_ex(ctx, out, &len);  // return value ignoriert
```

---

## 10. Genehmigung und Inkrafttreten

**Erstellt von:** ThemisDB Security Team  
**Erstellt am:** 15. Dezember 2025

**Genehmigt von:**
- [ ] Chief Security Officer (CSO)
- [ ] Chief Technology Officer (CTO)
- [ ] Compliance Officer

**Inkrafttreten:** 15. Dezember 2025  
**Nächste Überprüfung:** 15. Juni 2026 (6 Monate)

**Änderungshistorie:**

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | 2025-12-15 | Security Team | Initiale Version |

---

**Dokumentstatus:** ✅ Aktiv  
**Vertraulichkeit:** Intern  
**Klassifizierung:** Organisatorisch
