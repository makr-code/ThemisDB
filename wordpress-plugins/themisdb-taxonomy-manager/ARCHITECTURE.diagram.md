# Intelligent Taxonomy Management - Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    WordPress Post Editor / Import                        │
│                                                                           │
│  User creates/edits post  →  Auto-extraction triggers on save           │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    ThemisDB_Taxonomy_Extractor                           │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ 1. Extract text from title + content                          │      │
│  │ 2. Tokenize and filter with enhanced stop words              │      │
│  │ 3. Extract phrases (2-3 words) and keywords                  │      │
│  │ 4. Apply exclude patterns (months, numbers, dates)           │      │
│  └──────────────────────────────────────────────────────────────┘      │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         ThemisDB_TFIDF                                   │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ Score each term:                                              │      │
│  │  • TF: Term frequency in document                            │      │
│  │  • IDF: Inverse document frequency (uniqueness)              │      │
│  │  • TF-IDF = TF × log(Total Docs / Docs with Term)           │      │
│  │  • Return top 20 terms sorted by relevance                   │      │
│  └──────────────────────────────────────────────────────────────┘      │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                  Separate Categories & Tags                              │
│  ┌───────────────────────────────┬──────────────────────────────┐      │
│  │  CATEGORIES (Max 5)           │  TAGS (Max 10)               │      │
│  │  • Broad terms (2+ words)     │  • Specific terms (1 word)   │      │
│  │  • General concepts           │  • Technical keywords        │      │
│  │  • Hierarchical               │  • Flat structure            │      │
│  │  • "Vector Search"            │  • "JWT", "OAuth", "Docker"  │      │
│  └───────────────────────────────┴──────────────────────────────┘      │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                   ThemisDB_Taxonomy_Manager                              │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ FOR EACH CATEGORY:                                            │      │
│  │  1. find_matching_category()                                  │      │
│  │     • Exact match: "Security" = "Security"                   │      │
│  │     • Levenshtein ≤2: "Database" ≈ "Databases"              │      │
│  │     • Synonyms: "DB" = "Database"                            │      │
│  │     • Similarity ≥85%                                         │      │
│  │                                                               │      │
│  │  2. If not found, find_semantic_parent()                     │      │
│  │     • Check semantic mapping                                  │      │
│  │     • "OAuth" → Security                                      │      │
│  │     • "Vector" → LLM Integration                             │      │
│  │     • Create as child of parent                              │      │
│  │                                                               │      │
│  │  3. assign_with_hierarchy()                                  │      │
│  │     • Assign category + all parent categories                │      │
│  │     • Respect max_categories limit                           │      │
│  └──────────────────────────────────────────────────────────────┘      │
└────────────────────────────┬────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         WordPress Database                               │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ wp_terms: Category/tag data                                   │      │
│  │ wp_term_taxonomy: Hierarchical relationships                  │      │
│  │ wp_term_relationships: Post assignments                       │      │
│  └──────────────────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                    Analytics Dashboard (Admin)                           │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ ThemisDB_Taxonomy_Analytics                                   │      │
│  │  • get_taxonomy_statistics()                                  │      │
│  │  • get_consolidation_suggestions()                            │      │
│  │  • calculate_similarity()                                     │      │
│  │  • consolidate_categories()                                   │      │
│  │  • cleanup_unused_terms()                                     │      │
│  └──────────────────────────────────────────────────────────────┘      │
│                                                                           │
│  ┌───────────────┬───────────────┬───────────────┬───────────────┐    │
│  │ Total         │ Total         │ Unused        │ Consolidation │    │
│  │ Categories    │ Tags          │ Terms         │ Suggestions   │    │
│  │   [24]        │   [67]        │   [8]         │   [3]         │    │
│  │               │               │ [Cleanup]     │ [Auto Merge]  │    │
│  └───────────────┴───────────────┴───────────────┴───────────────┘    │
│                                                                           │
│  Consolidation Suggestions:                                              │
│  ┌──────────────┬──────────────┬────────────┬───────┬─────────┐       │
│  │ Term 1       │ Term 2       │ Similarity │ Posts │ Action  │       │
│  ├──────────────┼──────────────┼────────────┼───────┼─────────┤       │
│  │ Database     │ Databases    │    92%     │  15   │ [Merge] │       │
│  │ Authentication│ Auth         │    88%     │  23   │ [Merge] │       │
│  │ Performance  │ Optimization │    82%     │  31   │ [Merge] │       │
│  └──────────────┴──────────────┴────────────┴───────┴─────────┘       │
└─────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                      Semantic Mapping                                    │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ Security                                                      │      │
│  │  ├─ Authentication                                            │      │
│  │  │   ├─ OAuth                                                │      │
│  │  │   └─ JWT                                                  │      │
│  │  ├─ Encryption                                               │      │
│  │  │   ├─ SSL/TLS                                             │      │
│  │  │   └─ AES                                                 │      │
│  │                                                               │      │
│  │ LLM Integration                                              │      │
│  │  ├─ Vector Search                                            │      │
│  │  │   ├─ Embeddings                                          │      │
│  │  │   └─ Similarity                                          │      │
│  │  ├─ RAG                                                      │      │
│  │                                                               │      │
│  │ Performance                                                   │      │
│  │  ├─ Caching                                                  │      │
│  │  │   ├─ Redis                                               │      │
│  │  │   └─ Memcached                                           │      │
│  │  ├─ Optimization                                             │      │
│  │                                                               │      │
│  │ Development                                                   │      │
│  │  ├─ API                                                      │      │
│  │  │   ├─ REST                                                │      │
│  │  │   └─ gRPC                                                │      │
│  │  ├─ Containers                                               │      │
│  │  │   ├─ Docker                                              │      │
│  │  │   └─ Kubernetes                                          │      │
│  └──────────────────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                      Configuration Flow                                  │
│                                                                           │
│  Settings Page                     Plugin Behavior                      │
│  ┌────────────────────┐            ┌─────────────────────┐            │
│  │ Max Categories: 5  │────────────▶│ Limit assignment    │            │
│  │ Max Tags: 10       │────────────▶│ to configured max   │            │
│  │ Min TF-IDF: 0.5    │────────────▶│ Filter low scores   │            │
│  │ Similarity: 0.8    │────────────▶│ Merge suggestions   │            │
│  │ Prefer Existing: ✓ │────────────▶│ Match before create │            │
│  │ Auto-consolidate:✓ │────────────▶│ Scheduled merging   │            │
│  └────────────────────┘            └─────────────────────┘            │
└─────────────────────────────────────────────────────────────────────────┘
```

## Data Flow Example

**Input Post:**
```
Title: "Implementing JWT Authentication with OAuth 2.0"
Content: "This guide shows how to implement JWT tokens for secure 
          authentication using OAuth 2.0 protocol with SSL encryption..."
