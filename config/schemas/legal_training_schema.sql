-- ============================================================================
-- Legal Training Data Schema for ThemisDB
-- ============================================================================
-- This schema supports:
-- 1. Multi-source legal data ingestion (HuggingFace + custom docs)
-- 2. Auto-labeling using NLP Analyzer
-- 3. Knowledge graph enrichment
-- 4. LoRA training with continuous learning
-- ============================================================================

-- ============================================================================
-- GRAPH LAYER: Legal Knowledge Graph
-- ============================================================================

-- Legal documents (source texts)
CREATE COLLECTION legal_documents OPTIONS { 
    type: "document",
    schema: {
        text: { type: "string", required: true },
        source: { type: "string", required: true },
        source_type: { type: "string" },        -- "huggingface", "filesystem", "api"
        title: { type: "string" },
        document_type: { type: "string" },      -- "law", "regulation", "case_law", "custom"
        date: { type: "string" },
        authority: { type: "string" },
        tags: { type: "array" },
        priority: { type: "number", default: 0 },
        raw_data: { type: "boolean", default: true },  -- Not yet labeled
        labeled_at: { type: "string" },
        samples_created: { type: "number" },
        metadata: { type: "object" },
        embedding: { type: "array" },           -- E5-multilingual-base (768 dim)
        created_at: { type: "string" },
        updated_at: { type: "string" }
    }
}

-- Structural elements
CREATE COLLECTION paragraphs OPTIONS { 
    type: "document",
    schema: {
        document_id: { type: "string", required: true },
        index: { type: "number", required: true },
        text: { type: "string", required: true },
        start_char: { type: "number" },
        end_char: { type: "number" }
    }
}

CREATE COLLECTION sentences OPTIONS { 
    type: "document",
    schema: {
        paragraph_id: { type: "string", required: true },
        index: { type: "number", required: true },
        text: { type: "string", required: true },
        start_char: { type: "number" },
        end_char: { type: "number" }
    }
}

-- Legal entities
CREATE COLLECTION legal_provisions OPTIONS { 
    type: "document",
    schema: {
        provision_id: { type: "string", required: true },
        law: { type: "string" },                -- e.g., "BGB", "StGB"
        section: { type: "string" },            -- e.g., "§ 242"
        text: { type: "string" },
        interpretation: { type: "string" },
        created_at: { type: "string" }
    }
}

CREATE COLLECTION modal_verbs OPTIONS { 
    type: "document",
    schema: {
        verb: { type: "string", required: true },
        category: { type: "string", required: true },  -- "obligation", "permission", "prohibition"
        strength: { type: "number" },                  -- Normative strength [0, 1]
        deontic_logic: { type: "string" },             -- e.g., "MUST", "MAY", "MUST_NOT"
        interpretation: { type: "string" },
        position: { type: "number" },
        sentence_id: { type: "string" }
    }
}

CREATE COLLECTION case_law OPTIONS { 
    type: "document",
    schema: {
        case_id: { type: "string", required: true },
        court: { type: "string" },
        date: { type: "string" },
        summary: { type: "string" },
        full_text: { type: "string" },
        provisions_cited: { type: "array" },
        created_at: { type: "string" }
    }
}

-- Edge collections (relationships)
CREATE COLLECTION contains OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        relationship_type: { type: "string" }
    }
}

CREATE COLLECTION has_modality OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        confidence: { type: "number" }
    }
}

CREATE COLLECTION references OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        context: { type: "string" }
    }
}

CREATE COLLECTION interprets OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        interpretation_type: { type: "string" }
    }
}

CREATE COLLECTION similar_to OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        similarity_score: { type: "number" }
    }
}

CREATE COLLECTION supersedes OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true },
        effective_date: { type: "string" }
    }
}

CREATE COLLECTION cited_in OPTIONS { 
    type: "edge",
    schema: {
        _from: { type: "string", required: true },
        _to: { type: "string", required: true }
    }
}

-- Named graph
CREATE GRAPH legal_knowledge_graph
    EDGE DEFINITIONS (
        contains FROM legal_documents TO paragraphs,
        contains FROM paragraphs TO sentences,
        has_modality FROM sentences TO modal_verbs,
        has_modality FROM paragraphs TO modal_verbs,
        references FROM legal_documents TO legal_provisions,
        interprets FROM case_law TO legal_provisions,
        similar_to FROM legal_documents TO legal_documents,
        supersedes FROM legal_documents TO legal_documents,
        cited_in FROM case_law TO legal_provisions
    )

-- ============================================================================
-- RELATIONAL LAYER: Training Data
-- ============================================================================

-- Training samples (auto-labeled and enriched)
CREATE COLLECTION legal_training_samples OPTIONS { 
    type: "document",
    schema: {
        input: { type: "string", required: true },
        output: { type: "string", required: true },
        category: { type: "string", required: true },  -- "obligation", "permission", "prohibition"
        strength: { type: "number" },
        source_doc_id: { type: "string" },
        source_type: { type: "string" },               -- "huggingface", "custom", "legacy"
        auto_labeled: { type: "boolean", default: true },
        confidence: { type: "number" },                -- Confidence score [0, 1]
        needs_review: { type: "boolean", default: false },
        reviewed_by: { type: "string" },
        review_date: { type: "string" },
        context: { type: "string" },                   -- Legal interpretation
        graph_context: { type: "string" },             -- Added by enricher
        enrichment_sources: { type: "number" },
        context_quality_score: { type: "number" },
        embedding: { type: "array" },                  -- 768-dimensional
        created_at: { type: "string" },
        updated_at: { type: "string" }
    }
}

