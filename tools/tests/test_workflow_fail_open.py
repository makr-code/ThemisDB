from __future__ import annotations

import json
import sys
import tempfile
from datetime import date
from pathlib import Path
import unittest

TOOLS = Path(__file__).resolve().parents[1]
CI = TOOLS / "ci"
sys.path.insert(0, str(CI))

import check_workflow_fail_open as guard  # noqa: E402


class WorkflowFailOpenTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def _write_workflow(self, name: str, content: str) -> None:
        workflows = self.root / ".github" / "workflows"
        workflows.mkdir(parents=True, exist_ok=True)
        (workflows / name).write_text(content.strip() + "\n", encoding="utf-8")

    def _write_allowlist(self, exceptions: list[dict]) -> Path:
        path = self.root / "allowlist.json"
        path.write_text(json.dumps({"exceptions": exceptions}), encoding="utf-8")
        return path

    def test_unclassified_gate_tolerance_fails(self) -> None:
        self._write_workflow(
            "gate-example.yml",
            """
name: "Gate: Example"
on: workflow_dispatch
jobs:
  check:
    steps:
      - name: Run check
        run: test-command || true
""",
        )

        errors = guard.validate_fail_open_exceptions(
            self.root,
            allowlist_path=self._write_allowlist([]),
            as_of=date(2026, 11, 9),
        )
        self.assertTrue(any("unclassified shell-or-true" in error for error in errors))

    def test_unclassified_reusable_workflow_command_fails(self) -> None:
        self._write_workflow(
            "gate-example.yml",
            """
name: "Gate: Example"
on: workflow_dispatch
jobs:
  check:
    uses: ./.github/workflows/reusable.yml
    with:
      configure_command: |
        cmake -S . -B build || true
""",
        )

        errors = guard.validate_fail_open_exceptions(
            self.root,
            allowlist_path=self._write_allowlist([]),
            as_of=date(2026, 11, 9),
        )
        self.assertTrue(any("unclassified shell-or-true" in error for error in errors))

    def test_classified_gate_tolerance_requires_current_metadata(self) -> None:
        self._write_workflow(
            "gate-example.yml",
            """
name: "Gate: Example"
on: workflow_dispatch
jobs:
  check:
    steps:
      - name: Notify
        continue-on-error: true
        uses: actions/github-script@v7
""",
        )
        entry = {
            "workflow": ".github/workflows/gate-example.yml",
            "job": "check",
            "step": "Notify",
            "kind": "continue-on-error",
            "match": "True",
            "count": 1,
            "rationale": "Notification only; validation remains blocking.",
            "owner": "CI maintainers",
            "expires": "2027-03-31",
        }

        self.assertEqual(
            guard.validate_fail_open_exceptions(
                self.root,
                allowlist_path=self._write_allowlist([entry]),
                as_of=date(2026, 11, 9),
            ),
            [],
        )

        expired_entry = {**entry, "expires": "2026-11-08"}
        errors = guard.validate_fail_open_exceptions(
            self.root,
            allowlist_path=self._write_allowlist([expired_entry]),
            as_of=date(2026, 11, 9),
        )
        self.assertTrue(any("expired" in error for error in errors))

    def test_non_gate_workflow_tolerance_is_outside_gate_inventory(self) -> None:
        self._write_workflow(
            "maintenance-example.yml",
            """
name: "Maintenance: Example"
on: workflow_dispatch
jobs:
  check:
    steps:
      - name: Run check
        run: test-command || true
""",
        )
        self.assertEqual(
            guard.validate_fail_open_exceptions(
                self.root,
                allowlist_path=self._write_allowlist([]),
                as_of=date(2026, 11, 9),
            ),
            [],
        )

    def test_allowlisted_repository_inventory_is_current(self) -> None:
        repository_root = Path(__file__).resolve().parents[2]
        errors = guard.validate_fail_open_exceptions(
            repository_root,
            as_of=date(2026, 11, 9),
        )
        self.assertEqual(errors, [], "\n".join(errors))


if __name__ == "__main__":
    unittest.main()
