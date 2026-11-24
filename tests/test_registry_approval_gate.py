import json
import os
import re
import subprocess
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github/workflows"
REUSABLE = yaml.safe_load(
    (WORKFLOWS / "reusable-release-approval-comment-gate.yml").read_text()
)
STEPS = [
    step for job in REUSABLE["jobs"].values()
    for step in job.get("steps", [])
]
WRAPPERS = {
    "linux": ("release-linux-distro-approval.yml", "linux_distro_publish_from_issue"),
    "windows": ("release-windows-distro-approval.yml", "windows_distro_publish_from_issue"),
    "winget": ("release-winget-approval.yml", "winget_publish_from_issue"),
    "docker": ("release-docker-approval.yml", "docker_publish_from_issue"),
}


def step(name):
    return next(s for s in STEPS if s.get("name") == name)


def execute(script, *, env=None, body="", issue_body="", permission="maintain",
            prerelease=False, draft=False, labels=None, issue_state="open", live_state="open",
            action="created", live_body=None):
    script = script.replace("${{ steps.extract_version.outputs.version }}", "2.4.0-rc.1")
    harness = """
      const input = JSON.parse(process.argv[1]);
      Object.assign(process.env, input.env);
      const outputs = {}, failures = [], calls = [];
      const core = {
        setOutput: (key, value) => { outputs[key] = value; },
        setFailed: (value) => failures.push(value), notice: () => {}
      };
      const context = {
        repo: {owner: 'owner', repo: 'repo'},
        payload: {action: input.action, issue: {body: input.issue_body, number: 42, state: input.issue_state,
                          labels: input.labels.map(name => ({name}))},
                  comment: {body: input.body, user: {login: 'reviewer'}}}
      };
      const github = {rest: {
        repos: {
          getCollaboratorPermissionLevel: async () => ({data: {permission: input.permission}}),
          getReleaseByTag: async (args) => { calls.push(args); return {data: {
            prerelease: input.prerelease, draft: input.draft}}; }
        },
        actions: {createWorkflowDispatch: async (args) => calls.push(args)},
        issues: {get: async () => ({data: {state: input.live_state,
          body: input.live_body === null ? input.issue_body : input.live_body,
          labels: input.labels.map(name => ({name}))}})}
      }};
      (async () => {
        await (async () => { SCRIPT })();
        console.log(JSON.stringify({outputs, failures, calls}));
      })().catch(error => { console.error(error); process.exit(1); });
    """.replace("SCRIPT", script)
    result = subprocess.run(
        ["node", "-e", harness, json.dumps({
            "env": env or {}, "body": body, "issue_body": issue_body,
            "permission": permission, "prerelease": prerelease, "draft": draft,
            "labels": labels or [], "issue_state": issue_state, "live_state": live_state,
            "live_body": live_body, "action": action,
        })],
        capture_output=True, text=True, check=True,
    )
    return json.loads(result.stdout)


