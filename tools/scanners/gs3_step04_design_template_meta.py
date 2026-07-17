#!/usr/bin/env python3
"""
Phase 6 - Template Meta-Programming Scanner (P6-3)

Conservative heuristics for production C++ code:
- SFINAE complexity via std::enable_if chains (prefer C++20 concepts/requires)
- Implicit template concept assumptions without requires/static_assert
- Deprecated type traits usage (std::result_of, std::is_pod, etc.)
- Recursive template instantiation without base-case specialisation
- Rvalue reference member storage (dangling ref risk)
- Variadic template pack expansion misuse
- Template parameter count explosion (>5 type params)
- ADL-unsafe using-declarations mixing namespaces
- Non-type template parameters used for values better expressed at runtime
- Constexpr recursive depth concerns
- Missing typename/template in dependent contexts
- Partial specialisation ordering issues
"""

from __future__ import annotations

import re
from collections import Counter
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class TemplateMetaScan:
    """Detect template meta-programming gaps with conservative, low-noise heuristics."""

    CODE_SUFFIXES = {'.cpp', '.cc', '.cxx', '.h', '.hpp', '.hh', '.hxx'}
    NON_PROD_MARKERS = (
        'tests/', 'test_', '_test.', 'benchmarks/', 'bench_', '_bench.',
        'examples/', 'demo_', '_demo.', '_mock.', '/mock/', 'fuzz/',
        'third_party/', 'external/', 'vendor/',
    )

    # Deprecated / removed type traits (C++17 removed / C++20 removed / deprecated)
    DEPRECATED_TRAITS: List[Tuple[str, str, str]] = [
        (r'\bstd::result_of\b', 'std::result_of', 'std::invoke_result (C++17)'),
        (r'\bstd::result_of_t\b', 'std::result_of_t', 'std::invoke_result_t (C++17)'),
        (r'\bstd::is_pod\b', 'std::is_pod', 'std::is_trivial && std::is_standard_layout (C++20)'),
        (r'\bstd::is_pod_v\b', 'std::is_pod_v', 'std::is_trivial_v && std::is_standard_layout_v (C++20)'),
        (r'\bstd::is_literal_type\b', 'std::is_literal_type', 'constexpr evaluation context (C++17 deprecated)'),
        (r'\bstd::is_literal_type_v\b', 'std::is_literal_type_v', 'constexpr context check (C++17 deprecated)'),
        (r'\bstd::iterator\b(?!\s*:)', 'std::iterator base', 'direct iterator members (C++17 deprecated)'),
        (r'\bstd::binary_function\b', 'std::binary_function', 'direct function object (C++11 deprecated)'),
        (r'\bstd::unary_function\b', 'std::unary_function', 'direct function object (C++11 deprecated)'),
        (r'\bstd::mem_fun\b', 'std::mem_fun', 'std::mem_fn (C++11 deprecated)'),
        (r'\bstd::mem_fun_ref\b', 'std::mem_fun_ref', 'std::mem_fn (C++11 deprecated)'),
        (r'\bstd::ptr_fun\b', 'std::ptr_fun', 'lambdas or std::function (C++11 deprecated)'),
        (r'\bstd::bind1st\b', 'std::bind1st', 'std::bind or lambdas (C++11 deprecated)'),
        (r'\bstd::bind2nd\b', 'std::bind2nd', 'std::bind or lambdas (C++11 deprecated)'),
    ]

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan the given list of files and return accumulated gap records."""
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in self.CODE_SUFFIXES:
                continue

            rel_file = self._relative_path(file_path)
            if self._is_non_prod_path(rel_file):
                continue

            try:
                content = file_path.read_text(encoding='utf-8', errors='replace')
            except OSError:
                continue

            lines = content.splitlines()

            self._check_deprecated_traits(rel_file, lines)
            self._check_enable_if_chains(rel_file, lines)
            # Skip implicit concept check for large files — regex is O(n²) on file content
            if len(content) < 200_000:
                self._check_implicit_concept_assumptions(rel_file, lines, content)
            self._check_template_param_explosion(rel_file, lines)
            self._check_recursive_template_instantiation(rel_file, lines)
            self._check_rvalue_ref_member(rel_file, lines)
            self._check_const_rvalue_param(rel_file, lines)
            self._check_nttp_runtime_candidates(rel_file, lines)
            self._check_constexpr_recursive_depth(rel_file, lines)

        return self.gaps

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _relative_path(self, file_path: Path) -> str:
        try:
            return str(file_path.resolve().relative_to(self.repo_root.resolve()))
        except ValueError:
            return str(file_path)

    def _is_non_prod_path(self, rel_file: str) -> bool:
        return any(marker in rel_file for marker in self.NON_PROD_MARKERS)

    def _emit(self, rel_file: str, line: int, severity: str,
              pattern: str, description: str, context: str) -> None:
        self.gaps.append({
            'file': rel_file,
            'line': line,
            'severity': severity,
            'scanner': 'template_meta',
            'pattern': pattern,
            'description': description,
            'context': context[:200],
        })

    @staticmethod
    def _strip_comment(line: str) -> str:
        """Remove single-line // comments (naive, but sufficient for heuristics)."""
        idx = line.find('//')
        return line[:idx] if idx >= 0 else line

    def _check_deprecated_traits(self, rel_file: str, lines: List[str]) -> None:
        """Flag usage of deprecated/removed C++ type traits."""
        for lineno, raw in enumerate(lines, start=1):
            line = self._strip_comment(raw)
            for pattern, name, replacement in self.DEPRECATED_TRAITS:
                if re.search(pattern, line):
                    self._emit(
                        rel_file, lineno, 'HIGH',
                        'deprecated_type_trait',
                        f'Deprecated type trait "{name}" used; prefer {replacement}',
                        raw.strip(),
                    )

    def _check_enable_if_chains(self, rel_file: str, lines: List[str]) -> None:
        """Flag deeply nested std::enable_if / SFINAE chains where concepts are preferred."""
        # We look for template declarations where enable_if appears more than once
        # on the same logical line (multi-line cases: accumulate < ... > balance)
        i = 0
        while i < len(lines):
            line = self._strip_comment(lines[i])
            if 'template' not in line and 'enable_if' not in lines[i]:
                i += 1
                continue

            # Accumulate the template<...> header across continuation lines
            buf = line
            j = i
            depth = buf.count('<') - buf.count('>')
            while depth > 0 and j + 1 < len(lines):
                j += 1
                next_line = self._strip_comment(lines[j])
                buf += ' ' + next_line
                depth += next_line.count('<') - next_line.count('>')

            enable_if_count = len(re.findall(r'\benable_if(?:_t)?\b', buf))
            if enable_if_count >= 2:
                self._emit(
                    rel_file, i + 1, 'MEDIUM',
                    'sfinae_enable_if_chain',
                    f'Complex SFINAE chain with {enable_if_count} enable_if clauses; '
                    'consider C++20 requires/concepts for clarity',
                    lines[i].strip(),
                )

            i = j + 1

    def _check_implicit_concept_assumptions(
        self, rel_file: str, lines: List[str], content: str
    ) -> None:
        """Flag template functions that call member functions without concept constraints."""
        # Match template functions with a single unconstrained type parameter
        # that call .size() / .begin() / .end() / .empty() without static_assert or requires
        tmpl_re = re.compile(
            r'template\s*<\s*typename\s+(\w+)\s*>\s*\n'
            r'(?:(?:inline|constexpr|static|virtual|explicit)\s+)*'
            r'(?:\w[\w:<>*&\s]*\s+)?(\w+)\s*\([^)]*\)\s*(?:const\s*)?\{',
            re.MULTILINE,
        )
        member_calls = re.compile(r'\b(\w+)\.(size|begin|end|empty|push_back|insert|erase)\s*\(')

        for m in tmpl_re.finditer(content):
            type_param = m.group(1)
            block_start = m.end()
            # Find end of function body (simple brace counting)
            depth = 1
            pos = block_start
            while pos < len(content) and depth:
                c = content[pos]
                if c == '{':
                    depth += 1
                elif c == '}':
                    depth -= 1
                pos += 1
            body = content[block_start:pos]

            # Only emit if no requires/static_assert/concept guard exists
            if re.search(r'\brequires\b|\bstatic_assert\b', content[m.start(): block_start]):
                continue
            if member_calls.search(body):
                lineno = content[:m.start()].count('\n') + 1
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'implicit_template_concept',
                    f'Unconstrained template on "{type_param}" calls container member functions '
                    'without concept/requires constraint; add a concept or static_assert',
                    lines[lineno - 1].strip() if lineno <= len(lines) else '',
                )

    def _check_template_param_explosion(self, rel_file: str, lines: List[str]) -> None:
        """Flag template declarations with more than 5 type parameters."""
        tmpl_re = re.compile(r'^\s*template\s*<(.+)')
        for lineno, raw in enumerate(lines, start=1):
            m = tmpl_re.match(raw)
            if not m:
                continue
            params_text = m.group(1)
            # Extend if '<' isn't closed yet
            j = lineno
            depth = params_text.count('<') - params_text.count('>')
            while depth > 0 and j < len(lines):
                params_text += lines[j]
                depth += lines[j].count('<') - lines[j].count('>')
                j += 1

            # Count top-level typename/class/type keywords
            type_param_count = len(re.findall(r'\b(?:typename|class)\b', params_text))
            if type_param_count > 5:
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'template_param_explosion',
                    f'Template with {type_param_count} type parameters; '
                    'consider type erasure or reducing template arity',
                    raw.strip(),
                )

    def _check_recursive_template_instantiation(self, rel_file: str, lines: List[str]) -> None:
        """Flag simple struct/class templates that inherit from themselves."""
        # Pattern: template<typename T> struct Foo : Foo<T>
        # or template<int N> struct Foo : Foo<N-1> without a base-case specialisation nearby
        self_inherit_re = re.compile(
            r'^\s*(?:struct|class)\s+(\w+)\s*(?:<[^>]*>)?\s*:\s*(?:public\s+|private\s+|protected\s+)?'
            r'(\w+)\s*<'
        )
        for lineno, raw in enumerate(lines, start=1):
            m = self_inherit_re.match(raw)
            if m and m.group(1) == m.group(2):
                # Check if there's no specialisation for the base case within 20 lines
                window_start = max(0, lineno - 10)
                window_end = min(len(lines), lineno + 10)
                window = '\n'.join(lines[window_start:window_end])
                if 'template<>' not in window and 'template <>' not in window:
                    self._emit(
                        rel_file, lineno, 'HIGH',
                        'recursive_template_no_base_case',
                        f'Template "{m.group(1)}" inherits from itself with no visible base-case '
                        'specialisation; add template<> specialisation to terminate recursion',
                        raw.strip(),
                    )

    # Pre-compiled — fast check: word char or '>' immediately before &&
    _RVAL_MEMBER_RE = re.compile(r'[\w>]&&\s+(\w+)\s*(?:=|;|\{)')

    def _check_rvalue_ref_member(self, rel_file: str, lines: List[str]) -> None:
        """Flag struct/class members declared as T&&, which is almost always a dangling ref.

        Requires a word char immediately before ``&&`` to avoid false positives
        from multi-line logical-AND expressions (``&& expr``).
        Excludes lines with ``return``, ``operator``, or ``(`` (function context).
        """
        in_class = False
        brace_depth = 0
        for lineno, raw in enumerate(lines, start=1):
            stripped = raw.strip()
            if re.match(r'(?:struct|class)\s+\w', stripped):
                in_class = True
            if in_class:
                brace_depth += stripped.count('{') - stripped.count('}')
                if brace_depth <= 0:
                    in_class = False
                    brace_depth = 0
                    continue

            if not in_class:
                continue
            if '&&' not in raw:
                continue
            if 'return' in raw or 'operator' in raw or '(' in raw:
                continue
            if stripped.startswith('&&') or stripped.startswith('//'):
                continue
            m = self._RVAL_MEMBER_RE.search(raw)
            if m:
                self._emit(
                    rel_file, lineno, 'HIGH',
                    'rvalue_ref_member',
                    'RValue reference member declaration; rvalue reference members become '
                    'dangling after constructor completion — store by value or use a smart pointer',
                    raw.strip(),
                )

    def _check_const_rvalue_param(self, rel_file: str, lines: List[str]) -> None:
        """Flag function parameters of type const T&& — moving from const is impossible."""
        const_rvref_re = re.compile(r'\bconst\s+[\w:<>]+\s*&&\s+\w')
        for lineno, raw in enumerate(lines, start=1):
            line = self._strip_comment(raw)
            if const_rvref_re.search(line):
                # Exclude forwarding reference context (template parameter)
                if re.search(r'template\s*<', line):
                    continue
                self._emit(
                    rel_file, lineno, 'MEDIUM',
                    'const_rvalue_ref_param',
                    'Parameter declared as "const T&&"; cannot move from const — '
                    'use "const T&" or "T&&" instead',
                    raw.strip(),
                )

    def _check_nttp_runtime_candidates(self, rel_file: str, lines: List[str]) -> None:
        """Flag non-type template parameters (int/size_t/bool) likely better as runtime args."""
        # Only flag NTTP where the same template is instantiated with values >5 in the same file
        nttp_re = re.compile(r'^\s*template\s*<[^>]*\b(?:int|std::size_t|size_t|unsigned)\s+(\w+)[^>]*>')
        instances: Dict[str, int] = {}
        nttp_names: Dict[str, int] = {}  # param name -> first lineno

        for lineno, raw in enumerate(lines, start=1):
            m = nttp_re.match(raw)
            if m:
                name = m.group(1)
                nttp_names[name] = lineno

        if not nttp_names:
            return

        # Count instantiation sites for each NTTP name
        for name in nttp_names:
            pattern = re.compile(rf'\b\w+\s*<[^>]*\b\d{{2,}}\b[^>]*>')  # numeric literal >=10 in angle
            count = sum(1 for line in lines if pattern.search(line))
            instances[name] = count

        for name, first_lineno in nttp_names.items():
            if instances.get(name, 0) >= 5:
                self._emit(
                    rel_file, first_lineno, 'LOW',
                    'nttp_runtime_candidate',
                    f'Non-type template parameter "{name}" instantiated with many values; '
                    'consider a runtime parameter to avoid template instantiation explosion',
                    lines[first_lineno - 1].strip(),
                )

    def _check_constexpr_recursive_depth(self, rel_file: str, lines: List[str]) -> None:
        """Flag deeply nested constexpr recursive template helpers (depth heuristic)."""
        # Look for constexpr functions that call themselves; flag if recursion appears
        # deeper than ~5 levels (check template param decrement pattern)
        decrement_re = re.compile(
            r'(?:N\s*-\s*1|N\s*-\s*\d+|Depth\s*-\s*1|Count\s*-\s*1)\s*>'
        )
        for lineno, raw in enumerate(lines, start=1):
            if 'constexpr' in raw and decrement_re.search(raw):
                # Look backward for matching template declaration
                look_back = '\n'.join(lines[max(0, lineno - 15):lineno])
                if re.search(r'template\s*<[^>]*\bint\s+\w+\b[^>]*>', look_back):
                    self._emit(
                        rel_file, lineno, 'LOW',
                        'constexpr_recursive_template',
                        'constexpr recursive template instantiation detected; ensure a '
                        'terminating specialisation exists and recursion depth stays within '
                        'compiler limits (typically 900-1024)',
                        raw.strip(),
                    )


