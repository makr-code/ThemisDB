#!/usr/bin/env python3
"""
generate_rag_golden_dataset.py — Validator and catalog tool for the ThemisDB RAG golden dataset.

DESIGN PRINCIPLES (v3)
──────────────────────
1. Questions must NOT contain any expected_keyword.
   The retrieval test must prove the system can find the answer from documents —
   not by matching tokens from the question directly against the index.

2. Every entry carries provenance metadata (governance criterion):
     source_document  — relative path from repo root to the source document
     source_section   — heading/section where the fact was extracted
     indexed_at       — ISO date when the document was processed

3. Facts must be verifiable by reading the cited source document.
   No entry may be answerable from model priors alone.

MODES
──────────────────────
--validate    Validate an existing YAML dataset and report all violations.
--generate    Emit a stub YAML template from doku.db corpus (manual curation needed).

Usage:
    # Validate the authoritative dataset
    python scripts/generate_rag_golden_dataset.py \\
        --validate tests/llm/data/themisdb_rag_golden_dataset.yaml

    # Generate a candidate stub from corpus
    python scripts/generate_rag_golden_dataset.py \\
        --generate \\
        --input  doku.db.json \\
        --output /tmp/rag_candidates_stub.yaml \\
        [--max-entries 110] \\
        [--min-chunk-length 80]

Environment variables:
    THEMIS_DOCS_DB_JSON  — fallback path to doku.db.json for --generate mode
"""

import argparse
import json
import os
import re
import sys
from datetime import date
from typing import Any

# ─── Schema constants ──────────────────────────────────────────────────────────

REQUIRED_ENTRY_KEYS = {
    "id", "question", "expected_keywords",
    "knowledge_level", "source_document", "indexed_at",
}
OPTIONAL_ENTRY_KEYS = {
    "category", "expected_source_hint", "rarity_tier",
    "min_recall_score", "source_section",
}
VALID_KNOWLEDGE_LEVELS = {"general", "specific", "specialized"}
MIN_ENTRIES = 110
DISTRIBUTION_TARGETS = {"general": 0.20, "specific": 0.30, "specialized": 0.50}
DISTRIBUTION_TOLERANCE = 0.03
MIN_KEYWORD_LENGTH_FOR_LEAKAGE_CHECK = 5   # skip very short tokens (e.g. "P6")


# ─── YAML parser (minimal, handles dataset format) ─────────────────────────────

def _unquote(s: str) -> str:
    s = s.strip()
    if len(s) >= 2 and s[0] in ('"', "'") and s[-1] == s[0]:
        return s[1:-1]
    return s


def _parse_flow_seq(s: str) -> list[str]:
    s = s.strip().lstrip("[").rstrip("]")
    return [_unquote(tok.strip()) for tok in s.split(",") if tok.strip()]


def parse_dataset(path: str) -> list[dict[str, Any]]:
    """Parse the golden dataset YAML into a list of entry dicts."""
    entries: list[dict[str, Any]] = []
    current: dict[str, Any] = {}
    in_entry = False

    with open(path, encoding="utf-8") as f:
        for raw in f:
            line = raw.rstrip("\n")

            # Start of a new entry
            if re.match(r"  - id:", line):
                if in_entry and current.get("id"):
                    entries.append(current)
                current = {}
                in_entry = True
                current["id"] = _unquote(line.split(":", 1)[1].strip())
                continue

            if not in_entry:
                continue

            m = re.match(r"    (\w+): (.+)", line)
            if not m:
                continue
            key, val = m.group(1), m.group(2).strip()
            if key == "expected_keywords":
                current[key] = _parse_flow_seq(val)
            elif key == "min_recall_score":
                try:
                    current[key] = float(val)
                except ValueError:
                    current[key] = 0.7
            else:
                current[key] = _unquote(val)

    if in_entry and current.get("id"):
        entries.append(current)
    return entries


# ─── Validation ────────────────────────────────────────────────────────────────

