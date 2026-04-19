# Graph Soziales Netzwerk - Graph-Theorie und Algorithmen

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Übersicht

Dieses Dokument erklärt die Graph-Theorie-Konzepte und Algorithmen, die im Sozialen Netzwerk-Beispiel verwendet werden.

## Graph-Grundlagen

### Was ist ein Graph?

Ein Graph G = (V, E) besteht aus:
- **V (Vertices/Knoten)**: Menge von Objekten (hier: Benutzer)
- **E (Edges/Kanten)**: Menge von Verbindungen zwischen Knoten (hier: Freundschaften)

### Graph-Typen

**Ungerichteter Graph** (verwendet in diesem Beispiel):
```
A ←→ B    Freundschaft ist bidirektional
          Wenn A mit B befreundet ist, ist B auch mit A befreundet
```

**Gerichteter Graph** (nicht verwendet):
```
A → B     A folgt B, aber B folgt nicht notwendigerweise A
          (z.B. Twitter/Instagram)
```

### Graph-Repräsentation

**Adjazenzliste** (verwendet in ThemisDB):
```python
graph = {
    "Alice": ["Bob", "Charlie"],
    "Bob": ["Alice", "David"],
    "Charlie": ["Alice", "David"],
    "David": ["Bob", "Charlie"]
}
```

**Adjazenzmatrix** (Alternative):
```
     A  B  C  D
A [  0  1  1  0 ]
B [  1  0  0  1 ]
C [  1  0  0  1 ]
D [  0  1  1  0 ]
```

## Implementierte Algorithmen

### 1. Breadth-First Search (BFS)

**Zweck**: Kürzesten Pfad finden, Level-Order Traversierung

**Funktionsweise**:
```
1. Starte bei Startknoten
2. Besuche alle direkten Nachbarn (Level 1)
3. Dann alle Nachbarn der Nachbarn (Level 2)
4. Und so weiter...
```

**Visualisierung**:
```
Start: Alice

Level 0:  Alice
Level 1:  Bob, Charlie  (Freunde von Alice)
Level 2:  David, Eve    (Freunde der Freunde)
Level 3:  Frank         (3. Grad)
```

**Python-Implementierung**:
```python
from collections import deque

def bfs(graph, start):
    """Breadth-First Search"""
    visited = set()
    queue = deque([start])
    order = []
    
    while queue:
        node = queue.popleft()
        if node not in visited:
            visited.add(node)
            order.append(node)
            
            # Füge alle Nachbarn zur Queue hinzu
            for neighbor in graph.get(node, []):
                if neighbor not in visited:
                    queue.append(neighbor)
    
    return order

# Beispiel
result = bfs(graph, "Alice")
# → ["Alice", "Bob", "Charlie", "David", "Eve"]
```

**Komplexität**:
- Zeit: O(V + E) - Jeder Knoten und jede Kante wird einmal besucht
- Speicher: O(V) - Queue und visited-Set

**Anwendungen**:
- Kürzeste Pfade in ungewichteten Graphen
- Freunde von Freunden finden
- Netzwerk-Distanz berechnen

### 2. Kürzester Pfad (Shortest Path)

**Problem**: Finde den kürzesten Weg zwischen zwei Benutzern

**Algorithmus**: BFS mit Pfad-Tracking

```python
def shortest_path(graph, start, target):
    """Findet kürzesten Pfad zwischen zwei Knoten"""
    if start == target:
        return [start]
    
    visited = set()
    queue = deque([(start, [start])])  # (knoten, pfad)
    
    while queue:
        node, path = queue.popleft()
        
        if node == target:
            return path
        
        if node not in visited:
            visited.add(node)
            
            for neighbor in graph.get(node, []):
                if neighbor not in visited:
                    queue.append((neighbor, path + [neighbor]))
    
    return None  # Kein Pfad gefunden

# Beispiel
path = shortest_path(graph, "Alice", "Frank")
# → ["Alice", "Bob", "David", "Frank"]
```

**Eigenschaften**:
- Garantiert kürzesten Pfad in ungewichteten Graphen
- Stoppt sobald Ziel gefunden
- Komplexität: O(V + E)

**Visualisierung**:
```
Alice → Bob → David → Frank
  ↓      ↓      ↓
  3      2      1   (Schritte bis Frank)
```

### 3. Freunde von Freunden (Friends of Friends)

**Problem**: Finde Personen, die 2 Schritte entfernt sind

**Algorithmus**: BFS mit Level-Limitierung

