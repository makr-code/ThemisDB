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
            "maintenance-ci-health.yml": {"ci-health-dashboard", "update-chronic-issue"},
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
            "maintenance-build-issues.yml",
            "maintenance-ci-health.yml",
            "maintenance-issues.yml",
            "maintenance-docs.yml",
            "maintenance-pr-failure-diagnosis.yml",
            "maintenance-soll-ist-gap-issues.yml",
            "publish-wiki.yml",
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
        workflow_text = (WORKFLOWS / "maintenance-housekeeping.yml").read_text(encoding="utf-8")
        self.assertIn("if (dryRun)", workflow_text)
        self.assertIn("[dry-run] would post closure recommendation", workflow_text)

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

    def test_maintenance_issue_triage_dry_run_preserves_analysis_and_scheduled_writes(self) -> None:
        workflow = load_workflow("maintenance-issues.yml")
        triggers = workflow.get(True, workflow.get("on"))
        dry_run = triggers["workflow_dispatch"]["inputs"]["dry_run"]
        self.assertEqual(dry_run["type"], "boolean")
        self.assertIs(dry_run["default"], True)
        self.assertIn("scheduled runs remain live", dry_run["description"].lower())

        mutation_guard = "github.event_name != 'workflow_dispatch' || inputs.dry_run != true"
        jobs = workflow["jobs"]
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
                self.assertIn("github.event_name != 'workflow_dispatch'", step["if"])
                self.assertIn("inputs.dry_run != true", step["if"])
        self.assertTrue(any("dry-run" in str(step.get("run", "")).lower() for step in steps))
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


if __name__ == "__main__":
    unittest.main()
