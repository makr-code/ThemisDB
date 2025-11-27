"""
Bitbucket Source Scraper

Scrapes source code from Bitbucket repositories.
"""

import asyncio
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class BitbucketScraper(CodeSource):
    """
    Bitbucket source code scraper.

    Supports Bitbucket Cloud (bitbucket.org).
    """

    source_type = SourceType.BITBUCKET

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize Bitbucket scraper."""
        super().__init__(config)
        self.api_url = "https://api.bitbucket.org/2.0"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(int(self.config.requests_per_second * 2))

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            headers = {"Accept": "application/json"}
            auth = None

            if self.config.username and self.config.password:
                auth = aiohttp.BasicAuth(self.config.username, self.config.password)
            elif self.config.token:
                headers["Authorization"] = f"Bearer {self.config.token}"

            self._session = aiohttp.ClientSession(headers=headers, auth=auth)
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
                            retry_after = int(resp.headers.get("Retry-After", 60))
                            logger.warning("Rate limited", retry_after=retry_after)
                            await asyncio.sleep(retry_after)
                        elif resp.status == 404:
                            return None
                        else:
                            logger.error("API error", status=resp.status, url=url)
                            raise Exception(f"Bitbucket API error: {resp.status}")
                except aiohttp.ClientError as e:
                    logger.warning("Request failed", attempt=attempt, error=str(e))
                    await asyncio.sleep(2 ** attempt)

            raise Exception(f"Failed after {self.config.max_retries} retries")

    async def _get_file_content(self, workspace: str, repo_slug: str, path: str, commit: str) -> Optional[str]:
        """Get raw file content."""
        session = await self._get_session()
        url = f"{self.api_url}/repositories/{workspace}/{repo_slug}/src/{commit}/{path}"

        try:
            async with session.get(url) as resp:
                if resp.status == 200:
                    return await resp.text()
                return None
        except Exception:
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape source code from a Bitbucket repository.

        Args:
            identifier: Repository path (e.g., "workspace/repo-slug")
            version: Branch, tag, or commit (default: main branch)

        Yields:
            SourceFile objects for each code file
        """
        parts = identifier.split("/")
        if len(parts) != 2:
            logger.error("Invalid identifier format", identifier=identifier)
            return

        workspace, repo_slug = parts

        # Get repository info
        repo = await self._api_request(f"repositories/{workspace}/{repo_slug}")
        if not repo:
            logger.error("Repository not found", repo=identifier)
            return

        # Get default branch
        main_branch = repo.get("mainbranch", {}).get("name", "main")
        branch = version or main_branch

        # Get branch commit
        branch_info = await self._api_request(f"repositories/{workspace}/{repo_slug}/refs/branches/{branch}")
        if not branch_info:
            logger.error("Branch not found", branch=branch)
            return

        commit_sha = branch_info["target"]["hash"]

        logger.info(
            "Scraping Bitbucket repository",
            repo=identifier,
            branch=branch,
        )

        # Get source tree
        file_count = 0
        next_url = f"repositories/{workspace}/{repo_slug}/src/{commit_sha}/"

        while next_url and file_count < self.config.max_files_per_repo:
            # Handle pagination
            if next_url.startswith("http"):
                # Full URL from pagination
                session = await self._get_session()
                async with session.get(next_url) as resp:
                    if resp.status != 200:
                        break
                    data = await resp.json()
            else:
                data = await self._api_request(next_url)

            if not data:
                break

            for item in data.get("values", []):
                if item["type"] != "commit_file":
                    continue

                path = item["path"]
                size_bytes = item.get("size", 0)

                if not self._should_include_file(path, size_bytes):
                    continue

                # Get file content
                content = await self._get_file_content(workspace, repo_slug, path, commit_sha)
                if not content:
                    continue

                file_count += 1

                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, identifier, path),
                    path=path,
                    source_type=self.source_type,
                    content=content,
                    size_bytes=len(content.encode("utf-8")),
                    language=self._detect_language(path),
                    repo=identifier,
                    branch=branch,
                    commit_sha=commit_sha,
                    metadata={
                        "bitbucket_url": f"https://bitbucket.org/{identifier}",
                        "file_url": f"https://bitbucket.org/{identifier}/src/{branch}/{path}",
                    },
                )

            next_url = data.get("next")

        logger.info("Scraping complete", repo=identifier, files=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search for Bitbucket repositories."""
        params = {
            "q": f'name ~ "{query}"',
            "pagelen": min(limit, 100),
        }

        if language:
            params["q"] += f' AND language = "{language}"'

        results = await self._api_request("repositories", params=params)

        if not results:
            return []

        return [
            {
                "full_name": repo["full_name"],
                "name": repo["name"],
                "description": repo.get("description"),
                "url": repo["links"]["html"]["href"],
                "language": repo.get("language"),
                "is_private": repo.get("is_private", False),
                "created_on": repo.get("created_on"),
                "updated_on": repo.get("updated_on"),
            }
            for repo in results.get("values", [])[:limit]
        ]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get repository metadata."""
        parts = identifier.split("/")
        if len(parts) != 2:
            return {}

        workspace, repo_slug = parts
        repo = await self._api_request(f"repositories/{workspace}/{repo_slug}")

        if not repo:
            return {}

        return {
            "full_name": repo["full_name"],
            "name": repo["name"],
            "description": repo.get("description"),
            "url": repo["links"]["html"]["href"],
            "language": repo.get("language"),
            "is_private": repo.get("is_private", False),
            "mainbranch": repo.get("mainbranch", {}).get("name"),
            "created_on": repo.get("created_on"),
            "updated_on": repo.get("updated_on"),
            "size": repo.get("size"),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
