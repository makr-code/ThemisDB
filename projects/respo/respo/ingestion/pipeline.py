"""
RESPO Ingestion Pipeline

Complete pipeline for ingesting code repositories into the vector store.

When using ThemisDB as the backend, this pipeline also:
- Extracts code graph relationships (imports, calls, inheritance)
- Stores graph edges for traversal queries
- Enables hybrid search with graph expansion
"""

import asyncio
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, AsyncIterator, Optional, TYPE_CHECKING

import structlog

from respo.embedding import CodeEmbedder
from respo.graph.analyzer import CodeGraph, CodeGraphAnalyzer
from respo.ingestion.chunker import CodeChunk, CodeChunker
from respo.ingestion.github_scraper import CodeFile, GitHubScraper, ScraperConfig
from respo.vectorstore.base import VectorStoreBase

if TYPE_CHECKING:
    from respo.vectorstore.themis import ThemisVectorStore

logger = structlog.get_logger(__name__)


@dataclass
class IngestionStats:
    """Statistics from ingestion."""

    files_processed: int = 0
    chunks_created: int = 0
    chunks_indexed: int = 0
    graph_nodes: int = 0
    graph_edges: int = 0
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
    exclude_patterns: list[str] = field(default_factory=list)
    
    # Graph extraction (for ThemisDB)
    enable_graph: bool = True
    extract_imports: bool = True
    extract_calls: bool = True
    extract_inheritance: bool = True

    def __post_init__(self) -> None:
        if not self.exclude_patterns:
            self.exclude_patterns = ["**/test*", "**/spec*", "**/__pycache__/*"]


