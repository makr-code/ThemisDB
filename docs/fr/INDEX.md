# Documentation ThemisDB

**Version :** 1.3.0  
**Dernière mise à jour :** 20 décembre 2025

Bienvenue dans la documentation ThemisDB ! Ce guide vous aidera à trouver les informations dont vous avez besoin.

---

## 📚 Navigation Rapide

**Nouveau sur ThemisDB ?**
- [Guide de Démarrage Rapide](guides/QUICK_START.md) - Commencez en 5 minutes
- [Guide d'Installation](guides/INSTALLATION.md) - Instructions complètes d'installation
- [Aperçu de l'Architecture](architecture/OVERVIEW.md) - Comprendre le fonctionnement de ThemisDB
- [🧠 Guide Complet de Configuration LLM](guides/LLM_COMPLETE_SETUP_GUIDE.md) - **NOUVEAU** Guide complet pour la configuration et l'inférence LLM

**Business & Stratégie :**
- [Document stratégique : Industrie 4.0 & IoT](STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md) - ThemisDB pour la fabrication intelligente & applications IoT

**Utiliser ThemisDB :**
- [Langage de Requête AQL](aql/aql_syntax.md) - Apprendre le langage de requête
- [Référence API REST](apis/HTTP_API_REFERENCE.md) - Documentation API HTTP
- [SDKs Client](clients/README.md) - Documentation SDK pour 7 langages

**Exploiter ThemisDB :**
- [Configuration](operations/CONFIGURATION.md) - Configurer votre base de données
- [Surveillance](operations/MONITORING.md) - Surveiller les performances et la santé
- [Sauvegarde & Récupération](operations/BACKUP.md) - Protéger vos données

---

## 📖 Structure de la Documentation

### Démarrage

| Document | Description |
|----------|-------------|
| [Démarrage Rapide](guides/QUICK_START.md) | Tutoriel de 5 minutes pour démarrer |
| [Installation](guides/INSTALLATION.md) | Installation sur Linux, Windows, macOS, Docker |
| [Configuration](operations/CONFIGURATION.md) | Configurer ThemisDB selon vos besoins |
| [Première Requête](guides/FIRST_QUERY.md) | Écrire votre première requête AQL |

### Concepts de Base

| Document | Description |
|----------|-------------|
| [Aperçu de l'Architecture](architecture/OVERVIEW.md) | Architecture système de haut niveau |
| [Conception Multi-Modèle](architecture/architecture_base_entity.md) | Comment ThemisDB gère plusieurs modèles de données |
| [Modèle de Transaction](features/features_transactions.md) | Transactions ACID avec MVCC |
| [Couche de Stockage](architecture/architecture_storage.md) | Stockage LSM-Tree RocksDB |

### Fonctionnalités

| Document | Description |
|----------|-------------|
| [Aperçu des Fonctionnalités](features/features_overview.md) | Catalogue complet des fonctionnalités |
| [Recherche Vectorielle](features/features_vector_ops.md) | Recherche de similarité avec HNSW/FAISS |
| [Opérations de Graphe](features/features_graph.md) | Traversée de graphe et recherche de chemin |
| [Séries Temporelles](features/features_time_series.md) | Données de séries temporelles et compression |
| [Hypertables](features/features_hypertables.md) | Séries temporelles compatibles TimescaleDB (v1.2+) |
| [Recherche Hybride](search/hybrid_search_design.md) | Recherche BM25+Vecteur optimisée pour RAG (v1.2+) |
| [Analytique](observability/CEP_STREAMING_ANALYTICS.md) | Analytique CEP et OLAP |

### Langage de Requête (AQL)

| Document | Description |
|----------|-------------|
| [Syntaxe AQL](aql/aql_syntax.md) | Référence complète du langage AQL |
| [Exemples AQL](aql/aql_examples.md) | Modèles de requête courants |
| [Optimisation de Requête](aql/aql_explain_profile.md) | Commandes EXPLAIN et PROFILE |

### Référence API

| Document | Description |
|----------|-------------|
| [API REST](apis/HTTP_API_REFERENCE.md) | Points de terminaison de l'API HTTP |
| [API GraphQL](apis/apis_graphql.md) | Interface GraphQL |
| [SDKs Client](clients/README.md) | SDKs pour Python, JS, Rust, Go, Java, C#, Swift |

### Sécurité & Conformité

| Document | Description |
|----------|-------------|
| [Aperçu de la Sécurité](security/security_implementation.md) | Fonctionnalités de sécurité d'entreprise |
| [Configuration TLS](guides/guides_tls_setup.md) | Configurer TLS 1.3 et mTLS |
| [Configuration RBAC](guides/guides_rbac.md) | Contrôle d'accès basé sur les rôles |
| [Chiffrement](security/security_encryption_strategy.md) | Chiffrement des données au repos et en transit |
| [Journalisation d'Audit](features/features_audit_logging.md) | Journalisation des événements de sécurité |
| [Conformité](compliance/compliance_dashboard.md) | Conformité RGPD, SOC 2, HIPAA |

### Entreprise & Gouvernance

| Document | Description |
|----------|-------------|
| [Document stratégique CMS](governance/CMS_STRATEGY_PAPER.md) | ThemisDB pour la gestion de contenu (Gouvernement & Entreprise) |
| [Édition Enterprise](enterprise/README.md) | Fonctionnalités et licences d'entreprise |
| [Aperçu de la Gouvernance](governance/README.md) | Gouvernance et politiques des données |

### Opérations

| Document | Description |
|----------|-------------|
| [Guide de Déploiement](deployment/README.md) | Stratégies de déploiement en production |
| [Déploiement Docker](deployment/DOCKER_DEPLOYMENT.md) | Déploiement Docker et Kubernetes |
| [Configuration](operations/CONFIGURATION.md) | Référence de configuration |
| [Surveillance](operations/MONITORING.md) | Métriques Prometheus et alertes |
| [Sauvegarde & Récupération](operations/BACKUP.md) | Stratégies de sauvegarde et reprise après sinistre |
| [Dépannage](operations/TROUBLESHOOTING.md) | Problèmes courants et solutions |
| [Optimisation des Performances](performance/performance_memory.md) | Optimiser pour votre charge de travail |

### Développement

| Document | Description |
|----------|-------------|
| [Contribuer](../CONTRIBUTING.md) | Comment contribuer à ThemisDB |
| [Guide de Build](guides/guides_build_strategy.md) | Compiler depuis les sources |
| [Configuration de Développement](development/SETUP.md) | Configurer l'environnement de développement |
| [Guide de Test](development/TESTING.md) | Exécuter et écrire des tests |
| [Style de Code](development/CODE_STYLE.md) | Normes de codage |
| [Architecture](architecture/ARCHITECTURE_OVERVIEW.md) | Plongée dans les détails internes |

### Sujets Avancés

| Document | Description |
|----------|-------------|
| [Sharding](sharding/sharding_overview.md) | Sharding horizontal et routage |
| [Réplication](sharding/sharding_replication.md) | Leader-follower et multi-maître |
| [Accélération GPU](performance/performance_gpu_plan.md) | Backends CUDA, Vulkan, HIP |
| [Co-localisation vLLM](reports/VARIANT_STRATEGY_v1.1.0.md) | Optimisation de la charge de travail IA/ML |
| [Traitement de Contenu](content/content_architecture.md) | Traiter PDFs, images, vidéos, etc. |

### Notes de Version

| Document | Description |
|----------|-------------|
| [Changelog](../CHANGELOG.md) | Historique des versions et modifications |
| [Feuille de Route](roadmap/ROADMAP.md) | Plans et fonctionnalités futurs |
| [Version v1.3.0](../RELEASE_NOTES_v1.3.0.md) | Dernières notes de version |
| [Version v1.2.0](releases/v1.2.0.md) | Version précédente |
| [Version v1.1.0](releases/v1.1.0.md) | Version précédente |
| [Guides de Migration](guides/MIGRATION.md) | Migration entre versions |

---

## 🔍 Recherche par Sujet

### Par Cas d'Usage

**Construire une Application :**
- [Démarrage Rapide](guides/QUICK_START.md) → [API REST](apis/HTTP_API_REFERENCE.md) → [SDKs Client](clients/README.md)

**Analytique & BI :**
- [Fonctionnalités OLAP](observability/README.md) → [Export Parquet](observability/README.md) → [Séries Temporelles](timeseries/README.md)

**Applications IA/ML :**
- [Recherche Vectorielle](search/README.md) → [Recherche Hybride](search/hybrid_search_design.md) → [Cache d'Embeddings](storage/README.md)

**Applications de Graphe :**
- [Opérations de Graphe](features/README.md) → [Requêtes de Graphe AQL](aql/README.md) → [Algorithmes de Chemin](features/README.md)

**Déploiement en Production :**
- [Guide de Déploiement](deployment/README.md) → [Surveillance](observability/README.md) → [Sauvegarde](deployment/README.md) → [Sécurité](security/README.md)

### Par Technologie

**Docker/Kubernetes :**
- [Déploiement Docker](deployment/DOCKER_DEPLOYMENT.md)
- [Guide Kubernetes](deployment/README.md)
- [Charts Helm](../helm/README.md)

**Plateformes Cloud :**
- [Déploiement AWS/Azure/GCP](deployment/README.md)

**ARM/Raspberry Pi :**
- [Guide de Build ARM](build/README.md)
- [Déploiement ARM](deployment/README.md)

---

## 📊 Performance & Benchmarks

- [Aperçu des Performances](performance/README.md)
- [Guide de Benchmarking](../benchmarks/README.md)
- [Optimisation de la Mémoire](performance/README.md)
- [Performance GPU](performance/GPU_ACCELERATION_PLAN.md)
- [Optimisation des Requêtes](performance/README.md)

---

## 🤝 Ressources Communautaires

- **Dépôt GitHub :** [github.com/makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **Suivi des Issues :** [Signaler des bugs](https://github.com/makr-code/ThemisDB/issues)
- **Discussions :** [Forum communautaire](https://github.com/makr-code/ThemisDB/discussions)
- **Contribuer :** [Comment contribuer](../CONTRIBUTING.md)
- **Sécurité :** [Politique de sécurité](../SECURITY.md)

---

## 📝 Conventions de Documentation

**Indicateurs de Version :**
- Aucun marqueur : Disponible dans toutes les versions
- (v1.1+) : Disponible à partir de la version 1.1.0
- (v1.2+) : Disponible à partir de la version 1.2.0
- (v1.3+) : Disponible à partir de la version 1.3.0
- 🚧 Expérimental : Pas prêt pour la production
- 📋 Planifié : Fonctionnalité future

**Exemples de Code :**
- Les exemples utilisent des espaces réservés génériques comme `localhost:8765`
- Ajuster selon votre environnement
- La plupart des exemples fonctionnent avec la configuration par défaut

**Conventions :**
- Les chemins de fichiers utilisent des barres obliques (fonctionne aussi sur Windows)
- Les exemples de commandes supposent un shell bash (utilisateurs Windows : utiliser l'équivalent PowerShell)

---

## 🆘 Obtenir de l'Aide

1. **Consulter la Documentation :** Rechercher d'abord dans cette documentation
2. **Rechercher les Issues :** Vérifier si quelqu'un d'autre a eu le même problème
3. **Demander à la Communauté :** Utiliser [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
4. **Signaler un Bug :** Ouvrir une [issue](https://github.com/makr-code/ThemisDB/issues/new)

---

**Version de la Documentation :** 1.2.0  
**Dernière Mise à Jour :** 15 décembre 2025  
**Prochaine Révision :** 15 mars 2026
