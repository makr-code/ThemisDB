#!/usr/bin/env python3

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "ga_signoff.py"
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "ga-promotion-signoff.yml"
ISSUE_TEMPLATE_PATH = REPO_ROOT / ".github" / "ISSUE_TEMPLATE" / "ga_promotion_signoff.md"


def load_module():
    spec = importlib.util.spec_from_file_location("ga_signoff", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


ga_signoff = load_module()


class GASignoffTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.payload_path = self.root / "request.json"
        self.output_dir = self.root / "out"
        self.result_path = self.root / "result.json"
        self.maintainers_path = self.root / "maintainers.json"
        self.maintainers_path.write_text(
            json.dumps(
                {
                    "schema_version": 1,
                    "policy": "ga-promotion-signoff",
                    "maintainers": [
                        {"github": "makr-code", "role": "project-lead", "can_sign_ga": True},
                        {"github": "other-user", "role": "observer", "can_sign_ga": False},
                    ],
                }
            ),
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write_payload(self, approver: str = "__GITHUB_ACTOR__", signed_at: str = "__GITHUB_REVIEW_SUBMITTED_AT__") -> None:
        self.payload_path.write_text(
            json.dumps(
                {
                    "schema_version": 1,
                    "ga_name": "ThemisDB v2.4.0 GA",
                    "target_version": "v2.4.0",
                    "release_tag": "v2.4.0",
                    "sponsor": {"name": "Platform Release", "team": "Core Maintainers"},
                    "requested_by": {"github": "requester"},
                    "approver": {"github": approver, "role": "authorized-maintainer"},
                    "signoff": {
                        "signed_at": signed_at,
                        "rationale": "All GA gates passed.",
                    },
                    "repository_state": {
                        "repository": "makr-code/ThemisDB",
                        "ref": "__GITHUB_REF__",
                        "sha": "__GITHUB_SHA__",
                        "source_document": "docs/governance/GA_PROMOTION_SIGN_OFF.md",
                    },
                    "references": {
                        "issue_or_pr": "PR #1234",
                        "evidence_path": "docs/governance/GA_PROMOTION_SIGN_OFF.md#9-human-ga-approval--signature-block",
                    },
                    "notes": "Store the manifest as a workflow artifact.",
                }
            ),
            encoding="utf-8",
        )

    def run_script(self, *extra_args: str, expect_success: bool = True) -> subprocess.CompletedProcess[str]:
        command = [
            sys.executable,
            str(SCRIPT_PATH),
            "--payload",
            str(self.payload_path),
            "--authorized-maintainers",
            str(self.maintainers_path),
            "--output-dir",
            str(self.output_dir),
            "--result-json",
            str(self.result_path),
            "--github-repository",
            "makr-code/ThemisDB",
            "--github-ref",
            "refs/heads/develop",
            "--github-sha",
            "0123456789abcdef0123456789abcdef01234567",
            "--github-event-name",
            "pull_request_review",
            "--github-run-id",
            "999",
            "--github-actor",
            "",
            *extra_args,
        ]
        completed = subprocess.run(command, text=True, capture_output=True)
        if expect_success and completed.returncode != 0:
            self.fail(f"Expected success, got {completed.returncode}: {completed.stderr}\nSTDOUT: {completed.stdout}")
        if not expect_success and completed.returncode == 0:
            self.fail(f"Expected failure, got success: {completed.stdout}")
        return completed

    def test_preview_manifest_allows_ci_placeholders(self) -> None:
        self.write_payload()
        self.run_script("--allow-unresolved-placeholders")

        result = json.loads(self.result_path.read_text(encoding="utf-8"))
        manifest = json.loads(Path(result["manifest_path"]).read_text(encoding="utf-8"))

        self.assertTrue(result["preview"])
        self.assertTrue(manifest["preview"])
        self.assertEqual(manifest["canonical_record"]["approver"]["github"], "__GITHUB_ACTOR__")
        self.assertEqual(
            manifest["canonical_record"]["signoff"]["signed_at"],
            "__GITHUB_REVIEW_SUBMITTED_AT__",
        )

    def test_final_manifest_binds_authorized_actor_and_timestamp(self) -> None:
        self.write_payload()
        self.run_script(
            "--github-actor",
            "makr-code",
            "--signed-at",
            "2026-09-21T05:00:00+02:00",
        )

        result = json.loads(self.result_path.read_text(encoding="utf-8"))
        manifest = json.loads(Path(result["manifest_path"]).read_text(encoding="utf-8"))
        sha_line = Path(result["manifest_sha256_path"]).read_text(encoding="utf-8").strip()

        self.assertFalse(result["preview"])
        self.assertEqual(result["approver"], "makr-code")
        self.assertEqual(result["signed_at"], "2026-09-21T03:00:00Z")
        self.assertEqual(manifest["canonical_record"]["signoff"]["signed_at"], "2026-09-21T03:00:00Z")
        self.assertIn(result["manifest_sha256"], sha_line)
        self.assertTrue(manifest["validation"]["actor_matches_approver"])

    def test_rejects_unauthorized_actor(self) -> None:
        self.write_payload()
        completed = self.run_script(
            "--github-actor",
            "unauthorized-user",
            "--signed-at",
            "2026-09-21T03:00:00Z",
            expect_success=False,
        )
        self.assertIn("not authorized to sign GA promotions", completed.stderr)

    def test_rejects_mismatched_approver_and_actor(self) -> None:
        self.write_payload(approver="someone-else")
        completed = self.run_script(
            "--github-actor",
            "makr-code",
            "--signed-at",
            "2026-09-21T03:00:00Z",
            expect_success=False,
        )
        self.assertIn("does not match GitHub actor", completed.stderr)

    def test_workflow_is_scoped_and_supports_review_dispatch(self) -> None:
        workflow = WORKFLOW_PATH.read_text(encoding="utf-8")
        self.assertIn("pull_request_review:", workflow)
        self.assertIn("workflow_dispatch:", workflow)
        self.assertIn("docs/governance/ga_signoff_requests/**", workflow)
        self.assertIn("actions/upload-artifact@v4", workflow)
        self.assertIn("comment_issue", workflow)

    def test_issue_template_links_issue_to_payload_and_pr_flow(self) -> None:
        template = ISSUE_TEMPLATE_PATH.read_text(encoding="utf-8")
        self.assertIn("docs/governance/ga_signoff_requests/", template)
        self.assertIn("GA_PROMOTION_SIGN_OFF_REQUEST.example.yaml", template)
        self.assertIn("Handoff to PR", template)
        self.assertIn("authorized maintainer", template)


if __name__ == "__main__":
    unittest.main()
