"""
Base classes for source code scrapers.

Defines the common interface for all source scrapers.
"""

import hashlib
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import Any, AsyncIterator, Optional


class SourceType(Enum):
    """Supported source types."""

    GITHUB = "github"
    GITLAB = "gitlab"
    BITBUCKET = "bitbucket"
    DEBIAN = "debian"
    PYPI = "pypi"
    NPM = "npm"
    CRATES = "crates"
    KERNEL = "kernel"
    GNU = "gnu"
    APACHE = "apache"
    LOCAL = "local"


@dataclass
class SourceConfig:
    """Configuration for source scrapers."""

    # Authentication
    token: Optional[str] = None
    username: Optional[str] = None
    password: Optional[str] = None

    # Rate limiting
    requests_per_second: float = 2.0
    max_retries: int = 3

    # File filtering
    include_extensions: list[str] = field(default_factory=lambda: [
        ".py", ".js", ".ts", ".jsx", ".tsx",
        ".java", ".kt", ".scala",
        ".go", ".rs", ".c", ".cpp", ".h", ".hpp",
        ".rb", ".php", ".cs", ".swift", ".m",
        ".sh", ".bash", ".zsh",
        ".sql", ".graphql",
        ".yaml", ".yml", ".json", ".toml",
        ".md", ".rst", ".txt",
    ])
    exclude_patterns: list[str] = field(default_factory=lambda: [
        "**/node_modules/**",
        "**/vendor/**",
        "**/.git/**",
        "**/dist/**",
        "**/build/**",
        "**/__pycache__/**",
        "**/venv/**",
        "**/.venv/**",
    ])

    # Limits
    max_file_size_kb: int = 500
    max_files_per_repo: int = 10000

    # Clone settings
    clone_depth: int = 1
    temp_dir: Optional[Path] = None


@dataclass
class SourceFile:
    """Represents a source code file from any platform."""

    # Identification
    id: str
    path: str
    source_type: SourceType

    # Content
    content: str
    size_bytes: int

    # Metadata
    language: str
    repo: str
    version: Optional[str] = None
    branch: Optional[str] = None
    commit_sha: Optional[str] = None

    # Licensing
    license: Optional[str] = None

    # Timestamps
    last_modified: Optional[datetime] = None

    # Additional metadata
    metadata: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def generate_id(cls, source_type: SourceType, repo: str, path: str) -> str:
        """Generate a unique ID for a source file."""
        key = f"{source_type.value}:{repo}:{path}"
        return hashlib.sha256(key.encode()).hexdigest()[:16]


class CodeSource(ABC):
    """Abstract base class for source code scrapers."""

    source_type: SourceType

    def __init__(self, config: Optional[SourceConfig] = None):
        """Initialize the scraper with configuration."""
        self.config = config or SourceConfig()

    @abstractmethod
    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape source code from a repository or package.

        Args:
            identifier: Repository or package identifier (e.g., "owner/repo", "package-name")
            version: Optional version/branch/tag to scrape

        Yields:
            SourceFile objects for each code file
        """
        pass

    @abstractmethod
    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """
        Search for repositories or packages.

        Args:
            query: Search query
            language: Filter by programming language
            limit: Maximum results to return

        Returns:
            List of search results with metadata
        """
        pass

    @abstractmethod
    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """
        Get metadata for a repository or package.

        Args:
            identifier: Repository or package identifier

        Returns:
            Metadata dictionary
        """
        pass

    async def __aenter__(self):
        """Async context manager entry."""
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """Async context manager exit."""
        await self.close()

    async def close(self) -> None:
        """Cleanup resources."""
        pass

    def _should_include_file(self, path: str, size_bytes: int) -> bool:
        """Check if a file should be included based on config."""
        from fnmatch import fnmatch

        # Check size
        if size_bytes > self.config.max_file_size_kb * 1024:
            return False

        # Check extension
        ext = Path(path).suffix.lower()
        if ext not in self.config.include_extensions:
            return False

        # Check exclude patterns
        for pattern in self.config.exclude_patterns:
            if fnmatch(path, pattern):
                return False

        return True

    def _detect_language(self, path: str) -> str:
        """Detect programming language from file extension."""
        ext = Path(path).suffix.lower()
        language_map = {
            ".py": "python",
            ".js": "javascript",
            ".jsx": "javascript",
            ".ts": "typescript",
            ".tsx": "typescript",
            ".java": "java",
            ".kt": "kotlin",
            ".scala": "scala",
            ".go": "go",
            ".rs": "rust",
            ".c": "c",
            ".h": "c",
            ".cpp": "cpp",
            ".hpp": "cpp",
            ".cc": "cpp",
            ".cxx": "cpp",
            ".rb": "ruby",
            ".php": "php",
            ".cs": "csharp",
            ".swift": "swift",
            ".m": "objective-c",
            ".sh": "shell",
            ".bash": "shell",
            ".zsh": "shell",
            ".sql": "sql",
            ".graphql": "graphql",
            ".yaml": "yaml",
            ".yml": "yaml",
            ".json": "json",
            ".toml": "toml",
            ".md": "markdown",
            ".rst": "rst",
            ".txt": "text",
        }
        return language_map.get(ext, "unknown")
