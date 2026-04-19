# AQL Query Patterns for Ethical Discourse Retrieval

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

This document provides concrete AQL (Arango Query Language) patterns for retrieving ethical discourse data from ThemisDB's multi-model architecture.

## Pattern 1: Textual Similarity Search

**Use Case**: Find historically similar ethical dilemmas

```aql
SELECT 
    dilemma_id,
    description,
    TEXT_SIMILARITY(description, @query, threshold=0.65) AS similarity_score,
    decision_count,
    avg_satisfaction_score
FROM ethical_dilemmas
WHERE TEXT_SIMILARITY(description, @query, threshold=0.65) > 0
ORDER BY similarity_score DESC
LIMIT @limit
```

**Parameters**:
- `@query`: Current dilemma description
- `@limit`: Maximum results (default: 10)

**Example**:
```python
params = {
    'query': 'Should an AI prioritize privacy over security?',
    'limit': 10
}
```

## Pattern 2: Philosophy-Specific Arguments

**Use Case**: Retrieve arguments from a specific philosophical school

```aql
SELECT 
    argument_id,
    content,
    argument_type,
    strength,
    principle_basis,
    created_at
FROM ethical_arguments
WHERE philosophy_school = @school
  AND argument_type IN @types
ORDER BY 
    CASE strength
        WHEN 'decisive' THEN 4
        WHEN 'strong' THEN 3
        WHEN 'moderate' THEN 2
        ELSE 1
    END DESC,
    created_at DESC
LIMIT @limit
```

**Parameters**:
- `@school`: Philosophy school ID (e.g., 'kantianismus')
- `@types`: Argument types (e.g., ['pro', 'contra'])
- `@limit`: Maximum results (default: 20)

**Example**:
```python
params = {
    'school': 'kant',
    'types': ['pro', 'contra'],
    'limit': 20
}
```

## Pattern 3: Best-Practice Synthesis

**Use Case**: Retrieve highly-rated decisions for learning

```aql
SELECT 
    d.decision_id,
    d.decision_text,
    d.primary_philosophy,
    d.supporting_philosophies,
    d.satisfaction_score,
    d.consensus_level,
    d.argument_chain_ids,
    d.created_at
FROM ethical_decisions d
JOIN ethical_dilemmas e ON d.dilemma_id = e.dilemma_id
WHERE e.category = @category
GROUP BY d.decision_id
HAVING d.satisfaction_score > @min_satisfaction
ORDER BY d.satisfaction_score DESC, d.consensus_level DESC
LIMIT @limit
```

**Parameters**:
- `@category`: Dilemma category (e.g., 'bioethics')
- `@min_satisfaction`: Minimum satisfaction (default: 0.75)
- `@limit`: Maximum results (default: 5)

**Example**:
```python
params = {
    'category': 'autonomous_systems',
    'min_satisfaction': 0.80,
    'limit': 5
}
```

## Pattern 4: Vector Semantic Search

**Use Case**: Find semantically similar arguments using embeddings

```aql
SELECT 
    argument_id,
    content,
    philosophy_school,
    argument_type,
    VECTOR_DISTANCE(embedding, @query_embedding) AS distance,
    metadata
FROM vector_ethical_arguments
WHERE VECTOR_DISTANCE(embedding, @query_embedding) < @threshold
ORDER BY distance ASC
LIMIT @limit
```

**Parameters**:
- `@query_embedding`: Vector embedding (e.g., 768-dim array)
- `@threshold`: Distance threshold (default: 0.3)
- `@limit`: Maximum results (default: 15)

**Example**:
```python
params = {
    'query_embedding': [0.123, -0.456, ...],  # 768 dimensions
    'threshold': 0.3,
    'limit': 15
}
```

## Pattern 5: Argument Chain Traversal

**Use Case**: Follow chains of arguments through graph relationships

```aql
MATCH path = (start:EthicalArgument {id: @start_id})
             -[:supports|counters|rebuts*1..@max_depth]->(related:EthicalArgument)
RETURN 
    path,
    related.id AS argument_id,
    related.content AS content,
    related.philosophy_school AS school,
    length(path) AS chain_depth
ORDER BY length(path) ASC
```

**Parameters**:
- `@start_id`: Starting argument ID
- `@max_depth`: Maximum traversal depth (default: 5)

**Example**:
```python
params = {
    'start_id': 'arg_kant_001',
    'max_depth': 5
}
```

## Pattern 6: Temporal Filtering

**Use Case**: Retrieve recent debates for temporal context

```aql
SELECT 
    dilemma_id,
    description,
    category,
    created_at,
    decision_count,
    participating_schools
FROM ethical_dilemmas
WHERE created_at >= @start_date
  AND (@category IS NULL OR category = @category)
ORDER BY created_at DESC
LIMIT @limit
```

**Parameters**:
- `@start_date`: Start date (ISO format)
- `@category`: Optional category filter
- `@limit`: Maximum results (default: 20)

**Example**:
```python
from datetime import datetime, timedelta

params = {
    'start_date': (datetime.now() - timedelta(days=30)).isoformat(),
    'category': 'privacy',
    'limit': 20
}
```

## Pattern 7: Multi-Philosophy Consensus

**Use Case**: Find decisions with broad philosophical agreement

```aql
SELECT 
    d.decision_id,
    d.decision_text,
    d.primary_philosophy,
    d.supporting_philosophies,
    d.consensus_level,
    d.satisfaction_score,
    ARRAY_LENGTH(d.supporting_philosophies) + 1 AS philosophy_count
FROM ethical_decisions d
WHERE ARRAY_LENGTH(d.supporting_philosophies) + 1 >= @min_count
  AND d.consensus_level >= @min_consensus
ORDER BY d.consensus_level DESC, philosophy_count DESC
LIMIT @limit
```

