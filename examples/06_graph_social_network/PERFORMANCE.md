> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt.
> Für reproduzierbare Ergebnisse: CMake-Presets und aktuellen Teststand verwenden.

# Graph Soziales Netzwerk - Performance-Optimierung

## Übersicht

Dieser Guide beschreibt Performance-Optimierungen für Graph-Operationen und soziale Netzwerk-Anwendungen.

## Graph-Performance-Grundlagen

### Big-O Komplexität verstehen

**Graph-Größen**:
- **Kleine Graphen**: < 1.000 Knoten
- **Mittlere Graphen**: 1.000 - 100.000 Knoten
- **Große Graphen**: > 100.000 Knoten

**Typische Operationen**:
```python
# O(1) - Konstant
user = graph[user_id]

# O(V) - Linear mit Knoten
all_users = list(graph.keys())

# O(V + E) - Linear mit Knoten + Kanten
bfs_result = bfs(graph, start)

# O(V²) - Quadratisch
for u in graph:
    for v in graph:
        check_connection(u, v)
```

## Datenstruktur-Optimierungen

### 1. Effiziente Graph-Repräsentation

**Adjazenzliste (Standard)**:
```python
# Gut für sparse graphs (wenige Verbindungen)
graph = {
    "user1": {"user2", "user3"},  # Set für O(1) Lookup
    "user2": {"user1"},
    "user3": {"user1"}
}
```

**Vorteile**:
- Speicher: O(V + E)
- Nachbar-Lookup: O(1) mit Sets
- Iteration über Nachbarn: O(degree)

**Adjazenzmatrix** (für dichte Graphen):
```python
# Nur sinnvoll wenn E ≈ V²
import numpy as np

adjacency_matrix = np.zeros((n_users, n_users), dtype=bool)
adjacency_matrix[user1_id][user2_id] = True
```

**Vorteile**:
- Verbindungs-Check: O(1)
- Gut für Matrix-Operationen

**Nachteile**:
- Speicher: O(V²)
- Ungeeignet für sparse graphs

### 2. Index-Strukturen

**User-ID zu Array-Index Mapping**:
```python
class GraphIndex:
    def __init__(self):
        self.user_to_idx = {}  # "user_id" → int
        self.idx_to_user = []  # int → "user_id"
    
    def add_user(self, user_id):
        if user_id not in self.user_to_idx:
            idx = len(self.idx_to_user)
            self.user_to_idx[user_id] = idx
            self.idx_to_user.append(user_id)
        return self.user_to_idx[user_id]
    
    def get_idx(self, user_id):
        return self.user_to_idx.get(user_id)
    
    def get_user(self, idx):
        return self.idx_to_user[idx] if 0 <= idx < len(self.idx_to_user) else None
```

**Nutzen**:
- Arrays statt Dictionaries wo möglich
- Schnellerer Speicherzugriff
- Weniger Memory-Overhead

### 3. Set vs List für Nachbarn

```python
# Schlecht: O(n) für Membership-Test
neighbors_list = ["user1", "user2", "user3"]
if "user2" in neighbors_list:  # O(n)
    pass

# Gut: O(1) für Membership-Test
neighbors_set = {"user1", "user2", "user3"}
if "user2" in neighbors_set:  # O(1)
    pass
```

## Algorithmus-Optimierungen

### 1. BFS-Optimierung

**Standard-Implementation**:
```python
def bfs(graph, start):
    visited = set()
    queue = deque([start])
    
    while queue:
        node = queue.popleft()
        if node in visited:
            continue
        visited.add(node)
        
        for neighbor in graph[node]:
            if neighbor not in visited:
                queue.append(neighbor)
```

**Optimierte Version**:
```python
def bfs_optimized(graph, start, target=None):
    """Optimiert mit Early Stopping und vorallokiertem Set"""
    visited = set([start])  # Start direkt als visited
    queue = deque([start])
    
    while queue:
        node = queue.popleft()
        
        # Early stopping wenn Ziel gefunden
        if target and node == target:
            return True
        
        for neighbor in graph[node]:
            if neighbor not in visited:
                visited.add(neighbor)  # Sofort markieren
                queue.append(neighbor)
    
    return False if target else visited
```

**Verbesserungen**:
- Start-Knoten direkt als visited markieren
- Early stopping bei Ziel-Suche
- Keine redundanten Checks

### 2. Bidirektionale Suche

