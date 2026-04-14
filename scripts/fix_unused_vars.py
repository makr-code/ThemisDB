"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fix_unused_vars.py                                 ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-14 06:59:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     373                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Fix (void)varname; suppressions by replacing with [[maybe_unused]] annotations.
"""

import re
import os
import sys
from pathlib import Path

# Global counters
stats = {'void_removed': 0, 'maybe_unused_added': 0, 'files': set()}


def get_void_vars(line):
    """Extract varnames from (void)varname; on a given line. Returns list of varnames."""
    # Skip (void)0 patterns and macro defines
    stripped = line.strip()
    if stripped.startswith('#'):
        return []
    results = []
    for m in re.finditer(r'\(\s*void\s*\)\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*;', line):
        varname = m.group(1)
        # Skip void)0 (shouldn't match but just in case)
        if varname == '0':
            continue
        # Check it's not in a comment
        pos = m.start()
        pre = line[:pos]
        if '//' in pre:
            continue
        results.append(varname)
    return results


def is_used_elsewhere(lines, start, end, varname, void_line_idx):
    """Check if varname appears in lines[start:end] outside of (void)varname; casts.
    
    'start' is the opening-brace line of the function; we skip it since it contains
    the function signature where the parameter name appears but is NOT a use in the body.
    """
    word_re = re.compile(r'\b' + re.escape(varname) + r'\b')
    void_re = re.compile(r'\(\s*void\s*\)\s*' + re.escape(varname) + r'\s*;')
    # Start from start+1 to skip the opening-brace / signature line
    search_start = start + 1
    for i in range(search_start, min(end, len(lines))):
        if i == void_line_idx:
            continue
        line = lines[i]
        # Remove void casts from counting
        cleaned = void_re.sub('', line)
        # Remove comments
        ci = cleaned.find('//')
        if ci >= 0:
            cleaned = cleaned[:ci]
        if word_re.search(cleaned):
            return True
    return False


def find_enclosing_function_start(lines, idx):
    """Find the line index where the enclosing function body starts (the '{' line)."""
    depth = 0
    for i in range(idx, -1, -1):
        for ch in reversed(lines[i]):
            if ch == '}':
                depth += 1
            elif ch == '{':
                if depth == 0:
                    return i
                depth -= 1
    return 0


def find_param_line(lines, func_start, varname):
    """
    Search lines from func_start (inclusive) backward to find a function parameter
    declaration containing varname. Returns line index or None.
    """
    # Function signatures are usually within 30 lines before/at the opening brace
    param_re = re.compile(r'\b' + re.escape(varname) + r'\b')
    # Lines that are clearly part of a function signature (type + name, followed by , or ))
    sig_re = re.compile(
        r'(?:const\s+|volatile\s+|unsigned\s+|signed\s+)?'
        r'(?:[a-zA-Z_]\w*(?:::\w+)*'
        r'(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?'  # template args
        r')\s*[*&\s]*\b'
        + re.escape(varname) + r'\b\s*[,)\n{]'
    )
    
    # Also check func_start itself (case: single-line signature with '{' on same line)
    for i in range(func_start, max(func_start - 30, -1), -1):
        line = lines[i]
        if param_re.search(line) and sig_re.search(line):
            # Make sure it's not a function call or other use
            # A parameter line won't have = followed by (
            if not re.search(r'\b' + re.escape(varname) + r'\s*=\s*', line):
                return i
    return None


def find_local_decl_line(lines, from_idx, varname):
    """
    Search backward from from_idx for a local variable declaration of varname.
    Returns line index or None.
    """
    word_re = re.compile(r'\b' + re.escape(varname) + r'\b')
    # Common declaration patterns
    decl_patterns = [
        re.compile(r'\bauto\s*(?:[*&]{0,2}\s*)' + re.escape(varname) + r'\b'),
        re.compile(r'\b(?:const\s+)?(?:[a-zA-Z_]\w*(?:::\w+)*(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?)\s*[*&\s]*' + re.escape(varname) + r'\s*[=;{(]'),
    ]
    for i in range(from_idx - 1, max(from_idx - 100, -1), -1):
        line = lines[i]
        if not word_re.search(line):
            continue
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('*'):
            continue
        for dp in decl_patterns:
            if dp.search(line):
                return i
    return None


def add_maybe_unused_to_param(line, varname):
    """Add [[maybe_unused]] before the type of a parameter named varname."""
    if '[[maybe_unused]]' in line:
        return line

    stripped = line.lstrip()
    indent = line[:len(line) - len(stripped)]

    # The parameter may be the only content on this line (multi-line signature)
    # or it may follow a comma/open-paren on the same line.
    # Strategy: find the type token sequence preceding varname and insert before it.

    vname_re = re.compile(r'\b' + re.escape(varname) + r'\b')
    m = vname_re.search(line)
    if not m:
        return line

    # Walk backward from the match start to find the start of the type expression.
    # The type starts after: whitespace, '(', or ','
    pre = line[:m.start()]
    # Find the last delimiter (comma, open-paren) or beginning of meaningful content
    delim_re = re.compile(r'[,(]')
    last_delim = -1
    for dm in delim_re.finditer(pre):
        last_delim = dm.start()

    if last_delim >= 0:
        # Type starts after the delimiter
        type_start = last_delim + 1
        # Skip any whitespace
        while type_start < len(pre) and pre[type_start] in ' \t':
            type_start += 1
        prefix = line[:type_start]
        suffix = line[type_start:]
        return prefix + '[[maybe_unused]] ' + suffix
    else:
        # No delimiter found — parameter is at the start of line (after indent)
        return indent + '[[maybe_unused]] ' + stripped


def add_maybe_unused_to_local(line, varname):
    """Add [[maybe_unused]] before the type of a local variable declaration."""
    if '[[maybe_unused]]' in line:
        return line
    
    stripped = line.lstrip()
    indent = line[:len(line) - len(stripped)]
    
    # auto var = ...
    # Type var = ...
    # const Type var = ...
    # static Type var = ...
    
    # Insert [[maybe_unused]] right at the start (after indent), before any specifiers
    # But NOT after 'return', 'throw', etc.
    first_word = stripped.split()[0] if stripped.split() else ''
    if first_word in ('return', 'throw', 'delete', 'if', 'while', 'for', 'switch', 'case'):
        return line
    
    return indent + '[[maybe_unused]] ' + stripped


def remove_void_cast(line, varname):
    """Remove (void)varname; from line. Returns (new_line, was_only_content)."""
    new_line = re.sub(r'\(\s*void\s*\)\s*' + re.escape(varname) + r'\s*;\s*', '', line)
    # Clean up trailing whitespace but preserve newline
    nl = '\n' if line.endswith('\n') else ''
    stripped_new = new_line.rstrip()
    if stripped_new == '' or stripped_new.rstrip() == '':
        return None  # Delete line
    # Also handle inline comments that were after the void cast
    return stripped_new + nl if nl else stripped_new


def process_file(filepath):
    """Process a single file."""
    try:
        content = Path(filepath).read_text(encoding='utf-8', errors='replace')
    except Exception as e:
        return
    
    lines = content.splitlines(keepends=True)
    changes = {}  # line_idx -> new content or None (delete)
    
    for i, line in enumerate(lines):
        varnames = get_void_vars(line)
        if not varnames:
            continue
        
        # Find enclosing function body start
        func_start = find_enclosing_function_start(lines, i)
        
        for varname in varnames:
            # Special case: (void)this; just remove
            if varname == 'this':
                current = changes.get(i, line)
                if current is None:
                    continue
                result = remove_void_cast(current, varname)
                changes[i] = result
                stats['void_removed'] += 1
                continue
            
            # Find function end (scan forward for matching closing brace)
            depth = 0
            func_end = len(lines)
            for j in range(func_start, len(lines)):
                for ch in lines[j]:
                    if ch == '{':
                        depth += 1
                    elif ch == '}':
                        depth -= 1
                        if depth == 0:
                            func_end = j + 1
                            break
                if func_end != len(lines):
                    break
            
            # Check if used elsewhere in function
            used = is_used_elsewhere(lines, func_start, func_end, varname, i)
            
            # Remove void cast line
            current = changes.get(i, line)
            if current is not None:
                result = remove_void_cast(current, varname)
                changes[i] = result
            stats['void_removed'] += 1
            
            if not used:
                # Add [[maybe_unused]] to declaration
                # First try function parameter
                param_line_idx = find_param_line(lines, func_start, varname)
                if param_line_idx is not None:
                    decl = changes.get(param_line_idx, lines[param_line_idx])
                    if decl is not None and '[[maybe_unused]]' not in decl:
                        new_decl = add_maybe_unused_to_param(decl, varname)
                        if new_decl != decl:
                            changes[param_line_idx] = new_decl
                            stats['maybe_unused_added'] += 1
                else:
                    # Local variable
                    local_idx = find_local_decl_line(lines, i, varname)
                    if local_idx is not None:
                        decl = changes.get(local_idx, lines[local_idx])
                        if decl is not None and '[[maybe_unused]]' not in decl:
                            new_decl = add_maybe_unused_to_local(decl, varname)
                            if new_decl != decl:
                                changes[local_idx] = new_decl
                                stats['maybe_unused_added'] += 1
    
    if not changes:
        return
    
    new_lines = []
    for i, line in enumerate(lines):
        if i in changes:
            val = changes[i]
            if val is not None:
                new_lines.append(val)
            # else: delete line
        else:
            new_lines.append(line)
    
    new_content = ''.join(new_lines)
    if new_content != content:
        Path(filepath).write_text(new_content, encoding='utf-8')
        stats['files'].add(str(filepath))


def validate_changes(root):
    """Validate no bad patterns remain."""
    issues = []
    
    # Check for remaining (void)var patterns
    import subprocess
    r = subprocess.run(
        ['grep', '-rn', r'(void)[a-zA-Z_]', 'src/', 'include/',
         '--include=*.cpp', '--include=*.h', '--exclude-dir=.git'],
        cwd=str(root), capture_output=True, text=True
    )
    remaining = []
    for line in r.stdout.strip().split('\n'):
        if line and not re.search(r'#define|//.*\(void\)', line):
            remaining.append(line)
    
    # Check for bad [[maybe_unused]] placements
    r2 = subprocess.run(
        ['grep', '-rn', r'auto \[\[maybe_unused\]\]', 'src/', 'include/',
         '--include=*.cpp', '--include=*.h'],
        cwd=str(root), capture_output=True, text=True
    )
    bad_auto = [l for l in r2.stdout.strip().split('\n') if l]
    
    r3 = subprocess.run(
        ['grep', '-rn', r'\[\[maybe_unused\]\] return', 'src/', 'include/',
         '--include=*.cpp', '--include=*.h'],
        cwd=str(root), capture_output=True, text=True
    )
    bad_return = [l for l in r3.stdout.strip().split('\n') if l]
    
    return remaining, bad_auto, bad_return


def main():
    root = Path('/home/runner/work/ThemisDB/ThemisDB')
    
    all_files = []
    for d in [root / 'src', root / 'include']:
        if d.exists():
            all_files += list(d.rglob('*.cpp'))
            all_files += list(d.rglob('*.h'))
    
    print(f"Processing {len(all_files)} files...")
    
    for fp in sorted(all_files):
        process_file(fp)
    
    print(f"Files modified:          {len(stats['files'])}")
    print(f"(void)var; removed:      {stats['void_removed']}")
    print(f"[[maybe_unused]] added:  {stats['maybe_unused_added']}")
    
    print("\nValidating...")
    remaining, bad_auto, bad_return = validate_changes(root)
    
    if remaining:
        print(f"\n  Remaining (void)var patterns ({len(remaining)}):")
        for l in remaining[:30]:
            print(f"    {l}")
    else:
        print("  ✓ No remaining (void)var; patterns")
    
    if bad_auto:
        print(f"\n  Bad 'auto [[maybe_unused]]' patterns ({len(bad_auto)}):")
        for l in bad_auto[:10]:
            print(f"    {l}")
    else:
        print("  ✓ No bad auto [[maybe_unused]] patterns")
    
    if bad_return:
        print(f"\n  Bad '[[maybe_unused]] return' patterns ({len(bad_return)}):")
        for l in bad_return[:10]:
            print(f"    {l}")
    else:
        print("  ✓ No bad [[maybe_unused]] return patterns")


if __name__ == '__main__':
    main()