```python
def friends_of_friends(graph, user):
    """Findet Freunde von Freunden (2. Grad)"""
    # Level 1: Direkte Freunde
    friends = set(graph.get(user, []))
    
    # Level 2: Freunde der Freunde
    fof = set()
    for friend in friends:
        for friend_of_friend in graph.get(friend, []):
            if friend_of_friend != user and friend_of_friend not in friends:
                fof.add(friend_of_friend)
    
    return list(fof)

# Beispiel
result = friends_of_friends(graph, "Alice")
# → ["David", "Eve"] (keine direkten Freunde, aber 2 Schritte entfernt)
```

**Optimierung mit BFS**:
```python
def friends_of_friends_bfs(graph, user, max_level=2):
    """BFS-basierte Implementierung mit Level-Tracking"""
    visited = {user: 0}  # {knoten: level}
    queue = deque([(user, 0)])
    result = {}
    
    while queue:
        node, level = queue.popleft()
        
        if level < max_level:
            for neighbor in graph.get(node, []):
                if neighbor not in visited:
                    visited[neighbor] = level + 1
                    result[neighbor] = level + 1
                    queue.append((neighbor, level + 1))
    
    # Nur Level 2 zurückgeben
    return [node for node, lvl in result.items() if lvl == 2]
```

### 4. Freunde-Empfehlungen

**Problem**: Empfehle neue Freundschaften basierend auf gemeinsamen Freunden

**Algorithmus**: Mutual Friends Scoring

```python
def recommend_friends(graph, user, top_n=5):
    """Empfiehlt Freunde basierend auf gemeinsamen Kontakten"""
    friends = set(graph.get(user, []))
    
    # Zähle gemeinsame Freunde
    recommendations = {}
    
    for friend in friends:
        for potential_friend in graph.get(friend, []):
            # Nicht sich selbst oder bestehende Freunde
            if potential_friend == user or potential_friend in friends:
                continue
            
            # Zähle gemeinsame Freunde
            if potential_friend not in recommendations:
                recommendations[potential_friend] = 0
            recommendations[potential_friend] += 1
    
    # Sortiere nach Anzahl gemeinsamer Freunde
    sorted_recommendations = sorted(
        recommendations.items(),
        key=lambda x: x[1],
        reverse=True
    )
    
    return sorted_recommendations[:top_n]

# Beispiel
recommendations = recommend_friends(graph, "Alice", top_n=3)
# → [("David", 2), ("Eve", 1), ("Frank", 1)]
#    David hat 2 gemeinsame Freunde mit Alice
```

**Erweiterte Scoring-Methoden**:

```python
def advanced_recommendations(graph, user):
    """Erweiterte Empfehlungen mit mehreren Faktoren"""
    friends = set(graph.get(user, []))
    scores = {}
    
    for friend in friends:
        for potential in graph.get(friend, []):
            if potential == user or potential in friends:
                continue
            
            if potential not in scores:
                scores[potential] = {
                    'mutual_friends': 0,
                    'total_friends': len(graph.get(potential, [])),
                    'distance': 2
                }
            
            scores[potential]['mutual_friends'] += 1
    
    # Kombiniere Faktoren
    for potential, data in scores.items():
        # Score = Mutual Friends × 2 + (1 / Total Friends)
        # Bevorzuge Personen mit vielen gemeinsamen aber nicht zu vielen Gesamtfreunden
        data['score'] = (
            data['mutual_friends'] * 2 + 
            (1.0 / max(1, data['total_friends']))
        )
    
    return sorted(scores.items(), key=lambda x: x[1]['score'], reverse=True)
```

## Graph-Metriken

### Degree (Grad)

**Definition**: Anzahl der Verbindungen eines Knotens

```python
def node_degree(graph, node):
    """Berechnet den Grad eines Knotens"""
    return len(graph.get(node, []))

# Durchschnittlicher Grad
def average_degree(graph):
    total = sum(len(neighbors) for neighbors in graph.values())
    return total / len(graph) if graph else 0
```

**Interpretation**:
- Hoher Grad = Populärer Benutzer ("Hub")
- Niedriger Grad = Wenige Verbindungen

### Clustering-Koeffizient

**Definition**: Wie stark sind die Nachbarn eines Knotens untereinander verbunden?

```python
def clustering_coefficient(graph, node):
    """Berechnet Clustering-Koeffizient für einen Knoten"""
    neighbors = set(graph.get(node, []))
    if len(neighbors) < 2:
        return 0.0
    
    # Zähle Verbindungen zwischen Nachbarn
    connections = 0
    for n1 in neighbors:
        for n2 in neighbors:
            if n1 != n2 and n2 in graph.get(n1, []):
                connections += 1
    
    # connections ist doppelt gezählt (A→B und B→A)
    connections //= 2
    
    # Maximale mögliche Verbindungen
    max_connections = len(neighbors) * (len(neighbors) - 1) / 2
    
    return connections / max_connections if max_connections > 0 else 0.0
```

