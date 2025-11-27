"""
Tests for ThemisDB integration and Graph Analysis
"""

import pytest
from unittest.mock import AsyncMock, MagicMock, patch

from respo.graph.analyzer import (
    CodeGraph,
    CodeGraphAnalyzer,
    GraphEdge,
    GraphNode,
    PythonGraphAnalyzer,
    JavaScriptGraphAnalyzer,
)


# =============================================================================
# Python Graph Analyzer Tests
# =============================================================================


class TestPythonGraphAnalyzer:
    """Tests for Python code graph analysis."""

    def test_extract_imports(self) -> None:
        """Test extraction of import statements."""
        source = '''
import os
import sys
from pathlib import Path
from typing import List, Optional
from mypackage.module import MyClass
'''
        analyzer = PythonGraphAnalyzer("test.py", "test")
        graph = analyzer.analyze(source)
        
        # Should have module node
        modules = [n for n in graph.nodes if n.node_type == "module"]
        assert len(modules) == 1
        
        # Should have import edges
        import_edges = [e for e in graph.edges if e.edge_type == "imports"]
        assert len(import_edges) >= 4
        
        # Check specific imports
        imported = [e.target_id for e in import_edges]
        assert "os" in imported
        assert "sys" in imported
        assert "pathlib.Path" in imported

    def test_extract_function_definitions(self) -> None:
        """Test extraction of function definitions."""
        source = '''
def simple_function():
    pass

async def async_function(arg1: int, arg2: str) -> bool:
    """Docstring here."""
    return True

def decorated_function():
    @decorator
    def inner():
        pass
'''
        analyzer = PythonGraphAnalyzer("test.py", "test")
        graph = analyzer.analyze(source)
        
        functions = [n for n in graph.nodes if n.node_type == "function"]
        assert len(functions) >= 2
        
        func_names = [f.name for f in functions]
        assert "simple_function" in func_names
        assert "async_function" in func_names

    def test_extract_class_definitions(self) -> None:
        """Test extraction of class definitions and inheritance."""
        source = '''
class BaseClass:
    pass

class ChildClass(BaseClass):
    def method(self):
        pass

class MultiInherit(BaseClass, Mixin):
    pass
'''
        analyzer = PythonGraphAnalyzer("test.py", "test")
        graph = analyzer.analyze(source)
        
        classes = [n for n in graph.nodes if n.node_type == "class"]
        assert len(classes) == 3
        
        # Check inheritance edges
        inherits = [e for e in graph.edges if e.edge_type == "inherits"]
        assert len(inherits) >= 2

    def test_extract_function_calls(self) -> None:
        """Test extraction of function call relationships."""
        source = '''
def caller():
    callee()
    another_function(arg)
    
def callee():
    pass

def another_function(x):
    pass
'''
        analyzer = PythonGraphAnalyzer("test.py", "test")
        graph = analyzer.analyze(source)
        
        calls = [e for e in graph.edges if e.edge_type == "calls"]
        # caller should call callee and another_function
        assert len(calls) >= 2

    def test_qualified_names(self) -> None:
        """Test that qualified names are correctly generated."""
        source = '''
class MyClass:
    def my_method(self):
        pass
'''
        analyzer = PythonGraphAnalyzer("mymodule.py", "mymodule")
        graph = analyzer.analyze(source)
        
        method = next((n for n in graph.nodes if n.name == "my_method"), None)
        assert method is not None
        assert "mymodule.MyClass.my_method" in method.qualified_name


# =============================================================================
# JavaScript Graph Analyzer Tests
# =============================================================================


