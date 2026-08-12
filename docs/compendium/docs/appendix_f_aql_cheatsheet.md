# Appendix F: AQL Cheat Sheet & Quick Reference

> "Every expert was once a novice. This sheet shortens that journey."

---

## Overview
Quick reference for common AQL patterns, syntax, and idioms. For detailed docs, see Chapter 28.

---

## F.1 Basic Syntax

### Collections & Documents
```aql
-- Create Collection
CREATE COLLECTION users

-- Insert
INSERT {name: 'Alice', email: 'alice@example.com'} INTO users

-- Read
FOR u IN users RETURN u

-- Update
UPDATE 'users/123' WITH {status: 'active'} IN users

-- Delete
REMOVE 'users/123' IN users

-- Truncate (danger!)
TRUNCATE users
```

### Filtering
```aql
-- Simple equality
FOR u IN users FILTER u.status == 'active' RETURN u

-- Comparison operators
FILTER u.age > 18 AND u.age < 65
FILTER u.salary >= 50000
FILTER u.created_at < DATE_NOW()

-- IN operator
FILTER u.status IN ['active', 'pending', 'approved']

-- String operations
FILTER u.name LIKE 'Al%'                    -- SQL-like wildcard
FILTER u.email =~ '^.*@example\.com$'      -- Regex
FILTER CONTAINS(u.bio, 'engineer')         -- Substring

-- Null checks
FILTER u.middle_name != NULL
FILTER u.phone_number == NULL

-- Range
FILTER u.score >= 100 AND u.score <= 200   -- vs RANGE below
```

### Sorting & Limiting
```aql
-- Order results
FOR u IN users
  SORT u.created_at DESC, u.name ASC
  RETURN u

-- Limit results
FOR u IN users
  SORT u.created_at DESC
  LIMIT 10              -- First 10
  RETURN u

-- Pagination
FOR u IN users
  SORT u.created_at DESC
  LIMIT @offset, @limit  -- OFFSET, LIMIT
  RETURN u

-- Range operator (efficient for sorted data)
FOR u IN users
  RANGE u.score, 100, 200
  RETURN u
```

### Projection (Select Fields)
```aql
-- All fields
RETURN u

-- Specific fields
RETURN {name: u.name, email: u.email}

-- Computed fields
RETURN {
  name: u.name,
  age_years: DATE_DIFF(u.birth_date, DATE_NOW(), 'y'),
  email_masked: CONCAT(SUBSTRING(u.email, 0, 3), '***')
}

-- Nested structure
RETURN {
  user: {name: u.name, id: u._id},
  contact: {email: u.email, phone: u.phone}
}
```

---

## F.2 Advanced Filtering

### Aggregation Filter
```aql
-- Count documents
FOR u IN users
  COLLECT WITH COUNT INTO count
  RETURN {total_users: count}

-- Group and count
FOR u IN users
  COLLECT status = u.status WITH COUNT INTO count
  RETURN {status, count}

-- Group with aggregation
FOR o IN orders
  COLLECT customer_id = o.customer_id
  AGGREGATE total = SUM(o.amount), cnt = COUNT(1)
  RETURN {customer_id, total, cnt}
```

### Complex Filtering with Subqueries
```aql
-- Filter by aggregated value
FOR u IN users
  LET order_count = (
    FOR o IN orders FILTER o.user_id == u._id
    RETURN o
  ) | LENGTH
  FILTER order_count > 5
  RETURN {name: u.name, orders: order_count}

-- IN with subquery
FOR u IN users
  FILTER u._id IN (
    SELECT DISTINCT o.user_id FROM orders o WHERE o.status = 'completed'
  )
  RETURN u
```

### Array Operations
```aql
-- Check array contains
FOR u IN users
  FILTER 'premium' IN u.roles
  RETURN u

-- Array length
FILTER LENGTH(u.tags) > 3

-- Array flatten
FOR u IN users
  LET all_ids = FLATTEN(u.related_user_ids)
  RETURN {name: u.name, related_count: LENGTH(all_ids)}

-- Array filtering
FOR u IN users
  LET recent_orders = u.orders[* FILTER CURRENT.date > DATE_SUBTRACT(DATE_NOW(), 30, 'd')]
  RETURN {name: u.name, recent_count: LENGTH(recent_orders)}
```

