#!/usr/bin/env python3
"""Prepare and validate the Copilot PR task-completion assessment."""

from __future__ import annotations

import argparse
import html
import json
import re
import sys
import uuid
from pathlib import Path
from typing import Any


ALLOWED_STATUSES = {"fulfilled", "partial", "open", "unclear", "not_applicable"}
MAX_REQUIREMENTS = 50
MAX_TASK_LENGTH = 200
MAX_EVIDENCE_LENGTH = 200
CHECKBOX_PATTERN = re.compile(r"^\s*[-*+]\s+\[([ xX])\]\s+(.+?)\s*$")
TEXT_ID_PATTERN = re.compile(r"^TEXT-(PR|ISSUE)-[0-9]+-[0-9]+$")


def _read_context(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"Unable to read PR task context: {error}") from error
    if not isinstance(value, dict):
        raise ValueError("PR task context must be a JSON object.")
    return value


def _source_records(context: dict[str, Any]) -> list[tuple[str, str]]:
    pr = context.get("pull_request")
    if not isinstance(pr, dict) or not isinstance(pr.get("number"), int):
        raise ValueError("PR context is missing its pull request number.")
    records = [(f"PR #{pr['number']}", str(pr.get("body") or ""))]
    issues = context.get("issues", [])
    if not isinstance(issues, list):
        raise ValueError("PR context issues must be a list.")
    for issue in issues:
        if not isinstance(issue, dict) or not isinstance(issue.get("number"), int):
            raise ValueError("Linked issue context is malformed.")
        records.append((f"Issue #{issue['number']}", str(issue.get("body") or "")))
    return records


def _extract_checklists(context: dict[str, Any]) -> list[dict[str, Any]]:
    tasks: list[dict[str, Any]] = []
    for source, body in _source_records(context):
        prefix = "PR" if source.startswith("PR ") else f"ISSUE-{source.split('#', 1)[1]}"
        sequence = 0
        for line in body.splitlines():
            match = CHECKBOX_PATTERN.match(line)
            if not match:
                continue
            sequence += 1
            tasks.append({
                "id": f"{prefix}-{source.split('#', 1)[1]}-{sequence}" if prefix == "PR"
                else f"{prefix}-{sequence}",
                "source": source,
                "checked": match.group(1).lower() == "x",
                "requirement": match.group(2).strip(),
            })
    if len(tasks) > MAX_REQUIREMENTS:
        raise ValueError(f"Found {len(tasks)} checklist items; the limit is {MAX_REQUIREMENTS}.")
    return tasks


def _append_output(path: Path, key: str, value: str) -> None:
    delimiter = f"END_{uuid.uuid4().hex}"
    while delimiter in value:
        delimiter = f"END_{uuid.uuid4().hex}"
    with path.open("a", encoding="utf-8") as output:
        output.write(f"{key}<<{delimiter}\n{value}\n{delimiter}\n")


