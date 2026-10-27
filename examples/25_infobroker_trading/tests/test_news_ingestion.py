from datetime import datetime
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

import requests

from models import NewsItem
from news_ingestion import (
    NewsFeedSource,
    fetch_global_news,
    filter_sources_by_region,
    write_news_csv,
)


RSS_SAMPLE = b"""<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0">
  <channel>
    <title>Test Feed</title>
    <item>
      <title>Policy update affects exports</title>
      <description>State decision impacts logistics and manufacturing.</description>
      <pubDate>Mon, 07 Oct 2024 09:00:00 GMT</pubDate>
    </item>
  </channel>
</rss>
"""

ATOM_SAMPLE = b"""<?xml version="1.0" encoding="utf-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
  <entry>
    <title>Weather disruption in shipping lane</title>
    <summary>Storms delay cargo and increase freight rates.</summary>
    <updated>2024-10-08T10:00:00Z</updated>
  </entry>
</feed>
"""


class _MockResponse:
    def __init__(self, content: bytes, status_code: int = 200) -> None:
        self.content = content
        self.status_code = status_code

    def raise_for_status(self) -> None:
        if self.status_code >= 400:
            raise RuntimeError("http error")


class NewsIngestionTests(unittest.TestCase):
    @patch("news_ingestion.requests.get")
    def test_fetch_global_news_parses_rss_and_atom(self, mock_get) -> None:
        mock_get.side_effect = [_MockResponse(RSS_SAMPLE), _MockResponse(ATOM_SAMPLE)]
        sources = [
            NewsFeedSource("s1", "Feed1", "global", "https://example.com/rss.xml", 0.9),
            NewsFeedSource("s2", "Feed2", "apac", "https://example.com/atom.xml", 0.85),
        ]
        items = fetch_global_news(sources, max_items_per_source=5)
        self.assertEqual(len(items), 2)
        self.assertEqual(items[0].symbol, "GLOBAL")
        self.assertEqual(items[0].exchange, "WORLD")
        self.assertIsInstance(items[0].timestamp, datetime)

    @patch("news_ingestion.requests.get")
    def test_fetch_global_news_raises_when_all_sources_fail(self, mock_get) -> None:
        mock_get.side_effect = requests.RequestException("network down")
        sources = [NewsFeedSource("s1", "Feed1", "global", "https://example.com/rss.xml", 0.9)]
        with self.assertRaises(RuntimeError):
            fetch_global_news(sources, max_items_per_source=5)

    def test_filter_sources_by_region(self) -> None:
        sources = [
            NewsFeedSource("eu", "EU Feed", "europe", "https://example.com/eu.xml", 0.8),
            NewsFeedSource("ap", "APAC Feed", "apac", "https://example.com/ap.xml", 0.8),
        ]
        filtered = filter_sources_by_region(sources, ["apac"])
        self.assertEqual(len(filtered), 1)
        self.assertEqual(filtered[0].id, "ap")

    def test_write_news_csv_outputs_contract_columns(self) -> None:
        news_items = [
            NewsItem(
                id="n1",
                timestamp=datetime.fromisoformat("2024-10-07T09:00:00+00:00"),
                source="EU Feed",
                symbol="GLOBAL",
                exchange="WORLD",
                headline="Sample headline",
                body="Sample body",
                source_reliability=0.8,
            )
        ]
        with tempfile.TemporaryDirectory() as tmpdir:
            output = Path(tmpdir) / "news.csv"
            write_news_csv(str(output), news_items)
            content = output.read_text(encoding="utf-8")
            self.assertIn("id,timestamp,source,symbol,exchange,headline,body,source_reliability", content)
            self.assertIn("Sample headline", content)


if __name__ == "__main__":
    unittest.main()
