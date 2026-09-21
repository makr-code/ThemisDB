#!/usr/bin/env python3
"""Validate and materialize GA promotion sign-off manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import yaml  # type: ignore
except ImportError:  # pragma: no cover - optional dependency for YAML payloads
    yaml = None

SCHEMA_VERSION = 1
MANIFEST_KIND = "themisdb.ga-promotion-signoff-manifest"
PLACEHOLDER_PATTERN = re.compile(r"^__[A-Z0-9_]+__$")
ALLOWED_UNRESOLVED_PLACEHOLDERS = {
    "approver.github",
    "signoff.signed_at",
    "repository_state.ref",
    "repository_state.sha",
}
EXPECTED_PLACEHOLDER_VALUES = {
    "approver.github": {"__GITHUB_ACTOR__"},
    "signoff.signed_at": {"__GITHUB_REVIEW_SUBMITTED_AT__", "__CURRENT_UTC__"},
    "repository_state.ref": {"__GITHUB_REF__"},
    "repository_state.sha": {"__GITHUB_SHA__"},
}
SHA_PATTERN = re.compile(r"^[0-9a-fA-F]{7,40}$")


@dataclass
class ValidationResult:
    payload_path: str
    manifest_path: str
    manifest_sha256_path: str
    manifest_sha256: str
    canonical_record_sha256: str
    approver: str
    signed_at: str
    preview: bool


class SignoffError(RuntimeError):
    """Domain-specific validation error."""


def canonical_json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_payload(path: Path) -> dict[str, Any]:
    raw_text = path.read_text(encoding="utf-8")
    suffix = path.suffix.lower()
    if suffix == ".json":
        payload = json.loads(raw_text)
    else:
        if yaml is None:
            raise SignoffError(
                f"YAML payloads require PyYAML: cannot read {path}. Install pyyaml or use JSON."
            )
        payload = yaml.safe_load(raw_text)
    if not isinstance(payload, dict):
        raise SignoffError(f"Payload {path} must be a JSON/YAML object.")
    return payload


def load_authorized_maintainers(path: Path) -> dict[str, dict[str, Any]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    maintainers = payload.get("maintainers")
    if not isinstance(maintainers, list) or not maintainers:
        raise SignoffError(f"Authorized maintainer file {path} does not contain a usable 'maintainers' list.")

    result: dict[str, dict[str, Any]] = {}
    for entry in maintainers:
        if not isinstance(entry, dict):
            continue
        github = str(entry.get("github", "")).strip()
        if github and entry.get("can_sign_ga") is True:
            result[github] = entry
    if not result:
        raise SignoffError(f"Authorized maintainer file {path} does not contain any GA signers.")
    return result


def replace_placeholders(value: Any, context: dict[str, str]) -> Any:
    if isinstance(value, str):
        candidate = context.get(value)
        return candidate if candidate else value
    if isinstance(value, dict):
        return {key: replace_placeholders(subvalue, context) for key, subvalue in value.items()}
    if isinstance(value, list):
        return [replace_placeholders(item, context) for item in value]
    return value


def get_path(data: dict[str, Any], path: str) -> Any:
    current: Any = data
    for segment in path.split("."):
        if not isinstance(current, dict) or segment not in current:
            raise SignoffError(f"Missing required field '{path}'.")
        current = current[segment]
    return current


def require_string(data: dict[str, Any], path: str) -> str:
    value = get_path(data, path)
    if not isinstance(value, str) or not value.strip():
        raise SignoffError(f"Field '{path}' must be a non-empty string.")
    return value.strip()


def collect_unresolved_placeholders(value: Any, prefix: str = "") -> list[str]:
    placeholders: list[str] = []
    if isinstance(value, str) and PLACEHOLDER_PATTERN.match(value):
        placeholders.append(prefix)
    elif isinstance(value, dict):
        for key, subvalue in value.items():
            nested = f"{prefix}.{key}" if prefix else str(key)
            placeholders.extend(collect_unresolved_placeholders(subvalue, nested))
    elif isinstance(value, list):
        for index, subvalue in enumerate(value):
            nested = f"{prefix}[{index}]" if prefix else f"[{index}]"
            placeholders.extend(collect_unresolved_placeholders(subvalue, nested))
    return placeholders


def parse_timestamp(value: str) -> str:
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise SignoffError(f"Field 'signoff.signed_at' must be an ISO 8601 timestamp, got '{value}'.") from exc
    if parsed.tzinfo is None:
        raise SignoffError("Field 'signoff.signed_at' must include a timezone offset or use 'Z'.")
    return parsed.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")


def validate_payload(
    payload: dict[str, Any],
    authorized_maintainers: dict[str, dict[str, Any]],
    *,
    allow_unresolved_placeholders: bool,
    github_actor: str,
) -> dict[str, Any]:
    schema_version = payload.get("schema_version")
    if schema_version != SCHEMA_VERSION:
        raise SignoffError(f"Payload schema_version must be {SCHEMA_VERSION}, got {schema_version!r}.")

    canonical_record = {
        "schema_version": SCHEMA_VERSION,
        "ga_name": require_string(payload, "ga_name"),
        "target_version": require_string(payload, "target_version"),
        "release_tag": require_string(payload, "release_tag"),
        "sponsor": {
            "name": require_string(payload, "sponsor.name"),
            "team": require_string(payload, "sponsor.team"),
        },
        "requested_by": {
            "github": require_string(payload, "requested_by.github"),
        },
        "approver": {
            "github": require_string(payload, "approver.github"),
            "role": require_string(payload, "approver.role"),
        },
        "signoff": {
            "signed_at": require_string(payload, "signoff.signed_at"),
            "rationale": require_string(payload, "signoff.rationale"),
        },
        "repository_state": {
            "repository": require_string(payload, "repository_state.repository"),
            "ref": require_string(payload, "repository_state.ref"),
            "sha": require_string(payload, "repository_state.sha").lower(),
            "source_document": require_string(payload, "repository_state.source_document"),
        },
        "references": {},
    }

    references = payload.get("references")
    if isinstance(references, dict):
        for key in ("issue_or_pr", "evidence_path"):
            value = references.get(key)
            if value is not None:
                if not isinstance(value, str) or not value.strip():
                    raise SignoffError(f"Field 'references.{key}' must be a non-empty string when provided.")
                canonical_record["references"][key] = value.strip()

    notes = payload.get("notes")
    if notes is not None:
        if not isinstance(notes, str) or not notes.strip():
            raise SignoffError("Field 'notes' must be a non-empty string when provided.")
        canonical_record["notes"] = notes.strip()

    unresolved = collect_unresolved_placeholders(canonical_record)
    if allow_unresolved_placeholders:
        disallowed = sorted(path for path in unresolved if path not in ALLOWED_UNRESOLVED_PLACEHOLDERS)
        if disallowed:
            raise SignoffError(
                "Unresolved placeholders are only allowed for preview-safe CI fields. "
                f"Disallowed placeholders: {', '.join(disallowed)}"
            )
        for path in unresolved:
            value = get_path(canonical_record, path)
            allowed_values = EXPECTED_PLACEHOLDER_VALUES.get(path, set())
            if value not in allowed_values:
                raise SignoffError(
                    f"Field '{path}' uses unsupported placeholder value '{value}'. "
                    f"Allowed values: {', '.join(sorted(allowed_values))}"
                )
    elif unresolved:
        raise SignoffError(
            "Final GA sign-off manifests may not contain unresolved CI placeholders: "
            + ", ".join(sorted(unresolved))
        )

    approver = canonical_record["approver"]["github"]
    approver_is_placeholder = PLACEHOLDER_PATTERN.match(approver) is not None
    if github_actor:
        if github_actor not in authorized_maintainers:
            raise SignoffError(
                f"GitHub actor '{github_actor}' is not authorized to sign GA promotions."
            )
        if approver != github_actor:
            raise SignoffError(
                f"Approver '{approver}' does not match GitHub actor '{github_actor}'."
            )
    if not approver_is_placeholder and approver not in authorized_maintainers:
        raise SignoffError(
            f"Approver '{approver}' is not present in the authorized maintainer policy."
        )

    signed_at = canonical_record["signoff"]["signed_at"]
    if not PLACEHOLDER_PATTERN.match(signed_at):
        canonical_record["signoff"]["signed_at"] = parse_timestamp(signed_at)

    sha_value = canonical_record["repository_state"]["sha"]
    if not PLACEHOLDER_PATTERN.match(sha_value) and not SHA_PATTERN.match(sha_value):
        raise SignoffError(
            f"Field 'repository_state.sha' must look like a Git commit SHA, got '{sha_value}'."
        )

    return canonical_record


def build_manifest(
    canonical_record: dict[str, Any],
    *,
    payload_path: Path,
    payload_sha256: str,
    maintainers_path: Path,
    github_repository: str,
    github_ref: str,
    github_sha: str,
    github_actor: str,
    github_event_name: str,
    github_run_id: str,
    preview: bool,
) -> tuple[dict[str, Any], str, str]:
    canonical_record_json = canonical_json(canonical_record)
    canonical_record_sha256 = sha256_hex(canonical_record_json.encode("utf-8"))

    manifest = {
        "schema_version": SCHEMA_VERSION,
        "kind": MANIFEST_KIND,
        "preview": preview,
        "generated_at": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "payload_path": payload_path.as_posix(),
        "payload_sha256": payload_sha256,
        "canonical_record": canonical_record,
        "canonical_record_sha256": canonical_record_sha256,
        "validation": {
            "authorized_maintainers_policy": maintainers_path.as_posix(),
            "approver": canonical_record["approver"]["github"],
            "github_actor": github_actor,
            "actor_matches_approver": bool(github_actor) and github_actor == canonical_record["approver"]["github"],
        },
        "ci_context": {
            "repository": github_repository,
            "ref": github_ref,
            "sha": github_sha.lower() if github_sha else "",
            "event_name": github_event_name,
            "run_id": github_run_id,
        },
        "storage_model": {
            "repository_contents": "reviewable sign-off request payload only",
            "external_evidence": "GitHub Actions artifact containing the generated manifest and detached SHA-256",
            "trust_statement": (
                "This workflow provides integrity and auditability via canonical JSON hashing and "
                "GitHub identity/workflow context. It does not claim an external cryptographic signature "
                "beyond the GitHub-authorized maintainer identity bound to the workflow run."
            ),
        },
    }
    manifest_json = json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
    manifest_sha256 = sha256_hex(manifest_json.encode("utf-8"))
    return manifest, manifest_json, manifest_sha256


def write_outputs(output_dir: Path, payload_path: Path, manifest_json: str, manifest_sha256: str) -> tuple[Path, Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    stem = payload_path.stem
    manifest_path = output_dir / f"{stem}.ga-promotion-manifest.json"
    manifest_sha256_path = output_dir / f"{stem}.ga-promotion-manifest.sha256"
    manifest_path.write_text(manifest_json, encoding="utf-8")
    manifest_sha256_path.write_text(f"{manifest_sha256}  {manifest_path.name}\n", encoding="utf-8")
    return manifest_path, manifest_sha256_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate and materialize GA promotion sign-off manifests.")
    parser.add_argument("--payload", required=True, help="Path to the GA sign-off request payload (JSON or YAML).")
    parser.add_argument(
        "--authorized-maintainers",
        default=".github/ga-signoff-authorized-maintainers.json",
        help="JSON policy file listing GitHub handles allowed to sign GA promotions.",
    )
    parser.add_argument("--output-dir", required=True, help="Directory that will receive the manifest outputs.")
    parser.add_argument("--result-json", help="Optional path for a machine-readable summary JSON file.")
    parser.add_argument(
        "--allow-unresolved-placeholders",
        action="store_true",
        help="Allow a limited set of CI placeholders when generating a preview artifact on PR validation.",
    )
    parser.add_argument("--github-actor", default=os.environ.get("GITHUB_ACTOR", ""))
    parser.add_argument("--github-sha", default=os.environ.get("GITHUB_SHA", ""))
    parser.add_argument("--github-ref", default=os.environ.get("GITHUB_REF", ""))
    parser.add_argument("--github-repository", default=os.environ.get("GITHUB_REPOSITORY", ""))
    parser.add_argument("--github-event-name", default=os.environ.get("GITHUB_EVENT_NAME", ""))
    parser.add_argument("--github-run-id", default=os.environ.get("GITHUB_RUN_ID", ""))
    parser.add_argument(
        "--signed-at",
        default=os.environ.get("GA_SIGNOFF_SIGNED_AT", ""),
        help="Final signer timestamp. When omitted for workflow_dispatch, the script uses current UTC.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    payload_path = Path(args.payload).resolve()
    maintainers_path = Path(args.authorized_maintainers).resolve()
    output_dir = Path(args.output_dir).resolve()

    preview = bool(args.allow_unresolved_placeholders)
    signed_at = args.signed_at or ("" if preview else datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"))
    context = {
        "__GITHUB_ACTOR__": args.github_actor,
        "__GITHUB_SHA__": args.github_sha,
        "__GITHUB_REF__": args.github_ref,
        "__GITHUB_REPOSITORY__": args.github_repository,
        "__GITHUB_EVENT_NAME__": args.github_event_name,
        "__GITHUB_RUN_ID__": args.github_run_id,
        "__GITHUB_REVIEW_SUBMITTED_AT__": signed_at,
        "__CURRENT_UTC__": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
    }

    try:
        payload = read_payload(payload_path)
        payload_sha256 = sha256_hex(payload_path.read_bytes())
        authorized_maintainers = load_authorized_maintainers(maintainers_path)
        resolved_payload = replace_placeholders(payload, context)
        canonical_record = validate_payload(
            resolved_payload,
            authorized_maintainers,
            allow_unresolved_placeholders=preview,
            github_actor="" if preview else args.github_actor.strip(),
        )
        manifest, manifest_json, manifest_sha256 = build_manifest(
            canonical_record,
            payload_path=payload_path.relative_to(Path.cwd()) if payload_path.is_relative_to(Path.cwd()) else payload_path,
            payload_sha256=payload_sha256,
            maintainers_path=maintainers_path.relative_to(Path.cwd()) if maintainers_path.is_relative_to(Path.cwd()) else maintainers_path,
            github_repository=args.github_repository,
            github_ref=args.github_ref,
            github_sha=args.github_sha,
            github_actor="" if preview else args.github_actor.strip(),
            github_event_name=args.github_event_name,
            github_run_id=args.github_run_id,
            preview=preview,
        )
        manifest_path, manifest_sha256_path = write_outputs(output_dir, payload_path, manifest_json, manifest_sha256)
        result = ValidationResult(
            payload_path=(payload_path.relative_to(Path.cwd()) if payload_path.is_relative_to(Path.cwd()) else payload_path).as_posix(),
            manifest_path=manifest_path.as_posix(),
            manifest_sha256_path=manifest_sha256_path.as_posix(),
            manifest_sha256=manifest_sha256,
            canonical_record_sha256=manifest["canonical_record_sha256"],
            approver=canonical_record["approver"]["github"],
            signed_at=canonical_record["signoff"]["signed_at"],
            preview=preview,
        )
        if args.result_json:
            Path(args.result_json).resolve().write_text(
                json.dumps(asdict(result), indent=2, ensure_ascii=False),
                encoding="utf-8",
            )
        print(json.dumps(asdict(result), indent=2, ensure_ascii=False))
        return 0
    except (OSError, json.JSONDecodeError, SignoffError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
