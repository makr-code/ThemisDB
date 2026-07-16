#!/usr/bin/env python3
"""Debug wrapper scanner detection."""

import tempfile
from pathlib import Path
import sys
import os
import re

sys.path.insert(0, str(Path(__file__).parent.parent))

from scanners.gs3_step04_design_wrapper_abstraction_excess import WrapperAbstractionExcessScanner

code = """
class Item {
public:
    Item() {}
    int getValue() { return value_; }
private:
    int value_;
};

// Thin wrapper - no added value, just passthrough
class ItemWrapper {
public:
    ItemWrapper(Item* item) : item_(item) {}
    int getValue() { return item_->getValue(); }
private:
    Item* item_;
};
"""

scanner = WrapperAbstractionExcessScanner()

with tempfile.TemporaryDirectory() as tmpdir:
    test_file = Path(tmpdir) / "test.cpp"
    test_file.write_text(code)
    
    print(f"Test file: {test_file}")
    print(f"File exists: {test_file.exists()}")
    print(f"File content:\n{code}\n")
    
    # Test member variable detection directly on ItemWrapper class body
    class_body = """
{
public:
    ItemWrapper(Item* item) : item_(item) {}
    int getValue() { return item_->getValue(); }
private:
    Item* item_;
}
"""
    
    print("=== DIRECT METHOD TESTS ===")
    print(f"\nTesting class body:\n{class_body}")
    
    # Test _has_member_variables
    has_state = scanner._has_member_variables(class_body)
    print(f"  has_member_variables: {has_state}")
    
    # Test _check_if_wrapper
    wraps = scanner._check_if_wrapper(class_body)
    print(f"  _check_if_wrapper: {wraps}")
    
    # Test _calculate_wrapper_depth
    depth = scanner._calculate_wrapper_depth(class_body)
    print(f"  wrapper_depth: {depth}")
    
    # Test the scanner on full file
    print("\n=== SCANNER RESULTS ===")
    scanner.scan_file(str(test_file))
    
    print(f"Total gaps found: {len(scanner.gaps)}")
    print(f"File classes found: {len(scanner.file_classes.get(str(test_file), []))}")
    
    if str(test_file) in scanner.file_classes:
        for cls in scanner.file_classes[str(test_file)]:
            print(f"\nClass: {cls.name}")
            print(f"  Line: {cls.line}")
            print(f"  Methods: {cls.methods}")
            print(f"  Has state: {cls.has_state}")
            print(f"  Wraps other: {cls.wraps_other_class}")
            print(f"  Wrapper depth: {cls.wrapper_depth}")
    
    print(f"\nGaps detected:")
    for gap in scanner.gaps:
        print(f"  {gap.type} at line {gap.line}: {gap.description}")
