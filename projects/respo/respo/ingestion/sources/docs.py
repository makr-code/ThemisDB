"""
Documentation Scrapers

Scrapers for programming language documentation, API references, and syntax guides.
"""

import asyncio
import html
import re
from typing import Any, AsyncIterator, Optional
from urllib.parse import urljoin, urlparse

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class DevDocsScraper(CodeSource):
    """
    DevDocs.io scraper.

    DevDocs aggregates documentation for many languages and frameworks.
    Uses the DevDocs API to fetch documentation.
    """

    source_type = SourceType.LOCAL

    # Available documentation sets
    DOCSETS = {
        "python": "python~3.12",
        "javascript": "javascript",
        "typescript": "typescript",
        "react": "react",
        "vue": "vue~3",
        "node": "node",
        "express": "express",
        "django": "django~4.2",
        "flask": "flask~3.0",
        "fastapi": "fastapi",
        "rust": "rust",
        "go": "go",
        "java": "openjdk~21",
        "kotlin": "kotlin~1.9",
        "swift": "swift~5.9",
        "php": "php",
        "ruby": "ruby~3.3",
        "rails": "rails~7.1",
        "css": "css",
        "html": "html",
        "sql": "postgresql~16",
        "redis": "redis",
        "docker": "docker",
        "kubernetes": "kubernetes",
        "terraform": "terraform",
        "git": "git",
        "bash": "bash",
        "c": "c",
        "cpp": "cpp",
    }

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize DevDocs scraper."""
        super().__init__(config)
        self.api_url = "https://devdocs.io"
        self.docs_url = "https://documents.devdocs.io"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(5)

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _fetch_json(self, url: str) -> Optional[dict]:
        """Fetch JSON data."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception as e:
                logger.warning("Fetch failed", url=url, error=str(e))
                return None

    async def _fetch_text(self, url: str) -> Optional[str]:
        """Fetch text content."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.text()
                    return None
            except Exception as e:
                logger.warning("Fetch failed", url=url, error=str(e))
                return None

    def _extract_code_and_text(self, html_content: str) -> tuple[str, list[str]]:
        """Extract text content and code blocks from HTML."""
        # Extract code blocks
        code_blocks = []
        code_pattern = r'<pre[^>]*><code[^>]*>(.*?)</code></pre>'
        for match in re.finditer(code_pattern, html_content, re.DOTALL):
            code = html.unescape(match.group(1))
            code = re.sub(r'<[^>]+>', '', code)
            if code.strip():
                code_blocks.append(code.strip())

        # Extract text content - use html.parser for safety
        # Note: For production, consider using BeautifulSoup for more robust parsing
        # This regex-based approach is sufficient for documentation extraction
        # where we control the input source (DevDocs API)
        import html.parser
        
        class TextExtractor(html.parser.HTMLParser):
            def __init__(self):
                super().__init__()
                self.text_parts = []
                self.skip_tags = {'script', 'style'}
                self.current_skip = None
            
            def handle_starttag(self, tag, attrs):
                if tag.lower() in self.skip_tags:
                    self.current_skip = tag.lower()
            
            def handle_endtag(self, tag):
                if tag.lower() == self.current_skip:
                    self.current_skip = None
            
            def handle_data(self, data):
                if self.current_skip is None:
                    self.text_parts.append(data)
        
        extractor = TextExtractor()
        try:
            extractor.feed(html_content)
            text = ' '.join(extractor.text_parts)
        except Exception:
            # Fallback to simple tag removal
            text = re.sub(r'<[^>]+>', ' ', html_content)
        
        text = html.unescape(text)
        text = re.sub(r'\s+', ' ', text).strip()

        return text, code_blocks

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape documentation for a language/framework.

        Args:
            identifier: Language or framework name (e.g., "python", "react")
            version: Not used

        Yields:
            SourceFile objects for each documentation page
        """
        docset = self.DOCSETS.get(identifier.lower(), identifier)
        logger.info("Scraping DevDocs", docset=docset)

        # Fetch index
        index_url = f"{self.docs_url}/{docset}/index.json"
        index = await self._fetch_json(index_url)

        if not index:
            logger.error("Failed to fetch index", docset=docset)
            return

        entries = index.get("entries", [])
        file_count = 0

        for entry in entries:
            if file_count >= self.config.max_files_per_repo:
                break

            name = entry.get("name", "")
            path = entry.get("path", "")
            entry_type = entry.get("type", "")

            # Fetch page content
            page_url = f"{self.docs_url}/{docset}/{path}.html"
            html_content = await self._fetch_text(page_url)

            if not html_content:
                continue

            text, code_blocks = self._extract_code_and_text(html_content)

            if not text and not code_blocks:
                continue

            file_count += 1

            # Create combined content with structure
            content_parts = [
                f"# {name}",
                f"Type: {entry_type}",
                "",
                "## Description",
                text[:2000] if len(text) > 2000 else text,
            ]

            if code_blocks:
                content_parts.append("")
                content_parts.append("## Code Examples")
                for i, code in enumerate(code_blocks[:5], 1):
                    content_parts.append(f"\n### Example {i}")
                    content_parts.append(f"```\n{code}\n```")

            content = "\n".join(content_parts)

            yield SourceFile(
                id=SourceFile.generate_id(self.source_type, f"devdocs:{docset}", path),
                path=f"{docset}/{path}.md",
                source_type=self.source_type,
                content=content,
                size_bytes=len(content.encode("utf-8")),
                language=identifier.lower(),
                repo=f"devdocs:{docset}",
                metadata={
                    "source": "devdocs",
                    "docset": docset,
                    "name": name,
                    "type": entry_type,
                    "url": f"{self.api_url}/{docset}/{path}",
                    "code_examples": len(code_blocks),
                },
            )

        logger.info("Scraping complete", docset=docset, pages=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """List available docsets matching query."""
        results = []
        query_lower = query.lower()

        for name, docset in self.DOCSETS.items():
            if query_lower in name or query_lower in docset:
                results.append({
                    "name": name,
                    "docset": docset,
                    "url": f"{self.api_url}/{docset}/",
                })

        return results[:limit]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get docset metadata."""
        docset = self.DOCSETS.get(identifier.lower(), identifier)
        index_url = f"{self.docs_url}/{docset}/index.json"
        index = await self._fetch_json(index_url)

        if not index:
            return {}

        return {
            "docset": docset,
            "entries": len(index.get("entries", [])),
            "types": list(set(e.get("type", "") for e in index.get("entries", []))),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()


class CheatShScraper(CodeSource):
    """
    cheat.sh scraper.

    cheat.sh provides concise cheatsheets for many commands and languages.
    Perfect for quick syntax reference.
    """

    source_type = SourceType.LOCAL

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize cheat.sh scraper."""
        super().__init__(config)
        self.base_url = "https://cheat.sh"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(2)  # Be nice to cheat.sh

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession(
                headers={"User-Agent": "curl/7.68.0"}  # cheat.sh expects curl
            )
        return self._session

    async def _fetch(self, path: str) -> Optional[str]:
        """Fetch cheatsheet content."""
        async with self._rate_limiter:
            session = await self._get_session()
            url = f"{self.base_url}/{path}"

            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.text()
                    return None
            except Exception as e:
                logger.warning("Fetch failed", path=path, error=str(e))
                return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape cheatsheets for a language or command.

        Args:
            identifier: Language or command (e.g., "python", "git", "tar")
            version: Not used

        Yields:
            SourceFile objects for each cheatsheet
        """
        logger.info("Scraping cheat.sh", topic=identifier)

        # Fetch the main cheatsheet
        content = await self._fetch(f"{identifier}/:learn")

        if content:
            # Remove ANSI escape codes
            content = re.sub(r'\x1b\[[0-9;]*m', '', content)

            yield SourceFile(
                id=SourceFile.generate_id(self.source_type, "cheat.sh", f"{identifier}/learn"),
                path=f"{identifier}/learn.txt",
                source_type=self.source_type,
                content=content,
                size_bytes=len(content.encode("utf-8")),
                language=identifier.lower(),
                repo="cheat.sh",
                metadata={
                    "source": "cheat.sh",
                    "topic": identifier,
                    "type": "learn",
                    "url": f"{self.base_url}/{identifier}/:learn",
                },
            )

        # Fetch list of available sheets
        list_content = await self._fetch(f"{identifier}/:list")

        if list_content:
            sheets = [s.strip() for s in list_content.split("\n") if s.strip()]

            for sheet in sheets[:50]:  # Limit sheets per topic
                sheet_content = await self._fetch(f"{identifier}/{sheet}")

                if not sheet_content:
                    continue

                # Remove ANSI codes
                sheet_content = re.sub(r'\x1b\[[0-9;]*m', '', sheet_content)

                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, "cheat.sh", f"{identifier}/{sheet}"),
                    path=f"{identifier}/{sheet}.txt",
                    source_type=self.source_type,
                    content=sheet_content,
                    size_bytes=len(sheet_content.encode("utf-8")),
                    language=identifier.lower(),
                    repo="cheat.sh",
                    metadata={
                        "source": "cheat.sh",
                        "topic": identifier,
                        "sheet": sheet,
                        "url": f"{self.base_url}/{identifier}/{sheet}",
                    },
                )

        logger.info("Scraping complete", topic=identifier)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search cheat.sh."""
        # cheat.sh doesn't have a search API, fetch the main page
        content = await self._fetch(f"{query}")

        if not content:
            return []

        return [{
            "query": query,
            "url": f"{self.base_url}/{query}",
            "has_content": bool(content),
        }]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get topic metadata."""
        list_content = await self._fetch(f"{identifier}/:list")

        sheets = []
        if list_content:
            sheets = [s.strip() for s in list_content.split("\n") if s.strip()]

        return {
            "topic": identifier,
            "sheets": len(sheets),
            "url": f"{self.base_url}/{identifier}/",
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()


class TLDRScraper(CodeSource):
    """
    tldr-pages scraper.

    tldr-pages provides simplified, community-driven man pages.
    """

    source_type = SourceType.GITHUB

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize tldr scraper."""
        super().__init__(config)
        self.api_url = "https://api.github.com"
        self.raw_url = "https://raw.githubusercontent.com/tldr-pages/tldr/main/pages"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(10)

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            headers = {"Accept": "application/vnd.github+json"}
            if self.config.token:
                headers["Authorization"] = f"Bearer {self.config.token}"
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session

    async def _fetch(self, url: str) -> Optional[str]:
        """Fetch content."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.text()
                    return None
            except Exception as e:
                logger.warning("Fetch failed", url=url, error=str(e))
                return None

    async def _fetch_json(self, url: str) -> Optional[Any]:
        """Fetch JSON."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception:
                return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape tldr pages for a platform.

        Args:
            identifier: Platform (common, linux, osx, windows) or "all"
            version: Not used

        Yields:
            SourceFile objects for each tldr page
        """
        logger.info("Scraping tldr-pages", platform=identifier)

        platforms = ["common", "linux", "osx", "windows"] if identifier == "all" else [identifier]
        file_count = 0

        for platform in platforms:
            # Get directory listing from GitHub API
            tree_url = f"{self.api_url}/repos/tldr-pages/tldr/contents/pages/{platform}"
            files = await self._fetch_json(tree_url)

            if not files:
                continue

            for file_info in files:
                if not file_info["name"].endswith(".md"):
                    continue

                if file_count >= self.config.max_files_per_repo:
                    break

                # Fetch content
                content_url = f"{self.raw_url}/{platform}/{file_info['name']}"
                content = await self._fetch(content_url)

                if not content:
                    continue

                command = file_info["name"].replace(".md", "")
                file_count += 1

                yield SourceFile(
                    id=SourceFile.generate_id(self.source_type, "tldr", f"{platform}/{command}"),
                    path=f"{platform}/{command}.md",
                    source_type=self.source_type,
                    content=content,
                    size_bytes=len(content.encode("utf-8")),
                    language="markdown",
                    repo="tldr-pages/tldr",
                    metadata={
                        "source": "tldr",
                        "command": command,
                        "platform": platform,
                        "url": f"https://tldr.sh/{command}",
                    },
                )

        logger.info("Scraping complete", platform=identifier, pages=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search tldr pages."""
        # Search across platforms
        results = []
        platforms = ["common", "linux", "osx", "windows"]

        for platform in platforms:
            tree_url = f"{self.api_url}/repos/tldr-pages/tldr/contents/pages/{platform}"
            files = await self._fetch_json(tree_url)

            if not files:
                continue

            for file_info in files:
                name = file_info["name"].replace(".md", "")
                if query.lower() in name.lower():
                    results.append({
                        "command": name,
                        "platform": platform,
                        "url": f"https://tldr.sh/{name}",
                    })

                    if len(results) >= limit:
                        return results

        return results

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get platform metadata."""
        tree_url = f"{self.api_url}/repos/tldr-pages/tldr/contents/pages/{identifier}"
        files = await self._fetch_json(tree_url)

        return {
            "platform": identifier,
            "pages": len(files) if files else 0,
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()


class MDNScraper(CodeSource):
    """
    MDN Web Docs scraper.

    Scrapes JavaScript, CSS, HTML, and Web API documentation from MDN.
    """

    source_type = SourceType.LOCAL

    # MDN content areas
    AREAS = {
        "javascript": "Web/JavaScript",
        "css": "Web/CSS",
        "html": "Web/HTML",
        "webapi": "Web/API",
        "http": "Web/HTTP",
        "svg": "Web/SVG",
        "mathml": "Web/MathML",
    }

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
    ):
        """Initialize MDN scraper."""
        super().__init__(config)
        self.api_url = "https://developer.mozilla.org"
        self.content_url = "https://raw.githubusercontent.com/mdn/content/main/files/en-us"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(5)

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _fetch(self, url: str) -> Optional[str]:
        """Fetch content."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.text()
                    return None
            except Exception as e:
                logger.warning("Fetch failed", url=url, error=str(e))
                return None

    async def _fetch_json(self, url: str) -> Optional[Any]:
        """Fetch JSON."""
        async with self._rate_limiter:
            session = await self._get_session()
            try:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    return None
            except Exception:
                return None

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape MDN documentation for a topic.

        Args:
            identifier: Topic (javascript, css, html, webapi)
            version: Not used

        Yields:
            SourceFile objects for each documentation page
        """
        area = self.AREAS.get(identifier.lower())
        if not area:
            logger.error("Unknown MDN area", identifier=identifier)
            return

        logger.info("Scraping MDN", area=area)

        # Use MDN's sitemap or index
        # For now, fetch from GitHub content repo
        api_url = f"https://api.github.com/repos/mdn/content/contents/files/en-us/{area.lower()}"

        async def fetch_directory(path: str, depth: int = 0) -> AsyncIterator[SourceFile]:
            if depth > 3:  # Limit depth
                return

            files = await self._fetch_json(f"https://api.github.com/repos/mdn/content/contents/{path}")
            if not files:
                return

            for item in files:
                if item["type"] == "dir":
                    async for sf in fetch_directory(item["path"], depth + 1):
                        yield sf
                elif item["name"] == "index.md":
                    content = await self._fetch(item["download_url"])
                    if not content:
                        continue

                    # Extract title from frontmatter
                    title_match = re.search(r'^title:\s*(.+)$', content, re.MULTILINE)
                    title = title_match.group(1).strip('"\'') if title_match else item["path"]

                    # Extract slug
                    slug_match = re.search(r'^slug:\s*(.+)$', content, re.MULTILINE)
                    slug = slug_match.group(1) if slug_match else ""

                    yield SourceFile(
                        id=SourceFile.generate_id(self.source_type, "mdn", item["path"]),
                        path=item["path"].replace("files/en-us/", ""),
                        source_type=self.source_type,
                        content=content,
                        size_bytes=len(content.encode("utf-8")),
                        language="markdown",
                        repo="mdn/content",
                        metadata={
                            "source": "mdn",
                            "title": title,
                            "slug": slug,
                            "area": identifier,
                            "url": f"{self.api_url}/en-US/docs/{slug}",
                        },
                    )

        file_count = 0
        async for sf in fetch_directory(f"files/en-us/{area.lower()}"):
            yield sf
            file_count += 1
            if file_count >= self.config.max_files_per_repo:
                break

        logger.info("Scraping complete", area=area, pages=file_count)

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search MDN."""
        search_url = f"{self.api_url}/api/v1/search"
        params = {"q": query, "size": limit}

        if language:
            params["q"] = f"{query} {language}"

        data = await self._fetch_json(f"{search_url}?q={query}&size={limit}")

        if not data:
            return []

        return [
            {
                "title": doc.get("title"),
                "slug": doc.get("slug"),
                "url": f"{self.api_url}/en-US/docs/{doc.get('slug')}",
                "summary": doc.get("summary"),
            }
            for doc in data.get("documents", [])
        ]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get area metadata."""
        area = self.AREAS.get(identifier.lower(), identifier)
        return {
            "area": identifier,
            "path": area,
            "url": f"{self.api_url}/en-US/docs/{area}",
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
