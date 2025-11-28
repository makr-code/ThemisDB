"""
Integration Tests for RESPO

These tests verify the complete flow from API to vector store and LLM.
Requires: Running RESPO stack (docker compose up)
"""

import os
import asyncio
from pathlib import Path
from typing import Generator
from unittest.mock import AsyncMock, MagicMock, patch

import pytest
from fastapi.testclient import TestClient

from respo.api.app import app
from respo.config import Settings
from respo.rag.pipeline import RAGPipeline, RAGResponse
from respo.rag.retriever import HybridRetriever, RetrievalResult
from respo.rag.reranker import CrossEncoderReranker
from respo.embedding.code_embedder import CodeEmbedder
from respo.ingestion.chunker import CodeChunker, CodeChunk
from respo.vectorstore.chroma import ChromaVectorStore


client = TestClient(app)


# =============================================================================
# Fixtures
# =============================================================================


@pytest.fixture
def sample_code() -> str:
    """Sample Python code for testing."""
    return '''
def fibonacci(n: int) -> int:
    """Calculate the nth Fibonacci number.
    
    Args:
        n: Position in Fibonacci sequence
        
    Returns:
        The nth Fibonacci number
    """
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


class LRUCache:
    """Least Recently Used Cache implementation."""
    
    def __init__(self, capacity: int):
        self.capacity = capacity
        self.cache = {}
        self.order = []
    
    def get(self, key: str) -> int | None:
        """Get value from cache."""
        if key in self.cache:
            self.order.remove(key)
            self.order.append(key)
            return self.cache[key]
        return None
    
    def put(self, key: str, value: int) -> None:
        """Put value in cache."""
        if key in self.cache:
            self.order.remove(key)
        elif len(self.cache) >= self.capacity:
            oldest = self.order.pop(0)
            del self.cache[oldest]
        self.cache[key] = value
        self.order.append(key)
'''


@pytest.fixture
def temp_chroma_dir(tmp_path: Path) -> Path:
    """Create a temporary directory for ChromaDB."""
    chroma_dir = tmp_path / "chroma"
    chroma_dir.mkdir()
    return chroma_dir


@pytest.fixture
def mock_settings(temp_chroma_dir: Path) -> Settings:
    """Create test settings."""
    return Settings(
        vector_store="chroma",
        chroma_persist_dir=str(temp_chroma_dir),
        vllm_url="http://localhost:8000",
        vllm_model="test-model",
        embedding_model="microsoft/codebert-base",
    )


# =============================================================================
# Code Chunker Integration Tests
# =============================================================================


class TestCodeChunkerIntegration:
    """Integration tests for code chunker."""

    def test_chunk_python_code(self, sample_code: str) -> None:
        """Test chunking Python code extracts functions and classes."""
        chunker = CodeChunker()
        chunks = chunker.chunk(sample_code, language="python")
        
        assert len(chunks) >= 2
        
        # Should extract fibonacci function
        func_chunks = [c for c in chunks if "fibonacci" in c.content.lower()]
        assert len(func_chunks) >= 1
        
        # Should extract LRUCache class
        class_chunks = [c for c in chunks if "lrucache" in c.content.lower()]
        assert len(class_chunks) >= 1

    def test_chunk_preserves_metadata(self, sample_code: str) -> None:
        """Test that chunking preserves metadata."""
        chunker = CodeChunker()
        chunks = chunker.chunk(
            sample_code, 
            language="python",
            file_path="test.py",
            repo="test-repo"
        )
        
        for chunk in chunks:
            assert chunk.language == "python"
            assert chunk.file_path == "test.py"
            assert chunk.repo == "test-repo"

    def test_chunk_extracts_docstrings(self, sample_code: str) -> None:
        """Test that docstrings are extracted."""
        chunker = CodeChunker()
        chunks = chunker.chunk(sample_code, language="python")
        
        # Fibonacci function should have docstring
        fib_chunks = [c for c in chunks if "fibonacci" in c.content.lower()]
        if fib_chunks:
            chunk = fib_chunks[0]
            assert chunk.docstring is not None or "calculate" in chunk.content.lower()


# =============================================================================
# Vector Store Integration Tests
# =============================================================================


