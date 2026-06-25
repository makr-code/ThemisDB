#!/usr/bin/env python3
"""Track brace stack to find unmatched closing brace"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

stack = []  # Stack of (type, line_no, content)
errors = []

for i, line in enumerate(lines, 1):
    for j, char in enumerate(line):
        if char == '{':
            stack.append(('open', i, line.strip()[:70]))
        elif char == '}':
            if stack:
                open_info = stack.pop()
            else:
                errors.append((i, line.strip()[:70], "UNMATCHED: } without matching {"))

if stack:
    for open_info in stack:
        errors.append((open_info[1], open_info[2], "UNMATCHED: { without matching }"))

if errors:
    print("🔴 ERRORS FOUND:")
    for line_no, content, error in errors:
        print(f"  Line {line_no}: {error}")
        print(f"    {content}")
else:
    print("✅ All braces matched properly")
