# ThemisDB Encryption & Key Management Policy

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Policies

---


**Version:** 1.0  
**Datum:** 2025-11-27  
**Status:** Aktiv  
**Klassifizierung:** Vertraulich  
**Nächste Überprüfung:** 2026-05-27

---

## 1. Zweck und Geltungsbereich

### 1.1 Zweck

Diese Richtlinie definiert die Anforderungen für kryptographische Operationen und das Schlüsselmanagement innerhalb der ThemisDB-Infrastruktur. Sie gewährleistet den Schutz von Daten durch den Einsatz starker, aktueller Kryptographie.

### 1.2 Geltungsbereich

- Verschlüsselung von Daten at Rest
- Verschlüsselung von Daten in Transit
- Digitale Signaturen
- Schlüsselerzeugung, -speicherung, -rotation und -vernichtung
- Hardware Security Modules (HSM)
- Zertifikatsmanagement

### 1.3 Compliance-Referenzen

| Standard | Kontrolle | Beschreibung |
|----------|-----------|--------------|
| ISO 27001 | A.10 | Cryptographic Controls |
| BSI C5 | CRY-01 bis CRY-04 | Kryptographie |
| NIST SP 800-57 | - | Key Management |
| NIST SP 800-131A | - | Crypto Algorithm Transitions |
| eIDAS | Art. 26-28 | Elektronische Signaturen |
| PCI DSS v4.0 | 3.5-3.7, 4.1 | Cryptographic Key Management |
| FIPS 140-2/3 | Level 2+ | Cryptographic Module Security |

---

## 2. Kryptographische Standards

### 2.1 Zugelassene Algorithmen

#### Symmetrische Verschlüsselung

| Algorithmus | Schlüssellänge | Verwendung | Status |
|-------------|----------------|------------|--------|
| AES-256-GCM | 256 bit | Data at Rest, TLS | ✅ Empfohlen |
| AES-256-CBC | 256 bit | Legacy-Kompatibilität | ⚠️ Akzeptabel |
| ChaCha20-Poly1305 | 256 bit | Alternative zu AES | ✅ Empfohlen |
| AES-128 | 128 bit | Nicht verwenden | ❌ Verboten |
| 3DES | 168/112 bit | Legacy | ❌ Verboten |
| DES | 56 bit | - | ❌ Verboten |

#### Asymmetrische Verschlüsselung

| Algorithmus | Schlüssellänge | Verwendung | Status |
|-------------|----------------|------------|--------|
| RSA | 4096 bit | Signaturen, Key Exchange | ✅ Empfohlen |
| RSA | 3072 bit | Signaturen | ⚠️ Akzeptabel |
| RSA | 2048 bit | Legacy (bis 2030) | ⚠️ Auslaufend |
| RSA | < 2048 bit | - | ❌ Verboten |
| ECDSA (P-384) | 384 bit | Signaturen | ✅ Empfohlen |
| ECDSA (P-256) | 256 bit | Signaturen | ✅ Empfohlen |
| Ed25519 | 256 bit | Signaturen | ✅ Empfohlen |
| X25519 | 256 bit | Key Exchange | ✅ Empfohlen |

#### Hash-Funktionen

| Algorithmus | Output | Verwendung | Status |
|-------------|--------|------------|--------|
| SHA-3-256 | 256 bit | Hashing | ✅ Empfohlen |
| SHA-3-384 | 384 bit | Hashing | ✅ Empfohlen |
| SHA-256 | 256 bit | Hashing, HMAC | ✅ Empfohlen |
| SHA-384 | 384 bit | Hashing, HMAC | ✅ Empfohlen |
| SHA-512 | 512 bit | Hashing | ✅ Empfohlen |
| SHA-1 | 160 bit | Legacy-Kompatibilität | ❌ Verboten |
| MD5 | 128 bit | - | ❌ Verboten |

#### Passwort-Hashing

