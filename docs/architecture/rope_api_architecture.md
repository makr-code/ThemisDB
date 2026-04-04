# RoPE REST API Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Client Layer                             │
│  (curl, Python, JavaScript, any HTTP client)                    │
└─────────────────────────┬───────────────────────────────────────┘
                          │ HTTP/JSON
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    HTTP Server Layer                             │
│  src/server/http_server.cpp                                      │
│                                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ Request Router (classifyRoute)                             │  │
│  │                                                             │  │
│  │  /api/v1/vector-index/{index}/rope/config     → Route enum│  │
│  │  /api/v1/vector-index/{index}/rope/add        → Route enum│  │
│  │  /api/v1/vector-index/{index}/rope/search     → Route enum│  │
│  │  ... (8 routes total)                                      │  │
│  └───────────────────────┬───────────────────────────────────┘  │
│                          │                                        │
│  ┌───────────────────────▼───────────────────────────────────┐  │
│  │ Route Dispatcher (routeRequest switch)                     │  │
│  │                                                             │  │
│  │  case RopeConfigPost:  → rope_api_->handleConfigPost()    │  │
│  │  case RopeAddPost:     → rope_api_->handleAddPost()       │  │
│  │  case RopeSearchPost:  → rope_api_->handleSearchPost()    │  │
│  │  ... (8 cases total)                                       │  │
│  └───────────────────────┬───────────────────────────────────┘  │
└──────────────────────────┼───────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    RoPE API Handler                              │
│  src/server/rope_api_handler.cpp                                 │
│  include/server/rope_api_handler.h                               │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Configuration Management                                   │   │
│  │  • handleConfigPost()   - Enable RoPE                     │   │
│  │  • handleConfigGet()    - Get config                      │   │
│  │  • handleConfigDelete() - Disable RoPE                    │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Entity Operations                                          │   │
│  │  • handleAddPost()           - Add with position          │   │
│  │  • handleAddRelationalPost() - Add with relation          │   │
│  │  • handleBatchAddPost()      - Batch add                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Search & Monitoring                                        │   │
│  │  • handleSearchPost() - Rotation-aware search             │   │
│  │  • handleStatsGet()   - Get statistics                    │   │
│  └──────────────────────────┬───────────────────────────────┘   │
└─────────────────────────────┼───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                 Vector Index Manager Layer                       │
│  include/index/vector_index.h                                    │
│  src/index/vector_index.cpp                                      │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ RoPE Integration Methods                                   │   │
│  │  • setRotaryEmbeddingConfig()    - Configure              │   │
│  │  • isRotaryEmbeddingEnabled()    - Check status           │   │
│  │  • getRotaryEmbeddingConfig()    - Get config             │   │
│  │  • addEntityWithRotation()       - Add with position      │   │
│  │  • addEntityWithRelationalRotation() - Add with relation  │   │
│  │  • searchWithRotation()          - Search with rotation   │   │
│  └──────────────────────────┬───────────────────────────────┘   │
└─────────────────────────────┼───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Rotary Embeddings Layer                       │
│  include/index/rotary_embeddings.h                               │
│  src/index/rotary_embeddings.cpp                                 │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ RotaryEmbedding Class                                      │   │
│  │  • rotate()            - Apply positional rotation        │   │
│  │  • rotateRelational()  - Apply relational rotation        │   │
│  │  • rotateBatch()       - Batch rotation                   │   │
│  │  • computeThetaCache() - Precompute rotation angles       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ RotationConfig Struct                                      │   │
│  │  • hidden_dim          - Embedding dimension              │   │
│  │  • num_rotation_pairs  - Number of rotation pairs         │   │
│  │  • base_theta          - Base frequency                   │   │
│  │  • normalize_after     - L2 normalization flag            │   │
│  │  • theta_cache         - Precomputed angles               │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

Data Flow Example: Configure RoPE
══════════════════════════════════

Client:
  POST /api/v1/vector-index/documents/rope/config
  Body: {"hidden_dim": 768, "num_rotation_pairs": 384}
       │
       ▼
HttpServer::classifyRoute()
  Matches: /rope/config + POST → Route::RopeConfigPost
       │
       ▼
HttpServer::routeRequest()
  case RopeConfigPost: rope_api_->handleConfigPost(req)
       │
       ▼
RopeApiHandler::handleConfigPost()
  1. Parse JSON body
  2. Validate configuration
  3. Create RotationConfig object
  4. Call vector_index_->setRotaryEmbeddingConfig(config)
       │
       ▼
VectorIndexManager::setRotaryEmbeddingConfig()
  1. Validate config
  2. Create RotaryEmbedding instance
  3. Store in rotary_embedding_ member
  4. Set rotary_enabled_ = true
       │
       ▼
RotaryEmbedding::RotaryEmbedding(config)
  1. Store configuration
  2. Call computeThetaCache()
  3. Precompute rotation angles
       │
       ▼
Response:
  {"status": "success", "config": {...}}


Data Flow Example: Search with Rotation
════════════════════════════════════════

Client:
  POST /api/v1/vector-index/documents/rope/search
  Body: {"query": [...], "k": 10, "position": 100}
       │
       ▼
HttpServer → RopeApiHandler::handleSearchPost()
  1. Parse query vector and position
  2. Start timing measurement
  3. Call vector_index_->searchWithRotation(query, k, position)
       │
       ▼
VectorIndexManager::searchWithRotation()
  1. Check if RoPE enabled
  2. Call rotary_embedding_->rotate(query, position)
  3. Perform standard k-NN search with rotated query
  4. Return results
       │
       ▼
RotaryEmbedding::rotate()
  1. For each rotation pair:
     - Compute rotation angle: θ = position × theta_cache[i]
     - Apply 2D rotation: [cos(θ), -sin(θ); sin(θ), cos(θ)]
  2. Return rotated vector
       │
       ▼
Response:
  {"status": "success", "results": [...], "query_time_ms": 5.2}

Authentication Flow
═══════════════════

Request with auth header
       │
       ▼
HttpServer
  Extracts auth context
       │
       ▼
RopeApiHandler::handleXXX()
  Calls requireAccess(req, permission, resource, path)
       │
       ▼
RopeApiHandler::requireAccess()
  1. Check if auth_ is enabled
  2. If disabled → allow access (return nullopt)
  3. If enabled → perform auth check
  4. Return error response if unauthorized
       │
       ▼
Continue processing or return 403 Forbidden

Key Design Patterns
═══════════════════

1. Handler Delegation Pattern
   HttpServer → Specialized API Handlers → Core Services
   
2. Early Return Pattern
   Route matching checks most specific patterns first
   
3. Error Handling Pattern
   Consistent JSON error responses with status codes
   
4. Validation Pattern
   Input validation at API layer before calling core services
   
5. Timing Pattern
   Measure operation duration for performance monitoring
```