**Parameters**:
- `@min_count`: Minimum philosophies (default: 3)
- `@min_consensus`: Minimum consensus level (default: 0.8)
- `@limit`: Maximum results (default: 10)

**Example**:
```python
params = {
    'min_count': 3,
    'min_consensus': 0.8,
    'limit': 10
}
```

## Pattern 8: Outcome-Based Filtering

**Use Case**: Retrieve decisions with tracked positive outcomes

```aql
SELECT 
    d.decision_id,
    d.decision_text,
    d.primary_philosophy,
    o.overall_quality,
    o.satisfaction_score,
    o.feasibility_score,
    o.long_term_impact_score
FROM ethical_decisions d
JOIN decision_outcomes o ON d.decision_id = o.decision_id
WHERE o.overall_quality >= @min_quality
  AND o.timestamp >= @start_date
ORDER BY o.overall_quality DESC
LIMIT @limit
```

**Parameters**:
- `@min_quality`: Minimum quality score (default: 0.75)
- `@start_date`: Start date for outcomes
- `@limit`: Maximum results (default: 20)

**Example**:
```python
params = {
    'min_quality': 0.80,
    'start_date': '2024-01-01T00:00:00Z',
    'limit': 20
}
```

## Pattern 9: Cross-Philosophy Debate Analysis

**Use Case**: Analyze how different philosophies interact on specific topics

```aql
SELECT 
    topic,
    philosophy_school,
    argument_type,
    COUNT(*) AS argument_count,
    AVG(strength_numeric) AS avg_strength,
    ARRAY_AGG(content LIMIT 5) AS sample_arguments
FROM (
    SELECT 
        a.philosophy_school,
        a.argument_type,
        a.content,
        d.category AS topic,
        CASE a.strength
            WHEN 'decisive' THEN 4
            WHEN 'strong' THEN 3
            WHEN 'moderate' THEN 2
            ELSE 1
        END AS strength_numeric
    FROM ethical_arguments a
    JOIN argument_chains c ON a.id IN c.arguments
    JOIN ethical_dilemmas d ON c.dilemma_id = d.dilemma_id
    WHERE d.category = @topic
)
GROUP BY topic, philosophy_school, argument_type
ORDER BY argument_count DESC
```

**Parameters**:
- `@topic`: Topic/category to analyze

**Example**:
```python
params = {
    'topic': 'autonomous_systems'
}
```

## Combined RAG Context Query

**Use Case**: Build comprehensive context from multiple patterns

```python
# Pseudo-code showing combined retrieval
def build_comprehensive_rag_context(dilemma, philosophies, category):
    context = {
        # Pattern 1: Similar dilemmas
        'similar': query_pattern_1(dilemma),
        
        # Pattern 2: Philosophy arguments
        'arguments': {
            phil: query_pattern_2(phil) for phil in philosophies
        },
        
        # Pattern 3: Best practices
        'best_practices': query_pattern_3(category),
        
        # Pattern 4: Semantic similarity
        'semantic': query_pattern_4(embed(dilemma)),
        
        # Pattern 6: Recent context
        'recent': query_pattern_6(category, days=30),
        
        # Pattern 7: Consensus
        'consensus': query_pattern_7()
    }
    return context
```

## Performance Optimization

### Index Recommendations

```aql
-- Text similarity index
CREATE INDEX idx_dilemma_text ON ethical_dilemmas (description) FULLTEXT

-- Philosophy + type index
CREATE INDEX idx_arg_school_type ON ethical_arguments (philosophy_school, argument_type)

-- Satisfaction score index
CREATE INDEX idx_decision_satisfaction ON ethical_decisions (satisfaction_score)

-- Vector index (approximate nearest neighbors)
CREATE INDEX idx_vector_args ON vector_ethical_arguments (embedding) VECTOR

-- Temporal index
CREATE INDEX idx_dilemma_created ON ethical_dilemmas (created_at)
```

### Query Tips

1. **Use LIMIT**: Always specify reasonable limits
2. **Filter Early**: Put WHERE clauses before expensive operations
3. **Index Coverage**: Ensure queries use available indexes
4. **Vector Thresholds**: Adjust thresholds based on embedding model
5. **Caching**: Cache frequent queries (similar dilemmas, best practices)
6. **Batch Retrieval**: Combine multiple patterns in single query when possible

## Integration Example

```python
from rag_context_engine import RagContextEngine

rag = RagContextEngine(themis_client=client)

# Build comprehensive context using all patterns
context = rag.build_rag_context(
    dilemma_description="Should AI be allowed to make life-death decisions?",
    philosophy_schools=['kant', 'utilitarianism', 'virtue_ethics'],
    dilemma_category='autonomous_systems'
)

# Format for prompt injection
prompt_context = rag.format_context_for_prompt(context)

# Use in ethical decision-making
full_prompt = f"""
{base_prompt}

{prompt_context}

Provide your analysis...
"""
```

## Testing Queries

```bash
# Test pattern 1
arangosh --server.endpoint tcp://localhost:8529 \
  --server.database themis_ethics \
  --javascript.execute-string "db._query('SELECT ...')"

# Benchmark query performance
arangosh --time true --javascript.execute-string "..."
```

## References

- ArangoDB AQL Documentation: https://www.arangodb.com/docs/stable/aql/
- ThemisDB Multi-Model Architecture: See main documentation
- Vector Search Configuration: See vector_search_config.yaml
