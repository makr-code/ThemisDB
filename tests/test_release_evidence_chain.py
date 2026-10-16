#!/usr/bin/env python3

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "release_evidence_chain.py"


def load_module():
    spec = importlib.util.spec_from_file_location("release_evidence_chain", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


evidence_chain = load_module()


class ReleaseEvidenceChainTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.ledger_path = self.root / "ledger.jsonl"
        self.record_path = self.root / "record.json"

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def _run_generate(
        self,
        *,
        event_type: str,
        commit_sha: str,
        tag: str,
        version: str,
        approver_identity: str,
        approver_role: str,
        approval_scope: str,
        environment: str,
        workflow_name: str,
        workflow_run_id: str,
        source_workflow: str,
        source_run_id: str,
    ) -> int:
        args = evidence_chain.parse_args(
            [
                "generate",
                "--ledger",
                str(self.ledger_path),
                "--event-type",
                event_type,
                "--commit-sha",
                commit_sha,
                "--tag",
                tag,
                "--version",
                version,
                "--artifact-digest-sha256",
                "artifact-sha256-example",
                "--sbom-digest-sha256",
                "sbom-sha256-example",
                "--approver-identity",
                approver_identity,
                "--approver-role",
                approver_role,
                "--approval-scope",
                approval_scope,
                "--environment",
                environment,
                "--workflow-name",
                workflow_name,
                "--workflow-run-id",
                workflow_run_id,
                "--workflow-run-attempt",
                "1",
                "--workflow-url",
                f"https://github.com/makr-code/ThemisDB/actions/runs/{workflow_run_id}",
                "--job-name",
                "capture-evidence",
                "--source-workflow",
                source_workflow,
                "--source-run-id",
                source_run_id,
                "--repository",
                "makr-code/ThemisDB",
                "--record-output",
                str(self.record_path),
            ]
        )
        return evidence_chain.run_generate(args)

    def test_generates_genesis_record_with_expected_defaults(self) -> None:
        exit_code = self._run_generate(
            event_type="release_published",
            commit_sha="ABCDEF1234567890ABCDEF1234567890ABCDEF12",
            tag="v2.6.0",
            version="2.6.0",
            approver_identity="makr-code",
            approver_role="release_owner",
            approval_scope="release",
            environment="community-release",
            workflow_name="release-mainline-approval.yml",
            workflow_run_id="1001",
            source_workflow="release-mainline-approval.yml",
            source_run_id="1001",
        )
        self.assertEqual(exit_code, 0)
        records = evidence_chain.ReleaseEvidenceLedger(self.ledger_path).load_records()
        self.assertEqual(len(records), 1)
        self.assertEqual(records[0]["index"], 1)
        self.assertEqual(records[0]["previous_hash"], evidence_chain.GENESIS_HASH)
        self.assertEqual(records[0]["commit_sha"], "abcdef1234567890abcdef1234567890abcdef12")
        self.assertEqual(records[0]["approval_scope"], "release")

    def test_appends_hash_linked_records(self) -> None:
        self._run_generate(
            event_type="release_published",
            commit_sha="1111111111111111111111111111111111111111",
            tag="v2.6.1",
            version="2.6.1",
            approver_identity="makr-code",
            approver_role="release_owner",
            approval_scope="release",
            environment="community-release",
            workflow_name="release-mainline-approval.yml",
            workflow_run_id="2001",
            source_workflow="release-mainline-approval.yml",
            source_run_id="2001",
        )
        self._run_generate(
            event_type="deploy_executed",
            commit_sha="1111111111111111111111111111111111111111",
            tag="v2.6.1",
            version="2.6.1",
            approver_identity="makr-code",
            approver_role="deploy_owner",
            approval_scope="deploy",
            environment="docker-production",
            workflow_name="release-docker-image.yml",
            workflow_run_id="2002",
            source_workflow="release-docker-approval.yml",
            source_run_id="1999",
        )
        ledger = evidence_chain.ReleaseEvidenceLedger(self.ledger_path)
        records = ledger.load_records()
        self.assertEqual(len(records), 2)
        self.assertEqual(records[1]["previous_hash"], records[0]["record_hash"])
        verified_records = ledger.verify()
        self.assertEqual(len(verified_records), 2)

    def test_detects_tampering(self) -> None:
        self._run_generate(
            event_type="release_published",
            commit_sha="2222222222222222222222222222222222222222",
            tag="v2.6.2",
            version="2.6.2",
            approver_identity="makr-code",
            approver_role="release_owner",
            approval_scope="release",
            environment="community-release",
            workflow_name="release-mainline-approval.yml",
            workflow_run_id="3001",
            source_workflow="release-mainline-approval.yml",
            source_run_id="3001",
        )
        lines = self.ledger_path.read_text(encoding="utf-8").splitlines()
        tampered = json.loads(lines[0])
        tampered["environment"] = "tampered-environment"
        self.ledger_path.write_text(json.dumps(tampered, ensure_ascii=False, sort_keys=True) + "\n", encoding="utf-8")
        with self.assertRaises(evidence_chain.EvidenceChainError):
            evidence_chain.ReleaseEvidenceLedger(self.ledger_path).verify()


if __name__ == "__main__":
    unittest.main()
