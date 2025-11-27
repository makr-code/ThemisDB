"""
RESPO Ingestion Module

Provides code ingestion capabilities including:
- GitHub repository scraping
- Code chunking and parsing
- Ingestion pipeline
- Indexing
"""

from respo.ingestion.chunker import CodeChunk, CodeChunker
from respo.ingestion.github_scraper import (
    CodeFile,
    GitHubScraper,
    ScraperConfig,
    parse_github_url,
)
from respo.ingestion.pipeline import IngestionConfig, IngestionPipeline, IngestionStats

__all__ = [
    "CodeFile",
    "GitHubScraper",
    "ScraperConfig",
    "parse_github_url",
    "CodeChunk",
    "CodeChunker",
    "IngestionPipeline",
    "IngestionConfig",
    "IngestionStats",
]
