# Schlüssel-Lifecycle-Management für ThemisDB

**Version:** 1.0  
**Datum:** 15. Dezember 2025  
**Status:** Gültig  
**Referenz:** CRYPTOGRAPHY_POLICY.md Abschnitt 3  
**BSI C5:** CRY-02 (Schlüsselmanagement)

---

## 1. Übersicht

Dieses Dokument beschreibt den vollständigen Lebenszyklus kryptographischer Schlüssel in ThemisDB gemäß BSI C5 Anforderungen und Best Practices.

### 1.1 Lifecycle-Phasen

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Generation │ -> │   Storage   │ -> │Distribution │
└─────────────┘    └─────────────┘    └─────────────┘
                                              │
       ┌──────────────────────────────────────┘
       │
       v
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│    Usage    │ -> │  Rotation   │ -> │ Archivierung│
└─────────────┘    └─────────────┘    └─────────────┘
       │                                      │
       v                                      v
┌─────────────┐                        ┌─────────────┐
│Deactivation │                        │ Destruction │
└─────────────┘                        └─────────────┘
```

### 1.2 Schlüsseltypen

| Typ | Bezeichnung | Lebensdauer | Rotationsfrequenz |
|-----|-------------|-------------|-------------------|
| **KEK** | Key Encryption Key | 1 Jahr | Jährlich |
| **DEK** | Data Encryption Key | 3 Monate | Vierteljährlich |
| **TLS** | TLS-Zertifikate | 90 Tage | Alle 90 Tage |
| **Signing** | Signaturschlüssel | 2 Jahre | Alle 2 Jahre |

---

## 2. Phase 1: Schlüsselerzeugung

### 2.1 Anforderungen

**Algorithmus:**
```yaml
KEK:  AES-256-GCM (256 bit)
DEK:  AES-256-GCM (256 bit)
RSA:  2048 bit (minimum), 4096 bit (empfohlen)
ECDSA: P-256, P-384
```

**Entropiequelle:**
- OpenSSL `RAND_bytes()` (empfohlen)
- `/dev/urandom` (Linux)
- BCryptGenRandom (Windows)
- **NICHT:** std::rand(), time-based seeds

### 2.2 Generierungsprozess

**Automatische Generierung:**
```bash
# KEK-Generierung (manuell, 1x pro Jahr)
$ vault write transit/keys/kek-2025 \
    exportable=false \
    allow_plaintext_backup=false \
    type=aes256-gcm96

# DEK-Generierung (programmatisch)
POST /keys/rotate
{
  "key_id": "user_pii",
  "algorithm": "AES-256-GCM"
}
```

**Code-Beispiel:**
```cpp
// src/security/mock_key_provider.cpp
std::vector<uint8_t> key(32);  // 256 bit
if (RAND_bytes(key.data(), key.size()) != 1) {
    unsigned long err = ERR_get_error();
    throw KeyGenerationException(
        "Failed to generate key: " + 
        std::string(ERR_error_string(err, nullptr))
    );
}

// Audit-Log
AuditLogger::log(AuditEvent::KEY_GENERATED, {
    {"key_id", key_id},
    {"algorithm", "AES-256-GCM"},
    {"key_length", 256},
    {"generator", "OpenSSL RAND_bytes"},
    {"timestamp", getCurrentTimestamp()},
    {"operator", current_user}
});
```

### 2.3 Metadaten

```cpp
struct KeyMetadata {
    std::string key_id;           // "user_pii"
    uint32_t version;             // 1, 2, 3...
    std::string algorithm;        // "AES-256-GCM"
    int64_t created_at_ms;       // Unix timestamp
    int64_t expires_at_ms;       // 0 = never
    KeyStatus status;            // ACTIVE, ROTATING, DEPRECATED, DESTROYED
    std::string created_by;      // "system" or user_id
    std::vector<std::string> authorized_users;
};
```

---

## 3. Phase 2: Speicherung

### 3.1 KEK-Speicherung (Master Keys)

**Externe Key Management Systeme:**

**Option 1: HashiCorp Vault (Empfohlen)**
```yaml
Storage:
  Path: /v1/transit/keys/{key_id}
  Encryption: Vault Master Key (Shamir Secret Sharing)
  Access Control: AppRole, K8s Service Account
  Audit: Alle Zugriffe geloggt

