"""
NPM Source Scraper

Scrapes source code from npm registry (npmjs.com).
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


class NPMScraper(CodeSource):
    """
    NPM source code scraper.

    Downloads and extracts packages from npm registry.
    """

    source_type = SourceType.NPM

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
        registry_url: str = "https://registry.npmjs.org",
    ):
        """
        Initialize NPM scraper.

        Args:
            config: Scraper configuration
            registry_url: NPM registry URL
        """
        super().__init__(config)
        self.registry_url = registry_url.rstrip("/")
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
            url = f"{self.registry_url}/{endpoint}"

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
        """Download and extract an npm package tarball."""
        session = await self._get_session()

        try:
            async with session.get(url) as resp:
                if resp.status != 200:
                    return None

                data = await resp.read()

                # npm tarballs are .tgz files
                with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
                    tf.extractall(temp_dir)

                # npm packages are extracted to a "package" directory
                package_dir = temp_dir / "package"
                if package_dir.exists():
                    return package_dir

                # Fallback to first directory
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
        Scrape source code from an npm package.

        Args:
            identifier: Package name (e.g., "express", "@types/node")
            version: Specific version (default: latest)

        Yields:
            SourceFile objects for each code file
        """
        # Handle scoped packages
        package_name = identifier.replace("/", "%2F") if identifier.startswith("@") else identifier

        # Get package info
        pkg_info = await self._api_request(package_name)

        if not pkg_info:
            logger.error("Package not found", package=identifier)
            return

        # Get version info
        versions = pkg_info.get("versions", {})
        dist_tags = pkg_info.get("dist-tags", {})

        if version:
            pkg_version = version
        else:
            pkg_version = dist_tags.get("latest")

        if not pkg_version or pkg_version not in versions:
            logger.error("Version not found", package=identifier, version=version)
            return

        version_info = versions[pkg_version]
        tarball_url = version_info.get("dist", {}).get("tarball")

        if not tarball_url:
            logger.error("No tarball found", package=identifier)
            return

        logger.info("Downloading source", package=identifier, version=pkg_version)

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            extracted = await self._download_and_extract(tarball_url, temp_path)

            if not extracted:
                logger.error("Failed to extract source", package=identifier)
                return

            file_count = 0
            license_name = version_info.get("license")

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
                        "npm_url": f"https://www.npmjs.com/package/{identifier}",
                        "description": version_info.get("description"),
                        "author": version_info.get("author"),
                        "keywords": version_info.get("keywords", []),
                        "repository": version_info.get("repository"),
                        "dependencies": list(version_info.get("dependencies", {}).keys()),
                        "devDependencies": list(version_info.get("devDependencies", {}).keys()),
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
        """Search for npm packages."""
        session = await self._get_session()
        search_url = "https://registry.npmjs.org/-/v1/search"

        params = {
            "text": query,
            "size": min(limit, 250),
        }

        try:
            async with session.get(search_url, params=params) as resp:
                if resp.status != 200:
                    return []

                data = await resp.json()
                results = []

                for obj in data.get("objects", []):
                    pkg = obj.get("package", {})
                    results.append({
                        "name": pkg.get("name"),
                        "version": pkg.get("version"),
                        "description": pkg.get("description"),
                        "keywords": pkg.get("keywords", []),
                        "author": pkg.get("author"),
                        "url": f"https://www.npmjs.com/package/{pkg.get('name')}",
                        "score": obj.get("score", {}).get("final", 0),
                        "searchScore": obj.get("searchScore", 0),
                    })

                return results[:limit]

        except Exception as e:
            logger.error("Search failed", error=str(e))
            return []

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get package metadata."""
        package_name = identifier.replace("/", "%2F") if identifier.startswith("@") else identifier
        pkg_info = await self._api_request(package_name)

        if not pkg_info:
            return {}

        latest = pkg_info.get("dist-tags", {}).get("latest")
        latest_info = pkg_info.get("versions", {}).get(latest, {}) if latest else {}

        return {
            "name": pkg_info.get("name"),
            "description": pkg_info.get("description"),
            "latest_version": latest,
            "license": latest_info.get("license"),
            "author": latest_info.get("author"),
            "repository": latest_info.get("repository"),
            "keywords": latest_info.get("keywords", []),
            "homepage": latest_info.get("homepage"),
            "versions": list(pkg_info.get("versions", {}).keys()),
            "dist_tags": pkg_info.get("dist-tags", {}),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