---

## F.3 Joins & Relationships

### INNER JOIN (Graph Traversal)
```aql
-- Simple traverse
FOR u IN users
  FOR o IN orders FILTER o.user_id == u._id
  RETURN {user: u.name, order_id: o._id}

-- With graph edges
FOR v, e, p IN 1..3 OUTBOUND users/123 GRAPH 'social'
  RETURN {vertex: v.name, edge: e.type, path: p.vertices[*]._id}

-- Shortest path
FOR path IN SHORTEST_PATH('users/123' TO 'users/456' GRAPH 'social')
  RETURN path
```

### LEFT JOIN (Keep unmatched)
```aql
-- Users even if no orders
FOR u IN users
  LET orders = (FOR o IN orders FILTER o.user_id == u._id RETURN o)
  RETURN {user: u.name, order_count: LENGTH(orders), orders}
```

### Multiple joins
```aql
FOR u IN users
  LET orders = (FOR o IN orders FILTER o.user_id == u._id RETURN o)
  LET addresses = (FOR a IN addresses FILTER a.user_id == u._id RETURN a)
  FILTER LENGTH(orders) > 0 AND LENGTH(addresses) > 0
  RETURN {user: u.name, order_count: LENGTH(orders), address_count: LENGTH(addresses)}
```

---

## F.4 Transactions

### Basic Transaction
```aql
BEGIN
  INSERT {name: 'Bob', balance: 100} INTO accounts
  INSERT {from: 'Alice', to: 'Bob', amount: 50} INTO transfers
COMMIT
```

### With Error Handling
```aql
BEGIN
  LET update = UPDATE 'accounts/alice' 
    WITH {balance: (SELECT account.balance - 50 FROM accounts FILTER _id == 'accounts/alice')[0]} 
    IN accounts
  
  IF u.balance < 50 THEN
    ABORT "Insufficient funds"
  ELSE
    INSERT {from: 'alice', to: 'bob', amount: 50} INTO transfers
    COMMIT
  ENDIF
```

---

## F.5 Vector Search

### Simple Vector Search
```aql
-- Create vector index
CREATE INDEX idx_embedding ON articles(embedding)

-- Vector search (nearest neighbors)
FOR article IN articles
  FILTER DISTANCE(article.embedding, @query_embedding) < @threshold
  SORT DISTANCE(article.embedding, @query_embedding)
  LIMIT 10
  RETURN {title: article.title, distance: DISTANCE(article.embedding, @query_embedding)}

-- With parameters
db.query("""
  FOR article IN articles
    FILTER DISTANCE(article.embedding, @query_vec) < @threshold
    SORT DISTANCE(article.embedding, @query_vec)
    LIMIT @top_k
    RETURN article
""", bind_vars={
  'query_vec': [0.1, -0.2, 0.3, ...],
  'threshold': 0.5,
  'top_k': 5
})
```

### Hybrid Search (Vector + Text)
```aql
-- Combine vector + full-text
FOR article IN articles
  FILTER DISTANCE(article.embedding, @query_vec) < @threshold
  SEARCH article.title IN FULLTEXT(@query_text)
  SORT BM25(article) DESC  -- Relevance score
  RETURN {title: article.title, score: BM25(article)}
```

---

## F.6 Graph Queries

### Breadth-First Traversal
```aql
-- Get all friends of friends
FOR v IN 0..2 OUTBOUND 'users/alice' GRAPH 'social'
  RETURN {name: v.name, hops: LENGTH(p) - 1}

-- Prevent cycles
FOR v IN 1..3 OUTBOUND 'users/alice' GRAPH 'social'
  FILTER v._key NOT IN ['alice']  -- Don't revisit start node
  RETURN v
```

### Depth-First with Conditions
```aql
-- Traverse until condition met
FOR v IN OUTBOUND 'users/alice' GRAPH 'social'
  FILTER v.status != 'banned'
  LIMIT 100
  RETURN v
```

### Community Detection
```aql
-- Find clusters of connected users
FOR start IN users
  LET connected = (
    FOR v IN OUTBOUND start GRAPH 'social'
    RETURN DISTINCT v._id
  )
  COLLECT id = start._id, group = connected
  RETURN {user: id, cluster_size: LENGTH(group)}
```

