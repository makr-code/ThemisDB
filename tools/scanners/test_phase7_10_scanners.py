#!/usr/bin/env python3
"""
Tests for Phase 7-10 scanners:

  Phase 7 — Compliance & Audit:
    - P7-1: gs3_step04_quality_audit_logging.AuditLoggingScanImproved
    - P7-2: gs3_step04_design_deprecated_apis.DeprecatedAPIsScan

  Phase 8 — Performance & GPU:
    - P8-1: gs3_step04_design_performance_patterns.PerformanceAntiPatternsScanImproved
    - P8-2: gs3_step04_design_gpu_memory.GPUMemorySafetyScan

  Phase 9 — Domain-Specific:
    - P9-1: gs3_step04_design_query_correctness.QueryCorrectnessScan
    - P9-2: gs3_step04_design_distributed_consistency.DistributedConsistencyScanImproved
    - P9-3: gs3_step04_design_llm_ai_safety.LLMAISafetyScan

  Phase 10 — Runtime & Observability:
    - P10-1: gs3_step04_design_observability.ObservabilityScannerImproved
    - P10-2: gs3_step04_design_determinism.DeterminismScannerImproved
"""

from __future__ import annotations

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

# Allow running from repo root or from tools/scanners/
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

from tools.scanners.gs3_step04_quality_audit_logging import AuditLoggingScanImproved
from tools.scanners.gs3_step04_design_deprecated_apis import DeprecatedAPIsScan
from tools.scanners.gs3_step04_design_performance_patterns import PerformanceAntiPatternsScanImproved
from tools.scanners.gs3_step04_design_gpu_memory import GPUMemorySafetyScan
from tools.scanners.gs3_step04_design_query_correctness import QueryCorrectnessScan
from tools.scanners.gs3_step04_design_distributed_consistency import DistributedConsistencyScanImproved
from tools.scanners.gs3_step04_design_llm_ai_safety import LLMAISafetyScan
from tools.scanners.gs3_step04_design_observability import ObservabilityScannerImproved
from tools.scanners.gs3_step04_design_determinism import DeterminismScannerImproved
from tools.scanners.gs3_step04_quality_cpp_doxygen import ThemisCppDoxygenPolicyRulesScan


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write(tmp: Path, rel_path: str, code: str) -> Path:
    """Write a file at tmp/rel_path, creating parent dirs as needed."""
    f = tmp / rel_path
    f.parent.mkdir(parents=True, exist_ok=True)
    f.write_text(textwrap.dedent(code), encoding='utf-8')
    return f


def _patterns(gaps):
    """Collect 'pattern' values from a list of gap dicts."""
    return {g.get('pattern', g.get('type', '')) for g in gaps}


# ===========================================================================
# P7-1  AuditLoggingScanImproved
# ===========================================================================

class TestAuditLoggingScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = AuditLoggingScanImproved(str(self.tmp))

    def _src(self, name: str, code: str) -> Path:
        return _write(self.tmp, f'src/{name}', code)

    # --- missing_audit_log ---

    def test_authenticate_without_log_flagged(self):
        f = self._src('auth.cpp', """\
            bool authenticate(const std::string& user, const std::string& pass) {
                return check_credentials(user, pass);
            }
        """)
        gaps = self.scanner.scan_files([f])
        types = _patterns(gaps)
        self.assertIn('missing_audit_log', types)

    def test_authenticate_with_log_ok(self):
        f = self._src('auth_ok.cpp', """\
            bool authenticate(const std::string& user, const std::string& pass) {
                logger->info("auth attempt for {}", user);
                return check_credentials(user, pass);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_audit_log', _patterns(gaps))

    def test_authorize_without_log_flagged(self):
        f = self._src('authz.cpp', """\
            bool authorize(const std::string& role, const std::string& resource) {
                return policy_check(role, resource);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_audit_log', _patterns(gaps))

    # --- sensitive_data_logging ---

    def test_password_in_log_flagged(self):
        code = (
            'void handle_login(const std::string& password) {\n'
            '    std::cout << "password=" << password << std::endl;\n'
            '}\n'
        )
        f = self._src('login.cpp', code)
        gaps = self.scanner.scan_files([f])
        self.assertIn('sensitive_data_logging', _patterns(gaps))

    def test_token_in_log_flagged(self):
        f = self._src('api.cpp', """\
            void refresh(const std::string& token) {
                logger->info("token={}", token);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('sensitive_data_logging', _patterns(gaps))

    # --- test file exclusion ---

    def test_test_file_excluded(self):
        f = _write(self.tmp, 'tests/test_auth.cpp', """\
            bool authenticate(const std::string& user, const std::string& pass) {
                return check_credentials(user, pass);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual([], [g for g in gaps if g.get('type') == 'missing_audit_log'])


# ===========================================================================
# P7-2  DeprecatedAPIsScan
# ===========================================================================

class TestDeprecatedAPIsScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = DeprecatedAPIsScan(str(self.tmp))

    def _cpp(self, name: str, code: str) -> Path:
        return _write(self.tmp, name, code)

    # --- unsafe_c_functions ---

    def test_strcpy_flagged(self):
        f = self._cpp('copy.cpp', """\
            void copy_name(char* dst, const char* src) {
                strcpy(dst, src);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('strcpy' in g.get('pattern', '') for g in gaps))

    def test_sprintf_flagged(self):
        f = self._cpp('format.cpp', """\
            char buf[256];
            sprintf(buf, "%s %d", name, value);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('sprintf' in g.get('pattern', '') for g in gaps))

    def test_gets_flagged(self):
        f = self._cpp('input.cpp', """\
            char line[1024];
            gets(line);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('gets' in g.get('pattern', '') for g in gaps))

    # --- openssl_deprecated ---

    def test_md5_init_flagged(self):
        f = self._cpp('hash.cpp', """\
            #include <openssl/md5.h>
            MD5_CTX ctx;
            MD5_Init(&ctx);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('MD5' in g.get('pattern', '') for g in gaps))

    def test_sha1_deprecated(self):
        f = self._cpp('sha.cpp', """\
            SHA1_CTX ctx;
            SHA1_Init(&ctx);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('SHA1' in g.get('pattern', '') for g in gaps))

    # --- cpp_stdlib ---

    def test_auto_ptr_flagged(self):
        f = self._cpp('ptr.cpp', """\
            std::auto_ptr<int> p(new int(42));
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('auto_ptr' in g.get('pattern', '') for g in gaps))

    def test_random_shuffle_flagged(self):
        f = self._cpp('shuffle.cpp', """\
            std::random_shuffle(v.begin(), v.end());
        """)
        gaps = self.scanner.scan_files([f])
        self.assertTrue(any('random_shuffle' in g.get('pattern', '') for g in gaps))

    # --- comment line skipped ---

    def test_commented_strcpy_not_flagged(self):
        f = self._cpp('comment.cpp', """\
            // strcpy(dst, src);  // deprecated, use strncpy
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P8-1  PerformanceAntiPatternsScanImproved
# ===========================================================================

class TestPerformanceAntiPatternsScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = PerformanceAntiPatternsScanImproved(str(self.tmp))

    def _src(self, name: str, code: str) -> Path:
        return _write(self.tmp, f'src/{name}', code)

    # --- nested_loop_find ---

    def test_nested_loop_find_flagged(self):
        f = self._src('search.cpp', """\
            void search(const std::vector<int>& a, const std::vector<int>& b) {
                for (auto x : a) {
                    for (auto y : b) {
                        if (b.find(x) != b.end()) {}
                    }
                }
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('nested_loop_find', _patterns(gaps))

    # --- missing_vector_reserve ---

    def test_missing_reserve_flagged(self):
        f = self._src('build.cpp', """\
            std::vector<int> build(size_t count) {
                std::vector<int> result;
                for (size_t i = 0; i < count; ++i) {
                    result.push_back(i);
                }
                return result;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_vector_reserve', _patterns(gaps))

    def test_reserve_present_not_flagged(self):
        f = self._src('build_good.cpp', """\
            std::vector<int> build(size_t count) {
                std::vector<int> result;
                result.reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    result.push_back(i);
                }
                return result;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_vector_reserve', _patterns(gaps))

    # --- regex_in_loop ---

    def test_regex_in_loop_flagged(self):
        f = self._src('pattern.cpp', """\
            for (auto& s : items) {
                std::regex re("a+b");
                if (std::regex_match(s, re)) {}
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('regex_in_loop', _patterns(gaps))

    # --- endl_in_loop ---

    def test_endl_in_loop_flagged(self):
        f = self._src('log.cpp', """\
            for (int i = 0; i < n; ++i) {
                std::cout << i << std::endl;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('endl_in_loop', _patterns(gaps))

    # --- test file excluded ---

    def test_non_src_not_scanned(self):
        f = _write(self.tmp, 'tests/bench.cpp', """\
            for (auto& s : items) {
                std::regex re("a+b");
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P8-2  GPUMemorySafetyScan
# ===========================================================================

class TestGPUMemorySafetyScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = GPUMemorySafetyScan(str(self.tmp))

    def _cuda(self, name: str, code: str) -> Path:
        return _write(self.tmp, name, code)

    # --- unchecked_cuda_call ---

    def test_unchecked_cudamalloc_flagged(self):
        f = self._cuda('kernel.cu', """\
            float* d_data;
            cudaMalloc(&d_data, size * sizeof(float));
        """)
        # Rename to .cpp for scanner (it checks .cpp/.cc/.h/.hpp)
        f2 = _write(self.tmp, 'kernel.cpp', """\
            float* d_data;
            cudaMalloc(&d_data, size * sizeof(float));
        """)
        gaps = self.scanner.scan_files([f2])
        self.assertIn('unchecked_cuda_call', _patterns(gaps))

    def test_checked_cudamalloc_ok(self):
        f = _write(self.tmp, 'kernel_ok.cpp', """\
            float* d_data;
            cudaError_t err = cudaMalloc(&d_data, size);
            if (err != cudaSuccess) { throw std::runtime_error("..."); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('unchecked_cuda_call', _patterns(gaps))

    # --- kernel_config_validation ---

    def test_kernel_launch_without_validation_flagged(self):
        f = _write(self.tmp, 'gpu_launch.cpp', """\
            myKernel<<<grid, block>>>(data, n);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('kernel_config_validation', _patterns(gaps))

    def test_kernel_launch_with_check_ok(self):
        f = _write(self.tmp, 'gpu_launch_ok.cpp', """\
            assert(block.x <= 1024);
            myKernel<<<grid, block>>>(data, n);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('kernel_config_validation', _patterns(gaps))

    # --- missing_sync_threads ---

    def test_kernel_shared_without_sync_flagged(self):
        f = _write(self.tmp, 'kernel_sync.cpp', """\
            __global__ void vecAdd(float* a, float* b) {
                __shared__ float sdata[256];
                sdata[threadIdx.x] = a[threadIdx.x] + b[threadIdx.x];
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_sync_threads', _patterns(gaps))


# ===========================================================================
# P9-1  QueryCorrectnessScan
# ===========================================================================

class TestQueryCorrectnessScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = QueryCorrectnessScan(str(self.tmp))

    def _cpp(self, name: str, code: str) -> Path:
        return _write(self.tmp, name, code)

    # --- query_string_concat ---

    def test_string_concat_in_query_flagged(self):
        f = self._cpp('query.cpp', """\
            std::string q;
            q.append("SELECT * FROM users WHERE id = " + user_id);
            execute_query(q);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('query_string_concat', _patterns(gaps))

    # --- missing_join_condition ---

    def test_join_without_on_flagged(self):
        f = self._cpp('query.cpp', """\
            std::string q = "SELECT * FROM orders JOIN customers";
            execute(q);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_join_condition', _patterns(gaps))

    def test_join_with_on_ok(self):
        f = self._cpp('query.cpp', """\
            std::string q = "SELECT * FROM orders JOIN customers ON orders.cust_id = customers.id";
            execute(q);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_join_condition', _patterns(gaps))

    # --- having_without_group_by ---

    def test_having_without_group_by_flagged(self):
        f = self._cpp('query.cpp', """\
            std::string q = "SELECT dept FROM emp HAVING count > 5";
            execute(q);
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('having_without_group_by', _patterns(gaps))

    # --- missing_param_validation ---

    def test_unvalidated_param_flagged(self):
        f = self._cpp('query.cpp', """\
            void run_query(const std::string& user_param) {
                bind_param(0, user_param);
                execute_query("SELECT * FROM t WHERE id = ?");
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_param_validation', _patterns(gaps))


# ===========================================================================
# P9-2  DistributedConsistencyScanImproved
# ===========================================================================

class TestDistributedConsistencyScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = DistributedConsistencyScanImproved(str(self.tmp))

    def _dist(self, name: str, code: str) -> Path:
        # Distributed scanner requires src/ or include/ + distributed keywords in content
        return _write(self.tmp, f'src/{name}', code)

    # --- missing_consensus ---

    def test_write_without_consensus_flagged(self):
        f = self._dist('raft.cpp', """\
            class RaftNode {
                void apply(const Entry& e) {
                    storage.put(key, value);
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_consensus', _patterns(gaps))

    def test_write_with_quorum_ok(self):
        f = self._dist('raft_ok.cpp', """\
            class RaftLeader {
                void replicate(const Entry& e) {
                    if (quorum_reached()) {
                        storage.put(key, value);
                    }
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_consensus', _patterns(gaps))

    # --- leader_election ---

    def test_leader_elect_without_majority_flagged(self):
        f = self._dist('election.cpp', """\
            class consensus {
                void elect_leader() {
                    is_leader = true;
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('leader_election', _patterns(gaps))

    def test_leader_elect_with_majority_ok(self):
        f = self._dist('election_ok.cpp', """\
            class consensus {
                void elect_leader() {
                    if (votes > replica_count / 2) {
                        is_leader = true;  // majority required
                    }
                }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('leader_election', _patterns(gaps))

    # --- test file excluded ---

    def test_test_file_excluded(self):
        f = _write(self.tmp, 'tests/test_raft.cpp', """\
            class consensus {
                void replicate() { storage.put(key, value); }
            };
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P9-3  LLMAISafetyScan
# ===========================================================================

class TestLLMAISafetyScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = LLMAISafetyScan(str(self.tmp))

    def _llm(self, name: str, code: str) -> Path:
        # LLM scanner only scans files with llm/model/prompt keywords in path
        return _write(self.tmp, f'src/llm/{name}', code)

    # --- prompt_injection ---

    def test_prompt_injection_flagged(self):
        f = self._llm('prompt.cpp', """\
            std::string build_prompt(const std::string& user_input) {
                std::string prompt = user_input;
                return prompt;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('prompt_injection', _patterns(gaps))

    def test_sanitized_prompt_ok(self):
        f = self._llm('prompt_ok.cpp', """\
            std::string build_prompt(const std::string& user_input) {
                auto clean = sanitize(user_input);
                return "Answer this: " + clean;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('prompt_injection', _patterns(gaps))

    # --- missing_resource_limits ---

    def test_inference_without_token_limit_flagged(self):
        f = self._llm('infer.cpp', """\
            void run(const std::string& prompt) {
                auto result = model.generate(prompt);
                return result;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_resource_limits', _patterns(gaps))

    def test_inference_with_max_tokens_ok(self):
        f = self._llm('infer_ok.cpp', """\
            void run(const std::string& prompt) {
                auto result = model.generate(prompt);
                const size_t max_tokens = 512;
                (void)max_tokens;
                return result;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_resource_limits', _patterns(gaps))

    # --- model_integrity_gap ---

    def test_load_model_without_checksum_flagged(self):
        f = self._llm('loader.cpp', """\
            void load_model(const std::string& path) {
                model.load_model(path);
                model.run();
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('model_integrity_gap', _patterns(gaps))

    # --- non-llm file excluded ---

    def test_non_llm_file_excluded(self):
        f = _write(self.tmp, 'src/storage/cache.cpp', """\
            std::string build_prompt(const std::string& user_input) {
                return "Answer this: " + user_input;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P10-1  ObservabilityScannerImproved
# ===========================================================================

class TestObservabilityScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = ObservabilityScannerImproved(str(self.tmp))

    def _src(self, name: str, code: str) -> Path:
        return _write(self.tmp, f'src/{name}', code)

    # --- missing_trace_point ---

    def test_critical_function_without_trace_flagged(self):
        f = self._src('executor.cpp', """\
            void execute_query(const std::string& q) {
                parse(q);
                run(q);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('missing_trace_point', _patterns(gaps))

    def test_critical_function_with_trace_ok(self):
        f = self._src('executor_ok.cpp', """\
            void execute_query(const std::string& q) {
                auto span = tracer->start_span("execute_query");
                parse(q);
                run(q);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertNotIn('missing_trace_point', _patterns(gaps))

    # --- non_deterministic (non-production code filtered) ---

    def test_src_only_scanned(self):
        # non-src file should not be scanned
        f = _write(self.tmp, 'tests/test_exec.cpp', """\
            void execute_query(const std::string& q) {
                parse(q);
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P10-2  DeterminismScannerImproved
# ===========================================================================

class TestDeterminismScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = DeterminismScannerImproved(str(self.tmp))

    def _src(self, name: str, code: str) -> Path:
        return _write(self.tmp, f'src/{name}', code)

    # --- float_comparison ---

    def test_float_equality_flagged(self):
        f = self._src('calc.cpp', """\
            if (score == 1.0f) { accept(); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('float_comparison', _patterns(gaps))

    def test_double_inequality_flagged(self):
        f = self._src('check.cpp', """\
            if (result != 0.0) { reject(); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('float_comparison', _patterns(gaps))

    # --- uncontrolled_randomness ---

    def test_rand_without_seed_flagged(self):
        f = self._src('sample.cpp', """\
            int pick() {
                return rand() % 10;
            }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('uncontrolled_randomness', _patterns(gaps))

    # --- unordered_iteration ---

    def test_unordered_map_iter_flagged(self):
        f = self._src('iterate.cpp', """\
            std::unordered_map<std::string, int> m;
            for (auto& [k, v] : m) { process(k, v); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertIn('unordered_iteration', _patterns(gaps))

    # --- test file excluded ---

    def test_test_file_not_scanned(self):
        f = _write(self.tmp, 'tests/test_det.cpp', """\
            if (score == 1.0f) { accept(); }
        """)
        gaps = self.scanner.scan_files([f])
        self.assertEqual(gaps, [])


# ===========================================================================
# P10-8  ThemisCppDoxygenPolicyRulesScan
# ===========================================================================

class TestThemisCppDoxygenPolicyRulesScan(unittest.TestCase):

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.scanner = ThemisCppDoxygenPolicyRulesScan(str(self.tmp))

    def _header(self, name: str, code: str) -> Path:
        return _write(self.tmp, f'include/auth/{name}', code)

    def test_relative_header_path_is_supported(self):
        self._header('federated_identity_manager.h', """\
            class FederatedIdentityManager {
            public:
                bool authenticate(const std::string& user);
            };
        """)
        gaps = self.scanner.scan_files([Path('include/auth/federated_identity_manager.h')])
        self.assertIn('missing_doxygen_class', _patterns(gaps))
        self.assertIn('missing_doxygen_comment', _patterns(gaps))

    def test_absolute_header_path_still_supported(self):
        header = self._header('session_manager.h', """\
            class SessionManager {
            public:
                bool authenticate(const std::string& user);
            };
        """)
        gaps = self.scanner.scan_files([header])
        self.assertIn('missing_doxygen_class', _patterns(gaps))
        self.assertIn('missing_doxygen_comment', _patterns(gaps))


# ===========================================================================
# Integration: scan_files() contract for all Phase 7-10 scanners
# ===========================================================================

class TestPhase7to10Integration(unittest.TestCase):

    ALL_SCANNERS = [
        AuditLoggingScanImproved,
        DeprecatedAPIsScan,
        PerformanceAntiPatternsScanImproved,
        GPUMemorySafetyScan,
        QueryCorrectnessScan,
        DistributedConsistencyScanImproved,
        LLMAISafetyScan,
        ObservabilityScannerImproved,
        DeterminismScannerImproved,
    ]

    def test_empty_file_list_returns_empty(self):
        tmp = Path(tempfile.mkdtemp())
        for Scanner in self.ALL_SCANNERS:
            s = Scanner(str(tmp))
            result = s.scan_files([])
            self.assertIsInstance(result, list,
                                  f"{Scanner.__name__}.scan_files([]) must return list")
            self.assertEqual(result, [],
                             f"{Scanner.__name__}.scan_files([]) must return [] for empty input")

    def test_non_cpp_file_produces_no_gaps(self):
        tmp = Path(tempfile.mkdtemp())
        md_file = tmp / 'README.md'
        md_file.write_text('# Test\nsome text\n', encoding='utf-8')
        for Scanner in self.ALL_SCANNERS:
            s = Scanner(str(tmp))
            result = s.scan_files([md_file])
            self.assertIsInstance(result, list,
                                  f"{Scanner.__name__} must return list for .md input")
            self.assertEqual(result, [],
                             f"{Scanner.__name__} must produce no gaps for .md input")

    def test_each_scanner_instantiates_with_default_root(self):
        """Instantiation with default repo_root='.' must not raise."""
        for Scanner in self.ALL_SCANNERS:
            try:
                Scanner()
            except Exception as exc:  # pragma: no cover
                self.fail(f"{Scanner.__name__}() raised {exc}")


if __name__ == '__main__':
    unittest.main(verbosity=2)
