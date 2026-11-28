import re
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/release-mainline.yml"


def load_workflow():
    # BaseLoader preserves GitHub's "on" key rather than parsing it as a boolean.
    return yaml.load(WORKFLOW.read_text(encoding="utf-8"), Loader=yaml.BaseLoader)


def eligible(condition, event, conclusion="", upstream_event=""):
    values = {
        "github.event_name": event,
        "github.event.workflow_run.conclusion": conclusion,
        "github.event.workflow_run.event": upstream_event,
    }
    expression = re.sub(
        r"github\.event(?:_name|\.workflow_run\.(?:conclusion|event))",
        lambda match: repr(values[match.group()]),
        condition,
    )
    expression = expression.replace("&&", " and ").replace("||", " or ")
    if not re.fullmatch(r"[a-zA-Z0-9_' ()=.!-]+", expression):
        raise AssertionError(f"Unexpected eligibility expression: {expression}")
    return eval(expression, {"__builtins__": {}}, {})


class ReleaseMainlineTriggerTests(unittest.TestCase):
    def test_only_existing_controlled_entry_points_are_registered(self):
        triggers = load_workflow()["on"]
        self.assertEqual(set(triggers), {"workflow_run", "workflow_dispatch", "workflow_call"})
        self.assertEqual(triggers["workflow_run"], {
            "workflows": ["Build: Mainline"],
            "types": ["completed"],
            "branches": ["develop"],
        })
        self.assertEqual(triggers["workflow_dispatch"]["inputs"]["dry_run"]["default"], "true")
        self.assertEqual(triggers["workflow_call"]["inputs"]["dry_run"]["default"], "true")

    def test_release_events_cannot_start_entry_jobs_even_through_reusable_call(self):
        jobs = load_workflow()["jobs"]
        for job in ("mark-release-pending", "preflight-release-policy"):
            condition = jobs[job]["if"]
            for action, tag in (("created", "v2.4.0"), ("published", "v2.4.0"),
                                ("published", "v2.4.0-nightly"),
                                ("published", "enterprise-v2.4.0")):
                with self.subTest(job=job, action=action, tag=tag):
                    self.assertFalse(eligible(condition, "release"))
            for event, conclusion, upstream, expected in (
                ("workflow_dispatch", "", "", True),
                ("workflow_call", "", "", True),
                ("push", "", "", True),  # reusable workflow inherits caller event
                ("workflow_run", "success", "schedule", True),
                ("workflow_run", "failure", "schedule", True),
                ("workflow_run", "cancelled", "schedule", False),
                ("workflow_run", "success", "workflow_dispatch", False),
            ):
                with self.subTest(job=job, event=event, conclusion=conclusion, upstream=upstream):
                    self.assertEqual(eligible(condition, event, conclusion, upstream), expected)

    def test_no_release_event_can_reach_build_publication_or_always_jobs(self):
        jobs = load_workflow()["jobs"]
        self.assertEqual(set(jobs["build-matrix"]["needs"]),
                         {"mark-release-pending", "preflight-release-policy"})
        for name in ("ci-widget", "publish-community", "publish-private"):
            condition = jobs[name]["if"]
            with self.subTest(job=name):
                self.assertNotIn("github.event_name == 'release'", condition)
                self.assertIn("github.event_name == 'workflow_dispatch'", condition)
        for name in ("publish-community", "publish-private"):
            self.assertIn("inputs.dry_run != true", jobs[name]["if"])
            self.assertIn("needs.validate-release-manifest.result == 'success'", jobs[name]["if"])
        for name in ("finalize-release-status", "run-summary"):
            self.assertIn("github.event_name != 'release'", jobs[name]["if"])
        self.assertIn("needs.build-matrix.result == 'success'", jobs["package"]["if"])
        self.assertIn("needs.build-matrix.result == 'failure'", jobs["package"]["if"])
        self.assertIn("needs.validate-release-manifest.result == 'success'",
                      jobs["llm-inference-gate"]["if"])
        self.assertIn("needs.publish-community.result == 'success'", jobs["changelog"]["if"])
        self.assertIn("needs.publish-private.result == 'success'", jobs["changelog"]["if"])
        # All other jobs depend on the guarded entry jobs, directly or transitively.
        for name, job in jobs.items():
            if name not in ("mark-release-pending", "preflight-release-policy"):
                self.assertIn("needs", job, name)


if __name__ == "__main__":
    unittest.main()
