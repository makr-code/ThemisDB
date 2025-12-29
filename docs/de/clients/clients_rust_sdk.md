# 🦀 ThemisDB Rust SDK

<!-- Dokumentations-Metadaten -->
**Kategorie:**  SDK Implementation  
**Version:** v1.3.0  
**Status:** ✅ Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

##  Inhaltsverzeichnis

- [ Übersicht](#-übersicht)
- [ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [ Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [ Best Practices](#-best-practices)
- [ Troubleshooting](#-troubleshooting)
- [ Siehe auch](#-siehe-auch)
- [ Changelog](#-changelog)

---

##  Übersicht

Das Rust-SDK (`themisdb_sdk`) bietet type-safe und zero-copy Zugriff auf ThemisDB mit vollständiger async/await Unterstützung. Das SDK befindet sich im Alpha-Status  Breaking Changes sind möglich.

###  Zielgruppe

- Rust Systems Programmierer
- Performance-kritische Anwendungen
- Embedded Systems Entwickler
- CLI Tool Entwickler

###  Voraussetzungen

- Rust stable toolchain mit `cargo`
- ThemisDB-Endpunkt (z.B. `http://127.0.0.1:8765`)
- Optional: Topologie-Endpunkt

---

##  Features & Highlights

###  Kern-Features

| Feature | Beschreibung | Status |
|---------|--------------|--------|
|  **CRUD Operations** | Type-safe get, put, delete |  Stabil |
|  **AQL Queries** | Generic query execution |  Stabil |
|  **Vector Search** | Similarity search |  Stabil |
|  **Graph Traversal** | Graph operations |  Stabil |
|  **Batch Operations** | Parallel batch processing |  Stabil |
|  **Cursor Pagination** | Efficient large datasets |  Stabil |
|  **Topology-Aware** | Automatic shard routing |  Stabil |
|  **Retry Logic** | Configurable retries |  Stabil |

###  Besondere Rust-Features

-  **Type Safety** - Compile-time type checking
-  **Zero-Copy** - Optimierte Performance
-  **Async/Await** - Tokio-basiert
-  **Error Handling** - Result<T, ThemisError>
-  **Ownership** - Rust memory safety

---

##  Schnellstart

###  Installation

```toml
# Cargo.toml
[dependencies]
themisdb_sdk = { path = "../ThemisDB/clients/rust" }
# Oder via Git
themisdb_sdk = { git = "https://github.com/themisdb/themisdb-rust" }
```

###  Erste Schritte

```rust
use themisdb_sdk::{ThemisClient, ThemisClientConfig};

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let client = ThemisClient::new(ThemisClientConfig {
        endpoints: vec!["http://127.0.0.1:8765".into()],
        namespace: "default".into(),
        metadata_endpoint: Some("/_admin/cluster/topology".into()),
        ..Default::default()
    })?;

    let health = client.health().await?;
    println!("{:?}", health);

    Ok(())
}
```

---

##  Detaillierte Dokumentation

###  Konfiguration

```rust
let client = ThemisClient::new(ThemisClientConfig {
    endpoints: vec!["http://shard-1:8765".into()],
    namespace: "production".into(),
    metadata_endpoint: Some("http://etcd:2379/topology".into()),
    timeout_ms: 60000,
    max_retries: 5,
    ..Default::default()
})?;
```

| Feld | Typ | Beschreibung | Default |
|------|-----|--------------|---------|
| `endpoints` | `Vec<String>` | Bootstrap HTTP bases | **Required** |
| `namespace` | `String` | Namespace for URNs | `"default"` |
| `metadata_endpoint` | `Option<String>` | Topology service | `Some("/_admin...")` |
| `timeout_ms` | `u64` | Request timeout (ms) | `30000` |
| `max_retries` | `u32` | Retry count for 5xx | `3` |

###  CRUD Operationen

```rust
let user_id = "550e8400-e29b-41d4-a716-446655440000";

// CREATE / UPDATE
client.put("relational", "users", user_id, 
    &serde_json::json!({"name": "Alice", "age": 30})
).await?;

// READ
if let Some(user) = client.get::<serde_json::Value>("relational", "users", user_id).await? {
    println!("{}", user);
}

// DELETE
let removed = client.delete("relational", "users", user_id).await?;
```

###  Batch-Operationen

```rust
let batch = client.batch_get::<serde_json::Value>(
    "relational", 
    "users", 
    &["1".into(), "2".into(), "404".into()]
).await?;

println!("Found {} users", batch.found.len());
```

###  AQL Queries & Cursor

```rust
use themisdb_sdk::QueryOptions;

let page = client.query::<serde_json::Value>(
    "FOR u IN users RETURN u",
    QueryOptions {
        use_cursor: true,
        batch_size: Some(100),
        ..Default::default()
    }
).await?;

if page.has_more {
    if let Some(cursor) = page.next_cursor {
        let next = client.query::<serde_json::Value>(
            "FOR u IN users RETURN u",
            QueryOptions {
                use_cursor: true,
                cursor: Some(cursor),
                ..Default::default()
            }
        ).await?;
    }
}
```

###  Vector Search

```rust
let result = client.vector_search(
    &[0.12, -0.04, 0.9],
    Some(serde_json::json!({"namespace": "docs"})),
    Some(5)
).await?;
```

---

##  Best Practices

###  DO: Use Type Safety

```rust
#[derive(Deserialize)]
struct User {
    name: String,
    email: String,
    age: u32,
}

let user: Option<User> = client.get("relational", "users", id).await?;
```

###  DO: Handle Errors Properly

```rust
match client.get::<User>("relational", "users", id).await {
    Ok(Some(user)) => println!("Found: {}", user.name),
    Ok(None) => println!("Not found"),
    Err(ThemisError::Http(status, body)) => eprintln!("HTTP {}: {}", status, body),
    Err(e) => eprintln!("Error: {}", e),
}
```

---

##  Troubleshooting

###  Compilation Errors

**Problem:** Type mismatch errors

**Lösung:**
```bash
cargo clean
cargo build
```

###  Connection Errors

**Problem:** Cannot connect to ThemisDB

**Lösung:**
```rust
let client = ThemisClient::new(ThemisClientConfig {
    endpoints: vec!["http://localhost:8765".into()],
    timeout_ms: 60000,
    max_retries: 5,
    ..Default::default()
})?;
```

---

##  Siehe auch

- [ Python SDK](clients_python_sdk.md)
- [ JavaScript SDK](clients_javascript_sdk.md)
- [ HTTP API Reference](../apis/HTTP_API_REFERENCE.md)
- [ AQL Reference](../aql/AQL_REFERENCE.md)

---

##  Changelog

### Version 1.3.0 (22.12.2025)
-  Aktualisierung auf v1.3.0 Template
-  Erweiterte Code-Beispiele
-  Best Practices hinzugefügt
-  Alle Links aktualisiert

### Version 1.0.0 (05.12.2025)
-  Alpha Release
-  Full async/await support
-  Type-safe operations
