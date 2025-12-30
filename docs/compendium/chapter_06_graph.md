# Kapitel 6: Graph-Datenbanken

> *"The world is a graph, not a table."* — Graph-Datenbank-Community

## 6.1 Einführung: Die Welt der Verbindungen

Die Welt besteht aus Beziehungen. Menschen kennen andere Menschen. Websites verlinken auf andere Websites. Produkte werden zusammen gekauft. Straßen verbinden Städte. Diese natürlich vernetzten Strukturen lassen sich am besten als **Graphen** darstellen.

### Das Problem mit Relationalen Datenbanken

Versuchen Sie, folgende Frage mit SQL zu beantworten: *"Finde alle Freunde meiner Freunde, die in derselben Stadt wohnen und mindestens 3 gemeinsame Interessen haben."*

Mit relationalen Tabellen benötigen Sie:
- Multiple Self-Joins über die `friendships`-Tabelle
- Subqueries für gemeinsame Interessen
- Performance-Probleme bei mehr als 3-4 Hop-Levels
- Komplexe Query-Logik, die schwer zu warten ist

```aql
-- Freunde von Freunden (nur 2 Hops!)
SELECT DISTINCT u3.*
FROM users u1
JOIN friendships f1 ON u1.id = f1.user_id
JOIN users u2 ON f1.friend_id = u2.id
JOIN friendships f2 ON u2.id = f2.user_id
JOIN users u3 ON f2.friend_id = u3.id
WHERE u1.id = '...' 
  AND u3.city = u1.city
  AND ... -- gemeinsame Interessen?
```

Diese Query wird schnell unleserlich und langsam.

### Die Graph-Lösung

In ThemisDB wird dieselbe Frage intuitiv und performant:

```python
result = db.graph_query("""
    FOR user IN users
        FILTER user.id == @my_id
        FOR friend IN 1..2 OUTBOUND user friendships
            FILTER friend.city == user.city
            LET common_interests = LENGTH(
                INTERSECTION(user.interests, friend.interests)
            )
            FILTER common_interests >= 3
            RETURN friend
""", bind_vars={"my_id": my_id})
```

Die Graph-Query ist:
- ✅ Lesbarer und deklarativer
- ✅ Beliebig viele Hops möglich (`1..10`, `1..ANY`)
- ✅ Performance skaliert mit Graph-Struktur, nicht mit Datenmenge
- ✅ Native Graph-Algorithmen verfügbar

## 6.2 Property Graph Modell

ThemisDB verwendet das **Property Graph**-Modell, den De-facto-Standard für Graph-Datenbanken.

### Komponenten

**1. Knoten (Vertices/Nodes)**
- Repräsentieren Entitäten (Personen, Orte, Produkte)
- Haben einen eindeutigen Identifier
- Können beliebige Properties haben
- Können Labels/Types haben

```python
user_node = {
    "id": "user_001",
    "type": "User",
    "name": "Alice",
    "age": 28,
    "city": "Berlin",
    "interests": ["Python", "ML", "Hiking"]
}
```

**2. Kanten (Edges/Relationships)**
- Verbinden zwei Knoten (gerichtet oder ungerichtet)
- Haben einen Typ (z.B. "FRIEND", "LIKES", "WORKS_AT")
- Können Properties haben (z.B. "since", "strength")
- Erlauben Multi-Edges (mehrere Kanten zwischen denselben Knoten)

```python
friendship_edge = {
    "from": "user_001",
    "to": "user_002",
    "type": "FRIEND",
    "since": "2023-05-15",
    "strength": 0.85,  # 0-1 basierend auf Interaktionen
    "mutual_friends": 12
}
```

**3. Graph-Collections**

ThemisDB organisiert Graphs in Collections:
- **Vertex Collections**: Speichern Knoten (z.B. `users`, `posts`)
- **Edge Collections**: Speichern Kanten (z.B. `friendships`, `likes`)

```python
# Graph erstellen
db.create_vertex_collection("users")
db.create_edge_collection("friendships")

# Graph-Definition
graph_def = {
    "name": "social_network",
    "edge_definitions": [{
        "collection": "friendships",
        "from": ["users"],
        "to": ["users"]
    }]
}
db.create_graph(graph_def)
```

### Gerichtete vs. Ungerichtete Kanten

**Gerichtet (Directed):**
```python
# Alice folgt Bob, aber Bob folgt Alice nicht
{"from": "alice", "to": "bob", "type": "FOLLOWS"}
```

Beispiele: Twitter-Follows, Hyperlinks, Reporting-Struktur

**Ungerichtet (Undirected):**
```python
# Freundschaft ist bidirektional
{"from": "alice", "to": "bob", "type": "FRIEND"}
{"from": "bob", "to": "alice", "type": "FRIEND"}  # Beide Richtungen
```

