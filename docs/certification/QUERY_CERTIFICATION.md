# ThemisDB Query Expert Certification (TQE)

## Certification Overview

The **ThemisDB Query Expert Certification (TQE)** is an advanced certification that validates your expertise in writing complex, optimized queries across ThemisDB's multi-model architecture. This certification demonstrates that you can design efficient query strategies, optimize performance, implement advanced graph traversals, and leverage vector similarity search for AI/ML workloads.

### Certification Details

- **Certification Code**: TQE
- **Level**: Advanced
- **Duration**: 120 minutes (exam) + project submission
- **Question Count**: 30-35 questions + hands-on project
- **Question Types**: Multiple choice, scenario-based, and practical coding
- **Passing Score**: 75% (23/30 minimum on exam + passing project)
- **Validity**: 2 years
- **Prerequisites**: ThemisDB Fundamentals Certification (TDF)
- **Exam Fee**: $250 USD
- **Retake Fee**: $125 USD
- **Language**: English

---

## Target Audience

This certification is ideal for:

- **Senior Software Developers** building complex applications
- **Data Engineers** designing data pipelines
- **Backend Developers** optimizing database queries
- **Application Architects** designing query strategies
- **Analytics Engineers** building reporting systems
- **Performance Engineers** tuning query performance
- **ML Engineers** implementing vector search
- **API Developers** optimizing data access layers

---

## Prerequisites

### Required Certification
- **ThemisDB Fundamentals Certification (TDF)** - Must be current (not expired)

### Technical Prerequisites
- 6-12 months hands-on experience with ThemisDB
- Strong understanding of AQL fundamentals
- Experience with query optimization concepts
- Familiarity with index structures
- Understanding of graph algorithms (basic)
- Knowledge of vector embeddings (for AI/ML sections)

### Recommended Experience
- Built at least one production application using ThemisDB
- Optimized queries for performance
- Worked with multi-model data patterns
- Experience with query profiling and debugging

---

## Learning Objectives

Upon completing this certification, you will be able to:

### 1. Advanced AQL Techniques (25%)
- Write complex nested queries and subqueries
- Use window functions and CTEs
- Implement recursive queries
- Apply advanced aggregation patterns
- Master query composition and reusability
- Utilize dynamic query generation

### 2. Graph Traversal and Algorithms (20%)
- Implement shortest path algorithms
- Perform depth-first and breadth-first traversals
- Calculate graph metrics (centrality, clustering)
- Use pattern matching in graphs
- Optimize graph query performance
- Handle large-scale graph analytics

### 3. Vector Similarity Search (15%)
- Understand vector embedding concepts
- Implement k-NN searches
- Use approximate nearest neighbor (ANN) algorithms
- Optimize vector index configuration
- Apply similarity metrics (cosine, euclidean)
- Integrate with ML/AI pipelines

### 4. Query Optimization (20%)
- Analyze query execution plans
- Identify performance bottlenecks
- Apply optimization techniques
- Understand optimizer hints
- Optimize joins and subqueries
- Reduce query complexity

### 5. Index Strategies (15%)
- Design effective indexing strategies
- Use composite and partial indexes
- Implement full-text search indexes
- Configure vector indexes
- Understand index trade-offs
- Monitor index performance

### 6. Performance Tuning (5%)
- Profile query performance
- Use caching strategies
- Optimize batch operations
- Handle large result sets
- Minimize network latency
- Apply query result pagination

---

## Advanced AQL Topics

### Complex Queries and Subqueries

#### Correlated Subqueries
```aql
-- Find customers who ordered above their average
SELECT c.customer_id, c.name, o.order_total
FROM customers c
INNER JOIN orders o ON c.customer_id = o.customer_id
WHERE o.order_total > (
    SELECT AVG(o2.order_total)
    FROM orders o2
    WHERE o2.customer_id = c.customer_id
)
ORDER BY o.order_total DESC;
```

#### Common Table Expressions (CTEs)
```aql
WITH regional_sales AS (
    SELECT region, SUM(amount) as total_sales
    FROM orders
    GROUP BY region
),
top_regions AS (
    SELECT region
    FROM regional_sales
    WHERE total_sales > (SELECT AVG(total_sales) FROM regional_sales)
)
SELECT r.region, r.total_sales, p.product_name, SUM(o.amount) as product_sales
FROM top_regions t
INNER JOIN regional_sales r ON t.region = r.region
INNER JOIN orders o ON r.region = o.region
INNER JOIN products p ON o.product_id = p.product_id
GROUP BY r.region, r.total_sales, p.product_name
ORDER BY r.region, product_sales DESC;
```

