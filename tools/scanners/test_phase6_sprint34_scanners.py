#!/usr/bin/env python3

from __future__ import annotations

import shutil
import tempfile
import unittest
from pathlib import Path

from tools.scanners.gs3_step00_uniform_full import UniformFullScanner
from tools.scanners.gs3_step04_design_const_correctness import ConstCorrectnessApiDesignScan
from tools.scanners.gs3_step04_design_template_meta import TemplateMetaProgrammingScan


class Phase6Sprint34ScannerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.repo_root = Path(__file__).resolve().parents[2]
        ai_working = self.repo_root / "ai_working"
        ai_working.mkdir(parents=True, exist_ok=True)
        self.tmpdir = Path(tempfile.mkdtemp(prefix="phase6-s34-", dir=ai_working))
        (self.tmpdir / "src" / "sample").mkdir(parents=True, exist_ok=True)
        (self.tmpdir / "include" / "sample").mkdir(parents=True, exist_ok=True)

    def tearDown(self) -> None:
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def _write_file(self, rel_path: str, content: str) -> Path:
        path = self.tmpdir / rel_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def test_const_correctness_scanner_detects_key_patterns(self) -> None:
        file_path = self._write_file(
            "include/sample/const_issues.hpp",
            """
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

class CacheView {
public:
    std::vector<int>& items() const;
    int* raw() const;
    void setName(const std::string name);
    void setPath(const std::filesystem::path path);
    void setOptions(const std::optional<int> opt);

    int read() const {
        const_cast<CacheView*>(this)->value_ = 1;
        cache_.push_back(value_);
        return value_;
    }

private:
    mutable std::vector<int> cache_;
    int value_{0};
};
""",
        )

        scanner = ConstCorrectnessApiDesignScan(str(self.tmpdir))
        results = scanner.scan_files([file_path])
        patterns = {item["pattern"] for item in results}

        self.assertIn("mutable_vector_return_const_method", patterns)
        self.assertIn("non_const_ptr_return_const_method", patterns)
        self.assertIn("const_value_param_string", patterns)
        self.assertIn("const_value_param_filesystem_path", patterns)
        self.assertIn("const_value_param_optional", patterns)
        self.assertIn("const_cast_in_const_method", patterns)
        self.assertIn("mutable_member_write_in_const_method", patterns)

    def test_const_correctness_scanner_skips_safe_api_shapes(self) -> None:
        file_path = self._write_file(
            "include/sample/const_safe.hpp",
            """
#include <span>
#include <string>
#include <vector>

class SafeView {
public:
    const std::vector<int>& items() const;
    const int* raw() const;
    void setName(std::string_view name);
    std::span<const int> view() const;
};
""",
        )

        scanner = ConstCorrectnessApiDesignScan(str(self.tmpdir))
        results = scanner.scan_files([file_path])
        self.assertEqual(results, [])

    def test_template_meta_scanner_detects_legacy_patterns(self) -> None:
        file_path = self._write_file(
            "include/sample/template_issues.hpp",
            """
#include <type_traits>

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type oldStyle(T value);

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
struct EnableIfStruct;

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
struct EnumGate;

template <typename T>
using DetectValue = std::void_t<typename T::value_type>;

template <typename T>
using Nested = std::conditional_t<(sizeof(T) > 1), std::conditional_t<(sizeof(T) > 4), int, long>, short>;

template <typename T>
struct Dispatcher : std::integral_constant<bool, std::is_pointer<T>::value> {};

template <typename T> requires std::is_integral_v<T> && sizeof(T) > 1 && std::enable_if_t<true, bool>::value
void mixed(T);

template <>
struct Traits<int> {};
template <>
struct Traits<long> {};
""",
        )

        scanner = TemplateMetaProgrammingScan(str(self.tmpdir))
        results = scanner.scan_files([file_path])
        patterns = {item["pattern"] for item in results}

        self.assertIn("old_enable_if_type_alias", patterns)
        self.assertIn("enable_if_return_type", patterns)
        self.assertIn("enable_if_template_parameter", patterns)
        self.assertIn("enable_if_non_type_parameter", patterns)
        self.assertIn("void_t_detection_idiom", patterns)
        self.assertIn("nested_conditional_t_chain", patterns)
        self.assertIn("bool_constant_dispatch", patterns)
        self.assertIn("trait_guard_without_requires", patterns)
        self.assertIn("requires_enable_if_mixed", patterns)
        self.assertIn("explicit_specialization_cluster", patterns)

    def test_uniform_scanner_emits_phase6_sprint34_findings(self) -> None:
        self._write_file(
            "include/sample/uniform_phase6.hpp",
            """
#include <string>
#include <type_traits>
#include <vector>

class UniformCache {
public:
    std::vector<int>& items() const;
    void setName(const std::string name);
};

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
struct UniformEnableIf;
""",
        )

        scanner = UniformFullScanner(scan_mode="fast")
        results = scanner.scan(str(self.tmpdir))
        gap_types = {gap.type for gap in results}

        self.assertIn("mutable_vector_return_const_method", gap_types)
        self.assertIn("const_value_param_string", gap_types)
        self.assertIn("enable_if_template_parameter", gap_types)


if __name__ == "__main__":
    unittest.main()
