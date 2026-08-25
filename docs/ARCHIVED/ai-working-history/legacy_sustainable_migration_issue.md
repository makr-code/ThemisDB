## Ziel
Legacy-/Fallback-/Stub-Pfade mit Produktionsrisiko nachhaltig abbauen und auf kanonische Implementierungen umstellen (fail-closed, keine stillen Placeholder-Pfade in produktiven Flows).

## Kontext
Die priorisierte Liste stammt aus [ai_working/active_stubs.txt](ai_working/active_stubs.txt) (Einträge #279-#321) und fokussiert auf kritische/hohe Risiken sowie strukturelle Altpfade.

## Priorisierte Arbeitspakete

### P0 - Kritisch (Sicherheit / Korrektheit)
- [ ] Stub #279: Distributed TX Phase-2 ohne Remote-RPC entfernen
  - Datei: src/transaction/distributed_transaction_manager.cpp
  - Problem: Callback-lose Teilnehmer erhalten kein COMMIT/ABORT
  - Ziel: verbindliches RPC/mTLS Decision-Fanout für alle Remote-Teilnehmer
- [ ] Stub #302: Voice API Bearer-Token-Prüfung auf echte JWT/OIDC-Validierung umstellen
  - Datei: src/server/voice_api_handler.cpp
  - Problem: non-empty Token reicht aktuell aus
  - Ziel: Signatur, Expiry, Issuer, Audience, Revocation, Claims prüfen

### P1 - Hoch (Auth / API-Funktionalität / Datenintegrität)
- [ ] Stub #280: ROPE allow-all Fallback gegen echte RBAC-Prüfung ersetzen
  - Datei: src/server/rope_api_handler.cpp
- [ ] Stub #281 + #284: Wire-Protocol 501/503 Fallbacks abbauen
  - Dateien: src/themis/wire_protocol_server.cpp, src/network/wire_protocol_server.cpp
  - Ziel: QueryEngine/Geo/TS/Graph verbindlich injizieren; keine NOT_INTEGRATED-Pfade im produktiven Protokoll
- [ ] Stub #289 + #290: LoRA-Trainings-Placeholder (synthetisches Modell + Pseudo-AllReduce) entfernen
  - Dateien: src/llm/lora_framework/lora_training_service.cpp, src/llm/lora_framework/distributed_trainer.cpp
- [ ] Stub #300: restoreCollections von Voll-Restore auf echtes per-CF SST-Ingest umstellen
  - Datei: src/storage/backup_manager.cpp
- [ ] Stub #305: SSE keep-alive von buffered/sync auf echte asynchrone Stream-Lifecycle umstellen
  - Datei: src/server/changefeed_api_handler.cpp
- [ ] Stub #310: UNSIGNED-Signaturfallback fail-closed machen
  - Datei: src/sharding/auto_rebalancer.cpp

### P2 - Mittel (Betrieb, Observability, API-Verhalten)
- [ ] Stub #297, #301, #303, #304, #306, #307, #308, #309, #311
  - Dateien: src/llm/feedback_store.cpp, src/server/timeseries_api_handler.cpp, src/llm/llm_model_storage.cpp, src/llm/lora_framework/lora_feedback_storage.cpp, src/server/oauth2_provider.cpp, src/server/rope_api_handler.cpp, src/server/voice_api_handler.cpp, src/llm/gpu_memory_manager.cpp, src/sharding/paxos_state_persistence.cpp

### P3 - Cloud Backup Provider Placeholder-Pfade konsolidiert
- [ ] Stub #312-#321 als ein Paket behandeln
  - Datei: src/sharding/cloud_backup.cpp
  - Ziel: Azure/S3/GCS Upload/Download/Delete/List/Exists produktiv via SDK oder klaren injected Backends implementieren; keine Placeholder no-op Pfade

## Verbindliche Akzeptanzkriterien
- [ ] Keine produktiven Legacy-/Fallback-Pfade ohne explizite Human-Freigabe und Markierung gemäß COPILOT_INSTRUCTIONS
- [ ] Kritische Security-Pfade fail-closed (kein stilles Allow/Fallback)
- [ ] Für ersetzte Stub-Pfade: Unit- und Integrations-Tests inkl. Fehlerfällen (Auth fail, Netzwerkfehler, Timeout, NotFound)
- [ ] Wire-/HTTP-Endpoints liefern reale Funktion statt 501/503 Placeholder, wenn Feature laut Build/Config aktiviert ist
- [ ] Observability ergänzt (Metriken/Logs) für neue produktive Pfade
- [ ] ROADMAP/FUTURE_ENHANCEMENTS pro betroffenem Modul aktualisiert

## Umsetzungsvorschlag (inkrementell)
1. Security-Blocker zuerst: #302, #280, #310
2. Distributed correctness: #279, #300
3. Protocol completeness: #281, #284, #305
4. LLM training integrity: #289, #290
5. Cloud backup consolidation: #312-#321
6. Restliche Mittel-Priorität bündeln

## Hinweise
- Dieses Issue ist bewusst als Meta-Issue angelegt. Sub-Issues pro Arbeitspaket können zur Parallelisierung erstellt und hier verlinkt werden.
- Quelle der priorisierten Stubs: [ai_working/active_stubs.txt](ai_working/active_stubs.txt)
