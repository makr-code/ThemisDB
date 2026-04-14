"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingest_graph_phi3_lib.py                           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-14 11:54:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     547                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3db23979ed  2026-04-06  feat: Enable HTTP server by default + LLM API routing + c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import hashlib
import json
import os
import re
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Dict, Iterable, List, Optional, Set, Tuple
from urllib import error, parse, request


DEFAULT_EXTENSIONS = {
    ".txt",
    ".md",
    ".rst",
    ".json",
    ".yaml",
    ".yml",
    ".csv",
    ".xml",
    ".log",
}


@dataclass
class GraphNode:
    vertex_id: str
    label: str
    node_type: str
    properties: Dict[str, Any]
    source_doc: str


@dataclass
class GraphEdge:
    edge_id: str
    source: str
    target: str
    edge_type: str
    properties: Dict[str, Any]


@dataclass
class IngestionConfig:
    source: str = r"Y:\data"
    themis_url: str = "http://127.0.0.1:8765"
    bearer_token: str = ""
    model_id: str = "phi3"
    model_path: str = ""
    skip_model_load: bool = False
    include_ext: Set[str] = field(default_factory=lambda: set(DEFAULT_EXTENSIONS))
    max_file_size_mb: float = 8.0
    max_chars_per_file: int = 12000
    max_files: int = 0
    max_nodes_per_file: int = 40
    max_edges_per_file: int = 80
    llm_max_tokens: int = 1600
    llm_temperature: float = 0.1
    dry_run: bool = False
    timeout_sec: float = 45.0


@dataclass
class IngestionStats:
    scanned: int = 0
    processed: int = 0
    skipped: int = 0
    failed: int = 0
    write_failed: int = 0
    nodes: int = 0
    edges: int = 0
    elapsed_sec: float = 0.0
    total_candidates: int = 0


ProgressCallback = Callable[[str, Dict[str, Any], IngestionStats], None]
LogCallback = Callable[[str], None]


class ThemisHttpClient:
    def __init__(self, base_url: str, bearer_token: str, timeout_sec: float) -> None:
        self.base_url = base_url.rstrip("/")
        self.bearer_token = bearer_token.strip()
        self.timeout_sec = timeout_sec

    def _headers(self) -> Dict[str, str]:
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
        }
        if self.bearer_token:
            headers["Authorization"] = f"Bearer {self.bearer_token}"
        return headers

    def request_json(self, method: str, path: str, payload: Optional[Dict[str, Any]] = None) -> Tuple[int, Dict[str, Any]]:
        url = f"{self.base_url}{path}"
        body: Optional[bytes] = None
        if payload is not None:
            body = json.dumps(payload).encode("utf-8")

        req = request.Request(url, data=body, headers=self._headers(), method=method)
        try:
            with request.urlopen(req, timeout=self.timeout_sec) as resp:
                content = resp.read().decode("utf-8", errors="replace").strip()
                if not content:
                    return resp.status, {}
                return resp.status, json.loads(content)
        except error.HTTPError as ex:
            raw = ex.read().decode("utf-8", errors="replace")
            parsed: Dict[str, Any] = {}
            try:
                parsed = json.loads(raw)
            except json.JSONDecodeError:
                parsed = {"error": raw}
            return ex.code, parsed
        except error.URLError as ex:
            raise RuntimeError(f"Network error for {method} {path}: {ex}") from ex

    def health(self) -> Dict[str, Any]:
        status, payload = self.request_json("GET", "/health")
        if status != 200:
            raise RuntimeError(f"/health failed with status {status}: {payload}")
        return payload


