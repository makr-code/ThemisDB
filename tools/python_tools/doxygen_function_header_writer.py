#!/usr/bin/env python3
"""Generate or enrich Doxygen function headers for C/C++ files.

This tool is intentionally outside the scanner pipeline.
It can add missing Doxygen blocks above function declarations/definitions with:
- @brief
- @tparam (for template declarations)
- @param[in|out|in,out]
- @return (for non-void)
- @throws (heuristic for function definitions)
- @details (summary of detected function calls for definitions)

Generated text prefers short heuristic descriptions over placeholder-heavy TBD
entries. Default mode is dry-run. Use --apply to write changes after review.

Single-line Doxygen `///` comments are preserved as-is. Multi-line `///`
comment blocks and non-Doxygen comments are rewritten into `/** ... */` blocks.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

CPP_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx", ".ipp", ".tpp"}

EXCLUDED_DIRS = {
    ".git",
    ".venv",
    ".vscode",
    "build",
    "build-msvc-windows-debug",
    "build-msvc-windows-release",
    "vcpkg",
    "vcpkg_installed",
    "vcpkg_installed_linux",
    "third_party",
    "external",
    "node_modules",
    "artifacts",
}

CONTROL_KEYWORDS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "alignof",
    "decltype",
    "catch",
    "static_cast",
    "dynamic_cast",
    "reinterpret_cast",
    "const_cast",
    "new",
    "delete",
}

FUNC_START_RE = re.compile(
    r"^\s*(?:template\b|inline\b|static\b|constexpr\b|virtual\b|explicit\b|friend\b|extern\b|[A-Za-z_~])"
)
SIG_DECL_RE = re.compile(
    r"^\s*"
    r"(?:(?:template\s*<[^{};]+>)\s*)?"
    r"(?:(?:inline|static|constexpr|virtual|explicit|friend|extern|consteval|constinit|typename)\s+)*"
    r"(?P<ret>[A-Za-z_~][\w:\<\>\s\*&]+?)\s+"
    r"(?P<name>[~A-Za-z_]\w*(?:::\w+)*)\s*"
    r"\((?P<params>.*)\)\s*"
    r"(?:const\s*)?"
    r"(?:noexcept(?:\([^)]*\))?\s*)?"
    r"(?:->\s*[^;{]+\s*)?"
    r"(?:=\s*(?:0|default|delete)\s*)?"
    r"(?P<end>\{|;)(?:\s*.*)?$"
)
DOXYGEN_START_RE = re.compile(r"^\s*/\*\*")
CALL_RE = re.compile(r"\b([A-Za-z_]\w*(?:::\w+)*)\s*\(")


@dataclass
class ParameterDoc:
    name: str
    direction: str


@dataclass
class FunctionMatch:
    start_line: int
    end_line: int
    signature: str
    name: str
    return_type: str
    params: List[ParameterDoc]
    template_params: List[str]
    has_body: bool
    noexcept: bool


@dataclass
class ExistingComment:
    start_line: int
    end_line: int
    text: str


def should_skip_file(path: Path) -> bool:
    return any(part in EXCLUDED_DIRS for part in path.parts)


def iter_cpp_files(root: Path, include_paths: List[str], public_only: bool) -> Iterable[Path]:
    for rel in include_paths:
        base = (root / rel).resolve()
        if not base.exists() or not base.is_dir():
            continue
        for p in base.rglob("*"):
            if not p.is_file():
                continue
            if p.suffix.lower() not in CPP_EXTENSIONS:
                continue
            if should_skip_file(p):
                continue
            if public_only and "include" not in p.parts:
                continue
            yield p


def strip_strings_and_comments(text: str) -> str:
    text = re.sub(r"//.*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r'"(?:\\.|[^"\\])*"', '""', text)
    text = re.sub(r"'(?:\\.|[^'\\])*'", "''", text)
    return text


def looks_like_function_signature(sig: str) -> bool:
    s = " ".join(part.strip() for part in sig.splitlines())
    s = re.sub(r"\s+", " ", s).strip()
    if not s or "(" not in s or ")" not in s:
        return False
    if s.startswith("#"):
        return False

    has_body = False
    if "{" in s:
        s = s.split("{", 1)[0].rstrip()
        has_body = True
    if not s.endswith(";") and s.endswith(")"):
        s = s + ";"

    bad_prefixes = ("if", "for", "while", "switch", "catch", "return", "typedef", "using")
    if s.split()[0] in bad_prefixes:
        return False
    if "." in s.split("(", 1)[0] or "->" in s.split("(", 1)[0]:
        return False
    if "=" in s and "==" not in s and "=" not in s.split(")", 1)[-1]:
        return False
    return SIG_DECL_RE.match(s) is not None


def split_param_pieces(raw: str) -> List[str]:
    pieces: List[str] = []
    buf: List[str] = []
    depth_angle = 0
    depth_paren = 0
    for ch in raw:
        if ch == "," and depth_angle == 0 and depth_paren == 0:
            piece = "".join(buf).strip()
            if piece:
                pieces.append(piece)
            buf = []
            continue
        if ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle = max(0, depth_angle - 1)
        elif ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren = max(0, depth_paren - 1)
        buf.append(ch)
    tail = "".join(buf).strip()
    if tail:
        pieces.append(tail)
    return pieces


def is_likely_param_declaration(piece: str) -> bool:
    p = piece.strip()
    if not p:
        return False
    if p == "...":
        return True

    # Reject expression-like argument lists (local variable initialization etc.).
    if "." in p or "->" in p:
        return False
    if re.search(r"\b(return|if|for|while|switch)\b", p):
        return False
    if re.search(r"\s[+/%|^]\s", p):
        return False
    if re.search(r"\s-\s", p):
        return False

    decl_re = re.compile(
        r"^(?:const\s+|volatile\s+|constexpr\s+|typename\s+|class\s+|struct\s+)*"
        r"[A-Za-z_~][\w:<>\s\*&]*"
        r"(?:\s+[A-Za-z_]\w*|\s*\[[^\]]*\]|\s*\(\*\s*[A-Za-z_]\w*\s*\).*)?$"
    )
    return decl_re.match(p) is not None


def is_likely_function_param_list(raw_params: str) -> bool:
    if not raw_params:
        return True
    if raw_params.strip() == "void":
        return True
    pieces = split_param_pieces(raw_params)
    if not pieces:
        return True
    return all(is_likely_param_declaration(piece) for piece in pieces)


def infer_param_direction(param_decl: str) -> str:
    decl = re.sub(r"\s*=\s*.*$", "", param_decl).strip()
    if "&&" in decl:
        return "[in]"
    if "*" in decl:
        before_star = decl.split("*", 1)[0]
        return "[in]" if "const" in before_star.split() else "[in,out]"
    if "&" in decl:
        return "[in]" if re.search(r"\bconst\b", decl) else "[in,out]"
    return "[in]"


def parse_template_params(flat_signature: str) -> List[str]:
    stripped = flat_signature.lstrip()
    if not stripped.startswith("template"):
        return []
    lt = stripped.find("<")
    if lt < 0:
        return []

    depth = 0
    gt = -1
    for i in range(lt, len(stripped)):
        ch = stripped[i]
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
            if depth == 0:
                gt = i
                break
    if gt < 0:
        return []

    body = stripped[lt + 1 : gt]
    names: List[str] = []
    for piece in split_param_pieces(body):
        piece = re.sub(r"\s*=\s*.*$", "", piece).strip()
        m = re.search(r"([A-Za-z_]\w*)\s*$", piece)
        if m:
            names.append(m.group(1))
    return names


def parse_params(raw_params: str) -> List[ParameterDoc]:
    if not raw_params or raw_params == "void":
        return []

    docs: List[ParameterDoc] = []
    for p in split_param_pieces(raw_params):
        cleaned = re.sub(r"\s*=\s*.*$", "", p).strip()
        m = re.search(r"([A-Za-z_]\w*)\s*(\[.*\])?$", cleaned)
        name = m.group(1) if m else "param"
        docs.append(ParameterDoc(name=name, direction=infer_param_direction(cleaned)))
    return docs


def parse_signature(sig: str) -> Optional[FunctionMatch]:
    flat = " ".join(part.strip() for part in sig.splitlines())
    flat = re.sub(r"\s+", " ", flat).strip()
    has_body = False
    if "{" in flat:
        flat = flat.split("{", 1)[0].rstrip()
        has_body = True
    if not flat.endswith(";") and flat.endswith(")"):
        flat = flat + ";"

    m = SIG_DECL_RE.match(flat)
    if not m:
        return None

    raw_params = m.group("params").strip()
    if not is_likely_function_param_list(raw_params):
        return None

    return FunctionMatch(
        start_line=0,
        end_line=0,
        signature=flat,
        name=m.group("name"),
        return_type=m.group("ret").strip(),
        params=parse_params(raw_params),
        template_params=parse_template_params(flat),
        has_body=has_body or (m.group("end") == "{"),
        noexcept=("noexcept" in flat),
    )


def find_functions(lines: List[str]) -> List[FunctionMatch]:
    matches: List[FunctionMatch] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if not stripped or stripped.startswith("#") or stripped.startswith("//"):
            i += 1
            continue
        if "(" not in line:
            i += 1
            continue
        if not FUNC_START_RE.match(stripped):
            i += 1
            continue

        anchor = i
        accepted = False
        limit = min(len(lines), i + 16)
        for end in range(i, limit):
            candidate = "\n".join(lines[i : end + 1])
            candidate_clean = strip_strings_and_comments(candidate)
            if not looks_like_function_signature(candidate_clean):
                continue

            parsed = parse_signature(candidate_clean)
            if not parsed:
                continue

            parsed.start_line = anchor
            parsed.end_line = end
            parsed.has_body = "{" in candidate_clean
            matches.append(parsed)
            i = end + 1
            accepted = True
            break

        if not accepted:
            i += 1

    return matches


def find_immediate_doxygen_block(lines: List[str], func_start_line: int) -> Optional[Tuple[int, int]]:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return None

    if DOXYGEN_START_RE.match(lines[i]):
        j = i
        while j < len(lines):
            if "*/" in lines[j]:
                return (i, j)
            j += 1
        return None

    if lines[i].strip().endswith("*/"):
        end = i
        k = i
        while k >= 0:
            if DOXYGEN_START_RE.match(lines[k]):
                return (k, end)
            if lines[k].strip().startswith("/*"):
                return None
            k -= 1

    return None


def collect_multiline_triple_slash_comment(lines: List[str], func_start_line: int) -> Optional[ExistingComment]:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return None

    if not lines[i].lstrip().startswith("///"):
        return None

    end = i
    start = i
    while start - 1 >= 0 and lines[start - 1].lstrip().startswith("///"):
        start -= 1

    if start == end:
        return None

    text_parts = []
    for idx in range(start, end + 1):
        content = lines[idx].lstrip()[3:].strip()
        if content:
            text_parts.append(content)

    return ExistingComment(start_line=start, end_line=end, text=" ".join(text_parts).strip())


def has_immediate_single_line_triple_slash_doxygen(lines: List[str], func_start_line: int) -> bool:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return False

    if not lines[i].lstrip().startswith("///"):
        return False

    start = i
    while start - 1 >= 0 and lines[start - 1].lstrip().startswith("///"):
        start -= 1

    return start == i


def find_preceding_normal_comment(lines: List[str], func_start_line: int) -> Optional[ExistingComment]:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return None

    if DOXYGEN_START_RE.match(lines[i]):
        return None

    # Treat triple-slash Doxygen comments as already-documented input; do not rewrite them.
    if lines[i].lstrip().startswith("///"):
        return None

    if lines[i].lstrip().startswith("//"):
        end = i
        start = i
        while start - 1 >= 0:
            prev = lines[start - 1].strip()
            if prev.startswith("///"):
                return None
            if prev.startswith("//"):
                start -= 1
                continue
            break
        text_parts = []
        for idx in range(start, end + 1):
            text_parts.append(lines[idx].split("//", 1)[1].strip())
        text = " ".join(part for part in text_parts if part).strip()
        return ExistingComment(start_line=start, end_line=end, text=text)

    if lines[i].strip().endswith("*/"):
        end = i
        k = i
        while k >= 0 and "/*" not in lines[k]:
            k -= 1
        if k >= 0 and "/*" in lines[k] and not DOXYGEN_START_RE.match(lines[k]):
            text_parts = []
            for idx in range(k, end + 1):
                content = lines[idx]
                content = content.replace("/*", "").replace("*/", "")
                content = re.sub(r"^\s*\*\s?", "", content)
                content = content.strip()
                if content:
                    text_parts.append(content)
            text = " ".join(text_parts).strip()
            return ExistingComment(start_line=k, end_line=end, text=text)

    return None


def split_comment_text(comment_text: str, func_name: str) -> Tuple[str, str]:
    cleaned = re.sub(r"\s+", " ", comment_text or "").strip()
    if not cleaned:
        return f"TBD: Describe {func_name}.", "Calls: none detected."

    sentence_end = re.search(r"[.!?]", cleaned)
    if sentence_end:
        brief = cleaned[: sentence_end.end()].strip()
        tail = cleaned[sentence_end.end() :].strip()
    else:
        brief = cleaned
        tail = ""

    if not brief:
        brief = f"TBD: Describe {func_name}."

    details = tail if tail else "Calls: none detected."
    return brief, details


def find_function_body_block(lines: List[str], func: FunctionMatch) -> Optional[str]:
    if not func.has_body:
        return None

    start = func.start_line
    body_started = False
    depth = 0
    collected: List[str] = []

    for i in range(start, len(lines)):
        line = lines[i]
        clean = strip_strings_and_comments(line)
        for ch in clean:
            if ch == "{":
                depth += 1
                body_started = True
            if body_started:
                collected.append(ch)
            if ch == "}":
                depth -= 1
                if body_started and depth <= 0:
                    return "".join(collected)
    return None


def extract_called_functions(body: Optional[str], self_name: str) -> List[str]:
    if not body:
        return []

    thrown = set(extract_thrown_exceptions(body))
    names: List[str] = []
    for m in CALL_RE.finditer(body):
        call = m.group(1)
        base = call.split("::")[-1]
        if base in CONTROL_KEYWORDS:
            continue
        if base == self_name.split("::")[-1]:
            continue
        if base in thrown or call in thrown:
            continue
        names.append(call)

    unique: List[str] = []
    seen = set()
    for n in names:
        if n in seen:
            continue
        seen.add(n)
        unique.append(n)
    return unique[:8]


def extract_thrown_exceptions(body: Optional[str]) -> List[str]:
    if not body:
        return []
    names = re.findall(r"\bthrow\s+([A-Za-z_]\w*(?:::\w+)*)", body)
    unique: List[str] = []
    seen = set()
    for name in names:
        if name in seen:
            continue
        seen.add(name)
        unique.append(name)
    return unique[:3]


def is_void_return(return_type: str) -> bool:
    rt = return_type.strip()
    rt = re.sub(r"\b(static|inline|constexpr|virtual|explicit|friend|extern|mutable|typename)\b", "", rt)
    rt = re.sub(r"\s+", " ", rt).strip()
    return rt == "void"


def describe_param(direction: str) -> str:
    if direction == "[in,out]":
        return "Input/output parameter."
    if direction == "[out]":
        return "Output parameter."
    return "Input parameter."


def describe_return(return_type: str) -> str:
    normalized = re.sub(r"\s+", " ", return_type.strip()).lower()
    if normalized == "bool":
        return "True on success."
    if normalized.endswith("*"):
        return "Pointer to the result."
    return "Return value."


def describe_throws(thrown: List[str]) -> List[str]:
    if not thrown:
        return []
    return ["if an error occurs."] * len(thrown)


def describe_details(called: List[str], func_name: str) -> str:
    if called:
        joined = ", ".join(f"{c}()" for c in called)
        return f"Calls: {joined}."
    return f"Implements {func_name} without additional internal calls."


def build_doxygen_block(
    indent: str,
    func: FunctionMatch,
    called: List[str],
    thrown: List[str],
    existing_comment: Optional[ExistingComment],
) -> List[str]:
    name = func.name.split("::")[-1]
    if existing_comment:
        brief, details = split_comment_text(existing_comment.text, name)
    else:
        brief = f"TBD: Describe {name}."
        details = ""

    lines = [
        f"{indent}/**\n",
        f"{indent} * @brief {brief}\n",
    ]

    for tparam in func.template_params:
        lines.append(f"{indent} * @tparam {tparam} TBD: Describe template parameter.\n")

    for p in func.params:
        lines.append(f"{indent} * @param{p.direction} {p.name} {describe_param(p.direction)}\n")

    if not is_void_return(func.return_type):
        lines.append(f"{indent} * @return {describe_return(func.return_type)}\n")

    if func.has_body and thrown:
        for exc, detail in zip(thrown, describe_throws(thrown)):
            lines.append(f"{indent} * @throws {exc} {detail}\n")

    if func.noexcept:
        lines.append(f"{indent} * @note Exception safety: noexcept.\n")

    if func.has_body:
        details_text = describe_details(called, name)
        if details and details != "Calls: none detected.":
            details_text = f"{details} {details_text}"
        lines.append(f"{indent} * @details {details_text}\n")
    elif details and details != "Calls: none detected.":
        lines.append(f"{indent} * @details {details}\n")

    lines.append(f"{indent} */\n")
    return lines


def merge_existing_doxygen_block(
    block_lines: List[str],
    indent: str,
    func: FunctionMatch,
    called: List[str],
    thrown: List[str],
) -> Tuple[List[str], bool]:
    text = "".join(block_lines)
    existing_params = set(re.findall(r"@param(?:\[[^\]]+\])?\s+([A-Za-z_]\w*)", text))
    existing_tparams = set(re.findall(r"@tparam\s+([A-Za-z_]\w*)", text))
    has_brief = re.search(r"@brief\b", text) is not None
    has_return = re.search(r"@return\b|@retval\b", text) is not None
    has_details = re.search(r"@details\b|@note\b|@remark\b", text) is not None
    has_throws = re.search(r"@throws\b|@exception\b", text) is not None
    has_noexcept_note = re.search(r"Exception safety:\s*noexcept", text, re.IGNORECASE) is not None

    additions: List[str] = []
    short_name = func.name.split("::")[-1]

    if not has_brief:
        additions.append(f"{indent} * @brief TBD: Describe {short_name}.\n")

    for tparam in func.template_params:
        if tparam not in existing_tparams:
            additions.append(f"{indent} * @tparam {tparam} TBD: Describe template parameter.\n")

    for p in func.params:
        if p.name not in existing_params:
            additions.append(f"{indent} * @param{p.direction} {p.name} {describe_param(p.direction)}\n")

    if not is_void_return(func.return_type) and not has_return:
        additions.append(f"{indent} * @return {describe_return(func.return_type)}\n")

    if func.has_body and not has_throws:
        if thrown:
            for exc, detail in zip(thrown, describe_throws(thrown)):
                additions.append(f"{indent} * @throws {exc} {detail}\n")

    if func.noexcept and not has_noexcept_note:
        additions.append(f"{indent} * @note Exception safety: noexcept.\n")

    if func.has_body and not has_details:
        additions.append(f"{indent} * @details {describe_details(called, short_name)}\n")

    if not additions:
        return block_lines, False

    end_idx = len(block_lines) - 1
    while end_idx >= 0 and "*/" not in block_lines[end_idx]:
        end_idx -= 1
    if end_idx < 0:
        return block_lines, False

    merged = block_lines[:end_idx] + additions + block_lines[end_idx:]
    return merged, True


def detect_indent(line: str) -> str:
    m = re.match(r"^(\s*)", line)
    return m.group(1) if m else ""


def apply_to_file(path: Path, apply: bool, merge_existing: bool) -> tuple[int, int, int, int]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines(keepends=True)
    functions = find_functions(lines)

    if not functions:
        return 0, 0, 0, 0

    edits: List[Tuple[int, int, List[str]]] = []
    removed_comment_lines = 0
    merged_blocks = 0

    for func in functions:
        indent = detect_indent(lines[func.start_line])
        body = find_function_body_block(lines, func)
        called = extract_called_functions(body, func.name)
        thrown = extract_thrown_exceptions(body)

        multiline_triple_slash = collect_multiline_triple_slash_comment(lines, func.start_line)
        if multiline_triple_slash:
            block = build_doxygen_block(indent, func, called, thrown, multiline_triple_slash)
            edits.append((multiline_triple_slash.start_line, multiline_triple_slash.end_line, block))
            continue

        if has_immediate_single_line_triple_slash_doxygen(lines, func.start_line):
            continue

        existing_doxy = find_immediate_doxygen_block(lines, func.start_line)
        if existing_doxy:
            if not merge_existing:
                continue
            start, end = existing_doxy
            merged_block, changed = merge_existing_doxygen_block(
                lines[start : end + 1],
                indent,
                func,
                called,
                thrown,
            )
            if changed:
                edits.append((start, end, merged_block))
                merged_blocks += 1
            continue

        existing_comment = find_preceding_normal_comment(lines, func.start_line)
        block = build_doxygen_block(indent, func, called, thrown, existing_comment)

        if existing_comment:
            removed_comment_lines += existing_comment.end_line - existing_comment.start_line + 1
            edits.append((existing_comment.start_line, existing_comment.end_line, block))
        else:
            edits.append((func.start_line, func.start_line - 1, block))

    if not edits:
        return len(functions), 0, 0, 0

    for start, end, replacement in sorted(edits, key=lambda x: x[0], reverse=True):
        if end >= start:
            lines[start : end + 1] = replacement
        else:
            lines[start:start] = replacement

    if apply:
        path.write_text("".join(lines), encoding="utf-8")

    inserted_blocks = len(edits) - merged_blocks
    return len(functions), inserted_blocks, removed_comment_lines, merged_blocks


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Add or merge Doxygen function headers (@brief/@param/@return/@details) in C/C++ files."
    )
    parser.add_argument("--root", default=".", help="Repository root (default: .)")
    parser.add_argument(
        "--paths",
        nargs="+",
        default=["src", "include", "plugins"],
        help="Paths relative to --root to scan (default: src include plugins)",
    )
    parser.add_argument(
        "--public-only",
        action="store_true",
        help="Only process public API headers under include/**",
    )
    parser.add_argument("--apply", action="store_true", help="Write changes to files")
    parser.add_argument(
        "--no-merge-existing",
        action="store_true",
        help="Do not enrich existing Doxygen blocks; only create missing ones",
    )
    parser.add_argument("--limit", type=int, default=0, help="Limit number of files (debug)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(args.root).resolve()

    files = list(iter_cpp_files(root, args.paths, public_only=args.public_only))
    files.sort()
    if args.limit and args.limit > 0:
        files = files[: args.limit]

    merge_existing = not args.no_merge_existing
    total_files = 0
    touched_files = 0
    total_functions = 0
    inserted_blocks = 0
    removed_comment_lines = 0
    merged_blocks = 0

    for f in files:
        total_files += 1
        fn_count, add_count, removed_count, merged_count = apply_to_file(
            f,
            apply=args.apply,
            merge_existing=merge_existing,
        )
        total_functions += fn_count
        removed_comment_lines += removed_count
        merged_blocks += merged_count

        if add_count > 0 or merged_count > 0:
            touched_files += 1
            inserted_blocks += add_count
            rel = f.relative_to(root).as_posix()
            mode = "WRITE" if args.apply else "DRY-RUN"
            print(
                f"[{mode}] {rel}: add {add_count} block(s), merged {merged_count} block(s), replaced_comment_lines={removed_count}"
            )

    mode = "WRITE" if args.apply else "DRY-RUN"
    print(
        f"[{mode}] files={total_files} touched={touched_files} functions={total_functions} "
        f"inserted={inserted_blocks} merged={merged_blocks} replaced_comment_lines={removed_comment_lines} "
        f"public_only={args.public_only} merge_existing={merge_existing}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
