"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_aql_docs.py                               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:29:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     712                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8546b76889  2026-02-23  chore(aql): code audit - remove dead code, fix README link ║
    • 73784c95e7  2026-02-23  refactor(aql): address code review - export SKIP_HEADERS ... ║
    • 0c7487f402  2026-02-23  feat(aql): add doc auto-generation script and reference docs ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ThemisDB AQL Function Documentation Generator
==============================================

Parses C++ header files in ``<repo-root>/include/query/functions/`` and generates
a Markdown function reference document from the embedded FunctionSignature metadata.

Usage:
    python3 scripts/generate_aql_docs.py
    python3 scripts/generate_aql_docs.py --headers-dir include/query/functions \\
        --output docs/en/aql/aql_functions_reference.md
    python3 scripts/generate_aql_docs.py --dry-run   # print to stdout
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class ArgSpec:
    name: str = ""
    arg_type: str = "ANY"
    required: bool = True
    description: str = ""


@dataclass
class FunctionEntry:
    name: str = ""
    category: str = ""
    description: str = ""
    arguments: list = field(default_factory=list)
    return_type: str = "ANY"
    is_deterministic: bool = True
    is_aggregate: bool = False
    examples: list = field(default_factory=list)
    source_file: str = ""
    brief: str = ""


# ---------------------------------------------------------------------------
# Module-level constants
# ---------------------------------------------------------------------------

# Headers to skip: infrastructure files that don't define FunctionSignature classes
# compatible with themis::query::functions::FunctionSignature, or that use a
# completely different struct layout (e.g. ai_ml_functions.h uses themisdb namespace).
SKIP_HEADERS: frozenset = frozenset({
    'function_registry.h',
    'function_adapter.h',
    'holiday_provider.h',
    'ai_ml_functions.h',
})

# How many characters to search *backwards* from a signature() method to find
# the nearest preceding Doxygen /** ... */ comment block.  800 chars comfortably
# covers a typical class definition + multi-line comment without false-positives.
_DOXYGEN_LOOKBACK = 800


# ---------------------------------------------------------------------------
# Brace-balanced extraction helper
# ---------------------------------------------------------------------------

def extract_balanced_braces(text: str, start: int) -> Optional[str]:
    """Return the substring from the opening '{' at *start* to its matching '}'.

    Returns ``None`` if *start* does not point at a '{' or the braces are unbalanced.
    """
    if start >= len(text) or text[start] != '{':
        return None
    depth = 0
    i = start
    in_string = False
    escape = False
    string_char = ''
    while i < len(text):
        ch = text[i]
        if escape:
            escape = False
        elif ch == '\\' and in_string:
            escape = True
        elif in_string:
            if ch == string_char:
                in_string = False
        elif ch in ('"', "'"):
            in_string = True
            string_char = ch
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
        i += 1
    return None


# ---------------------------------------------------------------------------
# String literal extraction
# ---------------------------------------------------------------------------

_STR_LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')
_RAW_STR_LITERAL = re.compile(r'R"\((.+?)\)"', re.DOTALL)


def extract_string_literals(text: str) -> list:
    """Return all string literal values found in *text* (raw and normal)."""
    # Collect raw-string spans first so normal-string extraction can skip them
    raw_spans = []
    results = []
    for m in _RAW_STR_LITERAL.finditer(text):
        results.append((m.start(), m.group(1)))
        raw_spans.append((m.start(), m.end()))

    # Normal strings – skip any region already covered by a raw string
    for m in _STR_LITERAL.finditer(text):
        if any(rs <= m.start() < re_ for rs, re_ in raw_spans):
            continue
        results.append((m.start(), m.group(1)))

    results.sort(key=lambda x: x[0])
    return [v for _, v in results]


# ---------------------------------------------------------------------------
# ArgType normalisation
# ---------------------------------------------------------------------------

_ARGTYPE_MAP = {
    'ANY': 'ANY', 'STRING': 'STRING', 'NUMBER': 'NUMBER',
    'INTEGER': 'INTEGER', 'BOOLEAN': 'BOOLEAN', 'ARRAY': 'ARRAY',
    'OBJECT': 'OBJECT', 'GEOMETRY': 'GEOMETRY', 'VECTOR': 'VECTOR',
    'DOCUMENT': 'DOCUMENT', 'NULLABLE': 'NULLABLE',
    # PascalCase aliases
    'Any': 'ANY', 'String': 'STRING', 'Number': 'NUMBER',
    'Integer': 'INTEGER', 'Boolean': 'BOOLEAN',
    'Array': 'ARRAY', 'Object': 'OBJECT', 'Geometry': 'GEOMETRY',
    'Vector': 'VECTOR', 'Document': 'DOCUMENT', 'Nullable': 'NULLABLE',
}


