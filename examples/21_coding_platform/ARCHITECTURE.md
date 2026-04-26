> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Coding Platform - Architecture & Design

Dieses Dokument beschreibt die System-Architektur, Design-Entscheidungen und technische Details der ThemisDB Coding Platform.

## 📋 Inhaltsverzeichnis

1. [System Overview](#system-overview)
2. [Component Architecture](#component-architecture)
3. [Data Flow](#data-flow)
4. [Database Schema](#database-schema)
5. [API Design](#api-design)
6. [Embedding Pipeline](#embedding-pipeline)
7. [Scalability](#scalability)
8. [Security](#security)

## 1. System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Client Layer                         │
│  ┌──────────────────┐  ┌──────────────┐  ┌───────────────┐ │
│  │  Tkinter Desktop │  │VSCode Ext    │  │  Web UI       │ │
│  │      App         │  │              │  │  (Future)     │ │
│  └────────┬─────────┘  └──────┬───────┘  └───────┬───────┘ │
└───────────┼────────────────────┼──────────────────┼─────────┘
            │                    │                  │
            └────────────────────┼──────────────────┘
                                 │ HTTP REST API
                                 │
┌────────────────────────────────▼─────────────────────────────┐
│                    Application Layer                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │             API Server (Flask/FastAPI)               │   │
│  │  - Authentication & Authorization                    │   │
│  │  - Request Validation                                │   │
│  │  - Rate Limiting                                     │   │
│  └──────────┬───────────────────────────────────────────┘   │
│             │                                                │
│  ┌──────────▼───────────┐  ┌─────────────────────────┐     │
│  │  Business Logic      │  │   Background Workers    │     │
│  │  - Snippet Manager   │  │  - Scraping Jobs        │     │
│  │  - Project Manager   │  │  - Embedding Generation │     │
│  │  - Search Engine     │  │  - Index Maintenance    │     │
│  └──────────┬───────────┘  └──────────┬──────────────┘     │
└─────────────┼───────────────────────────┼──────────────────┘
              │                           │
              └───────────┬───────────────┘
                          │
┌─────────────────────────▼──────────────────────────────────┐
│                    Data Layer                              │
│  ┌──────────────────────────────────────────────────────┐ │
│  │                  ThemisDB                            │ │
│  │  ┌──────────────┐ ┌──────────────┐ ┌─────────────┐ │ │
│  │  │ Vector Model │ │Document Model│ │ Graph Model │ │ │
│  │  │ - Embeddings │ │ - Snippets   │ │ - Relations │ │ │
│  │  │ - Similarity │ │ - Projects   │ │ - Deps      │ │ │
│  │  └──────────────┘ └──────────────┘ └─────────────┘ │ │
│  └──────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────┘
```

### Technology Stack

**Frontend**:
- Tkinter (Desktop GUI)
- TypeScript + VSCode Extension API

**Backend**:
- Python 3.8+
- Flask/FastAPI (API Server)
- Celery (Background Tasks)

**Database**:
- ThemisDB (Multi-Model)
  - Vector Model (Code Embeddings)
  - Document Model (Snippets, Docs)
  - Graph Model (Dependencies, Relations)

**ML/AI**:
- sentence-transformers (Embeddings)
- microsoft/codebert-base (Code Embeddings)
- tree-sitter (Code Parsing)

**Web Scraping**:
- requests (HTTP)
- beautifulsoup4 (HTML Parsing)
- PyGithub (GitHub API)
- selenium (JS-rendered pages)

## 2. Component Architecture

### Desktop Application

```python
main.py
├── UI Layer (ui/)
│   ├── MainWindow
│   │   ├── MenuBar
│   │   ├── Toolbar
│   │   └── TabWidget
│   ├── SnippetPanel
│   │   ├── SnippetList
│   │   ├── SnippetEditor
│   │   └── SnippetDetails
│   ├── ProjectPanel
│   │   ├── ProjectTree
│   │   ├── FileEditor
│   │   └── ProjectSettings
│   ├── SearchPanel
│   │   ├── SearchBar
│   │   ├── FilterPanel
│   │   └── ResultsList
│   └── ScraperPanel
│       ├── JobList
│       ├── JobConfig
│       └── JobProgress
│
├── Business Logic
│   ├── SnippetManager
│   ├── ProjectManager
│   ├── SearchEngine
│   └── ScraperManager
│
└── Data Access (themis_client.py)
    └── ThemisDBClient
```

### VSCode Extension

```typescript
extension.ts
├── Activation
│   ├── Configuration
│   ├── API Client
│   └── Providers Registration
│
├── Providers
│   ├── SnippetTreeProvider
│   │   └── TreeView Data
│   ├── CodeLensProvider
│   │   └── Similar Code Hints
│   ├── CompletionProvider
│   │   └── Auto-Suggestions
│   └── HoverProvider
│       └── Snippet Preview
│
├── Commands
│   ├── searchSnippets()
│   ├── insertSnippet()
│   ├── saveSnippet()
│   └── findSimilar()
│
└── API Client
    └── REST API Communication
```

### Web Scraper

```python
web_scraper.py
├── BaseScraper (Abstract)
│   ├── fetch_with_retry()
│   ├── parse_content()
│   ├── extract_code()
│   └── create_snippet()
│
├── GitHubScraper
│   ├── scrape_repository()
│   ├── scrape_gists()
│   └── search_and_scrape()
│
├── StackOverflowScraper
│   ├── scrape_questions()
│   ├── extract_code_from_answers()
│   └── scrape_user_answers()
│
└── DocsCrawler
    ├── crawl_documentation()
    ├── extract_code_examples()
    └── build_doc_graph()
```

### Code Indexer

```python
code_indexer.py
├── EmbeddingGenerator
│   ├── generate_embedding()
│   ├── batch_generate()
│   └── update_model()
│
├── SimilarityEngine
│   ├── find_similar()
│   ├── compute_similarity()
│   └── rank_results()
│
└── Deduplicator
    ├── detect_duplicates()
    ├── merge_similar()
    └── hash_code()
```

## 3. Data Flow

### Snippet Creation Flow

```
User Input
    │
    ▼
┌────────────────┐
│  UI Component  │
└────────┬───────┘
         │ validate()
         ▼
┌────────────────┐
│ SnippetManager │
└────────┬───────┘
         │ create_snippet()
         ▼
┌────────────────┐
│ CodeIndexer    │ ──→ generate_embedding()
└────────┬───────┘
         │ snippet + embedding
         ▼
┌────────────────┐
│ ThemisDB Client│ ──→ POST /api/snippets
└────────┬───────┘
         │
         ▼
┌────────────────┐
│  ThemisDB      │
│  - Document    │ ──→ Store metadata, code
│  - Vector      │ ──→ Store embedding, build index
└────────┬───────┘
         │
         ▼
    Success
```

### Semantic Search Flow

```
User Query
    │
    ▼
┌────────────────┐
│  Search Panel  │
└────────┬───────┘
         │ query string
         ▼
┌────────────────┐
│ CodeIndexer    │ ──→ generate_query_embedding()
└────────┬───────┘
         │ query embedding
         ▼
┌────────────────┐
│ ThemisDB Client│ ──→ POST /api/search/semantic
└────────┬───────┘
         │
         ▼
┌────────────────┐
│  ThemisDB      │
│  Vector Search │ ──→ HNSW / FAISS similarity
└────────┬───────┘
         │ ranked results
         ▼
┌────────────────┐
│  Search Panel  │ ──→ Display results
└────────────────┘
```

### Web Scraping Flow

```
Scraping Job Config
    │
    ▼
┌────────────────┐
│ Scraper Panel  │ ──→ create_job()
└────────┬───────┘
         │
         ▼
┌────────────────┐
│ ScraperManager │ ──→ dispatch_to_worker()
└────────┬───────┘
         │
         ▼
┌────────────────┐
│ Background     │
│ Worker (Celery)│
└────────┬───────┘
         │
         ├──→ fetch_content()
         ├──→ parse_code()
         ├──→ detect_language()
         ├──→ generate_embedding()
         ├──→ check_duplicate()
         └──→ store_snippet()
              │
              ▼
         ┌────────────────┐
         │  ThemisDB      │
         └────────┬───────┘
                  │
                  ▼
         Update Job Status
              │
              ▼
         Notify UI
```

## 4. Database Schema

### ThemisDB Collections

#### Snippets (Document Model)

```json
{
  "collection": "snippets",
  "schema": {
    "id": "uuid",
    "title": "string",
    "description": "string",
    "code": "text",
    "language": "string",
    "framework": "string (optional)",
    "tags": ["string"],
    "metadata": {
      "author": "string",
      "source_url": "string",
      "source_type": "enum(github, stackoverflow, custom)",
      "license": "string",
      "stars": "integer",
      "created_at": "timestamp",
      "updated_at": "timestamp"
    },
    "stats": {
      "views": "integer",
      "copies": "integer",
      "likes": "integer"
    }
  },
  "indexes": [
    {"field": "language"},
    {"field": "framework"},
    {"field": "tags"},
    {"field": "metadata.created_at"}
  ]
}
```

#### Embeddings (Vector Model)

```json
{
  "collection": "embeddings",
  "schema": {
    "snippet_id": "uuid (foreign key)",
    "vector": "float[] (512 dimensions)",
    "model": "string (e.g., codebert-base)",
    "created_at": "timestamp"
  },
  "vector_index": {
    "type": "HNSW",
    "dimensions": 512,
    "distance_metric": "cosine"
  }
}
```

#### Projects (Document Model)

```json
{
  "collection": "projects",
  "schema": {
    "id": "uuid",
    "name": "string",
    "description": "string",
    "language": "string",
    "framework": "string",
    "files": [{
      "path": "string",
      "content": "text",
      "language": "string",
      "size": "integer"
    }],
    "structure": {
      "type": "tree",
      "root": "TreeNode"
    },
    "dependencies": ["string"],
    "readme": "text",
    "tags": ["string"],
    "metadata": {
      "source_url": "string",
      "source_type": "string",
      "stars": "integer",
      "forks": "integer"
    }
  }
}
```

#### Relations (Graph Model)

```json
{
  "collection": "relations",
  "edges": [
    {
      "from": "snippet_id",
      "to": "snippet_id",
      "type": "similar_to",
      "weight": "float (similarity score)"
    },
    {
      "from": "snippet_id",
      "to": "project_id",
      "type": "part_of"
    },
    {
      "from": "project_id",
      "to": "project_id",
      "type": "depends_on"
    }
  ]
}
```

## 5. API Design

### RESTful API

**Base URL**: `http://localhost:8080/api/v1`

#### Snippets

```http
# Search snippets (semantic)
POST /snippets/search
Content-Type: application/json
{
  "query": "async HTTP request",
  "language": "python",
  "limit": 10,
  "filters": {
    "framework": "aiohttp",
    "min_stars": 10
  }
}

# Get snippet
GET /snippets/:id

# Create snippet
POST /snippets
{
  "title": "...",
  "code": "...",
  "language": "python",
  "tags": ["..."]
}

# Update snippet
PUT /snippets/:id

# Delete snippet
DELETE /snippets/:id

# Find similar
POST /snippets/similar
{
  "code": "...",
  "language": "python",
  "limit": 5
}
```

#### Projects

```http
# List projects
GET /projects?language=python&framework=fastapi

# Get project
GET /projects/:id

# Create project
POST /projects

# Import from GitHub
POST /projects/import
{
  "source": "github",
  "url": "https://github.com/user/repo"
}
```

#### Scraping Jobs

```http
# Create job
POST /scraping/jobs
{
  "type": "github_repo",
  "url": "...",
  "config": {...}
}

# Get job status
GET /scraping/jobs/:id

# List jobs
GET /scraping/jobs?status=completed

# Cancel job
DELETE /scraping/jobs/:id
```

### Response Format

**Success**:
```json
{
  "status": "success",
  "data": {...},
  "meta": {
    "total": 100,
    "page": 1,
    "per_page": 10
  }
}
```

**Error**:
```json
{
  "status": "error",
  "error": {
    "code": "INVALID_INPUT",
    "message": "Language must be specified",
    "details": {...}
  }
}
```

## 6. Embedding Pipeline

### Model Selection

```python
# Primary Model: CodeBERT
MODEL = "microsoft/codebert-base"
DIMENSIONS = 512

# Alternative Models:
# - "microsoft/graphcodebert-base" (better for code structure)
# - "sentence-transformers/all-MiniLM-L6-v2" (faster, smaller)
```

### Embedding Generation

```python
from sentence_transformers import SentenceTransformer

class EmbeddingGenerator:
    def __init__(self):
        self.model = SentenceTransformer('microsoft/codebert-base')
    
    def generate(self, code: str, language: str) -> np.ndarray:
        # Preprocess code
        code = self.normalize_code(code, language)
        
        # Generate embedding
        embedding = self.model.encode(code)
        
        # Normalize (unit vector)
        embedding = embedding / np.linalg.norm(embedding)
        
        return embedding
    
    def normalize_code(self, code: str, language: str) -> str:
        # Remove comments
        code = self.remove_comments(code, language)
        
        # Normalize whitespace
        code = ' '.join(code.split())
        
        # Optional: Extract function signatures, variable names
        # for better semantic understanding
        
        return code
```

### Similarity Search

```python
def find_similar(query_embedding: np.ndarray, limit: int = 10):
    # ThemisDB Vector Search
    results = client.vector_search(
        collection="embeddings",
        query_vector=query_embedding,
        limit=limit,
        distance_metric="cosine"
    )
    
    # Results ranked by similarity
    return results
```

## 7. Scalability

### Horizontal Scaling

**API Server**:
```
Load Balancer (nginx)
    │
    ├──→ API Server 1
    ├──→ API Server 2
    └──→ API Server 3
```

**Background Workers**:
```
Celery Workers (auto-scale)
    ├──→ Worker 1 (scraping)
    ├──→ Worker 2 (scraping)
    ├──→ Worker 3 (embedding)
    └──→ Worker 4 (embedding)
```

**Database Sharding** (ThemisDB):
```
Snippets by Language:
    Shard 1: Python, Ruby
    Shard 2: JavaScript, TypeScript
    Shard 3: Java, Kotlin, Scala
    Shard 4: C++, Rust, Go
```

### Caching Strategy

**Multi-Layer Cache**:
```
Request
    │
    ▼
┌─────────────────┐
│  Redis Cache    │ ◄─── Most frequent queries (TTL: 5 min)
└─────────┬───────┘
          │ (miss)
          ▼
┌─────────────────┐
│  Local Cache    │ ◄─── Recent results (LRU, max 1000)
└─────────┬───────┘
          │ (miss)
          ▼
┌─────────────────┐
│  ThemisDB       │ ◄─── Source of truth
└─────────────────┘
```

### Performance Optimization

**Embeddings**:
- Batch generation (32-64 snippets at once)
- GPU acceleration when available
- Model quantization for faster inference

**Vector Search**:
- HNSW index for O(log N) search
- Pre-filtering by language/framework
- Parallel search across shards

## 8. Security

### Authentication & Authorization

```python
# JWT-based authentication
Authorization: Bearer <jwt_token>

# Token payload
{
  "user_id": "uuid",
  "email": "user@example.com",
  "roles": ["user", "admin"],
  "exp": 1640000000
}
```

### Access Control

```python
# Snippet visibility
class Snippet:
    visibility: enum("public", "private", "team")
    owner_id: uuid
    team_id: uuid (optional)

# Access rules
def can_access(user, snippet):
    if snippet.visibility == "public":
        return True
    elif snippet.visibility == "private":
        return user.id == snippet.owner_id
    elif snippet.visibility == "team":
        return user.team_id == snippet.team_id
```

### Input Validation

```python
from pydantic import BaseModel, validator

class SnippetCreate(BaseModel):
    title: str
    code: str
    language: str
    tags: List[str]
    
    @validator('code')
    def code_not_empty(cls, v):
        if not v.strip():
            raise ValueError('Code cannot be empty')
        return v
    
    @validator('language')
    def valid_language(cls, v):
        allowed = ['python', 'javascript', 'java', ...]
        if v not in allowed:
            raise ValueError(f'Language must be one of {allowed}')
        return v
```

### Rate Limiting

```python
from flask_limiter import Limiter

limiter = Limiter(
    app,
    key_func=lambda: request.headers.get('Authorization'),
    default_limits=["100 per hour"]
)

@app.route('/api/snippets/search', methods=['POST'])
@limiter.limit("20 per minute")
def search_snippets():
    pass
```

### Security Best Practices

- **XSS Prevention**: Sanitize all user input
- **SQL Injection**: Use parameterized queries
- **CSRF**: Use CSRF tokens for state-changing operations
- **Secrets Management**: Use environment variables, never hardcode
- **HTTPS**: Enforce TLS for all API communication
- **Logging**: Log all access attempts, audit trail

---

## 📚 Design Decisions

### Why Multi-Model Database (ThemisDB)?

1. **Vector Model**: Essential for semantic code search
2. **Document Model**: Flexible schema for diverse code structures
3. **Graph Model**: Natural fit for dependencies and relationships

### Why CodeBERT for Embeddings?

- Pre-trained on code (6 programming languages)
- Better understanding of code semantics vs general text models
- Good balance between quality and speed

### Why Tkinter for Desktop App?

- Standard library (no external dependencies)
- Cross-platform (Windows, macOS, Linux)
- Simple and lightweight
- Sufficient for proof-of-concept

### Future Improvements

- **Web Interface**: React/Vue.js for broader accessibility
- **Real-time Collaboration**: WebSocket for live code sharing
- **AI Code Generation**: LLM integration for code completion
- **Plugin System**: Extensible architecture for custom scrapers

---

**Questions?** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