def validate(path: str) -> int:
    """Validate the dataset at *path*.  Returns number of violations found."""
    print(f"Validating: {path}")
    entries = parse_dataset(path)
    violations: list[str] = []
    keyword_levels: dict[str, int] = {k: 0 for k in VALID_KNOWLEDGE_LEVELS}

    for e in entries:
        eid = e.get("id", "<unknown>")

        # Required keys
        for k in REQUIRED_ENTRY_KEYS:
            if not e.get(k):
                violations.append(f"{eid}: missing required field '{k}'")

        # knowledge_level
        level = e.get("knowledge_level", "")
        if level not in VALID_KNOWLEDGE_LEVELS:
            violations.append(f"{eid}: invalid knowledge_level='{level}'")
        else:
            keyword_levels[level] += 1

        # rarity_tier
        if level == "specialized" and e.get("rarity_tier") != "rare":
            violations.append(f"{eid}: specialized entry must have rarity_tier=rare")

        # expected_keywords
        kws = e.get("expected_keywords", [])
        if not kws:
            violations.append(f"{eid}: expected_keywords is empty")

        # Keyword/question leakage
        question_lower = e.get("question", "").lower()
        for kw in kws:
            if len(kw) >= MIN_KEYWORD_LENGTH_FOR_LEAKAGE_CHECK and kw.lower() in question_lower:
                violations.append(
                    f"{eid}: keyword '{kw}' appears in question — "
                    "question must not contain expected_keywords (v3 policy)"
                )

    total = len(entries)
    # Size gate
    if total < MIN_ENTRIES:
        violations.append(f"Dataset too small: {total} entries (minimum {MIN_ENTRIES})")

    # Distribution gate
    for level, target in DISTRIBUTION_TARGETS.items():
        actual = keyword_levels.get(level, 0) / max(total, 1)
        if abs(actual - target) > DISTRIBUTION_TOLERANCE:
            violations.append(
                f"Distribution mismatch for '{level}': "
                f"{keyword_levels[level]}/{total} = {actual:.2f} "
                f"(target {target:.2f} ± {DISTRIBUTION_TOLERANCE})"
            )

    # Report
    print(f"  Entries parsed: {total}")
    dist_str = "  ".join(f"{k}={keyword_levels[k]}" for k in VALID_KNOWLEDGE_LEVELS)
    print(f"  Distribution:   {dist_str}")

    if violations:
        print(f"\n  VIOLATIONS ({len(violations)}):")
        for v in violations:
            print(f"    ✗ {v}")
        return len(violations)

    print("  ✓ All checks passed")
    return 0


# ─── Stub generator ────────────────────────────────────────────────────────────