Beispiele: Facebook-Freunde, Co-Autoren, Straßen

ThemisDB speichert alle Kanten gerichtet, aber Graph-Queries können beide Richtungen traversieren:
- `OUTBOUND` - Von A nach B
- `INBOUND` - Von B nach A
- `ANY` - Beide Richtungen (für ungerichtete Graphs)

## 6.3 Graph-Traversierung

### Traversierungs-Arten

**1. Depth-First Search (DFS)**
- Geht zuerst in die Tiefe
- Verwendet Stack
- Findet schnell tiefe Pfade

```python
# DFS: Alle erreichbaren Knoten
FOR v, e, p IN 1..10 OUTBOUND @start_vertex friendships
    OPTIONS {bfs: false}  # DFS (default)
    RETURN {vertex: v, path: p}
```

**2. Breadth-First Search (BFS)**
- Geht zuerst in die Breite
- Verwendet Queue
- Findet kürzeste Pfade

```python
# BFS: Kürzester Pfad
FOR v, e, p IN 1..10 OUTBOUND @start_vertex friendships
    OPTIONS {bfs: true}  # BFS
    RETURN {vertex: v, path: p}
```

**3. Bounded Traversal**
- Limitiert Anzahl der Hops
- `1..1` - Nur direkte Nachbarn
- `1..2` - Bis zu 2 Hops
- `2..4` - Zwischen 2 und 4 Hops
- `1..ANY` - Alle erreichbaren Knoten

```python
# Freunde von Freunden (exakt 2 Hops)
FOR v IN 2..2 OUTBOUND @my_id friendships
    RETURN v
```

### Pattern Matching

Graph-Queries in ThemisDB unterstützen komplexe Patterns:

```python
# Triangle Pattern: A kennt B, B kennt C, C kennt A
FOR a IN users
    FOR b IN OUTBOUND a friendships
        FOR c IN OUTBOUND b friendships
            FILTER c._id == a._id
            RETURN {a: a.name, b: b.name, c: c.name}
```

```python
# Diamond Pattern: Mehrere Pfade zwischen zwei Knoten
FOR start IN users FILTER start.id == @user_id
    FOR end IN OUTBOUND start friendships
        LET paths = (
            FOR v, e, p IN 2..2 OUTBOUND start friendships
                FILTER v._id == end._id
                RETURN p
        )
        FILTER LENGTH(paths) > 1  # Mehrere Pfade
        RETURN {start: start.name, end: end.name, paths: paths}
```

## 6.4 Graph-Algorithmen

ThemisDB bietet native Implementierungen gängiger Graph-Algorithmen:

### Shortest Path (Dijkstra)

Findet den kürzesten Pfad zwischen zwei Knoten unter Berücksichtigung von Gewichten:

```python
from themis_client import shortest_path

path = shortest_path(
    graph="social_network",
    start_vertex="users/alice",
    target_vertex="users/bob",
    direction="outbound",
    weight_attribute="strength"  # Optional: Kantengewicht
)

print(f"Pfad: {' -> '.join([v['name'] for v in path['vertices']])}")
print(f"Distanz: {path['distance']}")
```

**Use Cases:**
- Routing und Navigation
- Social Distance berechnen
- Kostenoptimale Pfade finden

### All Shortest Paths

Findet alle kürzesten Pfade (falls mehrere existieren):

```python
paths = db.all_shortest_paths(
    graph="social_network",
    start_vertex="users/alice",
    target_vertex="users/bob"
)

for idx, path in enumerate(paths):
    print(f"Pfad {idx+1}: {' -> '.join([v['name'] for v in path['vertices']])}")
```

### K Shortest Paths

Findet die k kürzesten Pfade:

```python
paths = db.k_shortest_paths(
    graph="social_network",
    start_vertex="users/alice",
    target_vertex="users/bob",
    k=3  # Top 3 kürzeste Pfade
)
```

### Connected Components

Findet zusammenhängende Teilgraphen:

```python
components = db.connected_components(
    graph="social_network",
    direction="any"  # Ungerichtet behandeln
)

# Gruppiere Knoten nach Component
for component_id, vertices in components.items():
    print(f"Community {component_id}: {len(vertices)} Mitglieder")
```

**Use Cases:**
- Community-Erkennung
- Isolierte Gruppen finden
- Netzwerk-Fragmentierung analysieren

### PageRank

Berechnet die Wichtigkeit von Knoten basierend auf eingehenden Kanten:

```python
pagerank_scores = db.pagerank(
    graph="social_network",
    iterations=20,
    damping_factor=0.85
)

# Sortiere nach Score
top_users = sorted(
    pagerank_scores.items(), 
    key=lambda x: x[1], 
    reverse=True
)[:10]

for user_id, score in top_users:
    user = db.get("users", user_id)
    print(f"{user['name']}: {score:.4f}")
```

