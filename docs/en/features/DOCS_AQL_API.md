# Documentation Assistant AQL Functions

## Overview

ThemisDB provides integrated documentation assistance through AQL functions that leverage a pre-compiled documentation database and LLM capabilities. These functions enable users to query documentation, get configuration help, and troubleshoot issues directly from AQL queries.

**NEW: Unified HELP() Function** - A single intelligent function that automatically detects your intent and provides the most appropriate response. This is the **recommended** way to access documentation assistance.

## Primary Function (Recommended)

### HELP(query: string) -> string

**The unified intelligent helper function that automatically determines intent from your query.**

**Syntax:**
```sql
SELECT HELP(question_or_query) AS answer;
```

**Parameters:**
- `question_or_query` (string): Any question, problem description, or search request

**Returns:**
- String containing the appropriate response (answer, guidance, search results, or solution)

**How it works:**
The `HELP()` function analyzes your query and automatically routes it to the appropriate underlying function:
- **Configuration questions** (contains: "config", "configure", "setting", "setup") → Configuration help
- **Troubleshooting** (contains: "error", "fail", "problem", "issue", "hang", "crash") → Troubleshooting help
- **Search requests** (contains: "search", "find", "look for") → Document search
- **General questions** → RAG-powered query with context

**Examples:**
```sql
-- General documentation questions (RAG-powered)
SELECT HELP('How do I enable sharding?') AS answer;
SELECT HELP('What is vector search?') AS info;
SELECT HELP('Explain RAID configuration') AS explanation;

-- Configuration help (auto-detected)
SELECT HELP('Configure security settings') AS guide;
SELECT HELP('Setup replication') AS setup_guide;
SELECT HELP('How to configure sharding?') AS config;

-- Troubleshooting (auto-detected)
SELECT HELP('Server hangs at startup') AS solution;
SELECT HELP('Connection error on port 8529') AS fix;
SELECT HELP('Database fails to start') AS troubleshooting;

-- Document search (auto-detected)
SELECT HELP('Search for RAID documentation') AS search_results;
SELECT HELP('Find information about vector embeddings') AS docs;
SELECT HELP('Look for security best practices') AS references;

-- Works with compound queries
SELECT HELP('I have an error: server not responding') AS solution;
SELECT HELP('Need to configure TLS, how?') AS guide;
```

---

## Advanced Functions (For Explicit Control)

These functions are available when you need explicit control over the type of operation. The `HELP()` function uses these internally.

### DOCS_QUERY(query: string) -> string

Query the documentation database with natural language and get an AI-generated answer.

**Syntax:**
```sql
SELECT DOCS_QUERY(query_text) AS answer;
```

**Parameters:**
- `query_text` (string): Natural language question about ThemisDB

**Returns:**
- String containing the generated answer

**Examples:**
```sql
-- Basic usage
SELECT DOCS_QUERY('How do I enable sharding?') AS answer;

-- Multiple queries
SELECT 
    DOCS_QUERY('What is the default port?') AS port_info,
    DOCS_QUERY('How to configure TLS?') AS tls_info;

-- Use in WHERE clause
FOR doc IN :document
    FILTER doc.type == 'configuration'
    RETURN {
        title: doc.title,
        help: DOCS_QUERY(CONCAT('Explain ', doc.title))
    };
```

---

### DOCS_SEARCH(query: string, limit: int = 5) -> array<object>

Search the documentation database without LLM generation. Returns relevant documents ordered by relevance score.

**Syntax:**
```sql
SELECT DOCS_SEARCH(query_text, max_results) AS results;
```

**Parameters:**
- `query_text` (string): Search query
- `max_results` (int, optional): Maximum number of results (default: 5)

**Returns:**
- JSON array of document objects with:
  - `file_name`: Document filename
  - `file_path`: Full path to document
  - `relevance_score`: Relevance score (0.0 to 1.0)
  - `content_type`: MIME type
  - `content_preview`: First 200 characters of content
  - `metadata`: Additional metadata

**Examples:**
```sql
-- Basic search
SELECT DOCS_SEARCH('RAID configuration', 10) AS relevant_docs;

-- Search with filtering
LET docs = DOCS_SEARCH('vector embeddings', 5)
FOR doc IN docs
    FILTER doc.relevance_score > 0.7
    RETURN {
        title: doc.file_name,
        score: doc.relevance_score,
        preview: doc.content_preview
    };

-- Combine search with LLM
LET docs = DOCS_SEARCH('sharding', 3)
LET answer = DOCS_QUERY('Explain sharding configuration')
RETURN {
    generated_answer: answer,
    source_documents: docs
};
```

