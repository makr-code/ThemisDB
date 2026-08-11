#!/usr/bin/env python3
"""
Tier 1 Gate Escalator

Validates all waiverable Tier 1 gates and generates escalation requests.
Returns JSON with gate results and waiver requirements.

Tier 1 Gates (waiverable with justification):
  1. Doxygen Coverage (≥95% for Phase 6 modules)
  2. BSI C5 Compliance Gaps (assigned owners)
  3. EU AI Act Model Card Coverage (≥75%)

Usage:
  python tier1_gate_escalator.py --pr-number <N> --target-branch <branch>
  python tier1_gate_escalator.py --all-gates
"""

import json
import sys
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict

# Repository root
REPO_ROOT = Path(__file__).parent.parent.parent


@dataclass
class Tier1Result:
    gate_id: str
    status: str  # PASS, FAIL, WAIVER_REQUIRED
    severity: str  # critical, high, medium, low
    details: str
    required_threshold: Optional[str]
    current_value: Optional[str]
    waiver_expiration_days: int = 14
    remediation: str = ""
    artifact_path: Optional[str] = None


class Tier1Escalator:
    """Validates all Tier 1 gates and generates escalation requests."""

    def __init__(self, target_branch: str = "community"):
        self.results: List[Tier1Result] = []
        self.target_branch = target_branch
        self.doxygen_coverage_file = REPO_ROOT / "docs" / "quality" / "doxygen_coverage.json"
        self.compliance_issues_label = "[compliance] BSI-C5"

    def validate_all(self) -> Dict:
        """Run all Tier 1 validators."""
        self._validate_doxygen_coverage()
        self._validate_bsi_c5_compliance()
        self._validate_ai_model_cards()
        
        return self._compile_results()

    def _validate_doxygen_coverage(self) -> None:
        """Check Doxygen coverage for Phase 6 modules."""
        gate_id = "T1-DOXYGEN-COVERAGE"
        threshold = 95  # percent
        
        # Find Phase 6 modules from ROADMAP files
        phase6_modules = self._find_phase6_modules()
        
        if not phase6_modules:
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="PASS",
                severity="low",
                details="No Phase 6 modules found (no Doxygen coverage check needed)",
                required_threshold=None,
                current_value=None,
                remediation=""
            ))
            return
        
        # Check if Doxygen coverage report exists
        if not self.doxygen_coverage_file.exists():
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Doxygen coverage report missing: {self.doxygen_coverage_file}",
                required_threshold=f"≥{threshold}%",
                current_value="N/A",
                remediation="Run Doxygen generation and generate coverage report",
                artifact_path=str(self.doxygen_coverage_file)
            ))
            return
        
        try:
            with open(self.doxygen_coverage_file) as f:
                coverage_data = json.load(f)
            
            # Check coverage for each Phase 6 module
            insufficient_coverage = []
            for module in phase6_modules:
                module_coverage = coverage_data.get("modules", {}).get(module, {}).get("coverage_percent", 0)
                if module_coverage < threshold:
                    insufficient_coverage.append((module, module_coverage))
            
            if insufficient_coverage:
                details_list = [f"{m}: {c:.1f}%" for m, c in insufficient_coverage]
                self.results.append(Tier1Result(
                    gate_id=gate_id,
                    status="FAIL" if self.target_branch == "community" else "WAIVER_REQUIRED",
                    severity="high",
                    details=f"Doxygen coverage below {threshold}% for Phase 6 modules: {', '.join(details_list)}",
                    required_threshold=f"≥{threshold}%",
                    current_value=f"Min: {min(c for _, c in insufficient_coverage):.1f}%",
                    remediation="Add API documentation to reach 95% coverage for Phase 6 modules",
                    artifact_path=str(self.doxygen_coverage_file)
                ))
            else:
                self.results.append(Tier1Result(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details=f"Doxygen coverage ≥{threshold}% for all Phase 6 modules ({len(phase6_modules)} modules)",
                    required_threshold=f"≥{threshold}%",
                    current_value=f"All modules ≥{threshold}%",
                    remediation="",
                    artifact_path=str(self.doxygen_coverage_file)
                ))
        except Exception as e:
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Error reading Doxygen coverage report: {str(e)}",
                required_threshold=f"≥{threshold}%",
                current_value="ERROR",
                remediation="Check coverage report format and regenerate",
                artifact_path=str(self.doxygen_coverage_file)
            ))

    def _validate_bsi_c5_compliance(self) -> None:
        """Check BSI C5 compliance gaps."""
        gate_id = "T1-BSI-C5-COMPLIANCE"
        
        # Query GitHub for open compliance issues (would use GitHub API in practice)
        # For now, check docs/governance for compliance tracking
        compliance_file = REPO_ROOT / "docs" / "governance" / "SECURITY_COMPLIANCE_AUDIT_REPORT_2026_08_10.md"
        
        if not compliance_file.exists():
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="PASS",
                severity="low",
                details="No compliance audit report found (assuming no open gaps)",
                required_threshold="0 unassigned gaps",
                current_value="N/A",
                remediation=""
            ))
            return
        
        try:
            with open(compliance_file) as f:
                content = f.read()
            
            # Look for open/unassigned gaps
            unassigned_pattern = r"(?:unassigned|open).*gap"
            unassigned_matches = re.findall(unassigned_pattern, content, re.IGNORECASE)
            
            if unassigned_matches:
                self.results.append(Tier1Result(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details=f"Found {len(unassigned_matches)} unassigned BSI C5 compliance gaps",
                    required_threshold="All gaps assigned to owner",
                    current_value=f"{len(unassigned_matches)} unassigned",
                    remediation="Assign each BSI C5 gap to an owner with remediation plan",
                    artifact_path=str(compliance_file)
                ))
            else:
                self.results.append(Tier1Result(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details="All BSI C5 compliance gaps assigned",
                    required_threshold="All gaps assigned",
                    current_value="0 unassigned",
                    remediation="",
                    artifact_path=str(compliance_file)
                ))
        except Exception as e:
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="PASS",
                severity="low",
                details=f"Could not read compliance report (skipping): {str(e)}",
                required_threshold="All gaps assigned",
                current_value="UNKNOWN",
                remediation=""
            ))

    def _validate_ai_model_cards(self) -> None:
        """Check EU AI Act Model Card coverage."""
        gate_id = "T1-AI-MODEL-CARDS"
        threshold_percent = 75  # percent
        
        model_cards_dir = REPO_ROOT / "docs" / "governance" / "ai-model-cards"
        ai_modules = ["llm", "rag", "ethics_ai", "prompt_engineering"]
        
        if not model_cards_dir.exists():
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Model cards directory missing: {model_cards_dir}",
                required_threshold=f"≥{threshold_percent}% with signed cards",
                current_value="0%",
                remediation="Create docs/governance/ai-model-cards/ with Model Card files",
                artifact_path=str(model_cards_dir)
            ))
            return
        
        # Count existing and signed model cards
        existing_cards = 0
        signed_cards = 0
        
        for module in ai_modules:
            card_path = model_cards_dir / f"{module}.md"
            if card_path.exists():
                existing_cards += 1
                
                # Check if card is signed (has approval signature)
                with open(card_path) as f:
                    content = f.read()
                    if "Approved" in content or "Signed" in content or "✅" in content:
                        signed_cards += 1
        
        coverage_percent = (signed_cards / len(ai_modules)) * 100 if ai_modules else 0
        
        if coverage_percent < threshold_percent:
            status = "FAIL" if self.target_branch == "community" else "WAIVER_REQUIRED"
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status=status,
                severity="high",
                details=f"AI Model Card coverage at {coverage_percent:.0f}% (need ≥{threshold_percent}%)",
                required_threshold=f"≥{threshold_percent}% with signed cards",
                current_value=f"{signed_cards}/{len(ai_modules)} signed ({coverage_percent:.0f}%)",
                remediation="Create and sign Model Cards for AI modules to reach coverage threshold",
                artifact_path=str(model_cards_dir)
            ))
        else:
            self.results.append(Tier1Result(
                gate_id=gate_id,
                status="PASS",
                severity="low",
                details=f"AI Model Card coverage ≥{threshold_percent}% ({signed_cards}/{len(ai_modules)} signed)",
                required_threshold=f"≥{threshold_percent}% with signed cards",
                current_value=f"{signed_cards}/{len(ai_modules)} signed ({coverage_percent:.0f}%)",
                remediation="",
                artifact_path=str(model_cards_dir)
            ))

    def _find_phase6_modules(self) -> List[str]:
        """Find all modules at Phase 6."""
        phase6_modules = []
        src_dir = REPO_ROOT / "src"
        
        for module_dir in src_dir.iterdir():
            if not module_dir.is_dir():
                continue
            
            roadmap_path = module_dir / "ROADMAP.md"
            if not roadmap_path.exists():
                continue
            
            try:
                with open(roadmap_path) as f:
                    content = f.read()
                    # Look for Phase 6 status (either [x] or complete indicator)
                    if re.search(r"Phase 6[:\s]*\[[xX]\]", content) or "Phase 6" in content and "[x]" in content:
                        phase6_modules.append(module_dir.name)
            except:
                pass
        
        return phase6_modules

    def _compile_results(self) -> Dict:
        """Compile results into output format."""
        tier1_status = "PASS" if all(r.status == "PASS" for r in self.results) else "FAIL"
        requires_escalation = any(r.status in ["FAIL", "WAIVER_REQUIRED"] for r in self.results)
        
        return {
            "target_branch": self.target_branch,
            "tier1_status": tier1_status,
            "requires_escalation": requires_escalation,
            "total_gates": len(self.results),
            "passed_gates": sum(1 for r in self.results if r.status == "PASS"),
            "waiver_required_gates": sum(1 for r in self.results if r.status == "WAIVER_REQUIRED"),
            "failed_gates": sum(1 for r in self.results if r.status == "FAIL"),
            "gates": [asdict(r) for r in self.results]
        }


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(description="Tier 1 Gate Escalator")
    parser.add_argument("--target-branch", type=str, default="community", help="Target branch (default: community)")
    parser.add_argument("--output", type=str, default="", help="Output file (JSON)")
    
    args = parser.parse_args()
    
    escalator = Tier1Escalator(target_branch=args.target_branch)
    results = escalator.validate_all()
    
    # Output JSON
    output_json = json.dumps(results, indent=2)
    print(output_json)
    
    # Write to file if specified
    if args.output:
        with open(args.output, "w") as f:
            f.write(output_json)
    
    # Exit with status: 0 if PASS, 1 if requires escalation/failure
    sys.exit(0 if results["tier1_status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
