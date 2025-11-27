"""
Debian Source Scraper

Scrapes source code from Debian packages (apt-get source).
"""

import asyncio
import gzip
import io
import tarfile
import tempfile
from pathlib import Path
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class DebianScraper(CodeSource):
    """
    Debian source code scraper.

    Downloads and extracts Debian source packages.
    Supports packages from Debian, Ubuntu, and other APT repositories.
    """

    source_type = SourceType.DEBIAN

    # Default mirror URLs
    DEBIAN_MIRRORS = {
        "debian": "https://deb.debian.org/debian",
        "ubuntu": "https://archive.ubuntu.com/ubuntu",
        "debian-security": "https://security.debian.org/debian-security",
    }

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
        distribution: str = "debian",
        suite: str = "stable",
        components: Optional[list[str]] = None,
    ):
        """
        Initialize Debian scraper.

        Args:
            config: Scraper configuration
            distribution: Distribution name (debian, ubuntu)
            suite: Suite/codename (stable, bookworm, jammy, etc.)
            components: Repository components (main, contrib, non-free)
        """
        super().__init__(config)
        self.distribution = distribution
        self.suite = suite
        self.components = components or ["main"]
        self.mirror = self.DEBIAN_MIRRORS.get(distribution, self.DEBIAN_MIRRORS["debian"])
        self._session: Optional[aiohttp.ClientSession] = None
        self._sources_cache: Optional[dict] = None

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _fetch_sources_index(self) -> dict[str, dict]:
        """Fetch and parse the Sources index file."""
        if self._sources_cache:
            return self._sources_cache

        sources = {}
        session = await self._get_session()

        for component in self.components:
            url = f"{self.mirror}/dists/{self.suite}/{component}/source/Sources.gz"
            logger.info("Fetching Sources index", url=url)

            try:
                async with session.get(url) as resp:
                    if resp.status != 200:
                        logger.warning("Failed to fetch Sources", status=resp.status, url=url)
                        continue

                    compressed = await resp.read()
                    data = gzip.decompress(compressed).decode("utf-8")

                    # Parse Sources file (RFC 822 format)
                    current_package = {}
                    for line in data.split("\n"):
                        if not line:
                            if current_package.get("Package"):
                                sources[current_package["Package"]] = current_package
                            current_package = {}
                        elif line.startswith(" "):
                            # Continuation line
                            if "Files" in current_package:
                                current_package["Files"] += "\n" + line
                        elif ": " in line:
                            key, value = line.split(": ", 1)
                            current_package[key] = value

                    # Don't forget the last package
                    if current_package.get("Package"):
                        sources[current_package["Package"]] = current_package

            except Exception as e:
                logger.error("Error fetching Sources", error=str(e))

        self._sources_cache = sources
        logger.info("Loaded package sources", count=len(sources))
        return sources

    async def _download_and_extract(self, url: str, temp_dir: Path) -> Optional[Path]:
        """Download and extract a source tarball."""
        session = await self._get_session()

        try:
            async with session.get(url) as resp:
                if resp.status != 200:
                    return None

                data = await resp.read()

                # Determine extraction method
                if url.endswith(".tar.gz") or url.endswith(".tgz"):
                    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
                        tf.extractall(temp_dir)
                elif url.endswith(".tar.xz"):
                    import lzma
                    decompressed = lzma.decompress(data)
                    with tarfile.open(fileobj=io.BytesIO(decompressed), mode="r:") as tf:
                        tf.extractall(temp_dir)
                elif url.endswith(".tar.bz2"):
                    import bz2
                    decompressed = bz2.decompress(data)
                    with tarfile.open(fileobj=io.BytesIO(decompressed), mode="r:") as tf:
                        tf.extractall(temp_dir)
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
        Scrape source code from a Debian package.

        Args:
            identifier: Package name (e.g., "python3", "curl")
            version: Specific version (default: latest in suite)

        Yields:
            SourceFile objects for each code file
        """
        sources = await self._fetch_sources_index()

        if identifier not in sources:
            logger.error("Package not found", package=identifier)
            return

        pkg_info = sources[identifier]
        pkg_version = version or pkg_info.get("Version", "unknown")
        directory = pkg_info.get("Directory", "")

        # Find the orig tarball
        files_str = pkg_info.get("Files", "")
        orig_tarball = None

        for line in files_str.split("\n"):
            parts = line.strip().split()
            if len(parts) >= 3:
                filename = parts[2]
                if ".orig.tar" in filename:
                    orig_tarball = filename
                    break

        if not orig_tarball:
            logger.error("No source tarball found", package=identifier)
            return

        tarball_url = f"{self.mirror}/{directory}/{orig_tarball}"
        logger.info("Downloading source", package=identifier, url=tarball_url)

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            extracted = await self._download_and_extract(tarball_url, temp_path)

            if not extracted:
                logger.error("Failed to extract source", package=identifier)
                return

            file_count = 0

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
                    license=pkg_info.get("License"),
                    metadata={
                        "distribution": self.distribution,
                        "suite": self.suite,
                        "maintainer": pkg_info.get("Maintainer"),
                        "homepage": pkg_info.get("Homepage"),
                        "section": pkg_info.get("Section"),
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
        """Search for Debian packages."""
        sources = await self._fetch_sources_index()

        results = []
        query_lower = query.lower()

        for name, info in sources.items():
            if query_lower in name.lower() or query_lower in info.get("Description", "").lower():
                results.append({
                    "name": name,
                    "version": info.get("Version"),
                    "description": info.get("Description", "").split("\n")[0],
                    "section": info.get("Section"),
                    "maintainer": info.get("Maintainer"),
                    "homepage": info.get("Homepage"),
                    "distribution": self.distribution,
                    "suite": self.suite,
                })

                if len(results) >= limit:
                    break

        return results

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get package metadata."""
        sources = await self._fetch_sources_index()

        if identifier not in sources:
            return {}

        info = sources[identifier]
        return {
            "name": identifier,
            "version": info.get("Version"),
            "description": info.get("Description"),
            "section": info.get("Section"),
            "maintainer": info.get("Maintainer"),
            "homepage": info.get("Homepage"),
            "vcs_git": info.get("Vcs-Git"),
            "vcs_browser": info.get("Vcs-Browser"),
            "distribution": self.distribution,
            "suite": self.suite,
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
