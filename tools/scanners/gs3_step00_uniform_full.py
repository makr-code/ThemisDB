#!/usr/bin/env python3
"""
Gap Scanner V3 — Uniform Full Scanner (Phase 1-11)

Single, uniform scanner implementation that executes all historical scanner steps
and normalizes outputs into the shared Gap model.
"""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List

sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority
from scanners.gs3_step01_error_handling import ErrorHandlingScanner
from scanners.gs3_step01_memory_safety import MemorySafetyScanner
from scanners.gs3_step01_raii import RAIIScanner as ModernRAIIScanner
from scanners.gs3_step01_thread_safety import ThreadSafetyScanner
from scanners.gs3_step01_classic_concurrency import ConcurrencyGapScanner
from scanners.gs3_step01_classic_container import ContainerGapScanner
from scanners.gs3_step01_classic_memory import MemoryGapScanner
from scanners.gs3_step01_classic_performance import PerformanceGapScanner
from scanners.gs3_step01_classic_platform import PlatformGapScanner
from scanners.gs3_step01_classic_raii import RAIIGapScanner
from scanners.gs3_step01_classic_reliability import ReliabilityGapScanner
from scanners.gs3_step01_classic_security import SecurityGapScanner
from scanners.gs3_step02_exception_safety import ExceptionSafetyGapScanner
from scanners.gs3_step02_input_validation import InputValidationGapScanner
from scanners.gs3_step02_type_conversion import TypeConversionGapScanner
from scanners.gs3_step02_uninitialized import UninitializedGapScanner
from scanners.gs3_step02_virtual_oop import OOPGapScanner
from scanners.gs3_step03_attack_vectors import AttackVectorScanner
from scanners.gs3_step03_data_leak import DataLeakScanner
from scanners.gs3_step03_e2e_encryption import E2EEncryptionScanner
from scanners.gs3_step03_encryption_leak import EncryptionLeakScanner
from scanners.gs3_step03_key_failure import KeyFailureScanner
from scanners.gs3_step03_legacy_duplication import LegacyDuplicationScan
from scanners.gs3_step03_military_hardening import MilitaryHardeningScanner
from scanners.gs3_step04_audit_logging import AuditLoggingScan
from scanners.gs3_step04_deprecated_apis import DeprecatedAPIsScan
from scanners.gs3_step04_determinism import DeterminismScan
from scanners.gs3_step04_gpu_memory import GPUMemorySafetyScan
from scanners.gs3_step04_llm_ai_safety import LLMAISafetyScan
from scanners.gs3_step04_observability import ObservabilityScan
from scanners.gs3_step04_performance_patterns import PerformanceAntiPatternsScan
from scanners.gs3_step04_query_correctness import QueryCorrectnessScan
from scanners.gs3_step04_distributed_consistency import DistributedConsistencyScan


