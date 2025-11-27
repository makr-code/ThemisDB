"""
Crates.io Source Scraper

Scrapes source code from crates.io (Rust packages).
"""

import asyncio
import io
import tarfile
import tempfile
from pathlib import Path
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class CratesScraper(CodeSource):
    """
    Crates.io source code scraper.

    Downloads and extracts Rust crates from crates.io.
    """

    source_type = SourceType.CRATES

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize Crates.io scraper."""
        super().__init__(config)
        self.api_url = "https://crates.io/api/v1"
        self.download_url = "https://static.crates.io/crates"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(int(self.config.requests_per_second))

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession(
                headers={
                    "Accept": "application/json",
                    "User-Agent": "RESPO/0.1.0 (https://github.com/respo)",
                }
            )
        return self._session

    async def _api_request(self, endpoint: str) -> Optional[dict]:
        """Make a rate-limited API request."""
        async with self._rate_limiter:
            session = await self._get_session()
            url = f"{self.api_url}/{endpoint}"

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

    async def _download_and_extract(self, crate_name: str, version: str, temp_dir: Path) -> Optional[Path]:
        """Download and extract a crate."""
        session = await self._get_session()
        url = f"{self.download_url}/{crate_name}/{crate_name}-{version}.crate"

        try:
            async with session.get(url) as resp:
                if resp.status != 200:
                    return None

                data = await resp.read()

                # .crate files are gzipped tarballs
                with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
                    tf.extractall(temp_dir)

                # Crates are extracted to {name}-{version} directory
                crate_dir = temp_dir / f"{crate_name}-{version}"
                if crate_dir.exists():
                    return crate_dir

                # Fallback
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
        Scrape source code from a Rust crate.

        Args:
            identifier: Crate name (e.g., "serde", "tokio")
            version: Specific version (default: latest)

        Yields:
            SourceFile objects for each code file
        """
        # Get crate info
        crate_info = await self._api_request(f"crates/{identifier}")

        if not crate_info:
            logger.error("Crate not found", crate=identifier)
            return

        crate = crate_info.get("crate", {})
        versions_list = crate_info.get("versions", [])

        # Get version
        if version:
            crate_version = version
        else:
            crate_version = crate.get("newest_version")

        if not crate_version:
            logger.error("No version found", crate=identifier)
            return

        # Find version info
        version_info = None
        for v in versions_list:
            if v.get("num") == crate_version:
                version_info = v
                break

        logger.info("Downloading source", crate=identifier, version=crate_version)

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            extracted = await self._download_and_extract(identifier, crate_version, temp_path)

            if not extracted:
                logger.error("Failed to extract source", crate=identifier)
                return

            file_count = 0
            license_name = version_info.get("license") if version_info else None

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
                    version=crate_version,
                    license=license_name,
                    metadata={
                        "crates_url": f"https://crates.io/crates/{identifier}",
                        "docs_url": f"https://docs.rs/{identifier}/{crate_version}",
                        "description": crate.get("description"),
                        "categories": crate.get("categories", []),
                        "keywords": crate.get("keywords", []),
                        "repository": crate.get("repository"),
                        "downloads": crate.get("downloads", 0),
                    },
                )

                if file_count >= self.config.max_files_per_repo:
                    break

        logger.info("Scraping complete", crate=identifier, files=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search for Rust crates."""
        params = {"q": query, "per_page": min(limit, 100)}
        data = await self._api_request(f"crates?q={query}&per_page={min(limit, 100)}")

        if not data:
            return []

        results = []
        for crate in data.get("crates", []):
            results.append({
                "name": crate.get("name"),
                "version": crate.get("newest_version"),
                "description": crate.get("description"),
                "downloads": crate.get("downloads", 0),
                "recent_downloads": crate.get("recent_downloads", 0),
                "url": f"https://crates.io/crates/{crate.get('name')}",
                "repository": crate.get("repository"),
                "documentation": crate.get("documentation"),
            })

        return results[:limit]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get crate metadata."""
        crate_info = await self._api_request(f"crates/{identifier}")

        if not crate_info:
            return {}

        crate = crate_info.get("crate", {})
        versions = crate_info.get("versions", [])

        return {
            "name": crate.get("name"),
            "description": crate.get("description"),
            "newest_version": crate.get("newest_version"),
            "max_version": crate.get("max_version"),
            "downloads": crate.get("downloads"),
            "recent_downloads": crate.get("recent_downloads"),
            "repository": crate.get("repository"),
            "documentation": crate.get("documentation"),
            "homepage": crate.get("homepage"),
            "categories": crate.get("categories", []),
            "keywords": crate.get("keywords", []),
            "versions": [v.get("num") for v in versions],
            "created_at": crate.get("created_at"),
            "updated_at": crate.get("updated_at"),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