Features:
  - Encryption as a Service (kein Export des Schlüssels)
  - Automatic Rotation
  - Version History
  - Audit Logging
```

**Option 2: Hardware Security Module (HSM)**
```yaml
Interface: PKCS#11
Device: Utimaco, Thales, AWS CloudHSM
Storage: Tamper-proof hardware
Access: PIN/Password + Certificate
FIPS: 140-2 Level 3 oder höher
```

**Option 3: Cloud KMS**
```yaml
AWS KMS:
  - Customer Master Key (CMK)
  - Automatic rotation available
  - CloudTrail audit logs

Azure Key Vault:
  - Premium tier for HSM-backed keys
  - RBAC integration
  - Azure Monitor logs

Google Cloud KMS:
  - Cloud HSM option
  - IAM permissions
  - Cloud Audit Logs
```

### 3.2 DEK-Speicherung (Data Keys)

**At-Rest (verschlüsselt mit KEK):**
```json
{
  "key_id": "user_pii",
  "version": 2,
  "encrypted_key": "base64(KEK_encrypt(DEK))",
  "kek_id": "kek-2025",
  "kek_version": 1,
  "created_at": "2025-12-15T10:00:00Z"
}
```

**In-Memory Cache:**
```cpp
class KeyCache {
private:
    struct CacheEntry {
        std::vector<uint8_t> key;
        int64_t expires_at_ms;
        uint64_t access_count;
        std::chrono::steady_clock::time_point last_access;
    };
    
    std::map<std::string, CacheEntry> cache_;
    size_t max_size_ = 1000;
    int64_t ttl_ms_ = 3600000;  // 1 hour
    
public:
    std::optional<std::vector<uint8_t>> get(const std::string& key_id);
    void put(const std::string& key_id, const std::vector<uint8_t>& key);
    void evictLRU();  // Least Recently Used
    void clear();
};
```

**Sicherheitsanforderungen:**
- TTL: Maximum 1 Stunde
- Max Size: 1000 Schlüssel
- LRU Eviction bei Überlauf
- Secure Zero beim Entfernen: `explicit_bzero()`

---

## 4. Phase 3: Verteilung

### 4.1 Sichere Kanäle

**Application <-> KMS:**
```yaml
Protokoll: TLS 1.3
Client Auth: mTLS (mutual TLS)
Zertifikate: X.509 mit RSA 4096 bit oder ECDSA P-384
Cipher Suite: TLS_AES_256_GCM_SHA384
```

**Authentifizierung:**
```yaml
Vault:
  - AppRole (app_id + secret_id)
  - Kubernetes Service Account Token
  - JWT (für Cloud-native)

HSM:
  - PKCS#11 PIN + User Certificate
  - Two-Factor Authentication

Cloud KMS:
  - IAM Role (AWS)
  - Managed Identity (Azure)
  - Service Account (GCP)
```

### 4.2 Zugriffskontrolle

**RBAC-Modell:**
```yaml
Roles:
  crypto_admin:
    - keys:create
    - keys:rotate
    - keys:delete
    - keys:read_metadata
    
  crypto_operator:
    - keys:read
    - keys:rotate
    
  application:
    - keys:read (nur eigene keys)
    - keys:encrypt
    - keys:decrypt

Policies:
  - Principle of Least Privilege
  - Separation of Duties (kein Admin = Operator)
  - Dual-Control für kritische Operationen
```

---

## 5. Phase 4: Verwendung

### 5.1 Erlaubte Operationen

```cpp
// Verschlüsselung
auto blob = field_encryption->encrypt(plaintext, "user_pii");
// -> Ruft automatisch getKey("user_pii", latest_version) auf