**Für Shortest Path zwischen zwei Knoten**:
```python
def bidirectional_search(graph, start, target):
    """Sucht von beiden Enden gleichzeitig"""
    if start == target:
        return [start]
    
    # Zwei parallele BFS
    forward_visited = {start: None}
    backward_visited = {target: None}
    forward_queue = deque([start])
    backward_queue = deque([target])
    
    while forward_queue and backward_queue:
        # Forward step
        node = forward_queue.popleft()
        for neighbor in graph[node]:
            if neighbor in backward_visited:
                # Pfade treffen sich!
                return reconstruct_path(forward_visited, backward_visited, 
                                       neighbor, start, target)
            if neighbor not in forward_visited:
                forward_visited[neighbor] = node
                forward_queue.append(neighbor)
        
        # Backward step
        node = backward_queue.popleft()
        for neighbor in graph[node]:
            if neighbor in forward_visited:
                return reconstruct_path(forward_visited, backward_visited,
                                       neighbor, start, target)
            if neighbor not in backward_visited:
                backward_visited[neighbor] = node
                backward_queue.append(neighbor)
    
    return None  # Kein Pfad

def reconstruct_path(forward, backward, meeting_point, start, target):
    """Rekonstruiert Pfad von beiden Seiten"""
    # Forward path: start → meeting_point
    path = []
    node = meeting_point
    while node is not None:
        path.append(node)
        node = forward[node]
    path.reverse()
    
    # Backward path: meeting_point → target
    node = backward[meeting_point]
    while node is not None:
        path.append(node)
        node = backward[node]
    
    return path
```

**Speedup**: O(b^(d/2)) statt O(b^d) bei Branching Factor b und Tiefe d

### 3. Batch-Operationen

**Schlecht - Einzelne Queries**:
```python
for user in users:
    friends = get_friends(user)  # N Datenbankabfragen
    process(friends)
```

**Gut - Batch Loading**:
```python
# Eine Query für alle User
all_friendships = get_friendships_batch(users)
for user in users:
    friends = all_friendships[user]
    process(friends)
```

**ThemisDB Batch Query**:
```python
def get_friends_batch(user_ids):
    """Lädt Freunde für mehrere User gleichzeitig"""
    query = """
    MATCH (u:User)-[:FRIEND]-(f:User)
    WHERE u.id IN $user_ids
    RETURN u.id, COLLECT(f.id) as friends
    """
    return client.execute(query, user_ids=user_ids)
```

## Caching-Strategien

### 1. LRU Cache für Queries

```python
from functools import lru_cache

class SocialNetworkClient:
    @lru_cache(maxsize=1000)
    def get_user_friends(self, user_id):
        """Cached Freunde-Abfrage"""
        return self._fetch_friends(user_id)
    
    def add_friendship(self, user1, user2):
        """Invalidiert Cache bei Änderungen"""
        self._add_friendship(user1, user2)
        # Cache invalidieren
        self.get_user_friends.cache_clear()
```

**Selective Cache Invalidation**:
```python
class SmartCache:
    def __init__(self):
        self.cache = {}
        self.dependencies = {}  # Welche Cache-Keys von welchen Daten abhängen
    
    def get(self, key):
        return self.cache.get(key)
    
    def set(self, key, value, depends_on=None):
        self.cache[key] = value
        if depends_on:
            for dep in depends_on:
                if dep not in self.dependencies:
                    self.dependencies[dep] = set()
                self.dependencies[dep].add(key)
    
    def invalidate(self, data_key):
        """Invalidiert nur betroffene Cache-Einträge"""
        if data_key in self.dependencies:
            for cache_key in self.dependencies[data_key]:
                self.cache.pop(cache_key, None)
            del self.dependencies[data_key]
```

### 2. Materialized Views

**Häufig berechnete Werte vorberechnen**:
```python
class MaterializedGraph:
    def __init__(self, graph):
        self.graph = graph
        self._precompute()
    
    def _precompute(self):
        """Berechnet teure Metriken im Voraus"""
        self.degrees = {
            node: len(neighbors) 
            for node, neighbors in self.graph.items()
        }
        
        self.clustering = {
            node: self._clustering_coefficient(node)
            for node in self.graph.keys()
        }
        
        # PageRank vorberechnen
        self.pagerank = self._compute_pagerank()
    
    def get_degree(self, node):
        return self.degrees.get(node, 0)
    
    def update_friendship(self, user1, user2):
        """Update nur betroffene Metriken"""
        # Inkrementelles Update statt vollständiger Neuberechnung
        self.degrees[user1] += 1
        self.degrees[user2] += 1
        # Clustering nur für betroffene Knoten neu berechnen
        self.clustering[user1] = self._clustering_coefficient(user1)
        self.clustering[user2] = self._clustering_coefficient(user2)
```