def normalise_argtype(raw: str) -> str:
    """Strip leading namespace qualifiers and return the canonical ArgType name."""
    # "ArgType::NUMBER" -> "NUMBER"
    s = raw.strip()
    if '::' in s:
        s = s.rsplit('::', 1)[-1]
    return _ARGTYPE_MAP.get(s.strip(), s.strip())


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def parse_arg_spec_block(block: str) -> ArgSpec:
    """Parse a single ``{...}`` argument spec block into an :class:`ArgSpec`."""
    arg = ArgSpec()

    # Named fields: .name = "...", .type = ArgType::..., .required = bool, .description = "..."
    named_name = re.search(r'\.name\s*=\s*"([^"]*)"', block)
    named_type = re.search(r'\.type\s*=\s*(ArgType::\w+|\w+)', block)
    named_req = re.search(r'\.required\s*=\s*(true|false)', block)
    named_desc = re.search(r'\.description\s*=\s*"([^"]*)"', block)

    if named_name:
        arg.name = named_name.group(1)
        if named_type:
            arg.arg_type = normalise_argtype(named_type.group(1))
        if named_req:
            arg.required = named_req.group(1) == 'true'
        if named_desc:
            arg.description = named_desc.group(1)
        return arg

    # Positional: {"name", ArgType::TYPE, bool, default, "desc"}
    strings = extract_string_literals(block)
    # Find ArgType in the block
    type_match = re.search(r'ArgType::(\w+)', block)
    # Find required bool
    req_match = re.search(r',\s*(true|false)\s*,', block)

    if strings:
        arg.name = strings[0]
        if len(strings) > 1:
            arg.description = strings[-1]
    if type_match:
        arg.arg_type = normalise_argtype('ArgType::' + type_match.group(1))
    if req_match:
        arg.required = req_match.group(1) == 'true'
    return arg


def parse_arguments_block(args_block: str) -> list:
    """Parse the ``{...}`` arguments initialiser block into a list of :class:`ArgSpec`."""
    # Remove the outer braces
    inner = args_block.strip()
    if inner.startswith('{') and inner.endswith('}'):
        inner = inner[1:-1].strip()
    if not inner:
        return []

    # Split on top-level commas that separate argument specs
    specs = []
    depth = 0
    current = []
    for ch in inner:
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        if ch == ',' and depth == 0:
            specs.append(''.join(current).strip())
            current = []
        else:
            current.append(ch)
    if current:
        specs.append(''.join(current).strip())

    result = []
    for spec in specs:
        spec = spec.strip()
        if not spec:
            continue
        # Each argument spec should be enclosed in its own { }
        if not spec.startswith('{'):
            spec = '{' + spec + '}'
        arg = parse_arg_spec_block(spec)
        if arg.name:
            result.append(arg)
    return result


# ---------------------------------------------------------------------------
# FunctionSignature return-block parsing
# ---------------------------------------------------------------------------

def parse_named_signature(block: str) -> Optional[FunctionEntry]:
    """Parse a ``return { .name = ..., ... }`` named-field initialiser."""
    name_m = re.search(r'\.name\s*=\s*"([^"]*)"', block)
    if not name_m:
        return None

    entry = FunctionEntry()
    entry.name = name_m.group(1)

    cat_m = re.search(r'\.category\s*=\s*"([^"]*)"', block)
    if cat_m:
        entry.category = cat_m.group(1)

    desc_m = re.search(r'\.description\s*=\s*"([^"]*)"', block)
    if desc_m:
        entry.description = desc_m.group(1)

    ret_m = re.search(r'\.return_type\s*=\s*(ArgType::\w+|\w+)', block)
    if ret_m:
        entry.return_type = normalise_argtype(ret_m.group(1))

    det_m = re.search(r'\.is_deterministic\s*=\s*(true|false)', block)
    if det_m:
        entry.is_deterministic = det_m.group(1) == 'true'

    agg_m = re.search(r'\.is_aggregate\s*=\s*(true|false)', block)
    if agg_m:
        entry.is_aggregate = agg_m.group(1) == 'true'

    # Arguments block
    args_m = re.search(r'\.arguments\s*=\s*(\{)', block)
    if args_m:
        args_block = extract_balanced_braces(block, args_m.start(1))
        if args_block:
            entry.arguments = parse_arguments_block(args_block)

    # Examples block
    ex_m = re.search(r'\.examples\s*=\s*(\{)', block)
    if ex_m:
        ex_block = extract_balanced_braces(block, ex_m.start(1))
        if ex_block:
            entry.examples = extract_string_literals(ex_block)

    return entry


