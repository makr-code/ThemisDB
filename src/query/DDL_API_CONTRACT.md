# AQL DDL API Contract v1.0 (Frozen)

**Status**: FROZEN — Authoritative specification for Phase 2 implementation  
**Created**: 2026-08-06  
**Target Release**: v2.0.0 (Q4 2026)  
**Dependencies**: Mutations Phase 1 complete (parser foundation) — DDL Phase 1 starts Week 8  
**Blocking Items**: None (will block downstream features until complete)

---

## 1. Overview

This contract defines the API surface for AQL Data Definition Language (DDL) operations: CREATE COLLECTION, CREATE INDEX, CREATE VIEW, DROP COLLECTION, DROP INDEX, DROP VIEW. All Phase 2 implementations must conform to these specifications.

**Key Property**: DDL operations are **transactional**—a CREATE INDEX that fails must rollback completely, leaving the collection unchanged.

---

## 2. AST Node Types

All DDL operations are represented as AST nodes extending `DDLNode` base class.

### 2.1 Base DDL Node

```cpp
// include/query/aql_ddl_ast.h

class DDLNode : public AQLNode {
  public:
    enum class DDLType {
        CREATE_COLLECTION,
        CREATE_INDEX,
        CREATE_VIEW,
        DROP_COLLECTION,
        DROP_INDEX,
        DROP_VIEW,
    };

    virtual ~DDLNode() = default;
    
    DDLType getDDLType() const;
    std::string getTargetName() const;  // collection/index/view name
    bool isIfNotExists() const;         // CREATE ... IF NOT EXISTS
    bool isIfExists() const;            // DROP ... IF EXISTS
};
```

### 2.2 CREATE COLLECTION Node

```cpp
class CreateCollectionNode : public DDLNode {
  public:
    CreateCollectionNode(std::string name);
    
    std::string getCollectionName() const;
    
    // Collection options (all optional)
    int getShardCount() const;              // default: 1
    int getReplicationFactor() const;       // default: 1
    bool getWaitForSync() const;            // default: false
    
    // Error codes:
    // - ERR_ARANGO_DUPLICATE_NAME (400): collection already exists
    // - ERR_ARANGO_COLLECTION_TYPE_INVALID (400): invalid shard/replication
    // - ERR_ARANGO_INSUFFICIENT_RESOURCES (500): not enough shards/replicas
    
    // Example: CREATE COLLECTION users WITH { shardCount: 3, replicationFactor: 2 }
};
```

### 2.3 CREATE INDEX Node

```cpp
class CreateIndexNode : public DDLNode {
  public:
    enum class IndexType {
        PRIMARY,      // _key index (implicit, cannot create)
        HASH,         // hash index (exact match)
        SKIPLIST,     // skiplist (range queries)
        GEO,          // geospatial index (ST_* queries)
        FULLTEXT,     // full-text search index
        VECTOR,       // vector similarity index (for embeddings)
    };

    CreateIndexNode(std::string collection, IndexType type, 
                    std::vector<std::string> fields);
    
    std::string getCollectionName() const;
    IndexType getIndexType() const;
    std::vector<std::string> getFields() const;
    
    // Index options
    bool getUnique() const;         // UNIQUE constraint
    bool getSparse() const;         // SPARSE (ignore docs with missing field)
    std::string getName() const;    // custom index name
    
    // Error codes:
    // - ERR_ARANGO_DUPLICATE_NAME (400): index already exists
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): target collection doesn't exist
    // - ERR_ARANGO_INVALID_ATTRIBUTE_NAME (400): field doesn't exist/invalid
    // - ERR_ARANGO_INDEX_CREATION_FAILED (500): RocksDB index creation failed
    
    // Example: CREATE INDEX idx_name ON collection TYPE skiplist FIELDS (field1, field2) UNIQUE SPARSE
};
```

### 2.4 CREATE VIEW Node

```cpp
class CreateViewNode : public DDLNode {
  public:
    enum class ViewType {
        MATERIALIZED,   // materialized (eager evaluation + caching)
        STREAMING,      // streaming (lazy, on-demand)
    };

    CreateViewNode(std::string name, ViewType type, AQLExpression* query);
    
    std::string getViewName() const;
    ViewType getViewType() const;
    AQLExpression* getViewQuery() const;  // SELECT statement defining view
    
    // Error codes:
    // - ERR_ARANGO_DUPLICATE_NAME (400): view already exists
    // - ERR_ARANGO_INVALID_QUERY (400): view query invalid
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): referenced collection not found
    
    // Example: CREATE VIEW user_summary AS SELECT _key, COUNT(*) FROM users GROUP BY _key
};
```

