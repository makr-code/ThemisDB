-- Legal LoRA Training Pipeline - Database Schema
-- Complete schema for multi-source legal document ingestion and training

-- ============================================================================
-- GRAPH LAYER: Knowledge Graph for Legal Documents
-- ============================================================================

-- Create the legal knowledge graph
CREATE GRAPH legal_knowledge_graph;

-- Document collection (vertices)
CREATE COLLECTION legal_documents WITH SCHEMA {
    _key: string,
    title: string,
    content: string,
    document_type: string,        -- e.g., "regulation", "case_law", "guidance"
    source_id: string,             -- Source identifier (HuggingFace, filesystem, etc.)
    language: string,              -- "de", "en", etc.
    jurisdiction: string,          -- e.g., "federal", "state", "municipal"
    effective_date: date,
    created_at: timestamp,
    updated_at: timestamp,
    embedding: array,              -- 768-dim vector for semantic search
    metadata: object               -- Additional metadata (JSON)
};

-- Paragraph collection (vertices)
CREATE COLLECTION paragraphs WITH SCHEMA {
    _key: string,
    document_key: string,
    paragraph_number: integer,
    content: string,
    embedding: array,
    created_at: timestamp
};

-- Sentence collection (vertices)
CREATE COLLECTION sentences WITH SCHEMA {
    _key: string,
    paragraph_key: string,
    sentence_number: integer,
    content: string,
    embedding: array,
    created_at: timestamp
};

-- Modal verb collection (vertices)
CREATE COLLECTION modal_verbs WITH SCHEMA {
    _key: string,
    sentence_key: string,
    verb: string,                  -- e.g., "muss", "soll", "kann"
    category: string,              -- "obligation", "permission", "prohibition"
    strength: float,               -- Normative strength [0.0, 1.0]
    deontic_logic: string,         -- e.g., "O(φ)", "P(φ)", "F(φ)"
    interpretation: string,        -- Legal interpretation
    context_requirements: array,   -- Required checks/considerations
    position: integer,
    created_at: timestamp
};

-- Legal provisions collection (vertices)
CREATE COLLECTION legal_provisions WITH SCHEMA {
    _key: string,
    provision_id: string,          -- e.g., "BGB §123", "StGB §242"
    title: string,
    content: string,
    law_name: string,
    section: string,
    embedding: array,
    created_at: timestamp
};

-- Edge definitions for the graph
CREATE EDGE COLLECTION contains FROM legal_documents TO paragraphs;
CREATE EDGE COLLECTION has_paragraph FROM paragraphs TO sentences;
CREATE EDGE COLLECTION has_modality FROM sentences TO modal_verbs;
CREATE EDGE COLLECTION references FROM legal_documents TO legal_provisions;
CREATE EDGE COLLECTION similar_to FROM legal_documents TO legal_documents WITH SCHEMA {
    similarity_score: float
};
CREATE EDGE COLLECTION related_to FROM legal_provisions TO legal_provisions;
CREATE EDGE COLLECTION cites FROM legal_documents TO legal_documents WITH SCHEMA {
    citation_context: string
};

-- Register edge definitions with the graph
ALTER GRAPH legal_knowledge_graph
    ADD EDGE DEFINITIONS (
        contains FROM legal_documents TO paragraphs,
        has_paragraph FROM paragraphs TO sentences,
        has_modality FROM sentences TO modal_verbs,
        references FROM legal_documents TO legal_provisions,
        similar_to FROM legal_documents TO legal_documents,
        related_to FROM legal_provisions TO legal_provisions,
        cites FROM legal_documents TO legal_documents
    );

-- ============================================================================
-- RELATIONAL LAYER: Training Samples
-- ============================================================================

-- Training samples collection
CREATE COLLECTION legal_training_samples WITH SCHEMA {
    _key: string,
    input: string,                 -- Input text for training
    output: string,                -- Expected output/label
    category: string,              -- Category (e.g., "obligation", "permission")
    confidence: float,             -- Confidence score [0.0, 1.0]
    source_document_key: string,   -- Source document reference
    graph_context: string,         -- Enriched context from graph (JSON)
    embedding: array,              -- 768-dim vector
    
    -- Auto-labeling metadata
    labeled_by: string,            -- "auto" or user ID
    labeled_at: timestamp,
    reviewed: boolean,             -- Whether reviewed by human
    reviewed_by: string,
    reviewed_at: timestamp,
    
    -- Training metadata
    used_in_training: boolean,
    training_version: string,      -- Which adapter version used this sample
    
    created_at: timestamp,
    updated_at: timestamp
};

