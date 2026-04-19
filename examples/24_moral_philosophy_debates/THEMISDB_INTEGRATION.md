> **Hinweis:** API-Signaturen gegen aktuelle Endpunkte/Typen prüfen. Abweichungen mit `<!-- TODO: verify API -->` markieren.

# ThemisDB Integration Guide

## Overview

This example demonstrates the **multi-model capabilities** of ThemisDB by utilizing four different storage paradigms simultaneously:

1. **Graph Storage** - Philosophy relationships and argument connections
2. **Vector Storage** - Semantic search over arguments and philosophies  
3. **Timeline Storage** - Temporal tracking of debates and consensus evolution
4. **Relational Storage** - Structured metadata and analytics

## Storage Models in Detail

### 1. Graph Storage

Graph storage models the **relationships between entities** in the philosophical debate system.

#### Philosophy Relationships

```python
# Create influence relationship
client.create_philosophy_relationship(
    from_school="kant",
    to_school="rawls",
    relationship_type="influences",
    metadata={"aspect": "justice_theory", "strength": "strong"}
)

# Create criticism relationship
client.create_philosophy_relationship(
    from_school="nietzsche",
    to_school="kant",
    relationship_type="criticizes",
    metadata={"focus": "categorical_imperative"}
)
```

**Graph Edges:**
- `influences`: One philosophy influenced another (e.g., Kant → Rawls)
- `criticizes`: One philosophy criticizes another (e.g., Nietzsche → Kant)
- `synthesizes`: One philosophy synthesizes ideas from another (e.g., Habermas → Kant + Marx)
- `opposes`: Direct opposition between philosophies (e.g., Utilitarianism ↔ Deontology)

#### Argument Relationships

```python
# Create argument chain
client.create_argument_relationship(
    from_argument_id="arg_123",
    to_argument_id="arg_456",
    relationship_type="responds_to"
)

# Track support/refutation
client.create_argument_relationship(
    from_argument_id="arg_789",
    to_argument_id="arg_123",
    relationship_type="supports"
)
```

**Graph Edges:**
- `supports`: Argument A supports Argument B
- `refutes`: Argument A refutes Argument B
- `builds_on`: Argument A builds on Argument B's ideas
- `responds_to`: Argument A directly responds to Argument B

#### Use Cases

1. **Philosophy Network Analysis**: Discover clusters of related philosophies
2. **Influence Tracing**: Track intellectual lineages (e.g., Greek → Enlightenment → Modern)
3. **Debate Flow Visualization**: Visualize argument chains and response patterns
4. **Conflict Detection**: Identify opposing viewpoints for balanced debates

### 2. Vector Storage

Vector storage enables **semantic search** using embeddings.

#### Argument Embeddings

```python
# Store argument with embedding
embedding = generate_embedding(argument.content)  # Using sentence-transformers
client.store_argument_embedding(
    argument_id="arg_123",
    embedding=embedding,
    text=argument.content
)

# Search for similar arguments
query_embedding = generate_embedding("What is the role of duty in ethics?")
similar_args = client.search_similar_arguments(
    query_embedding=query_embedding,
    limit=10,
    min_similarity=0.75
)
```

#### Philosophy Embeddings

```python
# Store philosophy school embedding
theses_text = "\n".join(philosophy.main_theses)
embedding = generate_embedding(theses_text)
client.store_philosophy_embedding(
    school="kant",
    embedding=embedding,
    main_theses=philosophy.main_theses
)
```

#### Use Cases

1. **Semantic Argument Search**: Find arguments similar to a query regardless of exact wording
2. **Philosophy Matching**: Discover philosophies with similar core principles
3. **Context Retrieval**: Find relevant past arguments for current debate topics
4. **Duplicate Detection**: Identify when similar arguments have been made before
5. **Knowledge Graph Enhancement**: Use semantic similarity to suggest new relationships

### 3. Timeline Storage

Timeline storage tracks the **temporal evolution** of debates.

#### Timeline Events

```python
# Track debate start
client.add_timeline_event(
    debate_id="debate_001",
    event_type="debate_started",
    timestamp=datetime.now(),
    data={
        "news_article": article.title,
        "participants": ["kant", "mill", "rawls"],
        "dimensions": ["moral", "social", "political"]
    }
)

# Track argument posting
client.add_timeline_event(
    debate_id="debate_001",
    event_type="argument_posted",
    timestamp=datetime.now(),
    data={
        "philosopher": "kant",
        "dimension": "moral",
        "argument_id": "arg_123"
    }
)

# Track AI synthesis
client.add_timeline_event(
    debate_id="debate_001",
    event_type="synthesis_generated",
    timestamp=datetime.now(),
    data={
        "principle": "Human dignity is inviolable",
        "support_count": 3,
        "confidence": 0.85
    }
)
```

