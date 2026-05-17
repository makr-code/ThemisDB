#!/usr/bin/env python3
"""
ThemisDB Kickstarter Demo Data Generator
Generates comprehensive demo datasets for:
- Document Search (articles with text)
- Vector Search (embeddings)
- Graph Navigation (knowledge graph)
"""

import json
import random
from datetime import datetime, timedelta
from pathlib import Path

# Configuration
DEMO_DATA_DIR = Path(__file__).parent.parent / "data"  # Goes to demo/data, not demo/setup/data
DEMO_DATA_DIR.mkdir(exist_ok=True)

# =============================================================================
# 1. DOCUMENT DATA (Articles for Full-Text Search)
# =============================================================================

ARTICLES = [
    # AI & Machine Learning
    {
        "id": "doc_001",
        "title": "Deep Learning Architectures for Computer Vision",
        "author": "Dr. Alice Johnson",
        "category": "research",
        "published": "2024-03-15",
        "content": "This paper explores state-of-the-art deep learning architectures including ResNet, Vision Transformer, and YOLO v8. We demonstrate novel techniques for improving model efficiency and accuracy on large-scale image classification tasks.",
        "tags": ["AI", "deep-learning", "computer-vision", "neural-networks"],
        "citation_count": 342
    },
    {
        "id": "doc_002",
        "title": "Machine Learning Pipeline Optimization",
        "author": "Prof. Bob Chen",
        "category": "tutorial",
        "published": "2024-02-10",
        "content": "A comprehensive guide to building and optimizing machine learning pipelines. Covers data preprocessing, feature engineering, model selection, and hyperparameter tuning. Includes real-world examples from industry.",
        "tags": ["machine-learning", "optimization", "data-science", "pipeline"],
        "citation_count": 156
    },
    {
        "id": "doc_003",
        "title": "Transformer Models and Natural Language Processing",
        "author": "Dr. Carol Williams",
        "category": "research",
        "published": "2024-04-01",
        "content": "Explores the architecture and applications of transformer models in NLP. Includes analysis of BERT, GPT-4, and specialized models for machine translation, sentiment analysis, and question answering.",
        "tags": ["NLP", "transformers", "language-models", "AI"],
        "citation_count": 512
    },
    
    # Quantum Computing
    {
        "id": "doc_004",
        "title": "Quantum Error Correction and NISQ Algorithms",
        "author": "Dr. David Martinez",
        "category": "research",
        "published": "2024-05-12",
        "content": "Investigation of quantum error correction codes and their implementation on near-term quantum devices. Discusses surface codes, topological codes, and practical deployment challenges.",
        "tags": ["quantum-computing", "quantum-algorithms", "error-correction", "NISQ"],
        "citation_count": 287
    },
    {
        "id": "doc_005",
        "title": "Quantum Machine Learning: A Practical Perspective",
        "author": "Prof. Emma Davis",
        "category": "tutorial",
        "published": "2024-03-25",
        "content": "Practical guide to quantum machine learning algorithms including VQE, QAOA, and quantum neural networks. Includes implementation examples using Qiskit and Cirq frameworks.",
        "tags": ["quantum", "machine-learning", "quantum-ml", "algorithms"],
        "citation_count": 198
    },
    
    # Data Science & Analytics
    {
        "id": "doc_006",
        "title": "Time Series Analysis and Forecasting",
        "author": "Dr. Frank Wilson",
        "category": "research",
        "published": "2024-01-20",
        "content": "Advanced techniques for time series analysis including ARIMA, exponential smoothing, and neural network-based forecasting. Case studies from finance, weather prediction, and IoT sensors.",
        "tags": ["time-series", "forecasting", "analytics", "data-science"],
        "citation_count": 234
    },
    {
        "id": "doc_007",
        "title": "Graph Neural Networks for Knowledge Discovery",
        "author": "Dr. Grace Lee",
        "category": "research",
        "published": "2024-04-18",
        "content": "Comprehensive review of graph neural networks (GNNs) including GCN, GraphSAGE, and Graph Attention Networks. Applications in recommendation systems, knowledge graphs, and molecular modeling.",
        "tags": ["graph-neural-networks", "GNN", "knowledge-graphs", "deep-learning"],
        "citation_count": 423
    },
    
    # Data Engineering
    {
        "id": "doc_008",
        "title": "Distributed Data Processing at Scale",
        "author": "Prof. Henry Kim",
        "category": "tutorial",
        "published": "2024-02-28",
        "content": "Building and maintaining distributed data processing systems. Covers Apache Spark, Kafka, and cloud-based solutions. Real-world architecture patterns and failure handling strategies.",
        "tags": ["data-engineering", "distributed-systems", "big-data", "spark"],
        "citation_count": 189
    },
    {
        "id": "doc_009",
        "title": "Vector Databases for Semantic Search",
        "author": "Dr. Isabel Rodriguez",
        "category": "research",
        "published": "2024-05-01",
        "content": "Novel approaches to vector database design for efficient semantic search at scale. Discusses indexing strategies, distance metrics, and optimization techniques for billion-scale embeddings.",
        "tags": ["vector-databases", "semantic-search", "embeddings", "information-retrieval"],
        "citation_count": 267
    },
    
    # Database Systems
    {
        "id": "doc_010",
        "title": "Multi-Model Database Architecture",
        "author": "Dr. Jack Anderson",
        "category": "research",
        "published": "2024-03-08",
        "content": "Design and implementation of multi-model databases supporting relational, document, graph, and time-series data in a unified system. Query optimization and data consistency challenges.",
        "tags": ["databases", "multi-model", "system-design", "architecture"],
        "citation_count": 156
    },
    {
        "id": "doc_011",
        "title": "Transaction Processing in Distributed Databases",
        "author": "Prof. Karen Thompson",
        "category": "research",
        "published": "2024-04-22",
        "content": "Deep dive into transaction processing, ACID properties, and consistency models. Comparison of pessimistic and optimistic concurrency control in distributed settings.",
        "tags": ["transactions", "ACID", "databases", "concurrency"],
        "citation_count": 312
    },
    
    # AI Security & Ethics
    {
        "id": "doc_012",
        "title": "Adversarial Robustness in Deep Learning",
        "author": "Dr. Lisa Chang",
        "category": "research",
        "published": "2024-05-10",
        "content": "Analysis of adversarial attacks on neural networks and defense mechanisms. Discusses robustness certification, adversarial training, and evaluation frameworks.",
        "tags": ["AI-security", "adversarial-examples", "robustness", "deep-learning"],
        "citation_count": 378
    },
    {
        "id": "doc_013",
        "title": "Ethical Considerations in AI Systems",
        "author": "Prof. Michael Brown",
        "category": "tutorial",
        "published": "2024-02-14",
        "content": "Comprehensive overview of ethical challenges in AI including bias, fairness, transparency, and accountability. Guidelines for responsible AI development and deployment.",
        "tags": ["AI-ethics", "fairness", "transparency", "responsibility"],
        "citation_count": 289
    },
]

