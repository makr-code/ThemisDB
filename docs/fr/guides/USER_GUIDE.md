---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "23.12.2025"
audience: "Développeurs d'applications, ingénieurs de données, ingénieurs IA/ML"
---

# 📋 Guide Utilisateur ThemisDB

Guide complet pour les développeurs d'applications et ingénieurs de données.

## 📋 Table des Matières

- [📋 Aperçu](#-aperçu)
- [✨ Fonctionnalités & Points Forts](#-fonctionnalités--points-forts)
- [🚀 Démarrage Rapide](#-démarrage-rapide)
- [📖 Opérations Multi-Modèle](#-opérations-multi-modèle)
- [💡 Bonnes Pratiques](#-bonnes-pratiques)
- [🔧 Dépannage](#-dépannage)
- [📚 Voir Aussi](#-voir-aussi)

---

## 📋 Aperçu

ThemisDB est une base de données multi-modèle pour applications modernes avec :
- 6 modèles de données intégrés
- Fonctionnalités d'entreprise comme transactions & sécurité
- RAG (Retrieval-Augmented Generation) pour IA/ML
- Accélération GPU pour recherche vectorielle

**Public cible :** Développeurs d'applications, ingénieurs de données, ingénieurs IA/ML  
**Version :** 1.3.0  
**Dernière mise à jour :** Décembre 2025

---

## ✨ Fonctionnalités & Points Forts

- 🚀 **Multi-Modèle** - Relationnel, Document, Graphe, Vecteur, Séries Temporelles, Spatial
- 🔐 **Entreprise** - Transactions ACID, RBAC, TLS/mTLS
- 🎯 **Prêt pour RAG** - Recherche hybride (BM25 + Vecteur), Cache d'embeddings
- ⚡ **Haute Performance** - Optimisé SIMD, supporté GPU
- 📊 **Surveillance** - OpenTelemetry, métriques Prometheus

---

## 🚀 Démarrage Rapide (5 Minutes)

```bash
# 1. Télécharger l'image Docker
docker pull themisdb/themisdb:latest

# 2. Démarrer ThemisDB
docker run -d \
  -p 9042:9042 \
  -v /data/themis:/data \
  --name themisdb \
    themisdb/themisdb:latest

# 3. Se connecter avec le client Python
pip install themisdb-client
```

```python
from themisdb import ThemisClient

# Se connecter à la base de données
client = ThemisClient('localhost:9042')

# Créer la première entité
client.create_entity('users', {
    'id': 1,
    'name': 'Alice',
    'email': 'alice@example.com'
})

# Requêter les données
users = client.query('FOR user IN users RETURN user')
print(users)
```

---

## 📖 Opérations Multi-Modèle

ThemisDB supporte **6 modèles de données** dans une architecture unifiée :

### 1. Relationnel (Tabulaire)

```python
# Créer une structure de type table
client.create_collection('employees', schema={
    'id': 'int',
    'name': 'string',
    'department': 'string',
    'salary': 'float'
})

# Insérer des données
client.insert('employees', {
    'id': 1,
    'name': 'Alice',
    'department': 'Engineering',
    'salary': 75000.0
})

# Requête avec filtres
employees = client.query('''
    FOR e IN employees
    FILTER e.department == "Engineering"
    RETURN e
''')
```

### 2. Document (JSON)

```python
# Stocker des documents JSON flexibles
client.insert('products', {
    'id': 'prod-001',
    'name': 'Laptop Pro',
    'specs': {
        'cpu': 'Intel i7',
        'ram': '16GB',
        'storage': '512GB SSD'
    },
    'tags': ['electronics', 'computers']
})

# Requête par champs imbriqués
products = client.query('''
    FOR p IN products
    FILTER p.specs.ram >= "16GB"
    RETURN p
''')
```

### 3. Graphe (Relations)

```python
# Créer des nœuds
client.create_vertex('persons', {'id': 1, 'name': 'Alice'})
client.create_vertex('persons', {'id': 2, 'name': 'Bob'})

# Créer des arêtes (relations)
client.create_edge('knows', 
    from_vertex='persons/1',
    to_vertex='persons/2',
    properties={'since': 2020}
)

# Traverser le graphe
path = client.query('''
    FOR v, e, p IN 1..3 OUTBOUND 'persons/1' knows
    RETURN {vertex: v, edge: e}
''')
```

### 4. Vecteur (Similarité)

```python
# Insérer des embeddings
client.insert('documents', {
    'id': 'doc-001',
    'text': 'ThemisDB est une base de données multi-modèle',
    'embedding': [0.1, 0.2, 0.3, ...]  # 768-dim vector
})

# Recherche de similarité
similar = client.vector_search(
    collection='documents',
    query_vector=[0.15, 0.18, 0.32, ...],
    k=10,
    metric='cosine'
)
```

### 5. Séries Temporelles

```python
# Insérer des données de séries temporelles
client.insert('metrics', {
    'timestamp': '2025-01-15T10:30:00Z',
    'sensor_id': 'temp-001',
    'value': 22.5,
    'unit': 'celsius'
})

# Agréger par fenêtres de temps
avg_temp = client.query('''
    FOR m IN metrics
    FILTER m.timestamp >= "2025-01-15"
    COLLECT window = DATE_TRUNC(m.timestamp, "hour")
    AGGREGATE avg_value = AVG(m.value)
    RETURN {time: window, avg: avg_value}
''')
```

### 6. Spatial (Géographique)

```python
# Insérer des données géographiques
client.insert('locations', {
    'name': 'Office Berlin',
    'coordinates': {
        'lat': 52.5200,
        'lon': 13.4050
    }
})

# Recherche par proximité
nearby = client.query('''
    FOR loc IN locations
    FILTER DISTANCE(loc.coordinates, {lat: 52.52, lon: 13.40}) < 1000
    RETURN loc
''')
```

---

## 💡 Bonnes Pratiques

### Performance

1. **Utiliser des Index**
   ```python
   # Créer des index pour les requêtes fréquentes
   client.create_index('users', fields=['email'])
   client.create_index('products', fields=['category', 'price'])
   ```

2. **Opérations par Lot**
   ```python
   # Insérer plusieurs enregistrements à la fois
   client.batch_insert('users', [
       {'id': 1, 'name': 'Alice'},
       {'id': 2, 'name': 'Bob'},
       {'id': 3, 'name': 'Charlie'}
   ])
   ```

3. **Projection (Sélection de Champs)**
   ```python
   # Ne retourner que les champs nécessaires
   users = client.query('''
       FOR u IN users
       RETURN {name: u.name, email: u.email}
   ''')
   ```

### Sécurité

1. **Utiliser RBAC**
   ```python
   # Créer des rôles avec permissions
   client.create_role('analyst', permissions=['read'])
   client.create_role('admin', permissions=['read', 'write', 'delete'])
   ```

2. **Activer TLS**
   ```python
   client = ThemisClient('localhost:9042', tls=True, cert='./cert.pem')
   ```

3. **Chiffrer les Données Sensibles**
   ```python
   # Les champs marqués comme sensibles sont automatiquement chiffrés
   client.insert('users', {
       'name': 'Alice',
       'email': 'alice@example.com',  # Non chiffré
       'ssn': '123-45-6789'  # Chiffré automatiquement
   }, sensitive_fields=['ssn'])
   ```

---

## 🔧 Dépannage

### Problèmes de Connexion

```python
# Vérifier la connexion
try:
    client.ping()
    print("Connecté !")
except ConnectionError as e:
    print(f"Échec de connexion : {e}")
```

### Requêtes Lentes

```python
# Activer le profiling de requête
result = client.query('FOR u IN users RETURN u', profile=True)
print(result.execution_plan)
```

### Utilisation Mémoire Élevée

```yaml
# Ajuster la config (config.yaml)
storage:
  memtable_size_mb: 128
  block_cache_size_mb: 512
```

---

## 📚 Voir Aussi

### Documentation

- **[Démarrage Rapide](QUICK_START.md)** - Commencer en 5 minutes
- **[Guide Administrateur](ADMINISTRATOR_GUIDE.md)** - Opérations et déploiement
- **[Référence API](../apis/HTTP_API_REFERENCE.md)** - Documentation complète de l'API
- **[Référence AQL](../aql/aql_syntax.md)** - Langage de requête

### SDKs Client

- **Python** : [python_sdk_quickstart.md](../clients/python_sdk_quickstart.md)
- **JavaScript** : [javascript_sdk_quickstart.md](../clients/javascript_sdk_quickstart.md)
- **Rust** : [rust_sdk_quickstart.md](../clients/rust_sdk_quickstart.md)
- **Go** : [go_sdk_quickstart.md](../clients/go_sdk_quickstart.md)
- **Java** : [java_sdk_quickstart.md](../clients/java_sdk_quickstart.md)
- **C#** : [csharp_sdk_quickstart.md](../clients/csharp_sdk_quickstart.md)
- **Swift** : [swift_sdk_quickstart.md](../clients/swift_sdk_quickstart.md)

### Ressources Communautaires

- **GitHub** : [github.com/makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **Discussions** : [Forum communautaire](https://github.com/makr-code/ThemisDB/discussions)
- **Wiki** : [Documentation Wiki](https://github.com/makr-code/ThemisDB/wiki)

---

**Version :** 1.3.0  
**Public :** Développeurs & Ingénieurs de Données  
**Dernière Mise à Jour :** 23 Décembre 2025
