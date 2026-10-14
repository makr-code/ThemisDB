from pathlib import Path
import sys
sys.path.insert(0, ".")

from scripts.sync_soll_ist_gaps import load_module_task_template, render_module_task_template

# Load template
repo = Path(".")
try:
    template = load_module_task_template(repo)
    print(f"Template loaded: {len(template)} bytes")
    print(f"Contains IMPLEMENTATION_TASKS placeholder: {template.count('IMPLEMENTATION_TASKS') > 0}")
    print(f"Contains RISK_CONSTRAINT placeholder: {template.count('RISK_CONSTRAINT') > 0}")
    
    # Show lines around the placeholder
    lines = template.split('\n')
    for i, line in enumerate(lines):
        if 'IMPLEMENTATION_TASKS' in line:
            print(f"\nFound at line {i}:")
            for j in range(max(0, i-2), min(len(lines), i+3)):
                print(f"  {j}: {lines[j]}")
            break
    
    # Test rendering
    test_placeholders = {
        "MODULE_NAME": "test-module",
        "IMPLEMENTATION_TASKS": "- [ ] Task 1\n- [ ] Task 2",
        "RISK_CONSTRAINT": "Test constraint",
        "OVERALL_STATUS": "test",
        "SUMMARY_FINDINGS": "test",
        "SCOPE_FILES": "test",
        "STATUS": "open",
        "DOCS_STATUS": "test",
        "README_PATH": "test",
        "GRAPH_PATH": "test",
        "DOXYGEN_PATH": "test",
        "COMPLIANCE_REPORT_PATH": "test",
        "VALIDATION_REPRO": "test",
        "VALIDATION_GATE": "test",
        "VALIDATION_SUCCESS": "test",
        "IMPLEMENTATION_DETAILS": "test",
        "TEST_DETAILS": "test",
        "BENCHMARK_DETAILS": "test",
        "DOCUMENTATION_DETAILS": "test",
        "ACCEPTANCE_CRITERIA": "test",
        "OWNERSHIP_PATH": "test",
        "ISSUE_LABELS": "test",
        "ISSUE_MILESTONE": "test",
        "ISSUE_STATUS": "open",
        "TASK_SUMMARY": "test",
        "ISSUE_LABELS_JSON": "[]",
    }
    
    rendered = render_module_task_template(template, test_placeholders)
    print(f"\nRendered: {len(rendered)} bytes")
    
    # Show "What needs to be done" section
    lines = rendered.split('\n')
    for i, line in enumerate(lines):
        if 'What needs to be done' in line:
            print(f"\nWhat needs to be done (lines {i}-{i+2}):")
            for j in range(i, min(len(lines), i+3)):
                print(f"  {lines[j]}")
            break
            
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