| Algorithmus | Parameter | Verwendung | Status |
|-------------|-----------|------------|--------|
| Argon2id | m=64MB, t=3, p=4 | Passwörter | ✅ Empfohlen |
| bcrypt | cost=12+ | Passwörter | ✅ Empfohlen |
| scrypt | N=2^17, r=8, p=1 | Passwörter | ✅ Akzeptabel |
| PBKDF2-SHA256 | 310000 iter | Legacy | ⚠️ Auslaufend |

### 2.2 TLS-Konfiguration

#### Zugelassene TLS-Versionen

| Version | Status | Anmerkung |
|---------|--------|-----------|
| TLS 1.3 | ✅ Empfohlen | Standard |
| TLS 1.2 | ⚠️ Akzeptabel | Nur mit starken Cipher Suites |
| TLS 1.1 | ❌ Verboten | Veraltet |
| TLS 1.0 | ❌ Verboten | Veraltet |
| SSL 3.0 | ❌ Verboten | Unsicher |

#### TLS 1.3 Cipher Suites

```
TLS_AES_256_GCM_SHA384
TLS_CHACHA20_POLY1305_SHA256
TLS_AES_128_GCM_SHA256
```

#### TLS 1.2 Cipher Suites (falls erforderlich)

```
TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
```

---

## 3. Schlüsselhierarchie

### 3.1 Key Hierarchy Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                       ROOT KEY (RK)                              │
│                    [HSM-geschützt, offline]                      │
│                    Lebensdauer: 5 Jahre                          │
└─────────────────────────┬───────────────────────────────────────┘
                          │
          ┌───────────────┴───────────────┐
          ▼                               ▼
┌─────────────────────┐         ┌─────────────────────┐
│   KEY ENCRYPTION    │         │   KEY ENCRYPTION    │
│     KEY (KEK-1)     │         │     KEY (KEK-2)     │
│   [HSM/Vault]       │         │   [HSM/Vault]       │
│   Lebensdauer: 1 Jahr│        │   Lebensdauer: 1 Jahr│
└──────────┬──────────┘         └──────────┬──────────┘
           │                               │
     ┌─────┴─────┐                   ┌─────┴─────┐
     ▼           ▼                   ▼           ▼
┌─────────┐ ┌─────────┐         ┌─────────┐ ┌─────────┐
│  DEK-1  │ │  DEK-2  │         │  DEK-3  │ │  DEK-4  │
│ (Data)  │ │ (Data)  │         │ (Data)  │ │ (Data)  │
│ 90 Tage │ │ 90 Tage │         │ 90 Tage │ │ 90 Tage │
└─────────┘ └─────────┘         └─────────┘ └─────────┘
```

### 3.2 Schlüsseltypen

| Schlüsseltyp | Beschreibung | Speicherort | Lebensdauer |
|--------------|--------------|-------------|-------------|
| Root Key (RK) | Master-Schlüssel für alle anderen Keys | HSM (offline) | 5 Jahre |
| Key Encryption Key (KEK) | Verschlüsselt DEKs | HSM/Vault | 1 Jahr |
| Data Encryption Key (DEK) | Verschlüsselt Nutzdaten | Encrypted in DB | 90 Tage |
| Signing Key | Digitale Signaturen | HSM/Vault | 1 Jahr |
| TLS Certificate | Server-/Client-Authentifizierung | Vault | 1 Jahr |
| Token Signing Key | JWT/Session-Tokens | Vault | 90 Tage |

---

## 4. Schlüsselmanagement-Lifecycle

### 4.1 Schlüsselerzeugung

#### Anforderungen

| Anforderung | Umsetzung |
|-------------|-----------|
| Entropiequelle | Hardware RNG (RDRAND, RDSEED) |
| Mindestentropie | 256 bit |
| Erzeugungsort | HSM oder Vault |
| Generierungsnachweis | Audit-Log mit Zeitstempel |

#### Prozess

```
┌────────────────┐     ┌────────────────┐     ┌────────────────┐
│  1. Anfrage    │────▶│  2. Prüfung    │────▶│  3. Erzeugung  │
│  Key Request   │     │  Authorization │     │  in HSM/Vault  │
└────────────────┘     └────────────────┘     └───────┬────────┘
                                                      │