# =============================================================================
# 2. VECTOR DATA (Embeddings for Semantic Search)
# =============================================================================

def generate_mock_embedding(seed):
    """Generate a mock embedding vector"""
    random.seed(seed)
    return [random.uniform(-1, 1) for _ in range(128)]

VECTORS = [
    {
        "id": f"vec_{i:03d}",
        "doc_id": article["id"],
        "title": article["title"],
        "author": article["author"],
        "embedding": generate_mock_embedding(i),
        "score": random.uniform(0.7, 1.0),
        "relevance_tags": article["tags"]
    }
    for i, article in enumerate(ARTICLES)
]

# =============================================================================
# 3. GRAPH DATA (Knowledge Graph with Nodes and Edges)
# =============================================================================

RESEARCHERS = [
    {"id": "res_001", "name": "Dr. Alice Johnson", "affiliation": "MIT", "field": "Deep Learning"},
    {"id": "res_002", "name": "Prof. Bob Chen", "affiliation": "Stanford", "field": "ML Engineering"},
    {"id": "res_003", "name": "Dr. Carol Williams", "affiliation": "Berkeley", "field": "NLP"},
    {"id": "res_004", "name": "Dr. David Martinez", "affiliation": "Oxford", "field": "Quantum Computing"},
    {"id": "res_005", "name": "Prof. Emma Davis", "affiliation": "Cambridge", "field": "Quantum ML"},
    {"id": "res_006", "name": "Dr. Frank Wilson", "affiliation": "CMU", "field": "Time Series"},
    {"id": "res_007", "name": "Dr. Grace Lee", "affiliation": "Toronto", "field": "Graph Learning"},
    {"id": "res_008", "name": "Prof. Henry Kim", "affiliation": "Seoul National", "field": "Distributed Systems"},
    {"id": "res_009", "name": "Dr. Isabel Rodriguez", "affiliation": "ETH Zurich", "field": "Vector Databases"},
    {"id": "res_010", "name": "Dr. Jack Anderson", "affiliation": "Bell Labs", "field": "Database Systems"},
]

