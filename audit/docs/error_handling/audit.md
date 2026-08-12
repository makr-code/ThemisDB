# Error-Handling Audit – Usage Guide

The audit tool (`tools/error_handling_audit.py`) checks source files for compliance with the rules defined in [`docs/error_handling/checklist.md`](/docs/error_handling/checklist.md).

---

## Running the audit locally

From the **repository root**:

```bash
# Scan default paths (src/ include/ apps/ tools/ scripts/ clients/ plugins/)
python3 tools/error_handling_audit.py

# Scan specific directories or files
python3 tools/error_handling_audit.py src/query/ include/query/

# JSON output (useful for scripting / IDE integrations)
python3 tools/error_handling_audit.py --format json

# Suppress per-violation detail and only print the summary
python3 tools/error_handling_audit.py -q

# Disable colour (e.g. when piping to a file)
python3 tools/error_handling_audit.py --no-color | tee audit-report.txt
```

Exit code:
- `0` – no violations (or violations within budget, see `--max-violations`)
- `1` – one or more violations found
- `2` – internal/argument error

---

## CI integration

The workflow `.github/workflows/error-handling-audit.yml` runs automatically on every PR and on pushes to `community`/`develop` that touch source files. It:

1. Runs the unit tests for the audit tool itself (`tests/test_error_handling_audit.py`).
2. Runs the audit against the repository and fails if violations exceed the configured budget.

The `--max-violations N` flag lets the CI pass while the existing violation backlog is being worked down. When `N` is reached or exceeded the job fails, blocking the PR.

**To tighten the budget** (enforce stricter compliance over time): reduce `N` in the workflow file and fix the newly surfaced violations before merging.

---

## Ignoring files or directories

Edit `tools/error_handling_audit.ignore`. Each non-blank, non-comment line is a path **prefix** relative to the repo root. Example:

```
# Third-party submodule – not our code
llama.cpp

# Generated protobuf output
src/proto/generated
```

Rules for the ignore file:

| Syntax | Meaning |
|--------|---------|
| `some/dir` | Ignores everything under `some/dir/` and `some/dir` itself |
| `some/dir/*` | Same (trailing `*` is equivalent) |
| `# comment` | Ignored |

---

## Adding a rule exception for a single file

If a specific file must violate a rule for a documented reason, add it to the ignore file with a comment explaining why:

```
# tools/fault_injector.py uses bare except intentionally for test harness resilience
# See https://github.com/makr-code/ThemisDB/issues/XXXX for context
tools/fault_injector.py
```

---

## Rules reference

See [`docs/error_handling/checklist.md`](/docs/error_handling/checklist.md) for the full list of rules and their rationale. Quick summary:

| Rule | Language | What it checks |
|------|----------|----------------|
| RULE-CPP-001 | C++ | `return nullptr` in non-nullable functions → use `Result<T*>` |
| RULE-CPP-002 | C++ | `catch(...)` without a logging call |
| RULE-CPP-003 | C++ | Local `struct Status` definitions → use `Result<T>` |
| RULE-PY-001  | Python | `except:` / `except Exception:` without logging or re-raise |
| RULE-CS-001  | C#  | Empty or silent `catch` blocks |
| RULE-PHP-001 | PHP | Empty or silent `catch` blocks |
| RULE-PS1-001 | PowerShell | Empty or silent `catch` blocks |

---

## Running the unit tests

```bash
pip install pytest
python3 -m pytest tests/test_error_handling_audit.py -v
```
