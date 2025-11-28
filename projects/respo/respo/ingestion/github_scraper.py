"""
RESPO GitHub Scraper

A targeted scraper for extracting source code from GitHub repositories.
Supports repository cloning, file filtering, and code parsing.
"""

import asyncio
import os
import re
import shutil
import tempfile
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from types import TracebackType
from typing import AsyncIterator, Optional, Type

import httpx
import structlog

logger = structlog.get_logger(__name__)


@dataclass
class CodeFile:
    """Represents a scraped source code file."""

    path: str
    content: str
    language: str
    size_bytes: int
    repo: str
    branch: str
    commit_sha: Optional[str] = None
    last_modified: Optional[datetime] = None
    license: Optional[str] = None
    metadata: dict = field(default_factory=dict)


@dataclass
class ScraperConfig:
    """Configuration for the GitHub scraper."""

    # GitHub API settings
    github_token: Optional[str] = None
    api_base_url: str = "https://api.github.com"

    # Clone settings
    clone_depth: int = 1  # Shallow clone by default
    clone_timeout: int = 300  # 5 minutes

    # File filters
    include_extensions: list[str] = field(
        default_factory=lambda: [
            ".py",
            ".js",
            ".ts",
            ".tsx",
            ".jsx",
            ".go",
            ".rs",
            ".cpp",
            ".c",
            ".h",
            ".hpp",
            ".java",
            ".cs",
            ".rb",
            ".php",
            ".swift",
            ".kt",
            ".scala",
            ".md",
            ".rst",
            ".txt",
        ]
    )
    exclude_patterns: list[str] = field(
        default_factory=lambda: [
            "**/node_modules/**",
            "**/.git/**",
            "**/vendor/**",
            "**/dist/**",
            "**/build/**",
            "**/__pycache__/**",
            "**/*.min.js",
            "**/*.min.css",
            "**/package-lock.json",
            "**/yarn.lock",
            "**/Cargo.lock",
            "**/go.sum",
        ]
    )

    # Size limits
    max_file_size_kb: int = 500  # Skip files larger than 500KB
    max_files_per_repo: int = 10000

    # Rate limiting
    requests_per_minute: int = 30
    retry_attempts: int = 3
    retry_delay: float = 1.0


