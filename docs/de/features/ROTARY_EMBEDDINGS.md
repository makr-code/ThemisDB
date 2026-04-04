# Rotary Position Embeddings (RoPE) in ThemisDB

## Übersicht

ThemisDB integriert **Rotary Position Embeddings (RoPE)** aus Transformer-Inferenzmodellen in seine Vektorspeicher-Schicht und ermöglicht damit erweiterte positionelle Kodierungsfähigkeiten für sequentielle Entitäten, Knowledge-Graph-Operationen und die Verarbeitung temporaler Daten.

## Wissenschaftliche Grundlage

RoPE wurde in der Arbeit "RoFormer: Enhanced Transformer with Rotary Position Embedding" von Su, J., et al. (2021), arXiv:2104.09864 eingeführt. Die zentrale Innovation besteht darin, Rotationsmatrizen auf Embedding-Koordinaten anzuwenden, um Positionsinformationen zu kodieren:

### Mathematische Formulierung

Für eine versteckte Dimension `d` berechnet RoPE Rotationsfrequenzen:

```
θᵢ = base^(-2i/d)  wobei i ∈ [0, d/2)
```

Für jedes Koordinatenpaar `(x₀, x₁)` an Position `m` wird eine 2D-Rotation angewendet:

```
[x'₀]   [cos(mθ)  -sin(mθ)] [x₀]
[x'₁] = [sin(mθ)   cos(mθ)] [x₁]
```

Die vollständige Transformation für Position `m` ist:

```
f(xₘ) = R(xₘ, mθ₀) ⊕ R(xₘ, mθ₁) ⊕ ... ⊕ R(xₘ, mθ_{d/2-1})
```

Dieser Ansatz bewahrt:
- **Relative Positionsinformationen** zwischen Tokens/Entitäten
- **Vektormagnitude** (Rotation ist orthogonal)
- **Rechnerische Effizienz** durch vorberechneten Theta-Cache

## Architektur in ThemisDB

### Kernkomponenten

```
include/index/rotary_embeddings.h          # RoPE-Implementierung Header
src/index/rotary_embeddings.cpp            # Implementierung
tests/test_rotary_embeddings.cpp           # Unit-Tests (gtest)
benchmarks/bench_rotary_embeddings.cpp     # Performance-Benchmarks
```

### Integrationspunkte

1. **BaseEntity**: Erweitert zur Speicherung von Rotations-Metadaten
   - `embedding_rotation_pos`: Für Rotation verwendete Position
   - `embedding_rotation_type`: Relationstyp (für relationale Rotation)

2. **VectorIndexManager**: Zentraler Integrationspunkt
   - Rotationsbewusste Entity-Hinzufügung
   - Rotationsbewusste Suchoperationen
   - Relationale Rotation für Knowledge Graphs

## Anwendungsfälle

### 1. Sequentielle Dokumentenverarbeitung

Ideal für Dokumente mit inhärenter Reihenfolge (z.B. Kapitel, Seiten, Zeitreihen):

```cpp
// Rotationskonfiguration initialisieren
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.base_theta = 10000.0;
config.computeThetaCache();

vector_mgr->setRotaryEmbeddingConfig(config);

// Dokumente mit positioneller Kodierung hinzufügen
for (size_t i = 0; i < documents.size(); ++i) {
    BaseEntity doc(documents[i].id);
    doc.setField("embedding", documents[i].embedding);
    doc.setField("content", documents[i].text);
    
    // Position i kodiert sequentielle Reihenfolge
    vector_mgr->addEntityWithRotation(doc, "embedding", i);
}

// Suche mit Positionsbewusstsein
std::vector<float> query = getQueryEmbedding("Finde Dokument nahe Position 100");
auto [status, results] = vector_mgr->searchWithRotation(query, 10, 100);
```

### 2. Knowledge-Graph-Relationen

Ermöglicht TransE-ähnliche relationale Embeddings im Vektorraum:

```cpp
// Entity mit "parent_of" Relation
BaseEntity parent("entity_A");
parent.setField("embedding", embedding_A);

vector_mgr->addEntityWithRelationalRotation(
    parent, "embedding", "parent_of"
);

// Entity mit "child_of" Relation
BaseEntity child("entity_B");
child.setField("embedding", embedding_B);

vector_mgr->addEntityWithRelationalRotation(
    child, "embedding", "child_of"
);

// Verschiedene Relationstypen erhalten einzigartige Rotationen
// Ermöglicht Abfragen wie: "Finde Entitäten mit 'sibling_of' Relation"
```

