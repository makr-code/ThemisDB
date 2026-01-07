# ThemisDB Dokumentations-Benennungskonvention

**Version:** 1.0  
**Erstellt:** 2. Dezember 2025  
**Status:** ✅ IMPLEMENTIERT

## Einheitliche Dateistruktur pro Komponente

Jeder Dokumentations-Unterordner in `/docs` sollte folgende Dateien enthalten:

### Pflicht-Dateien

| Datei | Zweck | Beschreibung |
|-------|-------|--------------|
| `README.md` | Übersicht | Einstiegspunkt mit Kurzübersicht, Source-Code-Referenz |

### Standard-Dateien (Präfix = Ordnername)

| Datei | Zweck | Beschreibung |
|-------|-------|--------------|
| `{prefix}_overview.md` | Konzept | Detaillierte Konzeptbeschreibung |
| `{prefix}_architecture.md` | Architektur | Technische Architektur, Diagramme |
| `{prefix}_implementation.md` | Implementierung | Implementierungsdetails, Code-Beispiele |
| `{prefix}_api.md` | API-Referenz | HTTP/REST/gRPC API-Dokumentation |
| `{prefix}_config.md` | Konfiguration | Konfigurationsoptionen, YAML/JSON-Beispiele |
| `{prefix}_security.md` | Sicherheit | Sicherheitsaspekte, Berechtigungen |
| `{prefix}_performance.md` | Performance | Benchmarks, Tuning-Tipps |
| `{prefix}_troubleshooting.md` | Fehlerbehebung | Häufige Probleme und Lösungen |
| `{prefix}_roadmap.md` | Roadmap | Geplante Features und Verbesserungen |
| `{prefix}_changelog.md` | Änderungshistorie | Versionshistorie dieser Komponente |

## Durchgeführte Umbenennungen

### docs/sharding/
| Alt | Neu |
|-----|-----|
| `RAID_REDUNDANCY_ARCHITECTURE.md` | `sharding_redundancy.md` |
| `STREAMING_ARCHITECTURE.md` | `sharding_streaming.md` |
| `SHARDING_UNIFIED_DOCUMENTATION.md` | `sharding_overview.md` |
| `horizontal_scaling_strategy.md` | `sharding_strategy.md` |
| `implementation_summary.md` | `sharding_implementation.md` |

### docs/security/
| Alt | Neu |
|-----|-----|
| `INCIDENT_RESPONSE_PLAN.md` | `security_incident_response.md` |
| `encryption_strategy.md` | `security_encryption_strategy.md` |
| `hardening_guide.md` | `security_hardening.md` |
| `threat_model.md` | `security_threat_model.md` |
| ... (37 Dateien umbenannt) | ... |

### docs/performance/
| Alt | Neu |
|-----|-----|
| `GPU_ACCELERATION_PLAN.md` | `performance_gpu_plan.md` |
| `memory_tuning.md` | `performance_memory.md` |
| `benchmarks.md` | `performance_benchmarks.md` |
| ... (12 Dateien umbenannt) | ... |

### docs/architecture/
| Alt | Neu |
|-----|-----|
| `mvcc_design.md` | `architecture_mvcc.md` |
| `base_entity.md` | `architecture_base_entity.md` |
| `content_pipeline.md` | `architecture_content_pipeline.md` |
| ... (9 Dateien umbenannt) | ... |

### docs/aql/
| Alt | Neu |
|-----|-----|
| `syntax.md` | `aql_syntax.md` |
| `explain_profile.md` | `aql_explain_profile.md` |
| `query_engine.md` | `aql_query_engine.md` |
| ... (9 Dateien umbenannt) | ... |

### docs/storage/
| Alt | Neu |
|-----|-----|
| `rocksdb_layout.md` | `storage_rocksdb.md` |
| `CLOUD_BLOB_BACKENDS.md` | `storage_cloud_backends.md` |
| ... (5 Dateien umbenannt) | ... |

### docs/query/
| Alt | Neu |
|-----|-----|
| `VECTOR_HYBRID_SEARCH.md` | `query_vector_hybrid.md` |
| `FILTERED_VECTOR_SEARCH.md` | `query_filtered_vector.md` |
| ... (4 Dateien umbenannt) | ... |

### docs/geo/
| Alt | Neu |
|-----|-----|
| `architecture.md` | `geo_architecture.md` |
| `geo_integration_readme.md` | `geo_integration.md` |
| ... (3 Dateien umbenannt) | ... |

### docs/features/ (NEU)
| Alt | Neu |
|-----|-----|
| `audit_logging.md` | `features_audit_logging.md` |
| `vector_ops.md` | `features_vector_ops.md` |
| `transactions.md` | `features_transactions.md` |
| `indexes.md` | `features_indexes.md` |
| ... (31 Dateien umbenannt) | ... |

### docs/guides/ (NEU)
| Alt | Neu |
|-----|-----|
| `deployment.md` | `guides_deployment.md` |
| `tls_setup.md` | `guides_tls_setup.md` |
| `rbac.md` | `guides_rbac.md` |
| ... (11 Dateien umbenannt) | ... |

### docs/content/ (NEU)
| Alt | Neu |
|-----|-----|
| `ingestion.md` | `content_ingestion.md` |
| `search_api.md` | `content_search_api.md` |
| ... (9 Dateien umbenannt) | ... |

### docs/compliance/ (NEU)
| Alt | Neu |
|-----|-----|
| `DPIA.md` | `compliance_dpia.md` |
| `BCP_DRP.md` | `compliance_bcp_drp.md` |
| ... (5 Dateien umbenannt) | ... |

### docs/observability/ (NEU)
| Alt | Neu |
|-----|-----|
| `prometheus_metrics.md` | `observability_prometheus.md` |
| `tracing.md` | `observability_tracing.md` |
| ... (4 Dateien umbenannt) | ... |

### docs/clients/ (NEU)
| Alt | Neu |
|-----|-----|
| `javascript_sdk_quickstart.md` | `clients_javascript_sdk.md` |
| `python_sdk_quickstart.md` | `clients_python_sdk.md` |
| ... (6 Dateien umbenannt) | ... |

### docs/apis/ (NEU)
| Alt | Neu |
|-----|-----|
| `openapi.md` | `apis_openapi.md` |
| `graphql.md` | `apis_graphql.md` |
| ... (4 Dateien umbenannt) | ... |

### docs/policies/ (NEU)
| Alt | Neu |
|-----|-----|
| `ACCESS_CONTROL_POLICY.md` | `policies_access_control.md` |
| `DATA_CLASSIFICATION_POLICY.md` | `policies_data_classification.md` |
| ... (4 Dateien umbenannt) | ... |

## Cross-References

Jede README.md sollte "Verwandte Dokumentation" Links enthalten:
```markdown
## Verwandte Dokumentation

- [Architektur](./component_architecture.md)
- [API-Referenz](./component_api.md)
- [Übergeordnet: Feature X](../features/x.md)
```
