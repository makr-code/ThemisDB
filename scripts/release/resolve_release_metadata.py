#!/usr/bin/env python3
"""Resolve canonical release and package metadata from GitHub and the repository.

Shared by the release/deployment workflows (WinGet, Windows and Linux distribution,
Docker image) so that version, release assets and package metadata (project URL,
description, license, publisher, release notes) are derived from one source instead
of being hardcoded per workflow.

Data sources, in precedence order:
  * GitHub repository metadata (``gh api repos/<repo>``): URL, description, homepage,
    license SPDX id, topics, default branch.
  * Repository files: ``LICENSE`` (license name when GitHub reports NOASSERTION,
    copyright holder) and ``VERSION`` (canonical version for repository-only mode).
  * GitHub releases (``gh api repos/<repo>/releases``): tag, prerelease flag,
    publication state, release notes and asset names/URLs/digests.

Required values (repository URL, description, license, published release, required
assets) fail with an ``::error::`` annotation and exit code 1. Optional values with
an invalid format (homepage) emit a ``::warning::`` and fall back to the repository
URL so that a misconfigured GitHub setting cannot leak into package manifests.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any
from urllib.parse import urlparse

VERSION_RE = re.compile(r"^(\d+\.\d+\.\d+)(?:-([0-9A-Za-z][0-9A-Za-z.-]*))?$")
COMMUNITY_TAG_RE = re.compile(r"^v(\d+\.\d+\.\d+(?:-[0-9A-Za-z][0-9A-Za-z.-]*)?)$")
REPO_RE = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
SHORT_DESCRIPTION_MAX = 256
RELEASE_NOTES_MAX = 10000
TAG_MAX = 40
TAGS_MAX = 16


class MetadataError(RuntimeError):
    """Raised for missing or invalid release metadata."""


def warn(message: str) -> None:
    print(f"::warning::{message}", file=sys.stderr)


def gh_api(path: str) -> Any:
    """Call ``gh api`` and return parsed JSON; raise MetadataError on failure."""
    try:
        result = subprocess.run(
            ["gh", "api", "-H", "Accept: application/vnd.github+json", path],
            check=False,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError as exc:
        raise MetadataError("GitHub CLI 'gh' is required to resolve release metadata.") from exc
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip().splitlines()
        reason = detail[-1] if detail else f"exit code {result.returncode}"
        if "404" in reason or "Not Found" in reason:
            raise MetadataError(f"GitHub API resource not found: {path}")
        raise MetadataError(f"GitHub API request failed for {path}: {reason}")
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise MetadataError(f"GitHub API returned invalid JSON for {path}.") from exc


def is_http_url(value: str | None) -> bool:
    if not value or any(ch.isspace() for ch in value) or "\\" in value:
        return False
    parsed = urlparse(value)
    return parsed.scheme in {"http", "https"} and bool(parsed.netloc) and "." in parsed.netloc


def normalize_version(raw: str) -> str:
    value = raw.strip()
    if value.startswith("v"):
        value = value[1:]
    if not VERSION_RE.match(value):
        raise MetadataError(
            f"Invalid release version '{raw}'. Expected MAJOR.MINOR.PATCH[-suffix] (optionally prefixed with 'v')."
        )
    return value


def read_license_file(repo_root: Path) -> tuple[str, str]:
    """Return (license title, copyright line) from the repository LICENSE file."""
    path = repo_root / "LICENSE"
    if not path.is_file():
        return "", ""
    lines = [line.strip() for line in path.read_text(encoding="utf-8", errors="replace").splitlines()]
    non_empty = [line for line in lines if line]
    title = non_empty[0] if non_empty else ""
    copyright_line = next((line for line in non_empty if line.lower().startswith("copyright")), "")
    return title, copyright_line


def copyright_holder(copyright_line: str) -> str:
    match = re.match(r"(?i)^copyright\s*(?:\(c\)|©)?\s*(?:\d{4}(?:\s*[-,]\s*\d{4})*)?\s*(.*)$", copyright_line)
    return match.group(1).strip().rstrip(".") if match else ""


def normalize_tags(topics: list[Any]) -> list[str]:
    tags: list[str] = []
    for topic in topics:
        if not isinstance(topic, str):
            continue
        tag = topic.strip().lower()
        if not tag or len(tag) > TAG_MAX or any(ch.isspace() for ch in tag) or tag in tags:
            continue
        tags.append(tag)
        if len(tags) == TAGS_MAX:
            break
    return tags


def resolve_repository(repo: str, repo_root: Path) -> dict[str, Any]:
    data = gh_api(f"repos/{repo}")
    if not isinstance(data, dict):
        raise MetadataError(f"Unexpected repository metadata for {repo}.")

    repository_url = data.get("html_url") or ""
    if not is_http_url(repository_url) or not repository_url.startswith("https://"):
        raise MetadataError(f"Repository {repo} has no valid https html_url in GitHub metadata.")
    repository_url = repository_url.rstrip("/")

    description = " ".join((data.get("description") or "").split())
    if not description:
        raise MetadataError(
            f"Repository {repo} has no GitHub description. Set it under repository settings → About."
        )
    short_description = description
    if len(short_description) > SHORT_DESCRIPTION_MAX:
        short_description = short_description[: SHORT_DESCRIPTION_MAX - 1].rstrip() + "…"
        warn(f"Repository description exceeds {SHORT_DESCRIPTION_MAX} characters; truncated for package metadata.")

    homepage_raw = (data.get("homepage") or "").strip()
    homepage = repository_url
    if homepage_raw:
        if is_http_url(homepage_raw):
            homepage = homepage_raw
        else:
            warn(
                f"GitHub homepage '{homepage_raw}' is not a valid http(s) URL; using {repository_url} instead. "
                "Fix it under repository settings → About."
            )

    license_title, copyright_line = read_license_file(repo_root)
    license_info = data.get("license") or {}
    spdx = (license_info.get("spdx_id") or "").strip() if isinstance(license_info, dict) else ""
    if spdx and spdx.upper() != "NOASSERTION":
        license_name = spdx
    elif license_title:
        license_name = license_title
    else:
        raise MetadataError(
            f"License for {repo} cannot be determined: GitHub reports '{spdx or 'none'}' and no LICENSE file exists."
        )

    default_branch = data.get("default_branch") or ""
    if not default_branch:
        raise MetadataError(f"Repository {repo} has no default branch in GitHub metadata.")

    owner = data.get("owner") or {}
    owner_login = owner.get("login", "") if isinstance(owner, dict) else ""
    publisher = copyright_holder(copyright_line) or owner_login
    if not publisher:
        raise MetadataError(f"Publisher for {repo} cannot be determined from LICENSE or repository owner.")

    return {
        "repository": repo,
        "repository_url": repository_url,
        "default_branch": default_branch,
        "description": short_description,
        "homepage": homepage,
        "license": license_name,
        "license_spdx": spdx if spdx and spdx.upper() != "NOASSERTION" else "",
        "publisher": publisher,
        "publisher_url": repository_url,
        "publisher_support_url": f"{repository_url}/issues",
        "copyright": copyright_line or f"Copyright (c) {publisher}",
        "tags": normalize_tags(data.get("topics") or []),
    }


def release_is_published(release: dict[str, Any]) -> bool:
    published_at = release.get("published_at")
    if release.get("draft") is not False or not isinstance(published_at, str):
        return False
    try:
        datetime.fromisoformat(published_at.replace("Z", "+00:00"))
    except ValueError:
        return False
    return True


def select_release(
    repo: str, version: str | None, allow_prerelease: bool, include_nightly: bool
) -> dict[str, Any]:
    if version:
        tag = f"v{version}"
        release = gh_api(f"repos/{repo}/releases/tags/{tag}")
        if not isinstance(release, dict):
            raise MetadataError(f"Unexpected release metadata for {tag}.")
        if not release_is_published(release):
            raise MetadataError(f"Release {tag} exists but is not published (draft or missing published_at).")
        return release

    releases = gh_api(f"repos/{repo}/releases?per_page=100")
    if not isinstance(releases, list):
        raise MetadataError(f"Unexpected release list for {repo}.")
    candidates = []
    for release in releases:
        if not isinstance(release, dict) or not release_is_published(release):
            continue
        tag = release.get("tag_name") or ""
        if not COMMUNITY_TAG_RE.match(tag):
            continue
        if "nightly" in tag and not include_nightly:
            continue
        if (release.get("prerelease") or "-" in tag) and not allow_prerelease:
            continue
        candidates.append(release)
    if not candidates:
        scope = "published release" if allow_prerelease else "published stable release"
        raise MetadataError(
            f"No {scope} with a canonical vX.Y.Z tag found for {repo}. "
            "Pass an explicit version or enable prereleases."
        )
    return max(candidates, key=lambda item: item.get("published_at") or "")


def resolve_release(
    repo: str,
    version: str | None,
    allow_prerelease: bool,
    include_nightly: bool,
    required_assets: list[tuple[str, re.Pattern[str]]],
) -> dict[str, Any]:
    release = select_release(repo, version, allow_prerelease, include_nightly)
    tag = release.get("tag_name") or ""
    match = COMMUNITY_TAG_RE.match(tag)
    if not match:
        raise MetadataError(f"Release tag '{tag}' is not a canonical community tag (vX.Y.Z[-suffix]).")
    resolved_version = match.group(1)
    suffix = VERSION_RE.match(resolved_version).group(2)  # type: ignore[union-attr]
    is_prerelease = bool(release.get("prerelease")) or bool(suffix)

    release_url = release.get("html_url") or ""
    if not is_http_url(release_url):
        raise MetadataError(f"Release {tag} has no valid html_url.")

    notes = (release.get("body") or "").replace("\r\n", "\n").strip()
    if not notes:
        warn(f"Release {tag} has no release notes body; package manifests will only link {release_url}.")
    elif len(notes) > RELEASE_NOTES_MAX:
        notes = notes[: RELEASE_NOTES_MAX - 80].rstrip() + f"\n\n… truncated; see {release_url}"
        warn(f"Release notes for {tag} exceed {RELEASE_NOTES_MAX} characters; truncated.")

    assets = []
    for asset in release.get("assets") or []:
        if not isinstance(asset, dict) or not asset.get("name"):
            continue
        digest = asset.get("digest") or ""
        assets.append(
            {
                "name": asset["name"],
                "url": asset.get("browser_download_url") or "",
                "size": int(asset.get("size") or 0),
                "sha256": digest.split(":", 1)[1] if digest.startswith("sha256:") else "",
            }
        )
    if not assets:
        raise MetadataError(f"Release {tag} has no uploaded assets.")

    matched: dict[str, str] = {}
    for key, pattern in required_assets:
        hits = [asset for asset in assets if pattern.search(asset["name"])]
        if len(hits) != 1:
            names = ", ".join(asset["name"] for asset in hits) or "none"
            raise MetadataError(
                f"Release {tag} must contain exactly one asset for '{key}' matching /{pattern.pattern}/; found {len(hits)} ({names})."
            )
        if hits[0]["size"] <= 0:
            raise MetadataError(f"Release asset {hits[0]['name']} for '{key}' is empty.")
        matched[key] = hits[0]["name"]

    return {
        "version": resolved_version,
        "tag": tag,
        "release_type": "prerelease" if is_prerelease else "stable",
        "is_prerelease": is_prerelease,
        "published_at": release.get("published_at") or "",
        "release_url": release_url,
        "release_notes": notes,
        "assets": assets,
        "matched_assets": matched,
    }


def read_repository_version(repo_root: Path) -> str:
    path = repo_root / "VERSION"
    if not path.is_file():
        raise MetadataError(f"VERSION file not found: {path}")
    return normalize_version(path.read_text(encoding="utf-8").strip())


def parse_asset_spec(spec: str) -> tuple[str, re.Pattern[str]]:
    key, sep, pattern = spec.partition("=")
    if not sep or not re.fullmatch(r"[a-z0-9_-]+", key) or not pattern:
        raise MetadataError(f"Invalid --require-asset '{spec}'. Expected KEY=REGEX with lowercase KEY.")
    try:
        return key, re.compile(pattern, re.IGNORECASE)
    except re.error as exc:
        raise MetadataError(f"Invalid regex for --require-asset '{key}': {exc}") from exc


def write_github_output(path: str, metadata: dict[str, Any]) -> None:
    scalar_keys = [
        "repository", "repository_url", "default_branch", "description", "homepage", "license",
        "license_url", "publisher", "publisher_url", "publisher_support_url", "version", "tag",
        "release_type", "is_prerelease", "published_at", "release_url", "source",
    ]
    with open(path, "a", encoding="utf-8") as handle:
        for key in scalar_keys:
            if key not in metadata:
                continue
            value = metadata[key]
            if isinstance(value, bool):
                value = "true" if value else "false"
            value = str(value).replace("\r", " ").replace("\n", " ")
            handle.write(f"{key.replace('_', '-')}={value}\n")
        for key, name in metadata.get("matched_assets", {}).items():
            handle.write(f"asset-{key}={name}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", ""), help="owner/name (default: $GITHUB_REPOSITORY)")
    parser.add_argument("--version", default="", help="Explicit version/tag; blank resolves the latest published release")
    parser.add_argument("--allow-prerelease", action="store_true", help="Consider prereleases during auto-resolution")
    parser.add_argument("--include-nightly", action="store_true", help="Consider nightly tags during auto-resolution")
    parser.add_argument("--repository-only", action="store_true", help="Resolve repository metadata and VERSION only; no release lookup")
    parser.add_argument("--require-asset", action="append", default=[], metavar="KEY=REGEX", help="Require exactly one release asset matching REGEX")
    parser.add_argument("--repo-root", default=".", help="Repository checkout root for LICENSE/VERSION")
    parser.add_argument("--output", default="", help="Write resolved metadata JSON to this path")
    parser.add_argument("--github-output", default=os.environ.get("GITHUB_OUTPUT", ""), help="Append step outputs to this file")
    args = parser.parse_args(argv)

    try:
        repo = args.repo.strip()
        if not REPO_RE.match(repo):
            raise MetadataError(f"Invalid or missing repository '{repo}'. Expected owner/name.")
        repo_root = Path(args.repo_root)
        required_assets = [parse_asset_spec(spec) for spec in args.require_asset]
        if args.repository_only and required_assets:
            raise MetadataError("--require-asset cannot be combined with --repository-only.")

        metadata = resolve_repository(repo, repo_root)
        if args.repository_only:
            version = normalize_version(args.version) if args.version.strip() else read_repository_version(repo_root)
            metadata.update({"version": version, "tag": f"v{version}", "source": "repository"})
            ref = metadata["default_branch"]
        else:
            explicit = normalize_version(args.version) if args.version.strip() else None
            metadata.update(resolve_release(repo, explicit, args.allow_prerelease, args.include_nightly, required_assets))
            metadata["source"] = "explicit" if explicit else "latest-published"
            ref = metadata["tag"]
        metadata["license_url"] = f"{metadata['repository_url']}/blob/{ref}/LICENSE"
    except MetadataError as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1

    if args.output:
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        Path(args.output).write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    if args.github_output:
        write_github_output(args.github_output, metadata)
    summary = {key: metadata.get(key) for key in ("repository", "version", "tag", "release_type", "license", "source")}
    print(json.dumps(summary, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
