#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Build a production-ready offline documentation artifact.

Pipeline stages:
1) Repository markdown discovery (with excludes)
2) Markdown parsing (headings, links)
3) Hybrid chunking (paragraph/sentence/token) with overlap
4) Offline embedding generation + persistence in artifact
5) Graph edge generation (links/headings/chunk adjacency)

The output stays load-only for runtime use by `DocsAssistant`.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import logging
import math
import os
import re
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


if sys.platform == "win32":
    import codecs

    if hasattr(sys.stdout, "buffer"):
        sys.stdout = codecs.getwriter("utf-8")(sys.stdout.buffer, errors="replace")
    if hasattr(sys.stderr, "buffer"):
        sys.stderr = codecs.getwriter("utf-8")(sys.stderr.buffer, errors="replace")


SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent


class UTF8StreamHandler(logging.StreamHandler):
    def __init__(self) -> None:
        super().__init__()
        if hasattr(sys.stderr, "buffer"):
            self.stream = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
        else:
            self.stream = sys.stderr


logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s")
logger = logging.getLogger("DocsArtifactBuilder")
for h in logger.handlers:
    logger.removeHandler(h)
logger.addHandler(UTF8StreamHandler())


REPO_MARKDOWN_EXCLUDE_PATTERNS = {
    ".git",
    "__pycache__",
    "node_modules",
    ".venv",
    "build",
    "build-",
    "vcpkg",
    "vcpkg_installed",
    "artifacts",
    "models",
    "downloads",
    "logs",
    "tmp",
    "external",
    "llama.cpp",
    "stable-diffusion.cpp",
    "whisper.cpp",
}

SENTENCE_SPLIT_RE = re.compile(r"(?<=[.!?])\s+")
TOKEN_RE = re.compile(r"[A-Za-z0-9_\-]+", re.UNICODE)
HEADING_RE = re.compile(r"^(#{1,6})\s+(.+?)\s*$", re.MULTILINE)
MARKDOWN_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")


@dataclass
class ChunkConfig:
    target_tokens: int = 220
    overlap_tokens: int = 40
    min_chunk_tokens: int = 40
    max_chunk_tokens: int = 360


@dataclass
class EmbeddingPersistConfig:
    quantize: bool = True
    bits: int = 8


class EmbeddingBackend:
    """Offline embedding backend with sentence-transformers primary and hash fallback."""

    def __init__(self, model_name: str, dimensions: int) -> None:
        self.model_name = model_name
        self.dimensions = dimensions
        self._model = None
        self.backend_name = "hash-fallback"
        try:
            from sentence_transformers import SentenceTransformer  # type: ignore

            self._model = SentenceTransformer(model_name)
            self.backend_name = f"sentence-transformers:{model_name}"
            logger.info("Embedding backend: %s", self.backend_name)
        except Exception:
            logger.warning(
                "sentence-transformers not available; using deterministic hash embeddings (%d dims)",
                dimensions,
            )

    def encode(self, texts: list[str]) -> list[list[float]]:
        if self._model is not None:
            vectors = self._model.encode(texts, normalize_embeddings=True)
            return [[float(v) for v in row] for row in vectors]
        return [self._hash_embed(t) for t in texts]

    def _hash_embed(self, text: str) -> list[float]:
        vec = [0.0] * self.dimensions
        tokens = TOKEN_RE.findall(text.lower())
        if not tokens:
            return vec

        freqs: dict[str, int] = {}
        for tok in tokens:
            freqs[tok] = freqs.get(tok, 0) + 1

        for tok, tf in freqs.items():
            digest = hashlib.blake2b(tok.encode("utf-8"), digest_size=16).digest()
            idx = int.from_bytes(digest[:4], "little") % self.dimensions
            sign = -1.0 if (digest[4] & 1) else 1.0
            weight = 1.0 + math.log(float(tf))
            vec[idx] += sign * weight

        norm = math.sqrt(sum(v * v for v in vec))
        if norm > 0:
            vec = [round(v / norm, 6) for v in vec]
        return vec


def _is_excluded(path: Path, root: Path, exclude_patterns: set[str]) -> bool:
    rel = path.relative_to(root).as_posix().lower()
    parts = rel.split("/")
    for part in parts:
        for pattern in exclude_patterns:
            p = pattern.lower()
            if p in part:
                return True
    return False


def discover_markdown_files(root: Path, exclude_patterns: set[str]) -> list[Path]:
    files: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        d = Path(dirpath)
        dirnames[:] = [name for name in dirnames if not _is_excluded(d / name, root, exclude_patterns)]
        for name in filenames:
            p = d / name
            if p.suffix.lower() not in {".md", ".markdown"}:
                continue
            if _is_excluded(p, root, exclude_patterns):
                continue
            files.append(p)
    return sorted(files)


