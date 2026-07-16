#!/usr/bin/env python3
"""
Test script to verify the braces scanner fix.
"""

import sys
from pathlib import Path

# Add tools directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from scanners.gs3_step01_check_braces import BracesCheckScanner

def test_scanner():
    """Test the braces scanner on a sample C++ file."""
    
    # Create a test C++ file with forward declarations (should NOT generate false positives)
    test_content = '''
// Test file with forward declarations
class ForwardDeclared;  // This should NOT be flagged as unclosed
struct AnotherDecl;    // This should NOT be flagged as unclosed

namespace TestNamespace {
    class RealClass {
    public:
        void method() {
            // code
        }
    };
    
    struct RealStruct {
        int field;
    };
    
    void realFunction() {
        // code
    }
}
'''
    
    # Write test file
    test_file = Path('/tmp/test_braces.cpp')
    test_file.write_text(test_content)
    
    # Run scanner
    scanner = BracesCheckScanner()
    gaps = scanner.scan(str(test_file.parent))
    
    print(f"Test Results:")
    print(f"Files scanned: {scanner.files_scanned}")
    print(f"Gaps found: {len(gaps)}")
    
    for gap in gaps:
        print(f"  [{gap.type}] {gap.file}:{gap.line} - {gap.description}")
    
    # Check for false positives
    false_positives = [g for g in gaps if 'ForwardDeclared' in g.description or 'AnotherDecl' in g.description]
    
    if false_positives:
        print(f"\n❌ FALSE POSITIVES DETECTED: {len(false_positives)}")
        for fp in false_positives:
            print(f"  {fp.description}")
        return False
    else:
        print(f"\n✅ NO FALSE POSITIVES - Fix is working!")
        return True

if __name__ == "__main__":
    success = test_scanner()
    sys.exit(0 if success else 1)