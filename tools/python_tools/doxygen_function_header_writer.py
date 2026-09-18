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
    "ai_working",
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
        if not base.exists():
            continue
        if base.is_file():
            if base.suffix.lower() in CPP_EXTENSIONS and not should_skip_file(base):
                if not public_only or "include" in base.parts:
                    yield base
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
    depth = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if depth > 0:
            clean = strip_strings_and_comments(line)
            for ch in clean:
                if ch == "{":
                    depth += 1
                elif ch == "}":
                    depth -= 1
            i += 1
            continue

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
            if parsed.has_body:
                depth = 0
                body_end = end
                for j in range(anchor, len(lines)):
                    clean = strip_strings_and_comments(lines[j])
                    for ch in clean:
                        if ch == "{":
                            depth += 1
                        elif ch == "}":
                            depth -= 1
                    if j >= end and depth <= 0:
                        body_end = j
                        break
                i = body_end + 1
            else:
                i = end + 1
            accepted = True
            break

        if not accepted:
            i += 1

    return matches


def remove_internal_doxygen_comments(lines: List[str]) -> Tuple[List[str], int]:
    cleaned: List[str] = []
    removed_lines = 0
    depth = 0
    i = 0

    while i < len(lines):
        line = lines[i]
        stripped = line.lstrip()

        if depth > 0 and (stripped.startswith("/**") or stripped.startswith("///")):
            start = i
            if stripped.startswith("/**"):
                while i < len(lines):
                    removed_lines += 1
                    if "*/" in lines[i]:
                        i += 1
                        break
                    i += 1
                continue

            while i < len(lines) and lines[i].lstrip().startswith("///"):
                removed_lines += 1
                i += 1
            continue

        cleaned.append(line)
        clean = strip_strings_and_comments(line)
        for ch in clean:
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
        i += 1

    return cleaned, removed_lines


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
                block_lines = lines[i : j + 1]
                if is_section_heading_doxygen_block(block_lines):
                    return None
                return (i, j)
            j += 1
        return None

    if lines[i].strip().endswith("*/"):
        end = i
        k = i
        while k >= 0:
            if DOXYGEN_START_RE.match(lines[k]):
                block_lines = lines[k : end + 1]
                if is_section_heading_doxygen_block(block_lines):
                    return None
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


def collect_single_line_triple_slash_comment(lines: List[str], func_start_line: int) -> Optional[ExistingComment]:
    i = func_start_line - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0:
        return None

    if not lines[i].lstrip().startswith("///"):
        return None

    start = i
    while start - 1 >= 0 and lines[start - 1].lstrip().startswith("///"):
        start -= 1

    if start != i:
        return None

    content = lines[i].lstrip()[3:].strip()
    return ExistingComment(start_line=i, end_line=i, text=content)


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
        if is_section_heading_comment(lines, start, end):
            return None
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
        return fallback_brief(func_name), "Calls: none detected."

    sentence_end = re.search(r"[.!?]", cleaned)
    if sentence_end:
        brief = cleaned[: sentence_end.end()].strip()
        tail = cleaned[sentence_end.end() :].strip()
    else:
        brief = cleaned
        tail = ""

    if not brief:
        brief = fallback_brief(func_name)

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


def short_function_name(func_name: str) -> str:
    return func_name.split("::")[-1].lstrip("~")


def normalize_doc_text(text: str) -> str:
    return re.sub(r"\s+", " ", text or "").strip().rstrip(".").lower()