PAPERS = [
    {"id": "paper_001", "title": "Deep Learning Architectures for Computer Vision", "year": 2024},
    {"id": "paper_002", "title": "Machine Learning Pipeline Optimization", "year": 2024},
    {"id": "paper_003", "title": "Transformer Models and Natural Language Processing", "year": 2024},
    {"id": "paper_004", "title": "Quantum Error Correction and NISQ Algorithms", "year": 2024},
    {"id": "paper_005", "title": "Quantum Machine Learning: A Practical Perspective", "year": 2024},
    {"id": "paper_006", "title": "Vector Databases for Semantic Search", "year": 2024},
    {"id": "paper_007", "title": "Multi-Model Database Architecture", "year": 2024},
]

CONFERENCES = [
    {"id": "conf_001", "name": "NeurIPS 2024", "location": "Vancouver"},
    {"id": "conf_002", "name": "ICML 2024", "location": "Vienna"},
    {"id": "conf_003", "name": "ICCV 2024", "location": "Milan"},
    {"id": "conf_004", "name": "VLDB 2024", "location": "Singapore"},
]

GRAPH_NODES = []
GRAPH_EDGES = []

# Add researcher nodes
for researcher in RESEARCHERS:
    GRAPH_NODES.append({
        "id": f"node_{researcher['id']}",
        "_key": researcher["id"],
        "type": "researcher",
        "name": researcher["name"],
        "affiliation": researcher["affiliation"],
        "field": researcher["field"]
    })

# Add paper nodes
for paper in PAPERS:
    GRAPH_NODES.append({
        "id": f"node_{paper['id']}",
        "_key": paper["id"],
        "type": "paper",
        "title": paper["title"],
        "year": paper["year"]
    })

# Add conference nodes
for conference in CONFERENCES:
    GRAPH_NODES.append({
        "id": f"node_{conference['id']}",
        "_key": conference["id"],
        "type": "conference",
        "name": conference["name"],
        "location": conference["location"]
    })

# Create edges (relationships)
EDGE_DEFINITIONS = [
    # Researcher -> Paper (wrote)
    ("res_001", "paper_001", "wrote"),
    ("res_002", "paper_002", "wrote"),
    ("res_003", "paper_003", "wrote"),
    ("res_004", "paper_004", "wrote"),
    ("res_005", "paper_005", "wrote"),
    ("res_009", "paper_006", "wrote"),
    ("res_010", "paper_007", "wrote"),
    
    # Paper -> Paper (cites)
    ("paper_003", "paper_001", "cites"),
    ("paper_005", "paper_004", "cites"),
    ("paper_006", "paper_001", "cites"),
    ("paper_007", "paper_001", "cites"),
    
    # Researcher -> Researcher (collaborates)
    ("res_001", "res_003", "collaborates_with"),
    ("res_004", "res_005", "collaborates_with"),
    ("res_007", "res_009", "collaborates_with"),
    
    # Paper -> Conference (presented_at)
    ("paper_001", "conf_003", "presented_at"),
    ("paper_003", "conf_002", "presented_at"),
    ("paper_004", "conf_001", "presented_at"),
    ("paper_006", "conf_004", "presented_at"),
]

for from_id, to_id, relationship in EDGE_DEFINITIONS:
    GRAPH_EDGES.append({
        "id": f"edge_{from_id}_{to_id}_{relationship}",
        "_from": f"node_{from_id}",
        "_to": f"node_{to_id}",
        "type": relationship,
        "weight": random.uniform(0.5, 1.0)
    })

# =============================================================================
# WRITING FUNCTIONS
# =============================================================================

def write_articles_jsonl():
    """Write demo_articles.jsonl"""
    output_file = DEMO_DATA_DIR / "demo_articles.jsonl"
    with open(output_file, "w") as f:
        for article in ARTICLES:
            f.write(json.dumps(article) + "\n")
    print("[OK] Created {} with {} articles".format(output_file, len(ARTICLES)))

def write_embeddings_jsonl():
    """Write demo_embeddings.jsonl"""
    output_file = DEMO_DATA_DIR / "demo_embeddings.jsonl"
    with open(output_file, "w") as f:
        for vector in VECTORS:
            f.write(json.dumps(vector) + "\n")
    print("[OK] Created {} with {} embeddings".format(output_file, len(VECTORS)))

def write_graph_jsonl():
    """Write demo_knowledge_graph_nodes.jsonl and demo_knowledge_graph_edges.jsonl"""
    nodes_file = DEMO_DATA_DIR / "demo_knowledge_graph_nodes.jsonl"
    edges_file = DEMO_DATA_DIR / "demo_knowledge_graph_edges.jsonl"
    
    with open(nodes_file, "w") as f:
        for node in GRAPH_NODES:
            f.write(json.dumps(node) + "\n")
    print("[OK] Created {} with {} nodes".format(nodes_file, len(GRAPH_NODES)))
    
    with open(edges_file, "w") as f:
        for edge in GRAPH_EDGES:
            f.write(json.dumps(edge) + "\n")
    print("[OK] Created {} with {} edges".format(edges_file, len(GRAPH_EDGES)))

