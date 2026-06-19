#!/usr/bin/env python3
"""Ultra-detailed brace tracking - show every unmatched brace"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

open_stack = []  # List of (line_no, char_pos, context)
close_without_open = []

for line_no, line in enumerate(lines, 1):
    for col, char in enumerate(line):
        if char == '{':
            context = line.strip()[:60]
            open_stack.append((line_no, col, context))
        elif char == '}':
            if open_stack:
                open_stack.pop()
            else:
                context = line.strip()[:60]
                close_without_open.append((line_no, col, context))
                print(f"❌ LINE {line_no}: CLOSE without OPEN")
                print(f"   {context}")
                print()

if open_stack:
    print(f"\n🔴 UNCLOSED OPENINGS at EOF ({len(open_stack)} remaining):")
    for line_no, col, context in open_stack[-3:]:  # Show last 3
        print(f"   L{line_no}: {context}")