FUNCTION_BRIEF_OVERRIDES = {
    "registerPolicy": "Register a retention policy.",
    "removePolicy": "Remove a retention policy by name.",
    "getPolicies": "Return all configured retention policies.",
    "getPolicy": "Look up a retention policy by name.",
    "shouldArchive": "Check whether an entity has reached the archive threshold.",
    "shouldPurge": "Check whether an entity has reached the purge threshold.",
    "archiveEntity": "Archive an entity through the configured handler.",
    "purgeEntity": "Purge an entity through the configured handler.",
    "runRetentionCheck": "Run retention checks for all configured policies.",
    "getHistory": "Return a bounded slice of the recent retention action history.",
    "getPolicyStats": "Return the stored statistics for a retention policy.",
    "loadPolicies": "Load retention policies from a configuration file.",
    "classify": "Classify the semantic intent of a query.",
    "maybeAlert": "Produce an intent alert when the confidence threshold is met.",
    "intentName": "Return a human-readable name for an intent type.",
    "setInferenceFn": "Inject an inference function used by classify().",
    "buildEmbedding": "Build a deterministic anonymized embedding from an intent.",
    "configureLoraEndpoint": "Configure the LoRA classify endpoint.",
    "validateVector": "Validate that a JSON value is a numeric vector.",
    "validateSameDimension": "Validate that two vectors have the same dimension.",
    "toVector": "Convert a JSON array to std::vector<double>.",
    "fromVector": "Convert std::vector<double> to a JSON array.",
    "l2Norm": "Compute the L2 norm of a vector.",
    "dotProduct": "Compute the dot product of two vectors.",
    "set_modification_detected": "Explicitly mark modification detection state.",
    "reset": "Reset the modification detection flag.",
    "initial_size": "Return the container size captured at construction time.",
    "advance": "Advance an iterator within the validated range.",
    "can_advance": "Check whether an iterator can advance without leaving the range.",
    "RangeValidator": "Validate and store a safe iterator range.",
    "AccessControl": "Construct the access control subsystem.",
    "registerUser": "Register a new user with a password and optional plugin.",
    "changePassword": "Change a user's password.",
    "enrollMFA": "Enroll multi-factor authentication for a user.",
    "verifyMFA": "Verify a multi-factor authentication token.",
    "disableMFA": "Disable multi-factor authentication for a user.",
    "authorize": "Authorize an access control context.",
    "checkPermission": "Check whether a role grants permission for an action.",
    "assignRole": "Assign a role to a user.",
    "revokeRole": "Revoke a role from a user.",
    "getUserPermissions": "Get the permissions assigned to a user.",
    "getUserRoles": "Get the roles assigned to a user.",
    "createSession": "Create a session for an authenticated user.",
    "validateSession": "Validate a session token.",
    "invalidateSession": "Invalidate a session token.",
    "invalidateUserSessions": "Invalidate all sessions for a user.",
    "isRateLimited": "Check whether a user is rate limited.",
    "detectSQLInjection": "Detect SQL injection patterns in a query.",
    "detectSuspiciousQuery": "Detect suspicious query patterns.",
    "recordFailedLogin": "Record a failed login attempt.",
    "isLockedOut": "Check whether a user is locked out.",
    "logSecurityEvent": "Log a security event.",
    "updateConfig": "Update the access control configuration.",
    "addABACPolicy": "Add an ABAC policy.",
    "removeABACPolicy": "Remove an ABAC policy.",
    "isSessionExpired": "Check whether a session is expired.",
    "cleanupExpiredSessions": "Remove expired sessions from the cache.",
    "updateRateLimit": "Update rate limit state for a user.",
    "checkRateLimit": "Check whether a user exceeds the current rate limit.",
    "getUserRoleStore": "Return the user role store.",
    "getUserRegistrationPluginManager": "Return the user registration plugin manager.",
    "getUserSessionStore": "Return the user session store.",
    "getUserRolesLocked": "Get roles for a user while holding the lock.",
    "createSessionLocked": "Create a session while holding the lock.",
    "invalidateSessionLocked": "Invalidate a session while holding the lock.",
    "invalidateUserSessionsLocked": "Invalidate all user sessions while holding the lock.",
    "getStatistics": "Return access control statistics.",
    "getRBAC": "Return the RBAC subsystem.",
    "getABACEngine": "Return the ABAC policy engine.",
    "generateSessionToken": "Generate a new session token.",
    "ZeroTrustPolicyEnforcer": "Construct the zero-trust policy enforcer.",
    "addNetworkPolicy": "Register a network policy.",
    "removeNetworkPolicy": "Remove a network policy by id.",
    "getNetworkPolicies": "Return all currently registered network policies.",
    "verify": "Verify identity and enforce network policies for a request.",
    "setAllowUnverifiedToken": "Allow access when no token verifier is configured.",
    "setAllowEmptyNetworkPolicies": "Allow access when no network policies are registered.",
    "verifyToken": "Verify a bearer token or credential for a user.",
    "isIpAllowed": "Check whether an IP address is allowed for an identity.",
    "computeTrustScore": "Compute the composite zero-trust score.",
    "ipMatchesCidr": "Check whether an IP address matches a CIDR range.",
    "parseIpv4": "Parse an IPv4 address into an integer representation.",
    "parseIpv6": "Parse an IPv6 address into a byte array representation.",
    "ipv6MatchesCidr": "Check whether an IPv6 address matches a CIDR range.",
    "normaliseIpv4MappedIpv6": "Normalize an IPv4-mapped IPv6 address.",
    "ipMatchesCidrAny": "Check whether an IP matches any CIDR in a policy.",
    "findPolicyForIdentity": "Find the policy that applies to an identity.",
}