class IngestionPipeline:
    """
    Pipeline for ingesting code into the vector store.

    Steps:
    1. Parse/scrape source code files
    2. Chunk code into semantic units
    3. Extract code graph relationships (if ThemisDB)
    4. Generate embeddings
    5. Store in vector database with graph edges
    
    When using ThemisDB as backend:
    - Graph relationships (imports, calls, inheritance) are extracted
    - Enables hybrid search with graph expansion
    - Supports dependency traversal queries
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
        
        # Initialize graph analyzer if graph extraction is enabled
        self.graph_analyzer: Optional[CodeGraphAnalyzer] = None
        if self.config.enable_graph:
            self.graph_analyzer = CodeGraphAnalyzer()
        
        # Check if we're using ThemisDB (has graph capabilities)
        self._is_themis = self._check_themis_backend()
    
    def _check_themis_backend(self) -> bool:
        """Check if vector store is ThemisDB."""
        try:
            from respo.vectorstore.themis import ThemisVectorStore
            return isinstance(self.vector_store, ThemisVectorStore)
        except ImportError:
            return False

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
            
            # Extract graph relationships if enabled and using ThemisDB
            graph: Optional[CodeGraph] = None
            if self.graph_analyzer and self._is_themis:
                graph = self.graph_analyzer.analyze(
                    source=content,
                    language=language,
                    file_path=path,
                )
                stats.graph_nodes = len(graph.nodes)
                stats.graph_edges = len(graph.edges)

            # Prepare for indexing
            ids = []
            documents = []
            metadatas = []

            for i, chunk in enumerate(chunks):
                chunk_id = f"{path}:{chunk.start_line}-{chunk.end_line}"
                ids.append(chunk_id)
                documents.append(chunk.content)

                chunk_metadata: dict[str, Any] = {
                    "path": path,
                    "language": language,
                    "chunk_type": chunk.chunk_type,
                    "start_line": chunk.start_line,
                    "end_line": chunk.end_line,
                    "name": chunk.name,
                    **(metadata or {}),
                }
                
                # Add graph edges for this chunk if available
                if graph:
                    chunk_edges = self._get_edges_for_chunk(
                        graph, chunk, path
                    )
                    if chunk_edges:
                        chunk_metadata["graph_edges"] = chunk_edges

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
                graph_edges=stats.graph_edges,
            )

        except Exception as e:
            stats.errors = 1
            logger.error("Ingestion error", path=path, error=str(e))

        return stats
    
    def _get_edges_for_chunk(
        self,
        graph: CodeGraph,
        chunk: CodeChunk,
        path: str,
    ) -> list[dict[str, Any]]:
        """
        Get graph edges relevant to a code chunk.
        
        Args:
            graph: Full file graph
            chunk: Code chunk
            path: File path
            
        Returns:
            List of edge dictionaries
        """
        edges = []
        chunk_id = f"{path}:{chunk.start_line}-{chunk.end_line}"
        
        # Find edges where source matches chunk's code entity
        for edge in graph.edges:
            # Check if edge source is within chunk's line range
            source_node = next(
                (n for n in graph.nodes if n.id == edge.source_id),
                None
            )
            if source_node:
                if (source_node.line_start >= chunk.start_line and 
                    source_node.line_end <= chunk.end_line):
                    edges.append({
                        "source": chunk_id,
                        "target": edge.target_id,
                        "type": edge.edge_type,
                        "properties": edge.properties,
                    })
        
        return edges

    async def ingest_directory(
        self,
        directory: Path,
        repo_name: Optional[str] = None,
    ) -> IngestionStats:
        """
        Ingest all code files from a directory.
        
        When using ThemisDB:
        - Extracts cross-file dependencies
        - Builds complete dependency graph
        - Enables repository-wide traversal queries

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

                metadata: dict[str, Any] = {}
                if repo_name:
                    metadata["repo"] = repo_name

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
            graph_nodes=total_stats.graph_nodes,
            graph_edges=total_stats.graph_edges,
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
        
        When using ThemisDB:
        - Extracts cross-file dependency graph
        - Enables "find all usages" queries
        - Supports call graph visualization

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
            graph_edges=total_stats.graph_edges,
        )

        return total_stats
    
    async def analyze_dependencies(
        self,
        code_id: str,
    ) -> dict[str, Any]:
        """
        Analyze dependencies for a code entity (requires ThemisDB).
        
        Uses ThemisDB's graph traversal to find:
        - Direct imports
        - Transitive dependencies
        - Usages (reverse dependencies)
        
        Args:
            code_id: Code entity ID
            
        Returns:
            Dependency analysis result
        """
        if not self._is_themis:
            return {"error": "Dependency analysis requires ThemisDB backend"}
        
        # Import here to avoid circular imports
        from respo.vectorstore.themis import ThemisVectorStore
        themis_store: ThemisVectorStore = self.vector_store  # type: ignore
        
        dependencies = await themis_store.find_dependencies(
            code_id=code_id,
            include_transitive=True,
        )
        
        usages = await themis_store.find_usages(code_id=code_id)
        
        return {
            "code_id": code_id,
            "dependencies": dependencies,
            "usages": usages,
            "dependency_count": sum(len(v) for v in dependencies.values()),
            "usage_count": len(usages),
        }
    
    async def get_call_graph(
        self,
        function_id: str,
        depth: int = 3,
    ) -> dict[str, Any]:
        """
        Get the call graph for a function (requires ThemisDB).
        
        Args:
            function_id: Function ID
            depth: Maximum call depth
            
        Returns:
            Call graph structure
        """
        if not self._is_themis:
            return {"error": "Call graph requires ThemisDB backend"}
        
        from respo.vectorstore.themis import ThemisVectorStore
        themis_store: ThemisVectorStore = self.vector_store  # type: ignore
        
        return await themis_store.get_call_graph(
            function_id=function_id,
            depth=depth,
        )

    async def _process_batch(
        self,
        batch: list[tuple[str, str, str, dict[str, Any]]],
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

        async def limited_task(task: Any) -> IngestionStats:
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
            graph_nodes=a.graph_nodes + b.graph_nodes,
            graph_edges=a.graph_edges + b.graph_edges,
            errors=a.errors + b.errors,
            total_size_bytes=a.total_size_bytes + b.total_size_bytes,
        )
