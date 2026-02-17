-- PostgreSQL with pgvector Initialization Script
-- Includes vector extension for similarity search benchmarks

-- Enable pgvector extension
CREATE EXTENSION IF NOT EXISTS vector;

-- Create benchmark database schema
CREATE SCHEMA IF NOT EXISTS benchmark;

-- Documents table with vector embedding
CREATE TABLE IF NOT EXISTS benchmark.documents (
    id VARCHAR(255) PRIMARY KEY,
    title TEXT NOT NULL,
    content TEXT,
    category VARCHAR(100),
    embedding vector(384),  -- 384 dimensions for MiniLM embeddings
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    metadata JSONB
);

-- Create indexes
CREATE INDEX IF NOT EXISTS idx_documents_category ON benchmark.documents(category);
CREATE INDEX IF NOT EXISTS idx_documents_created_at ON benchmark.documents(created_at);
CREATE INDEX IF NOT EXISTS idx_documents_metadata ON benchmark.documents USING GIN(metadata);

-- Create HNSW index for vector search
CREATE INDEX IF NOT EXISTS idx_documents_embedding ON benchmark.documents 
    USING hnsw (embedding vector_l2_ops) WITH (m = 16, ef_construction = 200);

-- Full-text search index
CREATE INDEX IF NOT EXISTS idx_documents_fts ON benchmark.documents 
    USING GIN(to_tsvector('english', title || ' ' || COALESCE(content, '')));

-- Users table
CREATE TABLE IF NOT EXISTS benchmark.users (
    id VARCHAR(255) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255),
    age INTEGER,
    city VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_users_city ON benchmark.users(city);
CREATE INDEX IF NOT EXISTS idx_users_age ON benchmark.users(age);

-- Relationships table
CREATE TABLE IF NOT EXISTS benchmark.relationships (
    id SERIAL PRIMARY KEY,
    from_user VARCHAR(255) REFERENCES benchmark.users(id),
    to_user VARCHAR(255) REFERENCES benchmark.users(id),
    relationship_type VARCHAR(50),
    weight FLOAT DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_relationships_from ON benchmark.relationships(from_user);
CREATE INDEX IF NOT EXISTS idx_relationships_to ON benchmark.relationships(to_user);

-- Grant permissions
GRANT ALL PRIVILEGES ON SCHEMA benchmark TO benchmark;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA benchmark TO benchmark;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA benchmark TO benchmark;
