# AQL Mutations Guide

**Version:** 2.0.0 (EPIC-004 Phase 5)  
**Status:** ✅ Production-ready  
**Target Release:** ThemisDB v2.0.0 (Q3 2026)

---

## Overview

AQL 2.0.0 extends the query language with full DML (Data Manipulation Language) support.
You can now use `INSERT`, `UPDATE`, `REPLACE`, `REMOVE`, and `UPSERT` statements alongside
existing read queries (`FOR`, `FILTER`, `RETURN`) — including within multi-statement
transaction blocks.

**Backward compatibility:** All AQL v1 read queries work unchanged.
DML keywords are only active when `ValidationMode::AllowMutations` is set on the parser.

---

## Statement Reference

### INSERT

Insert one or more documents into a collection.

```aql
INSERT { name: "Alice", age: 30 } INTO users
```

Multiple documents:

```aql
INSERT [
  { name: "Bob",   role: "admin"  },
  { name: "Carol", role: "viewer" }
] INTO users
```

The `_key` field is optional. When omitted the storage layer generates a unique key.

### UPDATE

Update fields of existing documents.

```aql
UPDATE { _key: "user_1", name: "Alice B." } IN users
```

You can also use `SET` clause syntax:

```aql
UPDATE user_1 WITH { active: false } IN sessions
```

> **Note:** An `UPDATE` without a filter predicate targets a specific document by key.
> Use `FOR … FILTER … UPDATE` for predicate-based updates (EPIC-004 Q4 2026).

### REPLACE

Replace the entire document identified by key with a new document.

```aql
REPLACE "user_1" WITH { name: "Alice C.", role: "editor" } IN users
```

> **Difference from UPDATE:** `REPLACE` overwrites the whole document;
> `UPDATE` merges fields.

### REMOVE

Delete a document by key.

```aql
REMOVE "user_1" IN users
```

> ⚠️ **Warning:** `REMOVE` without a filter predicate targets a specific key.
> Omitting the key identifier removes the first matched document and emits a
> validator warning. Always specify a key or predicate.

### UPSERT

Insert a document if the search expression is not found; update it otherwise.

```aql
UPSERT { _key: "user_1" }
INSERT { name: "Dave", active: true }
UPDATE { active: true }
IN users
```

- **Insert branch:** executed when the document with `_key: "user_1"` does not exist.
- **Update branch:** executed when the document already exists (merges the update document).

---

## Transaction Blocks

Multiple statements — including mutations — can be grouped in an atomic transaction block.

```aql
BEGIN
  INSERT { name: "Eve" } INTO users
  REMOVE "old_user" IN users
COMMIT
```

All statements in a `BEGIN … COMMIT` block are executed atomically:
- If any statement fails, **all previously applied mutations are rolled back** via the undo log.
- Ending with `ROLLBACK` discards all changes without execution.

```aql
BEGIN
  INSERT { name: "Frank" } INTO users
ROLLBACK
```

---

## Error Codes

| Code | Meaning |
|------|---------|
| `COLLECTION_EMPTY` | Collection name is missing or empty. |
| `NO_DOCUMENTS` | INSERT document list is empty. |
| `NO_SET_CLAUSES` | UPDATE has no SET clauses and no update expression. |
| `NULL_SEARCH_EXPR` | REPLACE or UPSERT search expression is null. |
| `NULL_UPDATE_DOC` | UPSERT update_doc is null. |
| `FULL_REMOVE_WARNING` | REMOVE without filter (non-blocking warning). |

---

## Safety Validator

The `AqlMutationValidator` runs semantic checks after parsing:

```cpp
AqlMutationValidator validator;
auto result = validator.validate(*mutation_node);
if (!result.valid) {
    for (const auto& err : result.errors) {
        std::cerr << err << '\n';
    }
}
for (const auto& warn : result.warnings) {
    std::cerr << "[warn] " << warn << '\n';
}
```

---

## C++ Integration — Quick Start

```cpp
#include "query/aql_parser.h"
#include "query/aql_mutation_validator.h"
#include "query/aql_translator.h"
#include "query/mutation_executor.h"

using namespace themis::query;

// 1. Parse
AQLParser parser;
auto node = parser.parseMutation("INSERT {name:'Test'} INTO col");
if (!node) { /* handle parse error */ }

// 2. Validate
AqlMutationValidator validator;
auto vr = validator.validate(*node);
if (!vr.valid) { /* handle errors */ }

// 3. Translate
AqlMutationTranslator translator;
auto plan = translator.translate(node);

// 4. Execute
MutationExecutor executor;
MyStorageContext ctx;           // implements MutationExecutor::StorageContext
auto result = executor.execute(plan, ctx);
if (!result.success) { /* handle errors */ }
```

---

## Transaction Integration

Use `MutationTransactionContext` to wrap any `StorageContext` with undo-log support:

```cpp
#include "query/mutation_transaction.h"

MyStorageContext underlying;
MutationTransactionContext txn(underlying);

// Execute mutations through txn instead of directly through underlying
auto result = executor.execute(plan, txn);
if (!result.success) {
    txn.rollback();    // Reverses all mutations applied through txn
}
// On success: no explicit commit needed — changes are already in underlying.
```

---

## Performance Targets (Phase 5)

| Operation | Target Latency | Notes |
|-----------|---------------|-------|
| Parse INSERT | < 100 µs | Single-statement, typical doc |
| Validate INSERT | < 10 µs | Stateless |
| Translate INSERT | < 5 µs | Stateless |
| Execute INSERT (in-memory) | < 1 µs | MockStorageContext |
| Execute INSERT (RocksDB) | < 5 ms | Sync write |
| Batch INSERT 1 000 docs | < 500 ms | RocksDB WriteBatch |
| Transaction overhead | < 2x non-transactional | Undo log recording |

See `benchmarks/aql/bench_aql_mutations.cpp` for the full benchmark suite.

---

## See Also

- [`aql_mutations_reference.md`](aql_mutations_reference.md) — API reference for C++ headers
- [`AQL_MUTATIONS_ROADMAP.md`](../../../src/query/AQL_MUTATIONS_ROADMAP.md) — Implementation phases
- [`AQL_SYNTAX_GUIDE.md`](AQL_SYNTAX_GUIDE.md) — AQL v1 read-query syntax
