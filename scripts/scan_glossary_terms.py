#!/usr/bin/env python3
"""scan_glossary_terms.py — Extract glossary term candidates from repository documentation.

Usage:
    python scripts/scan_glossary_terms.py --output docs/glossary_candidates.json [--repo-root .]

Scans multiple sources to extract glossary term candidates:
1. Doxygen @term tags in C++ headers/source
2. Markdown ## Term headers in docs/
3. CAPS-WORD pattern heuristics in documentation
4. Existing glossary consolidation from docs/{de,fr,compendium}/glossary.md

Output: glossary_candidates.json
- Unvalidated term list with source locations
- Ready for Phase B.3 enrichment via build_glossary_with_copilot.py
- Includes: term name, aliases, references, source_file, line_number, occurrence_count
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Directories to scan for term candidates
SCAN_ROOTS = [
    "docs",           # Documentation
    "src",            # Source code with comments/Doxygen
    "include",        # Public headers
]

# File patterns to include
INCLUDE_PATTERNS = [
    "*.md",           # Markdown
    "*.h", "*.hpp",   # C++ headers
    "*.cpp", "*.cc",  # C++ source
]

# File patterns to exclude
EXCLUDE_PATTERNS = [
    "**/node_modules/**",
    "**/.git/**",
    "**/__pycache__/**",
    "**/build/**",
    "**/.cmake/**",
    "**/vcpkg/**",
    "**/ARCHIVED/**",
]

# Known glossary sources (existing terms)
EXISTING_GLOSSARY_SOURCES = [
    "docs/glossary.md",
    "docs/de/glossary.md",
    "docs/fr/glossary.md",
    "docs/compendium/docs/appendix_h_glossary.md",
]

# CAPS-word patterns to detect (heuristics)
CAPS_PATTERN = re.compile(r"\b[A-Z][A-Z0-9]{2,}\b")  # e.g., MVCC, AQL, WAL

# Doxygen @term tag pattern
DOXYGEN_TERM_PATTERN = re.compile(r"@term\s+(\w+(?:\s+\w+)*)", re.MULTILINE)

# Markdown ## Term header pattern
MARKDOWN_TERM_PATTERN = re.compile(r"^##\s+([^\n]+)$", re.MULTILINE)

# ---------------------------------------------------------------------------
# Term extraction
# ---------------------------------------------------------------------------

class TermScanner:
    """Scan and extract glossary term candidates."""
    
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.candidates: dict[str, dict[str, Any]] = {}  # term_id → term data
        self.term_aliases: dict[str, str] = {}  # alias → canonical term_id
        
    def scan(self) -> dict[str, dict[str, Any]]:
        """Execute full scan pipeline."""
        # Phase 1: Extract from existing glossaries
        self._scan_existing_glossaries()
        
        # Phase 2: Scan source files for new candidates
        self._scan_source_files()
        
        return self.candidates
    
    def _scan_existing_glossaries(self) -> None:
        """Extract terms from existing glossary files."""
        for glossary_path in EXISTING_GLOSSARY_SOURCES:
            full_path = self.repo_root / glossary_path
            if not full_path.exists():
                continue
            
            text = full_path.read_text(encoding="utf-8", errors="ignore")
            
            # Extract terms from markdown lists (- Term: description)
            for match in re.finditer(r"^-\s+(\w+(?:\s+\w+)*?):\s*(.+)$", text, re.MULTILINE):
                term_name = match.group(1).strip()
                description = match.group(2).strip()
                self._register_term(
                    term_name=term_name,
                    description=description,
                    source_file=str(glossary_path),
                    line_number=text[:match.start()].count('\n') + 1,
                    priority="high",  # Existing glossary terms are high-priority
                )
    
    def _scan_source_files(self) -> None:
        """Scan source files for term candidates."""
        for root_dir in SCAN_ROOTS:
            root_path = self.repo_root / root_dir
            if not root_path.exists():
                continue
            
            for source_file in root_path.rglob("*"):
                if not self._should_process_file(source_file):
                    continue
                
                try:
                    text = source_file.read_text(encoding="utf-8", errors="ignore")
                except Exception:
                    continue
                
                # Extract by file type
                if source_file.suffix in [".h", ".hpp", ".cpp", ".cc"]:
                    self._extract_from_cpp(text, source_file)
                elif source_file.suffix == ".md":
                    self._extract_from_markdown(text, source_file)
    
    def _should_process_file(self, path: Path) -> bool:
        """Check if file should be processed."""
        # Check exclusions
        path_str = str(path)
        for exclude in EXCLUDE_PATTERNS:
            if Path(path_str).match(exclude):
                return False
        
        # Check inclusions
        for include in INCLUDE_PATTERNS:
            if path.match(include):
                return True
        
        return False
    
    def _extract_from_cpp(self, text: str, source_file: Path) -> None:
        """Extract glossary terms from C++ source/headers."""
        # Extract Doxygen @term tags
        for match in DOXYGEN_TERM_PATTERN.finditer(text):
            term_name = match.group(1).strip()
            self._register_term(
                term_name=term_name,
                source_file=str(source_file.relative_to(self.repo_root)),
                line_number=text[:match.start()].count('\n') + 1,
                priority="high",
            )
        
        # Heuristic: CAPS-words in comments
        for match in CAPS_PATTERN.finditer(text):
            term_name = match.group(0)
            # Filter: skip common C++ keywords/acronyms
            if term_name in ["API", "CPU", "GPU", "RAM", "USB", "HTTP", "JSON", "XML", "SQL", "TCP", "UDP", "DNS", "URL", "UTF"]:
                continue
            
            self._register_term(
                term_name=term_name,
                source_file=str(source_file.relative_to(self.repo_root)),
                line_number=text[:match.start()].count('\n') + 1,
                priority="medium",
                is_heuristic=True,
            )
    
    def _extract_from_markdown(self, text: str, source_file: Path) -> None:
        """Extract glossary terms from Markdown."""
        # Extract ## headers as potential terms
        for match in MARKDOWN_TERM_PATTERN.finditer(text):
            header_text = match.group(1).strip()
            # Heuristic: single word or camelCase headers are likely terms
            if " " not in header_text or re.match(r"^[A-Z][a-z]+(?:[A-Z][a-z]+)*$", header_text):
                self._register_term(
                    term_name=header_text,
                    source_file=str(source_file.relative_to(self.repo_root)),
                    line_number=text[:match.start()].count('\n') + 1,
                    priority="medium",
                )
        
        # Heuristic: CAPS-words in markdown
        for match in CAPS_PATTERN.finditer(text):
            term_name = match.group(0)
            if term_name in ["API", "CPU", "GPU", "RAM", "USB", "HTTP", "JSON", "XML", "SQL", "TCP", "UDP", "DNS", "URL", "UTF"]:
                continue
            
            self._register_term(
                term_name=term_name,
                source_file=str(source_file.relative_to(self.repo_root)),
                line_number=text[:match.start()].count('\n') + 1,
                priority="low",
                is_heuristic=True,
            )
    
    def _register_term(
        self,
        term_name: str,
        source_file: str,
        line_number: int,
        priority: str = "medium",
        description: str = "",
        is_heuristic: bool = False,
    ) -> None:
        """Register or update a term candidate."""
        term_id = self._normalize_term_id(term_name)
        
        if term_id not in self.candidates:
            self.candidates[term_id] = {
                "id": term_id,
                "name": term_name,
                "aliases": [term_name] if term_name != term_id else [],
                "description": description,
                "category": None,  # To be assigned in Phase B.3
                "priority": priority,
                "is_heuristic": is_heuristic,
                "references": [],  # List of {source_file, line_number}
                "occurrence_count": 0,
            }
        
        # Update occurrence
        self.candidates[term_id]["occurrence_count"] += 1
        
        # Add reference
        ref = {"source_file": source_file, "line_number": line_number}
        if ref not in self.candidates[term_id]["references"]:
            self.candidates[term_id]["references"].append(ref)
        
        # Prefer non-heuristic references for priority
        if not is_heuristic and priority != self.candidates[term_id]["priority"]:
            self.candidates[term_id]["priority"] = priority
    
    def _normalize_term_id(self, term_name: str) -> str:
        """Normalize term name to ID (lowercase, underscore-separated)."""
        # Convert camelCase/PascalCase to snake_case
        term_id = re.sub(r"([a-z])([A-Z])", r"\1_\2", term_name)
        # Convert spaces and hyphens to underscores
        term_id = re.sub(r"[\s\-]+", "_", term_id)
        # Lowercase
        term_id = term_id.lower()
        return term_id


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

def write_candidates_json(
    candidates: dict[str, dict[str, Any]],
    output_path: Path,
) -> None:
    """Write glossary candidates to JSON file."""
    output_data = {
        "metadata": {
            "generated_at": datetime.now(UTC).isoformat(),
            "schema_version": "1.0",
            "source_count": len(candidates),
            "status": "candidates",
            "description": "Unvalidated glossary term candidates extracted from repository documentation and source code. Ready for Phase B.3 enrichment.",
        },
        "terms": candidates,
    }
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)
    
    print(f"✓ Wrote {len(candidates)} term candidates to {output_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Extract glossary term candidates from repository documentation."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path("."),
        help="Repository root directory (default: current directory)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/glossary_candidates.json"),
        help="Output JSON file for candidates (default: docs/glossary_candidates.json)",
    )
    parser.add_argument(
        "--min-priority",
        choices=["high", "medium", "low"],
        default="low",
        help="Include terms with at least this priority (default: low = all terms)",
    )
    
    args = parser.parse_args()
    
    # Validate repo root
    if not (args.repo_root / "docs").exists():
        print(f"Error: docs directory not found in {args.repo_root}", file=sys.stderr)
        return 1
    
    # Scan and extract
    print(f"Scanning {args.repo_root} for glossary term candidates...")
    scanner = TermScanner(args.repo_root)
    candidates = scanner.scan()
    
    # Filter by priority if requested
    priority_order = {"high": 2, "medium": 1, "low": 0}
    min_priority_level = priority_order.get(args.min_priority, 0)
    filtered = {
        term_id: term_data
        for term_id, term_data in candidates.items()
        if priority_order.get(term_data["priority"], 0) >= min_priority_level
    }
    
    if not filtered:
        print("Warning: No term candidates found matching criteria.", file=sys.stderr)
        return 1
    
    # Write output
    write_candidates_json(filtered, args.output)
    
    # Summary
    print(f"\nSummary:")
    print(f"  Total candidates: {len(filtered)}")
    print(f"  High priority:    {sum(1 for t in filtered.values() if t['priority'] == 'high')}")
    print(f"  Medium priority:  {sum(1 for t in filtered.values() if t['priority'] == 'medium')}")
    print(f"  Low priority:     {sum(1 for t in filtered.values() if t['priority'] == 'low')}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