// Entschlüsselung
auto plaintext = field_encryption->decrypt(blob);
// -> Verwendet key_id und version aus blob
```

### 5.2 Audit-Logging

**Jede Verwendung MUSS geloggt werden:**
```json
{
  "event": "KEY_USAGE",
  "operation": "encrypt",
  "key_id": "user_pii",
  "key_version": 2,
  "timestamp": "2025-12-15T12:00:00Z",
  "user_id": "system",
  "success": true,
  "duration_ms": 0.5
}
```

### 5.3 Rate Limiting

```yaml
Limits:
  - Max 10,000 ops/sec pro key_id
  - Max 1,000 getKey() calls/sec (Cache schützt)
  - Burst: 20,000 ops/sec (5 Sekunden)

Bei Überschreitung:
  - HTTP 429 (Too Many Requests)
  - Exponential Backoff
  - Alert an Operations Team
```

---

## 6. Phase 5: Rotation

### 6.1 Reguläre Rotation

**Zeitplan:**
```yaml
KEK:
  Frequenz: Jährlich
  Nächste: 2026-12-15
  Window: 4 Wochen (1. Dezember - 31. Dezember)

DEK:
  Frequenz: Vierteljährlich
  Schedule: 
    - Q1: 15. März
    - Q2: 15. Juni
    - Q3: 15. September
    - Q4: 15. Dezember
  Window: 2 Wochen
```

**Vier-Phasen-Prozess:**

```
Phase 1: Dual-Write (Woche 1-2)
├─ Neuer Schlüssel v3 erstellt
├─ Schreiboperationen: v3
├─ Leseoperationen: v2, v3
└─ Monitoring: Beide Versionen aktiv

Phase 2: Re-Encryption (Woche 3-4)
├─ Background Job
│  ├─ SELECT id FROM users WHERE email_key_version = 2
│  ├─ Batch Processing (1000 rows)
│  └─ Progress: "45,234 / 100,000 (45.2%)"
├─ Idempotent (Wiederholbar)
└─ Rate Limited (max 100 ops/sec)

Phase 3: Deprecation (Woche 5)
├─ Status v2: DEPRECATED
├─ Leseoperationen: v2 (warning), v3
├─ Monitoring: v2 access -> Alert
└─ Grace Period: 30 Tage

Phase 4: Deletion (Woche 8+)
├─ Prüfung: Keine v2 Zugriffe in letzten 7 Tagen
├─ Backup: Encrypted Backup mit v3
├─ DELETE FROM keys WHERE key_id='user_pii' AND version=2
└─ Audit Log Entry
```

**Code:**
```cpp
// Rotation initiieren
uint32_t new_version = key_provider->rotateKey("user_pii");
// -> Erstellt v3, setzt v2 auf ROTATING

// Re-Encryption Background Job
void reEncryptData(const std::string& key_id, uint32_t old_version) {
    auto rows = db->query(
        "SELECT id, encrypted_field FROM users "
        "WHERE encrypted_field_version = ?", 
        old_version
    );
    
    size_t total = rows.size();
    size_t processed = 0;
    
    for (auto& row : rows) {
        // Decrypt with old key
        auto plaintext = decrypt(row.encrypted_field, key_id, old_version);
        
        // Encrypt with new key
        auto new_blob = encrypt(plaintext, key_id);  // uses latest version
        
        // Update database
        db->update("users", row.id, {{"encrypted_field", new_blob}});
        
        processed++;
        if (processed % 1000 == 0) {
            Logger::info("Re-encryption progress: {}/{} ({:.1f}%)", 
                        processed, total, (processed * 100.0) / total);
        }
    }
}
```

### 6.2 Notfall-Rotation (Emergency)

**Trigger:**
- Kompromittierung (bestätigt oder vermutet)
- Insider-Threat
- HSM-Ausfall
- Compliance-Verstoß

**Prozess:**
```yaml
T+0 (Sofort):
  - Deaktivierung des kompromittierten Schlüssels
  - Incident Response aktivieren
  - Stakeholder informieren
  
T+1h:
  - Neuer Schlüssel generiert
  - Dual-Write aktiviert
  - Monitoring verstärkt
  
T+24h:
  - Re-Encryption mit Priorität (24/7)
  - Progress Reports alle 2 Stunden
  
T+48h:
  - Re-Encryption abgeschlossen
  - Alter Schlüssel gelöscht
  - Root-Cause-Analysis gestartet
  