def write_summary():
    """Write summary of generated data"""
    summary_file = DEMO_DATA_DIR / "DATA_SUMMARY.md"
    with open(summary_file, "w") as f:
        f.write("# ThemisDB Demo Data Summary\n\n")
        
        f.write("## Document Collection (demo_articles)\n")
        f.write(f"- **Total Articles:** {len(ARTICLES)}\n")
        f.write(f"- **Topics:** AI, Machine Learning, Quantum Computing, Data Science, Databases\n")
        f.write(f"- **Fields:** title, author, category, published, content, tags, citation_count\n\n")
        
        f.write("## Vector Collection (demo_embeddings)\n")
        f.write(f"- **Total Embeddings:** {len(VECTORS)}\n")
        f.write(f"- **Dimensions:** 128\n")
        f.write(f"- **Fields:** id, doc_id, title, author, embedding, score, relevance_tags\n\n")
        
        f.write("## Graph Collection (demo_knowledge_graph)\n")
        f.write(f"- **Total Nodes:** {len(GRAPH_NODES)}\n")
        f.write(f"- **Node Types:** researcher ({len([n for n in GRAPH_NODES if n['type'] == 'researcher'])}), paper ({len([n for n in GRAPH_NODES if n['type'] == 'paper'])}), conference ({len([n for n in GRAPH_NODES if n['type'] == 'conference'])})\n")
        f.write(f"- **Total Edges:** {len(GRAPH_EDGES)}\n")
        f.write(f"- **Relationship Types:** wrote, cites, collaborates_with, presented_at\n\n")
        
        f.write("## Import Instructions\n\n")
        f.write("```powershell\n")
        f.write("# Import document collection\n")
        f.write("themisctl batch-insert --collection demo_articles < demo_articles.jsonl\n\n")
        f.write("# Import vector collection\n")
        f.write("themisctl batch-insert --collection demo_embeddings < demo_embeddings.jsonl\n\n")
        f.write("# Import graph collection\n")
        f.write("themisctl batch-insert --collection demo_knowledge_graph < demo_knowledge_graph_nodes.jsonl\n")
        f.write("themisctl batch-insert --collection demo_knowledge_graph --edges < demo_knowledge_graph_edges.jsonl\n")
        f.write("```\n\n")
        
        f.write("## Sample Queries\n\n")
        f.write("### Document Search\n")
        f.write("```aql\n")
        f.write("FOR doc IN demo_articles\n")
        f.write("  FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%'\n")
        f.write("  SORT doc.published DESC\n")
        f.write("  LIMIT 5\n")
        f.write("  RETURN doc\n")
        f.write("```\n\n")
        
        f.write("### Vector Search\n")
        f.write("```aql\n")
        f.write("FOR vec IN demo_embeddings\n")
        f.write("  LET similarity = COSINE_SIMILARITY(vec.embedding, @query_embedding)\n")
        f.write("  FILTER similarity > 0.7\n")
        f.write("  SORT similarity DESC\n")
        f.write("  LIMIT 5\n")
        f.write("  RETURN { title: vec.title, similarity: similarity }\n")
        f.write("```\n\n")
        
        f.write("### Graph Traversal\n")
        f.write("```aql\n")
        f.write("FOR researcher IN demo_knowledge_graph\n")
        f.write("  FILTER researcher.type == 'researcher'\n")
        f.write("  FOR paper IN 1..2 OUTBOUND researcher._id graph_edges\n")
        f.write("    FILTER paper.type == 'paper'\n")
        f.write("  RETURN { researcher: researcher.name, paper: paper.title }\n")
        f.write("```\n")
    
    print("[OK] Created {}".format(summary_file))

# =============================================================================
# MAIN
# =============================================================================

def main():
    print("\n" + "="*60)
    print("ThemisDB Demo Data Generator")
    print("="*60 + "\n")
    
    write_articles_jsonl()
    write_embeddings_jsonl()
    write_graph_jsonl()
    write_summary()
    
    print("\n" + "="*60)
    print("Demo data generation complete!")
    print("="*60)
    print("\nGenerated files in: {}\n".format(DEMO_DATA_DIR.absolute()))
    print("Next steps:")
    print("1. Start ThemisDB server")
    print("2. Run themisctl to import collections")
    print("3. Execute demo queries from DEMO_QUERIES.md")
    print("\n")

if __name__ == "__main__":
    main()
