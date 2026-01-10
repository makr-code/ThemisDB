#!/usr/bin/env python3
"""
Add all remaining Mermaid diagrams to 10 more chapters
"""

from pathlib import Path

# Complete diagram mapping for all 13 chapters
CHAPTER_DIAGRAMS_COMPLETE = {
    'chapter_28_aql_reference.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '28.0',
                'title': 'AQL-Query-Execution-Pipeline',
                'desc': 'Vom AQL-Statement bis zum Query-Result',
                'mermaid': '''graph LR
    AQL[AQL Query] --> Parser[Parser]
    Parser --> AST[Abstract Syntax Tree]
    AST --> Optimizer[Query Optimizer]
    
    Optimizer --> LogPlan[Logical Plan]
    LogPlan --> PhysPlan[Physical Plan]
    
    PhysPlan --> IndexSel[Index Selection]
    PhysPlan --> JoinOrd[Join Ordering]
    PhysPlan --> PushDown[Predicate Pushdown]
    
    IndexSel --> Execution[Execution Engine]
    JoinOrd --> Execution
    PushDown --> Execution
    
    Execution --> Result[Query Result]
    
    style Optimizer fill:#f093fb
    style Execution fill:#4facfe'''
            }
        ]
    },
    'chapter_32_aql_oop_implementation.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '32.0',
                'title': 'AQL Class-Hierarchie und Objekt-Lifecycle',
                'desc': 'OOP-Konzepte in AQL: Vererbung und Objektbeziehungen',
                'mermaid': '''classDiagram
    class Entity {
        +String _key
        +DateTime created_at
        +DateTime updated_at
        +save()
        +delete()
        +validate()
    }
    
    class User {
        +String email
        +String name
        +authenticate()
        +getPermissions()
    }
    
    class Product {
        +String sku
        +Decimal price
        +calculateDiscount()
        +checkInventory()
    }
    
    class Order {
        +String order_id
        +Array items
        +calculateTotal()
        +processPayment()
    }
    
    Entity <|-- User
    Entity <|-- Product
    Entity <|-- Order
    
    User "1" --> "*" Order: places
    Order "1" --> "*" Product: contains'''
            }
        ]
    },
    'chapter_34_query_optimization.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '34.0',
                'title': 'Query-Plan-Optimierung',
                'desc': 'Cost-Based Optimization für Execution Plans',
                'mermaid': '''graph TB
    Query["SELECT * FROM users<br/>WHERE age greater than 25<br/>AND city = Berlin"] --> Optimizer[Query Optimizer]
    
    Optimizer --> Plan1["Plan 1:<br/>Index Scan on age<br/>Filter city"]
    Optimizer --> Plan2["Plan 2:<br/>Index Scan on city<br/>Filter age"]
    Optimizer --> Plan3["Plan 3:<br/>Composite Index<br/>age plus city"]
    
    Plan1 --> Cost1[Cost: 1200]
    Plan2 --> Cost2[Cost: 800]
    Plan3 --> Cost3[Cost: 150]
    
    Cost1 --> Select{Select<br/>Best Plan}
    Cost2 --> Select
    Cost3 --> Select
    
    Select --> Execute[Execute Plan 3]
    
    style Plan3 fill:#43e97b
    style Execute fill:#4facfe'''
            }
        ]
    },
    'chapter_35_data_modeling_patterns.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '35.0',
                'title': 'Data-Modeling-Patterns',
                'desc': 'Embedded vs Referenced vs Hybrid Schemas',
                'mermaid': '''graph LR
    subgraph "Embedded (1:Few)"
        UserEmb[User] --> AddressEmb[Addresses Array]
    end
    
    subgraph "Referenced (1:Many)"
        UserRef[User] --> OrderRef[Orders Collection]
        OrderRef --> OrderDoc1[Order 1]
        OrderRef --> OrderDoc2[Order 2]
        OrderRef --> OrderDoc3["Order N..."]
    end
    
    subgraph "Hybrid (Best of Both)"
        UserHyb[User] --> AddressHyb[Address Embedded]
        UserHyb --> OrderSummary[Recent Orders Embedded]
        UserHyb --> OrderRefHyb[All Orders Referenced]
    end
    
    style UserEmb fill:#4facfe
    style UserRef fill:#f093fb
    style UserHyb fill:#43e97b'''
            }
        ]
    },
    'chapter_36_security_hardening.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '36.0',
                'title': 'Security-Layers: Defense in Depth',
                'desc': 'Mehrschichtige Sicherheitsarchitektur',
                'mermaid': '''graph TB
    Client[Client Application] --> TLS[TLS slash SSL Layer]
    TLS --> Auth[Authentication]
    
    Auth --> JWT{JWT Token<br/>Validation}
    JWT -->|Valid| RBAC[RBAC Check]
    JWT -->|Invalid| Reject1[Reject 401]
    
    RBAC --> Perm{Permission<br/>Check}
    Perm -->|Granted| EncData[Encrypted Data Access]
    Perm -->|Denied| Reject2[Reject 403]
    
    EncData --> Decrypt[Decrypt at Runtime]
    Decrypt --> Result[Return Data]
    
    Result --> Audit[Audit Log]
    
    style TLS fill:#4facfe
    style EncData fill:#43e97b
    style Audit fill:#f093fb'''
            }
        ]
    },
    'chapter_37_ecosystem_integration.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '37.0',
                'title': 'Integration mit externen Systemen',
                'desc': 'Event-Driven Architecture mit CDC',
                'mermaid': '''graph LR
    ExtSys[External Systems] --> Gateway[API Gateway]
    Gateway --> Auth[Auth Service]
    
    Auth --> ThemisDB["(ThemisDB)"]
    
    ThemisDB --> CDC[Change Data Capture]
    CDC --> Kafka[Kafka Event Stream]
    
    Kafka --> Analytics[Analytics Service]
    Kafka --> Search[Search Service]
    Kafka --> Cache[Cache Service]
    
    style ThemisDB fill:#7c4dff
    style Kafka fill:#f093fb'''
            }
        ]
    },
    'chapter_38_observability_sre.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '38.0',
                'title': 'Observability-Säulen',
                'desc': 'Metrics, Logs, Traces für vollständige Observability',
                'mermaid': '''graph TB
    App[ThemisDB] --> Metrics[Metrics<br/>Prometheus]
    App --> Logs[Logs<br/>Loki]
    App --> Traces[Traces<br/>Jaeger]
    
    Metrics --> Dashboard[Grafana Dashboard]
    Logs --> Dashboard
    Traces --> Dashboard
    
    Dashboard --> Alerts{Alerts}
    Alerts -->|SLO Breach| Incident[Incident]
    Alerts -->|OK| Monitor[Continue Monitoring]
    
    Incident --> Diagnose[Diagnose]
    Diagnose --> Mitigate[Mitigate]
    Mitigate --> Postmortem[Postmortem]
    Postmortem --> Improve[Improve Systems]
    
    style Alerts fill:#ff6b6b
    style Dashboard fill:#4facfe
    style Improve fill:#43e97b'''
            }
        ]
    },
    'chapter_39_performance_tuning_cookbook.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '39.0',
                'title': 'Performance-Tuning-Workflow',
                'desc': 'Systematische Bottleneck-Identifikation und Optimierung',
                'mermaid': '''flowchart TD
    Start[Performance Issue] --> Profile[Profile System]
    
    Profile --> CPU{CPU<br/>Bottleneck?}
    Profile --> Memory{Memory<br/>Bottleneck?}
    Profile --> Disk{Disk<br/>Bottleneck?}
    Profile --> Network{Network<br/>Bottleneck?}
    
    CPU -->|Yes| OptQuery[Optimize Queries]
    CPU -->|Yes| AddIndex[Add Indexes]
    
    Memory -->|Yes| IncCache[Increase Cache]
    Memory -->|Yes| OptDataStruct[Optimize Data Structures]
    
    Disk -->|Yes| SSD[Use SSD]
    Disk -->|Yes| Partition[Partition Data]
    
    Network -->|Yes| CompData[Compress Data]
    Network -->|Yes| BatchReq[Batch Requests]
    
    OptQuery --> Verify[Verify Improvement]
    AddIndex --> Verify
    IncCache --> Verify
    OptDataStruct --> Verify
    SSD --> Verify
    Partition --> Verify
    CompData --> Verify
    BatchReq --> Verify
    
    Verify --> Done[Done]
    
    style Start fill:#ff6b6b
    style Verify fill:#f093fb
    style Done fill:#43e97b'''
            }
        ]
    },
    'chapter_40_data_governance_compliance.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '40.0',
                'title': 'Data-Governance-Framework',
                'desc': 'GDPR Compliance und Datenschutz-Layers',
                'mermaid': '''graph TB
    Data[Personal Data] --> Classification[Data Classification]
    
    Classification --> Public[Public Data]
    Classification --> Internal[Internal Data]
    Classification --> Confidential[Confidential Data]
    Classification --> Restricted[Restricted Data]
    
    Confidential --> Encrypt[Encryption Required]
    Restricted --> Encrypt
    
    Encrypt --> Access[Access Control]
    Access --> RBAC[RBAC Enforcement]
    
    RBAC --> Audit[Audit Logging]
    Audit --> Retention[Retention Policy]
    
    Retention --> Active[Active: 2 years]
    Retention --> Archive[Archive: 5 years]
    Retention --> Delete[Delete: after 7 years]
    
    style Confidential fill:#ff6b6b
    style Encrypt fill:#f093fb
    style Audit fill:#4facfe'''
            }
        ]
    },
    'chapter_41_hands_on_labs.md': {
        'insert_after': '## Überblick',
        'diagrams': [
            {
                'num': '41.0',
                'title': 'Lab-Curriculum-Flow',
                'desc': 'Struktur der Hands-on Laboratory Exercises',
                'mermaid': '''graph LR
    Setup[Lab Setup] --> Lab1[Lab 1:<br/>Installation]
    Lab1 --> Lab2[Lab 2:<br/>Basic Queries]
    Lab2 --> Lab3[Lab 3:<br/>Multi-Model]
    Lab3 --> Lab4[Lab 4:<br/>Performance]
    Lab4 --> Lab5[Lab 5:<br/>Replication]
    Lab5 --> Final[Final Project]
    
    Lab1 --> Check1{Validate}
    Lab2 --> Check2{Validate}
    Lab3 --> Check3{Validate}
    Lab4 --> Check4{Validate}
    Lab5 --> Check5{Validate}
    
    Check1 -->|Pass| Lab2
    Check2 -->|Pass| Lab3
    Check3 -->|Pass| Lab4
    Check4 -->|Pass| Lab5
    Check5 -->|Pass| Final
    
    Check1 -->|Fail| Trouble1[Troubleshoot]
    Check2 -->|Fail| Trouble2[Troubleshoot]
    
    style Final fill:#43e97b
    style Setup fill:#4facfe'''
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
                # Find the end of that section (next ## or end of file)
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
    print("  Adding Remaining Mermaid Diagrams")
    print("="*70)
    print()
    
    for chapter_file, diagram_info in CHAPTER_DIAGRAMS_COMPLETE.items():
        filepath = compendium_dir / chapter_file
        if filepath.exists():
            add_diagrams_to_chapter(filepath, diagram_info)
            print(f"[OK] Updated: {chapter_file} ({len(diagram_info['diagrams'])} diagrams)")
        else:
            print(f"[SKIP] Not found: {chapter_file}")
    
    print()
    print("="*70)
    print("Complete! Added diagrams to all 10 remaining chapters.")
    print("="*70)

if __name__ == "__main__":
    main()