#### Consensus Evolution Tracking

```python
# Track how support for a principle changes over time
client.track_consensus_evolution(
    debate_id="debate_001",
    principle="Greatest happiness principle",
    support_level=0.6,
    timestamp=datetime.now()
)
```

#### Use Cases

1. **Debate Replay**: Reconstruct the entire debate flow chronologically
2. **Consensus Tracking**: Visualize how agreement emerges over time
3. **Argument Impact Analysis**: Measure which arguments shifted the debate
4. **Pattern Recognition**: Identify temporal patterns in debate evolution
5. **Performance Metrics**: Track debate duration, argument frequency, synthesis timing

### 4. Relational Storage

Relational storage provides **structured data access** for analytics and queries.

#### Current Implementation

The existing methods use relational-style storage:

```python
# Store debate session (structured metadata)
client.store_debate_session(session)

# Store arguments with categorization
client.store_argument(argument)

# Store news articles with metadata
client.store_news_article(article)
```

#### Statistics and Analytics

```python
stats = client.get_debate_statistics()
# Returns:
# {
#     'total_debates': 150,
#     'total_arguments': 2400,
#     'total_articles': 300,
#     'consensus_rate': 0.65,
#     'most_active_philosophy': 'utilitarianism',
#     'avg_arguments_per_debate': 16.0,
#     'total_relationships': 450,
#     'timeline_events': 3200
# }
```

#### Use Cases

1. **Analytics Dashboard**: Aggregate statistics across all debates
2. **Philosophy Rankings**: Most active, most persuasive philosophies
3. **Topic Analysis**: Which news topics generate most debate
4. **Dimension Analysis**: Which dimensions are most controversial
5. **Participant Metrics**: Track individual philosophy performance

## Integration Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Moral Philosophy Debate System                │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐       │
│  │ News Research │  │ AI Synthesizer│  │ Knowledge     │       │
│  │ Module        │  │ (KI)          │  │ Researcher    │       │
│  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘       │
│          │                   │                   │               │
│          └───────────────────┴───────────────────┘               │
│                              │                                   │
│                   ┌──────────▼──────────┐                       │
│                   │  Debate Chat Engine  │                       │
│                   └──────────┬──────────┘                       │
│                              │                                   │
│                   ┌──────────▼──────────┐                       │
│                   │ ThemisDB Client      │                       │
│                   │ (Multi-Model)        │                       │
│                   └──────────┬──────────┘                       │
│                              │                                   │
└──────────────────────────────┼───────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                         ThemisDB Server                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ Graph        │  │ Vector       │  │ Timeline     │          │
│  │ Storage      │  │ Storage      │  │ Storage      │          │
│  │              │  │              │  │              │          │
│  │ • Relations  │  │ • Embeddings │  │ • Events     │          │
│  │ • Traversal  │  │ • Similarity │  │ • Evolution  │          │
│  │ • Networks   │  │ • Search     │  │ • Sequences  │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                   │
│  ┌──────────────────────────────────────────────────────┐       │
│  │ Relational Storage                                    │       │
│  │ • Structured Data  • Analytics  • Metadata           │       │
│  └──────────────────────────────────────────────────────┘       │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow Examples

### Example 1: Storing a Complete Debate

```python
# 1. Store news article (Relational)
client.store_news_article(article)

# 2. Create debate session (Relational)
session = DebateSession(...)
client.store_debate_session(session)

# 3. Add timeline event (Timeline)
client.add_timeline_event(
    debate_id=session.id,
    event_type="debate_started",
    timestamp=datetime.now(),
    data={"article_id": article.id}
)

# 4. Store each argument with embedding (Relational + Vector)
for argument in arguments:
    client.store_argument(argument)
    embedding = generate_embedding(argument.content)
    client.store_argument_embedding(
        argument_id=argument.id,
        embedding=embedding,
        text=argument.content
    )
    
    # 5. Create argument relationships (Graph)
    if argument.responds_to:
        client.create_argument_relationship(
            from_argument_id=argument.id,
            to_argument_id=argument.responds_to,
            relationship_type="responds_to"
        )

# 6. Track consensus evolution (Timeline)
for principle in ai_synthesis:
    client.track_consensus_evolution(
        debate_id=session.id,
        principle=principle.text,
        support_level=principle.confidence,
        timestamp=datetime.now()
    )
```

