#  ThemisDB JavaScript/TypeScript SDK

<!-- Dokumentations-Metadaten -->
**Kategorie:**  SDK Implementation  
**Version:** v1.3.0  
**Status:**  Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

##  Inhaltsverzeichnis

- [ Übersicht](#-übersicht)
- [ Features & Highlights](#-features--highlights)
- [ Schnellstart](#-schnellstart)
- [ Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [ Best Practices](#-best-practices)
- [ Troubleshooting](#-troubleshooting)
- [ Siehe auch](#-siehe-auch)
- [ Changelog](#-changelog)

---

##  Übersicht

Die JavaScript/TypeScript SDK (`@themisdb/sdk`) ermöglicht modernen Zugriff auf ThemisDB über Promise-basierte APIs für Node.js und Browser. Die SDK befindet sich im Alpha-Status  Breaking Changes sind möglich.

###  Zielgruppe

- JavaScript/TypeScript Entwickler
- Node.js Backend Entwickler  
- Web Frontend Entwickler
- Full-Stack Engineers

###  Voraussetzungen

- Node.js 18+ oder Browser mit globalem `fetch`
- npm oder pnpm
- HTTP-Zugriff auf ThemisDB-Endpunkt (z.B. `http://127.0.0.1:8765`)
- Optional: Topologie-Endpunkt

---

##  Features & Highlights

###  Kern-Features

| Feature | Beschreibung | Status |
|---------|--------------|--------|
|  **CRUD Operations** | Async get, put, delete |  Stabil |
|  **AQL Queries** | Promise-basierte Query Execution |  Stabil |
|  **Vector Search** | Similarity Search mit Filtern |  Stabil |
|  **Graph Traversal** | Graph-Operationen |  Stabil |
|  **Batch Operations** | batchGet, batchPut, batchDelete |  Stabil |
|  **Cursor Pagination** | Effiziente große Datensätze |  Stabil |
|  **Topology-Aware** | Automatisches Shard-Routing |  Stabil |
|  **Retry Logic** | Automatische Wiederholungen |  Stabil |

###  Besondere JavaScript-Features

-  **TypeScript Support** - Vollständige Type Definitions
-  **Promise-based** - Async/Await Pattern
-  **ESM & CommonJS** - Beide Module-Systeme
-  **Browser Compatible** - Kann in Web-Apps laufen
-  **Zero Dependencies** - Nutzt native Fetch API

---

##  Schnellstart

###  Installation

```bash
# Entwicklung im Repository
cd clients/javascript
npm install && npm run build

# Installation in anderem Projekt (lokaler Pfad)
npm install /path/to/ThemisDB/clients/javascript

# Via npm (nach Publishing)
npm install @themisdb/sdk
```

> **Hinweis:** SDK ist noch nicht im npm-Registry veröffentlicht. TypeScript 5.5.x erforderlich.

###  Erste Schritte

```typescript
import { ThemisClient } from "@themisdb/sdk";

// Client erstellen
const client = new ThemisClient({
   endpoints: ["http://127.0.0.1:8765"],
   namespace: "default",
   metadataEndpoint: "/_admin/cluster/topology"
});

// Health Check
const health = await client.health();
console.log(health);

// Cleanup
await client.close();
```

###  Try-Finally Pattern

```typescript
const client = new ThemisClient({ endpoints: ["http://localhost:8765"] });

try {
   const result = await client.query('FOR doc IN users RETURN doc');
   console.log(result.entities);
} finally {
   await client.close();
}
```

---

##  Detaillierte Dokumentation

###  Konfiguration

```typescript
const client = new ThemisClient({
   endpoints: ["http://shard-1:8765", "http://shard-2:8766"],
   namespace: "production",
   metadataEndpoint: "http://etcd:2379/topology",
   timeoutMs: 60000,     // 60 Sekunden
   maxRetries: 5
});
```

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `endpoints` | `string[]` | Bootstrap HTTP-Basen | **Pflicht** |
| `namespace` | `string` | Namespace für URNs | `"default"` |
| `metadataEndpoint` | `string` | Topologie-Service URL/Pfad | `"/_admin/cluster/topology"` |
| `timeoutMs` | `number` | Request Timeout (ms) | `30000` |
| `maxRetries` | `number` | Retry-Anzahl für 5xx | `3` |

###  CRUD Operationen

```typescript
const userId = "550e8400-e29b-41d4-a716-446655440000";

// CREATE / UPDATE
await client.put("relational", "users", userId, {
   name: "Alice Schmidt",
   email: "alice@example.com",
   age: 30
});

// READ
const user = await client.get("relational", "users", userId);
console.log(user);

// DELETE
const deleted = await client.delete("relational", "users", userId);
console.log(deleted); // true wenn vorhanden
```

###  Batch-Operationen

```typescript
// Batch GET
const batch = await client.batchGet("relational", "users", ["1", "2", "999"]);
console.log(batch.found["1"]);    // User Objekt
console.log(batch.missing);        // ["999"]
console.log(batch.errors);         // Fehler pro ID

// Batch PUT
await client.batchPut("relational", "users", {
   "1": { name: "Alice" },
   "2": { name: "Bob" }
});
```

###  AQL Queries & Cursor

```typescript
// Einfache Query
const result = await client.query('FOR u IN users FILTER u.age > 25 RETURN u');
console.log(result.entities);

// Mit Cursor-Pagination
const page = await client.query("FOR u IN users RETURN u", {
   useCursor: true,
   batchSize: 100
});

if (page.hasMore && page.nextCursor) {
   const next = await client.query("FOR u IN users RETURN u", {
      useCursor: true,
      cursor: page.nextCursor
   });
   console.log(next.items.length);
}
```

###  Vector Search

```typescript
const result = await client.vectorSearch([0.13, -0.4, 0.9], {
   topK: 5,
   filter: { namespace: "docs" }
});

console.log(result.results);
```

###  Fehlerbehandlung

```typescript
try {
   const result = await client.query('FOR u IN users RETURN u');
} catch (error) {
   if (error instanceof TopologyError) {
      console.error('Topologie konnte nicht geladen werden:', error);
   } else if (error.status >= 400) {
      console.error(`HTTP ${error.status}: ${error.statusText}`);
   } else {
      console.error('Netzwerkfehler:', error);
   }
}
```

---

##  Best Practices

###  DO: Async/Await verwenden

```typescript
//  Gut: Async/Await
const user = await client.get("relational", "users", userId);

//  Schlecht: Promise chains
client.get("relational", "users", userId).then(...).catch(...);
```

###  DO: Type Definitions nutzen

```typescript
interface User {
   name: string;
   email: string;
   age: number;
}

const user = await client.get<User>("relational", "users", userId);
console.log(user.name); // Type-safe
```

###  DO: Cursor für große Datasets

```typescript
async function processAllUsers(client: ThemisClient) {
   let page = await client.query("FOR u IN users RETURN u", { 
      useCursor: true, 
      batchSize: 100 
   });
   
   while (true) {
      page.items.forEach(processUser);
      
      if (!page.hasMore) break;
      
      page = await client.query("FOR u IN users RETURN u", {
         useCursor: true,
         cursor: page.nextCursor
      });
   }
}
```

---

##  Troubleshooting

###  ModuleNotFoundError

**Problem:** `Cannot find module '@themisdb/sdk'`

**Lösung:**
```bash
# Installation prüfen
npm list @themisdb/sdk

# Neuinstallation
npm install @themisdb/sdk

# Build prüfen
cd clients/javascript && npm run build
```

###  TopologyError

**Problem:** Topologie kann nicht geladen werden

**Lösung:**
```typescript
const client = new ThemisClient({
   endpoints: ["http://shard1:8765", "http://shard2:8765"],
   metadataEndpoint: null  // Deaktiviert Topologie-Fetch
});
```

###  Timeout Errors

**Problem:** Requests laufen in Timeout

**Lösung:**
```typescript
const client = new ThemisClient({
   endpoints: ["http://localhost:8765"],
   timeoutMs: 120000  // 2 Minuten
});
```

---

##  Siehe auch

- [ Python SDK](clients_python_sdk.md)
- [ Rust SDK](clients_rust_sdk.md)
- [ HTTP API Reference](../apis/HTTP_API_REFERENCE.md)
- [ AQL Reference](../aql/AQL_REFERENCE.md)
- [ Vector Search](../features/FEATURE_VECTOR_SEARCH.md)

---

##  Changelog

### Version 1.3.0 (22.12.2025)
-  Aktualisierung auf v1.3.0 Template
-  TypeScript Beispiele erweitert
-  Best Practices hinzugefügt
-  Troubleshooting Guide
-  Alle Links aktualisiert

### Version 1.0.0 (05.12.2025)
-  Alpha Release
-  CRUD, AQL, Vector, Graph Support
-  Promise-based API
