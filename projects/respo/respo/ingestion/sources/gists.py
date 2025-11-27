"""
GitHub Gists Source Scraper

Scrapes code snippets from GitHub Gists.
"""

import asyncio
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class GistsScraper(CodeSource):
    """
    GitHub Gists source code scraper.

    Fetches public gists from GitHub.
    """

    source_type = SourceType.GITHUB

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize Gists scraper."""
        super().__init__(config)
        self.api_url = "https://api.github.com"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(10)

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            headers = {
                "Accept": "application/vnd.github+json",
                "X-GitHub-Api-Version": "2022-11-28",
            }
            if self.config.token:
                headers["Authorization"] = f"Bearer {self.config.token}"

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
                        elif resp.status == 403:
                            # Rate limited
                            reset = resp.headers.get("X-RateLimit-Reset")
                            logger.warning("Rate limited", reset=reset)
                            await asyncio.sleep(60)
                        elif resp.status == 404:
                            return None
                        else:
                            logger.warning("API error", status=resp.status)
                except aiohttp.ClientError as e:
                    logger.warning("Request failed", attempt=attempt, error=str(e))
                    await asyncio.sleep(2 ** attempt)

            return None

    async def _get_raw_content(self, url: str) -> Optional[str]:
        """Fetch raw file content."""
        session = await self._get_session()
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
        Scrape gists from a user or search query.

        Args:
            identifier: Username or "public" for public gists
            version: Not used for gists

        Yields:
            SourceFile objects for each gist file
        """
        logger.info("Scraping GitHub Gists", identifier=identifier)

        file_count = 0
        page = 1

        while file_count < self.config.max_files_per_repo:
            if identifier == "public":
                # Public gists
                data = await self._api_request(
                    "gists/public",
                    params={"per_page": 100, "page": page},
                )
            else:
                # User's gists
                data = await self._api_request(
                    f"users/{identifier}/gists",
                    params={"per_page": 100, "page": page},
                )

            if not data:
                break

            for gist in data:
                gist_id = gist["id"]
                description = gist.get("description", "")
                owner = gist.get("owner", {}).get("login", "anonymous")

                for filename, file_info in gist.get("files", {}).items():
                    size_bytes = file_info.get("size", 0)
                    language = file_info.get("language", "").lower() or "unknown"

                    if not self._should_include_file(filename, size_bytes):
                        continue

                    # Get content
                    if file_info.get("truncated"):
                        # Need to fetch full content
                        content = await self._get_raw_content(file_info["raw_url"])
                    else:
                        content = file_info.get("content")

                    if not content:
                        continue

                    file_count += 1

                    yield SourceFile(
                        id=SourceFile.generate_id(self.source_type, f"gist:{gist_id}", filename),
                        path=f"{owner}/{gist_id}/{filename}",
                        source_type=self.source_type,
                        content=content,
                        size_bytes=len(content.encode("utf-8")),
                        language=self._detect_language(filename),
                        repo=f"gist:{gist_id}",
                        metadata={
                            "source": "github_gists",
                            "gist_id": gist_id,
                            "owner": owner,
                            "description": description,
                            "public": gist.get("public", True),
                            "url": gist.get("html_url"),
                            "created_at": gist.get("created_at"),
                            "updated_at": gist.get("updated_at"),
                        },
                    )

                    if file_count >= self.config.max_files_per_repo:
                        break

            if len(data) < 100:
                break
            page += 1

        logger.info("Scraping complete", identifier=identifier, files=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """
        Search gists is not directly supported by GitHub API.
        Returns recent public gists instead.
        """
        data = await self._api_request(
            "gists/public",
            params={"per_page": min(limit, 100)},
        )

        if not data:
            return []

        results = []
        query_lower = query.lower()

        for gist in data:
            # Filter by description or filename
            description = gist.get("description", "") or ""
            files = list(gist.get("files", {}).keys())

            if query_lower in description.lower() or any(query_lower in f.lower() for f in files):
                results.append({
                    "gist_id": gist["id"],
                    "description": description,
                    "owner": gist.get("owner", {}).get("login"),
                    "files": files,
                    "url": gist.get("html_url"),
                    "public": gist.get("public", True),
                    "created_at": gist.get("created_at"),
                })

                if len(results) >= limit:
                    break

        return results

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get gist metadata."""
        gist = await self._api_request(f"gists/{identifier}")

        if not gist:
            return {}

        return {
            "gist_id": gist["id"],
            "description": gist.get("description"),
            "owner": gist.get("owner", {}).get("login"),
            "files": list(gist.get("files", {}).keys()),
            "url": gist.get("html_url"),
            "public": gist.get("public", True),
            "created_at": gist.get("created_at"),
            "updated_at": gist.get("updated_at"),
            "comments": gist.get("comments", 0),
            "forks": len(gist.get("forks", [])),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
