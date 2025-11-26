"""
Tests for RAG Pipeline components.
"""

import pytest

from respo.rag.prompts import format_context, get_system_prompt
from respo.rag.reranker import NoOpReranker


class TestPrompts:
    """Tests for prompt templates."""

    def test_get_system_prompt_chat(self) -> None:
        """Test chat system prompt."""
        prompt = get_system_prompt("chat", "test context")
        assert "test context" in prompt
        assert "Software-Entwickler" in prompt

    def test_get_system_prompt_explain(self) -> None:
        """Test explain system prompt."""
        prompt = get_system_prompt("explain", "code context")
        assert "code context" in prompt
        assert "Erkläre" in prompt

    def test_get_system_prompt_review(self) -> None:
        """Test review system prompt."""
        prompt = get_system_prompt("review", "")
        assert "Code-Reviewer" in prompt
        assert "Bugs" in prompt

    def test_get_system_prompt_unknown_task(self) -> None:
        """Test unknown task falls back to chat."""
        prompt = get_system_prompt("unknown_task", "context")
        assert "context" in prompt

    def test_format_context_empty(self) -> None:
        """Test empty context formatting."""
        result = format_context([])
        assert "Kein relevanter Kontext" in result

    def test_format_context_single_doc(self) -> None:
        """Test single document context."""
        docs = [{"content": "def hello(): pass", "metadata": {"path": "test.py"}}]
        result = format_context(docs)
        assert "def hello()" in result
        assert "test.py" in result

    def test_format_context_multiple_docs(self) -> None:
        """Test multiple documents context."""
        docs = [
            {"content": "code1", "metadata": {"path": "a.py"}},
            {"content": "code2", "metadata": {"path": "b.py"}},
        ]
        result = format_context(docs)
        assert "code1" in result
        assert "code2" in result
        assert "Dokument 1" in result
        assert "Dokument 2" in result


class TestNoOpReranker:
    """Tests for NoOpReranker."""

    def test_rerank_passthrough(self) -> None:
        """Test that NoOpReranker passes through documents."""
        reranker = NoOpReranker()
        docs = [
            {"id": "1", "content": "a", "score": 0.9, "metadata": {}},
            {"id": "2", "content": "b", "score": 0.8, "metadata": {}},
        ]

        results = reranker.rerank("query", docs, top_k=2)

        assert len(results) == 2
        assert results[0].id == "1"
        assert results[0].score == 0.9

    def test_rerank_top_k(self) -> None:
        """Test top_k limiting."""
        reranker = NoOpReranker()
        docs = [
            {"id": str(i), "content": f"doc{i}", "score": 0.9 - i * 0.1, "metadata": {}}
            for i in range(5)
        ]

        results = reranker.rerank("query", docs, top_k=2)
        assert len(results) == 2
