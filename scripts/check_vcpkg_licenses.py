#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from collections import deque
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


DEFAULT_OUTPUT_DIR = Path("artifacts/license-compliance")
DEFAULT_PLATFORM_TAGS = {"linux", "x64"}


@dataclass
class Policy:
    allowed: set[str]
    restricted: dict[str, dict[str, Any]]
    unknown_action: str
    unknown_reason: str
    exceptions: list[dict[str, Any]]
    version: str


@dataclass
class DependencyRequest:
    name: str
    features: set[str] = field(default_factory=set)
    default_features: bool = True
    requested_by: set[str] = field(default_factory=set)
    direct: bool = False


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_policy(path: Path) -> Policy:
    data = load_json(path)
    policy = data["policy"]
    restricted: dict[str, dict[str, Any]] = {}
    for category, config in policy["restricted"].items():
        for license_id in config.get("licenses", []):
            restricted[license_id] = {
                "category": category,
                "action": config.get("action", "warn"),
                "reason": config.get("reason", ""),
            }
    return Policy(
        allowed=set(policy.get("allowed", [])),
        restricted=restricted,
        unknown_action=policy.get("unknown", {}).get("action", "warn"),
        unknown_reason=policy.get("unknown", {}).get("reason", "Manual review required"),
        exceptions=data.get("exceptions", {}).get("list", []),
        version=data.get("version", "unknown"),
    )


def normalize_feature_names(raw_features: Any) -> set[str]:
    names: set[str] = set()
    for feature in raw_features or []:
        if isinstance(feature, str):
            names.add(feature)
    return names


def tokenize_platform_expression(expression: str) -> list[str]:
    tokens: list[str] = []
    current: list[str] = []
    operators = {"(", ")", "!", "&", "|"}
    for char in expression:
        if char.isspace():
            if current:
                tokens.append("".join(current))
                current = []
            continue
        if char in operators:
            if current:
                tokens.append("".join(current))
                current = []
            tokens.append(char)
            continue
        current.append(char)
    if current:
        tokens.append("".join(current))
    return tokens


def evaluate_platform_expression(expression: str | None, platform_tags: set[str]) -> bool:
    if not expression:
        return True

    tokens = tokenize_platform_expression(expression)
    index = 0

    def parse_or() -> bool:
        nonlocal index
        value = parse_and()
        while index < len(tokens) and tokens[index] == "|":
            index += 1
            value = value or parse_and()
        return value

    def parse_and() -> bool:
        nonlocal index
        value = parse_unary()
        while index < len(tokens) and tokens[index] == "&":
            index += 1
            value = value and parse_unary()
        return value

    def parse_unary() -> bool:
        nonlocal index
        if index >= len(tokens):
            raise ValueError(f"Unexpected end of platform expression: {expression!r}")
        token = tokens[index]
        if token == "!":
            index += 1
            return not parse_unary()
        if token == "(":
            index += 1
            value = parse_or()
            if index < len(tokens) and tokens[index] == ")":
                index += 1
            return value
        index += 1
        return token in platform_tags

    result = parse_or()
    return result


def split_top_level(expression: str, operator: str) -> list[str]:
    depth = 0
    parts: list[str] = []
    current: list[str] = []
    index = 0
    token = f" {operator} "

    while index < len(expression):
        char = expression[index]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1

        if depth == 0 and expression.startswith(token, index):
            parts.append("".join(current).strip())
            current = []
            index += len(token)
            continue

        current.append(char)
        index += 1

    parts.append("".join(current).strip())
    return [part for part in parts if part]


def strip_wrapping_parentheses(expression: str) -> str:
    expr = expression.strip()
    while expr.startswith("(") and expr.endswith(")"):
        depth = 0
        balanced = True
        for index, char in enumerate(expr):
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            if depth == 0 and index != len(expr) - 1:
                balanced = False
                break
        if not balanced:
            break
        expr = expr[1:-1].strip()
    return expr


def merge_status(left: tuple[str, str], right: tuple[str, str]) -> tuple[str, str]:
    priority = {"allow": 0, "warn": 1, "block": 2}
    return right if priority[right[0]] > priority[left[0]] else left


def classify_license_atom(license_id: str, policy: Policy) -> tuple[str, str]:
    if license_id in policy.allowed:
        return "allow", f"{license_id} is on the allowed SPDX list"
    restricted = policy.restricted.get(license_id)
    if restricted:
        return restricted["action"], restricted["reason"] or f"{license_id} is restricted"
    return policy.unknown_action, policy.unknown_reason or f"{license_id} is not listed in policy"


