#!/usr/bin/env python3
"""Generate break-even summaries for hybrid CPU/GPU benchmark outputs."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass(frozen=True)
class PairResult:
    key: Tuple[int, ...]
    cpu_us: float
    gpu_us: float

    @property
    def speedup(self) -> float:
        if self.gpu_us <= 0:
            return 0.0
        return self.cpu_us / self.gpu_us


def _read_benchmarks(path: Optional[Path]) -> List[dict]:
    if path is None or not path.exists():
        return []
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return []
    benches = payload.get("benchmarks", [])
    if not isinstance(benches, list):
        return []
    return [b for b in benches if isinstance(b, dict)]


def _bench_time_us(entry: dict) -> Optional[float]:
    if "real_time" not in entry:
        return None
    unit = entry.get("time_unit", "ns")
    value = float(entry["real_time"])
    if unit == "us":
        return value
    if unit == "ns":
        return value / 1000.0
    if unit == "ms":
        return value * 1000.0
    if unit == "s":
        return value * 1_000_000.0
    return value


def _extract_key(name: str) -> Tuple[int, ...]:
    parts = name.split("/")[1:]
    key: List[int] = []
    for p in parts:
        if p.isdigit():
            key.append(int(p))
        else:
            break
    return tuple(key)


def _collect(prefix: str, benches: Iterable[dict]) -> Dict[Tuple[int, ...], float]:
    preferred: Dict[Tuple[int, ...], float] = {}
    fallback: Dict[Tuple[int, ...], float] = {}

    for entry in benches:
        name = str(entry.get("name", ""))
        if not name.startswith(prefix):
            continue

        metric = _bench_time_us(entry)
        if metric is None:
            continue

        key_source = str(entry.get("run_name", "")) or name
        key = _extract_key(key_source)
        if not key:
            continue

        agg = entry.get("aggregate_name")
        if agg == "mean":
            preferred[key] = metric
        elif agg is None:
            fallback[key] = metric

    return preferred or fallback


def _build_pairs(
    benches: Iterable[dict],
    cpu_prefix: str,
    gpu_prefix: str,
) -> List[PairResult]:
    cpu = _collect(cpu_prefix, benches)
    gpu = _collect(gpu_prefix, benches)
    keys = sorted(set(cpu).intersection(gpu))
    return [PairResult(key=k, cpu_us=cpu[k], gpu_us=gpu[k]) for k in keys]


def _gpu_wins_from(pairs: List[PairResult]) -> Optional[PairResult]:
    for pair in sorted(pairs, key=lambda p: p.key):
        if pair.speedup >= 1.0:
            return pair
    return None


def _summarize(
    benches: Iterable[dict],
    cpu_prefix: str,
    gpu_prefix: str,
    expected_key_names: List[str],
) -> dict:
    pairs = _build_pairs(benches, cpu_prefix, gpu_prefix)
    winner = _gpu_wins_from(pairs)

    pair_payload = []
    for p in pairs:
        named_key = {expected_key_names[i]: p.key[i] for i in range(min(len(expected_key_names), len(p.key)))}
        pair_payload.append(
            {
                "key": named_key,
                "cpu_us": round(p.cpu_us, 3),
                "gpu_us": round(p.gpu_us, 3),
                "speedup_cpu_over_gpu": round(p.speedup, 4),
                "gpu_wins": p.speedup >= 1.0,
            }
        )

    if winner is None:
        win_payload = None
        cpu_hint = "insufficient_data"
    else:
        win_payload = {
            expected_key_names[i]: winner.key[i] for i in range(min(len(expected_key_names), len(winner.key)))
        }
        cpu_hint = "gpu_from_threshold"

    return {
        "pair_count": len(pairs),
        "gpu_wins_from": win_payload,
        "classification": cpu_hint,
        "comparisons": pair_payload,
    }


def generate_report(ann_path: Path, tensor_path: Path, graph_path: Optional[Path]) -> dict:
    ann_benches = _read_benchmarks(ann_path)
    tensor_benches = _read_benchmarks(tensor_path)
    graph_benches = _read_benchmarks(graph_path)

    ann_summary = _summarize(
        ann_benches,
        "BM_ANN_CPUSIMDDispatchBatchKnn",
        "BM_ANN_MixedPathGpuQueryAccelerator",
        ["num_vectors", "dim", "top_k", "batch_size"],
    )
    tensor_summary = _summarize(
        tensor_benches,
        "BM_TEN_S4_BatchSimilarityCPU",
        "BM_TEN_S4_BatchSimilarityGPU",
        ["num_vectors", "dim", "batch_size"],
    )

    graph_summary = {
        "status": "available" if graph_benches else "missing_or_no_data",
        "note": (
            "Category-B parity benchmark executed; dedicated graph dispatch break-even binary still pending."
            if graph_benches
            else "No graph benchmark result found; dedicated bench_graph_cpu_gpu_dispatch is still planned."
        ),
        "benchmark_entries": len(graph_benches),
    }

    return {
        "ann": ann_summary,
        "tensor": tensor_summary,
        "graph": graph_summary,
    }


def _to_markdown(report: dict) -> str:
    lines = ["# Hybrid Break-Even Summary", ""]

    for section in ("ann", "tensor"):
        block = report[section]
        lines.append(f"## {section.upper()}")
        lines.append(f"- Pair count: {block['pair_count']}")
        lines.append(f"- Classification: {block['classification']}")
        if block["gpu_wins_from"] is None:
            lines.append("- GPU wins from: n/a")
        else:
            formatted = ", ".join(f"{k}={v}" for k, v in block["gpu_wins_from"].items())
            lines.append(f"- GPU wins from: {formatted}")
        lines.append("")

    lines.append("## GRAPH")
    lines.append(f"- Status: {report['graph']['status']}")
    lines.append(f"- Note: {report['graph']['note']}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ann", type=Path, required=True)
    parser.add_argument("--tensor", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=False)
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-md", type=Path, required=True)
    args = parser.parse_args()

    report = generate_report(args.ann, args.tensor, args.graph)

    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(json.dumps(report, indent=2), encoding="utf-8")
    args.output_md.parent.mkdir(parents=True, exist_ok=True)
    args.output_md.write_text(_to_markdown(report), encoding="utf-8")

    print(f"Wrote {args.output_json}")
    print(f"Wrote {args.output_md}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
