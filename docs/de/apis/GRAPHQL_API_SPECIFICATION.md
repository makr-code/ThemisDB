# GraphQL API Specification - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [GraphQL Endpoint](#graphql-endpoint)
- [Schema Definition](#schema-definition)
- [Queries](#queries)
- [Mutations](#mutations)
- [Subscriptions](#subscriptions)
- [Beispiele](#beispiele)
- [Best Practices](#best-practices)

---

## Übersicht

ThemisDB bietet eine vollständige GraphQL-API für flexible Datenabfragen. GraphQL ermöglicht es Clients, genau die Daten anzufordern, die sie benötigen, und reduziert Over-fetching und Under-fetching.

### GraphQL Vorteile

- ✅ **Flexible Queries**: Client bestimmt Struktur der Response
- ✅ **Single Endpoint**: Ein Endpoint für alle Operationen
- ✅ **Strong Typing**: Schema-basierte Type Safety
- ✅ **Real-time Updates**: Subscriptions für Live-Daten
- ✅ **Introspection**: Selbstbeschreibende API

---

## GraphQL Endpoint

### Basis-URL

```
https://your-themis-instance.com/graphql
```

### GraphQL Playground

Interaktive API-Exploration:
```
https://your-themis-instance.com/graphql/playground
```

---

## Schema Definition

### Core Types

```graphql
"""
Dokument in einer Kollektion
"""
type Document {
  """Eindeutige Dokument-ID"""
  id: ID!
  
  """Revisions-ID für Optimistic Locking"""
  rev: String!
  
  """Kollektion, in der das Dokument gespeichert ist"""
  collection: String!
  
  """Dokument-Felder als JSON"""
  data: JSON!
  
  """Erstellungszeitpunkt"""
  createdAt: DateTime!
  
  """Letzte Aktualisierung"""
  updatedAt: DateTime!
  
  """Metadaten"""
  metadata: Metadata
}

"""
Metadaten für Dokumente
"""
type Metadata {
  tags: [String!]
  source: String
  version: Int
  customFields: JSON
}

"""
Datenbank-Informationen
"""
type Database {
  id: ID!
  name: String!
  collections: [Collection!]!
  size: Int!
  documentCount: Int!
  status: DatabaseStatus!
}

enum DatabaseStatus {
  ONLINE
  OFFLINE
  MAINTENANCE
  READONLY
}

"""
Kollektion-Informationen
"""
type Collection {
  id: ID!
  name: String!
  database: Database!
  documentCount: Int!
  indexes: [Index!]!
  schema: JSON
}

"""
Index-Definition
"""
type Index {
  id: ID!
  name: String!
  type: IndexType!
  fields: [String!]!
  unique: Boolean!
  sparse: Boolean!
  size: Int!
}

enum IndexType {
  HASH
  BTREE
  FULLTEXT
  GEO
  VECTOR
}

"""
Query-Ergebnis mit Pagination
"""
type QueryResult {
  documents: [Document!]!
  totalCount: Int!
  hasMore: Boolean!
  cursor: String
  stats: QueryStats
}

"""
Query-Statistiken
"""
type QueryStats {
  executionTimeMs: Int!
  scanned: Int!
  filtered: Int!
  indexHits: Int!
}
```

### Custom Scalars

```graphql
"""
JSON-Datentyp für beliebige Strukturen
"""
scalar JSON

"""
DateTime im ISO-8601 Format
"""
scalar DateTime

"""
Binary Data als Base64-String
"""
scalar Base64
```

---

## Queries

### Root Query Type

```graphql
type Query {
  """Einzelnes Dokument abrufen"""
  document(
    database: String!
    collection: String!
    id: ID!
  ): Document
  
  """Dokumente mit Filterung"""
  documents(
    database: String!
    collection: String!
    filter: JSON
    sort: [SortInput!]
    limit: Int
    offset: Int
  ): QueryResult!
  
  """AQL Query ausführen"""
  executeQuery(
    database: String!
    query: String!
    bindVars: JSON
    options: QueryOptions
  ): QueryResult!
  
  """Datenbanken auflisten"""
  databases: [Database!]!
  
  """Einzelne Datenbank"""
  database(name: String!): Database
  
  """Kollektionen auflisten"""
  collections(database: String!): [Collection!]!
  
  """Einzelne Kollektion"""
  collection(
    database: String!
    name: String!
  ): Collection
  
  """Indizes auflisten"""
  indexes(
    database: String!
    collection: String!
  ): [Index!]!
  
  """Full-Text Search"""
  search(
    database: String!
    collection: String!
    query: String!
    fields: [String!]
    options: SearchOptions
  ): QueryResult!
  
  """Vector Search"""
  vectorSearch(
    database: String!
    collection: String!
    vector: [Float!]!
    k: Int!
    options: VectorSearchOptions
  ): QueryResult!
  
  """Hybrid Search (Full-Text + Vector)"""
  hybridSearch(
    database: String!
    collection: String!
    textQuery: String!
    vector: [Float!]
    options: HybridSearchOptions
  ): QueryResult!
}

input SortInput {
  field: String!
  order: SortOrder!
}

enum SortOrder {
  ASC
  DESC
}

input QueryOptions {
  batchSize: Int
  ttl: Int
  count: Boolean
  fullCount: Boolean
}

input SearchOptions {
  fuzzy: Boolean
  maxDistance: Int
  limit: Int
}

input VectorSearchOptions {
  metric: VectorMetric
  ef: Int
  limit: Int
}

enum VectorMetric {
  COSINE
  EUCLIDEAN
  DOT_PRODUCT
}

input HybridSearchOptions {
  textWeight: Float
  vectorWeight: Float
  limit: Int
}
```

---

## Mutations

### Root Mutation Type

```graphql
type Mutation {
  """Dokument erstellen"""
  createDocument(
    database: String!
    collection: String!
    document: JSON!
    metadata: MetadataInput
  ): Document!
  
  """Dokument aktualisieren"""
  updateDocument(
    database: String!
    collection: String!
    id: ID!
    rev: String!
    document: JSON!
  ): Document!
  
  """Dokument löschen"""
  deleteDocument(
    database: String!
    collection: String!
    id: ID!
    rev: String!
  ): Boolean!
  
  """Batch-Insert"""
  batchInsert(
    database: String!
    collection: String!
    documents: [JSON!]!
  ): BatchInsertResult!
  
  """Batch-Update"""
  batchUpdate(
    database: String!
    collection: String!
    updates: [UpdateInput!]!
  ): BatchUpdateResult!
  
  """Batch-Delete"""
  batchDelete(
    database: String!
    collection: String!
    ids: [ID!]!
  ): BatchDeleteResult!
  
  """Datenbank erstellen"""
  createDatabase(
    name: String!
    options: DatabaseOptions
  ): Database!
  
  """Datenbank löschen"""
  dropDatabase(name: String!): Boolean!
  
  """Kollektion erstellen"""
  createCollection(
    database: String!
    name: String!
    schema: JSON
  ): Collection!
  
  """Kollektion löschen"""
  dropCollection(
    database: String!
    name: String!
  ): Boolean!
  
  """Index erstellen"""
  createIndex(
    database: String!
    collection: String!
    type: IndexType!
    fields: [String!]!
    options: IndexOptions
  ): Index!
  
  """Index löschen"""
  dropIndex(
    database: String!
    collection: String!
    name: String!
  ): Boolean!
}

input MetadataInput {
  tags: [String!]
  source: String
  version: Int
  customFields: JSON
}

input UpdateInput {
  id: ID!
  rev: String!
  document: JSON!
}

input DatabaseOptions {
  storageEngine: String
  replicationFactor: Int
  shardingStrategy: String
}

input IndexOptions {
  name: String
  unique: Boolean
  sparse: Boolean
}

type BatchInsertResult {
  inserted: Int!
  ids: [ID!]!
  errors: [BatchError!]!
}

type BatchUpdateResult {
  updated: Int!
  errors: [BatchError!]!
}

type BatchDeleteResult {
  deleted: Int!
  errors: [BatchError!]!
}

type BatchError {
  index: Int!
  message: String!
  code: String!
}
```

---

## Subscriptions

### Root Subscription Type

```graphql
type Subscription {
  """Echtzeit-Updates für Dokumente"""
  documentChanges(
    database: String!
    collection: String!
    filter: JSON
  ): DocumentChange!
  
  """Query-Ergebnisse als Stream"""
  queryStream(
    database: String!
    query: String!
    bindVars: JSON
  ): Document!
  
  """Collection-Änderungen"""
  collectionChanges(
    database: String!
    collection: String!
  ): CollectionChange!
}

type DocumentChange {
  operation: ChangeOperation!
  document: Document!
  oldDocument: Document
}

enum ChangeOperation {
  INSERT
  UPDATE
  DELETE
}

type CollectionChange {
  type: String!
  collection: Collection!
  timestamp: DateTime!
}
```

---

## Beispiele

### Beispiel 1: Dokument abfragen

**Query:**
```graphql
query GetUser {
  document(
    database: "mydb"
    collection: "users"
    id: "user_123"
  ) {
    id
    rev
    data
    createdAt
    metadata {
      tags
      source
    }
  }
}
```

**Response:**
```json
{
  "data": {
    "document": {
      "id": "user_123",
      "rev": "1-abc",
      "data": {
        "name": "John Doe",
        "email": "john@example.com",
        "age": 30
      },
      "createdAt": "2026-01-24T14:00:00Z",
      "metadata": {
        "tags": ["customer", "premium"],
        "source": "api"
      }
    }
  }
}
```

### Beispiel 2: Gefilterte Dokumente mit Pagination

**Query:**
```graphql
query GetActiveUsers($minAge: Int!) {
  documents(
    database: "mydb"
    collection: "users"
    filter: { age: { $gte: $minAge }, status: "active" }
    sort: [{ field: "age", order: DESC }]
    limit: 10
    offset: 0
  ) {
    documents {
      id
      data
    }
    totalCount
    hasMore
    cursor
    stats {
      executionTimeMs
      scanned
      filtered
    }
  }
}
```

**Variables:**
```json
{
  "minAge": 25
}
```

**Response:**
```json
{
  "data": {
    "documents": {
      "documents": [
        {
          "id": "user_456",
          "data": {
            "name": "Alice Smith",
            "age": 35,
            "status": "active"
          }
        },
        {
          "id": "user_789",
          "data": {
            "name": "Bob Johnson",
            "age": 28,
            "status": "active"
          }
        }
      ],
      "totalCount": 2,
      "hasMore": false,
      "cursor": null,
      "stats": {
        "executionTimeMs": 45,
        "scanned": 100,
        "filtered": 2
      }
    }
  }
}
```

### Beispiel 3: AQL Query ausführen

**Query:**
```graphql
query ComplexAQLQuery {
  executeQuery(
    database: "mydb"
    query: """
      FOR u IN users
        FILTER u.role == @role
        LET projects = (
          FOR p IN projects
            FILTER p.assignee == u._id
            RETURN p
        )
        RETURN {
          user: u,
          projectCount: LENGTH(projects),
          projects: projects
        }
    """
    bindVars: { role: "developer" }
    options: { count: true }
  ) {
    documents {
      data
    }
    totalCount
    stats {
      executionTimeMs
    }
  }
}
```

### Beispiel 4: Dokument erstellen (Mutation)

**Mutation:**
```graphql
mutation CreateUser($document: JSON!) {
  createDocument(
    database: "mydb"
    collection: "users"
    document: $document
    metadata: {
      tags: ["new-user"]
      source: "graphql-api"
    }
  ) {
    id
    rev
    data
    createdAt
  }
}
```

**Variables:**
```json
{
  "document": {
    "name": "Jane Doe",
    "email": "jane@example.com",
    "age": 28,
    "role": "developer"
  }
}
```

### Beispiel 5: Batch-Insert

**Mutation:**
```graphql
mutation BatchCreateUsers($documents: [JSON!]!) {
  batchInsert(
    database: "mydb"
    collection: "users"
    documents: $documents
  ) {
    inserted
    ids
    errors {
      index
      message
      code
    }
  }
}
```

**Variables:**
```json
{
  "documents": [
    {"name": "User 1", "email": "user1@example.com"},
    {"name": "User 2", "email": "user2@example.com"},
    {"name": "User 3", "email": "user3@example.com"}
  ]
}
```

### Beispiel 6: Full-Text Search

**Query:**
```graphql
query SearchDocuments {
  search(
    database: "mydb"
    collection: "articles"
    query: "quantum computing"
    fields: ["title", "content"]
    options: {
      fuzzy: true
      maxDistance: 2
      limit: 10
    }
  ) {
    documents {
      id
      data
    }
    totalCount
  }
}
```

### Beispiel 7: Vector Search

**Query:**
```graphql
query FindSimilarDocuments($vector: [Float!]!) {
  vectorSearch(
    database: "mydb"
    collection: "embeddings"
    vector: $vector
    k: 5
    options: {
      metric: COSINE
      ef: 50
    }
  ) {
    documents {
      id
      data
    }
  }
}
```

**Variables:**
```json
{
  "vector": [0.1, 0.2, 0.3, 0.4, 0.5, /* ... 763 more values ... */]
}
```

### Beispiel 8: Subscription für Echtzeit-Updates

**Subscription:**
```graphql
subscription WatchUserChanges {
  documentChanges(
    database: "mydb"
    collection: "users"
    filter: { status: "active" }
  ) {
    operation
    document {
      id
      data
    }
    oldDocument {
      data
    }
  }
}
```

**Response Stream:**
```json
{
  "data": {
    "documentChanges": {
      "operation": "UPDATE",
      "document": {
        "id": "user_123",
        "data": {
          "name": "John Doe",
          "status": "active",
          "lastSeen": "2026-01-24T15:30:00Z"
        }
      },
      "oldDocument": {
        "data": {
          "name": "John Doe",
          "status": "active",
          "lastSeen": "2026-01-24T15:00:00Z"
        }
      }
    }
  }
}
```

---

## Client Integration

### JavaScript/TypeScript

```typescript
import { ApolloClient, InMemoryCache, gql } from '@apollo/client';

const client = new ApolloClient({
  uri: 'https://your-themis-instance.com/graphql',
  cache: new InMemoryCache(),
  headers: {
    Authorization: `Bearer ${token}`
  }
});

// Execute query
const { data } = await client.query({
  query: gql`
    query GetUser($id: ID!) {
      document(database: "mydb", collection: "users", id: $id) {
        id
        data
      }
    }
  `,
  variables: { id: 'user_123' }
});

console.log(data.document);
```

### Python

```python
from gql import gql, Client
from gql.transport.requests import RequestsHTTPTransport

transport = RequestsHTTPTransport(
    url='https://your-themis-instance.com/graphql',
    headers={'Authorization': f'Bearer {token}'}
)

client = Client(transport=transport, fetch_schema_from_transport=True)

query = gql('''
    query GetUser($id: ID!) {
        document(database: "mydb", collection: "users", id: $id) {
            id
            data
        }
    }
''')

result = client.execute(query, variable_values={'id': 'user_123'})
print(result['document'])
```

### Go

```go
package main

import (
    "context"
    "github.com/machinebox/graphql"
)

func main() {
    client := graphql.NewClient("https://your-themis-instance.com/graphql")
    
    req := graphql.NewRequest(`
        query GetUser($id: ID!) {
            document(database: "mydb", collection: "users", id: $id) {
                id
                data
            }
        }
    `)
    
    req.Var("id", "user_123")
    req.Header.Set("Authorization", "Bearer "+token)
    
    var resp struct {
        Document struct {
            ID   string
            Data map[string]interface{}
        }
    }
    
    if err := client.Run(context.Background(), req, &resp); err != nil {
        panic(err)
    }
    
    println(resp.Document.ID)
}
```

---

## Best Practices

### 1. Selektive Field Selection

**❌ Schlecht:**
```graphql
query GetUser {
  document(database: "mydb", collection: "users", id: "user_123") {
    id
    rev
    collection
    data
    createdAt
    updatedAt
    metadata {
      tags
      source
      version
      customFields
    }
  }
}
```

**✅ Gut:**
```graphql
query GetUser {
  document(database: "mydb", collection: "users", id: "user_123") {
    id
    data
  }
}
```

### 2. Pagination verwenden

```graphql
query GetUsersPaginated($limit: Int!, $offset: Int!) {
  documents(
    database: "mydb"
    collection: "users"
    limit: $limit
    offset: $offset
  ) {
    documents {
      id
      data
    }
    hasMore
    cursor
  }
}
```

### 3. Batching für Multiple Queries

```graphql
query GetMultipleResources {
  user: document(database: "mydb", collection: "users", id: "user_123") {
    id
    data
  }
  
  profile: document(database: "mydb", collection: "profiles", id: "prof_456") {
    id
    data
  }
  
  settings: document(database: "mydb", collection: "settings", id: "set_789") {
    id
    data
  }
}
```

### 4. Fragments für Wiederverwendung

```graphql
fragment UserFields on Document {
  id
  rev
  data
  createdAt
}

query GetUsers {
  activeUsers: documents(
    database: "mydb"
    collection: "users"
    filter: { status: "active" }
  ) {
    documents {
      ...UserFields
    }
  }
  
  inactiveUsers: documents(
    database: "mydb"
    collection: "users"
    filter: { status: "inactive" }
  ) {
    documents {
      ...UserFields
    }
  }
}
```

### 5. Error Handling

```typescript
try {
  const { data, errors } = await client.query({ query: GET_USER });
  
  if (errors) {
    errors.forEach(error => {
      console.error(`GraphQL Error: ${error.message}`);
      console.error(`Path: ${error.path}`);
      console.error(`Extensions:`, error.extensions);
    });
  }
  
  return data;
} catch (error) {
  console.error('Network Error:', error);
}
```

---

## Performance-Optimierung

### DataLoader für N+1 Problem

```javascript
const DataLoader = require('dataloader');

const documentLoader = new DataLoader(async (keys) => {
  const documents = await batchGetDocuments(keys);
  return keys.map(key => documents[key]);
});

// In resolver
const document = await documentLoader.load(id);
```

### Query Complexity Limiting

```javascript
const complexityLimit = require('graphql-query-complexity').default;

const schema = makeExecutableSchema({
  typeDefs,
  resolvers
});

const complexity = complexityLimit({
  maximumComplexity: 1000,
  variables: {},
  onComplete: (complexity) => {
    console.log(`Query complexity: ${complexity}`);
  }
});
```

---

## Siehe auch

- [REST API Specification](REST_API_SPECIFICATION.md)
- [gRPC API Specification](GRPC_API_SPECIFICATION.md)
- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md)
- [Search API Documentation](../search/SEARCH_API.md)
