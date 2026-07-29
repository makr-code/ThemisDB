#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import sys
import urllib.request
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

TOKEN_RE = re.compile(r"[A-Za-z0-9_\-]+", re.UNICODE)
HEADING_RE = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
SENTENCE_RE = re.compile(r"(?<=[.!?])\s+")
UNSAFE_PATTERNS = (
    "ignore previous instructions",
    "ignore all previous instructions",
    "system prompt",
    "reveal secret",
    "api key",
    "password",
    "private key",
)
CONTRADICTION_CUES = (
    "however",
    "but",
    "contradict",
    "in contrast",
    "on the other hand",
)


@dataclass
class Chunk:
    chunk_id: str
    file_path: str
    section_title: str
    line_start: int
    line_end: int
    text: str


class EmbeddingProvider:
    name = "base"

    def encode(self, texts: list[str]) -> list[list[float]]:
        raise NotImplementedError


class HashEmbeddingProvider(EmbeddingProvider):
    name = "hash"

    def __init__(self, dimensions: int) -> None:
        self.dimensions = dimensions

    def encode(self, texts: list[str]) -> list[list[float]]:
        return [self._embed_one(text) for text in texts]

    def _embed_one(self, text: str) -> list[float]:
        vec = [0.0] * self.dimensions
        freqs: dict[str, int] = {}
        for tok in TOKEN_RE.findall(text.lower()):
            freqs[tok] = freqs.get(tok, 0) + 1
        for tok, tf in freqs.items():
            digest = hashlib.blake2b(tok.encode("utf-8"), digest_size=16).digest()
            idx = int.from_bytes(digest[:4], "little") % self.dimensions
            sign = -1.0 if (digest[4] & 1) else 1.0
            vec[idx] += sign * (1.0 + math.log(float(tf)))
        return _normalize(vec)


class MockEmbeddingProvider(HashEmbeddingProvider):
    name = "mock"


class SentenceTransformerEmbeddingProvider(EmbeddingProvider):
    name = "sentence-transformers"

    def __init__(self, model_name: str) -> None:
        from sentence_transformers import SentenceTransformer  # type: ignore

        self.model_name = model_name
        self._model = SentenceTransformer(model_name)

    def encode(self, texts: list[str]) -> list[list[float]]:
        vectors = self._model.encode(texts, normalize_embeddings=True)
        return [[float(v) for v in row] for row in vectors]


class OpenAIEmbeddingProvider(EmbeddingProvider):
    name = "openai"

    def __init__(self, model_name: str, api_key: str) -> None:
        self.model_name = model_name
        self.api_key = api_key

    def encode(self, texts: list[str]) -> list[list[float]]:
        body = json.dumps({"model": self.model_name, "input": texts}).encode("utf-8")
        req = urllib.request.Request(
            "https://api.openai.com/v1/embeddings",
            data=body,
            headers={
                "Content-Type": "application/json",
                "Authorization": "Bearer " + self.api_key,
            },
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=30) as response:
            payload = json.loads(response.read().decode("utf-8"))
        return [[float(v) for v in row["embedding"]] for row in payload.get("data", [])]


WIKI_STATE_TEMPLATE: dict[str, Any] = {
    "version": "wiki-mvp-2",
    "sources": {},
    "pages": {},
    "links": [],
    "assertions": [],
    "tasks": [],
}


def _normalize(vec: list[float]) -> list[float]:
    norm = math.sqrt(sum(v * v for v in vec))
    if norm <= 0:
        return vec
    return [round(v / norm, 6) for v in vec]


def _tokenize(text: str) -> list[str]:
    return TOKEN_RE.findall(text)


def _slugify(text: str) -> str:
    base = re.sub(r"[^a-zA-Z0-9]+", "-", text.strip().lower()).strip("-")
    return base or "untitled"


def _utc_now() -> str:
    return datetime.now(UTC).isoformat()