---

### DOCS_CONFIG_HELP(topic: string) -> string

Get configuration assistance for a specific topic. This function is optimized for configuration-related queries and returns structured guidance.

**Syntax:**
```sql
SELECT DOCS_CONFIG_HELP(topic) AS help;
```

**Parameters:**
- `topic` (string): Configuration topic (e.g., "sharding", "security", "replication")

**Returns:**
- String containing configuration guidance

**Examples:**
```sql
-- Get help on specific topics
SELECT DOCS_CONFIG_HELP('security') AS security_config;
SELECT DOCS_CONFIG_HELP('sharding') AS sharding_config;
SELECT DOCS_CONFIG_HELP('replication') AS replication_config;

-- Batch configuration help
FOR topic IN ['security', 'replication', 'caching']
    RETURN {
        topic: topic,
        configuration_guide: DOCS_CONFIG_HELP(topic)
    };

-- Dynamic configuration help
FOR setting IN :configuration
    FILTER setting.needs_documentation == true
    RETURN {
        setting_name: setting.name,
        help: DOCS_CONFIG_HELP(setting.category)
    };
```

---

### DOCS_TROUBLESHOOT(error: string) -> string

Get troubleshooting assistance for errors or issues. This function analyzes the error description and provides potential solutions.

**Syntax:**
```sql
SELECT DOCS_TROUBLESHOOT(error_description) AS solution;
```

**Parameters:**
- `error_description` (string): Description of the error or issue

**Returns:**
- String containing troubleshooting guidance and potential solutions

**Examples:**
```sql
-- Troubleshoot specific errors
SELECT DOCS_TROUBLESHOOT('Server hangs at startup') AS solution;
SELECT DOCS_TROUBLESHOOT('Connection refused on port 8529') AS fix;
SELECT DOCS_TROUBLESHOOT('Out of memory error during query') AS help;

-- Log and troubleshoot errors
FOR error IN :error_log
    FILTER error.severity == 'HIGH'
    LIMIT 10
    RETURN {
        error_message: error.message,
        timestamp: error.timestamp,
        solution: DOCS_TROUBLESHOOT(error.message)
    };

-- Interactive troubleshooting
LET error_msg = 'Failed to create vector index'
LET initial_help = DOCS_TROUBLESHOOT(error_msg)
LET related_docs = DOCS_SEARCH(error_msg, 3)
RETURN {
    error: error_msg,
    troubleshooting_steps: initial_help,
    related_documentation: related_docs
};
```

---

### DOCS_STATS() -> object

Get statistics about the documentation database including total documents, database size, and cache information.

**Syntax:**
```sql
SELECT DOCS_STATS() AS stats;
```

**Returns:**
- JSON object containing:
  - `total_documents`: Total number of documents in database
  - `database_version`: Version of the documentation database
  - `generation_time`: When the database was generated
  - `themisdb_version`: ThemisDB version the docs are for
  - `cache_stats`: Cache hit/miss statistics

**Example:**
```sql
SELECT DOCS_STATS() AS documentation_info;
```

---

## Advanced Usage Patterns

### 1. Contextual Help System

```sql
-- Create a help function based on user context
LET user_query = 'How do I backup my data?'
LET search_results = DOCS_SEARCH(user_query, 5)
LET llm_answer = DOCS_QUERY(user_query)

RETURN {
    query: user_query,
    answer: llm_answer,
    related_documents: search_results,
    follow_up_queries: [
        'How often should I backup?',
        'What is the backup format?',
        'Can I restore from backup?'
    ]
};
```

### 2. Configuration Validation with Help

```sql
-- Validate configuration and provide help for invalid settings
FOR config IN :configuration
    LET is_valid = config.value != null
    LET help = is_valid ? null : DOCS_CONFIG_HELP(config.name)
    RETURN {
        setting: config.name,
        value: config.value,
        valid: is_valid,
        help_if_invalid: help
    };
```

### 3. Error Analysis Pipeline

```sql
-- Analyze recent errors and group by category
LET recent_errors = (
    FOR error IN :error_log
        FILTER error.timestamp > DATE_NOW() - 3600000  // Last hour
        RETURN error
)

LET analyzed_errors = (
    FOR error IN recent_errors
        LET solution = DOCS_TROUBLESHOOT(error.message)
        RETURN {
            error: error,
            solution: solution,
            category: error.category
        }
)

LET grouped = (
    FOR item IN analyzed_errors
        COLLECT category = item.category INTO group
        RETURN {
            category: category,
            count: LENGTH(group),
            solutions: group[*].item.solution
        }
)

RETURN grouped;
```