**Use Cases:**
- Influencer-Identifikation
- Content-Ranking
- Reputations-Systeme

### Betweenness Centrality

Misst, wie oft ein Knoten auf kürzesten Pfaden liegt:

```python
centrality = db.betweenness_centrality(
    graph="social_network"
)

# Knoten mit hoher Betweenness = Brücken zwischen Communities
bridges = [k for k, v in centrality.items() if v > 0.5]
```

**Use Cases:**
- Netzwerk-Bottlenecks identifizieren
- Wichtige Vermittler finden
- Kritische Infrastruktur-Knoten

## 6.5 Praxisbeispiel 1: Social Network

Jetzt setzen wir die Theorie in die Praxis um mit einem vollständigen Social Network.

### Das Projekt

Das Social Network-Example (`examples/06_graph_social_network`) implementiert:
- Benutzerprofile mit Interessen
- Bidirektionale Freundschaften
- Graph-Traversierung (Freunde von Freunden)
- Kürzeste Pfade zwischen Usern
- Community-Erkennung
- Freundschafts-Empfehlungen
- Interaktive Visualisierung mit NetworkX

### Datenmodell

```python
# models.py
from dataclasses import dataclass
from typing import List, Optional
from datetime import datetime

@dataclass
class User:
    """Ein Benutzer im sozialen Netzwerk."""
    id: str
    name: str
    bio: Optional[str] = None
    interests: List[str] = None
    location: Optional[str] = None
    joined: datetime = None
    
    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "bio": self.bio,
            "interests": self.interests or [],
            "location": self.location,
            "joined": self.joined.isoformat() if self.joined else None
        }

@dataclass
class Friendship:
    """Eine Freundschaft zwischen zwei Benutzern."""
    from_user: str
    to_user: str
    since: datetime
    strength: float = 1.0  # 0-1, basierend auf Interaktionen
    
    def to_dict(self):
        return {
            "from": f"users/{self.from_user}",
            "to": f"users/{self.to_user}",
            "since": self.since.isoformat(),
            "strength": self.strength
        }
```

### Graph Setup

```python
# themis_client.py (gekürzt)
class ThemisGraphClient:
    def __init__(self, host="localhost", port=8529):
        self.client = ThemisDB(host=host, port=port)
        self.db = self.client.db("social_network")
        self._setup_graph()
    
    def _setup_graph(self):
        """Erstellt Collections und Graph-Definition."""
        # Vertex Collection für User
        if not self.db.has_collection("users"):
            self.db.create_vertex_collection("users")
        
        # Edge Collection für Freundschaften
        if not self.db.has_collection("friendships"):
            self.db.create_edge_collection("friendships")
        
        # Graph-Definition
        if not self.db.has_graph("social_graph"):
            graph_def = {
                "name": "social_graph",
                "edge_definitions": [{
                    "collection": "friendships",
                    "from": ["users"],
                    "to": ["users"]
                }]
            }
            self.db.create_graph(graph_def)
        
        # Indexes für Performance
        self.db.add_index("users", ["name"])
        self.db.add_index("users", ["location"])
        self.db.add_index("friendships", ["from", "to"])
```

### Benutzer und Freundschaften

```python
def add_user(self, user: User) -> str:
    """Fügt einen neuen Benutzer hinzu."""
    user_doc = user.to_dict()
    result = self.db.insert("users", user_doc)
    return result["_key"]

def add_friendship(self, friendship: Friendship):
    """Erstellt eine bidirektionale Freundschaft."""
    # Richtung 1: A -> B
    self.db.insert("friendships", friendship.to_dict())
    
    # Richtung 2: B -> A (für ungerichteten Graph)
    reverse = Friendship(
        from_user=friendship.to_user,
        to_user=friendship.from_user,
        since=friendship.since,
        strength=friendship.strength
    )
    self.db.insert("friendships", reverse.to_dict())

def get_friends(self, user_id: str) -> List[User]:
    """Holt alle direkten Freunde eines Benutzers."""
    query = """
        FOR friend IN OUTBOUND @user_id friendships
            RETURN friend
    """
    result = self.db.execute_query(query, bind_vars={"user_id": f"users/{user_id}"})
    return [User(**doc) for doc in result]
```

### Friends-of-Friends (FoF)

Ein klassisches Graph-Problem: Finde Freunde meiner Freunde, die noch nicht meine Freunde sind.

