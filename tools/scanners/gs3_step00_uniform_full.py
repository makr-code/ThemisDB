#!/usr/bin/env python3
"""
Gap Scanner V3 — Uniform Full Scanner (Phase 1-11)

Single, uniform scanner implementation that executes all historical scanner steps
and normalizes outputs into the shared Gap model.
"""

from __future__ import annotations

import sys
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List

sys.path.insert(0, str(Path(__file__).parent.parent))

from gs3_base_scanner import BaseGapScanner, Gap, ScannerPriority
from scanners.gs3_step01_check_braces import BracesCheckScanner
from scanners.gs3_step01_check_namespace_unity import NamespaceUnityCheckScanner
from scanners.gs3_step01_ai_simulation_stub_leak import SimulationStubLeakScanner
from scanners.gs3_step01_ai_todo_productionlogic import TodoProductionlogicScanner
from scanners.gs3_step01_ai_error_handling_consistency import ErrorHandlingConsistencyScanner
from scanners.gs3_step01_ai_llm_prompt_injection import LlmPromptInjectionScanner
from scanners.gs3_step01_ai_header_drift import HeaderDriftScanner
from scanners.gs3_step01_core_error_handling import ErrorHandlingScanner
from scanners.gs3_step01_core_memory_safety import MemorySafetyScannerImproved as MemorySafetyScanner
from scanners.gs3_step01_core_raii import RAIIGapScanner as ModernRAIIScanner
from scanners.gs3_step01_core_thread_safety import ThreadSafetyScannerImproved as ThreadSafetyScanner
from scanners.gs3_step01_core_concurrency import ConcurrencyGapScanner
from scanners.gs3_step01_core_container import ContainerGapScanner
from scanners.gs3_step01_core_memory import MemoryGapScannerImproved as MemoryGapScanner
from scanners.gs3_step01_core_performance import PerformanceGapScanner
from scanners.gs3_step01_core_platform import PlatformGapScanner
from scanners.gs3_step01_core_raii import RAIIGapScanner
from scanners.gs3_step01_core_reliability import ReliabilityGapScanner
from scanners.gs3_step01_core_security import SecurityGapScanner
from scanners.gs3_step02_safety_exception import ExceptionSafetyGapScannerImproved as ExceptionSafetyGapScanner
from scanners.gs3_step02_safety_input_validation import InputValidationGapScanner
from scanners.gs3_step02_safety_type_conversion import TypeConversionGapScanner
from scanners.gs3_step02_safety_uninitialized import UninitializedGapScannerImproved as UninitializedGapScanner
from scanners.gs3_step02_safety_virtual_oop import OOPGapScanner
from scanners.gs3_step03_security_attack_vectors import AttackVectorScanner
from scanners.gs3_step03_security_data_leak import DataLeakScannerImproved as DataLeakScanner
from scanners.gs3_step03_security_e2e_encryption import E2EEncryptionScanner
from scanners.gs3_step03_security_encryption_leak import EncryptionLeakScannerImproved as EncryptionLeakScanner
from scanners.gs3_step03_security_key_failure import KeyFailureScannerImproved as KeyFailureScanner
from scanners.gs3_step03_security_legacy_duplication import LegacyDuplicationScanImproved as LegacyDuplicationScan
from scanners.gs3_step03_security_military_hardening import MilitaryHardeningScanner
from scanners.gs3_step04_quality_audit_logging import AuditLoggingScanImproved as AuditLoggingScan
from scanners.gs3_step04_design_deprecated_apis import DeprecatedAPIsScan
from scanners.gs3_step04_design_determinism import DeterminismScannerImproved as DeterminismScan
from scanners.gs3_step04_design_gpu_memory import GPUMemorySafetyScan
from scanners.gs3_step04_design_llm_ai_safety import LLMAISafetyScan
from scanners.gs3_step04_design_observability import ObservabilityScannerImproved as ObservabilityScan
from scanners.gs3_step04_design_performance_patterns import PerformanceAntiPatternsScanImproved as PerformanceAntiPatternsScan
from scanners.gs3_step04_design_query_correctness import QueryCorrectnessScan
from scanners.gs3_step04_design_distributed_consistency import DistributedConsistencyScanImproved as DistributedConsistencyScan
from scanners.gs3_step04_design_architecture import ThemisArchitectureRulesScan
from scanners.gs3_step04_design_bridge_interface import ThemisBridgeInterfaceRulesScan
from scanners.gs3_step04_design_error_rules import ThemisDesignErrorRulesScanImproved as ThemisDesignErrorRulesScan
from scanners.gs3_step04_quality_doc_freshness import ThemisDocFreshnessRulesScan
from scanners.gs3_step04_quality_docs_markdown import ThemisDocsMarkdownRulesScan
from scanners.gs3_step04_quality_cpp_doxygen import ThemisCppDoxygenPolicyRulesScan
from scanners.gs3_step04_design_module_governance import ThemisModuleGovernanceRulesScan


