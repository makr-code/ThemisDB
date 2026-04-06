# Vector Encryption Quick Start Guide

**Version:** 2.0 (Phase 1 + Phase 2)  
**Date:** December 15, 2025  
**Status:** Production Ready


## 📑 Table of Contents

- [TL;DR](#tldr---get-started-in-5-minutes)
- [Quick Reference](#quick-reference)
- [Common Scenarios](#common-scenarios)
- [Security Checklist](#security-checklist)

## TL;DR - Get Started in 5 Minutes

### 1. Enable Vector Encryption (Phase 1)

```cpp
// Initialize encryption
auto key_provider = std::make_shared<KeyProvider>();
auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);

// Enable encryption
VectorIndexManager vim(db);
vim.init("documents", 768);
vim.setVectorEncryptionEnabled(true);

// Add vectors - automatically encrypted!
BaseEntity entity("doc1");
entity.setField("embedding", std::vector<float>(768, 0.5f));
vim.addEntity(entity);
```

### 2. Enable HNSW Index Encryption (Phase 2)

```cpp
// Initialize HNSW encryption
EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption);

// Enable HNSW encryption
vim.setHnswEncryptionEnabled(true);

// Save - automatically encrypted!
vim.saveIndex("./data/hnsw_chunks");
```

### 3. Verify Encryption

```bash
# No plaintext files should exist
ls -la ./data/hnsw_chunks/
# Should see: index.bin.encrypted (NOT index.bin)

# Check database
# Should have "embedding_encrypted" field (NOT "embedding")
```

---

## Quick Reference

### Configuration

```cpp
// Phase 1: Vector Encryption
bool isEnabled = vim.isVectorEncryptionEnabled();
vim.setVectorEncryptionEnabled(true);
vim.setVectorKeyId("vector_embeddings");

// Phase 2: HNSW Index Encryption
bool isHnswEnabled = vim.isHnswEncryptionEnabled();
vim.setHnswEncryptionEnabled(true);
vim.setHnswKeyId("hnsw_index");
```

### Storage Format

**Encrypted Vector:**
```json
{
  "embedding_encrypted": "vector_embeddings:1:YWJj...:SGVs...:MTIz..."
}
```

**Encrypted HNSW Index:**
```
data/hnsw_chunks/
  ├─ index.bin.encrypted   ← Encrypted HNSW index
  ├─ meta.txt              ← Contains "encrypted" flag
  └─ labels.txt            ← PK mapping (not sensitive)
```

### Migration

```bash
# Dry run (no changes)
./migrate_vector_encryption \
  --db-path /var/lib/themisdb/data \
  --object-name documents \
  --dry-run

# Actual migration
./migrate_vector_encryption \
  --db-path /var/lib/themisdb/data \
  --object-name documents
```

---

## Common Scenarios

### Scenario 1: New Database with Full Encryption

```cpp
// Setup
auto db = std::make_unique<RocksDBWrapper>("/data/themisdb");
VectorIndexManager vim(*db);
vim.init("documents", 768);

// Enable both phases
vim.setVectorEncryptionEnabled(true);
vim.setHnswEncryptionEnabled(true);

// Use normally
BaseEntity doc("doc1");
doc.setField("embedding", vector);
vim.addEntity(doc);
vim.saveIndex("./hnsw");

// Result: 100% encrypted
```

### Scenario 2: Existing Database - Gradual Migration

```cpp
// Step 1: Enable encryption (new data only)
vim.setVectorEncryptionEnabled(true);

// Step 2: New vectors are encrypted automatically
vim.addEntity(newDocument);

// Step 3: Migrate old vectors (offline)
// Run: ./migrate_vector_encryption --db-path /data --object-name documents

// Step 4: Enable HNSW encryption
vim.setHnswEncryptionEnabled(true);
vim.saveIndex("./hnsw");
```

### Scenario 3: Auto-Save on Shutdown

```cpp
VectorIndexManager vim(*db);
vim.init("documents", 768);

// Enable encryption
vim.setVectorEncryptionEnabled(true);
vim.setHnswEncryptionEnabled(true);

// Configure auto-save
vim.setAutoSavePath("./hnsw", true);

// On shutdown, index is automatically saved (encrypted)
vim.shutdown();
```

### Scenario 4: Backward Compatibility

```cpp
// Load existing plaintext index
VectorIndexManager vim(*db);
vim.init("documents", 768);
vim.loadIndex("./hnsw");  // Works even if plaintext

// Enable encryption for future saves
vim.setHnswEncryptionEnabled(true);
vim.saveIndex("./hnsw");  // Now encrypted

// Old plaintext index is replaced with encrypted version
```

---

## Security Checklist

### Before Deployment

- [ ] Initialize FieldEncryption with KeyProvider
- [ ] Call setFieldEncryption() for both templates:
  - [ ] `EncryptedField<std::vector<float>>`
  - [ ] `EncryptedField<std::vector<uint8_t>>`
- [ ] Enable vector encryption: `setVectorEncryptionEnabled(true)`
- [ ] Enable HNSW encryption: `setHnswEncryptionEnabled(true)`

### After Deployment

- [ ] Verify no `index.bin` files (only `index.bin.encrypted`)
- [ ] Verify vectors have `embedding_encrypted` field
- [ ] Test search functionality
- [ ] Monitor encryption logs
- [ ] Verify backups are encrypted

---

## Performance Tips

### Optimize Index Load Time

```cpp
// Current: Sequential decryption
// For large indexes (>1 GB), consider:
// 1. Use SSD storage
// 2. Enable AES-NI hardware acceleration
// 3. See PERFORMANCE_OPTIMIZATION_NOTES.md for future optimizations
```

### Monitor Performance

```cpp
// Log encryption operations
THEMIS_INFO("Vector encryption: {}", enabled ? "ENABLED" : "DISABLED");
THEMIS_DEBUG("Encrypted vector for pk={}", pk);
THEMIS_INFO("HNSW index encrypted and saved to {}", directory);
```

---

## Troubleshooting

### Problem: "FieldEncryption not set"

**Cause:** Forgot to initialize encryption

**Solution:**
```cpp
auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);
EncryptedField<std::vector<uint8_t>>::setFieldEncryption(field_encryption);
```

### Problem: "index.bin.encrypted nicht gefunden"

**Cause:** Trying to load encrypted index but file doesn't exist

**Solution:**
```cpp
// Check if encryption is enabled
if (!vim.isHnswEncryptionEnabled()) {
    // Load plaintext instead
    vim.loadIndex("./hnsw");
}
```

### Problem: Search returns no results

**Cause:** Vectors not properly decrypted during index rebuild

**Solution:**
```cpp
// Rebuild from storage
vim.rebuildFromStorage();

// Verify encryption is configured
EXPECT_TRUE(vim.isVectorEncryptionEnabled());
```

---

## Code Examples

### Full Example

See `examples/example_vector_encryption.cpp` for complete working examples:

1. Basic vector encryption
2. HNSW index encryption
3. Full encryption (both phases)
4. Migration workflow
5. Auto-save configuration

### Integration Tests

See `tests/test_vector_encryption_integration.cpp` for:

- Phase 1 only tests
- Phase 2 only tests
- Full encryption tests
- Backward compatibility tests
- Performance benchmarks
- Error handling tests

---

## Next Steps

### For Development

1. **Run Tests:**
   ```bash
   cmake --build build
   cd build && ctest -R vector_encryption
   ```

2. **Run Examples:**
   ```bash
   ./example_vector_encryption
   ```

3. **Benchmarks:**
   ```bash
   ./bench_vector_encryption
   ```

### For Production

1. **Review Documentation:**
   - `VECTOR_ENCRYPTION_CONFIGURATION.md`
   - `HNSW_ENCRYPTION_CONFIGURATION.md`
   - `PHASE1_FINAL_REPORT.md`
   - `PHASE2_IMPLEMENTATION_REPORT.md`

2. **Plan Migration:**
   - Backup database
   - Run dry-run migration
   - Schedule downtime
   - Execute migration
   - Verify results

3. **Monitor:**
   - Encryption logs
   - Performance metrics
   - Storage usage
   - Error rates

---

## API Reference

### VectorIndexManager

```cpp
// Phase 1: Vector Encryption
void setVectorEncryptionEnabled(bool enabled);
bool isVectorEncryptionEnabled() const;
void setVectorKeyId(const std::string& keyId);
std::string getVectorKeyId() const;

// Phase 2: HNSW Index Encryption
void setHnswEncryptionEnabled(bool enabled);
bool isHnswEncryptionEnabled() const;
void setHnswKeyId(const std::string& keyId);
std::string getHnswKeyId() const;

// Persistence
Status saveIndex(const std::string& directory) const;
Status loadIndex(const std::string& directory);
void setAutoSavePath(const std::string& path, bool autoSave = true);
Status shutdown();

// CRUD
Status addEntity(const BaseEntity& e, std::string_view vectorField = "embedding");
Status rebuildFromStorage();
std::pair<Status, std::vector<Result>> searchKnn(const std::vector<float>& query, size_t k);
```

---

## Resources

**Documentation:**
- User Guide: `docs/security/VECTOR_ENCRYPTION_CONFIGURATION.md`
- HNSW Guide: `docs/security/HNSW_ENCRYPTION_CONFIGURATION.md`
- Performance Notes: `docs/security/PERFORMANCE_OPTIMIZATION_NOTES.md`

**Code:**
- Integration Tests: `tests/test_vector_encryption_integration.cpp`
- Examples: `examples/example_vector_encryption.cpp`
- Migration Tool: `tools/migrate_vector_encryption.cpp`

**Reports:**
- Phase 1 Report: `docs/security/PHASE1_FINAL_REPORT.md`
- Phase 2 Report: `docs/security/PHASE2_IMPLEMENTATION_REPORT.md`

---

## Support

**Issues:** https://github.com/makr-code/ThemisDB/issues  
**Security:** See `docs/security/README.md`  
**Performance:** See `PERFORMANCE_OPTIMIZATION_NOTES.md`

---

**Last Updated:** April 2026  
**Version:** 2.0  
**Status:** Production Ready ✅
