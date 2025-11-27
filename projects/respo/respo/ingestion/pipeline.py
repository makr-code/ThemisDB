"""
RESPO Ingestion Pipeline

Complete pipeline for ingesting code repositories into the vector store.
"""

import asyncio
from dataclasses import dataclass
from pathlib import Path
from typing import Any, AsyncIterator, Optional

import structlog

from respo.embedding import CodeEmbedder
from respo.ingestion.chunker import CodeChunk, CodeChunker
from respo.ingestion.github_scraper import CodeFile, GitHubScraper, ScraperConfig
from respo.vectorstore.base import VectorStoreBase

logger = structlog.get_logger(__name__)


@dataclass
class IngestionStats:
    """Statistics from ingestion."""

    files_processed: int = 0
    chunks_created: int = 0
    chunks_indexed: int = 0
    errors: int = 0
    total_size_bytes: int = 0


@dataclass
class IngestionConfig:
    """Configuration for ingestion pipeline."""

    # Chunking
    chunk_strategy: str = "semantic"
    max_chunk_size: int = 2000
    min_chunk_size: int = 100

    # Batching
    batch_size: int = 100
    max_concurrent: int = 4

    # Filtering
    include_languages: Optional[list[str]] = None
    exclude_patterns: list[str] = None  # type: ignore

    def __post_init__(self) -> None:
        if self.exclude_patterns is None:
            self.exclude_patterns = ["**/test*", "**/spec*", "**/__pycache__/*"]