class UniformFullScanner(BaseGapScanner):
    """Uniform phase 1-11 scanner with standardized output conversion."""

    PRIORITY = ScannerPriority.SEMANTIC
    ENABLED = True
    MAX_RUNTIME_SECONDS = 600
    CODE_EXTENSIONS = (".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx", ".c")
    TARGET_TOP_LEVEL_DIRS = ("src", "include", "tests", "benchmarks", "internal")
    EXCLUDED_DIR_NAMES = {
        ".git",
        ".venv",
        "build",
        "build-msvc-windows-release",
        "vcpkg",
        "vcpkg_installed",
        "vcpkg_installed_linux",
        "external",
        "third_party",
        "downloads",
        "tmp",
        "artifacts",
        "node_modules",
    }

    def __init__(self, scan_mode: str = "full", docs_doxygen: bool = False):
        super().__init__("Uniform Full Scanner", "4.0")
        self.scan_mode = scan_mode if scan_mode in {"fast", "full"} else "full"
        self.docs_doxygen = docs_doxygen

    def scan(self, source_dir: str) -> List[Gap]:
        gaps: List[Gap] = []
        self.source_path = Path(source_dir).resolve()
        self.repo_root = self._resolve_repo_root(self.source_path)
        output_dir = self.repo_root / "ai_working"
        output_dir.mkdir(parents=True, exist_ok=True)
        self._log(f"Start scan at {self.source_path}")
        self._log(f"Resolved repo root: {self.repo_root}")
        self._log(f"Mode: {self.scan_mode} (docs_doxygen={self.docs_doxygen})")

        # Modern phase 1 scanners (uniform local implementation)
        modern_phase1 = [
            ("phase1_braces_check", BracesCheckScanner()),
            ("phase1_namespace_unity_check", NamespaceUnityCheckScanner()),
            ("phase1_ai_simulation_stub_leak", SimulationStubLeakScanner()),
            ("phase1_ai_todo_productionlogic", TodoProductionlogicScanner()),
            ("phase1_ai_error_handling_consistency", ErrorHandlingConsistencyScanner()),
            ("phase1_ai_llm_prompt_injection", LlmPromptInjectionScanner()),
            ("phase1_ai_header_drift", HeaderDriftScanner()),
            ("phase1_memory_safety", MemorySafetyScanner()),
            ("phase1_error_handling", ErrorHandlingScanner()),
            ("phase1_thread_safety", ThreadSafetyScanner()),
            ("phase1_raii", ModernRAIIScanner()),
        ]

        for phase_key, scanner in modern_phase1:
            phase_start = time.perf_counter()
            try:
                modern_gaps = scanner.scan(str(self.source_path))
                gaps.extend(self._convert_iterable_result(modern_gaps, phase_key, "phase1_modern"))
                self._log(f"{phase_key}: +{len(modern_gaps or [])} findings in {time.perf_counter() - phase_start:.2f}s")
            except Exception:
                self._log(f"{phase_key}: failed after {time.perf_counter() - phase_start:.2f}s")
                continue

        # Classic phase 1-4 modules (module report format)
        classic_scope_supported = self.source_path in {self.repo_root, self.repo_root / "src"}
        classic_scanners = [
            ("security", SecurityGapScanner(str(self.repo_root))),
            ("memory", MemoryGapScanner(str(self.repo_root))),
            ("reliability", ReliabilityGapScanner(str(self.repo_root))),
            ("concurrency", ConcurrencyGapScanner(str(self.repo_root))),
            ("raii", RAIIGapScanner(str(self.repo_root))),
            ("container", ContainerGapScanner(str(self.repo_root))),
            ("platform", PlatformGapScanner(str(self.repo_root))),
            ("performance", PerformanceGapScanner(str(self.repo_root))),
        ]

        for phase_key, scanner in classic_scanners:
            phase_start = time.perf_counter()
            if not classic_scope_supported:
                self._log(f"classic_{phase_key}: skipped (scope {self.source_path.name} not supported)")
                continue
            try:
                result = scanner.run_full_scan(str(output_dir))
                gaps.extend(self._convert_classic_module_result(result, phase_key))
                self._log(f"classic_{phase_key}: processed in {time.perf_counter() - phase_start:.2f}s")
            except Exception as ex:
                self._log(f"classic_{phase_key}: failed after {time.perf_counter() - phase_start:.2f}s ({type(ex).__name__}: {ex})")
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
            phase_start = time.perf_counter()
            try:
                result = runner(scanner, self.source_path)
                gaps.extend(self._convert_iterable_result(result, phase_key, "phase5"))
                self._log(f"phase5_{phase_key}: +{len(result or [])} findings in {time.perf_counter() - phase_start:.2f}s")
            except Exception as ex:
                self._log(f"phase5_{phase_key}: failed after {time.perf_counter() - phase_start:.2f}s ({type(ex).__name__}: {ex})")
                continue

        cpp_files = self._collect_cpp_files(self.source_path)
        self.files_scanned = len(cpp_files)
        self._log(f"Collected {self.files_scanned} files for phase7+ scans")

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
            ("themis_architecture_rules", ThemisArchitectureRulesScan(str(self.repo_root))),
            ("themis_bridge_interface_rules", ThemisBridgeInterfaceRulesScan(str(self.repo_root))),
            ("themis_design_error_rules", ThemisDesignErrorRulesScan(str(self.repo_root))),
            ("themis_doc_freshness_rules", ThemisDocFreshnessRulesScan(str(self.repo_root))),
            ("themis_cpp_doxygen_policy_rules", ThemisCppDoxygenPolicyRulesScan(str(self.repo_root))),
            ("themis_module_governance_rules", ThemisModuleGovernanceRulesScan(str(self.repo_root))),
        ]

        if self.scan_mode == "full":
            phase7_10_scanners.append(
                ("themis_docs_markdown_rules", ThemisDocsMarkdownRulesScan(str(self.repo_root), run_doxygen=self.docs_doxygen))
            )
        else:
            self._log("phase7_10_themis_docs_markdown_rules: skipped in fast mode")

        for phase_key, scanner in phase7_10_scanners:
            phase_start = time.perf_counter()
            try:
                result = scanner.scan_files(cpp_files)
                gaps.extend(self._convert_iterable_result(result, phase_key, "phase7_10"))
                self._log(f"phase7_10_{phase_key}: +{len(result or [])} findings in {time.perf_counter() - phase_start:.2f}s")
            except Exception as ex:
                self._log(f"phase7_10_{phase_key}: failed after {time.perf_counter() - phase_start:.2f}s ({type(ex).__name__}: {ex})")
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
            phase_start = time.perf_counter()
            try:
                repo_result = scanner.scan_repository()
                phase_count = 0
                for module_items in repo_result.values():
                    phase_count += len(module_items or [])
                    gaps.extend(self._convert_iterable_result(module_items, phase_key, "phase11"))
                self._log(f"phase11_{phase_key}: +{phase_count} findings in {time.perf_counter() - phase_start:.2f}s")
            except Exception as ex:
                self._log(f"phase11_{phase_key}: failed after {time.perf_counter() - phase_start:.2f}s ({type(ex).__name__}: {ex})")
                continue

        try:
            phase_start = time.perf_counter()
            legacy_dup = LegacyDuplicationScan(str(self.source_path))
            dup_result = legacy_dup.scan_files(cpp_files)
            gaps.extend(self._convert_iterable_result(dup_result, "legacy_duplication", "phase11"))
            self._log(f"phase11_legacy_duplication: +{len(dup_result or [])} findings in {time.perf_counter() - phase_start:.2f}s")
        except Exception:
            self._log("phase11_legacy_duplication: failed")
            pass

        self._log(f"Completed scan with {len(gaps)} raw findings")
        return self.deduplicate(gaps)

    def _log(self, message: str) -> None:
        print(f"[UNIFORM] {message}")

    def _resolve_repo_root(self, source_path: Path) -> Path:
        """Best-effort repository root detection for scanners that expect <root>/src."""
        candidates = [source_path, source_path.parent, source_path.parent.parent]
        for candidate in candidates:
            try:
                if (candidate / "src").exists() and (candidate / "include").exists():
                    return candidate
            except Exception:
                continue
        return source_path

    def _run_path(self, scanner: Any, source_path: Path) -> List[Any]:
        return scanner.run_full_scan(source_path)

    def _run_str(self, scanner: Any, source_path: Path) -> List[Any]:
        return scanner.run_full_scan(str(source_path))

    def _collect_cpp_files(self, source_path: Path) -> List[Path]:
        if not source_path.exists():
            return []

        candidate_roots: List[Path] = []
        if source_path.is_file():
            return [source_path]

        # If scanner is invoked on repository root, collect only known code roots.
        if all((source_path / name).exists() for name in ("src", "include")):
            for name in self.TARGET_TOP_LEVEL_DIRS:
                root = source_path / name
                if root.exists() and root.is_dir():
                    candidate_roots.append(root)
        else:
            candidate_roots.append(source_path)

        files: List[Path] = []
        for root in candidate_roots:
            for ext in self.CODE_EXTENSIONS:
                files.extend(root.rglob(f"*{ext}"))

        unique_files: List[Path] = []
        seen: set[str] = set()
        for file_path in files:
            normalized_parts = {p.lower() for p in file_path.parts}
            if normalized_parts.intersection({d.lower() for d in self.EXCLUDED_DIR_NAMES}):
                continue
            key = str(file_path.resolve())
            if key in seen:
                continue
            seen.add(key)
            unique_files.append(file_path)

        return unique_files

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
            remediation=self._pick(item_dict, ["remediation"], default="Review finding and apply recommended module-specific fix."),
            context=self._pick(item_dict, ["context", "line_content", "snippet"], default=""),
            scanner=f"Uniform::{phase_key}",
            step=phase,
            impact_level=self._pick(item_dict, ["impact_level"], default=None),
            subsystem=self._pick(item_dict, ["subsystem"], default=None),
        )

    def _normalize_file_path(self, file_path: str) -> str:
        try:
            fp = Path(file_path)
            if fp.is_absolute():
                return str(fp.resolve().relative_to(self.source_path)).replace("\\", "/")
            if fp.exists():
                return str(fp.resolve().relative_to(self.source_path)).replace("\\", "/")

            # Some sub-scanners emit paths relative to module roots (e.g. concerns/...)
            # instead of repository roots. Try common lookup locations and promote to
            # repository-relative paths for stable aggregation.
            candidates: List[Path] = [
                self.source_path / fp,
                self.source_path / "src" / fp,
                self.source_path / "include" / fp,
                self.source_path / "tests" / fp,
                self.source_path / "benchmarks" / fp,
                self.source_path / "internal" / fp,
            ]

            # Handle module-relative paths such as concerns/... under src/*.
            src_root = self.source_path / "src"
            if src_root.exists():
                for module_dir in src_root.iterdir():
                    if module_dir.is_dir():
                        candidates.append(module_dir / fp)

            for candidate in candidates:
                if candidate.exists():
                    return str(candidate.resolve().relative_to(self.source_path)).replace("\\", "/")

            return str(fp).replace("\\", "/")
        except Exception:
            return str(file_path).replace("\\", "/")

    def _normalize_gap_type(self, item_dict: Dict[str, Any], phase_key: str) -> str:
        raw = self._pick(item_dict, ["gap_type", "type", "pattern", "category", "issue_type"], default=f"{phase_key}_gap")
        normalized = str(raw).replace(" ", "_").replace("-", "_").lower()

        type_aliases = {
            "pointer_arithmetic": "pointer_arithmetic_unbounded",
        }
        return type_aliases.get(normalized, normalized)

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
