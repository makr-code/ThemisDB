# 🔌 Documentation des APIs & Ingestion

> **Catégorie :** API de Base  
> **Depuis Version :** 1.3.0  
> **Statut :** ✅ Stable  
> **Mis à jour :** 22 décembre 2025

---

## 📋 Table des Matières

- [🎯 Aperçu](#-aperçu)
- [📊 APIs Disponibles](#-apis-disponibles)
- [🚀 Premiers Pas](#-premiers-pas)
- [📖 Documentation API](#-documentation-api)
- [💡 Bonnes Pratiques](#-bonnes-pratiques)
- [🔧 Dépannage](#-dépannage)
- [📚 Voir Aussi](#-voir-aussi)

---

## 🎯 Aperçu

Ingestion de données, requêtes et documentation API pour ThemisDB. Cette documentation couvre toutes les APIs HTTP, GraphQL, OpenAPI et autres protocoles.

## Référence du Code Source

| Composant | Header | Source |
|-----------|--------|--------|
| ContentManager | `include/content/content_manager.h` | `src/content/content_manager.cpp` |
| ContentProcessor | `include/content/content_processor.h` | `src/content/content_processor.cpp` |

---

## 📊 APIs Disponibles

### REST API HTTP
- **[Référence HTTP API](HTTP_API_REFERENCE.md)** - Référence complète des endpoints REST
- Authentification : Bearer Token
- Base URL : `http://localhost:8765`
- Format : JSON

### GraphQL API
- Requêtes flexibles
- Schéma introspectable
- Subscriptions en temps réel

### OpenAPI
- **Spécification :** [openapi.yaml](../../openapi.yaml)
- Compatible avec Swagger UI
- Génération automatique de clients

---

## 🚀 Premiers Pas

### 1. Vérifier la Santé du Serveur

```bash
curl http://localhost:8765/health
```

**Réponse :**
```json
{
  "status": "healthy",
  "version": "1.3.0",
  "uptime_seconds": 3600
}
```

### 2. Créer une Entité

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_API_KEY" \
  -d '{"blob": "{\"name\":\"Alice\",\"age\":30}"}'
```

### 3. Requêter avec AQL

```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_API_KEY" \
  -d '{
    "query": "FOR u IN users FILTER u.age > 18 RETURN u"
  }'
```

---

## 📖 Documentation API

### APIs Principales

| API | Description | Documentation |
|-----|-------------|---------------|
| **HTTP REST** | CRUD, requêtes, gestion | [HTTP_API_REFERENCE.md](HTTP_API_REFERENCE.md) |
| **AQL** | Langage de requête avancé | [../aql/README.md](../aql/README.md) |
| **GraphQL** | API flexible | [apis_graphql.md](apis_graphql.md) |
| **OpenAPI** | Spécification machine | [apis_openapi.md](apis_openapi.md) |

### APIs Spécialisées

| API | Description | Documentation |
|-----|-------------|---------------|
| **Recherche Hybride** | BM25 + Vecteur | [apis_hybrid_search.md](apis_hybrid_search.md) |
| **ContentFS** | Gestion de contenu | [apis_contentfs.md](apis_contentfs.md) |
| **CDC** | Change Data Capture | [../cdc/README.md](../cdc/README.md) |
| **LLM** | Intégration LLM | [../llm/README.md](../llm/README.md) |

---

## 💡 Bonnes Pratiques

### Authentification

1. **Utiliser des API Keys**
   ```bash
   # Créer une clé API
   curl -X POST http://localhost:8765/api/keys \
     -H "Authorization: Bearer ADMIN_KEY" \
     -d '{"name": "app-key", "permissions": ["read", "write"]}'
   ```

2. **Stocker les Clés de Manière Sécurisée**
   - Ne jamais committer dans le code source
   - Utiliser des variables d'environnement
   - Rotation régulière des clés

### Performance

1. **Utiliser le Batch**
   ```bash
   # Insérer plusieurs entités à la fois
   curl -X POST http://localhost:8765/batch/entities \
     -d '{"entities": [...]}'
   ```

2. **Pagination**
   ```bash
   # Limiter les résultats
   curl -X POST http://localhost:8765/query/aql \
     -d '{"query": "FOR u IN users LIMIT 100 RETURN u"}'
   ```

3. **Cache**
   - Utiliser le cache sémantique pour les requêtes répétées
   - Activer le cache d'embeddings pour la recherche vectorielle

---

## 🔧 Dépannage

### Erreur 401 Unauthorized

```bash
# Vérifier la clé API
curl -H "Authorization: Bearer YOUR_KEY" \
  http://localhost:8765/api/keys/validate
```

### Erreur 429 Rate Limit

```json
{
  "error": "Rate limit exceeded",
  "retry_after": 60
}
```

**Solution :** Attendre ou augmenter les limites dans la configuration.

### Timeout

**Solution :**
- Optimiser la requête avec des index
- Augmenter le timeout dans le client
- Utiliser la pagination

---

## 📚 Voir Aussi

### Documentation

- **[Référence HTTP API](HTTP_API_REFERENCE.md)** - Endpoints complets
- **[AQL](../aql/README.md)** - Langage de requête
- **[Guide Utilisateur](../guides/USER_GUIDE.md)** - Guide complet
- **[Démarrage Rapide](../guides/QUICK_START.md)** - Premiers pas

### SDKs Client

- **[Python SDK](../clients/python_sdk_quickstart.md)**
- **[JavaScript SDK](../clients/javascript_sdk_quickstart.md)**
- **[Rust SDK](../clients/rust_sdk_quickstart.md)**
- **[Go SDK](../clients/go_sdk_quickstart.md)**
- **[Java SDK](../clients/java_sdk_quickstart.md)**
- **[C# SDK](../clients/csharp_sdk_quickstart.md)**
- **[Swift SDK](../clients/swift_sdk_quickstart.md)**

---

**Version :** 1.3.0  
**Statut :** Stable  
**Dernière Mise à Jour :** 23 Décembre 2025
