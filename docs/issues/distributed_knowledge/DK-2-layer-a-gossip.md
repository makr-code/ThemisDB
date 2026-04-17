---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "module:sharding", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 2+3
---

# [DK-2] distributed_knowledge: Layer A — Adapter-Gossip-Integration

## Aufgabe

Verbinde `AdapterCapabilityAnnouncement` mit dem laufenden `GossipProtocol` und
dem `AdaptiveShardRouter`, sodass jeder Shard seinen domänenspezialisiertesten
LoRA-Adapter via Gossip ankündigt und der Router Anfragen automatisch dorthin
lenkt — **ohne Training, sofortiger Routing-Gewinn**.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `GossipProtocol::registerCustomHandler()` neue API | Änderungen am Gossip-Transportprotokoll |
| `handleMessage()` dispatch für `message_type="adapter_capability"` | Neue Gossip-Felder für andere Module |
| `AdaptiveShardRouter::updateAdapterCapability()` | ML-basierte Routing-Scores |
| `AdaptiveShardRouter::routeByDomain()` | Multi-Shard Fan-out Routing (→ DK-4) |
| 7 neue Tests (gossip: 3, router: 4) | Performance-Benchmarks (→ DK-8) |
| `GossipAdapterPublisher` an echten `GossipProtocol` koppeln | RLAIF oder LoRA-Training (→ DK-3/5) |

## Idee / Konzept

Der `GossipProtocol` ist bereits für Heartbeat- und Peer-Update-Nachrichten zuständig.
Er kennt bisher keinen Mechanismus für modulspezifische Payloads. Eine generische
`registerCustomHandler(type, fn)`-API öffnet den Gossip-Bus für alle Module —
das erste Nutzungsbeispiel ist `adapter_capability`.

```
Shard 7 hat SECURITY_MONITOR-Adapter mit accuracy_delta = +0.12
  ↓
AdapterRegistry → GossipAdapterPublisher.announce()
  ↓  payload: {message_type: "adapter_capability", domain_type: "SECURITY_MONITOR", ...}
GossipProtocol.broadcastMessage()
  ↓  empfangen auf allen Shards
GossipProtocol.handleMessage() → registrierter Handler
  ↓
AdaptiveShardRouter.updateAdapterCapability("shard-7", announcement)
  ↓
AQL-Query mit domain_hint="SECURITY_MONITOR"
  ↓
AdaptiveShardRouter.routeByDomain(SECURITY_MONITOR) → "shard-7"
```

**Kein Training erforderlich** — der Routing-Gewinn ist sofort nach dem ersten
Gossip-Zyklus (typisch 100–500 ms) aktiv.

## Technische Details

### Sub-Issue 2a — GossipProtocol: Custom Handler API

**Datei:** `include/sharding/gossip_protocol.h` + `src/sharding/gossip_protocol.cpp`

```cpp
// Neue öffentliche API
void registerCustomHandler(
    const std::string& message_type,
    std::function<void(const GossipMessage&)> handler
);
```

- Handler wird in `handleMessage()` vor den bestehenden type-Checks aufgerufen
- Duplicate `message_type`: letzter Handler gewinnt, Log-Warning
- Thread-safe: Handler-Map unter bestehendem Mutex

**Neue Tests in `tests/test_gossip_protocol.cpp`:**
- `GP-CUSTOM-01` registerCustomHandler() dispatcht korrekt bei passendem message_type
- `GP-CUSTOM-02` unbekannter message_type ruft keinen Handler auf
- `GP-CUSTOM-03` duplicate registration überschreibt und loggt Warning

### Sub-Issue 2b — AdaptiveShardRouter: Domain Scoring

**Datei:** `include/sharding/adaptive_shard_router.h` + `src/sharding/adaptive_shard_router.cpp`

```cpp
// Neue öffentliche APIs
void updateAdapterCapability(
    const std::string& shard_id,
    const AdapterCapabilityAnnouncement& announcement
);

std::string routeByDomain(AdapterDomainType domain) const;

double getAdapterAccuracyDelta(
    const std::string& shard_id,
    AdapterDomainType domain
) const; // für FederatedRAGMerger (→ DK-4)
```

**Score-Map:** `domain_type → sorted_list<{accuracy_delta, shard_id}>`  
**Fallback:** bei keinem Score für Domain → default `route()` Verhalten

**Neue Tests in `tests/test_adaptive_shard_router.cpp`:**
- `ASR-DOM-01` updateAdapterCapability() + routeByDomain() wählt Shard mit höchstem accuracy_delta
- `ASR-DOM-02` routeByDomain() fällt auf default-Routing zurück wenn kein Score vorhanden
- `ASR-DOM-03` höherer accuracy_delta überschreibt niedrigeren für gleichen Shard
- `ASR-DOM-04` getAdapterAccuracyDelta() gibt 0.0 zurück für unbekannte Shard/Domain-Kombination

### Verkabelung

```cpp
// In GossipProtocol-Setup (z.B. ShardManager oder DI-Container)
gossip.registerCustomHandler("adapter_capability", [&router](const GossipMessage& msg) {
    auto announcement = AdapterCapabilityAnnouncement::fromJson(msg.payload);
    router.updateAdapterCapability(msg.sender_id, announcement);
});

// In AdapterRegistry (nach jedem Training-Loop oder Timer)
publisher.announce(AdapterCapabilityAnnouncement{
    .adapter_version = registry.currentVersion(),
    .domain_type     = AdapterDomainType::SECURITY_MONITOR,
    .accuracy_delta  = metrics.accuracyDelta(),
    .training_samples = trainer.sampleCount()
});
```

## Abhängigkeiten

- **Vorbedingung:** DK-1 (kompilierendes Modul + Unit-Tests)
- **Blockiert:** DK-4 (braucht `getAdapterAccuracyDelta()` für RAG-Boost)

## Erfolgskriterien

- [ ] `GossipProtocol::registerCustomHandler()` API vorhanden und dokumentiert
- [ ] `handleMessage()` dispatcht `adapter_capability`-Nachrichten korrekt
- [ ] `AdaptiveShardRouter::updateAdapterCapability()` aktualisiert Domain-Score-Map
- [ ] `AdaptiveShardRouter::routeByDomain()` wählt Shard mit höchstem `accuracy_delta`
- [ ] `getAdapterAccuracyDelta()` gibt korrekten Wert zurück
- [ ] `GossipAdapterPublisher` nutzt registrierte Handler-Funktion als Gossip-Transport
- [ ] 3 neue Gossip-Tests grün
- [ ] 4 neue Router-Tests grün
- [ ] Keine Regressions in bestehenden Gossip- und Router-Tests
- [ ] Quick Win nachweisbar: Security-Query landet auf SECURITY_MONITOR-Shard

## Definition of Done

Domain-aware Routing funktioniert in einem In-Process-Test mit 3 Mock-Shards:
Shard 7 mit `SECURITY_MONITOR`-Ankündigung erhält 100 % der `domain_hint=SECURITY_MONITOR`-Queries.
