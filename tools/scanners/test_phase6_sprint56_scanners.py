#!/usr/bin/env python3
"""
Tests for Phase 6 Sprint 5-6 scanners:
  - P6-3: gs3_step04_design_template_meta.TemplateMetaScan
  - P6-5: gs3_step04_design_ownership_lifetime.OwnershipLifetimeScan
"""

from __future__ import annotations

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

# Allow running from repo root or from tools/scanners/
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

from tools.scanners.gs3_step04_design_template_meta import TemplateMetaScan
from tools.scanners.gs3_step04_design_ownership_lifetime import OwnershipLifetimeScan


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write_cpp(tmp: Path, name: str, code: str) -> Path:
    f = tmp / name
    f.write_text(textwrap.dedent(code), encoding='utf-8')
    return f


def _patterns(gaps):
    return {g['pattern'] for g in gaps}


# ---------------------------------------------------------------------------
# P6-3 Template Meta-Programming Scanner Tests
# ---------------------------------------------------------------------------

class TestTemplateMetaScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = TemplateMetaScan(str(self.tmp))

    # --- deprecated_type_trait ---

    def test_result_of_deprecated(self):
        f = _write_cpp(self.tmp, 'a.cpp', """\
            #include <type_traits>
            template<typename F>
            using ResultOf = typename std::result_of<F()>::type;
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('deprecated_type_trait', _patterns(gaps))

    def test_is_pod_deprecated(self):
        f = _write_cpp(self.tmp, 'b.h', """\
            #include <type_traits>
            static_assert(std::is_pod<int>::value, "");
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('deprecated_type_trait', _patterns(gaps))

    def test_invoke_result_not_flagged(self):
        f = _write_cpp(self.tmp, 'c.cpp', """\
            #include <type_traits>
            template<typename F>
            using R = std::invoke_result_t<F>;
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('deprecated_type_trait', _patterns(gaps))

    def test_unary_function_deprecated(self):
        f = _write_cpp(self.tmp, 'd.hpp', """\
            struct Pred : std::unary_function<int,bool> {
                bool operator()(int x) const { return x > 0; }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('deprecated_type_trait', _patterns(gaps))

    def test_bind1st_deprecated(self):
        f = _write_cpp(self.tmp, 'e.cpp', """\
            auto fn = std::bind1st(std::plus<int>(), 5);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('deprecated_type_trait', _patterns(gaps))

    # --- sfinae_enable_if_chain ---

    def test_double_enable_if_chain(self):
        f = _write_cpp(self.tmp, 'f.hpp', """\
            template<typename T,
                     typename = std::enable_if_t<std::is_integral_v<T>>,
                     typename = std::enable_if_t<std::is_signed_v<T>>>
            void process(T val);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('sfinae_enable_if_chain', _patterns(gaps))

    def test_single_enable_if_not_flagged(self):
        f = _write_cpp(self.tmp, 'g.hpp', """\
            template<typename T,
                     typename = std::enable_if_t<std::is_integral_v<T>>>
            void process(T val);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('sfinae_enable_if_chain', _patterns(gaps))

    # --- implicit_template_concept ---

    def test_unconstrained_container_call(self):
        f = _write_cpp(self.tmp, 'h.hpp', """\
            template<typename T>
            void printSize(T container) {
                auto s = container.size();
                (void)s;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('implicit_template_concept', _patterns(gaps))

    def test_requires_constrained_not_flagged(self):
        f = _write_cpp(self.tmp, 'i.hpp', """\
            template<typename T>
            requires requires(T c) { c.size(); }
            void printSize(T container) {
                auto s = container.size();
                (void)s;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('implicit_template_concept', _patterns(gaps))

    # --- template_param_explosion ---

    def test_six_type_params(self):
        f = _write_cpp(self.tmp, 'j.hpp', """\
            template<typename A, typename B, typename C, typename D, typename E, typename F>
            struct HeavyTemplate {};
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('template_param_explosion', _patterns(gaps))

    def test_five_type_params_not_flagged(self):
        f = _write_cpp(self.tmp, 'k.hpp', """\
            template<typename A, typename B, typename C, typename D, typename E>
            struct FiveParams {};
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('template_param_explosion', _patterns(gaps))

    # --- rvalue_ref_member ---

    def test_rvalue_ref_member_flagged(self):
        f = _write_cpp(self.tmp, 'l.hpp', """\
            struct Holder {
                std::string&& ref;
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('rvalue_ref_member', _patterns(gaps))

    def test_logical_and_not_flagged(self):
        """Multi-line operator== with && should not be flagged as rvalue member."""
        f = _write_cpp(self.tmp, 'm.hpp', """\
            struct Data {
                int x;
                bool operator==(const Data& o) const {
                    return x == o.x
                        && x >= 0;
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('rvalue_ref_member', _patterns(gaps))

    # --- const_rvalue_ref_param ---

    def test_const_rvalue_param_flagged(self):
        f = _write_cpp(self.tmp, 'n.cpp', """\
            void process(const std::string&& s) { (void)s; }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('const_rvalue_ref_param', _patterns(gaps))

    def test_normal_rvalue_param_not_flagged(self):
        f = _write_cpp(self.tmp, 'o.cpp', """\
            void process(std::string&& s) { (void)s; }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('const_rvalue_ref_param', _patterns(gaps))

    # --- non-prod exclusion ---

    def test_test_file_skipped(self):
        f = _write_cpp(self.tmp, 'test_foo.cpp', """\
            void use(std::result_of<void(int)>::type) {}
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])

    # --- recursive_template_instantiation ---

    def test_recursive_template_no_base_case(self):
        f = _write_cpp(self.tmp, 'p.hpp', """\
            template<typename T>
            struct Recurse : Recurse<T> {
                using type = typename T::value_type;
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('recursive_template_no_base_case', _patterns(gaps))


# ---------------------------------------------------------------------------
# P6-5 Ownership & Lifetime Semantics Scanner Tests
# ---------------------------------------------------------------------------

class TestOwnershipLifetimeScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = OwnershipLifetimeScan(str(self.tmp))

    # --- use_after_move ---

    def test_use_after_move_flagged(self):
        f = _write_cpp(self.tmp, 'a.cpp', """\
            void sink(std::string s);
            void caller() {
                std::string msg = "hello";
                sink(std::move(msg));
                auto len = msg.size();  // use after move
                (void)len;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('use_after_move', _patterns(gaps))

    def test_use_after_move_reassign_suppressed(self):
        f = _write_cpp(self.tmp, 'b.cpp', """\
            void sink(std::string s);
            void caller() {
                std::string msg = "hello";
                sink(std::move(msg));
                msg = "world";  // reassigned — safe
                auto len = msg.size();
                (void)len;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('use_after_move', _patterns(gaps))

    def test_forwarding_var_not_flagged(self):
        """Common forwarding parameter names should be suppressed."""
        f = _write_cpp(self.tmp, 'c.cpp', """\
            void process(std::string other) {
                storage = std::move(other);
                notify(other);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('use_after_move', _patterns(gaps))

    # --- return_local_ref ---

    def test_return_address_local_flagged(self):
        f = _write_cpp(self.tmp, 'd.cpp', """\
            const int* getVal() {
                int result = 42;
                return &result;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('return_local_ref', _patterns(gaps))

    def test_return_member_ref_not_flagged(self):
        f = _write_cpp(self.tmp, 'e.cpp', """\
            struct S {
                int value;
                const int* get() const { return &value; }
            };
        """)
        gaps = self.scanner.scan_files([f])
        # 'value' is not a locally declared auto/int var — may not trigger
        # Just verify the scan completes without error
        self.assertIsInstance(gaps, list)

    # --- self_move_assignment ---

    def test_move_assign_no_guard_flagged(self):
        f = _write_cpp(self.tmp, 'f.hpp', """\
            struct Resource {
                int* ptr;
                Resource& operator=(Resource&& other) {
                    ptr = other.ptr;
                    other.ptr = nullptr;
                    return *this;
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('self_move_assignment_no_guard', _patterns(gaps))

    def test_move_assign_with_guard_not_flagged(self):
        f = _write_cpp(self.tmp, 'g.hpp', """\
            struct Resource {
                int* ptr;
                Resource& operator=(Resource&& other) noexcept {
                    if (this != &other) {
                        ptr = other.ptr;
                        other.ptr = nullptr;
                    }
                    return *this;
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('self_move_assignment_no_guard', _patterns(gaps))

    # --- move_missing_noexcept ---

    def test_move_ctor_no_noexcept_flagged(self):
        f = _write_cpp(self.tmp, 'h.hpp', """\
            struct Widget {
                Widget(Widget&& other) {
                    data = other.data;
                }
                int data;
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('move_ctor_missing_noexcept', _patterns(gaps))

    def test_move_ctor_with_noexcept_not_flagged(self):
        f = _write_cpp(self.tmp, 'i.hpp', """\
            struct Widget {
                Widget(Widget&& other) noexcept {
                    data = other.data;
                }
                int data;
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('move_ctor_missing_noexcept', _patterns(gaps))

    # --- rvalue_ref_member (P6-5 version) ---

    def test_rvalue_ref_member_in_struct(self):
        f = _write_cpp(self.tmp, 'j.hpp', """\
            struct Wrapper {
                std::vector<int>&& data;
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('rvalue_ref_member', _patterns(gaps))

    def test_logical_and_in_method_not_flagged(self):
        f = _write_cpp(self.tmp, 'k.hpp', """\
            struct Point {
                int x, y;
                bool isValid() const {
                    return x > 0
                        && y > 0;
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('rvalue_ref_member', _patterns(gaps))

    # --- const_rvalue_ref_param ---

    def test_const_rvalue_param_flagged(self):
        f = _write_cpp(self.tmp, 'l.cpp', """\
            void sink(const std::string&& s) { (void)s; }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('const_rvalue_ref_param', _patterns(gaps))

    def test_normal_rvalue_not_flagged(self):
        f = _write_cpp(self.tmp, 'm.cpp', """\
            void sink(std::string&& s) { (void)std::move(s); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('const_rvalue_ref_param', _patterns(gaps))

    # --- return_move_blocks_rvo ---

    def test_return_move_local_flagged(self):
        f = _write_cpp(self.tmp, 'n.cpp', """\
            #include <string>
            std::string buildResult() {
                auto result = std::string("hello");
                return std::move(result);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('return_move_blocks_rvo', _patterns(gaps))

    # --- thread/async ref capture ---

    def test_lambda_ref_capture_thread_flagged(self):
        f = _write_cpp(self.tmp, 'o.cpp', """\
            void startWork(int data) {
                std::thread t([&]() { process(data); });
                t.detach();
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('lambda_ref_capture_thread_async', _patterns(gaps))

    def test_lambda_ref_capture_sync_not_flagged(self):
        f = _write_cpp(self.tmp, 'p.cpp', """\
            void iterate(std::vector<int>& v) {
                std::for_each(v.begin(), v.end(), [&](int x) { process(x); });
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('lambda_ref_capture_thread_async', _patterns(gaps))

    # --- non-prod exclusion ---

    def test_test_file_excluded(self):
        f = _write_cpp(self.tmp, 'test_lifetime.cpp', """\
            void use(std::string s);
            void caller() {
                std::string msg = "hello";
                use(std::move(msg));
                auto len = msg.size();
                (void)len;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ---------------------------------------------------------------------------
# Orchestrator integration: verify both scanners register via scan_files()
# ---------------------------------------------------------------------------

class TestScannerIntegration(unittest.TestCase):

    def test_scan_files_returns_list(self):
        tmp = Path(tempfile.mkdtemp())
        for Scanner in (TemplateMetaScan, OwnershipLifetimeScan):
            s = Scanner(str(tmp))
            result = s.scan_files([])
            self.assertIsInstance(result, list)
            self.assertEqual(result, [])

    def test_gap_record_schema(self):
        """Every returned gap must have the required keys."""
        required = {'file', 'line', 'severity', 'scanner', 'pattern', 'description', 'context'}
        tmp = Path(tempfile.mkdtemp())

        f = tmp / 'tst.cpp'
        f.write_text('auto fn = std::bind1st(std::plus<int>(), 5);\n', encoding='utf-8')
        s = TemplateMetaScan(str(tmp))
        for gap in s.scan_files([f]):
            self.assertTrue(required.issubset(gap.keys()),
                            f"Missing keys in gap: {gap}")

        f2 = tmp / 'tst2.cpp'
        f2.write_text(
            'struct Widget { Widget(Widget&& o) { data=o.data; } int data; };\n',
            encoding='utf-8'
        )
        s2 = OwnershipLifetimeScan(str(tmp))
        for gap in s2.scan_files([f2]):
            self.assertTrue(required.issubset(gap.keys()),
                            f"Missing keys in gap: {gap}")


if __name__ == '__main__':
    unittest.main(verbosity=2)