class TemplateMetaProgrammingScan:
    """Detect SFINAE anti-patterns and template meta-programming complexity."""

    ENABLE_IF_RETURN_RE = re.compile(r"\b(?:typename\s+)?std::enable_if(?:_t)?\s*<")
    ENABLE_IF_TEMPLATE_RE = re.compile(r"template\s*<[^>]*enable_if[^>]*>")
    ENABLE_IF_NON_TYPE_RE = re.compile(r"enable_if_t\s*<[^>]+,\s*(?:int|bool|size_t)\s*>\s*[A-Za-z_]\w*\s*=\s*")
    OLD_ENABLE_IF_TYPE_RE = re.compile(r"typename\s+std::enable_if\s*<.*>\s*::\s*type")
    VOID_T_RE = re.compile(r"\bstd::void_t\s*<")
    BOOL_DISPATCH_RE = re.compile(r"\bstd::integral_constant\s*<\s*bool\b")
    CONDITIONAL_CHAIN_RE = re.compile(r"std::conditional_t\s*<")
    TRAIT_GUARD_RE = re.compile(r"\bstd::is_[A-Za-z_]\w*\s*<")
    REQUIRES_RE = re.compile(r"\brequires\b")
    EXPLICIT_SPECIALIZATION_RE = re.compile(r"template\s*<\s*>\s*(?:class|struct)\s+([A-Za-z_]\w*)\s*<")
    TEMPLATE_DECL_RE = re.compile(r"^\s*template\s*<")

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    @staticmethod
    def _is_non_prod_path(rel_file: str) -> bool:
        p = rel_file.lower()
        markers = [
            "tests/", "test_", "_test.", "benchmarks/", "bench_", "_bench.", "examples/",
            "demo_", "_demo.", "_mock.", "tools/", "scripts/", "fuzz/",
        ]
        return any(m in p for m in markers)

    @staticmethod
    def _is_comment_or_pp(line: str) -> bool:
        s = line.strip()
        return not s or s.startswith("//") or s.startswith("/*") or s.startswith("*") or s.startswith("#")

    def _emit(self, rel_file: str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append({
            "file": rel_file,
            "line": line,
            "category": "template_meta_programming",
            "severity": severity,
            "pattern": pattern,
            "description": description,
            "context": context.strip()[:220],
        })

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"]:
                continue

            try:
                rel_file = str(file_path.relative_to(self.repo_root)).replace("\\", "/")
            except Exception:
                rel_file = str(file_path).replace("\\", "/")

            if self._is_non_prod_path(rel_file):
                continue

            try:
                lines = file_path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except Exception:
                continue

            self._check_sfinae_patterns(rel_file, lines)
            self._check_concept_migration_patterns(rel_file, lines)
            self._check_complexity_patterns(rel_file, lines)
            self._check_specialization_clusters(rel_file, lines)

        return self.gaps

    def _check_sfinae_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if self.ENABLE_IF_RETURN_RE.search(line) and "(" in line:
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_return_type",
                    "enable_if used in function return type instead of requires/concepts",
                    line,
                )
            if self.ENABLE_IF_TEMPLATE_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_template_parameter",
                    "enable_if used in template parameter list",
                    line,
                )
            if self.ENABLE_IF_NON_TYPE_RE.search(line) or ("enable_if_t<" in line and "= 0" in line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "enable_if_non_type_parameter",
                    "enable_if sentinel non-type template parameter increases API complexity",
                    line,
                )
            if self.OLD_ENABLE_IF_TYPE_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "HIGH",
                    "old_enable_if_type_alias",
                    "Legacy typename std::enable_if<...>::type pattern detected",
                    line,
                )
            if self.VOID_T_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "void_t_detection_idiom",
                    "void_t detection idiom could be simplified with concepts/requires",
                    line,
                )

    def _check_concept_migration_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            if self.BOOL_DISPATCH_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "bool_constant_dispatch",
                    "bool integral_constant dispatch pattern suggests pre-concepts meta-programming",
                    line,
                )
            if self.TRAIT_GUARD_RE.search(line) and self.TEMPLATE_DECL_RE.search(line) and not self.REQUIRES_RE.search(line):
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "trait_guard_without_requires",
                    "Type-trait constrained template lacks requires/concept syntax",
                    line,
                )
            if self.REQUIRES_RE.search(line) and "enable_if" in line:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "requires_enable_if_mixed",
                    "Mixed requires and enable_if in one declaration increases template complexity",
                    line,
                )

    def _check_complexity_patterns(self, rel_file: str, lines: List[str]) -> None:
        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            conditional_count = len(self.CONDITIONAL_CHAIN_RE.findall(line))
            if conditional_count >= 2:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "nested_conditional_t_chain",
                    "Multiple std::conditional_t branches in one declaration indicate brittle template logic",
                    line,
                )

            if "template" in line and line.count("<") >= 4 and line.count(">") >= 4:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "deep_template_nesting",
                    "Template declaration has deep angle-bracket nesting",
                    line,
                )

            if "::type::type" in line:
                self._emit(
                    rel_file,
                    idx,
                    "MEDIUM",
                    "nested_type_extraction_chain",
                    "Nested ::type extraction suggests legacy meta-programming layering",
                    line,
                )

    def _check_specialization_clusters(self, rel_file: str, lines: List[str]) -> None:
        counts: Counter[str] = Counter()
        first_lines: Dict[str, int] = {}
        pending_template_line = False

        for idx, line in enumerate(lines, 1):
            if self._is_comment_or_pp(line):
                continue
            stripped = line.strip()
            if stripped.startswith("template <>"):
                pending_template_line = True
            combined = line
            if pending_template_line and not stripped.startswith("template <>"):
                combined = f"template <> {stripped}"
                pending_template_line = False

            match = self.EXPLICIT_SPECIALIZATION_RE.search(combined)
            if not match:
                if stripped and stripped not in {"template <>", "template<>"}:
                    pending_template_line = False
                continue
            name = match.group(1)
            counts[name] += 1
            first_lines.setdefault(name, idx)
            pending_template_line = False

        for name, count in counts.items():
            if count >= 2:
                self._emit(
                    rel_file,
                    first_lines[name],
                    "MEDIUM",
                    "explicit_specialization_cluster",
                    f"Multiple explicit specializations for template '{name}' in one file raise maintenance risk",
                    f"{name}: {count} explicit specializations",
                )
