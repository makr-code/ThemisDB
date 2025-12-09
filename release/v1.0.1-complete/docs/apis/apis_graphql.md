# GraphQL API

**Status:** ✅ Implementiert  
**Version:** 1.0  
**Datum:** 30. November 2025

---

## Übersicht

ThemisDB bietet eine GraphQL API als Alternative zur REST API. GraphQL ermöglicht flexible Abfragen mit exakter Feldauswahl und effizienter Datenabfrage.

## Features

### ✅ Implementiert

- **GraphQL Parser**
  - Query, Mutation, Subscription Operationen
  - Field Selections mit Aliases
  - Argumente (alle Typen)
  - Variablen mit Default-Werten
  - Verschachtelte Selections
  - Kommentare

- **Schema**
  - Built-in Scalar Types (String, Int, Float, Boolean, ID, JSON)
  - Document Types
  - Graph Types (Node, Edge)
  - Vector Search Types
  - Timeseries Types
  - Query und Mutation Types

- **Executor**
  - Field Resolution
  - Nested Selection Execution
  - Error Handling
  - Custom Resolvers

- **Schema Introspection**
  - SDL (Schema Definition Language) Export
  - Type Queries

## GraphQL Schema

### Query Type

```graphql
type Query {
  # Einzelnes Dokument abrufen
  document(collection: String!, id: ID!): Document
  
  # Dokumente auflisten
  documents(collection: String!, limit: Int, offset: Int): [Document!]!
  
  # AQL Query ausführen
  aql(query: String!, variables: JSON): JSON
  
  # Vector-Suche
  vectorSearch(collection: String!, vector: [Float!]!, k: Int): [VectorSearchResult!]!
  
  # Graph-Traversierung
  graphTraversal(startNode: ID!, depth: Int, direction: String): [Node!]!
}
```

### Mutation Type

```graphql
type Mutation {
  # Dokument erstellen
  createDocument(collection: String!, input: DocumentInput!): Document!
  
  # Dokument aktualisieren
  updateDocument(collection: String!, id: ID!, input: DocumentInput!): Document
  
  # Dokument löschen
  deleteDocument(collection: String!, id: ID!): Boolean!
  
  # Graph-Kante erstellen
  createEdge(source: ID!, target: ID!, type: String!, properties: JSON): Edge!
}
```

### Document Type

```graphql
type Document {
  id: ID!
  collection: String!
  data: JSON!
  createdAt: String
  updatedAt: String
}

input DocumentInput {
  data: JSON!
}
```

### Graph Types

```graphql
type Node {
  id: ID!
  labels: [String]
  properties: JSON
}

type Edge {
  id: ID!
  type: String!
  source: Node!
  target: Node!
  properties: JSON
}
```

### Vector Search Type

```graphql
type VectorSearchResult {
  id: ID!
  score: Float!
  document: Document
}
```

## Verwendung

### Parser

```cpp
#include "api/graphql.h"

using namespace themis::graphql;

// GraphQL Query parsen
auto result = Parser::parse(R"(
    query GetUser($userId: ID!) {
        document(collection: "users", id: $userId) {
            id
            data
            createdAt
        }
    }
)");

if (result.success) {
    const auto& op = result.document.operations[0];
    std::cout << "Operation: " << op.name << std::endl;
}
```

### Executor mit Custom Resolvers

```cpp
// Execution Context aufbauen
ExecutionContext ctx;
ctx.tenant_id = "acme-corp";
ctx.user_id = "user123";

// Variable setzen
ctx.variables["userId"] = Value::string("user-456");

// Resolver definieren
ctx.resolvers["document"] = [](const Field& field, 
                                const std::shared_ptr<Value>& parent,
                                const ExecutionContext& ctx) {
    // Collection und ID aus Argumenten lesen
    auto collection = field.arguments.at("collection")->asString();
    auto id = field.arguments.at("id")->asString();
    
    // Dokument laden (hier: Mock-Daten)
    ValueMap doc;
    doc["id"] = Value::string(id);
    doc["collection"] = Value::string(collection);
    doc["data"] = Value::object({{"name", Value::string("John")}});
    doc["createdAt"] = Value::string("2025-11-30T12:00:00Z");
    
    return Value::object(std::move(doc));
};

// Query ausführen
Executor executor;
auto result = executor.execute(parseResult.document, ctx);

if (!result.hasErrors()) {
    // result.data enthält das Ergebnis
}
```

### Schema generieren

```cpp
Schema schema = ThemisSchemaBuilder::build();

// SDL exportieren
std::string sdl = schema.toSDL();
std::cout << sdl << std::endl;
```

## HTTP Endpoint (geplant)

```http
POST /graphql HTTP/1.1
Host: localhost:8080
Content-Type: application/json
X-Tenant-ID: acme-corp
Authorization: Bearer <token>

{
    "query": "query { documents(collection: \"users\", limit: 10) { id data } }",
    "variables": {}
}
```

### Response

```json
{
    "data": {
        "documents": [
            {"id": "user-1", "data": {"name": "Alice"}},
            {"id": "user-2", "data": {"name": "Bob"}}
        ]
    },
    "errors": []
}
```

## Beispiele

### Einfache Query

```graphql
{
    document(collection: "users", id: "user-123") {
        id
        data
    }
}
```

### Query mit Variablen

```graphql
query GetUser($collection: String!, $id: ID!) {
    document(collection: $collection, id: $id) {
        id
        data
        createdAt
        updatedAt
    }
}
```

### Mutation

```graphql
mutation CreateUser {
    createDocument(collection: "users", input: {
        data: {
            name: "John Doe",
            email: "john@example.com"
        }
    }) {
        id
        createdAt
    }
}
```

### Aliases

```graphql
{
    firstUser: document(collection: "users", id: "1") { id data }
    secondUser: document(collection: "users", id: "2") { id data }
}
```

### Vector Search

```graphql
{
    vectorSearch(collection: "documents", vector: [0.1, 0.2, 0.3], k: 10) {
        id
        score
        document {
            id
            data
        }
    }
}
```

### Graph Traversal

```graphql
{
    graphTraversal(startNode: "user:alice", depth: 3, direction: "OUTBOUND") {
        id
        labels
        properties
    }
}
```

## Value Types

```cpp
// Null
auto nullVal = Value::null();

// Boolean
auto boolVal = Value::boolean(true);

// Integer
auto intVal = Value::integer(42);

// Float
auto floatVal = Value::floating(3.14159);

// String
auto strVal = Value::string("Hello");

// Enum
auto enumVal = Value::enumValue("ACTIVE");

// List
ValueList list;
list.push_back(Value::integer(1));
list.push_back(Value::integer(2));
auto listVal = Value::list(std::move(list));

// Object
ValueMap map;
map["key"] = Value::string("value");
auto objVal = Value::object(std::move(map));
```

## Limitationen

- Keine Fragments (geplant)
- Keine Directives (geplant)
- Keine Inline Fragments
- Keine Subscriptions über WebSocket (nur Polling)

## Roadmap

- [ ] HTTP Handler Integration
- [ ] Fragment Support
- [ ] Directive Support
- [ ] Subscription über WebSocket
- [ ] Batching Support
- [ ] Persisted Queries
- [ ] Query Complexity Analysis

---

**Letzte Aktualisierung:** 30. November 2025  
**Maintainer:** ThemisDB Team
