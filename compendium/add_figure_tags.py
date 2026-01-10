#!/usr/bin/env python3
"""
Add <figure> tags and captions to all Mermaid diagrams.
Also suggest new diagram opportunities.
"""

import re
from pathlib import Path

# Mapping of diagram contexts to caption suggestions
DIAGRAM_SUGGESTIONS = {
    'chapter_01_introduction.md': {
        52: "ThemisDB Multi-Model Architektur",
        111: "Datenmodell-Übersicht",
        147: "Query-Processing-Pipeline",
        215: "Storage-Engine-Architektur",
        249: "Use-Case-Szenarien"
    },
    'chapter_02_architecture.md': {
        132: "Systemarchitektur-Übersicht",
        187: "Query-Engine-Komponenten",
        267: "Storage-Layer-Struktur",
        1246: "Transaction-Management-Flow",
        1300: "MVCC-Versionskontrolle",
        1352: "Index-Strukturen",
        1400: "Replication-Topologie",
        1519: "Failover-Mechanismus"
    },
    'chapter_03_multimodel.md': {
        209: "Datenmodell-Entscheidungsmatrix",
        220: "Multi-Model-Data-Flow"
    },
    'chapter_04_installation.md': {
        76: "Installations-Ablaufdiagramm"
    },
    'chapter_05_relational.md': {
        306: "Relationales Schema-Design",
        345: "Join-Optimierung-Strategie"
    },
    'chapter_06_graph.md': {
        81: "Graph-Traversierung-Algorithmus",
        170: "Shortest-Path-Berechnung",
        234: "Community-Detection-Workflow",
        258: "Pagerank-Algorithmus-Visualisierung"
    },
    'chapter_07_document.md': {
        89: "Dokument-Store-Architektur"
    },
    'chapter_08_storage_layer.md': {
        46: "Storage-Engine-Internals"
    },
    'chapter_08_vector.md': {
        28: "Vector-Embedding-Pipeline",
        94: "Similarity-Search-Ablauf",
        198: "Index-Struktur für Vektoren"
    },
    'chapter_09_timeseries.md': {
        36: "Timeseries-Data-Ingestion",
        94: "Aggregation-Pipeline",
        238: "Downsampling-Strategie"
    },
    'chapter_10_enterprise.md': {
        51: "Enterprise-Architektur-Übersicht",
        149: "RBAC-Hierarchie",
        1816: "Audit-Log-Flow",
        1938: "Compliance-Workflow"
    },
    'chapter_11_realtime.md': {
        19: "Real-time-Streaming-Architektur",
        105: "Change-Stream-Processing"
    },
    'chapter_12_computervision.md': {
        43: "Computer-Vision-Pipeline"
    },
    'chapter_13_fulltext.md': {
        97: "Fulltext-Search-Indexierung"
    },
    'chapter_14_geospatial.md': {
        102: "Geospatial-Index-Struktur",
        193: "Proximity-Search-Algorithmus"
    },
    'chapter_15_analytics.md': {
        19: "Analytics-Query-Pipeline"
    },
    'chapter_16_sharding.md': {
        73: "Replication vs Sharding Vergleich",
        142: "Hash-Based-Routing-Flow",
        695: "Shard-Key-Selection-Matrix",
        820: "Hot-Spare-Failover-Timeline",
        843: "Failover-Sequenzdiagramm",
        953: "Elastic-Sharding-Workflow"
    },
    'chapter_17_llm_integration.md': {
        14: "LLM-Integration-Architektur",
        261: "RAG-Pipeline-Flow",
        1414: "Embedding-Generierung-Prozess",
        1506: "Response-Caching-Flow",
        1717: "Context-Window-Management",
        1942: "Token-Optimization-Strategy",
        2278: "Multi-LLM-Orchestration"
    },
    'chapter_18_ml.md': {
        14: "ML-Pipeline-Architektur"
    },
    'chapter_19_monitoring_observability.md': {
        64: "Monitoring-Stack-Übersicht",
        1250: "Alert-Management-Workflow"
    },
    'chapter_20_backup.md': {
        70: "Backup-Strategy-Overview",
        92: "Point-in-Time-Recovery-Timeline"
    },
    'chapter_21_performance.md': {
        34: "Query-Performance-Optimization-Flow",
        724: "Index-Selection-Strategy",
        760: "Cache-Hierarchy-Diagram",
        879: "Query-Plan-Optimization",
        1017: "Resource-Allocation-Matrix",
        1033: "Performance-Tuning-Workflow"
    },
    'chapter_22_clients.md': {
        26: "Client-SDK-Architektur"
    },
    'chapter_24_ai_ethics.md': {
        655: "Ethical-Framework-Overview",
        676: "Bias-Detection-Pipeline",
        705: "Context-Recognition-Flow",
        745: "Augmentation-Templates-Matrix",
        824: "Ethical-Guardrails-Workflow",
        884: "Privacy-Protection-Layers",
        1048: "Fairness-Evaluation-Metrics",
        1106: "Transparency-Reporting-Flow"
    },
    'chapter_25_devops_infrastructure.md': {
        19: "DevOps-Pipeline-Architektur"
    },
    'chapter_29_analytics_process_mining.md': {
        34: "Process-Mining-Pipeline",
        174: "Event-Log-Processing",
        328: "Process-Discovery-Algorithm",
        422: "Conformance-Checking-Flow"
    },
    'chapter_30_deployment_operations.md': {
        656: "Deployment-Strategy-Matrix"
    },
    'chapter_31_api_protocols.md': {
        24: "API-Protocol-Stack",
        85: "Request-Response-Flow"
    },
    'chapter_33_best_practices.md': {
        84: "Best-Practices-Decision-Tree"
    }
}