def evaluate_license_expression(expression: str | None, policy: Policy) -> tuple[str, str]:
    if not expression:
        return policy.unknown_action, "Port manifest does not declare a license"

    expr = strip_wrapping_parentheses(" ".join(expression.strip().split()))
    or_parts = split_top_level(expr, "OR")
    if len(or_parts) > 1:
        evaluated = [evaluate_license_expression(part, policy) for part in or_parts]
        best = min(evaluated, key=lambda item: {"allow": 0, "warn": 1, "block": 2}[item[0]])
        return best

    and_parts = split_top_level(expr, "AND")
    if len(and_parts) > 1:
        result = ("allow", "All AND terms are allowed")
        for part in and_parts:
            result = merge_status(result, evaluate_license_expression(part, policy))
        return result

    with_parts = split_top_level(expr, "WITH")
    if len(with_parts) > 1:
        return evaluate_license_expression(with_parts[0], policy)

    return classify_license_atom(expr, policy)


def apply_package_exception(
    package_name: str,
    license_expression: str | None,
    action: str,
    reason: str,
    policy: Policy,
) -> tuple[str, str, dict[str, Any] | None]:
    for exception in policy.exceptions:
        if exception.get("package") != package_name:
            continue
        expected_license = exception.get("license")
        if expected_license and expected_license != license_expression:
            continue
        exception_action = exception.get("action", "warn")
        justification = exception.get("justification", "Approved package-specific policy exception")
        approved_by = exception.get("approver")
        approval_date = exception.get("approval_date")
        metadata = [f"original action: {action}", justification]
        if approved_by:
            metadata.append(f"approver: {approved_by}")
        if approval_date:
            metadata.append(f"approval date: {approval_date}")
        return (
            exception_action,
            "Policy exception applied (" + "; ".join(metadata) + ")",
            exception,
        )
    return action, reason, None


def parse_dependency_entry(
    entry: str | dict[str, Any],
    *,
    requested_by: str,
    direct: bool,
    platform_tags: set[str],
    include_host_deps: bool,
) -> DependencyRequest | None:
    if isinstance(entry, str):
        return DependencyRequest(name=entry, requested_by={requested_by}, direct=direct)
    if entry.get("host") and not include_host_deps:
        return None
    if not evaluate_platform_expression(entry.get("platform"), platform_tags):
        return None
    return DependencyRequest(
        name=entry["name"],
        features=normalize_feature_names(entry.get("features", [])),
        default_features=entry.get("default-features", True) is not False,
        requested_by={requested_by},
        direct=direct,
    )


def collect_root_requests(
    manifest: dict[str, Any],
    *,
    platform_tags: set[str],
    include_host_deps: bool,
) -> list[DependencyRequest]:
    requests = []
    for dep in manifest.get("dependencies", []):
        request = parse_dependency_entry(
            dep,
            requested_by="root dependency",
            direct=True,
            platform_tags=platform_tags,
            include_host_deps=include_host_deps,
        )
        if request is not None:
            requests.append(request)

    for feature_name, config in manifest.get("features", {}).items():
        for dep in config.get("dependencies", []):
            request = parse_dependency_entry(
                dep,
                requested_by=f"root feature:{feature_name}",
                direct=True,
                platform_tags=platform_tags,
                include_host_deps=include_host_deps,
            )
            if request is not None:
                requests.append(request)
    return requests


def resolve_manifest_path(ports_dir: Path | None, port_name: str) -> Path | None:
    if ports_dir is None:
        return None
    manifest_path = ports_dir / port_name / "vcpkg.json"
    return manifest_path if manifest_path.exists() else None