#### Recursive Queries
```aql
-- Find organizational hierarchy
WITH RECURSIVE emp_hierarchy AS (
    -- Base case: top-level managers
    SELECT employee_id, name, manager_id, 1 as level
    FROM employees
    WHERE manager_id IS NULL
    
    UNION ALL
    
    -- Recursive case: subordinates
    SELECT e.employee_id, e.name, e.manager_id, eh.level + 1
    FROM employees e
    INNER JOIN emp_hierarchy eh ON e.manager_id = eh.employee_id
)
SELECT * FROM emp_hierarchy
ORDER BY level, name;
```

#### Window Functions
```aql
-- Calculate running totals and rankings
SELECT 
    order_date,
    customer_id,
    order_total,
    -- Running total
    SUM(order_total) OVER (
        PARTITION BY customer_id 
        ORDER BY order_date
    ) as running_total,
    -- Ranking within customer
    ROW_NUMBER() OVER (
        PARTITION BY customer_id 
        ORDER BY order_total DESC
    ) as order_rank,
    -- Moving average (last 3 orders)
    AVG(order_total) OVER (
        PARTITION BY customer_id 
        ORDER BY order_date 
        ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
    ) as moving_avg
FROM orders
ORDER BY customer_id, order_date;
```

#### Advanced Aggregations
```aql
-- Multi-level aggregation with GROUPING SETS
SELECT 
    region,
    category,
    product_name,
    SUM(sales) as total_sales,
    COUNT(*) as order_count
FROM sales_data
GROUP BY GROUPING SETS (
    (region, category, product_name),  -- Detailed
    (region, category),                -- By category in region
    (region),                          -- By region
    ()                                 -- Grand total
)
ORDER BY region, category, product_name;

-- Using ROLLUP for hierarchical totals
SELECT 
    year,
    quarter,
    month,
    SUM(revenue) as total_revenue
FROM financial_data
GROUP BY ROLLUP(year, quarter, month);

-- Using CUBE for all combinations
SELECT 
    product_category,
    customer_segment,
    sales_channel,
    SUM(revenue) as total_revenue
FROM sales
GROUP BY CUBE(product_category, customer_segment, sales_channel);
```

### Dynamic Query Construction

```aql
-- Build queries dynamically with parameters
LET filters = {
    min_price: 100,
    max_price: 500,
    categories: ["electronics", "computers"],
    in_stock: true
};

FOR product IN products
    FILTER product.price >= filters.min_price
    FILTER product.price <= filters.max_price
    FILTER product.category IN filters.categories
    FILTER filters.in_stock ? product.stock > 0 : true
    SORT product.price ASC
    RETURN product;
```

---

## Graph Traversal Algorithms

### Shortest Path
```aql
-- Find shortest path between two nodes
FOR vertex, edge, path IN OUTBOUND SHORTEST_PATH
    'persons/alice' TO 'persons/bob'
    GRAPH 'social_network'
    RETURN {
        vertices: path.vertices[*].name,
        edges: path.edges[*].type,
        distance: LENGTH(path.edges)
    };

-- All shortest paths with weights
FOR path IN OUTBOUND K_SHORTEST_PATHS
    'locations/start' TO 'locations/end'
    GRAPH 'road_network'
    OPTIONS {
        weightAttribute: 'distance',
        defaultWeight: 1
    }
    LIMIT 5
    RETURN {
        path: path.vertices[*].name,
        total_distance: SUM(path.edges[*].distance)
    };
```

### Graph Traversal
```aql
-- Depth-first traversal with pruning
FOR vertex, edge, path IN 1..5 OUTBOUND
    'products/laptop'
    GRAPH 'recommendations'
    OPTIONS {
        order: "dfs",
        uniqueVertices: "global"
    }
    PRUNE edge.strength < 0.5
    FILTER vertex.category == "electronics"
    RETURN DISTINCT vertex;

-- Breadth-first with path tracking
FOR vertex, edge, path IN 1..3 ANY
    'persons/john'
    GRAPH 'social_network'
    OPTIONS {
        order: "bfs",
        uniqueVertices: "path"
    }
    RETURN {
        person: vertex.name,
        path_length: LENGTH(path.edges),
        connection_types: path.edges[*].type
    };
```

