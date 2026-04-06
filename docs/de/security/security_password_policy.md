# ThemisDB - Passwortrichtlinie

**Version:** v1.3.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern  
**BSI C5 Referenz:** IDM-06  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [1. Zweck](#1-zweck)
- [2. Geltungsbereich](#2-geltungsbereich)
- [3. Passwortanforderungen](#3-passwortanforderungen)
- [4. Passwortlebenszyklus](#4-passwortlebenszyklus)
- [5. Kontosperrung](#5-kontosperrung)
- [6. Sichere Speicherung](#6-sichere-speicherung)
- [7. API-Keys und Tokens](#7-api-keys-und-tokens)
- [8. Verschlüsselungspassphrasen](#8-verschlüsselungspassphrasen)

---

## 1. Zweck

Diese Passwortrichtlinie definiert die Anforderungen an Passwörter und geheime Authentifizierungsinformationen für ThemisDB-Systeme und -Anwendungen.

---

## 2. Geltungsbereich

Diese Richtlinie gilt für:
- API-Keys und Tokens
- Datenbank-Benutzerkonten
- Administrator-Zugänge
- Service-Accounts
- Verschlüsselungspassphrasen

---

## 3. Passwortanforderungen

### 3.1 Komplexitätsanforderungen

| Anforderung | Wert | Beschreibung |
|-------------|------|--------------|
| **Mindestlänge** | 12 Zeichen | Für Standard-Benutzer |
| **Mindestlänge (Admin)** | 16 Zeichen | Für privilegierte Konten |
| **Großbuchstaben** | ≥ 1 | A-Z |
| **Kleinbuchstaben** | ≥ 1 | a-z |
| **Zahlen** | ≥ 1 | 0-9 |
| **Sonderzeichen** | ≥ 1 | !@#$%^&*()_+-=[]{}|;':\",./<>? |
| **Maximallänge** | 128 Zeichen | Technische Grenze |

### 3.2 Verbotene Passwörter

Folgende Passwortmuster sind unzulässig:
- Wörterbuchwörter (in beliebiger Sprache)
- Benutzername oder Teile davon
- E-Mail-Adresse oder Teile davon
- Sequenzen (12345, abcdef)
- Wiederholungen (aaaa, 1111)
- Tastaturmuster (qwerty, asdfgh)
- Bekannte kompromittierte Passwörter (HaveIBeenPwned)

### 3.3 Beispiele

**Akzeptabel:**
```
Th3m1s-DB$ecure2025!
K0mpl3x#Passw0rt@Safe
9s$jK2#mNp4&vQ8x
```

**Nicht akzeptabel:**
```
password123
ThemisDB
admin2025
12345678901234
qwerty!@#$%^
```

---

## 4. Passwortlebenszyklus

### 4.1 Gültigkeitsdauer

| Kontotyp | Maximale Gültigkeitsdauer | Mindestgültigkeitsdauer |
|----------|--------------------------|------------------------|
| Standard-Benutzer | 90 Tage | 1 Tag |
| Administrator | 60 Tage | 1 Tag |
| Service-Account | 365 Tage | - |
| API-Key | 365 Tage | - |

### 4.2 Passworthistorie

| Parameter | Wert |
|-----------|------|
| **Gespeicherte Passwörter** | 12 |
| **Wiederverwendungs-Sperre** | 12 vorherige Passwörter |

### 4.3 Passwort-Reset

**Selbst-Reset:**
1. Identitätsverifizierung erforderlich
2. Neues Passwort muss Komplexitätsanforderungen erfüllen
3. Audit-Log-Eintrag wird erstellt

**Administrator-Reset:**
1. Muss dokumentiert werden
2. Benutzer muss Passwort bei nächster Anmeldung ändern
3. Audit-Log-Eintrag wird erstellt

---

## 5. Kontosperrung

### 5.1 Fehlgeschlagene Anmeldeversuche

| Parameter | Wert |
|-----------|------|
| **Maximale Fehlversuche** | 5 |
| **Sperrzeit** | 30 Minuten |
| **Zurücksetzen nach** | Erfolgreicher Anmeldung |

### 5.2 Benachrichtigung

Bei Kontosperrung:
- Audit-Log-Eintrag (Severity: HIGH)
- Optional: E-Mail-Benachrichtigung an Administrator
- Optional: SIEM-Alert

---

## 6. Sichere Speicherung

### 6.1 Passwort-Hashing

**Algorithmus:** Argon2id (empfohlen) oder bcrypt

**Parameter für Argon2id:**
```
memory: 64 MiB
iterations: 3
parallelism: 4
salt_length: 16 bytes
hash_length: 32 bytes
```

**Parameter für bcrypt:**
```
cost_factor: 12
salt_length: 16 bytes
```

### 6.2 Verbotene Praktiken

Folgende Praktiken sind untersagt:
- ❌ Passwörter im Klartext speichern
- ❌ Schwache Hashing-Algorithmen (MD5, SHA-1)
- ❌ Passwörter in Logs ausgeben
- ❌ Passwörter in Source Code hardcoden
- ❌ Passwörter unverschlüsselt übertragen

---

## 7. API-Keys und Tokens

### 7.1 Generierung

| Parameter | Wert |
|-----------|------|
| **Mindestlänge** | 32 Zeichen |
| **Entropie** | ≥ 256 bit |
| **Quelle** | Kryptographisch sicherer Zufallsgenerator |

### 7.2 Format

```
themis_[type]_[random_string]

Beispiele:
themis_live_a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
themis_test_x9y8z7w6v5u4t3s2r1q0p9o8n7m6l5k4
```

### 7.3 Rotation

| Token-Typ | Empfohlene Rotation |
|-----------|-------------------|
| Production API-Key | 90 Tage |
| Test API-Key | 365 Tage |
| Service Token | Bei Kompromittierung |

---

## 8. Verschlüsselungspassphrasen

### 8.1 Anforderungen für Key Encryption Keys (KEK)

| Anforderung | Wert |
|-------------|------|
| **Mindestlänge** | 32 Zeichen |
| **Entropie** | ≥ 256 bit |
| **Speicherung** | HSM oder Vault |

### 8.2 HSM-PINs

| Anforderung | Wert |
|-------------|------|
| **Mindestlänge** | 8 Zeichen |
| **Format** | Numerisch oder alphanumerisch |
| **Speicherung** | Niemals im Code |

---

## 9. Übertragung von Passwörtern

### 9.1 Erlaubte Methoden

- ✅ TLS 1.3 verschlüsselte Verbindungen
- ✅ mTLS für Service-to-Service
- ✅ Verschlüsselter Passwort-Manager (1Password, Bitwarden)
- ✅ HSM für Schlüsselmaterial

### 9.2 Verbotene Methoden

- ❌ E-Mail (unverschlüsselt)
- ❌ Chat/Messaging (unverschlüsselt)
- ❌ HTTP (ohne TLS)
- ❌ Textdateien
- ❌ Geteilte Dokumente

---

## 10. Verantwortlichkeiten

### 10.1 Benutzer

- Passwörter vertraulich behandeln
- Passwörter nicht teilen
- Verdächtige Aktivitäten melden
- Passwortmanager verwenden

### 10.2 Administratoren

- Richtlinie technisch durchsetzen
- Kompromittierte Passwörter zurücksetzen
- Audit-Logs überwachen
- Schulungen durchführen

### 10.3 Entwickler

- Sichere Passwort-APIs implementieren
- Keine Passwörter im Code
- Gitleaks für Secret-Scanning
- Sichere Defaults verwenden

---

## 11. ThemisDB-Implementierung

### 11.1 Konfiguration

```yaml
# config/security.yaml
authentication:
  password:
    min_length: 12
    min_length_admin: 16
    require_uppercase: true
    require_lowercase: true
    require_digit: true
    require_special: true
    max_age_days: 90
    max_age_admin_days: 60
    history_count: 12
    
  lockout:
    max_attempts: 5
    lockout_duration_minutes: 30
    
  hashing:
    algorithm: argon2id
    memory_kb: 65536
    iterations: 3
    parallelism: 4
```

### 11.2 API-Validierung

```cpp
// Passwort-Validierung wird bei User-Erstellung/Update aufgerufen
ValidationResult validatePassword(const std::string& password, const UserContext& ctx);

// Rückgabe:
// - VALID: Passwort erfüllt alle Anforderungen
// - TOO_SHORT: Mindestlänge nicht erreicht
// - MISSING_UPPERCASE: Keine Großbuchstaben
// - MISSING_LOWERCASE: Keine Kleinbuchstaben
// - MISSING_DIGIT: Keine Zahlen
// - MISSING_SPECIAL: Keine Sonderzeichen
// - IN_HISTORY: Passwort wurde kürzlich verwendet
// - COMPROMISED: Passwort in Breach-Datenbank gefunden
```

---

## 12. Ausnahmen

Ausnahmen von dieser Richtlinie erfordern:
1. Schriftliche Begründung
2. Risikobewertung
3. Genehmigung durch Security Lead
4. Befristung und Kompensationsmaßnahmen

---

## 13. Compliance-Mapping

| Standard | Anforderung | Status |
|----------|-------------|--------|
| BSI C5 | IDM-06 | ✅ |
| ISO 27001 | A.5.17 | ✅ |
| NIST CSF | PR.AA-1 | ✅ |
| PCI DSS | 8.3 | ✅ |
| HIPAA | 164.312(d) | ✅ |

---

## 14. Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | Dezember 2025 | ThemisDB Team | Erstversion |

---

**Letzte Aktualisierung:** Dezember 2025  
**Nächstes Review:** Dezember 2026
