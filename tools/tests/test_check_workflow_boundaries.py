#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import subprocess
from pathlib import Path

import yaml
import pytest

_HERE = Path(__file__).resolve().parent
_TOOLS = _HERE.parent
_CI = _TOOLS / "ci"

for _p in (_TOOLS, _CI):
    if str(_p) not in sys.path:
        sys.path.insert(0, str(_p))

import check_workflow_boundaries as guard  # noqa: E402


def _write_workflow(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.strip() + "\n", encoding="utf-8")


def test_reactivated_workflow_requires_docs_updates(tmp_path: Path):
    _write_workflow(
        tmp_path / ".github" / "workflows" / "09-pr-gates_test.yml",
        """
name: Test Guard
on:
  workflow_dispatch:
  pull_request:
    branches: [main]
    paths:
      - '.github/workflows/**'
permissions:
  contents: read
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )
    _write_workflow(
        tmp_path / ".github" / "no_workflows" / "09-pr-gates_test.yml",
        "name: Old Guard\non: workflow_dispatch\n",
    )

    violations = guard.evaluate(
        [{
            "status": "R100",
            "old": ".github/no_workflows/09-pr-gates_test.yml",
            "new": ".github/workflows/09-pr-gates_test.yml",
        }],
        tmp_path,
    )

    assert any("WORKFLOW_GUIDELINES" in violation for violation in violations)
    assert any("WORKFLOW_REGISTRY" in violation for violation in violations)


def test_pull_request_paths_must_not_be_repo_wide(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "guard.yml"
    _write_workflow(
        workflow,
        """
name: Broad Guard
on:
  pull_request:
    branches: [main]
    paths:
      - 'src/**'
permissions:
  contents: read
concurrency:
  group: test
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )

    violations = guard.validate_workflow_file(workflow)
    assert any("too broad" in violation for violation in violations)


