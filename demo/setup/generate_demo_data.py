#!/usr/bin/env python3
"""
ThemisDB Kickstarter Demo Data Generator
Generates realistic public-administration demo datasets for:
- Document Search (administrative law case documents)
- Vector Search (embeddings per case)
- Graph Navigation (authorities, cases, applicants, courts)
"""

import json
import random
from datetime import date, timedelta
from pathlib import Path

# Configuration
DEMO_DATA_DIR = Path(__file__).parent.parent / "data"
DEMO_DATA_DIR.mkdir(exist_ok=True)
RNG = random.Random(20260530)

DOC_COUNT = 108
EMBEDDING_DIM = 128

AUTHORITIES = [
    ("auth_001", "Landratsamt Muenchen", "Oberbayern"),
    ("auth_002", "Stadt Koeln Ordnungsamt", "Koeln"),
    ("auth_003", "Bezirksamt Hamburg-Mitte", "Hamburg"),
    ("auth_004", "Regierungspraesidium Stuttgart", "Stuttgart"),
    ("auth_005", "Kreisverwaltung Mainz-Bingen", "Rheinland-Pfalz"),
    ("auth_006", "Stadt Leipzig Bauordnungsamt", "Leipzig"),
    ("auth_007", "Landratsamt Karlsruhe", "Karlsruhe"),
    ("auth_008", "Stadt Dortmund Sozialamt", "Dortmund"),
    ("auth_009", "Bezirksregierung Duesseldorf", "Duesseldorf"),
    ("auth_010", "Landesamt fuer Umwelt Brandenburg", "Brandenburg"),
    ("auth_011", "Stadt Frankfurt Auslaenderbehoerde", "Frankfurt"),
    ("auth_012", "Landratsamt Nuernberger Land", "Mittelfranken"),
]

PROCEDURE_TYPES = [
    "Baugenehmigung",
    "Gewerbeuntersagung",
    "Immissionsschutzauflage",
    "Denkmalschutzanordnung",
    "Aufenthaltstitel",
    "Foerderbescheid",
    "Wasserrechtliche Erlaubnis",
    "Vergaberechtliche Entscheidung",
    "Datenschutzanordnung",
]

LEGAL_BASES = [
    "VwVfG",
    "VwGO",
    "BauGB",
    "BImSchG",
    "AufenthG",
    "DSGVO",
    "UVgO",
    "WHG",
    "DenkmSchG",
]

CASE_STATUS = ["erlassen", "angefochten", "teilweise aufgehoben", "bestaetigt", "in Vollzug"]
COURTS = [
    ("court_001", "VG Berlin"),
    ("court_002", "VG Muenchen"),
    ("court_003", "VG Koeln"),
    ("court_004", "OVG NRW"),
    ("court_005", "VGH Baden-Wuerttemberg"),
]


def make_embedding(seed: int) -> list[float]:
    local = random.Random(seed)
    return [round(local.uniform(-1.0, 1.0), 6) for _ in range(EMBEDDING_DIM)]