PARAM_DESCRIPTION_OVERRIDES = {
    ("registerPolicy", "policy"): "Retention policy definition to store.",
    ("removePolicy", "policy_name"): "Name of the retention policy to remove.",
    ("getPolicy", "policy_name"): "Name of the retention policy to retrieve.",
    ("shouldArchive", "entity_id"): "Identifier of the entity being evaluated.",
    ("shouldArchive", "created_at"): "Creation timestamp used for the age comparison.",
    ("shouldArchive", "policy_name"): "Name of the retention policy to apply.",
    ("shouldPurge", "entity_id"): "Identifier of the entity being evaluated.",
    ("shouldPurge", "created_at"): "Creation timestamp used for the age comparison.",
    ("shouldPurge", "policy_name"): "Name of the retention policy to apply.",
    ("archiveEntity", "entity_id"): "Identifier of the entity to archive.",
    ("archiveEntity", "policy_name"): "Name of the retention policy that triggered the action.",
    ("archiveEntity", "archive_handler"): "Callback that archives the entity.",
    ("purgeEntity", "entity_id"): "Identifier of the entity to purge.",
    ("purgeEntity", "policy_name"): "Name of the retention policy that triggered the action.",
    ("purgeEntity", "purge_handler"): "Callback that purges the entity.",
    ("runRetentionCheck", "entity_provider"): "Callback that returns candidate entities for a policy.",
    ("runRetentionCheck", "archive_handler"): "Callback used when an entity should be archived.",
    ("runRetentionCheck", "purge_handler"): "Callback used when an entity should be purged.",
    ("getHistory", "limit"): "Maximum number of history entries to return.",
    ("getPolicyStats", "policy_name"): "Name of the retention policy to query.",
    ("loadPolicies", "config_path"): "Path to the retention policy configuration file.",
    ("classify", "query"): "Raw query string to classify.",
    ("classify", "session_context"): "Current zero-trust context for the query.",
    ("maybeAlert", "result"): "Classification result produced by classify().",
    ("maybeAlert", "session_id"): "Session identifier.",
    ("maybeAlert", "confidence_threshold"): "Minimum confidence required to emit an alert.",
    ("intentName", "t"): "Intent type to render.",
    ("setInferenceFn", "fn"): "Inference function to inject.",
    ("buildEmbedding", "intent"): "Intent category to encode.",
    ("buildEmbedding", "primary_indicator"): "Primary indicator token used for the embedding.",
    ("configureLoraEndpoint", "endpoint_url"): "Full URL of the LoRA classify endpoint.",
    ("configureLoraEndpoint", "api_key"): "Optional bearer token for the endpoint.",
    ("configureLoraEndpoint", "timeout_ms"): "HTTP request timeout in milliseconds.",
    ("validateVector", "vec"): "JSON vector to validate.",
    ("validateVector", "funcName"): "Function name used in error messages.",
    ("validateSameDimension", "v1"): "First vector.",
    ("validateSameDimension", "v2"): "Second vector.",
    ("validateSameDimension", "funcName"): "Function name used in error messages.",
    ("toVector", "vec"): "JSON array of numbers to convert.",
    ("fromVector", "vec"): "Vector to convert to JSON.",
    ("l2Norm", "vec"): "Vector whose magnitude is computed.",
    ("dotProduct", "v1"): "First vector.",
    ("dotProduct", "v2"): "Second vector.",
    ("set_modification_detected", "detected"): "True to mark the container as modified.",
    ("advance", "it"): "Iterator to advance in place.",
    ("advance", "distance"): "Number of steps to advance.",
    ("advance", "begin"): "Beginning of the valid range.",
    ("advance", "end"): "End of the valid range.",
    ("can_advance", "it"): "Iterator to test.",
    ("can_advance", "distance"): "Number of steps to validate.",
    ("can_advance", "begin"): "Beginning of the valid range.",
    ("can_advance", "end"): "End of the valid range.",
    ("RangeValidator", "begin"): "Beginning of the iterator range.",
    ("RangeValidator", "end"): "End of the iterator range.",
    ("AccessControl", "config"): "Access control configuration.",
    ("registerUser", "user_id"): "User identifier.",
    ("registerUser", "password"): "Plaintext password to register.",
    ("registerUser", "plugin_name"): "Optional user-registration plugin name.",
    ("registerUser", "attributes"): "Optional user attributes passed to the plugin.",
    ("authenticate", "credentials"): "User credentials to authenticate.",
    ("getUserPermissions", "user_id"): "User identifier.",
    ("getUserRoles", "user_id"): "User identifier.",
    ("createSession", "user_id"): "User identifier.",
    ("createSession", "roles"): "Roles to attach to the session.",
    ("createSession", "mfa_verified"): "True if MFA was verified for the session.",
    ("validateSession", "session_token"): "Session token to validate.",
    ("changePassword", "user_id"): "User identifier.",
    ("changePassword", "old_password"): "Current password.",
    ("changePassword", "new_password"): "Replacement password.",
    ("enrollMFA", "user_id"): "User identifier.",
    ("verifyMFA", "user_id"): "User identifier.",
    ("verifyMFA", "token"): "MFA token to verify.",
    ("disableMFA", "user_id"): "User identifier.",
    ("authorize", "context"): "Authorization context to evaluate.",
    ("checkPermission", "role"): "Role to evaluate.",
    ("checkPermission", "resource"): "Protected resource identifier.",
    ("checkPermission", "action"): "Requested action.",
    ("assignRole", "user_id"): "User identifier.",
    ("assignRole", "role"): "Role to assign.",
    ("revokeRole", "user_id"): "User identifier.",
    ("revokeRole", "role"): "Role to revoke.",
    ("invalidateSession", "session_token"): "Session token to invalidate.",
    ("invalidateUserSessions", "user_id"): "User identifier.",
    ("isRateLimited", "user_id"): "User identifier.",
    ("isRateLimited", "resource"): "Resource being accessed.",
    ("detectSQLInjection", "query"): "Query string to inspect.",
    ("detectSuspiciousQuery", "query"): "Query string to inspect.",
    ("detectSuspiciousQuery", "user_id"): "User identifier.",
    ("recordFailedLogin", "user_id"): "User identifier.",
    ("recordFailedLogin", "ip_address"): "Source IP address.",
    ("isLockedOut", "user_id"): "User identifier.",
    ("logSecurityEvent", "event_type"): "Security event type.",
    ("logSecurityEvent", "details"): "Event payload/details.",
    ("updateConfig", "config"): "New access control configuration.",
    ("addABACPolicy", "policy"): "ABAC policy to add.",
    ("removeABACPolicy", "policy_id"): "Identifier of the ABAC policy to remove.",
    ("isSessionExpired", "session"): "Session to check.",
    ("updateRateLimit", "user_id"): "User identifier.",
    ("checkRateLimit", "user_id"): "User identifier.",
    ("getUserRoleStore", "role_store"): "Role store backing the access control subsystem.",
    ("getUserRegistrationPluginManager", "plugin_manager"): "Registration plugin manager backing the access control subsystem.",
    ("getUserSessionStore", "session_store"): "Session store backing the access control subsystem.",
    ("getUserRolesLocked", "user_id"): "User identifier.",
    ("createSessionLocked", "user_id"): "User identifier.",
    ("createSessionLocked", "roles"): "Roles to attach to the session.",
    ("createSessionLocked", "mfa_verified"): "True if MFA was verified for the session.",
    ("invalidateSessionLocked", "session_token"): "Session token to invalidate.",
    ("invalidateUserSessionsLocked", "user_id"): "User identifier.",
    ("getStatistics", "statistics"): "Access control statistics.",
    ("generateSessionToken", "session_token"): "Generated session token.",
    ("ZeroTrustPolicyEnforcer", "token_verifier"): "Optional token verification callback.",
    ("addNetworkPolicy", "policy"): "Network policy to add.",
    ("removeNetworkPolicy", "policy_id"): "Identifier of the policy to remove.",
    ("verify", "context"): "Zero-trust request context to verify.",
    ("verifyToken", "token"): "Token to verify.",
    ("verifyToken", "user_id"): "User identifier.",
    ("isIpAllowed", "client_ip"): "Client IP address.",
    ("isIpAllowed", "identity"): "Identity associated with the request.",
    ("computeTrustScore", "context"): "Zero-trust request context.",
    ("computeTrustScore", "identity_verified"): "True if identity verification succeeded.",
    ("computeTrustScore", "network_ok"): "True if network policy checks passed.",
    ("ipMatchesCidr", "ip"): "IP address to check.",
    ("ipMatchesCidr", "cidr"): "CIDR range to compare against.",
    ("parseIpv4", "ip"): "IPv4 address string.",
    ("parseIpv4", "out"): "Output numeric IPv4 value.",
    ("parseIpv6", "ip"): "IPv6 address string.",
    ("parseIpv6", "out"): "Output IPv6 byte array.",
    ("ipv6MatchesCidr", "ip"): "IPv6 address to check.",
    ("ipv6MatchesCidr", "cidr"): "CIDR range to compare against.",
    ("normaliseIpv4MappedIpv6", "ip"): "IP address to normalize.",
    ("ipMatchesCidrAny", "ip"): "IP address to check.",
    ("ipMatchesCidrAny", "cidr"): "CIDR range to compare against.",
    ("findPolicyForIdentity", "identity"): "Identity to look up.",
}


