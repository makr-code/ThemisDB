#!/usr/bin/env python3
import re

file_path = r'c:\VCC\themis\cmake\CMakeLists.txt'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Fix source paths: Add CMAKE_SOURCE_DIR prefix to relative paths
lines = content.split('\n')
fixed_lines = []

for line in lines:
    # Skip if already has CMAKE_SOURCE_DIR or is a generator expression or comment-only
    if '${CMAKE_SOURCE_DIR}' in line or '$<' in line or line.strip().startswith('#'):
        fixed_lines.append(line)
    else:
        # Replace relative paths at word boundaries
        # src/ files
        line = re.sub(r'(\s+)(src/[\w./\-]*\.(cpp|h|cc|cu))', r'\1${CMAKE_SOURCE_DIR}/src/\3', line)
        line = re.sub(r'(\s+)src/([^$])', r'\1${CMAKE_SOURCE_DIR}/src/\2', line)
        
        # tests/ files
        line = re.sub(r'(\s+)(tests/[\w./\-]*\.(cpp|h))', r'\1${CMAKE_SOURCE_DIR}/tests/\3', line)
        line = re.sub(r'(\s+)tests/([^$])', r'\1${CMAKE_SOURCE_DIR}/tests/\2', line)
        
        # benchmarks/ files
        line = re.sub(r'(\s+)(benchmarks/[\w./\-]*\.cpp)', r'\1${CMAKE_SOURCE_DIR}/benchmarks/\2', line)
        line = re.sub(r'(\s+)benchmarks/([^$])', r'\1${CMAKE_SOURCE_DIR}/benchmarks/\2', line)
        
        # proto/ files
        line = re.sub(r'(\s+)(proto/[\w./\-]*\.(cc|h))', r'\1${CMAKE_SOURCE_DIR}/proto/\3', line)
        line = re.sub(r'(\s+)proto/([^$])', r'\1${CMAKE_SOURCE_DIR}/proto/\2', line)
        
        # include/ files
        line = re.sub(r'(\s+)(include/[\w./\-]*\.h)', r'\1${CMAKE_SOURCE_DIR}/include/\2', line)
        line = re.sub(r'(\s+)include/([^$])', r'\1${CMAKE_SOURCE_DIR}/include/\2', line)
        
        fixed_lines.append(line)

fixed_content = '\n'.join(fixed_lines)

with open(file_path, 'w', encoding='utf-8') as f:
    f.write(fixed_content)

print('✓ Fixed cmake/CMakeLists.txt with CMAKE_SOURCE_DIR prefixes')
print('  - src/ → ${CMAKE_SOURCE_DIR}/src/')
print('  - tests/ → ${CMAKE_SOURCE_DIR}/tests/')
print('  - benchmarks/ → ${CMAKE_SOURCE_DIR}/benchmarks/')
print('  - proto/ → ${CMAKE_SOURCE_DIR}/proto/')
print('  - include/ → ${CMAKE_SOURCE_DIR}/include/')