def parse_positional_signature(block: str) -> Optional[FunctionEntry]:
    """Parse a positional ``return { "NAME", "Cat", "Desc", {...}, ArgType::..., ... }``."""
    # The positional order: name, category, description, arguments, return_type,
    #   is_deterministic, is_aggregate, examples, cost
    inner = block.strip()
    if inner.startswith('{') and inner.endswith('}'):
        inner = inner[1:-1].strip()

    # Split top-level comma-separated tokens (respecting nested braces and strings)
    tokens = []
    depth = 0
    in_string = False
    escape = False
    string_char = ''
    current = []
    i = 0
    while i < len(inner):
        ch = inner[i]
        if escape:
            escape = False
        elif ch == '\\' and in_string:
            escape = True
        elif in_string:
            if ch == string_char:
                in_string = False
        elif ch in ('"', "'"):
            in_string = True
            string_char = ch
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1

        if ch == ',' and depth == 0 and not in_string:
            tokens.append(''.join(current).strip())
            current = []
        else:
            current.append(ch)
        i += 1
    if current:
        tokens.append(''.join(current).strip())

    if len(tokens) < 2:
        return None

    entry = FunctionEntry()

    # Collect the initial string literals (name, category, description)
    string_tokens = []
    remaining_idx = 0
    for idx, tok in enumerate(tokens):
        tok_s = tok.strip()
        if tok_s.startswith('R"(') and ')' + '"' in tok_s:
            # Raw string: strip R"( prefix and )" suffix
            inner_raw = re.sub(r'^R"\(', '', tok_s)
            inner_raw = re.sub(r'\)"$', '', inner_raw)
            string_tokens.append(inner_raw)
        elif tok_s.startswith('"') and tok_s.endswith('"') and len(tok_s) >= 2:
            string_tokens.append(tok_s[1:-1])
        else:
            remaining_idx = idx
            break

    if len(string_tokens) < 1:
        return None

    entry.name = string_tokens[0] if len(string_tokens) > 0 else ''
    entry.category = string_tokens[1] if len(string_tokens) > 1 else ''
    entry.description = string_tokens[2] if len(string_tokens) > 2 else ''

    # After string tokens, next is arguments {}, return_type, is_det, is_agg, examples {}
    # Find the first { token for arguments
    rest_tokens = tokens[remaining_idx:]
    rest_str = ','.join(rest_tokens)

    # Arguments block: find the first { ... }
    first_brace = rest_str.find('{')
    if first_brace >= 0:
        args_block = extract_balanced_braces(rest_str, first_brace)
        if args_block:
            entry.arguments = parse_arguments_block(args_block)
            # After arguments block, continue
            after_args = rest_str[first_brace + len(args_block):]
            # Look for return type
            rt_m = re.search(r'ArgType::(\w+)', after_args)
            if rt_m:
                entry.return_type = normalise_argtype('ArgType::' + rt_m.group(1))
            # Look for booleans (is_deterministic, is_aggregate)
            bools = re.findall(r'\b(true|false)\b', after_args)
            if bools:
                entry.is_deterministic = bools[0] == 'true'
            if len(bools) > 1:
                entry.is_aggregate = bools[1] == 'true'
            # Examples block: second { ... }
            ex_brace = after_args.find('{')
            if ex_brace >= 0:
                ex_block = extract_balanced_braces(after_args, ex_brace)
                if ex_block:
                    entry.examples = extract_string_literals(ex_block)

    return entry if entry.name else None