---

## F.7 Time-Series Queries

### Bucket by Time
```aql
-- Sales per day
FOR order IN orders
  COLLECT date = DATE_FORMAT(order.created_at, '%yyyy-%mm-%dd')
  AGGREGATE total = SUM(order.amount), cnt = COUNT(1)
  SORT date
  RETURN {date, total, cnt}

-- Sales per hour (last 24h)
FOR order IN orders
  FILTER order.created_at > DATE_SUBTRACT(DATE_NOW(), 1, 'd')
  COLLECT hour = DATE_FORMAT(order.created_at, '%yyyy-%mm-%dd %hh:00')
  AGGREGATE total = SUM(order.amount)
  RETURN {hour, total}
```

### Moving Average
```aql
-- 7-day moving average of daily sales
FOR day IN 0..30
  LET current_date = DATE_SUBTRACT(DATE_NOW(), day, 'd')
  LET week_start = DATE_SUBTRACT(current_date, 7, 'd')
  LET sales = (
    FOR o IN orders
      FILTER o.created_at >= week_start AND o.created_at <= current_date
      RETURN o.amount
  )
  COLLECT date = DATE_FORMAT(current_date, '%yyyy-%mm-%dd')
  AGGREGATE avg = AVG(sales)
  SORT date
  RETURN {date, moving_avg_7d: avg}
```

---

## F.8 Window Functions

### Row Numbers & Ranking
```aql
-- Rank users by sales
FOR u IN users
  LET total_sales = (
    SELECT SUM(amount) AS total FROM orders WHERE customer_id == u._id
  )[0].total
  SORT total_sales DESC
  RETURN {
    rank: @row_number,
    name: u.name,
    total_sales
  }

-- Cumulative sum
FOR o IN orders
  SORT o.created_at
  COLLECT WITH COUNT INTO cnt
  AGGREGATE cumulative = SUM(o.amount)
  RETURN {cumulative, running_total: @row_number}
```

---

## F.9 Common Patterns

### Pagination with Cursor
```aql
-- Cursor-based pagination (efficient for large datasets)
FOR u IN users
  FILTER u._key > @cursor
  SORT u._key
  LIMIT 20
  RETURN u

-- No OFFSET (O(1) instead of O(n))
```

### Deduplication
```aql
-- Get distinct values
FOR u IN users
  COLLECT email = u.email
  RETURN email

-- Get first of each group
FOR u IN users
  COLLECT category = u.category
  INTO group = u
  RETURN {category, first_user: group[0]}
```

### Batch Operations
```aql
-- Batch insert
FOR item IN @items
  INSERT item INTO products

-- Batch update
FOR item IN items
  FILTER item.status == 'pending'
  UPDATE item WITH {status: 'processed'} IN items
```

### Error Handling
```aql
-- TRY-CATCH
BEGIN
  LET result = (
    TRY
      FOR u IN users FILTER u._id == @user_id RETURN u
    CATCH err
      RETURN {error: err.message}
  )
  RETURN result
COMMIT
```

---

## F.10 Performance Tips

### Query Optimization Checklist
```
✓ Use FILTER before SORT
✓ Use LIMIT to reduce result set
✓ Add indexes on FILTER/SORT fields
✓ Use COLLECT for aggregations (not manual loops)
✓ Avoid FULLTEXT searches on tiny datasets
✓ Stream large results (avoid materializing)
✓ Use bind parameters (@var) not inline values
```

### Slow Query Patterns to Avoid
```aql
-- ❌ SLOW: Regex without index
FILTER u.email =~ '.*@gmail\.com'

-- ✅ FAST: Use LIKE with index
CREATE INDEX idx_email ON users(email)
FILTER u.email LIKE '%@gmail.com'

-- ❌ SLOW: Expression in FILTER
FILTER YEAR(u.birth_date) == 1990

-- ✅ FAST: Use range on indexed field
CREATE INDEX idx_birth ON users(birth_date)
FILTER u.birth_date >= '1990-01-01' AND u.birth_date < '1991-01-01'
```

---

## F.11 Built-in Functions (Most Common)

