#!/usr/bin/env python3
"""
Lösche alte und doppelte Chapter-Dateien - AUTOMODE
"""
from pathlib import Path

CORRECT_FILES = {
    "preface.md", "index.md",
    "chapter_01_introduction.md", "chapter_02_architecture.md", "chapter_03_multimodel.md",
    "chapter_04_installation.md", "chapter_05_relational.md", "chapter_06_graph.md",
    "chapter_07_document.md", "chapter_08_vector.md", "chapter_09_timeseries.md",
    "chapter_10_enterprise.md", "chapter_11_realtime.md", "chapter_12_computervision.md",
    "chapter_13_fulltext.md", "chapter_14_geospatial.md", "chapter_15_analytics.md",
    "chapter_16_ml.md", "chapter_17_scaling.md", "chapter_18_ha.md", "chapter_19_monitoring.md",
    "chapter_20_performance.md", "chapter_21_auth.md", "chapter_22_encryption.md",
    "chapter_23_testing_qa.md", "chapter_24_ai_ethics.md", "chapter_25_devops_infrastructure.md",
    "chapter_26_migration_legacy.md", "chapter_27_troubleshooting.md", "chapter_28_aql_reference.md",
    "chapter_29_analytics_process_mining.md", "chapter_30_deployment_operations.md",
    "chapter_31_api_protocols.md", "chapter_32_aql_oop_implementation.md", "chapter_33_best_practices.md",
    "chapter_34_query_optimization.md", "chapter_35_data_modeling_patterns.md",
    "chapter_36_security_hardening.md", "chapter_37_ecosystem_integration.md",
    "chapter_38_observability_sre.md", "chapter_39_performance_tuning_cookbook.md",
    "chapter_40_data_governance_compliance.md", "chapter_41_hands_on_labs.md",
    "appendix_literatur.md", "appendix_d_feature_status.md", "appendix_e_incident_runbooks.md",
    "appendix_f_aql_cheatsheet.md", "appendix_g_configuration.md", "appendix_h_glossary.md",
    "appendix_i_troubleshooting.md",
}

compendium_dir = Path(".")
deleted_count = 0

for fpath in compendium_dir.glob("chapter_*.md"):
    if fpath.name not in CORRECT_FILES:
        fpath.unlink()
        deleted_count += 1
        print(f"🗑️  {fpath.name}")

for fpath in compendium_dir.glob("appendix_*.md"):
    if fpath.name not in CORRECT_FILES:
        fpath.unlink()
        deleted_count += 1
        print(f"🗑️  {fpath.name}")

print(f"\n✅ {deleted_count} veraltete Dateien gelöscht")
print(f"✓ {len(CORRECT_FILES)} korrekte Dateien bleiben")
