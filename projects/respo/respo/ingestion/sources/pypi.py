"""
PyPI Source Scraper

Scrapes source code from Python Package Index (pypi.org).
"""

import asyncio
import io
import tarfile
import tempfile
import zipfile
from pathlib import Path
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class PyPIScraper(CodeSource):
    """
    PyPI source code scraper.

    Downloads and extracts source distributions from PyPI.
    """

    source_type = SourceType.PYPI

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
        index_url: str = "https://pypi.org",
    ):
        """
        Initialize PyPI scraper.

        Args:
            config: Scraper configuration
            index_url: PyPI index URL (default: pypi.org)
        """
        super().__init__(config)
        self.index_url = index_url.rstrip("/")
        self.api_url = f"{self.index_url}/pypi"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(int(self.config.requests_per_second * 2))

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession(
                headers={"Accept": "application/json"}
            )
        return self._session

    async def _api_request(self, endpoint: str) -> Optional[dict]:
        """Make a rate-limited API request."""
        async with self._rate_limiter:
            session = await self._get_session()
            url = f"{self.api_url}/{endpoint}/json"

            for attempt in range(self.config.max_retries):
                try:
                    async with session.get(url) as resp:
                        if resp.status == 200:
                            return await resp.json()
                        elif resp.status == 404:
                            return None
                        else:
                            logger.warning("API error", status=resp.status, url=url)
                except aiohttp.ClientError as e:
                    logger.warning("Request failed", attempt=attempt, error=str(e))
                    await asyncio.sleep(2 ** attempt)

            return None

    async def _download_and_extract(self, url: str, temp_dir: Path) -> Optional[Path]:
        """Download and extract a source distribution."""
        session = await self._get_session()

        try:
            async with session.get(url) as resp:
                if resp.status != 200:
                    return None

                data = await resp.read()

                if url.endswith(".tar.gz") or url.endswith(".tgz"):
                    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
                        tf.extractall(temp_dir)
                elif url.endswith(".zip") or url.endswith(".whl"):
                    with zipfile.ZipFile(io.BytesIO(data)) as zf:
                        zf.extractall(temp_dir)
                else:
                    logger.warning("Unknown archive format", url=url)
                    return None

                # Return the extracted directory
                dirs = list(temp_dir.iterdir())
                if dirs:
                    return dirs[0] if dirs[0].is_dir() else temp_dir
                return temp_dir

        except Exception as e:
            logger.error("Error downloading/extracting", url=url, error=str(e))
            return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape source code from a PyPI package.

        Args:
            identifier: Package name (e.g., "requests", "flask")
            version: Specific version (default: latest)

        Yields:
            SourceFile objects for each code file
        """
        # Get package info
        endpoint = f"{identifier}/{version}" if version else identifier
        pkg_info = await self._api_request(endpoint)

        if not pkg_info:
            logger.error("Package not found", package=identifier)
            return

        info = pkg_info.get("info", {})
        urls = pkg_info.get("urls", [])
        pkg_version = info.get("version", version or "unknown")

        # Find source distribution
        sdist_url = None
        for url_info in urls:
            if url_info.get("packagetype") == "sdist":
                sdist_url = url_info.get("url")
                break

        if not sdist_url:
            # Fall back to wheel or any other distribution
            for url_info in urls:
                if url_info.get("url", "").endswith((".whl", ".tar.gz", ".zip")):
                    sdist_url = url_info.get("url")
                    break

        if not sdist_url:
            logger.error("No source distribution found", package=identifier)
            return

        logger.info("Downloading source", package=identifier, version=pkg_version)

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            extracted = await self._download_and_extract(sdist_url, temp_path)

            if not extracted:
                logger.error("Failed to extract source", package=identifier)
                return

            file_count = 0
            license_name = info.get("license")

            for file_path in extracted.rglob("*"):
                if not file_path.is_file():
                    continue

                rel_path = str(file_path.relative_to(extracted))
                size_bytes = file_path.stat().st_size

                if not self._should_include_file(rel_path, size_bytes):
                    continue

                try:
                    content = file_path.read_text(encoding="utf-8", errors="replace")
                except Exception:
                    continue

                file_count += 1

                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, identifier, rel_path),
                    path=rel_path,
                    source_type=self.source_type,
                    content=content,
                    size_bytes=size_bytes,
                    language=self._detect_language(rel_path),
                    repo=identifier,
                    version=pkg_version,
                    license=license_name,
                    metadata={
                        "pypi_url": f"{self.index_url}/project/{identifier}/",
                        "author": info.get("author"),
                        "author_email": info.get("author_email"),
                        "summary": info.get("summary"),
                        "requires_python": info.get("requires_python"),
                        "keywords": info.get("keywords"),
                        "classifiers": info.get("classifiers", []),
                    },
                )

                if file_count >= self.config.max_files_per_repo:
                    break

        logger.info("Scraping complete", package=identifier, files=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search for PyPI packages."""
        # PyPI doesn't have a great search API, so we use the simple search
        session = await self._get_session()
        search_url = f"{self.index_url}/search/"

        params = {"q": query}

        try:
            async with session.get(search_url, params=params) as resp:
                if resp.status != 200:
                    return []

                # Parse HTML response (simplified)
                html = await resp.text()
                results = []

                # Simple regex to extract package names from search results
                import re
                pattern = r'<a class="package-snippet"[^>]*href="/project/([^/]+)/"'
                matches = re.findall(pattern, html)

                for pkg_name in matches[:limit]:
                    pkg_info = await self._api_request(pkg_name)
                    if pkg_info:
                        info = pkg_info.get("info", {})
                        results.append({
                            "name": pkg_name,
                            "version": info.get("version"),
                            "summary": info.get("summary"),
                            "author": info.get("author"),
                            "license": info.get("license"),
                            "url": f"{self.index_url}/project/{pkg_name}/",
                            "requires_python": info.get("requires_python"),
                        })

                return results

        except Exception as e:
            logger.error("Search failed", error=str(e))
            return []

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get package metadata."""
        pkg_info = await self._api_request(identifier)

        if not pkg_info:
            return {}

        info = pkg_info.get("info", {})
        return {
            "name": info.get("name"),
            "version": info.get("version"),
            "summary": info.get("summary"),
            "description": info.get("description"),
            "author": info.get("author"),
            "author_email": info.get("author_email"),
            "license": info.get("license"),
            "url": info.get("project_url"),
            "home_page": info.get("home_page"),
            "package_url": info.get("package_url"),
            "requires_python": info.get("requires_python"),
            "keywords": info.get("keywords"),
            "classifiers": info.get("classifiers", []),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
