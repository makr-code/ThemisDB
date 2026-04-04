# Schema Design Tutorial

Master the art of designing optimal database schemas for ThemisDB. Learn multi-model design patterns, normalization strategies, and real-world examples.

## 🎯 What You'll Learn

- ✅ Multi-model schema design principles
- ✅ Normalization vs. denormalization trade-offs
- ✅ Index strategy and placement
- ✅ Relationship modeling patterns
- ✅ Real-world schema examples
- ✅ Performance optimization techniques

**Prerequisites:** [CRUD Tutorial](CRUD_TUTORIAL.md), [Batch Operations](BATCH_OPERATIONS.md)  
**Time Required:** 45 minutes  
**Difficulty:** Intermediate to Advanced

---

## Table of Contents

1. [Design Principles](#design-principles)
2. [Entity-Attribute Model](#entity-attribute-model)
3. [Normalization Strategies](#normalization-strategies)
4. [Relationship Patterns](#relationship-patterns)
5. [Index Design](#index-design)
6. [Real-World Examples](#real-world-examples)
7. [Migration Patterns](#migration-patterns)

---

## Design Principles

### 1. Understand Your Access Patterns

**Before designing, answer:**
- How will data be queried most often?
- What are the read/write ratios?
- What are the latency requirements?
- What consistency guarantees are needed?

**Example:**
```
E-commerce scenario:
- Products: 90% reads, 10% writes → Optimize for reads
- Orders: 50% reads, 50% writes → Balance both
- Analytics: 100% reads → Aggressive denormalization OK
```

### 2. Choose the Right Model

ThemisDB supports multiple models. Choose based on use case:

| Model | Best For | Example |
|-------|----------|---------|
| **Key-Value** | Simple lookups | User sessions, cache |
| **Document** | Hierarchical data | Product catalogs, CMS |
| **Graph** | Relationships | Social networks, recommendations |
| **Vector** | Similarity search | Image search, semantic search |

### 3. Start Simple, Evolve

```
✅ Good approach:
1. Start with simple schema
2. Monitor performance
3. Add indexes where needed
4. Denormalize hot paths
5. Split large entities

❌ Bad approach:
1. Over-engineer from day 1
2. Premature optimization
3. Complex schema before understanding access patterns
```

---

## Entity-Attribute Model

### Understanding Entities

In ThemisDB, an **entity** is a unique identifier with attributes:

```
Format: namespace:key
Example: users:alice, products:12345, orders:ord-2024-001
```

**Best Practices:**
- Use meaningful namespaces
- Keep keys human-readable when possible
- Use consistent naming conventions

### Attribute Storage

**Two approaches:**

**1. Blob Storage (Simple)**
```json
{
  "entity_id": "users:alice",
  "blob": "{\"name\":\"Alice\",\"email\":\"alice@example.com\",\"age\":30}"
}
```
- **Pros:** Simple, flexible
- **Cons:** Can't query individual fields efficiently

**2. Structured Attributes (Advanced)**
```json
{
  "entity_id": "users:alice",
  "attributes": {
    "name": "Alice",
    "email": "alice@example.com",
    "age": 30,
    "created_at": "2025-01-24T10:00:00Z"
  }
}
```
- **Pros:** Query individual fields, selective indexing
- **Cons:** Slightly more complex

**💡 Pro Tip:** Start with blobs, migrate to structured attributes when you need to query specific fields.

---

## Normalization Strategies

### Normalized Design (Traditional)

**Example: E-commerce**

```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {
    "name": "Alice Johnson",
    "email": "alice@example.com"
  }
}

// orders:order-001
{
  "entity_id": "orders:order-001",
  "attributes": {
    "user_id": "users:alice",
    "total": 129.99,
    "status": "shipped"
  }
}

// order_items:item-001
{
  "entity_id": "order_items:item-001",
  "attributes": {
    "order_id": "orders:order-001",
    "product_id": "products:laptop-001",
    "quantity": 1,
    "price": 129.99
  }
}
```

**Pros:**
- ✅ No data duplication
- ✅ Easy to update
- ✅ Consistent data

**Cons:**
- ❌ Multiple queries to fetch related data
- ❌ Slower read performance

### Denormalized Design (Performance)

```json
// orders:order-001 (with embedded data)
{
  "entity_id": "orders:order-001",
  "attributes": {
    "user": {
      "id": "users:alice",
      "name": "Alice Johnson",
      "email": "alice@example.com"
    },
    "items": [
      {
        "product_id": "products:laptop-001",
        "product_name": "ThinkPad X1",
        "quantity": 1,
        "price": 129.99
      }
    ],
    "total": 129.99,
    "status": "shipped",
    "created_at": "2025-01-24T10:00:00Z"
  }
}
```

**Pros:**
- ✅ Single query to fetch everything
- ✅ Fast reads
- ✅ Perfect for display

**Cons:**
- ❌ Data duplication
- ❌ Harder to update (must update multiple places)
- ❌ Possible inconsistency

### Hybrid Approach (Recommended)

**Normalize by default, denormalize read-heavy paths:**

```json
// orders:order-001 (hybrid)
{
  "entity_id": "orders:order-001",
  "attributes": {
    "user_id": "users:alice",
    "user_name": "Alice Johnson",  // Denormalized for display
    "item_ids": ["order_items:item-001"],
    "item_summary": "1 item(s)",   // Denormalized for quick display
    "total": 129.99,
    "status": "shipped"
  }
}
```

**When to denormalize:**
- Read-heavy data (90%+ reads)
- Display-only fields (names, summaries)
- Performance-critical paths

**When to normalize:**
- Frequently updated data
- Source of truth
- Data integrity is critical

---

## Relationship Patterns

### One-to-One

**Example: User → Profile**

```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {
    "name": "Alice",
    "email": "alice@example.com",
    "profile_id": "profiles:alice"
  }
}

// profiles:alice
{
  "entity_id": "profiles:alice",
  "attributes": {
    "bio": "Software Engineer",
    "avatar": "https://...",
    "website": "https://alice.dev"
  }
}
```

**Alternative (Embedded):**
```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {
    "name": "Alice",
    "email": "alice@example.com",
    "profile": {
      "bio": "Software Engineer",
      "avatar": "https://...",
      "website": "https://alice.dev"
    }
  }
}
```

### One-to-Many

**Example: User → Orders**

**Approach 1: Reference in child (Normalized)**
```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {"name": "Alice"}
}

// orders:order-001
{
  "entity_id": "orders:order-001",
  "attributes": {
    "user_id": "users:alice",
    "total": 99.99
  }
}

// Query: Find all orders for Alice
POST /query
{
  "table": "orders",
  "predicates": [{"column": "user_id", "value": "users:alice"}]
}
```

**Approach 2: Array of references in parent (Denormalized)**
```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {
    "name": "Alice",
    "order_ids": ["orders:order-001", "orders:order-002"]
  }
}
```

**⚠️ Warning:** Approach 2 doesn't scale for large collections (>1000 items).

### Many-to-Many

**Example: Students ↔ Courses**

**Approach: Junction entity**
```json
// students:alice
{
  "entity_id": "students:alice",
  "attributes": {"name": "Alice"}
}

// courses:cs101
{
  "entity_id": "courses:cs101",
  "attributes": {"name": "Computer Science 101"}
}

// enrollments:alice-cs101
{
  "entity_id": "enrollments:alice-cs101",
  "attributes": {
    "student_id": "students:alice",
    "course_id": "courses:cs101",
    "enrolled_at": "2025-01-01",
    "grade": "A"
  }
}
```

**Query: Find all courses for Alice**
```bash
curl -X POST http://localhost:8080/query \
  -d '{
    "table": "enrollments",
    "predicates": [{"column": "student_id", "value": "students:alice"}],
    "return": "entities"
  }'
```

### Graph Relationships

**For complex relationships, use graph model:**

```bash
# Create edge between entities
curl -X POST http://localhost:8080/graph/edge \
  -d '{
    "from": "users:alice",
    "to": "users:bob",
    "type": "follows",
    "properties": {"since": "2025-01-01"}
  }'

# Query: Find all users Alice follows
curl -X POST http://localhost:8080/graph/traverse \
  -d '{
    "start": "users:alice",
    "direction": "outbound",
    "edge_type": "follows",
    "depth": 1
  }'
```

---

## Index Design

### Index Strategy

**Golden Rules:**
1. Index columns used in WHERE clauses
2. Index columns used in ORDER BY
3. Index foreign key columns
4. Don't over-index (slows writes)

### Index Types

**1. B-Tree Index (General Purpose)**
```bash
# Create B-Tree index on age for range queries
curl -X POST http://localhost:8080/index/create \
  -d '{
    "table": "users",
    "column": "age",
    "type": "btree"
  }'

# Supports: =, !=, <, >, <=, >=, BETWEEN
```

**2. Hash Index (Exact Match)**
```bash
# Create Hash index on email for fast lookups
curl -X POST http://localhost:8080/index/create \
  -d '{
    "table": "users",
    "column": "email",
    "type": "hash"
  }'

# Supports: = only (but VERY fast)
```

**3. Composite Index**
```bash
# Create composite index for multi-column queries
curl -X POST http://localhost:8080/index/create \
  -d '{
    "table": "orders",
    "columns": ["user_id", "status", "created_at"],
    "type": "btree"
  }'

# Optimizes: WHERE user_id=? AND status=? ORDER BY created_at
```

**4. Vector Index (Similarity Search)**
```bash
# Create vector index for embeddings
curl -X POST http://localhost:8080/index/create \
  -d '{
    "table": "documents",
    "column": "embedding",
    "type": "vector",
    "dimension": 768,
    "metric": "cosine"
  }'
```

### Index Placement Strategy

**Example: E-commerce queries**

```
// Common queries:
1. Find product by ID → Hash index on product_id
2. Search products by category → B-Tree index on category
3. Filter by price range → B-Tree index on price
4. Sort by popularity → B-Tree index on sales_count
5. Similar products → Vector index on embedding

Indexes to create:
✅ products.product_id (hash)
✅ products.category (btree)
✅ products.price (btree)
✅ products.(category, price) (composite)
✅ products.embedding (vector)
```

### Avoiding Over-Indexing

**❌ Bad: Too many indexes**
```
users.id (hash)
users.email (hash)
users.name (btree)
users.age (btree)
users.city (btree)
users.country (btree)
users.created_at (btree)
users.updated_at (btree)
// 8 indexes = slow writes!
```

**✅ Good: Strategic indexes**
```
users.id (hash) ← Primary key lookup
users.email (hash) ← Login authentication
users.(city, age) (composite) ← Common filter
// 3 indexes = fast reads + fast writes
```

---

## Real-World Examples

### Example 1: Social Media Platform

**Entities:**
```json
// users:alice
{
  "entity_id": "users:alice",
  "attributes": {
    "username": "alice",
    "display_name": "Alice Johnson",
    "bio": "Software Engineer",
    "follower_count": 1520,  // Denormalized for performance
    "following_count": 342
  }
}

// posts:post-12345
{
  "entity_id": "posts:post-12345",
  "attributes": {
    "author_id": "users:alice",
    "author_name": "Alice Johnson",  // Denormalized
    "content": "Just deployed ThemisDB!",
    "like_count": 45,                 // Denormalized
    "comment_count": 12,              // Denormalized
    "created_at": "2025-01-24T10:00:00Z"
  }
}

// follows:alice-bob (graph edge)
{
  "entity_id": "follows:alice-bob",
  "attributes": {
    "from_user": "users:alice",
    "to_user": "users:bob",
    "created_at": "2025-01-20T08:00:00Z"
  }
}
```

**Indexes:**
```bash
# User lookups
POST /index/create {"table": "users", "column": "username", "type": "hash"}

# Timeline queries (posts by followed users)
POST /index/create {"table": "posts", "columns": ["author_id", "created_at"], "type": "btree"}

# Graph traversal (followers/following)
POST /index/create {"table": "follows", "column": "from_user", "type": "btree"}
POST /index/create {"table": "follows", "column": "to_user", "type": "btree"}
```

**Why this design?**
- Denormalized counts for fast display
- Graph edges for relationship queries
- Composite index for timeline optimization

### Example 2: E-Commerce Store

```json
// products:laptop-001
{
  "entity_id": "products:laptop-001",
  "attributes": {
    "sku": "LAPTOP-001",
    "name": "ThinkPad X1 Carbon",
    "description": "High-performance laptop",
    "price": 1299.99,
    "original_price": 1499.99,
    "discount_percent": 13,
    "category": "Electronics > Laptops",
    "tags": ["business", "ultralight", "premium"],
    "stock": 15,
    "rating_avg": 4.7,           // Denormalized
    "review_count": 234,          // Denormalized
    "embedding": [0.1, 0.2, ...]  // For similar products
  }
}

// orders:order-2024-001
{
  "entity_id": "orders:order-2024-001",
  "attributes": {
    "user_id": "users:alice",
    "user_name": "Alice Johnson",    // Denormalized
    "status": "shipped",
    "total": 1299.99,
    "shipping_address": {...},
    "items": [  // Embedded for performance
      {
        "product_id": "products:laptop-001",
        "product_name": "ThinkPad X1 Carbon",
        "quantity": 1,
        "price": 1299.99
      }
    ],
    "created_at": "2025-01-24T10:00:00Z"
  }
}

// reviews:review-001
{
  "entity_id": "reviews:review-001",
  "attributes": {
    "product_id": "products:laptop-001",
    "user_id": "users:alice",
    "user_name": "Alice J.",         // Partial denormalization
    "rating": 5,
    "title": "Excellent laptop!",
    "content": "Best purchase ever...",
    "verified_purchase": true,
    "created_at": "2025-01-25T14:30:00Z"
  }
}
```

**Indexes:**
```bash
# Product search and filtering
POST /index/create {"table": "products", "column": "category", "type": "btree"}
POST /index/create {"table": "products", "columns": ["category", "price"], "type": "btree"}
POST /index/create {"table": "products", "column": "embedding", "type": "vector", "dimension": 768}

# Order management
POST /index/create {"table": "orders", "columns": ["user_id", "status"], "type": "btree"}
POST /index/create {"table": "orders", "columns": ["status", "created_at"], "type": "btree"}

# Reviews
POST /index/create {"table": "reviews", "column": "product_id", "type": "btree"}
POST /index/create {"table": "reviews", "columns": ["product_id", "created_at"], "type": "btree"}
```

### Example 3: IoT Sensor Network

```json
// sensors:temp-sensor-001
{
  "entity_id": "sensors:temp-sensor-001",
  "attributes": {
    "name": "Temperature Sensor 1",
    "location": "Building A, Floor 3",
    "type": "temperature",
    "unit": "celsius",
    "last_reading": 22.5,       // Denormalized for quick check
    "last_update": "2025-01-24T15:30:00Z",
    "status": "online"
  }
}

// readings:temp-sensor-001-2025-01-24-15-30-00
{
  "entity_id": "readings:temp-sensor-001-2025-01-24-15-30-00",
  "attributes": {
    "sensor_id": "sensors:temp-sensor-001",
    "sensor_type": "temperature",  // Denormalized for queries
    "value": 22.5,
    "timestamp": "2025-01-24T15:30:00Z",
    "quality": "good"
  }
}
```

**Time-Series Optimization:**
```bash
# Partition readings by time (hourly buckets)
# readings:temp-sensor-001-2025-01-24-15
# Contains all readings for that hour

# Index for time-range queries
POST /index/create {"table": "readings", "columns": ["sensor_id", "timestamp"], "type": "btree"}
```

---

## Migration Patterns

### Schema Evolution

**Adding a Field:**
```bash
# Old entities don't need migration - just use default
curl -X GET http://localhost:8080/entities/users:alice
# Check if 'bio' exists, use default if not
```

**Changing Field Type:**
```python
# Migrate string ID to integer
def migrate_user_ids():
    users = query_all_users()
    for user in users:
        old_id = user['attributes']['id']  # String
        new_id = int(old_id)                # Integer
        user['attributes']['id'] = new_id
        update_entity(user)
```

**Splitting an Entity:**
```python
# Split user entity into user + profile
def split_user_profile():
    users = query_all_users()
    for user in users:
        # Create new profile entity
        profile = {
            "entity_id": f"profiles:{user['entity_id'].split(':')[1]}",
            "attributes": {
                "bio": user['attributes'].get('bio'),
                "avatar": user['attributes'].get('avatar')
            }
        }
        create_entity(profile)
        
        # Remove profile fields from user
        del user['attributes']['bio']
        del user['attributes']['avatar']
        update_entity(user)
```

---

## Best Practices

### ✅ DO:

1. **Design for queries, not structure**
   - Understand access patterns first
   - Optimize for common queries

2. **Use namespaces consistently**
   ```
   users:*
   products:*
   orders:*
   ```

3. **Denormalize read-heavy data**
   - Display names
   - Counts and aggregates
   - Frequently accessed relationships

4. **Version your schema**
   ```json
   {
     "entity_id": "users:alice",
     "schema_version": 2,
     "attributes": {...}
   }
   ```

5. **Monitor and iterate**
   - Start simple
   - Add indexes as needed
   - Refactor based on metrics

### ❌ DON'T:

1. **Don't over-normalize**
   - Causes excessive joins
   - Poor read performance

2. **Don't create indexes blindly**
   - Each index slows writes
   - Only index queried columns

3. **Don't store large BLOBs in entities**
   - Keep entities < 1MB
   - Store large files separately

4. **Don't ignore data growth**
   - Plan for partitioning
   - Archive old data

---

## Performance Checklist

Before going to production:

- [ ] Analyzed access patterns
- [ ] Created indexes on queried columns
- [ ] Tested with realistic data volumes
- [ ] Monitored query performance
- [ ] Implemented caching strategy
- [ ] Planned for data growth
- [ ] Documented schema design decisions

---

## What You've Learned ✅

- ✅ Multi-model design principles
- ✅ Normalization vs. denormalization trade-offs
- ✅ Index strategy and placement
- ✅ Real-world schema patterns
- ✅ Migration strategies
- ✅ Performance optimization

---

## Next Steps

1. **[Best Practices Guide](BEST_PRACTICES.md)** - Production patterns
2. **[Performance Guide](../performance/PERFORMANCE_GUIDE.md)** - Optimization techniques
3. **Try Examples:** Build a [CRM](../../examples/17_crm/) or [E-Commerce](../../examples/14_ecommerce_catalog/)

---

**Questions?** Ask in [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
