# Client SDKs Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Clients

---

## Übersicht

ThemisDB bietet Client SDKs für verschiedene Programmiersprachen.

## Verfügbare SDKs

| SDK | Version | Status | Repository |
|-----|---------|--------|------------|
| **Python** | 1.0.0 | ✅ Production | `themisdb-python` |
| **JavaScript/TypeScript** | 1.0.0 | ✅ Production | `themisdb-js` |
| **Rust** | 1.0.0 | ✅ Production | `themisdb-rust` |

## Features

Alle SDKs unterstützen:
- CRUD Operations
- AQL Query Execution
- Vector Search
- Graph Traversal
- Batch Operations
- Connection Pooling
- Retry Logic

## Quick Start

### Python
```python
from themisdb import ThemisDB

db = ThemisDB("http://localhost:8765")
result = db.query("FOR doc IN users RETURN doc")
```

### JavaScript
```javascript
import { ThemisDB } from 'themisdb';

const db = new ThemisDB('http://localhost:8765');
const result = await db.query('FOR doc IN users RETURN doc');
```

### Rust
```rust
use themisdb::ThemisDB;

let db = ThemisDB::new("http://localhost:8765")?;
let result = db.query("FOR doc IN users RETURN doc")?;
```

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [clients_python_sdk.md](clients_python_sdk.md) | Python SDK Documentation |
| [clients_javascript_sdk.md](clients_javascript_sdk.md) | JavaScript SDK Documentation |
| [clients_rust_sdk.md](clients_rust_sdk.md) | Rust SDK Documentation |
| [clients_sdk_implementation.md](clients_sdk_implementation.md) | SDK Implementation Details |
| [clients_sdk_analysis.md](clients_sdk_analysis.md) | SDK Analysis |
| [clients_sdk_audit.md](clients_sdk_audit.md) | SDK Audit |
| [clients_publishing_guide.md](clients_publishing_guide.md) | Publishing Guide |
| [clients_publishing_checklist.md](clients_publishing_checklist.md) | Publishing Checklist |

## Verwandte Dokumentation

- [API Documentation](../api/README.md) - REST API
- [AQL Documentation](../aql/README.md) - Query Language