def wrap_mermaid_in_figure(filepath):
    """Add <figure> and <figcaption> tags around Mermaid diagrams."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Skip if no Mermaid blocks
    if '```mermaid' not in content:
        return False
    
    # Get caption suggestions for this file
    suggestions = DIAGRAM_SUGGESTIONS.get(filepath.name, {})
    
    # Find all Mermaid blocks
    mermaid_pattern = r'(```mermaid\n.*?\n```)'
    matches = list(re.finditer(mermaid_pattern, content, re.DOTALL))
    
    if not matches:
        return False
    
    # Process from end to start to preserve positions
    modified = False
    offset = 0
    
    for i, match in enumerate(reversed(matches), 1):
        diagram_num = len(matches) - i + 1
        start_pos = match.start()
        end_pos = match.end()
        
        # Check if already wrapped
        context_before = content[max(0, start_pos-50):start_pos]
        context_after = content[end_pos:min(len(content), end_pos+50)]
        
        if '<figure>' in context_before:
            continue  # Already wrapped
        
        # Get line number for caption suggestion
        line_num = content[:start_pos].count('\n') + 1
        caption_text = suggestions.get(line_num, f"Diagramm {diagram_num}")
        
        # Get chapter number from filename
        chapter_match = re.search(r'chapter_(\d+)', filepath.name)
        chapter_num = chapter_match.group(1) if chapter_match else "0"
        
        # Wrap the diagram
        mermaid_block = match.group(0)
        wrapped = f'<figure>\n\n{mermaid_block}\n\n<figcaption><b>Abb. {chapter_num}.{diagram_num}:</b> {caption_text}</figcaption>\n</figure>'
        
        # Replace in content
        content = content[:start_pos] + wrapped + content[end_pos:]
        modified = True
    
    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    
    return False

def main():
    compendium_dir = Path(__file__).parent
    chapter_files = sorted(compendium_dir.glob("chapter_*.md"))
    
    # Skip chapter_00 as it already has proper formatting
    chapter_files = [f for f in chapter_files if f.name != 'chapter_00_genesis.md']
    
    print("="*80)
    print("  Adding <figure> tags and captions to Mermaid diagrams")
    print("="*80)
    print()
    
    modified_count = 0
    
    for filepath in chapter_files:
        if wrap_mermaid_in_figure(filepath):
            print(f"[OK] Updated: {filepath.name}")
            modified_count += 1
        else:
            print(f"     Skipped: {filepath.name}")
    
    print()
    print("="*80)
    print(f"Modified {modified_count} files")
    print("="*80)

if __name__ == "__main__":
    main()
