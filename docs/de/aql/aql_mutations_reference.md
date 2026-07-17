# AQL Mutations — API Reference

**Version:** 2.0.0 (EPIC-004 Phase 5)  
**Status:** ✅ Production-ready  
**Target Release:** ThemisDB v2.0.0 (Q3 2026)

---

## Headers

| Header | Purpose |
|--------|---------|
| `include/query/aql_parser.h` | Parsing — `AQLParser::parseMutation()`, `parseTransactionBlock()`, AST node types |
| `include/query/aql_mutation_validator.h` | Semantic validation — `AqlMutationValidator::validate()` |
| `include/query/aql_translator.h` | Translation — `AqlMutationTranslator::translate()` |
| `include/query/mutation_execution_plan.h` | Plan types — `MutationExecutionPlan`, `MutationStep`, `MutationResult` |
| `include/query/mutation_executor.h` | Execution — `MutationExecutor::execute()`, `StorageContext` |
| `include/query/mutation_transaction.h` | Transactions — `MutationTransactionContext`, `MutationUndoEntry` |

---

## AQLParser — Mutation Methods

### `parseMutation()`

```cpp
[[nodiscard]] std::shared_ptr<MutationNode>
    parseMutation(const std::string& input);
```

Parse a single DML statement.

**Parameters:**
- `input` — AQL DML string (`INSERT … INTO`, `UPDATE … IN`, `REMOVE … IN`, `REPLACE … WITH … IN`, `UPSERT … INSERT … UPDATE … IN`).

**Returns:** Shared pointer to the parsed `MutationNode` subtype, or `nullptr` on syntax error.

**Thread safety:** Stateless — safe for concurrent invocation.

---

### `parseTransactionBlock()`

```cpp
[[nodiscard]] Result<AqlTransactionBlock>
    parseTransactionBlock(const std::string& input);
```

Parse a `BEGIN … COMMIT/ROLLBACK` block containing any mix of read queries and DML mutations.

**Parameters:**
- `input` — Full transaction block string.

**Returns:**
- `Ok(AqlTransactionBlock)` on success.
- `Err(ParseError)` if the block contains a syntax error.

**AqlTransactionBlock fields:**

| Field | Type | Description |
|-------|------|-------------|
| `ordered_statements` | `std::vector<AqlStatement>` | Ordered mix of queries and mutations (Phase 4+). |
| `statements` | `std::vector<std::shared_ptr<Query>>` | Legacy: read-only queries (backward compat). |
| `action` | `AqlTransactionAction` | `Commit` or `Rollback`. |

---

## AqlMutationValidator

```cpp
class AqlMutationValidator {
public:
    [[nodiscard]] MutationValidationResult validate(const MutationNode& node) const;
    [[nodiscard]] bool isValidCollectionName(std::string_view name) const;
};
```

### `validate()`

Performs semantic validation on a parsed `MutationNode`.

**Parameters:**
- `node` — A `MutationNode` (or subtype) returned by `AQLParser::parseMutation()`.

**Returns:** `MutationValidationResult` with:
- `valid` — `false` if any error is present.
- `errors` — Blocking error messages.
- `warnings` — Non-blocking warnings (e.g., full-collection `REMOVE`).

**Validation rules by type:**

| Type | Rules |
|------|-------|
| `INSERT` | Collection non-empty; document list non-empty. |
| `UPDATE` | Collection non-empty; at least one SET clause or `update_expr`. |
| `REMOVE` | Collection non-empty; missing filter emits warning. |
| `REPLACE` | Collection non-empty; `search_expr` and `replacement` non-null. |
| `UPSERT` | Collection non-empty; `search_expr`, `insert_doc`, `update_doc` non-null. |

---

## AqlMutationTranslator

```cpp
class AqlMutationTranslator {
public:
    [[nodiscard]] query::MutationExecutionPlan
        translate(const std::shared_ptr<query::MutationNode>& node) const;
};
```

### `translate()`

Converts a validated `MutationNode` into a `MutationExecutionPlan`.

**Parameters:**
- `node` — Validated `MutationNode` shared pointer.  `nullptr` input returns an error plan.

**Returns:** `MutationExecutionPlan` with `steps` ordered for `MutationExecutor::execute()`.

**Failure handling:** If `node` is `nullptr` an error plan is returned (non-empty `error_code`, single `ValidatePredicate` error step).

---

## MutationExecutor