┌────────────────┐     ┌────────────────┐     ┌───────▼────────┐
│  6. Audit-Log  │◀────│  5. Speicherung│◀────│  4. Backup     │
│  Entry         │     │  in Vault      │     │  verschlüsselt │
└────────────────┘     └────────────────┘     └────────────────┘
```

### 4.2 Schlüsselspeicherung

#### Speicherorte nach Schlüsseltyp

| Schlüsseltyp | Primär | Backup | Zugriff |
|--------------|--------|--------|---------|
| Root Key | HSM (offline) | HSM (Tresor) | M-of-N Freigabe |
| KEK | HSM/Vault | Vault (anderer DC) | Admin mit MFA |
| DEK | In-Memory / Vault | Automatisch | API |
| TLS Certs | Vault | Vault Replikation | Automatisch |

#### Vault-Konfiguration

```yaml
vault:
  seal_type: "awskms"  # oder "gcpckms", "azurekeyvault", "hsm"
  
  secrets:
    engine: "transit"
    convergent_encryption: false
    
  auto_unseal:
    enabled: true
    recovery_shares: 5
    recovery_threshold: 3
    
  audit:
    enabled: true
    path: "file"
    options:
      file_path: "/var/log/vault/audit.log"
```

### 4.3 Schlüsselrotation

#### Rotationsintervalle

| Schlüsseltyp | Standard-Intervall | Maximum | Trigger |
|--------------|-------------------|---------|---------|
| Root Key | 5 Jahre | 7 Jahre | Manuell |
| KEK | 1 Jahr | 2 Jahre | Automatisch |
| DEK | 90 Tage | 180 Tage | Automatisch |
| Signing Key | 1 Jahr | 2 Jahre | Automatisch |
| TLS Certs | 1 Jahr | 1 Jahr | Automatisch (ACME) |
| Token Signing | 90 Tage | 180 Tage | Automatisch |

#### Rotationsprozess

```
┌─────────────────────────────────────────────────────────────────┐
│                    SCHLÜSSELROTATION                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Neuen Schlüssel erzeugen (Key_{n+1})                        │
│                     │                                            │
│                     ▼                                            │
│  2. Parallel-Betrieb: Key_n (Entschlüsselung)                   │
│                       Key_{n+1} (Verschlüsselung)               │
│                     │                                            │
│                     ▼                                            │
│  3. Re-Encryption aller Daten mit Key_{n+1}                     │
│                     │                                            │
│                     ▼                                            │
│  4. Key_n als "retired" markieren                               │
│                     │                                            │
│                     ▼                                            │
│  5. Grace Period (für Backups)                                  │
│                     │                                            │
│                     ▼                                            │
│  6. Key_n sicher vernichten                                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### Automatische Rotation

```cpp
// C++ Key Rotation
class KeyRotationService {
public:
    void scheduleRotation() {
        scheduler.schedule([this]() {
            if (shouldRotate(current_key)) {
                auto new_key = generateKey();
                vault.store(new_key);
                
                // Re-encrypt in background
                reencryptionService.start(current_key, new_key);
                
                // Mark old key
                current_key.markRetired();
                
                audit.log("KEY_ROTATION", {
                    {"old_key_id", current_key.id()},
                    {"new_key_id", new_key.id()}
                });
            }
        }, rotation_interval);
    }
    
private:
    bool shouldRotate(const Key& key) {
        return key.age() > max_key_age 
            || key.usageCount() > max_usage_count
            || key.isCompromised();
    }
};
```

### 4.4 Schlüsselvernichtung

