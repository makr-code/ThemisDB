"""Exercise selected CTest registrations without the full dependency build."""

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(
    all(shutil.which(tool) for tool in ("cmake", "ctest", "ninja")),
    "CMake, CTest and Ninja are required",
)
class CMakeRegistrationContracts(unittest.TestCase):
    def test_registration_with_and_without_python(self):
        test_cmake = (ROOT / "tests" / "CMakeLists.txt").read_text(encoding="utf-8")
        storage_cmake = (ROOT / "tests" / "storage" / "CMakeLists.txt").read_text(encoding="utf-8")
        postgres = test_cmake[
            test_cmake.index("add_test(\n    NAME PostgresImporterFocusedTests"):
            test_cmake.index("# PostgreSQL Importer v2.1+ Focused Tests")
        ]
        auth_start = test_cmake.index("# Auth Rate Limiter Distributed Tests")
        auth = test_cmake[auth_start:test_cmake.index("\n\nadd_auth_focused_alias(", auth_start)]
        storage = storage_cmake[
            storage_cmake.index("find_package(Python3"):
            storage_cmake.index("\nfile(GLOB STORAGE_MODULE_TEST_SOURCES")
        ]
        children = "\n".join(
            f"add_test(NAME test_postgres_importer_{name}_postgres_FocusedTests "
            f'COMMAND "${{CMAKE_COMMAND}}" -E echo {name})'
            for name in ("fk", "security_audit", "streaming")
        )

        with tempfile.TemporaryDirectory(prefix="themis-registration-") as directory:
            source = Path(directory) / "src"
            (source / "auth").mkdir(parents=True)
            (source / "auth" / "test_auth_rate_limiter_distributed.cpp").touch()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(RegistrationContracts NONE)\n"
                "enable_testing()\n"
                "add_executable(module_auth_test_auth_rate_limiter_distributed_focused IMPORTED)\n"
                "set_target_properties(module_auth_test_auth_rate_limiter_distributed_focused "
                'PROPERTIES IMPORTED_LOCATION "${CMAKE_COMMAND}")\n'
                "add_executable(test_wal_process_crash_worker IMPORTED)\n"
                "set_target_properties(test_wal_process_crash_worker "
                'PROPERTIES IMPORTED_LOCATION "${CMAKE_COMMAND}")\n'
                + children + "\n" + postgres + "\n" + auth + "\n" + storage,
                encoding="utf-8",
            )
            for generator in ("Ninja Multi-Config", "Ninja"):
                for use_python in (True, False):
                    with self.subTest(generator=generator, use_python=use_python):
                        has_python = use_python and sys.version_info >= (3, 10)
                        build = Path(directory) / f"{generator.replace(' ', '-')}-{use_python}"
                        args = ["cmake", "-S", str(source), "-B", str(build), "-G", generator]
                        args.append(
                            f"-DPython3_EXECUTABLE={sys.executable}"
                            if use_python else "-DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE"
                        )
                        configured = subprocess.run(args, text=True, capture_output=True, check=True)
                        listing = subprocess.run(
                            ["ctest", "--test-dir", str(build), "-C", "Debug", "--show-only=json-v1"],
                            text=True, capture_output=True, check=True,
                        )
                        tests = {test["name"]: test for test in json.loads(listing.stdout)["tests"]}
                        properties = {
                            prop["name"]: prop["value"]
                            for prop in tests["AuthRateLimiterDistributedTests"]["properties"]
                        }
                        self.assertEqual(
                            set(properties["LABELS"]),
                            {"auth", "rate-limiter", "distributed", "backend", "integration"},
                        )
                        self.assertEqual(properties["TIMEOUT"], 60.0)
                        for name in ("WALProcessCrashRecoveryTests", "WALProcessUnobservedAppendTests"):
                            self.assertEqual(name in tests, has_python)
                        if not has_python:
                            self.assertIn(
                                "WAL process-crash tests are not registered",
                                " ".join(configured.stderr.split()),
                            )
                        completed = subprocess.run(
                            ["ctest", "--test-dir", str(build), "-C", "Debug", "-V",
                             "-R", "^PostgresImporterFocusedTests$"],
                            text=True, capture_output=True, check=True,
                        )
                        for name in ("fk", "security_audit", "streaming"):
                            self.assertIn(
                                f"test_postgres_importer_{name}_postgres_FocusedTests",
                                completed.stdout,
                            )
                        self.assertIn("100% tests passed", completed.stdout)


if __name__ == "__main__":
    unittest.main()
