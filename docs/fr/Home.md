# Bienvenue sur ThemisDB

**Une base de données multi-modèle haute performance avec garanties ACID**

[![Version](https://img.shields.io/badge/version-1.3.3-blue)](https://github.com/makr-code/ThemisDB/releases/tag/v1.3.3)
[![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
[![Licence](https://img.shields.io/badge/license-MIT-green)](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## Aperçu

ThemisDB est une base de données multi-modèle prête pour la production qui combine les modèles **relationnel, graphe, vectoriel et document** dans un système unique avec un support complet des transactions ACID. Construite sur RocksDB avec des fonctionnalités avancées de sécurité et de conformité.

**Capacités Clés :**
- 🔒 **Transactions ACID** - Isolation complète des snapshots avec MVCC
- 🔍 **Multi-Modèle** - Une base de données pour relationnel, graphe, vecteur et documents
- 🚀 **Haute Performance** - 45K écritures/s, 120K lectures/s
- 🛡️ **Sécurité d'Entreprise** - TLS 1.3, RBAC, chiffrement, journalisation d'audit
- 🌐 **Distribué** - Sharding horizontal, réplication, prêt pour Kubernetes
- 🧠 **Prêt pour l'IA** - Recherche hybride, cache d'embeddings, accélération GPU

---

## Liens Rapides

### 🚀 Démarrage

- **[Guide de Démarrage Rapide](guides-QUICK_START)** - Commencez en 5 minutes
- **[Installation](guides-INSTALLATION)** - Installer sur Linux, Windows, macOS ou Docker
- **[Configuration](operations-CONFIGURATION)** - Configurer selon vos besoins
- **[Première Requête](guides-FIRST_QUERY)** - Écrire votre première requête AQL

### 📖 Apprendre ThemisDB

- **[Aperçu de l'Architecture](architecture-OVERVIEW)** - Comprendre la conception
- **[Langage de Requête AQL](aql-aql_syntax)** - Apprendre la syntaxe de requête
- **[Aperçu des Fonctionnalités](features-features_overview)** - Explorer toutes les fonctionnalités
- **[API REST](api-REST_API)** - Référence de l'API HTTP

### 🚀 Déployer en Production

- **[Guide de Déploiement](operations-DEPLOYMENT)** - Déploiement en production
- **[Guide Docker](DOCKER_DEPLOYMENT)** - Exécuter avec Docker/Kubernetes
- **[Surveillance](operations-MONITORING)** - Surveiller avec Prometheus
- **[Durcissement de la Sécurité](security-security_implementation)** - Sécuriser votre déploiement

---

## Fonctionnalités de Base

### Base de Données Multi-Modèle

**Relationnel :**
- Index secondaires (égalité, composite, plage)
- Requêtes AQL de type SQL
- Transactions ACID

**Graphe :**
- Stockage de graphe natif
- Traversées BFS, Dijkstra, A*
- Contraintes et élagage de chemin

**Vecteur :**
- Index HNSW et FAISS
- Recherche de similarité accélérée par GPU
- Recherche hybride pour les workflows RAG

**Document :**
- Stockage JSON avec schéma flexible
- Extraction rapide de champs
- Chiffrement basé sur le schéma

### Analytique Avancée

- **Moteur CEP** - Traitement d'événements complexes avec correspondance de motifs
- **OLAP** - CUBE, ROLLUP, fonctions de fenêtre
- **Séries Temporelles** - Compression Gorilla, agrégats continus
- **Streaming** - Traitement de données en temps réel

### Sécurité d'Entreprise

- **Authentification** - RBAC avec hiérarchie à 4 niveaux, mTLS
- **Chiffrement** - AES-256-GCM au repos, TLS 1.3 en transit
- **Audit** - 65+ types d'événements, intégration SIEM
- **Conformité** - Prêt pour RGPD, SOC 2, HIPAA
- **Secrets** - Intégration HashiCorp Vault

### Capacités Distribuées

- **Sharding** - Hachage cohérent, 150 nœuds virtuels
- **Réplication** - Leader-follower et multi-maître
- **Redondance** - Modes de type RAID (MIRROR, STRIPE, PARITY)
- **Kubernetes** - Opérateur avec CRDs
- **Surveillance** - 44 métriques Prometheus, tableaux de bord Grafana

---

## Benchmarks de Performance

| Opération | Débit | Latence (p50) |
|-----------|-------|---------------|
| Entity PUT | 45 000 ops/s | 0,02 ms |
| Entity GET | 120 000 ops/s | 0,008 ms |
| Requête indexée | 8 500 requêtes/s | 0,12 ms |
| Traversée de graphe | 3 200 ops/s | 0,31 ms |
| Vecteur ANN (k=10) | 1 800 requêtes/s | 0,55 ms |

**[Benchmarks Complets →](benchmarks-README)**

---

## Structure de la Documentation

Cette wiki est organisée dans les sections suivantes :

### Pour les Utilisateurs
- **Démarrage** - Installation, démarrage rapide, configuration
- **Fonctionnalités** - Documentation détaillée des fonctionnalités
- **Langage de Requête** - Syntaxe et exemples AQL
- **Référence API** - REST, GraphQL, SDKs client

### Pour les Opérateurs
- **Opérations** - Déploiement, surveillance, sauvegarde
- **Sécurité** - TLS, RBAC, chiffrement, conformité
- **Performance** - Optimisation et réglage

### Pour les Développeurs
- **Développement** - Compilation, test, contribution
- **Architecture** - Conception système et détails internes
- **Sujets Avancés** - Sharding, GPU, plugins

---

## Feuille de Route

### Terminé (v1.0 - v1.2)
- ✅ Transactions ACID avec MVCC
- ✅ Support multi-modèle (tous les 4 modèles)
- ✅ Sharding et réplication horizontaux
- ✅ Accélération GPU (10 backends)
- ✅ Fonctionnalités de sécurité d'entreprise
- ✅ SDKs client (7 langages)
- ✅ Hypertables et recherche hybride

### Focus Actuel (v1.3.0 - T1 2026)
- 🚧 Optimiseur de requête v2
- 🚧 Intégration RE2 pour la sécurité
- 🚧 Publication des SDKs (PyPI, npm, crates.io)
- 🚧 Tests de pénétration phase 1

### Planifié (v1.4.0+ - 2026)
- 📋 Déploiement multi-datacentre
- 📋 Fonctionnalités avancées ML/GNN
- 📋 Intégration DuckDB OLAP
- 📋 Vues matérialisées en temps réel

**[Feuille de Route Complète →](roadmap-ROADMAP)**

---

## Communauté & Support

- **📖 Documentation :** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **💬 Discussions :** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **🐛 Issues :** [Signaler des bugs](https://github.com/makr-code/ThemisDB/issues)
- **🔒 Sécurité :** [Politique de sécurité](SECURITY)
- **🤝 Contribuer :** [Guide de contribution](CONTRIBUTING)

---

## Exemple de Démarrage Rapide

```bash
# Télécharger et exécuter avec Docker
docker pull themisdb/themisdb:latest
docker run -d -p 8765:8765 themisdb/themisdb:latest

# Créer une entité
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30}"}'

# Requête
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"age","value":"30"}]}'
```

**[Démarrage Rapide Complet →](guides-QUICK_START)**

---

## Licence

ThemisDB est open source sous [Licence MIT](https://github.com/makr-code/ThemisDB/blob/main/LICENSE).

---

**Prêt à commencer ?** → **[Guide de Démarrage Rapide](guides-QUICK_START)**

**Besoin d'aide ?** → **[Discussions](https://github.com/makr-code/ThemisDB/discussions)**
