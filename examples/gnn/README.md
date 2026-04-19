> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# GNN Embeddings Example

Dieses Beispiel demonstriert die Verwendung von Graph Neural Network (GNN) Embeddings in ThemisDB.

## Übersicht

Das Beispiel zeigt:

1. **Erstellung eines Graphen**: Einfaches Social Network mit Benutzern und Beziehungen
2. **Modell-Registrierung**: Registrierung eines GraphSAGE-Modells
3. **Embedding-Generierung**: Generierung von Node-Embeddings für alle Benutzer
4. **Similarity Search**: Finden ähnlicher Benutzer basierend auf Embeddings
5. **Inkrementelle Updates**: Hinzufügen neuer Benutzer und Update der Embeddings
6. **Statistiken**: Anzeige von Embedding-Statistiken

## Kompilieren

```bash
# Mit CMake
mkdir build && cd build
cmake ..
make gnn_embeddings_example

# Ausführen
./gnn_embeddings_example
```

## Verwendung

Das Beispiel erstellt eine einfache Social-Network-Datenbank und demonstriert verschiedene GNN-Operationen:

### 1. Graph-Erstellung

```cpp
PropertyGraphManager pgm(db);
VectorIndexManager vim(db);
GNNEmbeddingManager gnn(db, pgm, vim);

// Benutzer hinzufügen
pgm.addNode("alice", "Person", "social_network");
// ... weitere Benutzer

// Beziehungen hinzufügen
pgm.addEdge("alice", "bob", "FOLLOWS", "social_network");
```

### 2. GNN-Modell registrieren

```cpp
gnn.registerModel(
    "social_graphsage",  // Modell-Name
    "graphsage",         // Modell-Typ
    128,                 // Embedding-Dimension
    "{}"                 // Konfiguration
);
```

### 3. Embeddings generieren

```cpp
// Alle Person-Nodes
gnn.generateNodeEmbeddings("social_network", "Person", "social_graphsage");

// Einzelner Node
gnn.updateNodeEmbedding("alice", "social_network", "social_graphsage");
```

### 4. Similarity Search

```cpp
auto [st, similar] = gnn.findSimilarNodes(
    "alice",            // Query-Node
    "social_network",   // Graph
    3,                  // Top-K
    "social_graphsage"  // Modell
);

for (const auto& result : similar) {
    std::cout << result.entity_id << " (sim: " << result.similarity << ")" << std::endl;
}
```

## Erwartete Ausgabe

```
=====================================
GNN Embeddings Example for ThemisDB
=====================================

[1] Creating sample social network graph...
  ✓ Added user: Alice
  ✓ Added user: Bob
  ✓ Added user: Charlie
  ✓ Added user: David
  ✓ Created 8 relationships
  Graph: 4 nodes, 8 edges

[2] Registering GNN model...
  ✓ Registered model: social_graphsage (GraphSAGE, 128-dim)

[3] Generating node embeddings...
  ✓ Generated embeddings for all Person nodes
  ✓ Alice's embedding: 128-dimensional vector
    First 5 values: [0.123, 0.456, 0.789, 0.234, 0.567, ...]

[4] Finding similar users (friend recommendations)...
  Users most similar to Alice:
    - charlie (similarity: 0.92)
    - bob (similarity: 0.87)
    - david (similarity: 0.75)

  Users most similar to Bob:
    - alice (similarity: 0.87)
    - david (similarity: 0.83)
    - charlie (similarity: 0.71)

[5] Demonstrating incremental embedding update...
  ✓ Added new user: Eve
  ✓ Created relationships for Eve
  ✓ Generated embedding for Eve

  Users most similar to Eve:
    - alice (similarity: 0.94)
    - charlie (similarity: 0.91)
    - bob (similarity: 0.78)

  → Recommendation: Eve should connect with these users!

[6] Embedding statistics...
  Total node embeddings: 5
  Total edge embeddings: 0

  Embeddings per model:
    social_graphsage: 5

  Embeddings per graph:
    social_network: 5

=====================================
Example complete!
=====================================
```

## Funktionsweise

### MVP-Implementation

Die aktuelle MVP-Implementation verwendet einfache feature-basierte Embeddings:

1. **Feature-Extraktion**: Node-Attribute werden in numerische Vektoren umgewandelt
2. **Normalisierung**: Features werden normalisiert
3. **Speicherung**: Embeddings werden in RocksDB und Vector Index gespeichert
4. **Similarity Search**: Cosine-Similarity über Vector Index (HNSW)

### Production-Integration (geplant)

Für die Production-Version mit echten GNN-Modellen:

1. **Training**: Python-Skript trainiert GraphSAGE/GCN/GAT-Modell
2. **Export**: Modell wird zu ONNX exportiert
3. **Inference**: C++ ONNX Runtime lädt Modell und generiert Embeddings
4. **Caching**: Embeddings werden in RocksDB gecacht

```cpp
// Zukünftige ONNX-Integration
GnnInference inference("models/social_graphsage.onnx");
auto embeddings = inference.generateEmbeddings(subgraph);
```

## Use Cases

### 1. Friend Recommendations

```cpp
// Finde ähnliche Benutzer für Freundschaftsvorschläge
auto [st, similar] = gnn.findSimilarNodes("alice", graph_id, 10, model);
```

### 2. Content Recommendations

```cpp
// Finde Benutzer mit ähnlichen Interessen
gnn.generateNodeEmbeddings(graph_id, "User", "interest_model");
auto [st, similar] = gnn.findSimilarNodes(user_id, graph_id, 20, "interest_model");
```

### 3. Fraud Detection

```cpp
// Finde verdächtige Konten mit ähnlichen Mustern
gnn.generateNodeEmbeddings(graph_id, "Account", "fraud_detector");
auto [st, suspicious] = gnn.findSimilarNodes(flagged_account, graph_id, 50, "fraud_detector");
```

### 4. Knowledge Graph Reasoning

```cpp
// Finde ähnliche Entitäten in Knowledge Graph
gnn.generateNodeEmbeddings(graph_id, "Entity", "kg_model");
auto [st, related] = gnn.findSimilarNodes(entity_id, graph_id, 15, "kg_model");
```

## Performance

Erwartete Performance (basierend auf Literatur):

- **Embedding-Generierung**: ~1000 nodes/sec (CPU), ~10,000 nodes/sec (GPU)
- **Similarity Search**: <5ms für Top-10 in 1M nodes (HNSW)
- **Inkrementelle Updates**: <10ms pro Node

## Integration mit Python-Tools

```bash
# 1. Daten exportieren
python tools/gnn/export_graph_data.py --graph social_network --output data/social.parquet

# 2. Modell trainieren
python tools/gnn/train_gnn.py --input data/social.parquet --model graphsage --output models/social.pth

# 3. Zu ONNX exportieren
python tools/gnn/export_to_onnx.py --model models/social.pth --output models/social.onnx

# 4. In ThemisDB verwenden
./gnn_embeddings_example
```

## Siehe auch

- [GNN Research Document](../../docs/research/GNN_BASED_INDEXING_AND_EMBEDDINGS.md)
- [GNN Tools README](../../tools/gnn/README.md)
- [Vector Index Documentation](../../docs/features/VECTOR_SEARCH.md)
- [Property Graph Documentation](../../docs/features/PROPERTY_GRAPH.md)

## Lizenz

Siehe [LICENSE](../../LICENSE) im Repository-Root.