def build_articles() -> list[dict]:
    base_day = date(2023, 1, 2)
    articles: list[dict] = []

    for idx in range(1, DOC_COUNT + 1):
        authority_id, authority_name, region = AUTHORITIES[(idx - 1) % len(AUTHORITIES)]
        procedure = PROCEDURE_TYPES[(idx - 1) % len(PROCEDURE_TYPES)]
        legal_basis = LEGAL_BASES[(idx - 1) % len(LEGAL_BASES)]
        status = CASE_STATUS[(idx - 1) % len(CASE_STATUS)]

        case_id = f"case_{idx:04d}"
        applicant_id = f"app_{((idx - 1) % 40) + 1:03d}"
        published = (base_day + timedelta(days=3 * idx)).isoformat()

        related = []
        if idx > 3:
            related.append(f"case_{idx - 1:04d}")
            if idx % 5 == 0:
                related.append(f"case_{idx - 3:04d}")

        content = (
            f"Bescheid zum Verfahren {procedure} im Verwaltungsfall {case_id}. "
            f"Zustaendige Behoerde: {authority_name}. "
            f"Rechtsgrundlage: {legal_basis}. "
            f"Aktueller Verfahrensstand: {status}. "
            f"Pruefung der Tatbestandsmerkmale, Ermessensausuebung und Verhaeltnismaessigkeit "
            f"wurden dokumentiert. Region: {region}."
        )

        articles.append(
            {
                "id": f"doc_{idx:03d}",
                "case_id": case_id,
                "applicant_id": applicant_id,
                "authority_id": authority_id,
                "authority_name": authority_name,
                "region": region,
                "title": f"Verwaltungsrechtlicher Bescheid {case_id} ({procedure})",
                "author": authority_name,
                "category": "verwaltungsrecht",
                "procedure_type": procedure,
                "legal_basis": legal_basis,
                "status": status,
                "published": published,
                "district_code": f"D-{(idx % 16) + 1:02d}",
                "related_case_ids": related,
                "content": content,
                "tags": [
                    "verwaltungsrecht",
                    procedure.lower().replace(" ", "-"),
                    legal_basis.lower(),
                    status.replace(" ", "-"),
                ],
                "citation_count": 20 + (idx % 80),
            }
        )

    return articles


def build_vectors(articles: list[dict]) -> list[dict]:
    vectors: list[dict] = []
    for idx, article in enumerate(articles, start=1):
        vectors.append(
            {
                "id": f"vec_{idx:03d}",
                "doc_id": article["id"],
                "case_id": article["case_id"],
                "authority_id": article["authority_id"],
                "title": article["title"],
                "procedure_type": article["procedure_type"],
                "embedding": make_embedding(idx * 7919),
                "score": round(0.72 + ((idx % 25) / 100.0), 4),
                "relevance_tags": article["tags"],
            }
        )
    return vectors


def build_graph(articles: list[dict]) -> tuple[list[dict], list[dict]]:
    nodes: list[dict] = []
    edges: list[dict] = []

    # Authority nodes
    for auth_id, auth_name, region in AUTHORITIES:
        nodes.append(
            {
                "id": f"node_{auth_id}",
                "_key": auth_id,
                "type": "authority",
                "name": auth_name,
                "region": region,
            }
        )

    # Applicant nodes
    for i in range(1, 41):
        app_id = f"app_{i:03d}"
        nodes.append(
            {
                "id": f"node_{app_id}",
                "_key": app_id,
                "type": "applicant",
                "name": f"Antragsteller {i:03d}",
                "sector": ["Bau", "Umwelt", "Gewerbe", "Soziales"][i % 4],
            }
        )

    # Court nodes
    for court_id, court_name in COURTS:
        nodes.append(
            {
                "id": f"node_{court_id}",
                "_key": court_id,
                "type": "court",
                "name": court_name,
            }
        )

    # Case nodes + relationships
    for idx, article in enumerate(articles, start=1):
        case_id = article["case_id"]
        app_id = article["applicant_id"]
        auth_id = article["authority_id"]

        nodes.append(
            {
                "id": f"node_{case_id}",
                "_key": case_id,
                "type": "case",
                "title": article["title"],
                "status": article["status"],
                "legal_basis": article["legal_basis"],
                "published": article["published"],
            }
        )

        edges.append(
            {
                "id": f"edge_{auth_id}_{case_id}_issued",
                "_from": f"node_{auth_id}",
                "_to": f"node_{case_id}",
                "type": "issued",
                "weight": round(0.8 + ((idx % 10) / 50.0), 3),
            }
        )

        edges.append(
            {
                "id": f"edge_{case_id}_{app_id}_concerns",
                "_from": f"node_{case_id}",
                "_to": f"node_{app_id}",
                "type": "concerns",
                "weight": 1.0,
            }
        )

        if idx % 3 == 0:
            court_id, _ = COURTS[idx % len(COURTS)]
            edges.append(
                {
                    "id": f"edge_{case_id}_{court_id}_appealed_at",
                    "_from": f"node_{case_id}",
                    "_to": f"node_{court_id}",
                    "type": "appealed_at",
                    "weight": 0.9,
                }
            )

        for related_case_id in article["related_case_ids"]:
            edges.append(
                {
                    "id": f"edge_{case_id}_{related_case_id}_references",
                    "_from": f"node_{case_id}",
                    "_to": f"node_{related_case_id}",
                    "type": "references",
                    "weight": 0.7,
                }
            )

    return nodes, edges