class RegistryApprovalGateTests(unittest.TestCase):
    def test_exact_commands_and_permissions(self):
        script = step("Validate maintainer and command")["with"]["script"]
        for suffix, (filename, job_name) in WRAPPERS.items():
            workflow = yaml.safe_load((WORKFLOWS / filename).read_text())
            self.assertEqual(workflow[True]["issue_comment"]["types"], ["created"])
            job = workflow["jobs"][job_name]
            config = job["with"]
            self.assertIn("github.event.issue.state == 'open'", job["if"])
            env = {
                "ISSUE_LABEL": config["issue_label"],
                "MATCH_REGEX": config["match_regex"],
                "HOLD_REGEX": config["hold_regex"],
                "REJECT_REGEX": config["reject_regex"],
                "ROLLBACK_REGEX": config["rollback_regex"],
            }
            match = re.search(r"fromJSON\('(\[.*?\])'\)", job["if"])
            self.assertIsNotNone(match)
            commands = json.loads(match.group(1))
            expected = {
                f"/publish-{suffix}": "approve", f"/approve-{suffix}": "approve",
                f"@publish-{suffix}": "approve", f"/hold-{suffix}": "hold",
                f"/pause-{suffix}": "hold", f"/reject-{suffix}": "reject",
                f"/cancel-{suffix}": "reject", f"/rollback-{suffix}": "rollback",
            }
            self.assertEqual(set(commands), set(expected))
            for command, status in expected.items():
                with self.subTest(command=command):
                    result = execute(script, env=env, body=command.upper(), labels=[config["issue_label"]])
                    self.assertEqual(result["outputs"]["status"], status)
                    for permission in ("write", "read", "none"):
                        denied = execute(script, env=env, body=command, permission=permission,
                                         labels=[config["issue_label"]])
                        self.assertTrue(denied["failures"])
                    admin = execute(script, env=env, body=command, permission="admin",
                                    labels=[config["issue_label"]])
                    self.assertEqual(admin["outputs"]["status"], status)
            for body in (f"please /publish-{suffix}", f"/publish-{suffix}-extra",
                         f"/publish-{suffix}\n/hold-{suffix}", f"`/publish-{suffix}`"):
                with self.subTest(body=body):
                    self.assertNotIn(body.lower(), commands)
                    result = execute(script, env=env, body=body, labels=[config["issue_label"]])
                    self.assertEqual(result["outputs"]["skip"], "true")
            self.assertEqual(execute(script, env=env, body=commands[0])["outputs"]["skip"], "true")
            closed = execute(script, env=env, body=commands[0], issue_state="closed",
                             labels=[config["issue_label"]])
            self.assertTrue(closed["failures"])
            self.assertNotIn("status", closed["outputs"])
            closed_after_event = execute(script, env=env, body=commands[0], live_state="closed",
                                         labels=[config["issue_label"]])
            self.assertTrue(closed_after_event["failures"])
            self.assertNotIn("status", closed_after_event["outputs"])
            edited = execute(script, env=env, body=commands[0], action="edited",
                             labels=[config["issue_label"]])
            self.assertEqual(edited["outputs"]["skip"], "true")

    def test_winget_opt_in_and_release_flag(self):
        script = step("Validate WinGet prerelease authorization")["with"]["script"]
        for version, flag, field, allowed in (
            ("2.4.0", False, "", False),
            ("2.4.0", False, "**WinGet prerelease opt-in:** true", True),
            ("2.4.0-rc.1", False, "", None),
            ("2.4.0-rc.1", False, "**WinGet prerelease opt-in:** false", None),
            ("2.4.0-rc.1", False, "**WinGet prerelease opt-in:** true", True),
            ("2.4.0", True, "", None),
            ("2.4.0", True, "**WinGet prerelease opt-in:** true", True),
            ("2.4.0", False, "**WinGet prerelease opt-in:** maybe", None),
            ("2.4.0", False, "**WinGet prerelease opt-in:** true\n**WinGet prerelease opt-in:** false", None),
        ):
            with self.subTest(version=version, flag=flag, field=field):
                result = execute(script, env={"VERSION": version}, issue_body=field, prerelease=flag)
                if allowed is None:
                    self.assertTrue(result["failures"])
                else:
                    self.assertFalse(result["failures"])
                    self.assertEqual(result["outputs"]["allowed"], str(allowed).lower())
                    self.assertEqual(result["calls"][0]["tag"], f"v{version}")
        self.assertTrue(execute(script, env={"VERSION": "2.4.0"}, draft=True)["failures"])

    def test_issue_version_and_approval_only_side_effects(self):
        script = step("Extract version from issue body")["with"]["script"]
        for body, expected in (
            ("## Release Version: v2.4.0\n", "2.4.0"),
            ("## Release Version: v2.4.0-rc.1\n", "2.4.0-rc.1"),
            ("## Release Version: v2.4.0invalid\n", None),
            ("No release version", None),
        ):
            result = execute(script, issue_body=body)
            if expected is None:
                self.assertTrue(result["failures"])
            else:
                self.assertEqual(result["outputs"]["version"], expected)
        jobs = REUSABLE["jobs"]
        pre = jobs["pre-validation"]
        protected = jobs["approval-gate"]
        self.assertNotIn("environment", pre)
        self.assertEqual(protected["needs"], ["pre-validation"])
        self.assertIn("environment", protected)
        condition = protected["if"]
        for token in ("result == 'success'", "outputs.gate-skip != 'true'",
                      "outputs.gate-is-maintainer == 'true'", "outputs.gate-status == 'approve'"):
            self.assertIn(token, condition)
        for result, skip, maintainer, status, expected in (
            ("success", "", "true", "approve", True),
            ("success", "", "true", "hold", False),
            ("success", "", "true", "reject", False),
            ("success", "", "true", "rollback", False),
            ("success", "true", "", "", False),
            ("failure", "", "false", "", False),
            ("cancelled", "", "true", "approve", False),
        ):
            expression = condition
            for key, value in {
                "result": result, "outputs.gate-skip": skip,
                "outputs.gate-is-maintainer": maintainer, "outputs.gate-status": status,
            }.items():
                expression = expression.replace(f"needs.pre-validation.{key}", repr(value))
            self.assertEqual(eval(expression.replace("&&", " and "), {"__builtins__": {}}), expected)
        for name in ("Extract version from issue body", "Verify release tag exists",
                     "Validate WinGet prerelease authorization", "Dispatch target workflow",
                     "Comment approval status", "Update issue label"):
            self.assertIn(step(name), protected["steps"])
        self.assertIn("steps.gate.outputs.status != 'approve'",
                      step("Handle hold/reject/rollback requests")["if"])
        self.assertIn(step("Handle hold/reject/rollback requests"), pre["steps"])
        escalation = jobs["human-loop-escalation"]
        self.assertEqual(escalation["needs"], ["pre-validation", "approval-gate"])
        self.assertNotIn("!= 'success'", escalation["if"])
        self.assertIn("needs.pre-validation.result == 'failure'", escalation["if"])
        self.assertIn("needs.approval-gate.result == 'failure'", escalation["if"])

    def test_dispatch_inputs_version_and_authorization(self):
        script = step("Dispatch target workflow")["with"]["script"]
        base = {"WORKFLOW_ID": "release-winget.yml", "TARGET_REF": "develop",
                "VERSION_INPUT_NAME": "version", "REQUIRE_PRERELEASE_OPT_IN": "true",
                "ISSUE_LABEL": "winget-release"}
        for allowed in ("true", "false"):
            result = execute(script, env={**base, "EXTRA_INPUTS_JSON": '{"dry_run":false}',
                                          "PRERELEASE_ALLOWED": allowed}, labels=["winget-release"])
            self.assertFalse(result["failures"])
            inputs = result["calls"][0]["inputs"]
            self.assertEqual(inputs["version"], "2.4.0-rc.1")
            self.assertEqual(inputs.get("allow_prerelease"), "true" if allowed == "true" else None)
        for extra in ('{"allow_prerelease":"true"}', "[]", "{"):
            result = execute(script, env={**base, "EXTRA_INPUTS_JSON": extra,
                                          "PRERELEASE_ALLOWED": "false"})
            self.assertTrue(result["failures"])
            self.assertFalse(result["calls"])
        missing = execute(script, env={**base, "EXTRA_INPUTS_JSON": "{}"})
        self.assertTrue(missing["failures"])
        self.assertFalse(missing["calls"])
        closed = execute(script, env={**base, "EXTRA_INPUTS_JSON": "{}",
                                      "PRERELEASE_ALLOWED": "false"}, live_state="closed")
        self.assertTrue(closed["failures"])
        self.assertFalse(closed["calls"])
        changed = execute(script, env={**base, "EXTRA_INPUTS_JSON": "{}",
                                       "PRERELEASE_ALLOWED": "false"},
                          labels=["winget-release"], issue_body="original", live_body="modified")
        self.assertTrue(changed["failures"])
        self.assertFalse(changed["calls"])
        revoked = execute(script, env={**base, "EXTRA_INPUTS_JSON": "{}",
                                       "PRERELEASE_ALLOWED": "false"},
                          labels=["winget-release"], permission="write")
        self.assertTrue(revoked["failures"])
        self.assertFalse(revoked["calls"])

    def test_wrapper_summaries_and_dispatch_language(self):
        for filename, job_name in WRAPPERS.values():
            workflow = yaml.safe_load((WORKFLOWS / filename).read_text())
            summary = workflow["jobs"]["summary"]
            self.assertEqual(summary["needs"], [job_name])
            self.assertEqual(summary["steps"][0]["uses"], "actions/checkout@v5")
            self.assertFalse(summary["steps"][0]["with"]["persist-credentials"])
            shared = summary["steps"][1]
            self.assertEqual(shared["uses"], "./.github/actions/write-run-summary")
            self.assertIn("not confirmed", shared["with"]["scope"])
            self.assertEqual(shared["with"]["job-results"], "${{ toJSON(needs) }}")
        text = step("Comment approval status")["with"]["script"]
        self.assertIn("/actions/workflows/", text)
        self.assertIn("publication has not been confirmed", text)
        self.assertNotIn("/actions/runs/", text)


if __name__ == "__main__":
    unittest.main()
