# ThemisDB Demo Queries - Copy & Paste Ready

Use these queries directly in themisctl for the Kickstarter demo video.

---

## 1. SYSTEM STATUS

### Check Server Health
```aql
RETURN { status: "OK", version: "1.0", timestamp: NOW() }
```

### View All Collections
```aql
FOR collection IN collections
RETURN collection.name
```

---

## 2. DOCUMENT SEARCH (Full-Text, SQL-like)

### Simple Filter
```aql
FOR doc IN demo_articles
  FILTER doc.title LIKE '%AI%'
  LIMIT 5
  RETURN doc
```

### Advanced Text Search with Sort
```aql
FOR doc IN demo_articles
  FILTER doc.title LIKE '%machine%' OR doc.content LIKE '%learning%'
  SORT doc.published DESC
  LIMIT 10
  RETURN {
    title: doc.title,
    published: doc.published,
    author: doc.author,
    snippet: SUBSTRING(doc.content, 0, 100)
  }
```

### Aggregation Example
```aql
FOR doc IN demo_articles
  FILTER doc.category == 'research'
  COLLECT category = doc.category WITH COUNT INTO count
  RETURN { category: category, count: count }
```

---

## 3. VECTOR SEARCH (Semantic/Similarity)

### Basic Vector Query
```aql
FOR doc IN demo_embeddings
  LET similarity = COSINE_SIMILARITY(doc.embedding, [0.1, -0.2, 0.8, 0.3])
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 5
  RETURN {
    title: doc.title,
    similarity: ROUND(similarity, 3)
  }
```

### Vector Search with Ranking
```aql
FOR doc IN demo_embeddings
  LET sim = COSINE_SIMILARITY(doc.embedding, @query_vec)
  FILTER sim > 0.5
  SORT sim DESC
  LIMIT 10
  RETURN {
    doc_id: doc._id,
    title: doc.title,
    score: ROUND(sim * 100, 1)  // Convert to percentage
  }
```

### Multi-Vector Fusion (Hybrid Search)
```aql
FOR doc IN demo_embeddings
  LET vec_sim = COSINE_SIMILARITY(doc.embedding, @query_embedding)
  LET text_match = (doc.title LIKE @keyword OR doc.tags CONTAINS @keyword) ? 1.0 : 0.0
  LET combined_score = (vec_sim * 0.7) + (text_match * 0.3)
  FILTER combined_score > 0.5
  SORT combined_score DESC
  LIMIT 5
  RETURN {
    title: doc.title,
    vector_score: ROUND(vec_sim, 3),
    text_match: text_match,
    final_score: ROUND(combined_score, 3)
  }
```

---

## 4. GRAPH NAVIGATION (Relationships)

### Simple Graph Traversal (1 Hop)
```aql
FOR researcher IN demo_graph
  FILTER researcher.type == 'researcher'
  FOR paper IN 1 OUTBOUND researcher._id graph_edges
    FILTER paper.type == 'paper'
  LIMIT 5
  RETURN {
    researcher: researcher.name,
    paper: paper.title
  }
```

### Multi-Hop Traversal (2-3 Hops)
```aql
FOR researcher IN demo_graph
  FILTER researcher.type == 'researcher'
  FOR connection IN 1..2 OUTBOUND researcher._id graph_edges
  LIMIT 8
  RETURN {
    researcher: researcher.name,
    depth: 1,
    connected_entity: connection.name,
    type: connection.type
  }
```

### Path Finding
```aql
FOR start IN demo_graph
  FILTER start.name == 'Alice'
  FOR end IN demo_graph
    FILTER end.name == 'Charlie'
    FOR path IN PATHS(start._id, end._id, 1..3, 'graph_edges')
    RETURN {
      path_length: LENGTH(path.vertices),
      nodes: [v.name FOR v IN path.vertices],
      edges: LENGTH(path.edges)
    }
```

### Degree Centrality (Find Important Nodes)
```aql
FOR node IN demo_graph
  LET in_degree = (
    FOR edge IN graph_edges
    FILTER edge._to == node._id
    RETURN 1
  )
  LET out_degree = (
    FOR edge IN graph_edges
    FILTER edge._from == node._id
    RETURN 1
  )
  SORT (LENGTH(in_degree) + LENGTH(out_degree)) DESC
  LIMIT 5
  RETURN {
    name: node.name,
    in_degree: LENGTH(in_degree),
    out_degree: LENGTH(out_degree),
    total: LENGTH(in_degree) + LENGTH(out_degree)
  }
```

---

## 5. ADVANCED: MULTI-MODEL FUSION