def stable_hash(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def canonical_vertex_id(raw: str) -> str:
    base = raw.strip() or "unknown"
    base = re.sub(r"\s+", "_", base)
    base = re.sub(r"[^A-Za-z0-9_:\-.]", "_", base)
    if len(base) > 140:
        base = base[:140]
    digest = stable_hash(raw)[:10]
    return f"v:{base}:{digest}"


def entity_uuid(vertex_id: str) -> str:
    return stable_hash(vertex_id)[:24]


def edge_id(source: str, target: str, edge_type: str, source_doc: str) -> str:
    key = f"{source}|{target}|{edge_type}|{source_doc}"
    return f"e:{stable_hash(key)[:24]}"


def file_node_id(path: Path) -> str:
    return canonical_vertex_id(f"file:{path.as_posix()}")


def extract_json_from_text(text: str) -> Optional[Dict[str, Any]]:
    stripped = text.strip()
    if not stripped:
        return None

    try:
        parsed = json.loads(stripped)
        if isinstance(parsed, dict):
            return parsed
    except json.JSONDecodeError:
        pass

    fenced = re.findall(r"```(?:json)?\s*(.*?)\s*```", stripped, flags=re.DOTALL | re.IGNORECASE)
    for block in fenced:
        try:
            parsed = json.loads(block)
            if isinstance(parsed, dict):
                return parsed
        except json.JSONDecodeError:
            continue

    start = stripped.find("{")
    end = stripped.rfind("}")
    if start != -1 and end != -1 and end > start:
        fragment = stripped[start : end + 1]
        try:
            parsed = json.loads(fragment)
            if isinstance(parsed, dict):
                return parsed
        except json.JSONDecodeError:
            return None

    return None


def read_text_file(path: Path, max_chars: int) -> Optional[str]:
    try:
        raw = path.read_bytes()
    except OSError:
        return None

    if b"\x00" in raw[:4096]:
        return None

    for encoding in ("utf-8", "cp1252", "latin-1"):
        try:
            text = raw.decode(encoding)
            return text[:max_chars]
        except UnicodeDecodeError:
            continue
    return None


def iter_files(source_dir: Path, include_ext: Set[str], max_file_size_mb: float) -> Iterable[Path]:
    max_bytes = int(max_file_size_mb * 1024 * 1024)
    for path in source_dir.rglob("*"):
        if not path.is_file():
            continue
        if include_ext and path.suffix.lower() not in include_ext:
            continue
        try:
            if path.stat().st_size > max_bytes:
                continue
        except OSError:
            continue
        yield path


def count_candidates(source_dir: Path, include_ext: Set[str], max_file_size_mb: float) -> int:
    count = 0
    for _ in iter_files(source_dir, include_ext, max_file_size_mb):
        count += 1
    return count


def build_phi3_prompt(file_path: str, content: str, max_nodes: int, max_edges: int) -> str:
    return (
        "Du bist ein Information-Extraction-System fuer eine Property-Graph-Datenbank. "
        "Lies den Dokumentinhalt und liefere NUR JSON ohne Erklaerung.\n\n"
        "Schema:\n"
        "{\n"
        '  "nodes": [\n'
        "    {\n"
        '      "id": "string (stabil, eindeutig)",\n'
        '      "label": "string",\n'
        '      "type": "Document|Person|Organization|Concept|Location|Event|Date|Other",\n'
        '      "properties": {"key": "value"}\n'
        "    }\n"
        "  ],\n"
        '  "edges": [\n'
        "    {\n"
        '      "from": "node id",\n'
        '      "to": "node id",\n'
        '      "type": "MENTIONS|RELATES_TO|PART_OF|REFERENCES|DEPENDS_ON|LOCATED_IN|OTHER",\n'
        '      "properties": {"key": "value"}\n'
        "    }\n"
        "  ],\n"
        '  "summary": "kurze Zusammenfassung"\n'
        "}\n\n"
        f"Regeln: Maximal {max_nodes} Nodes und maximal {max_edges} Edges. "
        "Nur Belege aus dem Text verwenden, keine Halluzinationen.\n\n"
        f"Datei: {file_path}\n"
        "Inhalt:\n"
        f"{content}"
    )


def llm_extract_graph(
    client: ThemisHttpClient,
    model_id: str,
    file_path: str,
    content: str,
    max_nodes: int,
    max_edges: int,
    max_tokens: int,
    temperature: float,
) -> Dict[str, Any]:
    prompt = build_phi3_prompt(file_path, content, max_nodes=max_nodes, max_edges=max_edges)
    status, payload = client.request_json(
        "POST",
        "/api/v1/llm/inference",
        {
            "model": model_id,
            "prompt": prompt,
            "max_tokens": max_tokens,
            "temperature": temperature,
        },
    )

    if status != 200:
        raise RuntimeError(f"LLM inference failed ({status}): {payload}")

    text = str(payload.get("text", "")).strip()
    extracted = extract_json_from_text(text)
    if extracted is None:
        raise RuntimeError("LLM response did not contain parseable JSON graph output")

    if "nodes" not in extracted or not isinstance(extracted.get("nodes"), list):
        extracted["nodes"] = []
    if "edges" not in extracted or not isinstance(extracted.get("edges"), list):
        extracted["edges"] = []

    return extracted


def upsert_entity(client: ThemisHttpClient, key: str, blob_object: Dict[str, Any]) -> None:
    encoded_key = parse.quote(key, safe="")
    status, payload = client.request_json(
        "PUT",
        f"/entities/{encoded_key}",
        {"blob": json.dumps(blob_object, ensure_ascii=False)},
    )
    if status not in (200, 201):
        raise RuntimeError(f"Entity upsert failed ({status}) for key {key}: {payload}")


def create_graph_edge(client: ThemisHttpClient, edge: GraphEdge) -> None:
    status, payload = client.request_json(
        "POST",
        "/graph/edge",
        {
            "id": edge.edge_id,
            "_from": edge.source,
            "_to": edge.target,
            "type": edge.edge_type,
            **edge.properties,
        },
    )
    if status not in (200, 201):
        raise RuntimeError(f"Graph edge create failed ({status}) for edge {edge.edge_id}: {payload}")


def maybe_load_model(client: ThemisHttpClient, model_id: str, model_path: str) -> None:
    payload: Dict[str, Any] = {"model_id": model_id}
    if model_path:
        payload["path"] = model_path

    status, body = client.request_json("POST", "/api/v1/llm/models/load", payload)
    if status != 200:
        raise RuntimeError(f"Model load failed ({status}): {body}")


def _emit(progress_cb: Optional[ProgressCallback], event: str, data: Dict[str, Any], stats: IngestionStats) -> None:
    if progress_cb:
        progress_cb(event, data, stats)


def _log(log_cb: Optional[LogCallback], message: str) -> None:
    if log_cb:
        log_cb(message)


def run_ingestion(config: IngestionConfig, progress_cb: Optional[ProgressCallback] = None, log_cb: Optional[LogCallback] = None) -> IngestionStats:
    stats = IngestionStats()

    source_dir = Path(config.source)
    if not source_dir.exists() or not source_dir.is_dir():
        raise RuntimeError(f"Source directory not found: {source_dir}")

    include_ext = {
        ext.lower() if ext.startswith(".") else f".{ext.lower()}"
        for ext in config.include_ext
    }

    stats.total_candidates = count_candidates(source_dir, include_ext, config.max_file_size_mb)
    _emit(progress_cb, "scan_start", {"total_candidates": stats.total_candidates}, stats)

    client = ThemisHttpClient(config.themis_url, config.bearer_token, timeout_sec=config.timeout_sec)
    health = client.health()
    _log(log_cb, f"Connected to ThemisDB: {health}")
    _emit(progress_cb, "health_ok", {"health": health}, stats)

    if not config.skip_model_load and not config.dry_run:
        maybe_load_model(client, config.model_id, config.model_path)
        _log(log_cb, f"Model '{config.model_id}' loaded or confirmed.")
        _emit(progress_cb, "model_loaded", {"model_id": config.model_id}, stats)

    node_map: Dict[str, GraphNode] = {}
    edge_map: Dict[str, GraphEdge] = {}
    start_time = time.time()

    for path in iter_files(source_dir, include_ext, config.max_file_size_mb):
        if config.max_files > 0 and stats.processed >= config.max_files:
            break

        stats.scanned += 1
        rel = path.relative_to(source_dir)
        content = read_text_file(path, max_chars=config.max_chars_per_file)
        if not content:
            stats.skipped += 1
            _emit(progress_cb, "file_skipped", {"file": rel.as_posix()}, stats)
            continue

        doc_vertex = file_node_id(rel)
        doc_node = GraphNode(
            vertex_id=doc_vertex,
            label=rel.name,
            node_type="Document",
            properties={
                "relative_path": rel.as_posix(),
                "extension": path.suffix.lower(),
                "size_bytes": path.stat().st_size,
            },
            source_doc=rel.as_posix(),
        )
        node_map[doc_vertex] = doc_node

        try:
            extracted = llm_extract_graph(
                client=client,
                model_id=config.model_id,
                file_path=rel.as_posix(),
                content=content,
                max_nodes=config.max_nodes_per_file,
                max_edges=config.max_edges_per_file,
                max_tokens=config.llm_max_tokens,
                temperature=config.llm_temperature,
            )

            summary = str(extracted.get("summary", "")).strip()
            if summary:
                doc_node.properties["summary"] = summary

            for raw in extracted.get("nodes", []):
                if not isinstance(raw, dict):
                    continue
                raw_id = str(raw.get("id", "")).strip()
                if not raw_id:
                    continue
                vertex = canonical_vertex_id(raw_id)
                label = str(raw.get("label", raw_id)).strip() or raw_id
                node_type = str(raw.get("type", "Other")).strip() or "Other"
                props = raw.get("properties", {})
                if not isinstance(props, dict):
                    props = {}
                props = dict(props)
                props["llm_raw_id"] = raw_id
                props["source_doc"] = rel.as_posix()

                existing = node_map.get(vertex)
                if existing is None:
                    node_map[vertex] = GraphNode(
                        vertex_id=vertex,
                        label=label,
                        node_type=node_type,
                        properties=props,
                        source_doc=rel.as_posix(),
                    )
                else:
                    existing.properties.update(props)

                link = GraphEdge(
                    edge_id=edge_id(doc_vertex, vertex, "MENTIONS", rel.as_posix()),
                    source=doc_vertex,
                    target=vertex,
                    edge_type="MENTIONS",
                    properties={"source_doc": rel.as_posix()},
                )
                edge_map[link.edge_id] = link

            for raw in extracted.get("edges", []):
                if not isinstance(raw, dict):
                    continue
                raw_from = str(raw.get("from", "")).strip()
                raw_to = str(raw.get("to", "")).strip()
                raw_type = str(raw.get("type", "OTHER")).strip().upper() or "OTHER"
                if not raw_from or not raw_to:
                    continue
                src = canonical_vertex_id(raw_from)
                dst = canonical_vertex_id(raw_to)
                props = raw.get("properties", {})
                if not isinstance(props, dict):
                    props = {}
                props = dict(props)
                props["source_doc"] = rel.as_posix()
                eid = edge_id(src, dst, raw_type, rel.as_posix())
                edge_map[eid] = GraphEdge(
                    edge_id=eid,
                    source=src,
                    target=dst,
                    edge_type=raw_type,
                    properties=props,
                )

            stats.processed += 1
            stats.nodes = len(node_map)
            stats.edges = len(edge_map)
            _log(log_cb, f"Processed {rel.as_posix()} (nodes={stats.nodes}, edges={stats.edges})")
            _emit(progress_cb, "file_processed", {"file": rel.as_posix()}, stats)
        except Exception as ex:
            stats.failed += 1
            _log(log_cb, f"Failed for {rel.as_posix()}: {ex}")
            _emit(progress_cb, "file_failed", {"file": rel.as_posix(), "error": str(ex)}, stats)

    if config.dry_run:
        stats.elapsed_sec = time.time() - start_time
        _emit(progress_cb, "done", {"dry_run": True}, stats)
        return stats

    for node in node_map.values():
        try:
            key = f"graph.default.nodes:{entity_uuid(node.vertex_id)}"
            blob = {
                "vertex_id": node.vertex_id,
                "label": node.label,
                "type": node.node_type,
                "properties": node.properties,
                "source_doc": node.source_doc,
            }
            upsert_entity(client, key, blob)
        except Exception as ex:
            stats.write_failed += 1
            _log(log_cb, f"Node write failed for {node.vertex_id}: {ex}")
            _emit(progress_cb, "node_write_failed", {"vertex_id": node.vertex_id, "error": str(ex)}, stats)

    for edge in edge_map.values():
        try:
            create_graph_edge(client, edge)
        except Exception as ex:
            stats.write_failed += 1
            _log(log_cb, f"Edge write failed for {edge.edge_id}: {ex}")
            _emit(progress_cb, "edge_write_failed", {"edge_id": edge.edge_id, "error": str(ex)}, stats)

    stats.elapsed_sec = time.time() - start_time
    _emit(progress_cb, "done", {"dry_run": False}, stats)
    return stats


def config_from_env() -> IngestionConfig:
    token = os.getenv("THEMIS_BEARER_TOKEN", "")
    return IngestionConfig(bearer_token=token)
