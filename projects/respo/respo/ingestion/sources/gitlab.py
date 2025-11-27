"""
GitLab Source Scraper

Scrapes source code from GitLab repositories (gitlab.com and self-hosted).
"""

import asyncio
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class GitLabScraper(CodeSource):
    """
    GitLab source code scraper.

    Supports both gitlab.com and self-hosted GitLab instances.
    """

    source_type = SourceType.GITLAB

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
        base_url: str = "https://gitlab.com",
    ):
        """
        Initialize GitLab scraper.

        Args:
            config: Scraper configuration
            base_url: GitLab instance URL (default: gitlab.com)
        """
        super().__init__(config)
        self.base_url = base_url.rstrip("/")
        self.api_url = f"{self.base_url}/api/v4"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(int(self.config.requests_per_second * 2))

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            headers = {"Accept": "application/json"}
            if self.config.token:
                headers["PRIVATE-TOKEN"] = self.config.token
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session

    async def _api_request(
        self,
        endpoint: str,
        params: Optional[dict] = None,
    ) -> Any:
        """Make a rate-limited API request."""
        async with self._rate_limiter:
            session = await self._get_session()
            url = f"{self.api_url}/{endpoint}"

            for attempt in range(self.config.max_retries):
                try:
                    async with session.get(url, params=params) as resp:
                        if resp.status == 200:
                            return await resp.json()
                        elif resp.status == 429:
                            # Rate limited, wait and retry
                            retry_after = int(resp.headers.get("Retry-After", 60))
                            logger.warning("Rate limited", retry_after=retry_after)
                            await asyncio.sleep(retry_after)
                        elif resp.status == 404:
                            return None
                        else:
                            logger.error("API error", status=resp.status, url=url)
                            raise Exception(f"GitLab API error: {resp.status}")
                except aiohttp.ClientError as e:
                    logger.warning("Request failed", attempt=attempt, error=str(e))
                    await asyncio.sleep(2 ** attempt)

            raise Exception(f"Failed after {self.config.max_retries} retries")

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape source code from a GitLab repository.

        Args:
            identifier: Project path (e.g., "group/project" or "group/subgroup/project")
            version: Branch, tag, or commit SHA (default: default branch)

        Yields:
            SourceFile objects for each code file
        """
        # URL-encode the project path
        import urllib.parse
        encoded_id = urllib.parse.quote(identifier, safe="")

        # Get project info
        project = await self._api_request(f"projects/{encoded_id}")
        if not project:
            logger.error("Project not found", project=identifier)
            return

        default_branch = version or project.get("default_branch", "main")
        license_name = project.get("license", {}).get("name") if project.get("license") else None

        logger.info(
            "Scraping GitLab repository",
            project=identifier,
            branch=default_branch,
        )

        # Get repository tree recursively
        page = 1
        file_count = 0

        while file_count < self.config.max_files_per_repo:
            tree = await self._api_request(
                f"projects/{encoded_id}/repository/tree",
                params={
                    "ref": default_branch,
                    "recursive": "true",
                    "per_page": 100,
                    "page": page,
                },
            )

            if not tree:
                break

            for item in tree:
                if item["type"] != "blob":
                    continue

                path = item["path"]

                # Check if file should be included (size check will be done after fetch)
                if not self._should_include_file(path, 0):
                    continue

                # Get file content
                try:
                    file_data = await self._api_request(
                        f"projects/{encoded_id}/repository/files/{urllib.parse.quote(path, safe='')}",
                        params={"ref": default_branch},
                    )

                    if not file_data:
                        continue

                    import base64
                    content = base64.b64decode(file_data["content"]).decode("utf-8", errors="replace")
                    size_bytes = len(content.encode("utf-8"))

                    # Final size check
                    if size_bytes > self.config.max_file_size_kb * 1024:
                        continue

                    file_count += 1

                    yield SourceFile(
                        id=SourceFile.generate_id(self.source_type, identifier, path),
                        path=path,
                        source_type=self.source_type,
                        content=content,
                        size_bytes=size_bytes,
                        language=self._detect_language(path),
                        repo=identifier,
                        branch=default_branch,
                        commit_sha=file_data.get("commit_id"),
                        license=license_name,
                        metadata={
                            "gitlab_url": f"{self.base_url}/{identifier}",
                            "file_url": f"{self.base_url}/{identifier}/-/blob/{default_branch}/{path}",
                        },
                    )

                except Exception as e:
                    logger.warning("Failed to fetch file", path=path, error=str(e))
                    continue

            if len(tree) < 100:
                break
            page += 1

        logger.info("Scraping complete", project=identifier, files=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search for GitLab projects."""
        params = {
            "search": query,
            "per_page": min(limit, 100),
            "order_by": "last_activity_at",
        }

        if language:
            # GitLab doesn't have direct language filter, use search
            params["search"] = f"{query} language:{language}"

        results = await self._api_request("projects", params=params)

        if not results:
            return []

        return [
            {
                "id": proj["id"],
                "full_name": proj["path_with_namespace"],
                "name": proj["name"],
                "description": proj.get("description"),
                "url": proj["web_url"],
                "stars": proj.get("star_count", 0),
                "forks": proj.get("forks_count", 0),
                "language": proj.get("language"),
                "topics": proj.get("topics", []),
                "last_activity": proj.get("last_activity_at"),
                "visibility": proj.get("visibility"),
            }
            for proj in results[:limit]
        ]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get project metadata."""
        import urllib.parse
        encoded_id = urllib.parse.quote(identifier, safe="")

        project = await self._api_request(f"projects/{encoded_id}")

        if not project:
            return {}

        return {
            "id": project["id"],
            "full_name": project["path_with_namespace"],
            "name": project["name"],
            "description": project.get("description"),
            "url": project["web_url"],
            "stars": project.get("star_count", 0),
            "forks": project.get("forks_count", 0),
            "default_branch": project.get("default_branch"),
            "topics": project.get("topics", []),
            "created_at": project.get("created_at"),
            "last_activity_at": project.get("last_activity_at"),
            "visibility": project.get("visibility"),
            "license": project.get("license", {}).get("name") if project.get("license") else None,
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
