# Fix all paths in cmake files based on correct_paths.json

import json
import re

# Load correct paths
with open('C:/VCC/themis/correct_paths.json', 'r') as f:
    correct_paths = json.load(f)

# Create mapping from filename to correct path
path_map = {filename: path for filename, path in correct_paths.items()}

# Files to fix
cmake_files = [
    'C:/VCC/themis/cmake/AccelerationBackends.cmake',
    'C:/VCC/themis/cmake/BufferManagement.cmake',
    'C:/VCC/themis/cmake/ContentProcessors.cmake',
    'C:/VCC/themis/cmake/DistributedTraining.cmake',
    'C:/VCC/themis/cmake/EditionFeatures.cmake',
    'C:/VCC/themis/cmake/ErrorHealthServices.cmake',
    'C:/VCC/themis/cmake/IndexQueryEnhancements.cmake',
    'C:/VCC/themis/cmake/LLMIntegration.cmake',
    'C:/VCC/themis/cmake/MiscellaneousFeatures.cmake',
    'C:/VCC/themis/cmake/RPCServices.cmake',
    'C:/VCC/themis/cmake/StorageEnhancements.cmake'
]

print("=== FIXING CMAKE PATHS ===\n")

for cmake_file in cmake_files:
    print(f"Processing {cmake_file.split('/')[-1]}...")
    
    with open(cmake_file, 'r') as f:
        content = f.read()
    
    # Fix each path
    for filename, correct_path in path_map.items():
        # Try to find incorrect patterns
        patterns = [
            f'../src/acceleration/{filename}',
            f'../src/content/{filename}',
            f'../src/llm/{filename}',
            f'../src/storage/{filename}',
            f'../src/server/{filename}',
            f'../src/training/{filename}',
            f'../src/plugins/{filename}',
            f'../src/index/{filename}',
            f'../src/query/{filename}',
            f'../src/analytics/{filename}',
            f'../src/base/{filename}',
            f'../src/buffer/{filename}',
            f'../src/monitoring/{filename}'
        ]
        
        for pattern in patterns:
            if pattern in content and pattern != correct_path:
                content = content.replace(pattern, correct_path)
                print(f"  ✓ Fixed {filename}")
    
    with open(cmake_file, 'w') as f:
        f.write(content)

print("\n✓ All paths fixed!")