def tokenize(text: str) -> list[str]:
    return TOKEN_RE.findall(text)


def split_paragraphs(text: str) -> list[str]:
    return [p.strip() for p in re.split(r"\n\s*\n", text) if p.strip()]


def split_sentences(text: str) -> list[str]:
    parts = [p.strip() for p in SENTENCE_SPLIT_RE.split(text) if p.strip()]
    return parts if parts else [text]


def chunk_text_hybrid(text: str, cfg: ChunkConfig) -> list[str]:
    paragraphs = split_paragraphs(text)
    if not paragraphs:
        return []

    chunks: list[str] = []
    current_sentences: list[str] = []
    current_tokens = 0

    def flush() -> None:
        nonlocal current_sentences, current_tokens
        if not current_sentences:
            return
        chunk = " ".join(current_sentences).strip()
        tok_count = len(tokenize(chunk))
        if tok_count >= cfg.min_chunk_tokens or not chunks:
            chunks.append(chunk)
        current_sentences = []
        current_tokens = 0

    for para in paragraphs:
        for sent in split_sentences(para):
            sent_tokens = len(tokenize(sent))
            if sent_tokens > cfg.max_chunk_tokens:
                # Hard token-window split for very long sentences/blocks.
                toks = tokenize(sent)
                i = 0
                while i < len(toks):
                    sub = toks[i : i + cfg.target_tokens]
                    if sub:
                        if current_sentences:
                            flush()
                        chunks.append(" ".join(sub))
                    step = max(1, cfg.target_tokens - cfg.overlap_tokens)
                    i += step
                continue

            if current_tokens + sent_tokens > cfg.target_tokens and current_sentences:
                prev_tokens = tokenize(" ".join(current_sentences))
                flush()
                if prev_tokens:
                    overlap = prev_tokens[-cfg.overlap_tokens :]
                    if overlap:
                        overlap_text = " ".join(overlap)
                        current_sentences = [overlap_text]
                        current_tokens = len(overlap)

            current_sentences.append(sent)
            current_tokens += sent_tokens

    flush()
    return chunks


def parse_markdown(text: str) -> tuple[list[dict[str, Any]], list[dict[str, str]]]:
    headings: list[dict[str, Any]] = []
    links: list[dict[str, str]] = []

    for m in HEADING_RE.finditer(text):
        headings.append(
            {
                "level": len(m.group(1)),
                "title": m.group(2).strip(),
                "byte_offset": m.start(),
            }
        )

    for m in MARKDOWN_LINK_RE.finditer(text):
        links.append({"label": m.group(1).strip(), "target": m.group(2).strip()})

    return headings, links


def stable_id(prefix: str, value: str) -> str:
    digest = hashlib.sha256(value.encode("utf-8")).hexdigest()[:16]
    return f"{prefix}_{digest}"


def resolve_markdown_link(source_rel: str, raw_target: str) -> str | None:
    target = raw_target.split("#", 1)[0].strip()
    if not target or target.startswith(("http://", "https://", "mailto:", "tel:")):
        return None
    src = Path(source_rel)
    resolved = (src.parent / target).as_posix()
    if not resolved.lower().endswith((".md", ".markdown")):
        return None
    return str(Path(resolved))


def quantize_int8(vec: list[float]) -> tuple[list[int], float]:
    if not vec:
        return [], 1.0
    max_abs = max(abs(v) for v in vec)
    if max_abs <= 1e-12:
        return [0 for _ in vec], 1.0
    scale = max_abs / 127.0
    q = [int(max(-127, min(127, round(v / scale)))) for v in vec]
    return q, float(scale)


def filter_graph_edges(
    edges: list[dict[str, Any]],
    mode: str,
    node_to_doc: dict[str, str],
) -> list[dict[str, Any]]:
    if mode == "full":
        return edges

    if mode == "doc-only":
        keep = {"doc_link", "document_has_heading", "heading_child"}
        return [e for e in edges if e.get("type") in keep]

    # intra-doc: keep only edges with same owning document context.
    filtered: list[dict[str, Any]] = []
    for e in edges:
        t = e.get("type")
        if t == "doc_link":
            continue
        src_doc = node_to_doc.get(str(e.get("src")), "")
        dst_doc = node_to_doc.get(str(e.get("dst")), "")
        if src_doc and dst_doc and src_doc == dst_doc:
            filtered.append(e)
        elif t in {"document_has_heading", "heading_child"}:
            filtered.append(e)
    return filtered


