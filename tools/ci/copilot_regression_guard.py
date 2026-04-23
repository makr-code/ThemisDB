#!/usr/bin/env python3
"""
Copilot Regression Guard for Build/Link Stability.

Checks:
1) Inventory of test executables where add_executable() lists only test sources.
2) Heuristic mapping test symbols/files -> likely missing src/**/*.cpp sources.
3) Validation of export/import macro wiring (THEMIS_BASE_EXPORTS, THEMIS_TEST_BUILD, THEMIS_BASE_API).
4) Parsing of MSVC linker errors (LNK2001/LNK2019/LNK1120) with source suggestions.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

EXEMPT_TARGET_KEYWORDS = {
    "interface",
    "interfaces",
    "plugin_api",
    "api",
    "contract",
    "registry",
}

TEST_SOURCE_PATTERN = re.compile(r"(^|/)(test_|.*test.*)", re.IGNORECASE)
LNK_PATTERN = re.compile(r"LNK(2001|2019|1120)", re.IGNORECASE)


def _camel_to_snake(name: str) -> str:
    step1 = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", name)
    step2 = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", step1)
    return step2.lower()


@dataclass
class TargetInventory:
    target: str
    line: int
    sources: list[str]
    has_themis_linkage: bool
    probable_sources: list[str]
    exempt: bool


def _extract_cmake_call_block(lines: list[str], start_idx: int) -> tuple[str, int]:
    block_lines = [lines[start_idx]]
    depth = lines[start_idx].count("(") - lines[start_idx].count(")")
    idx = start_idx + 1
    while idx < len(lines) and depth > 0:
        block_lines.append(lines[idx])
        depth += lines[idx].count("(") - lines[idx].count(")")
        idx += 1
    return "\n".join(block_lines), idx


def _parse_add_executable_blocks(cmake_text: str) -> list[tuple[str, int, list[str]]]:
    lines = cmake_text.splitlines()
    idx = 0
    blocks: list[tuple[str, int, list[str]]] = []
    while idx < len(lines):
        if "add_executable(" not in lines[idx]:
            idx += 1
            continue
        start_line = idx + 1
        block, idx = _extract_cmake_call_block(lines, idx)
        target_match = re.search(r"add_executable\(([^\s\)]+)", block)
        if not target_match:
            continue
        target = target_match.group(1)
        sources = re.findall(r"([\w./${}\\-]+\.(?:cpp|cc|cxx|c))", block)
        blocks.append((target, start_line, sources))
    return blocks


def _target_has_themis_linkage(cmake_text: str, target: str) -> bool:
    pattern = re.compile(
        rf"target_link_libraries\(\s*{re.escape(target)}\b([\s\S]*?)\)",
        re.MULTILINE,
    )
    for match in pattern.finditer(cmake_text):
        body = match.group(1)
        if re.search(r"\bthemis_[a-z0-9_]+\b|\$\{THEMIS_[A-Z0-9_]+\}", body):
            return True
    return False


def _target_has_direct_production_sources(cmake_text: str, target: str) -> bool:
    patterns = [
        re.compile(
            rf"add_executable\(\s*{re.escape(target)}\b([\s\S]*?)\)",
            re.MULTILINE,
        ),
        re.compile(
            rf"target_sources\(\s*{re.escape(target)}\b([\s\S]*?)\)",
            re.MULTILINE,
        ),
    ]
    for pattern in patterns:
        for match in pattern.finditer(cmake_text):
            body = match.group(1)
            if re.search(r"src[\\/].*\.cpp", body) or "${THEMIS_ROOT_DIR}/src/" in body:
                return True
    return False


def _is_test_source(path: str) -> bool:
    norm = path.replace("\\", "/")
    stem = Path(norm).stem.lower()
    return bool(TEST_SOURCE_PATTERN.search(norm.lower())) or stem.startswith("test_")


def _canonical_test_stem(source: str) -> set[str]:
    stem = Path(source).stem
    variants = {stem}
    if stem.startswith("test_"):
        variants.add(stem[5:])
    for suffix in (
        "_focused",
        "_comprehensive",
        "_integration",
        "_production",
        "_api",
        "_tests",
        "_test",
    ):
        new_variants = set()
        for item in variants:
            if item.endswith(suffix):
                new_variants.add(item[: -len(suffix)])
        variants.update(new_variants)
    # Heuristic: test_replication_crdt_types -> crdt_types
    for item in list(variants):
        parts = item.split("_")
        if len(parts) >= 3:
            variants.add("_".join(parts[-2:]))
        if len(parts) >= 2:
            variants.add(parts[-1])
    return {v for v in variants if v}


def _index_source_files(repo_root: Path) -> dict[str, list[str]]:
    index: dict[str, list[str]] = {}
    for cpp in sorted((repo_root / "src").rglob("*.cpp")):
        index.setdefault(cpp.stem.lower(), []).append(str(cpp.relative_to(repo_root)).replace("\\", "/"))
    return index


def _probable_sources_for_test_sources(test_sources: Iterable[str], source_index: dict[str, list[str]]) -> list[str]:
    out: list[str] = []
    seen: set[str] = set()
    for src in test_sources:
        for variant in _canonical_test_stem(src):
            keys = {variant.lower(), _camel_to_snake(variant)}
            for key in keys:
                for candidate in source_index.get(key, []):
                    if candidate not in seen:
                        seen.add(candidate)
                        out.append(candidate)
    return out


def inventory_test_targets(repo_root: Path, cmake_file: Path) -> tuple[list[TargetInventory], list[TargetInventory]]:
    cmake_text = cmake_file.read_text(encoding="utf-8", errors="ignore")
    source_index = _index_source_files(repo_root)

    inventory: list[TargetInventory] = []
    affected: list[TargetInventory] = []

    for target, line, sources in _parse_add_executable_blocks(cmake_text):
        if not sources:
            continue
        if not all(_is_test_source(s) for s in sources):
            continue

        probable = _probable_sources_for_test_sources(sources, source_index)
        has_link = _target_has_themis_linkage(cmake_text, target)
        has_direct_prod_sources = _target_has_direct_production_sources(cmake_text, target)
        exempt = any(token in target.lower() for token in EXEMPT_TARGET_KEYWORDS)

        item = TargetInventory(
            target=target,
            line=line,
            sources=sources,
            has_themis_linkage=has_link,
            probable_sources=probable,
            exempt=exempt,
        )
        inventory.append(item)

        if probable and not has_link and not has_direct_prod_sources and not exempt:
            affected.append(item)

    return inventory, affected


def validate_export_macros(repo_root: Path) -> list[str]:
    failures: list[str] = []

    header = (repo_root / "include/themis_export.h")
    cmake_tests = (repo_root / "tests/CMakeLists.txt")
    cmake_core = (repo_root / "cmake/CMakeLists.txt")

    if not header.exists():
        failures.append("include/themis_export.h not found")
        return failures

    header_text = header.read_text(encoding="utf-8", errors="ignore")
    if "THEMIS_TEST_BUILD" not in header_text:
        failures.append("THEMIS_TEST_BUILD is missing in include/themis_export.h")
    if "THEMIS_BASE_EXPORTS" not in header_text:
        failures.append("THEMIS_BASE_EXPORTS is missing in include/themis_export.h")
    if "THEMIS_BASE_API" not in header_text:
        failures.append("THEMIS_BASE_API is missing in include/themis_export.h")

    tests_text = cmake_tests.read_text(encoding="utf-8", errors="ignore") if cmake_tests.exists() else ""
    if "THEMIS_TEST_BUILD=1" not in tests_text:
        failures.append("THEMIS_TEST_BUILD=1 is missing in tests/CMakeLists.txt")

    core_text = cmake_core.read_text(encoding="utf-8", errors="ignore") if cmake_core.exists() else ""
    if "target_compile_definitions(themis_core PRIVATE THEMIS_BASE_EXPORTS)" not in core_text:
        failures.append("themis_core does not define THEMIS_BASE_EXPORTS in cmake/CMakeLists.txt")

    return failures


def _symbol_tokens(symbol: str) -> set[str]:
    clean = re.sub(r"[`'\"\?@!$%^&*()=+\[\]{}<>|~]", " ", symbol)
    clean = clean.replace("::", " ")
    raw_tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]{2,}", clean)
    skip = {
        "class", "struct", "public", "private", "protected", "void", "const",
        "unsigned", "signed", "short", "long", "int", "char", "float", "double",
        "std", "themis", "themisdb",
    }
    out = {t for t in raw_tokens if t.lower() not in skip}
    snake = {_camel_to_snake(t) for t in out}
    return {t.lower() for t in out}.union(snake)


def parse_lnk_errors(build_log: Path, source_index: dict[str, list[str]]) -> list[dict[str, object]]:
    if not build_log.exists():
        return []

    lines = build_log.read_text(encoding="utf-8", errors="ignore").splitlines()
    findings: list[dict[str, object]] = []

    for line_no, line in enumerate(lines, start=1):
        if not LNK_PATTERN.search(line):
            continue
        code_match = re.search(r"LNK(2001|2019|1120)", line, re.IGNORECASE)
        code = f"LNK{code_match.group(1)}" if code_match else "LNK"
        quoted = re.findall(r'"([^"]+)"', line)
        symbol = quoted[0] if quoted else ""

        suggestions: list[str] = []
        seen: set[str] = set()
        for token in _symbol_tokens(symbol):
            for path in source_index.get(token, []):
                if path not in seen:
                    seen.add(path)
                    suggestions.append(path)
                if len(suggestions) >= 5:
                    break
            if len(suggestions) >= 5:
                break

        findings.append(
            {
                "line": line_no,
                "code": code,
                "message": line.strip(),
                "symbol": symbol,
                "suggested_sources": suggestions,
            }
        )
    return findings


def _print_report(
    inventory: list[TargetInventory],
    affected: list[TargetInventory],
    macro_failures: list[str],
    lnk_findings: list[dict[str, object]],
) -> None:
    print("Copilot Regression Guard")
    print("=" * 80)
    print(f"Inventory (test-only add_executable targets): {len(inventory)}")
    print(f"Potential missing-source targets: {len(affected)}")
    print(f"Export macro validation failures: {len(macro_failures)}")
    print(f"LNK findings: {len(lnk_findings)}")

    if affected:
        print("\nPotentially missing sources:")
        for item in affected[:50]:
            print(
                f"  - {item.target} (line {item.line}) -> probable: "
                f"{', '.join(item.probable_sources[:3])}"
            )

    if macro_failures:
        print("\nMacro validation issues:")
        for failure in macro_failures:
            print(f"  - {failure}")

    if lnk_findings:
        print("\nLinker findings:")
        for finding in lnk_findings[:50]:
            suggested = ", ".join(finding["suggested_sources"][:3]) if finding["suggested_sources"] else "(no suggestion)"
            print(f"  - line {finding['line']} {finding['code']}: {suggested}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run ThemisDB Copilot regression guard checks")
    parser.add_argument("--repo-root", type=Path, default=Path("."), help="Repository root")
    parser.add_argument(
        "--cmake-file",
        type=Path,
        default=Path("tests/CMakeLists.txt"),
        help="CMake file to inventory test targets",
    )
    parser.add_argument("--build-log", type=Path, help="Optional build log for LNK parsing")
    parser.add_argument("--strict-missing-sources", action="store_true", help="Fail on potential missing-source test targets")
    parser.add_argument("--output-json", type=Path, help="Optional JSON report output path")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_arg_parser().parse_args(argv)
    repo_root = args.repo_root.resolve()
    cmake_file = args.cmake_file if args.cmake_file.is_absolute() else repo_root / args.cmake_file

    if not cmake_file.exists():
        print(f"ERROR: CMake file not found: {cmake_file}", file=sys.stderr)
        return 2

    inventory, affected = inventory_test_targets(repo_root, cmake_file)
    macro_failures = validate_export_macros(repo_root)

    source_index = _index_source_files(repo_root)
    lnk_findings = parse_lnk_errors(args.build_log, source_index) if args.build_log else []

    _print_report(inventory, affected, macro_failures, lnk_findings)

    report = {
        "inventory_count": len(inventory),
        "potential_missing_sources_count": len(affected),
        "inventory": [asdict(item) for item in inventory],
        "potential_missing_sources": [asdict(item) for item in affected],
        "macro_validation_failures": macro_failures,
        "lnk_findings": lnk_findings,
    }

    if args.output_json:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        args.output_json.write_text(json.dumps(report, indent=2), encoding="utf-8")

    fail = False
    if macro_failures:
        fail = True
    if lnk_findings:
        fail = True
    if args.strict_missing_sources and affected:
        fail = True

    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
