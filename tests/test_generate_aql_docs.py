"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_generate_aql_docs.py                          ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:53:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     551                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Unit tests for scripts/generate_aql_docs.py

Run with:  python3 -m pytest tests/test_generate_aql_docs.py -v
"""

import sys
import os
import textwrap
from pathlib import Path

# Make the scripts directory importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))

import generate_aql_docs as gen


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_header(tmp_path: Path, name: str, content: str) -> Path:
    p = tmp_path / name
    p.write_text(content, encoding='utf-8')
    return p


# ---------------------------------------------------------------------------
# extract_balanced_braces
# ---------------------------------------------------------------------------

class TestExtractBalancedBraces:

    def test_simple(self):
        assert gen.extract_balanced_braces('{hello}', 0) == '{hello}'

    def test_nested(self):
        assert gen.extract_balanced_braces('{a{b}c}', 0) == '{a{b}c}'

    def test_with_string_containing_brace(self):
        assert gen.extract_balanced_braces('{"{"}', 0) == '{"{"}'

    def test_offset(self):
        text = 'abc{def}'
        assert gen.extract_balanced_braces(text, 3) == '{def}'

    def test_not_opening_brace(self):
        assert gen.extract_balanced_braces('hello', 0) is None

    def test_unbalanced(self):
        assert gen.extract_balanced_braces('{unclosed', 0) is None


# ---------------------------------------------------------------------------
# extract_string_literals
# ---------------------------------------------------------------------------

class TestExtractStringLiterals:

    def test_simple_string(self):
        assert gen.extract_string_literals('"hello"') == ['hello']

    def test_multiple_strings(self):
        assert gen.extract_string_literals('"a", "b", "c"') == ['a', 'b', 'c']

    def test_raw_string_basic(self):
        result = gen.extract_string_literals('R"(hello)"')
        assert result == ['hello']

    def test_raw_string_with_inner_quotes(self):
        # R"(CONCAT("Hello", " ") // "Hello ")" → one raw string
        result = gen.extract_string_literals('R"(CONCAT("Hello", " ") // "Hello ")"')
        assert result == ['CONCAT("Hello", " ") // "Hello "']

    def test_raw_string_with_closing_paren_in_content(self):
        # Raw string where content contains ) but not )"
        result = gen.extract_string_literals('R"(ABS(-5) // 5)"')
        assert result == ['ABS(-5) // 5']

    def test_normal_strings_not_inside_raw(self):
        # Normal strings outside raw strings should be collected
        result = gen.extract_string_literals('"before" R"(raw)" "after"')
        assert 'before' in result
        assert 'after' in result
        assert 'raw' in result


# ---------------------------------------------------------------------------
# normalise_argtype
# ---------------------------------------------------------------------------

class TestNormaliseArgtype:

    def test_with_namespace(self):
        assert gen.normalise_argtype('ArgType::NUMBER') == 'NUMBER'

    def test_without_namespace(self):
        assert gen.normalise_argtype('STRING') == 'STRING'

    def test_pascal_case(self):
        assert gen.normalise_argtype('Array') == 'ARRAY'

    def test_unknown(self):
        assert gen.normalise_argtype('CUSTOM') == 'CUSTOM'


# ---------------------------------------------------------------------------
# parse_arg_spec_block
# ---------------------------------------------------------------------------

class TestParseArgSpecBlock:

    def test_named_fields(self):
        block = '{"name", ArgType::STRING, true, nullptr, "Input string"}'
        arg = gen.parse_arg_spec_block(block)
        assert arg.name == 'name'
        assert arg.arg_type == 'STRING'
        assert arg.required is True
        assert arg.description == 'Input string'

    def test_optional_arg(self):
        block = '{"limit", ArgType::INTEGER, false, nullptr, "Max results"}'
        arg = gen.parse_arg_spec_block(block)
        assert arg.required is False

    def test_named_fields_doxygen(self):
        block = '{.name = "query", .type = ArgType::STRING, .required = true, .description = "Search query"}'
        arg = gen.parse_arg_spec_block(block)
        assert arg.name == 'query'
        assert arg.arg_type == 'STRING'
        assert arg.description == 'Search query'


# ---------------------------------------------------------------------------
# parse_named_signature (named initializer)
# ---------------------------------------------------------------------------

class TestParseNamedSignature:

    def _block(self, name, cat, desc, args_str='{}', ret='ArgType::ANY',
                examples_str='{}', is_det='true', is_agg='false'):
        return (
            f'{{ .name = "{name}", '
            f'.category = "{cat}", '
            f'.description = "{desc}", '
            f'.arguments = {args_str}, '
            f'.return_type = {ret}, '
            f'.is_deterministic = {is_det}, '
            f'.is_aggregate = {is_agg}, '
            f'.examples = {examples_str} }}'
        )

    def test_basic(self):
        block = self._block('ABS', 'Math', 'Absolute value',
                             '{{"num", ArgType::NUMBER, true, nullptr, "Number"}}',
                             'ArgType::NUMBER',
                             '{R"(ABS(-5) // 5)"}')
        entry = gen.parse_named_signature(block)
        assert entry is not None
        assert entry.name == 'ABS'
        assert entry.category == 'Math'
        assert entry.description == 'Absolute value'
        assert entry.return_type == 'NUMBER'
        assert len(entry.arguments) == 1
        assert entry.arguments[0].name == 'num'
        assert entry.examples == ['ABS(-5) // 5']

    def test_aggregate(self):
        block = self._block('SUM', 'Math', 'Sum of values',
                             '{{"values", ArgType::ARRAY, true, nullptr, "Input"}}',
                             'ArgType::NUMBER',
                             '{"SUM([1,2,3])"}',
                             is_agg='true')
        entry = gen.parse_named_signature(block)
        assert entry.is_aggregate is True

    def test_no_name_returns_none(self):
        block = '{.category = "Math", .description = "no name"}'
        entry = gen.parse_named_signature(block)
        assert entry is None or not entry.name


# ---------------------------------------------------------------------------
# parse_positional_signature
# ---------------------------------------------------------------------------

class TestParsePositionalSignature:

    def test_basic_positional(self):
        block = textwrap.dedent('''\
            {
                "GEO_DISTANCE",
                "Geo",
                "Calculate great-circle distance in meters",
                {
                    {"geom1", ArgType::GEOMETRY, true, nullptr, "First geometry"},
                    {"geom2", ArgType::GEOMETRY, true, nullptr, "Second geometry"}
                },
                ArgType::NUMBER,
                true,
                false,
                {"GEO_DISTANCE(point1, point2)"}
            }''')
        entry = gen.parse_positional_signature(block)
        assert entry is not None
        assert entry.name == 'GEO_DISTANCE'
        assert entry.category == 'Geo'
        assert entry.description == 'Calculate great-circle distance in meters'
        assert entry.return_type == 'NUMBER'
        assert entry.is_deterministic is True
        assert entry.is_aggregate is False
        assert len(entry.arguments) == 2
        assert entry.examples == ['GEO_DISTANCE(point1, point2)']

    def test_description_with_parens(self):
        # Description contains '(ArangoDB compatible)' – must NOT be truncated
        block = textwrap.dedent('''\
            {
                "FUNC",
                "Cat",
                "Does something (ArangoDB compatible)",
                {},
                ArgType::ANY,
                true,
                false,
                {}
            }''')
        entry = gen.parse_positional_signature(block)
        assert entry is not None
        assert entry.description == 'Does something (ArangoDB compatible)'


# ---------------------------------------------------------------------------
# parse_header (full file integration)
# ---------------------------------------------------------------------------

class TestParseHeader:

    def _named_header(self):
        return textwrap.dedent('''\
            #pragma once
            namespace themis { namespace query { namespace functions {

            /**
             * @brief UPPER(str) - Uppercase
             */
            class UpperFunction : public IFunction {
            public:
                FunctionSignature signature() const override {
                    return {
                        .name = "UPPER",
                        .category = "String",
                        .description = "Converts a string to uppercase",
                        .arguments = {
                            {"str", ArgType::STRING, true, nullptr, "Input string"}
                        },
                        .return_type = ArgType::STRING,
                        .is_deterministic = true,
                        .examples = {R"(UPPER("hello") // "HELLO")"}
                    };
                }
            };

            }}} // namespaces
            ''')

    def _positional_header(self):
        return textwrap.dedent('''\
            #pragma once
            namespace themis { namespace query { namespace functions {

            class GeoDistFunc : public IFunction {
            public:
                FunctionSignature signature() const override {
                    return {
                        "GEODIST",
                        "Geo",
                        "Haversine distance",
                        {
                            {"p1", ArgType::GEOMETRY, true, nullptr, "Point 1"},
                            {"p2", ArgType::GEOMETRY, true, nullptr, "Point 2"}
                        },
                        ArgType::NUMBER,
                        true,
                        false,
                        {"GEODIST(a, b)"}
                    };
                }
            };

            }}} // namespaces
            ''')

    def test_parse_named_header(self, tmp_path):
        header = make_header(tmp_path, 'str_funcs.h', self._named_header())
        entries = gen.parse_header(header)
        assert len(entries) == 1
        e = entries[0]
        assert e.name == 'UPPER'
        assert e.category == 'String'
        assert e.return_type == 'STRING'
        assert e.source_file == 'str_funcs.h'
        assert len(e.arguments) == 1
        assert 'UPPER("hello")' in e.examples[0]

    def test_parse_positional_header(self, tmp_path):
        header = make_header(tmp_path, 'geo_funcs.h', self._positional_header())
        entries = gen.parse_header(header)
        assert len(entries) == 1
        e = entries[0]
        assert e.name == 'GEODIST'
        assert e.category == 'Geo'
        assert e.return_type == 'NUMBER'
        assert len(e.arguments) == 2

    def test_multiple_functions(self, tmp_path):
        content = textwrap.dedent('''\
            #pragma once
            namespace themis { namespace query { namespace functions {

            class F1 : public IFunction {
            public:
                FunctionSignature signature() const override {
                    return {
                        .name = "FUNC1",
                        .category = "Math",
                        .description = "First function",
                        .arguments = {},
                        .return_type = ArgType::NUMBER,
                        .examples = {}
                    };
                }
            };

            class F2 : public IFunction {
            public:
                FunctionSignature signature() const override {
                    return {
                        .name = "FUNC2",
                        .category = "String",
                        .description = "Second function",
                        .arguments = {},
                        .return_type = ArgType::STRING,
                        .examples = {}
                    };
                }
            };

            }}} // namespaces
            ''')
        header = make_header(tmp_path, 'multi.h', content)
        entries = gen.parse_header(header)
        assert len(entries) == 2
        names = {e.name for e in entries}
        assert names == {'FUNC1', 'FUNC2'}

    def test_empty_header(self, tmp_path):
        header = make_header(tmp_path, 'empty.h', '#pragma once\n')
        entries = gen.parse_header(header)
        assert entries == []


# ---------------------------------------------------------------------------
# generate_markdown
# ---------------------------------------------------------------------------

class TestGenerateMarkdown:

    def _sample_entries(self):
        e1 = gen.FunctionEntry(
            name='UPPER', category='String', description='Uppercase',
            arguments=[gen.ArgSpec('str', 'STRING', True, 'Input string')],
            return_type='STRING', examples=['UPPER("hello") // "HELLO"'],
            source_file='string_functions.h'
        )
        e2 = gen.FunctionEntry(
            name='ABS', category='Math', description='Absolute value',
            arguments=[gen.ArgSpec('num', 'NUMBER', True, 'Number')],
            return_type='NUMBER', examples=['ABS(-5) // 5'],
            source_file='math_functions.h'
        )
        return [e1, e2]

    def test_produces_markdown(self):
        md = gen.generate_markdown(self._sample_entries())
        assert md.startswith('# AQL Functions Reference')

    def test_contains_function_names(self):
        md = gen.generate_markdown(self._sample_entries())
        assert '### UPPER' in md
        assert '### ABS' in md

    def test_categories_in_toc(self):
        md = gen.generate_markdown(self._sample_entries())
        assert 'String Functions' in md
        assert 'Math Functions' in md

    def test_examples_in_aql_block(self):
        md = gen.generate_markdown(self._sample_entries())
        assert '```aql' in md
        assert 'ABS(-5) // 5' in md

    def test_arg_table(self):
        md = gen.generate_markdown(self._sample_entries())
        assert '| `str` |' in md
        assert '`string`' in md

    def test_source_file_reference(self):
        md = gen.generate_markdown(self._sample_entries())
        assert '`string_functions.h`' in md

    def test_optional_arg_marker(self):
        e = gen.FunctionEntry(
            name='FOO', category='Test', description='Test func',
            arguments=[
                gen.ArgSpec('required_arg', 'STRING', True, 'Req'),
                gen.ArgSpec('optional_arg', 'STRING', False, 'Opt'),
            ],
            return_type='STRING',
        )
        md = gen.generate_markdown([e])
        assert 'optional_arg?' in md
        assert 'required_arg,' in md or 'required_arg)' in md

    def test_aggregate_label(self):
        e = gen.FunctionEntry(
            name='SUMFUNC', category='Math', description='Sum',
            is_aggregate=True, return_type='NUMBER',
        )
        md = gen.generate_markdown([e])
        assert 'Aggregate' in md

    def test_non_deterministic_label(self):
        e = gen.FunctionEntry(
            name='RAND', category='Math', description='Random',
            is_deterministic=False, return_type='NUMBER',
        )
        md = gen.generate_markdown([e])
        assert 'Non-deterministic' in md


# ---------------------------------------------------------------------------
# Integration: parse real headers
# ---------------------------------------------------------------------------

class TestRealHeaders:
    """Smoke-test against the actual include/query/functions/ directory."""

    _HEADERS_DIR = (
        Path(__file__).parent.parent / 'include' / 'query' / 'functions'
    )
    _SKIP = gen.SKIP_HEADERS

    def test_headers_dir_exists(self):
        assert self._HEADERS_DIR.is_dir(), \
            f'Headers dir not found: {self._HEADERS_DIR}'

    def test_parses_at_least_50_functions(self):
        if not self._HEADERS_DIR.is_dir():
            return
        entries = []
        for h in self._HEADERS_DIR.glob('*.h'):
            if h.name not in self._SKIP:
                entries.extend(gen.parse_header(h))
        assert len(entries) >= 50, f'Expected >= 50 functions, got {len(entries)}'

    def test_all_entries_have_name_and_category(self):
        if not self._HEADERS_DIR.is_dir():
            return
        entries = []
        for h in self._HEADERS_DIR.glob('*.h'):
            if h.name not in self._SKIP:
                entries.extend(gen.parse_header(h))
        missing_name = [e for e in entries if not e.name]
        missing_cat = [e for e in entries if not e.category]
        assert not missing_name, f'Entries without name: {missing_name}'
        assert not missing_cat, f'Entries without category: {missing_cat}'

    def test_string_functions_present(self):
        if not self._HEADERS_DIR.is_dir():
            return
        h = self._HEADERS_DIR / 'string_functions.h'
        if not h.exists():
            return
        entries = gen.parse_header(h)
        names = {e.name for e in entries}
        for func in ('UPPER', 'LOWER', 'LENGTH', 'CONCAT'):
            assert func in names, f'{func} not found in string_functions.h'

    def test_math_functions_present(self):
        if not self._HEADERS_DIR.is_dir():
            return
        h = self._HEADERS_DIR / 'math_functions.h'
        if not h.exists():
            return
        entries = gen.parse_header(h)
        names = {e.name for e in entries}
        for func in ('ABS', 'CEIL', 'FLOOR', 'ROUND'):
            assert func in names, f'{func} not found in math_functions.h'

    def test_geo_distance_description_not_truncated(self):
        if not self._HEADERS_DIR.is_dir():
            return
        h = self._HEADERS_DIR / 'geo_functions.h'
        if not h.exists():
            return
        entries = gen.parse_header(h)
        geo_dist = next((e for e in entries if e.name == 'GEO_DISTANCE'), None)
        if geo_dist is None:
            return
        # Description must not start with '"' (regression: positional parser bug)
        assert not geo_dist.description.startswith('"'), \
            f'GEO_DISTANCE description starts with quote: {geo_dist.description!r}'
        # Description must be reasonably long
        assert len(geo_dist.description) > 10, \
            f'GEO_DISTANCE description too short: {geo_dist.description!r}'

    def test_generate_full_markdown(self):
        """End-to-end: full parse + markdown generation must not raise."""
        if not self._HEADERS_DIR.is_dir():
            return
        entries = []
        for h in self._HEADERS_DIR.glob('*.h'):
            if h.name not in self._SKIP:
                entries.extend(gen.parse_header(h))
        md = gen.generate_markdown(entries)
        assert '# AQL Functions Reference' in md
        assert len(md) > 1000
