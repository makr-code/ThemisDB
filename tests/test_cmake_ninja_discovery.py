#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
WORKFLOW = REPO_ROOT / ".github" / "workflows" / "reusable-cmake-build.yml"


@unittest.skipUnless(
    sys.platform.startswith("linux") and shutil.which("cmake") and shutil.which("ninja"),
    "requires Linux, CMake and Ninja",
)
class CMakeNinjaDiscoveryTests(unittest.TestCase):
    def run_discovery(self, root: Path, install_system_ninja: bool = True) -> dict[str, str]:
        ninja = Path(shutil.which("ninja")).resolve()
        system_ninja = root / "system" / "ninja"
        system_ninja.parent.mkdir()
        if install_system_ninja:
            system_ninja.symlink_to(ninja)

        shadow_bin = root / "shadow"
        shadow_bin.mkdir()
        shadow_ninja = shadow_bin / "ninja"
        shadow_ninja.write_text(
            '#!/bin/sh\nif [ "$1" = "--version" ]; then echo 1.11.1; else exit 99; fi\n',
            encoding="utf-8",
        )
        shadow_ninja.chmod(0o755)
        sudo = shadow_bin / "sudo"
        sudo.write_text(
            '#!/bin/sh\n'
            'echo "$*" >> "$INSTALL_LOG"\n'
            'if [ "$2" = install ]; then ln -sf "$REAL_NINJA" "$SYSTEM_NINJA"; fi\n',
            encoding="utf-8",
        )
        sudo.chmod(0o755)

        env = os.environ.copy()
        env.update(
            PATH=f"{shadow_bin}{os.pathsep}{os.environ['PATH']}",
            RUNNER_TEMP=str(root),
            GITHUB_ENV=str(root / "github_env"),
            GITHUB_PATH=str(root / "github_path"),
            REAL_NINJA=str(ninja),
            SYSTEM_NINJA=str(system_ninja),
            INSTALL_LOG=str(root / "install.log"),
        )
        workflow = WORKFLOW.read_text(encoding="utf-8")
        step = workflow.split("      - name: Ensure Ninja binary is discoverable (Linux)\n", 1)[1]
        step = step.split("      - name:", 1)[0]
        script = textwrap.dedent(step.split("        run: |\n", 1)[1])
        script = script.replace("/usr/bin/ninja", str(system_ninja))
        result = subprocess.run(["bash", "-c", script], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

        github_path = Path(env["GITHUB_PATH"])
        self.assertTrue(github_path.exists(), "validated Ninja must be published to PATH")
        additions = github_path.read_text(encoding="utf-8").splitlines()
        env["PATH"] = os.pathsep.join([*reversed(additions), env["PATH"]])
        for line in Path(env["GITHUB_ENV"]).read_text(encoding="utf-8").splitlines():
            key, value = line.split("=", 1)
            env[key] = value
        self.assertEqual(Path(shutil.which("ninja", path=env["PATH"])).resolve(), ninja)
        return env

    def test_validated_ninja_is_used_by_cmake_and_nested_configure(self) -> None:
        cmake = shutil.which("cmake")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            env = self.run_discovery(root)
            source = root / "source"
            nested = source / "nested"
            nested.mkdir(parents=True)
            (nested / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\n'
                'project(NestedNinja NONE)\n'
                'add_custom_target(smoke ALL COMMAND "${CMAKE_COMMAND}" -E echo nested-ok)\n',
                encoding="utf-8",
            )
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\n'
                'project(NinjaDiscovery NONE)\n'
                'execute_process(COMMAND "${CMAKE_COMMAND}" -G Ninja\n'
                '  -S "${CMAKE_CURRENT_SOURCE_DIR}/nested" -B "${CMAKE_BINARY_DIR}/nested"\n'
                '  COMMAND_ERROR_IS_FATAL ANY)\n',
                encoding="utf-8",
            )
            for command in (
                [cmake, "-G", "Ninja", "-S", str(source), "-B", str(root / "build")],
                [cmake, "--build", str(root / "build" / "nested")],
            ):
                result = subprocess.run(command, env=env, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_missing_system_ninja_is_installed_even_when_shadow_passes_version_check(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.run_discovery(root, install_system_ninja=False)
            self.assertIn("apt-get install", (root / "install.log").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
