#!/usr/bin/env python3
"""Test regex patterns."""

import re

# Test text
text = """
public:
    Item() {}
    int getValue() { return value_; }
private:
    int value_;
};
"""

member_pattern = r'\b(?:static\s+)?(?:const\s+)?(?:volatile\s+)?(?:mutable\s+)?(?:class\s+|struct\s+)?(?:std::|::\w+::)?[\w:]+\s+\*?&?\s*\w+_\s*[=;]'

print("Testing member pattern:")
print(f"Pattern: {member_pattern}")
print(f"Text:\n{text}")

match = re.search(member_pattern, text)
print(f"\nMatch found: {match is not None}")
if match:
    print(f"Matched: {match.group()}")

# Try simpler pattern
simple_pattern = r'int\s+value_\s*;'
print(f"\n\nSimple pattern: {simple_pattern}")
match2 = re.search(simple_pattern, text)
print(f"Match found: {match2 is not None}")
if match2:
    print(f"Matched: {match2.group()}")

# Try another simple pattern  
simple_pattern2 = r'\bint\s+\w+_\s*;'
print(f"\n\nSimple pattern 2: {simple_pattern2}")
match3 = re.search(simple_pattern2, text)
print(f"Match found: {match3 is not None}")
if match3:
    print(f"Matched: {match3.group()}")

# Try without underscore requirement
simple_pattern3 = r'\bint\s+\w+\s*;'
print(f"\n\nSimple pattern 3 (no underscore): {simple_pattern3}")
match4 = re.search(simple_pattern3, text)
print(f"Match found: {match4 is not None}")
if match4:
    print(f"Matched: {match4.group()}")
