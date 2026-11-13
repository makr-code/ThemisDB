import ast
import os
import re
import subprocess
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"

SARIF_UPLOAD_POLICY_CASES = (
    ("scheduled", "schedule", True, False, True),
    ("direct internal PR", "pull_request", None, True, True),
    ("direct fork PR", "pull_request", None, False, False),
    ("reusable PR default dry-run", "pull_request", True, True, False),
    ("reusable internal PR live", "pull_request", False, True, True),
    ("reusable fork PR live input", "pull_request", False, False, False),
    ("manual default dry-run", "workflow_dispatch", True, True, False),
    ("manual live scan", "workflow_dispatch", False, True, True),
    ("reusable push default dry-run", "push", True, True, False),
    ("reusable push live scan", "push", False, True, True),
)


def load_workflow(name: str) -> dict:
    return yaml.safe_load((WORKFLOWS / name).read_text(encoding="utf-8"))


def evaluate_sarif_upload_condition(
    condition: str,
    event_name: str,
    dry_run: bool | None,
    same_repository_pr: bool,
) -> bool:
    replacements = {
        "always()": "True",
        "github.event.pull_request.head.repo.full_name == github.repository": str(same_repository_pr),
        "github.event_name == 'schedule'": str(event_name == "schedule"),
        "github.event_name == 'pull_request'": str(event_name == "pull_request"),
        "github.event_name != 'pull_request'": str(event_name != "pull_request"),
        "inputs.dry_run != true": str(dry_run is not True),
    }
    for token in sorted(replacements, key=len, reverse=True):
        condition = condition.replace(token, replacements[token])
    condition = condition.replace("&&", " and ").replace("||", " or ")
    tree = ast.parse(condition, mode="eval")

    def evaluate(node: ast.AST) -> bool:
        if isinstance(node, ast.Expression):
            return evaluate(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, bool):
            return node.value
        if isinstance(node, ast.BoolOp) and isinstance(node.op, (ast.And, ast.Or)):
            values = [evaluate(value) for value in node.values]
            return all(values) if isinstance(node.op, ast.And) else any(values)
        raise AssertionError(f"Unsupported SARIF condition expression: {ast.dump(node)}")

    return evaluate(tree)


