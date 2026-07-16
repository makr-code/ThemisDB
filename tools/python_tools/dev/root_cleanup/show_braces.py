#!/usr/bin/env python3
"""Show every opening and closing brace with line numbers"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

opens = []
closes = []

for i, line in enumerate(lines, 1):
    for j, char in enumerate(line):
        if char == '{':
            opens.append((i, j, line.strip()))
        elif char == '}':
            closes.append((i, j, line.strip()))

print(f"Total OPENS ({len(opens)}):")
for line_no, col, content in opens:
    print(f"  L{line_no:3d}: {content[:70]}")

print()
print(f"Total CLOSES ({len(closes)}):")
for line_no, col, content in closes:
    print(f"  L{line_no:3d}: {content[:70]}")

print()
print(f"Difference: {len(closes) - len(opens)} extra closes")