def fetch_port_manifest(
    port_name: str,
    *,
    baseline: str,
    ports_dir: Path | None,
    cache: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    if port_name in cache:
        return cache[port_name]

    local_manifest = resolve_manifest_path(ports_dir, port_name)
    if local_manifest is not None:
        data = load_json(local_manifest)
        cache[port_name] = data
        return data

    url = f"https://raw.githubusercontent.com/microsoft/vcpkg/{baseline}/ports/{port_name}/vcpkg.json"
    request = Request(url, headers={"User-Agent": "ThemisDB-License-Compliance/1.0"})
    try:
        with urlopen(request, timeout=30) as response:
            data = json.load(response)
    except HTTPError as exc:
        if exc.code == 404:
            print(
                f"[WARNING] Port '{port_name}' not found in vcpkg registry at baseline {baseline} "
                f"(HTTP 404) — treating as unlicensed (no dependencies).",
                flush=True,
            )
            data: dict[str, Any] = {"name": port_name}
            cache[port_name] = data
            return data
        raise RuntimeError(f"Unable to fetch port manifest for {port_name}: HTTP {exc.code}") from exc
    except URLError as exc:
        raise RuntimeError(f"Unable to fetch port manifest for {port_name}: {exc.reason}") from exc

    cache[port_name] = data
    return data


def iter_enabled_dependencies(
    manifest: dict[str, Any],
    request: DependencyRequest,
    *,
    platform_tags: set[str],
    include_host_deps: bool,
) -> list[DependencyRequest]:
    dependencies: list[DependencyRequest] = []

    for dep in manifest.get("dependencies", []):
        parsed = parse_dependency_entry(
            dep,
            requested_by=manifest["name"],
            direct=False,
            platform_tags=platform_tags,
            include_host_deps=include_host_deps,
        )
        if parsed is not None:
            dependencies.append(parsed)

    enabled_features = set(request.features)
    if request.default_features:
        enabled_features.update(normalize_feature_names(manifest.get("default-features", [])))

    feature_map = manifest.get("features", {})
    for feature_name in sorted(enabled_features):
        feature_config = feature_map.get(feature_name, {})
        for dep in feature_config.get("dependencies", []):
            parsed = parse_dependency_entry(
                dep,
                requested_by=f"{manifest['name']} feature:{feature_name}",
                direct=False,
                platform_tags=platform_tags,
                include_host_deps=include_host_deps,
            )
            if parsed is not None:
                dependencies.append(parsed)

    return dependencies


def walk_dependency_graph(
    root_manifest: dict[str, Any],
    *,
    baseline: str,
    ports_dir: Path | None,
    platform_tags: set[str],
    include_host_deps: bool,
) -> dict[str, DependencyRequest]:
    requests = collect_root_requests(
        root_manifest,
        platform_tags=platform_tags,
        include_host_deps=include_host_deps,
    )
    queue: deque[DependencyRequest] = deque(requests)
    seen: dict[str, DependencyRequest] = {}
    cache: dict[str, dict[str, Any]] = {}

    while queue:
        current = queue.popleft()
        existing = seen.get(current.name)
        if existing is None:
            seen[current.name] = current
            effective = current
        else:
            changed = False
            new_features = current.features - existing.features
            if new_features:
                existing.features.update(new_features)
                changed = True
            if current.default_features and not existing.default_features:
                existing.default_features = True
                changed = True
            if not current.requested_by.issubset(existing.requested_by):
                existing.requested_by.update(current.requested_by)
                changed = True
            if current.direct and not existing.direct:
                existing.direct = True
                changed = True
            if not changed:
                continue
            effective = existing

        manifest = fetch_port_manifest(
            effective.name,
            baseline=baseline,
            ports_dir=ports_dir,
            cache=cache,
        )
        for child in iter_enabled_dependencies(
            manifest,
            effective,
            platform_tags=platform_tags,
            include_host_deps=include_host_deps,
        ):
            queue.append(child)

    return seen


def build_package_records(
    root_manifest: dict[str, Any],
    policy: Policy,
    *,
    baseline: str,
    ports_dir: Path | None,
    platform_tags: set[str],
    include_host_deps: bool,
) -> list[dict[str, Any]]:
    cache: dict[str, dict[str, Any]] = {}
    graph = walk_dependency_graph(
        root_manifest,
        baseline=baseline,
        ports_dir=ports_dir,
        platform_tags=platform_tags,
        include_host_deps=include_host_deps,
    )
    packages: list[dict[str, Any]] = []

    for name in sorted(graph):
        request = graph[name]
        manifest = fetch_port_manifest(name, baseline=baseline, ports_dir=ports_dir, cache=cache)
        action, reason = evaluate_license_expression(manifest.get("license"), policy)
        action, reason, exception = apply_package_exception(
            name,
            manifest.get("license"),
            action,
            reason,
            policy,
        )
        packages.append(
            {
                "name": name,
                "version": manifest.get("version"),
                "port_version": manifest.get("port-version"),
                "license_expression": manifest.get("license"),
                "action": action,
                "reason": reason,
                "direct": request.direct,
                "requested_by": sorted(request.requested_by),
                "selected_features": sorted(request.features),
                "default_features_enabled": request.default_features,
                "supports": manifest.get("supports"),
                "homepage": manifest.get("homepage"),
                "policy_exception": exception,
            }
        )

    return packages


def summarize(packages: list[dict[str, Any]]) -> dict[str, int]:
    summary = {
        "total": len(packages),
        "direct": sum(1 for pkg in packages if pkg["direct"]),
        "transitive": sum(1 for pkg in packages if not pkg["direct"]),
        "allow": sum(1 for pkg in packages if pkg["action"] == "allow"),
        "warn": sum(1 for pkg in packages if pkg["action"] == "warn"),
        "block": sum(1 for pkg in packages if pkg["action"] == "block"),
    }
    return summary


def write_reports(
    *,
    output_dir: Path,
    manifest_path: Path,
    baseline: str,
    policy: Policy,
    packages: list[dict[str, Any]],
) -> dict[str, Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    summary = summarize(packages)
    generated_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat()

    json_path = output_dir / "vcpkg-license-sbom.json"
    markdown_path = output_dir / "license-summary.md"

    json_path.write_text(
        json.dumps(
            {
                "generated_at": generated_at,
                "manifest": str(manifest_path),
                "builtin_baseline": baseline,
                "policy_version": policy.version,
                "summary": summary,
                "packages": packages,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    blocked = [pkg for pkg in packages if pkg["action"] == "block"]
    warnings = [pkg for pkg in packages if pkg["action"] == "warn"]
    lines = [
        "# vcpkg License Compliance Summary",
        "",
        f"- Generated: `{generated_at}`",
        f"- Manifest: `{manifest_path}`",
        f"- Builtin baseline: `{baseline}`",
        f"- Policy version: `{policy.version}`",
        "",
        "## Totals",
        "",
        f"- Packages scanned: **{summary['total']}**",
        f"- Direct packages: **{summary['direct']}**",
        f"- Transitive packages: **{summary['transitive']}**",
        f"- Allowed: **{summary['allow']}**",
        f"- Warning: **{summary['warn']}**",
        f"- Blocked: **{summary['block']}**",
        "",
        "## Review Rule",
        "",
        "- Pull requests must not merge while this workflow is failing.",
        "- Warning entries require human review and explicit justification in the PR.",
        "",
    ]

    if blocked:
        lines.extend(["## Blocked", ""])
        for pkg in blocked:
            lines.append(
                f"- `{pkg['name']}` — `{pkg.get('license_expression') or 'UNKNOWN'}` ({pkg['reason']})"
            )
        lines.append("")

    if warnings:
        lines.extend(["## Warnings", ""])
        for pkg in warnings:
            lines.append(
                f"- `{pkg['name']}` — `{pkg.get('license_expression') or 'UNKNOWN'}` ({pkg['reason']})"
            )
        lines.append("")

    markdown_path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    return {"json": json_path, "markdown": markdown_path}


def detect_default_ports_dir(manifest_path: Path) -> Path | None:
    candidate = manifest_path.parent / "vcpkg" / "ports"
    return candidate if candidate.exists() else None


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit vcpkg dependency licenses against .license-policy.json")
    parser.add_argument("--manifest", default="vcpkg.json", help="Path to the root vcpkg manifest")
    parser.add_argument("--policy", default=".license-policy.json", help="Path to the license policy file")
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR), help="Directory for generated reports")
    parser.add_argument("--baseline", help="Override the vcpkg builtin-baseline SHA")
    parser.add_argument("--ports-dir", help="Local vcpkg ports directory (avoids remote fetches)")
    parser.add_argument(
        "--platform-tags",
        default="linux,x64",
        help="Comma-separated vcpkg platform tags used to resolve conditional dependencies",
    )
    parser.add_argument(
        "--include-host-deps",
        action="store_true",
        help="Include host/build helper dependencies in the audit graph",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    manifest_path = Path(args.manifest).resolve()
    policy_path = Path(args.policy).resolve()
    output_dir = Path(args.output_dir).resolve()

    manifest = load_json(manifest_path)
    policy = load_policy(policy_path)
    baseline = args.baseline or manifest.get("builtin-baseline")
    if not baseline:
        raise SystemExit("vcpkg.json does not define builtin-baseline and --baseline was not provided")

    ports_dir = Path(args.ports_dir).resolve() if args.ports_dir else detect_default_ports_dir(manifest_path)
    platform_tags = {tag.strip() for tag in args.platform_tags.split(",") if tag.strip()} or set(DEFAULT_PLATFORM_TAGS)
    packages = build_package_records(
        manifest,
        policy,
        baseline=baseline,
        ports_dir=ports_dir,
        platform_tags=platform_tags,
        include_host_deps=args.include_host_deps,
    )
    report_paths = write_reports(
        output_dir=output_dir,
        manifest_path=manifest_path,
        baseline=baseline,
        policy=policy,
        packages=packages,
    )
    summary = summarize(packages)

    print(f"Scanned {summary['total']} packages from {manifest_path.name}")
    print(f"Allowed: {summary['allow']} | Warnings: {summary['warn']} | Blocked: {summary['block']}")
    print(f"JSON report: {report_paths['json']}")
    print(f"Markdown summary: {report_paths['markdown']}")

    return 1 if summary["block"] else 0


if __name__ == "__main__":
    sys.exit(main())
