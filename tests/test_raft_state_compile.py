#!/usr/bin/env python3

import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(
    os.environ.get("THEMIS_SOURCE_ROOT", Path(__file__).resolve().parent.parent)
).resolve()


class RaftStateCompileRegressionTests(unittest.TestCase):
    def test_raft_state_compiles_standalone_in_release_mode(self) -> None:
        compiler = shutil.which("g++") or shutil.which("c++")
        if compiler is None:
            self.skipTest("Requires a C++20 compiler (g++ or c++)")

        include_flags = []
        pkg_config = shutil.which("pkg-config")
        if pkg_config:
            for package in ("spdlog", "nlohmann_json"):
                result = subprocess.run(
                    [pkg_config, "--cflags-only-I", package],
                    capture_output=True, text=True, timeout=30,
                )
                if result.returncode == 0:
                    include_flags.extend(shlex.split(result.stdout))

        flags = [
            "-std=c++20", "-O3", "-DNDEBUG",
            "-DSPDLOG_COMPILED_LIB", "-DSPDLOG_FMT_EXTERNAL",
            "-DSPDLOG_DISABLE_DEFAULT_LOGGER", "-DSPDLOG_NO_EXCEPTIONS",
            *include_flags,
        ]
        # Probe only external dependencies: a broken repository source must fail,
        # not be mistaken for missing system headers and skipped.
        for header in ("spdlog/spdlog.h", "fmt/format.h", "nlohmann/json.hpp"):
            probe = subprocess.run(
                [compiler, *flags, "-x", "c++", "-E", "-"],
                input=f"#include <{header}>\n",
                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                text=True, timeout=30,
            )
            if probe.returncode != 0:
                self.skipTest(
                    f"Requires usable C++ dependency header <{header}> "
                    "(libspdlog-dev, libfmt-dev, nlohmann-json3-dev): "
                    f"{probe.stderr.strip()}"
                )

        with tempfile.TemporaryDirectory() as temporary:
            object_file = Path(temporary) / "raft_state.o"
            command = [
                compiler, *flags, "-I", str(REPO_ROOT / "include"),
                "-c", str(REPO_ROOT / "src/sharding/raft_state.cpp"),
                "-o", str(object_file),
            ]
            result = subprocess.run(
                command, capture_output=True, text=True, timeout=120,
            )
            self.assertEqual(
                result.returncode, 0,
                f"Standalone Release compilation failed:\n"
                f"{shlex.join(command)}\n{result.stdout}{result.stderr}",
            )
            self.assertTrue(object_file.is_file(), "Compiler did not produce raft_state.o")


if __name__ == "__main__":
    unittest.main()
