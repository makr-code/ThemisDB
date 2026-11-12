import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"


def load_workflow(name: str) -> dict:
    return yaml.safe_load((WORKFLOWS / name).read_text(encoding="utf-8"))


class WorkflowDryRunContractTests(unittest.TestCase):
    def test_run_summary_jobs_cover_all_workflow_jobs(self) -> None:
        expected = {
            "build-content-regression.yml": {"content-regression", "wave-d-soak"},
            "gate-pr-benchmark-ctest.yml": {"benchmark-ctest-coverage"},
            "maintenance-ci-health.yml": {"ci-health-dashboard"},
            "maintenance-workflow-guardrails-observe.yml": {"workflow-guardrails"},
            "security-consolidated.yml": {"trivy", "gitleaks", "kubesec"},
            "security-dast-zap.yml": {"zap-baseline", "zap-api-scan", "zap-full-scan"},
        }

        for workflow_name, expected_dependencies in expected.items():
            with self.subTest(workflow=workflow_name):
                jobs = load_workflow(workflow_name)["jobs"]
                summary_job = jobs["run-summary"]
                self.assertEqual(set(summary_job["needs"]), expected_dependencies)
                self.assertEqual(summary_job["if"], "always()")
                self.assertEqual(summary_job["permissions"], {"contents": "read"})

                steps = summary_job["steps"]
                self.assertEqual(steps[0]["uses"], "actions/checkout@v5")
                self.assertIs(steps[0]["with"]["persist-credentials"], False)
                self.assertEqual(steps[1]["uses"], "./.github/actions/write-run-summary")
                self.assertEqual(steps[1]["with"]["job-results"], "${{ toJSON(needs) }}")

    def test_workflow_inputs_declare_types_and_descriptions(self) -> None:
        for workflow_path in sorted(WORKFLOWS.glob("*.yml")):
            workflow = yaml.safe_load(workflow_path.read_text(encoding="utf-8")) or {}
            triggers = workflow.get(True, workflow.get("on", {})) or {}
            for trigger_name in ("workflow_dispatch", "workflow_call"):
                trigger = triggers.get(trigger_name, {}) or {}
                inputs = trigger.get("inputs", {}) or {}
                for input_name, spec in inputs.items():
                    with self.subTest(workflow=workflow_path.name, trigger=trigger_name, input=input_name):
                        self.assertIn("type", spec)
                        self.assertTrue(str(spec.get("description", "")).strip())
                        if spec["type"] == "choice":
                            self.assertTrue(spec.get("options"))

    def test_side_effect_entry_points_default_to_dry_run(self) -> None:
        manual_workflows = (
            "maintenance-ai-working.yml",
            "maintenance-architecture-ci.yml",
            "maintenance-compendium-sync.yml",
            "maintenance-housekeeping.yml",
            "maintenance-soll-ist-gap-issues.yml",
            "publish-wiki.yml",
            "release-docker-image.yml",
            "release-nightly.yml",
            "release-promote.yml",
            "release-rollback.yml",
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

    def test_nightly_manual_dry_run_validates_without_publishing_or_updating_issues(self) -> None:
        workflow = load_workflow("release-nightly.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled runs still publish", dry_run["description"])

        jobs = workflow["jobs"]
        self.assertIn("inputs.dry_run != true", jobs["mark-release-pending"]["if"])
        self.assertIn("'schedule'", jobs["mark-release-pending"]["if"])

        release_steps = jobs["publish-release"]["steps"]
        create_release = next(step for step in release_steps if step.get("name") == "Create GitHub Release")
        preview_release = next(
            step for step in release_steps if step.get("name") == "Preview GitHub Release publication"
        )
        self.assertIn("inputs.dry_run != true", create_release["if"])
        self.assertIn("inputs.dry_run", preview_release["if"])
        self.assertIn("$GITHUB_STEP_SUMMARY", preview_release["run"])

        docker_steps = jobs["publish-docker"]["steps"]
        by_name = {step["name"]: step for step in docker_steps}
        for login_name in ("Log in to Docker Hub", "Log in to GitHub Container Registry (OIDC)"):
            with self.subTest(login=login_name):
                self.assertIn("inputs.dry_run != true", by_name[login_name]["if"])

        build = by_name["Build staged Docker runtime candidate"]["with"]
        self.assertIn("inputs.dry_run != true", build["push"])
        self.assertIn("inputs.dry_run", build["load"])
        smoke = by_name["Validate default container startup before publication"]
        self.assertIn("inputs.dry_run", smoke["env"]["DRY_RUN"])
        self.assertIn("runtime_smoke_gate.py", smoke["run"])
        self.assertIn('if [ "${DRY_RUN}" = "true" ]', smoke["run"])
        self.assertIn("docker pull", smoke["run"])

        self.assertIn("inputs.dry_run != true", jobs["publish-docker-manifest"]["if"])
        self.assertIn("inputs.dry_run != true", jobs["finalize-release-status"]["if"])
        notify = jobs["notify"]["steps"][0]
        self.assertIn("$GITHUB_STEP_SUMMARY", notify["run"])
        self.assertIn("publication and issue updates were suppressed", notify["run"])

    def test_release_promotion_manual_dry_run_previews_without_github_writes(self) -> None:
        workflow = load_workflow("release-promote.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dispatch = triggers["workflow_dispatch"]
        dry_run = dispatch["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)

        jobs = workflow["jobs"]
        self.assertEqual(workflow["permissions"], {"contents": "read", "actions": "read"})
        self.assertIn("should-run", jobs["update-changelog"]["if"])

        create_pr = jobs["create-release-pr"]
        self.assertIn("inputs.dry_run != true", create_pr["if"])
        self.assertIn("github.event_name != 'workflow_dispatch'", create_pr["if"])
        self.assertEqual(create_pr["permissions"]["contents"], "write")
        self.assertEqual(create_pr["permissions"]["pull-requests"], "write")
        self.assertEqual(create_pr["permissions"]["issues"], "write")

        notify = jobs["notify"]
        self.assertIn("always()", notify["if"])
        self.assertIn(
            "needs.create-release-pr.result == 'success'",
            notify["steps"][0]["if"],
        )
        report = notify["steps"][1]
        self.assertIn("inputs.dry_run", report["env"]["DRY_RUN"])
        self.assertIn("GitHub writes suppressed", report["run"])
        self.assertIn("$GITHUB_STEP_SUMMARY", report["run"])

    def test_release_rollback_dry_run_validates_and_suppresses_mutations(self) -> None:
        workflow = load_workflow("release-rollback.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dispatch = triggers["workflow_dispatch"]
        dry_run = dispatch["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertEqual(
            workflow["permissions"],
            {"contents": "read", "actions": "read"},
        )

        jobs = workflow["jobs"]
        self.assertEqual(
            set(jobs["run-summary"]["needs"]),
            {
                "validate-rollback",
                "delete-github-release",
                "delete-docker-tags",
                "revert-version",
                "create-rollback-pr",
                "notify-rollback",
            },
        )
        validate = jobs["validate-rollback"]["steps"][0]
        self.assertIn("VERSION: ${{ inputs.version }}", validate["env"])
        self.assertIn("semantic version", validate["run"])
        self.assertIn("No rollback actions selected", validate["run"])
        self.assertIn("previous-version must differ", validate["run"])

        delete_release = jobs["delete-github-release"]
        self.assertIn("inputs.dry_run != true", delete_release["if"])
        self.assertEqual(delete_release["permissions"], {"contents": "write"})
        self.assertIn("process.env.VERSION", delete_release["steps"][0]["with"]["script"])
        self.assertIn("if (e.status === 404)", delete_release["steps"][0]["with"]["script"])

        docker_cleanup = jobs["delete-docker-tags"]
        self.assertNotIn("permissions", docker_cleanup)
        self.assertIn("not automated", docker_cleanup["steps"][0]["run"])
        self.assertIn("no registry mutation was attempted", docker_cleanup["steps"][0]["run"])

        version_plan = jobs["revert-version"]
        self.assertNotIn("permissions", version_plan)
        self.assertIn("Dry-run: no files were changed", version_plan["steps"][0]["run"])
        rollback_pr = jobs["create-rollback-pr"]
        self.assertIn("inputs.dry_run != true", rollback_pr["if"])
        self.assertEqual(
            rollback_pr["permissions"],
            {"contents": "write", "pull-requests": "write"},
        )
        notify = jobs["notify-rollback"]["steps"][0]
        self.assertIn("$GITHUB_STEP_SUMMARY", notify["run"])
        self.assertIn("No rollback side effects were started", notify["run"])

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
