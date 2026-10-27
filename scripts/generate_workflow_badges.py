#!/usr/bin/env python3
"""Generate a complete GitHub Actions workflow badge catalog."""

from __future__ import annotations

from pathlib import Path
import re


REPO = "makr-code/ThemisDB"
WORKFLOWS_DIR = Path(".github/workflows")
OUTPUT_FILE = Path("docs/en/badges/workflow-actions.md")


def read_workflow_name(path: Path) -> str:
    text = path.read_text(encoding="utf-8", errors="ignore")
    for line in text.splitlines():
        match = re.match(r"^name:\s*[\"']?(.*?)[\"']?\s*$", line.strip())
        if match:
            value = match.group(1).strip()
            if value:
                return value
    return "(unnamed workflow)"


def build_badge(file_name: str, label: str) -> str:
    badge_url = f"https://github.com/{REPO}/actions/workflows/{file_name}/badge.svg"
    workflow_url = f"https://github.com/{REPO}/actions/workflows/{file_name}"
    return f"[![{label}]({badge_url})]({workflow_url})"


def generate() -> None:
    workflows = sorted(p for p in WORKFLOWS_DIR.glob("*.yml"))

    lines = [
        "# Workflow Actions Badges",
        "",
        "This page provides a badge for every workflow in `.github/workflows/`.",
        "",
        f"**Total workflows:** {len(workflows)}",
        "",
        "| Workflow File | Workflow Name | Badge |",
        "|---|---|---|",
    ]

    for wf in workflows:
        wf_name = read_workflow_name(wf).replace("|", "\\|")
        file_name = wf.name
        badge = build_badge(file_name, file_name)
        lines.append(f"| `{file_name}` | {wf_name} | {badge} |")

    OUTPUT_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    generate()
