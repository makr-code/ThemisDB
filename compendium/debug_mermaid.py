"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debug_mermaid.py                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
from pathlib import Path
import re

file_path = Path("docs/chapter_02_architecture.md")
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Test das aktuelle regex
matches = re.findall(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
print(f"Mermaid-Blöcke gefunden: {len(matches)}")

# Teste die split operation
parts = re.split(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
print(f"Split-Parts: {len(parts)}")
print(f"Diagramme (ungerade Indices): {(len(parts)-1)//2}")

# Zähle die mermaid-Vorkommen mit grep-style
count_mermaid = len(re.findall(r'```mermaid', content))
print(f"Rohzählung '```mermaid': {count_mermaid}")

# Wenn weniger als erwartet, debugge
if len(matches) < 1:
    print("\n[DEBUG] Erste 1000 Zeichen:")
    print(content[:1000])
