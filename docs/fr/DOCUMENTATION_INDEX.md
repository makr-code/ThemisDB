# Index de la Documentation ThemisDB

**Dernière mise à jour :** 20 décembre 2025
**Version :** 1.3.0 (Version d'intégration LLM)

## 🎯 Démarrage rapide par rôle

### Pour les Développeurs
1. [README.md](../README.md) - Aperçu du projet & Démarrage rapide
2. [guides/guides_build_strategy.md](guides/guides_build_strategy.md) - Chaîne d'outils de build (Windows/Linux/Docker)
3. [docs/guides/guides_build.md](guides/guides_build.md) - Instructions détaillées de build
4. [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - État actuel du développement
5. [Fonctionnalités Enterprise](enterprise/README.md) - Fonctionnalités d'évolutivité Enterprise

### Pour les Parties Prenantes
1. [STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md](STRATEGIEPAPIER_INDUSTRIE_4_0_IOT.md) - ⭐ **NOUVEAU :** Stratégie Industrie 4.0 & IoT
2. [THEMIS_SACHSTANDSBERICHT_2025.md](reports/themis_sachstandsbericht_2025.md) - Résumé exécutif
3. ~~THEMIS_PROJECT_VALUATION.md~~ - 🔒 Confidentiel (disponible uniquement pour les clients sous licence)
4. [features/features_overview.md](features/features_overview.md) - Aperçu des fonctionnalités avec statut
5. [ROADMAP.md](roadmap/roadmap_overview.md) - Feuille de route de développement

### Pour la Conformité & les Audits
1. [compliance/compliance_dashboard.md](compliance/compliance_dashboard.md) - Résumé exécutif de conformité
2. [compliance/compliance_full_checklist.md](compliance/compliance_full_checklist.md) - BSI C5, ISO 27001, RGPD, eIDAS, SOC 2
3. [security/SECURITY_AUDIT_REPORT.md](security/security_audit_report.md) - Résultats de l'audit de sécurité
4. [SECURITY.md](../SECURITY.md) - Politique de divulgation des vulnérabilités
5. [legal/LICENSE_COMPATIBILITY_ANALYSIS.md](legal/LICENSE_COMPATIBILITY_ANALYSIS.md) - ⭐ Compatibilité des licences (v1.3.0)
6. [THIRD_PARTY_LICENSES.md](../THIRD_PARTY_LICENSES.md) - ⭐ Attribution des licences tierces (v1.3.0)

## 📚 Structure de la Documentation

### Documents au Niveau Racine
```
/
├── README.md                        # Aperçu du projet & Démarrage rapide
├── LICENSE                          # Licence MIT avec clause gouvernementale
├── THIRD_PARTY_LICENSES.md          # ⭐ Attribution des licences tierces (v1.3.0)
├── aql/                             # ⭐ Grammaire EBNF d'AQL (v1.3.0)
│   ├── AQL_GRAMMAR.ebnf             # Grammaire formelle complète
│   └── README.md                    # Aperçu d'AQL
├── features/features_overview.md    # Liste des fonctionnalités avec statut
├── ROADMAP.md                       # Feuille de route de développement
├── CHANGELOG.md                     # Historique des modifications
├── guides/guides_build_strategy.md  # Chaîne d'outils & Stratégie de build
├── INTEGRATION_ANALYSIS.md          # Analyse d'intégration Enterprise
├── TEST_REPORT.md                   # Rapport de test complet
├── DEVELOPMENT_AUDITLOG.md          # État du développement & Audit
├── DOCKER_DEPLOYMENT.md             # Guide de déploiement Docker (v1.3.0)
└── CONTRIBUTING.md                  # Directives de contribution
```

### docs/ - Documentation Structurée
```
docs/
├── enterprise/                      # Fonctionnalités Enterprise
│   └── README.md                    # Aperçu & Guide Enterprise
├── performance/                     # Performance & Benchmarks
│   └── ENTERPRISE_SCALABILITY_STRATEGY.md
├── security/                        # Sécurité & Conformité
├── legal/                           # ⭐ Juridique & Licences (v1.3.0)
│   └── LICENSE_COMPATIBILITY_ANALYSIS.md  # Analyse des licences de dépendances
├── architecture/                    # Documentation d'architecture
├── api/                            # Documentation API
└── guides/                         # Guides utilisateur
```

## 🚀 Fonctionnalités Enterprise

### Documentation
| Document | Objectif | Public cible |
|----------|----------|--------------|
| [enterprise/README.md](enterprise/README.md) | Aperçu & Démarrage rapide | Développeurs, DevOps |
| [enterprise/enterprise_scalability.md](enterprise/enterprise_scalability.md) | Détails des fonctionnalités & Exemples de code | Développeurs |
| [enterprise/enterprise_http_pool.md](enterprise/enterprise_http_pool.md) | Implémentation du client HTTP | Développeurs |
| [enterprise/enterprise_final_report.md](enterprise/enterprise_final_report.md) | Résumé de l'implémentation | Parties prenantes |
| [INTEGRATION_ANALYSIS.md](reports/INTEGRATION_ANALYSIS.md) | Intégration héritée | Développeurs |

### Statut
- ✅ **Token Bucket Rate Limiter** - Prêt pour production (5/5 Tests)
- ✅ **Rate Limiter par client** - Prêt pour production (3/3 Tests)
- ✅ **Load Shedder** - Prêt pour production (5/5 Tests)
- ✅ **HTTP Client Pool** - Prêt pour production (6/6 Tests)
- ✅ **Opérations par lot** - Prêt pour production (1/1 Tests)

**Couverture de test :** 20/20 (100%)

## 📖 Architecture & Conception

### Architecture de Base
- [architecture.md](architecture/architecture_overview.md) - Aperçu de l'architecture système
- [storage/storage_rocksdb.md](storage/storage_rocksdb.md) - Disposition de stockage RocksDB
- [mvcc_design.md](architecture/architecture_mvcc.md) - Conception des transactions MVCC
- [query_engine_aql.md](aql/aql_query_engine.md) - Moteur de requête & AQL

### Fonctionnalités Spéciales
- [geo/GEO_ARCHITECTURE.md](geo/geo_architecture.md) - Architecture géographique/spatiale
- [vector_ops.md](features/features_vector_ops.md) - Opérations vectorielles & HNSW
- [content_pipeline.md](architecture/architecture_content_pipeline.md) - Pipeline de traitement de contenu
- [search/hybrid_search_design.md](search/hybrid_search_design.md) - Recherche hybride

## 🔒 Sécurité & Conformité

### Sécurité
- [security/security_overview.md](security/security_overview.md) - Aperçu de la sécurité
- [encryption_strategy.md](security/security_encryption_strategy.md) - Stratégie de chiffrement
- [security/security_key_management.md](security/security_key_management.md) - Gestion des clés
- [security/security_threat_model.md](security/security_threat_model.md) - Modèle de menace
- [security_hardening_guide.md](security/security_hardening.md) - Guide de durcissement

### Conformité
- [compliance/compliance_dashboard.md](compliance/compliance_dashboard.md) - Tableau de bord exécutif
- [compliance/compliance_dpia.md](compliance/compliance_dpia.md) - Analyse d'impact relative à la protection des données (RGPD)
- [compliance/compliance_bcp_drp.md](compliance/compliance_bcp_drp.md) - Continuité d'activité & Reprise après sinistre
- [compliance_audit.md](features/features_compliance_audit.md) - Audit de conformité
- [AUDIT_LOGGING.md](features/features_audit_logging.md) - Journalisation d'audit

### PKI & eIDAS
- [pki_integration_architecture.md](security/security_pki_architecture.md) - Intégration PKI
- [eidas_qualified_signatures.md](security/security_eidas.md) - Signatures eIDAS
- [security/pki_rsa_integration.md](security/security_pki_rsa.md) - Intégration PKI RSA

## 🛠️ Build & Déploiement

### Documentation de Build
- [guides/guides_build_strategy.md](guides/guides_build_strategy.md) - Stratégie & Plateformes de build
- [guides/guides_build.md](guides/guides_build.md) - Instructions détaillées de build

### Déploiement
- [deployment.md](guides/guides_deployment.md) - Stratégies de déploiement
- [DOCKER_MULTI_ARCH_STRATEGY.md](deployment/deployment_docker_multiarch.md) - Docker multi-architecture
- [docs/CI_CD_MULTIARCH.md](deployment/deployment_cicd_multiarch.md) - CI/CD multi-architecture

### Spécifique aux Plateformes
- [ARM_RASPBERRY_PI_BUILD.md](deployment/deployment_arm_build.md) - Build Raspberry Pi
- [ARM_BENCHMARKS.md](deployment/deployment_arm_benchmarks.md) - Performance ARM
- [RASPBERRY_PI_TUNING.md](deployment/deployment_raspberry_tuning.md) - Guide d'optimisation Pi

## 📊 Performance & Benchmarks

- [performance_benchmarks.md](performance/performance_benchmarks.md) - Aperçu de la performance
- [compression_benchmarks.md](performance/performance_compression_benchmarks.md) - Compression
- [encryption_metrics.md](security/security_encryption_metrics.md) - Performance du chiffrement
- [performance/ENTERPRISE_SCALABILITY_STRATEGY.md](enterprise/enterprise_scalability.md) - Stratégie Enterprise

## 🔍 API & Langage de Requête

### AQL (Advanced Query Language)
- [aql_syntax.md](aql/aql_syntax.md) - Syntaxe AQL
- [aql-hybrid-queries.md](aql/aql_hybrid_queries.md) - Requêtes hybrides
- [aql_explain_profile.md](aql/aql_explain_profile.md) - EXPLAIN & PROFILE
- [recursive_path_queries.md](features/features_recursive_path.md) - Chemins récursifs
- [temporal_graphs.md](features/features_temporal_graphs.md) - Graphes temporels

### APIs
- [apis/openapi.md](apis/apis_openapi.md) - REST API & Spécification OpenAPI
- [apis/contentfs_api.md](apis/apis_contentfs.md) - API ContentFS
- [apis/hybrid_search_api.md](apis/apis_hybrid_search.md) - API de recherche hybride

## 👥 SDKs Client

- [clients/javascript_sdk_quickstart.md](clients/clients_javascript_sdk.md) - SDK JavaScript
- [clients/python_sdk_quickstart.md](clients/clients_python_sdk.md) - SDK Python
- [clients/rust_sdk_quickstart.md](clients/clients_rust_sdk.md) - SDK Rust

## 📝 Développement

### Directives
- [development/developers.md](development/developers.md) - Guide du développeur
- [code_quality.md](guides/guides_code_quality.md) - Pipeline de qualité du code
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Directives de contribution

### Statut & Planification
- [DEVELOPMENT_AUDITLOG.md](development/DEVELOPMENT_SUMMARY.md) - Audit de développement
- [development/implementation_status.md](development/implementation_status.md) - Statut
- [development/ROADMAP.md](development/ROADMAP.md) - Feuille de route
- [development/priorities.md](development/priorities.md) - Priorités

### Implémentations API
- [development/audit_api_implementation.md](development/audit_api_implementation.md) - API Audit
- [development/saga_api_implementation.md](development/saga_api_implementation.md) - API SAGA

## 🔗 Ressources Externes

### GitHub
- **Dépôt :** https://github.com/makr-code/ThemisDB
- **Wiki :** https://github.com/makr-code/ThemisDB/wiki
- **Issues :** https://github.com/makr-code/ThemisDB/issues

### Badges
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
- [![Security CI](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml)
- [![GPU CI](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml)

## 📋 Navigation par Thème

### Modules Source (16 Composants)

| Module | README | Source | Headers | LOC |
|--------|--------|--------|---------|-----|
| analytics | [docs/observability/README.md](observability/README.md) | 2 | 3 | 3,742 |
| cache | [docs/storage/README.md](storage/README.md) | 1 | 6 | 492 |
| cdc | [docs/cdc/README.md](cdc/README.md) | 1 | 1 | 510 |
| content | [docs/content/README.md](content/README.md) | 15 | 16 | 9,091 |
| geo | [docs/geo/README.md](geo/README.md) | 3 | 2 | 304 |
| governance | [docs/governance/README.md](governance/README.md) | 1 | 1 | 259 |
| index | [docs/search/README.md](search/README.md) | 11 | 12 | 14,629 |
| llm | [docs/llm/README.md](llm/README.md) | 2 | 2 | 679 |
| query | [docs/query/README.md](query/README.md) | 12 | 12 | 12,560 |
| replication | [docs/storage/README.md](storage/README.md) | 1 | 2 | 1,612 |
| security | [docs/security/README.md](security/README.md) | 16 | 16 | 8,138 |
| server | [docs/server/README.md](server/README.md) | 20 | 20 | 18,282 |
| sharding | [docs/sharding/README.md](sharding/README.md) | 19 | 21 | 12,278 |
| storage | [docs/storage/README.md](storage/README.md) | 10 | 9 | 4,591 |
| timeseries | [docs/timeseries/README.md](timeseries/README.md) | 8 | 7 | 2,767 |
| transaction | [docs/architecture/README.md](architecture/README.md) | 2 | 2 | 895 |

**Total :** 124 fichiers source, 132 fichiers header, 90 829 LOC

**Rapport d'audit :** [SOURCE_CODE_AUDIT.md](development/SOURCE_CODE_AUDIT.md)

### Fonctionnalités Multi-Modèles
- **Graphe :** [property_graph_model.md](features/features_property_graph.md), [graph_index.cpp.md](src/search/graph_index.cpp.md)
- **Géo/Spatial :** [GEO_ARCHITECTURE.md](geo/geo_architecture.md), [geo_acceleration_3d_games.md](geo/geo_acceleration_3d_games.md)
- **Séries temporelles :** [time_series.md](features/features_time_series.md), [timeseries/continuous_agg.cpp.md](src/timeseries/continuous_agg.cpp.md)
- **Document :** [content_pipeline.md](architecture/architecture_content_pipeline.md), [content/content_manager.cpp.md](src/content/content_manager.cpp.md)
- **Vecteur/Embedding :** [vector_ops.md](features/features_vector_ops.md), [gnn_embeddings.md](features/features_gnn_embeddings.md)

### Stockage & Persistance
- **RocksDB :** [storage/storage_rocksdb.md](storage/storage_rocksdb.md), [storage/rocksdb_wrapper.cpp.md](src/storage/rocksdb_wrapper.cpp.md)
- **MVCC :** [mvcc_design.md](architecture/architecture_mvcc.md)
- **Transactions :** [transactions.md](features/features_transactions.md), [transaction/saga.cpp.md](src/transaction/saga.cpp.md)
- **Compression :** [compression_strategy.md](performance/performance_compression_strategy.md), [timeseries/gorilla.cpp.md](src/timeseries/gorilla.cpp.md)

### Recherche & Indexation
- **Texte intégral :** [search/fulltext_api.md](search/fulltext_api.md), [search/stemming.md](search/stemming.md)
- **Recherche hybride :** [search/hybrid_search_design.md](search/hybrid_search_design.md)
- **Recherche vectorielle :** [vector_ops.md](features/features_vector_ops.md), [index/vector_index.cpp.md](src/search/vector_index.cpp.md)
- **Indexation géographique :** [geo/cpu_backend.cpp.md](src/geo/cpu_backend.cpp.md)

### Gouvernance & DCP
- **Détection DCP :** [security/pii_detection.md](security/security_pii_detection.md), [pii_api.md](security/security_pii_api.md)
- **Politiques :** [security/security_policies.md](security/security_policies.md), [governance/policy_engine.cpp.md](src/governance/policy_engine.cpp.md)
- **RBAC :** [rbac_authorization.md](guides/guides_rbac.md), [RBAC.md](guides/guides_rbac.md)
- **Rétention :** [security/audit_and_retention.md](security/security_audit_retention.md)

## ⚠️ Déprécié / Archive

Documentation obsolète ou remplacée :
- [archive/](archive/) - Documents archivés
- [reports/](reports/) - Rapports de fusion Git

## 🔄 Synchronisation

### Synchronisation Wiki
```powershell
./sync-wiki.ps1
```

### Aperçu Local
```powershell
./build-docs.ps1  # MkDocs → site/
```

### GitHub Pages
- **Primaire :** GitHub Wiki (faisant autorité)
- **Secondaire :** Build MkDocs (pour le développement)

## 📞 Support

- **Issues :** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Discussions :** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Sécurité :** Voir [SECURITY.md](../SECURITY.md)

---

**Statut de la documentation :** ✅ Consolidé (5 décembre 2025)  
**Responsable :** Équipe ThemisDB  
**Dernier audit :** 5 décembre 2025
