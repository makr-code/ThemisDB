# LoRA S3 Storage Backend - Spezifikation

**Status**: 📋 Specification Only - No Implementation  
**Priorität**: P1 (High) - Für Cloud-Deployments  
**Geschätzter Aufwand**: 3-4 Arbeitstage

## Übersicht

Diese Spezifikation definiert die Anforderungen und das Design für den S3-basierten Storage Backend für LoRA-Adapter. Der S3-Backend ermöglicht die Speicherung von LoRA-Adaptern in AWS S3 oder S3-kompatiblen Objektspeichern (MinIO, Ceph, etc.).

## Motivation

**Warum S3 Backend?**

1. **Cloud-Native Deployments**: Nahtlose Integration mit AWS/Azure/GCP
2. **Skalierbarkeit**: Unbegrenzte Speicherkapazität für große Adapter
3. **Kosteneffizienz**: Pay-per-use Modell, keine lokale Storage-Verwaltung
4. **Multi-Region**: Globale Verfügbarkeit und Disaster Recovery
5. **Lifecycle Policies**: Automatische Archivierung und Kostenkontrolle

## Architektur

### Komponenten

```
┌─────────────────────────────────────────────────────────────┐
│                 LoRAStorageService                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ FileSystem   │  │  ThemisDB    │  │     S3       │     │
│  │   Backend    │  │   Backend    │  │   Backend    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│         │                  │                  │            │
└─────────┼──────────────────┼──────────────────┼────────────┘
          │                  │                  │
          ▼                  ▼                  ▼
    Local Disk         RocksDB +          S3 Bucket
                      BlobStore           (AWS/MinIO)
                                               │
                                               ├─ lora-adapters/
                                               │  ├─ {adapter_id}/
                                               │  │  ├─ weights.bin
                                               │  │  ├─ metadata.json
                                               │  │  └─ versions/
                                               │  │     ├─ v1.bin
                                               │  │     └─ v2.bin
```

### Datenspeicherung

**S3 Bucket Struktur**:
```
themisdb-lora-adapters/
├── adapters/
│   ├── {adapter_id}/
│   │   ├── current/
│   │   │   ├── weights.bin          # Aktuelle Gewichte (komprimiert + verschlüsselt)
│   │   │   └── metadata.json        # Adapter Metadaten
│   │   └── versions/
│   │       ├── v1/
│   │       │   ├── weights.bin
│   │       │   └── metadata.json
│   │       └── v2/
│   │           ├── weights.bin
│   │           └── metadata.json
└── metadata/
    └── adapters.db                   # Optionales Index-File
```

## API Spezifikation

### Konfiguration

```cpp
// S3 Backend Konfiguration
struct S3Config {
    // AWS Credentials
    std::string access_key_id;      // AWS_ACCESS_KEY_ID
    std::string secret_access_key;  // AWS_SECRET_ACCESS_KEY
    std::string session_token;      // Optional: AWS_SESSION_TOKEN für temporäre Credentials
    
    // S3 Endpoint
    std::string bucket_name;        // z.B. "themisdb-lora-adapters"
    std::string region;             // z.B. "us-east-1"
    std::string endpoint_url;       // Optional: Custom endpoint für MinIO/Ceph
    
    // Transfer Optionen
    bool use_multipart_upload;      // Multipart Upload für >5MB Dateien (default: true)
    size_t multipart_chunk_size;    // Chunk-Größe in MB (default: 10MB)
    int max_retries;                // Retry-Versuche (default: 3)
    int timeout_seconds;            // Request Timeout (default: 60)
    
    // Sicherheit
    bool enable_encryption;         // Server-side encryption (default: true)
    std::string encryption_type;    // "AES256" oder "aws:kms" (default: "AES256")
    std::string kms_key_id;         // Optional: KMS Key ID für aws:kms
    
    // Lifecycle
    bool enable_versioning;         // S3 Versioning (default: true)
    int max_versions;               // Max Versionen pro Adapter (default: 5)
    int archive_after_days;         // Tage bis zur Archivierung (default: 90)
    
    // Performance
    bool use_transfer_acceleration; // S3 Transfer Acceleration (default: false)
    bool use_path_style;            // Path-style URLs (MinIO compatibility, default: false)
};
```

### Implementierung

