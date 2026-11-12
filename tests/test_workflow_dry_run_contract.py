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

        steps = workflow["jobs"]["soll_ist_sync"]["steps"]
        sync = next(step for step in steps if step["name"] == "Sync issues (idempotent)")
        upload = next(step for step in steps if step["name"] == "Upload issue sync result")
        for step in (sync, upload):
            with self.subTest(step=step["name"]):
                self.assertIn("github.event_name == 'schedule'", step["if"])
                self.assertIn("github.event.inputs.apply_changes == 'true'", step["if"])
                self.assertIn("github.event.inputs.dry_run != 'true'", step["if"])


if __name__ == "__main__":
    unittest.main()