### 2.5 DROP COLLECTION Node

```cpp
class DropCollectionNode : public DDLNode {
  public:
    DropCollectionNode(std::string name);
    
    std::string getCollectionName() const;
    
    // Error codes:
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): collection doesn't exist
    // - ERR_ARANGO_CANNOT_DROP_SYSTEM_COLLECTION (403): _system reserved
    // - ERR_ARANGO_COLLECTION_IN_USE (403): active transactions referencing it
};
```

### 2.6 DROP INDEX Node

```cpp
class DropIndexNode : public DDLNode {
  public:
    DropIndexNode(std::string collection, std::string indexName);
    
    std::string getCollectionName() const;
    std::string getIndexName() const;
    
    // Error codes:
    // - ERR_ARANGO_INDEX_NOT_FOUND (400): index doesn't exist
    // - ERR_ARANGO_CANNOT_DROP_PRIMARY_INDEX (403): _key index cannot drop
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): collection doesn't exist
};
```

### 2.7 DROP VIEW Node

```cpp
class DropViewNode : public DDLNode {
  public:
    DropViewNode(std::string name);
    
    std::string getViewName() const;
    
    // Error codes:
    // - ERR_ARANGO_VIEW_NOT_FOUND (400): view doesn't exist
};
```

---

## 3. TokenType Extensions

The AQL tokenizer must recognize DDL keywords:

```cpp
// include/query/aql_token.h (extensions)

enum class TokenType {
    // ... existing tokens ...
    
    // DDL keywords
    CREATE,         ///< CREATE keyword
    DROP,           ///< DROP keyword
    ALTER,          ///< ALTER keyword (future)
    COLLECTION,     ///< COLLECTION keyword
    INDEX,          ///< INDEX keyword
    VIEW,           ///< VIEW keyword
    
    // Index types
    TYPE,           ///< TYPE keyword (CREATE INDEX TYPE hash|skiplist|geo)
    HASH,           ///< HASH index type
    SKIPLIST,       ///< SKIPLIST index type
    GEO,            ///< GEO index type
    FULLTEXT,       ///< FULLTEXT index type
    VECTOR,         ///< VECTOR index type
    
    // Index modifiers
    UNIQUE,         ///< UNIQUE constraint
    SPARSE,         ///< SPARSE index modifier
    FIELDS,         ///< FIELDS keyword (index field list)
    WITH,           ///< WITH keyword (options clause)
    IF,             ///< IF keyword
    NOT,            ///< NOT keyword
    EXISTS,         ///< EXISTS keyword
};
```

---

## 4. Parser Method Signatures

All DDL parsers are methods on `AQLParser` class.

```cpp
// include/query/aql_parser.h (extensions)

class AQLParser {
  public:
    /// Parse CREATE COLLECTION name [WITH {...}]
    /// @return CreateCollectionNode* or nullptr
    CreateCollectionNode* parseCreateCollection();
    
    /// Parse CREATE INDEX name ON collection TYPE type FIELDS (...)
    /// @return CreateIndexNode* or nullptr
    CreateIndexNode* parseCreateIndex();
    
    /// Parse CREATE VIEW name AS SELECT ...
    /// @return CreateViewNode* or nullptr
    CreateViewNode* parseCreateView();
    
    /// Parse DROP COLLECTION name [IF EXISTS]
    /// @return DropCollectionNode* or nullptr
    DropCollectionNode* parseDropCollection();
    
    /// Parse DROP INDEX collection.indexName
    /// @return DropIndexNode* or nullptr
    DropIndexNode* parseDropIndex();
    
    /// Parse DROP VIEW name [IF EXISTS]
    /// @return DropViewNode* or nullptr
    DropViewNode* parseDropView();
};
```

---

## 5. CatalogManager Extension Points

`CatalogManager` (existing class in `src/storage/catalog_manager.h`) must be extended with DDL methods:

```cpp
// include/storage/catalog_manager.h (extensions)

class CatalogManager {
  public:
    // ===== Collection Operations =====
    
    /// Create collection with optional configuration
    /// @param name Collection name (must be unique, not exist)
    /// @param options {shardCount, replicationFactor, waitForSync}
    /// @return CollectionMetadata* or nullptr on error
    /// @throws CollectionAlreadyExists, InvalidConfiguration
    CollectionMetadata* createCollection(
        const std::string& name,
        const CollectionOptions& options = CollectionOptions{});
    
    /// Drop collection and all its indexes
    /// @param name Collection name
    /// @throws CollectionNotFound, CollectionInUse (active transactions)
    void dropCollection(const std::string& name);
    
    // ===== Index Operations =====
    
    /// Create index on collection
    /// @param collectionName Target collection
    /// @param indexName Index name (must be unique per collection)
    /// @param type Index type (HASH, SKIPLIST, GEO, FULLTEXT, VECTOR)
    /// @param fields Fields to index
    /// @param options {unique, sparse}
    /// @return IndexMetadata* or nullptr
    IndexMetadata* createIndex(
        const std::string& collectionName,
        const std::string& indexName,
        IndexType type,
        const std::vector<std::string>& fields,
        const IndexOptions& options = IndexOptions{});
    
    /// Drop index from collection
    /// @param collectionName Target collection
    /// @param indexName Index name to drop
    /// @throws IndexNotFound, CannotDropPrimaryIndex
    void dropIndex(const std::string& collectionName,
                   const std::string& indexName);
    
    /// Get all indexes for collection
    /// @return vector of IndexMetadata* (includes primary _key index)
    std::vector<const IndexMetadata*> getIndexes(
        const std::string& collectionName) const;
    
    // ===== View Operations =====
    
    /// Create view backed by SELECT query
    /// @param name View name
    /// @param query AQL SELECT statement
    /// @return ViewMetadata* or nullptr
    ViewMetadata* createView(
        const std::string& name,
        const AQLExpression* query);
    
    /// Drop view
    /// @param name View name
    /// @throws ViewNotFound
    void dropView(const std::string& name);
};
```

---

## 6. Index Metadata Schema

Index metadata persists in RocksDB under key format: `__index_{collectionName}_{indexName}`

```json
{
  "name": "idx_user_email",
  "collectionName": "users",
  "type": "SKIPLIST",
  "fields": ["email"],
  "unique": true,
  "sparse": false,
  "createdAt": "2026-08-06T12:34:56Z",
  "documentCount": 42,
  "indexSize": 12345,
  "lastRebuild": "2026-08-06T12:34:56Z"
}
```

---

## 7. Lifecycle: CREATE INDEX → CatalogManager → RocksDB

```
User Query
   ↓
Parser: parseCreateIndex()  → CreateIndexNode
   ↓
Validator: validateCreateIndex()  → check collection exists, fields valid
   ↓
DDLExecutor: executeCreateIndex(CreateIndexNode)
   ├─ CatalogManager::createIndex()  → metadata record
   ├─ RocksDB: create column family for index
   ├─ IndexBuilder: iterate collection, add all docs to index
   ├─ Write metadata to __index_* key
   └─ Return success (or rollback on error)
   ↓
QueryExecutor: plan queries using new index
```

---

## 8. Validation Rules (Phase 2 Validator)

All DDL operations must satisfy these invariants:

### 8.1 CREATE COLLECTION
- Name must be non-empty, alphanumeric + underscore
- Name must NOT be reserved (e.g., `_system`, `_users`)
- Name must NOT already exist in catalog
- shardCount must be ≥ 1 and ≤ max (default: no limit)
- replicationFactor must be ≥ 1 and ≤ available replicas

### 8.2 CREATE INDEX
- Index name must be unique per collection
- Target collection MUST exist
- Fields MUST exist in collection schema (or allow dynamic)
- For UNIQUE: fields must be unique across all documents (or fail)
- For GEO: fields must contain [lon, lat] coordinates
- For FULLTEXT: fields must be text

### 8.3 DROP COLLECTION
- Collection MUST exist
- Collection MUST NOT have active transactions
- Collection MUST NOT be referenced by any views

### 8.4 DROP INDEX
- Index MUST exist on target collection
- Cannot drop PRIMARY index (_key)

---

## 9. Error Codes

All DDL errors follow HTTP semantics:

### 4xx Validation Errors

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 400 | DUPLICATE_NAME | Collection/index/view already exists | Use different name or DROP first |
| 400 | NOT_FOUND | Collection/index/view doesn't exist | Check name spelling |
| 400 | INVALID_NAME | Name contains invalid characters | Use alphanumeric + underscore |
| 400 | INVALID_FIELD | Field doesn't exist / invalid type for index | Correct field name or create it first |
| 403 | CANNOT_DROP_SYSTEM | Attempt to drop system collection | System collections are protected |
| 403 | COLLECTION_IN_USE | Active transactions referencing collection | Wait for transactions to complete |
| 403 | CANNOT_DROP_PRIMARY | Attempt to drop _key index | Primary index cannot be dropped |