class WorkflowDryRunContractTests(unittest.TestCase):
    def test_run_summary_jobs_cover_all_workflow_jobs(self) -> None:
        expected = {
            "build-content-regression.yml": {"content-regression", "wave-d-soak"},
            "gate-pr-benchmark-ctest.yml": {"benchmark-ctest-coverage", "gate-feedback"},
            "maintenance-ci-health.yml": {"ci-health-dashboard", "update-chronic-issue"},
            "maintenance-workflow-guardrails-observe.yml": {"workflow-guardrails"},
            "security-consolidated.yml": {"trivy", "gitleaks", "kubesec"},
            "security-dast-zap.yml": {
                "validate-inputs",
                "zap-baseline",
                "zap-api-scan",
                "zap-full-scan",
            },
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
            "maintenance-build-issues.yml",
            "maintenance-ci-health.yml",
            "maintenance-issues.yml",
            "maintenance-docs.yml",
            "maintenance-pr-failure-diagnosis.yml",
            "maintenance-soll-ist-gap-issues.yml",
            "publish-wiki.yml",
            "build-widget.yml",
            "build-mainline.yml",
            "build-llm-inference.yml",
            "release-docker-image.yml",
            "release-nightly.yml",
            "release-promote.yml",
            "release-rollback.yml",
            "release-evidence-chain.yml",
            "release-linux-distribution.yml",
            "release-windows-distribution.yml",
            "release-winget.yml",
            "wiki-pr-gate.yml",
        )
        reusable_workflows = (
            "release-changelog.yml",
            "release-docker-image.yml",
            "release-linux-distribution.yml",
            "release-windows-distribution.yml",
            "release-mainline.yml",
            "reusable-docker-image-platform.yml",
            "release-winget.yml",
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

    def test_ai_working_maintenance_write_permissions_are_job_scoped(self) -> None:
        workflow = load_workflow("maintenance-ai-working.yml")

        self.assertEqual(workflow["permissions"], {"contents": "read"})
        self.assertEqual(
            workflow["jobs"]["ai-working-compact"]["permissions"],
            {"contents": "write"},
        )
        self.assertEqual(
            workflow["jobs"]["llm-wiki-sync"]["permissions"],
            {"contents": "write"},
        )

    def test_mainline_and_llm_manual_dry_runs_preserve_validation_and_suppress_tracker_writes(self) -> None:
        mainline = load_workflow("build-mainline.yml")
        mainline_triggers = mainline.get(True, mainline.get("on"))
        mainline_dry_run = mainline_triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertIs(mainline_dry_run["default"], True)
        self.assertEqual(mainline["permissions"], {"contents": "read", "actions": "read"})

        mainline_jobs = mainline["jobs"]
        self.assertEqual(
            mainline_jobs["mark-build-pending"]["permissions"],
            {"contents": "read", "issues": "write"},
        )
        self.assertEqual(
            mainline_jobs["update-build-labels"]["permissions"],
            {"contents": "read", "issues": "write"},
        )
        pending_steps = mainline_jobs["mark-build-pending"]["steps"]
        pending_preview = next(step for step in pending_steps if step.get("name") == "Preview build tracker pending label")
        self.assertIn("inputs.dry_run", pending_preview["if"])
        self.assertIn("$GITHUB_STEP_SUMMARY", pending_preview.get("run", ""))
        update_steps = mainline_jobs["update-build-labels"]["steps"]
        tracker_steps = [
            step for step in update_steps
            if step.get("uses") == "./.github/actions/status-flags-and-issues"
        ]
        self.assertEqual(len(tracker_steps), 3)
        self.assertTrue(all("inputs.dry_run != true" in step["if"] for step in tracker_steps))
        preview = next(step for step in update_steps if step.get("name") == "Preview build tracker status")
        self.assertIn("inputs.dry_run", preview["if"])
        self.assertIn("$GITHUB_STEP_SUMMARY", preview["run"])
        self.assertEqual(mainline_jobs["build"]["uses"], "./.github/workflows/reusable-cmake-build.yml")
        self.assertTrue(mainline_jobs["macos-kqueue-validation"]["steps"])

        llm = load_workflow("build-llm-inference.yml")
        llm_triggers = llm.get(True, llm.get("on"))
        llm_dry_run = llm_triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertIs(llm_dry_run["default"], True)
        self.assertEqual(llm["permissions"], {"contents": "read"})

        report = llm["jobs"]["report"]
        self.assertEqual(report["permissions"], {"contents": "read", "issues": "write"})
        report_steps = report["steps"]
        preview = next(step for step in report_steps if step.get("name") == "Preview failure tracker update")
        self.assertIn("inputs.dry_run", preview["if"])
        self.assertIn("$GITHUB_STEP_SUMMARY", preview["run"])
        llm_writes = [
            step for step in report_steps
            if step.get("uses") == "./.github/actions/status-flags-and-issues"
        ]
        self.assertEqual(len(llm_writes), 4)
        self.assertTrue(all("inputs.dry_run != true" in step["if"] for step in llm_writes))
        self.assertTrue(llm["jobs"]["run-inference-suite"]["steps"])
        self.assertTrue(llm["jobs"]["run-rag-suite"]["steps"])
        self.assertTrue(llm["jobs"]["run-adalora-suite"]["steps"])

    def test_pr_gate_feedback_permissions_are_scoped_to_mutating_jobs(self) -> None:
        workflow_permissions = {"contents": "read"}
        issue_and_pr_permissions = {
            "contents": "read",
            "issues": "write",
            "pull-requests": "write",
        }

        for name in (
            "gate-pr-doc-metadata.yml",
            "gate-pr-doxygen-governance.yml",
            "gate-pr-primary-doc-structure.yml",
            "gate-pr-plugin-boundary.yml",
            "gate-pr-edition-license.yml",
        ):
            with self.subTest(workflow=name):
                workflow = load_workflow(name)
                self.assertEqual(workflow["permissions"], workflow_permissions)

        for name in ("gate-pr-hash-sbom.yml", "gate-pr-community-failclosed.yml"):
            with self.subTest(workflow=name):
                workflow = load_workflow(name)
                self.assertEqual(workflow["permissions"], workflow_permissions)
                validation_job = next(
                    job for job in workflow["jobs"].values()
                    if job.get("name", "").endswith("Validation")
                )
                self.assertEqual(
                    validation_job["permissions"],
                    {"contents": "read", "pull-requests": "write"},
                )
                self.assertEqual(
                    workflow["jobs"]["human-loop-escalation"]["permissions"],
                    {"contents": "read", "issues": "write"},
                )
                self.assertEqual(
                    workflow["jobs"]["human-loop-resolution"]["permissions"],
                    {"contents": "read", "issues": "write"},
                )

        merge_readiness = load_workflow("gate-pr-merge-readiness.yml")
        self.assertEqual(merge_readiness["permissions"], workflow_permissions)
        expected_reusable_permissions = {
            "contents": "read",
            "pull-requests": "write",
            "issues": "write",
        }
        for caller in (
            "call-community-failclosed",
            "call-edition-license",
            "call-hash-sbom",
            "call-plugin-boundary",
        ):
            with self.subTest(caller=caller):
                self.assertEqual(
                    merge_readiness["jobs"][caller]["permissions"],
                    expected_reusable_permissions,
                )
        self.assertEqual(
            merge_readiness["jobs"]["call-final-runtime-api"]["permissions"],
            {"contents": "read"},
        )
        self.assertEqual(
            merge_readiness["jobs"]["merge-readiness"]["permissions"],
            expected_reusable_permissions,
        )

        version_targeting = load_workflow("gate-pr-version-targeting.yml")
        self.assertEqual(
            version_targeting["permissions"],
            {"contents": "read", "pull-requests": "read"},
        )
        self.assertEqual(
            version_targeting["jobs"]["gate-feedback"]["permissions"],
            {"pull-requests": "write", "issues": "write"},
        )

        for name, validation_job in (
            ("gate-pr-doc-metadata.yml", "doc-metadata-gate"),
            ("gate-pr-doxygen-governance.yml", "doxygen-governance"),
        ):
            with self.subTest(workflow=name, job=validation_job):
                jobs = load_workflow(name)["jobs"]
                self.assertEqual(jobs[validation_job]["permissions"], issue_and_pr_permissions)
                writes = [
                    step for step in jobs[validation_job]["steps"]
                    if step.get("uses") == "./.github/actions/status-flags-and-issues"
                ]
                self.assertTrue(writes)
                self.assertTrue(
                    all("github.event_name == 'pull_request'" in step["if"] for step in writes)
                )

        primary_feedback = load_workflow("gate-pr-primary-doc-structure.yml")["jobs"]["gate-feedback"]
        self.assertEqual(
            primary_feedback["permissions"],
            {"pull-requests": "write", "issues": "write"},
        )
        self.assertIn("github.event_name == 'pull_request'", primary_feedback["if"])

        for name, validation_job in (
            ("gate-pr-plugin-boundary.yml", "private-plugin-boundary-check"),
            ("gate-pr-edition-license.yml", "edition-license-validation"),
        ):
            with self.subTest(workflow=name, job=validation_job):
                jobs = load_workflow(name)["jobs"]
                self.assertEqual(
                    jobs[validation_job]["permissions"],
                    {"contents": "read", "pull-requests": "write"},
                )
                for feedback_job in ("human-loop-escalation", "human-loop-resolution"):
                    self.assertEqual(
                        jobs[feedback_job]["permissions"],
                        {"contents": "read", "issues": "write"},
                    )
                    self.assertIn("github.event_name == 'pull_request'", jobs[feedback_job]["if"])

    def test_rag_test_lanes_report_unimplemented_registrations_and_fail_closed(self) -> None:
        cases = {
            "gate-pr-rag-security.yml": {
                "test_job": "isolation_tests",
                "validation_job": "security_guardrails_validation",
                "build_step": "Build Tenant Policy Tests",
                "targets": ("module_security_test_retrieval_policy_focused",),
                "registration_step": "Verify Multi-Tenant Isolation Test Registration",
                "filter": "tenant_isolation|policy_enforcement",
                "noncritical_labels": ("rag-security-required", "quality/security"),
                "withheld_reason": "tenant-isolation test lane",
            },
            "gate-pr-rag-version.yml": {
                "test_job": "reindex_decision_tests",
                "validation_job": "version_governance_validation",
                "build_step": "Build Reindex and Canary Tests",
                "targets": (
                    "module_ingestion_test_reindex_decision_engine_focused",
                    "module_rag_test_model_promoter_focused",
                ),
                "reindex_registration_step": "Verify Reindex Decision Test Registration",
                "reindex_test": "test_reindex_decision_engine_ingestion_FocusedTests",
                "registration_step": "Verify Embedding Canary Test Registration",
                "filter": "embedding_canary",
                "noncritical_labels": ("rag-version-required", "quality/versioning"),
                "withheld_reason": "reindex/canary test lane",
            },
        }

        for workflow_name, contract in cases.items():
            with self.subTest(workflow=workflow_name):
                workflow = load_workflow(workflow_name)
                test_job = workflow["jobs"][contract["test_job"]]
                build_step = next(
                    step for step in test_job["steps"]
                    if step.get("name") == contract["build_step"]
                )
                build_script = build_step["run"]
                self.assertIn("--target help", build_script)
                for target in contract["targets"]:
                    self.assertIn(target, build_script)
                self.assertIn("UNIMPLEMENTED/BLOCKED", build_script)
                self.assertIn("exit 1", build_script)
                self.assertNotRegex(build_script, r"\|\|\s*\{")

                registration_step = next(
                    step for step in test_job["steps"]
                    if step.get("name") == contract["registration_step"]
                )
                registration_script = registration_step["run"]
                self.assertIn("ctest", registration_script)
                self.assertIn("-N -L", registration_script)
                self.assertIn(contract["filter"], registration_script)
                self.assertIn("UNIMPLEMENTED/BLOCKED", registration_script)
                self.assertIn("exit 1", registration_script)

                test_run = next(
                    step for step in test_job["steps"]
                    if "--no-tests=error" in step.get("run", "")
                )
                self.assertIn("ctest", test_run["run"])
                self.assertIn(f'-L "{contract["filter"]}"', test_run["run"])
                if "reindex_registration_step" in contract:
                    reindex_registration = next(
                        step for step in test_job["steps"]
                        if step.get("name") == contract["reindex_registration_step"]
                    )
                    self.assertIn(contract["reindex_test"], reindex_registration["run"])
                    reindex_run = next(
                        step for step in test_job["steps"]
                        if contract["reindex_test"] in step.get("run", "")
                        and "--no-tests=error" in step.get("run", "")
                    )
                    self.assertIn(contract["reindex_test"], reindex_run["run"])
                gate_status = workflow["jobs"]["gate_status"]
                self.assertIn(contract["test_job"], gate_status["needs"])
                status_script = gate_status["steps"][0]["run"]
                self.assertIn("tests_result", status_script)
                self.assertIn('"success"', status_script)
                self.assertIn('"skipped"', status_script)

                validation_label_step = next(
                    step for step in workflow["jobs"][contract["validation_job"]]["steps"]
                    if step.get("name") == "Apply Gate Label"
                )
                validation_labels = validation_label_step["with"]["script"]
                for label in contract["noncritical_labels"]:
                    self.assertIn(label, validation_labels)
                self.assertNotIn("'release_critical'", validation_labels)

                release_label_job = workflow["jobs"]["apply-release-critical-label"]
                self.assertEqual(
                    set(release_label_job["needs"]),
                    {contract["validation_job"], contract["test_job"], "gate_status"},
                )
                release_label_condition = release_label_job["if"]
                for required_job in (contract["validation_job"], contract["test_job"], "gate_status"):
                    self.assertIn(
                        f"needs.{required_job}.result == 'success'",
                        release_label_condition,
                    )
                release_label_script = release_label_job["steps"][0]["with"]["script"]
                self.assertIn("'release_critical'", release_label_script)

                run_summary = workflow["jobs"]["run-summary"]
                self.assertIn("apply-release-critical-label", run_summary["needs"])
                self.assertEqual(
                    run_summary["steps"][-1]["with"]["job-results"],
                    "${{ toJSON(needs) }}",
                )
                withheld_summary = next(
                    step for step in run_summary["steps"]
                    if step.get("name") == "Report release_critical label decision"
                )
                self.assertIn(
                    f"needs.{contract['test_job']}.result != 'success'",
                    withheld_summary["if"],
                )
                self.assertIn("release_critical was withheld", withheld_summary["run"])
                self.assertIn(contract["withheld_reason"], withheld_summary["run"])

    def test_benchmark_ctest_feedback_permissions_are_pr_only(self) -> None:
        workflow = load_workflow("gate-pr-benchmark-ctest.yml")
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        validation = workflow["jobs"]["benchmark-ctest-coverage"]
        self.assertEqual(validation["permissions"], {"contents": "read"})
        self.assertEqual(
            validation["outputs"]["gate_status"],
            "${{ steps.gate_logic.outputs.gate_status }}",
        )

        feedback = workflow["jobs"]["gate-feedback"]
        self.assertEqual(feedback["permissions"], {
            "issues": "write",
            "pull-requests": "write",
        })
        self.assertIn("github.event_name == 'pull_request'", feedback["if"])
        self.assertEqual(feedback["needs"], ["benchmark-ctest-coverage"])
        self.assertIn("gate-feedback", workflow["jobs"]["run-summary"]["needs"])
        self.assertTrue(
            all(
                step.get("uses") == "actions/github-script@v7"
                for step in feedback["steps"]
            )
        )

    def test_winget_e2e_dry_run_preserves_validation_and_suppresses_submission(self) -> None:
        workflow = load_workflow("build-widget.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("without pushing", dry_run["description"])
        self.assertEqual(workflow["permissions"], {"contents": "read", "actions": "read"})

        jobs = workflow["jobs"]
        resolve_version = next(
            step for step in jobs["download-release-assets"]["steps"]
            if step.get("name") == "Resolve version from tag"
        )
        self.assertEqual(resolve_version["env"]["TAG_INPUT"], "${{ inputs.tag_name }}")
        self.assertIn("unsupported characters", resolve_version["run"])
        self.assertNotIn("${{ inputs.tag_name }}", resolve_version["run"])

        validate = jobs["validate-manifests"]
        winget_validate = next(step for step in validate["steps"] if step.get("name") == "Validate with winget")
        self.assertIn("winget validate --manifest", winget_validate["run"])
        self.assertNotIn("inputs.dry_run", str(validate.get("if", "")))

        submit = jobs["submit-winget-pr"]
        self.assertIn("needs.validate-manifests.outputs.validated == 'true'", submit["if"])
        self.assertIn("inputs.dry_run != true", submit["if"])
        submit_script = next(step for step in submit["steps"] if "run" in step)
        self.assertIn("git push origin \"$BRANCH\" --force-with-lease", submit_script["run"])
        self.assertIn("gh pr create", submit_script["run"])

        report = next(step for step in jobs["notify"]["steps"] if step.get("name") == "Report")
        self.assertIn("$GITHUB_STEP_SUMMARY", report["run"])
        self.assertIn("No fork branch push or upstream pull request was attempted.", report["run"])
        self.assertEqual(report["env"]["SOURCE_WORKFLOW"], "${{ inputs.source_workflow }}")
        self.assertNotIn("${{ inputs.source_workflow }}", report["run"])

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
        self.assertEqual(validate["env"]["VERSION"], "${{ inputs.version }}")
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

    def test_wordpress_manual_dry_run_suppresses_tracker_issue_mutations(self) -> None:
        workflow = load_workflow("release-wordpress-press.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        jobs = workflow["jobs"]
        live_run = "github.event_name == 'release' || (github.event_name == 'workflow_dispatch' && inputs.dry_run != true)"
        self.assertEqual(jobs["mark-release-pending"]["if"], live_run)
        self.assertEqual(
            jobs["finalize-release-status"]["if"],
            f"always() && ({live_run})",
        )
        self.assertEqual(
            jobs["mark-release-pending"]["permissions"],
            {"contents": "read", "issues": "write"},
        )
        self.assertEqual(
            jobs["finalize-release-status"]["permissions"],
            {"contents": "read", "issues": "write"},
        )
        publish = next(
            step for step in jobs["wordpress-press"]["steps"]
            if step.get("name") == "Publish post to WordPress"
        )
        self.assertIn("steps.dry-run.outputs.enabled != 'true'", publish["if"])
        summary = next(
            step for step in jobs["wordpress-press"]["steps"]
            if step.get("name") == "Write workflow summary"
        )
        self.assertIn("GitHub release tracker updates: suppressed", summary["run"])

    def test_winget_dry_run_validates_manifests_without_external_or_issue_writes(self) -> None:
        workflow = load_workflow("release-winget.yml")
        triggers = workflow.get(True, workflow.get("on"))
        for trigger_name in ("workflow_dispatch", "workflow_call"):
            dry_run = triggers[trigger_name]["inputs"]["dry_run"]
            with self.subTest(trigger=trigger_name):
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        self.assertEqual(
            workflow["permissions"],
            {"contents": "read", "packages": "read", "actions": "read"},
        )
        jobs = workflow["jobs"]
        live_run = "github.event_name == 'release' || inputs.dry_run != true"
        self.assertEqual(jobs["mark-release-pending"]["if"], live_run)
        self.assertEqual(
            jobs["finalize-release-status"]["if"],
            f"always() && ({live_run})",
        )
        for job_name in ("mark-release-pending", "finalize-release-status"):
            with self.subTest(job=job_name):
                self.assertEqual(
                    jobs[job_name]["permissions"],
                    {"contents": "read", "issues": "write"},
                )

        manifest_validation = jobs["validate-manifests"]
        self.assertNotIn("dry_run", manifest_validation["if"])
        submit = jobs["create-fork-pr"]
        self.assertIn("inputs.dry_run != true", submit["if"])
        self.assertEqual(submit["permissions"], {"contents": "read"})
        notify = jobs["notify"]["steps"][0]
        self.assertIn("no WinGet PR or GitHub issue update was attempted", notify["run"])
        self.assertIn("$GITHUB_STEP_SUMMARY", notify["run"])
        self.assertIn("Manifest validation:", notify["run"])
        self.assertIn("${VALIDATE_RESULT}", notify["run"])
        self.assertIn("dry-run; submission and issue updates suppressed", jobs["run-summary"]["steps"][1]["with"]["scope"])

        approval = load_workflow("release-winget-approval.yml")
        approval_step = approval["jobs"]["winget_publish_from_issue"]
        self.assertIn('"dry_run":false', approval_step["with"]["dispatch_inputs_json"])

    def test_linux_distro_dry_run_prepares_bundle_without_publish_or_issue_writes(self) -> None:
        workflow = load_workflow("release-linux-distribution.yml")
        triggers = workflow.get(True, workflow.get("on"))
        for trigger_name in ("workflow_dispatch", "workflow_call"):
            dry_run = triggers[trigger_name]["inputs"]["dry_run"]
            with self.subTest(trigger=trigger_name):
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        jobs = workflow["jobs"]
        live_run = "github.event_name == 'release' || ((github.event_name == 'workflow_dispatch' || github.event_name == 'workflow_call') && inputs.dry_run != true)"
        self.assertEqual(jobs["mark-release-pending"]["if"], live_run)
        self.assertEqual(
            jobs["finalize-release-status"]["if"],
            f"always() && ({live_run})",
        )
        detect = jobs["detect-release"]
        detect_step = next(step for step in detect["steps"] if step.get("id") == "detect")
        self.assertEqual(detect_step["env"]["DRY_RUN_INPUT"], "${{ inputs.dry_run }}")
        self.assertIn('DRY_RUN_ENABLED="true"', detect_step["run"])
        self.assertIn("dry_run_enabled", detect["outputs"])

        self.assertEqual(
            jobs["prepare-linux-distro-bundle"]["if"],
            "needs.detect-release.outputs.should_prepare == 'true'",
        )
        publish = jobs["publish-linux-distro-bundle"]
        self.assertEqual(
            publish["if"],
            "needs.detect-release.outputs.publish_requested == 'true' && needs.detect-release.outputs.dry_run_enabled != 'true'",
        )
        summary = jobs["run-summary"]["steps"][1]["with"]["scope"]
        self.assertIn("dry-run; external writes suppressed", summary)
        self.assertIn("needs.detect-release.outputs.publish_requested", summary)

        approval = load_workflow("release-linux-distro-approval.yml")
        approval_step = approval["jobs"]["linux_distro_publish_from_issue"]
        self.assertIn('"dry_run":false', approval_step["with"]["dispatch_inputs_json"])

    def test_windows_distro_dry_run_prepares_bundle_without_publish_or_issue_writes(self) -> None:
        workflow = load_workflow("release-windows-distribution.yml")
        triggers = workflow.get(True, workflow.get("on"))
        for trigger_name in ("workflow_dispatch", "workflow_call"):
            dry_run = triggers[trigger_name]["inputs"]["dry_run"]
            with self.subTest(trigger=trigger_name):
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        jobs = workflow["jobs"]
        live_run = "github.event_name == 'release' || ((github.event_name == 'workflow_dispatch' || github.event_name == 'workflow_call') && inputs.dry_run != true)"
        self.assertEqual(jobs["mark-release-pending"]["if"], live_run)
        self.assertEqual(
            jobs["finalize-release-status"]["if"],
            f"always() && ({live_run})",
        )
        detect = jobs["detect-release"]
        detect_step = next(step for step in detect["steps"] if step.get("id") == "detect")
        self.assertEqual(detect_step["env"]["DRY_RUN_INPUT"], "${{ inputs.dry_run }}")
        self.assertIn('DRY_RUN_ENABLED="true"', detect_step["run"])
        self.assertIn("dry_run_enabled", detect["outputs"])

        self.assertEqual(
            jobs["prepare-windows-distro-bundle"]["if"],
            "always() && needs.detect-release.result != 'failure'",
        )
        publish = jobs["publish-windows-distro-bundle"]
        self.assertIn("needs.detect-release.outputs.publish_requested == 'true'", publish["if"])
        self.assertIn("needs.detect-release.outputs.dry_run_enabled != 'true'", publish["if"])
        summary = jobs["run-summary"]["steps"][1]["with"]["scope"]
        self.assertIn("dry-run; external writes suppressed", summary)
        self.assertIn("needs.detect-release.outputs.publish_requested", summary)

        approval = load_workflow("release-windows-distro-approval.yml")
        approval_step = approval["jobs"]["windows_distro_publish_from_issue"]
        self.assertIn('"dry_run":false', approval_step["with"]["dispatch_inputs_json"])

    def test_release_approval_permissions_are_scoped_to_mutating_jobs(self) -> None:
        wrapper_jobs = {
            "release-linux-distro-approval.yml": "linux_distro_publish_from_issue",
            "release-windows-distro-approval.yml": "windows_distro_publish_from_issue",
            "release-winget-approval.yml": "winget_publish_from_issue",
        }
        reusable_permissions = {
            "contents": "read",
            "issues": "write",
            "actions": "write",
        }

        for workflow_name, approval_job_name in wrapper_jobs.items():
            with self.subTest(workflow=workflow_name):
                workflow = load_workflow(workflow_name)
                self.assertEqual(workflow["permissions"], {"contents": "read"})
                approval_job = workflow["jobs"][approval_job_name]
                self.assertEqual(approval_job["permissions"], reusable_permissions)
                self.assertEqual(workflow["jobs"]["summary"]["permissions"], {"contents": "read"})
                self.assertIn('"dry_run":false', approval_job["with"]["dispatch_inputs_json"])

        reusable = load_workflow("reusable-release-approval-comment-gate.yml")
        self.assertEqual(reusable["permissions"], {"contents": "read"})
        self.assertEqual(reusable["jobs"]["approval-gate"]["permissions"], reusable_permissions)
        self.assertEqual(
            reusable["jobs"]["human-loop-escalation"]["permissions"],
            {"contents": "read", "issues": "write"},
        )

    def test_mainline_and_docker_approval_permissions_are_scoped(self) -> None:
        mainline = load_workflow("release-mainline-approval.yml")
        self.assertEqual(mainline["permissions"], {"contents": "read"})
        self.assertEqual(
            mainline["jobs"]["release_publish_from_issue"]["permissions"],
            {"contents": "write", "issues": "write"},
        )
        self.assertEqual(mainline["jobs"]["run-summary"]["permissions"], {"contents": "read"})

        docker = load_workflow("release-docker-approval.yml")
        self.assertEqual(docker["permissions"], {"contents": "read"})
        self.assertEqual(
            docker["jobs"]["docker_publish_from_issue"]["permissions"],
            {"contents": "read", "issues": "write", "actions": "write"},
        )
        self.assertEqual(docker["jobs"]["summary"]["permissions"], {"contents": "read"})
        dispatch_inputs = docker["jobs"]["docker_publish_from_issue"]["with"]["dispatch_inputs_json"]
        self.assertIn('"push_to_registry":"true"', dispatch_inputs)
        self.assertIn('"dry_run":false', dispatch_inputs)

    def test_reusable_release_approval_rejects_bad_dispatch_json_and_extracts_prerelease(self) -> None:
        reusable = load_workflow("reusable-release-approval-comment-gate.yml")
        approval_steps = reusable["jobs"]["approval-gate"]["steps"]
        extract = next(step for step in approval_steps if step.get("id") == "extract_version")
        dispatch = next(step for step in approval_steps if step.get("name") == "Dispatch target workflow")

        self.assertIn("Invalid dispatch_inputs_json; expected a JSON object.", dispatch["with"]["script"])
        self.assertIn("Array.isArray(extraInputs)", dispatch["with"]["script"])
        version_pattern = re.search(r"const match = body\.match\((/[^;\n]+/)\);", extract["with"]["script"])
        self.assertIsNotNone(version_pattern)
        node_script = (
            "const pattern = " + version_pattern.group(1) + ";\n"
            "const cases = ["
            "['## Release Version: v2.4.0', '2.4.0'],"
            "['## Release Version: v2.4.0-beta1', '2.4.0-beta1'],"
            "['## Release Version: 2.4.0-rc.1', '2.4.0-rc.1'],"
            "['## Release Version: v2.4.0invalid', null]"
            "];\n"
            "for (const [body, expected] of cases) {"
            "const match = body.match(pattern);"
            "if ((match ? match[1] : null) !== expected) process.exit(1);"
            "}"
        )
        result = subprocess.run(["node", "-e", node_script], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_release_evidence_dry_run_verifies_ledger_without_manual_artifact_upload(self) -> None:
        workflow = load_workflow("release-evidence-chain.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)

        steps = workflow["jobs"]["capture-evidence"]["steps"]
        generate = next(step for step in steps if step.get("name") == "Append evidence record to ledger")
        verify = next(step for step in steps if step.get("name") == "Verify full ledger chain")
        upload = next(step for step in steps if step.get("name") == "Upload ledger artifacts")
        summary = next(step for step in steps if step.get("name") == "Publish evidence summary")["run"]
        self.assertNotIn("if", generate)
        self.assertNotIn("if", verify)
        self.assertEqual(
            upload["if"],
            "always() && (github.event_name != 'workflow_dispatch' || inputs.dry_run != true)",
        )
        self.assertIn("artifact upload suppressed", summary)

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
        jobs = workflow["jobs"]
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        generation = jobs["generate-architecture"]
        self.assertEqual(generation["permissions"], {"contents": "read"})
        self.assertEqual(
            generation["outputs"]["changed"],
            "${{ steps.diff.outputs.changed }}",
        )
        steps = generation["steps"]
        checkout = next(step for step in steps if step.get("name") == "Checkout repository")
        self.assertIs(checkout["with"]["persist-credentials"], False)
        generate = next(step for step in steps if step.get("id") == "generate")
        validate = next(step for step in steps if step.get("id") == "validation")
        self.assertNotIn("--dry-run", generate["run"])
        self.assertNotIn("if", validate)
        self.assertNotIn("git push", "\n".join(step.get("run", "") for step in steps))
        artifact = next(step for step in steps if step.get("name") == "Upload validated architecture outputs")
        self.assertEqual(artifact["uses"], "actions/upload-artifact@v5")

        publish = jobs["publish-architecture"]
        self.assertEqual(
            publish["permissions"],
            {"contents": "write", "pull-requests": "write", "issues": "write"},
        )
        self.assertIn("needs.generate-architecture.outputs.changed == 'true'", publish["if"])
        self.assertIn("github.event_name != 'workflow_dispatch'", publish["if"])
        self.assertIn("inputs.dry_run != true", publish["if"])
        self.assertEqual(
            set(publish["needs"]),
            {"generate-architecture"},
        )
        publish_checkout = next(step for step in publish["steps"] if step.get("name") == "Checkout source revision")
        self.assertEqual(publish_checkout["with"]["ref"], "${{ github.sha }}")
        self.assertNotIn("persist-credentials", publish_checkout["with"])
        download = next(step for step in publish["steps"] if step.get("name") == "Download validated architecture outputs")
        self.assertEqual(download["with"]["name"], "architecture-outputs-${{ github.run_id }}")
        self.assertIn("git push origin", next(step for step in publish["steps"] if step.get("id") == "branch")["run"])
        self.assertIn("github.event_name == 'workflow_dispatch' && inputs.dry_run", steps[-1]["env"]["DRY_RUN"])
        self.assertEqual(set(jobs["run-summary"]["needs"]), {"generate-architecture", "publish-architecture"})

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
        workflow = load_workflow("maintenance-housekeeping.yml")
        self.assertEqual(workflow["permissions"], {})
        expected_job_permissions = {
            "run-summary": {"contents": "read"},
            "sync-labels": {"contents": "read", "issues": "write"},
            "sync-milestones": {"contents": "read", "issues": "write"},
            "assign-milestone": {"contents": "read", "issues": "write"},
            "recommend-closures": {"issues": "write", "pull-requests": "read"},
        }
        for job_name, permissions in expected_job_permissions.items():
            with self.subTest(job=job_name):
                self.assertEqual(workflow["jobs"][job_name]["permissions"], permissions)
        workflow_text = (WORKFLOWS / "maintenance-housekeeping.yml").read_text(encoding="utf-8")
        self.assertIn("if (dryRun)", workflow_text)
        self.assertIn("[dry-run] would post closure recommendation", workflow_text)

    def test_housekeeping_pull_request_target_uses_trusted_base_context(self) -> None:
        workflow = load_workflow("maintenance-housekeeping.yml")
        triggers = workflow.get(True, workflow.get("on"))
        pull_request = triggers["pull_request"]
        pull_request_target = triggers["pull_request_target"]

        for key in ("branches", "paths", "types"):
            with self.subTest(trigger_setting=key):
                self.assertEqual(pull_request_target[key], pull_request[key])

        base_ref = "${{ github.event.pull_request.base.sha || github.sha }}"
        for job_name in ("run-summary", "assign-milestone"):
            with self.subTest(job=job_name):
                checkout = next(
                    step for step in workflow["jobs"][job_name]["steps"]
                    if step.get("uses") == "actions/checkout@v5"
                )
                self.assertEqual(checkout["with"]["ref"], base_ref)
                self.assertIs(checkout["with"]["persist-credentials"], False)

        self.assertIn(
            "github.event.pull_request.number || github.ref",
            workflow["concurrency"]["group"],
        )
        self.assertNotIn("github.event.pull_request.head", str(workflow["jobs"]))
        self.assertNotIn("github.head_ref", str(workflow["jobs"]))

    def test_compendium_sync_uses_safe_version_resolution_and_fail_closed_scan(self) -> None:
        workflow = load_workflow("maintenance-compendium-sync.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dispatch = triggers["workflow_dispatch"]
        self.assertIs(dispatch["inputs"]["dry_run"]["default"], True)
        self.assertEqual(dispatch["inputs"]["target_version"]["default"], "")
        self.assertIn("CHAPTER_ROADMAP_MAPPING.yml", dispatch["inputs"]["target_version"]["description"])
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        jobs = workflow["jobs"]
        scan_job = jobs["drift-scan"]
        self.assertEqual(scan_job["permissions"], {"contents": "read"})
        self.assertEqual(
            scan_job["outputs"]["target_version"],
            "${{ steps.scan.outputs.target_version }}",
        )
        params = next(step for step in scan_job["steps"] if step.get("id") == "params")
        self.assertEqual(params["env"]["TARGET_VERSION_OVERRIDE"], "${{ inputs.target_version }}")
        self.assertIn("CHAPTER_ROADMAP_MAPPING.yml", params["run"])
        self.assertIn("re.fullmatch", params["run"])

        scan = next(step for step in scan_job["steps"] if step.get("id") == "scan")
        self.assertNotIn("|| true", scan["run"])
        self.assertIn("scan_status", scan["run"])
        self.assertIn("total_issues", scan["run"])
        self.assertIn("TARGET_VERSION", scan["env"])

        for job_name in ("open-tracker-issue", "no-drift-close-issue"):
            with self.subTest(job=job_name):
                job = jobs[job_name]
                self.assertEqual(job["permissions"], {"contents": "read", "issues": "write"})
                self.assertIn("github.event_name != 'workflow_dispatch'", job["if"])
                self.assertIn("inputs.dry_run != true", job["if"])
                self.assertIn("${{ needs.drift-scan.outputs.target_version }}", job["steps"][-1]["with"]["issue-marker"])
        execution_summary = next(
            step for step in scan_job["steps"] if step.get("name") == "Write scan execution summary"
        )
        self.assertIn("tracker issue writes were suppressed", execution_summary["run"])

    def test_manual_pr_failure_diagnosis_dry_run_previews_without_comment_writes(self) -> None:
        workflow = load_workflow("maintenance-pr-failure-diagnosis.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)

        steps = workflow["jobs"]["diagnose-pr-failure"]["steps"]
        diagnosis = next(step for step in steps if step.get("name") == "Generate and publish recommend-only diagnosis comment")
        script = diagnosis["with"]["script"]
        self.assertEqual(diagnosis["env"]["DRY_RUN"], "${{ inputs.dry_run }}")
        self.assertIn("context.eventName === 'workflow_dispatch' && process.env.DRY_RUN === 'true'", script)
        self.assertIn("plannedActions.push", script)
        self.assertIn("if (dryRun)", script)
        self.assertIn("comment writes: suppressed", script)
        self.assertEqual(triggers["workflow_run"]["types"], ["completed"])

    def test_ci_health_manual_dry_run_preserves_analysis_and_suppresses_issue_writes(self) -> None:
        workflow = load_workflow("maintenance-ci-health.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dispatch = triggers["workflow_dispatch"]
        self.assertEqual(dispatch["inputs"]["lookback_days"]["type"], "number")
        self.assertEqual(dispatch["inputs"]["lookback_days"]["default"], 7)
        dry_run = dispatch["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled runs remain live", dry_run["description"].lower())

        jobs = workflow["jobs"]
        analysis = jobs["ci-health-dashboard"]
        self.assertEqual(analysis["permissions"], {"contents": "read", "actions": "read"})
        self.assertIn("lookback_days", analysis["outputs"])
        stats = next(step for step in analysis["steps"] if step.get("id") == "stats")
        self.assertEqual(stats["env"]["LOOKBACK_DAYS"], "${{ inputs.lookback_days || 7 }}")
        self.assertIn("Number.isInteger(lookbackDays)", stats["with"]["script"])
        self.assertIn("lookbackDays > 90", stats["with"]["script"])
        self.assertNotIn("${{ inputs.lookback_days", stats["with"]["script"])
        self.assertIn("metrics and artifacts were still generated", stats["with"]["script"])
        self.assertIn("would be upserted", stats["with"]["script"])
        self.assertLess(
            stats["with"]["script"].index("const chronic ="),
            stats["with"]["script"].index("md.push(chronic.length > 0"),
        )

        issue_job = jobs["update-chronic-issue"]
        self.assertEqual(issue_job["permissions"], {"contents": "read", "issues": "write"})
        self.assertIn("github.event_name != 'workflow_dispatch'", issue_job["if"])
        self.assertIn("inputs.dry_run != true", issue_job["if"])
        self.assertIn("needs.ci-health-dashboard.result == 'success'", issue_job["if"])
        issue_steps = [step for step in issue_job["steps"] if step.get("uses") == "./.github/actions/status-flags-and-issues"]
        self.assertEqual(
            {step["with"]["operation"] for step in issue_steps},
            {"upsert_issue", "replace_status_group", "comment_issue"},
        )
        self.assertIn("${LOOKBACK_DAYS} days", issue_job["steps"][1]["run"])

    def test_build_error_manual_dry_run_suppresses_mutations_and_preserves_analysis(self) -> None:
        workflow = load_workflow("maintenance-build-issues.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)

        steps = workflow["jobs"]["aggregate-build-errors"]["steps"]
        write_steps = (
            "Ensure single chronic build failure issue",
            "Normalize chronic build issue labels",
            "Append latest chronic build errors as issue comment",
            "Send Slack notification for chronic errors",
            "Send Discord notification for chronic errors",
            "Publish Error Dashboard",
        )
        dry_run_guard = "github.event_name != 'workflow_dispatch' || inputs.dry_run != true"
        for name in write_steps:
            step = next(step for step in steps if step.get("name") == name)
            with self.subTest(step=name):
                self.assertIn(dry_run_guard, step["if"])

        pr_comment = next(step for step in steps if step.get("name") == "Post build failure comment on triggering PR")
        self.assertIn("github.event_name == 'workflow_run'", pr_comment["if"])
        self.assertIn("github.event.workflow_run.conclusion == 'failure'", pr_comment["if"])

        fetch = next(step for step in steps if step.get("name") == "Fetch failed workflow runs and error artifacts")
        aggregate = next(step for step in steps if step.get("name") == "Aggregate errors from artifacts")
        metrics = next(step for step in steps if step.get("name") == "Collect error metrics for Prometheus/Grafana")
        metrics_upload = next(step for step in steps if step.get("name") == "Upload metrics artifacts")
        self.assertNotIn("if", fetch)
        self.assertNotIn("if", aggregate)
        self.assertEqual(metrics["if"], "always()")
        self.assertEqual(metrics_upload["if"], "always()")
        comment_preview = next(step for step in steps if step.get("name") == "Prepare chronic build issue comment body")
        self.assertEqual(comment_preview["if"], "steps.aggregate.outputs.chronic_errors > 0")

        summary = next(step for step in steps if step.get("name") == "Print error aggregation summary")
        self.assertEqual(summary["env"]["DRY_RUN_ENABLED"], "${{ github.event_name == 'workflow_dispatch' && inputs.dry_run }}")
        self.assertIn("Build Error Dry-Run Preview", summary["run"])
        self.assertIn("Planned action:", summary["run"])
        run_summary_scope = workflow["jobs"]["run-summary"]["steps"][1]["with"]["scope"]
        self.assertIn("manual dry-run", run_summary_scope)

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

    def test_issue_and_wiki_publish_write_permissions_are_job_scoped(self) -> None:
        issue_sync = load_workflow("maintenance-soll-ist-gap-issues.yml")
        self.assertEqual(issue_sync["permissions"], {"contents": "read"})
        self.assertEqual(
            issue_sync["jobs"]["soll_ist_sync"]["permissions"],
            {"contents": "read", "issues": "write"},
        )
        self.assertEqual(issue_sync["jobs"]["run-summary"]["permissions"], {"contents": "read"})

        wiki_publish = load_workflow("wiki-publish-from-issue.yml")
        self.assertEqual(wiki_publish["permissions"], {"contents": "read"})
        self.assertEqual(
            wiki_publish["jobs"]["wiki_publish_from_issue"]["permissions"],
            {"contents": "write", "issues": "write"},
        )
        self.assertEqual(wiki_publish["jobs"]["run-summary"]["permissions"], {"contents": "read"})

    def test_maintenance_issue_triage_dry_run_preserves_analysis_and_scheduled_writes(self) -> None:
        workflow = load_workflow("maintenance-issues.yml")
        triggers = workflow.get(True, workflow.get("on"))
        self.assertEqual(
            [entry["cron"] for entry in triggers["schedule"]],
            ["0 4 * * 2", "0 4 * * 4"],
        )
        jobs = workflow["jobs"]
        self.assertIn("github.event.schedule == '0 4 * * 2'", jobs["gs3-gap-triage"]["if"])
        self.assertIn("github.event.schedule == '0 4 * * 2'", jobs["doxygen-header-update"]["if"])
        self.assertIn("github.event.schedule == '0 4 * * 4'", jobs["security-alert-triage"]["if"])
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled runs remain live", dry_run["description"].lower())

        mutation_guard = "github.event_name != 'workflow_dispatch' || inputs.dry_run != true"
        gs3 = jobs["gs3-gap-triage"]["steps"]
        gs3_write_steps = {
            "Upsert consolidated GS3 issue",
            "Normalize GS3 labels when open",
            "Post GS3 recovery update when clean",
            "Normalize GS3 labels on recovery",
            "Upsert scanner failure issue",
            "Normalize scanner runtime failure labels",
            "Post scanner recovery update",
        }
        for step in gs3:
            if step.get("name") in gs3_write_steps:
                with self.subTest(job="gs3", step=step["name"]):
                    self.assertIn(mutation_guard, step["if"])
        self.assertIn(
            "Preview GS3 tracker changes (manual dry-run)",
            {step.get("name") for step in gs3},
        )
        gs3_preview = next(step for step in gs3 if step.get("name") == "Preview GS3 tracker changes (manual dry-run)")
        self.assertIn("inputs.dry_run == true", gs3_preview["if"])

        security = jobs["security-alert-triage"]["steps"]
        security_write_steps = {
            "Open/update SLA issue for high/critical alerts",
            "Normalize security label group",
            "Post security recovery update",
            "Normalize security labels on recovery",
        }
        for step in security:
            if step.get("name") in security_write_steps:
                with self.subTest(job="security", step=step["name"]):
                    self.assertIn(mutation_guard, step["if"])
                    if "recovery" in step["name"].lower():
                        self.assertIn("steps.query.outcome == 'success'", step["if"])
        query = next(step for step in security if step.get("name") == "Query code scanning alerts for SLA breaches")
        self.assertEqual(query["env"]["SLA_DAYS"], "${{ env.SLA_DAYS }}")
        self.assertIn("process.env.SLA_DAYS", query["with"]["script"])
        self.assertNotIn("${{ inputs.sla_days", query["with"]["script"])
        self.assertIn("Job summary", {step.get("name") for step in security})

        doxygen = jobs["doxygen-header-update"]["steps"]
        for name in ("Configure git for PR commit", "Create PR branch and open PR"):
            step = next(step for step in doxygen if step.get("name") == name)
            with self.subTest(job="doxygen", step=name):
                self.assertIn(mutation_guard, step["if"])
        preview = next(step for step in doxygen if step.get("name") == "Preview Doxygen PR publication (manual dry-run)")
        self.assertIn("inputs.dry_run == true", preview["if"])
        self.assertIn("No branch was pushed", preview["run"])
        checkout = next(step for step in doxygen if step.get("name") == "Checkout repository")
        self.assertIn("persist-credentials", checkout["with"])

    def test_maintenance_docs_dry_run_preserves_checks_and_suppresses_mutations(self) -> None:
        workflow = load_workflow("maintenance-docs.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled and push runs remain live", dry_run["description"].lower())
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        mutation_guard = "github.event_name != 'workflow_dispatch' || inputs.dry_run != true"
        issue_action = "./.github/actions/status-flags-and-issues"
        issue_steps = [
            (job_name, step)
            for job_name, job in workflow["jobs"].items()
            for step in job.get("steps", [])
            if step.get("uses") == issue_action
        ]
        self.assertGreaterEqual(len(issue_steps), 10)
        for job_name, step in issue_steps:
            with self.subTest(job=job_name, step=step.get("name")):
                self.assertIn(mutation_guard, step["if"])

        jobs = workflow["jobs"]
        for job_name in ("ai-context-sync", "developer-llm-wiki"):
            run = next(step for step in jobs[job_name]["steps"] if step.get("name") in (
                "Run AI context lint",
                "Run developer LLM wiki sync/check",
            ))["run"]
            with self.subTest(job=job_name):
                self.assertIn("inputs.dry_run", run)
                self.assertIn("--apply-updates", run)

        commit_job = jobs["code-maturity-commit"]
        self.assertIn("inputs.dry_run != true", commit_job["if"])

        seed_job = jobs["docs-audit-issue-seed"]
        seed = next(step for step in seed_job["steps"] if step.get("name") == "Validate: Seed docs-audit issues")
        self.assertIn('inputs.dry_run', seed["run"])
        self.assertIn("--create", seed["run"])
        self.assertIn("GITHUB_STEP_SUMMARY", seed["run"])

        module_seed = next(
            step for step in jobs["roadmap-status-drift"]["steps"]
            if step.get("name") == "Validate: Create module-wise roadmap drift issues"
        )
        self.assertIn("--dry-run", module_seed["run"])

        doxygen_job = jobs["doxygen-api-extraction"]
        commit = next(step for step in doxygen_job["steps"] if step.get("name") == "Commit API reference artifacts")
        self.assertIn(mutation_guard, commit["if"])
        self.assertIn("Manual dry-run", next(step for step in doxygen_job["steps"] if step.get("name") == "Write job summary")["run"])
        self.assertIn("persist-credentials", next(step for step in doxygen_job["steps"] if step.get("name") == "Checkout repository")["with"])

    def test_maintenance_docs_tracker_mutations_require_successful_scan_outputs(self) -> None:
        jobs = load_workflow("maintenance-docs.yml")["jobs"]
        expected_output_guards = {
            ("developer-llm-wiki", "Open tracker issue for wiki drift findings"): (
                "steps.wiki_drift.outputs.findings_count != ''",
            ),
            ("docs-orphan-check", "Open tracker issue for orphan findings"): (
                "steps.orphan_scan.outputs.orphan_module_dirs != ''",
                "steps.orphan_scan.outputs.broken_primary_references != ''",
            ),
            ("docs-orphan-check", "Validate: Mark orphan tracker issue resolved when clean"): (
                "steps.orphan_scan.outputs.orphan_module_dirs != ''",
                "steps.orphan_scan.outputs.broken_primary_references != ''",
            ),
            ("root-docs-hygiene", "Open tracker issue for hygiene findings"): (
                "steps.root_hygiene.outputs.findings != ''",
            ),
            ("src-docs-alignment", "Validate: Build module finding comment payload"): (
                "steps.align_scan.outputs.undocumented_modules != ''",
            ),
            ("src-docs-alignment", "Open tracker issue for alignment findings"): (
                "steps.align_scan.outputs.undocumented_modules != ''",
            ),
            ("src-docs-alignment", "Validate: Comment module findings on existing alignment issue"): (
                "steps.align_scan.outputs.undocumented_modules != ''",
                "steps.align_tracker.outputs.issue-number != ''",
            ),
            ("src-docs-alignment", "Validate: Mark src/include alignment tracker issue resolved when clean"): (
                "steps.align_scan.outputs.undocumented_modules != ''",
            ),
            ("roadmap-status-drift", "Open tracker issue for ROADMAP drift"): (
                "steps.roadmap_drift.outputs.suspected_items != ''",
            ),
            ("docs-reconciliation", "Validate: Open tracker issue for reconciliation findings"): (
                "steps.reconcile.outputs.out_json != ''",
                "steps.reconcile.outputs.attention_count != ''",
            ),
            ("docs-reconciliation", "Validate: Mark reconciliation tracker issue resolved when clean"): (
                "steps.reconcile.outputs.out_json != ''",
                "steps.reconcile.outputs.attention_count != ''",
            ),
            ("ai-working-hygiene", "Open tracker issue for ai_working stale + ai_context orphan findings"): (
                "steps.hygiene.outputs.stale_count != ''",
                "steps.hygiene.outputs.orphan_ctx_count != ''",
            ),
        }
        for (job_name, step_name), expected_guards in expected_output_guards.items():
            step = next(
                item for item in jobs[job_name]["steps"]
                if item.get("name") == step_name
            )
            with self.subTest(job=job_name, step=step_name):
                for guard in expected_guards:
                    self.assertIn(guard, step["if"])

        reconciliation = next(
            step for step in jobs["docs-reconciliation"]["steps"]
            if step.get("name") == "Validate: Run issue/PR/docs reconciliation"
        )
        self.assertIn("set -o pipefail", reconciliation["run"])

    def test_nightly_coverage_manual_dry_run_preserves_validation_and_suppresses_baseline_push(self) -> None:
        workflow = load_workflow("coverage-report-nightly.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dispatch_inputs = triggers["workflow_dispatch"]["inputs"]
        dry_run = dispatch_inputs["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled runs remain live", dry_run["description"])
        self.assertEqual(workflow["permissions"], {"contents": "read"})

        job = workflow["jobs"]["coverage-collection"]
        self.assertEqual(job["permissions"], {"contents": "write"})
        steps = job["steps"]
        baseline_update = next(step for step in steps if step.get("name") == "Update baseline on develop")
        self.assertIn("github.event_name == 'schedule'", baseline_update["if"])
        self.assertIn("github.event.inputs.generate_baseline == 'true'", baseline_update["if"])
        self.assertIn("github.event.inputs.dry_run != 'true'", baseline_update["if"])
        self.assertIn("steps.focused_tests.outcome == 'success'", baseline_update["if"])
        self.assertIn("steps.critical_tests.outcome == 'success'", baseline_update["if"])
        self.assertIn("git push", baseline_update["run"])
        test_failure_gate = next(step for step in steps if step.get("name") == "Fail workflow when required tests fail")
        self.assertIn("steps.focused_tests.outcome == 'failure'", test_failure_gate["if"])
        self.assertIn("steps.critical_tests.outcome == 'failure'", test_failure_gate["if"])
        self.assertIn("exit 1", test_failure_gate["run"])

        for step_name in ("CMake configure (with coverage enabled)", "Build (with coverage)",
                          "Run focused tests (Tier A priority)", "Run release-critical tests (Tier A subset)"):
            step = next(step for step in steps if step.get("name") == step_name)
            with self.subTest(step=step_name):
                self.assertNotIn("dry_run", step.get("if", ""))

        artifact_upload = next(step for step in steps if step.get("name") == "Upload coverage artifacts")
        self.assertNotIn("dry_run", artifact_upload.get("if", ""))
        summary = next(step for step in steps if step.get("name") == "Workflow summary")
        self.assertEqual(summary["if"], "always()")
        self.assertIn("no commit or push was performed", summary["run"])
        self.assertIn("BASELINE_UPDATE_OUTCOME", summary["env"])

    def test_benchmark_workflow_block_limits_manual_and_workflow_write_scopes(self) -> None:
        performance = load_workflow("benchmark-performance-gate.yml")
        performance_triggers = performance.get(True, performance.get("on"))
        dispatch_inputs = performance_triggers["workflow_dispatch"]["inputs"]
        self.assertEqual(dispatch_inputs["dry_run"]["type"], "boolean")
        self.assertIs(dispatch_inputs["dry_run"]["default"], True)
        self.assertIs(dispatch_inputs["force_create_issue"]["default"], False)
        self.assertEqual(performance["permissions"], {"contents": "read"})

        report = performance["jobs"]["report"]
        self.assertEqual(report["permissions"], {"contents": "read", "issues": "write"})
        steps = report["steps"]
        upsert = next(step for step in steps if step.get("name") == "Upsert benchmark results tracker issue")
        comment = next(step for step in steps if step.get("name") == "Post full comparison report as issue comment")
        for step in (upsert, comment):
            with self.subTest(step=step["name"]):
                condition = step["if"]
                guarded = (
                    "!(github.event_name == 'workflow_dispatch' && inputs.dry_run == true)" in condition
                    or "github.event_name != 'workflow_dispatch' || inputs.dry_run != true" in condition
                )
                self.assertTrue(guarded, condition)
        mode_summary = next(step for step in steps if step.get("name") == "Summarize tracker write mode")
        self.assertEqual(mode_summary["if"], "always()")
        self.assertIn("Manual dry run", mode_summary["run"])
        self.assertIn("suppressed", mode_summary["run"])
        self.assertIn("comparison, report generation, and artifact uploads still run", mode_summary["run"])
        for name in ("Verify artifact presence", "Compare results against PERFORMANCE_EXPECTATIONS.md",
                     "Upload comparison report artifact"):
            self.assertTrue(any(step.get("name") == name for step in steps), name)

        benchmarks = load_workflow("build-benchmarks.yml")
        self.assertEqual(benchmarks["permissions"], {"contents": "read"})
        self.assertNotIn("git push", (WORKFLOWS / "build-benchmarks.yml").read_text(encoding="utf-8"))
        preflight = benchmarks["jobs"]["preflight-build-status"]
        self.assertEqual(preflight["permissions"], {"contents": "read", "issues": "write"})
        bootstrap = next(step for step in preflight["steps"] if step.get("name") == "Bootstrap build tracker issue (schedule gate)")
        self.assertEqual(bootstrap["if"], "github.event_name == 'schedule'")

        gpu = load_workflow("build-wave-a-gpu.yml")
        self.assertEqual(gpu["permissions"], {"contents": "read"})

        transaction = load_workflow("build-wave-b-transaction.yml")
        self.assertEqual(transaction["permissions"], {"contents": "read"})
        transaction_job = transaction["jobs"]["full-transaction-test-suite"]
        self.assertEqual(transaction_job["permissions"], {"contents": "read", "pull-requests": "write"})
        pr_comment = next(step for step in transaction_job["steps"] if step.get("name") == "Post test summary to PR")
        self.assertEqual(pr_comment["if"], "always() && github.event_name == 'pull_request'")

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

    def test_security_scanner_dry_run_defaults_and_upload_policy(self) -> None:
        dispatch_workflows = (
            "crda.yml",
            "security-codeql.yml",
            "security-consolidated.yml",
            "security-dast-zap.yml",
            "security-fortify.yml",
        )
        reusable_workflows = ("crda.yml", "security-dast-zap.yml")

        for workflow_name in dispatch_workflows:
            workflow = load_workflow(workflow_name)
            triggers = workflow.get(True, workflow.get("on"))
            dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
            with self.subTest(workflow=workflow_name, trigger="workflow_dispatch"):
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        for workflow_name in reusable_workflows:
            workflow = load_workflow(workflow_name)
            triggers = workflow.get(True, workflow.get("on"))
            dry_run = triggers["workflow_call"]["inputs"]["dry_run"]
            with self.subTest(workflow=workflow_name, trigger="workflow_call"):
                self.assertEqual(dry_run["type"], "boolean")
                self.assertIs(dry_run["default"], True)

        codeql = load_workflow("security-codeql.yml")
        self.assertIn("schedule", codeql.get(True, codeql.get("on")))
        self.assertEqual(codeql["permissions"]["security-events"], "write")
        expected_upload = (
            "${{ github.event_name == 'workflow_dispatch' && inputs.dry_run == true "
            "&& 'never' || 'always' }}"
        )
        expected_database_upload = (
            "${{ github.event_name != 'workflow_dispatch' || inputs.dry_run != true }}"
        )
        manual_dry_run = (
            "always() && github.event_name == 'workflow_dispatch' && inputs.dry_run == true"
        )
        for job_name in ("analyze", "analyze-csharp", "analyze-rust"):
            with self.subTest(codeql_job=job_name):
                steps = codeql["jobs"][job_name]["steps"]
                analyze = next(step for step in steps if "codeql-action/analyze@" in step.get("uses", ""))
                self.assertEqual(analyze["with"]["upload"], expected_upload)
                self.assertEqual(analyze["with"]["upload-database"], expected_database_upload)
                artifact = next(
                    step for step in steps
                    if step.get("name", "").startswith("Upload CodeQL ")
                )
                self.assertEqual(artifact["if"], manual_dry_run)
                self.assertEqual(artifact["uses"], "actions/upload-artifact@v5")
                self.assertEqual(artifact["with"]["path"], analyze["with"]["output"])
                self.assertEqual(artifact["with"]["if-no-files-found"], "warn")
                self.assertEqual(artifact["with"]["retention-days"], 30)

        consolidated = load_workflow("security-consolidated.yml")
        consolidated_triggers = consolidated.get(True, consolidated.get("on"))
        self.assertIn("schedule", consolidated_triggers)
        self.assertNotIn("pull_request", consolidated_triggers)
        for job_name, report_path, sarif_name in (
            ("trivy", "trivy-results.sarif", "Upload Trivy SARIF artifact for manual dry-run"),
            ("gitleaks", "gitleaks.sarif", "Upload Gitleaks SARIF artifact for manual dry-run"),
        ):
            with self.subTest(consolidated_job=job_name):
                job = consolidated["jobs"][job_name]
                self.assertIn("github.event_name == 'schedule'", job["if"])
                artifact = next(step for step in job["steps"] if step.get("name") == sarif_name)
                self.assertEqual(artifact["if"], manual_dry_run)
                self.assertEqual(artifact["with"]["path"], report_path)
                self.assertEqual(artifact["with"]["retention-days"], 30)
                upload = next(
                    step for step in job["steps"]
                    if "codeql-action/upload-sarif@" in step.get("uses", "")
                )
                self.assertIn("github.event_name != 'workflow_dispatch'", upload["if"])
                self.assertIn("inputs.dry_run != true", upload["if"])

        crda = load_workflow("crda.yml")
        crda_upload = next(
            step for step in crda["jobs"]["crda-scan"]["steps"]
            if "codeql-action/upload-sarif@" in step.get("uses", "")
        )
        self.assertIn("github.event_name == 'schedule'", crda_upload["if"])
        self.assertIn("github.event_name != 'pull_request'", crda_upload["if"])
        self.assertIn("github.event.pull_request.head.repo.full_name == github.repository", crda_upload["if"])
        self.assertIn("inputs.dry_run != true", crda_upload["if"])
        for condition_name, condition in (
            ("CRDA", crda_upload["if"]),
            (
                "DAST",
                next(
                    step["if"]
                    for step in load_workflow("security-dast-zap.yml")["jobs"]["zap-baseline"]["steps"]
                    if step.get("name") == "Upload ZAP Baseline SARIF to GitHub Security tab"
                ),
            ),
        ):
            for case_name, event_name, dry_run, same_repository_pr, expected in SARIF_UPLOAD_POLICY_CASES:
                with self.subTest(condition=condition_name, case=case_name):
                    self.assertEqual(
                        evaluate_sarif_upload_condition(
                            condition,
                            event_name,
                            dry_run,
                            same_repository_pr,
                        ),
                        expected,
                    )

        fortify = load_workflow("security-fortify.yml")
        self.assertIn("schedule", fortify.get(True, fortify.get("on")))
        fortify_steps = fortify["jobs"]["Fortify-AST-Scan"]["steps"]
        scanner = next(step for step in fortify_steps if "fortify/github-action@" in step.get("uses", ""))
        self.assertEqual(
            scanner["if"],
            "github.event_name != 'workflow_dispatch' || inputs.dry_run != true",
        )
        skipped = next(step for step in fortify_steps if step.get("name") == "Report intentional vendor-scan skip")
        self.assertEqual(
            skipped["if"],
            "github.event_name == 'workflow_dispatch' && inputs.dry_run == true",
        )
        self.assertIn("intentionally skipped", skipped["run"])
        self.assertIn("Scheduled runs", skipped["run"])

    def test_dast_dry_run_retains_reports_and_rejects_unsupported_modes(self) -> None:
        workflow = load_workflow("security-dast-zap.yml")
        triggers = workflow.get(True, workflow.get("on"))
        self.assertNotIn("target_url", str(triggers).lower())
        self.assertEqual(workflow["env"]["TARGET_URL"], "http://localhost:18080")
        self.assertEqual(
            triggers["workflow_dispatch"]["inputs"]["scan_mode"]["options"],
            ["baseline", "api-scan", "full-scan"],
        )
        self.assertEqual(triggers["workflow_call"]["inputs"]["scan_mode"]["default"], "baseline")

        validator = workflow["jobs"]["validate-inputs"]
        validation_step = validator["steps"][0]
        self.assertEqual(
            validation_step["env"]["SCAN_MODE"],
            "${{ github.event_name == 'schedule' && 'baseline' || inputs.scan_mode }}",
        )
        validation_script = validation_step["run"]
        self.assertIn("baseline|api-scan|full-scan", validation_script)
        self.assertIn("full-scan", validation_script)
        self.assertIn('"${EVENT_NAME}" != "workflow_dispatch"', validation_script)

        cases = (
            ("baseline", "schedule", True),
            ("api-scan", "workflow_call", True),
            ("full-scan", "workflow_dispatch", True),
            ("unsupported", "workflow_call", False),
            ("", "workflow_call", False),
            ("full-scan", "workflow_call", False),
            ("full-scan", "schedule", False),
        )
        for scan_mode, event_name, accepted in cases:
            with self.subTest(scan_mode=scan_mode, event=event_name):
                result = subprocess.run(
                    ["bash", "-e"],
                    input=validation_script,
                    text=True,
                    capture_output=True,
                    env={
                        **os.environ,
                        "SCAN_MODE": scan_mode,
                        "EVENT_NAME": event_name,
                        "GITHUB_OUTPUT": os.devnull,
                    },
                )
                self.assertEqual(result.returncode == 0, accepted, result.stderr)

        jobs = workflow["jobs"]
        for job_name, upload_name, artifact_name in (
            ("zap-baseline", "Upload ZAP Baseline SARIF to GitHub Security tab", "Upload ZAP Baseline report artifacts"),
            ("zap-api-scan", "Upload ZAP API Scan SARIF to GitHub Security tab", "Upload ZAP API Scan report artifacts"),
            ("zap-full-scan", "Upload ZAP Full Scan SARIF to GitHub Security tab", "Upload ZAP Full Scan report artifacts"),
        ):
            with self.subTest(dast_job=job_name):
                job = jobs[job_name]
                self.assertEqual(job["needs"], "validate-inputs")
                sarif_upload = next(step for step in job["steps"] if step.get("name") == upload_name)
                self.assertIn("github.event_name == 'schedule'", sarif_upload["if"])
                self.assertIn("github.event_name != 'pull_request'", sarif_upload["if"])
                self.assertIn(
                    "github.event.pull_request.head.repo.full_name == github.repository",
                    sarif_upload["if"],
                )
                self.assertIn("inputs.dry_run != true", sarif_upload["if"])
                artifact = next(step for step in job["steps"] if step.get("name") == artifact_name)
                self.assertEqual(artifact["if"], "always()")
                self.assertIn("report_sarif.json", artifact["with"]["path"])
                self.assertIn("report_json.json", artifact["with"]["path"])
                self.assertEqual(artifact["with"]["retention-days"], 90)

        for job_name, upload_name in (
            ("zap-baseline", "Upload ZAP Baseline SARIF to GitHub Security tab"),
            ("zap-api-scan", "Upload ZAP API Scan SARIF to GitHub Security tab"),
            ("zap-full-scan", "Upload ZAP Full Scan SARIF to GitHub Security tab"),
        ):
            upload_condition = next(
                step["if"] for step in jobs[job_name]["steps"]
                if step.get("name") == upload_name
            )
            for case_name, event_name, dry_run, same_repository_pr, expected in SARIF_UPLOAD_POLICY_CASES:
                with self.subTest(dast_job=job_name, case=case_name):
                    self.assertEqual(
                        evaluate_sarif_upload_condition(
                            upload_condition,
                            event_name,
                            dry_run,
                            same_repository_pr,
                        ),
                        expected,
                    )

        self.assertEqual(jobs["run-summary"]["if"], "always()")
        self.assertIn("validate-inputs", jobs["run-summary"]["needs"])


if __name__ == "__main__":
    unittest.main()