-- Ingestion sources tracking
CREATE COLLECTION ingestion_sources WITH SCHEMA {
    _key: string,
    source_id: string,
    source_type: string,           -- "huggingface", "filesystem", "api", "database"
    location: string,              -- URL, path, etc.
    priority: integer,
    enabled: boolean,
    last_ingestion: timestamp,
    documents_ingested: integer,
    bytes_ingested: integer,
    options: object,               -- Source-specific options
    created_at: timestamp,
    updated_at: timestamp
};

-- Ingestion history/logs
CREATE COLLECTION ingestion_logs WITH SCHEMA {
    _key: string,
    source_id: string,
    started_at: timestamp,
    completed_at: timestamp,
    status: string,                -- "success", "failed", "partial"
    documents_processed: integer,
    documents_failed: integer,
    bytes_processed: integer,
    error_message: string,
    metadata: object
};

-- ============================================================================
-- VECTOR LAYER: Semantic Search
-- ============================================================================

-- Create vector indexes for semantic search
CREATE VECTOR INDEX legal_documents_embedding 
    ON legal_documents (embedding) 
    WITH { 
        dimensions: 768, 
        metric: "cosine",
        ef_construction: 200,
        m: 16
    };

CREATE VECTOR INDEX training_samples_embedding 
    ON legal_training_samples (embedding) 
    WITH { 
        dimensions: 768, 
        metric: "cosine",
        ef_construction: 200,
        m: 16
    };

CREATE VECTOR INDEX provisions_embedding 
    ON legal_provisions (embedding) 
    WITH { 
        dimensions: 768, 
        metric: "cosine",
        ef_construction: 200,
        m: 16
    };

-- ============================================================================
-- INDEXES: Performance Optimization
-- ============================================================================

-- Document indexes
CREATE INDEX legal_documents_type ON legal_documents (document_type);
CREATE INDEX legal_documents_source ON legal_documents (source_id);
CREATE INDEX legal_documents_language ON legal_documents (language);
CREATE INDEX legal_documents_date ON legal_documents (effective_date);

-- Training sample indexes
CREATE INDEX training_samples_category ON legal_training_samples (category);
CREATE INDEX training_samples_confidence ON legal_training_samples (confidence);
CREATE INDEX training_samples_source ON legal_training_samples (source_document_key);
CREATE INDEX training_samples_labeled ON legal_training_samples (labeled_by);
CREATE INDEX training_samples_reviewed ON legal_training_samples (reviewed);
CREATE INDEX training_samples_used ON legal_training_samples (used_in_training);

-- Ingestion tracking indexes
CREATE INDEX ingestion_sources_type ON ingestion_sources (source_type);
CREATE INDEX ingestion_sources_enabled ON ingestion_sources (enabled);
CREATE INDEX ingestion_logs_source ON ingestion_logs (source_id);
CREATE INDEX ingestion_logs_status ON ingestion_logs (status);

-- ============================================================================
-- EXAMPLE QUERIES
-- ============================================================================

/*
-- Find all legal documents from a specific source
FOR doc IN legal_documents
    FILTER doc.source_id == "huggingface_legal"
    RETURN doc

-- Find training samples with low confidence for review
FOR sample IN legal_training_samples
    FILTER sample.confidence < 0.5
    FILTER sample.reviewed == false
    SORT sample.confidence ASC
    LIMIT 100
    RETURN sample

-- Find related documents via graph traversal
FOR doc IN legal_documents
    FILTER doc._key == "doc123"
    FOR related, edge IN 1..2 OUTBOUND doc GRAPH legal_knowledge_graph
        RETURN {
            document: related,
            relationship: edge._from + " -> " + edge._to,
            score: edge.similarity_score
        }

-- Semantic search for similar documents
FOR doc IN legal_documents
    LET query_embedding = EMBEDDING(legal_documents, "contract law obligations")
    LET similar = (
        FOR candidate IN legal_documents
            LET score = COSINE_SIMILARITY(query_embedding, candidate.embedding)
            FILTER score > 0.7
            SORT score DESC
            LIMIT 10
            RETURN {doc: candidate, score: score}
    )
    RETURN similar

-- Find documents with specific modal verbs
FOR doc IN legal_documents
    FOR para IN OUTBOUND doc contains
        FOR sent IN OUTBOUND para has_paragraph
            FOR modal IN OUTBOUND sent has_modality
                FILTER modal.verb == "muss"
                FILTER modal.strength > 0.8
                RETURN {
                    document: doc.title,
                    sentence: sent.content,
                    modality: modal
                }
*/