```python
def get_friend_suggestions(self, user_id: str, limit: int = 10) -> List[dict]:
    """Empfiehlt neue Freunde basierend auf gemeinsamen Freunden."""
    query = """
        FOR friend IN OUTBOUND @user_id friendships
            FOR fof IN OUTBOUND friend._id friendships
                FILTER fof._id != @user_id
                FILTER fof._id NOT IN (
                    FOR f IN OUTBOUND @user_id friendships
                        RETURN f._id
                )
                COLLECT fof_user = fof WITH COUNT INTO mutual_count
                SORT mutual_count DESC
                LIMIT @limit
                RETURN {
                    user: fof_user,
                    mutual_friends: mutual_count
                }
    """
    return self.db.execute_query(query, bind_vars={
        "user_id": f"users/{user_id}",
        "limit": limit
    })
```

**Was passiert hier?**
1. Traversiere zu allen Freunden (`OUTBOUND @user_id`)
2. Von jedem Freund, traversiere zu deren Freunden (`OUTBOUND friend._id`)
3. Filtere mich selbst heraus (`FILTER fof._id != @user_id`)
4. Filtere existierende Freunde heraus (Subquery)
5. Gruppiere nach FoF und zähle gemeinsame Freunde (`COLLECT ... WITH COUNT`)
6. Sortiere nach Anzahl gemeinsamer Freunde

### Kürzeste Pfade

Wie ist Alice mit Bob verbunden?

```python
def find_connection_path(self, user_id1: str, user_id2: str) -> dict:
    """Findet den kürzesten Pfad zwischen zwei Benutzern."""
    path = self.db.shortest_path(
        graph="social_graph",
        start_vertex=f"users/{user_id1}",
        target_vertex=f"users/{user_id2}",
        direction="any"  # Ungerichtet (beide Richtungen)
    )
    
    if path is None:
        return {"connected": False}
    
    return {
        "connected": True,
        "distance": path["distance"],
        "path": [v["name"] for v in path["vertices"]],
        "degrees_of_separation": len(path["vertices"]) - 1
    }
```

### Community-Erkennung

Welche natürlichen Gruppen gibt es im Netzwerk?

```python
def detect_communities(self) -> dict:
    """Findet Communities mittels Connected Components."""
    components = self.db.connected_components(
        graph="social_graph",
        direction="any"
    )
    
    # Statistiken pro Community
    communities = {}
    for component_id, user_ids in components.items():
        users = [self.db.get("users", uid) for uid in user_ids]
        
        # Gemeinsame Interessen
        all_interests = []
        for user in users:
            all_interests.extend(user.get("interests", []))
        interest_counts = Counter(all_interests)
        
        communities[component_id] = {
            "size": len(users),
            "members": [u["name"] for u in users],
            "top_interests": interest_counts.most_common(5)
        }
    
    return communities
```

### Interaktive Visualisierung

Das Example nutzt NetworkX für Visualisierung:

```python
import networkx as nx
import matplotlib.pyplot as plt

def visualize_network(self, user_id: Optional[str] = None, max_hops: int = 2):
    """Visualisiert das soziale Netzwerk."""
    G = nx.Graph()
    
    if user_id:
        # Nur Ego-Netzwerk eines Users
        query = f"""
            FOR v, e IN 1..{max_hops} ANY @user_id friendships
                RETURN {{vertex: v, edge: e}}
        """
        result = self.db.execute_query(query, bind_vars={"user_id": f"users/{user_id}"})
    else:
        # Ganzes Netzwerk
        users = self.db.all("users")
        friendships = self.db.all("friendships")
        
        for user in users:
            G.add_node(user["_key"], name=user["name"])
        
        for friendship in friendships:
            from_id = friendship["_from"].split("/")[1]
            to_id = friendship["_to"].split("/")[1]
            G.add_edge(from_id, to_id, weight=friendship.get("strength", 1.0))
    
    # Layout mit Spring-Algorithmus
    pos = nx.spring_layout(G, k=0.5, iterations=50)
    
    # Zeichnen
    plt.figure(figsize=(12, 8))
    nx.draw_networkx_nodes(G, pos, node_size=500, node_color='lightblue')
    nx.draw_networkx_edges(G, pos, alpha=0.5)
    nx.draw_networkx_labels(G, pos, labels=nx.get_node_attributes(G, 'name'))
    
    plt.title("Social Network Graph")
    plt.axis('off')
    plt.tight_layout()
    plt.show()
```

### Main Application