### Pattern Matching
```aql
-- Find triangles in graph (A->B, B->C, C->A)
FOR v1 IN persons
    FOR v2 IN OUTBOUND v1 knows
        FOR v3 IN OUTBOUND v2 knows
            FILTER v3 == v1
            RETURN DISTINCT {
                triangle: [v1.name, v2.name, v3.name]
            };

-- Find influential nodes (high degree centrality)
FOR vertex IN persons
    LET out_degree = LENGTH(
        FOR v IN OUTBOUND vertex knows RETURN 1
    )
    LET in_degree = LENGTH(
        FOR v IN INBOUND vertex knows RETURN 1
    )
    SORT (out_degree + in_degree) DESC
    LIMIT 10
    RETURN {
        name: vertex.name,
        connections: out_degree + in_degree,
        out_connections: out_degree,
        in_connections: in_degree
    };
```

### Graph Analytics
```aql
-- Calculate PageRank
LET graph_data = (
    FOR vertex IN vertices
        LET outbound = (
            FOR v IN OUTBOUND vertex edges
                RETURN v._id
        )
        RETURN {
            id: vertex._id,
            outbound: outbound
        }
)

-- Clustering coefficient
FOR vertex IN social_network_vertices
    LET neighbors = (
        FOR v IN ANY vertex social_network_edges
            RETURN v._id
    )
    LET neighbor_connections = (
        FOR n1 IN neighbors
            FOR n2 IN neighbors
                FILTER n1 != n2
                FOR edge IN social_network_edges
                    FILTER (edge._from == n1 AND edge._to == n2)
                        OR (edge._from == n2 AND edge._to == n1)
                    RETURN 1
    )
    LET possible_connections = LENGTH(neighbors) * (LENGTH(neighbors) - 1)
    RETURN {
        vertex: vertex._id,
        clustering_coefficient: possible_connections > 0 
            ? LENGTH(neighbor_connections) / possible_connections 
            : 0
    };
```

---

## Vector Similarity Search

### Vector Embeddings Basics
```aql
-- Store document with vector embedding
INSERT INTO documents {
    title: "Machine Learning Basics",
    content: "Introduction to ML concepts...",
    vector: [0.23, 0.45, 0.67, 0.12, 0.89, ...],  // 768-dim embedding
    created_at: NOW()
};

-- Create vector index
CREATE INDEX vector_idx ON documents (vector)
OPTIONS {
    type: "vector",
    dimensions: 768,
    metric: "cosine",
    algorithm: "hnsw",
    ef_construction: 200,
    m: 16
};
```

### K-Nearest Neighbors Search
```aql
-- Find similar documents
LET query_vector = [0.25, 0.43, 0.65, 0.15, 0.87, ...]

FOR doc IN documents
    LET similarity = COSINE_SIMILARITY(doc.vector, query_vector)
    SORT similarity DESC
    LIMIT 10
    RETURN {
        title: doc.title,
        similarity: similarity,
        content: SUBSTRING(doc.content, 0, 200)
    };

-- Using vector index for faster search
FOR doc IN documents
    VECTOR_SEARCH(doc.vector, query_vector, 10)
    RETURN {
        title: doc.title,
        distance: VECTOR_DISTANCE(doc.vector, query_vector, "cosine"),
        content: doc.content
    };
```

### Similarity Metrics
```aql
-- Cosine similarity (normalized dot product)
LET cosine_sim = COSINE_SIMILARITY(vec1, vec2)

-- Euclidean distance
LET euclidean_dist = EUCLIDEAN_DISTANCE(vec1, vec2)

-- Manhattan distance
LET manhattan_dist = MANHATTAN_DISTANCE(vec1, vec2)

-- Dot product
LET dot_prod = DOT_PRODUCT(vec1, vec2)

-- Combined similarity with filters
FOR doc IN documents
    LET similarity = COSINE_SIMILARITY(doc.vector, query_vector)
    FILTER similarity > 0.8
    FILTER doc.category == "technology"
    FILTER doc.published_date > DATE_SUB(NOW(), INTERVAL 30 DAY)
    SORT similarity DESC
    LIMIT 20
    RETURN doc;
```

### Hybrid Search (Vector + Full-Text)
```aql
-- Combine vector similarity with keyword search
LET query_vector = EMBED_TEXT("machine learning algorithms")
LET keyword_results = (
    FOR doc IN FULLTEXT(documents, "content", "machine learning")
        RETURN doc
)
LET vector_results = (
    FOR doc IN documents
        VECTOR_SEARCH(doc.vector, query_vector, 50)
        RETURN {doc: doc, vector_score: COSINE_SIMILARITY(doc.vector, query_vector)}
)

// Combine and re-rank
FOR result IN UNION(keyword_results, vector_results)
    LET keyword_score = result IN keyword_results ? 1 : 0
    LET vector_score = result.vector_score || 0
    LET combined_score = 0.6 * vector_score + 0.4 * keyword_score
    SORT combined_score DESC
    LIMIT 10
    RETURN {
        title: result.doc.title,
        combined_score: combined_score,
        vector_score: vector_score,
        has_keywords: keyword_score > 0
    };
```

