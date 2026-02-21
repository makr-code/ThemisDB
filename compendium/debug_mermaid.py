"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debug_mermaid.py                                   ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
