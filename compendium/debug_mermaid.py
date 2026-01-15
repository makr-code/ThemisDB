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
