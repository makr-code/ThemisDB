"""
Stack Overflow Source Scraper

Scrapes code snippets from Stack Overflow using the public data dump or API.
"""

import asyncio
import html
import re
from datetime import datetime
from typing import Any, AsyncIterator, Optional

import aiohttp
import structlog

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

logger = structlog.get_logger(__name__)


class StackOverflowScraper(CodeSource):
    """
    Stack Overflow code snippet scraper.

    Uses the Stack Exchange API to fetch questions and answers with code.
    For bulk data, consider using the Stack Overflow data dump.
    """

    source_type = SourceType.LOCAL  # Custom type for SO

    # Language tags mapping
    LANGUAGE_TAGS = {
        "python": "python",
        "javascript": "javascript",
        "typescript": "typescript",
        "java": "java",
        "c#": "csharp",
        "c++": "cpp",
        "c": "c",
        "go": "go",
        "rust": "rust",
        "ruby": "ruby",
        "php": "php",
        "swift": "swift",
        "kotlin": "kotlin",
        "scala": "scala",
        "r": "r",
        "sql": "sql",
        "bash": "shell",
        "shell": "shell",
    }

    def __init__(
        self,
        config: Optional[SourceConfig] = None,
        api_key: Optional[str] = None,
        site: str = "stackoverflow",
    ):
        """
        Initialize Stack Overflow scraper.

        Args:
            config: Scraper configuration
            api_key: Stack Exchange API key (optional, increases rate limits)
            site: Stack Exchange site (default: stackoverflow)
        """
        super().__init__(config)
        self.api_key = api_key
        self.site = site
        self.api_url = "https://api.stackexchange.com/2.3"
        self._session: Optional[aiohttp.ClientSession] = None
        self._rate_limiter = asyncio.Semaphore(30)  # 30 requests per second with key

    async def _get_session(self) -> aiohttp.ClientSession:
        """Get or create HTTP session."""
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession()
        return self._session

    async def _api_request(
        self,
        endpoint: str,
        params: Optional[dict] = None,
    ) -> Optional[dict]:
        """Make a rate-limited API request."""
        async with self._rate_limiter:
            session = await self._get_session()
            url = f"{self.api_url}/{endpoint}"

            request_params = {
                "site": self.site,
                **(params or {}),
            }
            if self.api_key:
                request_params["key"] = self.api_key

            for attempt in range(self.config.max_retries):
                try:
                    async with session.get(url, params=request_params) as resp:
                        if resp.status == 200:
                            return await resp.json()
                        elif resp.status == 400:
                            data = await resp.json()
                            logger.error("API error", error=data.get("error_message"))
                            return None
                        else:
                            logger.warning("API error", status=resp.status)
                except aiohttp.ClientError as e:
                    logger.warning("Request failed", attempt=attempt, error=str(e))
                    await asyncio.sleep(2 ** attempt)

            return None

    def _extract_code_blocks(self, html_content: str) -> list[tuple[str, str]]:
        """Extract code blocks from HTML content."""
        code_blocks = []

        # Match <pre><code> blocks
        pattern = r'<pre[^>]*><code[^>]*(?:class="([^"]*)")?[^>]*>(.*?)</code></pre>'
        matches = re.findall(pattern, html_content, re.DOTALL | re.IGNORECASE)

        for lang_class, code in matches:
            # Decode HTML entities
            code = html.unescape(code)
            # Remove HTML tags
            code = re.sub(r'<[^>]+>', '', code)
            # Detect language from class
            language = "unknown"
            if lang_class:
                for tag, lang in self.LANGUAGE_TAGS.items():
                    if tag in lang_class.lower():
                        language = lang
                        break

            if code.strip():
                code_blocks.append((code.strip(), language))

        return code_blocks

    async def scrape_repository(
        self,
        identifier: str,
        version: Optional[str] = None,
    ) -> AsyncIterator[SourceFile]:
        """
        Scrape code snippets for a topic/tag from Stack Overflow.

        Args:
            identifier: Tag or search query (e.g., "python", "async python")
            version: Not used for SO

        Yields:
            SourceFile objects for each code snippet
        """
        logger.info("Scraping Stack Overflow", tag=identifier)

        page = 1
        file_count = 0
        max_pages = 10  # Limit to avoid too many API calls

        while page <= max_pages and file_count < self.config.max_files_per_repo:
            # Fetch questions with answers
            data = await self._api_request(
                "questions",
                params={
                    "tagged": identifier,
                    "order": "desc",
                    "sort": "votes",
                    "filter": "withbody",
                    "pagesize": 100,
                    "page": page,
                },
            )

            if not data or not data.get("items"):
                break

            for question in data["items"]:
                question_id = question["question_id"]
                title = question.get("title", "")
                tags = question.get("tags", [])
                score = question.get("score", 0)

                # Extract code from question body
                if question.get("body"):
                    for code, language in self._extract_code_blocks(question["body"]):
                        if len(code) < 50:  # Skip very short snippets
                            continue

                        file_count += 1
                        yield SourceFile(
                            id=f"so-q-{question_id}-{file_count}",
                            path=f"questions/{question_id}/question.{self._get_extension(language)}",
                            source_type=self.source_type,
                            content=code,
                            size_bytes=len(code.encode("utf-8")),
                            language=language,
                            repo=f"stackoverflow/{identifier}",
                            metadata={
                                "source": "stackoverflow",
                                "type": "question",
                                "question_id": question_id,
                                "title": title,
                                "tags": tags,
                                "score": score,
                                "url": f"https://stackoverflow.com/q/{question_id}",
                            },
                        )

                # Fetch answers for this question
                answers_data = await self._api_request(
                    f"questions/{question_id}/answers",
                    params={
                        "order": "desc",
                        "sort": "votes",
                        "filter": "withbody",
                    },
                )

                if answers_data and answers_data.get("items"):
                    for answer in answers_data["items"]:
                        answer_id = answer["answer_id"]
                        answer_score = answer.get("score", 0)
                        is_accepted = answer.get("is_accepted", False)

                        if answer.get("body"):
                            for code, language in self._extract_code_blocks(answer["body"]):
                                if len(code) < 50:
                                    continue

                                file_count += 1
                                yield SourceFile(
                                    id=f"so-a-{answer_id}-{file_count}",
                                    path=f"questions/{question_id}/answer_{answer_id}.{self._get_extension(language)}",
                                    source_type=self.source_type,
                                    content=code,
                                    size_bytes=len(code.encode("utf-8")),
                                    language=language,
                                    repo=f"stackoverflow/{identifier}",
                                    metadata={
                                        "source": "stackoverflow",
                                        "type": "answer",
                                        "question_id": question_id,
                                        "answer_id": answer_id,
                                        "title": title,
                                        "tags": tags,
                                        "score": answer_score,
                                        "is_accepted": is_accepted,
                                        "url": f"https://stackoverflow.com/a/{answer_id}",
                                    },
                                )

                                if file_count >= self.config.max_files_per_repo:
                                    break

            if not data.get("has_more"):
                break
            page += 1

        logger.info("Scraping complete", tag=identifier, snippets=file_count)

    def _get_extension(self, language: str) -> str:
        """Get file extension for a language."""
        ext_map = {
            "python": "py",
            "javascript": "js",
            "typescript": "ts",
            "java": "java",
            "csharp": "cs",
            "cpp": "cpp",
            "c": "c",
            "go": "go",
            "rust": "rs",
            "ruby": "rb",
            "php": "php",
            "swift": "swift",
            "kotlin": "kt",
            "scala": "scala",
            "r": "r",
            "sql": "sql",
            "shell": "sh",
        }
        return ext_map.get(language, "txt")

    async def search(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 20,
    ) -> list[dict[str, Any]]:
        """Search Stack Overflow questions."""
        params = {
            "order": "desc",
            "sort": "relevance",
            "intitle": query,
            "pagesize": min(limit, 100),
        }

        if language:
            params["tagged"] = language

        data = await self._api_request("search/advanced", params=params)

        if not data:
            return []

        return [
            {
                "question_id": q["question_id"],
                "title": q.get("title"),
                "tags": q.get("tags", []),
                "score": q.get("score", 0),
                "answer_count": q.get("answer_count", 0),
                "is_answered": q.get("is_answered", False),
                "url": q.get("link"),
                "creation_date": q.get("creation_date"),
            }
            for q in data.get("items", [])[:limit]
        ]

    async def get_metadata(self, identifier: str) -> dict[str, Any]:
        """Get tag metadata."""
        data = await self._api_request(
            f"tags/{identifier}/info",
        )

        if not data or not data.get("items"):
            return {}

        tag = data["items"][0]
        return {
            "name": tag.get("name"),
            "count": tag.get("count", 0),
            "has_synonyms": tag.get("has_synonyms", False),
            "is_moderator_only": tag.get("is_moderator_only", False),
            "is_required": tag.get("is_required", False),
        }

    async def close(self) -> None:
        """Close the HTTP session."""
        if self._session and not self._session.closed:
            await self._session.close()