### String Functions
| Function | Example | Returns |
|----------|---------|---------|
| `CONCAT()` | `CONCAT('Hello', ' ', 'World')` | `'Hello World'` |
| `CONTAINS()` | `CONTAINS('Hello', 'ell')` | `true` |
| `SUBSTRING()` | `SUBSTRING('Hello', 0, 3)` | `'Hel'` |
| `LENGTH()` | `LENGTH('Hello')` | `5` |
| `UPPER()` | `UPPER('hello')` | `'HELLO'` |
| `LOWER()` | `LOWER('HELLO')` | `'hello'` |
| `SPLIT()` | `SPLIT('a,b,c', ',')` | `['a', 'b', 'c']` |

### Date/Time Functions
| Function | Example | Returns |
|----------|---------|---------|
| `DATE_NOW()` | `DATE_NOW()` | Current timestamp |
| `DATE_FORMAT()` | `DATE_FORMAT(d, '%yyyy-%mm-%dd')` | Formatted date |
| `DATE_ADD()` | `DATE_ADD('2025-01-01', 5, 'd')` | Date + 5 days |
| `DATE_SUBTRACT()` | `DATE_SUBTRACT(d, 1, 'm')` | Date - 1 month |
| `DATE_DIFF()` | `DATE_DIFF(d1, d2, 'd')` | Days between |

### Math Functions
| Function | Example | Returns |
|----------|---------|---------|
| `SUM()` | `SUM(o.amount)` | Sum of values |
| `AVG()` | `AVG(o.amount)` | Average |
| `MIN()` / `MAX()` | `MIN(values)` | Min/Max |
| `ROUND()` | `ROUND(3.7)` | `4` |
| `FLOOR()` / `CEIL()` | `FLOOR(3.7)` | `3` |
| `ABS()` | `ABS(-5)` | `5` |
| `SQRT()` | `SQRT(16)` | `4` |

### Array Functions
| Function | Example | Returns |
|----------|---------|---------|
| `LENGTH()` | `LENGTH([1,2,3])` | `3` |
| `APPEND()` | `APPEND(arr, item)` | Array + item |
| `REVERSE()` | `REVERSE([1,2,3])` | `[3,2,1]` |
| `UNIQUE()` | `UNIQUE([1,1,2])` | `[1,2]` |
| `FLATTEN()` | `FLATTEN([[1,2], [3]])` | `[1,2,3]` |
| `SLICE()` | `SLICE([1,2,3,4], 1, 3)` | `[2,3]` |

---

## F.12 Index Types

```aql
-- Unique Index
CREATE INDEX idx_email UNIQUE ON users(email)

-- Compound Index (Multiple fields)
CREATE INDEX idx_status_date ON orders(status, created_at)

-- Partial Index (Conditional)
CREATE INDEX idx_active_users ON users(name)
  FILTER u.status == 'active'

-- TTL Index (Auto-delete)
CREATE INDEX idx_temp ON temp_data(created_at)
  WITH {ttl: 3600}  -- Auto-delete after 1 hour

-- Full-Text Index
CREATE FULLTEXT INDEX idx_content ON articles(body)
```

---

## Summary

Bookmark this section! Most queries can be built from these patterns. When stuck:
1. **Check pattern above** 
2. **Run EXPLAIN to verify optimization**
3. **See Chapter 28 for detailed docs**

---

## v1.5.0 Additions: Neue AQL-Features & Patterns

### Hybrid Search (Vector + Keyword)

```aql
-- Hybrid Search: semantisch + Keyword (BM25 + Vector, RRF-Fusion)
FOR doc IN documents
  LET vec_score = VECTOR_SIMILARITY(doc.embedding, @query_vec, "cosine")
  LET bm25_score = BM25(doc, @keywords)
  LET rrf_score = 1.0 / (60.0 + RANK(vec_score)) + 1.0 / (60.0 + RANK(bm25_score))
  SORT rrf_score DESC
  LIMIT 20
  RETURN {doc, vec_score, bm25_score, rrf_score}

-- Kurzform mit HYBRID_SEARCH() (ThemisDB v1.5.0)
FOR doc IN HYBRID_SEARCH(documents,
  VECTOR: {field: "embedding", query: @query_vec, weight: 0.5},
  BM25:   {field: "content",   query: @keywords,  weight: 0.5},
  TOP: 20)
RETURN doc
```

