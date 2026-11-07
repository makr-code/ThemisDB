import json
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]


class WindowsReleaseBuildMemoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        workflow = yaml.safe_load(
            (ROOT / ".github/workflows/release-build-matrix.yml").read_text()
        )
        script = workflow["jobs"]["prepare-release-matrix"]["steps"][0]["run"]
        script = script.replace(' >> "$GITHUB_OUTPUT"', "")
        process = subprocess.run(
            ["bash", "-euo", "pipefail", "-c", script],
            check=True, capture_output=True, text=True,
            env={**os.environ, "BUILD_MATRIX": "all", "OS_SELECTOR": "all",
                 "ARCH_SELECTOR": "all", "EDITION": "community",
                 "BUILD_TYPE": "Release", "PACKAGING": "auto",
                 "DISTRIBUTION": "all", "RUN_NUMBER": "1"},
        )
        cls.lanes = {
            lane["id"]: lane
            for lane in json.loads(process.stdout.split("matrix=", 1)[1])["include"]
        }

    def run_windows_command(self, exit_code, command="build"):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            cmake = root / "cmake"
            cmake.write_text(
                "#!/bin/bash\n"
                'printf "parallel=%s\\nvcpkg=%s\\n" "$CMAKE_BUILD_PARALLEL_LEVEL" '
                '"$VCPKG_MAX_CONCURRENCY"\n'
                'printf "arg=%s\\n" "$@"\n'
                'exit "$TEST_BUILD_EXIT_CODE"\n'
            )
            cmake.chmod(0o755)
            result = subprocess.run(
                ["bash", "-c", self.lanes["windows-amd64"][f"{command}_command"]],
                cwd=root, capture_output=True, text=True,
                env={**os.environ, "PATH": f"{root}{os.pathsep}{os.environ['PATH']}",
                     "CMAKE_BUILD_PARALLEL_LEVEL": "8",
                     "VCPKG_MAX_CONCURRENCY": "8",
                     "TEST_BUILD_EXIT_CODE": str(exit_code)},
            )
            log = (root / f"{command}-windows-release.log").read_text()
            return result, log

    def test_windows_build_serializes_compilers_even_with_inherited_parallelism(self):
        result, log = self.run_windows_command(0)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(log, result.stdout)
        self.assertEqual(
            log.splitlines(),
            ["parallel=1", "vcpkg=1", "arg=--build", "arg=--preset",
             "arg=windows-release", "arg=--parallel", "arg=1"],
        )

    def test_windows_configure_uses_modest_independent_dependency_parallelism(self):
        result, log = self.run_windows_command(0, command="configure")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(log, result.stdout)
        self.assertEqual(
            log.splitlines()[:4],
            ["parallel=2", "vcpkg=2", "arg=--preset", "arg=windows-release"],
        )

    def test_compiler_failure_is_not_hidden_by_tee(self):
        result, log = self.run_windows_command(23)
        self.assertEqual(result.returncode, 23)
        self.assertEqual(log, result.stdout)

    def test_configure_failure_is_not_hidden_by_tee(self):
        result, log = self.run_windows_command(23, command="configure")
        self.assertEqual(result.returncode, 23)
        self.assertEqual(log, result.stdout)

    def test_other_platforms_keep_their_build_parallelism(self):
        for lane, parallelism in [("linux-amd64", "16"), ("linux-arm64", "8"),
                                  ("macos-arm64", "8")]:
            with self.subTest(lane=lane):
                command = next(
                    line for line in self.lanes[lane]["build_command"].splitlines()
                    if line.startswith("cmake --build")
                )
                args = shlex.split(command)
                self.assertEqual(args[args.index("--parallel") + 1], parallelism)


if __name__ == "__main__":
    unittest.main()
