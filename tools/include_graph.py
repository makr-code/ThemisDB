#!/usr/bin/env python3
"""
PoC Include & Documentation Graph Generator

Strategy:
- Prefer libclang (clang.cindex) for robust C/C++ parsing when available.
- Fallback to fast regex include extraction.
- Markdown files are chunked by top-level headings and added as nodes so
  documentation appears next to source nodes.

Output: JSON with nodes and edges. Nodes include file type and optional
markdown chunks. Edges encode include relationships.
"""

import os
import sys
import json
import argparse
import hashlib
from pathlib import Path
import re
from collections import defaultdict


def sha1(text: str) -> str:
    return hashlib.sha1(text.encode('utf-8')).hexdigest()


def find_files(source_dir: Path, exts):
    files = []
    for ext in exts:
        files.extend(source_dir.rglob(f'*{ext}'))
    return [p for p in files if p.is_file()]


INCLUDE_RE = re.compile(r'#\s*include\s*["<]([^">]+)[">]')


def parse_includes_regex(path: Path):
    try:
        txt = path.read_text(encoding='utf-8', errors='ignore')
    except Exception:
        return []
    return INCLUDE_RE.findall(txt)


def parse_includes_libclang(path: Path, repo_root: Path):
    try:
        import clang.cindex as clang
        index = clang.Index.create()
        # Use a permissive parse; if compile commands are available, clang will use them
        tu = index.parse(str(path), args=['-x', 'c++', '-std=c++17'])

        includes = []
        for inc in tu.get_includes():
            # inc.include can be None in some environments
            try:
                inc_path = inc.include.name if inc.include is not None else None
            except Exception:
                inc_path = None
            if inc_path:
                # Normalize to repo-relative when possible
                try:
                    rel = str(Path(inc_path).resolve().relative_to(repo_root.resolve()).as_posix())
                    includes.append(rel)
                except Exception:
                    includes.append(inc_path)

        return includes
    except Exception:
        return []


def chunk_markdown(path: Path, max_chunk_chars: int = 1200):
    try:
        txt = path.read_text(encoding='utf-8', errors='ignore')
    except Exception:
        return []

    # Split by H2/H1 headings but keep small chunks under max_chunk_chars
    parts = re.split(r'(?m)^#{1,3}\s+', txt)
    chunks = []
    for i, part in enumerate(parts):
        content = part.strip()
        if not content:
            continue
        title = content.splitlines()[0] if '\n' in content else content[:60]
        # further split large parts
        if len(content) > max_chunk_chars:
            for j in range(0, len(content), max_chunk_chars):
                sub = content[j:j+max_chunk_chars]
                chunks.append({'id': sha1(path.as_posix() + str(i) + str(j)), 'title': title, 'content': sub})
        else:
            chunks.append({'id': sha1(path.as_posix() + str(i)), 'title': title, 'content': content})

    return chunks


def try_resolve_include(include: str, file_dir: Path, repo_root: Path):
    # If include path is absolute-ish, try repo_root / include
    candidate = (file_dir / include).resolve()
    if candidate.exists():
        return str(candidate.relative_to(repo_root).as_posix())

    # Search for filename in repo (basename match)
    basename = os.path.basename(include)
    matches = list(repo_root.rglob(basename))
    if matches:
        return str(matches[0].relative_to(repo_root).as_posix())

    # fallback: return include as-is
    return include


def build_graph(source_dir: Path, repo_root: Path, use_libclang: bool = False):
    nodes = {}
    edges = []

    # Find source and header files
    src_exts = ['.c', '.cc', '.cxx', '.cpp', '.h', '.hh', '.hpp']
    md_exts = ['.md', '.markdown']

    src_files = find_files(source_dir, src_exts)
    md_files = find_files(source_dir, md_exts)

    # Process markdown files first
    for md in md_files:
        rel = str(md.relative_to(repo_root).as_posix())
        chunks = chunk_markdown(md)
        nodes[rel] = {
            'type': 'markdown',
            'path': rel,
            'chunks': chunks,
            'includes': []
        }

    # Process source files (regex include extraction)
    for sf in src_files:
        rel = str(sf.relative_to(repo_root).as_posix())
        if use_libclang:
            includes = parse_includes_libclang(sf, repo_root)
        else:
            includes = parse_includes_regex(sf)
        resolved = [try_resolve_include(inc, sf.parent, repo_root) for inc in includes]
        nodes[rel] = {
            'type': 'source' if sf.suffix in ('.c', '.cc', '.cxx', '.cpp') else 'header',
            'path': rel,
            'includes': resolved,
        }

        for target in resolved:
            edges.append({'from': rel, 'to': target, 'label': 'include'})

    return {'nodes': list(nodes.values()), 'edges': edges}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', nargs='?', default='.', help='Source directory')
    parser.add_argument('--output', '-o', default='ai_working/include_graph.json')
    parser.add_argument('--repo-root', default='.', help='Repo root for relative paths')
    parser.add_argument('--no-libclang', action='store_true', help='Disable libclang (force regex)')
    args = parser.parse_args()

    source = Path(args.source).resolve()
    repo_root = Path(args.repo_root).resolve()

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    use_libclang = False
    if not args.no_libclang:
        try:
            import clang.cindex as clang
            use_libclang = True
        except Exception:
            use_libclang = False

    graph = build_graph(source, repo_root, use_libclang)

    out_path.write_text(json.dumps(graph, indent=2), encoding='utf-8')
    print(f'Wrote include graph to {out_path}')


if __name__ == '__main__':
    main()