RETURN_DESCRIPTION_OVERRIDES = {
    "registerPolicy": "True when the policy was accepted and stored.",
    "removePolicy": "True when the policy existed and was removed.",
    "getPolicies": "Copy of the current policy list.",
    "getPolicy": "Pointer to the stored policy on success, or an error if it is missing.",
    "shouldArchive": "True when the entity should be archived.",
    "shouldPurge": "True when the entity should be purged.",
    "archiveEntity": "Action record with success state, error text, and timestamps.",
    "purgeEntity": "Action record with success state, error text, and timestamps.",
    "runRetentionCheck": "Aggregate retention statistics for the full run.",
    "getHistory": "Most recent actions, or the full history when the limit is zero or oversized.",
    "getPolicyStats": "Stored statistics, or a default-initialized record if the policy is unknown.",
    "validateVector": "None.",
    "validateSameDimension": "None.",
    "toVector": "std::vector<double> containing the numeric values.",
    "fromVector": "JSON array containing the vector values.",
    "l2Norm": "Euclidean length of the vector.",
    "dotProduct": "Dot product of the input vectors.",
    "set_modification_detected": "None.",
    "reset": "None.",
    "initial_size": "Initial container size captured at construction.",
    "advance": "None.",
    "can_advance": "True when the iterator can advance safely.",
    "RangeValidator": "Validated range object.",
    "AccessControl": "Access control subsystem instance.",
    "registerUser": "Registration result.",
    "authenticate": "Authentication result.",
    "changePassword": "Result indicating whether the password changed.",
    "enrollMFA": "Enrollment result as JSON.",
    "verifyMFA": "True when the token is valid.",
    "disableMFA": "Result indicating whether MFA was disabled.",
    "authorize": "True when the context is authorized.",
    "checkPermission": "True when the permission is granted.",
    "assignRole": "Result indicating whether the role was assigned.",
    "revokeRole": "Result indicating whether the role was revoked.",
    "getUserPermissions": "Permissions assigned to the user.",
    "getUserRoles": "Roles assigned to the user.",
    "createSession": "Session token.",
    "validateSession": "Validated session on success.",
    "invalidateSession": "None.",
    "invalidateUserSessions": "None.",
    "isRateLimited": "True when the user is rate limited.",
    "detectSQLInjection": "True when an injection pattern is detected.",
    "detectSuspiciousQuery": "True when the query is suspicious.",
    "recordFailedLogin": "None.",
    "isLockedOut": "True when the user is locked out.",
    "logSecurityEvent": "None.",
    "updateConfig": "None.",
    "addABACPolicy": "None.",
    "removeABACPolicy": "True when the policy was removed.",
    "isSessionExpired": "True when the session is expired.",
    "cleanupExpiredSessions": "None.",
    "updateRateLimit": "None.",
    "checkRateLimit": "True when the user remains within the configured limit.",
    "getUserRoleStore": "Role store reference.",
    "getUserRegistrationPluginManager": "Plugin manager reference.",
    "getUserSessionStore": "Session store reference.",
    "getUserRolesLocked": "Roles assigned to the user.",
    "createSessionLocked": "Session token.",
    "invalidateSessionLocked": "None.",
    "invalidateUserSessionsLocked": "None.",
    "getStatistics": "Access control statistics.",
    "getRBAC": "RBAC subsystem reference.",
    "getABACEngine": "ABAC policy engine reference.",
    "generateSessionToken": "Generated session token.",
    "ZeroTrustPolicyEnforcer": "Zero-trust policy enforcer instance.",
    "addNetworkPolicy": "None.",
    "removeNetworkPolicy": "True when a policy was removed.",
    "getNetworkPolicies": "Snapshot of network policies.",
    "verify": "Verification result.",
    "setAllowUnverifiedToken": "None.",
    "setAllowEmptyNetworkPolicies": "None.",
    "verifyToken": "True when the token is valid.",
    "isIpAllowed": "True when the IP is allowed.",
    "computeTrustScore": "Composite zero-trust score.",
    "ipMatchesCidr": "True when the IP matches the CIDR.",
    "parseIpv4": "True when the IPv4 address parsed successfully.",
    "parseIpv6": "True when the IPv6 address parsed successfully.",
    "ipv6MatchesCidr": "True when the IP matches the CIDR.",
    "normaliseIpv4MappedIpv6": "Normalized IP string.",
    "ipMatchesCidrAny": "True when the IP matches at least one CIDR.",
    "findPolicyForIdentity": "Matching network policy or null if none.",
}