T+7 Tage:
  - Incident Report finalisiert
  - Lessons Learned dokumentiert
  - Prozessverbesserungen implementiert
```

**Kommunikation:**
```
Interne Stakeholder:
  - CTO, CSO: Sofort (T+0)
  - Development Team: T+1h
  - Operations Team: T+1h
  - Compliance: T+4h

Externe Stakeholder:
  - Kunden (bei Datenleck): T+72h (gesetzliche Frist)
  - Behörden (BSI, Datenschutzbehörde): T+72h
  - Versicherer: T+7 Tage
```

---

## 7. Phase 6: Archivierung

### 7.1 DEPRECATED Status

**Anforderungen:**
- Schlüssel bleibt lesbar für 30 Tage
- NICHT für neue Verschlüsselung verwenden
- Monitoring für unerwartete Zugriffe
- Automatische Alerts bei Verwendung

**Metadaten:**
```json
{
  "key_id": "user_pii",
  "version": 2,
  "status": "DEPRECATED",
  "deprecated_at": "2025-12-15T10:00:00Z",
  "delete_after": "2026-01-15T10:00:00Z",
  "reason": "Regular rotation",
  "deprecated_by": "crypto_operator"
}
```

### 7.2 Backup

**Verschlüsselte Backups:**
```bash
# KEK-Backup (nur für Disaster Recovery)
$ vault operator rekey -init -backup \
    -pgp-keys="keybase:alice,keybase:bob,keybase:carol"

# DEK-Backup (verschlüsselt mit aktuellem KEK)
$ themisdb-backup --include-keys --encrypt-with kek-2025
```

**Aufbewahrungsfristen:**
```yaml
Aktive Schlüssel:
  - Indefinite (solange Status=ACTIVE)
  
Deprecated Schlüssel:
  - 30 Tage nach Deprecation
  - Dann Deletion
  
Backups:
  - 7 Jahre (Compliance-Anforderung)
  - Offline Storage (Cold Storage)
  - Air-Gapped System
  
Audit Logs:
  - 10 Jahre (DSGVO Art. 30)
  - WORM Storage (Write Once Read Many)
```

---

## 8. Phase 7: Vernichtung

### 8.1 Löschprozess

**Voraussetzungen:**
```yaml
Checks:
  - ✅ Status = DEPRECATED
  - ✅ Deprecated seit > 30 Tagen
  - ✅ Keine Zugriffe in letzten 7 Tagen
  - ✅ Backup erstellt
  - ✅ Genehmigung vorhanden
```

**Verfahren:**
```cpp
// 1. Secure Zero im Memory
void destroyKey(std::vector<uint8_t>& key) {
    #ifdef _WIN32
        SecureZeroMemory(key.data(), key.size());
    #else
        explicit_bzero(key.data(), key.size());
    #endif
    key.clear();
    key.shrink_to_fit();
}

// 2. Deletion aus KMS/Vault
vault->deleteKey("user_pii", version=2);

// 3. Deletion aus Cache
key_cache->remove("user_pii:2");

// 4. Update Metadaten
db->update("key_metadata", {
    {"key_id", "user_pii"},
    {"version", 2},
    {"status", "DESTROYED"},
    {"destroyed_at", getCurrentTimestamp()},
    {"destroyed_by", current_operator}
});

// 5. Audit Log
AuditLogger::log(AuditEvent::KEY_DESTROYED, {
    {"key_id", "user_pii"},
    {"version", 2},
    {"timestamp", getCurrentTimestamp()},
    {"operator", current_operator},
    {"approval_ticket", "SEC-2025-12345"}
});
```

### 8.2 Verifikation

**Nach Löschung:**
```bash
# Prüfen: Schlüssel nicht mehr abrufbar
$ vault read transit/keys/user_pii/v2
# Expected: Error: key not found

# Prüfen: Cache leer
$ redis-cli GET "key:user_pii:2"
# Expected: (nil)

# Prüfen: Metadaten aktualisiert
$ psql -c "SELECT status FROM key_metadata 
           WHERE key_id='user_pii' AND version=2"