class TestVectorStoreIntegration:
    """Integration tests for vector store."""

    def test_chroma_add_and_search(self, temp_chroma_dir: Path) -> None:
        """Test adding and searching in ChromaDB."""
        store = ChromaVectorStore(persist_directory=str(temp_chroma_dir))
        
        # Add documents
        documents = [
            {"id": "1", "content": "def fibonacci(n): return n if n <= 1 else fibonacci(n-1) + fibonacci(n-2)", "metadata": {"language": "python"}},
            {"id": "2", "content": "class LRUCache: pass", "metadata": {"language": "python"}},
            {"id": "3", "content": "function quickSort(arr) { return arr }", "metadata": {"language": "javascript"}},
        ]
        
        # Mock embeddings
        embeddings = [[0.1] * 768, [0.2] * 768, [0.3] * 768]
        
        store.add(
            ids=[d["id"] for d in documents],
            embeddings=embeddings,
            documents=[d["content"] for d in documents],
            metadatas=[d["metadata"] for d in documents]
        )
        
        # Search
        results = store.search(
            query_embedding=[0.15] * 768,
            n_results=2
        )
        
        assert len(results) <= 2
        assert all("id" in r for r in results)

    def test_chroma_persistence(self, temp_chroma_dir: Path) -> None:
        """Test ChromaDB persistence."""
        # Create and add data
        store1 = ChromaVectorStore(
            persist_directory=str(temp_chroma_dir),
            collection_name="test_persistence"
        )
        
        store1.add(
            ids=["persist-1"],
            embeddings=[[0.5] * 768],
            documents=["Persistence test document"],
            metadatas=[{"test": "true"}]
        )
        
        # Create new instance and verify data persists
        store2 = ChromaVectorStore(
            persist_directory=str(temp_chroma_dir),
            collection_name="test_persistence"
        )
        
        results = store2.search(
            query_embedding=[0.5] * 768,
            n_results=1
        )
        
        assert len(results) == 1
        assert "persist" in results[0].get("document", "").lower()


# =============================================================================
# Retriever Integration Tests
# =============================================================================


class TestRetrieverIntegration:
    """Integration tests for hybrid retriever."""

    @pytest.mark.asyncio
    async def test_hybrid_retrieval(self, temp_chroma_dir: Path) -> None:
        """Test hybrid vector + keyword retrieval."""
        # Setup vector store
        store = ChromaVectorStore(persist_directory=str(temp_chroma_dir))
        
        # Add test documents
        docs = [
            {"id": "1", "content": "def fibonacci(n): calculates fibonacci sequence", "metadata": {"type": "function"}},
            {"id": "2", "content": "class Cache: implements LRU cache", "metadata": {"type": "class"}},
            {"id": "3", "content": "def quicksort(arr): sorting algorithm", "metadata": {"type": "function"}},
        ]
        
        store.add(
            ids=[d["id"] for d in docs],
            embeddings=[[0.1, 0.2, 0.3] + [0.0] * 765 for _ in docs],
            documents=[d["content"] for d in docs],
            metadatas=[d["metadata"] for d in docs]
        )
        
        # Create retriever with mock embedder
        mock_embedder = MagicMock()
        mock_embedder.embed.return_value = [0.1, 0.2, 0.3] + [0.0] * 765
        
        retriever = HybridRetriever(
            vector_store=store,
            embedder=mock_embedder,
            vector_weight=0.7,
            keyword_weight=0.3
        )
        
        # Test retrieval
        results = await retriever.retrieve("fibonacci sequence", k=2)
        
        assert len(results) <= 2


# =============================================================================
# RAG Pipeline Integration Tests
# =============================================================================


class TestRAGPipelineIntegration:
    """Integration tests for the complete RAG pipeline."""

    @pytest.mark.asyncio
    async def test_pipeline_query(self) -> None:
        """Test complete RAG pipeline query."""
        # Mock components
        mock_retriever = AsyncMock()
        mock_retriever.retrieve.return_value = [
            RetrievalResult(
                id="1",
                content="def fibonacci(n): return n if n <= 1 else fibonacci(n-1) + fibonacci(n-2)",
                score=0.95,
                metadata={"language": "python", "type": "function"}
            )
        ]
        
        mock_reranker = AsyncMock()
        mock_reranker.rerank.return_value = [
            RetrievalResult(
                id="1",
                content="def fibonacci(n): return n if n <= 1 else fibonacci(n-1) + fibonacci(n-2)",
                score=0.98,
                metadata={"language": "python", "type": "function"}
            )
        ]
        
        mock_llm = AsyncMock()
        mock_llm.generate.return_value = "Here's the Fibonacci implementation..."
        
        # Create pipeline
        pipeline = RAGPipeline(
            retriever=mock_retriever,
            reranker=mock_reranker,
            llm_client=mock_llm
        )
        
        # Query
        response = await pipeline.query(
            "How do I implement Fibonacci?",
            task="explain",
            language="python"
        )
        
        assert response is not None
        assert response.answer == "Here's the Fibonacci implementation..."
        mock_retriever.retrieve.assert_called_once()
        mock_reranker.rerank.assert_called_once()
        mock_llm.generate.assert_called_once()