def describe_param(func_name: str, param_name: str, direction: str) -> str:
    short_name = short_function_name(func_name)
    override = PARAM_DESCRIPTION_OVERRIDES.get((short_name, param_name))
    if override:
        return override

    if param_name.endswith("_name"):
        subject = "retention policy" if "Policy" in short_name else param_name[:-5].replace("_", " ")
        if subject:
            return f"Name of the {subject}."

    if param_name.endswith("_path"):
        subject = param_name[:-5].replace("_", " ")
        if subject == "config":
            return "Path to the retention policy configuration file."
        if subject:
            return f"Path to the {subject}."

    if param_name.endswith("_id"):
        subject = param_name[:-3].replace("_", " ")
        if subject:
            return f"Identifier of the {subject}."

    if param_name == "created_at":
        return "Creation timestamp used for the age comparison."

    if param_name.endswith("_handler"):
        action = param_name[:-8].replace("_", " ")
        if action:
            return f"Callback that {action}s the entity."

    if direction == "[in,out]":
        return "Input/output parameter."
    if direction == "[out]":
        return "Output parameter."
    return "Input parameter."


def describe_return(return_type: str, func_name: str = "") -> str:
    short_name = short_function_name(func_name) if func_name else ""
    override = RETURN_DESCRIPTION_OVERRIDES.get(short_name)
    if override:
        return override

    normalized = re.sub(r"\s+", " ", return_type.strip()).lower()
    if normalized == "bool":
        if short_name == "shouldArchive":
            return "True when the entity should be archived."
        if short_name == "shouldPurge":
            return "True when the entity should be purged."
        return "True when the operation succeeds."
    if normalized.endswith("*"):
        return "Pointer to the result."
    return "Return value."


