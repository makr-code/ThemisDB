"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            secret_scan.py                                     ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 06:59:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     413                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 278d616814  2026-03-01  feat(security): secret scanning pre-commit hook for CI pi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
secret-scan.py – High-entropy string and secret pattern scanner for ThemisDB.

Scans staged (or explicitly listed) files for:
  1. High-entropy tokens (Shannon entropy ≥ 4.5 bits/char, minimum token
     length 20 characters) — threshold consistent with truffleHog and
     detect-secrets defaults.
  2. Known secret patterns (API keys, JWT secrets, private keys, etc.) via
     regular expressions aligned with the existing .gitleaks.toml rules.

Usage (pre-commit hook):
    python3 scripts/secret_scan.py [file1 file2 ...]

Usage (standalone / scan all tracked files):
    python3 scripts/secret_scan.py --all

Exit code:
    0  – no findings
    1  – one or more findings (scan failure); CI must treat this as a block
    2  – configuration / runtime error

Allow-list:
    Add patterns to .secret-scan-allowlist.txt (one regex per line, lines
    starting with '#' are comments).  Any finding whose matched string or the
    full line matches an allowlist pattern is suppressed.
"""

import argparse
import math
import re
import sys
from pathlib import Path
from typing import List, NamedTuple, Optional

# ---------------------------------------------------------------------------
# Configuration constants
# ---------------------------------------------------------------------------

ENTROPY_THRESHOLD: float = 4.5     # bits per character (Shannon entropy)
MIN_TOKEN_LENGTH: int = 20          # minimum token length to trigger entropy check
MAX_LINE_LENGTH: int = 2000         # skip very long lines (minified/binary)

# File extensions to skip (binary / generated / lock files).
SKIP_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".gif", ".ico", ".svg", ".woff", ".woff2",
    ".ttf", ".eot", ".pdf", ".zip", ".tar", ".gz", ".bz2", ".xz",
    ".exe", ".dll", ".so", ".dylib", ".a", ".lib",
    ".lock", ".sum", ".patch", ".diff",
    ".pb", ".bin", ".dat",
}

# Directories to skip entirely.
SKIP_DIRS = {
    ".git", "build", "_build", "dist", "node_modules",
    "vcpkg", "vcpkg_installed", "llama.cpp", "third_party", "external",
    ".venv", "__pycache__",
}

ALLOWLIST_FILE = Path(".secret-scan-allowlist.txt")

# ---------------------------------------------------------------------------
# Secret patterns (regex → description)
# Each tuple: (compiled_re, human-readable description, severity)
# ---------------------------------------------------------------------------

SECRET_PATTERNS = [
    # Generic high-risk patterns
    (re.compile(r'-----BEGIN\s+(RSA\s+)?PRIVATE KEY-----'), "PEM private key", "critical"),
    (re.compile(r'-----BEGIN\s+EC\s+PRIVATE KEY-----'), "EC PEM private key", "critical"),
    (re.compile(r'-----BEGIN\s+OPENSSH\s+PRIVATE KEY-----'), "OpenSSH private key", "critical"),

    # AWS
    (re.compile(r'AKIA[0-9A-Z]{16}'), "AWS Access Key ID", "critical"),
    (re.compile(
        r'(?i)aws[-_]?secret[-_]?access[-_]?key\s*[:=]\s*[\'"]?([A-Za-z0-9/+=]{40})[\'"]?'
    ), "AWS Secret Access Key", "critical"),

    # GitHub tokens
    (re.compile(r'ghp_[a-zA-Z0-9]{36}'), "GitHub Personal Access Token", "critical"),
    (re.compile(r'gho_[a-zA-Z0-9]{36}'), "GitHub OAuth Token", "critical"),
    (re.compile(r'ghs_[a-zA-Z0-9]{36}'), "GitHub App Token", "critical"),
    (re.compile(r'github_pat_[a-zA-Z0-9_]{82}'), "GitHub Fine-Grained PAT", "critical"),

    # JWT secrets
    (re.compile(
        r'(?i)jwt[-_]?secret\s*[:=]\s*[\'"]?([A-Za-z0-9+/]{32,})[\'"]?'
    ), "JWT secret key", "high"),

    # ThemisDB-specific tokens
    (re.compile(
        r'(?i)(themis|vccdb)[-_]?(api)?[-_]?key\s*[:=]\s*[\'"]?([a-zA-Z0-9]{32,})[\'"]?'
    ), "ThemisDB API Key", "high"),
    (re.compile(
        r'(?i)(admin|root)[-_]?token\s*[:=]\s*[\'"]?([a-zA-Z0-9]{40,})[\'"]?'
    ), "ThemisDB Admin Token", "high"),

    # Database connection strings with embedded credentials
    (re.compile(
        r'(?i)(mongodb|postgres|postgresql|mysql|redis)://[a-zA-Z0-9._%-]+:[^@\s]{3,}@'
    ), "Database connection string with credentials", "high"),

    # Encryption keys
    (re.compile(
        r'(?i)(encryption|cipher)[-_]?key\s*[:=]\s*[\'"]?([a-fA-F0-9]{32,})[\'"]?'
    ), "Encryption key", "high"),

    # Generic passwords (long, non-trivial values)
    (re.compile(
        r'(?i)\b(password|passwd|pwd)\s*[:=]\s*[\'"][^\'"]{8,}[\'"]'
    ), "Hard-coded password", "medium"),

    # Generic API keys
    (re.compile(
        r'(?i)\bapi[-_]?key\s*[:=]\s*[\'"]?([a-zA-Z0-9]{20,})[\'"]?'
    ), "Generic API key", "medium"),

    # Slack tokens
    (re.compile(r'xox[baprs]-[0-9]{10,13}-[a-zA-Z0-9-]{24,}'), "Slack token", "high"),

    # SendGrid / Twilio / Stripe
    (re.compile(r'SG\.[a-zA-Z0-9._-]{22,}\.[a-zA-Z0-9._-]{43,}'), "SendGrid API key", "high"),
    (re.compile(r'SK[a-fA-F0-9]{32}'), "Twilio Account SID", "medium"),
    (re.compile(r'sk_(live|test)_[a-zA-Z0-9]{24,}'), "Stripe secret key", "critical"),
]

# Patterns applied to the FULL LINE to determine false positives (context-based).
FP_LINE_RES = [
    re.compile(r'^\s*#'),                    # shell/Python comment lines
    re.compile(r'^\s*//'),                   # C++ comment lines
    re.compile(r'^\s*\*'),                   # doc-comment continuation lines
    re.compile(r'(?i)\$\{[^}]+\}'),         # template variables like ${SECRET}
    re.compile(r'(?i)<[A-Z_][A-Z0-9_]+>'),  # placeholders like <API_KEY>
]

# Patterns applied to the MATCHED VALUE only (value-based false positives).
FP_VALUE_RES = [
    re.compile(r'(?i)(example|sample|demo|placeholder|dummy|changeme|password123|yourpassword)'),
    re.compile(r'^x{8,}$', re.IGNORECASE),  # xxxxxxxx placeholders
    re.compile(r'^0{8,}$'),                  # 00000000 placeholders
    re.compile(r'^1{8,}$'),                  # 11111111 placeholders
    re.compile(r'^[0-9]{8}$'),               # exactly 8 sequential digits (weak password)
    re.compile(r'00000000-0000-0000-0000-000000000000'),  # UUID placeholder
    re.compile(r'(?i)xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'),  # UUID placeholder
]


# ---------------------------------------------------------------------------
# Finding dataclass
# ---------------------------------------------------------------------------

class Finding(NamedTuple):
    path: str
    line_no: int
    description: str
    severity: str
    snippet: str       # truncated, safe to print


# ---------------------------------------------------------------------------
# Entropy helpers
# ---------------------------------------------------------------------------

def shannon_entropy(data: str) -> float:
    """Return the Shannon entropy (bits per character) of *data*."""
    if not data:
        return 0.0
    freq: dict[str, int] = {}
    for ch in data:
        freq[ch] = freq.get(ch, 0) + 1
    total = len(data)
    entropy = 0.0
    for count in freq.values():
        p = count / total
        entropy -= p * math.log2(p)
    return entropy


# Token splitter: extract candidate high-entropy tokens from a line.
_TOKEN_RE = re.compile(r'[A-Za-z0-9+/=_\-]{20,}')


def high_entropy_tokens(line: str, threshold: float = ENTROPY_THRESHOLD) -> List[str]:
    """Return tokens from *line* whose Shannon entropy exceeds *threshold*."""
    results = []
    for token in _TOKEN_RE.findall(line):
        if len(token) >= MIN_TOKEN_LENGTH and shannon_entropy(token) >= threshold:
            results.append(token)
    return results


# ---------------------------------------------------------------------------
# Allow-list loader
# ---------------------------------------------------------------------------

def load_allowlist(path: Path = ALLOWLIST_FILE) -> List[re.Pattern]:
    """Load user-defined allow-list patterns from *path*."""
    patterns: List[re.Pattern] = []
    if not path.is_file():
        return patterns
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        stripped = raw_line.strip()
        if stripped and not stripped.startswith("#"):
            try:
                patterns.append(re.compile(stripped))
            except re.error as exc:
                print(f"[warn] Invalid allowlist pattern '{stripped}': {exc}", file=sys.stderr)
    return patterns


# ---------------------------------------------------------------------------
# Core scanner
# ---------------------------------------------------------------------------

def _is_false_positive(line: str, matched: str, allowlist: List[re.Pattern]) -> bool:
    """Return True if the finding should be suppressed.

    Context-based patterns (comment markers, template syntax) are checked
    against the full *line*.  Value-based patterns (placeholder words,
    sequential digits) are checked only against the *matched* token to avoid
    suppressing real secrets that happen to appear next to words like
    "example.com" in a hostname.
    """
    # Line-context checks
    for pat in FP_LINE_RES:
        if pat.search(line):
            return True
    # Matched-value checks
    for pat in FP_VALUE_RES:
        if pat.search(matched):
            return True
    # User-defined allow-list (checked against both for maximum flexibility)
    for pat in allowlist:
        if pat.search(line) or pat.search(matched):
            return True
    return False


def scan_file(
    path: Path,
    allowlist: List[re.Pattern],
    entropy_threshold: float = ENTROPY_THRESHOLD,
) -> List[Finding]:
    """Scan a single file and return all findings."""
    findings: List[Finding] = []

    # Skip binary / irrelevant extensions
    if path.suffix.lower() in SKIP_EXTENSIONS:
        return findings

    # Skip paths inside excluded directories
    parts = set(path.parts)
    if parts & SKIP_DIRS:
        return findings

    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return findings

    for line_no, line in enumerate(text.splitlines(), start=1):
        if len(line) > MAX_LINE_LENGTH:
            continue

        # 1. Known secret pattern check
        for pattern, description, severity in SECRET_PATTERNS:
            m = pattern.search(line)
            if m:
                matched = m.group(0)
                if not _is_false_positive(line, matched, allowlist):
                    snippet = line.strip()[:120]
                    findings.append(Finding(
                        path=str(path),
                        line_no=line_no,
                        description=description,
                        severity=severity,
                        snippet=snippet,
                    ))
                    break  # one finding per line per pattern type

        # 2. High-entropy token check
        for token in high_entropy_tokens(line, threshold=entropy_threshold):
            if not _is_false_positive(line, token, allowlist):
                snippet = line.strip()[:120]
                findings.append(Finding(
                    path=str(path),
                    line_no=line_no,
                    description=f"High-entropy token (entropy={shannon_entropy(token):.2f})",
                    severity="medium",
                    snippet=snippet,
                ))
                break  # one entropy finding per line

    return findings


# ---------------------------------------------------------------------------
# Output formatting
# ---------------------------------------------------------------------------

SEVERITY_ICON = {"critical": "🔴", "high": "🟠", "medium": "🟡", "low": "🔵"}


def print_findings(findings: List[Finding]) -> None:
    for f in findings:
        icon = SEVERITY_ICON.get(f.severity, "⚪")
        print(
            f"{icon} [{f.severity.upper()}] {f.path}:{f.line_no} – {f.description}\n"
            f"   {f.snippet}"
        )


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Secret scanner: high-entropy strings and known secret patterns."
    )
    parser.add_argument(
        "files",
        nargs="*",
        help="Files to scan (pre-commit passes staged files as positional arguments).",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        dest="scan_all",
        help="Scan all tracked/untracked non-ignored files in the repository.",
    )
    parser.add_argument(
        "--allowlist",
        default=str(ALLOWLIST_FILE),
        help=f"Path to the allow-list file (default: {ALLOWLIST_FILE}).",
    )
    parser.add_argument(
        "--entropy-threshold",
        type=float,
        default=ENTROPY_THRESHOLD,
        help=f"Shannon entropy threshold (default: {ENTROPY_THRESHOLD}).",
    )
    args = parser.parse_args(argv)

    threshold = args.entropy_threshold
    allowlist = load_allowlist(Path(args.allowlist))

    # Determine the file list
    if args.scan_all:
        import subprocess
        try:
            result = subprocess.run(
                ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
                capture_output=True,
                text=True,
                check=True,
            )
            file_list = [Path(p) for p in result.stdout.splitlines() if p]
        except subprocess.CalledProcessError as exc:
            print(f"[error] git ls-files failed: {exc}", file=sys.stderr)
            return 2
    else:
        file_list = [Path(f) for f in args.files]

    if not file_list:
        return 0

    all_findings: List[Finding] = []
    for path in file_list:
        if path.is_file():
            all_findings.extend(scan_file(path, allowlist, entropy_threshold=threshold))

    if all_findings:
        print(
            f"\n🔐 Secret scanner found {len(all_findings)} potential secret(s):\n",
            file=sys.stderr,
        )
        print_findings(all_findings)
        print(
            "\nTo suppress false positives, add a matching regex to "
            ".secret-scan-allowlist.txt\n",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
