#!/usr/bin/env python3
import markdown
import re

# Test markdown to HTML conversion
content = """
```mermaid
graph LR
    A --> B
```
"""

# Try with fenced_code extension
md = markdown.Markdown(extensions=['fenced_code', 'codehilite', 'tables'])
html = md.convert(content)
print("=== HTML OUTPUT (with fenced_code) ===")
print(html)
print()
print("=== CHECKS ===")
print(f"Has 'language-mermaid': {'language-mermaid' in html}")
print(f"Has 'mermaid': {'mermaid' in html}")
print()

# Test different patterns
patterns = [
    (r'<code class="language-mermaid">(.*?)</code>', 'language-mermaid in code'),
    (r'class="language-mermaid"', 'language-mermaid class'),
    (r'<pre>(.*?)</pre>', 'basic pre'),
]

for pattern, name in patterns:
    matches = re.findall(pattern, html, re.DOTALL)
    print(f"Pattern '{name}': Found" if pattern in html or matches else f"Pattern '{name}': Not found")