### Filtered Vector Search

```aql
-- Vektorsuche mit Vorfilter (pre-filtering, effizienter als Post-Filter)
FOR doc IN documents
  FILTER doc.category == "technical"
  FILTER doc.lang == "de"
  LET score = VECTOR_SIMILARITY(doc.embedding, @query_vec, "cosine")
  SORT score DESC
  LIMIT 10
  RETURN {doc._key, doc.title, score}

-- HNSW mit ef_search Override für höhere Recall
FOR doc IN documents OPTIONS {vectorIndexEfSearch: 200}
  LET score = VECTOR_SIMILARITY(doc.embedding, @query_vec, "cosine")
  SORT score DESC
  LIMIT 10
  RETURN doc
```

### Time-Series Aggregationen (neu in v1.5.0)

```aql
-- Rolling Average über 5-Minuten-Fenster
FOR m IN metrics
  FILTER m.ts >= DATE_SUBTRACT(NOW(), 1, "hour")
  COLLECT minute = DATE_TRUNC(m.ts, "minute") INTO bucket
  LET avg_val = AVERAGE(bucket[*].m.value)
  SORT minute ASC
  RETURN {minute, avg_val}

-- Anomalie-Erkennung: Werte über 3-Sigma-Grenze
LET stats = (
  FOR m IN metrics FILTER m.ts >= DATE_SUBTRACT(NOW(), 24, "hour")
  RETURN m.value
)
LET mean = AVERAGE(stats)
LET stddev = STDDEV_POPULATION(stats)
FOR m IN metrics
  FILTER m.ts >= DATE_SUBTRACT(NOW(), 1, "hour")
  FILTER m.value > mean + 3 * stddev
  RETURN {m.ts, m.value, zscore: (m.value - mean) / stddev}
```

### Graph-Traversals mit Kostenoptimierung

```aql
-- Kürzester Pfad (Dijkstra)
FOR path IN SHORTEST_PATH
  GRAPH "knowledge_graph"
  FROM "concepts/machine_learning"
  TO   "concepts/neural_network"
  OPTIONS {weightAttribute: "distance"}
  RETURN path

-- K-Hop-Nachbarn mit Tiefenbegrenzung und Kantenfilter
FOR v, e, p IN 1..3 OUTBOUND "users/alice"
  GRAPH "social_graph"
  FILTER e.weight > 0.5
  FILTER v.active == true
  RETURN DISTINCT v
```

### Encryption-aware Queries (v1.5.0)

```aql
-- Abfrage auf verschlüsselten Feldern (Entschlüsselung server-side)
-- Hinweis: Filterung auf verschlüsselten Feldern erfordert Scan mit Entschlüsselung
FOR u IN users
  LET email = DECRYPT(u.email_encrypted, "user_emails")
  FILTER email == @search_email
  RETURN {u._key, email}

-- Batch-Rückgabe verschlüsselter Felder (performanter als Einzel-Decrypt)
FOR u IN users LIMIT 100
  RETURN {
    id: u._key,
    email: DECRYPT(u.email_encrypted, "user_emails"),
    name: u.name
  }
```

### Performance-Hints & EXPLAIN

```aql
-- EXPLAIN für Query-Plan-Analyse
EXPLAIN FOR doc IN documents
  FILTER doc.category == "ml"
  LET s = VECTOR_SIMILARITY(doc.embedding, @q, "cosine")
  SORT s DESC LIMIT 10
  RETURN doc

-- Index-Nutzung erzwingen (Override Optimizer)
FOR doc IN documents OPTIONS {indexHint: "idx_category", forceIndexHint: true}
  FILTER doc.category == "ml"
  RETURN doc

-- Query-Cache deaktivieren (für Benchmarks)
FOR doc IN documents OPTIONS {useQueryCache: false}
  RETURN doc
```

---

**Appendix F — Stand:** v1.5.0-dev (Q3/Q4 2026 Update)  
**→ Weiter:** [Kapitel 28: AQL Referenz](chapter_28_aql_reference.md) | [Kapitel 34: Query Optimization](chapter_34_query_optimization.md)
