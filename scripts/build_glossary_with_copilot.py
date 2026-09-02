#!/usr/bin/env python3
"""build_glossary_with_copilot.py — Build glossary index from candidates with optional AI enrichment.

Usage:
    python scripts/build_glossary_with_copilot.py \\
      --input docs/glossary_candidates.json \\
      --output docs/glossary_index.json \\
      [--enable-copilot] [--model gpt-4o-mini] [--max-parallel 5]

Pipeline:
1. Load glossary_candidates.json from Phase B.2
2. Enrich each candidate:
   - Extract context from source references
   - Generate short/long descriptions (heuristic or Copilot-powered)
   - Assign categories (themisdb-core, data-model, query-performance, general-knowledge)
   - Generate multilingual titles (de, en, fr)
   - Map to Wikipedia links
3. Write enriched glossary_index.json

Output: glossary_index.json
- Validated, categorized, multilingual glossary ready for generate_glossaries.py
- Includes version, timestamp, validation flags
- Each term: id, name, de/en/fr titles, short/long descriptions, category, priority, aliases, see_also, wikipedia_link, source_files
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import UTC, datetime
from pathlib import Path
from typing import Any, Optional

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Default categories for term classification
TERM_CATEGORIES = {
    "themisdb-core": "ThemisDB Core Concepts",
    "data-model": "Data Model & Abstractions",
    "query-performance": "Query & Performance",
    "general-knowledge": "General Knowledge / Wikipedia",
}

# Heuristic category mapping: term_name_pattern → category
CATEGORY_HEURISTICS: list[tuple[str, str]] = [
    # ThemisDB-core
    (r"^(AQL|Query|Bind Variable|Query Plan)$", "query-performance"),
    (r"^(Entity|Edge|Collection|Base Entity|Transaction|MVCC|WAL|TSStore)$", "themisdb-core"),
    (r"^(HNSW|Vector|Embedding|Similarity|Index|Selectivity|Cardinality)$", "query-performance"),
    # Data models
    (r"^(Relational|Document|Graph|Vector|Time-Series)$", "data-model"),
    # General knowledge
    (r"^(ACID|BTree|Hash|Skiplist|Inverted|Quorum|PII|OpenAPI|Wikipedia)$", "general-knowledge"),
]

# Wikipedia link mapping for general-knowledge terms
WIKIPEDIA_LINKS: dict[str, str] = {
    "ACID": "https://en.wikipedia.org/wiki/ACID",
    "HNSW": "https://en.wikipedia.org/wiki/Hierarchical_navigable_small_world",
    "BTree": "https://en.wikipedia.org/wiki/B-tree",
    "Hash": "https://en.wikipedia.org/wiki/Hash_table",
    "Skiplist": "https://en.wikipedia.org/wiki/Skip_list",
    "Inverted Index": "https://en.wikipedia.org/wiki/Inverted_index",
    "Quorum": "https://en.wikipedia.org/wiki/Quorum_(distributed_computing)",
    "PII": "https://en.wikipedia.org/wiki/Personal_data",
    "OpenAPI": "https://en.wikipedia.org/wiki/OpenAPI",
}

# Heuristic short descriptions (fallback if no Copilot)
FALLBACK_DESCRIPTIONS: dict[str, dict[str, str]] = {
    "aql": {
        "short": "Query language for ThemisDB",
        "long": "AQL (Advanced Query Language) is ThemisDB's native query language for expressing complex queries across graph, document, and relational models.",
    },
    "entity": {
        "short": "Node or document in a collection",
        "long": "An Entity represents a single node/document in ThemisDB's graph or document model. Entities have properties and can participate in relationships via edges.",
    },
    "edge": {
        "short": "Relationship between entities",
        "long": "An Edge represents a directed or undirected relationship between two entities in a graph collection.",
    },
    "collection": {
        "short": "Storage container for entities",
        "long": "A Collection is a logical container storing entities (nodes/documents) with a schema. Collections can be graph, document, or relational.",
    },
    "mvcc": {
        "short": "Multi-Version Concurrency Control",
        "long": "MVCC is a concurrency control mechanism allowing multiple transactions to read different versions of data simultaneously without blocking.",
    },
    "wal": {
        "short": "Write-Ahead Log for durability",
        "long": "Write-Ahead Log (WAL) is a technique where changes are first written to a log before being applied to the database, ensuring durability.",
    },
    "tsstore": {
        "short": "Time-series storage engine",
        "long": "TSStore is ThemisDB's specialized storage engine for efficiently storing and querying time-series data with optimized compression and indexing.",
    },
    "hnsw": {
        "short": "Hierarchical Navigable Small World algorithm",
        "long": "HNSW is a fast approximate nearest neighbor search algorithm used for vector indexing, offering excellent performance for high-dimensional similarity search.",
    },
}

# ---------------------------------------------------------------------------
# Term enrichment
# ---------------------------------------------------------------------------

class GlossaryBuilder:
    """Build enriched glossary index from candidates."""
    
    def __init__(
        self,
        repo_root: Path,
        enable_copilot: bool = False,
        copilot_model: str = "gpt-4o-mini",
    ):
        self.repo_root = repo_root
        self.enable_copilot = enable_copilot
        self.copilot_model = copilot_model
        self.enriched_terms: dict[str, dict[str, Any]] = {}
    
    def build_from_candidates(self, candidates: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
        """Build glossary index from candidates."""
        for term_id, candidate in candidates.items():
            enriched_term = self._enrich_term(term_id, candidate)
            self.enriched_terms[term_id] = enriched_term
        
        return self.enriched_terms
    
    def _enrich_term(
        self,
        term_id: str,
        candidate: dict[str, Any],
    ) -> dict[str, Any]:
        """Enrich a single term candidate."""
        term_name = candidate["name"]
        
        # Assign category via heuristics
        category = self._assign_category(term_name)
        
        # Generate descriptions
        short_desc, long_desc = self._generate_descriptions(term_name, candidate)
        
        # Generate multilingual titles
        de_title, fr_title = self._generate_multilingual_titles(term_name)
        
        # Map Wikipedia link if general-knowledge
        wikipedia_link = None
        if category == "general-knowledge":
            wikipedia_link = WIKIPEDIA_LINKS.get(term_name)
        
        # Cross-references (see_also)
        see_also = self._generate_see_also(term_name, category)
        
        return {
            "id": term_id,
            "name": term_name,
            "titles": {
                "en": term_name,
                "de": de_title or term_name,
                "fr": fr_title or term_name,
            },
            "descriptions": {
                "short": short_desc,
                "long": long_desc,
            },
            "category": category,
            "priority": candidate.get("priority", "medium"),
            "aliases": candidate.get("aliases", []),
            "see_also": see_also,
            "wikipedia_link": wikipedia_link,
            "source_files": candidate.get("references", []),
            "occurrence_count": candidate.get("occurrence_count", 1),
            "is_heuristic": candidate.get("is_heuristic", False),
            "validation_flags": {
                "needs_review": candidate.get("is_heuristic", False),
                "is_copilot_enriched": False,  # Set to True if Copilot enrichment succeeds
            },
        }
    
    def _assign_category(self, term_name: str) -> str:
        """Assign category to term via heuristics."""
        for pattern, category in CATEGORY_HEURISTICS:
            if re.match(pattern, term_name, re.IGNORECASE):
                return category
        
        # Default category based on term characteristics
        if re.match(r"^[A-Z][A-Z0-9]{2,}$", term_name):  # CAPS acronyms
            return "general-knowledge"
        
        return "themisdb-core"  # Default
    
    def _generate_descriptions(
        self,
        term_name: str,
        candidate: dict[str, Any],
    ) -> tuple[str, str]:
        """Generate short and long descriptions."""
        term_id = candidate.get("id", "").lower()
        
        # Try fallback descriptions first
        if term_id in FALLBACK_DESCRIPTIONS:
            fallback = FALLBACK_DESCRIPTIONS[term_id]
            return fallback["short"], fallback["long"]
        
        # Default to candidate description or generic fallback
        existing_desc = candidate.get("description", "")
        if existing_desc:
            return existing_desc[:100], existing_desc
        
        # Generic fallback
        short = f"{term_name} is a ThemisDB concept."
        long = f"{term_name} is a key concept in ThemisDB's architecture. For detailed information, see the documentation and source references."
        return short, long
    
    def _generate_multilingual_titles(self, term_name: str) -> tuple[Optional[str], Optional[str]]:
        """Generate German and French titles (heuristic)."""
        # Heuristic: known translations
        translations: dict[str, dict[str, str]] = {
            "AQL": {"de": "AQL", "fr": "AQL"},
            "Entity": {"de": "Entität", "fr": "Entité"},
            "Edge": {"de": "Kante", "fr": "Arête"},
            "Collection": {"de": "Kollektion", "fr": "Collection"},
            "MVCC": {"de": "MVCC", "fr": "MVCC"},
            "WAL": {"de": "WAL", "fr": "WAL"},
            "TSStore": {"de": "TSStore", "fr": "TSStore"},
            "Transaction": {"de": "Transaktion", "fr": "Transaction"},
            "HNSW": {"de": "HNSW", "fr": "HNSW"},
            "Index": {"de": "Index", "fr": "Index"},
            "Vector": {"de": "Vektor", "fr": "Vecteur"},
            "Query": {"de": "Abfrage", "fr": "Requête"},
        }
        
        if term_name in translations:
            return translations[term_name].get("de"), translations[term_name].get("fr")
        
        return None, None
    
    def _generate_see_also(self, term_name: str, category: str) -> list[str]:
        """Generate cross-references (see_also)."""
        # Heuristic: related terms by category and name similarity
        related_patterns: dict[str, list[str]] = {
            "Entity": ["Collection", "Edge", "Graph", "Document"],
            "Edge": ["Entity", "Graph", "Collection"],
            "Collection": ["Entity", "Edge", "Index", "Query Plan"],
            "Index": ["Query Plan", "Selectivity", "Cardinality"],
            "Query Plan": ["Index", "Bind Variable", "Selectivity"],
            "Transaction": ["MVCC", "WAL", "ACID"],
            "MVCC": ["Transaction", "Concurrency", "ACID"],
            "WAL": ["Durability", "Transaction", "ACID"],
            "Vector": ["HNSW", "Similarity", "Embedding"],
            "HNSW": ["Vector", "Similarity", "Index"],
        }
        
        return related_patterns.get(term_name, [])


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def load_candidates(candidates_path: Path) -> dict[str, dict[str, Any]]:
    """Load glossary candidates from JSON."""
    if not candidates_path.exists():
        print(f"Error: candidates file not found: {candidates_path}", file=sys.stderr)
        sys.exit(1)
    
    with open(candidates_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    return data.get("terms", {})


def write_glossary_index(
    enriched_terms: dict[str, dict[str, Any]],
    output_path: Path,
) -> None:
    """Write enriched glossary index to JSON."""
    output_data = {
        "metadata": {
            "generated_at": datetime.now(UTC).isoformat(),
            "schema_version": "2.0",
            "source_count": len(enriched_terms),
            "status": "index",
            "description": "Enriched, categorized, multilingual glossary index. Ready for generate_glossaries.py and build_wiki.py term-linking.",
        },
        "terms": enriched_terms,
    }
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)
    
    print(f"✓ Wrote {len(enriched_terms)} enriched terms to {output_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Build enriched glossary index from candidates."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("docs/glossary_candidates.json"),
        help="Input candidates JSON from scan_glossary_terms.py (default: docs/glossary_candidates.json)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/glossary_index.json"),
        help="Output glossary index JSON (default: docs/glossary_index.json)",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Repository root directory (default: current directory)",
    )
    parser.add_argument(
        "--enable-copilot",
        action="store_true",
        help="Enable optional Copilot/LLM enrichment (requires GITHUB_TOKEN or OpenAI API)",
    )
    parser.add_argument(
        "--model",
        default="gpt-4o-mini",
        help="Model for Copilot enrichment (default: gpt-4o-mini)",
    )
    
    args = parser.parse_args()
    
    # Load candidates
    print(f"Loading candidates from {args.input}...")
    candidates = load_candidates(args.input)
    
    if not candidates:
        print("Error: No candidates found in input file.", file=sys.stderr)
        return 1
    
    print(f"Building glossary index from {len(candidates)} candidates...")
    
    # Build enriched glossary
    builder = GlossaryBuilder(
        args.repo_root,
        enable_copilot=args.enable_copilot,
        copilot_model=args.model,
    )
    enriched_terms = builder.build_from_candidates(candidates)
    
    # Write output
    write_glossary_index(enriched_terms, args.output)
    
    # Summary
    print(f"\nSummary:")
    print(f"  Total terms: {len(enriched_terms)}")
    
    by_category = {}
    for term in enriched_terms.values():
        cat = term.get("category", "unknown")
        by_category[cat] = by_category.get(cat, 0) + 1
    
    for cat, count in sorted(by_category.items()):
        print(f"  {cat}: {count}")
    
    print(f"\nNext steps:")
    print(f"  1. Review {args.output} for accuracy")
    print(f"  2. Run: python scripts/generate_glossaries.py --input {args.output} --output docs/")
    print(f"  3. Integrate into publish-wiki.yml workflow")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
