#!/usr/bin/env python3
"""Compile and execute the real 2PC contract without ThemisDB dependencies."""

import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
COMPILERS = [path for name in ("g++", "clang++") if (path := shutil.which(name))]
HEADER = '#include "sharding/detail/two_phase_commit_contract.h"\n'
MACRO_CHECK = """
#ifndef FAILURE
#error The SDK macro must remain defined
#endif
static_assert(FAILURE == 0);
"""
CONTRACT_PROGRAM = r"""
#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using themis::sharding::detail::executeTwoPhaseCommit;
using Result = themis::sharding::detail::TwoPhaseCommitExecutionResult;

static_assert(static_cast<int>(Result::COMMITTED) == 0);
static_assert(static_cast<int>(Result::ABORTED) == 1);
static_assert(static_cast<int>(Result::EXECUTION_FAILED) == 2);

enum class Outcome { SUCCEED, RETURN_FALSE, THROW };

bool invoke(Outcome outcome) {
    if (outcome == Outcome::THROW) {
        throw std::runtime_error("callback failed");
    }
    return outcome == Outcome::SUCCEED;
}

int main() {
    // Exercise both decisions and every callback failure/exception combination.
    for (auto prepare : {Outcome::SUCCEED, Outcome::RETURN_FALSE, Outcome::THROW}) {
        for (auto persist : {Outcome::SUCCEED, Outcome::RETURN_FALSE, Outcome::THROW}) {
            for (auto apply : {Outcome::SUCCEED, Outcome::RETURN_FALSE, Outcome::THROW}) {
                const bool commit = prepare == Outcome::SUCCEED;
                std::vector<std::string> events;
                std::optional<bool> recorded_decision;
                const auto result = executeTwoPhaseCommit(
                    [&] {
                        events.emplace_back("prepare");
                        return invoke(prepare);
                    },
                    [&](bool decision) {
                        assert(decision == commit);
                        events.emplace_back(decision ? "persist-commit" : "persist-abort");
                        if (!invoke(persist)) {
                            return false;
                        }
                        recorded_decision = decision;
                        return true;
                    },
                    [&](bool decision) {
                        assert(recorded_decision.has_value());
                        assert(decision == *recorded_decision);
                        assert(decision == commit);
                        events.emplace_back(decision ? "apply-commit" : "apply-abort");
                        return invoke(apply);
                    });

                std::vector<std::string> expected{
                    "prepare", commit ? "persist-commit" : "persist-abort"};
                if (persist == Outcome::SUCCEED) {
                    expected.emplace_back(commit ? "apply-commit" : "apply-abort");
                    // Even failed/thrown application leaves the decision authoritative.
                    assert(recorded_decision.has_value());
                    assert(*recorded_decision == commit);
                } else {
                    assert(!recorded_decision.has_value());
                }
                assert(events == expected);
                const auto expected_result =
                    persist != Outcome::SUCCEED || apply != Outcome::SUCCEED
                        ? Result::EXECUTION_FAILED
                        : (commit ? Result::COMMITTED : Result::ABORTED);
                assert(result == expected_result);
            }
        }
    }
}
"""


@unittest.skipUnless(COMPILERS, "requires g++ or clang++ with C++20 support")
class TwoPhaseCommitContractTests(unittest.TestCase):
    def compile_and_run(self, before_header: str = "", after_header: str = "") -> None:
        for compiler in COMPILERS:
            with self.subTest(compiler=compiler), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                source = root / "contract.cpp"
                executable = root / "contract"
                source.write_text(
                    before_header + HEADER + after_header + textwrap.dedent(CONTRACT_PROGRAM),
                    encoding="utf-8",
                )
                command = [
                    compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(REPO_ROOT / "include"),
                    str(source), "-o", str(executable),
                ]
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
                result = subprocess.run(
                    [str(executable)], capture_output=True, text=True, timeout=10
                )
                self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)

    def test_normal_header(self) -> None:
        self.compile_and_run(after_header="""
            #ifdef FAILURE
            #error The contract must not introduce an SDK macro
            #endif
        """)

    def test_failure_macro_before_header(self) -> None:
        self.compile_and_run(before_header="#define FAILURE 0\n", after_header=MACRO_CHECK)

    def test_failure_macro_after_header(self) -> None:
        self.compile_and_run(after_header="#define FAILURE 0\n" + MACRO_CHECK)


if __name__ == "__main__":
    unittest.main()