```cpp
class S3StorageBackend {
public:
    explicit S3StorageBackend(const S3Config& config);
    ~S3StorageBackend();
    
    /**
     * @brief Adapter in S3 speichern
     * 
     * Prozess:
     * 1. Gewichte serialisieren und komprimieren
     * 2. Optional verschlüsseln (client-side)
     * 3. Multipart Upload wenn >5MB
     * 4. Metadata als separate JSON-Datei
     * 5. S3 Tags setzen für Lifecycle
     * 
     * @param adapter_id Eindeutige Adapter-ID
     * @param weights Adapter-Gewichte
     * @param metadata Adapter-Metadaten
     * @return true bei Erfolg
     */
    bool saveAdapter(
        const std::string& adapter_id,
        const AdapterWeights& weights,
        const AdapterMetadata& metadata
    );
    
    /**
     * @brief Adapter aus S3 laden
     * 
     * Prozess:
     * 1. Metadata aus S3 lesen
     * 2. Gewichte herunterladen (mit Range-Requests für große Dateien)
     * 3. Optional entschlüsseln
     * 4. Dekomprimieren
     * 5. Validierung (Checksum)
     * 
     * @param adapter_id Adapter-ID
     * @return Optional mit Gewichten oder nullopt
     */
    std::optional<AdapterWeights> loadAdapter(const std::string& adapter_id);
    
    /**
     * @brief Adapter-Metadaten laden (ohne Gewichte)
     * 
     * @param adapter_id Adapter-ID
     * @return Optional mit Metadaten
     */
    std::optional<AdapterMetadata> loadMetadata(const std::string& adapter_id);
    
    /**
     * @brief Adapter aus S3 löschen
     * 
     * Prozess:
     * 1. Metadata-Datei löschen
     * 2. Gewichte-Datei löschen
     * 3. Optional: Alle Versionen löschen
     * 4. S3 Delete Marker setzen (bei Versioning)
     * 
     * @param adapter_id Adapter-ID
     * @return true bei Erfolg (idempotent)
     */
    bool deleteAdapter(const std::string& adapter_id);
    
    /**
     * @brief Prüfen ob Adapter existiert
     * 
     * @param adapter_id Adapter-ID
     * @return true wenn vorhanden
     */
    bool exists(const std::string& adapter_id);
    
    /**
     * @brief Alle Adapter-IDs auflisten
     * 
     * @return Vector mit Adapter-IDs
     */
    std::vector<std::string> listAdapters();
    
    /**
     * @brief Version erstellen
     * 
     * @param adapter_id Adapter-ID
     * @return Version-String (z.B. "v3")
     */
    std::string createVersion(const std::string& adapter_id);
    
    /**
     * @brief Zu Version zurückrollen
     * 
     * @param adapter_id Adapter-ID
     * @param version Version-String
     * @return true bei Erfolg
     */
    bool rollbackToVersion(const std::string& adapter_id, const std::string& version);
    
    /**
     * @brief Versionen auflisten
     * 
     * @param adapter_id Adapter-ID
     * @return Vector mit Version-Strings
     */
    std::vector<std::string> listVersions(const std::string& adapter_id);
    
    /**
     * @brief Pre-Signed URL für direkten Download
     * 
     * Ermöglicht zeitlich begrenzten direkten Zugriff ohne AWS Credentials.
     * Nützlich für Web-UIs und externe Tools.
     * 
     * @param adapter_id Adapter-ID
     * @param expiration_seconds Gültigkeit in Sekunden (default: 3600 = 1h)
     * @return Pre-Signed URL
     */
    std::string generatePresignedUrl(
        const std::string& adapter_id,
        int expiration_seconds = 3600
    );
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

## Abhängigkeiten

### AWS SDK für C++

**Installation via vcpkg**:
```json
{
  "dependencies": [
    {
      "name": "aws-sdk-cpp",
      "features": ["s3", "transfer"]
    }
  ]
}
```

**CMakeLists.txt**:
```cmake
find_package(AWSSDK REQUIRED COMPONENTS s3 transfer-manager)
target_link_libraries(themis_core PRIVATE 
    AWS::aws-sdk-cpp-s3 
    AWS::aws-sdk-cpp-transfer-manager
)
```

## Funktionale Anforderungen

### 1. Multipart Upload (FR-S3-001)

**Anforderung**: Unterstützung für Multipart Upload bei Dateien >5MB

**Begründung**: 
- AWS S3 limitiert PUT-Requests auf 5GB
- Bessere Performance bei großen Dateien
- Automatic retry für fehlgeschlagene Parts

**Implementation**:
```cpp
// Pseudo-Code
if (data_size > 5 * 1024 * 1024) {  // >5MB
    // Initiate multipart upload
    auto upload_id = s3_client->CreateMultipartUpload(bucket, key);
    
    // Upload parts (10MB chunks)
    std::vector<CompletedPart> parts;
    for (size_t offset = 0; offset < data_size; offset += chunk_size) {
        auto part = s3_client->UploadPart(
            bucket, key, upload_id, 
            part_number++, 
            data.substr(offset, chunk_size)
        );
        parts.push_back(part);
    }
    
    // Complete upload
    s3_client->CompleteMultipartUpload(bucket, key, upload_id, parts);
}
```

**Performance-Ziele**:
- Upload-Geschwindigkeit: >100 MB/s (bei guter Netzwerkverbindung)
- Chunk-Größe: Konfigurierbar (default 10MB)
- Parallelität: Bis zu 10 parallele Parts

### 2. Pre-Signed URLs (FR-S3-002)

**Anforderung**: Generierung von zeitlich begrenzten Download-URLs

**Use Cases**:
- Web-UI: Direkter Download ohne Backend-Proxy
- ML-Training: Externe Trainer können Adapter direkt laden
- APIs: Sichere Weitergabe ohne Credentials

**Sicherheit**:
- Maximale Gültigkeit: 7 Tage (konfigurierbar)
- IP-Whitelist optional
- Audit-Logging aller generierten URLs

### 3. Server-Side Encryption (FR-S3-003)

**Anforderung**: Verschlüsselung der Daten in S3

**Optionen**:
1. **SSE-S3** (Standard): AWS-managed keys (AES-256)
2. **SSE-KMS**: AWS KMS für Key Management und Rotation
3. **SSE-C**: Customer-provided keys (für vollständige Kontrolle)

**Empfehlung**: SSE-KMS für Produktion (auditierbar, Key-Rotation)

### 4. Versioning & Lifecycle (FR-S3-004)

**Anforderung**: Automatisches Versionsmanagement

**S3 Versioning**:
- Aktiviert via S3 Bucket Policy
- Jede Überschreibung erstellt neue Version
- Alte Versionen können wiederhergestellt werden

**Lifecycle Policies**:
```json
{
  "Rules": [
    {
      "Id": "ArchiveOldVersions",
      "Status": "Enabled",
      "Transitions": [
        {
          "Days": 90,
          "StorageClass": "GLACIER"
        }
      ],
      "NoncurrentVersionExpiration": {
        "NoncurrentDays": 180
      }
    }
  ]
}
```

### 5. Retry & Error Handling (FR-S3-005)

**Anforderung**: Robuste Fehlerbehandlung und Retry-Logik

**Retry-Strategie**:
- Maximale Retries: 3 (konfigurierbar)
- Exponential Backoff: 200ms, 400ms, 800ms
- Retry bei: 500, 502, 503, 504, ConnectionTimeout
- Keine Retry bei: 400, 403, 404

**Error Codes**:
```cpp
enum class S3Error {
    SUCCESS,
    NETWORK_ERROR,       // Connection failed, timeout
    AUTH_ERROR,          // 403 Forbidden, invalid credentials
    NOT_FOUND,           // 404 Not Found
    BUCKET_NOT_FOUND,    // Bucket doesn't exist
    RATE_LIMITED,        // 503 Slow Down
    INSUFFICIENT_STORAGE // 507 Insufficient Storage
};
```

## Nicht-Funktionale Anforderungen

### Performance (NFR-PERF-001)

**Latenz-Ziele**:
- Upload (<10MB): <3 Sekunden
- Upload (10-100MB): <15 Sekunden
- Download (<10MB): <2 Sekunden
- Download (10-100MB): <10 Sekunden
- Metadata-Abruf: <500ms

**Durchsatz**:
- Mindestens 50 MB/s Upload
- Mindestens 100 MB/s Download
- Parallelisierung: Bis zu 10 gleichzeitige Uploads

### Sicherheit (NFR-SEC-001)

**Authentifizierung**:
- AWS IAM Roles (bevorzugt in Production)
- Temporäre Credentials via STS
- Access Keys nur in Development

**Verschlüsselung**:
- TLS 1.3 für alle Verbindungen
- Server-Side Encryption (SSE-S3 oder SSE-KMS)
- Optional: Client-Side Encryption

**Audit**:
- Logging aller S3-Operationen
- CloudTrail Integration
- S3 Access Logs

### Zuverlässigkeit (NFR-REL-001)

**Verfügbarkeit**:
- S3 Standard: 99.99% Availability
- Multi-Region Backup optional
- Automatic Failover zu anderen Backends

**Datenintegrität**:
- MD5 Checksums für alle Uploads
- ETag-Validierung bei Downloads
- Corruption Detection

### Kosten (NFR-COST-001)

**Kostenoptimierung**:
- Lifecycle Policies für Archivierung
- S3 Intelligent-Tiering für variable Zugriffsmuster
- Kompression vor Upload (reduziert Storage-Kosten)

**Geschätzte Kosten** (AWS S3 us-east-1):
- Storage: $0.023 per GB/Monat
- PUT Requests: $0.005 per 1000 Requests
- GET Requests: $0.0004 per 1000 Requests
- Transfer Out: $0.09 per GB (erste 10TB)

**Beispiel**: 1000 Adapter à 50MB:
- Storage: 50GB × $0.023 = $1.15/Monat
- Uploads: 1000 × $0.005/1000 = $0.005
- Downloads (10x/Monat): 10,000 × $0.0004/1000 = $0.004

**Gesamt**: ~$1.16/Monat für 1000 Adapter

## Teststrategies

### Unit Tests

```cpp
TEST(S3StorageBackend, SaveAndLoadAdapter) {
    // Mock S3 Client
    auto mock_s3 = std::make_shared<MockS3Client>();
    S3Config config;
    config.bucket_name = "test-bucket";
    
    S3StorageBackend backend(config, mock_s3);
    
    // Create test adapter
    AdapterWeights weights = createTestWeights(1024 * 1024); // 1MB
    AdapterMetadata metadata = createTestMetadata();
    
    // Save
    EXPECT_TRUE(backend.saveAdapter("test-adapter", weights, metadata));
    
    // Verify S3 calls
    EXPECT_EQ(mock_s3->put_calls.size(), 2); // weights + metadata
    
    // Load
    auto loaded = backend.loadAdapter("test-adapter");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data, weights.data);
}

