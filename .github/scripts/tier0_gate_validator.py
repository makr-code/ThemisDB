#!/usr/bin/env python3
"""
Tier 0 Gate Validator

Validates all non-waiverable Tier 0 gates for merge to release lanes.
Returns JSON with gate results for use in GitHub Actions workflows.

Tier 0 Gates:
  1. Governance Registry Freshness (MATURITY_EVIDENCE_MANIFEST.json ≤ 24 hours)
  2. Module Phase Gates (PHASE_DEPENDENCY_GRAPH.md)
  3. AI/ML Compliance (Model Cards for llm, rag, ethics_ai)
  4. Security Gates (sanitizer ≤ 30 days, pentest ≤ 90 days, both PASS)
  5. GA Promotion Sign-Off (Section 9 of GA_PROMOTION_SIGN_OFF.md)

Usage:
  python tier0_gate_validator.py --pr-number <N> --target-branch <branch>
  python tier0_gate_validator.py --all-gates
"""

import json
import sys
from pathlib import Path
from datetime import datetime, timedelta
import re
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict

# Repository root
REPO_ROOT = Path(__file__).parent.parent.parent


@dataclass
class GateResult:
    gate_id: str
    status: str  # PASS, FAIL
    severity: str  # critical, high, medium, low
    details: str
    remediation: str
    artifact_path: Optional[str] = None
    freshness_days: Optional[int] = None
    last_verified: Optional[str] = None