def fallback_brief(func_name: str) -> str:
    name = short_function_name(func_name)
    override = FUNCTION_BRIEF_OVERRIDES.get(name)
    if override:
        return override

    name = name.replace("_", " ")
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", name)
    name = re.sub(r"\s+", " ", name).strip()
    if not name:
        return "Describe the function."
    return "{}.".format(name[0].upper() + name[1:])


def normalize_single_line_doxygen_block(block_lines: List[str], indent: str) -> List[str]:
    if len(block_lines) != 1:
        return block_lines

    stripped = block_lines[0].strip()
    if not (stripped.startswith("/**") and stripped.endswith("*/")):
        return block_lines

    inner = stripped[3:-2].strip()
    if inner.startswith("*"):
        inner = inner[1:].strip()

    normalized = [f"{indent}/**\n"]
    if inner:
        normalized.append(f"{indent} * {inner}\n")
    normalized.append(f"{indent} */\n")
    return normalized


PLACEHOLDER_COMMENT_PATTERNS = (
    r"\bplaceholder\b\s+\b(implementation|heuristic|path|paths|fallback|logic|code|mode)\b",
    r"\b(tbd|todo|stub|mock|simulation)\b",
    r"\bfor now\b",
    r"\bphase\s*2\.2\b",
    r"\bfuture implementation\b",
    r"\bwill be implemented\b",
    r"\bno-?op\b",
)