### 3. In-Memory Graph

**Für häufige Zugriffe**:
```python
from datetime import datetime, timedelta

class InMemoryGraph:
    def __init__(self, client):
        self.client = client
        self.graph = {}
        self.last_sync = None
        self.sync_interval = timedelta(minutes=5)
    
    def ensure_synced(self):
        """Sync mit DB wenn nötig"""
        now = datetime.now()
        if not self.last_sync or (now - self.last_sync) > self.sync_interval:
            self.sync_from_db()
    
    def sync_from_db(self):
        """Lädt kompletten Graph in Memory"""
        self.graph = self.client.load_full_graph()
        self.last_sync = datetime.now()
    
    def get_friends(self, user_id):
        """O(1) Lookup aus Memory"""
        self.ensure_synced()
        return self.graph.get(user_id, set())
```

## Database-Optimierung

### 1. Indizes

**ThemisDB Graph Indizes**:
```sql
-- Index auf User-IDs für schnelle Lookups
CREATE INDEX idx_user_id ON users(id);

-- Index auf Freundschafts-Kanten
CREATE INDEX idx_friendship_from ON friendships(from_user_id);
CREATE INDEX idx_friendship_to ON friendships(to_user_id);

-- Composite Index für bidirektionale Suche
CREATE INDEX idx_friendship_both ON friendships(from_user_id, to_user_id);
```

### 2. Query-Optimierung

**Schlecht - N+1 Problem**:
```python
users = get_all_users()
for user in users:
    friends = get_friends(user.id)  # N Queries!
    for friend in friends:
        print(friend.name)
```

**Gut - JOIN/COLLECT**:
```python
query = """
MATCH (u:User)-[:FRIEND]-(f:User)
RETURN u.id, u.name, COLLECT({id: f.id, name: f.name}) as friends
"""
results = client.execute(query)
```

### 3. Pagination

**Für große Result-Sets**:
```python
def get_friends_paginated(user_id, page=1, per_page=50):
    """Paginierte Freunde-Liste"""
    offset = (page - 1) * per_page
    
    query = """
    MATCH (u:User {id: $user_id})-[:FRIEND]-(f:User)
    RETURN f
    ORDER BY f.name
    SKIP $offset
    LIMIT $limit
    """
    
    return client.execute(query, 
                         user_id=user_id,
                         offset=offset,
                         limit=per_page)
```

## Parallelisierung

### 1. Thread-Pool für unabhängige Operationen

```python
from concurrent.futures import ThreadPoolExecutor

def compute_recommendations_parallel(user_ids, max_workers=4):
    """Berechnet Empfehlungen parallel"""
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(compute_recommendations, uid): uid 
            for uid in user_ids
        }
        
        results = {}
        for future in as_completed(futures):
            user_id = futures[future]
            results[user_id] = future.result()
        
        return results
```

### 2. Async I/O

```python
import asyncio

async def fetch_user_data_async(user_id):
    """Asynchroner Datenabruf"""
    loop = asyncio.get_event_loop()
    return await loop.run_in_executor(None, fetch_user_data, user_id)

async def fetch_multiple_users(user_ids):
    """Parallel mehrere User laden"""
    tasks = [fetch_user_data_async(uid) for uid in user_ids]
    return await asyncio.gather(*tasks)

# Verwendung
user_ids = ["user1", "user2", "user3"]
results = asyncio.run(fetch_multiple_users(user_ids))
```

## Memory-Optimierung

### 1. Generator statt Listen

```python
# Schlecht - Lädt alles in Memory
def get_all_friendships():
    friendships = []
    for user in all_users:
        for friend in user.friends:
            friendships.append((user, friend))
    return friendships  # Kann sehr groß werden!

# Gut - Lazy Evaluation
def get_all_friendships_generator():
    for user in all_users:
        for friend in user.friends:
            yield (user, friend)

# Verwendung
for user, friend in get_all_friendships_generator():
    process(user, friend)  # Nur ein Paar zur Zeit in Memory
```

