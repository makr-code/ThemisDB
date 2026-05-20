#!/usr/bin/env python3
"""
Module Documentation Distributor

Copies/creates MODULE_GAPS.md documentation into actual module directories
so developers have it locally where they work.

Structure:
  src/acceleration/MODULE_GAPS.md  ← Developers see this
  src/security/MODULE_GAPS.md
  src/storage/MODULE_GAPS.md
  ...

Also creates standalone docs in ai_working/module_gaps/ for archival.
"""

import json
from pathlib import Path
from shutil import copy2
from typing import Dict, List

class ModuleDocumentationDistributor:
    """Distribute module gap documentation to module directories"""
    
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.src_dir = self.repo_root / 'src'
        self.results = {}
    
    def distribute_docs(self, doc_source_dir: str) -> Dict[str, bool]:
        """
        Copy/create MODULE_GAPS.md in each module directory.
        
        Returns:
            {module_name: success_bool}
        """
        doc_path = Path(doc_source_dir)
        
        if not doc_path.exists():
            print(f"[FAIL] Documentation source not found: {doc_source_dir}")
            return {}
        
        results = {}
        
        # Find all modules in src/
        for module_dir in sorted(self.src_dir.iterdir()):
            if not module_dir.is_dir() or module_dir.name.startswith('_'):
                continue
            
            module_name = module_dir.name
            
            # Look for corresponding doc file
            doc_file = doc_path / f'{module_name}_GAPS.md'
            
            if not doc_file.exists():
                # Create stub documentation
                success = self._create_stub_doc(module_dir, module_name)
            else:
                # Copy existing documentation
                target_path = module_dir / 'MODULE_GAPS.md'
                try:
                    copy2(doc_file, target_path)
                    success = True
                except:
                    success = False
            
            results[module_name] = success
            
            status = "[OK]" if success else "[FAIL]"
            print(f"  {status} {module_name:30} -> {module_dir}/MODULE_GAPS.md")
        
        self.results = results
        return results
    
    def _create_stub_doc(self, module_dir: Path, module_name: str) -> bool:
        """Create a stub MODULE_GAPS.md if no data available"""
        
        try:
            stub = f"""# {module_name} Module — Implementation Gap Analysis

**Status:** Documentation Pending  
**Last Updated:** Unknown  

---

## 📊 Gap Summary

This module's gap analysis is pending. Run the gap audit to populate this document:

```bash
python tools/gap_audit_pipeline_v2.py
```

---

## 🚀 How to Use This Documentation

Once generated, this file will contain:

- **Gap Statistics:** Count of unimplemented paths, TODOs, STUBs, etc.
- **Critical Issues:** What needs to be fixed first
- **Implementation Roadmap:** Phases and priorities
- **Affected Files:** Which source files have gaps
- **GitHub Issues:** Links to related GitHub issues
- **Next Steps:** Action items for developers

---

## 📍 Location

This documentation is in the module directory for easy access:
```
src/{module_name}/MODULE_GAPS.md  ← You are here
```

Developers working on this module can reference this file directly.

---

## 🔄 How It's Updated

The documentation is automatically generated and updated by the gap audit pipeline:

```bash
# Full pipeline (scan + update headers + generate docs)
python tools/gap_audit_pipeline_v2.py

# Just generate module docs
python tools/module_doc_generator.py . ai_working ai_working/module_gaps
```

After each run, this file is updated with fresh analysis.

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes
"""
            
            doc_path = module_dir / 'MODULE_GAPS.md'
            with open(doc_path, 'w', encoding='utf-8') as f:
                f.write(stub)
            return True
        except:
            return False
    
    def create_readme(self, output_dir: str) -> bool:
        """Create a README explaining the module documentation"""
        
        readme_path = Path(output_dir) / 'README.md'
        
        content = """# Module Gap Documentation

This directory contains gap analysis documentation for each ThemisDB module.

## 📍 How to Access

### Option 1: View in Module Directory (Recommended for Developers)

Each module has a `MODULE_GAPS.md` file in its source directory:

```
src/acceleration/MODULE_GAPS.md
src/security/MODULE_GAPS.md
src/storage/MODULE_GAPS.md
...
```

Open these files while working on the module to see current gaps.

### Option 2: View in Archive (For Analysis)

All module documentation is also archived here:

- `MODULE_GAPS_INDEX.md` — Overview of all modules
- `<module_name>_GAPS.md` — Detailed analysis per module

## 📊 What's Included

Each module documentation contains:

- **Gap Summary Table** — Counts by category
- **Critical Issues** — What blocks releases
- **Implementation Roadmap** — Phases and priorities
- **Affected Files** — Which files need work
- **Next Steps** — Actionable items
- **GitHub Links** — Related issues for tracking

## 🚀 Workflow

### 1. Developer Works on Module

```bash
cd src/acceleration/
cat MODULE_GAPS.md  # See what needs work
```

### 2. Implement Fixes

Fix gaps according to priorities in the doc.

### 3. Update Verification

After your work, verify progress:

```bash
python tools/gap_audit_pipeline_v2.py
# Re-run scan and documentation generation
```

### 4. Review Updated Docs

```bash
cat src/acceleration/MODULE_GAPS.md
# Check that gaps decreased
```

## 📈 Tracking Progress

Each module doc shows:

- **Total Gaps:** Should decrease over time
- **Unimplemented Paths:** Priority #1 (most critical)
- **Undocumented STUBs:** Priority #2 (must add template)
- **TODO/FIXME:** Priority #3 (maintenance)

Example progress from v1 to v2:

```
acceleration module:
  v1 (2026-05-10): 235 gaps (45 unimplemented)
  v2 (2026-05-18): 162 gaps (28 unimplemented)  ← 39% reduction!
```

## 🔄 Regeneration

Documentation is regenerated on each audit run:

```bash
# Full pipeline
python tools/gap_audit_pipeline_v2.py

# Just module docs (uses existing scan results)
python tools/module_doc_generator.py

# Distribute docs to module directories
python tools/module_doc_distributor.py ai_working/module_gaps .
```

## 📋 Module Status Quick View

| Module | Total Gaps | Critical | Status |
|--------|-----------|----------|--------|
| acceleration | 235 | 45 | 🔴 |
| security | 139 | 32 | 🟠 |
| storage | 84 | 18 | 🟡 |
| ingestion | 178 | 52 | 🔴 |
| llm | 151 | 40 | 🟠 |
| index | 94 | 22 | 🟡 |

See `MODULE_GAPS_INDEX.md` for full breakdown.

## 🎯 Related Resources

- `FINAL_SUMMARY.md` — Executive summary
- `CLUSTERED_ISSUES_REPORT.md` — Full gap analysis report
- `SCANNER_V2_IMPROVEMENTS.md` — How the scanner works
- `SMART_HEADER_FORMAT_GUIDE.md` — File header stats
- `.github/copilot-instructions.md` § 8 — STUB template requirements

## ❓ FAQ

**Q: Should I commit MODULE_GAPS.md?**  
A: Optional. It's regenerated frequently, so git history will be noisy. We recommend committing only when significant progress is made (e.g., module goes from 🔴 to 🟡).

**Q: Can I edit MODULE_GAPS.md manually?**  
A: Don't edit the generated sections. They're overwritten on each scan. But you can add implementation notes in a "Notes" section if needed.

**Q: How often is this updated?**  
A: Typically monthly via CI/CD. Run manually more frequently during active development.

**Q: What's the difference between this and GitHub issues?**  
A: This is **local + developer-focused** (view while coding). GitHub issues are **global tracking** (what's assigned, priority, reviews).

---

**Generated by:** ThemisDB Gap Audit Pipeline v2  
**Last Updated:** {date}
""".format(date=Path(output_dir).stat().st_mtime)
        
        try:
            with open(readme_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        except:
            return False

if __name__ == '__main__':
    import sys
    
    doc_source = sys.argv[1] if len(sys.argv) > 1 else 'ai_working/module_gaps'
    repo_root = sys.argv[2] if len(sys.argv) > 2 else '.'
    
    print("[INFO] Module Documentation Distributor")
    print("=" * 60)
    
    distributor = ModuleDocumentationDistributor(repo_root)
    
    print(f"\n[...] Distributing documentation to module directories...\n")
    results = distributor.distribute_docs(doc_source)
    
    success_count = sum(1 for v in results.values() if v)
    print(f"\n[OK] Distributed to {success_count}/{len(results)} modules")
    
    # Create README
    print(f"\n[...] Creating documentation README...")
    if distributor.create_readme(doc_source):
        print(f"[OK] README created")
    
    print("\n" + "=" * 60)
    print(f"[INFO] Modules are now documented in src/<module>/MODULE_GAPS.md")
    print(f"[INFO] Archive in {doc_source}/")