def is_placeholder_comment_text(text: str) -> bool:
    normalized = re.sub(r"\s+", " ", text).strip().lower()
    if not normalized:
        return False
    return any(re.search(pattern, normalized) for pattern in PLACEHOLDER_COMMENT_PATTERNS)


def is_section_heading_comment(lines: List[str], start_line: int, end_line: int) -> bool:
    segment = lines[start_line : end_line + 1]
    if not segment:
        return False

    raw_text = " ".join(
        re.sub(r"^\s*//+\s?", "", line).strip()
        for line in segment
    ).strip()
    if not raw_text:
        return False

    words = re.findall(r"[A-Za-z0-9_]+", raw_text)
    has_separator_line = any(
        re.fullmatch(r"[\s/=\-─_]*", re.sub(r"^\s*//+\s?", "", line).strip())
        for line in segment
    )
    if has_separator_line and len(words) <= 4 and not re.search(r"[.!?:]", raw_text):
        return True

    if len(segment) == 1 and len(words) <= 3 and not re.search(r"[.!?:]", raw_text) and raw_text[0].isupper():
        return True

    return False


def is_section_heading_doxygen_block(block_lines: List[str]) -> bool:
    if not block_lines:
        return False

    text = " ".join(
        re.sub(r"^\s*\*\s?", "", line).strip()
        for line in block_lines
    )
    text = re.sub(r"^/\*\*", "", text).replace("*/", "").strip()
    if not text:
        return False

    words = re.findall(r"[A-Za-z0-9_]+", text)
    if len(words) <= 10 and not re.search(r"[.!?]", text) and re.search(r"[=\-─_]{6,}", text):
        return True

    if len(words) <= 6 and not re.search(r"[.!?]", text) and any(token in text.lower() for token in ("core", "auth", "mfa", "verification", "configuration", "management", "checks")):
        return True

    return False


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
        brief = fallback_brief(name)
        details = ""

    lines = [
        f"{indent}/**\n",
        f"{indent} * @brief {brief}\n",
    ]

    for tparam in func.template_params:
        lines.append(f"{indent} * @tparam {tparam} Template parameter.\n")

    for p in func.params:
        lines.append(f"{indent} * @param{p.direction} {p.name} {describe_param(func.name, p.name, p.direction)}\n")

    if not is_void_return(func.return_type):
        lines.append(f"{indent} * @return {describe_return(func.return_type, func.name)}\n")

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
    normalized_block_lines = normalize_single_line_doxygen_block(block_lines, indent)
    text = "".join(normalized_block_lines)
    existing_params = set(re.findall(r"@param(?:\[[^\]]+\])?\s+([A-Za-z_]\w*)", text))
    existing_tparams = set(re.findall(r"@tparam\s+([A-Za-z_]\w*)", text))
    has_brief = re.search(r"@brief\b", text) is not None
    has_return = re.search(r"@return\b|@retval\b", text) is not None
    has_details = re.search(r"@details\b|@note\b|@remark\b", text) is not None
    has_throws = re.search(r"@throws\b|@exception\b", text) is not None
    has_noexcept_note = re.search(r"Exception safety:\s*noexcept", text, re.IGNORECASE) is not None
    brief_line_re = re.compile(r"^(\s*\*\s*@brief\s+)(.*?)(\s*)$")
    param_line_re = re.compile(r"^(\s*\*\s*@param(?:\[[^\]]+\])?\s+)([A-Za-z_]\w*)(\s+)(.*?)(\s*)$")
    return_line_re = re.compile(r"^(\s*\*\s*@return\s+)(.*?)(\s*)$")

    short_name = short_function_name(func.name)
    desired_brief = fallback_brief(func.name)
    desired_return = describe_return(func.return_type, func.name)

    upgraded_block_lines: List[str] = []
    for line in normalized_block_lines:
        newline = "\n" if line.endswith("\n") else ""
        content = line[:-1] if newline else line

        brief_match = brief_line_re.match(content)
        if brief_match:
            current_brief = brief_match.group(2).strip()
            if short_name in FUNCTION_BRIEF_OVERRIDES or normalize_doc_text(current_brief) == normalize_doc_text(desired_brief):
                content = f"{brief_match.group(1)}{desired_brief}{brief_match.group(3)}"

        param_match = param_line_re.match(content)
        if param_match:
            current_desc = param_match.group(4).strip()
            desired_desc = describe_param(func.name, param_match.group(2), "[in]")
            if normalize_doc_text(current_desc) in {"", "input parameter", "output parameter", "input/output parameter", "parameter"}:
                content = f"{param_match.group(1)}{param_match.group(2)}{param_match.group(3)}{desired_desc}{param_match.group(5)}"

        return_match = return_line_re.match(content)
        if return_match and not is_void_return(func.return_type):
            current_return = return_match.group(2).strip()
            if normalize_doc_text(current_return) in {"", "true on success", "return value", "pointer to the result"}:
                content = f"{return_match.group(1)}{desired_return}{return_match.group(3)}"

        upgraded_block_lines.append(content + newline)

    normalized_block_lines = upgraded_block_lines

    additions: List[str] = []

    if not has_brief:
        additions.append(f"{indent} * @brief {fallback_brief(short_name)}\n")

    for tparam in func.template_params:
        if tparam not in existing_tparams:
            additions.append(f"{indent} * @tparam {tparam} Template parameter.\n")

    for p in func.params:
        if p.name not in existing_params:
            additions.append(f"{indent} * @param{p.direction} {p.name} {describe_param(func.name, p.name, p.direction)}\n")

    if not is_void_return(func.return_type) and not has_return:
        additions.append(f"{indent} * @return {desired_return}\n")

    if func.has_body and not has_throws:
        if thrown:
            for exc, detail in zip(thrown, describe_throws(thrown)):
                additions.append(f"{indent} * @throws {exc} {detail}\n")

    if func.noexcept and not has_noexcept_note:
        additions.append(f"{indent} * @note Exception safety: noexcept.\n")

    if func.has_body and not has_details:
        additions.append(f"{indent} * @details {describe_details(called, short_name)}\n")

    if not additions:
        if upgraded_block_lines != block_lines:
            return upgraded_block_lines, True
        return block_lines, False

    end_idx = len(normalized_block_lines) - 1
    while end_idx >= 0 and "*/" not in normalized_block_lines[end_idx]:
        end_idx -= 1
    if end_idx < 0:
        return block_lines, False

    merged = normalized_block_lines[:end_idx] + additions + normalized_block_lines[end_idx:]
    return merged, True


