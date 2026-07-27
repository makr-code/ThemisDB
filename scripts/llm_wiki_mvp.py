#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import sys
import urllib.request
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

TOKEN_RE = re.compile(r"[A-Za-z0-9_\-]+", re.UNICODE)
HEADING_RE = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
UNSAFE_PATTERNS = (
    "ignore previous instructions",
    "ignore all previous instructions",
    "system prompt",
    "reveal secret",
    "api key",
    "password",
    "private key",
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


def _normalize(vec: list[float]) -> list[float]:
    norm = math.sqrt(sum(v * v for v in vec))
    if norm <= 0:
        return vec
    return [round(v / norm, 6) for v in vec]


def _tokenize(text: str) -> list[str]:
    return TOKEN_RE.findall(text)


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
        "generated_at": datetime.now(UTC).isoformat(),
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


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="ThemisDB LLM Wiki MVP (index/query)")
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


if __name__ == "__main__":
    raise SystemExit(main())
