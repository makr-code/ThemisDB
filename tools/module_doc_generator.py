#!/usr/bin/env python3
"""
Module Gap Documentation Generator

Creates developer documentation for each module:
- MODULE_GAPS.md in each module directory
- Gap statistics and breakdown
- Implementation status
- Known issues
- Next steps and links to GitHub issues
"""

import json
from pathlib import Path
from datetime import datetime
from typing import Dict, List
from dataclasses import dataclass

@dataclass
class ModuleGapStats:
    """Gap statistics for a module"""
    name: str
    total_gaps: int = 0
    unimplemented: int = 0
    stub_documented: int = 0
    stub_undocumented: int = 0
    todo: int = 0
    fixme: int = 0
    technical_debt: int = 0
    platform_specific: int = 0
    mock: int = 0
    
    def get_severity_counts(self) -> Dict[str, int]:
        return {
            'unimplemented': self.unimplemented,
            'stub_undocumented': self.stub_undocumented,
            'todo': self.todo + self.fixme,
            'technical_debt': self.technical_debt,
        }
    
    def get_critical_paths(self) -> int:
        """Count paths that need immediate attention"""
        return self.unimplemented + self.stub_undocumented
    
    def get_health_status(self) -> str:
        """Determine health status"""
        critical = self.get_critical_paths()
        if critical == 0:
            return "✅ Healthy (ready)"
        elif critical <= 5:
            return "🟡 Minor Issues"
        elif critical <= 20:
            return "🟠 Multiple Issues"
        else:
            return "🔴 Critical State"