# Expected: DESTROYED
```

**Audit-Trail:**
- Wer hat gelöscht?
- Wann wurde gelöscht?
- Warum wurde gelöscht?
- Genehmigung vorhanden?
- Backup erstellt?

---

## 9. Monitoring und Alerting

### 9.1 Metriken

```yaml
Prometheus Metrics:

# Schlüssel-Alter
crypto_key_age_days{key_id, version, status}

# Verwendung
crypto_key_usage_total{key_id, version, operation}
crypto_key_errors_total{key_id, version, error_type}

# Cache
crypto_key_cache_hit_rate{key_id}
crypto_key_cache_size

# Rotation
crypto_key_rotation_age_days{key_id}
crypto_key_pending_reencryption_count{key_id}
```

### 9.2 Alerts

```yaml
Critical:
  - name: DeprecatedKeyAccess
    condition: crypto_key_usage_total{status="DEPRECATED"} > 0
    severity: critical
    action: Incident Response

  - name: KeyRotationOverdue
    condition: crypto_key_rotation_age_days > 365
    severity: critical
    action: Force Rotation

Warning:
  - name: KeyExpiringSoon
    condition: crypto_key_rotation_age_days > 335  # 30 Tage vor Ablauf
    severity: warning
    action: Schedule Rotation

  - name: HighErrorRate
    condition: rate(crypto_key_errors_total[5m]) > 0.01
    severity: warning
    action: Investigate

Info:
  - name: CacheLowHitRate
    condition: crypto_key_cache_hit_rate < 0.90
    severity: info
    action: Review Cache Settings
```

---

## 10. Verantwortlichkeiten

### 10.1 RACI-Matrix

| Aktivität | Responsible | Accountable | Consulted | Informed |
|-----------|-------------|-------------|-----------|----------|
| KEK-Generierung | Security Officer | CSO | Ops Team | Dev Team |
| DEK-Generierung | Crypto Operator | Security Officer | - | Audit |
| Reguläre Rotation | Crypto Operator | Security Officer | Dev Team | Management |
| Notfall-Rotation | Security Officer | CSO | CISO | All |
| Schlüssellöschung | Crypto Operator | Security Officer | Compliance | Audit |
| Audit-Review | Compliance Officer | CISO | Security Team | Management |

### 10.2 Eskalation

```
Level 1: Crypto Operator
  ├─ Reguläre Operationen
  └─ Standard-Rotation

Level 2: Security Officer
  ├─ Ausnahmen genehmigen
  ├─ Notfall-Rotation initiieren
  └─ Policies aktualisieren

Level 3: Chief Security Officer (CSO)
  ├─ Kompromittierung
  ├─ Externe Audits
  └─ Policy-Änderungen genehmigen

Level 4: C-Level (CTO, CISO)
  ├─ Datenlecks
  └─ Regulatorische Vorfälle
```

---

## 11. Disaster Recovery

### 11.1 Schlüsselverlust

**Szenario: KEK verloren (Vault-Ausfall)**

```yaml
Schritt 1: Backup wiederherstellen
  - Vault-Snapshot aus Air-Gapped Storage
  - Shamir Secret Sharing (3 of 5 Keyholders)
  
Schritt 2: Vault neu initialisieren
  - Cluster aufbauen
  - Backup einspielen
  - Zugriffstests
  
Schritt 3: Applikation neu konfigurieren
  - Neue Vault-Adresse
  - Neue AppRole Credentials
  - Connection-Tests
  
