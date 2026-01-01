-- PostgreSQL Initialization Script for Comparative Benchmarks
-- Creates necessary tables and indexes for benchmark tests

-- Create benchmark database schema
CREATE SCHEMA IF NOT EXISTS benchmark;

-- Documents table (for CRUD and query benchmarks)
CREATE TABLE IF NOT EXISTS benchmark.documents (
    id VARCHAR(255) PRIMARY KEY,
    title TEXT NOT NULL,
    content TEXT,
    category VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    metadata JSONB
);

-- Create indexes for query benchmarks
CREATE INDEX IF NOT EXISTS idx_documents_category ON benchmark.documents(category);
CREATE INDEX IF NOT EXISTS idx_documents_created_at ON benchmark.documents(created_at);
CREATE INDEX IF NOT EXISTS idx_documents_metadata ON benchmark.documents USING GIN(metadata);

-- Full-text search index
CREATE INDEX IF NOT EXISTS idx_documents_fts ON benchmark.documents USING GIN(to_tsvector('english', title || ' ' || COALESCE(content, '')));

-- Users table (for graph-like relationship benchmarks)
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

-- Relationships table (for graph benchmarks)
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
CREATE INDEX IF NOT EXISTS idx_relationships_type ON benchmark.relationships(relationship_type);

-- Function to update timestamps
CREATE OR REPLACE FUNCTION benchmark.update_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Trigger for automatic timestamp updates
CREATE TRIGGER documents_updated_at
    BEFORE UPDATE ON benchmark.documents
    FOR EACH ROW
    EXECUTE FUNCTION benchmark.update_updated_at();

-- Grant permissions
GRANT ALL PRIVILEGES ON SCHEMA benchmark TO benchmark;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA benchmark TO benchmark;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA benchmark TO benchmark;