```python
# main.py
def main():
    client = ThemisGraphClient()
    
    # Beispiel-Daten
    users = [
        User("alice", "Alice", "Software Engineer", ["Python", "ML", "Hiking"], "Berlin"),
        User("bob", "Bob", "Data Scientist", ["Python", "Statistics", "Music"], "Munich"),
        User("charlie", "Charlie", "DevOps", ["Docker", "Kubernetes", "Hiking"], "Berlin"),
        User("diana", "Diana", "Product Manager", ["Agile", "UX", "Music"], "Hamburg"),
        User("eve", "Eve", "ML Engineer", ["ML", "Python", "Research"], "Berlin"),
    ]
    
    for user in users:
        client.add_user(user)
    
    # Freundschaften
    friendships = [
        ("alice", "bob"),
        ("alice", "charlie"),
        ("bob", "diana"),
        ("charlie", "eve"),
        ("diana", "eve"),
    ]
    
    for user1, user2 in friendships:
        client.add_friendship(Friendship(user1, user2, datetime.now(), 0.8))
    
    # 1. Freunde von Alice
    print("Alice's Friends:")
    friends = client.get_friends("alice")
    for friend in friends:
        print(f"  - {friend.name} ({friend.location})")
    
    # 2. Freundschafts-Empfehlungen für Alice
    print("\nFriend Suggestions for Alice:")
    suggestions = client.get_friend_suggestions("alice", limit=3)
    for suggestion in suggestions:
        print(f"  - {suggestion['user']['name']}: {suggestion['mutual_friends']} mutual friends")
    
    # 3. Verbindung zwischen Alice und Eve
    print("\nConnection between Alice and Eve:")
    path = client.find_connection_path("alice", "eve")
    if path["connected"]:
        print(f"  Path: {' -> '.join(path['path'])}")
        print(f"  Degrees of Separation: {path['degrees_of_separation']}")
    
    # 4. Communities
    print("\nCommunities:")
    communities = client.detect_communities()
    for comm_id, comm_data in communities.items():
        print(f"  Community {comm_id}: {comm_data['size']} members")
        print(f"    Members: {', '.join(comm_data['members'])}")
        print(f"    Top Interests: {[i[0] for i in comm_data['top_interests']]}")
    
    # 5. Visualisierung
    client.visualize_network()

if __name__ == "__main__":
    main()
```

### Output

```
Alice's Friends:
  - Bob (Munich)
  - Charlie (Berlin)

Friend Suggestions for Alice:
  - Diana: 1 mutual friends
  - Eve: 1 mutual friends

Connection between Alice and Eve:
  Path: Alice -> Charlie -> Eve
  Degrees of Separation: 2

Communities:
  Community 1: 5 members
    Members: Alice, Bob, Charlie, Diana, Eve
    Top Interests: ['Python', 'ML', 'Hiking', 'Music', 'Docker']

[Visualization opens in matplotlib window]
```

### Performance-Optimierung

**1. Indexes auf häufig abgefragte Felder:**
```python
self.db.add_index("users", ["name"])
self.db.add_index("users", ["location"])
self.db.add_index("friendships", ["from", "to"])
```

**2. Batch-Operations für viele Inserts:**
```python
def add_users_batch(self, users: List[User]):
    """Fügt mehrere User auf einmal hinzu."""
    user_docs = [u.to_dict() for u in users]
    self.db.insert_many("users", user_docs)
```

**3. Bounded Traversals:**
```python
# Statt 1..ANY (alle Knoten)
FOR v IN 1..3 OUTBOUND @user_id friendships  # Max 3 Hops
    RETURN v
```

**4. Index-basierte Filters:**
```python
# Filter NACH Index-Lookup
FOR user IN users
    FILTER user.location == "Berlin"  # Nutzt Index
    FOR friend IN OUTBOUND user._id friendships
        RETURN friend
```

## 6.6 Praxisbeispiel 2: Recommendation Engine

Das zweite Example (`examples/19_recommendation_engine`) zeigt einen komplexeren Use Case: **ML-basierte Empfehlungen** mit Graph- und Vektor-Daten.

### Das Konzept

Empfehlungssysteme kombinieren mehrere Ansätze:

1. **Collaborative Filtering**: "User, die X mochten, mochten auch Y"
2. **Content-Based Filtering**: "Ähnliche Items basierend auf Features"
3. **Graph-basiert**: "Freunde deiner Freunde kauften X"
4. **Hybrid**: Kombination aller Ansätze

### Datenmodell

```python
@dataclass
class Item:
    """Ein empfehlbares Item (Produkt, Film, etc.)."""
    id: str
    title: str
    category: str
    features: List[str]
    embedding: List[float]  # Content-Embedding für Similarity
    
@dataclass
class Interaction:
    """Eine User-Item Interaktion."""
    user_id: str
    item_id: str
    type: str  # "view", "click", "purchase", "rate"
    rating: Optional[float] = None
    timestamp: datetime = None
    context: dict = None  # Device, location, etc.

@dataclass
class Recommendation:
    """Eine generierte Empfehlung."""
    user_id: str
    item_id: str
    score: float
    method: str  # "collaborative", "content_based", "graph", "hybrid"
    explanation: str
```

### Graph-Setup