```cpp
class MutationExecutor {
public:
    [[nodiscard]] MutationResult
        execute(const MutationExecutionPlan& plan, StorageContext& ctx) const;
};
```

### `execute()`

Executes a `MutationExecutionPlan` against a `StorageContext`.

**Parameters:**
- `plan` — Execution plan from `AqlMutationTranslator::translate()`.
- `ctx`  — Storage context implementation (must not be null).

**Returns:** `MutationResult`:

| Field | Type | Description |
|-------|------|-------------|
| `success` | `bool` | `true` when all steps succeeded. |
| `affected_count` | `int64_t` | Documents inserted / updated / removed. |
| `inserted_ids` | `std::vector<std::string>` | `_key` values for new documents. |
| `errors` | `std::vector<std::string>` | Error messages on failure. |
| `error_code` | `std::string` | Machine-readable code (empty on success). |

### `StorageContext` interface

Implement this to connect the executor to any storage backend:

```cpp
struct StorageContext {
    virtual bool put(std::string_view collection,
                     std::string_view key, std::string_view value) = 0;
    virtual bool remove(std::string_view collection, std::string_view key) = 0;
    virtual bool exists(std::string_view collection, std::string_view key) = 0;
    virtual std::string generateKey(std::string_view collection) = 0;
    virtual bool writeWAL(std::string_view collection, const nlohmann::json& entry) = 0;
    // Optional — override for full rollback fidelity:
    virtual std::optional<std::string> get(std::string_view collection,
                                            std::string_view key);
};
```

`get()` defaults to `std::nullopt`.  Implement it in production contexts to enable
full pre-mutation value capture for `MutationTransactionContext` rollback.

---

## MutationTransactionContext

```cpp
class MutationTransactionContext : public MutationExecutor::StorageContext {
public:
    explicit MutationTransactionContext(MutationExecutor::StorageContext& underlying);

    // StorageContext overrides — all forward to underlying and record undo entries
    bool put(std::string_view, std::string_view, std::string_view) override;
    bool remove(std::string_view, std::string_view) override;
    bool exists(std::string_view, std::string_view) override;
    std::string generateKey(std::string_view) override;
    bool writeWAL(std::string_view, const nlohmann::json&) override;
    std::optional<std::string> get(std::string_view, std::string_view) override;

    // Transaction control
    void rollback();
    [[nodiscard]] bool        empty() const noexcept;
    [[nodiscard]] std::size_t size()  const noexcept;
};
```

### `rollback()`

Reverses all recorded mutations in LIFO order, then clears the undo log.

**Undo semantics:**

| Compensated op | Undo action |
|---------------|-------------|
| `put()` on new key | `remove()` the key |
| `put()` on existing key | `put()` original value back |
| `remove()` on existing key | `put()` original value back |

Rollback fidelity for update/remove scenarios requires the underlying `StorageContext`
to implement `get()`.

**Idempotent:** calling `rollback()` on an empty undo log is a safe no-op.

---

## MutationUndoEntry

```cpp
struct MutationUndoEntry {
    enum class Op { Delete, Put, Insert };
    Op          op;
    std::string collection;
    std::string key;
    std::string original_value;  // Non-empty for Put and Insert ops.
};
```

---

## AST Node Types

All AST nodes inherit from `MutationNode` and are returned by `AQLParser::parseMutation()`.

| Class | DML statement | Key fields |
|-------|--------------|-----------|
| `InsertNode` | INSERT | `collection`, `documents` (list of expression pointers) |
| `UpdateNode` | UPDATE | `collection`, `set_clauses`, `update_expr`, `filter_expr` |
| `RemoveNode` | REMOVE | `collection`, `filter_expr` |
| `ReplaceNode` | REPLACE | `collection`, `search_expr`, `replacement` |
| `UpsertNode` | UPSERT | `collection`, `search_expr`, `insert_doc`, `update_doc` |

All nodes expose:
- `getType()` → `ASTNodeType`
- `toJSON()` → `nlohmann::json`

---

## See Also

- [`aql_mutations_guide.md`](aql_mutations_guide.md) — Syntax guide with examples
- [`AQL_MUTATIONS_ROADMAP.md`](../../../src/query/AQL_MUTATIONS_ROADMAP.md) — Implementation phases
- [`test_aql_mutations_phase5.cpp`](../../../tests/aql/test_aql_mutations_phase5.cpp) — Phase 5 reference tests