### 4. Interactive Documentation Explorer

```sql
-- Search-driven documentation browsing
LET search_term = 'performance tuning'
LET matching_docs = DOCS_SEARCH(search_term, 10)

FOR doc IN matching_docs
    LET summary = DOCS_QUERY(
        CONCAT('Summarize in 2 sentences: ', doc.content_preview)
    )
    RETURN {
        document: doc.file_name,
        path: doc.file_path,
        relevance: doc.relevance_score,
        summary: summary,
        full_content_available: doc.file_path
    };
```

### 5. Batch Configuration Assistance

```sql
-- Get help for all configuration categories
LET categories = ['security', 'performance', 'networking', 'storage', 'clustering']

FOR category IN categories
    LET config_help = DOCS_CONFIG_HELP(category)
    LET related_docs = DOCS_SEARCH(category, 3)
    RETURN {
        category: category,
        configuration_guide: config_help,
        documentation_references: related_docs
    };
```

---

## Performance Considerations

### Caching

The documentation assistant uses intelligent caching to improve performance:

- **Response Cache**: Frequently asked questions are cached
- **Document Cache**: Recently accessed documents stay in memory
- **Search Cache**: Recent search queries are cached

### Query Optimization

```sql
-- GOOD: Single query with multiple operations
LET help = DOCS_QUERY('sharding configuration')
LET docs = DOCS_SEARCH('sharding', 5)
RETURN { help, docs };

-- AVOID: Multiple separate queries in a loop without need
FOR i IN 1..100
    RETURN DOCS_QUERY('same question')  // This will cache, but unnecessary
```

### Batch Processing

```sql
-- GOOD: Batch related queries
LET topics = ['security', 'networking', 'storage']
FOR topic IN topics
    RETURN DOCS_CONFIG_HELP(topic);

-- AVOID: Individual queries when batch is possible
RETURN {
    security: DOCS_CONFIG_HELP('security'),
    networking: DOCS_CONFIG_HELP('networking'),
    storage: DOCS_CONFIG_HELP('storage')
}; // Better to use FOR loop
```

---

## Error Handling

All documentation functions throw exceptions with descriptive messages if:
1. Documentation database is not loaded
2. Query fails
3. Database is corrupted

Example error handling:

```sql
-- Graceful fallback
LET help = (
    DOCS_QUERY('How to configure X?') OR 
    'Documentation not available. Please check docs.themisdb.com'
)
RETURN help;
```

---

## Configuration

### Environment Variables

- `THEMIS_DOCS_DATABASE_PATH`: Path to documentation database
- `THEMIS_DOCS_DATABASE_TYPE`: Database type ("json" or "rocksdb")
- `THEMIS_ENABLE_DOCS_ASSISTANT`: Enable/disable documentation assistant

### Auto-Discovery

If not explicitly configured, the system searches for documentation database in this order:

1. `data/docs.db` (RocksDB)
2. `data/docs_database.json` (JSON)
3. `./docs.db` (Current directory, RocksDB)
4. `./docs_database.json` (Current directory, JSON)
5. `../data/docs.db` (Parent directory, RocksDB)
6. `../data/docs_database.json` (Parent directory, JSON)

---

## Building Documentation Database

The documentation database is automatically generated during build when `THEMIS_ENABLE_LLM=ON`:

```bash
# Build with documentation database
cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build

# Documentation database will be at: build/data/docs.db
```

Manual generation:

```bash
# Generate JSON database
python3 scripts/generate_docs_database.py --output data/docs_database.json

# Generate RocksDB database
python3 scripts/generate_docs_rocksdb.py --output data/docs.db --method cpp
g++ -std=c++17 data/import_docs_rocksdb.cpp -o import_docs_rocksdb -lrocksdb
./import_docs_rocksdb data/docs_database.json data/docs.db
```

---

## Limitations

1. **Database Size**: Documentation database is ~2-3 MB
2. **LLM Requirement**: Requires LLM model loaded for DOCS_QUERY, DOCS_CONFIG_HELP, and DOCS_TROUBLESHOOT
3. **Cache Size**: Response cache limited to 1000 entries by default
4. **Language**: Currently supports English documentation only

---

## See Also

- [LLM Functions](./llm_functions.md)
- [Vector Search Functions](./vector_search.md)
- [Documentation Build Process](../development/documentation_build.md)