### 3. Temporale Datenkodierung

Einbettung von Zeitstempeln als Rotationspositionen:

```cpp
// Zeitstempel in Position umwandeln
size_t position = timestamp / time_quantum;

BaseEntity event("event_" + std::to_string(event_id));
event.setField("embedding", event_embedding);
event.setField("timestamp", timestamp);

vector_mgr->addEntityWithRotation(event, "embedding", position);

// Suche nach Ereignissen in der Nähe einer bestimmten Zeit
size_t query_position = query_timestamp / time_quantum;
auto [status, results] = vector_mgr->searchWithRotation(
    query_embedding, 20, query_position
);
```

### 4. Multi-relationale Vektorsuche

Kombination von positioneller und relationaler Kodierung:

```cpp
// Knowledge Graph mit zeitlicher Ordnung
BaseEntity kg_node("node_X");
kg_node.setField("embedding", node_embedding);
kg_node.setField("created_at", creation_time);

// Zuerst temporale Rotation anwenden
size_t temporal_pos = creation_time / 3600; // stündliche Buckets
RotaryEmbedding rope(config);
auto temp_rotated = rope.rotate(node_embedding, temporal_pos);

// Dann relationale Rotation anwenden
auto final_embedding = rope.rotateRelational(temp_rotated, "belongs_to");

kg_node.setField("embedding", final_embedding);
vector_mgr->addEntity(kg_node, "embedding");
```

## API-Referenz

### RotationConfig

Konfiguration für Rotary Embeddings:

```cpp
struct RotationConfig {
    size_t hidden_dim;              // Embedding-Dimension (muss gerade sein)
    size_t num_rotation_pairs;      // Anzahl der 2D-Rotationspaare
    double base_theta = 10000.0;    // Basisfrequenz (aus RoPE-Paper)
    bool normalize_after = false;   // L2-Normalisierung nach Rotation
    
    std::vector<double> theta_cache; // Vorberechnete Theta-Werte
    
    void computeThetaCache();        // Theta-Cache berechnen
    bool isValid() const;            // Konfiguration validieren
};
```

**Beispiel:**

```cpp
RotationConfig config;
config.hidden_dim = 512;
config.num_rotation_pairs = 256;  // Muss <= hidden_dim / 2 sein
config.base_theta = 10000.0;      // Standardwert aus RoPE-Paper
config.normalize_after = false;    // Optionale L2-Normalisierung
config.computeThetaCache();        // Erforderlich vor Verwendung

if (!config.isValid()) {
    throw std::invalid_argument("Ungültige Konfiguration");
}
```

### RotaryEmbedding-Klasse

Kern-Rotationsoperationen:

```cpp
class RotaryEmbedding {
public:
    explicit RotaryEmbedding(const RotationConfig& config);
    
    // Kernoperationen
    std::vector<float> rotate(const std::vector<float>& embedding, size_t position) const;
    std::vector<float> rotateInverse(const std::vector<float>& embedding, size_t position) const;
    
    // Batch-Operationen
    std::vector<std::vector<float>> rotateBatch(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const;
    
    // Relationale Rotation (für KG)
    std::vector<float> rotateRelational(
        const std::vector<float>& embedding,
        const std::string& relation_type
    ) const;
    
    // Zugriff auf Konfiguration
    const RotationConfig& getConfig() const;
};
```

### VectorIndexManager-Erweiterungen

```cpp
// Rotary Embeddings aktivieren/deaktivieren
Status setRotaryEmbeddingConfig(const RotationConfig& config);
bool isRotaryEmbeddingEnabled() const;
std::optional<RotationConfig> getRotaryEmbeddingConfig() const;

// Mit Rotation hinzufügen
Status addEntityWithRotation(
    const BaseEntity& e,
    std::string_view vectorField,
    size_t position
);

// Mit relationaler Rotation hinzufügen
Status addEntityWithRelationalRotation(
    const BaseEntity& e,
    std::string_view vectorField,
    const std::string& relation_type
);

// Suche mit Rotation
std::pair<Status, std::vector<Result>> searchWithRotation(
    const std::vector<float>& query,
    int k,
    size_t query_position,
    const std::vector<std::string>* whitelistPks = nullptr
) const;
```

### BaseEntity-Erweiterungen