class IngestionPipeline:
    """
    Pipeline for ingesting code into the vector store.

    Steps:
    1. Parse/scrape source code files
    2. Chunk code into semantic units
    3. Generate embeddings
    4. Store in vector database
    """

    def __init__(
        self,
        vector_store: VectorStoreBase,
        embedder: CodeEmbedder,
        config: Optional[IngestionConfig] = None,
    ) -> None:
        """
        Initialize ingestion pipeline.

        Args:
            vector_store: Vector store backend
            embedder: Code embedder
            config: Ingestion configuration
        """
        self.vector_store = vector_store
        self.embedder = embedder
        self.config = config or IngestionConfig()
        self.chunker = CodeChunker(
            max_chunk_size=self.config.max_chunk_size,
            min_chunk_size=self.config.min_chunk_size,
        )

    async def ingest_file(
        self,
        content: str,
        path: str,
        language: str,
        metadata: Optional[dict[str, Any]] = None,
    ) -> IngestionStats:
        """
        Ingest a single file.

        Args:
            content: File content
            path: File path
            language: Programming language
            metadata: Additional metadata

        Returns:
            Ingestion statistics
        """
        stats = IngestionStats()
        stats.files_processed = 1
        stats.total_size_bytes = len(content.encode("utf-8"))

        try:
            # Chunk the code
            chunks = self.chunker.chunk(
                content,
                language,
                strategy=self.config.chunk_strategy,
            )
            stats.chunks_created = len(chunks)

            if not chunks:
                logger.warning("No chunks created", path=path)
                return stats

            # Prepare for indexing
            ids = []
            documents = []
            metadatas = []

            for i, chunk in enumerate(chunks):
                chunk_id = f"{path}:{chunk.start_line}-{chunk.end_line}"
                ids.append(chunk_id)
                documents.append(chunk.content)

                chunk_metadata = {
                    "path": path,
                    "language": language,
                    "chunk_type": chunk.chunk_type,
                    "start_line": chunk.start_line,
                    "end_line": chunk.end_line,
                    "name": chunk.name,
                    **(metadata or {}),
                }
                metadatas.append(chunk_metadata)

            # Generate embeddings
            embeddings = self.embedder.embed(documents)

            # Store in vector database
            await self.vector_store.add(
                ids=ids,
                embeddings=embeddings,
                documents=documents,
                metadatas=metadatas,
            )
            stats.chunks_indexed = len(chunks)

            logger.debug(
                "File ingested",
                path=path,
                chunks=len(chunks),
            )

        except Exception as e:
            stats.errors = 1
            logger.error("Ingestion error", path=path, error=str(e))

        return stats

    async def ingest_directory(
        self,
        directory: Path,
        repo_name: Optional[str] = None,
    ) -> IngestionStats:
        """
        Ingest all code files from a directory.

        Args:
            directory: Directory path
            repo_name: Optional repository name for metadata

        Returns:
            Ingestion statistics
        """
        total_stats = IngestionStats()

        # Language detection by extension
        extension_map = {
            ".py": "python",
            ".pyi": "python",
            ".js": "javascript",
            ".jsx": "javascript",
            ".ts": "typescript",
            ".tsx": "typescript",
            ".go": "go",
            ".rs": "rust",
            ".java": "java",
            ".cpp": "cpp",
            ".c": "c",
            ".h": "c",
            ".rb": "ruby",
            ".php": "php",
        }

        # Collect files
        files_to_process = []
        for ext, lang in extension_map.items():
            if self.config.include_languages:
                if lang not in self.config.include_languages:
                    continue

            for file_path in directory.rglob(f"*{ext}"):
                # Skip excluded patterns
                skip = False
                for pattern in self.config.exclude_patterns:
                    if file_path.match(pattern):
                        skip = True
                        break
                if not skip:
                    files_to_process.append((file_path, lang))

        logger.info("Found files to process", count=len(files_to_process))

        # Process files in batches
        batch = []
        for file_path, language in files_to_process:
            try:
                content = file_path.read_text(encoding="utf-8")
                relative_path = str(file_path.relative_to(directory))

                metadata = {"repo": repo_name} if repo_name else {}

                batch.append((content, relative_path, language, metadata))

                if len(batch) >= self.config.batch_size:
                    batch_stats = await self._process_batch(batch)
                    total_stats = self._merge_stats(total_stats, batch_stats)
                    batch = []

            except (UnicodeDecodeError, IOError) as e:
                logger.warning("Skipping unreadable file", path=str(file_path), error=str(e))
                total_stats.errors += 1

        # Process remaining files
        if batch:
            batch_stats = await self._process_batch(batch)
            total_stats = self._merge_stats(total_stats, batch_stats)

        logger.info(
            "Directory ingestion complete",
            files=total_stats.files_processed,
            chunks=total_stats.chunks_indexed,
            errors=total_stats.errors,
        )

        return total_stats

    async def ingest_github_repo(
        self,
        owner: str,
        repo: str,
        branch: Optional[str] = None,
        github_token: Optional[str] = None,
    ) -> IngestionStats:
        """
        Ingest a GitHub repository.

        Args:
            owner: Repository owner
            repo: Repository name
            branch: Branch to ingest
            github_token: GitHub token for private repos

        Returns:
            Ingestion statistics
        """
        total_stats = IngestionStats()

        scraper_config = ScraperConfig(github_token=github_token)

        async with GitHubScraper(scraper_config) as scraper:
            async for code_file in scraper.scrape_repository(
                owner=owner,
                repo=repo,
                branch=branch,
            ):
                # Filter by language if configured
                if self.config.include_languages:
                    if code_file.language not in self.config.include_languages:
                        continue

                metadata = {
                    "repo": f"{owner}/{repo}",
                    "branch": code_file.branch,
                    "commit_sha": code_file.commit_sha,
                    "license": code_file.license,
                }

                stats = await self.ingest_file(
                    content=code_file.content,
                    path=code_file.path,
                    language=code_file.language,
                    metadata=metadata,
                )

                total_stats = self._merge_stats(total_stats, stats)

        logger.info(
            "GitHub repo ingestion complete",
            repo=f"{owner}/{repo}",
            files=total_stats.files_processed,
            chunks=total_stats.chunks_indexed,
        )

        return total_stats

    async def _process_batch(
        self,
        batch: list[tuple[str, str, str, dict]],
    ) -> IngestionStats:
        """Process a batch of files concurrently."""
        total_stats = IngestionStats()

        # Create tasks
        tasks = [
            self.ingest_file(content, path, language, metadata)
            for content, path, language, metadata in batch
        ]

        # Run with concurrency limit
        semaphore = asyncio.Semaphore(self.config.max_concurrent)

        async def limited_task(task):
            async with semaphore:
                return await task

        results = await asyncio.gather(
            *[limited_task(task) for task in tasks],
            return_exceptions=True,
        )

        for result in results:
            if isinstance(result, Exception):
                total_stats.errors += 1
                logger.error("Batch processing error", error=str(result))
            else:
                total_stats = self._merge_stats(total_stats, result)

        return total_stats

    def _merge_stats(self, a: IngestionStats, b: IngestionStats) -> IngestionStats:
        """Merge two ingestion stats."""
        return IngestionStats(
            files_processed=a.files_processed + b.files_processed,
            chunks_created=a.chunks_created + b.chunks_created,
            chunks_indexed=a.chunks_indexed + b.chunks_indexed,
            errors=a.errors + b.errors,
            total_size_bytes=a.total_size_bytes + b.total_size_bytes,
        )
