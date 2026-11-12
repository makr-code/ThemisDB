import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"


def load_workflow(name: str) -> dict:
    return yaml.safe_load((WORKFLOWS / name).read_text(encoding="utf-8"))


class WorkflowDryRunContractTests(unittest.TestCase):
    def test_side_effect_entry_points_default_to_dry_run(self) -> None:
        manual_workflows = (
            "maintenance-ai-working.yml",
            "maintenance-architecture-ci.yml",
            "maintenance-compendium-sync.yml",
            "maintenance-housekeeping.yml",
            "maintenance-soll-ist-gap-issues.yml",
            "publish-wiki.yml",
            "release-docker-image.yml",
            "wiki-pr-gate.yml",
        )
        reusable_workflows = (
            "release-changelog.yml",
            "release-docker-image.yml",
            "release-mainline.yml",
            "reusable-docker-image-platform.yml",
        )

        for name in manual_workflows:
            with self.subTest(workflow=name, entry_point="workflow_dispatch"):
                workflow = load_workflow(name)
                triggers = workflow.get(True, workflow.get("on"))
                inputs = triggers["workflow_dispatch"]["inputs"]
                dry_run = next(spec for key, spec in inputs.items() if key.lower().replace("-", "_") == "dry_run")
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        for name in reusable_workflows:
            with self.subTest(workflow=name, entry_point="workflow_call"):
                inputs = load_workflow(name)[True]["workflow_call"]["inputs"]
                dry_run = next(spec for key, spec in inputs.items() if key.lower().replace("-", "_") == "dry_run")
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

    def test_wiki_dry_run_still_builds_and_validates_without_mutation(self) -> None:
        workflow = load_workflow("publish-wiki.yml")
        steps = workflow["jobs"]["publish-wiki"]["steps"]
        build = next(step for step in steps if step.get("id") == "build")
        validation = next(step for step in steps if step.get("id") == "linkcheck")

        self.assertIn("--output /tmp/wiki-staging", build["run"])
        self.assertNotIn("--dry-run", build["run"])
        self.assertNotIn("if", validation)

        publish = next(step for step in steps if step.get("id") == "publish")
        self.assertIn("steps.build.outputs.is_dry_run != 'true'", publish["if"])

    def test_architecture_dry_run_still_generates_and_validates(self) -> None:
        workflow = load_workflow("maintenance-architecture-ci.yml")
        steps = workflow["jobs"]["generate-architecture"]["steps"]
        generate = next(step for step in steps if step.get("id") == "generate")
        validate = next(step for step in steps if step.get("id") == "validation")
        branch = next(step for step in steps if step.get("id") == "branch")

        self.assertNotIn("--dry-run", generate["run"])
        self.assertNotIn("if", validate)
        self.assertIn("github.event.inputs.dry_run != 'true'", branch["if"])

    def test_manual_wiki_gate_dry_run_suppresses_issue_mutations(self) -> None:
        workflow = load_workflow("wiki-pr-gate.yml")
        steps = workflow["jobs"]["wiki_pr_gate"]["steps"]
        issue_steps = [
            step for step in steps
            if step.get("uses") == "./.github/actions/status-flags-and-issues"
        ]

        self.assertEqual(len(issue_steps), 3)
        for step in issue_steps:
            with self.subTest(step=step["name"]):
                self.assertIn("inputs.dry_run == true", step["if"])
                self.assertIn("workflow_dispatch", step["if"])

    def test_housekeeping_dry_run_previews_recommendation_comments(self) -> None:
        workflow_text = (WORKFLOWS / "maintenance-housekeeping.yml").read_text(encoding="utf-8")
        self.assertIn("if (dryRun)", workflow_text)
        self.assertIn("[dry-run] would post closure recommendation", workflow_text)

    def test_soll_ist_manual_dry_run_suppresses_issue_mutations_but_schedule_syncs(self) -> None:
        workflow = load_workflow("maintenance-soll-ist-gap-issues.yml")
        triggers = workflow.get(True, workflow.get("on"))
        inputs = triggers["workflow_dispatch"]["inputs"]
        self.assertIs(inputs["dry_run"]["default"], True)
        self.assertEqual(inputs["apply_changes"]["type"], "boolean")
        self.assertIs(inputs["apply_changes"]["default"], False)
        self.assertEqual(inputs["close_resolved"]["type"], "boolean")
        self.assertIs(inputs["close_resolved"]["default"], False)

        steps = workflow["jobs"]["soll_ist_sync"]["steps"]
        sync = next(step for step in steps if step["name"] == "Sync issues (idempotent)")
        upload = next(step for step in steps if step["name"] == "Upload issue sync result")
        for step in (sync, upload):
            with self.subTest(step=step["name"]):
                self.assertIn("github.event_name == 'schedule'", step["if"])
                self.assertIn("github.event.inputs.apply_changes == 'true'", step["if"])
                self.assertIn("github.event.inputs.dry_run != 'true'", step["if"])

    def test_manual_boolean_toggles_have_explicit_types_and_preserved_defaults(self) -> None:
        expected = {
            "build-wave-a-gpu.yml": ("run_representative_hardware", False),
            "coverage-report-nightly.yml": ("generate_baseline", False),
            "nightly-governance-audit.yml": ("include_pr_validation", True),
        }
        for workflow_name, (input_name, default) in expected.items():
            with self.subTest(workflow=workflow_name, input=input_name):
                workflow = load_workflow(workflow_name)
                triggers = workflow.get(True, workflow.get("on"))
                input_spec = triggers["workflow_dispatch"]["inputs"][input_name]
                self.assertEqual(input_spec["type"], "boolean")
                self.assertIs(input_spec["default"], default)

        coverage = load_workflow("coverage-report-nightly.yml")
        update = next(
            step for step in coverage["jobs"]["coverage-collection"]["steps"]
            if step.get("name") == "Update baseline on develop"
        )
        self.assertIn("github.event_name == 'schedule'", update["if"])
        self.assertIn("github.event.inputs.generate_baseline == 'true'", update["if"])

    def test_manual_parameter_inputs_have_explicit_types_and_supported_choices(self) -> None:
        expected = {
            "build-benchmarks.yml": {
                "backend_filter": ("choice", ["all", "cuda", "hip", "vulkan"]),
                "benchmark_filter": ("string", None),
            },
            "build-content-regression.yml": {
                "soak_duration_multiplier": ("number", None),
            },
            "build-wave-a-gpu.yml": {
                "phase": ("choice", ["all", "baseline", "fallback", "memory", "determinism"]),
                "parallel_jobs": ("number", None),
                "representative_cuda_arch": ("string", None),
            },
            "compliance-governance-gates.yml": {"rc_tag": ("string", None)},
            "compliance-supply-chain.yml": {"evidence_window_days": ("number", None)},
            "ga-promotion-signoff.yml": {
                "payload_path": ("string", None),
                "ref": ("string", None),
                "signed_at": ("string", None),
            },
            "maintenance-soll-ist-gap-issues.yml": {"source_gap_glob": ("string", None)},
            "release-evidence-chain.yml": {
                "event_type": ("string", None),
                "commit_sha": ("string", None),
                "tag": ("string", None),
                "version": ("string", None),
                "artifact_digest_sha256": ("string", None),
                "sbom_digest_sha256": ("string", None),
                "approver_identity": ("string", None),
                "approver_role": ("string", None),
                "approval_scope": ("string", None),
                "environment": ("string", None),
                "source_workflow": ("string", None),
                "source_run_id": ("string", None),
            },
            "security-pentest-quarterly.yml": {"scope": ("choice", ["quick-scan"])},
        }

        for workflow_name, input_specs in expected.items():
            workflow = load_workflow(workflow_name)
            triggers = workflow.get(True, workflow.get("on"))
            dispatch_inputs = triggers["workflow_dispatch"]["inputs"]
            for input_name, (input_type, options) in input_specs.items():
                with self.subTest(workflow=workflow_name, input=input_name):
                    input_spec = dispatch_inputs[input_name]
                    self.assertEqual(input_spec["type"], input_type)
                    if options is not None:
                        self.assertEqual(input_spec["options"], options)


if __name__ == "__main__":
    unittest.main()
