> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# LDBC (Linked Data Benchmark Council) for ThemisDB

## Overview

**LDBC Social Network Benchmark** is the standard benchmark for graph databases, focusing on social network workloads with complex graph traversals and analytical queries.

**Official Website:** https://ldbcouncil.org/

## What is LDBC?

LDBC models a social network similar to Facebook or LinkedIn with:
- **Persons** who know each other
- **Posts** and **Comments** created by persons
- **Forums** where persons are members
- **Tags** applied to posts, comments, and forums
- **Places** (cities, countries) where persons live
- **Organizations** (companies, universities) where persons study/work

### LDBC vs Other Benchmarks

| Feature | LDBC | TPC-C | TPC-H | Neo4j Benchmarks |
|---------|------|-------|-------|------------------|
| **Focus** | Graph traversals | Transactions | Analytics | Graph-specific |
| **Query Type** | Multi-hop | Point queries | Joins/Aggregations | Pathfinding |
| **Primary Metric** | Query latency | tpmC | QphH@SF | ms/query |
| **Use Case** | Social networks | E-commerce | Business intelligence | Graph analytics |

## LDBC Workloads

### Interactive Workload (SNB-I)

**Complex Reads (14 queries):**
- IC1: Friends with a given name
- IC2: Recent posts by friends
- IC3: Friends and friends-of-friends from countries  
- IC4: Tag statistics
- IC5: Forum membership
- IC6: Tag co-occurrence
- IC7: Recent likers
- IC8: Recent comments
- IC9: Recent posts by friends-of-friends
- IC10: Friend recommendations
- IC11: Working at company since year
- IC12: Comments on posts with tags
- IC13: Shortest path between persons
- IC14: Weighted shortest path

**Short Reads (7 queries):**
- IS1-IS7: Simple lookups (person details, messages, etc.)

**Updates (8 operations):**
- IU1-IU8: Add/delete persons, posts, comments, friendships

### Business Intelligence Workload (SNB-BI)

**25 analytical queries** combining graph traversal with aggregations:
- BI1: Posting statistics by country
- BI2: Top tags per month
- BI3: Popular topics in country
- BI4: Popular topics in 2 countries
- BI5: Top posters per country/topic
- ... (20 more queries)

### Social Network Generation (Datagen)

- **Scale Factor (SF):** Determines network size
  - SF=1: ~11K persons, ~1.7M edges (~1GB)
  - SF=10: ~110K persons, ~17M edges (~10GB)
  - SF=100: ~1.1M persons, ~170M edges (~100GB)
  - SF=1000: ~11M persons, ~1.7B edges (~1TB)

## Schema

### Core Entities

1. **Person**
   - id, firstName, lastName, gender, birthday, locationIP
   - birthPlace (City), lives (City)
   - email[], speaks[]

2. **Post**
   - id, imageFile, creationDate, locationIP, browserUsed, language, content
   - creator (Person)
   - containerForum (Forum)
   - locationCountry (Country)

3. **Comment**
   - id, creationDate, locationIP, browserUsed, content
   - creator (Person)
   - replyOfPost (Post) or replyOfComment (Comment)
   - locationCountry (Country)

4. **Forum**
   - id, title, creationDate
   - moderator (Person)

5. **Tag, TagClass**
   - Hierarchical classification system

6. **Place** (City, Country, Continent)
   - Hierarchical location data

7. **Organization** (Company, University)
   - Work/study history

### Relationships

- **Person-Person:** knows (friendship, bidirectional)
- **Person-Post:** likes
- **Person-Comment:** likes
- **Person-Forum:** hasMember
- **Person-Organization:** workAt, studyAt
- **Post/Comment-Tag:** hasTag

## Implementation Plan

### Phase 5.3: LDBC Foundation (Week 3)
- [ ] LDBC schema (Person, Post, Comment, Forum, Tag, Place)
- [ ] Data generation (SF=0.1 for testing, ~100MB)
- [ ] 5 representative queries:
  - IC1: Friends with given name (simple traversal)
  - IC2: Recent posts by friends (1-hop + filter)
  - IC3: Friends-of-friends (2-hop traversal)
  - IC13: Shortest path (pathfinding)
  - BI1: Posting statistics (aggregation)
- [ ] Build system integration
- [ ] Documentation

### Phase 5.3 Extended: Full LDBC Suite (Future)
- [ ] All 14 complex reads
- [ ] All 7 short reads
- [ ] 8 update operations
- [ ] 25 BI queries
- [ ] Performance comparison with Neo4j

## Performance Targets

### Industry Baselines (SF=10, 8-core, 32GB, NVMe)

| Database | IC13 (Shortest Path) | IC2 (Recent Posts) | Notes |
|----------|---------------------|-------------------|-------|
| **Neo4j** | 15-30ms | 5-10ms | Graph-native, optimized |
| **PostgreSQL+AGE** | 40-80ms | 20-40ms | Relational with graph extension |
| **TigerGraph** | 10-20ms | 3-8ms | Distributed graph |

### ThemisDB Targets (SF=1, 8-core, 32GB, NVMe)

| Query | Target | Baseline (Neo4j) |
|-------|--------|------------------|
| IC1 (Simple) | < 10ms | ~5ms |
| IC2 (1-hop) | < 20ms | ~10ms |
| IC3 (2-hop) | < 50ms | ~30ms |
| IC13 (Shortest path) | < 100ms | ~20ms |
| BI1 (Aggregation) | < 500ms | ~200ms |

## Usage (Planned)

```bash
# Build
cd build
cmake .. -DTHEMIS_BUILD_BENCHMARKS=ON
make bench_ldbc

# Run all LDBC queries
./bench_ldbc

# Run specific query
./bench_ldbc --benchmark_filter="IC13_ShortestPath"
./bench_ldbc --benchmark_filter="IC2_RecentPosts"

# Export results
./bench_ldbc --benchmark_out=ldbc_results.json --benchmark_out_format=json
```

## Status

**Phase 5.3:** Planned for Week 3 of Phase 5  
**Priority:** High (graph is a key ThemisDB feature)  
**Complexity:** Medium-High (graph traversal algorithms)

## References

- **LDBC SNB Specification:** https://ldbcouncil.org/benchmarks/snb/
- **Datagen:** https://github.com/ldbc/ldbc_snb_datagen
- **Academic Paper:** Erling et al., "The LDBC Social Network Benchmark" (SIGMOD 2015)
- **Neo4j Results:** https://ldbcouncil.org/benchmarks/snb-results/

## License

This benchmark is based on the LDBC Social Network Benchmark specifications. LDBC is a registered trademark of the Linked Data Benchmark Council.