def parse_signature_block(block: str) -> Optional[FunctionEntry]:
    """Try named parsing first, then positional."""
    entry = parse_named_signature(block)
    if entry and entry.name:
        return entry
    return parse_positional_signature(block)


# ---------------------------------------------------------------------------
# Brief comment extraction
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Brief comment extraction
# ---------------------------------------------------------------------------


def extract_brief(comment_block: str) -> str:
    """Extract the @brief text from a Doxygen comment block."""
    m = re.search(r'@brief\s+(.+)', comment_block)
    if m:
        return m.group(1).strip()
    return ''


# ---------------------------------------------------------------------------
# Header file parser
# ---------------------------------------------------------------------------

def parse_header(path: Path) -> list:
    """Parse a single C++ header file and return a list of :class:`FunctionEntry`."""
    text = path.read_text(encoding='utf-8', errors='replace')
    entries = []

    # Find all signature() method implementations
    # Pattern: signature() const override {\n    return {
    sig_method_re = re.compile(
        r'FunctionSignature\s+signature\s*\(\s*\)\s+const\s+override\s*\{',
        re.MULTILINE
    )

    for method_match in sig_method_re.finditer(text):
        method_start = method_match.start()

        # Find the opening brace of the method body
        brace_start = text.find('{', method_match.end() - 1)
        if brace_start < 0:
            continue

        # Extract the method body
        method_body = extract_balanced_braces(text, brace_start)
        if not method_body:
            continue

        # Find the return { ... } statement inside the method
        ret_match = re.search(r'\breturn\s*(\{)', method_body)
        if not ret_match:
            continue
        ret_brace_start = ret_match.start(1)
        sig_block = extract_balanced_braces(method_body, ret_brace_start)
        if not sig_block:
            continue

        entry = parse_signature_block(sig_block)
        if not entry or not entry.name:
            continue

        entry.source_file = path.name

        # Look for a @brief comment in the _DOXYGEN_LOOKBACK chars preceding the class definition
        # Search backwards from method_start for the class definition
        before = text[max(0, method_start - _DOXYGEN_LOOKBACK):method_start]
        # Find the last /** ... */ comment block before this method
        comments = list(re.finditer(r'/\*\*.*?\*/', before, re.DOTALL))
        if comments:
            last_comment = comments[-1].group(0)
            entry.brief = extract_brief(last_comment)

        entries.append(entry)

    return entries


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

_ARGTYPE_DISPLAY = {
    'ANY': 'any', 'STRING': 'string', 'NUMBER': 'number',
    'INTEGER': 'integer', 'BOOLEAN': 'boolean', 'ARRAY': 'array',
    'OBJECT': 'object', 'GEOMETRY': 'geometry', 'VECTOR': 'vector',
    'DOCUMENT': 'document', 'NULLABLE': 'nullable',
}

# Preferred ordering of categories in the output document
_CATEGORY_ORDER = [
    'String', 'Math', 'Array', 'Date', 'Document', 'JSON',
    'Geo', 'Vector', 'Graph', 'Relational', 'Window',
    'Security', 'File', 'Collection', 'LLM', 'LoRA',
    'Ethics', 'Process Mining', 'Fulltext',
]


def _argtype(t: str) -> str:
    return _ARGTYPE_DISPLAY.get(t, t.lower())


def _signature_line(entry: FunctionEntry) -> str:
    """Build a one-line function signature string like NAME(arg1, arg2?) -> TYPE."""
    args = []
    for a in entry.arguments:
        suffix = '' if a.required else '?'
        args.append(f'{a.name}{suffix}')
    ret = _argtype(entry.return_type)
    return f"`{entry.name}({', '.join(args)})` → `{ret}`"


def _arg_table(entry: FunctionEntry) -> str:
    if not entry.arguments:
        return ''
    rows = ['| Parameter | Type | Required | Description |',
            '|-----------|------|----------|-------------|']
    for a in entry.arguments:
        req = '✅' if a.required else '—'
        rows.append(f'| `{a.name}` | `{_argtype(a.arg_type)}` | {req} | {a.description} |')
    return '\n'.join(rows)


