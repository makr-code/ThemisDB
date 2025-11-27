"""
Tests for Code Chunker.
"""

import pytest

from respo.ingestion.chunker import CodeChunker


class TestCodeChunker:
    """Tests for CodeChunker class."""

    def test_python_function_chunking(self) -> None:
        """Test Python function extraction."""
        code = '''def hello():
    """Say hello."""
    print("Hello")

def world():
    print("World")
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "python", strategy="function")

        assert len(chunks) == 2
        assert chunks[0].name == "hello"
        assert chunks[0].chunk_type == "function"
        assert chunks[1].name == "world"

    def test_python_class_chunking(self) -> None:
        """Test Python class extraction."""
        code = '''class MyClass:
    """A class."""

    def method(self):
        pass

class OtherClass:
    pass
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "python", strategy="class")

        assert len(chunks) == 2
        assert chunks[0].name == "MyClass"
        assert chunks[0].chunk_type == "class"

    def test_sliding_window_chunking(self) -> None:
        """Test sliding window fallback."""
        code = "line1\n" * 100  # Simple repeated content

        chunker = CodeChunker(max_chunk_size=200, min_chunk_size=50)
        chunks = chunker.chunk(code, "unknown", strategy="sliding")

        assert len(chunks) > 1
        for chunk in chunks:
            assert chunk.chunk_type == "block"

    def test_async_function_detection(self) -> None:
        """Test async function detection in Python."""
        code = '''async def fetch_data():
    await something()
    return data
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "python", strategy="function")

        assert len(chunks) == 1
        assert chunks[0].name == "fetch_data"

    def test_javascript_function_chunking(self) -> None:
        """Test JavaScript function extraction."""
        code = '''function hello() {
    console.log("Hello");
}

function world() {
    console.log("World");
}
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "javascript", strategy="function")

        assert len(chunks) == 2
        assert chunks[0].name == "hello"
        assert chunks[1].name == "world"

    def test_empty_code(self) -> None:
        """Test empty code handling."""
        chunker = CodeChunker()
        chunks = chunker.chunk("", "python")

        assert len(chunks) == 0

    def test_docstring_extraction(self) -> None:
        """Test docstring extraction from Python functions."""
        code = '''def documented():
    """This is the docstring."""
    pass
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "python", strategy="function")

        assert len(chunks) == 1
        assert chunks[0].docstring == "This is the docstring."

    def test_chunk_metadata(self) -> None:
        """Test that chunks have correct metadata."""
        code = '''def test():
    pass
'''
        chunker = CodeChunker()
        chunks = chunker.chunk(code, "python", strategy="function")

        assert len(chunks) == 1
        chunk = chunks[0]
        assert chunk.start_line == 1
        assert chunk.language == "python"
        assert chunk.signature is not None
