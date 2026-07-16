from __future__ import annotations

from collections import deque
from html import unescape
import posixpath
from typing import Iterable, Set
from urllib.parse import parse_qsl, urlencode, urljoin, urlparse, urlunparse
from urllib.robotparser import RobotFileParser

import httpx
from bs4 import BeautifulSoup

from ..models import HarvestDocument
from .base import BaseFetcher


class HtmlDocsFetcher(BaseFetcher):
    def __init__(
        self,
        source_name: str,
        client: httpx.Client,
        seeds: list[str],
        allow_domains: list[str],
        max_pages: int,
        respect_robots: bool = True,
        dedupe_ignore_query_params: list[str] | None = None,
    ) -> None:
        super().__init__(source_name, client)
        self.seeds = [self._normalize_url(seed) for seed in seeds]
        self.allow_domains = set(allow_domains)
        self.max_pages = max_pages
        self.respect_robots = respect_robots
        self.dedupe_ignore_query_params = {
            item.lower() for item in (dedupe_ignore_query_params or ["utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content", "fbclid", "gclid", "ref"])
        }
        self._robots_cache: dict[str, RobotFileParser] = {}

    def fetch(self) -> Iterable[HarvestDocument]:
        queue = deque(self.seeds)
        seen: Set[str] = set()
        count = 0

        while queue and count < self.max_pages:
            url = self._normalize_url(queue.popleft())
            if url in seen:
                continue
            seen.add(url)

            if not self._is_allowed(url) or not self._is_fetch_allowed(url):
                continue

            response = self.client.get(url)
            if response.status_code >= 400:
                continue

            html = response.text
            canonical_url = self._canonicalize_from_html(url, html)
            if canonical_url and canonical_url != url:
                canonical_url = self._normalize_url(canonical_url)
                if canonical_url in seen:
                    continue
                if not self._is_allowed(canonical_url) or not self._is_fetch_allowed(canonical_url):
                    continue
                seen.add(canonical_url)
                url = canonical_url

            doc = self._extract_document(url, html)
            if doc is not None:
                yield doc
                count += 1

            for link in self._extract_links(url, html):
                if link not in seen:
                    queue.append(link)

    def _is_allowed(self, url: str) -> bool:
        host = urlparse(url).netloc.lower()
        return any(host == d or host.endswith("." + d) for d in self.allow_domains)

    def _is_fetch_allowed(self, url: str) -> bool:
        if not self.respect_robots:
            return True
        parsed = urlparse(url)
        robots_url = f"{parsed.scheme}://{parsed.netloc}/robots.txt"
        parser = self._robots_cache.get(robots_url)
        if parser is None:
            parser = RobotFileParser()
            parser.set_url(robots_url)
            try:
                parser.read()
            except Exception:
                return False
            self._robots_cache[robots_url] = parser
        return bool(parser.can_fetch(self.client.headers.get("User-Agent", "*"), url))

    def _normalize_url(self, url: str) -> str:
        parsed = urlparse(url)
        if parsed.scheme not in {"http", "https"}:
            return url

        scheme = parsed.scheme.lower()
        host = parsed.hostname.lower() if parsed.hostname else ""
        port = parsed.port
        default_port = (scheme == "http" and port == 80) or (scheme == "https" and port == 443)
        if port and not default_port:
            netloc = f"{host}:{port}"
        else:
            netloc = host

        path = parsed.path or "/"
        path = posixpath.normpath(path)
        if not path.startswith("/"):
            path = "/" + path
        if parsed.path.endswith("/") and not path.endswith("/"):
            path += "/"

        filtered_query = [
            (key, value)
            for key, value in parse_qsl(parsed.query, keep_blank_values=True)
            if key.lower() not in self.dedupe_ignore_query_params
        ]
        query = urlencode(filtered_query, doseq=True)

        return urlunparse((scheme, netloc, path, "", query, ""))

    def _canonicalize_from_html(self, base_url: str, html: str) -> str | None:
        soup = BeautifulSoup(html, "html.parser")
        canonical = soup.find("link", rel=lambda value: value and "canonical" in str(value).lower(), href=True)
        if canonical is None:
            return None
        return urljoin(base_url, canonical["href"].strip())

    def _extract_document(self, url: str, html: str) -> HarvestDocument | None:
        soup = BeautifulSoup(html, "html.parser")
        title = (soup.title.string or "").strip() if soup.title else self._safe_title_from_url(url)

        for bad in soup(["script", "style", "nav", "footer"]):
            bad.extract()

        text = unescape("\n".join(line.strip() for line in soup.get_text("\n").splitlines() if line.strip()))
        if not text:
            return None

        return HarvestDocument(
            source=self.source_name,
            url=url,
            title=title,
            content_raw=html,
            content_clean=text,
            metadata={"kind": "html_docs", "normalized_url": url},
        )

    def _extract_links(self, base_url: str, html: str) -> Iterable[str]:
        soup = BeautifulSoup(html, "html.parser")
        for anchor in soup.find_all("a", href=True):
            href = anchor["href"].strip()
            if not href or href.startswith("#"):
                continue
            absolute = self._normalize_url(urljoin(base_url, href))
            parsed = urlparse(absolute)
            if parsed.scheme in {"http", "https"} and self._is_allowed(absolute):
                yield absolute
