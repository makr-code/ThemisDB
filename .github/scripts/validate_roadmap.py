#!/usr/bin/env python3
"""
Validates ROADMAP.md and future_enhancement.md (if present) against structure standards.
Invoked as a GitHub Action on push when those files change.

Severity levels:
  ERROR   - Critical structural violation; CI fails.
  WARNING - Improvement recommended; CI passes.

Exit codes:
  0 - Passed (errors == 0; warnings may exist)
  1 - One or more errors found
"""

import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Patterns
# ---------------------------------------------------------------------------

# A valid task checkbox: - [x], - [ ], - [~], - [!], - [?], - [p], - [I], - [P]
TASK_CHECKBOX_RE = re.compile(r"^- \[[x ~!?pIP]\] .+", re.MULTILINE)

# Tasks that also carry a target annotation
TASK_WITH_TARGET_RE = re.compile(
    r"^- \[[x ~!?pIP]\] .+\(Target: (Q[1-4] \d{4}|Backlog|TBD)\)",
    re.MULTILINE,
)

# Phase header (anywhere in the file, at heading level 2–4)
PHASE_HEADER_RE = re.compile(r"^#{2,4}\s+Phase \d+", re.MULTILINE)

# Vague terms that should carry a metric when used in a task description
VAGUE_TERMS_RE = re.compile(
    r"\b(improve|optimize|enhance|better|faster)\b",
    re.IGNORECASE,
)

# Performance / measurable metric pattern
PERF_METRIC_RE = re.compile(
    r"[<>]=?\s*\d+\s*(ms|µs|us|ns|MB|GB|KB|throughput|req/s|ops/s|x\s+speedup)"
)

# Recommended section headers (absence → WARNING, not ERROR)
RECOMMENDED_SECTIONS = [
    "Current Status",
    "Production Readiness Checklist",
    "Known Issues",
]

# At least one of these should be present (absence → WARNING)
PROGRESS_SECTIONS = ["In Progress", "Planned Features", "Implementation Phases"]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_file(path: "Path") -> "str | None":
    """Return file content or None if the file does not exist."""
    if not path.exists():
        return None
    return path.read_text(encoding="utf-8")


def check_recommended_sections(content: str, filename: str) -> "list[str]":
    """Return WARNING strings for missing recommended sections."""
    warnings = []
    for section in RECOMMENDED_SECTIONS:
        pattern = re.compile(
            rf"^#{1,4}\s+.*{re.escape(section)}", re.MULTILINE | re.IGNORECASE
        )
        if not pattern.search(content):
            warnings.append(f"[{filename}] Recommended section not found: '{section}'")

    # At least one progress section
    has_progress = any(
        re.search(rf"^#{1,4}\s+.*{re.escape(s)}", content, re.MULTILINE | re.IGNORECASE)
        for s in PROGRESS_SECTIONS
    )
    if not has_progress:
        warnings.append(
            f"[{filename}] No progress section found"
            f" (expected one of: {', '.join(PROGRESS_SECTIONS)})"
        )
    return warnings


def check_phases(content: str, filename: str) -> "tuple[list[str], list[str]]":
    """Check implementation phases. Returns (errors, warnings)."""
    errors: "list[str]" = []
    warnings: "list[str]" = []
    phases = PHASE_HEADER_RE.findall(content)
    count = len(phases)
    if count == 0:
        # Only an error if the file also claims to have phases (contains the word "Phase")
        if re.search(r"\bphase\b", content, re.IGNORECASE):
            errors.append(
                f"[{filename}] File mentions phases but no 'Phase N' section headers found"
            )
        else:
            warnings.append(f"[{filename}] No 'Phase N' headers found; consider adding phases")
    elif count < 4:
        warnings.append(
            f"[{filename}] Only {count} phase(s) found; recommended minimum is 4"
        )
    return errors, warnings


def check_tasks(content: str, filename: str) -> "tuple[list[str], list[str]]":
    """Validate task checkboxes and target annotations."""
    errors: "list[str]" = []
    warnings: "list[str]" = []

    all_tasks = TASK_CHECKBOX_RE.findall(content)
    tasks_with_target = TASK_WITH_TARGET_RE.findall(content)

    total = len(all_tasks)
    with_target = len(tasks_with_target)
    without_target = total - with_target

    if total == 0:
        errors.append(f"[{filename}] No task checkboxes found (expected '- [ ] ...' lines)")
        return errors, warnings

    if without_target > 0:
        pct = (without_target / total) * 100
        warnings.append(
            f"[{filename}] {without_target}/{total} tasks ({pct:.0f}%) are missing"
            " '(Target: Q? YYYY|Backlog)' annotation"
        )

    return errors, warnings


def check_vague_descriptions(content: str, filename: str) -> "list[str]":
    """Warn about task lines with vague verbs and no measurable metric."""
    warnings: "list[str]" = []
    for i, line in enumerate(content.splitlines(), start=1):
        if not TASK_CHECKBOX_RE.match(line):
            continue
        if VAGUE_TERMS_RE.search(line) and not PERF_METRIC_RE.search(line):
            warnings.append(
                f"[{filename}:{i}] Vague description without metric:"
                f" {line.strip()[:80]}"
            )
    return warnings


def validate_file(path: "Path") -> "tuple[list[str], list[str]]":
    """Run all checks on a single roadmap file. Returns (errors, warnings)."""
    content = load_file(path)
    if content is None:
        return [], []

    filename = path.name
    errors: "list[str]" = []
    warnings: "list[str]" = []

    # Recommended sections → warnings only
    warnings += check_recommended_sections(content, filename)

    # Phases: missing phase headers in a phases-claiming file → error
    ph_errors, ph_warnings = check_phases(content, filename)
    errors += ph_errors
    warnings += ph_warnings

    # Task checkboxes: no tasks or vast majority missing targets → error
    task_errors, task_warnings = check_tasks(content, filename)
    errors += task_errors
    warnings += task_warnings

    # Vague descriptions → warnings
    warnings += check_vague_descriptions(content, filename)

    return errors, warnings


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent.parent

    # Only validate explicit roadmap / future-enhancement planning files.
    # FEATURE_ENHANCEMENT.md is the auto-generated code maturity analysis and
    # is intentionally excluded from structure validation.
    targets = [
        repo_root / "roadmap.md",
        repo_root / "ROADMAP.md",
        repo_root / "future_enhancement.md",
    ]

    all_errors: "list[str]" = []
    all_warnings: "list[str]" = []
    files_checked = 0

    for target in targets:
        content = load_file(target)
        if content is None:
            continue
        files_checked += 1
        errs, warns = validate_file(target)
        all_errors += errs
        all_warnings += warns

    if files_checked == 0:
        print("⚠️  No roadmap files found to validate (roadmap.md / future_enhancement.md)")
        return 0

    # Summary counts
    total_valid = sum(
        len(TASK_WITH_TARGET_RE.findall(load_file(t) or ""))
        for t in targets
    )

    print(f"✅ Valid Tasks (with Target annotation): {total_valid}")
    print(f"⚠️  Warnings: {len(all_warnings)}")
    print(f"❌ Errors:   {len(all_errors)}")

    if all_warnings:
        print("\n── Warnings ──────────────────────────────────────────────")
        for w in all_warnings:
            print(f"  ⚠️  {w}")

    if all_errors:
        print("\n── Errors ────────────────────────────────────────────────")
        for e in all_errors:
            print(f"  ❌ {e}")
        print("\n❌ Roadmap validation FAILED")
        return 1

    print("\n✅ Roadmap validation PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