---

## Query Optimization

### Execution Plan Analysis
```aql
-- View execution plan
EXPLAIN
SELECT c.name, COUNT(o.id) as order_count
FROM customers c
LEFT JOIN orders o ON c.id = o.customer_id
WHERE c.region = "North"
GROUP BY c.id, c.name
HAVING order_count > 10;

-- Returns plan showing:
-- - Index usage
-- - Join strategy
-- - Filter application order
-- - Estimated rows at each stage
```

### Optimization Techniques

#### Index Usage
```aql
-- Bad: Full table scan
SELECT * FROM orders WHERE customer_id = 123;

-- Good: Uses index
CREATE INDEX idx_customer ON orders (customer_id);
SELECT * FROM orders WHERE customer_id = 123;

-- Better: Covering index
CREATE INDEX idx_customer_date ON orders (customer_id, order_date, total);
SELECT order_date, total FROM orders WHERE customer_id = 123;
```

#### Join Optimization
```aql
-- Bad: Multiple separate queries
LET customer = DOCUMENT("customers/123")
LET orders = (FOR o IN orders FILTER o.customer_id == 123 RETURN o)
LET products = (FOR o IN orders FOR p IN products 
                FILTER p.id == o.product_id RETURN p)

-- Good: Single optimized query with proper joins
FOR c IN customers
    FILTER c._id == "customers/123"
    FOR o IN orders
        FILTER o.customer_id == c.id
        FOR p IN products
            FILTER p.id == o.product_id
            RETURN {customer: c, order: o, product: p}
```

#### Subquery Optimization
```aql
-- Bad: Correlated subquery in SELECT
SELECT 
    c.name,
    (SELECT COUNT(*) FROM orders WHERE customer_id = c.id) as order_count
FROM customers c;

-- Good: Use JOIN and GROUP BY
SELECT c.name, COUNT(o.id) as order_count
FROM customers c
LEFT JOIN orders o ON c.id = o.customer_id
GROUP BY c.id, c.name;
```

#### Filter Early
```aql
-- Bad: Filter after joins
SELECT *
FROM customers c
JOIN orders o ON c.id = o.customer_id
JOIN products p ON o.product_id = p.id
WHERE c.region = "North" AND o.order_date > '2024-01-01';

-- Good: Filter before joins
SELECT *
FROM (SELECT * FROM customers WHERE region = "North") c
JOIN (SELECT * FROM orders WHERE order_date > '2024-01-01') o 
    ON c.id = o.customer_id
JOIN products p ON o.product_id = p.id;
```

### Optimizer Hints
```aql
-- Force index usage
SELECT /*+ INDEX(orders idx_customer_date) */
    customer_id, order_date, total
FROM orders
WHERE customer_id = 123 AND order_date > '2024-01-01';

-- Control join order
SELECT /*+ LEADING(c, o, p) */
    c.name, o.total, p.name
FROM customers c
JOIN orders o ON c.id = o.customer_id
JOIN products p ON o.product_id = p.id;

-- Disable specific optimization
SELECT /*+ NO_MERGE(subq) */
    *
FROM (SELECT * FROM large_table WHERE condition) subq;
```

---

## Index Strategies

### Index Types

#### B-Tree Index (Default)
```aql
-- Single column
CREATE INDEX idx_email ON users (email);

-- Composite index (order matters!)
CREATE INDEX idx_region_status ON customers (region, status);

-- Best for: equality, range queries, sorting
```

#### Hash Index
```aql
-- Optimized for equality only
CREATE INDEX idx_user_hash ON sessions (user_id)
OPTIONS {type: "hash"};

-- Best for: exact match lookups
-- Not suitable for: range queries, sorting
```

#### Full-Text Index
```aql
-- Text search index
CREATE INDEX idx_content_fulltext ON articles (title, content)
OPTIONS {
    type: "fulltext",
    analyzer: "text_en",
    features: ["frequency", "position"]
};

-- Usage
FOR doc IN FULLTEXT(articles, "content", "query terms")
    RETURN doc;
```

