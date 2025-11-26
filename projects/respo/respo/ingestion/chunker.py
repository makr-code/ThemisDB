"""
RESPO Code Chunker

Intelligent code chunking using AST-based parsing.
"""

import re
from dataclasses import dataclass
from typing import Optional


@dataclass
class CodeChunk:
    """A chunk of source code."""

    content: str
    chunk_type: str  # 'function', 'class', 'method', 'block'
    name: Optional[str]
    start_line: int
    end_line: int
    language: str
    signature: Optional[str] = None
    docstring: Optional[str] = None
    imports: list[str] = None  # type: ignore

    def __post_init__(self) -> None:
        if self.imports is None:
            self.imports = []


class CodeChunker:
    """
    Intelligent code chunker with language-aware parsing.

    Supports:
    - Function-level chunking
    - Class-level chunking
    - Semantic chunking (preserves logical units)
    - Sliding window (fallback)
    """

    # Regex patterns for common languages
    PATTERNS = {
        "python": {
            "function": r"^(async\s+)?def\s+(\w+)\s*\([^)]*\)\s*(?:->.*?)?:",
            "class": r"^class\s+(\w+)(?:\([^)]*\))?:",
            "import": r"^(?:from\s+\S+\s+)?import\s+.+",
        },
        "javascript": {
            "function": r"(?:async\s+)?function\s+(\w+)\s*\([^)]*\)",
            "class": r"class\s+(\w+)(?:\s+extends\s+\w+)?",
            "arrow": r"(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s+)?\([^)]*\)\s*=>",
            "import": r"^import\s+.+",
        },
        "typescript": {
            "function": r"(?:async\s+)?function\s+(\w+)\s*(?:<[^>]*>)?\s*\([^)]*\)",
            "class": r"class\s+(\w+)(?:<[^>]*>)?(?:\s+extends\s+\w+)?",
            "interface": r"interface\s+(\w+)(?:<[^>]*>)?",
            "type": r"type\s+(\w+)(?:<[^>]*>)?\s*=",
            "import": r"^import\s+.+",
        },
    }

    def __init__(
        self,
        max_chunk_size: int = 2000,
        min_chunk_size: int = 100,
        overlap: int = 50,
    ) -> None:
        """
        Initialize chunker.

        Args:
            max_chunk_size: Maximum chunk size in characters
            min_chunk_size: Minimum chunk size
            overlap: Overlap between chunks for sliding window
        """
        self.max_chunk_size = max_chunk_size
        self.min_chunk_size = min_chunk_size
        self.overlap = overlap

    def chunk(
        self,
        code: str,
        language: str,
        strategy: str = "semantic",
    ) -> list[CodeChunk]:
        """
        Chunk source code.

        Args:
            code: Source code
            language: Programming language
            strategy: 'semantic', 'function', 'class', or 'sliding'

        Returns:
            List of code chunks
        """
        language = language.lower()

        if strategy == "sliding":
            return self._sliding_window_chunk(code, language)
        elif strategy == "function":
            chunks = self._function_chunk(code, language)
        elif strategy == "class":
            chunks = self._class_chunk(code, language)
        else:  # semantic
            chunks = self._semantic_chunk(code, language)

        # Fall back to sliding window if no chunks found
        if not chunks:
            return self._sliding_window_chunk(code, language)

        return chunks

    def _semantic_chunk(self, code: str, language: str) -> list[CodeChunk]:
        """Semantic chunking - preserves logical units."""
        chunks = []

        # First try to get functions and classes
        functions = self._function_chunk(code, language)
        classes = self._class_chunk(code, language)

        chunks.extend(functions)
        chunks.extend(classes)

        # Sort by start line
        chunks.sort(key=lambda c: c.start_line)

        return chunks

    def _function_chunk(self, code: str, language: str) -> list[CodeChunk]:
        """Extract functions as chunks."""
        chunks = []
        patterns = self.PATTERNS.get(language, self.PATTERNS.get("python", {}))
        func_pattern = patterns.get("function")

        if not func_pattern:
            return chunks

        lines = code.split("\n")

        for i, line in enumerate(lines):
            match = re.match(func_pattern, line.strip())
            if match:
                # Find function end
                end_line = self._find_block_end(lines, i, language)
                func_code = "\n".join(lines[i : end_line + 1])

                # Extract function name
                groups = match.groups()
                func_name = groups[-1] if groups else None

                # Extract docstring
                docstring = self._extract_docstring(lines, i + 1, language)

                chunks.append(
                    CodeChunk(
                        content=func_code,
                        chunk_type="function",
                        name=func_name,
                        start_line=i + 1,
                        end_line=end_line + 1,
                        language=language,
                        signature=line.strip(),
                        docstring=docstring,
                    )
                )

        return chunks

    def _class_chunk(self, code: str, language: str) -> list[CodeChunk]:
        """Extract classes as chunks."""
        chunks = []
        patterns = self.PATTERNS.get(language, {})
        class_pattern = patterns.get("class")

        if not class_pattern:
            return chunks

        lines = code.split("\n")

        for i, line in enumerate(lines):
            match = re.match(class_pattern, line.strip())
            if match:
                end_line = self._find_block_end(lines, i, language)
                class_code = "\n".join(lines[i : end_line + 1])

                groups = match.groups()
                class_name = groups[0] if groups else None

                docstring = self._extract_docstring(lines, i + 1, language)

                chunks.append(
                    CodeChunk(
                        content=class_code,
                        chunk_type="class",
                        name=class_name,
                        start_line=i + 1,
                        end_line=end_line + 1,
                        language=language,
                        signature=line.strip(),
                        docstring=docstring,
                    )
                )

        return chunks

    def _sliding_window_chunk(self, code: str, language: str) -> list[CodeChunk]:
        """Fallback sliding window chunking."""
        chunks = []
        lines = code.split("\n")

        current_chunk_lines = []
        current_start = 1
        current_size = 0

        for i, line in enumerate(lines):
            line_size = len(line) + 1  # +1 for newline

            if current_size + line_size > self.max_chunk_size and current_chunk_lines:
                # Save current chunk
                chunks.append(
                    CodeChunk(
                        content="\n".join(current_chunk_lines),
                        chunk_type="block",
                        name=None,
                        start_line=current_start,
                        end_line=current_start + len(current_chunk_lines) - 1,
                        language=language,
                    )
                )

                # Start new chunk with overlap
                overlap_lines = current_chunk_lines[-self.overlap :] if self.overlap > 0 else []
                current_chunk_lines = overlap_lines
                current_start = i + 1 - len(overlap_lines)
                current_size = sum(len(l) + 1 for l in current_chunk_lines)

            current_chunk_lines.append(line)
            current_size += line_size

        # Add final chunk
        if current_chunk_lines and current_size >= self.min_chunk_size:
            chunks.append(
                CodeChunk(
                    content="\n".join(current_chunk_lines),
                    chunk_type="block",
                    name=None,
                    start_line=current_start,
                    end_line=current_start + len(current_chunk_lines) - 1,
                    language=language,
                )
            )

        return chunks

    def _find_block_end(self, lines: list[str], start: int, language: str) -> int:
        """Find the end of a code block (function/class)."""
        if language == "python":
            return self._find_python_block_end(lines, start)
        else:
            return self._find_brace_block_end(lines, start)

    def _find_python_block_end(self, lines: list[str], start: int) -> int:
        """Find end of Python block using indentation."""
        if start >= len(lines):
            return start

        # Get initial indentation
        initial_line = lines[start]
        initial_indent = len(initial_line) - len(initial_line.lstrip())

        for i in range(start + 1, len(lines)):
            line = lines[i]

            # Skip empty lines and comments
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            # Check indentation
            current_indent = len(line) - len(line.lstrip())
            if current_indent <= initial_indent:
                return i - 1

        return len(lines) - 1

    def _find_brace_block_end(self, lines: list[str], start: int) -> int:
        """Find end of brace-delimited block."""
        brace_count = 0
        started = False

        for i in range(start, len(lines)):
            line = lines[i]

            for char in line:
                if char == "{":
                    brace_count += 1
                    started = True
                elif char == "}":
                    brace_count -= 1

            if started and brace_count <= 0:
                return i

        return len(lines) - 1

    def _extract_docstring(
        self, lines: list[str], start: int, language: str
    ) -> Optional[str]:
        """Extract docstring from code."""
        if start >= len(lines):
            return None

        if language == "python":
            # Look for triple-quoted string
            line = lines[start].strip()
            if line.startswith('"""') or line.startswith("'''"):
                quote = line[:3]
                if line.endswith(quote) and len(line) > 6:
                    return line[3:-3]

                # Multi-line docstring
                docstring_lines = [line[3:]]
                for i in range(start + 1, len(lines)):
                    line = lines[i].strip()
                    if line.endswith(quote):
                        docstring_lines.append(line[:-3])
                        return "\n".join(docstring_lines)
                    docstring_lines.append(line)

        return None