### 5xx Runtime Errors

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 500 | INDEX_CREATION_FAILED | RocksDB index creation failed | Check disk space; rebuild index |
| 500 | METADATA_CORRUPTION | Index metadata corrupted | Rebuild index; check catalog |
| 500 | TRANSACTION_ROLLBACK | DDL transaction failed mid-way | Rollback complete; retry |

---

## 10. Integration Points

DDL must integrate with these existing systems:

### 10.1 Parser Integration
- `AQLParser::parseStatement()` — recognizes CREATE/DROP keywords
- Returns typed `DDLNode*`

### 10.2 Executor Integration
- `QueryExecutor::execute(DDLNode*)` → delegates to `DDLExecutor`
- `DDLExecutor::executeDDL(DDLNode*)` → DDL logic

### 10.3 Storage Integration
- `CatalogManager` — central metadata store (new extension methods)
- RocksDB — persists index metadata under `__index_*` keys
- Index Registry — register/deregister indexes for query optimization

### 10.4 Optimizer Integration
- Query optimizer MUST recognize new indexes
- Optimizer MUST use CREATE INDEX hints for query planning
- Example: FILTER x.email = "foo@bar.com" uses email SKIPLIST index

### 10.5 Transaction Integration
- DDL operations MUST be transactional
- BEGIN/COMMIT wrapping DDL ensures atomicity
- Rollback must undo CREATE INDEX (delete RocksDB column family)

---

## 11. Performance Targets (Phase 5 Gates)

DDL operations MUST meet these targets at scale:

| Operation | Target | Basis |
|-----------|--------|-------|
| CREATE COLLECTION | ≤ 100ms | metadata creation + RocksDB setup |
| CREATE INDEX (empty) | ≤ 500ms | column family creation + metadata |
| CREATE INDEX (1M docs) | ≤ 30s | iterate + batch index writes |
| DROP COLLECTION (empty) | ≤ 100ms | metadata deletion + cleanup |
| DROP COLLECTION (1M docs) | ≤ 5s | index cleanup + RocksDB removal |
| CREATE VIEW | ≤ 200ms | query compilation + metadata |

---

## 12. Test Requirements (Phase 4)

Phase 2 implementation must include these test categories:

### Parser Tests (30+)
- Valid CREATE/DROP syntax
- IF NOT EXISTS / IF EXISTS modifiers
- Index type variations (HASH, SKIPLIST, GEO, FULLTEXT)
- Error cases (missing keywords, duplicate names)

### Validator Tests (20+)
- Collection name validation
- Field existence checks
- Uniqueness constraint validation
- Reserved name protection

### Executor Tests (25+)
- CREATE COLLECTION → metadata persisted
- CREATE INDEX → RocksDB column family created
- DROP operations → complete cleanup
- Transaction rollback on error

### Integration Tests (50+)
- CREATE → INSERT uses index automatically
- CREATE VIEW → SELECT FROM view works
- DROP COLLECTION with dependent view → error
- Concurrent CREATE operations → serialized safely

---

## 13. Dependency Tree

```
DDL Phase 1-6
├── Phase 1: Design / API Contract (THIS DOCUMENT) ✓
├── Phase 2: Core Implementation (Weeks 8-20)
│   ├── Week 8-9: Parser (parseCreate*/parseDrop*)
│   ├── Week 10-11: CatalogManager extension + RocksDB schema
│   ├── Week 12-13: Executor + index builder
│   └── Week 14-20: Integration + cross-feature tests (with Mutations)
├── Phase 3: Error Handling
├── Phase 4: Tests (1000+ total across all 4 features)
├── Phase 5: Performance Hardening
└── Phase 6: Documentation & Acceptance
```

**Critical Dependency**: Mutations Phase 1 parser foundation MUST be complete before DDL Phase 1 starts.

---

## 14. Approval & Sign-Off

This contract MUST be approved by:

- [ ] **Architecture Review**: Tech Lead (CatalogManager extension is backward-compatible)
- [ ] **Parser Lead**: TokenType + parser methods follow existing patterns
- [ ] **Storage Lead**: RocksDB column family creation + metadata persistence
- [ ] **Query Optimization Lead**: Optimizer can recognize new indexes
- [ ] **PM**: Dependencies on Mutations Phase 1 completion

**Approval Status**: PENDING SIGN-OFF

**Approved By**: _______________  
**Date**: _______________  
**Changes Since Frozen**: None (this is the frozen version)

---

## 15. Amendments Log

| Date | Amendment | Approved By |
|------|-----------|-------------|
| 2026-08-06 | Initial contract created | PENDING |

(Amendments require explicit written approval and update to this section)