def test_pull_request_concurrency_group_must_not_use_run_id(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "guard.yml"
    _write_workflow(
        workflow,
        """
name: Guard
on:
  pull_request:
    branches: [main]
    paths:
      - '.github/workflows/**'
permissions:
  contents: read
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}-${{ github.run_id }}
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )

    violations = guard.validate_workflow_file(workflow)
    assert any("github.run_id" in violation for violation in violations)


def test_reactivated_workflow_with_manual_dispatch_and_docs_passes(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "09-pr-gates_test.yml"
    _write_workflow(
        workflow,
        """
name: Workflow Boundary Guard
on:
  pull_request:
    branches: [main, develop]
    paths:
      - '.github/workflows/**'
      - '.github/no_workflows/**'
      - '.github/WORKFLOW_GUIDELINES.md'
      - '.github/WORKFLOW_REGISTRY.md'
      - 'tools/ci/check_workflow_boundaries.py'
      - 'tools/tests/test_check_workflow_boundaries.py'
  workflow_dispatch:
permissions:
  contents: read
concurrency:
  group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}
  cancel-in-progress: true
jobs:
  guard:
    runs-on: ubuntu-latest
    steps:
      - run: echo ok
""",
    )
    _write_workflow(
        tmp_path / ".github" / "no_workflows" / "09-pr-gates_test.yml",
        "name: Old Guard\non: workflow_dispatch\n",
    )
    (tmp_path / ".github" / "WORKFLOW_GUIDELINES.md").write_text("updated\n", encoding="utf-8")
    (tmp_path / ".github" / "WORKFLOW_REGISTRY.md").write_text("updated\n", encoding="utf-8")

    violations = guard.evaluate(
        [
            {
                "status": "R100",
                "old": ".github/no_workflows/09-pr-gates_test.yml",
                "new": ".github/workflows/09-pr-gates_test.yml",
            },
            {"status": "M", "path": ".github/WORKFLOW_GUIDELINES.md"},
            {"status": "M", "path": ".github/WORKFLOW_REGISTRY.md"},
        ],
        tmp_path,
    )

    assert violations == []


def test_main_returns_zero_for_irrelevant_changes(tmp_path: Path):
    diff = tmp_path / "changes.txt"
    diff.write_text("M\tREADME.md\n", encoding="utf-8")

    rc = guard.main(["--repo-root", str(tmp_path), "--diff-file", str(diff)])
    assert rc == 0


@pytest.mark.parametrize(
    "checkout",
    [
        [],
        [{"uses": "actions/checkout@v5", "if": "false"}],
        [{"uses": "actions/checkout@v5", "continue-on-error": True}],
        [{"uses": "actions/checkout@v5", "with": {"path": "other"}}],
        [{"uses": "actions/checkout@v5", "with": {"repository": "other/repo"}}],
    ],
)
def test_local_summary_requires_unconditional_checkout_in_same_job(tmp_path: Path, checkout):
    workflow = tmp_path / "summary.yml"
    workflow.write_text(
        yaml.safe_dump({
            "name": "Summary",
            "on": {"workflow_dispatch": None},
            "permissions": {"contents": "read"},
            "jobs": {
                "build": {"steps": [{"uses": "actions/checkout@v5"}]},
                "summary": {
                    "steps": checkout + [
                        {"uses": "./.github/actions/write-run-summary"},
                        {"uses": "actions/checkout@v5"},
                    ],
                },
            },
        }),
        encoding="utf-8",
    )
    violations = guard.validate_workflow_file(workflow)
    assert len(violations) == 1
    assert "job summary must check out the repository" in violations[0]


def test_local_summary_accepts_prior_checkout(tmp_path: Path):
    workflow = tmp_path / "summary.yml"
    _write_workflow(
        workflow,
        """
name: Summary
on:
  workflow_dispatch:
permissions:
  contents: read
jobs:
  summary:
    steps:
      - uses: actions/checkout@v5
        with:
          persist-credentials: false
      - uses: ./.github/actions/write-run-summary
""",
    )
    assert guard.validate_workflow_file(workflow) == []


@pytest.mark.parametrize(
    ("cancel", "accepted"),
    [
        ("${{ github.event_name == 'pull_request' }}", True),
        ("${{ github.event_name != 'pull_request' }}", False),
        (False, False),
    ],
)
def test_mixed_workflows_cancel_only_pull_requests(tmp_path: Path, cancel, accepted):
    workflow = tmp_path / "mixed.yml"
    workflow.write_text(
        yaml.safe_dump({
            "name": "Mixed workflow",
            "on": {
                "pull_request": {
                    "branches": ["develop"],
                    "paths": [".github/workflows/mixed.yml"],
                },
                "workflow_dispatch": None,
            },
            "permissions": {"contents": "read"},
            "concurrency": {"group": "mixed-${{ github.ref }}", "cancel-in-progress": cancel},
            "jobs": {"check": {"steps": [{"run": "echo ok"}]}},
        }),
        encoding="utf-8",
    )
    violations = guard.validate_workflow_file(workflow)
    assert (violations == []) is accepted


def test_repository_summary_jobs_check_out_local_action():
    workflows = Path(__file__).resolve().parents[2] / ".github" / "workflows"
    violations = []
    summary_jobs = 0
    for workflow in sorted(workflows.glob("*.yml")):
        data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
        summary_jobs += sum(
            step.get("uses") == "./.github/actions/write-run-summary"
            for job in data.get("jobs", {}).values()
            for step in job.get("steps", [])
        )
        guard._validate_summary_checkout(data, str(workflow), violations)
    assert summary_jobs > 0
    assert violations == []


def test_core_gate_delegates_issue_permissions_to_build_components():
    workflows = Path(__file__).resolve().parents[2] / ".github" / "workflows"
    core = yaml.safe_load((workflows / "gate-pr-core.yml").read_text(encoding="utf-8"))
    assert core["jobs"]["build-components-gate"]["permissions"]["issues"] == "write"
    rag = yaml.safe_load((workflows / "reusable-rag-phase-gate.yml").read_text(encoding="utf-8"))
    assert rag["permissions"]["pull-requests"] == "write"
    assert rag["permissions"]["issues"] == "write"
    for phase in range(7, 11):
        caller = yaml.safe_load((workflows / f"gate-pr-rag-phase{phase}.yml").read_text(encoding="utf-8"))
        assert caller["permissions"]["pull-requests"] == "write"


def test_cmake_cache_retries_disable_forced_launcher_without_resetting_toolchain():
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "reusable-cmake-build.yml"
    steps = yaml.safe_load(workflow.read_text(encoding="utf-8"))["jobs"]["build"]["steps"]
    for name in ("Configure", "Build"):
        script = next(step["run"] for step in steps if step.get("name") == name)
        retry = next(line for line in script.splitlines() if 'cmake "$BUILD_DIR"' in line)
        assert "-DTHEMIS_ENABLE_COMPILER_CACHE=OFF" in retry
        assert "-DCMAKE_C_COMPILER_LAUNCHER=" in retry
        assert "-DCMAKE_CXX_COMPILER_LAUNCHER=" in retry
        assert "-DCMAKE_TOOLCHAIN_FILE=" not in retry
    scripts = "\n".join(step.get("run", "") for step in steps)
    assert 'vcpkg_binary_sources="clear;files,${vcpkg_cache_dir},readwrite"' in scripts
    assert ";nuget," not in scripts


@pytest.mark.parametrize("stage", ["Configure", "Build"])
@pytest.mark.parametrize("retry_status", [0, 9])
def test_cmake_cache_recovery_propagates_status(tmp_path: Path, stage, retry_status):
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "reusable-cmake-build.yml"
    steps = yaml.safe_load(workflow.read_text(encoding="utf-8"))["jobs"]["build"]["steps"]
    script = next(step["run"] for step in steps if step.get("name") == stage)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    script = script.replace("${{ steps.resolve_mode.outputs.resolved_build_dir }}", str(build_dir))
    cmake = tmp_path / "cmake"
    cmake.write_text(
        '#!/bin/bash\n'
        'echo "$*" >> "$CALLS_FILE"\n'
        'if [[ "$*" == *"-DTHEMIS_ENABLE_COMPILER_CACHE=OFF"* ]]; then\n'
        '  touch "$RETRY_DONE"\n'
        '  exit "$RETRY_STATUS"\n'
        'fi\n'
        'if [[ "$1" == "--build" && -f "$RETRY_DONE" ]]; then exit 0; fi\n'
        'echo "sccache: error: Server startup failed"\n'
        'exit 1\n',
        encoding="utf-8",
    )
    cmake.chmod(0o755)
    calls = tmp_path / "calls"
    env = {
        **os.environ,
        "PATH": f"{tmp_path}:{os.environ['PATH']}",
        "WORKING_DIRECTORY": str(tmp_path),
        "RUNNER_TEMP": str(tmp_path),
        "RUNNER_ARCH": "X64",
        "BUILD_DIR": str(build_dir),
        "CONFIGURE_COMMAND": "cmake --preset check",
        "BUILD_COMMAND": f"cmake --build {build_dir}",
        "CALLS_FILE": str(calls),
        "RETRY_DONE": str(tmp_path / "retry-done"),
        "RETRY_STATUS": str(retry_status),
    }
    result = subprocess.run(["bash", "-c", script], env=env, capture_output=True, text=True)
    assert result.returncode == retry_status, result.stdout + result.stderr
    commands = calls.read_text(encoding="utf-8").splitlines()
    assert len(commands) == (3 if stage == "Build" and retry_status == 0 else 2)
    assert "-DTHEMIS_ENABLE_COMPILER_CACHE=OFF" in commands[1]
    assert "-DCMAKE_TOOLCHAIN_FILE=" not in commands[1]


@pytest.mark.parametrize("directory", ["", "build-fast", "/tmp/rag-build"])
def test_cmake_resolver_honors_explicit_binary_directory(tmp_path: Path, directory):
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "reusable-cmake-build.yml"
    steps = yaml.safe_load(workflow.read_text(encoding="utf-8"))["jobs"]["build"]["steps"]
    step = next(step for step in steps if step.get("id") == "resolve_mode")
    output = tmp_path / "outputs"
    env = {
        **os.environ,
        **{key: "" for key in step["env"]},
        "PRESET": "check",
        "BUILD_DIRECTORY": directory,
        "GITHUB_OUTPUT": str(output),
    }
    subprocess.run(["bash", "-c", step["run"]], env=env, check=True, capture_output=True)
    expected = directory or "build-check"
    assert f"resolved_build_dir={expected}\n" in output.read_text(encoding="utf-8")
    assert f"cmake -S . -B {expected} -G Ninja" in output.read_text(encoding="utf-8")


def test_custom_cmake_callers_declare_binary_directories():
    workflows = Path(__file__).resolve().parents[2] / ".github" / "workflows"
    expected = {
        "build-mainline.yml": {"build-fast", "build"},
        "build-clang-fast.yml": {"build-clang-fast"},
        "build-sanitizer-nightly.yml": {"build-sanitizer"},
        "edition-hyperscaler-ci.yml": {"build-hyperscaler"},
        "gate-pr-rag-eval.yml": {"/tmp/rag-build"},
        "security-fuzzing.yml": {"build-fuzz"},
    }
    for name, directories in expected.items():
        jobs = yaml.safe_load((workflows / name).read_text(encoding="utf-8"))["jobs"]
        actual = {
            job["with"]["build_directory"]
            for job in jobs.values()
            if job.get("uses") == "./.github/workflows/reusable-cmake-build.yml"
        }
        assert actual == directories
    matrix = yaml.safe_load((workflows / "release-build-matrix.yml").read_text(encoding="utf-8"))
    assert any(
        job.get("with", {}).get("build_directory") == "${{ matrix.build_directory }}"
        for job in matrix["jobs"].values()
    )


def test_docker_nested_concurrency_groups_do_not_collide():
    workflows = Path(__file__).resolve().parents[2] / ".github" / "workflows"
    platform = yaml.safe_load((workflows / "reusable-docker-image-platform.yml").read_text(encoding="utf-8"))
    cache = yaml.safe_load((workflows / "reusable-llm-model-cache.yml").read_text(encoding="utf-8"))
    parent = yaml.safe_load((workflows / "release-docker-image.yml").read_text(encoding="utf-8"))
    assert parent["jobs"]["build-platforms"]["strategy"]["matrix"]["platform"] == [
        "linux/amd64", "linux/arm64",
    ]

    def resolve_group(workflow, target):
        group = workflow["concurrency"]["group"]
        for expression, value in {
            "github.workflow": "Release: Docker Image Consumer",
            "github.ref": "refs/heads/develop",
            "inputs.platform": target,
            "inputs.llm_model": "tinyllama",
            "inputs.cache_key_suffix": "develop",
        }.items():
            group = group.replace("${{ " + expression + " }}", value)
        assert "${{" not in group
        assert workflow["concurrency"]["cancel-in-progress"] is False
        return group

    amd64 = resolve_group(platform, "linux/amd64")
    arm64 = resolve_group(platform, "linux/arm64")
    model = resolve_group(cache, "linux/amd64")
    assert len({amd64, arm64, model}) == 3
    assert model == resolve_group(cache, "linux/arm64")
    assert not any(group.startswith("docker-image-") for group in (amd64, arm64, model))


def test_docker_local_export_avoids_attestation_manifest_and_uses_native_runners():
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "reusable-docker-image-platform.yml"
    build = yaml.safe_load(workflow.read_text(encoding="utf-8"))["jobs"]["build"]
    assert build["runs-on"] == "${{ inputs.platform == 'linux/arm64' && 'ubuntu-24.04-arm' || 'ubuntu-24.04' }}"
    candidate = next(step for step in build["steps"] if step.get("id") == "build-candidate")["with"]
    push = "${{ inputs.push_to_registry == true || inputs.push_to_registry == 'true' }}"
    assert candidate["push"] == candidate["provenance"] == candidate["sbom"] == push
    assert candidate["load"] == "${{ ! (inputs.push_to_registry == true || inputs.push_to_registry == 'true') }}"
    assert candidate["target"] == "runtime"


def test_wave4_gate_runs_benchmark_with_build_runtime_and_transfers_results():
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "benchmark-gate.yml"
    jobs = yaml.safe_load(workflow.read_text(encoding="utf-8"))["jobs"]
    build = jobs["build-wave4-release-gates"]["with"]
    assert "--preset linux-release" in build["configure_command"]
    assert "-DTHEMIS_BUILD_BENCHMARKS=ON" in build["configure_command"]
    assert "--target bench_w4a_release_gates" in build["build_command"]
    results = Path(build["artifact_paths"])
    assert results == Path("benchmark-artifacts/w4a_release_gates.json")
    assert "./build-linux-release/bin/bench_w4a_release_gates" in build["validation_command"]
    assert f"--benchmark_out={results}" in build["validation_command"]
    assert "BENCHMARK_SEED=42" in build["validation_command"]
    steps = jobs["run-wave4-release-gates"]["steps"]
    download = next(step for step in steps if step.get("uses", "").startswith("actions/download-artifact@"))
    assert download["with"]["name"] == build["artifact_name"]
    assert download["with"]["path"] == str(results.parent)
    compare = next(step["run"] for step in steps if "report_variance.py" in step.get("run", ""))
    assert f"--input {results}" in compare


def test_coverage_installs_real_test_and_storage_dependencies():
    workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "coverage-report-nightly.yml"
    data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
    assert data["env"]["THEMIS_ALLOW_MISSING_ROCKSDB"] == "OFF"
    steps = data["jobs"]["coverage-collection"]["steps"]
    install = next(step["run"] for step in steps if step.get("name") == "Install coverage tools")
    for package in ("librocksdb-dev", "nlohmann-json3-dev", "libgtest-dev", "libgmock-dev", "libpugixml-dev"):
        assert package in install.split()
    configure = next(step["run"] for step in steps if step.get("name") == "CMake configure (with coverage enabled)")
    assert "-DTHEMIS_BUILD_TESTS=ON" in configure
    assert "-DTHEMIS_ALLOW_MISSING_ROCKSDB=OFF" in configure


@pytest.mark.parametrize(
    ("body", "introduced", "exit_code"),
    [
        ("uses: ./.github/workflows/reusable.yml", True, 0),
        ("steps:\n      - uses: ./.github/actions/local", True, 0),
        ("steps:\n      - uses: vendor/action@" + "a" * 40 + " # pinned", True, 0),
        ("steps:\n      - uses: vendor/action@v1", True, 1),
        ("steps:\n      - uses: vendor/action@v1", False, 0),
        ("steps:\n      - run: |\n          git push origin HEAD", True, 1),
        ("steps:\n      - run: git push origin HEAD", True, 1),
        ('steps:\n      - run: "git push origin HEAD"', True, 1),
        ("steps:\n      - run: 'git push origin HEAD'", True, 1),
        ("steps:\n      - run: |\n          git push origin HEAD", False, 0),
        ("steps:\n      - run: echo check\n        continue-on-error: true", True, 1),
        ("steps:\n      - continue-on-error: true\n        run: echo check", True, 1),
        ("steps:\n      - run: echo check\n        continue-on-error: true", False, 0),
    ],
)
def test_core_preflight_local_refs_comments_and_new_policy_violations(
    tmp_path: Path, monkeypatch, body, introduced, exit_code
):
    source = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "gate-pr-core.yml"
    data = yaml.safe_load(source.read_text(encoding="utf-8"))
    step = next(
        step for step in data["jobs"]["preflight-ci-policy"]["steps"]
        if step.get("name") == "Run workflow preflight checks"
    )
    code = step["run"].split("python3 - <<'PY'\n", 1)[1].rsplit("\nPY", 1)[0]
    workflow = tmp_path / ".github" / "workflows" / "test.yml"
    text = "name: Test\non: workflow_dispatch\npermissions:\n  contents: read\njobs:\n  check:\n    " + body + "\n"
    _write_workflow(workflow, text)
    changed = tmp_path / "changed-files.txt"
    changed.write_text(".github/workflows/test.yml\n", encoding="utf-8")
    code = code.replace('"/tmp/changed_files.txt"', repr(str(changed)))
    diff = "\n".join("+" + line for line in text.splitlines()) if introduced else ""
    monkeypatch.setattr(
        subprocess, "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(args[0], 0, stdout=diff),
    )
    monkeypatch.setenv("BASE_REF", "develop")
    monkeypatch.chdir(tmp_path)
    try:
        exec(compile(code, str(source), "exec"), {})
    except SystemExit as exc:
        assert exc.code == exit_code
    else:
        assert exit_code == 0


def test_merge_readiness_requires_final_runtime_api_gate(tmp_path: Path):
    workflow = tmp_path / ".github" / "workflows" / "gate-pr-merge-readiness.yml"
    workflow.parent.mkdir(parents=True, exist_ok=True)
    workflow.write_text(
        """
name: "Gate: PR Merge Readiness"
on:
  pull_request:
    branches: [develop, community]
    paths:
      - '.github/workflows/**'
      - '.github/WORKFLOW_GUIDELINES.md'
permissions:
  contents: read
concurrency:
  group: ${{ github.event.pull_request.number || github.ref }}
  cancel-in-progress: true
jobs:
  call-community-failclosed:
    uses: ./.github/workflows/gate-pr-community-failclosed.yml
  call-final-runtime-api:
    if: github.event_name == 'pull_request' && (github.base_ref == 'develop' || github.base_ref == 'community')
    uses: ./.github/workflows/gate-pr-final-runtime-api.yml
  merge-readiness:
    needs:
      - call-community-failclosed
      - call-final-runtime-api
    if: always() && github.event_name == 'pull_request'
    runs-on: ubuntu-latest
    steps:
      - name: Evaluate gate outcomes
        id: evaluate
        run: |
          runtime_result="${{ needs.call-final-runtime-api.result }}"
          runtime_icon=$( [[ "$runtime_result" == 'success' || "$runtime_result" == 'skipped' ]] && echo '✅' || echo '❌' )
          echo "runtime_icon=${runtime_icon}" >> "$GITHUB_OUTPUT"
      - name: Post merge readiness comment
        uses: ./.github/actions/post-failure-comment
        with:
          checklist: |
            - ${{ steps.evaluate.outputs.runtime_icon }} Final Runtime API (`${{ needs.call-final-runtime-api.result }}`)
""".strip() + "\n",
        encoding="utf-8",
    )

    violations = guard.validate_workflow_file(workflow)
    assert not violations

    text = workflow.read_text(encoding="utf-8")
    parsed = yaml.safe_load(text)
    jobs = parsed["jobs"]
    assert "call-final-runtime-api" in jobs
    assert jobs["call-final-runtime-api"]["uses"] == "./.github/workflows/gate-pr-final-runtime-api.yml"
    assert "call-final-runtime-api" in jobs["merge-readiness"]["needs"]
    assert "needs.call-final-runtime-api.result" in text
    assert "Final Runtime API" in text

    repo_workflow = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "gate-pr-merge-readiness.yml"
    if not repo_workflow.exists():
        pytest.skip(f"real workflow not available at {repo_workflow}")
    repo_text = repo_workflow.read_text(encoding="utf-8")
    repo_parsed = yaml.safe_load(repo_text)
    needs_list = repo_parsed["jobs"]["merge-readiness"].get("needs", [])
    if isinstance(needs_list, str):
        needs_list = [needs_list]
    assert "call-final-runtime-api" in repo_parsed["jobs"]
    assert "call-final-runtime-api" in needs_list
    assert "needs.call-final-runtime-api.result" in repo_text
    assert "Final Runtime API" in repo_text