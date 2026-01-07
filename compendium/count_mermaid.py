#!/usr/bin/env python3
import re
from pathlib import Path

count = 0
files_with_diagrams = []

for f in sorted(Path(".").glob("chapter_*.md")) + sorted(Path(".").glob("appendix_*.md")):
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
        matches = len(re.findall(r'```mermaid', content))
        if matches > 0:
            files_with_diagrams.append((f.name, matches))
            count += matches

print(f"📊 Mermaid-Diagramme gefunden:\n")
for fname, num in files_with_diagrams:
    print(f"  {fname}: {num}")

print(f"\n📈 Gesamt: {count} Diagramme in {len(files_with_diagrams)} Dateien")