#### Vernichtungsverfahren

| Schlüsseltyp | Methode | Nachweis |
|--------------|---------|----------|
| HSM Keys | Crypto-Erase (FIPS) | HSM-Audit-Log |
| Vault Keys | Vault Destroy | Vault-Audit-Log |
| In-Memory | Secure Memset + Deallocate | Application-Log |
| Backup Keys | Crypto-Shredding | Zertifikat |

#### Sichere Löschung

```cpp
// Secure Key Destruction
void secureDestroy(Key& key) {
    // 1. Overwrite with random data
    std::random_device rd;
    std::generate(key.data.begin(), key.data.end(), 
                  [&rd]() { return rd(); });
    
    // 2. Overwrite with zeros
    std::fill(key.data.begin(), key.data.end(), 0);
    
    // 3. Memory barrier
    std::atomic_thread_fence(std::memory_order_seq_cst);
    
    // 4. Deallocate
    sodium_memzero(key.data.data(), key.data.size());
    
    // 5. Audit
    audit.log("KEY_DESTROYED", {{"key_id", key.id()}});
}
```

---

## 5. Hardware Security Module (HSM)

### 5.1 HSM-Anforderungen

| Anforderung | Minimum | Empfohlen |
|-------------|---------|-----------|
| FIPS 140-2 Level | Level 2 | Level 3 |
| Common Criteria | EAL 4+ | EAL 5+ |
| Tamper Response | Evident | Active |
| Key Storage Capacity | 100 Keys | 1000+ Keys |
| Crypto Performance | 1000 ops/s | 10000+ ops/s |

### 5.2 Unterstützte HSM-Lösungen

| Hersteller | Modell | Zertifizierung |
|------------|--------|----------------|
| Thales | Luna Network HSM | FIPS 140-2 L3, CC EAL4+ |
| AWS | CloudHSM | FIPS 140-2 L3 |
| Azure | Dedicated HSM | FIPS 140-2 L3 |
| Google | Cloud HSM | FIPS 140-2 L3 |
| Utimaco | SecurityServer | FIPS 140-2 L3, CC EAL4+ |

### 5.3 HSM-Konfiguration

```yaml
hsm:
  provider: "thales"  # oder "aws", "azure", "gcp"
  
  thales:
    slot: 0
    pin_type: "crypto_officer"
    network:
      primary: "hsm1.internal:1792"
      secondary: "hsm2.internal:1792"
    
  high_availability:
    enabled: true
    mode: "active_passive"
    failover_timeout: 30s
    
  operations:
    key_generation: true
    signing: true
    encryption: true
    key_wrapping: true
```

---

## 6. Envelope Encryption

### 6.1 Envelope Encryption Pattern

```
┌─────────────────────────────────────────────────────────────────┐
│                    ENVELOPE ENCRYPTION                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │
│  │  Plaintext   │───▶│  Encrypt     │───▶│  Ciphertext  │       │
│  │  Data        │    │  with DEK    │    │  Data        │       │
│  └──────────────┘    └──────┬───────┘    └──────────────┘       │
│                             │                                    │
│                      DEK (generated)                            │
│                             │                                    │
│                      ┌──────▼───────┐    ┌──────────────┐       │
│                      │  Encrypt     │───▶│  Encrypted   │       │
│                      │  with KEK    │    │  DEK         │       │
│                      └──────────────┘    └──────────────┘       │
│                                                                  │
│  Stored: { encrypted_data, encrypted_dek, kek_id, iv, tag }    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Implementation

```cpp
// Envelope Encryption Implementation
struct EncryptedEnvelope {
    std::vector<uint8_t> encrypted_data;
    std::vector<uint8_t> encrypted_dek;
    std::string kek_id;
    std::vector<uint8_t> iv;
    std::vector<uint8_t> auth_tag;
};

