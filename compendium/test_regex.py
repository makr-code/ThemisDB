"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_regex.py                                      ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
