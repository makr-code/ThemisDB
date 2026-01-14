# BaseEntity Principle in ThemisDB LoRA Framework

## 📖 Understanding BaseEntity

### Core Principle
**BaseEntity is THE canonical storage unit for ALL data in ThemisDB**

```cpp
/// Base Entity: The canonical storage unit for all data models
/// Each logical entity (row, document, node, edge, vector object) is stored as one blob
```

### Key Characteristics

1. **Universal Storage Unit**: Everything in ThemisDB is stored as BaseEntity
   - Documents
   - Graph nodes/edges
   - Vector embeddings
   - Process definitions
   - **LoRA adapters** ✅

2. **Schema-less**: Flexible field map
   ```cpp
   using FieldMap = std::map<std::string, Value>;
   ```

3. **Multi-format Support**:
   - Binary (fast, compact)
   - JSON (human-readable)

4. **Lazy Parsing**: Fields extracted on-demand for performance

## 🎯 How LoRA Framework Follows BaseEntity Principle

### Current Implementation ✅

Our `lora_storage_service_themisdb.cpp` correctly uses BaseEntity:

```cpp
// 1. CREATE: Save adapter as BaseEntity
BaseEntity::FieldMap fields;
fields["adapter_id"] = Value(adapter_id);
fields["version"] = Value(metadata.version);
fields["base_model"] = Value(metadata.base_model);
fields["description"] = Value(metadata.description);
fields["training_samples"] = Value(static_cast<int64_t>(metadata.training_samples));
fields["validation_accuracy"] = Value(static_cast<double>(metadata.validation_accuracy));

BaseEntity entity = BaseEntity::fromFields(adapter_id, fields);
```

```cpp
// 2. Store large weights in BlobStorage
if (config_.blob_manager && weights.data.size() > 1024 * 1024) {
    auto blob_ref = config_.blob_manager->put(adapter_id, weights.data);
    fields["blob_ref_type"] = Value(static_cast<int64_t>(static_cast<int>(blob_ref.type)));
    fields["blob_ref_path"] = Value(blob_ref.path);
} else {
    // Small adapters inline
    fields["weights_data"] = Value(weights.data);
}
```

```cpp
// 3. STORE: Serialize and save to RocksDB
auto blob = entity.serialize();
config_.db->put(key, blob);
```

```cpp
// 4. READ: Deserialize from RocksDB
auto data = config_.db->get(key);
BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
```

```cpp
// 5. UPDATE: Modify fields
entity.setField("version", Value(metadata.version));
entity.setField("description", Value(metadata.description));
auto blob = entity.serialize();
config_.db->put(key, blob);
```

### Schema for LoRA Adapters

```json
{
  "_key": "themis_help_lora_v1",
  "adapter_id": "themis_help_lora",
  "version": "1.0.0",
  "base_model": "llama-2-7b",
  "description": "Documentation assistance adapter",
  "training_samples": 5000,
  "validation_accuracy": 0.92,
  "format": "safetensors",
  "size_bytes": 33554432,
  
  // Small adapters: inline
  "weights_data": [binary blob],
  
  // Large adapters: blob reference
  "blob_ref_type": 3,  // BlobStorageType::FILESYSTEM
  "blob_ref_path": "data/blobs/themis_help_lora.bin",
  
  // Timestamps
  "created_at": 1736601600,
  "updated_at": 1736601600
}
```

## ✅ Compliance Checklist

### We Follow These BaseEntity Principles:

- [x] **Store as BaseEntity**: All LoRA adapters stored using BaseEntity
- [x] **Use FieldMap**: Structured metadata in fields
- [x] **Serialize properly**: Use `entity.serialize()` for storage
- [x] **Deserialize properly**: Use `BaseEntity::deserialize()` for loading
- [x] **Primary Key**: Each adapter has unique key (`adapter_id`)
- [x] **Lazy parsing**: Fields extracted on-demand
- [x] **Multi-format**: Support both inline and blob storage
- [x] **Integration**: Works with BlobStorageManager for large data

### Pattern: Collection-based Storage

```cpp
// Collection key pattern: "collection:primary_key"
std::string key = config_.collection_name + ":" + adapter_id;
// Example: "lora_adapters:themis_help_lora"
```

### Pattern: Large Data Handling

```cpp
// ThemisDB pattern for large blobs:
// 1. Store metadata in BaseEntity
// 2. Store large data in BlobStorage
// 3. Reference blob from BaseEntity

if (data.size() > THRESHOLD) {
    auto ref = blob_manager->put(id, data);
    entity.setField("blob_ref", ref.path);
} else {
    entity.setField("data", data);
}
```

## 🏗️ Architecture Alignment

### ThemisDB Storage Hierarchy

