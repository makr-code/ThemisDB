"""
RESPO Ingestion Module

Provides code ingestion capabilities including:
- GitHub repository scraping
- Code chunking and parsing
- ThemisDB indexing
"""

from respo.ingestion.github_scraper import (
    CodeFile,
    GitHubScraper,
    ScraperConfig,
    parse_github_url,
)

__all__ = [
    "CodeFile",
    "GitHubScraper",
    "ScraperConfig",
    "parse_github_url",
]
