#!/usr/bin/env python3
"""Check for external submodules in gap scanner results"""
import json
import glob
import os

external_modules = ['llama.cpp', 'whisper.cpp', 'vcpkg', 'onnx-clip']

print("=" * 80)
print("EXTERNAL SUBMODULE ANALYSIS")
print("=" * 80)

# 1. Check which gap_scan files reference external modules
print("\n1. GAP SCANNER RESULTS ANALYSIS")
print("-" * 40)

external_references = {}
for gap_file in glob.glob('ai_working/gap_scan_*.json'):
    try:
        with open(gap_file, encoding='utf-8') as f:
            data = json.load(f)
            if isinstance(data, list):
                external_count = 0
                for g in data:
                    filepath = g.get('file', '')
                    if any(mod in filepath for mod in external_modules):
                        external_count += 1
                
                if external_count > 0:
                    module_name = os.path.basename(gap_file).replace('gap_scan_', '').replace('.json', '')
                    external_references[module_name] = {
                        'total': len(data),
                        'external': external_count,
                        'pct': round(100 * external_count / len(data), 1) if len(data) > 0 else 0
                    }
    except Exception as e:
        pass

if external_references:
    print("\n⚠️  FOUND EXTERNAL SUBMODULE REFERENCES IN GAP SCANNER:")
    for module, stats in sorted(external_references.items()):
        print(f"  {module}: {stats['external']}/{stats['total']} ({stats['pct']}%) external")
else:
    print("\n✓ No external submodule references in gap_scan_*.json files")

# 2. Check for external modules in verified L0.5 results
print("\n2. VERIFIED L0.5 RESULTS ANALYSIS")
print("-" * 40)

for verified_file in glob.glob('ai_working/gap_scan_results_verified_L0.5*.json'):
    try:
        with open(verified_file, encoding='utf-8') as f:
            data = json.load(f)
            external_count = 0
            total = len(data)
            
            for g in data:
                filepath = g.get('file', '')
                if any(mod in filepath for mod in external_modules):
                    external_count += 1
            
            if external_count > 0:
                print(f"\n⚠️  {os.path.basename(verified_file)}:")
                print(f"    External references: {external_count}/{total} ({round(100*external_count/total,1)}%)")
            else:
                print(f"\n✓ {os.path.basename(verified_file)}: No external references")
    except Exception as e:
        print(f"  Error: {e}")

# 3. Check for external modules in MODULE_GAPS.md files
print("\n3. MODULE_GAPS.MD FILES ANALYSIS")
print("-" * 40)

module_gaps_files = glob.glob('src/*/MODULE_GAPS.md')
print(f"\nFound {len(module_gaps_files)} MODULE_GAPS.md files")

for gaps_file in module_gaps_files[:3]:
    try:
        with open(gaps_file, encoding='utf-8') as f:
            content = f.read()
            has_external = any(mod in content for mod in external_modules)
            print(f"  {gaps_file}: {'⚠️  HAS EXTERNAL REFS' if has_external else '✓ Clean'}")
    except Exception as e:
        pass

# 4. Summary
print("\n" + "=" * 80)
print("SUMMARY")
print("=" * 80)
print("""
✓ If NO external references found above:
  → External submodules were properly excluded from gap scanner
  → L0.5 findings are clean (no false cascading)
  → L1-L3 propagation did NOT include external findings

⚠️  If external references found:
  → Exclusion filter may need review
  → May need to add --exclude-submodules flag to scanner
  → L0-L3 propagation should skip these findings
""")