```python
def _setup_graph(self):
    """Erstellt Multi-Graph für Recommendations."""
    # Vertex Collections
    self.db.create_vertex_collection("users")
    self.db.create_vertex_collection("items")
    
    # Edge Collections
    self.db.create_edge_collection("interactions")  # User -> Item
    self.db.create_edge_collection("similar_items")  # Item -> Item
    self.db.create_edge_collection("user_similarity")  # User -> User
    
    # Graph-Definition
    graph_def = {
        "name": "recommendation_graph",
        "edge_definitions": [
            {
                "collection": "interactions",
                "from": ["users"],
                "to": ["items"]
            },
            {
                "collection": "similar_items",
                "from": ["items"],
                "to": ["items"]
            },
            {
                "collection": "user_similarity",
                "from": ["users"],
                "to": ["users"]
            }
        ]
    }
    self.db.create_graph(graph_def)
```

### Collaborative Filtering

Findet Items, die ähnliche User mochten:

```python
def collaborative_filtering(self, user_id: str, limit: int = 10) -> List[Recommendation]:
    """Empfehle Items basierend auf ähnlichen Usern."""
    query = """
        // 1. Finde ähnliche User (basierend auf vergangenen Interaktionen)
        FOR similar_user IN OUTBOUND @user_id user_similarity
            SORT similar_user.similarity DESC
            LIMIT 20
            
            // 2. Finde Items, die ähnliche User mochten
            FOR item IN OUTBOUND similar_user._id interactions
                FILTER item.interaction_type IN ["purchase", "rate"]
                FILTER item.rating >= 4  // Nur positive
                
                // 3. Filtere Items, die User schon kennt
                FILTER item._id NOT IN (
                    FOR known_item IN OUTBOUND @user_id interactions
                        RETURN known_item._id
                )
                
                // 4. Aggregiere Score
                COLLECT item_id = item._id 
                AGGREGATE score = AVG(item.rating * similar_user.similarity)
                
                SORT score DESC
                LIMIT @limit
                
                // 5. Hole Item-Details
                LET item_doc = DOCUMENT(item_id)
                RETURN {
                    item_id: item_id,
                    score: score,
                    method: "collaborative",
                    explanation: CONCAT("Users similar to you rated this ", score)
                }
    """
    
    results = self.db.execute_query(query, bind_vars={
        "user_id": f"users/{user_id}",
        "limit": limit
    })
    
    return [Recommendation(**r) for r in results]
```

### Content-Based Filtering

Findet ähnliche Items basierend auf Features:

```python
def content_based_filtering(self, user_id: str, limit: int = 10) -> List[Recommendation]:
    """Empfehle Items ähnlich zu den, die User mag."""
    query = """
        // 1. Finde Items, die User mag
        FOR liked_item IN OUTBOUND @user_id interactions
            FILTER liked_item.rating >= 4
            
            // 2. Finde ähnliche Items
            FOR similar_item IN OUTBOUND liked_item._id similar_items
                SORT similar_item.similarity DESC
                
                // 3. Filtere bekannte Items
                FILTER similar_item._id NOT IN (
                    FOR known IN OUTBOUND @user_id interactions
                        RETURN known._id
                )
                
                // 4. Aggregiere
                COLLECT item_id = similar_item._id
                AGGREGATE score = AVG(similar_item.similarity)
                
                SORT score DESC
                LIMIT @limit
                
                LET item_doc = DOCUMENT(item_id)
                RETURN {
                    item_id: item_id,
                    score: score,
                    method: "content_based",
                    explanation: CONCAT("Similar to items you liked")
                }
    """
    
    results = self.db.execute_query(query, bind_vars={
        "user_id": f"users/{user_id}",
        "limit": limit
    })
    
    return [Recommendation(**r) for r in results]
```

### Graph-basierte Empfehlungen

Nutzt Social Graph für Empfehlungen:

```python
def graph_based_recommendations(self, user_id: str, limit: int = 10) -> List[Recommendation]:
    """Empfehle Items, die Freunde mochten."""
    query = """
        // 1. Traversiere zu Freunden (1-2 Hops)
        FOR friend IN 1..2 OUTBOUND @user_id friendships
            
            // 2. Finde Items, die Freund kaufte
            FOR item IN OUTBOUND friend._id interactions
                FILTER item.interaction_type == "purchase"
                
                // 3. Filtere bekannte Items
                FILTER item._id NOT IN (
                    FOR known IN OUTBOUND @user_id interactions
                        RETURN known._id
                )
                
                // 4. Score basierend auf Freundschafts-Stärke und Rating
                COLLECT item_id = item._id
                AGGREGATE score = AVG(friend.friendship_strength * item.rating)
                
                SORT score DESC
                LIMIT @limit
                
                LET item_doc = DOCUMENT(item_id)
                RETURN {
                    item_id: item_id,
                    score: score,
                    method: "graph_based",
                    explanation: "Your friends liked this"
                }
    """
    
    results = self.db.execute_query(query, bind_vars={
        "user_id": f"users/{user_id}",
        "limit": limit
    })
    
    return [Recommendation(**r) for r in results]
```