class TestJavaScriptGraphAnalyzer:
    """Tests for JavaScript/TypeScript code graph analysis."""

    def test_extract_es6_imports(self) -> None:
        """Test extraction of ES6 import statements."""
        source = '''
import React from 'react';
import { useState, useEffect } from 'react';
import * as utils from './utils';
import type { Props } from './types';
'''
        analyzer = JavaScriptGraphAnalyzer("test.ts", "test")
        graph = analyzer.analyze(source)
        
        import_edges = [e for e in graph.edges if e.edge_type == "imports"]
        assert len(import_edges) >= 3

    def test_extract_commonjs_imports(self) -> None:
        """Test extraction of CommonJS require statements."""
        source = '''
const express = require('express');
const { Router } = require('express');
let path = require('path');
'''
        analyzer = JavaScriptGraphAnalyzer("test.js", "test")
        graph = analyzer.analyze(source)
        
        import_edges = [e for e in graph.edges if e.edge_type == "imports"]
        assert len(import_edges) >= 2

    def test_extract_class_definitions(self) -> None:
        """Test extraction of class definitions."""
        source = '''
class BaseComponent {
    constructor() {}
}

export class MyComponent extends BaseComponent implements IComponent {
    render() {}
}
'''
        analyzer = JavaScriptGraphAnalyzer("test.ts", "test")
        graph = analyzer.analyze(source)
        
        classes = [n for n in graph.nodes if n.node_type == "class"]
        assert len(classes) == 2
        
        inherits = [e for e in graph.edges if e.edge_type == "inherits"]
        assert len(inherits) >= 1

    def test_extract_functions(self) -> None:
        """Test extraction of function definitions."""
        source = '''
function regularFunction() {}

export async function asyncFunction() {}

const arrowFunction = () => {};

const asyncArrow = async (x) => x * 2;
'''
        analyzer = JavaScriptGraphAnalyzer("test.js", "test")
        graph = analyzer.analyze(source)
        
        functions = [n for n in graph.nodes if n.node_type == "function"]
        assert len(functions) >= 2


# =============================================================================
# CodeGraphAnalyzer Tests
# =============================================================================


class TestCodeGraphAnalyzer:
    """Tests for the unified code graph analyzer."""

    def test_python_analysis(self) -> None:
        """Test analyzing Python code."""
        analyzer = CodeGraphAnalyzer()
        
        source = '''
def hello():
    print("Hello")
'''
        graph = analyzer.analyze(source, "python", "hello.py")
        
        assert len(graph.nodes) >= 1
        assert graph.nodes[0].node_type == "module"

    def test_javascript_analysis(self) -> None:
        """Test analyzing JavaScript code."""
        analyzer = CodeGraphAnalyzer()
        
        source = '''
function greet(name) {
    console.log("Hello " + name);
}
'''
        graph = analyzer.analyze(source, "javascript", "greet.js")
        
        assert len(graph.nodes) >= 1

    def test_unsupported_language(self) -> None:
        """Test handling of unsupported language."""
        analyzer = CodeGraphAnalyzer()
        
        source = "print('hello')"
        graph = analyzer.analyze(source, "unknown", "test.unknown")
        
        # Should return minimal graph
        assert len(graph.nodes) == 1
        assert graph.nodes[0].node_type == "module"

    def test_graph_to_dict(self) -> None:
        """Test graph serialization."""
        graph = CodeGraph(
            nodes=[
                GraphNode(
                    id="test.func",
                    node_type="function",
                    name="func",
                    qualified_name="test.func",
                    file_path="test.py",
                    line_start=1,
                    line_end=3,
                )
            ],
            edges=[
                GraphEdge(
                    source_id="test.func",
                    target_id="os.path",
                    edge_type="imports",
                )
            ],
        )
        
        data = graph.to_dict()
        
        assert "nodes" in data
        assert "edges" in data
        assert len(data["nodes"]) == 1
        assert len(data["edges"]) == 1
        assert data["nodes"][0]["type"] == "function"
        assert data["edges"][0]["type"] == "imports"


# =============================================================================
# ThemisDB Vector Store Tests (Mocked)
# =============================================================================


