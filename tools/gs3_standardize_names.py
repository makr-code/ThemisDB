#!/usr/bin/env python3
"""
GS3 Scanner File Standardization Tool

Renames all scanner files to follow consistent naming convention:
  gs3_step<N>_<category>_<name>.py

Where categories are:
  - ai        (AI/Vibe specific)
  - core      (C++ baseline issues)
  - check     (Syntactic checks)
  - safety    (Exception & input safety)
  - security  (Cryptography & hardening)
  - design    (Architecture & governance)
  - quality   (Documentation & API standards)
"""

import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple


RENAME_MAP: Dict[str, str] = {
    # Step 0 - Meta (keep as-is)
    "gs3_step00_uniform_full.py": None,  # No change
    
    # Step 1 - Baseline (ai_* keep, classic_* → core_*, others → core_ or check_)
    # AI scanners (keep)
    "gs3_step01_ai_error_handling_consistency.py": None,
    "gs3_step01_ai_header_drift.py": None,
    "gs3_step01_ai_llm_prompt_injection.py": None,
    "gs3_step01_ai_simulation_stub_leak.py": None,
    "gs3_step01_ai_todo_productionlogic.py": None,
    
    # Classic → Core
    "gs3_step01_classic_concurrency.py": "gs3_step01_core_concurrency.py",
    "gs3_step01_classic_container.py": "gs3_step01_core_container.py",
    "gs3_step01_classic_memory_improved.py": "gs3_step01_core_memory.py",
    "gs3_step01_classic_performance.py": "gs3_step01_core_performance.py",
    "gs3_step01_classic_platform.py": "gs3_step01_core_platform.py",
    "gs3_step01_classic_raii.py": "gs3_step01_core_raii.py",
    "gs3_step01_classic_reliability.py": "gs3_step01_core_reliability.py",
    "gs3_step01_classic_security.py": "gs3_step01_core_security.py",
    
    # No prefix → Core or Check
    "gs3_step01_error_handling.py": "gs3_step01_core_error_handling.py",
    "gs3_step01_memory_safety_improved.py": "gs3_step01_core_memory_safety.py",
    "gs3_step01_thread_safety_improved.py": "gs3_step01_core_thread_safety.py",
    "gs3_step01_braces_check.py": "gs3_step01_check_braces.py",
    "gs3_step01_namespace_unity_check.py": "gs3_step01_check_namespace_unity.py",
    
    # Note: gs3_step01_raii.py is duplicate of gs3_step01_classic_raii.py - will be deleted
    "gs3_step01_raii.py": None,  # Mark for deletion
    
    # Step 2 - Context-Aware (all → safety_*)
    "gs3_step02_exception_safety_improved.py": "gs3_step02_safety_exception.py",
    "gs3_step02_input_validation.py": "gs3_step02_safety_input_validation.py",
    "gs3_step02_type_conversion.py": "gs3_step02_safety_type_conversion.py",
    "gs3_step02_uninitialized_improved.py": "gs3_step02_safety_uninitialized.py",
    "gs3_step02_virtual_oop.py": "gs3_step02_safety_virtual_oop.py",
    
    # Step 3 - Security (all → security_*, remove _improved suffix)
    "gs3_step03_attack_vectors.py": "gs3_step03_security_attack_vectors.py",
    "gs3_step03_data_leak_improved.py": "gs3_step03_security_data_leak.py",
    "gs3_step03_e2e_encryption.py": "gs3_step03_security_e2e_encryption.py",
    "gs3_step03_encryption_leak_improved.py": "gs3_step03_security_encryption_leak.py",
    "gs3_step03_key_failure_improved.py": "gs3_step03_security_key_failure.py",
    "gs3_step03_legacy_duplication_improved.py": "gs3_step03_security_legacy_duplication.py",
    "gs3_step03_military_hardening.py": "gs3_step03_security_military_hardening.py",
    
    # Step 4 - Design & Quality (split by category, remove _improved suffix)
    # Design rules
    "gs3_step04_architecture_rules.py": "gs3_step04_design_architecture.py",
    "gs3_step04_bridge_interface_rules.py": "gs3_step04_design_bridge_interface.py",
    "gs3_step04_deprecated_apis.py": "gs3_step04_design_deprecated_apis.py",
    "gs3_step04_design_error_rules_improved.py": "gs3_step04_design_error_rules.py",
    "gs3_step04_determinism_improved.py": "gs3_step04_design_determinism.py",
    "gs3_step04_distributed_consistency_improved.py": "gs3_step04_design_distributed_consistency.py",
    "gs3_step04_gpu_memory.py": "gs3_step04_design_gpu_memory.py",
    "gs3_step04_llm_ai_safety.py": "gs3_step04_design_llm_ai_safety.py",
    "gs3_step04_module_governance_rules.py": "gs3_step04_design_module_governance.py",
    "gs3_step04_observability_improved.py": "gs3_step04_design_observability.py",
    "gs3_step04_performance_patterns_improved.py": "gs3_step04_design_performance_patterns.py",
    "gs3_step04_query_correctness.py": "gs3_step04_design_query_correctness.py",
    
    # Quality standards
    "gs3_step04_audit_logging_improved.py": "gs3_step04_quality_audit_logging.py",
    "gs3_step04_cpp_doxygen_policy_rules.py": "gs3_step04_quality_cpp_doxygen.py",
    "gs3_step04_doc_freshness_rules.py": "gs3_step04_quality_doc_freshness.py",
    "gs3_step04_docs_markdown_rules.py": "gs3_step04_quality_docs_markdown.py",
}


