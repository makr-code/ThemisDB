#!/usr/bin/env python3
"""Measure ThemisDB entity writes and validated readbacks for JSONL data.

The runner does not measure full-text search or recovery. Data generation is
synthetic and is kept separate from database measurement.
"""

import argparse
import asyncio
import json
import logging
import math
import random
import string
import time
import uuid
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Dict, Iterable, Iterator, List
from urllib.parse import urlsplit

import httpx


logging.basicConfig(level=logging.INFO, format="[%(asctime)s] %(levelname)s: %(message)s")
logger = logging.getLogger(__name__)


class WikipediaDataGenerator:
    """Generate an explicitly synthetic JSONL corpus for smoke/load input."""

    def __init__(self, output_dir: str = "data/wikipedia", *, seed: int = 42):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.random = random.Random(seed)
        self.titles = self._generate_titles()
        self.contributors = self._generate_contributors()

    def _generate_titles(self) -> List[str]:
        topics = [
            "History", "Science", "Technology", "Geography", "Biology",
            "Physics", "Chemistry", "Mathematics", "Literature", "Art",
            "Music", "Sports", "Medicine", "Philosophy", "Religion",
            "Politics", "Economics", "Law", "Education", "Culture",
        ]
        adjectives = [
            "The", "Ancient", "Modern", "Digital", "Global", "Historical",
            "Contemporary", "Classical", "Quantum", "Computational", "Evolutionary",
        ]
        return [
            f"{self.random.choice(adjectives)} {self.random.choice(topics)} "
            f"({self.random.randint(500, 2024)})"
            for _ in range(100000)
        ]

    def _generate_contributors(self) -> List[str]:
        first_names = [
            "John", "Mary", "Robert", "Patricia", "Michael", "Jennifer",
            "William", "Linda", "David", "Barbara", "Richard", "Susan",
        ]
        last_names = [
            "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia",
            "Miller", "Davis", "Rodriguez", "Martinez", "Hernandez", "Lopez",
        ]
        return [
            f"{self.random.choice(first_names)} {self.random.choice(last_names)}"
            for _ in range(10000)
        ]

    def generate_articles(self, count: int = 100000) -> Iterator[Dict]:
        if isinstance(count, bool) or not isinstance(count, int) or count <= 0:
            raise ValueError("article count must be a positive integer")
        base_time = datetime.now(timezone.utc) - timedelta(days=365 * 15)
        words = [
            "the", "be", "to", "of", "and", "a", "in", "that", "have", "it",
            "for", "not", "on", "with", "history", "known", "time", "country",
        ]
        for index in range(count):
            text_length = self.random.randint(500, 50000)
            text_words = []
            generated_length = 0
            while generated_length < text_length:
                word = self.random.choice(words)
                word = word.capitalize() if self.random.random() > 0.9 else word
                text_words.append(word)
                generated_length += len(word) + 1
            article = {
                "id": index,
                "title": self.titles[self.random.randrange(len(self.titles))],
                "namespace": 0,
                "revision_id": self.random.randint(1000000, 9999999),
                "timestamp": (
                    base_time + timedelta(hours=self.random.randint(0, 365 * 15 * 24))
                ).isoformat(),
                "contributor": self.contributors[self.random.randrange(len(self.contributors))],
                "contributor_id": self.random.randint(1, 100000),
                "comment": "Synthetic benchmark article",
                "text": " ".join(text_words),
                "categories": ["Synthetic benchmark data"],
            }
            article["size_bytes"] = len(article["text"].encode("utf-8"))
            article["word_count"] = len(text_words)
            yield article

    def save_as_jsonl(self, articles: Iterable[Dict], filename: str = "wikipedia_articles.jsonl"):
        output_file = self.output_dir / filename
        with output_file.open("x", encoding="utf-8") as output:
            article_count = 0
            for article in articles:
                output.write(json.dumps(article, ensure_ascii=False) + "\n")
                article_count += 1
        logger.info("Saved %s synthetic articles to %s", article_count, output_file)
        return output_file


