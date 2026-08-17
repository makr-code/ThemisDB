# LLM Wiki Module — Architecture

<!-- Status: PRODUCTION_READY | validated: 2026-08-10 -->

## Overview

The LLM wiki module provides plugin-based integration for external knowledge sources (Confluence, Notion, internal wikis) into ThemisDB's LLM context window, enabling controlled knowledge base access with guardrails, access policies, and dynamic workspace isolation.

## Design Principles

1. **Plugin Architecture:** Multiple wiki backends supported through standardized interface
2. **Workspace Isolation:** Separate knowledge bases per tenant/workspace; no cross-contamination
3. **Access Control:** Hierarchical permission model (public, authenticated, role-based)
4. **Query Optimization:** Caching and indexing for efficient knowledge retrieval
5. **Guardrails:** LLM-specific safety checks and rate limiting
6. **Audit Trail:** All knowledge access logged for compliance and debugging

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  LLM Inference Request                                      │
│  • Query requiring external knowledge                       │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  WikiContextManager (Main API)                              │
│  • Coordinate wiki access and guardrails                    │
│  • Manage workspace isolation                               │
│  • Apply access policies                                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
   ┌──────────┐ ┌──────────┐ ┌──────────────┐
   │Confluence│ │Notion    │ │Internal Wiki │
   │Plugin    │ │Plugin    │ │Plugin        │
   │          │ │          │ │              │
   └────┬─────┘ └────┬─────┘ └──────┬───────┘
        │            │              │
        └────────────┼──────────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │  AccessControl         │
        │  • Permission checks   │
        │  • Workspace isolation │
        │  • Rate limiting       │
        └────────────────────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │  Guardrails & Filters  │
        │  • Content validation  │
        │  • Sensitivity checks  │
        │  • Token counting      │
        └────────────────────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │  Result Cache          │
        │  • Query results       │
        │  • TTL-based eviction  │
        │  • Per-workspace keys  │
        └────────────────────────┘
```

## Core Components

### WikiContextManager

**Purpose:** Unified interface for retrieving contextual knowledge from configured wiki sources.

**Responsibilities:**
- Route queries to appropriate wiki backend(s)
- Apply access control policies
- Enforce guardrails (token limits, content filters)
- Cache results for efficiency
- Track usage for rate limiting and audit

**Public API:**
```cpp
class WikiContextManager {
  Result<WikiContext> getContext(
    const LLMQuery& query,
    const WorkspaceId& workspace,
    const UserId& user
  );
  
  Result<std::vector<WikiArticle>> search(
    const std::string& query_text,
    const WorkspaceId& workspace
  );
};
```

### Plugin Interface

**Purpose:** Standardized contract for wiki backend implementations.

**Responsibilities:**
- Connect to wiki source (API, database, file system)
- Execute queries/searches
- Return article/document results
- Handle authentication with wiki system
- Implement caching and rate limiting

**Plugin Abstraction:**
```cpp
class WikiPlugin {
  virtual Result<std::vector<Article>> search(
    const std::string& query
  ) = 0;
  
  virtual Result<Article> getArticle(const ArticleId& id) = 0;
  
  virtual Result<> authenticate(const Credentials& creds) = 0;
};
```

### Access Control Layer

**Purpose:** Enforce permission policies and workspace isolation.

**Access Model:**
- **Workspace-Level:** Separate knowledge bases per tenant
- **Document-Level:** Granular permissions (public, authenticated, role-based)
- **User-Level:** Identity and role tracking
- **Query-Level:** Rate limiting per user/workspace

**Permission Checks:**
```
Can user access document?
  ├─► Is document public? → YES
  ├─► Is user authenticated? → Check workspace
  └─► Does user have required role? → Check document ACL
```

### Guardrails & Content Filters

**Purpose:** Prevent sensitive content leakage and enforce LLM safety.

**Enforcement Points:**

1. **Content Sensitivity Filtering**
   - Detect and filter: credentials, PII, confidential markings
   - Signature-based (regex patterns, keyword lists)
   - Configurable per workspace

2. **Token Counting**
   - Measure context tokens before including
   - Respect LLM context window limits
   - Graceful truncation if over limit

3. **Rate Limiting**
   - Per-user, per-workspace rate limits
   - Query quota management
   - Token consumption tracking

4. **Audit Logging**
   - All access logged with: user, workspace, documents, timestamp
   - Enables compliance and forensics

### Query Cache

**Purpose:** Reduce repeated queries to wiki backends.

**Strategy:**
- Key: `<workspace_id, query_hash>`
- Value: cached results with TTL
- LRU eviction when capacity exceeded
- Configurable TTL per workspace

**Configuration:**
- Max cache size: configurable (default: 1 GB)
- TTL: configurable (default: 1 hour)
- Enable/disable per workspace

## Data Flow

### Context Retrieval Pipeline

```
LLM Query (+ workspace + user)
  │
  ├─► Query Cache Lookup
  │   ├─► Cache hit → Return cached results
  │   └─► Cache miss → Continue
  │
  ├─► Identify wiki sources
  │
  ├─► For each wiki source:
  │   ├─► Check access permissions (user)
  │   ├─► Execute search query
  │   └─► Get ranked results
  │
  ├─► Apply Guardrails:
  │   ├─► Filter sensitive content
  │   ├─► Count tokens
  │   └─► Rank by relevance
  │
  ├─► Query Cache Store
  │   └─► Cache results with TTL
  │
  └─► Return context to LLM
       ├─► Ranked articles
       ├─► Token count
       └─► Audit event logged