#### Geospatial Index
```aql
-- For location queries
CREATE INDEX idx_location ON stores (location)
OPTIONS {type: "geo"};

-- Find nearby stores
FOR store IN stores
    FILTER GEO_DISTANCE(store.location, [lat, lng]) < 5000  // 5km
    RETURN store;
```

#### Vector Index
```aql
-- For similarity search
CREATE INDEX idx_embedding ON documents (embedding)
OPTIONS {
    type: "vector",
    dimensions: 768,
    metric: "cosine",
    algorithm: "hnsw",  // Hierarchical Navigable Small World
    ef_construction: 200,
    m: 16
};
```

### Partial Indexes
```aql
-- Index only subset of data
CREATE INDEX idx_active_users ON users (last_login)
WHERE active = true;

-- Smaller index, faster queries for active users
SELECT * FROM users 
WHERE active = true AND last_login > '2024-01-01';
```

### Index Maintenance
```aql
-- Analyze index usage
SHOW INDEX STATISTICS FOR TABLE orders;

-- Rebuild fragmented index
REBUILD INDEX idx_customer_date ON orders;

-- Drop unused index
DROP INDEX idx_old_unused ON orders;
```

### Index Trade-offs

| Aspect | Benefit | Cost |
|--------|---------|------|
| **Write Performance** | - | Slower inserts/updates |
| **Read Performance** | Much faster queries | - |
| **Storage** | - | Additional disk space |
| **Memory** | Cached indexes speed queries | Higher memory usage |
| **Maintenance** | - | Requires monitoring & tuning |

---

## Hands-On Labs

### Lab 1: Complex Query Optimization (4 hours)

**Objective**: Optimize a slow-running analytics query

**Scenario**: E-commerce platform with 10M orders, 1M customers

**Tasks**:
1. Analyze the provided slow query execution plan
2. Identify bottlenecks (missing indexes, inefficient joins)
3. Create appropriate indexes
4. Rewrite query for better performance
5. Measure improvement (target: 10x faster)

**Deliverables**:
- Original vs optimized query comparison
- Execution plan analysis
- Index creation scripts
- Performance metrics

---

### Lab 2: Graph Traversal (3 hours)

**Objective**: Implement recommendation engine using graph traversals

**Scenario**: Social network with friend connections and product purchases

**Tasks**:
1. Build friend-of-friend recommendation query
2. Implement collaborative filtering (bought together)
3. Calculate influence scores
4. Find shortest connection path between users
5. Optimize graph queries for 1M+ nodes

**Deliverables**:
- Graph traversal queries
- Performance benchmarks
- Recommendation results

---

### Lab 3: Vector Similarity Search (4 hours)

**Objective**: Build semantic search for document collection

**Scenario**: 100K technical documents with embeddings

**Tasks**:
1. Create and configure vector index
2. Implement k-NN search
3. Build hybrid search (vector + full-text)
4. Optimize for query latency (<100ms)
5. Handle relevance ranking

**Deliverables**:
- Vector search implementation
- Hybrid search queries
- Performance tuning results
- Search quality metrics

---

### Lab 4: Multi-Model Integration (5 hours)

**Objective**: Build complex application using all models

**Scenario**: IoT platform with devices, time-series data, relationships

**Tasks**:
1. Design schema across document, graph, time-series
2. Write queries joining multiple models
3. Implement real-time analytics
4. Optimize for high write throughput
5. Build dashboard queries

**Deliverables**:
- Schema design document
- Multi-model queries
- Performance optimization report

---

## Sample Exam Questions

### Section 1: Advanced AQL

**Question 1**: What is the main advantage of using a CTE over a subquery?
- A) CTEs are always faster
- B) CTEs can be referenced multiple times
- C) CTEs use less memory
- D) CTEs are required for joins

**Answer**: B

---

**Question 2**: Which window function would you use to calculate a running total?
- A) ROW_NUMBER()
- B) RANK()
- C) SUM() OVER()
- D) COUNT() OVER()

**Answer**: C

---

**Question 3**: What does GROUPING SETS allow that GROUP BY doesn't?
- A) Faster aggregation
- B) Multiple grouping combinations in one query
- C) Better index usage
- D) Parallel execution

**Answer**: B

---

**Question 4**: In a recursive CTE, what prevents infinite loops?
- A) Automatic depth limit
- B) Cycle detection in the recursive term
- C) Maximum iteration configuration
- D) All of the above

**Answer**: D

---

