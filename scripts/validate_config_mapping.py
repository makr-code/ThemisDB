"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            validate_config_mapping.py                         ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:40:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     189                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Validate the config path mapping table.

This script validates:
1. All paths in the C++ mapping table are present
2. No duplicate legacy paths exist
3. New paths follow hierarchical structure
4. All category values are valid
"""

import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

# Valid categories from the schema
VALID_CATEGORIES = {
    "ai_ml",
    "security",
    "compliance",
    "data_management",
    "performance",
    "deprecated",
    "core",
    "platform",
    "networking",
    "content",
    "monitoring",
    "features",
    "assistants",
    "processing",
    "licensing",
}

# Expected hierarchical structure
CATEGORY_PATH_MAPPING = {
    "ai_ml": "config/ai_ml/",
    "security": "config/security/",
    "compliance": "config/compliance/",
    "data_management": "config/data_management/",
    "performance": "config/performance/",
    "deprecated": "config/deprecated/",
    "core": "config/core/",
    "platform": "config/platform/",
    "networking": "config/networking/",
    "content": "config/content/",
    "monitoring": "config/monitoring/",
    "features": "config/features/",
    "assistants": "config/assistants/",
    "processing": "config/processing/",
    "licensing": "config/licensing/",
}


def extract_mappings_from_cpp(cpp_file: Path) -> List[Tuple[str, str]]:
    """Extract path mappings from the C++ source file."""
    mappings = []
    
    with open(cpp_file, 'r') as f:
        content = f.read()
    
    # Find the mapping table initialization
    pattern = r'\{"([^"]+)",\s*"([^"]+)"\}'
    matches = re.findall(pattern, content)
    
    for legacy_path, new_path in matches:
        mappings.append((legacy_path, new_path))
    
    return mappings


def infer_category(new_path: str) -> str:
    """Infer category from the new path."""
    for category, prefix in CATEGORY_PATH_MAPPING.items():
        if new_path.startswith(prefix):
            return category
    return "unknown"


def validate_mappings(mappings: List[Tuple[str, str]]) -> Tuple[bool, List[str]]:
    """Validate the mapping table."""
    errors = []
    warnings = []
    seen_legacy = set()
    seen_new = set()
    
    for legacy_path, new_path in mappings:
        # Check for duplicate legacy paths
        if legacy_path in seen_legacy:
            errors.append(f"Duplicate legacy path: {legacy_path}")
        seen_legacy.add(legacy_path)
        
        # Check for duplicate new paths (warning, not error)
        if new_path in seen_new:
            warnings.append(f"Duplicate new path: {new_path}")
        seen_new.add(new_path)
        
        # Validate path format
        if not legacy_path.startswith("config/"):
            errors.append(f"Legacy path does not start with 'config/': {legacy_path}")
        
        if not new_path.startswith("config/"):
            errors.append(f"New path does not start with 'config/': {new_path}")
        
        # Validate hierarchical structure (should have at least one subdirectory)
        new_path_parts = new_path.split("/")
        if len(new_path_parts) < 3:  # config/category/file.yaml
            errors.append(
                f"New path does not follow hierarchical structure: {new_path}"
            )
        
        # Infer and validate category
        category = infer_category(new_path)
        if category == "unknown":
            warnings.append(f"Could not infer category for new path: {new_path}")
        elif category not in VALID_CATEGORIES:
            errors.append(f"Invalid category '{category}' inferred from: {new_path}")
        
        # Check that legacy and new paths are different
        if legacy_path == new_path:
            warnings.append(f"Legacy and new path are identical: {legacy_path}")
    
    # Print warnings
    if warnings:
        print(f"\n⚠️  Warnings ({len(warnings)}):")
        for warning in warnings:
            print(f"  - {warning}")
    
    return len(errors) == 0, errors


def main():
    """Main validation function."""
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    cpp_file = repo_root / "src" / "config" / "config_path_resolver.cpp"
    
    if not cpp_file.exists():
        print(f"❌ Error: Could not find {cpp_file}")
        return 1
    
    print("🔍 Validating config path mapping table...")
    print(f"   Source: {cpp_file.relative_to(repo_root)}")
    
    # Extract mappings
    mappings = extract_mappings_from_cpp(cpp_file)
    print(f"\n📊 Found {len(mappings)} path mappings")
    
    # Validate
    is_valid, errors = validate_mappings(mappings)
    
    if is_valid:
        print("\n✅ All validations passed!")
        print(f"   - {len(mappings)} mappings validated")
        print(f"   - All paths follow hierarchical structure")
        print(f"   - No duplicate legacy paths")
        return 0
    else:
        print(f"\n❌ Validation failed with {len(errors)} error(s):")
        for error in errors:
            print(f"  - {error}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