### Hybrid Recommendations

Kombiniert alle Methoden mit Gewichtung:

```python
def hybrid_recommendations(self, user_id: str, limit: int = 10) -> List[Recommendation]:
    """Kombiniert alle Empfehlungsmethoden."""
    # Hole Empfehlungen von allen Methoden
    collaborative = self.collaborative_filtering(user_id, limit=20)
    content_based = self.content_based_filtering(user_id, limit=20)
    graph_based = self.graph_based_recommendations(user_id, limit=20)
    
    # Gewichtung
    weights = {
        "collaborative": 0.4,
        "content_based": 0.3,
        "graph_based": 0.3
    }
    
    # Kombiniere Scores
    combined_scores = {}
    for recs, method in [
        (collaborative, "collaborative"),
        (content_based, "content_based"),
        (graph_based, "graph_based")
    ]:
        for rec in recs:
            if rec.item_id not in combined_scores:
                combined_scores[rec.item_id] = {
                    "score": 0,
                    "methods": [],
                    "explanations": []
                }
            
            combined_scores[rec.item_id]["score"] += rec.score * weights[method]
            combined_scores[rec.item_id]["methods"].append(method)
            combined_scores[rec.item_id]["explanations"].append(rec.explanation)
    
    # Sortiere und limitiere
    sorted_items = sorted(
        combined_scores.items(),
        key=lambda x: x[1]["score"],
        reverse=True
    )[:limit]
    
    # Erstelle finale Recommendations
    recommendations = []
    for item_id, data in sorted_items:
        recommendations.append(Recommendation(
            user_id=user_id,
            item_id=item_id,
            score=data["score"],
            method="hybrid",
            explanation=f"Recommended via: {', '.join(data['methods'])}"
        ))
    
    return recommendations
```

### Similarity Berechnung

Berechne User-User und Item-Item Similarity:

```python
def compute_user_similarity(self):
    """Berechnet Similarity zwischen allen User-Paaren."""
    users = self.db.all("users")
    
    for i, user1 in enumerate(users):
        for user2 in users[i+1:]:
            # Gemeinsame Interaktionen
            common_items = self._get_common_interactions(user1["_id"], user2["_id"])
            
            if len(common_items) >= 3:  # Mindestens 3 gemeinsame
                similarity = self._calculate_similarity(
                    user1["interaction_vector"],
                    user2["interaction_vector"]
                )
                
                # Speichere Kante
                self.db.insert("user_similarity", {
                    "from": user1["_id"],
                    "to": user2["_id"],
                    "similarity": similarity,
                    "common_items": len(common_items)
                })

def compute_item_similarity(self):
    """Berechnet Similarity zwischen Items (content-based)."""
    items = self.db.all("items")
    
    for i, item1 in enumerate(items):
        for item2 in items[i+1:]:
            # Cosine Similarity der Embeddings
            similarity = cosine_similarity(
                item1["embedding"],
                item2["embedding"]
            )
            
            if similarity > 0.7:  # Threshold
                self.db.insert("similar_items", {
                    "from": item1["_id"],
                    "to": item2["_id"],
                    "similarity": similarity
                })
```

### Real-Time Updates

Aktualisiere Empfehlungen bei neuen Interaktionen:

```python
def record_interaction(self, interaction: Interaction):
    """Zeichnet Interaktion auf und aktualisiert Empfehlungen."""
    # Speichere Interaktion
    interaction_doc = {
        "from": f"users/{interaction.user_id}",
        "to": f"items/{interaction.item_id}",
        "type": interaction.type,
        "rating": interaction.rating,
        "timestamp": interaction.timestamp.isoformat(),
        "context": interaction.context
    }
    self.db.insert("interactions", interaction_doc)
    
    # Bei Purchase: Update User-Vektor
    if interaction.type == "purchase":
        self._update_user_vector(interaction.user_id, interaction.item_id)
        
        # Recalculate Similarity (async)
        self._queue_similarity_update(interaction.user_id)
```

## 6.7 Graph Best Practices

### 1. Modeling Guidelines

**DO:**
- ✅ Nutze sprechende Edge-Namen (`FRIEND`, `LIKES`, `WORKS_AT`)
- ✅ Speichere Properties auf Kanten (Zeitstempel, Gewichte)
- ✅ Denormalisiere häufig benötigte Daten
- ✅ Nutze Bidirektionale Kanten für ungerichtete Graphs

**DON'T:**
- ❌ Zu viele Edge-Types (schwer zu querien)
- ❌ Properties als separate Knoten (ineffizient)
- ❌ Extrem tiefe Hierarchien (> 10 Levels)

### 2. Performance-Tipps

