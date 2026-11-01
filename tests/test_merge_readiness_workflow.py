import os
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/gate-pr-merge-readiness.yml"


class MergeReadinessWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.job = yaml.safe_load(WORKFLOW.read_text())["jobs"]["merge-readiness"]

    def test_cancelled_workflow_does_not_publish_a_verdict(self):
        self.assertEqual(
            self.job["if"],
            "${{ !cancelled() && github.event_name == 'pull_request' }}",
        )

    def evaluate(self, results):
        script = next(
            step["run"] for step in self.job["steps"] if step.get("id") == "evaluate"
        )
        for gate, result in zip(self.job["needs"], results):
            script = script.replace("${{ needs." + gate + ".result }}", result)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "output"
            result = subprocess.run(
                ["bash", "-e", "-c", script],
                env={**os.environ, "GITHUB_OUTPUT": str(output)},
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            values = dict(line.split("=", 1) for line in output.read_text().splitlines())

        verdict = next(
            step["run"] for step in self.job["steps"]
            if step.get("name") == "Enforce merge-readiness verdict"
        )
        for key, value in values.items():
            verdict = verdict.replace("${{ steps.evaluate.outputs." + key + " }}", value)
        result = subprocess.run(
            ["bash", "-e", "-c", verdict], capture_output=True, text=True
        )
        return values, result

    def test_success_and_intentional_skips_remain_ready(self):
        for index in range(len(self.job["needs"]) + 1):
            results = ["success"] * len(self.job["needs"])
            if index < len(results):
                results[index] = "skipped"
            with self.subTest(results=results):
                values, verdict = self.evaluate(results)
                self.assertEqual(values["all_passed"], "true")
                self.assertEqual(values["failed_gates"], "")
                self.assertEqual(verdict.returncode, 0, verdict.stderr)

    def test_failed_or_independently_cancelled_gate_remains_blocking(self):
        names = [
            "Community Fail-Closed", "Edition & License", "Hash & SBOM Integrity",
            "Plugin Boundary", "Final Runtime API",
        ]
        for index, name in enumerate(names):
            for status in ["failure", "cancelled"]:
                results = ["success"] * len(self.job["needs"])
                results[index] = status
                with self.subTest(gate=name, status=status):
                    values, verdict = self.evaluate(results)
                    self.assertEqual(values["all_passed"], "false")
                    self.assertEqual(values["failed_gates"], name)
                    self.assertEqual(verdict.returncode, 1)
                    self.assertIn(name, verdict.stdout)


if __name__ == "__main__":
    unittest.main()
