"""
RESPO Source Scrapers

Multi-platform source code scrapers for various code repositories, package registries,
documentation, forums, and other programming resources.

## Available Scrapers

### Git Platforms
- GitLabScraper: GitLab repositories (gitlab.com, self-hosted)
- BitbucketScraper: Bitbucket Cloud repositories

### Package Registries
- PyPIScraper: Python Package Index (pypi.org)
- NPMScraper: Node.js packages (npmjs.com)
- CratesScraper: Rust crates (crates.io)
- DebianScraper: Debian/Ubuntu source packages

### Code Snippets & Forums
- StackOverflowScraper: Stack Overflow Q&A with code
- GistsScraper: GitHub Gists

### Documentation & References
- DevDocsScraper: DevDocs.io aggregated documentation
- CheatShScraper: cheat.sh command/language cheatsheets
- TLDRScraper: Simplified man pages (tldr-pages)
- MDNScraper: MDN Web Docs (JS, CSS, HTML, Web APIs)
"""

from respo.ingestion.sources.base import CodeSource, SourceConfig, SourceFile, SourceType

# Git Platforms
from respo.ingestion.sources.gitlab import GitLabScraper
from respo.ingestion.sources.bitbucket import BitbucketScraper

# Package Registries
from respo.ingestion.sources.debian import DebianScraper
from respo.ingestion.sources.pypi import PyPIScraper
from respo.ingestion.sources.npm import NPMScraper
from respo.ingestion.sources.crates import CratesScraper

# Code Snippets & Forums
from respo.ingestion.sources.stackoverflow import StackOverflowScraper
from respo.ingestion.sources.gists import GistsScraper

# Documentation & References
from respo.ingestion.sources.docs import (
    DevDocsScraper,
    CheatShScraper,
    TLDRScraper,
    MDNScraper,
)

__all__ = [
    # Base
    "CodeSource",
    "SourceConfig",
    "SourceFile",
    "SourceType",
    # Git Platforms
    "GitLabScraper",
    "BitbucketScraper",
    # Package Registries
    "DebianScraper",
    "PyPIScraper",
    "NPMScraper",
    "CratesScraper",
    # Code Snippets & Forums
    "StackOverflowScraper",
    "GistsScraper",
    # Documentation
    "DevDocsScraper",
    "CheatShScraper",
    "TLDRScraper",
    "MDNScraper",
    # Factory
    "get_scraper",
]


def get_scraper(source_type: str, **kwargs) -> CodeSource:
    """
    Factory function to get a scraper by source type.

    Args:
        source_type: Type of source (github, gitlab, pypi, npm, etc.)
        **kwargs: Additional arguments for the scraper

    Returns:
        Configured scraper instance
    """
    scrapers = {
        "gitlab": GitLabScraper,
        "bitbucket": BitbucketScraper,
        "debian": DebianScraper,
        "ubuntu": lambda **kw: DebianScraper(distribution="ubuntu", **kw),
        "pypi": PyPIScraper,
        "npm": NPMScraper,
        "crates": CratesScraper,
        "stackoverflow": StackOverflowScraper,
        "gists": GistsScraper,
        "devdocs": DevDocsScraper,
        "cheatsh": CheatShScraper,
        "tldr": TLDRScraper,
        "mdn": MDNScraper,
    }

    scraper_class = scrapers.get(source_type.lower())
    if not scraper_class:
        raise ValueError(f"Unknown source type: {source_type}. Available: {list(scrapers.keys())}")

    return scraper_class(**kwargs)
