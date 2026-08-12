# Kapitel 22b: Verschlüsselung

> *"Kryptographie ist die letzte Verteidigungslinie — sie macht gestohlene Daten wertlos."*

---

## Überblick

Dieses Kapitel beschreibt das vollständige Verschlüsselungskonzept von ThemisDB: von der Transportverschlüsselung über Data-at-Rest bis hin zur feingranularen Column-Level Encryption für sensible Felder und der Verschlüsselung von Vektordaten.

**Was Sie in diesem Kapitel lernen werden:**
- Kryptographie-Policy und zugelassene Algorithmen
- Transport-Verschlüsselung (TLS 1.3)
- Data-at-Rest Encryption auf Storage-Ebene
- Column-Level Encryption (AES-256-GCM) für Felder
- Vektor-Verschlüsselung (HNSW-kompatibel)
- Key Management (KEK/DEK-Hierarchie, Vault, HSM, PKCS#11)
- Post-Quantum Hybrid Encryption
- BSI C5 / ISO 27001 / DSGVO Compliance

**Voraussetzungen:** Kapitel 21a (Authentifizierung), Kapitel 36 (Security Hardening).

---

## 22.1 Kryptographie-Policy (Überblick)

ThemisDB folgt einer verbindlichen Kryptographie-Policy, die auf BSI TR-02102-1, NIST SP 800-38D und ISO/IEC 27001 basiert.

### 22.1.1 Zugelassene Algorithmen

| Anwendungsfall | Algorithmus | Schlüssellänge | Standard |
|---|---|---|---|
| Symmetrische Verschlüsselung (primär) | AES-256-GCM | 256 bit | BSI C5 CRY-01 |
| Symmetrische Verschlüsselung (alt.) | ChaCha20-Poly1305 | 256 bit | NIST |
| Asymmetrische Verschlüsselung | RSA-OAEP/SHA-256 | ≥2048 bit | BSI |
| Elliptic Curve | ECDH/P-256, P-384 | — | NIST |
| Key Exchange | X25519, X448 | — | RFC 7748 |
| Hashing | SHA-256, SHA-384, SHA-512 | — | FIPS 180-4 |
| Schlüsselableitung | HKDF-SHA-256 | — | RFC 5869 |
| Post-Quantum (Hybrid) | CRYSTALS-Kyber-768 + X25519 | — | NIST PQC Round 3 |

**Explizit verboten:**
- ❌ AES-ECB (Pattern Leakage, keine Authentifizierung)
- ❌ DES / 3DES (veraltet)
- ❌ RC4 (Keystream-Bias)
- ❌ MD5 / SHA-1 für Sicherheitszwecke

---

## 22.2 Transport-Verschlüsselung (TLS)

### 22.2.1 TLS 1.3 Konfiguration

ThemisDB erzwingt **TLS 1.3** für alle externen Verbindungen. TLS 1.2 wird nur für Legacy-Clients mit expliziter Konfiguration unterstützt.

```yaml
# themisdb.yml — TLS-Konfiguration
server:
  tls:
    enabled: true
    min_version: "TLS1.3"
    cert_file: "/etc/themisdb/tls/server.crt"
    key_file:  "/etc/themisdb/tls/server.key"
    ca_file:   "/etc/themisdb/tls/ca.crt"
    cipher_suites:
      - TLS_AES_256_GCM_SHA384
      - TLS_CHACHA20_POLY1305_SHA256
      - TLS_AES_128_GCM_SHA256
```

### 22.2.2 Mutual TLS (mTLS) für Cluster-Kommunikation

Shard-to-Shard-Kommunikation verwendet **mTLS** mit gegenseitiger Zertifikatsverifizierung:

```yaml
cluster:
  shard_communication:
    mtls:
      enabled: true
      client_cert: "/etc/themisdb/tls/shard.crt"
      client_key:  "/etc/themisdb/tls/shard.key"
      verify_peer: true
```

```
Shard A ──── [mTLS: Client-Cert + Server-Cert Validation] ──── Shard B
              ↑                                           ↑
         CA-signed                                   CA-signed
```

**Implementierung:** `src/security/` — mTLS über OpenSSL; Details: [`docs/de/security/MTLS_SHARD_COMMUNICATION.md`](../../de/security/MTLS_SHARD_COMMUNICATION.md)

---

## 22.3 Data-at-Rest Encryption (Storage-Ebene)

### 22.3.1 RocksDB Encryption-at-Rest

ThemisDB kann RocksDB mit einem Encryption-Provider konfigurieren, der alle SST-Dateien und den WAL verschlüsselt.

```cpp
// Konfiguration in StorageEngine
rocksdb::DBOptions options;
std::shared_ptr<rocksdb::EncryptionProvider> provider;
rocksdb::EncryptionProvider::CreateFromString(
    rocksdb::ConfigOptions{},
    "AES",
    &provider);
options.env = rocksdb::NewEncryptedEnv(rocksdb::Env::Default(),
                                        provider);
```

### 22.3.2 Verschlüsselte Backups

Backups werden mit AES-256-GCM verschlüsselt. Der Backup-Schlüssel ist vom operativen DEK getrennt und wird im Key Management System (KMS) gespeichert.

```
DB-Daten ──── Backup ──── AES-256-GCM (Backup-DEK) ──── Backup-Datei
                                 ↑
                    Backup-DEK verschlüsselt mit KEK (Vault/HSM)
```

---

## 22.4 Column-Level Encryption (Feld-Ebene)

Column-Level Encryption (CLE) ermöglicht die granulare Verschlüsselung einzelner Datenfelder at-rest — unabhängig vom Storage-Layer.

### 22.4.1 Architektur

```
┌─────────────────────────────────────────────────────────┐
│ Application Layer                                       │
│   EncryptedField<T>  (Template, transparent für Devs)   │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│ Encryption Layer                                        │
│   FieldEncryption: AES-256-GCM Encrypt/Decrypt          │
│   Key Cache: TTL-basiert (1h, max. 1000 Keys)           │
└────────────┬────────────────────┬───────────────────────┘
             │                    │
   ┌──────────▼──────┐  ┌─────────▼───────┐
   │ Key Management  │  │  RocksDB Storage │
   │ VaultKeyProvider│  │ (Ciphertext)     │
   │ HSMProvider     │  └─────────────────┘
   └─────────────────┘
```

**Implementierung:** `include/security/encryption.h` (610 Zeilen), `src/security/field_encryption.cpp` (712 Zeilen)

### 22.4.2 EncryptedField\<T\> Template

Entwickler nutzen das `EncryptedField<T>` Template — Verschlüsselung ist transparent:

```cpp
#include "security/encryption.h"

struct UserProfile {
    // Sensible Felder werden automatisch verschlüsselt
    EncryptedField<std::string>  email;
    EncryptedField<std::string>  ssn;
    EncryptedField<std::string>  credit_card;
    EncryptedField<std::vector<float>> embedding;  // Vektordaten

    // Nicht-sensible Felder unverschlüsselt
    std::string username;
    uint64_t    created_at;
};

// Nutzung — transparent:
UserProfile profile;
profile.email.set("alice@example.com");          // Encrypt on set
std::string email = profile.email.get();         // Decrypt on get
```

### 22.4.3 AES-256-GCM Encrypt/Decrypt

```cpp
// Intern: FieldEncryption::encrypt()
struct EncryptedBlob {
    std::string  key_id;         // z.B. "user_emails"
    uint32_t     key_version;    // Für Key Rotation
    std::vector<uint8_t> iv;     // 12 Bytes — einmalig pro Operation
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;    // 16 Bytes GCM Auth-Tag
};

EncryptedBlob blob = field_encryption.encrypt(
    plaintext_bytes,
    "user_emails",    // key_id
    {}                // optional AAD (Additional Authenticated Data)
);
```

**Wichtig:** Jede Verschlüsselungsoperation generiert einen **neuen, zufälligen IV** (96 bit / 12 Bytes). Wiederverwendung von IVs ist fatal für GCM-Sicherheit.

### 22.4.4 Key Rotation (ohne Downtime)

ThemisDB unterstützt **Lazy Re-Encryption** und **Dual-Write**:

```
Alt-Key (v1) ──── Lesen:  Entschlüsseln mit v1
                  Schreiben: Neu verschlüsseln mit v2 (Dual-Write)

Nach vollständiger Migration: Alt-Key v1 löschen
```

```cpp
// Key Rotation auslösen
POST /keys/rotate
{ "key_id": "user_emails" }

// Antwort
{ "success": true, "key_id": "user_emails", "new_version": 3 }
```

---

## 22.5 Vektor-Verschlüsselung

Vektordaten (Embeddings) können ebenfalls verschlüsselt gespeichert werden, was HNSW-Ähnlichkeitssuche auf verschlüsselten Daten ermöglicht.

### 22.5.1 Ansätze

| Ansatz | Performance | Suchbarkeit | Empfehlung |
|---|---|---|---|
| Individuelle Verschlüsselung (pro Vektor) | ⭐⭐⭐ | ✅ vollständig (Entschlüsselung vor HNSW) | **Empfohlen** |
| Batch-Verschlüsselung (Blöcke) | ⭐⭐⭐⭐ | ✅ batch-level | Für große Datasets |
| Storage-Level (RocksDB) | ⭐⭐⭐⭐⭐ | ✅ vollständig | Einfachste Integration |
| Homomorphe Verschlüsselung | ⭐ | ✅ direkt auf Ciphertext | Experimentell (zu langsam) |

**Empfohlener Ansatz:** Individuelle Vektorverschlüsselung (AES-256-GCM) kombiniert mit RocksDB-Level-Encryption.

### 22.5.2 Konfiguration

```yaml
# themisdb.yml
vector:
  encryption:
    enabled: true
    key_id: "vector_embeddings"
    algorithm: "AES-256-GCM"
    batch_decrypt_threshold: 1000  # Batch-Entschlüsselung für HNSW-Suche
```

---

## 22.6 Key Management (KEK/DEK-Hierarchie)

### 22.6.1 Schlüssel-Hierarchie

```
┌─────────────────────────────────────────────┐
│  Master Key (MK)                            │
│  → Schutz durch HSM / Cloud KMS             │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│  Key Encryption Key (KEK)                   │
│  → Verschlüsselt DEKs                       │
│  → Rotation: jährlich (BSI C5 CRY-02)       │
└─────────────────┬───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│  Data Encryption Keys (DEK)                 │
│  → Einer pro Feld-Kategorie (email, ssn...) │
│  → Rotation: 90 Tage (empfohlen)            │
│  → Versioniert für Lazy Re-Encryption       │
└─────────────────────────────────────────────┘
```

### 22.6.2 KeyProvider Interface

```cpp
class KeyProvider {
public:
    virtual ~KeyProvider() = default;
    virtual std::vector<uint8_t> getKey(
        const std::string& key_id,
        uint32_t version = 0) = 0;
    virtual uint32_t getCurrentVersion(
        const std::string& key_id) = 0;
    virtual bool rotateKey(const std::string& key_id) = 0;
};
```

### 22.6.3 Provider-Implementierungen

| Provider | Produktionsreif | Use Case |
|---|---|---|
| **MockKeyProvider** | ✅ | Development / Tests |
| **VaultKeyProvider** | ✅ (739 Zeilen) | Enterprise KMS (HashiCorp Vault) |
| **HSMProvider (PKCS#11)** | ✅ (1056 Zeilen) | Hardware Security Module |

**VaultKeyProvider** — HashiCorp Vault KV v2 und Transit Engine:

```yaml
security:
  key_provider: "vault"
  vault:
    address: "https://vault.example.com:8200"
    token_env: "VAULT_TOKEN"
    kv_mount: "secret"
    transit_mount: "transit"
    timeout_ms: 5000
    cache_ttl_seconds: 3600
```

**HSMProvider** — PKCS#11 für Hardware-Schlüsselschutz:

```yaml
security:
  key_provider: "hsm"
  hsm:
    library_path: "/usr/lib/softhsm/libsofthsm2.so"
    slot_id: 0
    pin_env: "HSM_PIN"
    key_label_prefix: "themisdb_"
```

---

## 22.7 Post-Quantum Hybrid Encryption

ThemisDB implementiert **Post-Quantum Hybrid Encryption** als Vorbereitung auf quantencomputer-resistente Kryptographie.

### 22.7.1 Hybrid-Ansatz

```
Klassisch: X25519 (ECDH)      ─┐
                                ├── HKDF-SHA-256 → Kombinierter Key → AES-256-GCM
PQC:       CRYSTALS-Kyber-768  ─┘
```

Durch die Kombination beider Schlüsselaustauschmechanismen bleibt die Verschlüsselung auch dann sicher, wenn einer der Algorithmen kompromittiert wird (klassisch durch Quantencomputer, PQC durch analytische Angriffe).

**Implementierung:** `src/security/post_quantum_crypto.cpp`

```yaml
security:
  post_quantum:
    enabled: false  # Opt-in (erhöht Key-Exchange-Overhead ~2ms)
    algorithm: "kyber768_x25519_hkdf_sha256"
```

---

## 22.8 BSI C5 / ISO 27001 / DSGVO Compliance

### 22.8.1 Compliance-Matrix

| Kontrolle | Anforderung | ThemisDB-Implementierung | Status |
|---|---|---|---|
| BSI C5 CRY-01 | Kryptographie-Policy | `CRYPTOGRAPHY_POLICY.md` | ✅ |
| BSI C5 CRY-02 | Schlüsselmanagement | VaultKeyProvider / HSMProvider | ✅ |
| BSI C5 CRY-03 | Data-at-Rest Encryption | RocksDB Encryption + Column-Level | ✅ |
| BSI C5 CRY-04 | Data-in-Transit Encryption | TLS 1.3 + mTLS | ✅ |
| BSI C5 CRY-05 | Schlüsselrotation | Lazy Re-Encryption, Dual-Write | ✅ |
| BSI C5 CRY-06 | Kryptographische Integrität | GCM Auth-Tag | ✅ |
| ISO 27001 A.10 | Kryptographie-Kontrollen | Vollständig abgedeckt | ✅ |
| DSGVO Art. 32 | Geeignete technische Maßnahmen | AES-256-GCM + Key Rotation | ✅ |
| DSGVO Art. 25 | Privacy by Design | EncryptedField\<T\> als Default | ✅ |

**Gesamtbewertung:** ✅ **BSI C5 KONFORM** (Audit: `docs/de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md`)

---

## 22.9 Performance-Charakteristika

| Operation | Overhead | Messmethode |
|---|---|---|
| AES-256-GCM Encrypt (1 KB) | < 1 ms | `tests/test_encryption_perf.cpp` |
| AES-256-GCM Decrypt (1 KB) | < 1 ms | — |
| Key Cache Hit | < 0,1 ms | TTL-basierter In-Memory-Cache |
| Key Cache Miss (Vault) | 2–10 ms | Netzwerk-RTT abhängig |
| HSM-Operation | 5–20 ms | Hardware-abhängig |
| TLS Handshake (TLS 1.3) | 1–5 ms | 0-RTT für Wiederverbindungen |
| Vector Encrypt (1536-dim float) | < 2 ms | Batch-Mode empfohlen |

**Batch-Optimierung:** Für Massen-Operationen (z.B. Vektorindex-Rebuild) unterstützt `FieldEncryption` Batch-Encrypt/Decrypt mit OpenSSL-Pipeline.

---

## 22.10 Troubleshooting

| Problem | Ursache | Lösung |
|---|---|---|
| `EncryptionError: key not found` | Key ID existiert nicht in KMS | `GET /keys` prüfen; Key-Konfiguration validieren |
| `DecryptionError: tag mismatch` | Datenkorruption oder falscher Key | Key-Version prüfen; ggf. Backup wiederherstellen |
| Hohe Latenz bei Entschlüsselung | Key Cache Miss | Cache TTL erhöhen; Vault-Verbindung prüfen |
| TLS Handshake fehlgeschlagen | Zertifikat abgelaufen | `openssl x509 -in server.crt -dates` prüfen |
| HSM nicht erreichbar | PKCS#11 Library-Pfad falsch | `library_path` in Config prüfen |
| `VAULT_TOKEN` nicht gesetzt | ENV-Variable fehlt | Deployment-Konfiguration prüfen |

---

## 22.11 Phase-3-Sync: Weiterführende Referenzen (docs/de/) {#chapter22_11_cross-references}

> Detaillierte Implementierungsdokumentation zu den behandelten Verschlüsselungsthemen:

| Thema | Referenz |
|---|---|
| Column-Level Encryption (Design + Status) | [`docs/de/security/security_column_encryption.md`](../../de/security/security_column_encryption.md) |
| Kryptographie-Policy | [`docs/de/security/CRYPTOGRAPHY_POLICY.md`](../../de/security/CRYPTOGRAPHY_POLICY.md) |
| Schlüsselverwaltung | [`docs/de/security/security_key_management.md`](../../de/security/security_key_management.md) |
| Key Rotation Verfahren | [`docs/de/security/security_key_rotation.md`](../../de/security/security_key_rotation.md) |
| Key Lifecycle Management | [`docs/de/security/KEY_LIFECYCLE_MANAGEMENT.md`](../../de/security/KEY_LIFECYCLE_MANAGEMENT.md) |
| HSM PKCS#11 Integration | [`docs/de/security/security_hsm.md`](../../de/security/security_hsm.md) |
| Vault LoRA Setup | [`docs/de/security/vault_lora_setup.md`](../../de/security/vault_lora_setup.md) |
| BSI C5 Column Encryption Compliance | [`docs/de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md`](../../de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md) |
| BSI C5 Executive Summary | [`docs/de/security/BSI_C5_EXECUTIVE_SUMMARY.md`](../../de/security/BSI_C5_EXECUTIVE_SUMMARY.md) |
| BSI C5 Multi-Model Encryption | [`docs/de/security/BSI_C5_MULTI_MODEL_ENCRYPTION_ANALYSIS.md`](../../de/security/BSI_C5_MULTI_MODEL_ENCRYPTION_ANALYSIS.md) |
| Vektor-Verschlüsselung (Quick Start) | [`docs/de/security/QUICK_START_VECTOR_ENCRYPTION.md`](../../de/security/QUICK_START_VECTOR_ENCRYPTION.md) |
| Vektor-Verschlüsselung (Impl. Summary) | [`docs/de/security/VECTOR_ENCRYPTION_IMPLEMENTATION_SUMMARY.md`](../../de/security/VECTOR_ENCRYPTION_IMPLEMENTATION_SUMMARY.md) |
| HNSW Encryption Konfiguration | [`docs/de/security/HNSW_ENCRYPTION_CONFIGURATION.md`](../../de/security/HNSW_ENCRYPTION_CONFIGURATION.md) |
| Encrypted HNSW Suchbarkeit | [`docs/de/security/ENCRYPTED_HNSW_SEARCHABILITY.md`](../../de/security/ENCRYPTED_HNSW_SEARCHABILITY.md) |
| Symmetrische Verschlüsselungsansätze | [`docs/de/security/SYMMETRIC_ENCRYPTION_APPROACHES.md`](../../de/security/SYMMETRIC_ENCRYPTION_APPROACHES.md) |
| Verschlüsselung at-rest (Forschung) | [`docs/de/security/security_at_rest_encryption_research.md`](../../de/security/security_at_rest_encryption_research.md) |
| Verschlüsselung Strategie | [`docs/de/security/security_encryption_strategy.md`](../../de/security/security_encryption_strategy.md) |
| Verschlüsselung Deployment | [`docs/de/security/security_encryption_deployment.md`](../../de/security/security_encryption_deployment.md) |
| PKI-Architektur | [`docs/de/security/security_pki_architecture.md`](../../de/security/security_pki_architecture.md) |
| mTLS Shard-Kommunikation | [`docs/de/security/MTLS_SHARD_COMMUNICATION.md`](../../de/security/MTLS_SHARD_COMMUNICATION.md) |
| Security Primärquellen | [`docs/de/security/PRIMARY_SOURCES.md`](../../de/security/PRIMARY_SOURCES.md) |

**→ Zurück:** [Kapitel 22a: Clients](chapter_22_clients.md)
**→ Weiter:** [Kapitel 36: Security Hardening](chapter_36_security_hardening.md)

---

**Kapitel 22b von 43** | **Teil VI: Sicherheit** | **Phase-3-Sync: ✅** | **~3.200 Wörter**