class TestThemisVectorStore:
    """Tests for ThemisDB vector store (mocked)."""

    @pytest.mark.asyncio
    async def test_add_documents_with_graph(self) -> None:
        """Test adding documents with graph edges."""
        from respo.vectorstore.themis import ThemisVectorStore, ThemisConfig
        
        with patch("httpx.AsyncClient") as mock_client_class:
            mock_client = AsyncMock()
            mock_client_class.return_value = mock_client
            
            # Mock responses
            mock_response = MagicMock()
            mock_response.raise_for_status = MagicMock()
            mock_response.json.return_value = {"success": True}
            mock_client.post.return_value = mock_response
            mock_client.get.return_value = mock_response
            
            store = ThemisVectorStore(ThemisConfig(
                url="http://localhost:8765",
                enable_graph=True,
            ))
            store._client = mock_client
            store._initialized = True
            
            # Add documents with graph edges
            await store.add(
                ids=["test:1-5"],
                embeddings=[[0.1] * 768],
                documents=["def test(): pass"],
                metadatas=[{
                    "language": "python",
                    "graph_edges": [
                        {
                            "source": "test:1-5",
                            "target": "os.path",
                            "type": "imports",
                        }
                    ]
                }],
            )
            
            # Verify POST was called for documents
            assert mock_client.post.called

    @pytest.mark.asyncio
    async def test_hybrid_search(self) -> None:
        """Test hybrid search with graph expansion."""
        from respo.vectorstore.themis import ThemisVectorStore, ThemisConfig
        
        with patch("httpx.AsyncClient") as mock_client_class:
            mock_client = AsyncMock()
            mock_client_class.return_value = mock_client
            
            mock_response = MagicMock()
            mock_response.raise_for_status = MagicMock()
            mock_response.json.return_value = {
                "results": [
                    {
                        "id": "test:1-5",
                        "content": "def test(): pass",
                        "fused_score": 0.95,
                        "vector_score": 0.9,
                        "keyword_score": 0.8,
                        "graph_score": 0.7,
                        "metadata": {"language": "python"},
                    }
                ]
            }
            mock_client.post.return_value = mock_response
            
            store = ThemisVectorStore(ThemisConfig())
            store._client = mock_client
            store._initialized = True
            
            results = await store.hybrid_search(
                query_embedding=[0.1] * 768,
                query_text="test function",
                k=10,
                expand_graph=True,
            )
            
            assert len(results) == 1
            assert results[0].id == "test:1-5"
            assert results[0].metadata["_vector_score"] == 0.9

    @pytest.mark.asyncio
    async def test_graph_traverse(self) -> None:
        """Test graph traversal."""
        from respo.vectorstore.themis import ThemisVectorStore, ThemisConfig
        
        with patch("httpx.AsyncClient") as mock_client_class:
            mock_client = AsyncMock()
            mock_client_class.return_value = mock_client
            
            mock_response = MagicMock()
            mock_response.raise_for_status = MagicMock()
            mock_response.json.return_value = {
                "nodes": [
                    {"id": "callee1", "edge_type": "calls"},
                    {"id": "callee2", "edge_type": "calls"},
                ]
            }
            mock_client.post.return_value = mock_response
            
            store = ThemisVectorStore(ThemisConfig())
            store._client = mock_client
            store._initialized = True
            
            nodes = await store.graph_traverse(
                start_id="test.main",
                edge_types=["calls"],
                direction="outgoing",
                depth=2,
            )
            
            assert len(nodes) == 2
            assert nodes[0]["id"] == "callee1"


# =============================================================================
# Ingestion Pipeline with Graph Tests
# =============================================================================


class TestIngestionWithGraph:
    """Tests for ingestion pipeline with graph extraction."""

    @pytest.mark.asyncio
    async def test_ingest_extracts_graph(self) -> None:
        """Test that ingestion extracts graph when using ThemisDB."""
        from respo.ingestion.pipeline import IngestionPipeline, IngestionConfig
        from respo.vectorstore.themis import ThemisVectorStore
        
        # Mock ThemisDB
        mock_store = MagicMock(spec=ThemisVectorStore)
        mock_store.add = AsyncMock()
        
        # Mock embedder
        mock_embedder = MagicMock()
        mock_embedder.embed.return_value = [[0.1] * 768]
        
        config = IngestionConfig(enable_graph=True)
        pipeline = IngestionPipeline(
            vector_store=mock_store,
            embedder=mock_embedder,
            config=config,
        )
        pipeline._is_themis = True  # Force ThemisDB mode
        
        source = '''
import os
def my_function():
    os.path.exists("test")
'''
        
        stats = await pipeline.ingest_file(
            content=source,
            path="test.py",
            language="python",
        )
        
        assert stats.files_processed == 1
        assert stats.graph_nodes >= 1
        assert stats.graph_edges >= 1
        
        # Verify store.add was called with graph edges
        mock_store.add.assert_called_once()
        call_args = mock_store.add.call_args
        metadatas = call_args.kwargs.get("metadatas", call_args[0][3] if len(call_args[0]) > 3 else None)
        
        # At least one metadata should have graph_edges
        assert any("graph_edges" in m for m in (metadatas or []) if m)
