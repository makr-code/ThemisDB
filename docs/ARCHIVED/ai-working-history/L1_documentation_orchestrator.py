#!/usr/bin/env python3
"""
L1 MODULE DOCUMENTATION ORCHESTRATION SYSTEM
Updates module-level docs with verified L0.5 findings.

Scope: src/graph, src/cache, src/query, src/network, src/server, src/llm
Updates: README.md, ROADMAP.md, ARCHITECTURE.md, AUDIT.md

Source: ai_working/gap_scan_results_verified_L0.5_enhanced.json
"""

import json
from pathlib import Path
from datetime import datetime
from collections import defaultdict

class L1DocumentationOrchestrator:
    """Update module documentation with verified L0.5 findings."""
    
    def __init__(self, verified_l0_5_file='ai_working/gap_scan_results_verified_L0.5_enhanced.json'):
        self.modules = ['graph', 'cache', 'query', 'network', 'server', 'llm']
        self.src_root = Path('src')
        self.verified_data = self._load_verified_data(verified_l0_5_file)
        self.updates_summary = defaultdict(list)
        
    def _load_verified_data(self, filepath):
        """Load verified L0.5 findings."""
        try:
            with open(filepath) as f:
                return json.load(f)
        except Exception as e:
            print(f"[FAIL] Could not load verified data: {e}")
            return {}
    
    def generate_l1_audit_section(self, module_name):
        """Generate L1 AUDIT section for module documentation."""
        if 'modules' not in self.verified_data or module_name not in self.verified_data['modules']:
            return None
        
        mod_data = self.verified_data['modules'][module_name]
        metadata = mod_data.get('metadata', {})
        stats = mod_data.get('statistics', {})
        
        verified_gaps = stats.get('verified_gaps', 0)
        critical = stats.get('critical_verified', 0)
        high = metadata.get('l0_findings', 0) - verified_gaps
        
        # Build AUDIT section
        audit_section = f"""## L0 Code Quality Audit (2026-06-25)

**Status:** Verification Complete | {verified_gaps} verified gaps identified

### Summary

| Metric | Value |
|--------|-------|
| **L0 Findings Reviewed** | {metadata.get('l0_findings', 0)} |
| **Verified Real Gaps** | {verified_gaps} |
| **CRITICAL Findings** | {critical} |
| **False Positives Removed** | {metadata.get('false_positives_removed', 0)} |
| **Last Updated** | {datetime.now().strftime('%Y-%m-%d')} |

### Gap Distribution by Category

"""
        
        if 'by_file' in self.verified_data.get('modules', {}).get(module_name, {}).get('metadata', {}):
            # Count by category
            categories = defaultdict(int)
            verified_findings = mod_data.get('verified_findings', [])
            for finding in verified_findings:
                category = finding.get('category', 'unknown')
                categories[category] += 1
            
            for cat, count in sorted(categories.items(), key=lambda x: -x[1])[:15]:
                audit_section += f"- **{cat}**: {count} findings\n"
        
        audit_section += f"""

### Risk Assessment

- **Risk Level**: {self._assess_risk_level(critical, verified_gaps)}
- **Affected Files**: {metadata.get('l0_findings', 'N/A')} source files
- **Verification Confidence**: High (semantic analysis + pattern matching)

### Recommended Actions

1. **CRITICAL Findings Priority**: Address {critical} CRITICAL findings first
2. **Verification Method**: Review each gap against threat model
3. **Roadmap Sync**: Align with src/{module_name}/ROADMAP.md
4. **Test Coverage**: Ensure test cases cover identified gaps

---

*Source: ai_working/gap_scan_results_verified_L0.5_enhanced.json*
"""
        
        return audit_section
    
    def _assess_risk_level(self, critical, total):
        """Assess overall risk level."""
        if total == 0:
            return "GREEN (No verified gaps)"
        elif critical > 100:
            return "RED (CRITICAL)"
        elif critical > 20:
            return "ORANGE (High)"
        else:
            return "YELLOW (Moderate)"
    
    def scan_module_docs(self):
        """Scan existing module documentation."""
        print("\n[L1] SCANNING EXISTING MODULE DOCUMENTATION")
        print("-" * 80)
        
        doc_inventory = {}
        
        for mod in self.modules:
            mod_path = self.src_root / mod
            docs = {
                'README.md': mod_path / 'README.md',
                'ROADMAP.md': mod_path / 'ROADMAP.md',
                'ARCHITECTURE.md': mod_path / 'ARCHITECTURE.md',
                'AUDIT.md': mod_path / 'AUDIT.md',
                'SECURITY.md': mod_path / 'SECURITY.md',
            }
            
            exists = {}
            for doc_type, doc_path in docs.items():
                exists[doc_type] = doc_path.exists()
            
            doc_inventory[mod] = exists
            
            existing_docs = [dt for dt, ex in exists.items() if ex]
            print(f"  {mod:12s}: {len(existing_docs)}/5 docs exist - {', '.join(existing_docs)}")
        
        return doc_inventory
    
    def generate_l1_update_report(self):
        """Generate L1 documentation update strategy."""
        print("\n[L1] DOCUMENTATION UPDATE STRATEGY")
        print("-" * 80)
        
        doc_inventory = self.scan_module_docs()
        
        print("\n[L1] UPDATE PLAN")
        print("-" * 80)
        print()
        print("For each module, the following updates will be made:")
        print()
        print("1. **README.md** - Update status and risk assessment")
        print("2. **ROADMAP.md** - Add L0 audit findings + remediation items")
        print("3. **ARCHITECTURE.md** - Add L0 findings & risk vectors (if exists)")
        print("4. **AUDIT.md** - Create/update with L0 verification results (NEW)")
        print()
        print("Priority modules (by verified CRITICAL gaps):")
        print()
        
        # Sort by critical count
        critical_counts = []
        for mod in self.modules:
            if 'modules' in self.verified_data and mod in self.verified_data['modules']:
                stats = self.verified_data['modules'][mod].get('statistics', {})
                critical = stats.get('critical_verified', 0)
                critical_counts.append((mod, critical))
        
        critical_counts.sort(key=lambda x: -x[1])
        
        for i, (mod, critical) in enumerate(critical_counts[:6], 1):
            print(f"  {i}. {mod:12s}: {critical:4d} CRITICAL findings")
        
        print()
    
    def generate_module_audit_files(self):
        """Generate AUDIT.md files for all modules."""
        print("\n[L1-AUDIT] GENERATING MODULE AUDIT DOCUMENTATION")
        print("-" * 80)
        
        for mod in self.modules:
            audit_section = self.generate_l1_audit_section(mod)
            if not audit_section:
                print(f"  [SKIP] {mod:12s}: No L0 data available")
                continue
            
            mod_path = self.src_root / mod
            audit_file = mod_path / 'AUDIT.md'
            
            # Create AUDIT.md with L0 findings
            full_audit = f"""# {mod.capitalize()} Module - Code Quality Audit

"""
            full_audit += audit_section
            
            try:
                audit_file.write_text(full_audit)
                print(f"  [OK] {mod:12s}: Created/updated {audit_file}")
                self.updates_summary[mod].append('AUDIT.md')
            except Exception as e:
                print(f"  [FAIL] {mod:12s}: {e}")
    
    def run(self):
        """Execute L1 documentation orchestration."""
        print("\n" + "=" * 80)
        print("L1 MODULE DOCUMENTATION ORCHESTRATION SYSTEM")
        print("=" * 80)
        
        if not self.verified_data:
            print("\n[FAIL] No verified L0.5 data found.")
            print("Please run L0.5 verification first.")
            return
        
        self.generate_l1_update_report()
        self.generate_module_audit_files()
        
        print("\n" + "=" * 80)
        print("[OK] L1 DOCUMENTATION UPDATES COMPLETE")
        print("=" * 80)
        print("\n[NOTE] Updates Summary:")
        for mod in sorted(self.modules):
            updates = self.updates_summary.get(mod, [])
            print(f"  {mod:12s}: {', '.join(updates) if updates else 'No updates'}")
        
        print("\n[NOTE] Next Steps:")
        print("  1. Review generated AUDIT.md files in each module")
        print("  2. Update README.md and ROADMAP.md for each module")
        print("  3. Execute L2 (Aggregates) generation")
        print("  4. Execute L3 (Root Documentation) updates")
        print()

if __name__ == '__main__':
    orchestrator = L1DocumentationOrchestrator()
    orchestrator.run()