class ModuleDocumentationGenerator:
    """Generate developer documentation for modules"""
    
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.scan_results = {}
    
    def load_scan_results(self, scan_dir: str) -> bool:
        """Load gap scan results from JSON files"""
        scan_path = Path(scan_dir)
        
        if not scan_path.exists():
            print(f"❌ Scan directory not found: {scan_dir}")
            return False
        
        # Load all gap_scan_v2_<module>.json files
        for json_file in sorted(scan_path.glob('gap_scan_v2_*.json')):
            if json_file.name == 'gap_scan_v2_aggregate.json':
                continue
            
            try:
                with open(json_file) as f:
                    data = json.load(f)
                    module = data.get('module')
                    if module:
                        self.scan_results[module] = data
            except:
                pass
        
        return len(self.scan_results) > 0
    
    def generate_module_docs(self, module_name: str, 
                           output_dir: str = None) -> bool:
        """Generate documentation for a single module"""
        
        if module_name not in self.scan_results:
            return False
        
        module_data = self.scan_results[module_name]
        gaps_by_file = module_data.get('gaps_by_file', {})
        
        # Parse statistics
        stats = ModuleGapStats(name=module_name)
        total_gaps = module_data.get('stats', {}).get('total', 0)
        
        for category in ['unimplemented', 'stub_documented', 'stub_undocumented', 
                        'todo_item', 'fixme_item', 'technical_debt', 
                        'platform_fallback', 'mock_framework']:
            count = module_data.get('stats', {}).get(category, 0)
            if category == 'unimplemented':
                stats.unimplemented = count
            elif category == 'stub_documented':
                stats.stub_documented = count
            elif category == 'stub_undocumented':
                stats.stub_undocumented = count
            elif category == 'todo_item':
                stats.todo = count
            elif category == 'fixme_item':
                stats.fixme = count
            elif category == 'technical_debt':
                stats.technical_debt = count
            elif category == 'platform_fallback':
                stats.platform_specific = count
            elif category == 'mock_framework':
                stats.mock = count
        
        stats.total_gaps = total_gaps
        
        # Generate documentation
        doc = self._generate_doc_content(stats, gaps_by_file)
        
        # Determine output path
        if output_dir:
            doc_path = Path(output_dir) / 'MODULE_GAPS.md'
        else:
            # Try module-specific directory
            module_dir = self.repo_root / 'src' / module_name
            if module_dir.exists():
                doc_path = module_dir / 'MODULE_GAPS.md'
            else:
                doc_path = self.repo_root / 'ai_working' / f'{module_name}_GAPS.md'
        
        # Write documentation
        doc_path.parent.mkdir(parents=True, exist_ok=True)
        with open(doc_path, 'w', encoding='utf-8') as f:
            f.write(doc)
        
        return True
    
    def _generate_doc_content(self, stats: ModuleGapStats, 
                            gaps_by_file: Dict) -> str:
        """Generate the markdown content for module documentation"""
        
        # Sample gaps by category
        sample_gaps = self._collect_sample_gaps(gaps_by_file)
        
        doc = f"""# {stats.name} Module — Implementation Gap Analysis

**Status:** {stats.get_health_status()}  
**Last Scanned:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  
**Total Gaps:** {stats.total_gaps}  

---

## 📊 Gap Summary

| Category | Count | Priority | Status |
|----------|-------|----------|--------|
| Unimplemented Paths | {stats.unimplemented} | 🔴 CRITICAL | Blocks release |
| STUB (undocumented) | {stats.stub_undocumented} | 🟠 HIGH | Missing template |
| STUB (documented) | {stats.stub_documented} | ✅ OK | By design |
| TODO/FIXME | {stats.todo + stats.fixme} | 🟡 MEDIUM | Pending work |
| Technical Debt | {stats.technical_debt} | 🔵 LOW | Nice to have |
| Platform-Specific | {stats.platform_specific} | ✅ OK | By design |
| Mock/Test Code | {stats.mock} | ⚪ N/A | Test only |
| **TOTAL** | **{stats.total_gaps}** | | |

---

## 🎯 Critical Issues

### Unimplemented Paths ({stats.unimplemented})

These are production code paths that throw "not implemented" errors.

**Action Required:** Implement or document each one.

{self._format_sample_gaps('unimplemented', sample_gaps.get('unimplemented', []))}

### Undocumented STUBs ({stats.stub_undocumented})

STUB markers without the required 4-line documentation template.

**Action Required:** Add documentation per COPILOT_INSTRUCTIONS.md § 8

```cpp
// STUB/SIMULATION NOTE:
// Purpose: [why this stub exists]
// Activation: [build flag or condition]
// Production Delta: [how behavior differs from production]
// Removal Plan: [when/how this will be removed]
```

{self._format_sample_gaps('stub', sample_gaps.get('stub_undocumented', []))}

---

## 📝 Code Debt

### TODO/FIXME Items ({stats.todo + stats.fixme})

{self._format_sample_gaps('todo', sample_gaps.get('todo', []))}

### Technical Debt ({stats.technical_debt})

{self._format_sample_gaps('debt', sample_gaps.get('technical_debt', []))}

---

## 📈 Implementation Roadmap

### Phase 1: Critical Path (1-2 weeks)
- [ ] Fix all {stats.unimplemented} unimplemented paths
- [ ] Add documentation to {stats.stub_undocumented} undocumented STUBs
- [ ] Link TODO items to GitHub issues

### Phase 2: Polish (1 week)
- [ ] Reduce technical debt ({stats.technical_debt} items)
- [ ] Update tests for new implementations
- [ ] Performance validation

### Phase 3: Verification (ongoing)
- [ ] Code review of implementations
- [ ] Integration testing
- [ ] Documentation review

---

## 🔗 Related Issues

Links to GitHub issues tracking this module's gaps:

- **META-001:** Complete unimplemented code paths
- **META-002:** Standardize STUB documentation
- **MODULE-{stats.name.upper()}:** Module-specific issues

Visit: https://github.com/makr-code/ThemisDB/issues?q=label:gap-scan&q={stats.name}

---

## 📚 Files Affected

Files with gaps in this module:

{self._format_affected_files(gaps_by_file)}

---

## 🚀 Next Steps

1. **Review** this documentation
2. **Prioritize** gaps by severity
3. **Create pull requests** for each fix
4. **Link to GitHub issues** for tracking
5. **Update headers** with `python tools/gap_audit_pipeline_v2.py`
6. **Re-scan** monthly to track progress

---

**Generated by:** ThemisDB Gap Audit Pipeline v2  
**Format:** THEMIS_MODULE_GAPS_v1
"""
        return doc
    
    def _collect_sample_gaps(self, gaps_by_file: Dict) -> Dict[str, List[Dict]]:
        """Collect sample gaps by category (max 3 per category)"""
        samples = {
            'unimplemented': [],
            'stub_undocumented': [],
            'todo': [],
            'technical_debt': [],
        }
        
        for file_path, gaps in gaps_by_file.items():
            for gap in gaps:
                category = gap.get('category', 'unknown')
                
                if category == 'unimplemented' and len(samples['unimplemented']) < 3:
                    samples['unimplemented'].append({
                        'file': file_path,
                        'line': gap.get('line', 0),
                        'snippet': gap.get('snippet', '')
                    })
                elif category == 'stub_undocumented' and len(samples['stub_undocumented']) < 3:
                    samples['stub_undocumented'].append({
                        'file': file_path,
                        'line': gap.get('line', 0),
                        'snippet': gap.get('snippet', '')
                    })
                elif category == 'todo_item' and len(samples['todo']) < 3:
                    samples['todo'].append({
                        'file': file_path,
                        'line': gap.get('line', 0),
                        'snippet': gap.get('snippet', '')
                    })
                elif category == 'technical_debt' and len(samples['technical_debt']) < 3:
                    samples['technical_debt'].append({
                        'file': file_path,
                        'line': gap.get('line', 0),
                        'snippet': gap.get('snippet', '')
                    })
        
        return samples
    
    def _format_sample_gaps(self, category: str, gaps: List[Dict]) -> str:
        """Format sample gaps for display"""
        if not gaps:
            return "*(No examples to show)*\n"
        
        output = "**Examples:**\n\n"
        for gap in gaps[:3]:
            file_rel = gap['file']
            line = gap['line']
            snippet = gap['snippet'][:80]
            
            output += f"- `{file_rel}:{line}` — {snippet}\n"
        
        return output
    
    def _format_affected_files(self, gaps_by_file: Dict) -> str:
        """Format list of affected files"""
        if not gaps_by_file:
            return "*(No affected files)*\n"
        
        output = "| File | Gap Count | Status |\n"
        output += "|------|-----------|--------|\n"
        
        for file_path in sorted(gaps_by_file.keys())[:15]:  # Top 15 files
            gap_count = len(gaps_by_file[file_path])
            status = "🔴" if gap_count > 10 else "🟡" if gap_count > 5 else "🟢"
            
            # Make path relative to src/
            if file_path.startswith('src/'):
                file_short = file_path[4:]
            else:
                file_short = file_path
            
            output += f"| `{file_short}` | {gap_count} | {status} |\n"
        
        if len(gaps_by_file) > 15:
            output += f"| *(+{len(gaps_by_file) - 15} more files)* | | |\n"
        
        return output
    
    def generate_all_module_docs(self, output_dir: str = None) -> Dict[str, bool]:
        """Generate documentation for all modules"""
        results = {}
        
        for module_name in sorted(self.scan_results.keys()):
            success = self.generate_module_docs(module_name, output_dir)
            results[module_name] = success
            
            if success:
                print(f"  [OK] {module_name}")
            else:
                print(f"  [FAIL] {module_name}")
        
        return results
    
    def generate_module_index(self, output_dir: str) -> bool:
        """Generate index of all module documentation"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        index_path = output_path / 'MODULE_GAPS_INDEX.md'
        
        # Sort by critical count
        modules_sorted = sorted(
            self.scan_results.items(),
            key=lambda x: x[1].get('stats', {}).get('unimplemented', 0),
            reverse=True
        )
        
        doc = """# Module Gap Documentation Index