class UniformFullScanner(BaseGapScanner):
    """Uniform phase 1-11 scanner with standardized output conversion."""

    PRIORITY = ScannerPriority.SEMANTIC
    ENABLED = True
    MAX_RUNTIME_SECONDS = 600

    def __init__(self):
        super().__init__("Uniform Full Scanner", "4.0")

    def scan(self, source_dir: str) -> List[Gap]:
        gaps: List[Gap] = []
        self.source_path = Path(source_dir).resolve()
        output_dir = self.source_path.parent / "ai_working"
        output_dir.mkdir(parents=True, exist_ok=True)

        # Modern phase 1 scanners (uniform local implementation)
        modern_phase1 = [
            ("phase1_memory_safety", MemorySafetyScanner()),
            ("phase1_error_handling", ErrorHandlingScanner()),
            ("phase1_thread_safety", ThreadSafetyScanner()),
            ("phase1_raii", ModernRAIIScanner()),
        ]

        for phase_key, scanner in modern_phase1:
            try:
                modern_gaps = scanner.scan(str(self.source_path))
                gaps.extend(self._convert_iterable_result(modern_gaps, phase_key, "phase1_modern"))
            except Exception:
                continue

        # Classic phase 1-4 modules (module report format)
        classic_scanners = [
            ("security", SecurityGapScanner(str(self.source_path))),
            ("memory", MemoryGapScanner(str(self.source_path))),
            ("reliability", ReliabilityGapScanner(str(self.source_path))),
            ("concurrency", ConcurrencyGapScanner(str(self.source_path))),
            ("raii", RAIIGapScanner(str(self.source_path))),
            ("container", ContainerGapScanner(str(self.source_path))),
            ("platform", PlatformGapScanner(str(self.source_path))),
            ("performance", PerformanceGapScanner(str(self.source_path))),
        ]

        for phase_key, scanner in classic_scanners:
            try:
                result = scanner.run_full_scan(str(output_dir))
                gaps.extend(self._convert_classic_module_result(result, phase_key))
            except Exception:
                continue

        # Phase 5 scanners (run_full_scan -> list of dataclasses)
        phase5_scanners = [
            ("type_conversion", TypeConversionGapScanner(), self._run_path),
            ("input_validation", InputValidationGapScanner(), self._run_path),
            ("exception_safety", ExceptionSafetyGapScanner(), self._run_str),
            ("uninitialized", UninitializedGapScanner(), self._run_str),
            ("virtual_oop", OOPGapScanner(), self._run_str),
        ]

        for phase_key, scanner, runner in phase5_scanners:
            try:
                result = runner(scanner, self.source_path)
                gaps.extend(self._convert_iterable_result(result, phase_key, "phase5"))
            except Exception:
                continue

        cpp_files = self._collect_cpp_files(self.source_path)
        self.files_scanned = len(cpp_files)

        # Phase 7-10 scanners (scan_files -> list of dict)
        phase7_10_scanners = [
            ("audit_logging", AuditLoggingScan(str(self.source_path))),
            ("deprecated_apis", DeprecatedAPIsScan(str(self.source_path))),
            ("performance_patterns", PerformanceAntiPatternsScan(str(self.source_path))),
            ("gpu_memory", GPUMemorySafetyScan(str(self.source_path))),
            ("query_correctness", QueryCorrectnessScan(str(self.source_path))),
            ("distributed_consistency", DistributedConsistencyScan(str(self.source_path))),
            ("llm_ai_safety", LLMAISafetyScan(str(self.source_path))),
            ("observability", ObservabilityScan(str(self.source_path))),
            ("determinism", DeterminismScan(str(self.source_path))),
        ]

        for phase_key, scanner in phase7_10_scanners:
            try:
                result = scanner.scan_files(cpp_files)
                gaps.extend(self._convert_iterable_result(result, phase_key, "phase7_10"))
            except Exception:
                continue

        # Phase 11 scanners (scan_repository or scan_files)
        phase11_repo_scanners = [
            ("data_leak", DataLeakScanner(str(self.source_path))),
            ("encryption_leak", EncryptionLeakScanner(str(self.source_path))),
            ("e2e_encryption", E2EEncryptionScanner(str(self.source_path))),
            ("key_failure", KeyFailureScanner(str(self.source_path))),
            ("attack_vectors", AttackVectorScanner(str(self.source_path))),
            ("military_hardening", MilitaryHardeningScanner(str(self.source_path))),
        ]

        for phase_key, scanner in phase11_repo_scanners:
            try:
                repo_result = scanner.scan_repository()
                for module_items in repo_result.values():
                    gaps.extend(self._convert_iterable_result(module_items, phase_key, "phase11"))
            except Exception:
                continue

        try:
            legacy_dup = LegacyDuplicationScan(str(self.source_path))
            dup_result = legacy_dup.scan_files(cpp_files)
            gaps.extend(self._convert_iterable_result(dup_result, "legacy_duplication", "phase11"))
        except Exception:
            pass

        return self.deduplicate(gaps)

    def _run_path(self, scanner: Any, source_path: Path) -> List[Any]:
        return scanner.run_full_scan(source_path)

    def _run_str(self, scanner: Any, source_path: Path) -> List[Any]:
        return scanner.run_full_scan(str(source_path))

    def _collect_cpp_files(self, source_path: Path) -> List[Path]:
        files: List[Path] = []
        for ext in (".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx", ".c"):
            files.extend(source_path.rglob(f"*{ext}"))
        return [f for f in files if "test" not in f.parts and "build" not in f.parts]

    def _convert_classic_module_result(self, result: Dict[str, Any], phase_key: str) -> List[Gap]:
        converted: List[Gap] = []
        for module_data in (result or {}).values():
            gaps_by_file = module_data.get("gaps_by_file", {}) if isinstance(module_data, dict) else {}
            for file_path, items in gaps_by_file.items():
                for item in items or []:
                    converted.append(self._to_gap(item, phase_key, "phase1_4", file_hint=file_path))
        return converted

    def _convert_iterable_result(self, items: Iterable[Any], phase_key: str, phase: str) -> List[Gap]:
        converted: List[Gap] = []
        for item in items or []:
            converted.append(self._to_gap(item, phase_key, phase))
        return converted

    def _to_gap(self, item: Any, phase_key: str, phase: str, file_hint: str = "") -> Gap:
        item_dict = item.to_dict() if hasattr(item, "to_dict") else (item if isinstance(item, dict) else {})

        file_path = self._pick(item_dict, ["file", "file_path"], default=file_hint)
        line = int(self._pick(item_dict, ["line", "line_num", "line_number"], default=1) or 1)
        severity = self._normalize_severity(self._pick(item_dict, ["severity"], default="MEDIUM"))
        gap_type = self._normalize_gap_type(item_dict, phase_key)

        return Gap(
            file=self._normalize_file_path(str(file_path)),
            line=line,
            type=gap_type,
            severity=severity,
            confidence=self._confidence_for(severity),
            description=self._pick(item_dict, ["description", "reason", "issue", "pattern"], default=f"{phase_key} finding"),
            remediation="Review finding and apply recommended module-specific fix.",
            context=self._pick(item_dict, ["context", "line_content", "snippet"], default=""),
            scanner=f"Uniform::{phase_key}",
            step=phase,
        )

    def _normalize_file_path(self, file_path: str) -> str:
        try:
            fp = Path(file_path)
            if fp.is_absolute():
                return str(fp.resolve().relative_to(self.source_path)).replace("\\", "/")
            return str(fp).replace("\\", "/")
        except Exception:
            return str(file_path).replace("\\", "/")

    def _normalize_gap_type(self, item_dict: Dict[str, Any], phase_key: str) -> str:
        raw = self._pick(item_dict, ["gap_type", "type", "pattern", "category", "issue_type"], default=f"{phase_key}_gap")
        return str(raw).replace(" ", "_").replace("-", "_").lower()

    def _normalize_severity(self, severity: Any) -> str:
        sev = str(severity).upper()
        if "CRITICAL" in sev:
            return "CRITICAL"
        if "HIGH" in sev:
            return "HIGH"
        if "LOW" in sev:
            return "LOW"
        return "MEDIUM"

    def _confidence_for(self, severity: str) -> float:
        if severity == "CRITICAL":
            return 0.85
        if severity == "HIGH":
            return 0.75
        if severity == "LOW":
            return 0.6
        return 0.68

    def _pick(self, item_dict: Dict[str, Any], keys: List[str], default: Any = "") -> Any:
        for key in keys:
            value = item_dict.get(key)
            if value is not None and str(value).strip():
                return value
        return default