```cpp
// Prüfen, ob Entity rotiertes Embedding hat
bool hasRotatedEmbedding(std::string_view field_name) const;

// Rotationsposition abrufen
std::optional<size_t> getRotationPosition(std::string_view field_name) const;

// Rotationstyp (Relation) abrufen
std::optional<std::string> getRotationType(std::string_view field_name) const;
```

## Performance-Charakteristiken

### Rechenkomplexität

- **Einzelrotation**: O(d) wobei d die Embedding-Dimension ist
- **Batch-Rotation**: O(n×d) wobei n die Batch-Größe ist
- **Theta-Cache-Berechnung**: O(d/2), einmalig bei Initialisierung
- **Relationale Rotation**: O(d) + O(1) Hash-Lookup (gecacht)

### Benchmark-Ergebnisse

Basierend auf `bench_rotary_embeddings.cpp`:

| Operation | Dimension | Durchsatz | Anmerkungen |
|-----------|-----------|-----------|-------------|
| Einzelrotation | 128 | ~1-2 µs | Schnell für typische Verwendung |
| Einzelrotation | 1024 | ~8-15 µs | Skaliert linear |
| Batch-Rotation (100) | 128 | ~150-200 µs | ~1,5-2 µs pro Vektor |
| Relationale Rotation | 128 | ~1-2 µs | Ähnlich wie Einzelrotation |
| VectorIndex-Integration | 128 | <10% Overhead | Minimaler Performance-Einfluss |

**Wichtige Erkenntnisse:**

1. **Vernachlässigbarer Overhead**: Rotary Embeddings verursachen <10% Performance-Overhead im Vergleich zu Standard-Vektoroperationen
2. **Lineare Skalierung**: Performance skaliert linear mit Embedding-Dimension
3. **Batch-Effizienz**: Batch-Operationen erreichen nahezu lineare Skalierung
4. **Cache-Effektivität**: Vorberechneter Theta-Cache eliminiert wiederholte Berechnungen

## Best Practices für Konfiguration

### Dimensionswahl

```cpp
// Standarddimensionen (typische Embeddings)
config.hidden_dim = 768;        // BERT-base, RoBERTa
config.hidden_dim = 1024;       // BERT-large
config.hidden_dim = 4096;       // GPT-3, LLaMA-7B

// Immer gerade Dimensionen verwenden
config.num_rotation_pairs = config.hidden_dim / 2;
```

### Base-Theta-Auswahl

```cpp
// Standardwert (aus RoPE-Paper)
config.base_theta = 10000.0;    // Gut für die meisten Anwendungsfälle

// Längere Sequenzen (>2048 Tokens)
config.base_theta = 100000.0;   // Bessere Langstrecken-Kodierung

// Kürzere Sequenzen (<512 Tokens)
config.base_theta = 1000.0;     // Feinere Positionierung
```

### Normalisierung

```cpp
// Im Allgemeinen nicht erforderlich (Rotation erhält Magnitude)
config.normalize_after = false;

// Verwenden, wenn kombiniert mit anderen Operationen, die Magnitude beeinflussen können
config.normalize_after = true;
```

## Integration mit vorhandenen Features

### HNSW-Index-Kompatibilität

RoPE funktioniert nahtlos mit HNSW-Vektorsuche:

```cpp
// HNSW-Index initialisieren
vector_mgr->init("vectors", 768, VectorIndexManager::Metric::COSINE,
                16,    // M-Parameter
                200,   // efConstruction
                64);   // efSearch

// Rotary Embeddings aktivieren
RotationConfig config;
config.hidden_dim = 768;
config.num_rotation_pairs = 384;
config.computeThetaCache();
vector_mgr->setRotaryEmbeddingConfig(config);

// HNSW verwendet automatisch rotierte Vektoren
```

### Verschlüsselungsunterstützung

Rotierte Embeddings funktionieren mit ThemisDBs Feldverschlüsselung:

```cpp
// Vektorverschlüsselung aktivieren
vector_mgr->setVectorEncryptionEnabled(true);
vector_mgr->setVectorKeyId("my_vector_key");

// Rotation erfolgt vor Verschlüsselung
vector_mgr->addEntityWithRotation(entity, "embedding", position);
// → Rotieren → Verschlüsseln → Speichern
```

### Transaktionsunterstützung

Rotary Embeddings nehmen an ACID-Transaktionen teil:

```cpp
auto txn = db->beginTransaction();

for (const auto& doc : batch) {
    vector_mgr->addEntityWithRotation(doc, "embedding", doc.position, txn);
}

txn.commit();  // Alle Rotationen atomar persistiert
```

## Testen

Umfassende Testsuite in `tests/test_rotary_embeddings.cpp`:

```bash
# Tests bauen
cmake --build . --target themis_tests

# Alle RoPE-Tests ausführen
./tests/themis_tests --gtest_filter="*RotaryEmbedding*"

# Spezifischen Test ausführen
./tests/themis_tests --gtest_filter="RotaryEmbeddingTest.InverseRotation"
```

**Testabdeckung:**

- ✅ Konfigurationsvalidierung
- ✅ Theta-Cache-Berechnung
- ✅ Basis-Rotationsoperationen
- ✅ Inverse Rotation (Rotation + Inverse = Identität)
- ✅ Positionale Orthogonalität
- ✅ Batch-Operationen
- ✅ Relationale Rotation
- ✅ VectorIndexManager-Integration
- ✅ BaseEntity-Metadaten-Unterstützung
- ✅ Fehlerbehandlung

## Benchmarking

Performance-Benchmarks in `benchmarks/bench_rotary_embeddings.cpp`:

```bash
# Benchmarks bauen
cmake --build . --target bench_rotary_embeddings

# Alle Benchmarks ausführen
./benchmarks/bench_rotary_embeddings

# Spezifischen Benchmark ausführen
./benchmarks/bench_rotary_embeddings --benchmark_filter="SingleRotation"

# JSON-Ergebnisse ausgeben
./benchmarks/bench_rotary_embeddings --benchmark_format=json > results.json
```

## Fehlersuche

### Häufige Probleme

**Problem**: `Invalid RotationConfig` Fehler

```cpp
// Problem: Ungerade Dimension
config.hidden_dim = 127;  // ❌ Muss gerade sein
config.num_rotation_pairs = 64;

// Lösung:
config.hidden_dim = 128;  // ✅ Gerade Dimension
config.num_rotation_pairs = 64;
```

**Problem**: `theta_cache is empty` Fehler

```cpp
// Problem: Cache-Berechnung vergessen
RotationConfig config;
config.hidden_dim = 128;
// ❌ Fehlt: config.computeThetaCache();
RotaryEmbedding rope(config);  // Wirft Exception

// Lösung:
config.computeThetaCache();  // ✅ Vor Verwendung berechnen
RotaryEmbedding rope(config);
```

**Problem**: Dimensionskonflikt bei Rotation

```cpp
// Problem: Vektorgröße passt nicht zur Konfiguration
std::vector<float> vec(256);  // 256-dim Vektor
RotaryEmbedding rope(config);  // config.hidden_dim = 128
rope.rotate(vec, 0);  // ❌ Wirft Exception

// Lösung: Dimensionen angleichen
std::vector<float> vec(128);  // ✅ Passt zur Konfiguration
```

## Zukünftige Erweiterungen

Geplante Verbesserungen (noch nicht implementiert):

- [ ] **CUDA/HIP-Kernel**: GPU-beschleunigte Rotation für große Batches
- [ ] **Gelernte Rotationsparameter**: Trainierbare θ-Werte für domänenspezifische Optimierung
- [ ] **LoRA-Integration**: Kombination mit LoRA-Adaptern für dynamische Rotationsmuster
- [ ] **REST-API-Endpunkte**: HTTP-Endpunkte für Rotationskonfiguration
- [ ] **Visualisierungstools**: Tools zur Visualisierung rotierter Embedding-Räume

## Referenzen

1. **Su, J., Lu, Y., Pan, S., Wen, B., & Liu, Y.** (2021). "RoFormer: Enhanced Transformer with Rotary Position Embedding." *arXiv preprint arXiv:2104.09864*. https://arxiv.org/abs/2104.09864

2. **Vaswani, A., et al.** (2017). "Attention Is All You Need." *Advances in Neural Information Processing Systems*.

3. **Bordes, A., et al.** (2013). "Translating Embeddings for Modeling Multi-relational Data" (TransE). *Advances in Neural Information Processing Systems*.

## Support

Für Fragen, Probleme oder Feature-Requests zu RoPE in ThemisDB:

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: https://themisdb.com/docs/features/rotary-embeddings
- Community-Forum: https://community.themisdb.com

---

**Letzte Aktualisierung**: 2026-01-27  
**Version**: 1.5.0+  
**Status**: Produktionsbereit