def generate_stub(
    corpus_path: str,
    output_path: str,
    max_entries: int = 110,
    min_chunk_length: int = 80,
) -> None:
    """Emit a stub YAML template from the corpus.

    WARNING: Output requires mandatory human curation before use:
      - Rephrase every question so it does NOT contain any expected_keyword.
      - Fill in source_section for each entry.
      - Verify that each keyword is truly present in the cited source document.
      - Remove or replace synthetic/generic keywords.
    """
    with open(corpus_path, encoding="utf-8") as f:
        corpus = json.load(f)

    chunks = corpus if isinstance(corpus, list) else corpus.get("chunks", [])
    chunks = [c for c in chunks if len(c.get("text", "")) >= min_chunk_length]

    TODAY = date.today().isoformat()
    HEADER = f"""\
# ThemisDB RAG Golden Dataset — STUB (requires human curation)
# Generated: {TODAY}
# ──────────────────────────────────────────────────────────────────────────────
# MANDATORY CURATION TASKS before this file may replace the authoritative dataset:
#
#   1. For EVERY entry: rephrase the question so NO expected_keyword appears in it.
#      Test: grep -i "<keyword>" <question_text> must return empty.
#
#   2. Fill in source_section for every entry (heading path within source_document).
#
#   3. Verify each keyword actually appears in the cited source_document.
#
#   4. Replace auto-extracted "keywords" with specific, rare terms from the document
#      (technical identifiers, thresholds, version strings, test IDs, etc.).
#
#   5. Ensure distribution: 20% general / 30% specific / 50% specialized.
#
#   6. Run:  python scripts/generate_rag_golden_dataset.py --validate <this_file>
#      until 0 violations are reported.
#
# This file was auto-generated and MUST NOT be committed as-is.
# ──────────────────────────────────────────────────────────────────────────────

version: 3
dataset_name: themisdb_rag_golden_dataset_stub
created: "{TODAY}"
description: >
  AUTO-GENERATED STUB — human curation required.
  See header comments for mandatory tasks before use.
target_min_entries: {max_entries}
target_distribution: {{general: 0.20, specific: 0.30, specialized: 0.50}}
question_policy: keywords_must_not_appear_in_question

entries:

"""

    # Deduplicate by source file, pick chunks with identifiable tokens
    seen_docs: set[str] = set()
    stub_entries: list[str] = []
    for i, chunk in enumerate(chunks):
        if len(stub_entries) >= max_entries:
            break
        doc = chunk.get("source", chunk.get("doc_id", "unknown"))
        if doc in seen_docs:
            continue
        seen_docs.add(doc)

        text = chunk.get("text", "")
        # Extract candidate keywords: identifiers, version strings, numbers with units
        raw_kws = re.findall(
            r'[A-Z][A-Z0-9_]{3,}|v\d+\.\d+\.\d+|\d+[\.,]\d+\s*(?:ms|µs|%|GB|MB)|'
            r'[a-z][a-z0-9_]{4,}(?:::[a-z][a-z0-9_]+)+',
            text,
        )
        kws = list(dict.fromkeys(kw.strip() for kw in raw_kws))[:5]
        if not kws:
            kws = ["TODO_fill_keyword"]

        level = "general" if i % 5 == 0 else ("specific" if i % 5 <= 1 else "specialized")
        rarity = '"rare"' if level == "specialized" else '""'

        entry = (
            f"  - id: RAG-GD-{i+1:03d}\n"
            f"    category: TODO\n"
            f"    # CURATION: rephrase so NO keyword appears in this question\n"
            f"    question: \"TODO: Ask about the concept without naming it directly — "
            f"{', '.join(kws[:2])}\"\n"
            f"    expected_keywords: [{', '.join(kws)}]\n"
            f"    expected_source_hint: TODO\n"
            f"    knowledge_level: {level}\n"
            f"    rarity_tier: {rarity}\n"
            f"    min_recall_score: 0.70\n"
            f"    source_document: {doc}\n"
            f"    source_section: \"TODO: fill section heading\"\n"
            f"    indexed_at: \"{TODAY}\"\n"
        )
        stub_entries.append(entry)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(HEADER)
        f.write("\n".join(stub_entries))
        f.write("\n")

    print(f"Stub written: {output_path} ({len(stub_entries)} entries)")
    print("NOTE: This stub MUST be curated before use. See header comments.")


# ─── CLI ───────────────────────────────────────────────────────────────────────

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument("--validate", metavar="YAML",
                      help="Validate an existing dataset YAML and report violations.")
    mode.add_argument("--generate", action="store_true",
                      help="Generate a stub YAML template from the corpus (requires --input).")
    p.add_argument("--input", metavar="CORPUS_JSON",
                   help="Path to doku.db.json (required for --generate).")
    p.add_argument("--output", metavar="OUT_YAML",
                   help="Output path for --generate mode.")
    p.add_argument("--max-entries", type=int, default=110,
                   help="Maximum stub entries to emit (default: 110).")
    p.add_argument("--min-chunk-length", type=int, default=80,
                   help="Minimum source chunk length to consider (default: 80).")
    args = p.parse_args()

    if args.validate:
        n = validate(args.validate)
        return 1 if n > 0 else 0

    # --generate
    corpus = args.input or os.environ.get("THEMIS_DOCS_DB_JSON")
    if not corpus:
        p.error("--generate requires --input or THEMIS_DOCS_DB_JSON env var")
    if not os.path.isfile(corpus):
        p.error(f"Corpus file not found: {corpus}")
    out = args.output or "/tmp/rag_candidates_stub.yaml"
    generate_stub(corpus, out, max_entries=args.max_entries,
                  min_chunk_length=args.min_chunk_length)
    return 0


if __name__ == "__main__":
    sys.exit(main())