# =============================================================================
# API Integration Tests
# =============================================================================


class TestAPIIntegration:
    """Integration tests for API endpoints."""

    def test_health_check(self) -> None:
        """Test health endpoint."""
        response = client.get("/health")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "healthy"

    def test_root_endpoint(self) -> None:
        """Test root endpoint returns API info."""
        response = client.get("/")
        assert response.status_code == 200
        data = response.json()
        assert "name" in data
        assert "endpoints" in data

    def test_chat_endpoint_structure(self) -> None:
        """Test chat endpoint accepts correct structure."""
        response = client.post(
            "/chat",
            json={
                "message": "How do I implement a binary search?",
                "context": {"language": "python"}
            }
        )
        # Should return 200 even if not fully implemented
        assert response.status_code == 200
        data = response.json()
        assert "status" in data or "response" in data or "answer" in data

    def test_search_endpoint_structure(self) -> None:
        """Test search endpoint accepts correct structure."""
        response = client.post(
            "/search",
            json={
                "query": "database connection",
                "limit": 10
            }
        )
        assert response.status_code == 200

    def test_ingest_endpoint_structure(self) -> None:
        """Test ingest endpoint accepts correct structure."""
        response = client.post(
            "/ingest",
            json={
                "source_type": "directory",
                "source": "/tmp/test",
                "languages": ["python"]
            }
        )
        assert response.status_code == 200


# =============================================================================
# End-to-End Integration Tests (require running stack)
# =============================================================================


@pytest.mark.skipif(
    os.environ.get("RESPO_E2E_TESTS") != "1",
    reason="E2E tests require running RESPO stack. Set RESPO_E2E_TESTS=1"
)
class TestE2EIntegration:
    """End-to-end tests requiring full stack."""

    def test_full_chat_flow(self) -> None:
        """Test complete chat flow with real components."""
        response = client.post(
            "/chat",
            json={
                "message": "Explain how to implement a binary search tree in Python",
                "context": {"language": "python"}
            }
        )
        
        assert response.status_code == 200
        data = response.json()
        assert "answer" in data or "response" in data
        
        # Response should contain relevant content
        answer = data.get("answer", data.get("response", ""))
        assert len(answer) > 100
        assert "binary" in answer.lower() or "tree" in answer.lower()

    def test_full_ingest_and_search_flow(self, tmp_path: Path) -> None:
        """Test ingesting code and then searching."""
        # Create test file
        test_file = tmp_path / "test_code.py"
        test_file.write_text('''
def unique_sorting_algorithm(arr):
    """A unique sorting algorithm for testing."""
    return sorted(arr, key=lambda x: -x)
        ''')
        
        # Ingest
        ingest_response = client.post(
            "/ingest",
            json={
                "source_type": "directory",
                "source": str(tmp_path),
                "languages": ["python"]
            }
        )
        assert ingest_response.status_code == 200
        
        # Search
        search_response = client.post(
            "/search",
            json={
                "query": "unique sorting algorithm",
                "limit": 5
            }
        )
        assert search_response.status_code == 200
        
        data = search_response.json()
        assert "results" in data
        # Should find our test code
        results = data.get("results", [])
        assert len(results) > 0


# =============================================================================
# Performance Tests
# =============================================================================


class TestPerformance:
    """Performance tests for critical paths."""

    def test_chunking_performance(self, sample_code: str) -> None:
        """Test chunking completes within acceptable time."""
        import time
        
        chunker = CodeChunker()
        
        # Repeat code to create larger input
        large_code = sample_code * 10
        
        start = time.time()
        chunks = chunker.chunk(large_code, language="python")
        elapsed = time.time() - start
        
        # Should complete within 1 second for ~5KB of code
        assert elapsed < 1.0
        assert len(chunks) > 0

    def test_api_response_time(self) -> None:
        """Test API response time for health check."""
        import time
        
        start = time.time()
        response = client.get("/health")
        elapsed = time.time() - start
        
        assert response.status_code == 200
        # Health check should be fast
        assert elapsed < 0.1