class WikipediaStressTest:
    """Measure actual ThemisDB entity ingestion and read-after-write behavior."""

    def __init__(self, data_file: str, endpoint: str, *, dataset_kind: str,
                 max_articles: int = 1000, timeout: float = 10.0,
                 concurrency: int = 1):
        if dataset_kind not in {"synthetic", "wikipedia-dump"}:
            raise ValueError("dataset_kind must be synthetic or wikipedia-dump")
        if isinstance(max_articles, bool) or not isinstance(max_articles, int) or max_articles <= 0:
            raise ValueError("max_articles must be a positive integer")
        if isinstance(concurrency, bool) or not isinstance(concurrency, int) or concurrency <= 0:
            raise ValueError("concurrency must be a positive integer")
        parsed_endpoint = urlsplit(endpoint)
        if (parsed_endpoint.scheme not in {"http", "https"} or not parsed_endpoint.hostname or
                parsed_endpoint.username is not None or parsed_endpoint.password is not None or
                parsed_endpoint.path not in {"", "/"} or parsed_endpoint.query or parsed_endpoint.fragment):
            raise ValueError("endpoint must be an HTTP(S) origin without credentials, path, query or fragment")
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("timeout must be finite and positive")
        self.data_file = Path(data_file)
        self.endpoint = endpoint.rstrip("/")
        self.dataset_kind = dataset_kind
        self.max_articles = max_articles
        self.timeout = timeout
        self.concurrency = concurrency

    @staticmethod
    def _percentile(samples: List[float], percentile: float) -> float:
        if not samples:
            return 0.0
        ordered = sorted(samples)
        index = (len(ordered) - 1) * percentile / 100
        lower = int(index)
        upper = min(lower + 1, len(ordered) - 1)
        return ordered[lower] + (ordered[upper] - ordered[lower]) * (index - lower)

    def _load_articles(self):
        with self.data_file.open("r", encoding="utf-8") as source:
            for line_number, line in enumerate(source, start=1):
                if line_number > self.max_articles:
                    break
                try:
                    article = json.loads(line)
                except json.JSONDecodeError as error:
                    raise ValueError(f"Invalid JSONL at line {line_number}: {error}") from error
                if not isinstance(article, dict) or not isinstance(article.get("title"), str):
                    raise ValueError(f"Article at line {line_number} must be an object with a title")
                yield article

    async def run_stress_test(self, output_file: str):
        if not self.data_file.is_file():
            raise FileNotFoundError(f"Dataset does not exist: {self.data_file}")
        report_path = Path(output_file)
        if report_path.exists():
            raise FileExistsError(f"Report already exists: {report_path}")

        collection = "wiki_bench_" + uuid.uuid4().hex
        attempted_keys = []
        write_latencies = []
        read_latencies = []
        bytes_written = 0
        article_count = 0
        started = time.perf_counter()

        async with httpx.AsyncClient(
            base_url=self.endpoint, timeout=self.timeout, trust_env=False
        ) as client:
            try:
                articles = iter(self._load_articles())
                article_count = 0

                async def measure_article(index, article):
                    nonlocal bytes_written
                    key = f"article{index:020d}"
                    entity_key = f"{collection}:{key}"
                    serialized = json.dumps(
                        article, ensure_ascii=False, separators=(",", ":"), allow_nan=False
                    )
                    attempted_keys.append(entity_key)

                    operation_start = time.perf_counter()
                    response = await client.put(
                        f"/entities/{entity_key}", json={"blob": serialized}
                    )
                    response.raise_for_status()
                    if response.status_code not in (200, 201):
                        raise RuntimeError(f"ThemisDB rejected article write {index}")
                    if response.json().get("success") is not True:
                        raise RuntimeError(f"ThemisDB did not confirm article write {index}")
                    write_latencies.append((time.perf_counter() - operation_start) * 1000)
                    bytes_written += len(serialized.encode("utf-8"))

                    operation_start = time.perf_counter()
                    response = await client.get(f"/entities/{entity_key}")
                    response.raise_for_status()
                    payload = response.json()
                    if payload.get("key") != entity_key or payload.get("blob") != serialized:
                        raise RuntimeError(f"Read-after-write validation failed for article {index}")
                    read_latencies.append((time.perf_counter() - operation_start) * 1000)

                while True:
                    batch = []
                    for _ in range(self.concurrency):
                        try:
                            article = next(articles)
                        except StopIteration:
                            break
                        article_count += 1
                        batch.append((article_count, article))
                    if not batch:
                        break

                    outcomes = await asyncio.gather(
                        *(measure_article(index, article) for index, article in batch),
                        return_exceptions=True,
                    )
                    for outcome in outcomes:
                        if isinstance(outcome, BaseException):
                            raise outcome

                if article_count == 0:
                    raise ValueError(f"Dataset contains no articles: {self.data_file}")
            finally:
                cleanup_errors = []
                for offset in range(0, len(attempted_keys), self.concurrency):
                    batch = attempted_keys[offset:offset + self.concurrency]

                    async def delete_entity(entity_key):
                        response = await client.delete(f"/entities/{entity_key}")
                        if response.status_code != 404:
                            response.raise_for_status()

                    outcomes = await asyncio.gather(
                        *(delete_entity(entity_key) for entity_key in batch),
                        return_exceptions=True,
                    )
                    cleanup_errors.extend(
                        (batch[index], outcome)
                        for index, outcome in enumerate(outcomes)
                        if isinstance(outcome, BaseException)
                    )
                if cleanup_errors:
                    failed_keys = ", ".join(key for key, _ in cleanup_errors)
                    raise RuntimeError(f"Failed to clean up benchmark entities: {failed_keys}") from cleanup_errors[0][1]

        elapsed = time.perf_counter() - started
        report = {
            "schema_version": 1,
            "benchmark_kind": "themisdb_http_entity_ingest_readback",
            "external_comparison_ready": False,
            "dataset": {
                "source": str(self.data_file.resolve()),
                "kind": self.dataset_kind,
                "articles_measured": article_count,
                "serialized_content_bytes": bytes_written,
            },
            "protocol": {
                "endpoint": self.endpoint,
                "concurrency": self.concurrency,
                "operations": (
                    "up to concurrency independent PUT entity then exact GET readback pairs; "
                    "owned entities deleted after measurement"
                ),
                "latency_scope": "HTTP client, network and server response",
                "full_text_search_measured": False,
                "recovery_or_memory_stress_measured": False,
            },
            "measurements": {
                "elapsed_seconds": elapsed,
                "write_operations": len(write_latencies),
                "read_operations": len(read_latencies),
                "articles_per_second": article_count / elapsed if elapsed > 0 else 0,
                "write_latency_p50_ms": self._percentile(write_latencies, 50),
                "write_latency_p95_ms": self._percentile(write_latencies, 95),
                "read_latency_p50_ms": self._percentile(read_latencies, 50),
                "read_latency_p95_ms": self._percentile(read_latencies, 95),
            },
        }
        report_path.parent.mkdir(parents=True, exist_ok=True)
        with report_path.open("x", encoding="utf-8") as output:
            json.dump(report, output, indent=2, allow_nan=False)
        return report


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=["generate", "test", "both"], default="test")
    parser.add_argument("--articles", type=int, default=1000, help="Synthetic articles to generate")
    parser.add_argument("--data-file", type=Path, help="Input or generated JSONL path")
    parser.add_argument("--endpoint", help="ThemisDB HTTP(S) origin")
    parser.add_argument("--dataset-kind", choices=["synthetic", "wikipedia-dump"])
    parser.add_argument("--max-articles", type=int, default=1000)
    parser.add_argument("--concurrency", type=int, default=1,
                        help="Maximum number of independent article write/read pairs in flight")
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--output", type=Path, default=Path("wikipedia_stress_test_results.json"))
    args = parser.parse_args()

    if args.mode in {"test", "both"}:
        if not args.endpoint or not args.data_file:
            parser.error("--endpoint and --data-file are required for test/both mode")
        if args.dataset_kind is None and args.mode == "test":
            parser.error("--dataset-kind must identify synthetic or wikipedia-dump input")
    if args.mode in {"generate", "both"}:
        if args.articles <= 0:
            parser.error("--articles must be positive")
        data_file = args.data_file or Path("data/wikipedia/wikipedia_articles.jsonl")
        generator = WikipediaDataGenerator(str(data_file.parent))
        args.data_file = generator.save_as_jsonl(
            generator.generate_articles(args.articles), data_file.name
        )
        if args.mode == "generate":
            return
        args.dataset_kind = "synthetic"
    if args.max_articles <= 0 or args.timeout <= 0 or args.concurrency <= 0:
        parser.error("--max-articles, --timeout, and --concurrency must be positive")

    test = WikipediaStressTest(
        str(args.data_file), args.endpoint, dataset_kind=args.dataset_kind,
        max_articles=args.max_articles, timeout=args.timeout,
        concurrency=args.concurrency,
    )
    report = await test.run_stress_test(str(args.output))
    logger.info(
        "Measured %s articles: %.2f articles/sec; write p95 %.3f ms; read p95 %.3f ms",
        report["dataset"]["articles_measured"],
        report["measurements"]["articles_per_second"],
        report["measurements"]["write_latency_p95_ms"],
        report["measurements"]["read_latency_p95_ms"],
    )


if __name__ == "__main__":
    asyncio.run(main())