### Example 2: Analyzing Philosophy Relationships

```python
# 1. Store philosophy relationships (Graph)
client.create_philosophy_relationship(
    from_school="kant",
    to_school="rawls",
    relationship_type="influences"
)

# 2. Store philosophy embeddings (Vector)
for school, profile in PHILOSOPHY_PROFILES.items():
    embedding = generate_embedding(" ".join(profile.main_theses))
    client.store_philosophy_embedding(
        school=school,
        embedding=embedding,
        main_theses=profile.main_theses
    )

# 3. Query related philosophies (Graph + Vector)
relationships = client.get_philosophy_relationships("kant")
similar = client.search_similar_philosophies(kant_embedding, limit=5)
```

### Example 3: Temporal Analysis

```python
# Get debate timeline
timeline = client.get_debate_timeline(
    debate_id="debate_001",
    start_time=datetime(2024, 1, 1),
    end_time=datetime(2024, 1, 31)
)

# Analyze consensus evolution
for event in timeline:
    if event['event_type'] == 'synthesis_generated':
        print(f"At {event['timestamp']}, principle '{event['data']['principle']}' "
              f"had {event['data']['confidence']} confidence")
```

## Performance Considerations

### Graph Queries
- **Complexity**: O(V + E) for traversal
- **Optimization**: Index on relationship types
- **Best For**: Small-medium networks (<10K nodes)

### Vector Search
- **Complexity**: O(n) linear scan, O(log n) with HNSW index
- **Optimization**: Use approximate nearest neighbor (ANN) algorithms
- **Best For**: Semantic similarity, <100K vectors with ANN

### Timeline Queries
- **Complexity**: O(log n) with time-based indexing
- **Optimization**: Index on timestamp + debate_id
- **Best For**: Range queries, temporal analysis

### Relational Queries
- **Complexity**: O(log n) with proper indexing
- **Optimization**: Index on frequently queried fields
- **Best For**: Structured queries, aggregations, statistics

## Future Enhancements

### Planned Features

1. **Advanced Graph Algorithms**
   - PageRank for philosophy influence scoring
   - Community detection for philosophy clusters
   - Shortest path for intellectual lineage

2. **Enhanced Vector Search**
   - Multi-vector search (argument + context)
   - Hybrid search (semantic + keyword)
   - Cross-lingual embeddings

3. **Real-time Timeline Analytics**
   - Streaming consensus tracking
   - Live debate visualization
   - Predictive modeling

4. **Complex Relational Queries**
   - Multi-dimensional analytics
   - Cross-debate comparisons
   - Philosopher performance metrics

## Example Queries

### Graph: Find Philosophy Influences

```python
# Find all philosophies influenced by Kant
influenced_by_kant = client.get_philosophy_relationships(
    school="kant",
    relationship_type="influences"
)
```

### Vector: Semantic Argument Search

```python
# Find arguments about "duty and obligation"
query = "What is the relationship between moral duty and ethical obligation?"
query_embedding = generate_embedding(query)
similar = client.search_similar_arguments(
    query_embedding=query_embedding,
    limit=10,
    min_similarity=0.75
)
```

### Timeline: Debate Evolution

```python
# Track how a debate evolved over time
timeline = client.get_debate_timeline(
    debate_id="debate_001"
)
for event in timeline:
    print(f"{event['timestamp']}: {event['event_type']} - {event['data']}")
```

### Relational: Statistics

```python
# Get overall statistics
stats = client.get_debate_statistics()
print(f"Total debates: {stats['total_debates']}")
print(f"Average arguments per debate: {stats['avg_arguments_per_debate']}")
print(f"Most active philosophy: {stats['most_active_philosophy']}")
```

## Conclusion

This multi-model approach demonstrates ThemisDB's flexibility and power:

- **Graph** captures the interconnected nature of philosophical thought
- **Vector** enables semantic understanding and similarity
- **Timeline** tracks the evolution of ideas and consensus
- **Relational** provides structure and analytics

Together, these create a comprehensive system for storing, analyzing, and understanding philosophical debates.
