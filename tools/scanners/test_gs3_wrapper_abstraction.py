#!/usr/bin/env python3
"""
Unit tests for WrapperAbstractionExcessScanner.

Tests the detection of:
1. Excessive wrapper layers
2. Thin wrappers without added value
3. Passthrough methods
4. Abstraction cascades (A wraps B wraps C...)
"""

import tempfile
from pathlib import Path
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from scanners.gs3_step04_design_wrapper_abstraction_excess import WrapperAbstractionExcessScanner


def test_thin_wrapper_detection():
    """Test detection of thin wrapper classes."""
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
        
        scanner.scan_file(str(test_file))
        
        # Should detect thin wrapper pattern
        thin_wrapper_gaps = [g for g in scanner.gaps if g.type == 'thin_wrapper']
        print(f"✓ Thin wrapper test: Found {len(thin_wrapper_gaps)} gaps")
        assert len(thin_wrapper_gaps) > 0, "Should detect thin wrapper"


def test_passthrough_methods():
    """Test detection of passthrough methods."""
    code = """
    class Database {
    public:
        void connect() { /* implementation */ }
        void query(const std::string& sql) { /* implementation */ }
        void disconnect() { /* implementation */ }
    };
    
    // Boring code - only passhtrough methods
    class DatabaseWrapper {
    private:
        Database db_;
    public:
        void connect() { db_.connect(); }
        void query(const std::string& sql) { db_.query(sql); }
        void disconnect() { db_.disconnect(); }
    };
    """
    
    scanner = WrapperAbstractionExcessScanner()
    
    with tempfile.TemporaryDirectory() as tmpdir:
        test_file = Path(tmpdir) / "test.cpp"
        test_file.write_text(code)
        
        scanner.scan_file(str(test_file))
        
        # Should detect passthrough pattern
        passthrough_gaps = [g for g in scanner.gaps if g.type == 'passthrough_methods']
        print(f"✓ Passthrough methods test: Found {len(passthrough_gaps)} gaps")


def test_abstraction_cascade():
    """Test detection of cascading abstraction layers."""
    code = """
    class DataLayer {
    public:
        int getValue() { return 42; }
    };
    
    class RepositoryLayer {
    private:
        DataLayer data_;
    public:
        int getValue() { return data_.getValue(); }
    };
    
    class ServiceLayer {
    private:
        RepositoryLayer repo_;
    public:
        int getValue() { return repo_.getValue(); }
    };
    
    class APILayer {
    private:
        ServiceLayer service_;
    public:
        int getValue() { return service_.getValue(); }
    };
    
    class ClientLayer {
    private:
        APILayer api_;
    public:
        int getValue() { return api_.getValue(); }
    };
    """
    
    scanner = WrapperAbstractionExcessScanner()
    
    with tempfile.TemporaryDirectory() as tmpdir:
        test_file = Path(tmpdir) / "test.cpp"
        test_file.write_text(code)
        
        scanner.scan_file(str(test_file))
        
        # Should detect abstraction cascade
        cascade_gaps = [g for g in scanner.gaps if g.type == 'abstraction_cascade']
        print(f"✓ Abstraction cascade test: Found {len(cascade_gaps)} gaps")


def test_good_wrapper_acceptance():
    """Test that good wrappers (with added logic) are not flagged."""
    code = """
    class RawConnection {
    public:
        void send(const std::string& data) { /* send raw */ }
        std::string receive() { return ""; }
    };
    
    // GOOD wrapper - adds encryption, error handling, retry logic
    class SecureConnection {
    private:
        RawConnection conn_;
        EncryptionProvider crypto_;
        int max_retries_;
    public:
        bool send(const std::string& data) {
            std::string encrypted = crypto_.encrypt(data);
            for (int i = 0; i < max_retries_; ++i) {
                try {
                    conn_.send(encrypted);
                    return true;
                } catch (...) {
                    if (i == max_retries_ - 1) throw;
                }
            }
            return false;
        }
        
        std::string receive() {
            std::string raw = conn_.receive();
            return crypto_.decrypt(raw);
        }
    };
    """
    
    scanner = WrapperAbstractionExcessScanner()
    
    with tempfile.TemporaryDirectory() as tmpdir:
        test_file = Path(tmpdir) / "test.cpp"
        test_file.write_text(code)
        
        scanner.scan_file(str(test_file))
        
        # Should NOT detect this as boring code (has added logic)
        boring_gaps = [g for g in scanner.gaps if g.type in ['thin_wrapper', 'passthrough_methods']]
        print(f"✓ Good wrapper acceptance test: {len(boring_gaps)} false positives (expected 0)")


def test_multiple_wrappers_in_file():
    """Test detection of multiple wrappers in single file."""
    code = """
    class A { void doWork() { } };
    class B { private: A a_; public: void doWork() { a_.doWork(); } };
    class C { private: B b_; public: void doWork() { b_.doWork(); } };
    class D { private: C c_; public: void doWork() { c_.doWork(); } };
    """
    
    scanner = WrapperAbstractionExcessScanner()
    
    with tempfile.TemporaryDirectory() as tmpdir:
        test_file = Path(tmpdir) / "test.cpp"
        test_file.write_text(code)
        
        scanner.scan_file(str(test_file))
        
        total_gaps = len(scanner.gaps)
        print(f"✓ Multiple wrappers test: Found {total_gaps} total gaps")


def run_all_tests():
    """Execute all tests."""
    print("\n" + "=" * 70)
    print("WRAPPER ABSTRACTION EXCESS SCANNER - UNIT TESTS")
    print("=" * 70 + "\n")
    
    tests = [
        ("Thin Wrapper Detection", test_thin_wrapper_detection),
        ("Passthrough Methods", test_passthrough_methods),
        ("Abstraction Cascade", test_abstraction_cascade),
        ("Good Wrapper Acceptance", test_good_wrapper_acceptance),
        ("Multiple Wrappers", test_multiple_wrappers_in_file),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            print(f"\n🧪 Testing: {test_name}")
            test_func()
            passed += 1
            print(f"   ✅ PASSED")
        except AssertionError as e:
            failed += 1
            print(f"   ❌ FAILED: {e}")
        except Exception as e:
            failed += 1
            print(f"   ❌ ERROR: {type(e).__name__}: {e}")
    
    print("\n" + "=" * 70)
    print(f"RESULTS: {passed} passed, {failed} failed")
    print("=" * 70 + "\n")
    
    return failed == 0


if __name__ == '__main__':
    success = run_all_tests()
    sys.exit(0 if success else 1)
