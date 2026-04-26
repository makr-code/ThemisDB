"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_error_handling_audit.py                       ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:53:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     446                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 4e8e0763cc  2026-03-09  fix: move from __future__ import annotations to top of te... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

#!/usr/bin/env python3
"""
Unit tests for tools/error_handling_audit.py

Run with:  python3 -m pytest tests/test_error_handling_audit.py -v
"""

import sys
import os
import tempfile
from pathlib import Path

# Make sure the tools directory is importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))

import error_handling_audit as audit

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_file(tmp_path: Path, name: str, content: str) -> Path:
    p = tmp_path / name
    p.write_text(content, encoding="utf-8")
    return p


# ---------------------------------------------------------------------------
# RULE-CPP-001: return nullptr
# ---------------------------------------------------------------------------

class TestCpp001:

    def test_no_violation_when_no_nullptr(self, tmp_path):
        f = make_file(tmp_path, "ok.cpp", 'Result<Foo*> get() { return Ok(ptr); }\n')
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_return_nullptr(self, tmp_path):
        src = "Foo* get() {\n    if (!ready) return nullptr;\n    return ptr;\n}\n"
        f = make_file(tmp_path, "bad.cpp", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-CPP-001"
        assert v[0].line == 2

    def test_no_violation_in_line_comment(self, tmp_path):
        src = "// return nullptr;\nResult<Foo*> get() { return Ok(ptr); }\n"
        f = make_file(tmp_path, "comment.cpp", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_for_or_null_function(self, tmp_path):
        src = (
            "Foo* getFooOrNull() {\n"
            "    if (!ready) return nullptr;\n"
            "    return ptr;\n"
            "}\n"
        )
        f = make_file(tmp_path, "nullable.cpp", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert v == []

    def test_multiple_violations(self, tmp_path):
        src = (
            "Foo* a() { return nullptr; }\n"
            "Bar* b() { return nullptr; }\n"
        )
        f = make_file(tmp_path, "multi.cpp", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert len(v) == 2

    def test_violation_in_header_file(self, tmp_path):
        src = "Foo* get() { return nullptr; }\n"
        f = make_file(tmp_path, "bad.h", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert len(v) == 1

    def test_no_violation_in_asterisk_comment_line(self, tmp_path):
        src = " * return nullptr;  // doc comment\n"
        f = make_file(tmp_path, "doccomment.cpp", src)
        v = audit.check_cpp_001(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# RULE-CPP-002: catch(...) without logging
# ---------------------------------------------------------------------------

class TestCpp002:

    def test_no_violation_when_no_catch_all(self, tmp_path):
        src = "try { doWork(); } catch (const std::exception& e) { log(e.what()); }\n"
        f = make_file(tmp_path, "ok.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_empty_catch_all(self, tmp_path):
        src = "try { doWork(); } catch (...) {}\n"
        f = make_file(tmp_path, "bad.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-CPP-002"

    def test_no_violation_catch_all_with_themis_error(self, tmp_path):
        src = (
            "try {\n"
            "    doWork();\n"
            "} catch (...) {\n"
            '    THEMIS_ERROR("unexpected exception");\n'
            "    throw;\n"
            "}\n"
        )
        f = make_file(tmp_path, "ok.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_catch_all_with_spdlog(self, tmp_path):
        src = (
            "try {\n"
            "    doWork();\n"
            "} catch (...) {\n"
            '    spdlog::error("failed");\n'
            "}\n"
        )
        f = make_file(tmp_path, "ok.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_multiline_with_logging_after_leading_brace(self, tmp_path):
        # The leading `}` before `catch` must not confuse brace counting
        src = (
            "    }\n"
            "} catch (...) {\n"
            '    THEMIS_WARN("fallback");\n'
            "}\n"
        )
        f = make_file(tmp_path, "tricky.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert v == []

    def test_violation_multiline_catch_all_no_log(self, tmp_path):
        src = (
            "} catch (...) {\n"
            "    // just a comment, no log\n"
            "    return false;\n"
            "}\n"
        )
        f = make_file(tmp_path, "bad.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert len(v) == 1

    def test_no_violation_in_comment(self, tmp_path):
        src = "// } catch (...) {}\n"
        f = make_file(tmp_path, "comment.cpp", src)
        v = audit.check_cpp_002(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# RULE-CPP-003: struct Status
# ---------------------------------------------------------------------------

class TestCpp003:

    def test_no_violation_without_struct_status(self, tmp_path):
        src = "struct Foo { int x; };\n"
        f = make_file(tmp_path, "ok.h", src)
        v = audit.check_cpp_003(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_struct_status(self, tmp_path):
        src = "struct Status { bool ok; std::string msg; };\n"
        f = make_file(tmp_path, "bad.h", src)
        v = audit.check_cpp_003(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-CPP-003"

    def test_no_violation_in_rocksdb_namespace(self, tmp_path):
        src = "namespace rocksdb {\nstruct Status { int code; };\n}\n"
        f = make_file(tmp_path, "rocksdb.h", src)
        v = audit.check_cpp_003(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_in_comment(self, tmp_path):
        src = "// struct Status { bool ok; };\n"
        f = make_file(tmp_path, "comment.h", src)
        v = audit.check_cpp_003(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# RULE-PY-001: bare except without logging
# ---------------------------------------------------------------------------

class TestPy001:

    def test_no_violation_on_specific_except(self, tmp_path):
        src = (
            "try:\n"
            "    do_work()\n"
            "except ValueError as e:\n"
            "    logger.error(str(e))\n"
        )
        f = make_file(tmp_path, "ok.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_bare_except_pass(self, tmp_path):
        src = "try:\n    do_work()\nexcept:\n    pass\n"
        f = make_file(tmp_path, "bad.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-PY-001"

    def test_violation_on_except_exception_pass(self, tmp_path):
        src = "try:\n    do_work()\nexcept Exception:\n    pass\n"
        f = make_file(tmp_path, "bad.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert len(v) == 1

    def test_no_violation_on_except_exception_with_logging(self, tmp_path):
        src = (
            "try:\n"
            "    do_work()\n"
            "except Exception as e:\n"
            "    logging.error(str(e))\n"
            "    raise\n"
        )
        f = make_file(tmp_path, "ok.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_with_raise(self, tmp_path):
        src = "try:\n    do_work()\nexcept Exception as e:\n    raise\n"
        f = make_file(tmp_path, "ok.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert v == []

    def test_no_violation_on_specific_typed_except(self, tmp_path):
        # "except IOError" is not "except Exception" or bare
        src = "try:\n    do_work()\nexcept IOError:\n    pass\n"
        f = make_file(tmp_path, "ok.py", src)
        v = audit.check_py_001(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# RULE-CS-001: empty C# catch
# ---------------------------------------------------------------------------

class TestCs001:

    def test_no_violation_with_logging(self, tmp_path):
        src = (
            "try { DoWork(); }\n"
            "catch (Exception ex) { logger.LogError(ex, \"err\"); throw; }\n"
        )
        f = make_file(tmp_path, "ok.cs", src)
        v = audit.check_cs_001(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_empty_catch(self, tmp_path):
        src = "try { DoWork(); }\ncatch (Exception) { }\n"
        f = make_file(tmp_path, "bad.cs", src)
        v = audit.check_cs_001(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-CS-001"

    def test_no_violation_with_throw(self, tmp_path):
        src = "try { DoWork(); }\ncatch (Exception) { throw; }\n"
        f = make_file(tmp_path, "ok.cs", src)
        v = audit.check_cs_001(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# RULE-PHP-001: empty PHP catch
# ---------------------------------------------------------------------------

class TestPhp001:

    def test_no_violation_with_throw(self, tmp_path):
        src = "try {\n    doWork();\n} catch (\\Exception $e) {\n    throw $e;\n}\n"
        f = make_file(tmp_path, "ok.php", src)
        v = audit.check_php_001(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_empty_catch(self, tmp_path):
        src = "try {\n    doWork();\n} catch (\\Exception $e) {\n}\n"
        f = make_file(tmp_path, "bad.php", src)
        v = audit.check_php_001(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-PHP-001"


# ---------------------------------------------------------------------------
# RULE-PS1-001: empty PowerShell catch
# ---------------------------------------------------------------------------

class TestPs1001:

    def test_no_violation_with_throw(self, tmp_path):
        src = "try {\n    Invoke-Work\n} catch {\n    throw\n}\n"
        f = make_file(tmp_path, "ok.ps1", src)
        v = audit.check_ps1_001(f, audit.lines_of(f))
        assert v == []

    def test_violation_on_empty_catch(self, tmp_path):
        src = "try {\n    Invoke-Work\n} catch {\n}\n"
        f = make_file(tmp_path, "bad.ps1", src)
        v = audit.check_ps1_001(f, audit.lines_of(f))
        assert len(v) == 1
        assert v[0].rule == "RULE-PS1-001"

    def test_no_violation_with_write_error(self, tmp_path):
        src = "try {\n    Invoke-Work\n} catch {\n    Write-Error $_\n}\n"
        f = make_file(tmp_path, "ok.ps1", src)
        v = audit.check_ps1_001(f, audit.lines_of(f))
        assert v == []


# ---------------------------------------------------------------------------
# Ignore-list helpers
# ---------------------------------------------------------------------------

class TestIgnoreList:

    def test_load_empty_file(self, tmp_path):
        f = make_file(tmp_path, "ignore", "")
        patterns = audit.load_ignore_patterns(str(f))
        assert patterns == []

    def test_load_with_comments(self, tmp_path):
        content = "# comment\nllama.cpp\n# another comment\nvcpkg\n"
        f = make_file(tmp_path, "ignore", content)
        patterns = audit.load_ignore_patterns(str(f))
        assert patterns == ["llama.cpp", "vcpkg"]

    def test_is_ignored_prefix_match(self):
        assert audit.is_ignored("llama.cpp/src/foo.cpp", ["llama.cpp"])
        assert not audit.is_ignored("src/foo.cpp", ["llama.cpp"])

    def test_is_ignored_exact_match(self):
        assert audit.is_ignored("llama.cpp", ["llama.cpp"])

    def test_is_ignored_wildcard(self):
        assert audit.is_ignored("tmp_issue_1234.md", ["tmp_*"])

    def test_load_nonexistent_file(self, tmp_path):
        patterns = audit.load_ignore_patterns(str(tmp_path / "nonexistent"))
        assert patterns == []


# ---------------------------------------------------------------------------
# Integration: audit_file dispatches correctly
# ---------------------------------------------------------------------------

class TestAuditFile:

    def test_audit_cpp_file(self, tmp_path):
        src = "Foo* get() { return nullptr; }\n"
        f = make_file(tmp_path, "bad.cpp", src)
        v = audit.audit_file(f)
        assert any(x.rule == "RULE-CPP-001" for x in v)

    def test_audit_py_file(self, tmp_path):
        src = "try:\n    work()\nexcept:\n    pass\n"
        f = make_file(tmp_path, "bad.py", src)
        v = audit.audit_file(f)
        assert any(x.rule == "RULE-PY-001" for x in v)

    def test_audit_cs_file(self, tmp_path):
        src = "try { Work(); }\ncatch (Exception) { }\n"
        f = make_file(tmp_path, "bad.cs", src)
        v = audit.audit_file(f)
        assert any(x.rule == "RULE-CS-001" for x in v)

    def test_audit_clean_file(self, tmp_path):
        src = "Result<int> get() { return Ok(42); }\n"
        f = make_file(tmp_path, "ok.cpp", src)
        v = audit.audit_file(f)
        assert v == []


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

class TestFormatting:

    def _make_violation(self):
        return audit.Violation(
            rule="RULE-CPP-001",
            file="src/foo.cpp",
            line=10,
            column=5,
            text="    return nullptr;",
            message="return nullptr detected",
        )

    def test_format_text_no_color(self):
        v = self._make_violation()
        out = audit.format_text([v], color=False)
        assert "RULE-CPP-001" in out
        assert "src/foo.cpp" in out
        assert "10:5" in out
        assert "1 violation(s)" in out

    def test_format_json(self):
        import json
        v = self._make_violation()
        out = audit.format_json([v])
        data = json.loads(out)
        assert len(data) == 1
        assert data[0]["rule"] == "RULE-CPP-001"
        assert data[0]["file"] == "src/foo.cpp"
        assert data[0]["line"] == 10

    def test_format_text_empty(self):
        out = audit.format_text([], color=False)
        assert "0 violation(s)" in out
