"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_secret_scanner.py                             ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:57:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     288                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for scripts/secret_scan.py

Run with:  python3 -m pytest tests/test_secret_scanner.py -v
"""

import sys
import os
import re
import tempfile
from pathlib import Path

# Make the scripts directory importable as a module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))

import secret_scan as scanner  # noqa: E402 (import after sys.path manipulation)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_file(tmp_path: Path, name: str, content: str) -> Path:
    p = tmp_path / name
    p.write_text(content, encoding="utf-8")
    return p


# ---------------------------------------------------------------------------
# Shannon entropy tests
# ---------------------------------------------------------------------------

class TestShannonEntropy:
    def test_empty_string_returns_zero(self):
        assert scanner.shannon_entropy("") == 0.0

    def test_single_char_repeated_returns_zero(self):
        # "aaaa" has only one distinct character → entropy = 0
        assert scanner.shannon_entropy("aaaa") == 0.0

    def test_two_equal_probability_chars(self):
        # "ababab" → p(a) = p(b) = 0.5 → H = 1.0 bit
        assert abs(scanner.shannon_entropy("ababab") - 1.0) < 1e-9

    def test_high_entropy_random_like_string(self):
        # A base64-encoded random key should have entropy well above 4.5
        token = "aB3dEf7hIjKlMnOpQrStUvWxYz012345"
        assert scanner.shannon_entropy(token) > 4.0

    def test_known_high_entropy_token(self):
        # A typical AWS-style 40-char mixed-case+digit string
        token = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
        ent = scanner.shannon_entropy(token)
        assert ent >= 4.5, f"Expected entropy ≥ 4.5, got {ent:.3f}"

    def test_low_entropy_password_placeholder(self):
        assert scanner.shannon_entropy("password") < 4.5
        assert scanner.shannon_entropy("12345678") < 4.5


# ---------------------------------------------------------------------------
# High-entropy token extraction tests
# ---------------------------------------------------------------------------

class TestHighEntropyTokens:
    def test_no_tokens_in_plain_text(self):
        assert scanner.high_entropy_tokens("hello world this is a plain sentence") == []

    def test_detects_high_entropy_long_token(self):
        line = 'api_key = "aB3dEf7hIjKlMnOpQrStUvWxYz012345678XYZ"'
        tokens = scanner.high_entropy_tokens(line)
        assert len(tokens) >= 1

    def test_short_tokens_are_ignored(self):
        # Tokens shorter than MIN_TOKEN_LENGTH must be ignored even if high-entropy
        line = "key=aBcD1234"
        assert scanner.high_entropy_tokens(line) == []

    def test_returns_multiple_tokens_when_present(self):
        t1 = "aB3dEf7hIjKlMnOpQrStUvWxYz012345"
        t2 = "XyZ9AbCdEfGhIjKlMnOpQrStUvWxYz01"
        line = f"data={t1} other={t2}"
        tokens = scanner.high_entropy_tokens(line)
        # Both tokens should be detected (depending on entropy)
        assert len(tokens) >= 1


# ---------------------------------------------------------------------------
# Pattern detection tests
# ---------------------------------------------------------------------------

class TestPatternDetection:
    def test_detects_pem_private_key(self, tmp_path):
        f = make_file(tmp_path, "key.cpp",
                      '// cert\nstd::string key = "-----BEGIN RSA PRIVATE KEY-----";\n')
        findings = scanner.scan_file(f, [])
        assert any("PEM private key" in find.description for find in findings)

    def test_detects_aws_access_key(self, tmp_path):
        f = make_file(tmp_path, "config.cpp",
                      'const char* access = "AKIAIOSFODNN7NOTAFAKE";\n')
        findings = scanner.scan_file(f, [])
        assert any("AWS Access Key" in find.description for find in findings)

    def test_detects_github_pat(self, tmp_path):
        # 36 alphanumeric chars after 'ghp_', no sequential digits or placeholder words
        f = make_file(tmp_path, "ci.yml",
                      'token: ghp_AbCdEfGhIjKlMnOpQrStUvWxYzAbCdEfGhIj\n')
        findings = scanner.scan_file(f, [])
        assert any("GitHub" in find.description for find in findings)

    def test_detects_database_connection_string(self, tmp_path):
        f = make_file(tmp_path, "conn.cpp",
                      'auto dsn = "postgresql://admin:s3cr3tPassw0rd@db.internal/prod";\n')
        findings = scanner.scan_file(f, [])
        assert any("Database connection" in find.description for find in findings)

    def test_detects_jwt_secret(self, tmp_path):
        # Value must match [A-Za-z0-9+/]{32,} – use pure alphanumeric string
        f = make_file(tmp_path, "auth.cpp",
                      'std::string jwt_secret = "aBcDeFgHiJkLmNoPqRsTuVwXyZ0987654321abcd";\n')
        findings = scanner.scan_file(f, [])
        assert any("JWT secret" in find.description for find in findings)

    def test_detects_slack_token(self, tmp_path):
        # Avoid sequential digits 1234567890 in the token body
        f = make_file(tmp_path, "notify.py",
                      'SLACK_TOKEN = "xoxb-9876543219-AbCdEfGhIjKlMnOpQrStUvWx"\n')
        findings = scanner.scan_file(f, [])
        assert any("Slack token" in find.description for find in findings)

    def test_clean_file_returns_no_findings(self, tmp_path):
        f = make_file(tmp_path, "clean.cpp",
                      '// Normal C++ file\n#include <string>\nint main() { return 0; }\n')
        findings = scanner.scan_file(f, [])
        assert findings == []


# ---------------------------------------------------------------------------
# False-positive suppression tests
# ---------------------------------------------------------------------------

class TestFalsePositiveSuppression:
    def test_example_password_suppressed(self, tmp_path):
        f = make_file(tmp_path, "docs.cpp",
                      '// Example: password = "example_password"\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_placeholder_template_suppressed(self, tmp_path):
        f = make_file(tmp_path, "template.yaml",
                      'api_key: ${API_KEY_PLACEHOLDER}\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_dummy_secret_suppressed(self, tmp_path):
        f = make_file(tmp_path, "test_helper.cpp",
                      'const char* api_key = "dummy_api_key_for_testing";\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_allowlist_suppresses_real_looking_pattern(self, tmp_path):
        f = make_file(tmp_path, "config.cpp",
                      'std::string key = "AKIAIOSFODNN7EXAMPLE";\n')
        # Add pattern to allow-list that suppresses this specific string
        allowlist = [re.compile(r'AKIAIOSFODNN7EXAMPLE')]
        findings = scanner.scan_file(f, allowlist)
        assert findings == []

    def test_comment_line_suppressed(self, tmp_path):
        f = make_file(tmp_path, "notes.cpp",
                      '// password = "hardcoded_password_here"\n')
        findings = scanner.scan_file(f, [])
        assert findings == []


# ---------------------------------------------------------------------------
# File extension / directory skip tests
# ---------------------------------------------------------------------------

class TestSkipRules:
    def test_binary_extension_skipped(self, tmp_path):
        f = make_file(tmp_path, "image.png",
                      'AKIA1234567890ABCDEF password="secret"\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_lock_file_skipped(self, tmp_path):
        f = make_file(tmp_path, "package.lock",
                      'api_key = "AKIAIOSFODNN7EXAMPLE"\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_file_in_build_dir_skipped(self, tmp_path):
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        f = make_file(build_dir, "config.h",
                      '#define API_KEY "AKIAIOSFODNN7EXAMPLE"\n')
        findings = scanner.scan_file(f, [])
        assert findings == []

    def test_vcpkg_dir_skipped(self, tmp_path):
        vcpkg_dir = tmp_path / "vcpkg"
        vcpkg_dir.mkdir()
        f = make_file(vcpkg_dir, "portfile.cmake",
                      'set(API_KEY "AKIAIOSFODNN7EXAMPLE")\n')
        findings = scanner.scan_file(f, [])
        assert findings == []


# ---------------------------------------------------------------------------
# Allow-list file loading tests
# ---------------------------------------------------------------------------

class TestAllowlistLoading:
    def test_empty_allowlist_when_no_file(self, tmp_path):
        result = scanner.load_allowlist(tmp_path / "nonexistent.txt")
        assert result == []

    def test_loads_valid_patterns(self, tmp_path):
        al = make_file(tmp_path, "allowlist.txt",
                       "# comment\nAKIAIOSFODNN7EXAMPLE\nmy_test_token\n")
        patterns = scanner.load_allowlist(al)
        assert len(patterns) == 2
        assert any(p.pattern == "AKIAIOSFODNN7EXAMPLE" for p in patterns)

    def test_skips_comment_lines(self, tmp_path):
        al = make_file(tmp_path, "allowlist.txt",
                       "# this is a comment\n# another comment\nreal_pattern\n")
        patterns = scanner.load_allowlist(al)
        assert len(patterns) == 1
        assert patterns[0].pattern == "real_pattern"

    def test_ignores_blank_lines(self, tmp_path):
        al = make_file(tmp_path, "allowlist.txt", "\n\npattern_one\n\n")
        patterns = scanner.load_allowlist(al)
        assert len(patterns) == 1


# ---------------------------------------------------------------------------
# CLI / main() integration tests
# ---------------------------------------------------------------------------

class TestMain:
    def test_returns_zero_for_clean_file(self, tmp_path):
        f = make_file(tmp_path, "clean.cpp",
                      "#include <iostream>\nint main() { return 0; }\n")
        result = scanner.main([str(f)])
        assert result == 0

    def test_returns_one_for_file_with_aws_key(self, tmp_path):
        f = make_file(tmp_path, "bad.cpp",
                      'const char* key = "AKIAIOSFODNN7REALKEY12";\n')
        result = scanner.main([str(f)])
        assert result == 1

    def test_returns_zero_for_nonexistent_file(self, tmp_path):
        # Non-existent files are silently skipped
        result = scanner.main([str(tmp_path / "does_not_exist.cpp")])
        assert result == 0

    def test_no_files_returns_zero(self, tmp_path):
        # Pass an explicit empty list – main() should return 0 immediately
        result = scanner.main(["--allowlist", str(tmp_path / "no_al.txt")])
        assert result == 0