class EnvelopeEncryption {
public:
    EncryptedEnvelope encrypt(const std::vector<uint8_t>& plaintext) {
        // 1. Generate random DEK
        auto dek = crypto::generateRandomKey(256);
        
        // 2. Generate random IV
        auto iv = crypto::generateRandomBytes(12);
        
        // 3. Encrypt data with DEK (AES-256-GCM)
        auto [ciphertext, tag] = crypto::aes256gcm_encrypt(
            plaintext, dek, iv);
        
        // 4. Encrypt DEK with KEK (from HSM/Vault)
        auto encrypted_dek = vault.encrypt(current_kek_id, dek);
        
        // 5. Secure destroy plaintext DEK
        crypto::secureZero(dek);
        
        return {ciphertext, encrypted_dek, current_kek_id, iv, tag};
    }
    
    std::vector<uint8_t> decrypt(const EncryptedEnvelope& envelope) {
        // 1. Decrypt DEK with KEK
        auto dek = vault.decrypt(envelope.kek_id, envelope.encrypted_dek);
        
        // 2. Decrypt data with DEK
        auto plaintext = crypto::aes256gcm_decrypt(
            envelope.encrypted_data, dek, envelope.iv, envelope.auth_tag);
        
        // 3. Secure destroy DEK
        crypto::secureZero(dek);
        
        return plaintext;
    }
};
```

---

## 7. Digitale Signaturen

### 7.1 Signatur-Anforderungen

| Verwendung | Algorithmus | Schlüssellänge |
|------------|-------------|----------------|
| Code Signing | RSA-SHA384 | 4096 bit |
| Document Signing | ECDSA P-384 | 384 bit |
| API Requests | Ed25519 | 256 bit |
| Audit Logs | Ed25519 | 256 bit |
| JWT Tokens | RS256/ES256 | 4096/256 bit |

### 7.2 Signatur-Schema

```
┌─────────────────────────────────────────────────────────────────┐
│                    ENCRYPT-THEN-SIGN                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Plaintext ───▶ Encrypt with DEK ───▶ Ciphertext            │
│                                                │                 │
│  2. Ciphertext ─▶ Hash (SHA-256) ─▶ Digest    │                 │
│                                         │      │                 │
│  3. Digest ────▶ Sign with Private Key ▶ Signature              │
│                                                │                 │
│  4. Output: { ciphertext, signature, signer_id, timestamp }     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 7.3 Audit-Log-Signierung

```cpp
// Audit Log Signing
class SignedAuditLog {
public:
    struct SignedEntry {
        std::string payload;
        std::string signature;
        std::string previous_hash;
        uint64_t sequence;
        std::chrono::system_clock::time_point timestamp;
    };
    
    void append(const AuditEvent& event) {
        auto json = serialize(event);
        
        SignedEntry entry;
        entry.payload = json;
        entry.sequence = next_sequence++;
        entry.timestamp = std::chrono::system_clock::now();
        entry.previous_hash = last_entry_hash;
        
        // Create hash chain
        auto hash = crypto::sha256(
            entry.previous_hash + entry.payload + 
            std::to_string(entry.sequence));
        
        // Sign with Ed25519
        entry.signature = signing_key.sign(hash);
        
        last_entry_hash = hash;
        entries.push_back(entry);
    }
};
```

---

## 8. Zertifikatsmanagement

### 8.1 Zertifikats-Hierarchie

```
┌─────────────────────────────────────────────────────────────────┐
│                    ROOT CA (Offline)                            │
│                    Validity: 20 years                           │
│                    RSA-4096, SHA-384                            │
└─────────────────────────┬───────────────────────────────────────┘
                          │
          ┌───────────────┼───────────────┐
          ▼               ▼               ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   Issuing CA 1  │ │   Issuing CA 2  │ │   Issuing CA 3  │
│   (Server TLS)  │ │   (Client mTLS) │ │   (Code Sign)   │
│   10 years      │ │   10 years      │ │   10 years      │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
   ┌─────┴─────┐       ┌─────┴─────┐       ┌─────┴─────┐
   ▼           ▼       ▼           ▼       ▼           ▼
┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
│Server │ │Server │ │Client │ │Client │ │ Code  │ │ Code  │
│ Cert  │ │ Cert  │ │ Cert  │ │ Cert  │ │ Cert  │ │ Cert  │
│1 year │ │1 year │ │1 year │ │1 year │ │2 years│ │2 years│
└───────┘ └───────┘ └───────┘ └───────┘ └───────┘ └───────┘
```

