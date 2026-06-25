#!/usr/bin/env python3
"""Find exact line with extra closing brace"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

balance = 0
for i, line in enumerate(lines, 1):
    for char in line:
        if char == '{':
            balance += 1
        elif char == '}':
            balance -= 1
            if balance < 0:
                print(f"🎯 FOUND IT at LINE {i}:")
                print(f"   {line.rstrip()}")
                print()
                print("Context (5 lines before):")
                for j in range(max(0, i-6), i):
                    print(f"  {j+1:3d}: {lines[j].rstrip()}")
                print()
                print("This line contains the extra closing brace!")
                exit(0)

print("❌ Could not find point where balance goes negative (unexpected)")