**Question 5**: When should you use a correlated subquery?
- A) Never, they're always slow
- B) When the subquery depends on the outer query
- C) Only for EXISTS checks
- D) When joining is not possible

**Answer**: B

---

### Section 2: Graph Algorithms

**Question 6**: Which algorithm finds the shortest path in a weighted graph?
- A) Depth-First Search
- B) Breadth-First Search
- C) Dijkstra's Algorithm
- D) PageRank

**Answer**: C

---

**Question 7**: What is the time complexity of BFS in a graph with V vertices and E edges?
- A) O(V)
- B) O(E)
- C) O(V + E)
- D) O(V * E)

**Answer**: C

---

**Question 8**: What does the PRUNE keyword do in a graph traversal?
- A) Deletes vertices
- B) Stops traversal down a path
- C) Removes duplicate results
- D) Optimizes the query

**Answer**: B

---

**Question 9**: Which centrality measure counts the shortest paths through a vertex?
- A) Degree Centrality
- B) Closeness Centrality
- C) Betweenness Centrality
- D) PageRank

**Answer**: C

---

**Question 10**: What is the purpose of uniqueVertices: "path" option?
- A) Ensures global uniqueness
- B) Allows vertex repetition in different paths
- C) Prevents cycles within a single path
- D) Improves performance

**Answer**: C

---

### Section 3: Vector Search

**Question 11**: What does cosine similarity measure?
- A) Euclidean distance between vectors
- B) Angle between vectors (direction similarity)
- C) Manhattan distance
- D) Vector magnitude difference

**Answer**: B

---

**Question 12**: Why use approximate nearest neighbor (ANN) instead of exact k-NN?
- A) Better accuracy
- B) Simpler implementation
- C) Much faster for large datasets
- D) Uses less memory

**Answer**: C

---

**Question 13**: What is HNSW in vector indexing?
- A) A distance metric
- B) An approximate nearest neighbor algorithm
- C) A vector dimension
- D) A similarity function

**Answer**: B

---

**Question 14**: When would you use Manhattan distance over cosine similarity?
- A) For normalized vectors
- B) For directional similarity
- C) For coordinate-based data
- D) Never, cosine is always better

**Answer**: C

---

**Question 15**: What is the main trade-off when increasing the 'ef' parameter in HNSW?
- A) Accuracy vs build time
- B) Accuracy vs query time
- C) Memory vs accuracy
- D) Build time vs query time

**Answer**: B

---

### Section 4: Query Optimization

**Question 16**: What does EXPLAIN show you?
- A) Query results
- B) Query execution plan
- C) Index definitions
- D) Table schema

**Answer**: B

---

**Question 17**: Why are covering indexes beneficial?
- A) They use less disk space
- B) They're faster to create
- C) Query doesn't need to access table data
- D) They work for all queries

**Answer**: C

---

**Question 18**: What is a join selectivity?
- A) Which join algorithm is used
- B) The order of tables in the join
- C) The percentage of rows that match
- D) The number of joins in a query

**Answer**: C

---

**Question 19**: When should you use query hints?
- A) Always, they improve all queries
- B) Never, the optimizer knows best
- C) When optimizer makes suboptimal choices
- D) Only for simple queries

**Answer**: C

---

**Question 20**: What is predicate pushdown?
- A) Moving filters before joins
- B) Pushing queries to replicas
- C) Distributing queries across shards
- D) Caching filter results

**Answer**: A

---

### Section 5: Index Strategies

**Question 21**: For a composite index on (A, B, C), which queries can use it?
- A) Only queries filtering on all three
- B) Queries filtering on A, or A+B, or A+B+C
- C) Any queries using A, B, or C
- D) Only queries filtering on A

**Answer**: B

---

**Question 22**: When should you use a hash index?
- A) For range queries
- B) For exact equality lookups
- C) For sorting
- D) For full-text search

**Answer**: B

---

**Question 23**: What is a partial index?
- A) An incomplete index
- B) An index on part of a column
- C) An index with a WHERE clause
- D) A temporary index

**Answer**: C

---

**Question 24**: Why might you have multiple indexes on the same column?
- A) Faster queries
- B) Redundancy for safety
- C) Different index types for different query patterns
- D) It's not allowed

**Answer**: C

---

**Question 25**: What happens when you have too many indexes?
- A) Queries become faster
- B) Writes become slower
- C) Better memory usage
- D) Automatic optimization

**Answer**: B

---

### Scenario-Based Questions