```
┌─────────────────────────────────────┐
│     Application Layer               │
│  (LoRA Orchestrator, Services)      │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│        BaseEntity Layer              │
│  Universal storage abstraction       │
│  - FieldMap                          │
│  - Serialization                     │
│  - Format detection                  │
└──────────────┬──────────────────────┘
               │
       ┌───────┴────────┐
       ▼                ▼
┌─────────────┐  ┌─────────────┐
│  RocksDB    │  │BlobStorage  │
│  Wrapper    │  │  Manager    │
│  (Metadata) │  │ (Large data)│
└─────────────┘  └─────────────┘
```

### LoRA Adapter as BaseEntity

```
LoRA Adapter
    │
    ├─ adapter_id (Primary Key)
    ├─ version
    ├─ base_model  
    ├─ metadata (BaseEntity fields)
    │   ├─ training_samples
    │   ├─ validation_accuracy
    │   └─ hyperparameters
    │
    └─ weights (stored appropriately)
        ├─ < 1MB: Inline in BaseEntity
        └─ > 1MB: BlobStorage with reference
```

## 🔍 Real-World Examples

### Example 1: Small Adapter (Inline Storage)

```cpp
// Adapter < 1MB: stored inline
BaseEntity::FieldMap fields;
fields["adapter_id"] = Value("small_adapter");
fields["weights_data"] = Value(weights.data);  // Inline binary

BaseEntity entity = BaseEntity::fromFields("small_adapter", fields);
auto blob = entity.serialize();
db->put("lora_adapters:small_adapter", blob);
```

### Example 2: Large Adapter (Blob Storage)

```cpp
// Adapter > 1MB: stored in blob storage
BaseEntity::FieldMap fields;
fields["adapter_id"] = Value("large_adapter");

// Store weights in blob storage
auto ref = blob_manager->put("large_adapter", weights.data);
fields["blob_ref_type"] = Value(static_cast<int64_t>(ref.type));
fields["blob_ref_path"] = Value(ref.path);

BaseEntity entity = BaseEntity::fromFields("large_adapter", fields);
auto blob = entity.serialize();
db->put("lora_adapters:large_adapter", blob);
```

### Example 3: Update Metadata

```cpp
// Load existing adapter
auto data = db->get("lora_adapters:my_adapter");
BaseEntity entity = BaseEntity::deserialize("my_adapter", *data);

// Update fields
entity.setField("validation_accuracy", Value(0.95));
entity.setField("updated_at", Value(current_timestamp));

// Save back
auto blob = entity.serialize();
db->put("lora_adapters:my_adapter", blob);
```

## 🎓 Best Practices

### DO ✅

1. **Always use BaseEntity for storage**
   ```cpp
   BaseEntity entity = BaseEntity::fromFields(id, fields);
   ```

2. **Use proper Value types**
   ```cpp
   fields["count"] = Value(static_cast<int64_t>(count));
   fields["score"] = Value(static_cast<double>(score));
   fields["name"] = Value(std::string(name));
   ```

3. **Use BlobStorage for large data**
   ```cpp
   if (size > THRESHOLD) {
       auto ref = blob_manager->put(id, data);
       entity.setField("blob_ref", ref.path);
   }
   ```

4. **Serialize before storing**
   ```cpp
   auto blob = entity.serialize();
   db->put(key, blob);
   ```

5. **Deserialize after loading**
   ```cpp
   auto data = db->get(key);
   BaseEntity entity = BaseEntity::deserialize(id, *data);
   ```

### DON'T ❌

1. **Don't bypass BaseEntity**
   ```cpp
   // ❌ Wrong
   db->put(key, raw_weights_data);
   
   // ✅ Correct
   BaseEntity entity = BaseEntity::fromFields(id, fields);
   db->put(key, entity.serialize());
   ```

2. **Don't store raw JSON strings**
   ```cpp
   // ❌ Wrong
   db->put(key, json_string);
   
   // ✅ Correct
   BaseEntity entity = BaseEntity::fromJson(id, json_string);
   db->put(key, entity.serialize());
   ```

3. **Don't mix storage formats**
   ```cpp
   // ❌ Wrong: Different format for each adapter
   
   // ✅ Correct: Consistent BaseEntity usage
   ```

## 📝 Summary

### Why BaseEntity?

1. **Consistency**: All ThemisDB data uses same abstraction
2. **Flexibility**: Schema-less supports evolution
3. **Performance**: Lazy parsing, efficient serialization
4. **Integration**: Works with all ThemisDB features
5. **Multi-model**: Supports documents, graphs, vectors, etc.

### LoRA Framework Compliance

✅ Our implementation **fully follows** the BaseEntity principle:
- All adapters stored as BaseEntity
- Proper serialization/deserialization
- Integration with BlobStorage
- Collection-based organization
- Field-based metadata

### Next Steps

Continue following BaseEntity principle for:
- [ ] Training job metadata
- [ ] Feedback data
- [ ] Version history
- [ ] Deployment records

---

*Generated: 2026-01-11*
*Status: BaseEntity principle correctly implemented*