TEST(S3StorageBackend, MultipartUpload) {
    auto mock_s3 = std::make_shared<MockS3Client>();
    S3Config config;
    config.use_multipart_upload = true;
    config.multipart_chunk_size = 10; // 10MB
    
    S3StorageBackend backend(config, mock_s3);
    
    // Create 50MB adapter
    AdapterWeights weights = createTestWeights(50 * 1024 * 1024);
    
    // Save
    EXPECT_TRUE(backend.saveAdapter("large-adapter", weights, {}));
    
    // Verify multipart upload
    EXPECT_TRUE(mock_s3->initiated_multipart);
    EXPECT_EQ(mock_s3->uploaded_parts.size(), 5); // 50MB / 10MB
    EXPECT_TRUE(mock_s3->completed_multipart);
}

TEST(S3StorageBackend, PresignedUrl) {
    S3StorageBackend backend(createTestConfig());
    
    auto url = backend.generatePresignedUrl("test-adapter", 3600);
    
    EXPECT_TRUE(url.starts_with("https://"));
    EXPECT_TRUE(url.contains("X-Amz-Signature"));
    EXPECT_TRUE(url.contains("X-Amz-Expires=3600"));
}
```

### Integration Tests

```cpp
TEST(S3Integration, RealS3Upload) {
    // Requires AWS credentials in environment
    if (!std::getenv("AWS_ACCESS_KEY_ID")) {
        GTEST_SKIP() << "AWS credentials not configured";
    }
    
    S3Config config;
    config.bucket_name = "themisdb-test-bucket";
    config.region = "us-east-1";
    
    S3StorageBackend backend(config);
    
    AdapterWeights weights = createTestWeights(1024 * 1024);
    AdapterMetadata metadata;
    metadata.adapter_id = "integration-test-" + generateUUID();
    
    // Save
    EXPECT_TRUE(backend.saveAdapter(metadata.adapter_id, weights, metadata));
    
    // Load
    auto loaded = backend.loadAdapter(metadata.adapter_id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data, weights.data);
    
    // Delete
    EXPECT_TRUE(backend.deleteAdapter(metadata.adapter_id));
    EXPECT_FALSE(backend.exists(metadata.adapter_id));
}
```

### Performance Tests

```cpp
TEST(S3Performance, UploadLatency) {
    S3StorageBackend backend(createTestConfig());
    
    // Test verschiedene Größen
    std::vector<size_t> sizes = {1, 10, 50, 100}; // MB
    
    for (auto size_mb : sizes) {
        auto weights = createTestWeights(size_mb * 1024 * 1024);
        
        auto start = std::chrono::high_resolution_clock::now();
        backend.saveAdapter("perf-test", weights, {});
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        spdlog::info("Upload {}MB: {}ms", size_mb, duration.count());
        
        // Performance targets
        if (size_mb <= 10) {
            EXPECT_LT(duration.count(), 3000); // <3s for <10MB
        } else {
            EXPECT_LT(duration.count(), 15000); // <15s for <100MB
        }
        
        backend.deleteAdapter("perf-test");
    }
}
```

## Kompatibilität

### S3-Kompatible Storage

Der S3-Backend muss mit folgenden Systemen kompatibel sein:

1. **AWS S3**
   - Standard S3 API
   - S3 Transfer Acceleration
   - S3 Versioning
   - S3 Lifecycle Policies

2. **MinIO**
   - Self-hosted S3-kompatibler Storage
   - Path-style URLs erforderlich
   - Keine Transfer Acceleration

3. **Ceph RADOS Gateway**
   - S3 API via RGW
   - Eingeschränktes Multipart Upload
   - Kein KMS Support

4. **Azure Blob Storage**
   - S3 API via Kompatibilitäts-Layer
   - Unterschiedliche Authentifizierung

**Konfiguration für MinIO**:
```cpp
S3Config config;
config.endpoint_url = "http://localhost:9000";
config.use_path_style = true;  // WICHTIG für MinIO
config.verify_ssl = false;      // Bei self-signed certs
```

## Migration von anderen Backends

### Von FileSystem zu S3

```cpp
// Migration Script Pseudo-Code
void migrateFromFilesystemToS3(
    const std::string& filesystem_path,
    S3StorageBackend& s3_backend
) {
    auto filesystem_backend = LoRAStorageService::create(
        LoRAStorageService::Backend::FileSystem,
        {.filesystem_path = filesystem_path}
    );
    
    // Liste alle Adapter
    auto adapters = filesystem_backend.listAdapters();
    
    for (const auto& adapter_id : adapters) {
        spdlog::info("Migrating adapter: {}", adapter_id);
        
        // Load from filesystem
        auto weights = filesystem_backend.loadAdapter(adapter_id);
        auto metadata = filesystem_backend.loadMetadata(adapter_id);
        
        if (weights && metadata) {
            // Save to S3
            if (s3_backend.saveAdapter(adapter_id, *weights, *metadata)) {
                spdlog::info("  ✓ Migrated successfully");
                
                // Optional: Delete from filesystem after successful upload
                // filesystem_backend.deleteAdapter(adapter_id);
            } else {
                spdlog::error("  ✗ Migration failed");
            }
        }
    }
}
```

### Von ThemisDB zu S3

```cpp
void migrateFromThemisDBToS3(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<BlobStorageManager> blob_manager,
    S3StorageBackend& s3_backend
) {
    auto themisdb_backend = LoRAStorageService::create(
        LoRAStorageService::Backend::ThemisDB,
        {.db = db, .blob_manager = blob_manager}
    );
    
    auto adapters = themisdb_backend.listAdapters();
    
    for (const auto& adapter_id : adapters) {
        auto weights = themisdb_backend.loadAdapter(adapter_id);
        auto metadata = themisdb_backend.loadMetadata(adapter_id);
        
        if (weights && metadata) {
            s3_backend.saveAdapter(adapter_id, *weights, *metadata);
        }
    }
}
```

## Monitoring & Observability

### Metriken

**CloudWatch Metriken** (bei AWS S3):
- `s3_put_requests` - Anzahl PUT Requests
- `s3_get_requests` - Anzahl GET Requests
- `s3_delete_requests` - Anzahl DELETE Requests
- `s3_bytes_uploaded` - Hochgeladene Bytes
- `s3_bytes_downloaded` - Heruntergeladene Bytes
- `s3_4xx_errors` - Client-Fehler (Auth, NotFound, etc.)
- `s3_5xx_errors` - Server-Fehler
- `s3_upload_latency_ms` - Upload-Latenz
- `s3_download_latency_ms` - Download-Latenz

### Logging

```cpp
// Beispiel Log-Ausgaben
spdlog::info("S3: Uploading adapter {} ({} MB)", adapter_id, size_mb);
spdlog::debug("S3: Using multipart upload with {} parts", num_parts);
spdlog::info("S3: Upload completed in {}ms", duration_ms);
spdlog::error("S3: Upload failed after {} retries: {}", max_retries, error_msg);
```

### Health Checks

```cpp
struct S3HealthStatus {
    bool connected;              // Kann S3 erreicht werden?
    bool bucket_accessible;      // Bucket existiert und ist zugreifbar?
    bool can_write;              // Schreibrechte vorhanden?
    bool can_read;               // Leserechte vorhanden?
    int latency_ms;              // Latenz zu S3
    std::string error_message;   // Fehler falls nicht healthy
};

