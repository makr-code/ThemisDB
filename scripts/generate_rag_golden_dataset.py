#!/usr/bin/env python3
"""
generate_rag_golden_dataset.py — Bootstrap generator for the ThemisDB RAG golden dataset.

Reads a docs_database.json file (produced by scripts/generate_docs_database.py)
and emits candidate question/keyword entries in YAML format.  The output is
intended as a first-pass draft that a human curator should review and reduce to
the authoritative golden dataset at tests/llm/data/themisdb_rag_golden_dataset.yaml.

Usage:
    python scripts/generate_rag_golden_dataset.py \
        --input  build/data/docs_database.json \
        --output /tmp/rag_golden_candidates.yaml \
        [--max-entries 80] \
        [--min-chunk-length 80]

Environment variables:
    THEMIS_DOCS_DB_JSON  — fallback path to docs_database.json
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
import textwrap
from typing import Any


# ---------------------------------------------------------------------------
# Keyword extraction heuristics
# ---------------------------------------------------------------------------

# Technical terms that are high-value retrieval signals
_TECH_TERMS_RE = re.compile(
    r"\b("
    r"WikiIndexStore|BM25|HNSW|RRF|RocksDB|AdaLoRA|LoRA|GGUF|llama\.cpp"
    r"|sccache|RAII|CMake|preset|checkpoint|plugin|stream|inference"
    r"|Phase\s*[1-6]|Wave\s*[A-D]|community|military|develop"
    r"|DEB|RPM|CPack|PEFT|tensor.train|TT.core"
    r"|WikiRagSource|WikiChunkSplitter|InlineTrainingEngine|AdaLoraTTBridge"
    r")\b",
    re.IGNORECASE,
)

# Heading patterns that suggest a retrievable topic
_HEADING_PATTERNS = [
    re.compile(r"^#{1,4}\s+(.+)$", re.MULTILINE),   # Markdown headings
    re.compile(r"^([A-Z][A-Za-z ]{4,40})\s*$", re.MULTILINE),  # Capitalised lines
]


def _extract_keywords_from_text(text: str, max_kw: int = 5) -> list[str]:
    """Return up to *max_kw* unique technical keywords found in *text*."""
    seen: dict[str, int] = {}
    for m in _TECH_TERMS_RE.finditer(text):
        kw = m.group(0).strip()
        # Normalise case variants
        canonical = kw[0].upper() + kw[1:]
        seen[canonical] = seen.get(canonical, 0) + 1

    # Sort by frequency descending; keep top max_kw
    ranked = sorted(seen.items(), key=lambda x: -x[1])
    return [kw for kw, _ in ranked[:max_kw]]


def _extract_topic_from_heading(text: str) -> str | None:
    """Return the first meaningful heading found in *text*, or None."""
    for pat in _HEADING_PATTERNS:
        for m in pat.finditer(text):
            heading = m.group(1).strip() if pat.groups else m.group(0).strip()
            heading = re.sub(r"[#*`_]", "", heading).strip()
            if 5 < len(heading) < 80:
                return heading
    return None


# ---------------------------------------------------------------------------
# Question templates
# ---------------------------------------------------------------------------

_QUESTION_TEMPLATES = [
    "What is {topic} in ThemisDB?",
    "How does {topic} work in ThemisDB?",
    "Describe the purpose of {topic}.",
    "What is the role of {topic} in the ThemisDB architecture?",
    "How is {topic} configured or used?",
]


def _make_question(topic: str, idx: int) -> str:
    tpl = _QUESTION_TEMPLATES[idx % len(_QUESTION_TEMPLATES)]
    return tpl.format(topic=topic)


# ---------------------------------------------------------------------------
# YAML helpers (no external library dependency)
# ---------------------------------------------------------------------------

def _yaml_str(s: str) -> str:
    """Wrap a string in double quotes if it contains special YAML chars."""
    if any(c in s for c in ':[]{}#&*?|<>=!,%@`\'"'):
        escaped = s.replace('"', '\\"')
        return f'"{escaped}"'
    return s


def _entry_to_yaml(entry: dict[str, Any]) -> str:
    lines = [
        f"  - id: {entry['id']}",
        f"    category: {entry['category']}",
        f"    question: {_yaml_str(entry['question'])}",
        "    expected_keywords: [" + ", ".join(entry['expected_keywords']) + "]",
        f"    expected_source_hint: {_yaml_str(entry['expected_source_hint'])}",
        f"    min_recall_score: {entry['min_recall_score']:.1f}",
        "",
    ]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Core processing
# ---------------------------------------------------------------------------

def load_chunks(json_path: pathlib.Path) -> list[dict]:
    """Load chunks from a docs_database.json file.

    The file may be a flat list of objects or a dict with a 'documents'/'chunks'
    key — both layouts are handled.
    """
    raw = json.loads(json_path.read_text(encoding="utf-8"))
    if isinstance(raw, list):
        return raw
    for key in ("documents", "chunks", "entries", "data"):
        if key in raw and isinstance(raw[key], list):
            return raw[key]
    raise ValueError(
        f"Unrecognised docs_database.json layout — expected list or dict with "
        f"'documents'/'chunks' key, got keys: {list(raw.keys())[:10]}"
    )


def generate_candidates(chunks: list[dict], max_entries: int,
                         min_chunk_length: int) -> list[dict]:
    """Generate candidate golden-dataset entries from *chunks*."""
    entries: list[dict] = []
    seen_topics: set[str] = set()
    entry_idx = 0

    for chunk in chunks:
        if len(entries) >= max_entries:
            break

        # Extract text content
        content: str = (
            chunk.get("content") or
            chunk.get("text") or
            chunk.get("body") or
            ""
        )
        if len(content) < min_chunk_length:
            continue

        # Source hint
        source: str = (
            chunk.get("doc_id") or
            chunk.get("source") or
            chunk.get("file") or
            ""
        )
        # Use only the last path component to keep hints short
        if source:
            source = pathlib.Path(source).stem[:40]

        # Topic from heading
        topic = _extract_topic_from_heading(content)
        if topic is None:
            # Fallback: use first sentence / first 60 chars
            first_line = content.strip().split("\n")[0][:60].strip()
            if len(first_line) > 10:
                topic = first_line
            else:
                continue

        # Deduplicate by normalised topic
        norm_topic = re.sub(r"\W+", " ", topic).strip().lower()
        if norm_topic in seen_topics:
            continue
        seen_topics.add(norm_topic)

        # Keywords
        keywords = _extract_keywords_from_text(content)
        if not keywords:
            # Fall back to topic words
            keywords = [w for w in topic.split()[:4] if len(w) > 3]
        if not keywords:
            continue

        entry_idx += 1
        entries.append({
            "id": f"RAG-GD-CAND-{entry_idx:03d}",
            "category": "generated",
            "question": _make_question(topic, entry_idx),
            "expected_keywords": keywords[:5],
            "expected_source_hint": source,
            "min_recall_score": 0.7,
        })

    return entries


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

_HEADER = textwrap.dedent("""\
    # ThemisDB RAG Golden Dataset — CANDIDATE DRAFT
    # ─────────────────────────────────────────────────────────────────────────────
    # AUTO-GENERATED by scripts/generate_rag_golden_dataset.py
    # REQUIRES human review before use as the authoritative golden dataset.
    # Merge selected entries into tests/llm/data/themisdb_rag_golden_dataset.yaml
    # ─────────────────────────────────────────────────────────────────────────────

    version: 1
    dataset_name: themisdb_rag_golden_dataset_candidates
    description: >
      Automatically generated candidate entries.  Review and curate before use.

    entries:

    """)


def write_yaml(entries: list[dict], out_path: pathlib.Path) -> None:
    lines = [_HEADER]
    for e in entries:
        lines.append(_entry_to_yaml(e))
    out_path.write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _find_default_input() -> pathlib.Path | None:
    env = pathlib.Path(
        __import__("os").environ.get("THEMIS_DOCS_DB_JSON",
                                     "build/data/docs_database.json")
    )
    if env.is_file():
        return env
    candidates = [
        pathlib.Path("build/data/docs_database.json"),
        pathlib.Path("../build/data/docs_database.json"),
        pathlib.Path("data/docs_database.json"),
    ]
    for c in candidates:
        if c.is_file():
            return c
    return None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate candidate RAG golden dataset entries from docs_database.json"
    )
    parser.add_argument("--input", "-i", type=pathlib.Path, default=None,
                        help="Path to docs_database.json")
    parser.add_argument("--output", "-o", type=pathlib.Path,
                        default=pathlib.Path("/tmp/rag_golden_candidates.yaml"),
                        help="Output YAML file path")
    parser.add_argument("--max-entries", type=int, default=80,
                        help="Maximum number of candidate entries to generate (default: 80)")
    parser.add_argument("--min-chunk-length", type=int, default=80,
                        help="Minimum chunk character length to consider (default: 80)")
    args = parser.parse_args()

    input_path: pathlib.Path | None = args.input
    if input_path is None:
        input_path = _find_default_input()
    if input_path is None or not input_path.is_file():
        print(
            "ERROR: docs_database.json not found.  "
            "Run 'cmake --build build --target docs_database' first, "
            "or pass --input <path>.",
            file=sys.stderr,
        )
        return 1

    print(f"[rag-golden-gen] Loading chunks from {input_path} …", flush=True)
    try:
        chunks = load_chunks(input_path)
    except (json.JSONDecodeError, ValueError, OSError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(f"[rag-golden-gen] Loaded {len(chunks)} chunks", flush=True)
    entries = generate_candidates(chunks, args.max_entries, args.min_chunk_length)
    print(f"[rag-golden-gen] Generated {len(entries)} candidate entries", flush=True)

    write_yaml(entries, args.output)
    print(f"[rag-golden-gen] Written to {args.output}", flush=True)
    print(
        "[rag-golden-gen] NEXT STEP: Review and curate the output, then merge "
        "selected entries into tests/llm/data/themisdb_rag_golden_dataset.yaml",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
