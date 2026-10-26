"""Global news source ingestion for the infobroker example."""

from __future__ import annotations

import csv
import hashlib
from dataclasses import dataclass
from datetime import datetime, timezone
from email.utils import parsedate_to_datetime
from typing import Iterable, List, Sequence
from xml.etree import ElementTree as ET

import requests

from models import NewsItem


@dataclass(frozen=True)
class NewsFeedSource:
    """Declarative feed source configuration."""

    id: str
    name: str
    region: str
    url: str
    reliability: float


def default_global_sources() -> List[NewsFeedSource]:
    """Returns curated global RSS/Atom feeds for broad coverage."""

    return [
        NewsFeedSource("bbc_world", "BBC World", "global", "https://feeds.bbci.co.uk/news/world/rss.xml", 0.92),
        NewsFeedSource("dw_world", "DW World", "europe", "https://rss.dw.com/rdf/rss-en-world", 0.87),
        NewsFeedSource("france24_world", "France24 World", "europe", "https://www.france24.com/en/rss", 0.86),
        NewsFeedSource("aljazeera", "Al Jazeera", "mena", "https://www.aljazeera.com/xml/rss/all.xml", 0.84),
        NewsFeedSource("japantimes", "The Japan Times", "apac", "https://www.japantimes.co.jp/feed/", 0.83),
        NewsFeedSource("str_times_world", "Straits Times World", "apac", "https://www.straitstimes.com/news/world/rss.xml", 0.82),
        NewsFeedSource("cbc_world", "CBC World", "americas", "https://www.cbc.ca/cmlink/rss-world", 0.84),
        NewsFeedSource("abc_au_world", "ABC Australia World", "apac", "https://www.abc.net.au/news/feed/45910/rss.xml", 0.84),
    ]


def _strip_tag(tag: str) -> str:
    if "}" in tag:
        return tag.split("}", 1)[1]
    return tag


def _text_or_empty(elem: ET.Element | None) -> str:
    if elem is None or elem.text is None:
        return ""
    return elem.text.strip()


def _parse_timestamp(value: str) -> datetime:
    if not value:
        return datetime.now(timezone.utc)
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        pass
    try:
        dt = parsedate_to_datetime(value)
        if dt.tzinfo is None:
            return dt.replace(tzinfo=timezone.utc)
        return dt.astimezone(timezone.utc)
    except (TypeError, ValueError):
        return datetime.now(timezone.utc)


def _build_news_id(source_id: str, title: str, ts: datetime) -> str:
    digest = hashlib.sha1(f"{source_id}|{title}|{ts.isoformat()}".encode("utf-8")).hexdigest()[:12]
    return f"{source_id}_{digest}"


def _parse_rss_items(root: ET.Element, source: NewsFeedSource, max_items: int) -> List[NewsItem]:
    items: List[NewsItem] = []
    channel = root.find("channel")
    if channel is None:
        return items

    for item in channel.findall("item")[:max_items]:
        title = _text_or_empty(item.find("title")) or "untitled"
        description = _text_or_empty(item.find("description"))
        published = _parse_timestamp(_text_or_empty(item.find("pubDate")))
        items.append(
            NewsItem(
                id=_build_news_id(source.id, title, published),
                timestamp=published,
                source=source.name,
                symbol="GLOBAL",
                exchange="WORLD",
                headline=title,
                body=description,
                source_reliability=source.reliability,
            )
        )
    return items


def _parse_atom_items(root: ET.Element, source: NewsFeedSource, max_items: int) -> List[NewsItem]:
    items: List[NewsItem] = []
    ns = "{http://www.w3.org/2005/Atom}"
    for entry in root.findall(f"{ns}entry")[:max_items]:
        title = _text_or_empty(entry.find(f"{ns}title")) or "untitled"
        summary = _text_or_empty(entry.find(f"{ns}summary"))
        content = _text_or_empty(entry.find(f"{ns}content"))
        published = _parse_timestamp(
            _text_or_empty(entry.find(f"{ns}published")) or _text_or_empty(entry.find(f"{ns}updated"))
        )
        items.append(
            NewsItem(
                id=_build_news_id(source.id, title, published),
                timestamp=published,
                source=source.name,
                symbol="GLOBAL",
                exchange="WORLD",
                headline=title,
                body=summary or content,
                source_reliability=source.reliability,
            )
        )
    return items


def _parse_feed_xml(xml_bytes: bytes, source: NewsFeedSource, max_items: int) -> List[NewsItem]:
    root = ET.fromstring(xml_bytes)
    root_tag = _strip_tag(root.tag).lower()
    if root_tag == "rss":
        return _parse_rss_items(root, source, max_items)
    if root_tag == "feed":
        return _parse_atom_items(root, source, max_items)
    raise ValueError(f"unsupported feed format for source {source.id}: {root.tag}")


def fetch_global_news(
    sources: Sequence[NewsFeedSource],
    max_items_per_source: int = 20,
    timeout_sec: int = 15,
) -> List[NewsItem]:
    """Fetches and parses a set of global RSS/Atom feeds."""

    if max_items_per_source < 1:
        raise ValueError("max_items_per_source must be >= 1")
    fetched: List[NewsItem] = []
    seen: set[tuple[str, str, datetime]] = set()
    failures: List[str] = []

    for source in sources:
        try:
            response = requests.get(source.url, timeout=timeout_sec)
            response.raise_for_status()
            parsed = _parse_feed_xml(response.content, source, max_items_per_source)
            for item in parsed:
                key = (item.source, item.headline, item.timestamp)
                if key in seen:
                    continue
                seen.add(key)
                fetched.append(item)
        except (requests.RequestException, ET.ParseError, ValueError) as exc:
            failures.append(f"{source.id}: {exc}")

    if not fetched:
        failure_report = "; ".join(failures) if failures else "no feed items parsed"
        raise RuntimeError(f"global news ingestion failed: {failure_report}")
    return sorted(fetched, key=lambda item: item.timestamp)


def write_news_csv(path: str, news_items: Iterable[NewsItem]) -> None:
    """Writes normalized news records to the repository's CSV contract."""

    with open(path, "w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=["id", "timestamp", "source", "symbol", "exchange", "headline", "body", "source_reliability"],
        )
        writer.writeheader()
        for item in news_items:
            writer.writerow(
                {
                    "id": item.id,
                    "timestamp": item.timestamp.isoformat().replace("+00:00", "Z"),
                    "source": item.source,
                    "symbol": item.symbol,
                    "exchange": item.exchange,
                    "headline": item.headline,
                    "body": item.body,
                    "source_reliability": f"{item.source_reliability:.2f}",
                }
            )


def filter_sources_by_region(sources: Sequence[NewsFeedSource], regions: Sequence[str]) -> List[NewsFeedSource]:
    """Filters source catalog by region values."""

    if not regions:
        return list(sources)
    allowed = {region.strip().lower() for region in regions if region.strip()}
    return [source for source in sources if source.region.lower() in allowed]
