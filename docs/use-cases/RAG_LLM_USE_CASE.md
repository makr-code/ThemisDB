# RAG & LLM Applications with ThemisDB

## Overview

This comprehensive guide demonstrates building production-grade Retrieval-Augmented Generation (RAG) systems using ThemisDB's native vector search, integrated llama.cpp engine, and multi-modal capabilities. We'll cover document processing, semantic search, context retrieval optimization, and LLM integration patterns.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Schema Design](#schema-design)
3. [Document Processing Pipeline](#document-processing-pipeline)
4. [Vector Embeddings](#vector-embeddings)
5. [Semantic Search](#semantic-search)
6. [RAG Implementation](#rag-implementation)
7. [LLM Integration](#llm-integration)
8. [Context Optimization](#context-optimization)
9. [Performance Tuning](#performance-tuning)
10. [Production Deployment](#production-deployment)

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Document Sources                         │
├──────────┬──────────┬──────────┬──────────┬─────────────────┤
│ PDFs     │ Markdown │ Websites │ APIs     │ Databases       │
└────┬─────┴────┬─────┴────┬─────┴────┬─────┴────┬────────────┘
     │          │          │          │          │
     └──────────┴──────────┴──────────┴──────────┘
                         │
         ┌───────────────▼────────────────┐
         │   Document Processing Pipeline │
         │  - Format Conversion           │
         │  - Text Extraction             │
         │  - Chunking Strategy           │
         │  - Metadata Extraction         │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │     Embedding Generation       │
         │  - Native llama.cpp            │
         │  - Batch Processing            │
         │  - Model Selection             │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │       ThemisDB Storage         │
         │  ┌──────────────────────────┐  │
         │  │ Document Store (Doc)     │  │
         │  │ - Original content       │  │
         │  │ - Chunks & metadata      │  │
         │  ├──────────────────────────┤  │
         │  │ Vector Index (HNSW)      │  │
         │  │ - Embeddings             │  │
         │  │ - Semantic search        │  │
         │  ├──────────────────────────┤  │
         │  │ Knowledge Graph          │  │
         │  │ - Entity relationships   │  │
         │  │ - Citation networks      │  │
         │  ├──────────────────────────┤  │
         │  │ Query Cache              │  │
         │  │ - LLM responses          │  │
         │  │ - Retrieved contexts     │  │
         │  └──────────────────────────┘  │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │       RAG Query Pipeline       │
         │  1. Query Embedding            │
         │  2. Vector Search (HNSW)       │
         │  3. Hybrid Search (optional)   │
         │  4. Context Assembly           │
         │  5. Prompt Construction        │
         │  6. LLM Generation             │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │     Native LLM Engine          │
         │  - llama.cpp integration       │
         │  - LoRA adapters               │
         │  - Multi-GPU support           │
         │  - Streaming responses         │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │         Application            │
         │  - Q&A Systems                 │
         │  - Chatbots                    │
         │  - Document Analysis           │
         │  - Code Assistants             │
         └────────────────────────────────┘
```

## Schema Design

### Document Collection

```aql
// Store processed documents with metadata
CREATE COLLECTION documents {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "document_id",
        shards: 8
    },
    indexes: {
        unique: ["document_id"],
        fulltext: ["title", "content"],
        composite: [
            ["source_type", "created_at"],
            ["status", "updated_at"]
        ]
    }
}

// Document schema
{
    "document_id": "DOC-2024-001",
    "title": "ThemisDB Architecture Guide",
    "content": "Full text content of the document...",
    "content_hash": "sha256:abc123...",
    "source": {
        "type": "pdf",  // pdf, markdown, html, api, etc.
        "url": "https://example.com/docs/architecture.pdf",
        "original_path": "/uploads/architecture.pdf",
        "file_size": 2458624,
        "page_count": 45
    },
    "metadata": {
        "author": "ThemisDB Team",
        "created_at": "2024-01-15T10:00:00Z",
        "updated_at": "2024-01-20T15:30:00Z",
        "language": "en",
        "version": "1.0",
        "tags": ["architecture", "technical", "database"],
        "category": "documentation"
    },
    "processing": {
        "status": "completed",  // pending, processing, completed, failed
        "chunks_created": 42,
        "embeddings_generated": 42,
        "processed_at": "2024-01-20T15:35:00Z",
        "processing_time_ms": 3450
    },
    "statistics": {
        "word_count": 8542,
        "char_count": 54238,
        "estimated_tokens": 10677
    },
    "access": {
        "visibility": "public",  // public, private, restricted
        "permissions": ["read", "search"],
        "accessed_count": 142,
        "last_accessed": "2024-01-21T09:15:00Z"
    }
}
```

### Chunks Collection

```aql
// Store document chunks with embeddings
CREATE COLLECTION chunks {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "chunk_id",
        shards: 16
    },
    indexes: {
        unique: ["chunk_id"],
        composite: [
            ["document_id", "chunk_index"],
            ["document_id", "section"]
        ],
        vector: {
            field: "embedding",
            dimensions: 768,  // or 384, 1024, 1536 depending on model
            metric: "cosine",
            index_type: "hnsw",
            m: 16,              // HNSW parameter
            ef_construction: 200,
            ef_search: 100
        },
        fulltext: ["content"]
    }
}

// Chunk schema
{
    "chunk_id": "CHUNK-DOC-2024-001-005",
    "document_id": "DOC-2024-001",
    "chunk_index": 5,
    "section": "3.2 Vector Storage",
    "content": "ThemisDB implements HNSW (Hierarchical Navigable Small World) indexes for efficient vector similarity search...",
    "content_preview": "ThemisDB implements HNSW...",  // First 200 chars
    "embedding": [0.023, -0.145, 0.678, ...],  // 768-dimensional vector
    "metadata": {
        "start_char": 2450,
        "end_char": 3120,
        "word_count": 112,
        "token_count": 145,
        "page_number": 8,
        "heading_level": 2,
        "heading_path": "Architecture > Storage > Vector Storage"
    },
    "context": {
        "previous_chunk_id": "CHUNK-DOC-2024-001-004",
        "next_chunk_id": "CHUNK-DOC-2024-001-006",
        "parent_section": "3. Architecture",
        "related_chunks": ["CHUNK-DOC-2024-001-012", "CHUNK-DOC-2024-001-023"]
    },
    "created_at": "2024-01-20T15:35:12Z"
}
```

### Query Cache Collection

```aql
// Cache LLM responses for efficiency
CREATE COLLECTION query_cache {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "query_hash",
        shards: 8
    },
    indexes: {
        unique: ["query_hash"],
        ttl: {
            field: "expires_at",
            expire_after_seconds: 0
        }
    }
}

// Cache entry schema
{
    "query_hash": "sha256:def456...",
    "query": "How does ThemisDB implement vector search?",
    "query_embedding": [0.123, -0.456, ...],
    "retrieved_chunks": [
        {
            "chunk_id": "CHUNK-DOC-2024-001-005",
            "relevance_score": 0.92,
            "content": "..."
        }
    ],
    "response": "ThemisDB implements vector search using HNSW indexes...",
    "model": "llama-3-8b",
    "parameters": {
        "temperature": 0.7,
        "max_tokens": 500
    },
    "metadata": {
        "retrieval_time_ms": 45,
        "generation_time_ms": 1250,
        "total_time_ms": 1295,
        "cache_hits": 5
    },
    "created_at": "2024-01-21T10:00:00Z",
    "expires_at": "2024-01-22T10:00:00Z"
}
```

### Knowledge Graph

```aql
// Create graph for entity relationships and citations
CREATE GRAPH knowledge_graph {
    vertices: ["documents", "entities", "concepts"],
    edges: ["cites", "mentions", "related_to", "derived_from"]
}

// Entity vertex
{
    "_id": "entities/hnsw-algorithm",
    "entity_id": "hnsw-algorithm",
    "name": "HNSW Algorithm",
    "type": "algorithm",
    "description": "Hierarchical Navigable Small World graph algorithm for approximate nearest neighbor search",
    "aliases": ["Hierarchical NSW", "HNSW"],
    "metadata": {
        "first_mentioned": "DOC-2024-001",
        "mention_count": 15,
        "importance_score": 0.89
    }
}

// Citation edge
{
    "_from": "documents/DOC-2024-001",
    "_to": "documents/DOC-2024-015",
    "edge_type": "cites",
    "context": "For more details on query optimization, see...",
    "citation_count": 3,
    "created_at": "2024-01-20T15:35:00Z"
}
```

## Document Processing Pipeline

### Text Extraction and Chunking

```aql
// Process uploaded document
BEGIN TRANSACTION

// Insert document record
LET doc = INSERT {
    document_id: @documentId,
    title: @title,
    content: @content,
    source: @source,
    metadata: @metadata,
    processing: {
        status: "processing",
        started_at: DATE_ISO8601(DATE_NOW())
    }
} INTO documents RETURN NEW

// Chunking strategy: Semantic chunking with overlap
LET chunks = @processedChunks  // Processed by chunking service

// Insert chunks
FOR chunk IN chunks
    INSERT {
        chunk_id: chunk.id,
        document_id: doc.document_id,
        chunk_index: chunk.index,
        section: chunk.section,
        content: chunk.content,
        metadata: chunk.metadata,
        context: chunk.context,
        created_at: DATE_ISO8601(DATE_NOW())
    } INTO chunks

// Update document with processing status
UPDATE doc WITH {
    processing: {
        status: "embedding",
        chunks_created: LENGTH(chunks),
        chunked_at: DATE_ISO8601(DATE_NOW())
    }
} IN documents

COMMIT TRANSACTION
```

### Chunking Strategies (Application Code)

```cpp
// C++ chunking implementation
#include <themis/nlp/chunking.hpp>

class DocumentChunker {
public:
    enum class Strategy {
        Fixed,          // Fixed token/character count
        Semantic,       // Semantic boundaries (paragraphs, sections)
        Sliding,        // Sliding window with overlap
        Hierarchical    // Multi-level chunks (document -> section -> paragraph)
    };
    
    struct ChunkConfig {
        Strategy strategy = Strategy::Semantic;
        size_t target_size = 512;      // Target tokens per chunk
        size_t max_size = 1024;        // Maximum tokens per chunk
        size_t min_size = 100;         // Minimum tokens per chunk
        size_t overlap = 50;           // Overlap tokens between chunks
        bool preserve_sentences = true;
        bool preserve_paragraphs = true;
    };
    
    std::vector<Chunk> chunk_document(
        const std::string& content,
        const ChunkConfig& config
    ) {
        std::vector<Chunk> chunks;
        
        switch (config.strategy) {
            case Strategy::Semantic:
                chunks = semantic_chunking(content, config);
                break;
            case Strategy::Fixed:
                chunks = fixed_chunking(content, config);
                break;
            case Strategy::Sliding:
                chunks = sliding_chunking(content, config);
                break;
            case Strategy::Hierarchical:
                chunks = hierarchical_chunking(content, config);
                break;
        }
        
        return chunks;
    }
    
private:
    std::vector<Chunk> semantic_chunking(
        const std::string& content,
        const ChunkConfig& config
    ) {
        std::vector<Chunk> chunks;
        
        // Parse document structure
        auto sections = parse_sections(content);
        
        size_t chunk_index = 0;
        for (const auto& section : sections) {
            auto paragraphs = split_paragraphs(section.content);
            
            std::string current_chunk;
            size_t current_tokens = 0;
            
            for (const auto& para : paragraphs) {
                auto para_tokens = count_tokens(para);
                
                if (current_tokens + para_tokens > config.max_size && 
                    current_tokens >= config.min_size) {
                    // Save current chunk
                    chunks.push_back({
                        .index = chunk_index++,
                        .content = current_chunk,
                        .section = section.heading,
                        .token_count = current_tokens
                    });
                    
                    // Start new chunk with overlap
                    if (config.overlap > 0) {
                        current_chunk = get_last_tokens(current_chunk, config.overlap) + "\n\n" + para;
                        current_tokens = config.overlap + para_tokens;
                    } else {
                        current_chunk = para;
                        current_tokens = para_tokens;
                    }
                } else {
                    current_chunk += (current_chunk.empty() ? "" : "\n\n") + para;
                    current_tokens += para_tokens;
                }
            }
            
            // Add remaining content
            if (!current_chunk.empty()) {
                chunks.push_back({
                    .index = chunk_index++,
                    .content = current_chunk,
                    .section = section.heading,
                    .token_count = current_tokens
                });
            }
        }
        
        return chunks;
    }
};
```

## Vector Embeddings

### Generate Embeddings using Native LLM

```aql
// Generate embeddings for chunks using ThemisDB's native llama.cpp
FOR chunk IN chunks
    FILTER chunk.embedding == null
    
    // Generate embedding using native LLM function
    LET embedding = LLM_EMBED(chunk.content, {
        model: "all-MiniLM-L6-v2",  // 384-dim
        // model: "bge-large-en-v1.5",  // 1024-dim
        // model: "text-embedding-ada-002",  // 1536-dim (OpenAI)
        pooling: "mean",
        normalize: true,
        batch_size: 32
    })
    
    UPDATE chunk WITH {
        embedding: embedding,
        embedding_model: "all-MiniLM-L6-v2",
        embedding_generated_at: DATE_ISO8601(DATE_NOW())
    } IN chunks
    
RETURN {processed: COUNT()}
```

### Batch Embedding Generation (C++)

```cpp
// High-performance batch embedding generation
#include <themis/llm/embeddings.hpp>

class EmbeddingGenerator {
private:
    themis::llm::EmbeddingModel model;
    const size_t batch_size = 32;
    
public:
    EmbeddingGenerator(const std::string& model_path) {
        // Load embedding model (e.g., sentence-transformers)
        model = themis::llm::EmbeddingModel::load(
            model_path,
            {
                .use_gpu = true,
                .gpu_layers = 32,
                .batch_size = batch_size,
                .normalize = true
            }
        );
    }
    
    std::vector<std::vector<float>> generate_embeddings(
        const std::vector<std::string>& texts
    ) {
        std::vector<std::vector<float>> embeddings;
        embeddings.reserve(texts.size());
        
        // Process in batches for efficiency
        for (size_t i = 0; i < texts.size(); i += batch_size) {
            size_t batch_end = std::min(i + batch_size, texts.size());
            std::vector<std::string> batch(
                texts.begin() + i,
                texts.begin() + batch_end
            );
            
            auto batch_embeddings = model.encode(batch);
            embeddings.insert(
                embeddings.end(),
                batch_embeddings.begin(),
                batch_embeddings.end()
            );
        }
        
        return embeddings;
    }
    
    // Async embedding generation
    std::future<std::vector<std::vector<float>>> generate_embeddings_async(
        const std::vector<std::string>& texts
    ) {
        return std::async(
            std::launch::async,
            [this, texts]() { return generate_embeddings(texts); }
        );
    }
};

// Usage
EmbeddingGenerator generator("/models/all-MiniLM-L6-v2");

// Get chunks from database
auto chunks = db.query<Chunk>("FOR c IN chunks FILTER c.embedding == null RETURN c");

// Extract text content
std::vector<std::string> texts;
for (const auto& chunk : chunks) {
    texts.push_back(chunk.content);
}

// Generate embeddings
auto embeddings = generator.generate_embeddings(texts);

// Update chunks with embeddings
for (size_t i = 0; i < chunks.size(); ++i) {
    db.update("chunks", chunks[i].id, {
        {"embedding", embeddings[i]},
        {"embedding_model", "all-MiniLM-L6-v2"}
    });
}
```

## Semantic Search

### Vector Similarity Search

```aql
// Basic semantic search using HNSW index
LET query_text = @query
LET query_embedding = LLM_EMBED(query_text, {
    model: "all-MiniLM-L6-v2",
    pooling: "mean",
    normalize: true
})

FOR chunk IN chunks
    LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
    FILTER similarity > 0.7  // Similarity threshold
    SORT similarity DESC
    LIMIT 10
    
    LET doc = FIRST(
        FOR d IN documents
            FILTER d.document_id == chunk.document_id
            RETURN d
    )
    
    RETURN {
        chunk_id: chunk.chunk_id,
        document_id: chunk.document_id,
        document_title: doc.title,
        section: chunk.section,
        content: chunk.content,
        similarity_score: similarity,
        metadata: chunk.metadata
    }

// Using HNSW-specific parameters for better performance
FOR chunk IN chunks
    SEARCH VECTOR(chunk.embedding, query_embedding, {
        metric: "cosine",
        ef_search: 100,  // HNSW search parameter (higher = more accurate but slower)
        top_k: 10
    })
    RETURN {
        chunk_id: chunk.chunk_id,
        content: chunk.content,
        similarity_score: SCORE()
    }
```

### Hybrid Search (Vector + Full-Text)

```aql
// Combine semantic search with keyword matching
LET query_text = @query
LET query_embedding = LLM_EMBED(query_text, {model: "all-MiniLM-L6-v2"})

// Vector search results
LET vector_results = (
    FOR chunk IN chunks
        LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
        FILTER similarity > 0.65
        LIMIT 20
        RETURN {
            chunk: chunk,
            score: similarity,
            method: "vector"
        }
)

// Full-text search results
LET fulltext_results = (
    FOR chunk IN chunks
        SEARCH ANALYZER(chunk.content IN TOKENS(query_text, "text_en"), "text_en")
        LIMIT 20
        RETURN {
            chunk: chunk,
            score: BM25(chunk) / 10,  // Normalize BM25 score
            method: "fulltext"
        }
)

// Merge and re-rank using Reciprocal Rank Fusion (RRF)
FOR result IN UNION(vector_results, fulltext_results)
    COLLECT chunk_id = result.chunk.chunk_id
    AGGREGATE
        chunk = FIRST(result.chunk),
        scores = SUM(result.score),
        methods = UNIQUE(result.method)
    
    // Boost if found by both methods
    LET combined_score = scores * (LENGTH(methods) == 2 ? 1.3 : 1.0)
    
    SORT combined_score DESC
    LIMIT 10
    
    RETURN {
        chunk_id: chunk_id,
        content: chunk.content,
        section: chunk.section,
        relevance_score: combined_score,
        matched_by: methods
    }
```

### Filtered Search with Metadata

```aql
// Search with filters on document metadata
LET query_embedding = LLM_EMBED(@query, {model: "all-MiniLM-L6-v2"})

FOR chunk IN chunks
    // Filter by document metadata
    LET doc = FIRST(
        FOR d IN documents
            FILTER d.document_id == chunk.document_id
            FILTER d.metadata.category IN @categories
            FILTER d.metadata.language == @language
            FILTER d.access.visibility == "public"
            RETURN d
    )
    FILTER doc != null
    
    // Vector similarity search
    LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
    FILTER similarity > 0.7
    
    SORT similarity DESC
    LIMIT @limit
    
    RETURN {
        chunk: chunk,
        document: doc,
        similarity: similarity
    }
```

## RAG Implementation

### Basic RAG Query Pipeline

```aql
// Complete RAG pipeline in single query
LET query = @userQuery
LET query_embedding = LLM_EMBED(query, {model: "all-MiniLM-L6-v2"})

// Step 1: Retrieve relevant chunks
LET relevant_chunks = (
    FOR chunk IN chunks
        LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
        FILTER similarity > 0.7
        SORT similarity DESC
        LIMIT 5
        
        LET doc = FIRST(
            FOR d IN documents
                FILTER d.document_id == chunk.document_id
                RETURN d
        )
        
        RETURN {
            content: chunk.content,
            section: chunk.section,
            document: doc.title,
            similarity: similarity
        }
)

// Step 2: Assemble context
LET context = JOIN(
    FOR chunk IN relevant_chunks
        RETURN CONCAT(
            "[Source: ", chunk.document, " - ", chunk.section, "]\n",
            chunk.content
        ),
    "\n\n---\n\n"
)

// Step 3: Construct prompt
LET prompt = CONCAT(
    "Context information:\n\n",
    context,
    "\n\nQuestion: ", query,
    "\n\nAnswer based on the context provided:"
)

// Step 4: Generate response using LLM
LET response = LLM_QUERY(prompt, {
    model: "llama-3-8b",
    temperature: 0.7,
    max_tokens: 500,
    stop: ["\n\nQuestion:", "\n\nContext:"]
})

// Step 5: Return complete result
RETURN {
    query: query,
    answer: response,
    sources: relevant_chunks,
    metadata: {
        chunks_retrieved: LENGTH(relevant_chunks),
        context_length: LENGTH(context),
        model: "llama-3-8b"
    }
}
```

### Advanced RAG with Re-ranking

```aql
// RAG with cross-encoder re-ranking for better relevance
LET query = @userQuery
LET query_embedding = LLM_EMBED(query, {model: "all-MiniLM-L6-v2"})

// Stage 1: Candidate retrieval (fast, larger pool)
LET candidates = (
    FOR chunk IN chunks
        LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
        FILTER similarity > 0.6
        SORT similarity DESC
        LIMIT 20  // Retrieve more candidates
        RETURN {
            chunk: chunk,
            bi_encoder_score: similarity
        }
)

// Stage 2: Re-rank using cross-encoder (slower, more accurate)
LET reranked = (
    FOR candidate IN candidates
        LET cross_encoder_score = LLM_SCORE(
            query,
            candidate.chunk.content,
            {
                model: "cross-encoder/ms-marco-MiniLM-L-6-v2",
                task: "reranking"
            }
        )
        RETURN {
            chunk: candidate.chunk,
            bi_encoder_score: candidate.bi_encoder_score,
            cross_encoder_score: cross_encoder_score,
            final_score: (candidate.bi_encoder_score + cross_encoder_score) / 2
        }
)

// Stage 3: Select top results after re-ranking
LET top_results = (
    FOR result IN reranked
        SORT result.final_score DESC
        LIMIT 5
        RETURN result
)

// Stage 4: Generate response
LET context = JOIN(
    FOR result IN top_results
        RETURN result.chunk.content,
    "\n\n"
)

LET prompt = CONCAT(
    "Answer the question based on the following context.\n\n",
    "Context:\n", context, "\n\n",
    "Question: ", query, "\n\n",
    "Answer:"
)

LET response = LLM_QUERY(prompt, {
    model: "llama-3-8b",
    temperature: 0.7,
    max_tokens: 500
})

RETURN {
    answer: response,
    sources: top_results,
    retrieval_strategy: "bi-encoder + cross-encoder re-ranking"
}
```

### Multi-Query RAG

```aql
// Generate multiple query variations for better recall
LET original_query = @userQuery

// Generate query variations using LLM
LET query_variations = LLM_QUERY(
    CONCAT(
        "Generate 3 alternative phrasings of this question: '", original_query, "'\n",
        "Format: one question per line."
    ),
    {
        model: "llama-3-8b",
        temperature: 0.8,
        max_tokens: 150
    }
)

LET all_queries = APPEND([original_query], SPLIT(query_variations, "\n"))

// Retrieve chunks for each query variation
LET all_chunks = FLATTEN(
    FOR query IN all_queries
        FILTER LENGTH(TRIM(query)) > 0
        LET query_embedding = LLM_EMBED(query, {model: "all-MiniLM-L6-v2"})
        
        FOR chunk IN chunks
            LET similarity = COSINE_SIMILARITY(chunk.embedding, query_embedding)
            FILTER similarity > 0.7
            RETURN {
                chunk: chunk,
                similarity: similarity,
                query: query
            }
)

// Deduplicate and rank by max similarity
FOR result IN all_chunks
    COLLECT chunk_id = result.chunk.chunk_id
    AGGREGATE
        chunk = FIRST(result.chunk),
        max_similarity = MAX(result.similarity),
        query_matches = COUNT()
    SORT max_similarity DESC
    LIMIT 5
    RETURN {
        chunk: chunk,
        relevance_score: max_similarity,
        matched_queries: query_matches
    }
```

## LLM Integration

### Native llama.cpp Integration

```cpp
// C++ example using ThemisDB's native LLM capabilities
#include <themis/llm/engine.hpp>
#include <themis/client.hpp>

class RAGSystem {
private:
    themis::Client db;
    themis::llm::Engine llm;
    
public:
    RAGSystem(const std::string& db_uri, const std::string& model_path)
        : db(db_uri) {
        
        // Initialize LLM engine with llama.cpp
        llm = themis::llm::Engine::create({
            .model_path = model_path,
            .context_size = 4096,
            .gpu_layers = 35,
            .batch_size = 512,
            .threads = 8,
            .use_mmap = true,
            .use_mlock = false
        });
        
        // Load LoRA adapters if available
        if (std::filesystem::exists("/models/lora/rag-adapter.bin")) {
            llm.load_lora_adapter("/models/lora/rag-adapter.bin");
        }
    }
    
    struct RAGResponse {
        std::string answer;
        std::vector<RetrievedChunk> sources;
        float confidence;
        int total_tokens;
    };
    
    RAGResponse query(const std::string& user_query) {
        // Step 1: Generate query embedding
        auto query_embedding = llm.embed(user_query);
        
        // Step 2: Vector search in ThemisDB
        auto aql = R"(
            FOR chunk IN chunks
                LET similarity = COSINE_SIMILARITY(chunk.embedding, @embedding)
                FILTER similarity > 0.7
                SORT similarity DESC
                LIMIT 5
                RETURN {
                    content: chunk.content,
                    section: chunk.section,
                    similarity: similarity
                }
        )";
        
        auto results = db.query<RetrievedChunk>(aql, {
            {"embedding", query_embedding}
        });
        
        // Step 3: Construct prompt with context
        std::string context;
        for (const auto& chunk : results) {
            context += chunk.content + "\n\n";
        }
        
        std::string prompt = fmt::format(
            "Context:\n{}\n\n"
            "Question: {}\n\n"
            "Answer based on the context:",
            context, user_query
        );
        
        // Step 4: Generate response
        auto response = llm.generate(prompt, {
            .temperature = 0.7,
            .top_p = 0.9,
            .max_tokens = 500,
            .stop_sequences = {"\n\nQuestion:", "\n\nContext:"}
        });
        
        return {
            .answer = response.text,
            .sources = results,
            .confidence = response.confidence,
            .total_tokens = response.tokens_generated
        };
    }
    
    // Streaming response for better UX
    void query_stream(
        const std::string& user_query,
        std::function<void(const std::string&)> callback
    ) {
        auto query_embedding = llm.embed(user_query);
        auto results = retrieve_chunks(query_embedding);
        auto prompt = construct_prompt(results, user_query);
        
        llm.generate_stream(prompt, {
            .temperature = 0.7,
            .max_tokens = 500
        }, callback);
    }
};

// Usage
RAGSystem rag("themis://localhost:8529", "/models/llama-3-8b.gguf");

auto response = rag.query("How does ThemisDB implement vector search?");
std::cout << "Answer: " << response.answer << "\n\n";
std::cout << "Sources:\n";
for (const auto& source : response.sources) {
    std::cout << "  - " << source.section << " (score: " 
              << source.similarity << ")\n";
}
```

### Prompt Engineering for RAG

```aql
// Advanced prompt construction with system message
LET query = @userQuery
LET retrieved_chunks = @chunks  // From retrieval step

LET system_message = @"
You are a helpful AI assistant that answers questions based on provided context.
Rules:
1. Only answer based on the given context
2. If the context doesn't contain enough information, say so
3. Cite sources when making specific claims
4. Be concise but comprehensive
5. If asked about something not in context, politely decline
"@

LET context = JOIN(
    FOR i IN 0..LENGTH(retrieved_chunks)-1
        LET chunk = retrieved_chunks[i]
        RETURN CONCAT(
            "[", i+1, "] ",
            chunk.document, " - ", chunk.section, "\n",
            chunk.content
        ),
    "\n\n"
)

LET user_message = CONCAT(
    "Context information:\n\n",
    context,
    "\n\n---\n\n",
    "Question: ", query, "\n\n",
    "Provide a detailed answer based on the context. ",
    "Use [1], [2], etc. to cite sources."
)

LET response = LLM_CHAT([
    {role: "system", content: system_message},
    {role: "user", content: user_message}
], {
    model: "llama-3-8b",
    temperature: 0.7,
    max_tokens: 1000
})

RETURN {
    answer: response,
    sources: retrieved_chunks
}
```

## Context Optimization

### Context Window Management

```aql
// Optimize context to fit within LLM token limits
LET query = @userQuery
LET max_context_tokens = 3000  // Reserve 1000 for question + response
LET retrieved_chunks = @chunks

// Estimate tokens and select chunks that fit
LET selected_chunks = (
    LET sorted = (
        FOR chunk IN retrieved_chunks
            SORT chunk.similarity DESC
            RETURN chunk
    )
    
    LET result = []
    LET total_tokens = 0
    
    FOR chunk IN sorted
        LET chunk_tokens = chunk.metadata.token_count
        FILTER total_tokens + chunk_tokens <= max_context_tokens
        LET total_tokens = total_tokens + chunk_tokens
        LET result = APPEND(result, chunk)
        RETURN result
)

RETURN selected_chunks
```

### Chunk Deduplication

```aql
// Remove duplicate or highly similar chunks
LET retrieved_chunks = @chunks

// Deduplicate by content similarity
LET deduplicated = (
    FOR i IN 0..LENGTH(retrieved_chunks)-1
        LET chunk_i = retrieved_chunks[i]
        
        // Check if similar to any previous chunk
        LET is_duplicate = LENGTH(
            FOR j IN 0..i-1
                LET chunk_j = retrieved_chunks[j]
                LET similarity = COSINE_SIMILARITY(chunk_i.embedding, chunk_j.embedding)
                FILTER similarity > 0.95
                RETURN 1
        ) > 0
        
        FILTER !is_duplicate
        RETURN chunk_i
)

RETURN deduplicated
```

### Contextual Compression

```aql
// Extract only relevant sentences from chunks
LET query = @userQuery
LET chunks = @retrievedChunks

FOR chunk IN chunks
    // Split into sentences
    LET sentences = SPLIT(chunk.content, ".")
    
    // Score each sentence for relevance
    LET scored_sentences = (
        FOR sentence IN sentences
            FILTER LENGTH(TRIM(sentence)) > 20
            LET sentence_embedding = LLM_EMBED(TRIM(sentence), {model: "all-MiniLM-L6-v2"})
            LET query_embedding = LLM_EMBED(query, {model: "all-MiniLM-L6-v2"})
            LET relevance = COSINE_SIMILARITY(sentence_embedding, query_embedding)
            FILTER relevance > 0.6
            SORT relevance DESC
            RETURN {
                sentence: TRIM(sentence),
                relevance: relevance
            }
    )
    
    // Take top sentences
    LET compressed = JOIN(
        FOR s IN scored_sentences
            LIMIT 3
            RETURN s.sentence,
        ". "
    )
    
    RETURN {
        chunk_id: chunk.chunk_id,
        original_length: LENGTH(chunk.content),
        compressed_content: compressed,
        compressed_length: LENGTH(compressed),
        compression_ratio: LENGTH(compressed) / LENGTH(chunk.content)
    }
```

## Performance Tuning

### Vector Index Optimization

```aql
// Create optimized HNSW index
CREATE INDEX idx_chunks_vector ON chunks (embedding)
    TYPE vector
    OPTIONS {
        dimensions: 768,
        metric: "cosine",
        index_type: "hnsw",
        m: 16,              // Number of connections per layer (higher = more accurate)
        ef_construction: 200, // Construction time parameter (higher = better quality)
        ef_search: 100,      // Search time parameter (higher = more accurate)
        // Adjust based on workload:
        // High recall: m=32, ef_construction=400, ef_search=200
        // Balanced: m=16, ef_construction=200, ef_search=100
        // High speed: m=8, ef_construction=100, ef_search=50
    }

// Monitor index performance
FOR chunk IN chunks
    LET stats = VECTOR_INDEX_STATS("idx_chunks_vector")
    RETURN stats
```

### Caching Strategy

```aql
// Check cache before processing query
LET query = @userQuery
LET query_hash = SHA256(query)

// Check cache
LET cached = FIRST(
    FOR entry IN query_cache
        FILTER entry.query_hash == query_hash
        FILTER entry.expires_at > DATE_NOW()
        
        // Update cache statistics
        UPDATE entry WITH {
            metadata: MERGE(entry.metadata, {
                cache_hits: entry.metadata.cache_hits + 1,
                last_accessed: DATE_ISO8601(DATE_NOW())
            })
        } IN query_cache
        
        RETURN entry
)

// Return cached result if available
FILTER cached != null
RETURN {
    answer: cached.response,
    sources: cached.retrieved_chunks,
    cached: true,
    cache_age_seconds: DATE_DIFF(cached.created_at, DATE_NOW(), "seconds")
}
```

### Batch Processing

```cpp
// Batch embedding generation and insertion
class BatchProcessor {
private:
    themis::Client db;
    themis::llm::Engine llm;
    const size_t batch_size = 100;
    
public:
    void process_documents(const std::vector<Document>& documents) {
        // Process in batches
        for (size_t i = 0; i < documents.size(); i += batch_size) {
            auto batch_end = std::min(i + batch_size, documents.size());
            std::vector<Document> batch(
                documents.begin() + i,
                documents.begin() + batch_end
            );
            
            process_batch(batch);
        }
    }
    
private:
    void process_batch(const std::vector<Document>& batch) {
        // Step 1: Chunk all documents
        std::vector<Chunk> all_chunks;
        for (const auto& doc : batch) {
            auto chunks = chunk_document(doc);
            all_chunks.insert(all_chunks.end(), chunks.begin(), chunks.end());
        }
        
        // Step 2: Generate embeddings in batch
        std::vector<std::string> texts;
        for (const auto& chunk : all_chunks) {
            texts.push_back(chunk.content);
        }
        auto embeddings = llm.embed_batch(texts);
        
        // Step 3: Batch insert into database
        std::vector<json> chunk_docs;
        for (size_t i = 0; i < all_chunks.size(); ++i) {
            chunk_docs.push_back({
                {"chunk_id", all_chunks[i].id},
                {"content", all_chunks[i].content},
                {"embedding", embeddings[i]},
                {"metadata", all_chunks[i].metadata}
            });
        }
        
        db.insert_batch("chunks", chunk_docs);
    }
};
```

## Production Deployment

### Monitoring and Metrics

```cpp
// Prometheus metrics for RAG system
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>

class RAGMetrics {
private:
    prometheus::Counter& queries_total;
    prometheus::Counter& cache_hits;
    prometheus::Histogram& retrieval_latency;
    prometheus::Histogram& generation_latency;
    prometheus::Histogram& context_length;
    prometheus::Gauge& active_queries;
    
public:
    void record_query(
        bool cache_hit,
        double retrieval_ms,
        double generation_ms,
        size_t context_tokens
    ) {
        queries_total.Increment();
        if (cache_hit) cache_hits.Increment();
        retrieval_latency.Observe(retrieval_ms);
        generation_latency.Observe(generation_ms);
        context_length.Observe(context_tokens);
    }
};
```

### Configuration Example

```yaml
# RAG system configuration
rag:
  embeddings:
    model: "all-MiniLM-L6-v2"
    dimensions: 384
    batch_size: 32
    device: "cuda:0"
  
  retrieval:
    strategy: "hybrid"  # vector, fulltext, hybrid
    top_k: 5
    similarity_threshold: 0.7
    rerank: true
    rerank_model: "cross-encoder/ms-marco-MiniLM-L-6-v2"
  
  generation:
    model: "llama-3-8b"
    context_window: 4096
    max_output_tokens: 500
    temperature: 0.7
    top_p: 0.9
    lora_adapters: ["rag-adapter"]
  
  caching:
    enabled: true
    ttl_seconds: 3600
    max_entries: 10000
  
  chunking:
    strategy: "semantic"
    target_size: 512
    max_size: 1024
    overlap: 50
```

## Best Practices

1. **Chunking Strategy**
   - Use semantic chunking for better context preservation
   - Include overlap between chunks (50-100 tokens)
   - Preserve document structure (sections, paragraphs)
   - Test different chunk sizes for your use case

2. **Embedding Models**
   - Choose model based on use case (speed vs accuracy)
   - Use domain-specific models when available
   - Batch embedding generation for efficiency
   - Normalize embeddings for cosine similarity

3. **Retrieval**
   - Implement hybrid search (vector + keyword)
   - Use re-ranking for improved relevance
   - Filter by metadata to reduce search space
   - Adjust HNSW parameters based on workload

4. **Context Management**
   - Respect LLM context window limits
   - Prioritize most relevant chunks
   - Remove duplicate content
   - Compress context when needed

5. **Prompt Engineering**
   - Use clear system messages
   - Provide explicit instructions
   - Include source citation format
   - Test with various query types

6. **Performance**
   - Cache frequently asked questions
   - Use batch processing for ingestion
   - Monitor and optimize slow queries
   - Consider read replicas for scaling

## Related Documentation

- [Vector Search Guide](../features/vector-search.md)
- [LLM Integration](../llm/integration.md)
- [LoRA Adapters](../llm/lora-adapters.md)
- [AQL Functions](../aql/functions.md)

## Example Projects

- [Document Search Example](../../examples/07_vector_search_documents/)
- [LLM Examples](../../examples/llm/)
- [Chatbot Implementation](../../examples/18_realtime_chat/)

## Conclusion

ThemisDB's native LLM integration and efficient vector search make it an ideal platform for building production RAG systems. The combination of HNSW indexing, integrated llama.cpp, and flexible AQL queries enables sophisticated retrieval and generation workflows with minimal infrastructure complexity.