S3HealthStatus checkS3Health() {
    // 1. Test Bucket-Zugriff (HEAD Bucket)
    // 2. Test Schreibrechte (PUT kleines Test-Objekt)
    // 3. Test Leserechte (GET Test-Objekt)
    // 4. Latenz messen
    // 5. Aufräumen (DELETE Test-Objekt)
}
```

## Sicherheitsempfehlungen

### 1. IAM Permissions (Principle of Least Privilege)

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "LoRAAdapterStorage",
      "Effect": "Allow",
      "Action": [
        "s3:PutObject",
        "s3:GetObject",
        "s3:DeleteObject",
        "s3:ListBucket",
        "s3:GetObjectVersion",
        "s3:ListBucketVersions"
      ],
      "Resource": [
        "arn:aws:s3:::themisdb-lora-adapters",
        "arn:aws:s3:::themisdb-lora-adapters/*"
      ]
    },
    {
      "Sid": "KMSEncryption",
      "Effect": "Allow",
      "Action": [
        "kms:Decrypt",
        "kms:Encrypt",
        "kms:GenerateDataKey"
      ],
      "Resource": "arn:aws:kms:us-east-1:123456789:key/abc-123"
    }
  ]
}
```

### 2. Bucket Policy

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "EnforceSSL",
      "Effect": "Deny",
      "Principal": "*",
      "Action": "s3:*",
      "Resource": "arn:aws:s3:::themisdb-lora-adapters/*",
      "Condition": {
        "Bool": {
          "aws:SecureTransport": "false"
        }
      }
    },
    {
      "Sid": "RequireEncryption",
      "Effect": "Deny",
      "Principal": "*",
      "Action": "s3:PutObject",
      "Resource": "arn:aws:s3:::themisdb-lora-adapters/*",
      "Condition": {
        "StringNotEquals": {
          "s3:x-amz-server-side-encryption": ["AES256", "aws:kms"]
        }
      }
    }
  ]
}
```

### 3. VPC Endpoint (für private Netzwerke)

```
S3 VPC Endpoint → Kein Internet-Zugriff erforderlich
- Reduziert Kosten (kein NAT Gateway)
- Erhöht Sicherheit (Traffic bleibt in AWS)
- Bessere Performance
```

## Implementierungsplan

### Phase 1: Grundlegende S3-Integration (2 Tage)
- [ ] S3Config Struktur
- [ ] AWS SDK Integration
- [ ] Basis CRUD-Operationen (save/load/delete)
- [ ] Unit Tests mit Mock S3 Client

### Phase 2: Erweiterte Features (1 Tag)
- [ ] Multipart Upload für große Dateien
- [ ] Pre-Signed URLs
- [ ] Versioning Support
- [ ] Retry-Logik und Error Handling

### Phase 3: Testing & Validierung (1 Tag)
- [ ] Integration Tests mit echtem S3
- [ ] Performance Tests
- [ ] Migration Scripts
- [ ] Dokumentation

### Phase 4: Production Readiness (0.5 Tage)
- [ ] Monitoring & Logging
- [ ] Health Checks
- [ ] Security Audit
- [ ] Deployment Guide

**Gesamt**: ~4.5 Tage

## Offene Fragen

1. **Multi-Region Support**: Soll Cross-Region Replication unterstützt werden?
2. **Compression**: Soll Kompression vor Upload durchgeführt werden (zusätzlich zu S3's eigener Kompression)?
3. **CDN Integration**: CloudFront für schnellere Downloads?
4. **Kostenbudget**: Gibt es ein monatliches Kostenbudget für S3?

## Referenzen

- **AWS S3 API**: https://docs.aws.amazon.com/s3/
- **AWS SDK C++**: https://github.com/aws/aws-sdk-cpp
- **MinIO**: https://min.io/
- **S3 Best Practices**: https://docs.aws.amazon.com/AmazonS3/latest/userguide/security-best-practices.html

---

**Erstellt**: 15. Januar 2025  
**Autor**: ThemisDB Team  
**Status**: 📋 Specification - Awaiting Implementation  
**Version**: 1.0