RTO: 4 Stunden (Recovery Time Objective)
RPO: 0 (Recovery Point Objective - keine Daten verloren)
```

**Szenario: DEK verloren (Cache-Flush)**

```yaml
Auswirkung: Keine Datenverlust
Lösung: Automatischer Re-Fetch aus KMS
Downtime: < 1 Sekunde (erste Request nach Cache-Miss)
```

### 11.2 Kompromittierung

**Playbook:** Siehe `docs/security/security_incident_response.md`

**Zusammenfassung:**
1. **Detect:** Monitoring-Alert oder Incident Report
2. **Contain:** Schlüssel deaktivieren (T+0)
3. **Eradicate:** Neuer Schlüssel, Re-Encryption (T+1h - T+48h)
4. **Recover:** Normalbetrieb wiederherstellen (T+48h)
5. **Lessons Learned:** RCA, Process Improvements (T+7 Tage)

---

## 12. Compliance-Nachweise

### 12.1 Audit-Checkpoints

**Quartalsweise:**
- [ ] Alle Schlüssel-Metadaten geprüft
- [ ] Rotation-Zeitplan eingehalten
- [ ] Keine DEPRECATED-Key-Zugriffe
- [ ] Cache-Konfiguration korrekt
- [ ] Backup vorhanden und getestet

**Jährlich:**
- [ ] Externe Sicherheitsaudit
- [ ] Penetration Testing
- [ ] Policy-Review und Update
- [ ] Disaster Recovery Drill
- [ ] Compliance-Zertifizierung (BSI C5, ISO 27001)

### 12.2 Dokumentations-Anforderungen

```yaml
Pflichtdokumente:
  - ✅ CRYPTOGRAPHY_POLICY.md
  - ✅ KEY_LIFECYCLE_MANAGEMENT.md (dieses Dokument)
  - ✅ security_column_encryption.md (Design)
  - ✅ security_key_management.md (Betrieb)
  - ✅ security_incident_response.md (Notfall)

Aufbewahrung:
  - Policies: Permanent
  - Audit Logs: 10 Jahre
  - Incident Reports: 7 Jahre
  - Backups: 7 Jahre
```

---

## 13. Tooling und Automation

### 13.1 Key Rotation Script

```bash
#!/bin/bash
# rotate-keys.sh - Automatisierte Schlüsselrotation

KEY_ID="user_pii"

echo "Initiating key rotation for $KEY_ID..."

# Phase 1: Rotation starten
NEW_VERSION=$(curl -X POST https://themisdb/keys/rotate \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"key_id\": \"$KEY_ID\"}" | jq -r '.version')

echo "New version: $NEW_VERSION"

# Phase 2: Re-Encryption
curl -X POST https://themisdb/admin/reencrypt \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"key_id\": \"$KEY_ID\", \"old_version\": $(($NEW_VERSION - 1))}"

# Phase 3: Monitoring
while true; do
    PROGRESS=$(curl https://themisdb/admin/reencrypt/status/$KEY_ID | jq -r '.progress')
    echo "Progress: $PROGRESS%"
    [ "$PROGRESS" == "100" ] && break
    sleep 60
done

# Phase 4: Deprecate old key
curl -X POST https://themisdb/keys/deprecate \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"key_id\": \"$KEY_ID\", \"version\": $(($NEW_VERSION - 1))}"

echo "Rotation complete!"
```

### 13.2 Monitoring Dashboard

```yaml
Grafana Dashboard: "Cryptographic Key Lifecycle"

Panels:
  - Key Age (Gauge)
  - Rotation Timeline (Timeline)
  - Cache Hit Rate (Graph)
  - Error Rate (Graph)
  - Deprecated Key Access (Stat - should be 0)
  - Re-Encryption Progress (Gauge)
```

---

## Anhang: Referenzen

**Interne Dokumente:**
- CRYPTOGRAPHY_POLICY.md
- docs/security/security_column_encryption.md
- docs/security/security_key_management.md
- docs/security/security_incident_response.md
- docs/compliance/compliance_full_checklist.md

**Standards:**
- BSI C5:2020 - CRY-02 (Schlüsselmanagement)
- ISO/IEC 27001:2013 - A.10.1.2
- NIST SP 800-57 - Key Management
- NIST SP 800-131A - Transitioning to Cryptographic Algorithms

**BSI-Veröffentlichungen:**
- BSI TR-02102-1: Kryptographische Verfahren
- BSI TR-03116: eCard-API-Framework - Kryptographie
- BSI IT-Grundschutz: CON.1 Kryptokonzept

---

**Dokument Status:** ✅ Gültig  
**Erstellt:** 15. Dezember 2025  
**Nächste Review:** 15. Juni 2026  
**Owner:** Security Team
