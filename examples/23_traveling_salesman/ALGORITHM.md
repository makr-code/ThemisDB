# TSP Algorithmen - Detaillierte Erklärung

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

Dieses Dokument erklärt die im Beispiel implementierten Algorithmen zur Lösung des Traveling Salesman Problems.

## 📋 Inhaltsverzeichnis

- [Problem-Definition](#problem-definition)
- [Komplexitätsklassen](#komplexitätsklassen)
- [Algorithmen](#algorithmen)
  - [1. Brute Force](#1-brute-force-exakte-lösung)
  - [2. Nearest Neighbor](#2-nearest-neighbor-greedy)
  - [3. 2-Opt Heuristik](#3-2-opt-heuristik)
  - [4. Christofides Algorithmus](#4-christofides-algorithmus)
- [Vergleich](#algorithmen-vergleich)
- [Implementierungs-Details](#implementierungs-details)

---

## Problem-Definition

### Formale Definition

Gegeben:
- **G = (V, E)**: Vollständiger Graph mit n Städten
- **d: E → ℝ⁺**: Distanzfunktion (Gewichte der Kanten)

Gesucht:
- **Hamilton-Kreis H**: Weg, der jede Stadt genau einmal besucht
- **Minimiere**: Σ d(e) für alle e ∈ H

### Eigenschaften

**NP-schwer**: Kein polynomialer Algorithmus für exakte Lösung bekannt

**Vollständig**: In der Praxis gibt es meist Wege zwischen allen Städtepaaren

**Metrisch**: Oft gilt die Dreiecks-Ungleichung: d(a,c) ≤ d(a,b) + d(b,c)

---

## Komplexitätsklassen

### Exakte Algorithmen

| Anzahl Städte | Mögliche Routen | Berechenbar? |
|---------------|-----------------|--------------|
| 5 | 24 | ✅ Instant |
| 10 | 181.440 | ✅ < 1s |
| 15 | ~653 Milliarden | ⚠️ Minuten |
| 20 | ~60 Quintillionen | ❌ Jahre |
| 25 | ~310²⁴ | ❌ Unmöglich |

**Formel**: (n-1)! / 2 Routen (bei symmetrischem TSP)

### Approximationsalgorithmen

**Ziel**: Finde Lösung in polynomialer Zeit mit Qualitätsgarantie

**Beispiel Christofides**: Höchstens 1.5× der optimalen Länge

---

## Algorithmen

### 1. Brute Force (Exakte Lösung)

#### Prinzip

Teste **alle möglichen Routen** und wähle die kürzeste.

#### Pseudocode

```python
def brute_force_tsp(cities, distances):
    n = len(cities)
    best_route = None
    best_distance = float('inf')
    
    # Generiere alle Permutationen
    for perm in permutations(cities[1:]):  # Erste Stadt fixiert
        route = [cities[0]] + list(perm) + [cities[0]]
        
        # Berechne Distanz
        distance = sum(distances[route[i]][route[i+1]] 
                      for i in range(n))
        
        # Update bestes Ergebnis
        if distance < best_distance:
            best_distance = distance
            best_route = route
    
    return best_route, best_distance
```

#### Komplexität

- **Zeit**: O(n!)
- **Speicher**: O(n)

#### Optimierungen

**1. Branch and Bound**: Abbruch, wenn Partial-Tour schon länger als bestes Ergebnis

```python
def branch_and_bound(partial_tour, current_distance, best_distance):
    if current_distance >= best_distance:
        return  # Abbruch - keine Verbesserung möglich
    # ... weiter
```

**2. Dynamische Programmierung (Held-Karp)**: O(n² × 2ⁿ) statt O(n!)

```python
def held_karp(cities, distances):
    n = len(cities)
    # memo[subset][city] = minimale Distanz um subset zu besuchen, endend in city
    memo = {}
    
    # Base case: Nur Start
    for i in range(1, n):
        memo[(frozenset([0, i]), i)] = distances[0][i]
    
    # Iterativ größere Subsets
    for size in range(2, n):
        for subset in combinations(range(1, n), size):
            bits = frozenset([0] + list(subset))
            for k in subset:
                # Berechne min für dieses subset endend in k
                prev_bits = bits - {k}
                min_dist = float('inf')
                for m in prev_bits:
                    if m == 0: continue
                    dist = memo.get((prev_bits, m), float('inf'))
                    dist += distances[m][k]
                    min_dist = min(min_dist, dist)
                memo[(bits, k)] = min_dist
    
    # Finale Distanz: Zurück zum Start
    all_bits = frozenset(range(n))
    return min(memo[(all_bits, k)] + distances[k][0] 
               for k in range(1, n))
```

#### Wann verwenden?

- ✅ n ≤ 10 Städte
- ✅ Wenn exakte Lösung erforderlich
- ✅ Als Benchmark für Heuristiken

---

### 2. Nearest Neighbor (Greedy)

#### Prinzip

Starte bei einer Stadt und besuche immer die **nächste noch nicht besuchte** Stadt.

#### Pseudocode

```python
def nearest_neighbor(start_city, cities, distances):
    unvisited = set(cities) - {start_city}
    route = [start_city]
    current = start_city
    total_distance = 0
    
    while unvisited:
        # Finde nächste Stadt
        nearest = min(unvisited, key=lambda city: distances[current][city])
        
        # Besuche Stadt
        route.append(nearest)
        total_distance += distances[current][nearest]
        current = nearest
        unvisited.remove(nearest)
    
    # Zurück zum Start
    route.append(start_city)
    total_distance += distances[current][start_city]
    
    return route, total_distance
```

#### Komplexität

- **Zeit**: O(n²)
- **Speicher**: O(n)

#### Qualität

**Worst-Case**: Kann beliebig schlecht sein (kein Approximationsfaktor)

**Durchschnitt**: Oft 20-30% länger als optimal

**Beispiel**:
```
Städte: A, B, C, D
Distanzen:
  A-B: 1, A-C: 2, A-D: 100
  B-C: 100, B-D: 100
  C-D: 1

Nearest Neighbor von A:
A → B (1) → D (100) → C (1) → A (2) = 104

Optimal:
A → B (1) → C (100) → D (1) → A (100) = 202
Nein, falsch!

Optimal:
A → C (2) → D (1) → B (100) → A (1) = 104
Ähnlich!

Tatsächlich optimal:
A → B (1) → C (100) → D (1) → A (100) = 202
ODER
A → C (2) → B (100) → D (100) → A (100) = 302
ODER
A → C (2) → D (1) → B (100) → A (1) = 104 ✓
```

#### Verbesserungen

**Multi-Start**: Starte von verschiedenen Städten und wähle beste Route

```python
def multi_start_nearest_neighbor(cities, distances):
    best_route = None
    best_distance = float('inf')
    
    for start_city in cities:
        route, distance = nearest_neighbor(start_city, cities, distances)
        if distance < best_distance:
            best_distance = distance
            best_route = route
    
    return best_route, best_distance
```

#### Wann verwenden?

- ✅ Sehr große Probleme (100+ Städte)
- ✅ Wenn Geschwindigkeit wichtiger als Qualität
- ✅ Als Startlösung für andere Algorithmen

---

### 3. 2-Opt Heuristik

#### Prinzip

Verbessere eine gegebene Route iterativ durch **Kantentausch**.

**2-Opt Swap**: Entferne zwei Kanten, füge zwei neue ein (ohne Kreuzung)

#### Visualisierung

```
Vorher:                   Nachher:
A ──── B                  A      B
│      │                   \    /
│      │         →          \  /
│      │                     \/
C ──── D                     /\
                            /  \
                           C    D

Entfernt: A-C, B-D        Eingefügt: A-B, C-D
Wenn dist(A,B) + dist(C,D) < dist(A,C) + dist(B,D): Tausch!
```

#### Pseudocode

```python
def two_opt(route, distances):
    improved = True
    best_route = route[:]
    
    while improved:
        improved = False
        
        for i in range(1, len(route) - 2):
            for j in range(i + 1, len(route)):
                # Prüfe 2-opt swap
                if j - i == 1: 
                    continue  # Benachbarte Städte
                
                # Berechne Distanz-Änderung
                old_dist = (distances[route[i-1]][route[i]] +
                           distances[route[j-1]][route[j]])
                new_dist = (distances[route[i-1]][route[j-1]] +
                           distances[route[i]][route[j]])
                
                if new_dist < old_dist:
                    # Führe Swap durch (Reverse Segment)
                    best_route[i:j] = reversed(best_route[i:j])
                    improved = True
                    break
            
            if improved:
                break
    
    return best_route
```

#### Komplexität

- **Zeit**: O(n²) pro Iteration, oft wenige Iterationen
- **Speicher**: O(n)

#### Qualität

**Ergebnis**: Lokales Optimum (kann nicht mehr mit 2-opt verbessert werden)

**Praxis**: Oft sehr nah am globalen Optimum (meist < 5% Abweichung)

#### Varianten

**3-Opt**: Entferne 3 Kanten, teste 7 Rekombinationen

```python
def three_opt(route, distances):
    # 7 mögliche Rekombinationen für 3 Kanten
    # Komplexität: O(n³) pro Iteration
    pass
```

**k-Opt**: Verallgemeinerung auf k Kanten (selten verwendet wegen Komplexität)

#### Wann verwenden?

- ✅ Beste Balance zwischen Qualität und Geschwindigkeit
- ✅ Nach Nearest Neighbor zur Verbesserung
- ✅ Für 10-100 Städte optimal

---

### 4. Christofides Algorithmus

#### Prinzip

**Approximationsalgorithmus** mit Garantie: Höchstens **1.5× optimal**

**Schritte**:
1. Berechne Minimum Spanning Tree (MST)
2. Finde Knoten mit ungeradem Grad
3. Berechne Minimum Weight Perfect Matching für ungerade Knoten
4. Kombiniere zu Eulerian Graph
5. Finde Eulerian Tour
6. Konvertiere zu Hamilton Tour (überspringe bereits besuchte)

#### Pseudocode

```python
def christofides(cities, distances):
    # 1. Minimum Spanning Tree (Prim/Kruskal)
    mst = minimum_spanning_tree(cities, distances)
    
    # 2. Knoten mit ungeradem Grad
    odd_degree_vertices = [v for v in cities if degree(v, mst) % 2 == 1]
    
    # 3. Minimum Perfect Matching für ungerade Knoten
    matching = min_weight_perfect_matching(odd_degree_vertices, distances)
    
    # 4. Kombiniere MST und Matching
    multigraph = combine(mst, matching)
    
    # 5. Eulerian Tour (jede Kante einmal)
    euler_tour = find_eulerian_tour(multigraph, start=cities[0])
    
    # 6. Konvertiere zu Hamilton Tour
    hamilton_tour = []
    visited = set()
    for city in euler_tour:
        if city not in visited:
            hamilton_tour.append(city)
            visited.add(city)
    hamilton_tour.append(hamilton_tour[0])  # Zurück zum Start
    
    return hamilton_tour
```

#### Komplexität

- **Zeit**: O(n³) (durch Matching-Schritt)
- **Speicher**: O(n²)

#### Beweis der Approximation

**Theorem**: Christofides liefert höchstens 1.5× optimale Lösung

**Beweisskizze**:
- MST ≤ OPT (da OPT ein Spanning Tree ist)
- Matching ≤ 0.5× OPT (da Matching Teil einer Tour ist)
- Total: MST + Matching ≤ 1.5× OPT

#### Wann verwenden?

- ✅ Wenn Qualitätsgarantie wichtig ist
- ✅ Für mittlere Probleme (20-50 Städte)
- ⚠️ Komplexer zu implementieren

---

## Algorithmen-Vergleich

### Zusammenfassung

| Algorithmus | Komplexität | Qualität | Garantie | Geeignet für |
|-------------|-------------|----------|----------|--------------|
| **Brute Force** | O(n!) | Optimal | 1.0× | n ≤ 10 |
| **Held-Karp (DP)** | O(n² 2ⁿ) | Optimal | 1.0× | n ≤ 20 |
| **Nearest Neighbor** | O(n²) | ~1.25× | Keine | n > 50 |
| **2-Opt** | O(n²) × k | ~1.05× | Keine | Alle |
| **3-Opt** | O(n³) × k | ~1.02× | Keine | n < 100 |
| **Christofides** | O(n³) | ≤ 1.5× | 1.5× | n < 100 |
| **Lin-Kernighan** | ~O(n².²) | ~1.01× | Keine | Alle |

### Empfehlungen

**Kleine Probleme (n ≤ 10)**:
- Verwenden Sie Brute Force für exakte Lösung

**Mittlere Probleme (10 < n ≤ 50)**:
- 2-Opt mit Multi-Start Nearest Neighbor
- Oder Christofides für Garantie

**Große Probleme (n > 50)**:
- Nearest Neighbor + 2-Opt
- Lin-Kernighan (fortgeschritten)

**Sehr große Probleme (n > 1000)**:
- Spezielle Heuristiken (Genetic Algorithms, Simulated Annealing)
- Divide-and-Conquer Ansätze

---

## Implementierungs-Details

### Distanzberechnung

**Euklidische Distanz** (Standard):
```python
def euclidean_distance(city1, city2):
    dx = city1.x - city2.x
    dy = city1.y - city2.y
    return math.sqrt(dx * dx + dy * dy)
```

**Manhattan-Distanz** (Gitter-Layout):
```python
def manhattan_distance(city1, city2):
    return abs(city1.x - city2.x) + abs(city1.y - city2.y)
```

**Haversine-Distanz** (GPS-Koordinaten):
```python
def haversine_distance(lat1, lon1, lat2, lon2):
    R = 6371  # Erdradius in km
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = (math.sin(dlat/2)**2 + 
         math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) *
         math.sin(dlon/2)**2)
    c = 2 * math.asin(math.sqrt(a))
    return R * c
```

### Performance-Tipps

**1. Distanzmatrix cachen**:
```python
def build_distance_matrix(cities):
    n = len(cities)
    matrix = [[0] * n for _ in range(n)]
    
    for i in range(n):
        for j in range(i + 1, n):
            dist = euclidean_distance(cities[i], cities[j])
            matrix[i][j] = matrix[j][i] = dist
    
    return matrix
```

**2. Symmetrie ausnutzen**: Bei symmetrischem TSP nur Hälfte berechnen

**3. Früher Abbruch**: Bei Branch and Bound

**4. Parallelisierung**: Multi-Start Nearest Neighbor parallel ausführen

### Testing

**Kleine Instanzen**: Mit Brute Force verifizieren

```python
def test_algorithm(algorithm, cities, distances):
    # Berechne mit Algorithmus
    route, dist = algorithm(cities, distances)
    
    # Vergleiche mit Brute Force (wenn n klein)
    if len(cities) <= 10:
        optimal_route, optimal_dist = brute_force(cities, distances)
        ratio = dist / optimal_dist
        print(f"Approximation ratio: {ratio:.3f}")
    
    return route, dist
```

**Bekannte Instanzen**: TSPLIB (http://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/)

---

## Literatur & Ressourcen

### Bücher

- "The Traveling Salesman Problem: A Computational Study" - Applegate, Bixby, Chvátal, Cook
- "In Pursuit of the Traveling Salesman" - William Cook
- "Introduction to Algorithms" - Cormen et al. (Kapitel 15, 35)

### Papers

- Christofides (1976): "Worst-Case Analysis of a New Heuristic for the TSP"
- Lin & Kernighan (1973): "An Effective Heuristic for the TSP"
- Held & Karp (1962): "A Dynamic Programming Approach to Sequencing Problems"

### Tools & Benchmarks

- **Concorde TSP Solver**: http://www.math.uwaterloo.ca/tsp/concorde.html
- **TSPLIB**: Benchmark-Instanzen
- **LKH Solver**: Implementierung von Lin-Kernighan-Helsgaun

---

**Letzte Aktualisierung**: 2026-01-14