**Interpretation**:
- 1.0 = Alle Freunde sind auch untereinander befreundet (Clique)
- 0.0 = Keine Freunde sind untereinander befreundet

### Betweenness Centrality

**Definition**: Wie oft liegt ein Knoten auf kürzesten Pfaden zwischen anderen Knoten?

```python
def betweenness_centrality(graph, node):
    """Vereinfachte Betweenness Centrality"""
    # Zähle wie oft der Knoten auf kürzesten Pfaden liegt
    count = 0
    total_paths = 0
    
    nodes = list(graph.keys())
    for start in nodes:
        for end in nodes:
            if start != end and start != node and end != node:
                path = shortest_path(graph, start, end)
                if path:
                    total_paths += 1
                    if node in path:
                        count += 1
    
    return count / total_paths if total_paths > 0 else 0.0
```

**Interpretation**:
- Hohe Betweenness = "Brücke" zwischen Communities
- Wichtiger Knoten für Informationsfluss

### Dichte (Density)

**Definition**: Verhältnis tatsächlicher zu möglichen Kanten

```python
def graph_density(graph):
    """Berechnet Dichte des Graphen"""
    n = len(graph)
    if n < 2:
        return 0.0
    
    # Tatsächliche Kanten (jede Kante einmal zählen)
    actual_edges = sum(len(neighbors) for neighbors in graph.values()) // 2
    
    # Maximale mögliche Kanten in ungerichtetem Graph
    max_edges = n * (n - 1) / 2
    
    return actual_edges / max_edges if max_edges > 0 else 0.0
```

**Interpretation**:
- 1.0 = Vollständiger Graph (jeder mit jedem verbunden)
- 0.0 = Keine Verbindungen

## Community Detection

### Connected Components

**Problem**: Finde separate Gruppen ohne Verbindungen untereinander

```python
def find_connected_components(graph):
    """Findet zusammenhängende Komponenten"""
    visited = set()
    components = []
    
    for node in graph.keys():
        if node not in visited:
            # BFS um Komponente zu finden
            component = []
            queue = deque([node])
            
            while queue:
                current = queue.popleft()
                if current not in visited:
                    visited.add(current)
                    component.append(current)
                    
                    for neighbor in graph.get(current, []):
                        if neighbor not in visited:
                            queue.append(neighbor)
            
            components.append(component)
    
    return components
```

### Modularity-basierte Communities

**Konzept**: Finde Gruppen mit vielen internen und wenigen externen Verbindungen

```python
def simple_community_detection(graph, max_communities=3):
    """Vereinfachte Community Detection"""
    # Starte mit jedem Knoten als eigene Community
    communities = {node: {node} for node in graph.keys()}
    
    # Merge Communities basierend auf Verbindungen
    changed = True
    while changed and len(set(map(frozenset, communities.values()))) > max_communities:
        changed = False
        
        for node in graph.keys():
            # Finde Community mit meisten Verbindungen
            connections = {}
            for neighbor in graph.get(node, []):
                neighbor_comm = frozenset(communities[neighbor])
                connections[neighbor_comm] = connections.get(neighbor_comm, 0) + 1
            
            if connections:
                best_comm = max(connections.items(), key=lambda x: x[1])[0]
                if best_comm != frozenset(communities[node]):
                    # Merge Communities
                    new_comm = set(best_comm) | communities[node]
                    for member in new_comm:
                        communities[member] = new_comm
                    changed = True
    
    # Konvertiere zu eindeutiger Liste
    unique_communities = []
    seen = set()
    for comm in communities.values():
        comm_frozen = frozenset(comm)
        if comm_frozen not in seen:
            seen.add(comm_frozen)
            unique_communities.append(list(comm))
    
    return unique_communities
```

## Erweiterte Algorithmen

### Dijkstra's Algorithm (Gewichtete Graphen)

Für zukünftige Erweiterung mit gewichteten Freundschaften:

```python
import heapq

def dijkstra(weighted_graph, start, target):
    """
    Kürzester Pfad in gewichtetem Graph
    
    weighted_graph Format: {node: [(neighbor, weight), ...]}
    Beispiel: {'A': [('B', 5), ('C', 3)], 'B': [('A', 5), ('D', 2)]}
    """
    distances = {node: float('inf') for node in weighted_graph.keys()}
    distances[start] = 0
    previous = {}
    pq = [(0, start)]
    
    while pq:
        current_distance, current_node = heapq.heappop(pq)
        
        if current_node == target:
            break
        
        if current_distance > distances[current_node]:
            continue
        
        for neighbor, weight in weighted_graph.get(current_node, []):
            distance = current_distance + weight
            
            if distance < distances[neighbor]:
                distances[neighbor] = distance
                previous[neighbor] = current_node
                heapq.heappush(pq, (distance, neighbor))
    
    # Rekonstruiere Pfad
    path = []
    current = target
    while current in previous:
        path.append(current)
        current = previous[current]
    path.append(start)
    path.reverse()
    
    return path, distances[target]
```

### PageRank (Influence Scoring)

Bestimme einflussreiche Benutzer:

```python
def pagerank(graph, damping=0.85, iterations=100):
    """Berechnet PageRank für alle Knoten"""
    n = len(graph)
    ranks = {node: 1.0 / n for node in graph.keys()}
    
    for _ in range(iterations):
        new_ranks = {}
        
        for node in graph.keys():
            rank_sum = 0
            # Summe der Ranks von Knoten die auf diesen zeigen
            for other_node in graph.keys():
                if node in graph.get(other_node, []):
                    out_degree = len(graph[other_node])
                    rank_sum += ranks[other_node] / out_degree
            
            new_ranks[node] = (1 - damping) / n + damping * rank_sum
        
        ranks = new_ranks
    
    return ranks
```

## Performance-Überlegungen

### Komplexität der Algorithmen

| Algorithm | Zeit | Speicher | Anwendungsfall |
|-----------|------|----------|----------------|
| BFS | O(V + E) | O(V) | Kürzeste Pfade, Level-Order |
| DFS | O(V + E) | O(V) | Pfad-Existenz, Zyklen |
| Dijkstra | O((V + E) log V) | O(V) | Gewichtete Pfade |
| PageRank | O(k × E) | O(V) | Influence Scoring |

### Optimierungen

**1. Caching**:
```python
from functools import lru_cache

@lru_cache(maxsize=1000)
def shortest_path_cached(start, target):
    return shortest_path(graph, start, target)
```

**2. Bidirectionale Suche**:
```python
def bidirectional_search(graph, start, target):
    """Suche von beiden Enden gleichzeitig"""
    # Schneller für große Graphen
    # Komplexität: O(b^(d/2)) statt O(b^d)
    pass  # Implementation
```

**3. A* mit Heuristik**:
```python
def a_star(graph, start, target, heuristic):
    """A* für optimierte Pfadsuche"""
    # Nutzt Heuristik um Suche zu leiten
    pass  # Implementation
```

## Praktische Tipps

### Graph visualisieren
```python
import networkx as nx
import matplotlib.pyplot as plt

def visualize_graph(graph):
    G = nx.Graph()
    for node, neighbors in graph.items():
        for neighbor in neighbors:
            G.add_edge(node, neighbor)
    
    pos = nx.spring_layout(G)
    nx.draw(G, pos, with_labels=True, node_color='lightblue', 
            node_size=500, font_size=10)
    plt.show()
```

### Graph-Statistiken ausgeben
```python
def print_graph_stats(graph):
    print(f"Knoten: {len(graph)}")
    print(f"Kanten: {sum(len(n) for n in graph.values()) // 2}")
    print(f"Dichte: {graph_density(graph):.3f}")
    print(f"Ø Grad: {average_degree(graph):.2f}")
    
    # Finde wichtigste Knoten
    degrees = {n: node_degree(graph, n) for n in graph.keys()}
    top_nodes = sorted(degrees.items(), key=lambda x: x[1], reverse=True)[:5]
    print("\nTop 5 Knoten nach Grad:")
    for node, degree in top_nodes:
        print(f"  {node}: {degree} Verbindungen")
```

## Weiterführende Themen

- **Small World Networks**: Hoher Clustering, kurze Pfadlängen
- **Scale-Free Networks**: Power-Law Degree Distribution
- **Network Motifs**: Wiederkehrende Muster
- **Temporal Networks**: Zeitabhängige Graphen
- **Multilayer Networks**: Mehrere Beziehungstypen

## Literatur

- **Introduction to Graph Theory** - Douglas West
- **Networks** - Mark Newman
- **Graph Algorithms** - Shimon Even
- **NetworkX Documentation** - Python Graph Library

---

**Letzte Aktualisierung**: 2025-12-22
