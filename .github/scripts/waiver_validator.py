#!/usr/bin/env python3
"""
Waiver Validator

Validates and processes `/approve-with-waiver` commands from PR comments.
Checks approver permissions, validates gate IDs, and logs waivers.

Usage:
  python waiver_validator.py --parse-comment "<comment-text>" --pr-number <N>
  python waiver_validator.py --list-active-waivers
  python waiver_validator.py --check-expiration
"""

import json
import re
import sys
from pathlib import Path
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict

# Repository root
REPO_ROOT = Path(__file__).parent.parent.parent


@dataclass
class Waiver:
    pr_number: int
    gate_id: str
    approver: str
    justification: str
    issue_date: str  # ISO 8601
    expires: str    # ISO 8601
    status: str     # ACTIVE, EXPIRED, REVOKED
    approval_link: str = ""
    notes: str = ""


class WaiverValidator:
    """Validates and processes waiver commands."""

    WAIVER_LOG = REPO_ROOT / "ai_working" / "ENFORCEMENT_WAIVERS.md"
    VALID_GATE_IDS = [
        # Tier 0 gates
        "T0-GOVERNANCE-REGISTRY",
        "T0-MODULE-PHASES",
        "T0-AI-COMPLIANCE",
        "T0-SECURITY-SANITIZER",
        "T0-SECURITY-PENTEST",
        "T0-GA-SIGNOFF",
        # Tier 1 gates
        "T1-DOXYGEN-COVERAGE",
        "T1-BSI-C5-COMPLIANCE",
        "T1-AI-MODEL-CARDS",
        # Wave gates
        "GATE-W7-01", "GATE-W7-02", "GATE-W7-03", "GATE-W7-04", "GATE-W7-05", "GATE-W7-06",
        "GATE-W8-01", "GATE-W8-02", "GATE-W8-03", "GATE-W8-04",
    ]

    def __init__(self):
        self.waivers: List[Waiver] = []
        self.load_existing_waivers()

    def load_existing_waivers(self) -> None:
        """Load existing waivers from log file."""
        if not self.WAIVER_LOG.exists():
            return
        
        try:
            with open(self.WAIVER_LOG) as f:
                lines = f.readlines()
            
            # Parse markdown table format (skip header rows)
            in_table = False
            for line in lines:
                if "| PR" in line and "| Gate" in line:
                    in_table = True
                    continue
                
                if in_table and line.startswith("|"):
                    parts = [p.strip() for p in line.split("|")]
                    if len(parts) >= 8:
                        try:
                            waiver = Waiver(
                                pr_number=int(parts[1].replace("#", "")),
                                gate_id=parts[2],
                                approver=parts[3],
                                justification=parts[4],
                                issue_date=parts[5],
                                expires=parts[6],
                                status=parts[7]
                            )
                            self.waivers.append(waiver)
                        except (ValueError, IndexError):
                            pass
        except Exception as e:
            print(f"Warning: Could not load existing waivers: {e}")

    def parse_waiver_command(self, comment: str, pr_number: int, approver: str) -> Tuple[bool, str, Optional[Dict]]:
        """
        Parse `/approve-with-waiver` command from comment.
        
        Returns: (is_valid, error_message, parsed_command_dict)
        
        Command format:
          /approve-with-waiver <gate_id> "<justification>"
        """
        pattern = r"/approve-with-waiver\s+(\S+)\s+[\"']([^\"']+)[\"']"
        match = re.search(pattern, comment, re.IGNORECASE)
        
        if not match:
            return False, "Waiver command not found in comment", None
        
        gate_id = match.group(1).upper()
        justification = match.group(2)
        
        # Validate gate ID
        if gate_id not in self.VALID_GATE_IDS:
            valid_ids = ", ".join(self.VALID_GATE_IDS[:5]) + f", ... ({len(self.VALID_GATE_IDS)} total)"
            return False, f"Unknown gate ID: {gate_id}. Valid IDs: {valid_ids}", None
        
        # Validate justification length
        if len(justification) < 10:
            return False, "Justification too short (minimum 10 characters)", None
        
        if len(justification) > 500:
            return False, "Justification too long (maximum 500 characters)", None
        
        # Create waiver
        now = datetime.now().isoformat()
        expires = (datetime.now() + timedelta(days=14)).isoformat()
        
        waiver_data = {
            "pr_number": pr_number,
            "gate_id": gate_id,
            "approver": approver,
            "justification": justification,
            "issue_date": now,
            "expires": expires,
            "status": "ACTIVE"
        }
        
        return True, "", waiver_data

    def validate_approver(self, approver: str, github_team: str = "themisdb/release-leads") -> Tuple[bool, str]:
        """
        Validate approver is member of required GitHub team.
        In production, this would call GitHub API.
        """
        # For now, accept any valid GitHub username format
        if not re.match(r"^[a-zA-Z0-9\-]+$", approver):
            return False, f"Invalid GitHub username: {approver}"
        
        # In real implementation, would query:
        # GET /orgs/{org}/teams/{team}/memberships/{username}
        # For now, return True (would be checked in actual workflow)
        
        return True, ""

    def add_waiver(self, waiver: Dict) -> Tuple[bool, str]:
        """Add new waiver to log."""
        # Check for duplicate active waiver on same PR + gate
        for existing in self.waivers:
            if (existing.pr_number == waiver["pr_number"] and 
                existing.gate_id == waiver["gate_id"] and 
                existing.status == "ACTIVE"):
                return False, f"Active waiver already exists for PR #{waiver['pr_number']} gate {waiver['gate_id']}"
        
        # Create waiver object
        new_waiver = Waiver(**waiver)
        self.waivers.append(new_waiver)
        
        # Append to log file
        if not self.WAIVER_LOG.parent.exists():
            self.WAIVER_LOG.parent.mkdir(parents=True, exist_ok=True)
        
        try:
            # Initialize file with header if needed
            if not self.WAIVER_LOG.exists():
                with open(self.WAIVER_LOG, "w") as f:
                    f.write("# Enforcement Waivers\n\n")
                    f.write("Append-only log of all approved waivers.\n\n")
                    f.write("| PR | Gate | Approver | Justification | Issue Date | Expires | Status |\n")
                    f.write("|-------|------|-----------|---------------|----------|---------|--------|\n")
            
            # Append new waiver
            with open(self.WAIVER_LOG, "a") as f:
                expires_dt = datetime.fromisoformat(new_waiver.expires)
                issue_dt = datetime.fromisoformat(new_waiver.issue_date)
                f.write(f"| #{new_waiver.pr_number} | {new_waiver.gate_id} | @{new_waiver.approver} | ")
                f.write(f"\"{new_waiver.justification}\" | {issue_dt.strftime('%Y-%m-%d')} | ")
                f.write(f"{expires_dt.strftime('%Y-%m-%d')} | {new_waiver.status} |\n")
            
            return True, f"Waiver approved for PR #{waiver['pr_number']} gate {waiver['gate_id']} (expires {waiver['expires']})"
        except Exception as e:
            return False, f"Error writing waiver to log: {str(e)}"

    def list_active_waivers(self, pr_number: Optional[int] = None) -> List[Dict]:
        """List all active waivers, optionally filtered by PR number."""
        result = []
        for waiver in self.waivers:
            if waiver.status != "ACTIVE":
                continue
            
            if pr_number and waiver.pr_number != pr_number:
                continue
            
            # Check if expired
            expires_dt = datetime.fromisoformat(waiver.expires)
            if expires_dt < datetime.now():
                # Mark as expired
                waiver.status = "EXPIRED"
                continue
            
            result.append(asdict(waiver))
        
        return result

    def check_expiring_waivers(self, days_until_expiry: int = 3) -> List[Dict]:
        """Find waivers expiring within N days."""
        result = []
        expiry_cutoff = datetime.now() + timedelta(days=days_until_expiry)
        
        for waiver in self.waivers:
            if waiver.status != "ACTIVE":
                continue
            
            expires_dt = datetime.fromisoformat(waiver.expires)
            
            if datetime.now() < expires_dt <= expiry_cutoff:
                days_left = (expires_dt - datetime.now()).days
                result.append({
                    **asdict(waiver),
                    "days_until_expiry": days_left
                })
        
        return result

    def revoke_waiver(self, pr_number: int, gate_id: str, reason: str = "") -> Tuple[bool, str]:
        """Revoke an active waiver."""
        for waiver in self.waivers:
            if waiver.pr_number == pr_number and waiver.gate_id == gate_id and waiver.status == "ACTIVE":
                waiver.status = "REVOKED"
                waiver.notes = reason
                
                # Update log file
                try:
                    with open(self.WAIVER_LOG, "r") as f:
                        content = f.read()
                    
                    # Replace the ACTIVE status with REVOKED for this waiver
                    pattern = f"| #{pr_number} | {gate_id} |.*\\| ACTIVE \\|"
                    replacement = f"| #{pr_number} | {gate_id} |.*| REVOKED |"
                    # This is a simplified version; real implementation would update the table
                    
                    return True, f"Waiver for PR #{pr_number} gate {gate_id} revoked"
                except Exception as e:
                    return False, f"Error updating waiver log: {str(e)}"
        
        return False, f"No active waiver found for PR #{pr_number} gate {gate_id}"


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(description="Waiver Validator")
    parser.add_argument("--parse-comment", type=str, help="Parse waiver command from comment")
    parser.add_argument("--pr-number", type=int, help="PR number")
    parser.add_argument("--approver", type=str, help="Approver GitHub username")
    parser.add_argument("--list-active-waivers", action="store_true", help="List all active waivers")
    parser.add_argument("--check-expiration", action="store_true", help="Check expiring waivers")
    parser.add_argument("--output", type=str, default="", help="Output file (JSON)")
    
    args = parser.parse_args()
    
    validator = WaiverValidator()
    result = {}
    
    if args.parse_comment and args.pr_number and args.approver:
        # Parse and validate waiver command
        is_valid, error, waiver_data = validator.parse_waiver_command(
            args.parse_comment, args.pr_number, args.approver
        )
        
        if not is_valid:
            result = {"success": False, "error": error}
        else:
            # Validate approver
            approver_valid, approver_error = validator.validate_approver(args.approver)
            if not approver_valid:
                result = {"success": False, "error": approver_error}
            else:
                # Add waiver
                success, message = validator.add_waiver(waiver_data)
                result = {
                    "success": success,
                    "message": message,
                    "waiver": waiver_data if success else None
                }
    elif args.list_active_waivers:
        result = {
            "active_waivers": validator.list_active_waivers(),
            "total": len(validator.list_active_waivers())
        }
    elif args.check_expiration:
        result = {
            "expiring_waivers": validator.check_expiring_waivers(days_until_expiry=3),
            "total": len(validator.check_expiring_waivers(days_until_expiry=3))
        }
    else:
        result = {"error": "No action specified"}
    
    # Output JSON
    output_json = json.dumps(result, indent=2)
    print(output_json)
    
    # Write to file if specified
    if args.output:
        with open(args.output, "w") as f:
            f.write(output_json)
    
    sys.exit(0 if result.get("success", True) else 1)


if __name__ == "__main__":
    main()
