#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))

from scripts.sync_soll_ist_gaps import load_module_task_template, render_module_task_template

repo = Path("c:\\Projects\\ThemisDB")
template = load_module_task_template(repo)
print("=== TEMPLATE LOADED ===")
print(f"Length: {len(template)}")
print(f"Has {{{{IMPLEMENTATION_TASKS}}}}: {'{{{{IMPLEMENTATION_TASKS}}}}' in template}")
print(f"Has {{{{RISK_CONSTRAINT}}}}: {'{{{{RISK_CONSTRAINT}}}}' in template}")

# Test rendering
test_placeholders = {
    "MODULE_NAME": "test-module",
    "IMPLEMENTATION_TASKS": "- [ ] Test task 1\n- [ ] Test task 2",
    "RISK_CONSTRAINT": "Test Risk",
    "OWNERSHIP_PATH": "test-ownership",
    "OVERALL_STATUS": "test status",
    "SUMMARY_FINDINGS": "test findings",
    "SCOPE_FILES": "test/files",
    "STATUS": "open",
    "DOCS_STATUS": "test docs",
    "README_PATH": "test/README.md",
    "GRAPH_PATH": "test/graph.json",
    "DOXYGEN_PATH": "test/doxygen",
    "COMPLIANCE_REPORT_PATH": "test/compliance",
    "VALIDATION_REPRO": "test command",
    "VALIDATION_GATE": "test gate",
    "VALIDATION_SUCCESS": "test success",
    "IMPLEMENTATION_DETAILS": "test impl details",
    "TEST_DETAILS": "test test details",
    "BENCHMARK_DETAILS": "test bench",
    "DOCUMENTATION_DETAILS": "test doc details",
    "ACCEPTANCE_CRITERIA": "- [ ] test",
}

rendered = render_module_task_template(template, test_placeholders)
print("\n=== RELEVANT LINES ===")
for i, line in enumerate(rendered.split('\n'), 1):
    if any(kw in line.lower() for kw in ['needs to be done', 'implementation', 'risk', 'ownership']):
        print(f"{i}: {line}")

# Find the actual "What needs to be done" section
print("\n=== WHAT NEEDS TO BE DONE SECTION ===")
lines = rendered.split('\n')
for i, line in enumerate(lines):
    if '## What needs to be done' in line:
        print(f"Line {i}: {line}")
        print(f"Line {i+1}: {lines[i+1] if i+1 < len(lines) else 'EOF'}")
        break