def generate_markdown(entries: list) -> str:
    """Convert a list of :class:`FunctionEntry` to a Markdown reference document."""
    now = datetime.now(timezone.utc).strftime('%Y-%m-%d')

    lines = [
        '# AQL Functions Reference',
        '',
        f'**Generated:** {now}  ',
        '**Source:** Auto-generated from C++ header files in `include/query/functions/`  ',
        '**Do not edit manually** – re-run `scripts/generate_aql_docs.py` to update.',
        '',
        '---',
        '',
        '## Table of Contents',
        '',
    ]

    # Collect categories in preferred order, then alphabetically for unknowns
    by_cat: dict = {}
    for e in entries:
        cat = e.category or 'Uncategorized'
        by_cat.setdefault(cat, []).append(e)

    ordered_cats = []
    for cat in _CATEGORY_ORDER:
        if cat in by_cat:
            ordered_cats.append(cat)
    for cat in sorted(by_cat.keys()):
        if cat not in ordered_cats:
            ordered_cats.append(cat)

    for cat in ordered_cats:
        anchor = cat.lower().replace(' ', '-')
        lines.append(f'- [{cat} Functions](#{anchor}-functions)')

    lines += ['', '---', '']

    for cat in ordered_cats:
        cat_entries = sorted(by_cat[cat], key=lambda e: e.name)
        anchor = cat.lower().replace(' ', '-')
        lines += [f'## {cat} Functions', '']

        # Summary table
        lines += [
            f'| Function | Description |',
            f'|----------|-------------|',
        ]
        for e in cat_entries:
            brief = e.description or e.brief
            lines.append(f'| [{e.name}](#{e.name.lower()}) | {brief} |')

        lines.append('')

        # Detailed entries
        for e in cat_entries:
            lines += [
                f'### {e.name}',
                '',
                f'**Signature:** {_signature_line(e)}  ',
            ]
            if e.is_aggregate:
                lines[-1] += '  \n**Aggregate:** ✅'
            if not e.is_deterministic:
                lines[-1] += '  \n**Non-deterministic** (result may vary)'
            lines += ['']

            desc = e.description or e.brief
            if desc:
                lines += [desc, '']

            arg_table = _arg_table(e)
            if arg_table:
                lines += ['**Parameters:**', '', arg_table, '']

            if e.examples:
                lines += ['**Examples:**', '']
                lines += ['```aql']
                for ex in e.examples:
                    lines.append(ex)
                lines += ['```', '']

            lines += [f'*Source: `{e.source_file}`*', '', '---', '']

    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description='Generate AQL function reference documentation from C++ headers.')
    p.add_argument(
        '--headers-dir',
        default=None,
        help='Path to include/query/functions/ directory '
             '(default: auto-detected relative to script location)')
    p.add_argument(
        '--output',
        default=None,
        help='Output Markdown file path '
             '(default: docs/en/aql/aql_functions_reference.md relative to repo root)')
    p.add_argument(
        '--dry-run',
        action='store_true',
        help='Print generated Markdown to stdout instead of writing to file')
    return p


def _find_repo_root() -> Path:
    """Walk up from this script until we find a CMakeLists.txt or .git directory."""
    here = Path(__file__).resolve()
    for parent in [here.parent] + list(here.parents):
        if (parent / 'CMakeLists.txt').exists() or (parent / '.git').exists():
            return parent
    return here.parent


def main(argv=None) -> int:
    args = build_arg_parser().parse_args(argv)

    repo_root = _find_repo_root()

    headers_dir = Path(args.headers_dir) if args.headers_dir else \
        repo_root / 'include' / 'query' / 'functions'

    if not headers_dir.is_dir():
        print(f'ERROR: headers directory not found: {headers_dir}', file=sys.stderr)
        return 1

    # Exclude infrastructure headers and files that use a different FunctionSignature struct
    all_entries = []
    for header in sorted(headers_dir.glob('*.h')):
        if header.name in SKIP_HEADERS:
            continue
        entries = parse_header(header)
        all_entries.extend(entries)

    if not all_entries:
        print('WARNING: no AQL function entries found', file=sys.stderr)
        return 0

    print(f'Parsed {len(all_entries)} AQL functions from {headers_dir}', file=sys.stderr)

    markdown = generate_markdown(all_entries)

    if args.dry_run:
        print(markdown)
        return 0

    output_path = Path(args.output) if args.output else \
        repo_root / 'docs' / 'en' / 'aql' / 'aql_functions_reference.md'

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(markdown, encoding='utf-8')
    print(f'Written: {output_path}', file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