### 8.2 Zertifikats-Lebenszyklus

| Phase | Beschreibung | Automatisierung |
|-------|--------------|-----------------|
| Request | CSR-Generierung | cert-manager |
| Validation | Domain/Org Validation | ACME (Let's Encrypt) |
| Issuance | Zertifikat ausstellen | PKI/CA |
| Deployment | Verteilung an Services | Vault/K8s Secrets |
| Monitoring | Ablaufüberwachung | Prometheus Alerts |
| Renewal | Erneuerung vor Ablauf | Automatisch (30 Tage) |
| Revocation | Widerruf bei Kompromittierung | CRL/OCSP |

### 8.3 Automatische Erneuerung

```yaml
# cert-manager Configuration
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: themisdb-tls
spec:
  secretName: themisdb-tls-secret
  duration: 2160h  # 90 days
  renewBefore: 360h  # 15 days
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
    - themisdb.example.com
    - "*.themisdb.example.com"
```

---

## 9. Notfallverfahren

### 9.1 Key Compromise Response

```
┌─────────────────────────────────────────────────────────────────┐
│              SCHLÜSSELKOMPROMITTIERUNG                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. SOFORT: Kompromittierten Schlüssel sperren                  │
│     └── Vault: vault write transit/keys/<name>/config           │
│             min_decryption_version=<n> deletion_allowed=true    │
│                                                                  │
│  2. BENACHRICHTIGUNG: Security Team + Management                │
│                                                                  │
│  3. ASSESSMENT: Umfang der Kompromittierung feststellen         │
│     └── Welche Daten betroffen?                                 │
│     └── Wie lange war der Schlüssel kompromittiert?             │
│                                                                  │
│  4. ROTATION: Neuen Schlüssel erzeugen                          │
│                                                                  │
│  5. RE-ENCRYPTION: Alle betroffenen Daten neu verschlüsseln     │
│                                                                  │
│  6. REVOCATION: Betroffene Zertifikate widerrufen               │
│                                                                  │
│  7. FORENSIK: Root Cause Analysis durchführen                   │
│                                                                  │
│  8. MELDUNG: Bei PII-Beteiligung DSB/Aufsichtsbehörde           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 9.2 HSM Failure

| Szenario | Response |
|----------|----------|
| HSM nicht erreichbar | Failover zu Backup-HSM |
| HSM-Ausfall | Disaster Recovery Prozess |
| Tamper Alert | Sofortige Untersuchung, HSM isolieren |
| Key Unavailable | Recovery mit M-of-N Shares |

### 9.3 Key Recovery

```
M-of-N Key Recovery Prozess:
─────────────────────────────
1. Mindestens M von N Key Custodians erforderlich
2. Jeder Custodian authentifiziert sich (MFA)
3. Jeder Custodian gibt seinen Share ein
4. System rekonstruiert den Schlüssel
5. Recovery-Vorgang wird vollständig protokolliert
6. Schlüssel steht für Operationen zur Verfügung
7. Shares werden nach Verwendung wieder gesperrt

Beispiel: 3-of-5 Recovery
- 5 Key Custodians (geografisch verteilt)
- Mindestens 3 müssen verfügbar sein
- Keine einzelne Person kann Schlüssel wiederherstellen
```

---

## 10. Audit und Compliance

### 10.1 Logging-Anforderungen

| Ereignis | Log-Inhalt | Retention |
|----------|------------|-----------|
| Key Generation | Key ID, Algo, Length, Requester | 7 Jahre |
| Key Usage | Key ID, Operation, Timestamp | 3 Jahre |
| Key Rotation | Old ID, New ID, Reason | 7 Jahre |
| Key Destruction | Key ID, Method, Approver | 7 Jahre |
| Access Denied | Key ID, Requester, Reason | 1 Jahr |
| HSM Operations | All operations | 7 Jahre |

### 10.2 Crypto-Audit Checklist

```
□ Alle verwendeten Algorithmen sind zugelassen
□ Keine deprecated Algorithmen in Produktion
□ Schlüsselrotation erfolgt planmäßig
□ HSM-Zertifizierungen sind aktuell
□ Key Custodians sind definiert und aktuell
□ Recovery-Tests wurden durchgeführt
□ Audit-Logs sind vollständig und integer
□ TLS-Konfiguration entspricht Best Practices
□ Zertifikate haben ausreichende Laufzeit
□ Crypto-Bibliotheken sind aktuell
```

### 10.3 Compliance-Mapping

| Kontrolle | BSI C5 | ISO 27001 | NIST | PCI DSS |
|-----------|--------|-----------|------|---------|
| Algo-Standards | CRY-01 | A.10.1.1 | SC-12 | 3.5 |
| Key Management | CRY-02 | A.10.1.2 | SC-12 | 3.5-3.7 |
| Key Storage | CRY-03 | A.10.1.2 | SC-12 | 3.5.2 |
| Key Rotation | CRY-04 | A.10.1.2 | SC-12 | 3.6.4 |
| TLS Config | CRY-01 | A.13.1.1 | SC-8 | 4.1 |

---

## 11. Review und Aktualisierung

### 11.1 Review-Zyklus

| Aktivität | Häufigkeit | Verantwortlich |
|-----------|------------|----------------|
| Richtlinien-Review | Jährlich | CISO |
| Algorithmen-Review | Halbjährlich | Security Architect |
| Key Inventory | Quartalsweise | Key Custodians |
| HSM-Audit | Jährlich | Externer Auditor |
| Crypto-Assessment | Jährlich | Penetration Tester |

### 11.2 Crypto-Agility

ThemisDB implementiert Crypto-Agility für zukünftige Algorithmus-Updates:

```cpp
// Crypto-Agile Design
class CryptoProvider {
public:
    virtual std::vector<uint8_t> encrypt(
        const std::vector<uint8_t>& data,
        const Key& key) = 0;
    
    virtual std::vector<uint8_t> decrypt(
        const std::vector<uint8_t>& ciphertext,
        const Key& key) = 0;
        
    virtual ~CryptoProvider() = default;
};

class AES256GCMProvider : public CryptoProvider { /* ... */ };
class ChaCha20Provider : public CryptoProvider { /* ... */ };
class PostQuantumProvider : public CryptoProvider { /* ... */ }; // Future
```

### 11.3 Post-Quantum Readiness

Vorbereitung auf Post-Quantum-Kryptographie:

| Phase | Zeitrahmen | Maßnahmen |
|-------|------------|-----------|
| Awareness | 2024-2025 | Evaluierung PQC-Algorithmen (CRYSTALS-Kyber, CRYSTALS-Dilithium) |
| Hybrid | 2025-2027 | Hybrid-Verschlüsselung (klassisch + PQC) |
| Migration | 2027-2030 | Vollständige Migration zu PQC |

---

## 12. Änderungshistorie

| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| 1.0 | 2025-11-27 | Erstversion | Security Team |

---

**Genehmigt von:**

| Rolle | Name | Datum | Unterschrift |
|-------|------|-------|--------------|
| CISO | _________________ | __________ | _____________ |
| CTO | _________________ | __________ | _____________ |
| Security Architect | _________________ | __________ | _____________ |