class GitHubScraper:
    """
    GitHub repository scraper for extracting source code.

    Supports:
    - Repository cloning via git
    - GitHub API for metadata
    - File filtering by extension and patterns
    - Rate limiting and retry logic
    """

    # Language detection by extension
    EXTENSION_LANGUAGE_MAP = {
        ".py": "python",
        ".pyi": "python",
        ".pyx": "python",
        ".js": "javascript",
        ".jsx": "javascript",
        ".mjs": "javascript",
        ".ts": "typescript",
        ".tsx": "typescript",
        ".go": "go",
        ".rs": "rust",
        ".cpp": "cpp",
        ".cc": "cpp",
        ".cxx": "cpp",
        ".c": "c",
        ".h": "c",
        ".hpp": "cpp",
        ".hxx": "cpp",
        ".java": "java",
        ".cs": "csharp",
        ".rb": "ruby",
        ".php": "php",
        ".swift": "swift",
        ".kt": "kotlin",
        ".kts": "kotlin",
        ".scala": "scala",
        ".md": "markdown",
        ".rst": "restructuredtext",
        ".txt": "text",
        ".json": "json",
        ".yaml": "yaml",
        ".yml": "yaml",
        ".toml": "toml",
        ".xml": "xml",
        ".html": "html",
        ".css": "css",
        ".scss": "scss",
        ".sql": "sql",
        ".sh": "shell",
        ".bash": "shell",
        ".zsh": "shell",
        ".ps1": "powershell",
        ".dockerfile": "dockerfile",
    }

    def __init__(self, config: Optional[ScraperConfig] = None) -> None:
        """Initialize the scraper with configuration."""
        self.config = config or ScraperConfig()
        self._http_client: Optional[httpx.AsyncClient] = None
        self._request_times: list[float] = []

    async def __aenter__(self) -> "GitHubScraper":
        """Async context manager entry."""
        self._http_client = httpx.AsyncClient(
            headers=self._get_headers(),
            timeout=httpx.Timeout(30.0),
        )
        return self

    async def __aexit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_val: Optional[BaseException],
        exc_tb: Optional[TracebackType],
    ) -> None:
        """Async context manager exit."""
        if self._http_client:
            await self._http_client.aclose()

    def _get_headers(self) -> dict[str, str]:
        """Get HTTP headers for GitHub API requests."""
        headers = {
            "Accept": "application/vnd.github.v3+json",
            "User-Agent": "RESPO-Scraper/1.0",
        }
        if self.config.github_token:
            headers["Authorization"] = f"token {self.config.github_token}"
        return headers

    async def _rate_limit(self) -> None:
        """Enforce rate limiting."""
        now = asyncio.get_event_loop().time()
        # Remove requests older than 1 minute
        self._request_times = [t for t in self._request_times if now - t < 60]

        if len(self._request_times) >= self.config.requests_per_minute:
            # Wait until oldest request is more than 1 minute old
            wait_time = 60 - (now - self._request_times[0])
            if wait_time > 0:
                logger.info("Rate limiting", wait_seconds=wait_time)
                await asyncio.sleep(wait_time)

        self._request_times.append(now)

    def _detect_language(self, file_path: str) -> str:
        """Detect programming language from file extension."""
        ext = Path(file_path).suffix.lower()
        return self.EXTENSION_LANGUAGE_MAP.get(ext, "unknown")

    def _should_include_file(self, file_path: str) -> bool:
        """Check if file should be included based on filters."""
        path = Path(file_path)

        # Check extension
        if path.suffix.lower() not in self.config.include_extensions:
            return False

        # Check exclude patterns
        for pattern in self.config.exclude_patterns:
            if path.match(pattern):
                return False

        return True

    async def get_repo_info(self, owner: str, repo: str) -> dict:
        """Get repository metadata from GitHub API."""
        await self._rate_limit()

        if not self._http_client:
            raise RuntimeError("Scraper not initialized. Use async context manager.")

        url = f"{self.config.api_base_url}/repos/{owner}/{repo}"

        for attempt in range(self.config.retry_attempts):
            try:
                response = await self._http_client.get(url)
                response.raise_for_status()
                return response.json()
            except httpx.HTTPError as e:
                logger.warning(
                    "API request failed",
                    url=url,
                    attempt=attempt + 1,
                    error=str(e),
                )
                if attempt < self.config.retry_attempts - 1:
                    await asyncio.sleep(self.config.retry_delay * (attempt + 1))
                else:
                    raise

        return {}

    async def clone_repository(
        self,
        owner: str,
        repo: str,
        branch: Optional[str] = None,
        target_dir: Optional[str] = None,
    ) -> str:
        """
        Clone a GitHub repository.

        Args:
            owner: Repository owner
            repo: Repository name
            branch: Branch to clone (default: default branch)
            target_dir: Target directory (default: temp directory)

        Returns:
            Path to cloned repository
        """
        if target_dir is None:
            target_dir = tempfile.mkdtemp(prefix=f"respo_{owner}_{repo}_")

        # Build clone URL (always use public URL, auth via git credential)
        clone_url = f"https://github.com/{owner}/{repo}.git"

        # Build git clone command
        cmd = ["git", "clone", "--depth", str(self.config.clone_depth)]
        if branch:
            cmd.extend(["--branch", branch])
        cmd.extend([clone_url, target_dir])

        logger.info("Cloning repository", owner=owner, repo=repo, branch=branch)

        # Set up environment with token for git credential helper
        env = os.environ.copy()
        if self.config.github_token:
            # Use GIT_ASKPASS to provide credentials securely
            env["GIT_TERMINAL_PROMPT"] = "0"
            env["GH_TOKEN"] = self.config.github_token
            # Configure git to use token via credential helper
            cmd = [
                "git",
                "-c", f"credential.helper=!echo password={self.config.github_token}",
                "-c", "credential.username=x-access-token",
                "clone",
                "--depth", str(self.config.clone_depth),
            ]
            if branch:
                cmd.extend(["--branch", branch])
            cmd.extend([clone_url, target_dir])

        try:
            process = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                env=env,
            )
            stdout, stderr = await asyncio.wait_for(
                process.communicate(),
                timeout=self.config.clone_timeout,
            )

            if process.returncode != 0:
                error_msg = stderr.decode() if stderr else "Unknown error"
                raise RuntimeError(f"Git clone failed: {error_msg}")

            logger.info("Repository cloned", path=target_dir)
            return target_dir

        except asyncio.TimeoutError:
            shutil.rmtree(target_dir, ignore_errors=True)
            raise RuntimeError(f"Clone timeout after {self.config.clone_timeout}s")

    async def get_commit_sha(self, repo_path: str) -> Optional[str]:
        """Get the current commit SHA of a cloned repository."""
        try:
            process = await asyncio.create_subprocess_exec(
                "git",
                "rev-parse",
                "HEAD",
                cwd=repo_path,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, _ = await process.communicate()
            if process.returncode == 0:
                return stdout.decode().strip()
        except Exception as e:
            logger.warning("Failed to get commit SHA", error=str(e))
        return None

    async def scrape_repository(
        self,
        owner: str,
        repo: str,
        branch: Optional[str] = None,
        cleanup: bool = True,
    ) -> AsyncIterator[CodeFile]:
        """
        Scrape all matching files from a GitHub repository.

        Args:
            owner: Repository owner
            repo: Repository name
            branch: Branch to scrape (default: default branch)
            cleanup: Remove cloned repo after scraping

        Yields:
            CodeFile objects for each matching file
        """
        repo_path = None
        try:
            # Get repo info for metadata
            repo_info = await self.get_repo_info(owner, repo)
            default_branch = repo_info.get("default_branch", "main")
            license_info = repo_info.get("license", {})
            license_name = license_info.get("spdx_id") if license_info else None

            # Clone repository
            branch = branch or default_branch
            repo_path = await self.clone_repository(owner, repo, branch)
            commit_sha = await self.get_commit_sha(repo_path)

            # Walk through files
            file_count = 0
            repo_root = Path(repo_path)

            for file_path in repo_root.rglob("*"):
                if file_count >= self.config.max_files_per_repo:
                    logger.warning(
                        "Max files limit reached",
                        limit=self.config.max_files_per_repo,
                    )
                    break

                if not file_path.is_file():
                    continue

                relative_path = str(file_path.relative_to(repo_root))

                if not self._should_include_file(relative_path):
                    continue

                # Check file size
                file_size = file_path.stat().st_size
                if file_size > self.config.max_file_size_kb * 1024:
                    logger.debug("Skipping large file", path=relative_path, size_kb=file_size // 1024)
                    continue

                try:
                    content = file_path.read_text(encoding="utf-8")
                except (UnicodeDecodeError, IOError) as e:
                    logger.debug("Skipping unreadable file", path=relative_path, error=str(e))
                    continue

                file_count += 1
                yield CodeFile(
                    path=relative_path,
                    content=content,
                    language=self._detect_language(relative_path),
                    size_bytes=file_size,
                    repo=f"{owner}/{repo}",
                    branch=branch,
                    commit_sha=commit_sha,
                    license=license_name,
                    metadata={
                        "stars": repo_info.get("stargazers_count", 0),
                        "forks": repo_info.get("forks_count", 0),
                        "description": repo_info.get("description"),
                        "topics": repo_info.get("topics", []),
                    },
                )

            logger.info(
                "Repository scraped",
                owner=owner,
                repo=repo,
                files_found=file_count,
            )

        finally:
            if cleanup and repo_path and os.path.exists(repo_path):
                shutil.rmtree(repo_path, ignore_errors=True)
                logger.debug("Cleaned up repository", path=repo_path)

    async def scrape_repositories(
        self,
        repos: list[tuple[str, str]],
        branch: Optional[str] = None,
    ) -> AsyncIterator[CodeFile]:
        """
        Scrape multiple repositories.

        Args:
            repos: List of (owner, repo) tuples
            branch: Branch to scrape (None = default branch)

        Yields:
            CodeFile objects from all repositories
        """
        for owner, repo in repos:
            try:
                async for code_file in self.scrape_repository(owner, repo, branch):
                    yield code_file
            except Exception as e:
                logger.error(
                    "Failed to scrape repository",
                    owner=owner,
                    repo=repo,
                    error=str(e),
                )

    async def search_repositories(
        self,
        query: str,
        language: Optional[str] = None,
        min_stars: int = 0,
        max_results: int = 100,
    ) -> list[dict]:
        """
        Search for repositories using GitHub search API.

        Args:
            query: Search query
            language: Filter by language
            min_stars: Minimum stars
            max_results: Maximum results to return

        Returns:
            List of repository metadata dicts
        """
        await self._rate_limit()

        if not self._http_client:
            raise RuntimeError("Scraper not initialized. Use async context manager.")

        # Build search query
        search_query = query
        if language:
            search_query += f" language:{language}"
        if min_stars > 0:
            search_query += f" stars:>={min_stars}"

        url = f"{self.config.api_base_url}/search/repositories"
        params = {
            "q": search_query,
            "sort": "stars",
            "order": "desc",
            "per_page": min(max_results, 100),
        }

        results = []
        page = 1

        while len(results) < max_results:
            params["page"] = page

            try:
                response = await self._http_client.get(url, params=params)
                response.raise_for_status()
                data = response.json()

                items = data.get("items", [])
                if not items:
                    break

                for item in items:
                    if len(results) >= max_results:
                        break
                    results.append({
                        "owner": item["owner"]["login"],
                        "repo": item["name"],
                        "full_name": item["full_name"],
                        "description": item.get("description"),
                        "stars": item["stargazers_count"],
                        "forks": item["forks_count"],
                        "language": item.get("language"),
                        "topics": item.get("topics", []),
                        "url": item["html_url"],
                    })

                page += 1

                # Rate limit check
                await self._rate_limit()

            except httpx.HTTPError as e:
                logger.error("Search failed", error=str(e))
                break

        return results


def parse_github_url(url: str) -> tuple[str, str]:
    """
    Parse a GitHub URL to extract owner and repo.

    Args:
        url: GitHub URL (e.g., https://github.com/owner/repo)

    Returns:
        Tuple of (owner, repo)

    Raises:
        ValueError: If URL is invalid
    """
    # Handle various GitHub URL formats
    patterns = [
        r"github\.com[/:]([^/]+)/([^/]+?)(?:\.git)?$",  # HTTPS or SSH with optional .git
        r"github\.com[/:]([^/]+)/([^/]+)",  # HTTPS or SSH
        r"^([^/]+)/([^/]+)$",  # owner/repo format
    ]

    for pattern in patterns:
        match = re.search(pattern, url)
        if match:
            owner, repo = match.groups()
            # Remove .git suffix if present
            repo = repo.removesuffix(".git")
            return owner, repo

    raise ValueError(f"Invalid GitHub URL: {url}")