def _default_schema_text() -> str:
    return (
        "# LLM Wiki Schema\n\n"
        "## Layers\n"
        "- raw_sources/: immutable source-of-truth files\n"
        "- wiki/pages/: LLM-maintained markdown pages\n"
        "- wiki/index.md: content catalog\n"
        "- wiki/log.md: append-only operation log\n\n"
        "## Page Types\n"
        "- source_summary\n- entity\n- concept\n- topic\n- synthesis\n- query_answer\n\n"
        "## Governance\n"
        "- Every claim should carry source references.\n"
        "- Contradictions create review tasks instead of silent overwrite.\n"
        "- Keep links explicit and bidirectional where useful.\n"
    )


def _workspace_paths(workspace_root: Path) -> dict[str, Path]:
    return {
        "root": workspace_root,
        "raw": workspace_root / "raw_sources",
        "wiki": workspace_root / "wiki",
        "pages": workspace_root / "wiki" / "pages",
        "index": workspace_root / "wiki" / "index.md",
        "log": workspace_root / "wiki" / "log.md",
        "schema": workspace_root / "wiki" / "schema.md",
        "state": workspace_root / "wiki" / "state.json",
        "cache_index": workspace_root / "wiki" / "wiki_index.json",
    }


def build_embedding_provider(provider_name: str, dimensions: int) -> EmbeddingProvider:
    provider = provider_name.lower().strip()
    if provider == "mock":
        return MockEmbeddingProvider(dimensions)
    if provider == "sentence-transformers":
        model_name = os.getenv("THEMIS_LLM_WIKI_EMBEDDING_MODEL", "sentence-transformers/all-MiniLM-L6-v2")
        try:
            return SentenceTransformerEmbeddingProvider(model_name)
        except Exception:
            return HashEmbeddingProvider(dimensions)
    if provider == "openai":
        api_key = os.getenv("OPENAI_API_KEY", "")
        model_name = os.getenv("OPENAI_EMBEDDING_MODEL", "text-embedding-3-small")
        if api_key:
            try:
                return OpenAIEmbeddingProvider(model_name, api_key)
            except Exception:
                pass
        return HashEmbeddingProvider(dimensions)
    return HashEmbeddingProvider(dimensions)


def discover_markdown_files(source_root: Path) -> list[Path]:
    excluded = {".git", "node_modules", "build", "vcpkg", "external", "__pycache__", ".venv"}
    files: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(source_root):
        current = Path(dirpath)
        rel_parts = set(current.relative_to(source_root).parts) if current != source_root else set()
        if rel_parts & excluded:
            continue
        dirnames[:] = [d for d in dirnames if d not in excluded]
        for filename in filenames:
            p = current / filename
            if p.suffix.lower() in {".md", ".markdown"}:
                files.append(p)
    return sorted(files)


def split_sections_with_lines(markdown_text: str) -> list[tuple[str, int, int, str]]:
    lines = markdown_text.splitlines()
    if not lines:
        return []
    sections: list[tuple[str, int, int, str]] = []
    current_title = "ROOT"
    current_start = 1
    buffer: list[str] = []
    for idx, raw_line in enumerate(lines, start=1):
        heading = HEADING_RE.match(raw_line)
        if heading:
            if buffer:
                sections.append((current_title, current_start, idx - 1, "\n".join(buffer).strip()))
            current_title = heading.group(2).strip()
            current_start = idx
            buffer = [raw_line]
            continue
        buffer.append(raw_line)
    if buffer:
        sections.append((current_title, current_start, len(lines), "\n".join(buffer).strip()))
    return [s for s in sections if s[3]]


