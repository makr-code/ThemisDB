#!/usr/bin/env python3
"""
Add missing Mermaid diagrams to 13 chapters
"""

from pathlib import Path

# Mapping of chapters to diagrams
CHAPTER_DIAGRAMS = {
    'chapter_23_testing_qa.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '23.0',
                'title': 'CI/CD Test-Pipeline',
                'desc': 'Automatisierte Qualitätssicherung auf mehreren Ebenen',
                'mermaid': '''graph TB
    Commit[Code Commit] --> Build[Build & Compile]
    Build --> UnitTests[Unit Tests<br/>Functions & Logic]
    UnitTests --> IntTests[Integration Tests<br/>Transactions & Data]
    IntTests --> E2E[E2E Tests<br/>Full Workflows]
    E2E --> QualityGate{Quality Gate<br/>Coverage greater than 80 percent}
    QualityGate -->|Pass| Deploy[Deploy to Staging]
    QualityGate -->|Fail| Notify[Notify Developers]
    
    style QualityGate fill:#f093fb
    style Deploy fill:#43e97b
    style Notify fill:#ff6b6b'''
            }
        ]
    },
    'chapter_26_migration_legacy.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '26.0',
                'title': 'Migration-Strategie: Strangler Pattern',
                'desc': 'Schrittweise Migration vom Legacy-System zu ThemisDB',
                'mermaid': '''sequenceDiagram
    participant Legacy
    participant Proxy
    participant ThemisDB
    
    Note over Legacy,ThemisDB: Phase 1: Dual Write
    Proxy->>Legacy: Write
    Proxy->>ThemisDB: Write (shadow)
    
    Note over Legacy,ThemisDB: Phase 2: Validation
    Proxy->>Legacy: Read
    Proxy->>ThemisDB: Read (compare)
    
    Note over Legacy,ThemisDB: Phase 3: Cutover
    Proxy->>ThemisDB: Read slash Write (primary)
    Proxy->>Legacy: Write (backup)
    
    Note over Legacy,ThemisDB: Phase 4: Decommission
    Proxy->>ThemisDB: Read slash Write (only)'''
            }
        ]
    },
    'chapter_27_troubleshooting.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '27.0',
                'title': 'Troubleshooting-Decision-Tree',
                'desc': 'Symptomatische Diagnose und systematische Fehlerbehebung',
                'mermaid': '''flowchart TD
    Problem[Performance Issue] --> Symptoms{Symptom<br/>Analysis}
    
    Symptoms -->|High Latency| LatencyCheck[Check Query Plan]
    Symptoms -->|High CPU| CPUCheck[Check Resource Usage]
    Symptoms -->|Memory Issues| MemCheck[Check Cache Hit Rate]
    
    LatencyCheck --> Index{Index<br/>Missing?}
    Index -->|Yes| AddIndex[Add Index]
    Index -->|No| QueryOpt[Optimize Query]
    
    CPUCheck --> Parallel{Parallel<br/>Queries?}
    Parallel -->|Yes| LimitConn[Limit Connections]
    Parallel -->|No| BadQuery[Identify Bad Query]
    
    MemCheck --> CacheSize{Cache<br/>Too Small?}
    CacheSize -->|Yes| IncCache[Increase Cache Size]
    CacheSize -->|No| CheckLeak[Check Memory Leak]
    
    style Problem fill:#ff6b6b
    style AddIndex fill:#43e97b
    style IncCache fill:#43e97b'''
            }
        ]
    }
}

def add_diagrams_to_chapter(filepath, diagrams_info):
    """Add diagrams to a chapter file"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    for diagram in diagrams_info['diagrams']:
        figure_block = f"""
<figure>

```mermaid
{diagram['mermaid']}
```

<figcaption><b>Abb. {diagram['num']}:</b> {diagram['title']}</figcaption>
</figure>

---"""
        
        # Find insertion point
        insert_after = diagrams_info['insert_after']
        if insert_after in content:
            pos = content.find(insert_after)
            if pos != -1:
                # Find the end of that section
                next_heading = content.find('\n##', pos + len(insert_after))
                if next_heading == -1:
                    next_heading = len(content)
                
                # Insert before the next heading
                content = content[:next_heading] + figure_block + '\n' + content[next_heading:]
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    
    return True

def main():
    compendium_dir = Path(__file__).parent
    
    print("="*70)
    print("  Adding Mermaid Diagrams to Chapters")
    print("="*70)
    print()
    
    for chapter_file, diagram_info in CHAPTER_DIAGRAMS.items():
        filepath = compendium_dir / chapter_file
        if filepath.exists():
            add_diagrams_to_chapter(filepath, diagram_info)
            print(f"[OK] Updated: {chapter_file} ({len(diagram_info['diagrams'])} diagrams)")
        else:
            print(f"[SKIP] Not found: {chapter_file}")
    
    print()
    print("="*70)
    print("Complete! Added diagrams to key chapters.")
    print("="*70)

if __name__ == "__main__":
    main()
