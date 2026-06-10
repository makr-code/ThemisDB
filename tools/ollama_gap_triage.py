#!/usr/bin/env python3
"""
Optional local Ollama triage for scanner findings.

This tool is intentionally lightweight:
- reads a scanner JSON file (metadata.gaps or top-level items)
- sends only uncertain/high-noise findings to a local Ollama model
- writes machine-readable triage results (TP/FP/? + rationale)

Usage:
  python tools/ollama_gap_triage.py \
    --input ai_working/sample_validation_assessment_src.json \
    --output ai_working/ollama_triage_src.json \
    --model qwen2.5-coder:7b \
    --max-items 40
"""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path
from typing import Any, Dict, List

NOISY_TYPES = {
    "pointer_arithmetic_unbounded",
    "missing_vector_reserve",
    "uncaught_exception",
    "generic_catch",
    "no_retry_logic",
}

PROMPT_TEMPLATE = """You are validating a static-analysis finding for C++ code.
Return ONLY compact JSON with keys: label, confidence, rationale.
- label must be one of: TP, FP, ?
- confidence must be a float 0..1
- rationale max 2 short sentences

Finding:
- file: {file}
- line: {line}
- type: {type}
- severity: {severity}
- description: {description}
- context:
{context}
"""


def load_findings(payload: Dict[str, Any]) -> List[Dict[str, Any]]:
    if "gaps" in payload and isinstance(payload["gaps"], list):
        return payload["gaps"]
    if "items" in payload and isinstance(payload["items"], list):
        return payload["items"]
    return []


def should_triage(item: Dict[str, Any]) -> bool:
    finding_type = str(item.get("type", "")).strip()
    existing = str(item.get("assessment", "")).strip()
    if existing == "?":
        return True
    return finding_type in NOISY_TYPES


def build_prompt(item: Dict[str, Any]) -> str:
    return PROMPT_TEMPLATE.format(
        file=item.get("file", ""),
        line=item.get("line", 0),
        type=item.get("type", ""),
        severity=item.get("severity", ""),
        description=item.get("description", ""),
        context=item.get("context", "")[:1200],
    )


def run_ollama(model: str, prompt: str, timeout: int) -> Dict[str, Any]:
    proc = subprocess.run(
        ["ollama", "run", model, prompt],
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )
    raw = (proc.stdout or "").strip()
    if not raw:
        return {"label": "?", "confidence": 0.0, "rationale": "no model output"}

    # Try strict JSON first.
    try:
        parsed = json.loads(raw)
        return {
            "label": parsed.get("label", "?") if isinstance(parsed, dict) else "?",
            "confidence": float(parsed.get("confidence", 0.0)) if isinstance(parsed, dict) else 0.0,
            "rationale": str(parsed.get("rationale", "")) if isinstance(parsed, dict) else "",
            "raw": raw,
        }
    except Exception:
        pass

    # Fallback: heuristic parse from free text.
    lowered = raw.lower()
    if "\"label\": \"tp\"" in lowered or "label: tp" in lowered:
        label = "TP"
    elif "\"label\": \"fp\"" in lowered or "label: fp" in lowered:
        label = "FP"
    else:
        label = "?"

    return {
        "label": label,
        "confidence": 0.4,
        "rationale": raw[:240],
        "raw": raw,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Local Ollama triage for noisy scanner findings")
    parser.add_argument("--input", required=True, help="Input JSON with findings (gaps/items)")
    parser.add_argument("--output", required=True, help="Output JSON with triage decisions")
    parser.add_argument("--model", default="qwen2.5-coder:7b", help="Ollama model")
    parser.add_argument("--max-items", type=int, default=50, help="Maximum findings to triage")
    parser.add_argument("--timeout", type=int, default=45, help="Per-item timeout in seconds")
    args = parser.parse_args()

    in_path = Path(args.input)
    out_path = Path(args.output)

    payload = json.loads(in_path.read_text(encoding="utf-8"))
    findings = load_findings(payload)

    selected = [f for f in findings if should_triage(f)][: max(0, args.max_items)]

    results: List[Dict[str, Any]] = []
    for idx, item in enumerate(selected, 1):
        prompt = build_prompt(item)
        decision = run_ollama(args.model, prompt, timeout=args.timeout)
        results.append(
            {
                "index": idx,
                "file": item.get("file", ""),
                "line": item.get("line", 0),
                "type": item.get("type", ""),
                "severity": item.get("severity", ""),
                "model": args.model,
                "decision": decision,
            }
        )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(
        json.dumps(
            {
                "input": str(in_path),
                "model": args.model,
                "selected": len(selected),
                "results": results,
            },
            indent=2,
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    print(f"WROTE {out_path.as_posix()}")
    print(f"TRIAGED {len(selected)} findings")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