**Question 26**: You have a query joining 4 large tables that takes 30 seconds. The execution plan shows sequential scans on all tables. What should you do first?
- A) Add more RAM
- B) Rewrite the query
- C) Add appropriate indexes on join columns
- D) Use query hints

**Answer**: C

---

**Question 27**: Your graph traversal query for 3-hop friend recommendations times out. How do you optimize it?
- A) Increase the traversal depth
- B) Add PRUNE conditions and use uniqueVertices
- C) Remove all filters
- D) Use a different graph

**Answer**: B

---

**Question 28**: Vector similarity search returns results in 2 seconds but you need <100ms. What's the best approach?
- A) Buy faster hardware
- B) Reduce vector dimensions
- C) Use approximate nearest neighbor (ANN) index
- D) Cache all results

**Answer**: C

---

**Question 29**: A query performs well with 1000 rows but slows dramatically with 1M rows. Why?
- A) Bad code
- B) Algorithmic complexity (e.g., O(n²) operation)
- C) Network latency
- D) Disk failure

**Answer**: B

---

**Question 30**: Your analytics query needs data from document, graph, and time-series models. How do you structure it?
- A) Three separate queries
- B) Single query with multi-model joins
- C) Materialize views for each model first
- D) Use only one model

**Answer**: B

---

### Practical Coding Questions

**Question 31**: Write a query to find the top 10 customers by total order value in the last 30 days, including their order count and average order value.

**Solution**:
```aql
SELECT 
    c.customer_id,
    c.name,
    COUNT(o.id) as order_count,
    SUM(o.total) as total_value,
    AVG(o.total) as avg_order_value
FROM customers c
INNER JOIN orders o ON c.id = o.customer_id
WHERE o.order_date >= DATE_SUB(NOW(), INTERVAL 30 DAY)
GROUP BY c.customer_id, c.name
ORDER BY total_value DESC
LIMIT 10;
```

---

**Question 32**: Write a graph traversal query to find all products that customers who bought product X also bought, ranked by frequency.

**Solution**:
```aql
FOR order1 IN orders
    FILTER order1.product_id == "products/X"
    FOR order2 IN orders
        FILTER order2.customer_id == order1.customer_id
        FILTER order2.product_id != "products/X"
        COLLECT product_id = order2.product_id 
        WITH COUNT INTO frequency
        SORT frequency DESC
        LIMIT 10
        FOR product IN products
            FILTER product._id == product_id
            RETURN {
                product: product,
                frequency: frequency
            };
```

---

**Question 33**: Optimize this slow query:
```aql
SELECT * FROM users WHERE email LIKE '%@gmail.com';
```

**Solution**:
```aql
-- Add computed column and index
ALTER TABLE users ADD COLUMN email_domain VARCHAR GENERATED ALWAYS AS (
    SUBSTRING(email, POSITION('@' IN email) + 1)
);

CREATE INDEX idx_email_domain ON users (email_domain);

-- Optimized query
SELECT * FROM users WHERE email_domain = 'gmail.com';
```

---

**Question 34**: Write a query using vector similarity to find documents similar to a given document, but only in the same category and published in the last year.

**Solution**:
```aql
LET target_doc = DOCUMENT("documents/123")

FOR doc IN documents
    FILTER doc.category == target_doc.category
    FILTER doc.published_date >= DATE_SUB(NOW(), INTERVAL 1 YEAR)
    FILTER doc._id != target_doc._id
    LET similarity = COSINE_SIMILARITY(doc.embedding, target_doc.embedding)
    FILTER similarity > 0.7
    SORT similarity DESC
    LIMIT 10
    RETURN {
        document: doc,
        similarity: similarity
    };
```

---

**Question 35**: Write a recursive CTE to calculate the total size of a folder including all subfolders.

**Solution**:
```aql
WITH RECURSIVE folder_sizes AS (
    -- Base case: files
    SELECT 
        folder_id,
        file_size as size
    FROM files
    
    UNION ALL
    
    -- Recursive case: subfolders
    SELECT 
        f.parent_folder_id as folder_id,
        fs.size
    FROM folders f
    INNER JOIN folder_sizes fs ON f.folder_id = fs.folder_id
)
SELECT 
    folder_id,
    SUM(size) as total_size
FROM folder_sizes
WHERE folder_id = 'root_folder_id'
GROUP BY folder_id;
```

---

## Project Assignment

### Overview
Build a complete query optimization solution for a multi-model e-commerce application.

### Scenario
You're given a ThemisDB database with:
- 5M products (document model)
- 2M customers (document model)
- 20M orders (document model)
- Product categories graph (graph model)
- Clickstream events (time-series model)
- Product embeddings for search (vector model)