-- Modal verb annotations (extracted by NLP Analyzer)
CREATE COLLECTION modal_verb_annotations OPTIONS {
    type: "document",
    schema: {
        document_id: { type: "string", required: true },
        verb: { type: "string", required: true },
        category: { type: "string", required: true },
        position: { type: "number" },
        context_before: { type: "string" },
        context_after: { type: "string" },
        deontic_logic: { type: "string" },
        interpretation: { type: "string" },
        confidence: { type: "number" },
        sentence_index: { type: "number" },
        created_at: { type: "string" }
    }
}

-- Ingestion metadata (tracking)
CREATE COLLECTION ingestion_metadata OPTIONS {
    type: "document",
    schema: {
        source_id: { type: "string", required: true },
        source_type: { type: "string" },
        last_ingested: { type: "string" },
        documents_count: { type: "number" },
        status: { type: "string" },
        error_message: { type: "string" },
        config: { type: "object" },
        created_at: { type: "string" },
        updated_at: { type: "string" }
    }
}

-- LoRA adapter metadata
CREATE COLLECTION lora_adapters OPTIONS {
    type: "document",
    schema: {
        adapter_id: { type: "string", required: true },
        base_model: { type: "string", required: true },
        domain: { type: "string" },
        rank: { type: "number" },
        alpha: { type: "number" },
        training_samples: { type: "number" },
        validation_accuracy: { type: "number" },
        embedding: { type: "array" },                   -- For semantic routing
        metadata: { type: "object" },
        created_at: { type: "string" },
        version: { type: "string" }
    }
}

-- ============================================================================
-- VECTOR LAYER: Embeddings
-- ============================================================================

-- Full-text indexes
CREATE FULLTEXT INDEX legal_documents_text 
    ON legal_documents (text)

CREATE FULLTEXT INDEX legal_documents_title 
    ON legal_documents (title)

CREATE FULLTEXT INDEX legal_training_samples_input 
    ON legal_training_samples (input)

-- Vector indexes (for semantic search)
CREATE VECTOR INDEX legal_documents_embedding 
    ON legal_documents (embedding) 
    WITH { 
        dimensions: 768,        -- E5-multilingual-base
        metric: "cosine",
        index_type: "hnsw",
        m: 16,
        ef_construction: 200,
        ef_search: 100
    }

CREATE VECTOR INDEX legal_training_samples_embedding
    ON legal_training_samples (embedding)
    WITH {
        dimensions: 768,
        metric: "cosine",
        index_type: "hnsw"
    }

CREATE VECTOR INDEX lora_adapters_embedding
    ON lora_adapters (embedding)
    WITH {
        dimensions: 768,
        metric: "cosine",
        index_type: "hnsw"
    }

-- ============================================================================
-- INDEXES FOR PERFORMANCE
-- ============================================================================

-- Document indexes
CREATE INDEX legal_documents_source ON legal_documents (source)
CREATE INDEX legal_documents_source_type ON legal_documents (source_type)
CREATE INDEX legal_documents_tags ON legal_documents (tags[*])
CREATE INDEX legal_documents_raw_data ON legal_documents (raw_data)
CREATE INDEX legal_documents_date ON legal_documents (date)
CREATE INDEX legal_documents_priority ON legal_documents (priority)

-- Training sample indexes
CREATE INDEX legal_training_samples_category ON legal_training_samples (category)
CREATE INDEX legal_training_samples_source_type ON legal_training_samples (source_type)
CREATE INDEX legal_training_samples_confidence ON legal_training_samples (confidence)
CREATE INDEX legal_training_samples_needs_review ON legal_training_samples (needs_review)
CREATE INDEX legal_training_samples_auto_labeled ON legal_training_samples (auto_labeled)

-- Annotation indexes
CREATE INDEX modal_verb_annotations_document ON modal_verb_annotations (document_id)
CREATE INDEX modal_verb_annotations_category ON modal_verb_annotations (category)

-- Ingestion metadata indexes
CREATE INDEX ingestion_metadata_source ON ingestion_metadata (source_id)
CREATE INDEX ingestion_metadata_status ON ingestion_metadata (status)

-- ============================================================================
-- VIEWS FOR CONVENIENT DATA ACCESS
-- ============================================================================

-- Training data view (enriched with graph context)
CREATE VIEW legal_training_view AS
    FOR sample IN legal_training_samples
        LET doc = DOCUMENT(sample.source_doc_id)
        LET modalities = (
            FOR v IN 1..1 OUTBOUND doc contains, has_modality
                FILTER v._collection == "modal_verbs"
                RETURN v
        )
        LET related = (
            FOR v IN 1..2 OUTBOUND doc references, similar_to
                LIMIT 5
                RETURN v
        )
        RETURN MERGE(sample, {
            document: doc,
            modalities: modalities,
            related_docs: related
        })

-- Samples needing human review
CREATE VIEW samples_needing_review AS
    FOR sample IN legal_training_samples
        FILTER sample.needs_review == true
        FILTER sample.reviewed_by == null OR sample.reviewed_by == ""
        SORT sample.confidence ASC
        RETURN sample

-- Document statistics by source
CREATE VIEW documents_by_source AS
    FOR doc IN legal_documents
        COLLECT source = doc.source WITH COUNT INTO count
        RETURN {
            source: source,
            count: count
        }

-- Training sample distribution by category
CREATE VIEW samples_by_category AS
    FOR sample IN legal_training_samples
        COLLECT category = sample.category WITH COUNT INTO count
        RETURN {
            category: category,
            count: count
        }

-- Recent ingestion activity
CREATE VIEW recent_ingestion_activity AS
    FOR meta IN ingestion_metadata
        SORT meta.last_ingested DESC
        LIMIT 20
        RETURN meta