def get_rename_operations() -> Tuple[List[Tuple[str, str]], List[str]]:
    """
    Get list of (old_name, new_name) and list of files to delete.
    """
    renames = []
    deletes = []
    
    for old_name, new_name in RENAME_MAP.items():
        if new_name is None:
            if old_name != "gs3_step00_uniform_full.py" and not old_name.startswith("gs3_step01_ai_"):
                # Mark for deletion if not kept
                deletes.append(old_name)
        else:
            renames.append((old_name, new_name))
    
    return renames, deletes


def execute_renames(scanners_dir: Path, dry_run: bool = False) -> bool:
    """
    Execute all rename operations.
    
    Args:
        scanners_dir: Path to tools/scanners directory
        dry_run: If True, show what would be done without doing it
        
    Returns:
        True if successful, False otherwise
    """
    renames, deletes = get_rename_operations()
    
    print("\n" + "=" * 100)
    print("GS3 SCANNER FILE STANDARDIZATION")
    print("=" * 100 + "\n")
    
    if dry_run:
        print("DRY RUN MODE - No changes will be made\n")
    
    # Show renames
    print(f"RENAME OPERATIONS ({len(renames)} files):\n")
    for old_name, new_name in renames:
        old_path = scanners_dir / old_name
        new_path = scanners_dir / new_name
        
        if not old_path.exists():
            print(f"  [!] {old_name} -> {new_name}")
            print(f"      (source file not found)")
        else:
            if new_path.exists():
                print(f"  [E] {old_name} -> {new_name}")
                print(f"      (target file already exists)")
            else:
                status = "(would rename)" if dry_run else "(renaming)"
                print(f"  [+] {old_name} -> {new_name} {status}")
                
                if not dry_run:
                    try:
                        old_path.rename(new_path)
                        print(f"      [OK] Done")
                    except Exception as e:
                        print(f"      [E] Error: {e}")
                        return False
    
    # Show deletions
    if deletes:
        print(f"\n\nDELETION OPERATIONS ({len(deletes)} files):\n")
        for filename in deletes:
            file_path = scanners_dir / filename
            
            if not file_path.exists():
                print(f"  [!] {filename} (not found, skipping)")
            else:
                status = "(would delete)" if dry_run else "(deleting)"
                print(f"  [-] {filename} {status}")
                
                if not dry_run:
                    try:
                        file_path.unlink()
                        print(f"      [OK] Done")
                    except Exception as e:
                        print(f"      [E] Error: {e}")
                        return False
    
    print("\n" + "=" * 100)
    
    if dry_run:
        print("DRY RUN COMPLETE - No changes were made")
        print("\nRun with --execute to apply changes")
    else:
        print("[OK] STANDARDIZATION COMPLETE")
        print("\nAll scanner files renamed to follow convention:")
        print("  gs3_step<N>_<category>_<name>.py")
    
    print("=" * 100 + "\n")
    
    return True


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Standardize GS3 scanner file names",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python gs3_standardize_names.py --dry-run
  python gs3_standardize_names.py --execute
        """
    )
    
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done without making changes"
    )
    
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Actually execute the renames/deletions"
    )
    
    args = parser.parse_args()
    
    # Default to dry-run if neither is specified
    if not args.dry_run and not args.execute:
        args.dry_run = True
    
    scanners_dir = Path(__file__).parent / "scanners"
    
    if not scanners_dir.exists():
        print(f"Error: Scanners directory not found: {scanners_dir}")
        return 1
    
    success = execute_renames(scanners_dir, dry_run=args.dry_run)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