### Document + Vector + Graph Join
```aql
FOR article IN demo_articles
  FILTER article.category == 'research'
  LET related_vectors = (
    FOR vec IN demo_embeddings
    LET sim = COSINE_SIMILARITY(vec.embedding, article.embedding)
    FILTER sim > 0.6
    SORT sim DESC
    LIMIT 3
    RETURN { title: vec.title, similarity: ROUND(sim, 2) }
  )
  LET author_node = (
    FOR node IN demo_graph
    FILTER node.name == article.author
    RETURN node
  )[0]
  LET co_authors = (
    FOR collaborator IN 1 OUTBOUND author_node._id graph_edges
    FILTER collaborator.type == 'researcher'
    RETURN collaborator.name
  )
  RETURN {
    article: article.title,
    author: article.author,
    similar_papers: related_vectors,
    collaborators: co_authors,
    publication_year: article.published
  }
```

---

## 6. LLM & RAG FEATURES

### Natural Language Query (RAG)
```bash
themisctl rag query \
  --collection demo_articles \
  --top-k 3 \
  "What are the latest advances in machine learning for drug discovery?"
```

### Multi-Collection RAG
```bash
themisctl rag query \
  --collection demo_articles \
  --collection demo_embeddings \
  --top-k 5 \
  "Explain quantum computing and its applications"
```

### RAG with Constraints
```bash
themisctl rag query \
  --collection demo_articles \
  --top-k 3 \
  --filter "published > 2024-01-01" \
  --lang en \
  "Recent developments in AI safety"
```

---

## 7. PERFORMANCE & MONITORING

### Query Explain (Execution Plan)
```bash
themisctl query --explain \
  "FOR doc IN demo_articles FILTER doc.title LIKE '%AI%' LIMIT 5 RETURN doc"
```

### Query with Timing
```bash
themisctl query --show-timing \
  "FOR doc IN demo_articles \
   FILTER doc.category == 'research' \
   SORT doc.published DESC \
   LIMIT 100 \
   RETURN doc"
```

### System Statistics
```bash
themisctl admin stats
```

### Index Status
```bash
themisctl admin indexes --collection demo_articles
```

---

## 8. INTERACTIVE REPL MODE

### Start REPL
```bash
themisctl repl --host localhost:8765
```

### Inside REPL - Example Queries
```
> FOR doc IN demo_articles LIMIT 3 RETURN doc
> EXPLAIN SELECT * FROM demo_articles WHERE category = 'AI'
> SHOW COLLECTIONS
> SHOW INDEXES demo_articles
> SHOW STATS
> EXIT
```

---

## 9. BATCH OPERATIONS

### Insert Multiple Documents
```bash
cat << 'EOF' | themisctl batch-insert --collection demo_articles
{"title": "New AI Paper", "author": "Jane Doe", "content": "...", "category": "research"}
{"title": "Machine Learning Guide", "author": "John Smith", "content": "...", "category": "tutorial"}
EOF
```

### Export Results to File
```bash
themisctl query \
  "FOR doc IN demo_articles FILTER doc.category = 'research' LIMIT 100 RETURN doc" \
  --output-format jsonl > results.jsonl
```

### Import from External Source
```bash
themisctl batch-insert --collection demo_articles < data/articles.jsonl
```

---

## 10. PRACTICAL DEMO COMBINATIONS

### "Show Search Capabilities" (1 min)
```bash
# 1. Text Search
themisctl query "FOR doc IN demo_articles FILTER doc.title LIKE '%AI%' LIMIT 5 RETURN doc.title"

# 2. Vector Search
themisctl query "FOR doc IN demo_embeddings LIMIT 5 RETURN {title: doc.title, score: doc.score}"

# 3. Graph
themisctl query "FOR r IN demo_graph FILTER r.type == 'researcher' FOR p IN 1 OUTBOUND r._id graph_edges LIMIT 5 RETURN {name: r.name, connected_to: p.name}"
```

### "Show AI Features" (1.5 min)
```bash
# 1. RAG Query
themisctl rag query --collection demo_articles --top-k 3 "What is machine learning?"

# 2. Performance Stats
themisctl admin stats

# 3. Query Recommendations
themisctl index recommend --collection demo_articles
```

### "Full System Demo" (10 min)
```bash
# Run all demo script
.\demo\kickstarter_demo_script.ps1
```

---

## TIPS FOR DEMO VIDEO

✓ **Read the query first**: "Next query demonstrates graph traversal..."
✓ **Pause between queries**: Let results be seen
✓ **Comment on results**: "See how fast that completed? Millions of documents..."
✓ **Show stats**: Always run `admin stats` to prove performance
✓ **Vary the data**: Use different collections (documents, vectors, graphs)
✓ **Explain concepts**: Connect query to real use-case

---

## COMMON DEMO FLOW (12 min)

1. **Intro** (30 sec): "This is ThemisDB..."
2. **Document Search** (2 min): Text queries, filtering, sorting
3. **Vector Search** (2 min): Semantic similarity, embeddings
4. **Graph** (2 min): Traversal, relationships, multi-hop
5. **RAG/LLM** (2.5 min): Natural language, AI agent
6. **Performance** (1 min): Stats, throughput, latency
7. **Recommendations** (1 min): Index optimization, system insights
8. **Closing** (30 sec): "Fully operational, production-ready"

---

**Total: ~12 minutes - Perfect for Kickstarter!**