def write_jsonl(path: Path, rows: list[dict]) -> None:
    with path.open("w", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, ensure_ascii=False) + "\n")


def write_summary(articles: list[dict], vectors: list[dict], graph_nodes: list[dict], graph_edges: list[dict]) -> None:
    summary_file = DEMO_DATA_DIR / "DATA_SUMMARY.md"
    node_type_counts: dict[str, int] = {}
    for n in graph_nodes:
        node_type_counts[n["type"]] = node_type_counts.get(n["type"], 0) + 1

    with summary_file.open("w", encoding="utf-8") as f:
        f.write("# ThemisDB Demo Data Summary\n\n")
        f.write("## Verwaltungsrechtliche Demo-Daten\n")
        f.write(f"- **Dokumente (demo_articles):** {len(articles)}\n")
        f.write(f"- **Vektoren (demo_embeddings):** {len(vectors)}\n")
        f.write(f"- **Graph-Knoten (demo_knowledge_graph):** {len(graph_nodes)}\n")
        f.write(f"- **Graph-Kanten (demo_knowledge_graph):** {len(graph_edges)}\n\n")

        f.write("## Relationale Felder im Dokumentmodell\n")
        f.write("- case_id, applicant_id, authority_id\n")
        f.write("- procedure_type, legal_basis, status, region\n")
        f.write("- related_case_ids als Fall-Referenzen\n\n")

        f.write("## Graph-Node-Typen\n")
        for node_type in sorted(node_type_counts):
            f.write(f"- {node_type}: {node_type_counts[node_type]}\n")

        f.write("\n## Hinweis\n")
        f.write("Die Ingestion erfolgt im Demo-Setup weiterhin ueber Key/Blob-Payloads.\n")


def main() -> None:
    print("\n" + "=" * 60)
    print("ThemisDB Demo Data Generator")
    print("=" * 60 + "\n")

    articles = build_articles()
    vectors = build_vectors(articles)
    graph_nodes, graph_edges = build_graph(articles)

    articles_file = DEMO_DATA_DIR / "demo_articles.jsonl"
    vectors_file = DEMO_DATA_DIR / "demo_embeddings.jsonl"
    graph_nodes_file = DEMO_DATA_DIR / "demo_knowledge_graph_nodes.jsonl"
    graph_edges_file = DEMO_DATA_DIR / "demo_knowledge_graph_edges.jsonl"

    write_jsonl(articles_file, articles)
    print(f"[OK] Created {articles_file} with {len(articles)} articles")

    write_jsonl(vectors_file, vectors)
    print(f"[OK] Created {vectors_file} with {len(vectors)} embeddings")

    write_jsonl(graph_nodes_file, graph_nodes)
    print(f"[OK] Created {graph_nodes_file} with {len(graph_nodes)} nodes")

    write_jsonl(graph_edges_file, graph_edges)
    print(f"[OK] Created {graph_edges_file} with {len(graph_edges)} edges")

    write_summary(articles, vectors, graph_nodes, graph_edges)
    print(f"[OK] Created {DEMO_DATA_DIR / 'DATA_SUMMARY.md'}")

    print("\n" + "=" * 60)
    print("Demo data generation complete!")
    print("=" * 60)
    print(f"\nGenerated files in: {DEMO_DATA_DIR.absolute()}\n")


if __name__ == "__main__":
    main()
