# AQL Mutations API Contract v1.0 (Frozen)

**Status**: FROZEN — Authoritative specification for Phase 2 implementation  
**Created**: 2026-08-06  
**Target Release**: v2.0.0 (Q4 2026)  
**Dependencies**: AQL Parser foundation (Phase 1 parser work)  
**Blocking Items**: None (independent feature track)

---

## 1. Overview

This contract defines the API surface for AQL mutation operations (INSERT, UPDATE, REPLACE, REMOVE, UPSERT). All implementations in Phase 2 must conform to these specifications. No deviations without explicit contract amendment.

---

## 2. AST Node Types

All mutation operations are represented as AST nodes extending `AQLNode` base class.

### 2.1 Base Mutation Node

```cpp
// include/query/aql_mutation_ast.h

class MutationNode : public AQLNode {
  public:
    enum class MutationType {
        INSERT,    // INSERT INTO collection VALUES {...}
        UPDATE,    // UPDATE collection SET field=value [WHERE ...]
        REPLACE,   // REPLACE INTO collection VALUES {...}
        REMOVE,    // REMOVE FROM collection [WHERE ...]
        UPSERT,    // UPSERT INTO collection VALUES {...}
    };

    virtual ~MutationNode() = default;
    
    MutationType getMutationType() const;
    std::string getTargetCollection() const;
    std::vector<std::string> getAffectedFields() const;
    bool hasTransaction() const;  // true if inside BEGIN...COMMIT
};
```

### 2.2 INSERT Node

```cpp
class InsertNode : public MutationNode {
  public:
    InsertNode(std::string collection, AQLExpression* valuesExpr);
    
    // Returns: array of document objects or single document
    AQLExpression* getValuesExpression() const;
    
    // Returns: true if IGNORE DUPLICATES clause present
    bool getIgnoreDuplicates() const;
    
    // Error codes on insertion:
    // - ERR_ARANGO_DUPLICATE_KEY (409): unique constraint violation
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): target collection doesn't exist
    // - ERR_ARANGO_DOCUMENT_TYPE_INVALID (400): invalid document structure
};
```

### 2.3 UPDATE Node

```cpp
class UpdateNode : public MutationNode {
  public:
    UpdateNode(std::string collection, AQLExpression* updateExpr, 
               AQLExpression* whereClause);
    
    // Update expression: typically an object {field: newValue}
    AQLExpression* getUpdateExpression() const;
    
    // Optional WHERE predicate; nullptr = update all
    AQLExpression* getWhereClause() const;
    
    // Error codes:
    // - ERR_ARANGO_DOCUMENT_NOT_FOUND (400): target doc not found
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400): target collection doesn't exist
    // - ERR_ARANGO_WRITE_CONFLICT (403): concurrent update detected
};
```

### 2.4 REPLACE Node

```cpp
class ReplaceNode : public MutationNode {
  public:
    ReplaceNode(std::string collection, AQLExpression* documentExpr,
                AQLExpression* whereClause);
    
    // Full document replacement expression
    AQLExpression* getDocumentExpression() const;
    
    // Optional WHERE; nullptr = replace all
    AQLExpression* getWhereClause() const;
    
    // Error codes:
    // - ERR_ARANGO_DOCUMENT_NOT_FOUND (400): target doc not found
    // - ERR_ARANGO_DUPLICATE_KEY (409): unique constraint violation
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400)
};
```

### 2.5 REMOVE Node

```cpp
class RemoveNode : public MutationNode {
  public:
    RemoveNode(std::string collection, AQLExpression* whereClause);
    
    // Optional WHERE predicate; nullptr = delete all (DANGEROUS!)
    AQLExpression* getWhereClause() const;
    
    // Error codes:
    // - ERR_ARANGO_DOCUMENT_NOT_FOUND (400): no matching documents
    // - ERR_ARANGO_COLLECTION_NOT_FOUND (400)
};
```

### 2.6 UPSERT Node

```cpp
class UpsertNode : public MutationNode {
  public:
    UpsertNode(std::string collection, 
               AQLExpression* searchExpr,
               AQLExpression* insertExpr,
               AQLExpression* updateExpr);
    
    // Search condition (typically {_key: value})
    AQLExpression* getSearchExpression() const;
    
    // Insert if not found
    AQLExpression* getInsertExpression() const;
    
    // Update if found
    AQLExpression* getUpdateExpression() const;
    
    // Error codes: union of INSERT + UPDATE error codes
};
```

---

## 3. TokenType Extensions

The AQL tokenizer must recognize mutation keywords:

```cpp
// include/query/aql_token.h

enum class TokenType {
    // ... existing tokens ...
    
    // Mutation keywords
    INSERT,      ///< INSERT keyword
    INTO,        ///< INTO keyword (part of INSERT/REPLACE)
    UPDATE,      ///< UPDATE keyword
    REPLACE,     ///< REPLACE keyword
    REMOVE,      ///< REMOVE keyword (vs. DELETE for SQL compat)
    UPSERT,      ///< UPSERT keyword
    SET,         ///< SET keyword (UPDATE SET field=value)
    
    // Mutation modifiers
    VALUES,      ///< VALUES keyword (INSERT/REPLACE VALUES)
    IGNORE,      ///< IGNORE keyword (IGNORE DUPLICATES)
    DUPLICATES,  ///< DUPLICATES keyword
};
```

---

## 4. Parser Method Signatures

All mutation parsers are methods on `AQLParser` class.

```cpp
// include/query/aql_parser.h (extensions)

class AQLParser {
  public:
    /// Parse INSERT INTO collection VALUES {...}
    /// @param position Current token position (advanced past "INSERT")
    /// @return InsertNode* or nullptr on parse error
    /// @throws ParseException on malformed input
    InsertNode* parseInsert();
    
    /// Parse UPDATE collection SET field=value WHERE ...
    /// @return UpdateNode* or nullptr
    UpdateNode* parseUpdate();
    
    /// Parse REPLACE INTO collection VALUES {...}
    /// @return ReplaceNode* or nullptr
    ReplaceNode* parseReplace();
    
    /// Parse REMOVE FROM collection WHERE ...
    /// @return RemoveNode* or nullptr
    RemoveNode* parseRemove();
    
    /// Parse UPSERT INTO collection SEARCH {...} INSERT {...} UPDATE {...}
    /// @return UpsertNode* or nullptr
    UpsertNode* parseUpsert();
};
```

---

## 5. Reference Implementation Pattern

Mutations parser shall follow the same recursive-descent pattern as `sql_parser.cpp`:

1. **Tokenization**: Break input into TokenType sequence (handled by existing lexer)
2. **Recursive Descent**: parseInsert() → parseExpression() → ... (left-associative)
3. **AST Construction**: Build typed AST nodes during parse (not post-process)
4. **Error Reporting**: Collect all parse errors; don't stop at first error (error recovery)

### Example: parseInsert()

```cpp
InsertNode* AQLParser::parseInsert() {
    // Current token: INSERT (already consumed by caller)
    if (!match(TokenType::INTO)) {
        error("Expected INTO after INSERT");
        return nullptr;
    }
    
    // Expect collection identifier
    if (currentToken().type != TokenType::IDENTIFIER) {
        error("Expected collection name");
        return nullptr;
    }
    std::string collection = currentToken().value;
    advance();
    
    // Expect VALUES or {...}
    if (!match(TokenType::VALUES)) {
        error("Expected VALUES");
        return nullptr;
    }
    
    AQLExpression* valuesExpr = parseExpression();
    if (!valuesExpr) {
        error("Expected document(s) after VALUES");
        return nullptr;
    }
    
    auto node = new InsertNode(collection, valuesExpr);
    
    // Optional: IGNORE DUPLICATES
    if (match(TokenType::IGNORE)) {
        if (!match(TokenType::DUPLICATES)) {
            error("Expected DUPLICATES after IGNORE");
        }
        node->setIgnoreDuplicates(true);
    }
    
    return node;
}
```

---

## 6. Error Codes

All mutation errors are categorized by HTTP semantics:

### 4xx Validation Errors (Client Fault)

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 400 | BAD_REQUEST | Syntax error, invalid collection, type mismatch | Retry with corrected query |
| 400 | ARANGO_COLLECTION_NOT_FOUND | Target collection doesn't exist | Create collection first |
| 400 | ARANGO_DOCUMENT_NOT_FOUND | UPDATE/REMOVE target not found | Verify WHERE clause |
| 400 | ARANGO_DOCUMENT_TYPE_INVALID | Document structure invalid | Correct document format |
| 403 | WRITE_CONFLICT | Concurrent update detected (optimistic lock) | Retry transaction |
| 409 | DUPLICATE_KEY | Unique constraint violation | Verify _key uniqueness |

### 5xx Runtime Errors (Server Fault)

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 500 | INTERNAL_ERROR | Unexpected error in executor | Retry; escalate if persists |
| 500 | INDEX_UPDATE_FAILED | Secondary index update failed | Rebuild index; check disk space |
| 500 | TRANSACTION_ROLLBACK | Transaction aborted mid-execution | Rollback all changes; retry |

---

## 7. Validation Rules (Phase 2 Validator)

All mutations must satisfy these invariants before execution:

### 7.1 Collection Validation
- Target collection MUST exist in CatalogManager
- Target collection MUST have at least one shard
- Target collection MUST NOT be a view (views are read-only)

