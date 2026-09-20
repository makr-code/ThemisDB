"""Canonical GitHub Wiki page layout contract for ThemisDB.

This module defines the required top-level structure of the GitHub Wiki so the
published site remains navigable, user-focused, and source-ranked instead of a
raw mirror of repository files.
"""

from __future__ import annotations

WIKI_PAGE_LAYOUT = [
    {
        "key": "home",
        "slug": "Home",
        "title": "Home",
        "section": "Overview",
        "required": True,
        "priority": 100,
    },
    {
        "key": "getting_started",
        "slug": "Getting-Started",
        "title": "Getting Started",
        "section": "Getting Started",
        "required": True,
        "priority": 95,
    },
    {
        "key": "architecture",
        "slug": "Architecture",
        "title": "Architecture",
        "section": "Architecture",
        "required": True,
        "priority": 90,
    },
    {
        "key": "modules",
        "slug": "Modules",
        "title": "Modules",
        "section": "Modules",
        "required": True,
        "priority": 88,
    },
    {
        "key": "api_contracts",
        "slug": "APIs-and-Contracts",
        "title": "APIs and Contracts",
        "section": "APIs and Contracts",
        "required": True,
        "priority": 87,
    },
    {
        "key": "operations",
        "slug": "Operations",
        "title": "Operations",
        "section": "Operations",
        "required": True,
        "priority": 86,
    },
    {
        "key": "governance",
        "slug": "Governance",
        "title": "Governance",
        "section": "Governance",
        "required": True,
        "priority": 84,
    },
    {
        "key": "troubleshooting",
        "slug": "Troubleshooting",
        "title": "Troubleshooting",
        "section": "Troubleshooting",
        "required": True,
        "priority": 82,
    },
    {
        "key": "release_notes",
        "slug": "Release-Notes",
        "title": "Release Notes",
        "section": "Release Notes",
        "required": True,
        "priority": 80,
    },
]

REQUIRED_PAGE_SLUGS = {entry["slug"] for entry in WIKI_PAGE_LAYOUT if entry.get("required")}
REQUIRED_PAGE_KEYS = [entry["key"] for entry in WIKI_PAGE_LAYOUT if entry.get("required")]
REQUIRED_SECTION_ORDER = [entry["section"] for entry in WIKI_PAGE_LAYOUT if entry.get("required")]


def required_page_names() -> list[str]:
    """Return canonical wiki page names in the required order."""
    return [entry["slug"] for entry in WIKI_PAGE_LAYOUT if entry.get("required")]