```

**Processing:**
```
1. Extraction:
   Raw terms: ["JWT", "Authentication", "OAuth", "2.0", "tokens", 
                "secure", "SSL", "encryption", "guide", "implement"]

2. Filtering (Stop words + Patterns):
   Removed: ["2.0", "guide", "implement"]
   Kept: ["JWT", "Authentication", "OAuth", "tokens", 
          "secure", "SSL", "encryption"]

3. TF-IDF Scoring:
   JWT: 0.85
   Authentication: 0.78
   OAuth: 0.82
   SSL: 0.73
   encryption: 0.69
   tokens: 0.45
   secure: 0.41

4. Separation:
   Categories: ["Authentication"] (broad, 2+ words implied in context)
   Tags: ["JWT", "OAuth", "SSL", "encryption"]

5. Semantic Mapping:
   "Authentication" → matches "Authentication" under "Security"
   Assigns: Security (parent) + Authentication (child)

6. Final Assignment:
   Categories: [Security, Authentication]
   Tags: [JWT, OAuth, SSL, encryption]
```

## Benefits Visualization

```
BEFORE                              AFTER
──────                              ─────
Categories: 52                      Categories: 24
├─ Januar                          ├─ Security
├─ 2026                            │  ├─ Authentication
├─ test                            │  └─ Encryption
├─ 01                              ├─ LLM Integration
├─ Database                        │  ├─ Vector Search
├─ Databases                       │  └─ RAG
├─ DB                              ├─ Performance
├─ ...meaningless...               │  └─ Caching
                                   ├─ Development
Hierarchy: None                    │  └─ API
Duplicates: Many                   └─ Operations
Overlaps: Yes                          └─ Monitoring

                                   Hierarchy: 3 levels
                                   Duplicates: None
                                   Overlaps: None
```
