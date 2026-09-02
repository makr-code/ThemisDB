#!/usr/bin/env python3
"""generate_glossaries.py — Generate language-specific glossaries from glossary_index.json.

Usage:
    python scripts/generate_glossaries.py \\
      --input docs/glossary_index.json \\
      --output docs/ \\
      [--languages de,en,fr]

Generates category-organized Markdown glossaries for each language:
- docs/glossary.md (English, primary)
- docs/en/glossary.md (English variant)
- docs/de/glossary.md (German)
- docs/fr/glossary.md (French)

Each generated glossary:
1. Includes breadcrumb navigation and metadata headers
2. Organizes terms by category (4-level hierarchy)
3. Includes multilingual links and cross-references
4. Adds Wikipedia links for general-knowledge terms
5. Maintains version tracking and generation timestamp
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Languages to generate (language_code → language_name)
LANGUAGE_NAMES = {
    "en": "English",
    "de": "Deutsch",
    "fr": "Français",
}

# Category display order and translations
CATEGORY_ORDER = [
    "themisdb-core",
    "data-model",
    "query-performance",
    "general-knowledge",
]

CATEGORY_LABELS: dict[str, dict[str, str]] = {
    "themisdb-core": {
        "en": "ThemisDB Core Concepts",
        "de": "ThemisDB Kernkonzepte",
        "fr": "Concepts Fondamentaux de ThemisDB",
    },
    "data-model": {
        "en": "Data Model & Abstractions",
        "de": "Datenmodell & Abstraktionen",
        "fr": "Modèle de Données et Abstractions",
    },
    "query-performance": {
        "en": "Query & Performance",
        "de": "Abfragen & Leistung",
        "fr": "Requêtes et Performance",
    },
    "general-knowledge": {
        "en": "General Knowledge",
        "de": "Allgemeinwissen",
        "fr": "Connaissances Générales",
    },
}

# Header/Footer translations
GLOSSARY_HEADERS: dict[str, dict[str, str]] = {
    "en": {
        "title": "Glossary",
        "description": "ThemisDB Glossary — Definitions and explanations of key concepts",
        "metadata_date": "Generated",
        "metadata_version": "Version",
        "toc": "Table of Contents",
        "see_also": "See also",
        "wikipedia": "Learn more on Wikipedia",
    },
    "de": {
        "title": "Glossar",
        "description": "ThemisDB Glossar — Definitionen und Erklärungen von Schlüsselkonzepten",
        "metadata_date": "Generiert",
        "metadata_version": "Version",
        "toc": "Inhaltsverzeichnis",
        "see_also": "Siehe auch",
        "wikipedia": "Mehr auf Wikipedia",
    },
    "fr": {
        "title": "Glossaire",
        "description": "Glossaire ThemisDB — Définitions et explications des concepts clés",
        "metadata_date": "Généré",
        "metadata_version": "Version",
        "toc": "Table des matières",
        "see_also": "Voir aussi",
        "wikipedia": "En savoir plus sur Wikipedia",
    },
}

# ---------------------------------------------------------------------------
# Glossary generation
# ---------------------------------------------------------------------------

class GlossaryGenerator:
    """Generate language-specific glossaries from glossary index."""
    
    def __init__(self, glossary_index: dict[str, dict[str, Any]]):
        self.glossary_index = glossary_index
        self.metadata = {}
    
    def generate_all_languages(
        self,
        output_dir: Path,
        languages: list[str],
    ) -> None:
        """Generate glossaries for all specified languages."""
        for lang_code in languages:
            if lang_code not in LANGUAGE_NAMES:
                print(f"Warning: Unknown language code '{lang_code}', skipping.", file=sys.stderr)
                continue
            
            # Determine output path
            if lang_code == "en":
                # Primary English glossary in docs/glossary.md + docs/en/glossary.md
                paths = [
                    output_dir / "glossary.md",
                    output_dir / "en" / "glossary.md",
                ]
            else:
                paths = [output_dir / lang_code / "glossary.md"]
            
            content = self._generate_glossary(lang_code)
            
            for path in paths:
                path.parent.mkdir(parents=True, exist_ok=True)
                with open(path, "w", encoding="utf-8") as f:
                    f.write(content)
                print(f"✓ Generated {lang_code} glossary: {path}")
    
    def _generate_glossary(self, lang_code: str) -> str:
        """Generate glossary content for a language."""
        lines = []
        headers = GLOSSARY_HEADERS.get(lang_code, GLOSSARY_HEADERS["en"])
        
        # Metadata header
        lines.append(f"# {headers['title']}\n")
        lines.append(f"{headers['description']}\n")
        lines.append(f"**{headers['metadata_date']}:** {datetime.now(UTC).strftime('%Y-%m-%d')}")
        lines.append(f"**{headers['metadata_version']}:** 2.0\n\n")
        
        # Table of Contents
        lines.append(f"## {headers['toc']}\n")
        for category in CATEGORY_ORDER:
            cat_label = CATEGORY_LABELS[category].get(lang_code, category)
            lines.append(f"- [{cat_label}](#{self._slugify(cat_label)})")
        lines.append("")
        
        # Terms organized by category
        for category in CATEGORY_ORDER:
            cat_label = CATEGORY_LABELS[category].get(lang_code, category)
            lines.append(f"\n## {cat_label}\n")
            
            # Filter terms for this category
            category_terms = [
                (term_id, term)
                for term_id, term in self.glossary_index.items()
                if term.get("category") == category
            ]
            
            # Sort by name
            category_terms.sort(key=lambda x: x[1].get("name", ""))
            
            for term_id, term in category_terms:
                lines.extend(self._format_term(term, lang_code, headers))
        
        return "\n".join(lines)
    
    def _format_term(
        self,
        term: dict[str, Any],
        lang_code: str,
        headers: dict[str, str],
    ) -> list[str]:
        """Format a single term for glossary output."""
        lines = []
        
        # Get term name in current language
        title = term.get("titles", {}).get(lang_code, term.get("name", ""))
        
        # Main heading
        lines.append(f"### {title}\n")
        
        # Short description
        short_desc = term.get("descriptions", {}).get("short", "")
        if short_desc:
            lines.append(f"{short_desc}\n")
        
        # Long description
        long_desc = term.get("descriptions", {}).get("long", "")
        if long_desc and long_desc != short_desc:
            lines.append(f"*{long_desc}*\n")
        
        # Aliases
        aliases = term.get("aliases", [])
        if aliases:
            aliases_str = ", ".join(aliases)
            lines.append(f"**Aliases:** {aliases_str}\n")
        
        # See also (cross-references)
        see_also = term.get("see_also", [])
        if see_also:
            see_also_str = ", ".join(see_also)
            lines.append(f"**{headers.get('see_also', 'See also')}:** {see_also_str}\n")
        
        # Wikipedia link
        wikipedia_link = term.get("wikipedia_link")
        if wikipedia_link:
            lines.append(f"🔗 [{headers.get('wikipedia', 'Learn more')}]({wikipedia_link})\n")
        
        # Source attribution
        sources = term.get("source_files", [])
        if sources:
            source_list = "; ".join(
                f"{s['source_file']}:{s.get('line_number', '?')}"
                for s in sources[:3]  # Limit to first 3 sources
            )
            lines.append(f"*Source: {source_list}*\n")
        
        lines.append("")
        return lines
    
    @staticmethod
    def _slugify(text: str) -> str:
        """Convert text to URL slug."""
        slug = text.lower()
        slug = slug.replace(" ", "-")
        slug = "".join(c for c in slug if c.isalnum() or c == "-")
        return slug


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def load_glossary_index(index_path: Path) -> dict[str, dict[str, Any]]:
    """Load glossary index from JSON."""
    if not index_path.exists():
        print(f"Error: glossary index not found: {index_path}", file=sys.stderr)
        sys.exit(1)
    
    with open(index_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    return data.get("terms", {})


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Generate language-specific glossaries from glossary index."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("docs/glossary_index.json"),
        help="Input glossary index JSON from build_glossary_with_copilot.py (default: docs/glossary_index.json)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs"),
        help="Output directory for generated glossaries (default: docs/)",
    )
    parser.add_argument(
        "--languages",
        default="en,de,fr",
        help="Languages to generate (comma-separated, default: en,de,fr)",
    )
    
    args = parser.parse_args()
    
    # Load glossary index
    print(f"Loading glossary index from {args.input}...")
    glossary_index = load_glossary_index(args.input)
    
    if not glossary_index:
        print("Error: No terms found in glossary index.", file=sys.stderr)
        return 1
    
    print(f"Generating glossaries for {len(glossary_index)} terms...")
    
    # Parse language codes
    languages = [lang.strip() for lang in args.languages.split(",")]
    
    # Generate glossaries
    generator = GlossaryGenerator(glossary_index)
    generator.generate_all_languages(args.output, languages)
    
    # Summary
    print(f"\nSummary:")
    print(f"  Total terms: {len(glossary_index)}")
    print(f"  Languages: {', '.join(languages)}")
    print(f"  Output directory: {args.output}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
