#!/usr/bin/env python3
import re
import sys

filepath = 'tests/CMakeLists.txt'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Find all add_executable lines with "focused" in their name and wrap them with if(NOT SKIP_FOCUSED_TESTS)...endif()
pattern = r'(add_executable\([^\)]*_focused[^\)]*\))'

def replace_func(match):
    exec_line = match.group(1)
    return f'if(NOT SKIP_FOCUSED_TESTS)\n    {exec_line}\nendif()'

new_content = re.sub(pattern, replace_func, content)
count = len(re.findall(pattern, content))

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(new_content)

print(f"Wrapped {count} focused test executables with if(NOT SKIP_FOCUSED_TESTS)...endif()")