```

### Workspace Isolation Model

```
ThemisDB Cluster
  │
  ├─► Workspace A
  │   ├─► Users: alice, bob
  │   ├─► Wiki Sources: Internal Wiki A, Confluence
  │   ├─► Cache: isolated L1 (workspace A only)
  │   └─► Audit Log: workspace A events only
  │
  ├─► Workspace B
  │   ├─► Users: charlie, diana
  │   ├─► Wiki Sources: Notion, GitHub Wiki
  │   ├─► Cache: isolated L1 (workspace B only)
  │   └─► Audit Log: workspace B events only
  │
  └─► Global
      ├─► Shared Cache L2 (cross-workspace, anonymous)
      └─► Central Audit Log (all events, workspace-tagged)
```

## Plugin Ecosystem

### Built-In Plugins

1. **Confluence Plugin**
   - Atlassian Confluence API integration
   - Authentication: API tokens, OAuth
   - Search: CQL (Confluence Query Language)
   - Caching: REST API rate limit aware

2. **Notion Plugin**
   - Notion API (v1) integration
   - Authentication: ******
   - Search: Notion database queries
   - Caching: TTL-aware

3. **Internal Wiki Plugin**
   - Custom markdown/JSON file system
   - No authentication required
   - Full-text search via indexing
   - File-system watcher for dynamic updates

### Plugin Development Guide

To implement a custom wiki plugin:

1. Extend `WikiPlugin` base class
2. Implement `search()` and `getArticle()` methods
3. Handle authentication and credentials
4. Implement local caching where appropriate
5. Return results in standardized Article format

Example:
```cpp
class CustomWikiPlugin : public WikiPlugin {
  Result<std::vector<Article>> search(const std::string& query) override {
    // Connect to custom wiki API
    // Execute search
    // Parse results into Article objects
    // Apply local caching
    // Return results
  }
};
```

## Concurrency Model

### Thread Safety

1. **Per-Workspace Access:** Multiple readers allowed
   - Search operations don't modify wiki state
   - Read-write lock per workspace
   - Enables concurrent queries

2. **Cache Updates:** Atomic with compare-and-swap
   - Prevents race conditions during cache write
   - Minimal lock contention

3. **Plugin Access:** Thread-safe plugin calls
   - Plugins responsible for own synchronization
   - Context manager serializes plugin invocations

### Synchronization Primitives

- `std::shared_mutex` for per-workspace access control
- `std::atomic<>` for cache counters
- `std::condition_variable` for plugin coordination

## Performance Characteristics

### Target Latencies (P99)

- **Cache Hit:** < 1 ms
- **Single Plugin Query:** < 500 ms (depends on wiki backend)
- **Multi-Plugin Query:** < 1000 ms (parallel queries)
- **Context Retrieval End-to-End:** < 2 seconds

### Throughput

- **Queries/sec:** > 10 concurrent queries
- **Cache Capacity:** > 10k documents
- **Plugin Concurrency:** > 5 concurrent plugin operations

### Resource Consumption

- **Per-Workspace Memory:** < 100 MB (caches + state)
- **Cache Memory:** ~10 KB per cached result
- **Total Memory:** < 1 GB for typical deployment

## Error Handling

### Graceful Degradation

1. **Plugin Unavailable** → Skip that source; continue with others
2. **Query Timeout** → Return partial results from other sources
3. **Access Denied** → Return empty results; log audit event
4. **Cache Corruption** → Bypass cache; recompute results
5. **Token Limit Exceeded** → Truncate results to fit limit

### Error Codes (E9600–E9699)

- E9600: Wiki plugin not found
- E9601: Authentication failed with wiki backend
- E9602: Query timeout
- E9603: Access denied for user/workspace
- E9604: Content sensitivity filter blocked result

## Integration Points

### LLM Inference Pipeline

Wiki context is fetched and injected into LLM prompt:
1. LLM query received
2. WikiContextManager retrieves relevant context
3. Context injected into system prompt
4. LLM generates response with context

### Document Indexing

When documents reference wiki articles:
1. Extract article references
2. Validate user access to articles
3. Pre-fetch and cache for RAG pipeline

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`README.md`](README.md) — Module overview
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/llm_wiki/wiki_context_manager.h`](../../include/llm_wiki/wiki_context_manager.h) — Public API