### Requirements

**Part 1: Query Analysis (20%)**
1. Profile 10 provided slow queries
2. Document execution plans
3. Identify bottlenecks
4. Prioritize optimization efforts

**Part 2: Index Strategy (25%)**
1. Design comprehensive index strategy
2. Create required indexes
3. Justify each index
4. Document trade-offs

**Part 3: Query Optimization (30%)**
1. Optimize all 10 queries
2. Rewrite for better performance
3. Use advanced AQL features
4. Achieve 10x performance improvement minimum

**Part 4: Multi-Model Integration (25%)**
1. Build product recommendation system using graph traversal
2. Implement semantic search using vectors
3. Create real-time analytics using time-series
4. Integrate all models in unified queries

**Part 5: Documentation (10%)**
1. Performance comparison report
2. Query optimization techniques used
3. Index strategy rationale
4. Best practices guide

### Deliverables

1. **Code Repository**
   - All optimized queries
   - Index creation scripts
   - Test data generators
   - Performance benchmarks

2. **Documentation**
   - Optimization report (5-10 pages)
   - Before/after comparisons
   - Lessons learned
   - Recommendations

3. **Presentation**
   - 10-minute video walkthrough
   - Key findings and achievements
   - Demo of optimizations

### Evaluation Criteria

| Criterion | Weight | Description |
|-----------|--------|-------------|
| **Performance Improvement** | 30% | Actual query speedup achieved |
| **Technical Correctness** | 25% | Queries produce correct results |
| **Index Strategy** | 20% | Appropriate and justified indexes |
| **Code Quality** | 15% | Clean, readable, maintainable |
| **Documentation** | 10% | Clear explanations and analysis |

### Passing Criteria
- Minimum 70% overall score
- All queries must function correctly
- At least 5x average performance improvement
- Complete documentation

### Submission
- Submit within 2 weeks of exam completion
- Upload to certification portal
- Include GitHub repository link

---

## Certification Criteria

### Overall Requirements

**To pass the TQE certification, you must:**

1. **Score 75% or higher** on written exam (23/30 minimum)
2. **Pass the project assignment** (70% or higher)
3. **Complete within time limits**
4. **Demonstrate practical competency**

### Evaluation Breakdown

**Written Exam (60%)**

| Topic Area | Questions | Weight |
|-----------|-----------|--------|
| Advanced AQL | 8-10 | 25% |
| Graph Algorithms | 6-7 | 20% |
| Vector Search | 4-5 | 15% |
| Query Optimization | 6-7 | 20% |
| Index Strategies | 4-5 | 15% |
| Performance Tuning | 2-3 | 5% |
| **Total** | **30-35** | **100%** |

**Project Assignment (40%)**

- Must achieve 70% or higher
- All sections must be completed
- Code must execute successfully
- Performance targets must be met

### Retake Policy

**If you fail the exam:**
- 14-day waiting period
- $125 retake fee
- Must retake entire exam

**If you fail the project:**
- Can resubmit within 30 days
- $75 resubmission fee
- Address evaluator feedback

---

## Certification Benefits

### Professional Recognition
- Advanced-level digital badge
- Query Expert designation
- Portfolio-ready project
- Industry recognition

### Career Opportunities
- Average 20% salary increase
- Senior developer roles
- Database specialist positions
- Performance engineering roles

### Community Access
- Expert forum access
- Quarterly expert webinars
- Early access to query features
- Influence product roadmap

### Continuing Education
- 25% off advanced training
- Free query optimization workshops
- Conference speaker opportunities
- Mentorship program eligibility

---

## Next Steps

After earning TQE certification:

1. **Apply Your Skills**: Use in production projects
2. **Share Knowledge**: Blog, present, teach
3. **Contribute**: ThemisDB query optimization
4. **Advance**: Pursue TOC or TSC certification
5. **Specialize**: Become query optimization consultant

---

## Support and Resources

**Certification Support**: certification@themisdb.com  
**Technical Questions**: query-experts@themisdb.com  
**Project Help**: project-support@themisdb.com  

**Study Materials**: [https://learn.themisdb.com/query-expert](https://learn.themisdb.com/query-expert)

---

**Ready to become a ThemisDB Query Expert?**

[Register for TQE Certification →](https://certify.themisdb.com/register/tqe)

---

*Last Updated: April 2026*  
*Version: 1.0*  
*© 2025 ThemisDB. All rights reserved.*