**Indexes:**
```python
# Composite Index für häufige Lookups
db.add_index("friendships", ["from", "to"])
db.add_index("interactions", ["user_id", "timestamp"])
```

**Bounded Traversals:**
```python
# Limitiere Hops
FOR v IN 1..3 OUTBOUND @start  # Nicht 1..ANY
    LIMIT 100  # Früh limitieren
    RETURN v
```

**Prune Early:**
```python
# Filter so früh wie möglich
FOR v IN 1..5 OUTBOUND @start friendships
    FILTER v.city == "Berlin"  # Früher Filter
    FOR v2 IN OUTBOUND v._id friendships
        RETURN v2
```

### 3. Häufige Patterns

**Pattern 1: Mutual Friends**
```python
FOR friend IN OUTBOUND @user1 friendships
    FILTER friend._id IN (
        FOR f IN OUTBOUND @user2 friendships
            RETURN f._id
    )
    RETURN friend
```

**Pattern 2: Influencer (hohe Degree)**
```python
FOR user IN users
    LET friend_count = LENGTH(
        FOR f IN OUTBOUND user._id friendships
            RETURN 1
    )
    FILTER friend_count > 100
    SORT friend_count DESC
    RETURN {user: user, friends: friend_count}
```

**Pattern 3: Isolated Nodes**
```python
FOR user IN users
    LET connections = (
        FOR v IN ANY user._id friendships
            RETURN 1
    )
    FILTER LENGTH(connections) == 0
    RETURN user
```

## 6.8 Vergleich: ThemisDB vs. Neo4j vs. ArangoDB

| Feature | ThemisDB | Neo4j | ArangoDB |
|---------|----------|-------|----------|
| **Graph Model** | Property Graph | Property Graph | Property Graph |
| **Query Language** | AQL | Cypher | AQL |
| **Multi-Model** | ✅ 4 Models | ❌ Graph only | ✅ 3 Models |
| **ACID** | ✅ Full | ✅ Full | ✅ Full |
| **Sharding** | ✅ Automatic | ❌ Enterprise only | ✅ Yes |
| **Graph Algorithms** | ✅ Built-in | ✅ GDS Library | ✅ Pregel |
| **License** | Apache 2.0 | GPLv3 + Commercial | Apache 2.0 |
| **Embedding Support** | ✅ Native | ❌ No | ❌ No |

**Wann ThemisDB?**
- Multi-Model Daten (Graph + Vektor + Relational)
- Native Vektor-Suche benötigt
- Open-Source und selbst-hosted
- Kombinierte Queries über mehrere Modelle

**Wann Neo4j?**
- Reines Graph-Problem
- Cypher-Erfahrung im Team
- Enterprise Support benötigt
- Graph Data Science Library

**Wann ArangoDB?**
- Ähnlich wie ThemisDB (Multi-Model)
- Mature Community
- Foxx Microservices

## 6.9 Zusammenfassung

In diesem Kapitel haben Sie gelernt:

✅ **Property Graph Modell** - Knoten, Kanten, Properties  
✅ **Graph-Traversierung** - DFS, BFS, Pattern Matching  
✅ **Graph-Algorithmen** - Shortest Path, PageRank, Community Detection  
✅ **Praxis: Social Network** - Vollständiges soziales Netzwerk mit Visualisierung  
✅ **Praxis: Recommendations** - ML-basierte Empfehlungen mit Hybrid-Ansatz  
✅ **Best Practices** - Modeling, Performance, Patterns  

Graph-Datenbanken sind die natürliche Wahl für vernetzte Daten. ThemisDB's Property Graph-Implementierung bietet performante Traversierung, native Algorithmen und die einzigartige Möglichkeit, Graphs mit anderen Modellen zu kombinieren.

Im nächsten Kapitel schauen wir uns **Dokument-Speicherung** an - flexibles, schema-less Design für semi-strukturierte Daten.

---

**Übungen:**

1. Erweitern Sie das Social Network um Posts und Likes (User -> Post -> Likes)
2. Implementieren Sie einen Follower/Following-Mechanismus (Twitter-Style)
3. Fügen Sie PageRank-Berechnung hinzu, um Influencer zu finden
4. Erstellen Sie einen Interest-Based-Graph (User -> Interest <- User)
5. Bauen Sie ein Skill-Recommendation-System für LinkedIn-Style-Netzwerk

**Weiterführende Ressourcen:**

- 📖 `examples/06_graph_social_network/` - Vollständiger Code
- 📖 `examples/19_recommendation_engine/` - Recommendation System
- 📖 `docs/de/features/features_property_graph.md` - Graph-Features
- 📖 NetworkX Documentation - Graph-Visualisierung
- 📖 "Graph Algorithms" (Mark Needham, Amy E. Hodler) - Tiefere Algorithmen
