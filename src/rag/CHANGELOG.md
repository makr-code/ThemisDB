<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — RAG Module

All notable changes to the RAG module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- Multi-modal RAG pipeline with text, image, and audio retrieval
- Hybrid dense+sparse retrieval (BM25 + vector)
- Cross-encoder reranking for improved relevance scoring
- Contextual compression to reduce token usage
- Citation and source attribution in generated answers
- RAG evaluation framework with RAGAS metrics
- Streaming RAG responses via SSE

### Changed
- Improved chunking strategies (sentence-aware, recursive)
- Enhanced metadata filtering during retrieval

## [1.0.0] — 2024-06-01
### Added
- Initial RAG pipeline: document ingestion, chunking, embedding, retrieval
- Integration with LLM module for answer generation
- Vector store integration for semantic search
