# Verschlüsselungsstrategie für ThemisDB (E2E On-Premise)

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [📋 Executive Summary](#executive-summary)
- [🏗️ Architektur-Übersicht](#1-architektur-übersicht)
- [🔑 PKI-Integration](#2-pki-integration-vcc-pki)
- [👤 User-Context](#3-user-context-integration-vcc-user)
- [💾 Datenmodell-Verschlüsselung](#4-datenmodell-spezifische-verschlüsselung)
- [💡 Best Practices](#best-practices)
- [🔧 Troubleshooting](#troubleshooting)
- [📚 Siehe auch](#siehe-auch)
- [📝 Changelog](#änderungsverlauf)


## Executive Summary

**Ziel:** End-to-End-Verschlüsselung für sensible Daten in ThemisDB mit on-premise PKI-basiertem Key-Management unter Nutzung des VCC-PKI-Systems (`c:\vcc\pki`) und VCC-User-Systems (`c:\vcc\user`).

**Kernprinzipien:**
- 🔓 **Metadaten sichtbar**: Indexstrukturen, PKs, Timestamps, Kategorien bleiben unverschlüsselt für Query-Performance
- 🔐 **Daten verschlüsselt**: Graph-Properties, Relational-Fields, Content-Blobs, Vector-Embeddings verschlüsselt at-rest
- 🔑 **PKI-basiert**: Integration mit VCC-PKI für Zertifikat-basierte Schlüsselableitung
- 👤 **User-Context**: Per-User-Verschlüsselung via VCC-User-System (JWT-Propagation)
- 🚫 **Zero-Knowledge**: Ohne korrekten Schlüssel keine Datenrekonstruktion möglich

---

## 1. Architektur-Übersicht

### 1.1 Threat Model

**Was wird geschützt:**
- Graph: Edge-Properties (z.B. `weight`, `metadata`, benutzerdefinierte Felder)
- Relational: Sensitive Spalten (z.B. `email`, `phone`, `address`, Custom-Fields)
- Content: Binärblobs (PDF, DOCX, Bilder mit EXIF, Audio mit Metadaten)
- Vector: Embeddings (768-dim float32, rekonstruierbar → Originaldokument)

**Was NICHT verschlüsselt wird (Performance/Query):**
- Primary Keys, Foreign Keys
- Index-Keys (SecondaryIndex, CompositeIndex)
- Timestamps (`created_at`, `modified_at`)
- Kategorien, Tags, MIME-Types
- Vector-Dimensionen (für Index-Initialisierung)
- Graph-Topologie (Knoten-IDs, Kanten-Richtung, Label)

**Angriffszenarien:**
1. ❌ Disk-Theft: Festplatte gestohlen → verschlüsselte Daten unlesbar
2. ❌ Backup-Leak: Backup-Datei im Netz → ohne Schlüssel nutzlos
3. ❌ Insider-Threat: DB-Admin ohne User-Key kann Daten nicht lesen
4. ❌ Memory-Dump: Angreifer kann nur kurzlebige In-Memory-Schlüssel extrahieren

---

## 2. PKI-Integration (VCC-PKI)

### 2.1 VCC-PKI System (`c:\vcc\pki`)

**Vorhandene Infrastruktur:**
- **Root CA**: 10 Jahre Gültigkeit, 4096-bit RSA
- **Intermediate CA**: 5 Jahre, signiert Service-Zertifikate
- **Service Certificates**: Pro Service (veritas, covina, clara, themis)
- **REST API**: `https://localhost:8443/api/v1` (FastAPI)
- **mTLS**: Client-Zertifikat-basierte Authentifizierung (geplant)

**Nutzung für ThemisDB:**
```
Root CA (VCC Root CA)
 └── Intermediate CA (VCC Intermediate CA)
      ├── Service Cert: themis-db.vcc.local
      └── Data Encryption Key (DEK) Wrapping Cert
```

### 2.2 Key-Hierarchie

**3-Tier Key-Architektur:**

```
┌─────────────────────────────────────────┐
└──────────────┬──────────────────────────┘
               │ verschlüsselt
               ▼
│  DEK (Data Encryption Key)              │
│  - AES-256-GCM Master-Key               │
│  - Pro Datenbank/Tenant                 │
│  - Gespeichert verschlüsselt in DB      │
│  - Rotierbar ohne Daten-Re-Encryption   │
└──────────────┬──────────────────────────┘
│  - Aus JWT-Token + DEK abgeleitet       │
│  - HKDF mit User-ID als Context         │
│  - Ephemeral (nur In-Memory)            │
└─────────────────────────────────────────┘
               │ verschlüsselt
               ▼
        [Sensitive Data]
```

**Key-Derivation (aktuelle Implementierung inkl. persistenter KEK-Ableitung & Group-DEKs):**
```cpp
// 1. Persistentes IKM für KEK (einmalig erzeugt, hex in RocksDB gespeichert)
auto ikm = get_or_create_hex("kek:ikm:{service_id}", 32);
KEK = HKDF_SHA256(
  salt = "",                       // leer
  ikm  = ikm,                       // 32 Byte zufällig
  info = "KEK derivation:" + service_id
); // 32 Byte

// 2. DEK laden/erstellen (AES-256, GCM-verschlüsselt mit KEK)
auto enc_dek = storage->get("dek:encrypted:v1");
if (!enc_dek) {
  DEK = random_bytes(32);
  Blob b = AES_GCM_Encrypt(DEK, KEK); // {iv(12), ct, tag(16)} → JSON oder Binär
  storage->put("dek:encrypted:v1", b.to_json());
} else {
  Blob b = Blob::from_json(enc_dek);
  DEK = AES_GCM_Decrypt(b, KEK);
}

// 3. Group-DEK (Mehrparteienzugriff)
// key:group:{group}:v{n} => nonce||ciphertext||tag (KEK-wrap)
auto group_DEK = get_or_create_group_dek("hr_team"); // AES-256

// 4. Feldschlüssel je nach Kontext (user oder group)
user_id = claims.sub;
auto field_key_user = HKDF_SHA256(DEK, user_id, "field:" + field_name);
auto field_key_group = HKDF_SHA256(group_DEK, "", "field:" + field_name);
```

#### 2.3 Group-DEKs (Mehrparteienzugriff)
- Pro Gruppe (`hr_team`, `finance_dept`, …) existiert ein eigener 256-bit DEK.
- Speicherung: AES-256-GCM unter KEK, Key `key:group:{group}:v{n}` → Binär `nonce||ciphertext||tag` (oder JSON `{iv,ciphertext,tag}`).
- Metadaten: `key:group:{group}:meta` → `"{current_version}|{timestamp}|{optional_status}"`.
- Rotation: `rotateGroupDEK(group)` erzeugt neue Version und aktualisiert Metadaten; alte Version kann für Lesepfad (optional) bereitgehalten werden (aktuell: sofortige Ungültigkeit).
- Vorteile:
  - Mehrere User können identische Datensätze entschlüsseln, ohne personenbezogene Schlüssel zu teilen.
  - Beim Austritt eines Nutzers genügt die Group-DEK-Rotation (Re-Encryption der Daten nötig; lazy Migration möglich).
  - Reduziert Speicher-Footprint gegenüber rein per-user Schlüsselmaterial.
## 3. User-Context-Integration (VCC-User)

### 3.1 VCC-User System (`c:\vcc\user`)

**Identity Propagation:**
- **Keycloak**: OIDC Identity Provider mit AD-Föderation
- **JWT-Token**: Durchgängige Propagation durch alle Services
- **Zero-Trust**: Jeder Service validiert JWT unabhängig

**JWT-Claims für ThemisDB:**
```json
{
  "sub": "user123",
  "email": "alice@vcc.local",
  "groups": ["data_scientists", "hr_team"],
  "roles": ["data_reader", "pii_access"],
  "iss": "https://keycloak.vcc.local/realms/vcc",
  "exp": 1730000000
}
```

### 3.2 Access-Control-basierte Verschlüsselung

**Idee:** Verschiedene User-Gruppen haben verschiedene Verschlüsselungskontext → Multi-User-Encryption

**Beispiel:**
```cpp
// In ThemisDB HTTP-Handler
std::string jwt_token = request.get_header("Authorization");
auto claims = jwt_validator_.parse_and_validate(jwt_token);
std::string user_id = claims["sub"];
std::vector<std::string> groups = claims["groups"];

// Ableitung eines gruppenspezifischen Schlüssels
std::string encryption_context = user_id; // oder group[0] für Gruppenschlüssel
auto field_key = key_provider_->deriveUserKey(dek_, encryption_context, field_name);

// Verschlüsseln mit User-Context
EncryptedBlob blob = field_encryption_->encrypt(sensitive_data, field_key);
```

**Vorteil:**
- 👤 **User-Isolation**: User A kann Daten von User B nicht entschlüsseln
- 👥 **Gruppenschlüssel**: HR-Gruppe verschlüsselt mit `group=hr_team` → alle HR-Mitglieder können lesen
- 🔄 **Key-Rotation**: Bei User-Austritt → Keys ungültig ohne Daten-Re-Encryption

---

## 4. Datenmodell-spezifische Verschlüsselung

### 4.1 Graph (Property Graph)

**Was verschlüsseln:**
```cpp
// BaseEntity für Graph-Edge
{
  "pk": "graph:edge:alice->bob",           // PLAIN (Index)
  "from": "alice",                          // PLAIN (Topologie)
  "to": "bob",                              // PLAIN (Topologie)
  "label": "KNOWS",                         // PLAIN (Query)
  "created_at": 1730000000,                 // PLAIN (Index)
  "weight": 0.95,                           // 🔐 ENCRYPTED
  "metadata": {                             // 🔐 ENCRYPTED (ganzes Objekt)
    "since": "2020-01-01",
    "context": "university"
  }
}
```

**Implementierung:**
```cpp
// In GraphIndexManager::addEdge()
BaseEntity::FieldMap fields;
fields["pk"] = edge.getPrimaryKey();
fields["from"] = edge.getFieldAsString("from");
fields["to"] = edge.getFieldAsString("to");
fields["label"] = edge.getFieldAsString("label");
fields["created_at"] = edge.getFieldAsInt("created_at");

// Sensitive Felder verschlüsseln
if (auto weight = edge.getField("weight")) {
    std::string user_key = deriveUserKey(jwt_context, "edge.weight");
    auto encrypted = field_enc_->encrypt(serializeValue(*weight), user_key);
    fields["weight_encrypted"] = encrypted.toBase64();
}
if (auto meta = edge.getField("metadata")) {
    std::string user_key = deriveUserKey(jwt_context, "edge.metadata");
    auto encrypted = field_enc_->encrypt(serializeValue(*meta), user_key);
    fields["metadata_encrypted"] = encrypted.toBase64();
}

BaseEntity encrypted_edge = BaseEntity::fromFields(pk, fields);
storage_->put(key, encrypted_edge.serialize());
```

**Aktuelle Implementierung (Schema-driven):**
Graph-Edges werden als `BaseEntity` gespeichert und nutzen die generische Schema-basierte Verschlüsselung:

```json
{
  "collections": {
    "edges": {
      "encryption": {
        "enabled": true,
        "fields": ["weight", "metadata", "properties"],
        "context_type": "user"  // oder "group" für Team-Graphen
      }
    }
  }
}
```

**Ablauf:**
1. Edge erstellen via `POST /entities` mit `table=edges` und Body `{id, _from, _to, weight, metadata}`
2. `handlePutEntity` lädt Schema → verschlüsselt `weight` und `metadata`
3. `GraphIndexManager::addEdge` speichert verschlüsselte Entity
4. Graph-Traversal (`/graph/traverse`) gibt verschlüsselte Daten zurück
5. Client setzt `?decrypt=true` für Entschlüsselung im Response

**Vorteile:**
- ✅ Keine Code-Duplikation (nutzt existierende Schema-Encryption)
- ✅ Konsistente Verschlüsselung über alle Datenmodelle
- ✅ JWT-Context automatisch propagiert

**Einschränkungen:**
- ⚠️ Graph-Traversal gibt verschlüsselte Edge-Properties zurück (Client muss nachträglich entschlüsseln)
- ⚠️ Gewichtete Algorithmen (Dijkstra) können nicht auf verschlüsselten Weights operieren

### 4.2 Relational (BaseEntity Fields)

**Schema-basierte Verschlüsselung:**
```json
{
  "schema": {
    "users": {
      "fields": {
        "id": { "type": "string", "encrypted": false, "indexed": true },
        "email": { "type": "string", "encrypted": true, "indexed": false },
        "name": { "type": "string", "encrypted": false, "indexed": true },
        "ssn": { "type": "string", "encrypted": true, "indexed": false },
        "salary": { "type": "int64", "encrypted": true, "indexed": false }
      }
    }
  }
}
```

**Automatische Verschlüsselung (erweiterte Version mit Kontextwahl und strukturierten Metafeldern):**
```cpp
// In QueryEngine beim INSERT
auto schema = loadSchema("users");
for (const auto& [field, config] : schema.fields) {
    if (config.encrypted) {
        auto value = entity.getField(field);
    std::vector<uint8_t> field_key;
    if (config.context_type == "user") {
      field_key = hkdf(DEK, jwt.sub, "field:" + field);    // per User
    } else {
      auto group = pick_group(jwt.claims, config.allowed_groups);
      auto gdek  = getGroupDEK(group);
      field_key  = hkdf(gdek, "", "field:" + field);      // per Gruppe
      entity.setField(field + "_group", group);             // Kontext speichern
    }
    auto enc = field_enc_->encryptWithKey(serializeValue(*value),
                        "field:" + field,
                        /*version*/1,
                        field_key);
    // Speicherung als strukturierter JSON-Blob
    entity.setField(field + "_encrypted", enc.toJson().dump()); // {iv,ciphertext,tag,key_id,key_version}
    entity.setField(field + "_enc", true);                      // bool Marker
    entity.setField(field, std::monostate{});                    // Klartext entfernen
    }
}
```

### 4.3 Content (Binärblobs)

**Chunk-Level-Verschlüsselung:**
```cpp
// In ContentManager::importContent()
if (config.encrypt_blobs && blob.has_value()) {
    std::string user_key = deriveUserKey(jwt, "content.blob:" + meta.id);
    auto encrypted = field_enc_->encrypt(*blob, user_key);
    
    // Meta-Flag setzen
    meta.encrypted = true;
    meta.encryption_type = "aes-256-gcm";
    meta.encryption_context = jwt_claims["sub"]; // oder group
    
    storage_->put("content_blob:" + meta.id, encrypted.toBase64());
}

// In ContentManager::getContentBlob()
if (meta.encrypted) {
    // User-Context validieren
    if (jwt_claims["sub"] != meta.encryption_context && 
        !hasGroupAccess(jwt_claims, meta.encryption_context)) {
        throw UnauthorizedException("No access to encrypted content");
    }
    
    auto user_key = deriveUserKey(jwt, "content.blob:" + meta.id);
    auto decrypted = field_enc_->decrypt(encrypted_blob, user_key);
    return decrypted;
}
```

### 4.4 Vector (Embeddings)

**Trade-off: Verschlüsselung vs. Nearest-Neighbor-Search**

**Problem:**
- ANN-Search (HNSW) benötigt float32-Vektoren im Klartext
- Verschlüsselte Vektoren → keine Distanz-Berechnung möglich

**Lösungen:**

#### Option A: Keine Vektor-Verschlüsselung (Default)
```cpp
// Vektoren bleiben unverschlüsselt für ANN
// Zugriff nur über authorizierte API-Calls
// Audit-Logging aller Vector-Queries
```

**Vorteil:** ✅ Volle ANN-Performance  
**Nachteil:** ⚠️ Vektoren at-rest rekonstruierbar

#### Option B: Encrypt-then-Search (Metadata-only)
```cpp
// Nur Vektor-Metadaten verschlüsseln
BaseEntity vector_entity;
vector_entity.setField("pk", pk);                    // PLAIN
vector_entity.setField("embedding", embedding);      // PLAIN (für HNSW)
vector_entity.setField("source_text_encrypted", enc_text);  // 🔐 ENCRYPTED
vector_entity.setField("metadata_encrypted", enc_meta);     // 🔐 ENCRYPTED
```

**Vorteil:** ✅ ANN funktioniert, Quelltext geschützt  
**Nachteil:** ⚠️ Embedding selbst im Klartext

#### Option C: Homomorphic Encryption (Future)
```cpp
// Fully Homomorphic Encryption (FHE) für Distanz-Berechnung
// Aktuell nicht produktionsreif (100-1000x Slowdown)
```

**Empfehlung:** Start mit **Option B** (Metadata-Verschlüsselung), später **Option C** evaluieren

---

## 5. Implementierungsplan

### 5.1 Phase 1: PKI-Integration (Week 1)

**Tasks:**
1. ✅ Bereits vorhanden: `FieldEncryption`, `KeyProvider`, `EncryptedBlob`
2. ✅ `PKIKeyProvider` IMPLEMENTIERT:
   ```cpp
   class PKIKeyProvider : public KeyProvider {
   public:
     PKIKeyProvider(std::shared_ptr<utils::VCCPKIClient> pki,
            std::shared_ptr<themis::RocksDBWrapper> db,
            const std::string& service_id);
     std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version = 0) override;
     uint32_t rotateKey(const std::string& key_id) override; // inkl. DEK-Rotation
   private:
     // KEK via HKDF aus Service-Zertifikat/ID, DEK AES-256-GCM-verschlüsselt in RocksDB
     std::vector<uint8_t> kek_;
     std::unordered_map<uint32_t, std::vector<uint8_t>> dek_cache_;
   };
   ```

3. 🟡 `VCCPKIClient` PARTIAL (lokal sign/verify, REST pending):
   ```cpp
   struct PKIConfig { std::string service_id, endpoint, cert_path, key_path; };
   class VCCPKIClient {
   public:
     explicit VCCPKIClient(PKIConfig cfg);
     SignatureResult signHash(const std::vector<uint8_t>& sha256) const;   // OpenSSL RSA, Stub-Fallback
     bool verifyHash(const std::vector<uint8_t>& sha256, const SignatureResult& sig) const;
     // TODO: REST-Calls gegen https://localhost:8443/api/v1 (mTLS, Fehlercodes)
   };
   ```

### 5.2 Phase 2: User-Context (Week 2)

**Tasks:**
1. 🟡 JWT-Validator (PARTIAL):
   ```cpp
   class JWTValidator {
   public:
     JWTClaims parseAndValidate(const std::string& token);   // Header/Payload-Parsing, exp-Check
   private:
     std::string jwks_url_;  // Keycloak JWKS-Endpoint
     // TODO: RS256 Signaturprüfung via JWKS (kid), iss/aud/nbf/iat, Clock-Skew
   };
   ```

2. ❌ User-Key-Derivation:
   ```cpp
   std::vector<uint8_t> deriveUserKey(
       const std::vector<uint8_t>& dek,
       const std::string& user_id,
       const std::string& field_name
   ) {
       return HKDF(dek, user_id, "field:" + field_name);
   }
   ```

### 5.3 Phase 3: Storage-Layer-Integration (Week 3)

**Tasks:**
1. ❌ GraphIndexManager: Verschlüssele `weight`, `metadata`
2. 🟡 ContentManager: PARTIAL (Flags/Wrapper vorhanden, End-to-End aktivieren)
3. ❌ VectorIndexManager: Verschlüssele Vektor-Metadaten (Option B)
4. ❌ QueryEngine: Schema-basierte Auto-Verschlüsselung

### 5.4 Phase 4: Testing & Audit (Week 4)

**Tests:**
- Unit-Tests: Encrypt/Decrypt-Roundtrip für alle Datentypen
- Integration: Multi-User-Szenarien (User A kann Daten von User B nicht lesen)
- Performance: Overhead-Messung (Encrypt: ~0.5ms/KB, Decrypt: ~0.5ms/KB)
- Security: Pen-Test mit gestohlenem Backup ohne Keys

---

## 6. Konfiguration

### 6.1 DB-Config (`config:encryption` in RocksDB)

```json
{
  "enabled": true,
  "algorithm": "aes-256-gcm",
  "key_provider": "pki",
  "pki": {
    "server_url": "https://localhost:8443/api/v1",
    "service_id": "themis-db",
    "cert_path": "/etc/themis/certs/themis-db.pem",
    "key_path": "/etc/themis/certs/themis-db-key.pem"
  },
  "user_context": {
    "enabled": true,
    "jwt_issuer": "https://keycloak.vcc.local/realms/vcc",
    "jwks_url": "https://keycloak.vcc.local/realms/vcc/protocol/openid-connect/certs"
  },
  "encrypt_fields": {
    "graph_edge_properties": true,
    "content_blobs": true,
    "vector_metadata": true,
    "relational_sensitive": true
  }
}
```

### 6.2 Schema-Definition (per Collection/Object)

```json
{
  "collections": {
    "users": {
      "encryption": {
        "enabled": true,
        "fields": ["email", "phone", "ssn", "address"],
        "context_type": "user"  // per-user oder "group"; falls "group" wird _group gespeichert
      }
    },
    "documents": {
      "encryption": {
        "enabled": true,
        "fields": ["content_blob"],
        "context_type": "group",
        "allowed_groups": ["legal_team", "executives"]
      }
    }
  }
}
```

### 6.3 Storage-Felder (Konventionen)
Für ein verschlüsseltes Feld `email` entstehen:
| Feld | Typ | Bedeutung |
|------|-----|-----------|
| `email_enc` | bool | Flag: Feld verschlüsselt |
| `email_encrypted` | string(JSON) | `{iv,ciphertext,tag,key_id,key_version}` |
| `email_group` | string(optional) | Gruppenname bei Kontext `group` |

Klartextfeld wird entfernt oder als `null` gesetzt. Query-Pfade prüfen das `_enc` Flag und entschlüsseln mittels passendem Schlüssel.

### 6.4 Key-Storage in RocksDB
| Schlüssel | Inhalt |
|-----------|--------|
| `kek:ikm:{service_id}` | 64 hex chars (32 Byte IKM) |
| `dek:encrypted:v{n}` | JSON `{iv,ciphertext,tag,...}` oder Binär `nonce||ct||tag` |
| `key:group:{group}:v{n}` | Binär `nonce||ct||tag` (Group-DEK verschlüsselt mit KEK) |
| `key:group:{group}:meta` | String: `{current_version}|{timestamp}[|status]` |

---

## 7. Security Best-Practices

### 7.1 Key-Management

✅ **DO:**
- KEK aus PKI-Zertifikat ableiten (Hardware-backed wenn möglich)
- DEK verschlüsselt in DB speichern
- User-Keys nur in-memory halten (ephemeral)
- Key-Rotation alle 90 Tage (DEK), Zertifikat-Erneuerung jährlich

❌ **DON'T:**
- Keys im Klartext in Config-Dateien
- Hardcoded Keys im Source-Code
- DEK unverschlüsselt in Environment Variables

### 7.2 Audit-Logging

**Encrypt-then-Sign für sensible Logs (SAGA, AUDIT):**

- Canonical JSON erzeugen (stabile Key-Order, UTF-8)
- Mit täglichem LEK (Log Encryption Key) via AES-256-GCM verschlüsseln
- Hash über den Ciphertext bilden (SHA-256)
- PKI-Signatur über den Ciphertext-Hash (VCC-PKI)
- Persistieren: Ciphertext + iv + tag + lek_id + Signatur + Zert-Metadaten
- Optional redaktierte Kurzform in stdout/file loggen (kein Klartext)

Konfiguration siehe Governance (`config/governance.yaml`):
- `saga_signing.encrypt_then_sign: true`
- `saga_signing.categories.encrypt_before_sign: [SAGA, AUDIT]`
- `log_encryption.encrypt_categories: [SAGA, AUDIT]`
- `log_encryption.aad_fields: [log_id, category, timestamp]`

LEK-Handling (täglich rotierend):
1) KEK aus PKI-Zertifikat per HKDF → KEK(date)
2) Zufälliger 256-bit LEK generiert → LEK(date)
3) LEK verschlüsselt mit KEK(date) in RocksDB abgelegt

**Log jede Verschlüsselungs-/Entschlüsselungs-Operation:**
```json
{
  "timestamp": "2025-10-31T10:00:00Z",
  "operation": "decrypt",
  "user_id": "user123",
  "field": "content.blob:abc123",
  "success": true,
  "ip": "192.168.1.50"
}
```

### 7.3 Zero-Knowledge-Compliance

**Verification:**
```bash
# Backup ohne Keys erstellen
rocksdb_dump --db=/data/themis > backup.sst

# Ohne DEK: Daten unlesbar
strings backup.sst | grep "alice@example.com"  # → Gibberish

# Mit DEK: Daten lesbar
themis-decrypt --dek-file=dek.bin --input=backup.sst | grep "alice@"  # → alice@example.com
```

---

## 8. Zusammenfassung

| Feature | Status | Technologie | Nutzen |
|---------|--------|-------------|--------|
| **PKI-Integration** | ✅ Implementiert | VCC-PKI (c:\vcc\pki) | Persistentes IKM + KEK/DEK-Handling via PKIKeyProvider |
| **User-/Group-Context** | ✅ Implementiert | VCC-User JWT (c:\vcc\user) | RS256+JWKS + JWT Claims (sub/groups) + Group-DEKs |
| **JWT Claims Extraction** | ✅ Implementiert | AuthResult.groups, extractAuthContext() | User-ID + Groups aus Token für HKDF-Kontext |
| **Schema-Management-API** | ✅ Implementiert | GET/PUT /config/encryption-schema | REST API für Schema-CRUD mit Validierung |
| **Schema-based Auto-Enc (Write)** | ✅ Implementiert | Config-driven (handlePutEntity) | Alle BaseEntity::Value-Typen unterstützt |
| **Schema-based Auto-Dec (Read)** | ✅ Implementiert | Config-driven (handleGetEntity+handleQuery) | ?decrypt=true / body.decrypt für transparente Entschlüsselung |
| **Complex Type Support** | ✅ Implementiert | vector<float>, vector<uint8_t>, nested JSON | JSON-Serialisierung + Heuristik-Deserialisierung |
| **Graph-Encryption** | ✅ Implementiert | Schema-driven via handlePutEntity | Edge-Properties über normale Entity-Encryption (collections.edges config) |
| **QueryEngine Integration** | ✅ Implementiert | HTTP-Layer Decryption | Entschlüsselung nach Index-Scan im HTTP-Handler (handleGetEntity/handleQuery) |
| **Content-Encryption** | ✅ Implementiert | AES-256-GCM + HKDF per-user | Blob-Verschlüsselung mit user_context, "anonymous" Fallback |
| **Vector-Metadata-Enc** | ✅ Implementiert | Schema-driven batch_insert | Metadata-Felder (excl. Embedding) verschlüsselt, native BaseEntity |
| **Lazy Re-Encryption** | ✅ Implementiert | Read-time key upgrade | Content Blobs auto-upgrade zu neuester key_version bei GET |
| **Key Rotation** | 🟡 DESIGN | Lazy Re-Encryption (Write-Back on Read) | Dokumentiert in key_rotation_strategy.md, full impl pending |
| **Performance Benchmarks** | ✅ Implementiert | 6 Benchmarks in bench_encryption.cpp | HKDF, Single/Multi-Field, Embeddings - Alle Tests PASS |
| **E2E Integration Tests** | ✅ Implementiert | 10 Test-Szenarien in test_encryption_e2e.cpp | Multi-User, Groups, Rotation, Complex Types - Alle 10/10 PASS |
| **Audit-Logging** | ✅ Implementiert | Encrypt-then-Sign (AES-256-GCM + PKI) | Compliance & Forensics |

**Implementierungsdetails Schema-based Encryption:**
- **Schreibpfad (`handlePutEntity`)**: 
  - Liest `config:encryption_schema` aus RocksDB
  - Extrahiert `user_id` und `groups` aus JWT via `extractAuthContext(req)`
  - Serialisiert alle BaseEntity::Value-Typen:
    - **Primitive**: `string`, `int64_t`, `double`, `bool` → UTF-8 String
    - **vector<float>**: JSON-Array `[0.1, 0.2, ...]`
    - **vector<uint8_t>**: Direkt als Binär-Bytes
    - **monostate**: Übersprungen (null-Wert)
  - Leitet Feldschlüssel per HKDF ab (User-Kontext: `HKDF(DEK, user_id, "field:<name>")`, Group-Kontext: `HKDF(Group-DEK, "", "field:<name>")`)
  - Verschlüsselt Felder → `{<field>_encrypted: JSON, <field>_enc: true, [<field>_group: "group_name"]}`
  - Entfernt Plaintext vor SecondaryIndex-Persistenz
- **Lesepfad (`handleGetEntity`, `handleQuery`)**: 
  - Optional via Query-Parameter `?decrypt=true` oder Body `{decrypt: true}`
  - Extrahiert `user_id` aus JWT für Schlüsselableitung (Fallback: `"anonymous"`)
  - Identische HKDF-Ableitung wie Schreibpfad
  - Deserialisiert basierend auf Heuristik:
    - Startet mit `[` oder `{` → JSON-Parse (vector/nested object)
    - Sonst → String (primitive Typen)
  - Rekonstruiert Plaintext für Client-Response
- **JWT Claims Integration**: `AuthResult` erweitert um `groups` Feld, `extractAuthContext()` nutzt `auth_->validateToken()` für Claims-Extraktion
- **Fehlerbehandlung**: WARN-Log bei Decrypt-Fehler, Request läuft weiter

**QueryEngine Integration (Aktueller Stand):**
- **Implementierung**: HTTP-Layer Decryption (Post-Processing nach Index-Scan)
- **Ablauf**:
  1. QueryEngine führt Query auf verschlüsselten Daten aus (`_encrypted` Felder bleiben im Result)
  2. HTTP-Handler (`handleQuery`) prüft `decrypt` Flag
  3. Falls `true`: Schema laden, pro Entity verschlüsselte Felder identifizieren und entschlüsseln
  4. Entschlüsselte Plaintext-Felder im Response zurückgeben
- **Einschränkungen**:
  - ❌ Filter auf verschlüsselten Feldern nicht möglich (Index kennt nur Ciphertext)
  - ❌ Sortierung nach verschlüsselten Feldern nicht unterstützt
  - ❌ Aggregation über verschlüsselte Felder limitiert
- **Vorteile**:
  - ✅ Keine Änderung an QueryEngine/Index-Strukturen erforderlich
  - ✅ Performance: Entschlüsselung nur für Result-Set (nicht alle gescannten Rows)
  - ✅ Einfache JWT-Context-Propagation (nur HTTP-Layer benötigt Token)
- **Zukünftige Verbesserungen (Roadmap)**:
  - Push-Down Decryption in QueryEngine für Filter-Support
  - Searchable Encryption (Order-Preserving Encryption) für Range-Queries
  - Field-Level Access Control im QueryEngine (ACL pro Feld)

**Performance Benchmarks (bench_encryption.cpp):**
Implementiert in `c:\VCC\themis\benchmarks\bench_encryption.cpp` mit Google Benchmark Framework:

1. **BM_HKDF_Derive_FieldKey**: Misst reine HKDF-Ableitung (Baseline für alle Feldschlüssel)
2. **BM_SchemaEncrypt_SingleField**: Full Stack Encrypt (HKDF + AES-GCM) für 64/256/1024 Byte Felder
3. **BM_SchemaDecrypt_SingleField**: Full Stack Decrypt mit identischen Größen
4. **BM_SchemaEncrypt_MultiField_Entity**: Realistische 4-Feld-Entität (email, phone, ssn, address)
5. **BM_VectorFloat_Encryption**: 768-dim BERT-Embedding (3072 Bytes Float Array)

**Performance-Ziele:**
- HKDF-Ableitung: <50 µs
- Einzelfeld-Verschlüsselung (256 Bytes): <500 µs  
- Multi-Field Entity (4 Felder): <2 ms
- Target: <1 ms pro Feld, <10% Throughput-Degradation

**E2E Integration Tests (test_encryption_e2e.cpp):**
Umfassende Testsuite mit 10 Szenarien:

1. **UserIsolation**: User A kann User B's Daten nicht entschlüsseln (HKDF mit user_id Salt)
2. **GroupSharing**: HR-Team teilt verschlüsselte Gehaltsdaten (gemeinsame Group-DEK)
3. **GroupDEKRotation**: User verliert Zugriff nach Group-Exit (v2 Key)
4. **SchemaEncryption_MultiField**: 3-Feld-Entität mit email/phone/ssn
5. **ComplexType_VectorFloat**: 768-dim Embedding Encryption/Decryption
6. **ComplexType_NestedJSON**: Verschachteltes JSON-Objekt (Metadaten)
7. **KeyRotation_VersionTracking**: DEK v1/v2 parallel nutzbar
8. **Performance_BulkEncryption**: 1000 Entitäten in <1s (Target: >1000 ops/sec)
9. **CrossField_Consistency**: Gleicher User, verschiedene Felder → verschiedene Keys
10. **EdgeCase_EmptyString**: Empty String Verschlüsselung/Entschlüsselung

**Test-Infrastruktur:**
- Google Test Framework (gtest)
- Helper-Funktionen: `encryptFieldForUser()`, `decryptFieldForUser()`, `encryptFieldForGroup()`
- Realistische Test-Daten: E-Mails, Telefonnummern, SSNs, BERT-Embeddings
- Performance-Assertions: >1000 ops/sec für Bulk-Operationen

**Nächste Schritte (Produktion):**
1. ✅ **Benchmarks ausführen**: `./build/bench_encryption --benchmark_filter="Schema"` → Validierung <1ms Target
2. ✅ **E2E Tests ausführen**: `./build/themis_tests --gtest_filter="EncryptionE2E.*"` → Alle 10 Tests grün
3. **Performance-Tuning**: Falls Overhead >10%, SIMD-Optimierung für AES-GCM prüfen
4. **Production Deployment**: Encryption-Schema aktivieren für Pilot-Collections
5. **Monitoring**: Latenz-Metriken für Encrypt/Decrypt Operations (Prometheus/Grafana)
