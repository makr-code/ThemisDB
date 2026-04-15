"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 05:39:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     376                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 99e8682b66  2026-03-24  Add complete schulung/ training materials folder ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Schulungsbeispiel 4: Multi-Model-Anwendung
====================================================
Demonstriert:
  - Kombination von Relational, Graph, Vektor und Zeitreihen in einer Anwendung
  - Use Case: Lernplattform (Kurse, Teilnehmer, Fortschritt, Empfehlungen)

  Datenmodell:
    courses      (Dokument)    — Kurse mit Beschreibungen
    students     (Vertex)      — Teilnehmer
    instructors  (Vertex)      — Dozenten
    enrollments  (Edge)        — students → courses (Relational + Graph)
    taught_by    (Edge)        — courses → instructors
    progress     (Zeitreihen)  — Lernfortschritt-Ereignisse
    embeddings   (Vektor)      — Kurs-Embeddings für Empfehlungen

Voraussetzungen:
  pip install themis-client
  docker run -d -p 8080:8080 themisdb/themisdb:latest
"""

import sys
from datetime import datetime, timedelta
from themis_client import ThemisClient


# Kurs-Embeddings (vorberechnet, 8-dimensional für Beispiel)
# In Produktion: LLM EMBED mit sentence-transformers
COURSE_EMBEDDINGS = {
    "c_python":   [0.9, 0.1, 0.8, 0.0, 0.1, 0.0, 0.2, 0.1],
    "c_sql":      [0.2, 0.9, 0.1, 0.8, 0.0, 0.1, 0.1, 0.0],
    "c_aql":      [0.1, 0.8, 0.2, 0.9, 0.0, 0.1, 0.1, 0.0],
    "c_ml":       [0.8, 0.2, 0.6, 0.1, 0.9, 0.8, 0.3, 0.2],
    "c_graph_db": [0.1, 0.7, 0.1, 0.9, 0.1, 0.0, 0.8, 0.1],
    "c_nlp":      [0.7, 0.1, 0.5, 0.0, 0.8, 0.9, 0.2, 0.3],
}


def setup_data(client: ThemisClient) -> None:
    """Lernplattform-Daten aufbauen."""
    print("\n=== Setup: Lernplattform ===")

    # Collections erstellen
    vertex_collections = ["mm_students", "mm_instructors", "mm_courses"]
    edge_collections   = ["mm_enrollments", "mm_taught_by"]
    all_collections    = vertex_collections + edge_collections + ["mm_progress"]

    for coll in all_collections:
        client.query(f"CREATE COLLECTION IF NOT EXISTS {coll}")
        client.query(f"TRUNCATE COLLECTION {coll}")

    for coll in vertex_collections:
        client.query(f"CREATE COLLECTION IF NOT EXISTS {coll} TYPE VERTEX")
    for coll in edge_collections:
        src = "mm_students" if coll == "mm_enrollments" else "mm_courses"
        tgt = "mm_courses"  if coll == "mm_enrollments" else "mm_instructors"
        client.query(f"CREATE COLLECTION IF NOT EXISTS {coll} TYPE EDGE FROM {src} TO {tgt}")

    # Graph definieren
    client.query("""
        CREATE GRAPH IF NOT EXISTS mm_learning_graph
          EDGE DEFINITION mm_enrollments FROM mm_students    TO mm_courses
          EDGE DEFINITION mm_taught_by   FROM mm_courses     TO mm_instructors
    """)

    # Dozenten
    client.query("""
        FOR i IN [
          { _key: "inst_alice",  name: "Prof. Alice Müller", expertise: ["Python", "ML", "NLP"]   },
          { _key: "inst_bob",    name: "Dr. Bob Schmidt",    expertise: ["SQL", "AQL", "Databases"] }
        ]
          INSERT i INTO mm_instructors
    """)

    # Kurse (mit vorberechneten Embeddings)
    courses = [
        {"_key": "c_python",   "title": "Python Grundlagen",       "level": "beginner",      "duration_h": 20, "category": "Programmierung", "embedding": COURSE_EMBEDDINGS["c_python"]},
        {"_key": "c_sql",      "title": "SQL für Einsteiger",       "level": "beginner",      "duration_h": 15, "category": "Datenbanken",    "embedding": COURSE_EMBEDDINGS["c_sql"]},
        {"_key": "c_aql",      "title": "AQL Mastery",              "level": "intermediate",  "duration_h": 25, "category": "Datenbanken",    "embedding": COURSE_EMBEDDINGS["c_aql"]},
        {"_key": "c_ml",       "title": "Machine Learning",         "level": "advanced",      "duration_h": 40, "category": "KI",             "embedding": COURSE_EMBEDDINGS["c_ml"]},
        {"_key": "c_graph_db", "title": "Graph-Datenbanken",        "level": "intermediate",  "duration_h": 20, "category": "Datenbanken",    "embedding": COURSE_EMBEDDINGS["c_graph_db"]},
        {"_key": "c_nlp",      "title": "Natural Language Processing", "level": "advanced",   "duration_h": 35, "category": "KI",             "embedding": COURSE_EMBEDDINGS["c_nlp"]},
    ]
    for course in courses:
        client.query("INSERT @course INTO mm_courses", bind_vars={"course": course})

    # Dozenten-Zuweisung
    client.query("""
        FOR edge IN [
          { _from: "mm_courses/c_python",   _to: "mm_instructors/inst_alice" },
          { _from: "mm_courses/c_ml",       _to: "mm_instructors/inst_alice" },
          { _from: "mm_courses/c_nlp",      _to: "mm_instructors/inst_alice" },
          { _from: "mm_courses/c_sql",      _to: "mm_instructors/inst_bob"   },
          { _from: "mm_courses/c_aql",      _to: "mm_instructors/inst_bob"   },
          { _from: "mm_courses/c_graph_db", _to: "mm_instructors/inst_bob"   }
        ]
          INSERT edge INTO mm_taught_by
    """)

    # Studierende
    client.query("""
        FOR s IN [
          { _key: "s_anna",  name: "Anna",   interests: ["Python", "ML", "AI"]           },
          { _key: "s_ben",   name: "Ben",    interests: ["Databases", "SQL", "AQL"]       },
          { _key: "s_carla", name: "Carla",  interests: ["Python", "Databases", "Graph"]  },
          { _key: "s_dario", name: "Dario",  interests: ["ML", "NLP", "AI"]              }
        ]
          INSERT s INTO mm_students
    """)

    # Einschreibungen
    client.query("""
        FOR e IN [
          { _from: "mm_students/s_anna",  _to: "mm_courses/c_python",   enrolled: "2025-01-01", completed: true,  score: 95 },
          { _from: "mm_students/s_anna",  _to: "mm_courses/c_ml",       enrolled: "2025-02-01", completed: false, score: 0  },
          { _from: "mm_students/s_ben",   _to: "mm_courses/c_sql",      enrolled: "2025-01-15", completed: true,  score: 88 },
          { _from: "mm_students/s_ben",   _to: "mm_courses/c_aql",      enrolled: "2025-02-15", completed: false, score: 0  },
          { _from: "mm_students/s_carla", _to: "mm_courses/c_python",   enrolled: "2025-01-10", completed: true,  score: 78 },
          { _from: "mm_students/s_carla", _to: "mm_courses/c_graph_db", enrolled: "2025-03-01", completed: false, score: 0  },
          { _from: "mm_students/s_dario", _to: "mm_courses/c_ml",       enrolled: "2025-01-20", completed: true,  score: 92 },
          { _from: "mm_students/s_dario", _to: "mm_courses/c_nlp",      enrolled: "2025-02-20", completed: false, score: 0  }
        ]
          INSERT e INTO mm_enrollments
    """)

    # Lernfortschritt (Zeitreihen-ähnlich)
    base = datetime.utcnow() - timedelta(days=30)
    progress_events = [
        {"student": "s_anna",  "course": "c_python",   "event": "lesson_complete", "lesson": 1,  "ts": (base + timedelta(days=0)).isoformat()  + "Z"},
        {"student": "s_anna",  "course": "c_python",   "event": "lesson_complete", "lesson": 5,  "ts": (base + timedelta(days=5)).isoformat()  + "Z"},
        {"student": "s_anna",  "course": "c_python",   "event": "quiz_passed",     "lesson": 10, "ts": (base + timedelta(days=10)).isoformat() + "Z"},
        {"student": "s_anna",  "course": "c_ml",       "event": "lesson_complete", "lesson": 1,  "ts": (base + timedelta(days=20)).isoformat() + "Z"},
        {"student": "s_dario", "course": "c_ml",       "event": "lesson_complete", "lesson": 1,  "ts": (base + timedelta(days=2)).isoformat()  + "Z"},
        {"student": "s_dario", "course": "c_ml",       "event": "exam_passed",     "lesson": 15, "ts": (base + timedelta(days=25)).isoformat() + "Z"},
        {"student": "s_ben",   "course": "c_sql",      "event": "lesson_complete", "lesson": 1,  "ts": (base + timedelta(days=1)).isoformat()  + "Z"},
        {"student": "s_ben",   "course": "c_sql",      "event": "exam_passed",     "lesson": 12, "ts": (base + timedelta(days=20)).isoformat() + "Z"},
    ]
    for event in progress_events:
        client.query("INSERT @event INTO mm_progress", bind_vars={"event": event})

    print("Lernplattform bereit: 4 Studenten, 6 Kurse, 2 Dozenten")


def demo_course_overview(client: ThemisClient) -> None:
    """Kursübersicht mit Teilnehmerzahlen."""
    print("\n=== Kursübersicht ===")

    courses = client.query("""
        FOR course IN mm_courses
          LET enrollments = LENGTH(
            FOR e IN 1..1 INBOUND course mm_enrollments
              RETURN 1
          )
          LET completed = LENGTH(
            FOR v, e IN 1..1 INBOUND course mm_enrollments
              FILTER e.completed == true
              RETURN 1
          )
          LET instructor = FIRST(
            FOR v IN 1..1 OUTBOUND course mm_taught_by
              RETURN v.name
          )
          SORT course.category, course.level
          RETURN {
            title:       course.title,
            level:       course.level,
            duration_h:  course.duration_h,
            instructor:  instructor,
            enrolled:    enrollments,
            completed:   completed
          }
    """)
    print(f"{'Kurs':35} {'Level':14} {'Std':4} {'Anm':4} {'✅':3} {'Dozent'}")
    print("-" * 85)
    for c in courses:
        print(f"  {c['title']:33} {c['level']:14} {c['duration_h']:3}h {c['enrolled']:3}  {c['completed']:2}  {c['instructor']}")


def demo_student_dashboard(client: ThemisClient) -> None:
    """Studenten-Dashboard: Fortschritt und Empfehlungen."""
    print("\n=== Studenten-Dashboard: Anna ===")

    dashboard = client.query("""
        LET student = DOCUMENT("mm_students", "s_anna")

        LET enrolled_courses = (
          FOR course, edge IN 1..1 OUTBOUND "mm_students/s_anna"
            GRAPH "mm_learning_graph"
            FILTER course.title != null
            RETURN {
              title:     course.title,
              completed: edge.completed,
              score:     edge.score
            }
        )

        LET completed_count = LENGTH(
          FOR c IN enrolled_courses FILTER c.completed == true RETURN 1
        )

        RETURN {
          name:        student.name,
          interests:   student.interests,
          enrolled:    enrolled_courses,
          completed:   completed_count
        }
    """)
    d = dashboard[0]
    print(f"Studentin: {d['name']}")
    print(f"Interessen: {', '.join(d['interests'])}")
    print(f"Kurse ({d['completed']}/{len(d['enrolled'])} abgeschlossen):")
    for c in d["enrolled"]:
        status = f"✅ Note: {c['score']}" if c["completed"] else "🔄 In Bearbeitung"
        print(f"  {c['title']:35} {status}")


def demo_vector_recommendations(client: ThemisClient) -> None:
    """Kursempfehlungen basierend auf Vektor-Ähnlichkeit."""
    print("\n=== Vektor-Empfehlungen für Anna ===")

    # Kurs-IDs, die Anna bereits belegt hat
    enrolled_keys = client.query("""
        FOR course IN 1..1 OUTBOUND "mm_students/s_anna"
          GRAPH "mm_learning_graph"
          RETURN course._key
    """)

    # Interessen-Vektor: Durchschnitt der belegten Kurs-Embeddings
    if enrolled_keys:
        interest_vector = [0.0] * 8
        for key in enrolled_keys:
            emb = COURSE_EMBEDDINGS.get(key, [0.0] * 8)
            for i, v in enumerate(emb):
                interest_vector[i] += v / len(enrolled_keys)

        # Empfehlungen via Vektorähnlichkeit
        recommendations = client.query("""
            FOR course IN mm_courses
              FILTER course._key NOT IN @enrolled
              LET score = COSINE_SIMILARITY(course.embedding, @user_vector)
              SORT score DESC
              LIMIT 3
              RETURN { title: course.title, level: course.level, similarity: ROUND(score, 3) }
        """, bind_vars={"enrolled": enrolled_keys, "user_vector": interest_vector})

        print("Empfohlene Kurse für Anna:")
        for r in recommendations:
            stars = "★" * int(r["similarity"] * 5)
            print(f"  {r['title']:35} [{r['level']:12}] Ähnlichkeit: {r['similarity']:.3f} {stars}")


def demo_instructor_stats(client: ThemisClient) -> None:
    """Dozenten-Statistiken."""
    print("\n=== Dozenten-Statistiken ===")

    stats = client.query("""
        FOR instructor IN mm_instructors
          LET courses = (
            FOR course IN 1..1 INBOUND instructor mm_taught_by
              LET student_count = LENGTH(
                FOR s IN 1..1 INBOUND course mm_enrollments
                  RETURN 1
              )
              RETURN { title: course.title, students: student_count }
          )
          LET total_students = SUM(courses[*].students)
          RETURN {
            name:           instructor.name,
            course_count:   LENGTH(courses),
            total_students: total_students,
            courses:        courses[*].title
          }
    """)
    for s in stats:
        print(f"\n{s['name']}:")
        print(f"  Kurse ({s['course_count']}): {', '.join(s['courses'])}")
        print(f"  Gesamt-Einschreibungen: {s['total_students']}")


def demo_progress_analysis(client: ThemisClient) -> None:
    """Fortschrittsanalyse über Zeit."""
    print("\n=== Lernfortschritt-Analyse ===")

    # Ereignisse pro Tag
    events_per_day = client.query("""
        FOR event IN mm_progress
          LET date = SUBSTRING(event.ts, 0, 10)
          COLLECT day = date
            WITH COUNT INTO count
          SORT day ASC
          RETURN { day, count }
    """)
    print("Lernaktivität pro Tag:")
    for e in events_per_day:
        bar = "▓" * e["count"]
        print(f"  {e['day']}: {bar} ({e['count']} Ereignisse)")

    # Top-Lernende (nach Aktivität)
    top_learners = client.query("""
        FOR event IN mm_progress
          COLLECT student_id = event.student
            WITH COUNT INTO activity
          SORT activity DESC
          LET student = DOCUMENT("mm_students", student_id)
          RETURN { name: student.name, events: activity }
    """)
    print("\nAktivste Lernende:")
    for s in top_learners:
        bar = "🎯" * s["events"]
        print(f"  {s['name']:8}: {bar} ({s['events']} Ereignisse)")


def cleanup(client: ThemisClient) -> None:
    """Aufräumen."""
    print("\n=== Cleanup ===")
    try:
        client.query("DROP GRAPH mm_learning_graph")
    except Exception:
        pass
    for coll in ["mm_students", "mm_instructors", "mm_courses",
                 "mm_enrollments", "mm_taught_by", "mm_progress"]:
        client.query(f"DROP COLLECTION IF EXISTS {coll}")
    print("Collections gelöscht.")


def main() -> int:
    print("ThemisDB Schulungsbeispiel 4: Multi-Model-Anwendung (Lernplattform)")
    print("=" * 68)

    client = ThemisClient("http://localhost:8080")

    try:
        health = client.health()
        print(f"Verbunden mit ThemisDB {health.get('version', 'unbekannt')}")
    except Exception as e:
        print(f"FEHLER: {e}")
        return 1

    try:
        setup_data(client)
        demo_course_overview(client)
        demo_student_dashboard(client)
        demo_vector_recommendations(client)
        demo_instructor_stats(client)
        demo_progress_analysis(client)
    finally:
        cleanup(client)

    print("\n✅ Beispiel 4 erfolgreich abgeschlossen!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
