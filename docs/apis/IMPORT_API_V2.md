# Import API v2.0 – Specification

## Base URL

```
http://localhost:8765/api/v1/import
```

---

## Existing Endpoints (v1.x – unchanged)

### `POST /api/v1/import/postgresql`

Start an asynchronous PostgreSQL dump import.

**Request body:**
```json
{
  "source_path": "/path/to/dump.sql",
  "options": {
    "dry_run": false,
    "continue_on_error": true,
    "batch_size": 1000,
    "preserve_relationships": true,
    "validate_references": false,
    "relationship_mapping_mode": "auto"
  }
}
```

**Response `200`:**
```json
{ "id": "import-1741693543000-1234", "status": "running", "stage": "pending" }
```

---

### `GET /api/v1/import/{job_id}/status`

Retrieve live progress or final stats for a job.

**Response `200`:**
```json
{
  "id": "import-1741693543000-1234",
  "status": "completed",
  "stats": {
    "tables_processed": 3,
    "imported_records": 15000,
    "relationships_processed": 4,
    "indexes_processed": 6,
    "elapsed_seconds": 0.42
  }
}
```

---

### `POST /api/v1/import/{job_id}/cancel`

Cancel a running import job.

---

### `GET /api/v1/import/jobs`

List all jobs (running + completed).

---

### `GET /api/v1/import/metrics`

Prometheus text-format metrics aggregated across all jobs.

---

## New v2.0 Endpoints

### `GET /api/v1/import/schema/{job_id}`

Return the parsed schema (tables, FKs, indexes, graph relationships) for the dump file associated with a job.

This endpoint can be called while a job is running or after it completes.

**Response `200`:**
```json
{
  "job_id": "import-1741693543000-1234",
  "schema": {
    "tables": [
      {
        "name": "orders",
        "schema": "public",
        "columns": ["id", "user_id", "amount"],
        "column_types": { "id": "integer", "user_id": "integer", "amount": "decimal" },
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
        "column_defaults": { "amount": "0.00" },
        "column_constraints": {},
        "indexes": [
          { "name": "idx_user_id", "type": "btree", "columns": ["user_id"], "unique": false }
        ]
      }
    ],
    "relationships": [{
      "_type": "fk_orders_users",
      "_from": "orders/user_id",
      "_to": "users/id",
      "cardinality": "MANY_TO_ONE",
      "source_table": "orders",
      "target_table": "users"
    }],
    "circular_references": [],
    "custom_types": { "order_status": "string" }
  }
}
```

**Response `404`:** Job not found.

**Response `409`:** Job has no `source_path` available (not yet started).

---

### `POST /api/v1/import/schema/validate`

Validate FK mappings in a dump file **without** importing data. Use this for pre-flight checks before triggering a full import.

**Request body:**
```json
{
  "source_path": "/path/to/dump.sql",
  "options": {
    "preserve_relationships": true
  }
}
```

**Response `200` (valid):**
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

**Response `200` (invalid):**
```json
{
  "valid": false,
  "tables": 2,
  "relationships": 1,
  "warnings": [
    "FK 'fk_orders_users' in table 'orders' references unknown table 'users'"
  ],
  "errors": [],
  "circular_references": ["a → b → a"]
}
```

**Response `400`:** Missing or invalid `source_path`.

**Response `422`:** Could not parse schema from the given file.

---

### `PUT /api/v1/import/{job_id}/relationships`

Override or configure the relationship mappings for a job. Useful when auto-detection produces incorrect cardinality or edge types, or when `relationship_mapping_mode` is set to `"manual"`.

Calling this endpoint stores the provided mappings; they will be returned by `GET /api/v1/import/schema/{job_id}` and used by the import pipeline when relationship mapping mode is `"manual"`.

**Request body:** JSON array of relationship objects.
```json
[
  {
    "edge_type": "orders_belongs_to_users",
    "source_table": "orders",
    "source_column": "user_id",
    "target_table": "users",
    "target_column": "id",
    "cardinality": "MANY_TO_ONE"
  }
]
```

Each object must have `source_table` and `target_table`. Additional fields are optional.

**Response `200`:**
```json
{
  "job_id": "import-1741693543000-1234",
  "relationships_configured": 1
}
```

**Response `400`:** Body is not a JSON array, or a relationship entry is missing required fields.

**Response `404`:** Job not found.

---

## `ImportOptions` Fields (v2.0 additions)

| Field | Type | Default | Description |
|---|---|---|---|
| `preserve_relationships` | bool | `true` | Extract FK constraints as graph edges |
| `validate_references` | bool | `false` | Validate FK references before import |
| `relationship_mapping_mode` | string | `"auto"` | `"auto"` / `"manual"` / `"skip"` |

---

## `ImportStats` Fields (v2.0 additions)

| Field | Type | Description |
|---|---|---|
| `relationships_processed` | uint | FK constraints mapped to graph edges |
| `indexes_processed` | uint | `CREATE INDEX` statements parsed |