def chunk_section_text(
    section_title: str,
    file_path: str,
    line_start: int,
    section_text: str,
    max_tokens: int,
    overlap_tokens: int,
    chunk_id_prefix: str,
) -> list[Chunk]:
    section_lines = section_text.splitlines()
    paragraphs: list[tuple[int, int, str]] = []
    para_start = 0
    para_buffer: list[str] = []
    for i, line in enumerate(section_lines):
        if line.strip():
            if not para_buffer:
                para_start = i
            para_buffer.append(line)
        elif para_buffer:
            paragraphs.append((para_start, i - 1, "\n".join(para_buffer).strip()))
            para_buffer = []
    if para_buffer:
        paragraphs.append((para_start, len(section_lines) - 1, "\n".join(para_buffer).strip()))

    chunks: list[Chunk] = []
    buffer_parts: list[tuple[int, int, str]] = []
    token_count = 0
    chunk_index = 0

    def flush() -> None:
        nonlocal buffer_parts, token_count, chunk_index
        if not buffer_parts:
            return
        c_start = line_start + buffer_parts[0][0]
        c_end = line_start + buffer_parts[-1][1]
        c_text = "\n\n".join(p[2] for p in buffer_parts).strip()
        chunk_id = f"{chunk_id_prefix}-{chunk_index}"
        chunks.append(
            Chunk(
                chunk_id=chunk_id,
                file_path=file_path,
                section_title=section_title,
                line_start=c_start,
                line_end=c_end,
                text=c_text,
            )
        )
        chunk_index += 1
        if overlap_tokens > 0 and c_text:
            overlap = _tokenize(c_text)[-overlap_tokens:]
            buffer_parts = [(buffer_parts[-1][0], buffer_parts[-1][1], " ".join(overlap))] if overlap else []
            token_count = len(overlap)
        else:
            buffer_parts = []
            token_count = 0

    for para in paragraphs:
        p_tokens = len(_tokenize(para[2]))
        if p_tokens == 0:
            continue
        if token_count > 0 and token_count + p_tokens > max_tokens:
            flush()
        buffer_parts.append(para)
        token_count += p_tokens
    flush()
    return chunks