def prepare(context_path: Path, prompt_path: Path) -> int:
    context = _read_context(context_path)
    context["checklist_items"] = _extract_checklists(context)
    context_path.write_text(
        json.dumps(context, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
    )
    prompt = (
        Path(__file__).resolve().parents[2]
        / ".github"
        / "prompts"
        / "pr-task-completion-review.txt"
    ).read_text(encoding="utf-8")
    prompt_path.write_text(
        f"{prompt.rstrip()}\n\n```json\n{json.dumps(context, ensure_ascii=False)}\n```\n",
        encoding="utf-8",
    )
    return 1 if context.get("truncated") else 0


def _fallback_items(context: dict[str, Any], reason: str) -> list[dict[str, str]]:
    items = []
    for task in context.get("checklist_items", []):
        if not isinstance(task, dict):
            continue
        items.append({
            "id": str(task.get("id", "unverified")),
            "source": str(task.get("source", "PR")),
            "requirement": str(task.get("requirement", "Checklist item could not be assessed.")),
            "status": "unclear",
            "evidence": reason,
        })
    return items


def _parse_response(response_path: Path) -> dict[str, Any]:
    text = response_path.read_text(encoding="utf-8").strip()
    if text.startswith("```"):
        text = re.sub(r"^```(?:json)?\s*|\s*```$", "", text, flags=re.IGNORECASE)
    value = json.loads(text)
    if not isinstance(value, dict) or not isinstance(value.get("requirements"), list):
        raise ValueError("Copilot response must contain a requirements array.")
    return value


def _clean_cell(value: str, max_length: int) -> str:
    flattened = " ".join(value.split())[:max_length]
    escaped = html.escape(flattened, quote=True)
    for char, replacement in {
        "|": "&#124;",
        "`": "&#96;",
        "[": "&#91;",
        "]": "&#93;",
    }.items():
        escaped = escaped.replace(char, replacement)
    return escaped or "—"


def _normalize_requirements(
    context: dict[str, Any],
    response_path: Path,
) -> tuple[list[dict[str, str]], list[str]]:
    errors: list[str] = []
    try:
        response = _parse_response(response_path)
    except (OSError, json.JSONDecodeError, ValueError) as error:
        return _fallback_items(context, f"Copilot assessment unavailable or invalid: {error}"), [
            "The Copilot response was missing, invalid, or could not be parsed."
        ]

    raw_requirements = response["requirements"]
    if len(raw_requirements) > MAX_REQUIREMENTS:
        errors.append(f"Copilot returned more than {MAX_REQUIREMENTS} requirements.")
        raw_requirements = raw_requirements[:MAX_REQUIREMENTS]

    allowed_sources = {source for source, _ in _source_records(context)}
    requirements: list[dict[str, str]] = []
    seen_ids: set[str] = set()
    for item in raw_requirements:
        if not isinstance(item, dict):
            errors.append("Copilot returned a malformed requirement.")
            continue
        identifier = item.get("id")
        source = item.get("source")
        text = item.get("requirement")
        status = item.get("status")
        evidence = item.get("evidence")
        if not all(isinstance(value, str) for value in (identifier, source, text, status, evidence)):
            errors.append("Copilot returned a requirement with missing or non-text fields.")
            continue
        identifier = identifier.strip()
        if (
            not identifier
            or len(identifier) > 80
            or identifier in seen_ids
            or source not in allowed_sources
            or status not in ALLOWED_STATUSES
            or not text.strip()
        ):
            errors.append(f"Copilot returned an invalid or duplicate requirement: {identifier or 'unknown'}.")
            continue
        if not identifier.startswith(("PR-", "ISSUE-", "TEXT-")):
            errors.append(f"Copilot returned an unsupported requirement id: {identifier}.")
            continue
        if identifier.startswith("TEXT-") and not TEXT_ID_PATTERN.fullmatch(identifier):
            errors.append(f"Copilot returned an invalid prose requirement id: {identifier}.")
            continue
        seen_ids.add(identifier)
        requirements.append({
            "id": identifier,
            "source": source,
            "requirement": text.strip()[:MAX_TASK_LENGTH],
            "status": status,
            "evidence": evidence.strip()[:MAX_EVIDENCE_LENGTH],
        })

    for task in context.get("checklist_items", []):
        identifier = task["id"]
        if identifier not in seen_ids:
            errors.append(f"Copilot omitted checklist item {identifier}.")
            requirements.append({
                "id": identifier,
                "source": task["source"],
                "requirement": task["requirement"][:MAX_TASK_LENGTH],
                "status": "unclear",
                "evidence": "Copilot did not return an assessment for this checklist item.",
            })
            continue
        if not task["checked"]:
            for requirement in requirements:
                if requirement["id"] == identifier and requirement["status"] == "fulfilled":
                    requirement["status"] = "open"
                    requirement["evidence"] = (
                        "The source checklist is still unchecked; mark it complete only after the work is evidenced."
                    )
    return requirements, errors


def _render_report(
    context: dict[str, Any],
    requirements: list[dict[str, str]],
    errors: list[str],
) -> tuple[bool, int, str]:
    actionable = [item for item in requirements if item["status"] != "not_applicable"]
    fulfilled = sum(item["status"] == "fulfilled" for item in actionable)
    percent = round(100 * fulfilled / len(actionable)) if actionable else 0
    passed = (
        not errors
        and not context.get("truncated", False)
        and bool(actionable)
        and fulfilled == len(actionable)
    )
    verdict = "PASS" if passed else "BLOCKED"
    lines = [
        "<!-- pr-task-completion-gate -->",
        f"## Copilot task completion review: {verdict}",
        "",
        f"**Fulfillment: {fulfilled}/{len(actionable)} ({percent}%)** "
        f"· {len(requirements) - len(actionable)} not applicable",
        "",
    ]
    if context.get("truncated"):
        lines.append("Review context is incomplete or stale; the assessment fails closed.")
        lines.append("")
    if errors:
        lines.append("**Validation issues**")
        lines.extend(f"- {_clean_cell(error, 300)}" for error in errors)
        lines.append("")

    open_items = [item for item in requirements if item["status"] not in {"fulfilled", "not_applicable"}]
    if open_items:
        lines.extend(["**Open or unverified tasks**", ""])
        for item in open_items:
            lines.append(
                f"- **{_clean_cell(item['source'], 80)} · {_clean_cell(item['status'], 30)}** — "
                f"{_clean_cell(item['requirement'], MAX_TASK_LENGTH)}"
            )
            if item["evidence"]:
                lines.append(f"  - Evidence: {_clean_cell(item['evidence'], MAX_EVIDENCE_LENGTH)}")
        lines.append("")
    if not requirements:
        lines.extend([
            "**Open or unverified tasks**",
            "",
            "- No actionable requirements were returned. The gate fails closed until the PR or linked issue defines reviewable tasks.",
            "",
        ])
    elif not actionable:
        lines.extend([
            "**Open or unverified tasks**",
            "",
            "- No actionable requirements remain after exclusions; the gate fails closed.",
            "",
        ])

    lines.extend([
        "<details>",
        "<summary>Full assessment</summary>",
        "",
        "| Source | Requirement | Status | Evidence |",
        "|---|---|---|---|",
    ])
    for item in requirements:
        lines.append(
            f"| {_clean_cell(item['source'], 80)} | "
            f"{_clean_cell(item['requirement'], MAX_TASK_LENGTH)} | "
            f"{_clean_cell(item['status'], 30)} | "
            f"{_clean_cell(item['evidence'], MAX_EVIDENCE_LENGTH)} |"
        )
    lines.extend([
        "",
        "</details>",
        "",
        "Copilot reviews the supplied PR/issue text and diff; it does not execute tests. "
        "Treat this report as evidence for human review, not a replacement for required CI or maintainer approval.",
    ])
    return passed, percent, "\n".join(lines)


def evaluate(
    context_path: Path,
    response_path: Path,
    output_path: Path,
    github_output_path: Path,
) -> int:
    try:
        context = _read_context(context_path)
    except ValueError as error:
        context = {
            "pull_request": {"number": 0, "body": ""},
            "issues": [],
            "checklist_items": [],
            "truncated": True,
        }
        errors = [str(error)]
        requirements: list[dict[str, str]] = []
    else:
        requirements, errors = _normalize_requirements(context, response_path)
    if context.get("truncated"):
        reasons = context.get("truncation_reasons") or []
        errors.extend(str(reason) for reason in reasons)
        errors.append("The PR/issue/diff context was truncated or stale and could not be fully reviewed.")
    passed, percent, comment = _render_report(context, requirements, errors)
    report = {
        "passed": passed,
        "completion_percent": percent,
        "requirements": requirements,
        "errors": errors,
    }
    output_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    github_output_path.write_text("", encoding="utf-8")
    _append_output(github_output_path, "passed", str(passed).lower())
    _append_output(github_output_path, "comment", comment)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    prepare_parser = subparsers.add_parser("prepare")
    prepare_parser.add_argument("--context", type=Path, required=True)
    prepare_parser.add_argument("--prompt", type=Path, required=True)
    evaluate_parser = subparsers.add_parser("evaluate")
    evaluate_parser.add_argument("--context", type=Path, required=True)
    evaluate_parser.add_argument("--response", type=Path, required=True)
    evaluate_parser.add_argument("--output", type=Path, required=True)
    evaluate_parser.add_argument("--github-output", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if args.command == "prepare":
            return prepare(args.context, args.prompt)
        return evaluate(args.context, args.response, args.output, args.github_output)
    except (OSError, ValueError) as error:
        print(f"::error::{error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
