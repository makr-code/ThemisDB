"""Exercise selected CTest registrations without the full dependency build."""

import json
import os
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
    def test_unified_rag_sources_do_not_duplicate_suites(self):
        test_cmake = (ROOT / "tests" / "CMakeLists.txt").read_text(encoding="utf-8")
        selection = test_cmake[
            test_cmake.index("file(GLOB_RECURSE ALL_TEST_SOURCES"):
            test_cmake.index("# Additional GoogleTest sources")
        ]
        prompt = test_cmake[
            test_cmake.index("set(_themis_reincluded_prompt_tests\n"):
            test_cmake.index("# Re-include additional prompt_engineering tests")
        ]
        llm = test_cmake[
            test_cmake.index("if(THEMIS_ENABLE_LLM)\n    target_sources(themis_tests PRIVATE\n        ${CMAKE_CURRENT_SOURCE_DIR}/test_prompt_evaluator.cpp"):
            test_cmake.index("# Re-include the ROCm/HIP backend test")
        ]
        phase11 = (ROOT / "tests" / "test_phase11_lifecycle.cpp").read_text(encoding="utf-8")
        self.assertNotRegex(phase11, r"\bint\s+main\s*\(")
        self.assertIn("TEST(RAGPhase11LifecycleTest, CompleteLifecycle)", phase11)

        with tempfile.TemporaryDirectory(prefix="themis-unified-rag-") as directory:
            source = Path(directory) / "src"
            source.mkdir()
            cmake_source = selection + """
add_library(themis_tests INTERFACE)
function(target_sources target visibility)
    _target_sources(${target} INTERFACE ${ARGN})
endfunction()
""" + prompt + llm + """
get_target_property(reincluded themis_tests INTERFACE_SOURCES)
file(WRITE "${CMAKE_BINARY_DIR}/sources.txt" "${ALL_TEST_SOURCES};${reincluded}")
"""
            cmake_source = cmake_source.replace(
                "${CMAKE_CURRENT_SOURCE_DIR}", (ROOT / "tests").as_posix()
            )
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(UnifiedRagSources NONE)\n" + cmake_source,
                encoding="utf-8",
            )
            for enabled in (False, True):
                with self.subTest(llm_enabled=enabled):
                    build = Path(directory) / f"build-{enabled}"
                    subprocess.run(
                        ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                         f"-DTHEMIS_ENABLE_LLM={'ON' if enabled else 'OFF'}"],
                        text=True, capture_output=True, check=True,
                    )
                    sources = (build / "sources.txt").read_text(encoding="utf-8").split(";")
                    for stem, expected in (("builder", 1), ("injection", int(enabled))):
                        root_copy = str(ROOT / "tests" / f"test_rag_prompt_{stem}.cpp")
                        module_copy = str(ROOT / "tests" / "rag" / f"test_rag_prompt_{stem}.cpp")
                        self.assertNotIn(root_copy, sources)
                        self.assertEqual(sources.count(module_copy), expected)

    def test_module_dependencies_and_scraper_includes(self):
        test_sources = {
            "chaos": ("test_chaos_network.cpp",),
            "content": ("test_content_metrics.cpp",),
            "ethics": ("test_ethics_ai_pipeline.cpp",),
            "scraper": ("test_scraper_plugin.cpp", "test_scraper_v12_features_focused.cpp"),
        }
        with tempfile.TemporaryDirectory(prefix="themis-module-registration-") as directory:
            source = Path(directory) / "src"
            source.mkdir()
            for module, names in test_sources.items():
                module_dir = source / module
                module_dir.mkdir()
                shutil.copyfile(ROOT / "tests" / module / "CMakeLists.txt",
                                module_dir / "CMakeLists.txt")
                for name in names:
                    (module_dir / name).touch()
            (source / "CMakeLists.txt").write_text(
                f"""cmake_minimum_required(VERSION 3.20)
project(ModuleRegistrationContracts NONE)
set(THEMIS_ROOT_DIR [[{ROOT}]])
add_library(themis_core INTERFACE)
add_library(spdlog::spdlog INTERFACE IMPORTED)
add_library(Threads::Threads INTERFACE IMPORTED)
add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
foreach(module IN ITEMS network content query storage)
    if(ENABLE_${{module}})
        add_library(themis_${{module}} INTERFACE)
    endif()
endforeach()
function(add_executable target)
    add_library(${{target}} INTERFACE)
endfunction()
function(target_include_directories target visibility)
    _target_include_directories(${{target}} INTERFACE ${{ARGN}})
endfunction()
function(target_link_libraries target visibility)
    _target_link_libraries(${{target}} INTERFACE ${{ARGN}})
endfunction()
function(target_sources)
endfunction()
function(target_compile_definitions)
endfunction()
function(target_compile_features)
endfunction()
function(themis_register_module_focused_test)
endfunction()
function(add_test)
endfunction()
function(set_tests_properties)
endfunction()
foreach(module IN ITEMS chaos content ethics scraper)
    add_subdirectory(${{module}})
endforeach()
function(check_link target dependency enabled)
    get_target_property(links ${{target}} INTERFACE_LINK_LIBRARIES)
    if(NOT "themis_core" IN_LIST links)
        message(FATAL_ERROR "${{target}} is missing themis_core")
    endif()
    if(enabled AND NOT "${{dependency}}" IN_LIST links)
        message(FATAL_ERROR "${{target}} is missing ${{dependency}}")
    elseif(NOT enabled AND "${{dependency}}" IN_LIST links)
        message(FATAL_ERROR "${{target}} links missing ${{dependency}}")
    endif()
endfunction()
check_link(module_chaos_test_chaos_network_focused themis_network "${{ENABLE_network}}")
check_link(module_content_test_content_metrics_focused themis_content "${{ENABLE_content}}")
check_link(module_ethics_test_ethics_ai_pipeline_focused themis_query "${{ENABLE_query}}")
check_link(module_ethics_test_ethics_ai_pipeline_focused themis_storage "${{ENABLE_storage}}")
get_target_property(plugin_includes module_scraper_test_scraper_plugin_focused INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(other_includes module_scraper_test_scraper_v12_features_focused_focused INTERFACE_INCLUDE_DIRECTORIES)
set(plugin_dir "${{THEMIS_ROOT_DIR}}/plugins/scraper/include")
if(NOT plugin_dir IN_LIST plugin_includes OR plugin_dir IN_LIST other_includes)
    message(FATAL_ERROR "Scraper plugin include is missing or leaked to another target")
endif()
""",
                encoding="utf-8",
            )
            for enabled in ((), ("network", "content", "query", "storage"),
                            ("query",), ("storage",)):
                with self.subTest(enabled=enabled):
                    build = Path(directory) / ("build-" + ("-".join(enabled) or "none"))
                    subprocess.run(
                        ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                         *(f"-DENABLE_{module}=ON" for module in enabled)],
                        text=True, capture_output=True, check=True,
                    )

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

    def test_llm_model_path_environment(self):
        source_text = (ROOT / "tests/llm/CMakeLists.txt").read_text(encoding="utf-8")
        start = source_text.index("option(THEMIS_LLM_INFERENCE_TESTS")
        end = source_text.index("endif() # THEMIS_LLM_INFERENCE_TESTS", start)
        registration = source_text[start:end + len("endif() # THEMIS_LLM_INFERENCE_TESTS")]
        test_names = (
            "test_llm_tinyllama_inference_LlmInferenceCITests",
            "test_llm_doku_rag_LlmInferenceCITests",
            "test_llm_adalora_doku_training_LlmInferenceCITests",
        )

        with tempfile.TemporaryDirectory(prefix="themis-model-path-") as directory:
            root = Path(directory)
            source = root / "source"
            source.mkdir()
            (source / "data").mkdir()
            (source / "data/themisdb_rag_golden_dataset.yaml").touch()
            (source / "probe.py").write_text(
                "import os, sys\n"
                "from pathlib import Path\n"
                "actual = os.environ.get('THEMIS_TEST_MODEL_PATH')\n"
                "print('model path:', actual)\n"
                "valid = actual == sys.argv[1]\n"
                "if sys.argv[2] == 'configured':\n"
                "    valid = valid and Path(actual).is_file()\n"
                "sys.exit(0 if valid else 1)\n",
                encoding="utf-8",
            )
            # Keep the actual registration intact; replace only dependencies and
            # executables with a probe so CTest itself exercises ENVIRONMENT.
            (source / "CMakeLists.txt").write_text(
                f"""cmake_minimum_required(VERSION 3.20)
project(ModelPathContract NONE)
enable_testing()
set(THEMIS_ROOT_DIR [[{ROOT.as_posix()}]])
set(THEMIS_ENABLE_LLM ON)
set(THEMIS_BUILD_MODULAR OFF)
function(add_executable target)
    _add_executable(${{target}} IMPORTED GLOBAL)
    set_target_properties(${{target}} PROPERTIES IMPORTED_LOCATION [[{Path(sys.executable).as_posix()}]])
endfunction()
function(target_include_directories)
endfunction()
function(target_link_libraries)
endfunction()
function(target_sources)
endfunction()
function(target_compile_definitions)
endfunction()
function(add_custom_command)
endfunction()
function(themis_llm_sync_runtime_dlls)
endfunction()
function(themis_llm_rescan_sharding_dependencies)
endfunction()
function(themis_register_module_focused_test)
    cmake_parse_arguments(REG "" "NAME" "" ${{ARGN}})
    add_test(NAME "${{REG_NAME}}"
        COMMAND [[{Path(sys.executable).as_posix()}]] "${{CMAKE_CURRENT_SOURCE_DIR}}/probe.py" "${{CONTRACT_EXPECTED_PATH}}" "${{CONTRACT_MODE}}")
endfunction()
""" + registration + "\n",
                encoding="utf-8",
            )
            inherited = "inherited-model-path"
            environment = dict(os.environ, THEMIS_TEST_MODEL_PATH=inherited)
            build = root / "build"
            for model_dir in (None, root / "model files", root / "replacement model"):
                with self.subTest(model_dir=model_dir):
                    expected = (
                        f"{model_dir.as_posix()}/tinyllama.gguf"
                        if model_dir is not None else inherited
                    )
                    if model_dir is not None:
                        model_dir.mkdir()
                        (model_dir / "tinyllama.gguf").touch()
                    subprocess.run(
                        ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                         "-DTHEMIS_LLM_INFERENCE_TESTS=ON",
                         f"-DTHEMIS_TEST_MODEL_DIR={model_dir.as_posix() if model_dir else ''}",
                         f"-DCONTRACT_EXPECTED_PATH={expected}",
                         f"-DCONTRACT_MODE={'configured' if model_dir else 'inherited'}"],
                        env=environment, text=True, capture_output=True, check=True,
                    )
                    listing = subprocess.run(
                        ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
                        env=environment, text=True, capture_output=True, check=True,
                    )
                    completed = subprocess.run(
                        ["ctest", "--test-dir", str(build), "--output-on-failure"],
                        env=environment, text=True, capture_output=True,
                    )
                    self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
                    tests = {test["name"]: test for test in json.loads(listing.stdout)["tests"]}
                    for name in test_names:
                        properties = {p["name"]: p["value"] for p in tests[name]["properties"]}
                        variables = properties.get("ENVIRONMENT", [])
                        model_variables = [
                            item for item in variables if item.startswith("THEMIS_TEST_MODEL_PATH=")
                        ]
                        self.assertEqual(
                            model_variables,
                            [f"THEMIS_TEST_MODEL_PATH={expected}"] if model_dir else [],
                            name,
                        )
                        if name == "test_llm_doku_rag_LlmInferenceCITests":
                            self.assertTrue(any(
                                item.startswith("THEMIS_RAG_GOLDEN_DATASET_PATH=")
                                for item in variables
                            ))


if __name__ == "__main__":
    unittest.main()