### 2. Iterative Tiefensuche

**Vermeidet Stack Overflow bei tiefen Graphen**:
```python
def dfs_iterative(graph, start):
    """Iterative DFS - kein Recursion-Limit"""
    visited = set()
    stack = [start]
    
    while stack:
        node = stack.pop()
        if node not in visited:
            visited.add(node)
            # Nachbarn in umgekehrter Reihenfolge für gleiche Traversierung
            stack.extend(reversed(list(graph[node])))
    
    return visited
```

## Monitoring und Profiling

### 1. Performance-Metriken tracken

```python
import time
from functools import wraps

def timed(func):
    """Decorator zum Messen der Ausführungszeit"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        duration = time.time() - start
        
        # Log zu Monitoring-System
        metrics.record(f"{func.__name__}_duration", duration)
        
        if duration > 1.0:  # Warnung bei > 1 Sekunde
            logger.warning(f"{func.__name__} took {duration:.2f}s")
        
        return result
    return wrapper

@timed
def compute_recommendations(user_id):
    # Implementation
    pass
```

### 2. Memory Profiling

```python
import tracemalloc

def profile_memory(func):
    """Profilt Memory-Verwendung"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        tracemalloc.start()
        
        result = func(*args, **kwargs)
        
        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        
        logger.info(f"{func.__name__} - Current: {current / 1024**2:.1f} MB, "
                   f"Peak: {peak / 1024**2:.1f} MB")
        
        return result
    return wrapper
```

### 3. Query-Performance tracken

```python
class InstrumentedClient:
    def __init__(self, client):
        self.client = client
        self.query_stats = {}
    
    def execute(self, query, **params):
        start = time.time()
        result = self.client.execute(query, **params)
        duration = time.time() - start
        
        # Tracke Query-Performance
        query_hash = hash(query)
        if query_hash not in self.query_stats:
            self.query_stats[query_hash] = {
                'query': query,
                'count': 0,
                'total_time': 0,
                'max_time': 0
            }
        
        stats = self.query_stats[query_hash]
        stats['count'] += 1
        stats['total_time'] += duration
        stats['max_time'] = max(stats['max_time'], duration)
        
        return result
    
    def print_slow_queries(self, threshold=1.0):
        """Zeigt langsame Queries"""
        for stats in self.query_stats.values():
            avg_time = stats['total_time'] / stats['count']
            if avg_time > threshold:
                print(f"Slow query (avg {avg_time:.2f}s):")
                print(f"  {stats['query'][:100]}...")
                print(f"  Count: {stats['count']}, Max: {stats['max_time']:.2f}s")
```

## Best Practices

### ✅ DO

1. **Index strategisch setzen** - Für häufige Queries
2. **Batch-Operations nutzen** - N+1 vermeiden
3. **Cachen** - Für teure Berechnungen
4. **Paginieren** - Große Result-Sets aufteilen
5. **Profile regelmäßig** - Bottlenecks identifizieren
6. **Sets für Membership-Tests** - O(1) statt O(n)
7. **Generator für große Datenmengen** - Memory sparen
8. **Bidirektionale Suche** - Für Shortest Path

### ❌ DON'T

1. **N+1 Queries** - Batch stattdessen
2. **Vollständiger Graph-Scan** - Index nutzen
3. **Unbegrenzte Result-Sets** - Immer limitieren
4. **Recursive DFS bei tiefen Graphen** - Stack Overflow
5. **Alle Daten in Memory** - Stream/Generator
6. **Keine Monitoring** - Performance blind
7. **Vorzeitige Optimierung** - Erst messen!
8. **Cache nie invalidieren** - Stale Data

## Performance-Checkliste

### Development
- [ ] Queries mit EXPLAIN analysieren
- [ ] Indizes für alle JOIN-Felder
- [ ] Pagination implementiert
- [ ] Caching-Strategie definiert
- [ ] Memory-Profile erstellt

### Testing
- [ ] Load-Tests mit realistischen Daten
- [ ] Slow-Query-Log aktiviert
- [ ] Memory-Leaks geprüft
- [ ] Concurrent-Access getestet

### Production
- [ ] Monitoring-Dashboard
- [ ] Alerting für langsame Queries
- [ ] Auto-Scaling konfiguriert
- [ ] Backup-Strategie
- [ ] Cache-Warming bei Deployment

---

**Letzte Aktualisierung**: 2025-12-22