### 7.2 Document Validation
- Insert document MUST be valid JSON object
- All custom fields MUST pass schema validation (if schema exists)
- _key field (if present) MUST be unique across collection
- _rev field (if present) MUST be valid revision format

### 7.3 Index Validation
- INSERT/REPLACE MUST trigger secondary index updates
- UPDATE/REMOVE MUST trigger index removal/addition for affected indexes
- Unique indexes MUST enforce uniqueness (error on violation)

### 7.4 Transaction Validation
- All mutations in a transaction MUST be atomic (all-or-nothing)
- Rollback MUST restore all affected indexes
- WAL MUST have entries for all mutations in transaction

---

## 8. Integration Points

Mutations must integrate with these existing systems:

### 8.1 Parser Integration
- `AQLParser::parseStatement()` — recognizes INSERT/UPDATE/REPLACE/REMOVE/UPSERT
- Returns typed `MutationNode*` (not generic `AQLNode*`)

### 8.2 Executor Integration
- `QueryExecutor::execute(MutationNode*)` → delegates to `MutationExecutor`
- `MutationExecutor::executeMutation(MutationNode*)` → mutation logic

### 8.3 Storage Integration
- RocksDB `put()` for INSERT/REPLACE
- RocksDB `delete()` for REMOVE
- RocksDB `get()` + `put()` for UPDATE

### 8.4 Transaction Integration
- `TransactionContext::beginTransaction()` / `commit()` / `rollback()`
- Mutations use transaction context for WAL logging
- Rollback must undo all RocksDB operations

### 8.5 Index Integration
- `IndexManager::updateIndex()` on every mutation
- Unique index constraint checks in validator
- Index removal on DELETE; index addition on INSERT/REPLACE

---

## 9. Performance Targets (Phase 5 Gates)

Mutations MUST meet these performance targets at scale:

| Operation | Target | Basis |
|-----------|--------|-------|
| INSERT single doc | ≤ 10ms | per-document RocksDB + index latency |
| INSERT batch (1K docs) | ≤ 500ms | 0.5ms per doc in transaction |
| UPDATE single doc | ≤ 15ms | read + modify + write cycle |
| REPLACE single doc | ≤ 15ms | delete + insert cycle |
| REMOVE single doc | ≤ 8ms | RocksDB delete + index cleanup |
| UPSERT single doc | ≤ 20ms | read + conditional insert/update |

---

## 10. Test Requirements (Phase 4)

Phase 2 implementation must be accompanied by these test categories:

### Parser Tests (50+)
- Valid INSERT/UPDATE/REPLACE/REMOVE/UPSERT syntax
- Error cases (missing keywords, malformed expressions)
- Edge cases (empty documents, large batches)

### Validator Tests (30+)
- Collection existence checks
- Uniqueness constraint violations
- Type validation errors

### Executor Tests (50+)
- Single-document mutations
- Batch mutations
- Transaction semantics (atomicity, rollback)
- Index update verification

### Integration Tests (100+)
- INSERT → subsequent SELECT returns document
- UPDATE → field values changed correctly
- REPLACE → old fields removed, new fields added
- REMOVE → document deleted
- UPSERT → insert if not found, update if found

---

## 11. Dependency Tree

```
MUTATIONS Phase 1-6
├── Phase 1: Design / API Contract (THIS DOCUMENT) ✓
├── Phase 2: Core Implementation
│   ├── Week 3-4: Parser (parseInsert/Update/Replace/Remove/Upsert)
│   ├── Week 5-6: Validator (constraint checking)
│   ├── Week 7-10: Executor (RocksDB integration)
│   ├── Week 11-13: Transaction integration (WAL + rollback)
│   └── Week 14-15: Documentation + hardening
├── Phase 3: Error Handling
├── Phase 4: Tests (1000+ total across all 4 features)
├── Phase 5: Performance Hardening
└── Phase 6: Documentation & Acceptance
```

---

## 12. Approval & Sign-Off

This contract MUST be approved by:

- [ ] **Architecture Review**: Tech Lead (verify AST design is backward-compatible)
- [ ] **Parser Lead**: Confirm TokenType + parser methods follow existing patterns
- [ ] **Storage Lead**: Confirm RocksDB integration points
- [ ] **PM**: Confirm dependencies on other tracks (DDL, Geospatial)

**Approval Status**: PENDING SIGN-OFF

**Approved By**: _______________  
**Date**: _______________  
**Changes Since Frozen**: None (this is the frozen version)

---

## 13. Amendments Log

| Date | Amendment | Approved By |
|------|-----------|-------------|
| 2026-08-06 | Initial contract created | PENDING |

(Amendments require explicit written approval and update to this section)

