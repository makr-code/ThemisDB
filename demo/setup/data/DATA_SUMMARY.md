# ThemisDB Demo Data Summary

## Document Collection (demo_articles)
- **Total Articles:** 13
- **Topics:** AI, Machine Learning, Quantum Computing, Data Science, Databases
- **Fields:** title, author, category, published, content, tags, citation_count

## Vector Collection (demo_embeddings)
- **Total Embeddings:** 13
- **Dimensions:** 128
- **Fields:** id, doc_id, title, author, embedding, score, relevance_tags

## Graph Collection (demo_knowledge_graph)
- **Total Nodes:** 21
- **Node Types:** researcher (10), paper (7), conference (4)
- **Total Edges:** 18
- **Relationship Types:** wrote, cites, collaborates_with, presented_at

## Import Instructions

```powershell
# Import document collection
themisctl batch-insert --collection demo_articles < demo_articles.jsonl

# Import vector collection
themisctl batch-insert --collection demo_embeddings < demo_embeddings.jsonl

# Import graph collection
themisctl batch-insert --collection demo_knowledge_graph < demo_knowledge_graph_nodes.jsonl
themisctl batch-insert --collection demo_knowledge_graph --edges < demo_knowledge_graph_edges.jsonl
```

## Sample Queries

### Document Search
```aql
FOR doc IN demo_articles
  FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%'
  SORT doc.published DESC
  LIMIT 5
  RETURN doc
```

### Vector Search
```aql
FOR vec IN demo_embeddings
  LET similarity = COSINE_SIMILARITY(vec.embedding, @query_embedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 5
  RETURN { title: vec.title, similarity: similarity }
```

### Graph Traversal
```aql
FOR researcher IN demo_knowledge_graph
  FILTER researcher.type == 'researcher'
  FOR paper IN 1..2 OUTBOUND researcher._id graph_edges
    FILTER paper.type == 'paper'
  RETURN { researcher: researcher.name, paper: paper.title }
```
