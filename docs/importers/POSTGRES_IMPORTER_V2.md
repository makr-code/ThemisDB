# PostgreSQL Importer v2.0

> Alignment note (2026-05-31): This document is a secondary feature guide.
> Authoritative current workload and target behavior are defined in:
> - `src/importers/FUTURE_ENHANCEMENTS.md`
> - `src/importers/MODULE_GAPS.md`
> - `src/importers/ROADMAP.md`
> If this guide conflicts with newer planning docs, planning docs take precedence.

## Overview

ThemisDB v2.0 upgrades the PostgreSQL pg_dump importer with full **Foreign Key Preservation**, **Relationship Mapping**, **Index Extraction**, and **Constraint Conservation**. Import any PostgreSQL dump and have its relational structure automatically converted to ThemisDB Graph relationships.

---

## New Features

### 1. Foreign Key Preservation

PostgreSQL Foreign Keys are automatically extracted from both `CREATE TABLE` and `ALTER TABLE` statements and stored as structured metadata.

**Input (PostgreSQL dump):**
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    amount DECIMAL(10,2) DEFAULT 0.00,
    CONSTRAINT fk_orders_users FOREIGN KEY (user_id)
        REFERENCES users(id) ON DELETE CASCADE
);
```

**ThemisDB Schema after import:**
```json
{
  "tables": [
    {
      "name": "orders",
      "columns": ["id", "user_id", "amount"],
      "primary_keys": ["id"],
      "foreign_keys": [{
        "name": "fk_orders_users",
        "source_column": "user_id",
        "target_table": "users",
        "target_column": "id",
        "on_delete": "CASCADE",
        "on_update": "",
        "deferrable": false,
        "initially_deferred": false
      }],
      "column_defaults": { "amount": "0.00" }
    }
  ],
  "relationships": [{
    "_type": "fk_orders_users",
    "_from": "orders/user_id",
    "_to": "users/id",
    "cardinality": "MANY_TO_ONE"
  }]
}
```

### 2. DEFERRABLE / INITIALLY DEFERRED Support

PostgreSQL's transactional FK deferral semantics are fully preserved:

```sql
ALTER TABLE orders ADD CONSTRAINT fk_deferred
    FOREIGN KEY (user_id) REFERENCES users(id)
    DEFERRABLE INITIALLY DEFERRED;
```

This produces:
```json
{
  "deferrable": true,
  "initially_deferred": true
}
```

### 3. Composite Foreign Keys

Multi-column foreign keys are correctly joined with commas:

```sql
CONSTRAINT fk_composite FOREIGN KEY (order_id, item_id)
    REFERENCES order_details(order_id, item_id)
```

→ `"source_column": "order_id,item_id"`, `"target_column": "order_id,item_id"`

### 4. Index Extraction

All `CREATE INDEX` and `CREATE UNIQUE INDEX` statements are parsed:

```sql
CREATE UNIQUE INDEX idx_email ON users(email);
CREATE INDEX idx_geo ON locations USING gist (coordinates);
CREATE INDEX idx_active ON users(status) WHERE active = true;
```

Index types supported: **btree** (default), **hash**, **gist** (geospatial), **gin** (JSON/array), **brin**.

### 5. Relationship Mapping

FKs are automatically converted to typed graph edges:

| FK Scenario | Cardinality |
|---|---|
| Source column = source table PK | ONE_TO_ONE |
| Source column ≠ source table PK, target = target PK | MANY_TO_ONE |
| Target column ≠ target table PK | MANY_TO_MANY |

### 6. Circular Reference Detection

The importer detects and reports circular FK chains:

```json
{
  "circular_references": ["a → b → a"]
}
```

### 7. Column Defaults & Constraints

Inline column modifiers are now preserved:

```sql
CREATE TABLE accounts (
    balance DECIMAL DEFAULT 0.00,
    status  VARCHAR(20) DEFAULT 'active',
    name    TEXT NOT NULL UNIQUE
);
```

→ `column_defaults: {"balance": "0.00", "status": "'active'"}`,  
→ `column_constraints: {"name": "NOT NULL,UNIQUE"}`

---

## Configuration

```cpp
ImportOptions opts;
opts.preserve_relationships   = true;      // Extract FK → Graph edges (default: true)
opts.validate_references      = true;      // Validate FKs before import (default: false)
opts.relationship_mapping_mode = "auto";   // "auto" | "manual" | "skip"
```

---

## REST API

### Validate Schema Before Import

```bash
curl -X POST http://localhost:8765/api/v1/import/schema/validate \
  -H "Content-Type: application/json" \
  -d '{"source_path": "/path/to/dump.sql"}'
```

Response:
```json
{
  "valid": true,
  "tables": 3,
  "relationships": 2,
  "warnings": [],
  "errors": [],
  "circular_references": []
}
```

### Preview Schema with Relationships

```bash
# After starting an import job:
curl http://localhost:8765/api/v1/import/schema/{job_id}
```

### Configure Custom Relationship Mappings

```bash
curl -X PUT http://localhost:8765/api/v1/import/{job_id}/relationships \
  -H "Content-Type: application/json" \
  -d '[{
    "edge_type": "orders_belongs_to_users",
    "source_table": "orders",
    "source_column": "user_id",
    "target_table": "users",
    "target_column": "id",
    "cardinality": "MANY_TO_ONE"
  }]'
```

---

## Migration from v1.x

v1.x imports continue to work without changes. The new v2.0 features are additive:

| Feature | v1.x | v2.0 |
|---|---|---|
| Foreign Keys | ❌ Skipped | ✅ Preserved |
| Inline REFERENCES | ❌ Skipped | ✅ Extracted |
| DEFERRABLE FKs | ❌ | ✅ |
| Composite FKs | ❌ | ✅ |
| CREATE INDEX | ❌ | ✅ |
| Column DEFAULT | ❌ | ✅ |
| UNIQUE inline | ❌ | ✅ |
| Graph relationships | ❌ Manual | ✅ Auto |
| Circular ref detection | ❌ | ✅ |
| getSourceSchema() | Tables only | Full + FKs + indexes + rels |

### Upgrade Steps

1. No code changes required for existing imports
2. Enable relationship preservation: `opts.preserve_relationships = true;`
3. Use the new validation API before large imports: `POST /api/v1/import/schema/validate`
4. Review auto-generated relationships, customize if needed: `PUT /api/v1/import/{job_id}/relationships`

---

## Performance

| Operation | Target | Notes |
|---|---|---|
| DDL Parsing | < 5ms / 1000 lines | Regex-based, zero external deps |
| FK Validation | < 50ms / 100 relationships | In-memory graph traversal |
| Index Parsing | < 1ms / index | Single regex match |
| Circular Detection | < 10ms / 100 tables | O(V+E) DFS |
| Import Throughput | ≥ 50,000 rows/sec | Unchanged from v1.x |
| Memory | < 100MB / 10M rows | Streaming; FK metadata is tiny |
