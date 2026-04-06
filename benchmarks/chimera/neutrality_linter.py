"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            neutrality_linter.py                               ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     379                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
CHIMERA Neutrality Linter

Checks CHIMERA files for vendor-specific references and neutrality violations.

Configuration can be loaded from neutrality_linter_config.yaml if present.
"""

import re
import sys
from pathlib import Path
from typing import List, Dict
from dataclasses import dataclass
from enum import Enum


# Try to load configuration from YAML file if available
def load_config():
    """Load configuration from YAML file if available"""
    try:
        import yaml
        config_path = Path(__file__).parent / "neutrality_linter_config.yaml"
        if config_path.exists():
            with open(config_path) as f:
                return yaml.safe_load(f)
    except ImportError:
        pass  # yaml not available
    except Exception as e:
        print(f"Warning: Could not load config: {e}")
    return None


CONFIG = load_config()


class ViolationLevel(str, Enum):
    """Severity levels for neutrality violations"""
    CRITICAL = "CRITICAL"
    MAJOR = "MAJOR"
    MINOR = "MINOR"
    WARNING = "WARNING"


@dataclass
class Violation:
    file: str
    line: int
    column: int
    level: str
    message: str
    context: str


# Vendor names to detect (case-insensitive)
# Can be overridden by neutrality_linter_config.yaml
VENDOR_NAMES = (CONFIG.get('vendor_names', []) if CONFIG else []) or [
    "themisdb",
    "oracle",
    "microsoft",
    "amazon",
    "google",
    "mongodb",
    "postgresql",
    "mysql",
    "redis",
    "elasticsearch",
    "neo4j",
    "cassandra",
    "cockroachdb",
    "yugabyte",
    "fauna",
    "snowflake",
    "databricks",
    "pinecone",
    "weaviate",
    "milvus",
    "qdrant"
]

# Marketing terms to detect
# Can be overridden by neutrality_linter_config.yaml
MARKETING_TERMS = (CONFIG.get('marketing_terms', []) if CONFIG else []) or [
    "enterprise",
    "professional",
    "ultimate",
    "premium",
    "platinum",
    "gold",
    "best",
    "winner",
    "superior",
    "leading",
    "recommended",
    "preferred"
]

# Allowed exceptions (contexts where vendor names are OK)
# Can be overridden by neutrality_linter_config.yaml
ALLOWED_CONTEXTS = (CONFIG.get('allowed_contexts', []) if CONFIG else []) or [
    "PostgreSQLAdapter",  # Protocol names
    "MongoDBAdapter",
    "MySQLAdapter",
    "RedisAdapter",
    "Neo4jAdapter",
    "CassandraAdapter",
    "ElasticsearchAdapter",
    "# Example:",  # Documentation examples showing what NOT to do
    "# ❌",
    "# DON'T",
    "# Bad",
    "# Wrong",
    "## Prohibited:",
    "### ❌ DON'T",
    "### Prohibited",
    "### Prohibited Terms",
    "**Prohibited:**"
]


class NeutralityLinter:
    """Linter for checking CHIMERA neutrality compliance"""
    
    def __init__(self, root_path: Path):
        self.root_path = root_path
        self.violations: List[Violation] = []
    
    def check_file(self, file_path: Path) -> None:
        """Check a single file for violations"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            for line_num, line in enumerate(lines, start=1):
                self.check_line(file_path, line_num, line)
                
        except Exception as e:
            print(f"Error checking {file_path}: {e}")
    
    def check_line(self, file_path: Path, line_num: int, line: str) -> None:
        """Check a single line for violations"""
        
        # Check for vendor names
        for vendor in VENDOR_NAMES:
            pattern = re.compile(rf'\b{vendor}\b', re.IGNORECASE)
            matches = pattern.finditer(line)
            
            for match in matches:
                # Check if this specific match is in an allowed context
                matched_text = match.group()
                start_pos = match.start()
                
                # Check if vendor name is part of a URI scheme (e.g., postgresql:// or mongodb://)
                if start_pos + len(matched_text) < len(line):
                    if line[start_pos + len(matched_text):start_pos + len(matched_text) + 3] == "://":
                        # This is a URI scheme, which is allowed for standard protocols
                        continue
                
                # Check if vendor name is part of an allowed context pattern
                is_allowed = False
                for ctx in ALLOWED_CONTEXTS:
                    # Check if the context pattern appears near the matched vendor name
                    if ctx in line:
                        # For adapter names, check if vendor is part of the adapter name
                        if "Adapter" in ctx and matched_text.lower() in ctx.lower():
                            is_allowed = True
                            break
                        # For documentation markers, check if they appear on the line
                        elif ctx.startswith("#") or ctx.startswith("**") or ctx.startswith("###"):
                            is_allowed = True
                            break
                
                if is_allowed:
                    continue
                
                # Determine severity based on context
                level = self._determine_level(file_path, line, vendor)
                
                self.violations.append(Violation(
                    file=str(file_path.relative_to(self.root_path)),
                    line=line_num,
                    column=match.start() + 1,
                    level=level,
                    message=f"Vendor name detected: '{vendor}'",
                    context=line.strip()
                ))
        
        # Check for marketing terms
        for term in MARKETING_TERMS:
            pattern = re.compile(rf'\b{term}\b', re.IGNORECASE)
            matches = pattern.finditer(line)
            
            for match in matches:
                # Lower severity for marketing terms
                self.violations.append(Violation(
                    file=str(file_path.relative_to(self.root_path)),
                    line=line_num,
                    column=match.start() + 1,
                    level=ViolationLevel.MAJOR,
                    message=f"Marketing term detected: '{term}'",
                    context=line.strip()
                ))
    
    def _determine_level(self, file_path: Path, line: str, vendor: str) -> str:
        """Determine severity level based on context"""
        
        # Critical in system IDs and adapter names
        if 'system_id' in line or 'adapter_name' in line:
            return ViolationLevel.CRITICAL
        
        # Critical in configuration files
        if file_path.suffix in ['.yaml', '.yml', '.toml', '.json']:
            return ViolationLevel.CRITICAL
        
        # Major in code
        if file_path.suffix in ['.py', '.cpp', '.h', '.js', '.ts']:
            return ViolationLevel.MAJOR
        
        # Major in documentation
        if file_path.suffix in ['.md', '.rst', '.txt']:
            # But only warning in references/citations
            if 'reference' in line.lower() or 'citation' in line.lower():
                return ViolationLevel.WARNING
            return ViolationLevel.MAJOR
        
        return ViolationLevel.MINOR
    
    def scan_directory(self, patterns: List[str]) -> None:
        """Scan directory for files matching patterns"""
        for pattern in patterns:
            for file_path in self.root_path.rglob(pattern):
                # Skip certain directories and files
                if any(part in file_path.parts for part in ['.git', 'node_modules', '__pycache__', 'venv']):
                    continue
                
                # Skip the linter config file itself
                if file_path.name == 'neutrality_linter_config.yaml':
                    continue
                
                self.check_file(file_path)
    
    def report(self) -> Dict[str, int]:
        """Generate report of violations"""
        counts = {
            ViolationLevel.CRITICAL.value: 0,
            ViolationLevel.MAJOR.value: 0,
            ViolationLevel.MINOR.value: 0,
            ViolationLevel.WARNING.value: 0
        }
        
        # Sort violations by level and file
        sorted_violations = sorted(
            self.violations,
            key=lambda v: (
                [ViolationLevel.CRITICAL.value, ViolationLevel.MAJOR.value, 
                 ViolationLevel.MINOR.value, ViolationLevel.WARNING.value].index(v.level),
                v.file,
                v.line
            )
        )
        
        current_file = None
        for violation in sorted_violations:
            counts[violation.level] += 1
            
            # Print file header if changed
            if violation.file != current_file:
                print(f"\n📄 {violation.file}")
                current_file = violation.file
            
            # Print violation
            icon = {
                ViolationLevel.CRITICAL.value: "🔴",
                ViolationLevel.MAJOR.value: "🟠",
                ViolationLevel.MINOR.value: "🟡",
                ViolationLevel.WARNING.value: "⚠️"
            }[violation.level]
            
            print(f"  {icon} Line {violation.line}:{violation.column} [{violation.level}] {violation.message}")
            print(f"     {violation.context[:100]}")
        
        return counts
    
    def summary(self, counts: Dict[str, int]) -> int:
        """Print summary and return exit code"""
        print("\n" + "=" * 70)
        print("CHIMERA NEUTRALITY LINTER SUMMARY")
        print("=" * 70)
        
        total = sum(counts.values())
        
        print(f"\n📊 Total violations: {total}")
        print(f"   🔴 Critical: {counts[ViolationLevel.CRITICAL.value]}")
        print(f"   🟠 Major:    {counts[ViolationLevel.MAJOR.value]}")
        print(f"   🟡 Minor:    {counts[ViolationLevel.MINOR.value]}")
        print(f"   ⚠️  Warning: {counts[ViolationLevel.WARNING.value]}")
        
        # Determine exit code
        if counts[ViolationLevel.CRITICAL.value] > 0:
            print("\n❌ FAILED: Critical violations found")
            return 2
        elif counts[ViolationLevel.MAJOR.value] > 0:
            print("\n⚠️  WARNING: Major violations found")
            return 1
        elif counts[ViolationLevel.MINOR.value] > 0:
            print("\n✓ PASSED with minor issues")
            return 0
        else:
            print("\n✅ PASSED: No violations found")
            return 0


def main():
    """Main entry point"""
    print("🔬 CHIMERA Neutrality Linter")
    print("=" * 70)
    
    # Get CHIMERA directory
    chimera_dir = Path(__file__).parent
    
    print(f"\n📂 Scanning: {chimera_dir}")
    print(f"   Checking for vendor-specific references...")
    
    # Create linter
    linter = NeutralityLinter(chimera_dir)
    
    # Scan files
    patterns = [
        "*.py",
        "*.md",
        "*.yaml",
        "*.yml",
        "*.toml",
        "*.json",
        "*.html"
    ]
    
    linter.scan_directory(patterns)
    
    # Report results
    counts = linter.report()
    exit_code = linter.summary(counts)
    
    print("\n" + "=" * 70)
    print("💡 Tips:")
    print("   - Use generic system identifiers (system_a, system_b)")
    print("   - Use protocol names for adapters (PostgreSQLAdapter)")
    print("   - Avoid marketing terms (best, winner, superior)")
    print("   - See NEUTRALITY_STYLEGUIDE.md for details")
    print("=" * 70)
    
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
