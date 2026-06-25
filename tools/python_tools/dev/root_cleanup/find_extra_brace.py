#!/usr/bin/env python3
"""Find the extra closing brace in ontology_manager.cpp"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Track all braces with line numbers
open_count = 0
close_count = 0
balance = 0

for i, line in enumerate(lines, 1):
    for char in line:
        if char == '{':
            open_count += 1
            balance += 1
        elif char == '}':
            close_count += 1
            balance -= 1
            # Print when balance goes negative (close without open)
            if balance < 0:
                print(f"❌ LINE {i}: Balance goes NEGATIVE! Current balance: {balance}")
                print(f"    {line.rstrip()}")
                break

print()
print(f"Total opens: {open_count}")
print(f"Total closes: {close_count}")
print(f"Final balance: {balance}")

# Now let's find namespace lines
print("\n" + "="*70)
print("NAMESPACE TRACKING:")
print("="*70)
with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    for i, line in enumerate(f, 1):
        if 'namespace' in line.lower():
            print(f"Line {i:3d}: {line.rstrip()}")