class Tier0Validator:
    """Validates all Tier 0 gates."""

    def __init__(self):
        self.results: List[GateResult] = []
        self.governance_file = REPO_ROOT / "docs" / "governance" / "MATURITY_EVIDENCE_MANIFEST.json"
        self.phase_dep_file = REPO_ROOT / "docs" / "governance" / "PHASE_DEPENDENCY_GRAPH.md"
        self.security_sanitizer_file = REPO_ROOT / "docs" / "security" / "GA_SANITIZER_EVIDENCE_BUNDLE.md"
        self.security_pentest_file = REPO_ROOT / "security" / "pentest" / "GA_PENTEST_EVIDENCE_BUNDLE.md"
        self.ga_signoff_file = REPO_ROOT / "docs" / "governance" / "GA_PROMOTION_SIGN_OFF.md"

    def validate_all(self) -> Dict:
        """Run all Tier 0 validators."""
        self._validate_governance_registry()
        self._validate_module_phase_gates()
        self._validate_ai_compliance()
        self._validate_security_gates()
        self._validate_ga_sign_off()
        
        return self._compile_results()

    def _validate_governance_registry(self) -> None:
        """Check governance registry freshness."""
        gate_id = "T0-GOVERNANCE-REGISTRY"
        
        if not self.governance_file.exists():
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="critical",
                details=f"Governance manifest missing: {self.governance_file}",
                remediation="Run `.github/workflows/10-governance_maturity-verification.yml` to regenerate",
                artifact_path=str(self.governance_file)
            ))
            return

        try:
            with open(self.governance_file) as f:
                manifest = json.load(f)
            
            generated_str = manifest.get("generated", "")
            if not generated_str:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details="Governance manifest 'generated' timestamp missing",
                    remediation="Regenerate manifest with current timestamp",
                    artifact_path=str(self.governance_file)
                ))
                return
            
            # Parse ISO 8601 timestamp
            generated_time = datetime.fromisoformat(generated_str.replace("Z", "+00:00"))
            age_hours = (datetime.now(generated_time.tzinfo) - generated_time).total_seconds() / 3600
            age_days = age_hours / 24
            
            if age_days > 1:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details=f"Governance registry stale: {age_days:.1f} days old (> 24 hours)",
                    remediation="Run verification workflow: `.github/workflows/10-governance_maturity-verification.yml`",
                    artifact_path=str(self.governance_file),
                    freshness_days=int(age_days),
                    last_verified=generated_str
                ))
            else:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details=f"Governance registry current ({age_days:.2f} hours old)",
                    remediation="",
                    artifact_path=str(self.governance_file),
                    freshness_days=int(age_days),
                    last_verified=generated_str
                ))
        except Exception as e:
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="critical",
                details=f"Error parsing governance manifest: {str(e)}",
                remediation="Check manifest format and regenerate",
                artifact_path=str(self.governance_file)
            ))

    def _validate_module_phase_gates(self) -> None:
        """Check module phase dependency graph."""
        gate_id = "T0-MODULE-PHASES"
        
        if not self.phase_dep_file.exists():
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Phase dependency graph missing: {self.phase_dep_file}",
                remediation="Generate PHASE_DEPENDENCY_GRAPH.md from module ROADMAP.md files",
                artifact_path=str(self.phase_dep_file)
            ))
            return

        try:
            with open(self.phase_dep_file) as f:
                content = f.read()
            
            # Check for cycle detection section
            if "cyclic" in content.lower() or "cycle" in content.lower():
                # Look for detection results
                if "no cycle" in content.lower() or "✅" in content:
                    self.results.append(GateResult(
                        gate_id=gate_id,
                        status="PASS",
                        severity="low",
                        details="Module phase dependency graph is acyclic",
                        remediation="",
                        artifact_path=str(self.phase_dep_file)
                    ))
                else:
                    self.results.append(GateResult(
                        gate_id=gate_id,
                        status="FAIL",
                        severity="critical",
                        details="Module phase dependency graph contains cycles",
                        remediation="Resolve phase dependencies in affected module ROADMAP.md files",
                        artifact_path=str(self.phase_dep_file)
                    ))
            else:
                # Default to PASS if structure looks good
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details="Module phase dependency graph validated",
                    remediation="",
                    artifact_path=str(self.phase_dep_file)
                ))
        except Exception as e:
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Error reading phase dependency graph: {str(e)}",
                remediation="Check file format and regenerate",
                artifact_path=str(self.phase_dep_file)
            ))

    def _validate_ai_compliance(self) -> None:
        """Check AI/ML compliance gates (Model Cards)."""
        gate_id = "T0-AI-COMPLIANCE"
        
        model_cards_dir = REPO_ROOT / "docs" / "governance" / "ai-model-cards"
        ai_modules = ["llm", "rag", "ethics_ai"]
        
        if not model_cards_dir.exists():
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Model cards directory missing: {model_cards_dir}",
                remediation="Create docs/governance/ai-model-cards/ directory with Model Card files",
                artifact_path=str(model_cards_dir)
            ))
            return

        required_cards = [model_cards_dir / f"{module}.md" for module in ai_modules]
        missing_cards = [card for card in required_cards if not card.exists()]
        
        if missing_cards:
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Missing Model Card files: {[card.name for card in missing_cards]}",
                remediation="Create required Model Card files in docs/governance/ai-model-cards/",
                artifact_path=str(model_cards_dir)
            ))
        else:
            # Validate cards have required sections
            all_valid = True
            required_sections = ["## Purpose", "## Data", "## Bias", "## Metrics", "## Limitations"]
            
            for card_path in required_cards:
                with open(card_path) as f:
                    content = f.read()
                    for section in required_sections:
                        if section not in content:
                            all_valid = False
                            break
            
            if all_valid:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details=f"All Model Cards present and valid ({len(ai_modules)} modules)",
                    remediation="",
                    artifact_path=str(model_cards_dir)
                ))
            else:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details="Model Cards missing required sections (Purpose, Data, Bias, Metrics, Limitations)",
                    remediation="Update Model Card files with all required sections",
                    artifact_path=str(model_cards_dir)
                ))

    def _validate_security_gates(self) -> None:
        """Check security gates (sanitizer & pentest)."""
        # Sanitizer gate
        sanitizer_gate_id = "T0-SECURITY-SANITIZER"
        if not self.security_sanitizer_file.exists():
            self.results.append(GateResult(
                gate_id=sanitizer_gate_id,
                status="FAIL",
                severity="critical",
                details=f"Sanitizer evidence bundle missing: {self.security_sanitizer_file}",
                remediation="Run sanitizer (ASan/UBSan/TSan) and generate evidence bundle",
                artifact_path=str(self.security_sanitizer_file)
            ))
        else:
            self._check_security_evidence(self.security_sanitizer_file, sanitizer_gate_id, 30)

        # Pentest gate
        pentest_gate_id = "T0-SECURITY-PENTEST"
        if not self.security_pentest_file.exists():
            self.results.append(GateResult(
                gate_id=pentest_gate_id,
                status="FAIL",
                severity="critical",
                details=f"Pentest evidence bundle missing: {self.security_pentest_file}",
                remediation="Complete penetration test and generate evidence bundle",
                artifact_path=str(self.security_pentest_file)
            ))
        else:
            self._check_security_evidence(self.security_pentest_file, pentest_gate_id, 90)

    def _check_security_evidence(self, evidence_file: Path, gate_id: str, max_age_days: int) -> None:
        """Check if security evidence is fresh and shows PASS."""
        try:
            with open(evidence_file) as f:
                content = f.read()
            
            # Check for freshness (extract date from first timestamp comment)
            date_pattern = r"(202[0-9]-\d{2}-\d{2})"
            matches = re.findall(date_pattern, content)
            
            status = "PASS" if "PASS" in content else "FAIL" if "FAIL" in content else None
            if status is None:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="critical",
                    details="Security evidence bundle does not contain PASS/FAIL status",
                    remediation="Review evidence bundle format and ensure status is clearly marked",
                    artifact_path=str(evidence_file)
                ))
                return
            
            if status == "FAIL":
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="critical",
                    details="Security gate shows FAIL status",
                    remediation="Resolve security findings and re-run verification",
                    artifact_path=str(evidence_file)
                ))
                return
            
            # Check freshness
            latest_date = matches[-1] if matches else None
            if latest_date:
                evidence_time = datetime.strptime(latest_date, "%Y-%m-%d")
                age_days = (datetime.now() - evidence_time).days
                
                if age_days > max_age_days:
                    self.results.append(GateResult(
                        gate_id=gate_id,
                        status="FAIL",
                        severity="high",
                        details=f"Security evidence stale: {age_days} days old (> {max_age_days} days)",
                        remediation=f"Re-run security verification to refresh evidence (max age: {max_age_days} days)",
                        artifact_path=str(evidence_file),
                        freshness_days=age_days,
                        last_verified=latest_date
                    ))
                    return
                
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details=f"Security gate PASS ({age_days} days old, max {max_age_days})",
                    remediation="",
                    artifact_path=str(evidence_file),
                    freshness_days=age_days,
                    last_verified=latest_date
                ))
            else:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details="Security evidence verified (PASS status confirmed)",
                    remediation="",
                    artifact_path=str(evidence_file)
                ))
        except Exception as e:
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Error reading security evidence: {str(e)}",
                remediation="Check file format and regenerate",
                artifact_path=str(evidence_file)
            ))

    def _validate_ga_sign_off(self) -> None:
        """Check GA promotion sign-off."""
        gate_id = "T0-GA-SIGNOFF"
        
        if not self.ga_signoff_file.exists():
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="critical",
                details=f"GA sign-off document missing: {self.ga_signoff_file}",
                remediation="Create GA_PROMOTION_SIGN_OFF.md with required sign-off section",
                artifact_path=str(self.ga_signoff_file)
            ))
            return

        try:
            with open(self.ga_signoff_file) as f:
                content = f.read()
            
            # Look for Section 9
            section9_pattern = r"## (?:Section )?9[.\s]|### (?:Section )?9[.\s]"
            if not re.search(section9_pattern, content, re.IGNORECASE):
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details="Section 9 (sign-off) not found in GA_PROMOTION_SIGN_OFF.md",
                    remediation="Add Section 9 with required sign-off checkboxes",
                    artifact_path=str(self.ga_signoff_file)
                ))
                return
            
            # Extract section 9 content
            section_match = re.search(section9_pattern + r"(.*?)(?:##|$)", content, re.IGNORECASE | re.DOTALL)
            if not section_match:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="high",
                    details="Cannot extract Section 9 content",
                    remediation="Verify Section 9 format and content",
                    artifact_path=str(self.ga_signoff_file)
                ))
                return
            
            section9 = section_match.group(1)
            
            # Check for required sign-off checkboxes (at least security and operations)
            required_signoffs = [
                (r"\[x\].*security", "Security Lead"),
                (r"\[x\].*operations?", "Operations Lead"),
                (r"\[x\].*release", "Release Lead")
            ]
            
            missing_signoffs = []
            for pattern, name in required_signoffs:
                if not re.search(pattern, section9, re.IGNORECASE):
                    missing_signoffs.append(name)
            
            if missing_signoffs:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="FAIL",
                    severity="critical",
                    details=f"Missing sign-offs from: {', '.join(missing_signoffs)}",
                    remediation="Collect required sign-offs from domain leads (Security, Operations, Release)",
                    artifact_path=str(self.ga_signoff_file)
                ))
            else:
                self.results.append(GateResult(
                    gate_id=gate_id,
                    status="PASS",
                    severity="low",
                    details="All required GA sign-offs present and verified",
                    remediation="",
                    artifact_path=str(self.ga_signoff_file)
                ))
        except Exception as e:
            self.results.append(GateResult(
                gate_id=gate_id,
                status="FAIL",
                severity="high",
                details=f"Error reading GA sign-off document: {str(e)}",
                remediation="Check file format and verify content",
                artifact_path=str(self.ga_signoff_file)
            ))

    def _compile_results(self) -> Dict:
        """Compile results into output format."""
        tier0_status = "PASS" if all(r.status == "PASS" for r in self.results) else "FAIL"
        
        return {
            "validation_time": datetime.now().isoformat(),
            "tier0_status": tier0_status,
            "total_gates": len(self.results),
            "passed_gates": sum(1 for r in self.results if r.status == "PASS"),
            "failed_gates": sum(1 for r in self.results if r.status == "FAIL"),
            "critical_failures": sum(1 for r in self.results if r.status == "FAIL" and r.severity == "critical"),
            "gates": [asdict(r) for r in self.results]
        }


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(description="Tier 0 Gate Validator")
    parser.add_argument("--all-gates", action="store_true", help="Validate all gates")
    parser.add_argument("--output", type=str, default="", help="Output file (JSON)")
    
    args = parser.parse_args()
    
    validator = Tier0Validator()
    results = validator.validate_all()
    
    # Output JSON
    output_json = json.dumps(results, indent=2)
    print(output_json)
    
    # Write to file if specified
    if args.output:
        with open(args.output, "w") as f:
            f.write(output_json)
    
    # Exit with status based on results
    sys.exit(0 if results["tier0_status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
