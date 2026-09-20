#!/usr/bin/env python3
from pathlib import Path
import json
import sys
sys.path.insert(0, ".")

# Read a sample gap report to get real data
gap_report = Path("ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json")
if not gap_report.exists():
    print(f"Report not found: {gap_report}")
    sys.exit(1)

with open(gap_report) as f:
    data = json.load(f)

# Get access-model data
module_name = "access-model"
access_model_data = data["modules"].get(module_name)
if not access_model_data:
    print(f"Module {module_name} not found in report")
    sys.exit(1)

print(f"Found module data for {module_name}")
print(f"Keys: {list(access_model_data.keys())}")

# Check the gate structure
impl = access_model_data.get("impl_status", {})
gates = access_model_data.get("module_gates", {})
docs = access_model_data.get("docs_status", {})

print(f"\nImpl status: {impl}")
print(f"Gates status: {gates}")
print(f"Docs status: {docs}")

# Check what tasks would be generated
impl_tasks = []
if impl.get('actionable_total', 0) > 0:
    sev = impl.get('severity_counts', {})
    sev_summary = f"Critical {sev.get('CRITICAL', 0)}, High {sev.get('HIGH', 0)}, Medium {sev.get('MEDIUM', 0)}"
    impl_tasks.append(f"- [ ] Implementierung: Code-Findings beheben ({sev_summary})")
    print(f"\nAdded impl task")
else:
    print(f"\nNo impl tasks (actionable_total={impl.get('actionable_total', 0)})")

if gates.get('tests_missing', False):
    impl_tasks.append("- [ ] Tests: Fehlende Tests für Modulkontext schreiben")
    print(f"Added missing tests task")
elif gates.get('failing_tests', 0) > 0:
    impl_tasks.append(f"- [ ] Tests: {gates.get('failing_tests', 0)} fehlgeschlagene Tests reparieren")
    print(f"Added failing tests task")

if gates.get('benchmarks_missing', False):
    impl_tasks.append("- [ ] Benchmarks: Fehlende Benchmarks ergänzen")
    print(f"Added missing benchmarks task")
elif gates.get('failing_benchmarks', 0) > 0:
    impl_tasks.append(f"- [ ] Benchmarks: {gates.get('failing_benchmarks', 0)} fehlgeschlagene Benchmarks reparieren")
    print(f"Added failing benchmarks task")

if docs.get('gap_open', False):
    missing_labels = docs.get('missing_core_docs', [])
    if missing_labels:
        impl_tasks.append(f"- [ ] Dokumentation: Governance-Dokumente erstellen/aktualisieren ({len(missing_labels)} fehlend)")
        print(f"Added docs task: {len(missing_labels)} missing")
    else:
        print(f"No missing docs labels found")
else:
    print(f"Docs gap_open is False")

if not impl_tasks:
    impl_tasks.append("- Keine kritischen Aufgaben identifiziert; siehe Details unter Fehlende Implementierung/Tests/Benchmarks/Dokumentation")

print(f"\nFinal tasks ({len(impl_tasks)} total):")
for task in impl_tasks:
    print(f"  {task}")
