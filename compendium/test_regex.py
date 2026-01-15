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
