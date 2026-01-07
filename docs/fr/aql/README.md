# 🔍 Module AQL (Advanced Query Language)

**Catégorie :** 🔍 AQL de Base  
**Version :** v1.3.0  
**Statut :** ✅ Prêt pour Production  
**Date :** 22 décembre 2025

---

## 📑 Table des Matières

- [📋 Aperçu](#-aperçu)
- [✨ Fonctionnalités & Points Forts](#-fonctionnalités--points-forts)
- [🚀 Démarrage Rapide](#-démarrage-rapide)
- [📖 Documentation Détaillée](#-documentation-détaillée)
- [💡 Bonnes Pratiques](#-bonnes-pratiques)
- [🔧 Dépannage](#-dépannage)
- [📚 Voir Aussi](#-voir-aussi)

---

## 📋 Aperçu

ThemisDB implémente une version étendue d'**AQL (Advanced Query Language)** – un langage de requête déclaratif, similaire à SQL, avec des fonctionnalités supplémentaires pour les requêtes multi-modèles sur les données relationnelles, graphes, vecteurs et documents.

## Référence du Code Source

| Composant | Header | Source | Description |
|-----------|--------|--------|-------------|
| AQLParser | `aql_parser.h` | `aql_parser.cpp` | Parser & AST |
| AQLTranslator | `aql_translator.h` | `aql_translator.cpp` | AST → Plan d'exécution |
| AQLRunner | `aql_runner.h` | `aql_runner.cpp` | Exécution de requête |
| QueryOptimizer | `query_optimizer.h` | `query_optimizer.cpp` | Optimisation de plan |
| LetEvaluator | `let_evaluator.h` | `let_evaluator.cpp` | Liaisons LET |
| CTECache | `cte_cache.h` | `cte_cache.cpp` | Clauses WITH |
| WindowEvaluator | `window_evaluator.h` | `window_evaluator.cpp` | Fonctions fenêtre |

## Types de Nœuds AST

```cpp
enum class ASTNodeType {
    // Nœuds de requête
    Query,              // Nœud racine
    ForNode,            // FOR variable IN collection
    FilterNode,         // FILTER condition
    SortNode,           // SORT expr [ASC|DESC]
    LimitNode,          // LIMIT offset, count
    ReturnNode,         // RETURN expression
    LetNode,            // LET variable = expression
    CollectNode,        // COLLECT ... AGGREGATE ...
    WithNode,           // WITH cteName AS subquery
    
    // Expressions
    BinaryOp,           // ==, !=, >, <, AND, OR, +, -, *, /
    UnaryOp,            // NOT, -, +
    FunctionCall,       // CONCAT, SUM, LOWER, etc.
    FieldAccess,        // doc.field, doc.nested.field
    Literal,            // "string", 123, true, false, null
    Variable,           // doc, user
    ArrayLiteral,       // [1, 2, 3]
    ObjectConstruct,    // {name: doc.name}
    SimilarityCall,     // SIMILARITY(expr, [vector], k?)
    ProximityCall,      // PROXIMITY(expr, [lon,lat])
    SubqueryExpr,       // Sous-requête dans expression
    AnyExpr,            // Quantificateur ANY
    AllExpr             // Quantificateur ALL
};
```

## Opérateurs

```cpp
enum class BinaryOperator {
    // Comparaison
    Eq, Neq, Lt, Lte, Gt, Gte,
    // Logique
    And, Or, Xor,
    // Arithmétique
    Add, Sub, Mul, Div, Mod,
    // Appartenance
    In
};

enum class UnaryOperator {
    Not, Minus, Plus
};
```

## Syntaxe de Requête

### Requête de Base

```aql
FOR doc IN users
  FILTER doc.age > 18
  SORT doc.name ASC
  LIMIT 0, 10
  RETURN doc
```

### Agrégation

```aql
FOR doc IN orders
  COLLECT city = doc.city
  AGGREGATE total = SUM(doc.amount),
            count = LENGTH(doc)
  RETURN {city, total, count}
```

### Jointure

```aql
FOR user IN users
  FOR order IN orders
    FILTER order.user_id == user.id
    RETURN {
      user: user.name,
      order: order.product
    }
```

### Recherche Vectorielle

```aql
FOR doc IN documents
  LET score = SIMILARITY(doc.embedding, @queryVector)
  FILTER score > 0.8
  SORT score DESC
  LIMIT 10
  RETURN {doc, score}
```

### Traversée de Graphe

```aql
FOR v, e, p IN 1..3 OUTBOUND 'users/alice' friends
  RETURN {
    vertex: v,
    edge: e,
    path: p
  }
```

---

## ✨ Fonctionnalités & Points Forts

### Requêtes Multi-Modèles

- **Relationnel** : Filtres, tris, agrégations
- **Document** : Accès aux champs imbriqués, tableaux
- **Graphe** : Traversées, recherche de chemins
- **Vecteur** : Recherche de similarité, ANN
- **Séries Temporelles** : Fenêtres temporelles, agrégats
- **Spatial** : Requêtes géographiques, proximité

### Fonctions Intégrées

**Chaînes de caractères :**
- `CONCAT(str1, str2, ...)` - Concaténer
- `LOWER(str)`, `UPPER(str)` - Conversion de casse
- `SUBSTRING(str, offset, length)` - Extraire sous-chaîne
- `TRIM(str)` - Supprimer espaces

**Mathématiques :**
- `SUM(expr)`, `AVG(expr)` - Agrégats
- `MIN(expr)`, `MAX(expr)` - Min/Max
- `COUNT(expr)` - Comptage
- `ROUND(num, decimals)` - Arrondir

**Tableaux :**
- `LENGTH(array)` - Longueur
- `FIRST(array)`, `LAST(array)` - Premier/Dernier élément
- `UNIQUE(array)` - Valeurs uniques
- `FLATTEN(array)` - Aplatir tableau imbriqué

**Date/Heure :**
- `DATE_NOW()` - Date/heure actuelle
- `DATE_FORMAT(date, format)` - Formater date
- `DATE_DIFF(date1, date2, unit)` - Différence
- `DATE_TRUNC(date, unit)` - Tronquer

**Vecteur/Similarité :**
- `SIMILARITY(vec1, vec2)` - Similarité cosinus
- `DISTANCE(vec1, vec2)` - Distance euclidienne
- `DOT_PRODUCT(vec1, vec2)` - Produit scalaire

**Spatial/Géographique :**
- `DISTANCE(point1, point2)` - Distance géographique
- `WITHIN_RECTANGLE(point, sw, ne)` - Dans rectangle
- `NEAR(point, center, radius)` - Proximité

---

## 🚀 Démarrage Rapide

### Requête Simple

```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.age > 18 RETURN u"
  }'
```

### Avec Paramètres

```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.city == @city RETURN u",
    "bindVars": {"city": "Berlin"}
  }'
```

### Client Python

```python
from themisdb import ThemisClient

client = ThemisClient('localhost:8765')

# Requête simple
users = client.query('''
    FOR u IN users
    FILTER u.age > 18
    SORT u.name
    RETURN u
''')

# Avec paramètres
users = client.query('''
    FOR u IN users
    FILTER u.city == @city
    RETURN u
''', bind_vars={'city': 'Berlin'})
```

---

## 💡 Bonnes Pratiques

### Optimisation des Performances

1. **Utiliser des Index**
   ```aql
   # Créer un index d'abord
   # Puis utiliser dans les filtres
   FOR doc IN users
     FILTER doc.email == @email  # Utilise l'index
     RETURN doc
   ```

2. **Filtrer Tôt**
   ```aql
   # Bon : Filtrer avant le tri
   FOR doc IN users
     FILTER doc.active == true
     SORT doc.name
     RETURN doc
   
   # Moins bon : Filtrer après le tri
   FOR doc IN users
     SORT doc.name
     FILTER doc.active == true
     RETURN doc
   ```

3. **Projection (Limiter les Champs)**
   ```aql
   # Retourner uniquement les champs nécessaires
   FOR doc IN users
     RETURN {name: doc.name, email: doc.email}
   ```

4. **Utiliser LIMIT**
   ```aql
   # Limiter les résultats
   FOR doc IN users
     LIMIT 100
     RETURN doc
   ```

---

## 🔧 Dépannage

### Requête Lente

```aql
# Utiliser EXPLAIN pour voir le plan d'exécution
EXPLAIN FOR doc IN users FILTER doc.age > 18 RETURN doc
```

### Erreur de Syntaxe

```bash
# Vérifier la syntaxe avec le validateur
curl -X POST http://localhost:8765/query/aql/validate \
  -d '{"query": "FOR u IN users RETURN u"}'
```

### Index Manquant

```bash
# Créer un index
curl -X POST http://localhost:8765/index/create \
  -d '{"collection": "users", "fields": ["age"]}'
```

---

## 📚 Voir Aussi

### Documentation AQL

- **[Syntaxe AQL](aql_syntax.md)** - Référence complète de syntaxe
- **[Fonctions](aql_functions_reference.md)** - Toutes les fonctions intégrées
- **[Requêtes Hybrides](aql_hybrid_queries.md)** - Recherche hybride (BM25 + Vecteur)
- **[Moteur de Requête](aql_query_engine.md)** - Détails d'implémentation

### Guides

- **[Guide de Démarrage Rapide](../guides/QUICK_START.md)** - Premiers pas
- **[Guide Utilisateur](../guides/USER_GUIDE.md)** - Guide complet
- **[Référence API](../apis/HTTP_API_REFERENCE.md)** - API REST

---

## ✨ Exemples Avancés

### CTE (Common Table Expressions)

```aql
WITH topUsers = (
  FOR u IN users
    FILTER u.score > 100
    RETURN u
)
FOR u IN topUsers
  FILTER u.active == true
  RETURN u
```

### Fonctions Fenêtre

```aql
FOR doc IN sales
  WINDOW {
    preceding: 2,
    following: 0
  }
  AGGREGATE movingAvg = AVG(doc.amount)
  RETURN {date: doc.date, movingAvg}
```

### Sous-requêtes

```aql
FOR user IN users
  LET orderCount = (
    FOR order IN orders
      FILTER order.user_id == user.id
      RETURN 1
  )
  RETURN {
    user: user.name,
    orders: LENGTH(orderCount)
  }
```

---

**Version :** 1.3.0  
**Statut :** Production Ready  
**Dernière Mise à Jour :** 23 Décembre 2025
