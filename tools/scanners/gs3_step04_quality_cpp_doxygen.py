#!/usr/bin/env python3
"""
Phase 10-8: Themis C++ Doxygen Policy Rules Scanner

Rule sources:
- Public C++ API documentation requirements
- Doxygen comment contracts for purpose/params/return behavior

Detects:
- missing doxygen comment for public API declarations
- missing @brief tag in existing doxygen comments
- missing @param tags for named parameters
- missing @return tag for non-void return declarations
- missing @tparam tags for named template parameters
- missing @throws tags for documented throwing definitions
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional


@dataclass
class _Decl:
    text: str
    start_line: int
    end_line: int
    class_name: Optional[str]
    access: str
    has_body: bool


class ThemisCppDoxygenPolicyRulesScan:
    """Scan public C++ declarations for Doxygen policy coverage gaps."""

    HEADER_EXTS = {".h", ".hpp", ".hh", ".hxx"}
    SOURCE_EXTS = {".c", ".cc", ".cpp", ".cxx"}
    ALL_EXTS = HEADER_EXTS | SOURCE_EXTS
    ACCESS_RE = re.compile(r"^\s*(public|protected|private)\s*:\s*$")
    CLASS_RE = re.compile(r"^\s*(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)[^;{]*\{\s*$")
    FUNCTION_NAME_RE = re.compile(r"([~A-Za-z_][A-Za-z0-9_:~]*)\s*\(")
    SKIP_PATH_MARKERS = (
        "/third_party/",
        "/external/",
        "/generated/",
        "/gen/",
        "/proto/",
        "/detail/",
        "/internal/",
        "/impl/",
        "/mock/",
        "/tests/",
        "/benchmarks/",
    )

    def __init__(self, repo_root: str = "."):
        self.repo_root = Path(repo_root)
        self.gaps: List[Dict] = []

    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        self.gaps = []

        scoped_modules = self._modules_in_scope(file_list)
        header_files = self._collect_public_headers(file_list, scoped_modules)
        
        # Also scan source files for public API implementations
        source_files = self._collect_source_files(file_list, scoped_modules)

        for header_path in header_files:
            self._scan_file(header_path)
            
        for source_path in source_files:
            self._scan_file(source_path)

        return self.gaps

    def _modules_in_scope(self, file_list: List[Path]) -> List[str]:
        modules: List[str] = []
        seen = set()
        for path in file_list:
            parts_lower = [part.lower() for part in path.parts]
            for anchor in ("src", "include"):
                if anchor in parts_lower:
                    idx = parts_lower.index(anchor)
                    if idx + 1 < len(path.parts):
                        name = path.parts[idx + 1]
                        if name not in seen:
                            seen.add(name)
                            modules.append(name)
        return modules

    def _collect_public_headers(self, file_list: List[Path], scoped_modules: List[str]) -> List[Path]:
        header_files = [
            path
            for path in file_list
            if path.suffix.lower() in self.HEADER_EXTS and self._is_public_api_header(path)
        ]

        include_root = self.repo_root / "include"
        if not include_root.exists() or not include_root.is_dir():
            return header_files

        if scoped_modules:
            for module in scoped_modules:
                candidate = include_root / module
                if candidate.exists() and candidate.is_dir():
                    for ext in self.HEADER_EXTS:
                        header_files.extend(candidate.rglob(f"*{ext}"))
        else:
            for ext in self.HEADER_EXTS:
                header_files.extend(include_root.rglob(f"*{ext}"))

        unique: List[Path] = []
        seen = set()
        for path in header_files:
            try:
                key = str(path.resolve())
            except Exception:
                key = str(path)
            if key in seen:
                continue
            seen.add(key)
            unique.append(path)

        return [path for path in unique if not self._should_skip_header(path)]

    def _is_public_api_header(self, path: Path) -> bool:
        normalized = "/".join(part.lower() for part in path.parts)
        return "/include/" in normalized or normalized.startswith("include/")
    
    def _is_source_file(self, path: Path) -> bool:
        """Check if this is a source file (not a header)."""
        return path.suffix.lower() in self.SOURCE_EXTS
    
    def _collect_source_files(self, file_list: List[Path], scoped_modules: List[str]) -> List[Path]:
        """Collect source files that might contain public API implementations."""
        source_files = [
            path
            for path in file_list
            if self._is_source_file(path) and self._is_public_api_source(path)
        ]
        
        src_root = self.repo_root / "src"
        if src_root.exists() and src_root.is_dir():
            if scoped_modules:
                for module in scoped_modules:
                    candidate = src_root / module
                    if candidate.exists() and candidate.is_dir():
                        for ext in self.SOURCE_EXTS:
                            source_files.extend(candidate.rglob(f"*{ext}"))
            else:
                for ext in self.SOURCE_EXTS:
                    source_files.extend(src_root.rglob(f"*{ext}"))
        
        unique: List[Path] = []
        seen = set()
        for path in source_files:
            try:
                key = str(path.resolve())
            except Exception:
                key = str(path)
            if key in seen:
                continue
            seen.add(key)
            unique.append(path)
        
        return [path for path in unique if not self._should_skip_header(path)]
    
    def _is_public_api_source(self, path: Path) -> bool:
        """Check if source file is in public API areas."""
        normalized = "/".join(part.lower() for part in path.parts)
        return "/src/" in normalized or normalized.startswith("src/")

    def _should_skip_header(self, path: Path) -> bool:
        normalized = "/" + "/".join(part.lower() for part in path.parts) + "/"
        if any(marker in normalized for marker in self.SKIP_PATH_MARKERS):
            return True

        name = path.name.lower()
        if name.endswith("_internal.h") or name.endswith("_internal.hpp"):
            return True

        return False

    def _scan_file(self, file_path: Path) -> None:
        try:
            text = file_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            return

        lines = text.splitlines()
        declarations = self._collect_declarations(lines)
        rel = str(file_path.relative_to(self.repo_root)).replace("\\", "/")

        for decl in declarations:
            if not self._looks_like_function_declaration(decl.text):
                continue

            signature = self._normalize_signature(decl.text)
            info = self._parse_signature(signature, decl.class_name)
            if info is None:
                continue

            if decl.class_name and decl.access != "public":
                continue

            if info["is_static"] and not decl.class_name:
                continue

            if info["skip_doc_enforcement"]:
                continue
            
            # Check class documentation if this is a class method
            if decl.class_name:
                class_doc = self._extract_leading_class_doc(lines, decl.start_line)
                if class_doc is None:
                    # Check if the class itself needs documentation
                    class_info = self._find_class_definition(lines, decl.class_name, decl.start_line)
                    if class_info and class_info['needs_doc']:
                        self._append(
                            rel,
                            class_info['line'],
                            "MEDIUM",
                            "missing_doxygen_class",
                            f"Class '{decl.class_name}' is missing a Doxygen comment",
                            decl.class_name,
                        )
                    elif class_info and class_info["template_params"]:
                        missing_tparams = [
                            param for param in class_info["template_params"]
                            if not self._has_tparam_doc(class_info["doc"], param)
                        ]
                        if missing_tparams:
                            self._append(
                                rel,
                                class_info["line"],
                                "LOW",
                                "missing_doxygen_tparam",
                                (
                                    f"Class '{decl.class_name}' is missing @tparam for: "
                                    f"{', '.join(missing_tparams)}"
                                ),
                                decl.class_name,
                            )

            doc = self._extract_leading_doc(lines, decl.start_line)
            if doc is None:
                self._append(
                    rel,
                    decl.start_line,
                    "MEDIUM",
                    "missing_doxygen_comment",
                    f"Public declaration '{info['name']}' is missing a Doxygen comment",
                    signature,
                )
                continue

            if "@brief" not in doc.lower():
                self._append(
                    rel,
                    decl.start_line,
                    "LOW",
                    "missing_doxygen_brief",
                    f"Doxygen comment for '{info['name']}' is missing @brief",
                    signature,
                )

            missing_params = [param for param in info["params"] if not self._has_param_doc(doc, param)]
            if missing_params:
                self._append(
                    rel,
                    decl.start_line,
                    "LOW",
                    "missing_doxygen_param",
                    f"Doxygen comment for '{info['name']}' is missing @param for: {', '.join(missing_params)}",
                    signature,
                )

            missing_tparams = [
                param for param in info["template_params"] if not self._has_tparam_doc(doc, param)
            ]
            if missing_tparams:
                self._append(
                    rel,
                    decl.start_line,
                    "LOW",
                    "missing_doxygen_tparam",
                    f"Doxygen comment for '{info['name']}' is missing @tparam for: {', '.join(missing_tparams)}",
                    signature,
                )

            if info["needs_return"] and "@return" not in doc.lower():
                self._append(
                    rel,
                    decl.start_line,
                    "LOW",
                    "missing_doxygen_return",
                    f"Doxygen comment for '{info['name']}' is missing @return",
                    signature,
                )

            if self._needs_throws_doc(lines, decl, signature) and not self._has_throws_doc(doc):
                self._append(
                    rel,
                    decl.start_line,
                    "LOW",
                    "missing_doxygen_throws",
                    f"Doxygen comment for '{info['name']}' is missing @throws/@exception",
                    signature,
                )

    def _append(self, file_rel: str, line: int, severity: str, pattern: str, description: str, context: str) -> None:
        self.gaps.append(
            {
                "file": file_rel,
                "line": line,
                "category": "cpp_doxygen_policy_rules",
                "severity": severity,
                "pattern": pattern,
                "description": description,
                "context": context[:180],
            }
        )

    def _collect_declarations(self, lines: List[str]) -> List[_Decl]:
        decls: List[_Decl] = []
        class_stack: List[Dict[str, object]] = []

        stmt_parts: List[str] = []
        stmt_start = 0

        in_block_comment = False
        brace_depth = 0

        for index, line in enumerate(lines, start=1):
            stripped = line.strip()

            if in_block_comment:
                if "*/" in line:
                    in_block_comment = False
                continue

            if stripped.startswith("/*"):
                if "*/" not in stripped:
                    in_block_comment = True
                continue

            if stripped.startswith("//"):
                continue

            access_match = self.ACCESS_RE.match(stripped)
            if access_match and class_stack:
                class_stack[-1]["access"] = access_match.group(1)
                continue

            class_match = self.CLASS_RE.match(stripped)
            if class_match:
                kind = class_match.group(1)
                name = class_match.group(2)
                class_stack.append({"name": name, "access": "public" if kind == "struct" else "private", "depth": brace_depth + 1})

            current_class = class_stack[-1] if class_stack else None
            class_depth = int(current_class["depth"]) if current_class else 0

            if current_class and brace_depth > class_depth:
                brace_depth += line.count("{") - line.count("}")
                while class_stack:
                    expected = int(class_stack[-1]["depth"])
                    if brace_depth < expected:
                        class_stack.pop()
                    else:
                        break
                continue

            if not stmt_parts and not stripped:
                continue

            if not stmt_parts:
                stmt_start = index

            if stripped and not stripped.startswith("#"):
                stmt_parts.append(stripped)

            candidate = " ".join(stmt_parts)
            is_function_like = self._looks_like_function_declaration(candidate)
            terminates_decl = ";" in stripped or ("{" in stripped and is_function_like)

            if terminates_decl and is_function_like:
                joined = " ".join(stmt_parts)
                if "{" in joined:
                    joined = joined.split("{", 1)[0].strip()
                decls.append(
                    _Decl(
                        text=joined,
                        start_line=stmt_start,
                        end_line=index,
                        class_name=current_class["name"] if current_class else None,
                        access=current_class["access"] if current_class else "public",
                        has_body="{" in stripped or "{" in joined,
                    )
                )
                stmt_parts = []
            elif ";" in stripped and not is_function_like:
                stmt_parts = []

            brace_depth += line.count("{") - line.count("}")
            while class_stack:
                expected = int(class_stack[-1]["depth"])
                if brace_depth < expected:
                    class_stack.pop()
                else:
                    break

        return decls

    def _looks_like_function_declaration(self, text: str) -> bool:
        normalized = text.strip()
        if not (normalized.endswith(";") or normalized.endswith("{")):
            return False
        if "(" not in normalized or ")" not in normalized:
            return False

        rejects = (
            "typedef ",
            "using ",
            "enum ",
            "static_assert",
            "friend class",
            "friend struct",
        )
        lowered = normalized.lower()
        if any(token in lowered for token in rejects):
            return False
        if "=" in normalized and "operator=" not in normalized:
            return False
        if normalized.endswith("{") and "{" in normalized[:-1] and "}" not in normalized:
            return True
        return True

    def _normalize_signature(self, signature: str) -> str:
        signature = re.sub(r"\s+", " ", signature).strip()
        signature = signature.replace(" ;", ";")
        return signature

    def _parse_signature(self, signature: str, class_name: Optional[str]) -> Optional[Dict]:
        left_paren = signature.find("(")
        right_paren = signature.rfind(")")
        if left_paren <= 0 or right_paren <= left_paren:
            return None

        prefix = signature[:left_paren].strip()
        params_str = signature[left_paren + 1 : right_paren].strip()
        name_match = self.FUNCTION_NAME_RE.search(signature)
        if not name_match:
            return None

        full_name = name_match.group(1)
        name = full_name.split("::")[-1]
        return_type = prefix[: -len(full_name)].strip() if prefix.endswith(full_name) else ""
        template_params = self._extract_template_params(signature)

        if name.startswith("operator"):
            return {
                "name": name,
                "params": [],
                "template_params": template_params,
                "needs_return": False,
                "needs_throws": False,
                "is_static": False,
                "skip_doc_enforcement": True,
            }

        params = self._extract_param_names(params_str)
        ctor_or_dtor = bool(class_name and (name == class_name or name == f"~{class_name}"))
        is_destructor = bool(class_name and name == f"~{class_name}")
        is_default_ctor = bool(class_name and name == class_name and not params_str)
        is_copy_move_ctor = False
        if class_name and name == class_name:
            compact = params_str.replace(" ", "")
            is_copy_move_ctor = (
                compact == f"const{class_name}&"
                or compact == f"{class_name}&&"
                or compact.startswith(f"const{class_name}&,")
                or compact.startswith(f"{class_name}&&,")
            )

        needs_return = not ctor_or_dtor and return_type.lower() != "void"

        is_static = " static " in f" {signature} "
        is_override = " override" in f" {signature} "
        is_defaulted_or_deleted = "= default" in signature or "= delete" in signature

        # Skip internal/trivial declarations to reduce false positives on non-public surfaces.
        owner_name = (class_name or "").lower()
        is_internal_owner = owner_name.endswith("impl") or owner_name.endswith("internal") or owner_name.endswith("private")
        is_internal_name = name.startswith("_") or name.lower().endswith("_impl") or name.lower().endswith("_internal")
        is_macro_like = name.isupper()
        is_trivial_accessor = bool(re.match(r"^(get|set|is|has)[A-Z_].*", name))

        skip_doc_enforcement = bool(
            is_destructor
            or is_default_ctor
            or is_copy_move_ctor
            or is_override
            or is_defaulted_or_deleted
            or ctor_or_dtor
            or is_internal_owner
            or is_internal_name
            or is_macro_like
            or is_trivial_accessor
        )

        return {
            "name": name,
            "params": params,
            "template_params": template_params,
            "needs_return": needs_return,
            "is_static": is_static,
            "skip_doc_enforcement": skip_doc_enforcement,
        }

    def _has_param_doc(self, doc: str, param: str) -> bool:
        pattern = re.compile(rf"@param(?:\[[^\]]+\])?\s+{re.escape(param)}\b", re.IGNORECASE)
        return bool(pattern.search(doc))

    def _has_tparam_doc(self, doc: str, param: str) -> bool:
        pattern = re.compile(rf"@tparam\s+{re.escape(param)}\b", re.IGNORECASE)
        return bool(pattern.search(doc))

    def _has_throws_doc(self, doc: str) -> bool:
        lowered = doc.lower()
        return "@throws" in lowered or "@exception" in lowered

    def _extract_template_params(self, signature: str) -> List[str]:
        match = re.search(r"template\s*<(.+?)>\s*", signature)
        if not match:
            return []

        params: List[str] = []
        for chunk in self._split_params(match.group(1)):
            token = chunk.split("=", 1)[0].strip()
            if not token or token == "...":
                continue
            candidates = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", token)
            if not candidates:
                continue
            if candidates[0] == "template" and len(candidates) > 1:
                params.append(candidates[-1])
                continue
            params.append(candidates[-1])
        return params

    def _needs_throws_doc(self, lines: List[str], decl: _Decl, signature: str) -> bool:
        if not decl.has_body:
            return False
        if " noexcept" in f" {signature} " and "noexcept(false)" not in signature:
            return False
        return self._body_contains_throw(lines, decl)

    def _body_contains_throw(self, lines: List[str], decl: _Decl) -> bool:
        body_started = False
        brace_depth = 0
        for line in lines[decl.start_line - 1 :]:
            in_string = False
            escaped = False
            code_chars: List[str] = []
            for ch in line:
                if ch == '"' and not escaped:
                    in_string = not in_string
                if not in_string:
                    code_chars.append(ch)
                escaped = ch == "\\" and not escaped
            code = "".join(code_chars).split("//", 1)[0]
            for ch in code:
                if ch == "{":
                    brace_depth += 1
                    body_started = True
                elif ch == "}":
                    brace_depth -= 1
            if body_started and re.search(r"\bthrow\b", code):
                return True
            if body_started and brace_depth <= 0:
                break
        return False

    def _extract_param_names(self, params_str: str) -> List[str]:
        if not params_str or params_str == "void":
            return []

        chunks = self._split_params(params_str)
        names: List[str] = []
        for chunk in chunks:
            token = chunk.strip()
            if not token or token == "...":
                continue
            token = token.split("=", 1)[0].strip()

            candidates = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", token)
            if not candidates:
                continue

            name = candidates[-1]
            if name in {"const", "volatile", "noexcept", "final", "override"}:
                continue
            names.append(name)

        return names

    def _split_params(self, params: str) -> List[str]:
        parts: List[str] = []
        buf: List[str] = []
        angle = 0
        paren = 0
        bracket = 0

        for ch in params:
            if ch == "<":
                angle += 1
            elif ch == ">" and angle > 0:
                angle -= 1
            elif ch == "(":
                paren += 1
            elif ch == ")" and paren > 0:
                paren -= 1
            elif ch == "[":
                bracket += 1
            elif ch == "]" and bracket > 0:
                bracket -= 1

            if ch == "," and angle == 0 and paren == 0 and bracket == 0:
                parts.append("".join(buf))
                buf = []
                continue

            buf.append(ch)

        if buf:
            parts.append("".join(buf))

        return parts

    def _extract_leading_doc(self, lines: List[str], start_line: int) -> Optional[str]:
        idx = start_line - 2
        while idx >= 0 and not lines[idx].strip():
            idx -= 1

        if idx < 0:
            return None

        line = lines[idx].lstrip()
        if line.startswith("///"):
            collected: List[str] = []
            while idx >= 0 and lines[idx].lstrip().startswith("///"):
                collected.append(lines[idx].lstrip()[3:].strip())
                idx -= 1
            collected.reverse()
            return "\n".join(collected)

        if "*/" in line:
            block: List[str] = [line]
            idx -= 1
            found_start = False
            while idx >= 0:
                current = lines[idx].lstrip()
                block.append(current)
                if current.startswith("/**") or current.startswith("/*!"):
                    found_start = True
                    break
                if current.startswith("/*"):
                    break
                idx -= 1
            if not found_start:
                return None
            block.reverse()
            return "\n".join(block)

        return None
    
    def _extract_leading_class_doc(self, lines: List[str], start_line: int) -> Optional[str]:
        """Extract leading documentation for a class."""
        # Look backwards from the start_line to find class definition and its doc
        idx = start_line - 2
        while idx >= 0 and not lines[idx].strip():
            idx -= 1
        
        if idx < 0:
            return None
        
        line = lines[idx].lstrip()
        if line.startswith("///"):
            collected: List[str] = []
            while idx >= 0 and lines[idx].lstrip().startswith("///"):
                collected.append(lines[idx].lstrip()[3:].strip())
                idx -= 1
            collected.reverse()
            return "\n".join(collected)
        
        if "*/" in line:
            block: List[str] = [line]
            idx -= 1
            found_start = False
            while idx >= 0:
                current = lines[idx].lstrip()
                block.append(current)
                if current.startswith("/**") or current.startswith("/*!"):
                    found_start = True
                    break
                if current.startswith("/*"):
                    break
                idx -= 1
            if not found_start:
                return None
            block.reverse()
            return "\n".join(block)
        
        return None
    
    def _find_class_definition(self, lines: List[str], class_name: str, start_line: int) -> Optional[Dict]:
        """Find the class definition and check if it has documentation."""
        # Look backwards for class definition
        class_pattern = re.compile(rf'\b(class|struct)\s+{re.escape(class_name)}\s*[{{:]')
        
        idx = start_line - 1
        while idx >= 0:
            line = lines[idx].strip()
            if class_pattern.search(line):
                # Check if there's documentation before this line
                class_doc = self._extract_leading_class_doc(lines, idx)
                needs_doc = class_doc is None or "@brief" not in (class_doc or "").lower()
                template_params = self._extract_template_prefix_params(lines, idx + 1)
                return {
                    'line': idx + 1,  # 1-indexed
                    'name': class_name,
                    'needs_doc': needs_doc,
                    'doc': class_doc or "",
                    'template_params': template_params,
                }
            idx -= 1
        
        return None

    def _extract_template_prefix_params(self, lines: List[str], start_line: int) -> List[str]:
        idx = start_line - 2
        template_lines: List[str] = []
        while idx >= 0:
            stripped = lines[idx].strip()
            if not stripped:
                idx -= 1
                continue
            if stripped.startswith("template"):
                template_lines.append(stripped)
                idx -= 1
                continue
            break
        if not template_lines:
            return []
        template_lines.reverse()
        return self._extract_template_params(" ".join(template_lines))