def detect_indent(line: str) -> str:
    m = re.match(r"^(\s*)", line)
    return m.group(1) if m else ""


def apply_to_file(path: Path, apply: bool, merge_existing: bool) -> tuple[int, int, int, int]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines(keepends=True)
    lines, removed_internal_comment_lines = remove_internal_doxygen_comments(lines)
    functions = find_functions(lines)

    if not functions:
        return 0, 0, 0, 0

    edits: List[Tuple[int, int, List[str]]] = []
    removed_comment_lines = removed_internal_comment_lines
    merged_blocks = 0

    for func in functions:
        indent = detect_indent(lines[func.start_line])
        body = find_function_body_block(lines, func)
        called = extract_called_functions(body, func.name)
        thrown = extract_thrown_exceptions(body)

        multiline_triple_slash = collect_multiline_triple_slash_comment(lines, func.start_line)
        if multiline_triple_slash:
            existing_comment = None if is_placeholder_comment_text(multiline_triple_slash.text) else multiline_triple_slash
            block = build_doxygen_block(indent, func, called, thrown, existing_comment)
            edits.append((multiline_triple_slash.start_line, multiline_triple_slash.end_line, block))
            continue

        single_triple_slash = collect_single_line_triple_slash_comment(lines, func.start_line)
        if single_triple_slash:
            if is_placeholder_comment_text(single_triple_slash.text):
                block = build_doxygen_block(indent, func, called, thrown, None)
                edits.append((single_triple_slash.start_line, single_triple_slash.end_line, block))
            continue

        existing_doxy = find_immediate_doxygen_block(lines, func.start_line)
        if existing_doxy:
            start, end = existing_doxy
            block_text = "".join(lines[start : end + 1])
            if is_placeholder_comment_text(block_text) or is_section_heading_doxygen_block(lines[start : end + 1]):
                merged_block = build_doxygen_block(indent, func, called, thrown, None)
                edits.append((start, end, merged_block))
                continue
            if not merge_existing:
                continue
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
        if existing_comment and is_placeholder_comment_text(existing_comment.text):
            existing_comment = None
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
