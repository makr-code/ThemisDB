#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Doxygen AutoFix (C/C++) - Enhanced Version
- Scans src/ and include/
- Detects function signatures heuristically
- Adds/updates Doxygen blocks (@brief, @param, @return)
- Converts existing regular comments to Doxygen format
- Uses optional Ollama for better @brief text
- Supports --check-only, --apply, and --convert-existing

WICHTIG:
- This enhanced version includes a write component to convert existing comments to Doxygen
- Conservative approach: skips uncertain cases rather than breaking code
- New feature: Can convert existing // and /* */ comments to Doxygen format
"""

from __future__ import annotations
import argparse
import json
import os
import re
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple, Dict, Any

# Optional: requests für Ollama HTTP API
try:
    import requests
except Exception:
    requests = None


# -----------------------------
# Models
# -----------------------------

@dataclass
class FunctionSig:
    start_line: int
    end_line: int
    indent: str
    raw_signature: str
    return_type: str
    name: str
    params_raw: str
    params: List[Tuple[str, str]]  # [(type, name)]
    is_void: bool


@dataclass
class Change:
    file: str
    line: int
    action: str  # "insert" | "update" | "skip"
    reason: str
    function: str


@dataclass
class Config:
    root: Path
    model: str
    ollama_url: str
    use_ollama: bool
    check_only: bool
    apply: bool
    convert_existing: bool = False  # New: Convert existing comments to Doxygen
    limit_files: Optional[int] = None  # Limit number of files to process (for testing)
    include_paths: List[str] = field(default_factory=lambda: ["src", "include"])
    exts: List[str] = field(default_factory=lambda: [".h", ".hpp", ".hh", ".hxx", ".c", ".cc", ".cpp", ".cxx"])
    timeout_sec: int = 45
    max_retries: int = 2


# -----------------------------
# Utility / Filters
# -----------------------------

EXCLUDE_PARTS = {
    "third_party", "vcpkg", "vcpkg_installed", "vcpkg_installed_linux",
    "build", "build-msvc-windows-release", ".git", "releases"
}

FUNC_SIG_RE = re.compile(
    r"""^
    (?P<indent>\s*)
    (?P<ret>[~\w:\<\>\,\s\*&]+?)          # return type (heuristic)
    \s+
    (?P<name>[A-Za-z_]\w*(?:::\w+)*)      # func name
    \s*
    \(
      (?P<params>[^;{}()]*(?:\([^)]*\)[^;{}()]*)?)   # param list heuristic
    \)
    \s*
    (?P<suffix>const\s*)?
    (?P<trailer>\{|\;\s*$)
    """,
    re.VERBOSE,
)

DOXY_START_RE = re.compile(r'^\s*/\*\*')
DOXY_END_RE = re.compile(r'\*/\s*$')
TAG_BRIEF_RE = re.compile(r'@brief\b')
TAG_PARAM_RE = re.compile(r'@param\b')
TAG_RETURN_RE = re.compile(r'@return\b')

# Patterns for existing comments that can be converted to Doxygen
SINGLE_LINE_COMMENT_RE = re.compile(r'^\s*//')
MULTI_LINE_COMMENT_START_RE = re.compile(r'^\s*/\*')
MULTI_LINE_COMMENT_END_RE = re.compile(r'\*/\s*$')


def is_in_scope(p: Path, cfg: Config) -> bool:
    # Convert to absolute paths for comparison
    try:
        abs_path = p.resolve()
        abs_root = cfg.root.resolve()
        
        # Check if the path is under excluded directories
        path_str = str(abs_path).lower().replace("\\", "/")
        root_str = str(abs_root).lower().replace("\\", "/")
        
        # Check for excluded directory patterns
        exclude_patterns = [
            "/vcpkg/", "/third_party/", "/external/", "/build/", 
            "/.git/", "/releases/", "/proto/", "/packages/",
            "vcpkg_installed", "llama.cpp", "whisper.cpp"
        ]
        
        if any(pattern in path_str for pattern in exclude_patterns):
            return False
        
        # Check if the path is under our source directories
        for prefix in cfg.include_paths:
            prefix_path = (abs_root / prefix).resolve()
            prefix_str = str(prefix_path).lower().replace("\\", "/")
            
            if path_str.startswith(prefix_str + "/") or path_str == prefix_str:
                return True
        
        # Also check if it's directly under src or include
        if "/src/" in path_str or "/include/" in path_str or path_str.endswith("/src") or path_str.endswith("/include"):
            return True
            
    except Exception:
        return False
    
    return False


def find_existing_comments(lines: List[str], func_start_line: int) -> Optional[Tuple[int, int, str]]:
    """
    Find existing regular comments (// or /* */) that could be converted to Doxygen.
    Returns (start_line, end_line, comment_text) or None if no convertible comment found.
    """
    # Look for comments in the 10 lines before the function
    start_search = max(0, func_start_line - 10)
    for i in range(start_search, func_start_line):
        line = lines[i].strip()
        
        # Check for single-line comments
        if SINGLE_LINE_COMMENT_RE.match(line):
            comment_text = line[2:].strip()  # Remove //
            return (i, i, comment_text)
        
        # Check for multi-line comments
        if MULTI_LINE_COMMENT_START_RE.match(line):
            comment_lines = [line[2:].strip()]  # Remove /*
            j = i + 1
            while j < len(lines) and not MULTI_LINE_COMMENT_END_RE.search(lines[j]):
                comment_lines.append(lines[j].strip())
                j += 1
            if j < len(lines):
                last_line = lines[j].strip()
                if MULTI_LINE_COMMENT_END_RE.search(last_line):
                    # Remove */ from the end
                    last_line = MULTI_LINE_COMMENT_END_RE.sub('', last_line)
                    comment_lines.append(last_line)
                comment_text = ' '.join(comment_lines)
                return (i, j, comment_text)
    
    return None


def iter_source_files(cfg: Config):
    # Use more targeted search instead of rglob("*") for performance
    for include_path in cfg.include_paths:
        base_path = cfg.root / include_path
        if base_path.exists() and base_path.is_dir():
            for ext in cfg.exts:
                try:
                    for path in base_path.rglob(f"*{ext}"):
                        if path.is_file():
                            yield path
                except Exception:
                    # Skip directories that cause errors (permission issues, etc.)
                    continue
    
    # Also check root directory for files that might be in src/include but not in subdirectories
    for ext in cfg.exts:
        try:
            for path in cfg.root.glob(f"*{ext}"):
                if path.is_file() and is_in_scope(path, cfg):
                    yield path
        except Exception:
            continue


# -----------------------------
# Parsing (heuristic)
# -----------------------------

def split_params(params_raw: str) -> List[str]:
    params_raw = params_raw.strip()
    if not params_raw or params_raw == "void":
        return []
    out = []
    buf = []
    depth = 0
    for ch in params_raw:
        if ch == ',' and depth == 0:
            out.append("".join(buf).strip())
            buf = []
            continue
        if ch in "([<":
            depth += 1
        elif ch in ")]>":
            depth = max(0, depth - 1)
        buf.append(ch)
    if buf:
        out.append("".join(buf).strip())
    return out


def parse_param_decl(decl: str) -> Tuple[str, str]:
    decl = decl.strip()
    decl = re.sub(r'\s*=\s*.*$', '', decl)  # default values entfernen
    m = re.match(r'^(.*?)([A-Za-z_]\w*)$', decl)
    if not m:
        return decl, "param"
    ptype = m.group(1).strip()
    pname = m.group(2).strip()
    return ptype, pname


def find_function_sigs(lines: List[str]) -> List[FunctionSig]:
    sigs = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        # Skip preprocessor lines quickly
        if line.lstrip().startswith("#"):
            i += 1
            continue

        candidate = line.rstrip("\n")
        start = i
        # Multi-line signatures sammeln bis "{" oder ";" auftaucht
        if "(" in candidate and not candidate.strip().startswith("//"):
            j = i
            acc = [candidate.strip()]
            while j + 1 < n and ("{" not in acc[-1] and ";" not in acc[-1]):
                j += 1
                nxt = lines[j].strip()
                if nxt.startswith("//"):
                    break
                acc.append(nxt)
                if "{" in nxt or ";" in nxt:
                    break

            raw = " ".join(x for x in acc if x)
            raw = re.sub(r'\s+', ' ', raw).strip()

            m = FUNC_SIG_RE.match(raw)
            if m:
                ret = m.group("ret").strip()
                name = m.group("name").strip()
                params_raw = m.group("params").strip()
                plist = [parse_param_decl(p) for p in split_params(params_raw)]
                is_void = ret == "void"
                indent = m.group("indent") or re.match(r'^(\s*)', line).group(1)
                sigs.append(FunctionSig(
                    start_line=start,
                    end_line=j,
                    indent=indent,
                    raw_signature=raw,
                    return_type=ret,
                    name=name,
                    params_raw=params_raw,
                    params=plist,
                    is_void=is_void
                ))
                i = j + 1
                continue
        i += 1
    return sigs


def find_existing_doxygen_block(lines: List[str], func_start_line: int) -> Optional[Tuple[int, int]]:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return None
    if not DOXY_END_RE.search(lines[i]):
        return None

    end = i
    while i >= 0:
        if DOXY_START_RE.search(lines[i]):
            return i, end
        i -= 1
    return None


def convert_to_doxygen_format(comment_text: str, func: FunctionSig, indent: str) -> List[str]:
    """
    Convert existing comment text to Doxygen format with proper tags.
    
    Args:
        comment_text: The existing comment text
        func: The function signature information
        indent: The indentation to use
        
    Returns:
        List of formatted Doxygen comment lines
    """
    lines = []
    
    # Start Doxygen block
    lines.append(f"{indent}/**")
    
    # Process the comment text - try to extract meaningful information
    if comment_text:
        # Clean up the text
        cleaned_text = re.sub(r'\s+', ' ', comment_text).strip()
        
        # If it looks like a description, use it as @brief
        if cleaned_text and not any(tag in cleaned_text.lower() for tag in ['@param', '@return', '@brief']):
            lines.append(f"{indent} * @brief {cleaned_text}")
        else:
            # If it already has some structure, keep it but ensure it's in Doxygen format
            lines.append(f"{indent} * {cleaned_text}")
    else:
        lines.append(f"{indent} * @brief TODO: Describe {func.name.split('::')[-1]}.")
    
    # Add parameter documentation if parameters exist
    for _ptype, pname in func.params:
        lines.append(f"{indent} * @param {pname} TODO: describe parameter.")
    
    # Add return documentation if not void
    if not func.is_void:
        lines.append(f"{indent} * @return TODO: describe return value.")
    
    # End Doxygen block
    lines.append(f"{indent} */")
    
    return lines


# -----------------------------
# Doxygen generation / update
# -----------------------------

def default_brief(func: FunctionSig) -> str:
    base = func.name.split("::")[-1]
    return f"TODO: Describe {base}."


def build_doxy_block(func: FunctionSig, brief: str) -> List[str]:
    indent = re.match(r'^(\s*)', func.raw_signature).group(1) if func.raw_signature else func.indent
    lines = [f"{func.indent}/**\n", f"{func.indent} * @brief {brief}\n"]
    for _ptype, pname in func.params:
        lines.append(f"{func.indent} * @param {pname} TODO: describe parameter.\n")
    if not func.is_void:
        lines.append(f"{func.indent} * @return TODO: describe return value.\n")
    lines.append(f"{func.indent} */\n")
    return lines


def ensure_tags_in_existing(block_lines: List[str], func: FunctionSig, brief: str) -> List[str]:
    text = "".join(block_lines)
    has_brief = bool(TAG_BRIEF_RE.search(text))
    has_return = bool(TAG_RETURN_RE.search(text))
    existing_params = set(re.findall(r'@param\s+([A-Za-z_]\w*)', text))

    out = list(block_lines)
    insert_idx = len(out) - 1 if out and "*/" in out[-1] else len(out)

    additions = []
    if not has_brief:
        additions.append(f"{func.indent} * @brief {brief}\n")

    for _ptype, pname in func.params:
        if pname not in existing_params:
            additions.append(f"{func.indent} * @param {pname} TODO: describe parameter.\n")

    if not func.is_void and not has_return:
        additions.append(f"{func.indent} * @return TODO: describe return value.\n")

    if additions:
        out[insert_idx:insert_idx] = additions
    return out


def ollama_brief(cfg: Config, func: FunctionSig, context: str) -> str:
    if not cfg.use_ollama:
        return default_brief(func)
    if requests is None:
        return default_brief(func)

    prompt = (
        "Write exactly ONE concise Doxygen @brief sentence in English.\n"
        "No markdown, no tag, no quotes.\n"
        f"Function signature: {func.raw_signature}\n"
        f"Nearby context:\n{context[:1200]}\n"
    )

    payload = {
        "model": cfg.model,
        "prompt": prompt,
        "stream": False
    }

    for _ in range(cfg.max_retries + 1):
        try:
            r = requests.post(f"{cfg.ollama_url.rstrip('/')}/api/generate", json=payload, timeout=cfg.timeout_sec)
            r.raise_for_status()
            data = r.json()
            text = (data.get("response") or "").strip().splitlines()[0].strip()
            if text:
                text = re.sub(r'^\s*[@#*\-]+\s*', '', text)
                text = text.replace("/**", "").replace("*/", "").strip()
                return text[:180]
        except Exception:
            time.sleep(0.6)
    return default_brief(func)


# -----------------------------
# Main processing
# -----------------------------

def process_file(path: Path, cfg: Config) -> Tuple[List[str], List[Change]]:
    original = path.read_text(encoding="utf-8", errors="ignore").splitlines(keepends=True)
    lines = list(original)
    changes: List[Change] = []

    sigs = find_function_sigs(lines)
    if not sigs:
        return lines, changes

    # rückwärts bearbeiten (Line-Offsets stabil halten)
    for func in reversed(sigs):
        block = find_existing_doxygen_block(lines, func.start_line)
        context_start = max(0, func.start_line - 8)
        context_end = min(len(lines), func.end_line + 8)
        context = "".join(lines[context_start:context_end])

        brief = ollama_brief(cfg, func, context)

        if block is None:
            # Check if there are existing comments that can be converted
            if cfg.convert_existing:
                existing_comment = find_existing_comments(lines, func.start_line)
                if existing_comment:
                    start_line, end_line, comment_text = existing_comment
                    new_block = convert_to_doxygen_format(comment_text, func, func.indent)
                    # Remove the existing comment lines
                    lines[start_line:end_line + 1] = []
                    # Insert the new Doxygen block before the function
                    lines[func.start_line:func.start_line] = new_block
                    changes.append(Change(
                        file=str(path), line=func.start_line + 1, action="convert",
                        reason="converted existing comment to doxygen", function=func.name
                    ))
                    continue  # Skip the normal insertion below
            
            # No existing Doxygen block and no convertible comments - add new block
            new_block = build_doxy_block(func, brief)
            lines[func.start_line:func.start_line] = new_block
            changes.append(Change(
                file=str(path), line=func.start_line + 1, action="insert",
                reason="missing doxygen block", function=func.name
            ))
        else:
            b0, b1 = block
            old_block = lines[b0:b1 + 1]
            new_block = ensure_tags_in_existing(old_block, func, brief)
            if "".join(old_block) != "".join(new_block):
                lines[b0:b1 + 1] = new_block
                changes.append(Change(
                    file=str(path), line=b0 + 1, action="update",
                    reason="missing doxygen tags", function=func.name
                ))

    return lines, changes


def main():
    ap = argparse.ArgumentParser(description="Auto-fix Doxygen comments with optional Ollama.")
    ap.add_argument("--root", default=".", help="Repository root")
    ap.add_argument("--model", default="qwen2.5-coder:7b", help="Ollama model name")
    ap.add_argument("--ollama-url", default="http://127.0.0.1:11434", help="Ollama base URL")
    ap.add_argument("--no-ollama", action="store_true", help="Disable Ollama, use template brief only")
    ap.add_argument("--check-only", action="store_true", help="Do not write files; just report")
    ap.add_argument("--apply", action="store_true", help="Write changes to files")
    ap.add_argument("--convert-existing", action="store_true", help="Convert existing // and /* */ comments to Doxygen format")
    ap.add_argument("--limit", type=int, default=None, help="Limit number of files to process (for testing)")
    ap.add_argument("--report", default="ai_working/doxygen_autofix_report.json", help="Report path")
    args = ap.parse_args()

    if not args.check_only and not args.apply:
        print("Choose one mode: --check-only or --apply", file=sys.stderr)
        sys.exit(2)

    cfg = Config(
        root=Path(args.root).resolve(),
        model=args.model,
        ollama_url=args.ollama_url,
        use_ollama=not args.no_ollama,
        check_only=args.check_only,
        apply=args.apply,
        convert_existing=args.convert_existing,
        limit_files=args.limit,
    )

    all_changes: List[Change] = []
    files_changed = 0
    files_scanned = 0

    for fp in iter_source_files(cfg):
        if cfg.limit_files and files_scanned >= cfg.limit_files:
            print(f"Reached limit of {cfg.limit_files} files")
            break
            
        files_scanned += 1
        if files_scanned % 10 == 0:  # Progress indicator every 10 files
            print(f"Scanned {files_scanned} files...")
        
        try:
            new_lines, changes = process_file(fp, cfg)
            if changes:
                all_changes.extend(changes)
                files_changed += 1
                if cfg.apply:
                    fp.write_text("".join(new_lines), encoding="utf-8")
        except Exception as e:
            print(f"Error processing {fp}: {e}")
            continue

    report = {
        "root": str(cfg.root),
        "files_scanned": files_scanned,
        "files_changed": files_changed,
        "changes": [c.__dict__ for c in all_changes],
    }

    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"Scanned: {files_scanned}")
    print(f"Changed: {files_changed}")
    print(f"Total edits: {len(all_changes)}")
    print(f"Report: {report_path}")

    # Exit code für CI: im check-only Mode 1 wenn Befunde existieren
    if cfg.check_only and all_changes:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
