import ast
import itertools
import unittest
from pathlib import Path

import yaml


WORKFLOW = Path(__file__).resolve().parents[1] / ".github/workflows/release-windows-distribution.yml"


def evaluate_condition(condition, detect_result, prepare_result, publish_requested, dry_run):
    replacements = {
        "always()": "True",
        "needs.detect-release.result": repr(detect_result),
        "needs.prepare-windows-distro-bundle.result": repr(prepare_result),
        "needs.detect-release.outputs.publish_requested": repr(publish_requested),
        "needs.detect-release.outputs.dry_run_enabled": repr(dry_run),
    }
    for token, value in replacements.items():
        condition = condition.replace(token, value)
    tree = ast.parse(condition.replace("&&", " and ").replace("||", " or "), mode="eval")

    def evaluate(node):
        if isinstance(node, ast.Expression):
            return evaluate(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (bool, str)):
            return node.value
        if isinstance(node, ast.BoolOp) and isinstance(node.op, (ast.And, ast.Or)):
            values = [evaluate(value) for value in node.values]
            return all(values) if isinstance(node.op, ast.And) else any(values)
        if isinstance(node, ast.Compare) and len(node.ops) == 1:
            left, right = evaluate(node.left), evaluate(node.comparators[0])
            if isinstance(node.ops[0], ast.Eq):
                return left == right
            if isinstance(node.ops[0], ast.NotEq):
                return left != right
        raise AssertionError(f"Unsupported condition: {ast.dump(node)}")

    return evaluate(tree)


class WindowsDistroPublishGateTests(unittest.TestCase):
    def test_publication_requires_successful_prerequisites_and_live_request(self):
        jobs = yaml.safe_load(WORKFLOW.read_text(encoding="utf-8"))["jobs"]
        publish = jobs["publish-windows-distro-bundle"]
        self.assertEqual(
            set(publish["needs"]),
            {"detect-release", "prepare-windows-distro-bundle"},
        )
        condition = publish["if"]
        results = ("success", "failure", "cancelled", "skipped")
        for detect, prepare, requested, dry_run in itertools.product(
            results, results, ("true", "false", ""), ("true", "false")
        ):
            with self.subTest(detect=detect, prepare=prepare, requested=requested, dry_run=dry_run):
                self.assertEqual(
                    evaluate_condition(condition, detect, prepare, requested, dry_run),
                    detect == "success"
                    and prepare == "success"
                    and requested == "true"
                    and dry_run != "true",
                )


if __name__ == "__main__":
    unittest.main()