Overview of all module gap analyses.

**Updated:** {date}

---

## 📊 Modules by Gap Count (Highest to Lowest)

| Module | Total Gaps | Unimplemented | STUB | TODO | Status |
|--------|-----------|---------------|------|------|--------|
""".format(date=datetime.now().strftime('%Y-%m-%d'))
        
        for module_name, module_data in modules_sorted:
            stats = module_data.get('stats', {})
            total = stats.get('total', 0)
            unimpl = stats.get('unimplemented', 0)
            stubs = stats.get('stub_documented', 0) + stats.get('stub_undocumented', 0)
            todos = stats.get('todo_item', 0) + stats.get('fixme_item', 0)
            
            # Health status
            if unimpl == 0 and stubs == 0 and todos == 0:
                health = "✅"
            elif unimpl > 50:
                health = "🔴"
            elif unimpl > 10:
                health = "🟠"
            else:
                health = "🟡"
            
            doc += f"| `{module_name}` | {total} | {unimpl} | {stubs} | {todos} | {health} |\n"
        
        doc += """\n---

## 🚀 Quick Actions

### Critical Modules (Unimplemented > 50)

"""
        critical_modules = [m for m in modules_sorted if m[1].get('stats', {}).get('unimplemented', 0) > 50]
        for module_name, _ in critical_modules[:5]:
            doc += f"- [ ] Fix [{module_name}]({module_name}_GAPS.md)\n"
        
        doc += """\n### High Priority (Unimplemented 10-50)

"""
        high_modules = [m for m in modules_sorted 
                       if 10 <= m[1].get('stats', {}).get('unimplemented', 0) <= 50]
        for module_name, _ in high_modules[:5]:
            doc += f"- [ ] Fix [{module_name}]({module_name}_GAPS.md)\n"
        
        doc += "\n---\n\n## 📋 Full Module List\n\n"
        
        for module_name in sorted(self.scan_results.keys()):
            doc += f"- [{module_name}]({module_name}_GAPS.md)\n"
        
        doc += """\n---

**Legend:**
- ✅ Healthy (ready for production)
- 🟡 Minor Issues (< 10 unimplemented)
- 🟠 Multiple Issues (10-50 unimplemented)
- 🔴 Critical State (> 50 unimplemented)
"""
        
        with open(index_path, 'w', encoding='utf-8') as f:
            f.write(doc)
        
        return True

if __name__ == '__main__':
    import sys
    
    repo_root = sys.argv[1] if len(sys.argv) > 1 else '.'
    scan_dir = sys.argv[2] if len(sys.argv) > 2 else 'ai_working'
    output_dir = sys.argv[3] if len(sys.argv) > 3 else 'ai_working/module_gaps'
    
    print("[INFO] Module Gap Documentation Generator")
    print("=" * 60)
    
    gen = ModuleDocumentationGenerator(repo_root)
    
    if not gen.load_scan_results(scan_dir):
        print("[FAIL] No scan results found")
        sys.exit(1)
    
    print(f"\n[OK] Loaded {len(gen.scan_results)} modules\n")
    
    print("[...] Generating module documentation...")
    results = gen.generate_all_module_docs(output_dir)
    
    success_count = sum(1 for v in results.values() if v)
    print(f"\n[OK] Generated {success_count}/{len(results)} module docs")
    
    print("\n[...] Generating module index...")
    if gen.generate_module_index(output_dir):
        print(f"[OK] Index created: {Path(output_dir) / 'MODULE_GAPS_INDEX.md'}")
    
    print("\n" + "=" * 60)
    print(f"[INFO] Output directory: {output_dir}/")
