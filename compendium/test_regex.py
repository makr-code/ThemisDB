"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_regex.py                                      ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
import re

content = """
Some text here

```mermaid
graph LR
    A --> B
```

More text
"""

# Test das aktuelle regex
matches = re.findall(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
print(f"Matches gefunden: {len(matches)}")
print(f"Matches: {matches}")

# Test mit split
parts = re.split(r'```mermaid\n(.*?)\n```', content, flags=re.DOTALL)
print(f"\nParts (split): {len(parts)} items")
for i, part in enumerate(parts):
    print(f"Part {i}: {repr(part[:50] if len(part) > 50 else part)}")
