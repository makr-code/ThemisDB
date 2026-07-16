"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            schema.py                                          ║
  Module:          tools/themis_config_editor                         ║
  Description:     Declarative tab/field schema for the config editor. ║
                   Each entry in TAB_SCHEMA is (tab_title, [FieldDef])║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

from typing import List, Tuple

from .models import FieldDef, FieldType

# ---------------------------------------------------------------------------
# Helper alias for brevity
# ---------------------------------------------------------------------------
_S = FieldType.SECTION
_STR = FieldType.STRING
_INT = FieldType.INT
_FLT = FieldType.FLOAT
_BOOL = FieldType.BOOL
_ENUM = FieldType.ENUM
_PATH = FieldType.PATH
_TEXT = FieldType.TEXT

# ---------------------------------------------------------------------------
# TAB_SCHEMA
# Each element: (display_name, [FieldDef, ...])
# ---------------------------------------------------------------------------

TAB_SCHEMA: List[Tuple[str, List[FieldDef]]] = [

    # =========================================================================
    # 1. Allgemein (General) — most-used settings at a glance
    # =========================================================================
    ("Allgemein", [
        FieldDef("", "Server", _S),
        FieldDef("server.host", "Bind-Adresse",
                 _STR, "0.0.0.0",
                 tooltip="Netzwerk-Interface, auf dem der Server lauscht. 0.0.0.0 = alle Interfaces."),
        FieldDef("server.port", "HTTP-Port",
                 _INT, 8765,
                 tooltip="Haupt-HTTP-API-Port (REST/AQL).", unit=""),
        FieldDef("server.worker_threads", "Worker-Threads",
                 _INT, 0,
                 tooltip="0 = Auto-Erkennung (CPU-Cores)."),
        FieldDef("server.max_connections", "Max. Verbindungen",
                 _INT, 100,
                 tooltip="Maximale gleichzeitige TCP-Verbindungen."),
        FieldDef("server.request_timeout_ms", "Request-Timeout",
                 _INT, 30000, unit="ms",
                 tooltip="Zeitlimit pro HTTP-Request in Millisekunden."),

        FieldDef("", "Storage", _S),
        FieldDef("storage.rocksdb_path", "RocksDB-Pfad",
                 _PATH, "./data/rocksdb", path_is_dir=True,
                 tooltip="Verzeichnis, in dem RocksDB die Datenbankdateien ablegt."),
        FieldDef("storage.memtable_size_mb", "Memtable-Größe",
                 _INT, 256, unit="MB",
                 tooltip="Schreibpuffer pro Column-Family (64–512 MB empfohlen)."),
        FieldDef("storage.block_cache_size_mb", "Block-Cache",
                 _INT, 1024, unit="MB",
                 tooltip="Lesepuffer für alle Column-Families (512–4096 MB empfohlen)."),

        FieldDef("", "Logging", _S),
        FieldDef("logging.level", "Log-Level",
                 _ENUM, "INFO",
                 choices=["DEBUG", "INFO", "WARN", "ERROR"],
                 tooltip="Globaler Log-Level des Servers."),
        FieldDef("logging.file", "Log-Datei",
                 _PATH, "./logs/themis_server.log", path_is_dir=False,
                 tooltip="Pfad zur Log-Datei (wird automatisch rotiert)."),
    ]),

    # =========================================================================
    # 2. Storage / RocksDB
    # =========================================================================
    ("Storage / RocksDB", [
        FieldDef("", "Pfade", _S),
        FieldDef("storage.rocksdb_path", "Datenbankpfad",
                 _PATH, "./data/rocksdb", path_is_dir=True,
                 tooltip="Hauptverzeichnis für RocksDB SSTable-Dateien."),
        FieldDef("storage.wal_dir", "WAL-Verzeichnis",
                 _PATH, "", path_is_dir=True,
                 tooltip="Separates Verzeichnis für Write-Ahead-Log (leer = im rocksdb_path)."),

        FieldDef("", "Speicher-Konfiguration", _S),
        FieldDef("storage.memtable_size_mb", "Memtable-Größe",
                 _INT, 256, unit="MB",
                 tooltip="Größe des Schreibpuffers je Column-Family."),
        FieldDef("storage.block_cache_size_mb", "Block-Cache",
                 _INT, 1024, unit="MB",
                 tooltip="Gemeinsamer Lesepuffer aller Column-Families."),
        FieldDef("storage.block_cache_shard_bits", "Cache-Shard-Bits",
                 _INT, -1,
                 tooltip="-1 = automatisch. 4 = 16 Shards, 6 = 64 Shards."),
        FieldDef("storage.cache_index_and_filter_blocks", "Index/Filter im Cache",
                 _BOOL, True,
                 tooltip="Index- und Filterblöcke im Block-Cache vorhalten."),
        FieldDef("storage.pin_l0_filter_and_index_blocks_in_cache", "L0-Blöcke pinnen",
                 _BOOL, True,
                 tooltip="L0-Index/Filter-Blöcke dauerhaft im Cache halten (weniger Eviction)."),
        FieldDef("storage.high_pri_pool_ratio", "High-Priority-Pool-Anteil",
                 _FLT, 0.5,
                 tooltip="Anteil des Caches für Index/Filter-Blöcke (0.0–1.0)."),
        FieldDef("storage.db_write_buffer_size_mb", "DB-Write-Buffer gesamt",
                 _INT, 0, unit="MB",
                 tooltip="Gesamtlimit aller Memtables in MB (0 = unbegrenzt)."),

        FieldDef("", "Schreib-Konfiguration", _S),
        FieldDef("storage.enable_wal", "WAL aktivieren",
                 _BOOL, True,
                 tooltip="Write-Ahead-Log für Durability. Nur für Benchmarks deaktivieren!"),
        FieldDef("storage.max_write_buffer_number", "Max. Write-Buffer",
                 _INT, 3,
                 tooltip="Anzahl Memtables je Column-Family."),
        FieldDef("storage.min_write_buffer_number_to_merge", "Min. Buffer zum Merge",
                 _INT, 1,
                 tooltip="Mindestanzahl Memtables, die vor dem Flush zusammengefasst werden."),
        FieldDef("storage.allow_concurrent_memtable_write", "Paralleles Schreiben",
                 _BOOL, True,
                 tooltip="Gleichzeitige Schreiboperationen in verschiedene Memtables."),
        FieldDef("storage.enable_pipelined_write", "Pipelined Writes",
                 _BOOL, True,
                 tooltip="Schreiboperationen über mehrere Threads bündeln."),
        FieldDef("storage.write_policy", "Write-Policy",
                 _ENUM, "WritePrepared",
                 choices=["WriteCommitted", "WritePrepared", "WriteUnprepared"],
                 tooltip="Transaktions-Schreibstrategie. WritePrepared empfohlen."),
        FieldDef("storage.two_write_queues", "Zwei Schreib-Queues",
                 _BOOL, True,
                 tooltip="Separate Queues für Prepare/Commit-Phase (weniger Contention)."),

        FieldDef("", "BlobDB (Großwerte)", _S),
        FieldDef("storage.enable_blobdb", "BlobDB aktivieren",
                 _BOOL, True,
                 tooltip="Große Werte außerhalb des LSM-Trees speichern."),
        FieldDef("storage.blob_size_threshold", "Blob-Schwellenwert",
                 _INT, 4096, unit="Bytes",
                 tooltip="Werte größer als dieser Schwellenwert landen in BlobDB."),

        FieldDef("", "Kompaktierung", _S),
        FieldDef("storage.max_background_jobs", "Max. Hintergrund-Jobs",
                 _INT, 4,
                 tooltip="Anzahl paralleler Compaction/Flush-Threads (2–16 je nach CPU)."),
        FieldDef("storage.max_subcompactions", "Sub-Compactions",
                 _INT, 1,
                 tooltip="Parallele Sub-Compactions je Job (1 = deaktiviert)."),
        FieldDef("storage.use_universal_compaction", "Universal Compaction",
                 _BOOL, False,
                 tooltip="Universal statt Leveled Compaction (besser für Write-Heavy)."),
        FieldDef("storage.dynamic_level_bytes", "Dynamische Level-Bytes",
                 _BOOL, True,
                 tooltip="Level-Größen dynamisch an die Datenmenge anpassen."),
        FieldDef("storage.target_file_size_base_mb", "Ziel-SSTable-Größe",
                 _INT, 64, unit="MB",
                 tooltip="Zielgröße für SSTable-Dateien."),
        FieldDef("storage.max_bytes_for_level_base_mb", "Max. Level-Base-Bytes",
                 _INT, 256, unit="MB",
                 tooltip="Größe von Level 1 (nachfolgende Levels ×10)."),
        FieldDef("storage.level0_file_num_compaction_trigger", "L0-Compaction-Trigger",
                 _INT, 4,
                 tooltip="Anzahl L0-Dateien, ab der Compaction startet."),
        FieldDef("storage.level0_slowdown_writes_trigger", "L0-Slowdown-Trigger",
                 _INT, 20,
                 tooltip="Anzahl L0-Dateien, ab der Schreibvorgänge gebremst werden."),
        FieldDef("storage.level0_stop_writes_trigger", "L0-Stop-Trigger",
                 _INT, 36,
                 tooltip="Anzahl L0-Dateien, ab der Schreibvorgänge gestoppt werden."),

        FieldDef("", "Bloom-Filter & I/O", _S),
        FieldDef("storage.bloom_bits_per_key", "Bloom-Filter Bits/Key",
                 _INT, 10,
                 tooltip="Höher = weniger Falsch-Positive, mehr Speicher. 10 ≈ 1% FPR."),
        FieldDef("storage.use_direct_reads", "Direct I/O (Reads)",
                 _BOOL, False,
                 tooltip="OS-Page-Cache für Lesevorgänge umgehen (nützlich bei großem Block-Cache)."),
        FieldDef("storage.use_direct_io_for_flush_and_compaction", "Direct I/O (Flush/Compact)",
                 _BOOL, False,
                 tooltip="OS-Page-Cache für Flush und Compaction umgehen."),

        FieldDef("", "Kompression", _S),
        FieldDef("storage.compression.default", "Standard-Kompression",
                 _ENUM, "lz4",
                 choices=["none", "lz4", "lz4hc", "zstd", "snappy", "zlib", "bzip2"],
                 tooltip="Kompressionsalgorithmus für alle Levels. lz4 = schnell + gut."),
        FieldDef("storage.compression.bottommost", "Bottommost-Kompression",
                 _ENUM, "zstd",
                 choices=["none", "lz4", "lz4hc", "zstd", "snappy", "zlib", "bzip2"],
                 tooltip="Kompression für den untersten Level (kalte Daten). zstd = beste Ratio."),

        FieldDef("", "TTL (Time-To-Live)", _S),
        FieldDef("storage.enable_ttl", "TTL aktivieren",
                 _BOOL, False,
                 tooltip="Automatisches Löschen von Daten nach Ablauf der TTL."),
        FieldDef("storage.ttl_seconds", "TTL-Dauer",
                 _INT, 0, unit="s",
                 tooltip="Daten älter als dieser Wert werden automatisch gelöscht (0 = deaktiviert)."),
    ]),

    # =========================================================================
    # 3. Server & Netzwerk
    # =========================================================================
    ("Server & Netzwerk", [
        FieldDef("", "HTTP-Server", _S),
        FieldDef("server.host", "Bind-Adresse", _STR, "0.0.0.0",
                 tooltip="Netzwerk-Interface (0.0.0.0 = alle, 127.0.0.1 = nur lokal)."),
        FieldDef("server.port", "HTTP-Port", _INT, 8765,
                 tooltip="REST/AQL-API-Port."),
        FieldDef("server.worker_threads", "Worker-Threads", _INT, 0,
                 tooltip="0 = Auto. Empfehlung: Anzahl CPU-Cores."),
        FieldDef("server.max_request_size_mb", "Max. Request-Größe", _INT, 10, unit="MB",
                 tooltip="Maximale Größe einer einzelnen HTTP-Anfrage."),
        FieldDef("server.request_timeout_ms", "Request-Timeout", _INT, 30000, unit="ms",
                 tooltip="Zeitlimit pro HTTP-Request in Millisekunden."),
        FieldDef("server.max_connections", "Max. Verbindungen", _INT, 100,
                 tooltip="Maximale Anzahl gleichzeitiger TCP-Verbindungen."),

        FieldDef("", "TLS / HTTPS", _S),
        FieldDef("server.enable_tls", "TLS aktivieren", _BOOL, False,
                 tooltip="HTTPS-Verschlüsselung aktivieren."),
        FieldDef("server.tls_cert_path", "Zertifikat (PEM)", _PATH, "",
                 tooltip="Pfad zum Server-Zertifikat im PEM-Format."),
        FieldDef("server.tls_key_path", "Privater Schlüssel (PEM)", _PATH, "",
                 tooltip="Pfad zum privaten Schlüssel im PEM-Format."),
        FieldDef("server.tls_ca_cert_path", "CA-Zertifikat (mTLS)", _PATH, "",
                 tooltip="CA-Zertifikat für Client-Zertifikatsverifizierung (mutual TLS)."),
        FieldDef("server.tls_require_client_cert", "Client-Zertifikat erforderlich", _BOOL, False,
                 tooltip="Mutual TLS: Clients müssen ein Zertifikat vorlegen."),
        FieldDef("server.tls_min_version", "Min. TLS-Version",
                 _ENUM, "TLSv1.3", choices=["TLSv1.2", "TLSv1.3"],
                 tooltip="Minimale TLS-Protokollversion. TLSv1.3 empfohlen."),

        FieldDef("", "Erweiterte Protokolle", _S),
        FieldDef("server.enable_http2", "HTTP/2 aktivieren", _BOOL, False,
                 tooltip="HTTP/2-Protokoll (erfordert TLS mit ALPN)."),
        FieldDef("server.enable_http3", "HTTP/3 (QUIC) aktivieren", _BOOL, False,
                 tooltip="HTTP/3-Protokoll über QUIC/UDP."),
        FieldDef("server.http3_port", "HTTP/3-Port", _INT, 0,
                 tooltip="UDP-Port für HTTP/3 (0 = gleicher Port wie HTTP)."),
        FieldDef("server.enable_websocket", "WebSocket aktivieren", _BOOL, False,
                 tooltip="WebSocket-Protokoll für bidirektionale Verbindungen."),

        FieldDef("", "Health/Error-Service", _S),
        FieldDef("server.health_error_service_enabled", "Health-Service aktivieren", _BOOL, True,
                 tooltip="Separater Dienst für Health-Checks und Fehlerdiagnose."),
        FieldDef("server.health_error_service_bind_address", "Health-Service-Adresse",
                 _STR, "127.0.0.1",
                 tooltip="Bind-Adresse des Health-Service (Standard: nur lokal erreichbar)."),
        FieldDef("server.health_error_service_port", "Health-Service-Port", _INT, 9090,
                 tooltip="Port des Health-Service (Standard: 9090)."),

        FieldDef("", "SSE / CDC-Streaming", _S),
        FieldDef("sse.max_events_per_second", "Max. Events/Sekunde", _INT, 0,
                 tooltip="Server-seitiges Rate-Limit für SSE-Streams (0 = unbegrenzt)."),
        FieldDef("sse.max_buffered_events", "Max. gepufferte Events", _INT, 1000,
                 tooltip="Puffergröße je SSE-Verbindung (1000–10000 je nach RAM)."),
        FieldDef("sse.heartbeat_interval_ms", "Heartbeat-Intervall", _INT, 15000, unit="ms",
                 tooltip="Intervall für Keep-Alive-Heartbeats bei SSE."),
        FieldDef("sse.event_poll_interval_ms", "Event-Poll-Intervall", _INT, 500, unit="ms",
                 tooltip="Wie oft der Changefeed auf neue Events geprüft wird."),
        FieldDef("sse.drop_oldest_on_overflow", "Älteste Events bei Überlauf verwerfen",
                 _BOOL, True,
                 tooltip="Wenn der Puffer voll ist: älteste (True) oder neueste (False) Events verwerfen."),
    ]),

    # =========================================================================
    # 4. Sicherheit (Security)
    # =========================================================================
    ("Sicherheit", [
        FieldDef("", "HSM — Hardware Security Module", _S),
        FieldDef("hsm.provider", "HSM-Provider",
                 _ENUM, "stub",
                 choices=["stub", "pkcs11", "aws_kms", "azure_keyvault", "gcp_kms", "vault"],
                 tooltip="Schlüsselverwaltung. 'stub' = NUR für Entwicklung! In Produktion aws_kms, pkcs11 etc. verwenden."),

        FieldDef("", "PKCS#11 (On-Premises HSM)", _S),
        FieldDef("hsm.pkcs11.library_path", "PKCS#11-Library", _PATH, "",
                 tooltip="Pfad zur .so/.dll-Datei des HSM-Treibers (z.B. SoftHSM2, Luna, AWS CloudHSM)."),
        FieldDef("hsm.pkcs11.slot_id", "Slot-ID", _INT, 0,
                 tooltip="HSM-Slot (meist 0 für Single-Slot-Geräte)."),
        FieldDef("hsm.pkcs11.token_label", "Token-Label", _STR, "",
                 tooltip="Token-Label für Multi-Token-Setups (optional)."),
        FieldDef("hsm.pkcs11.key_label", "Schlüssel-Label", _STR, "themis-master-key",
                 tooltip="Name des Master-Schlüssels im HSM."),
        FieldDef("hsm.pkcs11.session_pool_size", "Session-Pool", _INT, 4,
                 tooltip="Anzahl gleichzeitiger PKCS#11-Sessions."),

        FieldDef("", "AWS KMS", _S),
        FieldDef("hsm.aws_kms.region", "AWS-Region", _STR, "us-east-1",
                 tooltip="AWS-Region des KMS-Schlüssels."),
        FieldDef("hsm.aws_kms.key_id", "KMS-Key-ARN oder Alias", _STR, "",
                 tooltip="Vollständiger ARN oder Alias des KMS-Schlüssels."),

        FieldDef("", "Azure Key Vault", _S),
        FieldDef("hsm.azure_keyvault.vault_url", "Vault-URL", _STR, "",
                 tooltip="URL des Azure Key Vault, z.B. https://myvault.vault.azure.net/"),
        FieldDef("hsm.azure_keyvault.key_name", "Schlüsselname", _STR, "themis-master-key",
                 tooltip="Name des Schlüssels im Azure Key Vault."),
        FieldDef("hsm.azure_keyvault.auth_method", "Auth-Methode",
                 _ENUM, "managed_identity",
                 choices=["managed_identity", "service_principal", "cli"],
                 tooltip="Azure-Authentifizierungsmethode."),

        FieldDef("", "HashiCorp Vault", _S),
        FieldDef("hsm.vault.address", "Vault-Adresse", _STR, "https://vault.example.com:8200",
                 tooltip="URL des HashiCorp Vault-Servers."),
        FieldDef("hsm.vault.mount_path", "Transit-Mount-Pfad", _STR, "transit",
                 tooltip="Pfad des Transit-Engines in Vault."),
        FieldDef("hsm.vault.key_name", "Schlüsselname", _STR, "themis-master-key",
                 tooltip="Name des Transit-Schlüssels in Vault."),
        FieldDef("hsm.vault.auth_method", "Auth-Methode",
                 _ENUM, "token",
                 choices=["token", "approle", "kubernetes", "aws"],
                 tooltip="Vault-Authentifizierungsmethode."),
        FieldDef("hsm.vault.tls_verify", "TLS-Verifizierung", _BOOL, True,
                 tooltip="TLS-Zertifikat des Vault-Servers prüfen. Nur für Tests deaktivieren!"),

        FieldDef("", "Feld-Verschlüsselung", _S),
        FieldDef("field_encryption.enabled", "Feld-Verschlüsselung aktivieren", _BOOL, True,
                 tooltip="Felder in Dokumenten einzeln verschlüsseln."),
        FieldDef("field_encryption.algorithm", "Verschlüsselungsalgorithmus",
                 _ENUM, "AES-256-GCM",
                 choices=["AES-256-GCM", "AES-128-GCM", "ChaCha20-Poly1305"],
                 tooltip="Algorithmus für die Feld-Verschlüsselung. AES-256-GCM empfohlen."),

        FieldDef("", "Zugriffskontrolle", _S),
        FieldDef("access_control.rbac_enabled", "RBAC aktivieren", _BOOL, True,
                 tooltip="Role-Based Access Control für alle Operationen erzwingen."),
        FieldDef("access_control.require_authentication", "Authentifizierung erforderlich",
                 _BOOL, True,
                 tooltip="Alle API-Anfragen müssen authentifiziert sein."),
        FieldDef("access_control.session_timeout", "Session-Timeout", _INT, 60, unit="min",
                 tooltip="Inaktive Sessions werden nach diesem Zeitraum ungültig."),

        FieldDef("", "TLS (Security-Datei)", _S),
        FieldDef("tls.enabled", "TLS aktivieren", _BOOL, True,
                 tooltip="TLS für alle Verbindungen aktivieren (Eintrag aus security.yaml)."),
        FieldDef("tls.cert_file", "Zertifikat", _PATH, "./certs/server.crt",
                 tooltip="Pfad zum Server-Zertifikat (PEM)."),
        FieldDef("tls.key_file", "Privater Schlüssel", _PATH, "./certs/server.key",
                 tooltip="Pfad zum privaten Schlüssel (PEM)."),
        FieldDef("tls.ca_file", "CA-Zertifikat", _PATH, "./certs/ca.crt",
                 tooltip="CA-Zertifikat für Client-Verifikation (mTLS)."),
        FieldDef("tls.min_version", "Min. TLS-Version",
                 _ENUM, "1.3", choices=["1.2", "1.3"],
                 tooltip="Minimale TLS-Protokollversion."),
        FieldDef("tls.require_client_cert", "Client-Zertifikat erforderlich", _BOOL, False,
                 tooltip="Mutual TLS: Client-Zertifikate erzwingen."),
    ]),

    # =========================================================================
    # 5. Features & Flags
    # =========================================================================
    ("Features & Flags", [
        FieldDef("", "Kernfunktionen", _S),
        FieldDef("features.semantic_cache", "Semantischer Cache", _BOOL, False,
                 tooltip="Abfrageergebnisse auf Basis semantischer Ähnlichkeit cachen."),
        FieldDef("features.llm_store", "LLM-Store", _BOOL, False,
                 tooltip="Embedding- und Modell-Management aktivieren."),
        FieldDef("features.llm_query_enhancement", "LLM-Query-Enhancement", _BOOL, False,
                 tooltip="LLM-Kontext in Abfragen einbeziehen (Enterprise-Feature)."),
        FieldDef("features.cdc", "Change Data Capture (CDC)", _BOOL, False,
                 tooltip="Echtzeit-Replikation über Änderungs-Streams."),
        FieldDef("features.timeseries", "Zeitreihen-Support", _BOOL, False,
                 tooltip="Zeitreihen-Datenhaltung und -abfragen aktivieren."),
        FieldDef("features.pii_manager", "PII-Manager", _BOOL, False,
                 tooltip="Persistente Verwaltung von personenbezogenen Datenmappings."),
        FieldDef("features.update_checker", "Update-Checker", _BOOL, False,
                 tooltip="Automatisch auf neue ThemisDB-Versionen prüfen."),

        FieldDef("", "Datenhaltungsrichtlinien (Retention)", _S),
        FieldDef("features.retention.enabled", "Retention aktivieren", _BOOL, False,
                 tooltip="Automatische Datenlöschung nach Ablauf der Haltedauer aktivieren."),
        FieldDef("features.retention.interval_hours", "Prüf-Intervall", _INT, 24, unit="h",
                 tooltip="Wie oft die Retention-Richtlinien geprüft und angewendet werden."),
        FieldDef("features.retention.policies_path", "Richtlinien-Datei", _PATH,
                 "./config/retention_policies.yaml",
                 tooltip="Pfad zur YAML-Datei mit den Retention-Richtlinien."),
    ]),

    # =========================================================================
    # 6. Performance & Limits
    # =========================================================================
    ("Performance & Limits", [
        FieldDef("", "Performance-Tuning", _S),
        FieldDef("performance.enable_statistics", "RocksDB-Statistiken", _BOOL, True,
                 tooltip="Detaillierte RocksDB-Metriken sammeln (leichter Overhead)."),
        FieldDef("performance.optimize_for_point_lookup", "Point-Lookup-Optimierung",
                 _BOOL, False,
                 tooltip="Für punktuelle Lesevorgänge optimieren (auf Kosten von Range-Scans)."),
        FieldDef("storage.enable_high_parallel_tuning", "High-Parallel-Tuning", _BOOL, False,
                 tooltip="Automatische Optimierung für Systeme mit vielen CPU-Cores."),
        FieldDef("storage.high_parallel_thread_threshold", "Thread-Schwellenwert", _INT, 16,
                 tooltip="Ab dieser Thread-Anzahl greift das High-Parallel-Tuning."),

        FieldDef("", "API-Rate-Limiting", _S),
        FieldDef("rate_limiting.audit_rate_limit_per_minute", "Audit-API-Limit", _INT, 100,
                 unit="req/min",
                 tooltip="Maximale Audit-API-Anfragen pro Minute (0 = unbegrenzt)."),
        FieldDef("rate_limiting.bucket_capacity", "Bucket-Kapazität", _INT, 100,
                 tooltip="Token-Bucket-Kapazität (maximaler Burst)."),
        FieldDef("rate_limiting.refill_rate", "Refill-Rate", _INT, 100,
                 unit="tokens/min",
                 tooltip="Anzahl Tokens, die pro Minute nachgefüllt werden."),
        FieldDef("rate_limiting.per_ip_enabled", "Per-IP-Limiting", _BOOL, True,
                 tooltip="Rate-Limiting pro IP-Adresse aktivieren."),
        FieldDef("rate_limiting.per_user_enabled", "Per-User-Limiting", _BOOL, True,
                 tooltip="Rate-Limiting pro Benutzer aktivieren (erfordert Authentifizierung)."),

        FieldDef("", "Rate-Limiter V2 (empfohlen)", _S),
        FieldDef("rate_limiting.v2.enabled", "Rate-Limiter V2 aktivieren", _BOOL, False,
                 tooltip="Erweiterte Variante mit Priority-Lanes für VIP-Clients."),
        FieldDef("rate_limiting.v2.capacity_per_client", "Kapazität/Client", _INT, 100,
                 tooltip="Token-Bucket-Kapazität pro Client."),
        FieldDef("rate_limiting.v2.refill_rate_per_client", "Refill-Rate/Client", _INT, 10,
                 unit="tokens/s",
                 tooltip="Token-Nachfüllrate pro Client und Sekunde."),
        FieldDef("rate_limiting.v2.max_clients", "Max. Clients", _INT, 10000,
                 tooltip="Maximale Anzahl gleichzeitig verfolgter Clients."),
        FieldDef("rate_limiting.v2.enable_priority_lanes", "Priority-Lanes", _BOOL, True,
                 tooltip="HIGH/NORMAL/LOW Prioritäts-Lanes für unterschiedliche Clients."),
        FieldDef("rate_limiting.v2.high_capacity", "HIGH-Kapazität", _INT, 2000,
                 tooltip="Token-Kapazität für VIP-Clients (HIGH-Lane)."),
        FieldDef("rate_limiting.v2.high_refill_rate", "HIGH-Refill", _INT, 200,
                 unit="tokens/s",
                 tooltip="Token-Nachfüllrate für VIP-Clients."),

        FieldDef("", "Multi-Tenancy", _S),
        FieldDef("tenants.enforce_quotas", "Quoten erzwingen", _BOOL, False,
                 tooltip="Ressourcenquoten pro Tenant aktivieren."),
        FieldDef("tenants.tenant_header", "Tenant-HTTP-Header", _STR, "X-Tenant-ID",
                 tooltip="HTTP-Header zur Identifikation des Tenants."),
        FieldDef("tenants.max_tenants", "Max. Tenants", _INT, 1000,
                 tooltip="Maximale Anzahl registrierter Tenants."),
        FieldDef("tenants.default_tenant_id", "Standard-Tenant-ID", _STR, "default",
                 tooltip="Tenant-ID für Anfragen ohne explizite Angabe."),
        FieldDef("tenants.allow_default_tenant", "Standard-Tenant erlauben", _BOOL, True,
                 tooltip="Anfragen ohne Tenant-ID-Header zulassen."),
        FieldDef("tenants.default_quotas.max_concurrent_queries", "Max. gleichz. Abfragen",
                 _INT, 100,
                 tooltip="Maximale parallele Abfragen pro Tenant."),
        FieldDef("tenants.default_quotas.max_connections", "Max. Verbindungen/Tenant",
                 _INT, 50,
                 tooltip="Maximale gleichzeitige Verbindungen pro Tenant."),
        FieldDef("tenants.default_quotas.requests_per_second", "Anfragen/Sekunde/Tenant",
                 _INT, 1000,
                 tooltip="Request-Rate-Limit pro Tenant."),
    ]),

    # =========================================================================
    # 7. Vektor-Index
    # =========================================================================
    ("Vektor-Index", [
        FieldDef("", "Allgemein", _S),
        FieldDef("vector_index.object_name", "Object-Name (Namespace)", _STR, "documents",
                 tooltip="Namespace des Vektor-Index (leer = kein Auto-Init)."),
        FieldDef("vector_index.dimension", "Vektordimension", _INT, 768,
                 tooltip="Dimensionalität der Einbettungsvektoren (z.B. 384, 768, 1536)."),
        FieldDef("vector_index.metric", "Distanzmetrik",
                 _ENUM, "COSINE",
                 choices=["COSINE", "L2", "DOT"],
                 tooltip="COSINE = Kosinus-Ähnlichkeit (empfohlen), L2 = Euklidisch, DOT = Skalarprodukt."),
        FieldDef("vector_index.engine", "Index-Engine",
                 _ENUM, "hnsw",
                 choices=["hnsw"],
                 tooltip="Indexalgorithmus. HNSW = Hierarchical Navigable Small World."),

        FieldDef("", "HNSW-Parameter", _S),
        FieldDef("vector_index.hnsw_m", "HNSW M",
                 _INT, 16,
                 tooltip="Verbindungen pro Knoten. Höher = bessere Trefferquote, mehr Speicher (8–64)."),
        FieldDef("vector_index.hnsw_ef_construction", "ef_construction",
                 _INT, 200,
                 tooltip="Build-Zeit-Parameter. Höher = bessere Qualität, langsamerer Aufbau (100–800)."),
        FieldDef("vector_index.ef_search", "ef_search",
                 _INT, 64,
                 tooltip="Suchzeit-Parameter. Höher = bessere Trefferquote, langsamer (50–200)."),
        FieldDef("vector_index.max_elements", "Max. Vektoren",
                 _INT, 100000,
                 tooltip="Maximale Anzahl Vektoren im Index (Speicher wird vorab reserviert)."),

        FieldDef("", "GPU-Beschleunigung", _S),
        FieldDef("vector_index.use_gpu", "GPU-Beschleunigung", _BOOL, False,
                 tooltip="CUDA/FAISS-GPU für Vektoroperationen verwenden."),

        FieldDef("", "Persistenz", _S),
        FieldDef("vector_index.auto_save", "Auto-Save", _BOOL, True,
                 tooltip="Index beim Herunterfahren automatisch speichern."),
        FieldDef("vector_index.save_path", "Speicherpfad", _PATH, "./data/vector_indexes",
                 path_is_dir=True,
                 tooltip="Verzeichnis, in dem der Vektor-Index gespeichert wird."),
        FieldDef("vector_index.save_on_shutdown", "Beim Herunterfahren speichern", _BOOL, True,
                 tooltip="Index beim regulären Server-Shutdown speichern."),
        FieldDef("vector_index.load_on_startup", "Beim Start laden", _BOOL, True,
                 tooltip="Gespeicherten Index beim Serverstart automatisch laden."),
    ]),

    # =========================================================================
    # 8. Logging & Tracing
    # =========================================================================
    ("Logging & Tracing", [
        FieldDef("", "Server-Logging", _S),
        FieldDef("logging.level", "Log-Level",
                 _ENUM, "INFO",
                 choices=["DEBUG", "INFO", "WARN", "ERROR"],
                 tooltip="Globaler Log-Level des Servers."),
        FieldDef("logging.file", "Log-Datei",
                 _PATH, "./logs/themis_server.log",
                 tooltip="Pfad zur Server-Log-Datei."),
        FieldDef("logging.max_size_mb", "Max. Log-Dateigröße", _INT, 100, unit="MB",
                 tooltip="Maximale Größe der Log-Datei vor der Rotation."),
        FieldDef("logging.max_files", "Max. rotierte Dateien", _INT, 5,
                 tooltip="Anzahl alter Log-Dateien, die aufbewahrt werden."),

        FieldDef("", "Audit-Logging", _S),
        FieldDef("audit.enabled", "Audit-Log aktivieren", _BOOL, True,
                 tooltip="Sicherheitsrelevante Operationen in einem Audit-Log protokollieren."),
        FieldDef("audit.log_path", "Audit-Log-Pfad", _PATH, "./logs/audit.log",
                 tooltip="Pfad zur Audit-Log-Datei."),
        FieldDef("audit.retention_days", "Aufbewahrungsdauer", _INT, 365, unit="Tage",
                 tooltip="Audit-Logs werden nach diesem Zeitraum automatisch gelöscht."),
        FieldDef("audit.log_sensitive_operations", "Sensitive Operationen loggen",
                 _BOOL, True,
                 tooltip="Auch HSM-Zugriffe, Schlüsselrotation etc. im Audit-Log erfassen."),

        FieldDef("", "Distributed Tracing (OpenTelemetry)", _S),
        FieldDef("tracing.enabled", "Tracing aktivieren", _BOOL, False,
                 tooltip="Verteiltes Tracing via OpenTelemetry/OTLP aktivieren."),
        FieldDef("tracing.service_name", "Service-Name", _STR, "themis-server",
                 tooltip="Service-Name in den Traces (erscheint in Jaeger/Grafana Tempo)."),
        FieldDef("tracing.otlp_endpoint", "OTLP-Endpoint", _STR,
                 "http://localhost:4318",
                 tooltip="URL des OpenTelemetry-Collectors (z.B. Jaeger, Grafana Tempo)."),
    ]),

    # =========================================================================
    # 9. Rohbearbeitung (Raw Editor) — rendered by RawEditorTab, not BaseTab
    #    Empty list: the tab class handles its own UI.
    # =========================================================================
    ("Rohbearbeitung", []),
]
