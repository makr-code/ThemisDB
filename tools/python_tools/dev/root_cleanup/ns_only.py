#!/usr/bin/env python3
"""Count ONLY namespace opening/closing braces"""

with open(r"c:\Projects\ThemisDB\src\graph\ontology_manager.cpp", 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

ns_opens = []
ns_closes = []

for i, line in enumerate(lines, 1):
    stripped = line.strip()
    if stripped.startswith('namespace'):
        # This is a namespace open
        ns_opens.append((i, line.rstrip()))
    if '} // namespace' in stripped or '}' == stripped and i > 290:
        # This MIGHT be a namespace close
        # Let's check: is it AFTER the last namespace open?
        if 'namespace' in stripped or i in [292, 701, 702]:
            ns_closes.append((i, line.rstrip()))

print("NAMESPACE OPENS:")
for line_no, line in ns_opens:
    print(f"  L{line_no}: {line}")

print("\nPOSSIBLE NAMESPACE CLOSES:")
for line_no, line in ns_closes:
    print(f"  L{line_no}: {line}")
