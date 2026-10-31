from pathlib import Path

import yaml


def test_release_critical_gate_has_two_hour_timeout():
    repository_root = Path(__file__).resolve().parents[2]
    workflow_path = (
        repository_root
        / ".github"
        / "workflows"
        / "reusable-pr-build-components-gate.yml"
    )

    workflow = yaml.safe_load(workflow_path.read_text(encoding="utf-8"))
    timeout = workflow["jobs"]["release-critical-tests"]["timeout-minutes"]

    assert timeout >= 120
