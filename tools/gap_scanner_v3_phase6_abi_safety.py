#!/usr/bin/env python3
"""
Phase 6-1: ABI Safety & Memory Layout Scanner

CWE-400 (Resource Exhaustion), CWE-401 (Missing Release of Memory)

Detects ABI-breaking patterns, padding assumptions, and memory-layout violations
that can cause subtle bugs across compilation units or platform upgrades:

  A-1  Implicit padding (field reordering opportunity)
  A-2  Virtual-base diamond inheritance (ABI instability)
  A-3  #pragma pack directive (cross-TU ABI hazard)
  A-4  POD → non-POD transition (vtable injection)
  A-5  Bitfield layout assumptions (UB across compilers)
  A-6  Direct std::vector internal access (layout assumption)
  A-7  Misaligned pointer cast (alignment UB)
  A-8  Hidden offsetof dependency (fragile serialization)
  A-9  reinterpret_cast on aggregates (strict-aliasing UB)
  A-10 memcpy/memset on non-trivial types (undefined behaviour)

Expected gaps: 800–1,200 across the full codebase.
Complexity: HIGH (struct layout analysis, compiler-specific ABI rules).
"""

import re
from pathlib import Path
from typing import List, Dict


class ABISafetyScanner:
    """Scan C/C++ source files for ABI safety and memory-layout issues."""

    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    # ------------------------------------------------------------------
    # Public entry point
    # ------------------------------------------------------------------

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan *file_list* and return a list of gap dictionaries."""
        self.gaps = []

        for file_path in file_list:
            if file_path.suffix not in {'.cpp', '.cc', '.c', '.h', '.hpp', '.hxx'}:
                continue
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as fh:
                    content = fh.read()
                    lines = content.split('\n')
            except OSError:
                continue

            self._check_pragma_pack(file_path, lines)
            self._check_bitfield_layout(file_path, lines)
            self._check_pod_to_nonpod(file_path, lines)
            self._check_virtual_diamond(file_path, lines, content)
            self._check_implicit_padding(file_path, lines, content)
            self._check_vector_internals(file_path, lines)
            self._check_misaligned_cast(file_path, lines)
            self._check_offsetof_usage(file_path, lines)
            self._check_reinterpret_aggregate(file_path, lines)
            self._check_memop_nontrivial(file_path, lines)

        return self.gaps

    # ------------------------------------------------------------------
    # Helper
    # ------------------------------------------------------------------

    def _gap(self, file_path: Path, line: int, pattern: str,
             severity: str, description: str, context: str) -> None:
        try:
            rel = str(file_path.relative_to(self.repo_root))
        except ValueError:
            rel = str(file_path)
        self.gaps.append({
            'file': rel,
            'line': line,
            'category': 'abi_safety',
            'severity': severity,
            'pattern': pattern,
            'description': description,
            'context': context[:200].strip(),
        })

    # ------------------------------------------------------------------
    # A-3  #pragma pack
    # ------------------------------------------------------------------

    def _check_pragma_pack(self, file_path: Path, lines: List[str]) -> None:
        """A-3: #pragma pack directives introduce cross-TU ABI hazards."""
        for idx, line in enumerate(lines, 1):
            if re.search(r'#\s*pragma\s+pack\s*\(', line):
                self._gap(
                    file_path, idx, 'pragma_pack', 'HIGH',
                    '#pragma pack directive may break ABI across compilation units; '
                    'use alignas() instead and document packing rationale',
                    line,
                )

    # ------------------------------------------------------------------
    # A-5  Bitfield layout
    # ------------------------------------------------------------------

    def _check_bitfield_layout(self, file_path: Path, lines: List[str]) -> None:
        """A-5: Bitfield layout is implementation-defined and non-portable."""
        for idx, line in enumerate(lines, 1):
            if re.search(r':\s*\d+\s*;', line) and not re.search(
                r'//.*:\s*\d+', line
            ):
                # Heuristic: member declaration with ': N;' bit width
                if re.search(r'\b(int|uint|bool|char|short|long|unsigned)\b', line):
                    self._gap(
                        file_path, idx, 'bitfield_layout', 'MEDIUM',
                        'Bitfield layout is implementation-defined across compilers; '
                        'prefer std::bitset<N> or explicit bitmask constants',
                        line,
                    )

    # ------------------------------------------------------------------
    # A-4  POD → non-POD (virtual destructor on plain struct)
    # ------------------------------------------------------------------

    def _check_pod_to_nonpod(self, file_path: Path, lines: List[str]) -> None:
        """A-4: Adding virtual destructor to a previously POD struct injects vtable."""
        for idx, line in enumerate(lines, 1):
            if re.search(r'\bvirtual\s+~\w+\s*\(\s*\)', line):
                # Check surrounding context for 'struct' within 30 lines above
                start = max(0, idx - 30)
                ctx = '\n'.join(lines[start:idx])
                if re.search(r'\bstruct\b', ctx) and not re.search(
                    r'\bclass\b', ctx
                ):
                    self._gap(
                        file_path, idx, 'pod_to_nonpod', 'HIGH',
                        'Virtual destructor on struct turns POD into polymorphic type; '
                        'document ABI change or use separate interface class',
                        line,
                    )

    # ------------------------------------------------------------------
    # A-2  Virtual diamond inheritance
    # ------------------------------------------------------------------

    def _check_virtual_diamond(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """A-2: Multiple virtual inheritance paths to the same base (diamond)."""
        # Find lines with two or more inheritance bases including 'virtual'
        for idx, line in enumerate(lines, 1):
            if re.search(r':\s*.*virtual\s+\w+.*,.*virtual\s+\w+', line):
                self._gap(
                    file_path, idx, 'virtual_diamond', 'HIGH',
                    'Diamond (virtual) inheritance complicates ABI layout and vtable offsets; '
                    'prefer composition or single-inheritance interface chains',
                    line,
                )

    # ------------------------------------------------------------------
    # A-1  Implicit padding (mixed-alignment struct)
    # ------------------------------------------------------------------

    _FIELD_RE = re.compile(
        r'^\s*(bool|char|short|int|long|float|double|uint8_t|uint16_t|'
        r'int8_t|int16_t|uint32_t|int32_t|uint64_t|int64_t)\s+\w+',
    )

    def _check_implicit_padding(
        self, file_path: Path, lines: List[str], content: str
    ) -> None:
        """A-1: Struct fields with mixed alignment can introduce hidden padding."""
        in_struct = False
        brace_depth = 0
        struct_start = 0
        field_types: List[str] = []

        _TYPE_SIZES = {
            'bool': 1, 'char': 1, 'int8_t': 1, 'uint8_t': 1,
            'short': 2, 'int16_t': 2, 'uint16_t': 2,
            'int': 4, 'int32_t': 4, 'uint32_t': 4, 'float': 4,
            'long': 8, 'int64_t': 8, 'uint64_t': 8, 'double': 8,
        }

        for idx, line in enumerate(lines, 1):
            stripped = line.strip()

            if re.search(r'\b(struct|class)\b', stripped) and not re.search(
                r'^//', stripped
            ):
                in_struct = True
                brace_depth = 0
                struct_start = idx
                field_types = []

            if in_struct:
                brace_depth += stripped.count('{') - stripped.count('}')

                m = self._FIELD_RE.match(line)
                if m:
                    field_types.append(m.group(1))

                if brace_depth == 0 and struct_start < idx:
                    in_struct = False
                    # Simple heuristic: detect small type followed by larger type
                    for i in range(len(field_types) - 1):
                        t_cur = field_types[i]
                        t_next = field_types[i + 1]
                        sz_cur = _TYPE_SIZES.get(t_cur, 4)
                        sz_next = _TYPE_SIZES.get(t_next, 4)
                        if sz_cur < sz_next and sz_next >= 4:
                            self._gap(
                                file_path, struct_start,
                                'implicit_padding', 'MEDIUM',
                                f'Struct field order ({t_cur} before {t_next}) may create '
                                f'implicit padding; reorder fields by descending alignment '
                                f'to eliminate waste',
                                lines[struct_start - 1] if struct_start > 0 else '',
                            )
                            break

    # ------------------------------------------------------------------
    # A-6  Direct std::vector internal access
    # ------------------------------------------------------------------

    def _check_vector_internals(self, file_path: Path, lines: List[str]) -> None:
        """A-6: Accessing std::vector internals instead of .data()/.size()."""
        for idx, line in enumerate(lines, 1):
            # e.g. &v[0] used as a pointer, relying on contiguous layout
            if re.search(r'&\s*\w+\s*\[\s*0\s*\]', line):
                self._gap(
                    file_path, idx, 'vector_internal_access', 'LOW',
                    'Use .data() instead of &v[0] to access contiguous storage; '
                    '&v[0] is UB on empty vector',
                    line,
                )

    # ------------------------------------------------------------------
    # A-7  Misaligned pointer cast
    # ------------------------------------------------------------------

    def _check_misaligned_cast(self, file_path: Path, lines: List[str]) -> None:
        """A-7: C-style or reinterpret cast producing potentially misaligned pointer."""
        for idx, line in enumerate(lines, 1):
            # (T*) cast from char* / void* / byte*
            if re.search(
                r'\(\s*(int|long|double|float|uint32_t|uint64_t|int32_t|int64_t)\s*\*\s*\)'
                r'\s*(char|void|uint8_t|std::byte)',
                line,
            ):
                self._gap(
                    file_path, idx, 'misaligned_cast', 'HIGH',
                    'Cast from narrow-type pointer may produce misaligned access; '
                    'use std::memcpy or std::bit_cast to read aligned values',
                    line,
                )

    # ------------------------------------------------------------------
    # A-8  offsetof dependency
    # ------------------------------------------------------------------

    def _check_offsetof_usage(self, file_path: Path, lines: List[str]) -> None:
        """A-8: offsetof assertions/serialization creates fragile layout dependency."""
        for idx, line in enumerate(lines, 1):
            if re.search(r'\boffsetof\s*\(', line):
                self._gap(
                    file_path, idx, 'offsetof_dependency', 'MEDIUM',
                    'offsetof dependency creates fragile ABI contract; '
                    'use explicit serialization (protobuf / flatbuffers) or '
                    'document layout invariant with static_assert',
                    line,
                )

    # ------------------------------------------------------------------
    # A-9  reinterpret_cast on aggregate
    # ------------------------------------------------------------------

    def _check_reinterpret_aggregate(
        self, file_path: Path, lines: List[str]
    ) -> None:
        """A-9: reinterpret_cast on struct/class violates strict-aliasing rule."""
        for idx, line in enumerate(lines, 1):
            if re.search(r'reinterpret_cast\s*<', line):
                # Exclude known safe patterns: cast to char*, void*, std::byte*
                if not re.search(
                    r'reinterpret_cast\s*<\s*(const\s+)?(char|void|uint8_t|std::byte)\s*\*',
                    line,
                ):
                    self._gap(
                        file_path, idx, 'reinterpret_aggregate', 'HIGH',
                        'reinterpret_cast on non-byte type may violate strict-aliasing rule; '
                        'use std::bit_cast<> (C++20) or std::memcpy for safe type punning',
                        line,
                    )

    # ------------------------------------------------------------------
    # A-10  memcpy/memset on non-trivial types
    # ------------------------------------------------------------------

    def _check_memop_nontrivial(self, file_path: Path, lines: List[str]) -> None:
        """A-10: memcpy/memset on non-trivially-copyable objects is UB."""
        for idx, line in enumerate(lines, 1):
            if re.search(r'\b(memcpy|memset|memmove)\s*\(', line):
                # Look at argument: skip C-style raw buffers (char[], uint8_t[], void*)
                if not re.search(
                    r'(char|uint8_t|void|std::byte|unsigned char)\s*[\[\*]', line
                ):
                    self._gap(
                        file_path, idx, 'memop_nontrivial', 'HIGH',
                        'memcpy/memset on potentially non-trivial type is undefined behaviour; '
                        'use copy/move constructors or std::copy instead',
                        line,
                    )
