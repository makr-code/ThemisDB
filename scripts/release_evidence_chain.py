#!/usr/bin/env python3
"""Generate and verify ThemisDB release/deployment evidence-chain ledgers.

The ledger format is JSONL and append-only.
Each record links to the previous record via `previous_hash`.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
import re


GENESIS_HASH = "0" * 64
SCHEMA_VERSION = "1.0"
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")


class EvidenceChainError(RuntimeError):
    """Domain-specific validation error."""


def canonical_json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


@dataclass(frozen=True)
class WorkflowMetadata:
    workflow: str
    workflow_run_id: str
    workflow_run_attempt: int
    workflow_url: str
    job_name: str
    source_workflow: str
    source_run_id: str


@dataclass(frozen=True)
class ApprovalMetadata:
    approver_identity: str
    approver_role: str
    approval_scope: str
    environment: str


@dataclass(frozen=True)
class ReleaseEvidenceInput:
    index: int
    event_type: str
    commit_sha: str
    tag: str
    version: str
    artifact_digest_sha256: str
    sbom_digest_sha256: str
    previous_hash: str
    timestamp_utc: str
    repository: str
    workflow: WorkflowMetadata
    approval: ApprovalMetadata

    def to_payload(self) -> dict[str, Any]:
        return {
            "schema_version": SCHEMA_VERSION,
            "index": self.index,
            "event_type": self.event_type,
            "commit_sha": self.commit_sha,
            "tag": self.tag,
            "version": self.version,
            "artifact_digest_sha256": self.artifact_digest_sha256,
            "sbom_digest_sha256": self.sbom_digest_sha256,
            "previous_hash": self.previous_hash,
            "approver_identity": self.approval.approver_identity,
            "approver_role": self.approval.approver_role,
            "approval_scope": self.approval.approval_scope,
            "timestamp_utc": self.timestamp_utc,
            "environment": self.approval.environment,
            "workflow": self.workflow.workflow,
            "workflow_run_id": self.workflow.workflow_run_id,
            "workflow_run_attempt": self.workflow.workflow_run_attempt,
            "workflow_url": self.workflow.workflow_url,
            "job_name": self.workflow.job_name,
            "source_workflow": self.workflow.source_workflow,
            "source_run_id": self.workflow.source_run_id,
            "repository": self.repository,
            "compliance_framework": "SOC2",
            "design_principles": ["OOP", "SoC", "least-privilege", "traceability"],
        }


class ReleaseEvidenceLedger:
    """Append-only JSONL ledger manager with chain validation."""

    def __init__(self, ledger_path: Path) -> None:
        self.ledger_path = ledger_path

    def load_records(self) -> list[dict[str, Any]]:
        if not self.ledger_path.exists():
            return []
        records: list[dict[str, Any]] = []
        for line_number, raw in enumerate(self.ledger_path.read_text(encoding="utf-8").splitlines(), start=1):
            raw = raw.strip()
            if not raw:
                continue
            try:
                record = json.loads(raw)
            except json.JSONDecodeError as exc:
                raise EvidenceChainError(
                    f"Invalid JSONL record at line {line_number} in {self.ledger_path}: {exc}"
                ) from exc
            if not isinstance(record, dict):
                raise EvidenceChainError(f"Record at line {line_number} must be a JSON object")
            records.append(record)
        return records

    def last_record_hash(self) -> str:
        records = self.load_records()
        if not records:
            return GENESIS_HASH
        last_hash = records[-1].get("record_hash", "")
        if not isinstance(last_hash, str) or len(last_hash) != 64:
            raise EvidenceChainError("Last record is missing a valid 'record_hash'")
        return last_hash

    @staticmethod
    def compute_payload_hash(payload: dict[str, Any]) -> str:
        return sha256_hex(canonical_json(payload).encode("utf-8"))

    @staticmethod
    def compute_record_hash(previous_hash: str, payload_hash: str) -> str:
        return sha256_hex(f"{previous_hash}:{payload_hash}".encode("utf-8"))

    @staticmethod
    def compute_signature(record_hash: str, approver_identity: str, workflow_run_id: str) -> str:
        marker = f"{record_hash}:{approver_identity}:{workflow_run_id}"
        return f"sha256:{sha256_hex(marker.encode('utf-8'))}"

    def append(self, event_input: ReleaseEvidenceInput) -> dict[str, Any]:
        payload = event_input.to_payload()
        payload_hash = self.compute_payload_hash(payload)
        record_hash = self.compute_record_hash(event_input.previous_hash, payload_hash)
        signature = self.compute_signature(
            record_hash=record_hash,
            approver_identity=event_input.approval.approver_identity,
            workflow_run_id=event_input.workflow.workflow_run_id,
        )
        record = {
            **payload,
            "payload_hash": payload_hash,
            "record_hash": record_hash,
            "signature": signature,
        }
        self.ledger_path.parent.mkdir(parents=True, exist_ok=True)
        with self.ledger_path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, sort_keys=True, ensure_ascii=False) + "\n")
        return record

    def verify(self) -> list[dict[str, Any]]:
        records = self.load_records()
        previous_hash = GENESIS_HASH
        for position, record in enumerate(records, start=1):
            current_previous_hash = record.get("previous_hash")
            if current_previous_hash != previous_hash:
                raise EvidenceChainError(
                    f"Chain break at position {position}: previous_hash={current_previous_hash!r} expected={previous_hash!r}"
                )
            payload = {k: v for k, v in record.items() if k not in {"payload_hash", "record_hash", "signature"}}
            expected_payload_hash = self.compute_payload_hash(payload)
            actual_payload_hash = record.get("payload_hash")
            if actual_payload_hash != expected_payload_hash:
                raise EvidenceChainError(
                    f"Payload hash mismatch at position {position}: actual={actual_payload_hash!r} expected={expected_payload_hash!r}"
                )
            expected_record_hash = self.compute_record_hash(previous_hash, expected_payload_hash)
            actual_record_hash = record.get("record_hash")
            if actual_record_hash != expected_record_hash:
                raise EvidenceChainError(
                    f"Record hash mismatch at position {position}: actual={actual_record_hash!r} expected={expected_record_hash!r}"
                )
            expected_signature = self.compute_signature(
                expected_record_hash,
                str(record.get("approver_identity", "")),
                str(record.get("workflow_run_id", "")),
            )
            actual_signature = record.get("signature")
            if actual_signature != expected_signature:
                raise EvidenceChainError(
                    f"Signature mismatch at position {position}: actual={actual_signature!r} expected={expected_signature!r}"
                )
            previous_hash = expected_record_hash
        return records


def validate_nonempty(name: str, value: str) -> str:
    if not value or not value.strip():
        raise EvidenceChainError(f"Argument '{name}' must be a non-empty string")
    return value.strip()


def validate_digest(name: str, value: str) -> str:
    normalized = validate_nonempty(name, value).lower()
    if normalized == "unknown":
        return normalized
    if not SHA256_PATTERN.match(normalized):
        raise EvidenceChainError(
            f"Argument '{name}' must be a 64-char lowercase SHA-256 hex digest or 'unknown', got '{value}'."
        )
    return normalized


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ThemisDB release evidence-chain generator/verifier.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate = subparsers.add_parser("generate", help="Append one evidence record to a JSONL ledger.")
    generate.add_argument("--ledger", required=True, help="Path to the JSONL evidence ledger.")
    generate.add_argument("--event-type", required=True)
    generate.add_argument("--commit-sha", required=True)
    generate.add_argument("--tag", required=True)
    generate.add_argument("--version", required=True)
    generate.add_argument("--artifact-digest-sha256", required=True)
    generate.add_argument("--sbom-digest-sha256", required=True)
    generate.add_argument("--approver-identity", required=True)
    generate.add_argument("--approver-role", required=True)
    generate.add_argument("--approval-scope", required=True)
    generate.add_argument("--environment", required=True)
    generate.add_argument("--workflow-name", required=True)
    generate.add_argument("--workflow-run-id", required=True)
    generate.add_argument("--workflow-run-attempt", type=int, default=1)
    generate.add_argument("--workflow-url", required=True)
    generate.add_argument("--job-name", required=True)
    generate.add_argument("--source-workflow", required=True)
    generate.add_argument("--source-run-id", required=True)
    generate.add_argument("--repository", required=True)
    generate.add_argument("--timestamp-utc", default="", help="Optional RFC3339 UTC timestamp override.")
    generate.add_argument("--record-output", default="", help="Optional output path for the generated record JSON.")

    verify = subparsers.add_parser("verify", help="Verify full ledger chain integrity.")
    verify.add_argument("--ledger", required=True, help="Path to the JSONL evidence ledger.")
    return parser.parse_args(argv)


def run_generate(args: argparse.Namespace) -> int:
    ledger = ReleaseEvidenceLedger(Path(args.ledger).resolve())
    previous_hash = ledger.last_record_hash()
    existing = ledger.load_records()
    index = len(existing) + 1

    timestamp_utc = args.timestamp_utc.strip() if args.timestamp_utc else utc_now()
    if timestamp_utc.endswith("+00:00"):
        timestamp_utc = timestamp_utc.replace("+00:00", "Z")

    workflow = WorkflowMetadata(
        workflow=validate_nonempty("workflow-name", args.workflow_name),
        workflow_run_id=validate_nonempty("workflow-run-id", args.workflow_run_id),
        workflow_run_attempt=int(args.workflow_run_attempt),
        workflow_url=validate_nonempty("workflow-url", args.workflow_url),
        job_name=validate_nonempty("job-name", args.job_name),
        source_workflow=validate_nonempty("source-workflow", args.source_workflow),
        source_run_id=validate_nonempty("source-run-id", args.source_run_id),
    )
    approval = ApprovalMetadata(
        approver_identity=validate_nonempty("approver-identity", args.approver_identity),
        approver_role=validate_nonempty("approver-role", args.approver_role),
        approval_scope=validate_nonempty("approval-scope", args.approval_scope),
        environment=validate_nonempty("environment", args.environment),
    )
    record_input = ReleaseEvidenceInput(
        index=index,
        event_type=validate_nonempty("event-type", args.event_type),
        commit_sha=validate_nonempty("commit-sha", args.commit_sha).lower(),
        tag=validate_nonempty("tag", args.tag),
        version=validate_nonempty("version", args.version),
        artifact_digest_sha256=validate_digest("artifact-digest-sha256", args.artifact_digest_sha256),
        sbom_digest_sha256=validate_digest("sbom-digest-sha256", args.sbom_digest_sha256),
        previous_hash=previous_hash,
        timestamp_utc=timestamp_utc,
        repository=validate_nonempty("repository", args.repository),
        workflow=workflow,
        approval=approval,
    )
    record = ledger.append(record_input)
    ledger.verify()
    if args.record_output:
        output_path = Path(args.record_output).resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(record, indent=2, ensure_ascii=False, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(record, indent=2, ensure_ascii=False, sort_keys=True))
    return 0


def run_verify(args: argparse.Namespace) -> int:
    ledger = ReleaseEvidenceLedger(Path(args.ledger).resolve())
    records = ledger.verify()
    summary = {
        "ledger": str(Path(args.ledger).resolve()),
        "record_count": len(records),
        "head_record_hash": records[-1]["record_hash"] if records else GENESIS_HASH,
        "status": "ok",
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False, sort_keys=True))
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        if args.command == "generate":
            return run_generate(args)
        if args.command == "verify":
            return run_verify(args)
        raise EvidenceChainError(f"Unsupported command: {args.command}")
    except (EvidenceChainError, OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
