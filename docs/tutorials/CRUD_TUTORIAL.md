# CRUD Operations Tutorial

Master Create, Read, Update, and Delete operations in ThemisDB. This tutorial covers everything from basic operations to advanced patterns.

## 🎯 What You'll Learn

- ✅ All CRUD operation variations
- ✅ Conditional updates and deletes
- ✅ Upserts and merge operations
- ✅ Batch CRUD operations
- ✅ Transaction management
- ✅ Version control and conflict resolution

**Prerequisites:** Complete [Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md)  
**Time Required:** 30 minutes  
**Difficulty:** Beginner to Intermediate

---

## Table of Contents

1. [Create Operations](#create-operations)
2. [Read Operations](#read-operations)
3. [Update Operations](#update-operations)
4. [Delete Operations](#delete-operations)
5. [Advanced Patterns](#advanced-patterns)
6. [Best Practices](#best-practices)

---

## Create Operations

### Basic Create

```bash
# Create a single entity
curl -X PUT http://localhost:8080/entities/products:laptop-001 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"ThinkPad X1\",\"price\":1299.99,\"stock\":15,\"category\":\"electronics\"}"
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "products:laptop-001",
  "version": 1,
  "created_at": "2025-01-24T10:00:00Z"
}
```

### Create with Attributes

Instead of storing everything in a blob, use structured attributes:

```bash
curl -X PUT http://localhost:8080/entities/products:laptop-002 \
  -H "Content-Type: application/json" \
  -d '{
    "attributes": {
      "name": "MacBook Pro",
      "price": 2399.99,
      "stock": 8,
      "category": "electronics",
      "tags": ["premium", "apple", "laptop"]
    }
  }'
```

**Advantages:**
- Individual attribute indexing
- Selective updates
- Better query performance

### Conditional Create (Create If Not Exists)

```bash
# Only create if entity doesn't exist
curl -X PUT http://localhost:8080/entities/products:laptop-003 \
  -H "Content-Type: application/json" \
  -H "If-None-Match: *" \
  -d '{
    "blob": "{\"name\":\"Dell XPS 15\",\"price\":1899.99,\"stock\":12}"
  }'
```

**Response if entity exists:**
```json
{
  "status": "error",
  "code": "CONFLICT",
  "message": "Entity already exists"
}
```

### Batch Create

```bash
# Create multiple entities in one request
curl -X POST http://localhost:8080/batch/create \
  -H "Content-Type: application/json" \
  -d '{
    "entities": [
      {
        "entity_id": "products:mouse-001",
        "blob": "{\"name\":\"Logitech MX Master\",\"price\":99.99,\"stock\":50}"
      },
      {
        "entity_id": "products:keyboard-001",
        "blob": "{\"name\":\"Keychron K2\",\"price\":79.99,\"stock\":30}"
      },
      {
        "entity_id": "products:monitor-001",
        "blob": "{\"name\":\"Dell U2720Q\",\"price\":599.99,\"stock\":10}"
      }
    ]
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "created": 3,
  "results": [
    {"entity": "products:mouse-001", "version": 1},
    {"entity": "products:keyboard-001", "version": 1},
    {"entity": "products:monitor-001", "version": 1}
  ]
}
```

**Performance:** Batch creates are 50-100x faster than individual requests!

---

## Read Operations

### Basic Read

```bash
# Read single entity
curl http://localhost:8080/entities/products:laptop-001
```

**Expected Output:**
```json
{
  "entity_id": "products:laptop-001",
  "version": 1,
  "blob": "{\"name\":\"ThinkPad X1\",\"price\":1299.99,\"stock\":15}",
  "created_at": "2025-01-24T10:00:00Z",
  "updated_at": "2025-01-24T10:00:00Z"
}
```

### Read with Version

```bash
# Read specific version
curl http://localhost:8080/entities/products:laptop-001?version=1
```

### Read Multiple Entities

```bash
curl -X POST http://localhost:8080/batch/read \
  -H "Content-Type: application/json" \
  -d '{
    "entity_ids": [
      "products:laptop-001",
      "products:mouse-001",
      "products:keyboard-001"
    ]
  }'
```

### Read with Projection (Select Specific Fields)

```bash
# Read only specific attributes
curl -X POST http://localhost:8080/entities/products:laptop-001/attributes \
  -H "Content-Type: application/json" \
  -d '{
    "fields": ["name", "price"]
  }'
```

**Expected Output:**
```json
{
  "entity_id": "products:laptop-001",
  "attributes": {
    "name": "ThinkPad X1",
    "price": 1299.99
  }
}
```

### Read with Pattern Matching

```bash
# Read all products (entities starting with "products:")
curl "http://localhost:8080/entities?prefix=products:"
```

### Conditional Read

```bash
# Read only if version matches (for caching)
curl http://localhost:8080/entities/products:laptop-001 \
  -H "If-None-Match: \"version-1\""
```

**Response if unchanged:** `304 Not Modified`

---

## Update Operations

### Basic Update (Replace)

```bash
# Update entire entity
curl -X PUT http://localhost:8080/entities/products:laptop-001 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"ThinkPad X1 Carbon\",\"price\":1399.99,\"stock\":20,\"category\":\"electronics\"}"
  }'
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "products:laptop-001",
  "version": 2,
  "updated_at": "2025-01-24T11:00:00Z"
}
```

### Partial Update (Patch)

```bash
# Update only specific attributes
curl -X PATCH http://localhost:8080/entities/products:laptop-001 \
  -H "Content-Type: application/json" \
  -d '{
    "attributes": {
      "stock": 18,
      "price": 1349.99
    }
  }'
```

**Advantage:** Other attributes remain unchanged!

### Conditional Update (Optimistic Locking)

```bash
# Update only if version matches (prevents race conditions)
curl -X PUT http://localhost:8080/entities/products:laptop-001 \
  -H "Content-Type: application/json" \
  -H "If-Match: \"version-2\"" \
  -d '{
    "blob": "{\"name\":\"ThinkPad X1 Carbon Gen 9\",\"price\":1449.99,\"stock\":18}"
  }'
```

**Response if version mismatch:**
```json
{
  "status": "error",
  "code": "CONFLICT",
  "message": "Version mismatch. Expected 2, got 3"
}
```

### Atomic Increment/Decrement

```bash
# Atomically decrement stock (thread-safe)
curl -X POST http://localhost:8080/entities/products:laptop-001/increment \
  -H "Content-Type: application/json" \
  -d '{
    "attribute": "stock",
    "delta": -1
  }'
```

**Use Case:** Perfect for inventory management!

### Upsert (Update or Insert)

```bash
# Create if doesn't exist, update if exists
curl -X POST http://localhost:8080/entities/products:laptop-004/upsert \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"HP Spectre\",\"price\":1599.99,\"stock\":7}"
  }'
```

### Batch Update

```bash
# Update multiple entities
curl -X POST http://localhost:8080/batch/update \
  -H "Content-Type: application/json" \
  -d '{
    "updates": [
      {
        "entity_id": "products:laptop-001",
        "attributes": {"stock": 15}
      },
      {
        "entity_id": "products:mouse-001",
        "attributes": {"price": 89.99}
      }
    ]
  }'
```

---

## Delete Operations

### Basic Delete

```bash
# Delete an entity
curl -X DELETE http://localhost:8080/entities/products:laptop-004
```

**Expected Output:**
```json
{
  "status": "success",
  "entity": "products:laptop-004",
  "deleted": true
}
```

### Conditional Delete

```bash
# Delete only if version matches
curl -X DELETE http://localhost:8080/entities/products:laptop-003 \
  -H "If-Match: \"version-1\""
```

### Soft Delete

```bash
# Mark as deleted without removing data
curl -X PATCH http://localhost:8080/entities/products:laptop-002 \
  -H "Content-Type: application/json" \
  -d '{
    "attributes": {
      "deleted": true,
      "deleted_at": "2025-01-24T12:00:00Z"
    }
  }'
```

**Advantage:** Can recover deleted data!

### Batch Delete

```bash
# Delete multiple entities
curl -X POST http://localhost:8080/batch/delete \
  -H "Content-Type: application/json" \
  -d '{
    "entity_ids": [
      "products:laptop-003",
      "products:laptop-004"
    ]
  }'
```

### Delete by Query

```bash
# Delete all out-of-stock products
curl -X POST http://localhost:8080/query/delete \
  -H "Content-Type: application/json" \
  -d '{
    "table": "products",
    "predicates": [
      {
        "column": "stock",
        "operator": "=",
        "value": 0
      }
    ]
  }'
```

**⚠️ Warning:** Use with caution! Always test with a SELECT query first.

---

## Advanced Patterns

### Transaction-Safe CRUD

```bash
# Start transaction
tx_response=$(curl -X POST http://localhost:8080/tx/begin)
tx_id=$(echo $tx_response | jq -r '.tx_id')

# Create within transaction
curl -X PUT http://localhost:8080/tx/$tx_id/entities/orders:order-001 \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"customer\":\"alice\",\"total\":129.99,\"items\":[\"laptop-001\"]}"
  }'

# Update stock within same transaction
curl -X PATCH http://localhost:8080/tx/$tx_id/entities/products:laptop-001 \
  -H "Content-Type: application/json" \
  -d '{
    "attributes": {"stock": 14}
  }'

# Commit transaction (all or nothing)
curl -X POST http://localhost:8080/tx/$tx_id/commit
```

### Merge Strategy

```bash
# Merge with conflict resolution
curl -X POST http://localhost:8080/entities/products:laptop-001/merge \
  -H "Content-Type: application/json" \
  -d '{
    "attributes": {
      "description": "High-performance laptop",
      "warranty": "3 years"
    },
    "strategy": "merge",
    "on_conflict": "keep_newer"
  }'
```

### Copy Entity

```bash
# Create a copy with new ID
curl -X POST http://localhost:8080/entities/products:laptop-001/copy \
  -H "Content-Type: application/json" \
  -d '{
    "new_entity_id": "products:laptop-001-backup"
  }'
```

### Bulk Import from JSON

```bash
# Import from file
curl -X POST http://localhost:8080/import \
  -H "Content-Type: application/json" \
  -d @products.json
```

**products.json:**
```json
{
  "entities": [
    {"entity_id": "products:item1", "blob": "{...}"},
    {"entity_id": "products:item2", "blob": "{...}"}
  ]
}
```

---

## Best Practices

### 1. Use Batch Operations

❌ **Bad:**
```bash
for i in {1..100}; do
  curl -X PUT http://localhost:8080/entities/user:$i -d '{...}'
done
```

✅ **Good:**
```bash
curl -X POST http://localhost:8080/batch/create -d '{
  "entities": [/* all 100 entities */]
}'
```

**Performance:** 100x faster!

### 2. Always Handle Errors

```bash
response=$(curl -s -w "\n%{http_code}" -X PUT http://localhost:8080/entities/test -d '{}')
body=$(echo "$response" | sed '$d')
http_code=$(echo "$response" | tail -n1)

if [ "$http_code" != "200" ]; then
  echo "Error: $body"
  exit 1
fi
```

### 3. Use Optimistic Locking for Consistency

```bash
# Read current version
entity=$(curl http://localhost:8080/entities/products:laptop-001)
version=$(echo $entity | jq -r '.version')

# Update with version check
curl -X PUT http://localhost:8080/entities/products:laptop-001 \
  -H "If-Match: \"version-$version\"" \
  -d '{...}'
```

### 4. Prefer Partial Updates

```bash
# Only update what changed
curl -X PATCH http://localhost:8080/entities/products:laptop-001 \
  -d '{"attributes": {"price": 1299.99}}'
```

### 5. Use Transactions for Related Operations

```bash
# All succeed or all fail
curl -X POST http://localhost:8080/tx/begin
# ... multiple operations ...
curl -X POST http://localhost:8080/tx/$tx_id/commit
```

---

## Common Pitfalls

### ❌ Not Checking If Entity Exists

```bash
# This silently overwrites existing entity!
curl -X PUT http://localhost:8080/entities/user:alice -d '{...}'
```

✅ **Solution:**
```bash
curl -X PUT http://localhost:8080/entities/user:alice \
  -H "If-None-Match: *" \
  -d '{...}'
```

### ❌ Race Conditions on Updates

```bash
# Two clients update simultaneously - one wins, one loses data
```

✅ **Solution:** Use versioning
```bash
curl -X PUT ... -H "If-Match: \"version-5\"" ...
```

### ❌ Forgetting to Handle 404s

```bash
curl http://localhost:8080/entities/nonexistent
# Returns 404, but script continues
```

✅ **Solution:** Check HTTP status codes

---

## Performance Tips

### 1. Batch Size

- Optimal batch size: 100-1000 entities
- Too small: Network overhead
- Too large: Memory pressure

### 2. Use Projections

```bash
# Read only needed fields
curl -X POST .../attributes -d '{"fields": ["name", "price"]}'
```

### 3. Avoid Full Entity Scans

```bash
# Slow: curl "http://localhost:8080/entities?prefix=products:"
# Fast: Use query with index
```

### 4. Monitor with Metrics

```bash
curl http://localhost:4318/metrics | grep crud_operations
```

---

## Testing Your Knowledge

Try these exercises:

1. **Exercise 1:** Create 10 products with batch operation
2. **Exercise 2:** Update price of 5 products conditionally
3. **Exercise 3:** Delete all products with stock = 0
4. **Exercise 4:** Implement safe stock decrement with transactions
5. **Exercise 5:** Merge two entity versions

**Solutions:** See [Interactive Examples](INTERACTIVE_EXAMPLES.md)

---

## What You've Learned ✅

- ✅ All CRUD operation variations
- ✅ Batch operations for performance
- ✅ Optimistic locking for consistency
- ✅ Transaction management
- ✅ Advanced patterns (upsert, merge, atomic ops)
- ✅ Best practices and common pitfalls

---

## Next Steps

1. **[Batch Operations Guide](BATCH_OPERATIONS.md)** - Deep dive into batch processing
2. **[Schema Design Tutorial](SCHEMA_DESIGN.md)** - Design optimal schemas
3. **[Best Practices Guide](BEST_PRACTICES.md)** - Production patterns
4. **Try Examples:** [Todo App](../../examples/02_todo_app/)

---

**Need help?** See [FAQ](../FAQ.md) or [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