def ingest_markdown(source_root: Path, max_tokens: int, overlap_tokens: int) -> list[Chunk]:
    chunks: list[Chunk] = []
    for path in discover_markdown_files(source_root):
        rel = path.relative_to(source_root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        sections = split_sections_with_lines(text)
        for section_idx, (title, start_line, _, section_text) in enumerate(sections):
            chunk_prefix = hashlib.sha1(f"{rel}:{title}:{section_idx}".encode("utf-8")).hexdigest()[:12]
            chunks.extend(
                chunk_section_text(
                    section_title=title,
                    file_path=rel,
                    line_start=start_line,
                    section_text=section_text,
                    max_tokens=max_tokens,
                    overlap_tokens=overlap_tokens,
                    chunk_id_prefix=chunk_prefix,
                )
            )
    return chunks


def _contains_unsafe_pattern(text: str) -> bool:
    lowered = text.lower()
    return any(p in lowered for p in UNSAFE_PATTERNS)


def sanitize_query(query: str) -> tuple[str, bool]:
    compact = " ".join(query.strip().split())
    return compact, _contains_unsafe_pattern(compact)


def build_index(
    source_root: Path,
    output_path: Path,
    provider_name: str,
    dimensions: int,
    max_tokens: int,
    overlap_tokens: int,
) -> dict[str, Any]:
    chunks = ingest_markdown(source_root, max_tokens=max_tokens, overlap_tokens=overlap_tokens)
    provider = build_embedding_provider(provider_name, dimensions=dimensions)
    embeddings = provider.encode([c.text for c in chunks]) if chunks else []
    records = []
    for idx, chunk in enumerate(chunks):
        emb = embeddings[idx] if idx < len(embeddings) else [0.0] * dimensions
        records.append(
            {
                "chunk_id": chunk.chunk_id,
                "file_path": chunk.file_path,
                "section_title": chunk.section_title,
                "line_start": chunk.line_start,
                "line_end": chunk.line_end,
                "token_count": len(_tokenize(chunk.text)),
                "text": chunk.text,
                "embedding": emb,
            }
        )
    artifact = {
        "version": "mvp-1",
        "generated_at": _utc_now(),
        "source_root": str(source_root.resolve()),
        "embedding": {
            "provider": provider.name,
            "requested_provider": provider_name,
            "dimensions": dimensions,
        },
        "chunking": {"max_tokens": max_tokens, "overlap_tokens": overlap_tokens},
        "chunks": records,
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(artifact, indent=2, ensure_ascii=False), encoding="utf-8")
    return artifact


def cosine_similarity(a: list[float], b: list[float]) -> float:
    if not a or not b or len(a) != len(b):
        return 0.0
    dot = sum(x * y for x, y in zip(a, b))
    an = math.sqrt(sum(x * x for x in a))
    bn = math.sqrt(sum(y * y for y in b))
    if an <= 0.0 or bn <= 0.0:
        return 0.0
    return dot / (an * bn)


def query_index(
    index_path: Path,
    question: str,
    top_k: int,
    min_score: float,
    provider_name: str | None = None,
) -> dict[str, Any]:
    raw = json.loads(index_path.read_text(encoding="utf-8"))
    safe_question, query_flagged = sanitize_query(question)
    emb_meta = raw.get("embedding", {})
    dimensions = int(emb_meta.get("dimensions", 384))
    provider = build_embedding_provider(provider_name or emb_meta.get("provider", "hash"), dimensions=dimensions)
    q_embedding = provider.encode([safe_question])[0]

    scored: list[dict[str, Any]] = []
    filtered_unsafe = 0
    for chunk in raw.get("chunks", []):
        if _contains_unsafe_pattern(chunk.get("text", "")):
            filtered_unsafe += 1
            continue
        score = cosine_similarity(q_embedding, chunk.get("embedding", []))
        if score < min_score:
            continue
        scored.append(
            {
                "score": round(score, 6),
                "source": {
                    "file_path": chunk.get("file_path", ""),
                    "section_title": chunk.get("section_title", ""),
                    "line_start": chunk.get("line_start", 0),
                    "line_end": chunk.get("line_end", 0),
                },
                "text_preview": chunk.get("text", "")[:400],
            }
        )
    scored.sort(key=lambda item: item["score"], reverse=True)
    return {
        "query": safe_question,
        "query_flagged_for_prompt_injection": query_flagged,
        "filtered_unsafe_chunks": filtered_unsafe,
        "top_k": top_k,
        "min_score": min_score,
        "results": scored[:top_k],
    }


def _format_result(result: dict[str, Any]) -> str:
    lines = [
        f"Query: {result['query']}",
        f"Prompt-injection flagged: {'yes' if result['query_flagged_for_prompt_injection'] else 'no'}",
        f"Filtered unsafe chunks: {result['filtered_unsafe_chunks']}",
        "",
    ]
    if not result["results"]:
        lines.append("No results above threshold.")
        return "\n".join(lines)
    for idx, item in enumerate(result["results"], start=1):
        src = item["source"]
        lines.extend(
            [
                f"[{idx}] score={item['score']}",
                f"    source: {src['file_path']} :: {src['section_title']} (lines {src['line_start']}-{src['line_end']})",
                f"    preview: {item['text_preview'].replace(chr(10), ' ')}",
            ]
        )
    return "\n".join(lines)


def init_wiki_workspace(workspace_root: Path, schema_text: str | None = None) -> dict[str, Any]:
    p = _workspace_paths(workspace_root)
    p["raw"].mkdir(parents=True, exist_ok=True)
    p["pages"].mkdir(parents=True, exist_ok=True)
    if not p["schema"].exists():
        p["schema"].write_text(schema_text or _default_schema_text(), encoding="utf-8")
    if not p["index"].exists():
        p["index"].write_text("# index\n\n", encoding="utf-8")
    if not p["log"].exists():
        p["log"].write_text("# log\n\n", encoding="utf-8")
    if not p["state"].exists():
        p["state"].write_text(json.dumps(WIKI_STATE_TEMPLATE, indent=2, ensure_ascii=False), encoding="utf-8")
    return {"workspace_root": str(workspace_root), "created": True}


def _load_state(workspace_root: Path) -> dict[str, Any]:
    p = _workspace_paths(workspace_root)
    if not p["state"].exists():
        init_wiki_workspace(workspace_root)
    raw = json.loads(p["state"].read_text(encoding="utf-8"))
    state = dict(WIKI_STATE_TEMPLATE)
    state.update(raw)
    for field in ("sources", "pages"):
        state[field] = dict(state.get(field, {}))
    for field in ("links", "assertions", "tasks"):
        state[field] = list(state.get(field, []))
    return state


def _save_state(workspace_root: Path, state: dict[str, Any]) -> None:
    _workspace_paths(workspace_root)["state"].write_text(
        json.dumps(state, indent=2, ensure_ascii=False), encoding="utf-8"
    )


def _append_log(workspace_root: Path, operation: str, title: str, details: list[str] | None = None) -> None:
    p = _workspace_paths(workspace_root)
    timestamp = datetime.now(UTC).strftime("%Y-%m-%d")
    body = [f"## [{timestamp}] {operation} | {title}"]
    for detail in (details or []):
        body.append(f"- {detail}")
    body.append("")
    with p["log"].open("a", encoding="utf-8") as handle:
        handle.write("\n".join(body))


def _summarize_text(text: str, sentence_limit: int = 4) -> list[str]:
    compact = re.sub(r"\s+", " ", text.strip())
    if not compact:
        return ["No textual content extracted."]
    parts = [p.strip() for p in SENTENCE_RE.split(compact) if p.strip()]
    if not parts:
        return [compact[:300]]
    selected = parts[:sentence_limit]
    return [line if line.endswith((".", "!", "?")) else line + "." for line in selected]


def _extract_topic_terms(text: str, max_terms: int = 5) -> list[str]:
    stopwords = {
        "the",
        "and",
        "for",
        "with",
        "this",
        "that",
        "from",
        "into",
        "your",
        "their",
        "about",
        "when",
        "where",
        "which",
        "have",
        "will",
        "shall",
        "could",
        "should",
        "would",
        "also",
        "more",
        "less",
        "than",
        "each",
        "them",
        "they",
        "what",
        "wie",
        "und",
        "der",
        "die",
        "das",
        "mit",
        "von",
        "für",
        "eine",
        "einer",
        "einem",
        "über",
    }
    counts: dict[str, int] = {}
    for token in TOKEN_RE.findall(text.lower()):
        if len(token) < 5 or token in stopwords:
            continue
        counts[token] = counts.get(token, 0) + 1
    ranked = sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    return [term for term, _ in ranked[:max_terms]]


def _register_page(
    state: dict[str, Any],
    page_id: str,
    page_title: str,
    path: str,
    page_type: str,
    source_ids: list[str],
) -> None:
    state["pages"][page_id] = {
        "title": page_title,
        "path": path,
        "type": page_type,
        "source_ids": sorted(set(source_ids)),
        "updated_at": _utc_now(),
    }


def _add_link(state: dict[str, Any], from_page: str, to_page: str, relation: str, evidence_source: str) -> None:
    link = {
        "from": from_page,
        "to": to_page,
        "relation": relation,
        "evidence_source": evidence_source,
        "created_at": _utc_now(),
    }
    if link not in state["links"]:
        state["links"].append(link)


def _rewrite_index(workspace_root: Path, state: dict[str, Any]) -> None:
    p = _workspace_paths(workspace_root)
    by_type: dict[str, list[tuple[str, dict[str, Any]]]] = {}
    for page_id, meta in state["pages"].items():
        by_type.setdefault(meta.get("type", "unknown"), []).append((page_id, meta))
    lines = ["# index", ""]
    for category in sorted(by_type.keys()):
        lines.append(f"## {category}")
        for page_id, meta in sorted(by_type[category], key=lambda item: item[1].get("title", "")):
            lines.append(
                f"- [{meta.get('title', page_id)}](pages/{meta.get('path','')}) "
                f"— id: `{page_id}`; sources: {len(meta.get('source_ids', []))}"
            )
        lines.append("")
    p["index"].write_text("\n".join(lines).strip() + "\n", encoding="utf-8")


def ingest_source(
    workspace_root: Path,
    source_path: Path,
    title: str | None = None,
    provider_name: str = "hash",
    embedding_dim: int = 384,
) -> dict[str, Any]:
    init_wiki_workspace(workspace_root)
    p = _workspace_paths(workspace_root)
    state = _load_state(workspace_root)

    content = source_path.read_text(encoding="utf-8", errors="replace")
    source_hash = hashlib.sha1(content.encode("utf-8")).hexdigest()[:16]
    source_id = f"source-{source_hash}"
    source_title = title or source_path.stem.replace("_", " ").replace("-", " ").strip().title()
    raw_copy = p["raw"] / f"{source_id}-{source_path.name}"
    if not raw_copy.exists():
        shutil.copyfile(source_path, raw_copy)

    state["sources"][source_id] = {
        "title": source_title,
        "original_path": str(source_path),
        "raw_copy": raw_copy.name,
        "sha1": source_hash,
        "ingested_at": _utc_now(),
    }

    summary_lines = _summarize_text(content)
    page_id = f"page-source-{_slugify(source_title)}"
    page_file = f"{_slugify(source_title)}.md"
    page_path = p["pages"] / page_file
    page_content = [
        f"# {source_title}",
        "",
        "## Type",
        "source_summary",
        "",
        "## Summary",
        *[f"- {line}" for line in summary_lines],
        "",
        "## Sources",
        f"- `{source_id}` → `raw_sources/{raw_copy.name}`",
        "",
    ]
    page_path.write_text("\n".join(page_content), encoding="utf-8")
    _register_page(state, page_id, source_title, page_file, "source_summary", [source_id])

    terms = _extract_topic_terms(content)
    concept_pages: list[str] = []
    for term in terms:
        concept_id = f"page-concept-{_slugify(term)}"
        concept_file = f"concept-{_slugify(term)}.md"
        concept_pages.append(concept_id)
        concept_path = p["pages"] / concept_file
        if concept_path.exists():
            existing = concept_path.read_text(encoding="utf-8")
        else:
            existing = f"# Concept: {term}\n\n## Linked Sources\n"
        marker = f"- [{source_title}]({page_file})"
        if marker not in existing:
            existing = existing.rstrip() + f"\n{marker}\n"
        concept_path.write_text(existing, encoding="utf-8")
        _register_page(state, concept_id, f"Concept: {term}", concept_file, "concept", [source_id])
        _add_link(state, page_id, concept_id, "about", source_id)

    for cue in CONTRADICTION_CUES:
        if cue in content.lower():
            state["tasks"].append(
                {
                    "id": f"task-{hashlib.sha1((source_id + cue).encode('utf-8')).hexdigest()[:10]}",
                    "type": "contradiction_review",
                    "status": "open",
                    "source_id": source_id,
                    "note": f"Cue detected: {cue}",
                    "created_at": _utc_now(),
                }
            )
            break
    if _contains_unsafe_pattern(content):
        state["tasks"].append(
            {
                "id": f"task-{hashlib.sha1((source_id + 'unsafe').encode('utf-8')).hexdigest()[:10]}",
                "type": "needs_review",
                "status": "open",
                "source_id": source_id,
                "note": "Unsafe prompt-injection style pattern detected in source text.",
                "created_at": _utc_now(),
            }
        )

    state["assertions"].append(
        {
            "id": f"assert-{hashlib.sha1((source_id + page_id).encode('utf-8')).hexdigest()[:10]}",
            "page_id": page_id,
            "source_id": source_id,
            "confidence": 0.55,
            "summary": summary_lines[0],
            "created_at": _utc_now(),
        }
    )

    _save_state(workspace_root, state)
    _rewrite_index(workspace_root, state)
    _append_log(
        workspace_root,
        operation="ingest",
        title=source_title,
        details=[
            f"source_id: {source_id}",
            f"summary_page: pages/{page_file}",
            f"concept_links: {len(concept_pages)}",
            f"embedding_provider: {provider_name}",
            f"embedding_dim: {embedding_dim}",
        ],
    )

    return {
        "source_id": source_id,
        "summary_page": str(page_path),
        "concept_links": len(concept_pages),
        "tasks_open": len([t for t in state["tasks"] if t.get("status") == "open"]),
    }


def query_workspace(
    workspace_root: Path,
    question: str,
    top_k: int,
    min_score: float,
    provider_name: str,
    embedding_dim: int,
    save_as_page: bool = False,
    page_title: str | None = None,
) -> dict[str, Any]:
    init_wiki_workspace(workspace_root)
    p = _workspace_paths(workspace_root)
    state = _load_state(workspace_root)
    build_index(
        source_root=p["wiki"],
        output_path=p["cache_index"],
        provider_name=provider_name,
        dimensions=embedding_dim,
        max_tokens=220,
        overlap_tokens=40,
    )
    result = query_index(
        index_path=p["cache_index"],
        question=question,
        top_k=top_k,
        min_score=min_score,
        provider_name=provider_name,
    )
    if save_as_page:
        answer_title = page_title or f"Query: {question[:60]}"
        answer_slug = _slugify(answer_title)
        answer_id = f"page-query-{answer_slug}"
        answer_file = f"query-{answer_slug}.md"
        answer_path = p["pages"] / answer_file
        citations = [
            f"- {r['source']['file_path']}::{r['source']['section_title']}:{r['source']['line_start']}-{r['source']['line_end']}"
            for r in result["results"]
        ]
        content = [
            f"# {answer_title}",
            "",
            "## Question",
            question,
            "",
            "## Evidence",
            *(citations or ["- no citations found"]),
            "",
            "## Draft Answer",
            "This page captures retrieval evidence and should be refined by the LLM maintainer.",
            "",
        ]
        answer_path.write_text("\n".join(content), encoding="utf-8")
        _register_page(state, answer_id, answer_title, answer_file, "query_answer", [])
        _save_state(workspace_root, state)
        _rewrite_index(workspace_root, state)
        _append_log(
            workspace_root,
            operation="query",
            title=answer_title,
            details=[f"question: {question}", f"saved_page: pages/{answer_file}", f"hits: {len(result['results'])}"],
        )
        result["saved_page"] = str(answer_path)
    return result


def lint_workspace(workspace_root: Path) -> dict[str, Any]:
    init_wiki_workspace(workspace_root)
    state = _load_state(workspace_root)

    incoming: dict[str, int] = {page_id: 0 for page_id in state["pages"].keys()}
    for link in state["links"]:
        target = link.get("to")
        if target in incoming:
            incoming[target] += 1

    orphan_pages = [
        page_id
        for page_id, count in incoming.items()
        if count == 0 and state["pages"].get(page_id, {}).get("type") not in {"source_summary"}
    ]

    link_pairs = {(l.get("from"), l.get("to")) for l in state["links"]}
    missing_backlinks = []
    for src, dst in sorted(link_pairs):
        if (dst, src) not in link_pairs:
            missing_backlinks.append({"from": src, "to": dst})

    latest_source_time = ""
    for source in state["sources"].values():
        latest_source_time = max(latest_source_time, str(source.get("ingested_at", "")))

    stale_synthesis = []
    for page_id, meta in state["pages"].items():
        if meta.get("type") != "synthesis":
            continue
        if str(meta.get("updated_at", "")) < latest_source_time:
            stale_synthesis.append(page_id)

    unresolved_contradictions = [
        t
        for t in state["tasks"]
        if t.get("type") in {"contradiction_review", "needs_review"} and t.get("status") == "open"
    ]

    report = {
        "orphan_pages": orphan_pages,
        "missing_backlinks": missing_backlinks,
        "stale_synthesis_pages": stale_synthesis,
        "unresolved_contradictions": unresolved_contradictions,
    }
    _append_log(
        workspace_root,
        operation="lint",
        title="workspace health",
        details=[
            f"orphans: {len(orphan_pages)}",
            f"missing_backlinks: {len(missing_backlinks)}",
            f"stale_synthesis: {len(stale_synthesis)}",
            f"open_contradictions: {len(unresolved_contradictions)}",
        ],
    )
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="ThemisDB LLM Wiki MVP (index/query + persistent wiki operations)")
    sub = parser.add_subparsers(dest="command", required=True)

    index_cmd = sub.add_parser("index", help="Index markdown docs into an embeddable JSON artifact")
    index_cmd.add_argument("--source-root", default=".", help="Root directory containing markdown docs")
    index_cmd.add_argument("--output", default="artifacts/llm-wiki-mvp/index.json", help="Output index JSON")
    index_cmd.add_argument("--embedding-provider", default=os.getenv("THEMIS_LLM_WIKI_EMBEDDING_PROVIDER", "hash"))
    index_cmd.add_argument("--embedding-dim", type=int, default=384)
    index_cmd.add_argument("--chunk-max-tokens", type=int, default=220)
    index_cmd.add_argument("--chunk-overlap-tokens", type=int, default=40)

    query_cmd = sub.add_parser("query", help="Run top-k retrieval against a generated index")
    query_cmd.add_argument("--index", default="artifacts/llm-wiki-mvp/index.json")
    query_cmd.add_argument("--question", required=True)
    query_cmd.add_argument("--top-k", type=int, default=5)
    query_cmd.add_argument("--min-score", type=float, default=0.15)
    query_cmd.add_argument("--embedding-provider", default=None)
    query_cmd.add_argument("--json", action="store_true", dest="as_json")

    init_cmd = sub.add_parser("wiki-init", help="Initialize a persistent wiki workspace")
    init_cmd.add_argument("--workspace-root", required=True)

    ingest_cmd = sub.add_parser("wiki-ingest", help="Ingest one immutable source into wiki workspace")
    ingest_cmd.add_argument("--workspace-root", required=True)
    ingest_cmd.add_argument("--source", required=True)
    ingest_cmd.add_argument("--title", default=None)
    ingest_cmd.add_argument("--embedding-provider", default=os.getenv("THEMIS_LLM_WIKI_EMBEDDING_PROVIDER", "hash"))
    ingest_cmd.add_argument("--embedding-dim", type=int, default=384)

    wquery_cmd = sub.add_parser("wiki-query", help="Query the persistent wiki workspace")
    wquery_cmd.add_argument("--workspace-root", required=True)
    wquery_cmd.add_argument("--question", required=True)
    wquery_cmd.add_argument("--top-k", type=int, default=5)
    wquery_cmd.add_argument("--min-score", type=float, default=0.1)
    wquery_cmd.add_argument("--embedding-provider", default=os.getenv("THEMIS_LLM_WIKI_EMBEDDING_PROVIDER", "hash"))
    wquery_cmd.add_argument("--embedding-dim", type=int, default=384)
    wquery_cmd.add_argument("--save-as-page", action="store_true")
    wquery_cmd.add_argument("--page-title", default=None)
    wquery_cmd.add_argument("--json", action="store_true", dest="as_json")

    lint_cmd = sub.add_parser("wiki-lint", help="Run health checks over the persistent wiki workspace")
    lint_cmd.add_argument("--workspace-root", required=True)
    lint_cmd.add_argument("--json", action="store_true", dest="as_json")

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "index":
        artifact = build_index(
            source_root=Path(args.source_root),
            output_path=Path(args.output),
            provider_name=args.embedding_provider,
            dimensions=args.embedding_dim,
            max_tokens=args.chunk_max_tokens,
            overlap_tokens=args.chunk_overlap_tokens,
        )
        print(
            f"Indexed {len(artifact.get('chunks', []))} chunks to {args.output} "
            f"(provider={artifact['embedding']['provider']})"
        )
        return 0

    if args.command == "query":
        result = query_index(
            index_path=Path(args.index),
            question=args.question,
            top_k=args.top_k,
            min_score=args.min_score,
            provider_name=args.embedding_provider,
        )
        if args.as_json:
            print(json.dumps(result, indent=2, ensure_ascii=False))
        else:
            print(_format_result(result))
        return 0

    if args.command == "wiki-init":
        result = init_wiki_workspace(Path(args.workspace_root))
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return 0

    if args.command == "wiki-ingest":
        result = ingest_source(
            workspace_root=Path(args.workspace_root),
            source_path=Path(args.source),
            title=args.title,
            provider_name=args.embedding_provider,
            embedding_dim=args.embedding_dim,
        )
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return 0

    if args.command == "wiki-query":
        result = query_workspace(
            workspace_root=Path(args.workspace_root),
            question=args.question,
            top_k=args.top_k,
            min_score=args.min_score,
            provider_name=args.embedding_provider,
            embedding_dim=args.embedding_dim,
            save_as_page=args.save_as_page,
            page_title=args.page_title,
        )
        if args.as_json:
            print(json.dumps(result, indent=2, ensure_ascii=False))
        else:
            print(_format_result(result))
        return 0

    if args.command == "wiki-lint":
        result = lint_workspace(Path(args.workspace_root))
        if args.as_json:
            print(json.dumps(result, indent=2, ensure_ascii=False))
        else:
            print(json.dumps(result, indent=2, ensure_ascii=False))
        return 0

    parser.print_help()
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