def build_docs_artifact(
    output_path: Path,
    source_root: Path,
    chunk_cfg: ChunkConfig,
    embedding_cfg: EmbeddingPersistConfig,
    embedding_model: str,
    embedding_dimensions: int,
    exclude_patterns: set[str],
    graph_mode: str,
) -> bool:
    files = discover_markdown_files(source_root, exclude_patterns)
    if not files:
        logger.error("No markdown files found in %s", source_root)
        return False

    logger.info("Discovered %d markdown files", len(files))
    embedder = EmbeddingBackend(embedding_model, embedding_dimensions)

    documents: list[dict[str, Any]] = []
    chunks: list[dict[str, Any]] = []
    graph_nodes: list[dict[str, Any]] = []
    graph_edges: list[dict[str, Any]] = []
    node_to_doc: dict[str, str] = {}

    path_to_doc_id: dict[str, str] = {}
    pending_doc_links: list[tuple[str, str, str]] = []

    total_chars = 0
    total_tokens = 0

    for p in files:
        rel = p.relative_to(source_root).as_posix()
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except Exception as exc:
            logger.warning("Skipping unreadable file %s: %s", rel, exc)
            continue

        file_hash = hashlib.sha256(text.encode("utf-8")).hexdigest()
        headings, links = parse_markdown(text)
        doc_id = stable_id("doc", rel)
        path_to_doc_id[rel] = doc_id

        doc_chunks = chunk_text_hybrid(text, chunk_cfg)
        vectors = embedder.encode(doc_chunks) if doc_chunks else []

        token_count = len(tokenize(text))
        total_chars += len(text)
        total_tokens += token_count

        documents.append(
            {
                "doc_id": doc_id,
                "file_path": rel,
                "file_hash": file_hash,
                "title": headings[0]["title"] if headings else p.stem,
                "char_count": len(text),
                "token_count": token_count,
                "headings": headings,
                "links": links,
                "metadata": {"file_name": p.name, "extension": p.suffix.lower(), "source": "repo_markdown"},
            }
        )
        graph_nodes.append({"id": doc_id, "type": "document", "path": rel})
        node_to_doc[doc_id] = doc_id

        for i, ctext in enumerate(doc_chunks):
            chunk_id = stable_id("chunk", f"{doc_id}:{i}")
            chunk_tokens = len(tokenize(ctext))
            emb = vectors[i] if i < len(vectors) else [0.0] * embedding_dimensions
            chunk = {
                "chunk_id": chunk_id,
                "doc_id": doc_id,
                "chunk_index": i,
                "text": ctext,
                "token_count": chunk_tokens,
                "embedding_dim": len(emb),
            }
            if embedding_cfg.quantize and embedding_cfg.bits == 8:
                emb_q, emb_scale = quantize_int8(emb)
                chunk["embedding_q"] = emb_q
                chunk["embedding_scale"] = emb_scale
                chunk["embedding_quantized"] = True
            else:
                chunk["embedding"] = emb
                chunk["embedding_quantized"] = False
            chunks.append(chunk)
            graph_nodes.append({"id": chunk_id, "type": "chunk", "doc_id": doc_id})
            node_to_doc[chunk_id] = doc_id
            graph_edges.append({"src": doc_id, "dst": chunk_id, "type": "document_has_chunk", "weight": 1.0})
            if i > 0:
                prev_chunk_id = stable_id("chunk", f"{doc_id}:{i-1}")
                graph_edges.append({"src": prev_chunk_id, "dst": chunk_id, "type": "chunk_next", "weight": 1.0})

        # Heading hierarchy edges.
        heading_stack: dict[int, str] = {}
        for h in headings:
            hid = stable_id("heading", f"{doc_id}:{h['level']}:{h['title']}:{h['byte_offset']}")
            graph_nodes.append({"id": hid, "type": "heading", "doc_id": doc_id, "title": h["title"], "level": h["level"]})
            node_to_doc[hid] = doc_id
            graph_edges.append({"src": doc_id, "dst": hid, "type": "document_has_heading", "weight": 1.0})
            parent_level = h["level"] - 1
            while parent_level > 0 and parent_level not in heading_stack:
                parent_level -= 1
            if parent_level in heading_stack:
                graph_edges.append({"src": heading_stack[parent_level], "dst": hid, "type": "heading_child", "weight": 1.0})
            heading_stack[h["level"]] = hid

        # Deferred doc link resolution.
        for link in links:
            resolved = resolve_markdown_link(rel, link["target"])
            if resolved:
                pending_doc_links.append((doc_id, resolved, link["target"]))

    for src_doc_id, target_path, raw_target in pending_doc_links:
        dst_doc_id = path_to_doc_id.get(Path(target_path).as_posix())
        if dst_doc_id:
            graph_edges.append(
                {
                    "src": src_doc_id,
                    "dst": dst_doc_id,
                    "type": "doc_link",
                    "weight": 1.0,
                    "metadata": {"raw_target": raw_target},
                }
            )

    graph_edges = filter_graph_edges(graph_edges, graph_mode, node_to_doc)

    # Backward compatibility for current DocsAssistant parser.
    legacy_documents: list[dict[str, Any]] = []
    chunk_map: dict[str, list[dict[str, Any]]] = {}
    for c in chunks:
        chunk_map.setdefault(c["doc_id"], []).append(c)
    for d in documents:
        d_chunks = chunk_map.get(d["doc_id"], [])
        preview = d_chunks[0]["text"] if d_chunks else ""
        legacy_documents.append(
            {
                "file_path": d["file_path"],
                "file_hash": d["file_hash"],
                "mime_type": "text/markdown",
                "metadata": d["metadata"],
                "themis_metadata": {
                    "vector": {
                        "text_content": preview,
                        "content_length": d["char_count"],
                        "embedding_required": True,
                    },
                    "graph": {
                        "entity_type": "Document",
                        "entity_id": d["doc_id"],
                        "relationships": [l["target"] for l in d.get("links", [])],
                    },
                },
            }
        )

    artifact = {
        "version": "2.0",
        "generated": datetime.now().isoformat(),
        "pipeline": {
            "mode": "offline_build",
            "source_mode": "repo_markdown",
            "chunking": {
                "target_tokens": chunk_cfg.target_tokens,
                "overlap_tokens": chunk_cfg.overlap_tokens,
                "min_chunk_tokens": chunk_cfg.min_chunk_tokens,
                "max_chunk_tokens": chunk_cfg.max_chunk_tokens,
            },
            "embedding": {
                "backend": embedder.backend_name,
                "model": embedding_model,
                "dimension": embedding_dimensions,
                "quantized": embedding_cfg.quantize,
                "quantization_bits": embedding_cfg.bits if embedding_cfg.quantize else 0,
            },
            "graph_mode": graph_mode,
        },
        "statistics": {
            "document_count": len(documents),
            "chunk_count": len(chunks),
            "graph_nodes": len(graph_nodes),
            "graph_edges": len(graph_edges),
            "total_chars": total_chars,
            "total_tokens": total_tokens,
        },
        "documents": legacy_documents,
        "artifact_documents": documents,
        "chunks": chunks,
        "graph": {
            "nodes": graph_nodes,
            "edges": graph_edges,
        },
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as f:
        json.dump(artifact, f, indent=2, ensure_ascii=False)

    logger.info("Artifact written: %s", output_path)
    logger.info("Documents: %d | Chunks: %d | Graph edges: %d", len(documents), len(chunks), len(graph_edges))
    logger.info("Output size: %.2f MB", output_path.stat().st_size / (1024 * 1024))
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate ThemisDB docs artifact (offline, load-only runtime)")
    parser.add_argument("--output", default="artifacts/docs-db/docs_artifact.json", help="Output artifact JSON path")
    parser.add_argument("--repo-markdown", action="store_true", default=True, help="Scan repository markdown")
    parser.add_argument("--embedding-model", default="sentence-transformers/all-MiniLM-L6-v2", help="Embedding model name")
    parser.add_argument("--embedding-dim", type=int, default=384, help="Fallback embedding dimensions")
    parser.add_argument("--quantize-embeddings", action="store_true", default=True, help="Persist chunk embeddings in quantized int8 form")
    parser.add_argument("--no-quantize-embeddings", action="store_true", default=False, help="Persist chunk embeddings as float arrays")
    parser.add_argument("--graph-mode", choices=["full", "doc-only", "intra-doc"], default="full", help="Graph edge persistence mode")
    parser.add_argument("--chunk-target-tokens", type=int, default=220)
    parser.add_argument("--chunk-overlap-tokens", type=int, default=40)
    parser.add_argument("--chunk-min-tokens", type=int, default=40)
    parser.add_argument("--chunk-max-tokens", type=int, default=360)
    args = parser.parse_args()

    logger.info("=" * 60)
    logger.info("ThemisDB Documentation Artifact Builder")
    logger.info("=" * 60)

    cfg = ChunkConfig(
        target_tokens=args.chunk_target_tokens,
        overlap_tokens=args.chunk_overlap_tokens,
        min_chunk_tokens=args.chunk_min_tokens,
        max_chunk_tokens=args.chunk_max_tokens,
    )

    quantize_embeddings = True
    if args.no_quantize_embeddings:
        quantize_embeddings = False
    elif args.quantize_embeddings:
        quantize_embeddings = True

    ecfg = EmbeddingPersistConfig(quantize=quantize_embeddings, bits=8)

    ok = build_docs_artifact(
        output_path=Path(args.output),
        source_root=REPO_ROOT,
        chunk_cfg=cfg,
        embedding_cfg=ecfg,
        embedding_model=args.embedding_model,
        embedding_dimensions=args.embedding_dim,
        exclude_patterns=REPO_MARKDOWN_EXCLUDE_PATTERNS,
        graph_mode=args.graph_mode,
    )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
