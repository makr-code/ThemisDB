#!/usr/bin/env python3
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path


_HERE = Path(__file__).resolve().parent
_TOOLS = _HERE.parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from check_compliance_governance_controls import run  # noqa: E402


VALID_DOC = """# Title
## Compliance-Control-Matrix
| Control ID | Severity | Bereich | Control | Owner | Evidence-Typ | Prüffrequenz | Enforcement |
|---|---|---|---|---|---|---|---|
| C-001 | Critical | Secure SDLC | Control | Team | PR evidence | Je PR | Gate |
## Governance-Policy-Set
## Audit-Logik (Risk Acceptance / Exception History)
| Exception ID | Betroffener Control | Entscheidung (wer) | Entscheidung (wann) | Ablaufdatum | Mitigation-Plan | Status |
|---|---|---|---|---|---|---|
| RA-2026-001 | C-001 | Security Officer | 2026-04-20 | 2026-06-30 | Plan | Open |
## Folge-Issues pro Lücke (Gap-Backlog)
| Gap ID | Severity | Lücke | Owner | Deadline | Tracking |
|---|---|---|---|---|---|
| GAP-001 | Critical | Gap text | Team | 2026-06-30 | gap-1 |
## Implementation Phases
## Definition of Done
"""


class ComplianceGovernanceGateTests(unittest.TestCase):
    def test_gate_passes_with_complete_document(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            doc = repo / "docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md"
            doc.parent.mkdir(parents=True, exist_ok=True)
            doc.write_text(VALID_DOC, encoding="utf-8")
            self.assertEqual(run(repo), 0)

    def test_gate_fails_when_critical_metadata_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            doc = repo / "docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md"
            doc.parent.mkdir(parents=True, exist_ok=True)
            invalid = VALID_DOC.replace(
                "| C-001 | Critical | Secure SDLC | Control | Team | PR evidence | Je PR | Gate |",
                "| C-001 | Critical | Secure SDLC | Control | - | PR evidence | Je PR | Gate |",
            )
            doc.write_text(invalid, encoding="utf-8")
            self.assertEqual(run(repo), 1)

    def test_gate_fails_when_file_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(run(Path(tmp)), 1)


if __name__ == "__main__":
    unittest.main()
