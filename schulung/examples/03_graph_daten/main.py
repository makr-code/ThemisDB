"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 18:45:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     325                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 99e8682b66  2026-03-24  Add complete schulung/ training materials folder ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Schulungsbeispiel 3: Graph-Daten
=========================================
Demonstriert:
  - Vertex- und Edge-Collections erstellen
  - Graphen definieren
  - OUTBOUND/INBOUND/ANY Traversierungen
  - Tiefensteuerung (min..max)
  - Kürzeste Pfade (SHORTEST_PATH)
  - K kürzeste Pfade (K_SHORTEST_PATHS)
  - Kanten-Filter bei Traversierung
  - Grad-Berechnung (Anzahl Verbindungen)

Voraussetzungen:
  pip install themis-client
  docker run -d -p 8080:8080 themisdb/themisdb:latest
"""

import sys
from themis_client import ThemisClient


def setup_social_network(client: ThemisClient) -> None:
    """Soziales Netzwerk aufbauen."""
    print("\n=== Setup: Soziales Netzwerk ===")

    # Collections erstellen
    for coll in ["sg_users", "sg_follows", "sg_posts", "sg_likes"]:
        client.query(f"CREATE COLLECTION IF NOT EXISTS {coll}")
        client.query(f"TRUNCATE COLLECTION {coll}")

    client.query("CREATE COLLECTION IF NOT EXISTS sg_users TYPE VERTEX")
    client.query("CREATE COLLECTION IF NOT EXISTS sg_follows TYPE EDGE FROM sg_users TO sg_users")
    client.query("CREATE COLLECTION IF NOT EXISTS sg_posts TYPE VERTEX")
    client.query("CREATE COLLECTION IF NOT EXISTS sg_likes TYPE EDGE FROM sg_users TO sg_posts")

    client.query("""
        CREATE GRAPH IF NOT EXISTS sg_social
          EDGE DEFINITION sg_follows FROM sg_users TO sg_users
          EDGE DEFINITION sg_likes   FROM sg_users TO sg_posts
    """)

    # Benutzer einfügen
    client.query("""
        FOR user IN [
          { _key: "alice",   name: "Alice",   city: "Berlin",  followers: 0 },
          { _key: "bob",     name: "Bob",     city: "Hamburg", followers: 0 },
          { _key: "clara",   name: "Clara",   city: "München", followers: 0 },
          { _key: "david",   name: "David",   city: "Berlin",  followers: 0 },
          { _key: "eva",     name: "Eva",     city: "Köln",    followers: 0 },
          { _key: "frank",   name: "Frank",   city: "Hamburg", followers: 0 }
        ]
          INSERT user INTO sg_users
    """)

    # Follow-Beziehungen (gerichtet: A folgt B)
    client.query("""
        FOR edge IN [
          { _from: "sg_users/alice", _to: "sg_users/bob",   weight: 0.9 },
          { _from: "sg_users/alice", _to: "sg_users/clara", weight: 0.7 },
          { _from: "sg_users/bob",   _to: "sg_users/david", weight: 0.8 },
          { _from: "sg_users/bob",   _to: "sg_users/eva",   weight: 0.5 },
          { _from: "sg_users/clara", _to: "sg_users/frank", weight: 0.6 },
          { _from: "sg_users/david", _to: "sg_users/eva",   weight: 0.4 },
          { _from: "sg_users/eva",   _to: "sg_users/frank", weight: 0.95 },
          { _from: "sg_users/frank", _to: "sg_users/alice", weight: 0.3 }
        ]
          INSERT edge INTO sg_follows
    """)

    # Posts und Likes
    client.query("""
        FOR post IN [
          { _key: "post1", title: "ThemisDB ist toll!", author: "alice" },
          { _key: "post2", title: "Graph-Datenbanken",  author: "bob"   },
          { _key: "post3", title: "AQL Tutorial",       author: "clara" }
        ]
          INSERT post INTO sg_posts
    """)
    client.query("""
        FOR like IN [
          { _from: "sg_users/alice", _to: "sg_posts/post2" },
          { _from: "sg_users/bob",   _to: "sg_posts/post1" },
          { _from: "sg_users/bob",   _to: "sg_posts/post3" },
          { _from: "sg_users/clara", _to: "sg_posts/post1" },
          { _from: "sg_users/david", _to: "sg_posts/post1" }
        ]
          INSERT like INTO sg_likes
    """)
    print("Soziales Netzwerk mit 6 Benutzern, 8 Follow-Beziehungen und 3 Posts erstellt.")


def demo_basic_traversal(client: ThemisClient) -> None:
    """Grundlegende Traversierungen."""
    print("\n=== Grundlegende Traversierung ===")

    # Direkte Follower von Alice
    direct = client.query("""
        FOR v IN 1..1 OUTBOUND "sg_users/alice"
          GRAPH "sg_social"
          FILTER IS_VERTEX(v)
          FILTER v.city != null   -- nur Benutzer (nicht Posts)
          RETURN v.name
    """)
    print(f"Alice folgt direkt: {direct}")

    # Transitive Verbindungen (bis Tiefe 3)
    transitive = client.query("""
        FOR v IN 1..3 OUTBOUND "sg_users/alice"
          GRAPH "sg_social"
          FILTER v.name != null AND v._key != "alice"
          RETURN DISTINCT v.name
    """)
    print(f"Alice erreicht (bis Tiefe 3): {sorted(transitive)}")

    # Wer folgt Alice? (INBOUND)
    followers = client.query("""
        FOR v IN 1..1 INBOUND "sg_users/alice"
          GRAPH "sg_social"
          FILTER v.name != null
          RETURN v.name
    """)
    print(f"Wer folgt Alice?: {followers}")


def demo_path_analysis(client: ThemisClient) -> None:
    """Pfadanalyse und kürzeste Wege."""
    print("\n=== Pfadanalyse ===")

    # Kürzester Pfad von Alice zu Frank
    path = client.query("""
        FOR path IN OUTBOUND SHORTEST_PATH
          "sg_users/alice" TO "sg_users/frank"
          GRAPH "sg_social"
          RETURN path.vertices[*].name
    """)
    if path:
        print(f"Kürzester Pfad Alice → Frank: {' → '.join(path[0])}")
    else:
        print("Kein Pfad gefunden.")

    # Alle einfachen Pfade (bis Länge 4)
    all_paths = client.query("""
        FOR p IN 1..4 OUTBOUND ALL_SHORTEST_PATHS
          "sg_users/alice" TO "sg_users/eva"
          GRAPH "sg_social"
          RETURN p.vertices[*].name
    """)
    print(f"\nAlle kürzesten Pfade Alice → Eva:")
    for p in all_paths:
        print(f"  {' → '.join(p)}")


def demo_edge_filters(client: ThemisClient) -> None:
    """Kanten-Filter bei Traversierung."""
    print("\n=== Kanten-Filter ===")

    # Nur starke Verbindungen (weight > 0.7)
    strong = client.query("""
        FOR v, e IN 1..2 OUTBOUND "sg_users/alice"
          GRAPH "sg_social"
          FILTER e.weight > 0.7
          FILTER v.name != null
          RETURN { person: v.name, strength: e.weight }
    """)
    print("Starke Verbindungen von Alice (weight > 0.7):")
    for r in strong:
        print(f"  {r['person']:10}: {r['strength']}")


def demo_degree_analysis(client: ThemisClient) -> None:
    """Grad-Analyse (In-/Out-Degree)."""
    print("\n=== Grad-Analyse ===")

    # Out-Degree: Wem folgt jeder Benutzer?
    out_degree = client.query("""
        FOR user IN sg_users
          LET following = LENGTH(
            FOR v IN 1..1 OUTBOUND user
              GRAPH "sg_social"
              FILTER v.name != null
              RETURN 1
          )
          SORT following DESC
          RETURN { name: user.name, following }
    """)
    print("Out-Degree (folgt ... Personen):")
    for r in out_degree:
        bar = "█" * r["following"]
        print(f"  {r['name']:8}: {bar} ({r['following']})")

    # In-Degree: Wer hat die meisten Follower?
    in_degree = client.query("""
        FOR user IN sg_users
          LET followers = LENGTH(
            FOR v IN 1..1 INBOUND user
              GRAPH "sg_social"
              FILTER v.name != null
              RETURN 1
          )
          SORT followers DESC
          RETURN { name: user.name, followers }
    """)
    print("\nIn-Degree (wird von ... Personen gefolgt):")
    for r in in_degree:
        bar = "█" * r["followers"]
        print(f"  {r['name']:8}: {bar} ({r['followers']})")


def demo_friend_recommendations(client: ThemisClient) -> None:
    """Empfehlungs-Algorithmus: Freunde von Freunden."""
    print("\n=== Freundes-Empfehlungen für Alice ===")

    recommendations = client.query("""
        LET already_following = (
          FOR v IN 1..1 OUTBOUND "sg_users/alice"
            GRAPH "sg_social"
            FILTER v.name != null
            RETURN v._key
        )

        FOR v IN 2..2 OUTBOUND "sg_users/alice"
          GRAPH "sg_social"
          FILTER v.name != null
          FILTER v._key != "alice"
          FILTER v._key NOT IN already_following
          COLLECT user = v._key, user_name = v.name
            WITH COUNT INTO score
          SORT score DESC
          RETURN { name: user_name, common_connections: score }
    """)
    if recommendations:
        print("Empfohlene Personen für Alice:")
        for r in recommendations:
            print(f"  {r['name']:10}: {r['common_connections']} gemeinsame Verbindung(en)")
    else:
        print("Keine Empfehlungen gefunden.")


def demo_post_reach(client: ThemisClient) -> None:
    """Reichweite eines Posts."""
    print("\n=== Post-Reichweite ===")

    # Wer hat Post1 geliked und was sind deren Verbindungen?
    result = client.query("""
        FOR liker, edge IN 1..1 INBOUND "sg_posts/post1"
          GRAPH "sg_social"
          FILTER liker.name != null
          LET reach = (
            FOR follower IN 1..1 INBOUND liker
              GRAPH "sg_social"
              FILTER follower.name != null
              RETURN follower.name
          )
          RETURN { liker: liker.name, seen_by: reach }
    """)
    print("Post 'ThemisDB ist toll!' — Likes und Reichweite:")
    total_reach = set()
    for r in result:
        print(f"  {r['liker']:8} liked → sichtbar für: {r['seen_by']}")
        total_reach.update(r["seen_by"])
    print(f"  Gesamtreichweite: {sorted(total_reach)}")


def cleanup(client: ThemisClient) -> None:
    """Aufräumen."""
    print("\n=== Cleanup ===")
    client.query("DROP GRAPH sg_social")
    for coll in ["sg_users", "sg_follows", "sg_posts", "sg_likes"]:
        client.query(f"DROP COLLECTION IF EXISTS {coll}")
    print("Graph und Collections gelöscht.")


def main() -> int:
    print("ThemisDB Schulungsbeispiel 3: Graph-Daten")
    print("=" * 45)

    client = ThemisClient("http://localhost:8080")

    try:
        health = client.health()
        print(f"Verbunden mit ThemisDB {health.get('version', 'unbekannt')}")
    except Exception as e:
        print(f"FEHLER: {e}")
        return 1

    try:
        setup_social_network(client)
        demo_basic_traversal(client)
        demo_path_analysis(client)
        demo_edge_filters(client)
        demo_degree_analysis(client)
        demo_friend_recommendations(client)
        demo_post_reach(client)
    finally:
        cleanup(client)

    print("\n✅ Beispiel 3 erfolgreich abgeschlossen!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
