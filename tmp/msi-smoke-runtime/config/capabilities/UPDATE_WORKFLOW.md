# Capability Update History - Hamburg Bauamt Shard

Dieses Dokument zeigt beispielhaft, wie die Capability-Konfiguration eines Shards
über die Zeit fortgeschrieben wird (wie ein "lebendiges Dokument").

## Version History

### v1.4.3 - 2026-02-10
**Änderungen:**
- Neue Keywords hinzugefügt: `renovation_permits`, `sanierungsgenehmigungen`
- Data type `renovation_permits` hinzugefügt
- Performance verbessert: avg_query_time_ms: 50 → 45ms

**Git Diff:**
```diff
 last_updated: "2026-02-10T10:15:30Z"
-version: "1.4.2"
+version: "1.4.3"

   data_types:
     - building_permits
+    - renovation_permits
     - construction_plans

   keywords:
     - baurecht
     - baugenehmigung
+    - sanierungsgenehmigungen
+    - renovation_permits

 metadata:
-  avg_query_time_ms: 50
+  avg_query_time_ms: 45
```

**Trigger:** Query-Log-Analyse zeigte häufige Suchen nach "Sanierungsgenehmigung"

---

### v1.4.2 - 2026-02-05
**Änderungen:**
- Sub-capability `processing_stages` hinzugefügt
- 3 neue Bezirksämter als Organizations ergänzt
- Embeddings neu generiert

**Git Diff:**
```diff
 last_updated: "2026-02-05T14:22:18Z"
-version: "1.4.1"
+version: "1.4.2"

   organizations:
     - hamburg_bauamt
     - hamburg_stadtplanung
+    - bezirksamt_nord
+    - bezirksamt_mitte
+    - bezirksamt_wandsbek

+  sub_capabilities:
+    processing_stages:
+      - eingegangen
+      - in_pruefung
+      - genehmigt
+      - abgelehnt

 embeddings:
-  last_generated: "2026-01-15T08:00:00Z"
+  last_generated: "2026-02-05T14:00:00Z"
```

**Trigger:** Organisationsstruktur-Änderung nach Bezirksreform

---

### v1.4.1 - 2026-01-22
**Änderungen:**
- Keywords optimiert (generische Keywords entfernt)
- Cache hit ratio verbessert: 0.68 → 0.73
- Quality indicators aktualisiert

**Git Diff:**
```diff
 last_updated: "2026-01-22T09:11:45Z"
-version: "1.4.0"
+version: "1.4.1"

   keywords:
-    - dokument      # zu generisch, entfernt
-    - datei         # zu generisch, entfernt
     - baurecht
     - baugenehmigung

 metadata:
-  cache_hit_ratio: 0.68
+  cache_hit_ratio: 0.73

 quality_indicators:
-  completeness_score: 0.93
+  completeness_score: 0.95
```

**Trigger:** Performance-Review und Keyword-Bereinigung

---

### v1.4.0 - 2026-01-15
**Änderungen:**
- Major Update: `building_types` sub-capability hinzugefügt
- 15 neue technische Keywords
- Metadata-Struktur erweitert

**Git Diff:**
```diff
 last_updated: "2026-01-15T16:30:00Z"
-version: "1.3.2"
+version: "1.4.0"

+  sub_capabilities:
+    building_types:
+      - wohngebaeude
+      - gewerbegebaeude
+      - industriegebaeude
+      - oeffentliche_gebaeude
+      - landwirtschaftliche_gebaeude

   keywords:
     - baurecht
     - baugenehmigung
+    - wohnflaeche
+    - geschossflaeche
+    - grundflaechenzahl
+    - geschossflaechenzahl
+    - firsthoehe
+    - traufhoehe
     [... 9 weitere Keywords]

+metadata:
+  common_query_patterns:
+    - "baugenehmigung hamburg *"
+    - "bebauungsplan * hamburg"
```

**Trigger:** Feature-Request für granularere Gebäudetyp-Suche

---

## Wie Updates durchgeführt werden

### 1. Automatische Updates (täglich)
```bash
#!/bin/bash
# scripts/update-capabilities-metadata.sh

# Performance-Metriken aktualisieren
themis-admin capabilities update-metrics --shard $SHARD_ID

# Timestamp aktualisieren
yq eval -i ".last_updated = \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"" \
  config/capabilities/${SHARD_ID}.yaml

# Committen wenn Änderungen vorhanden
git add config/capabilities/${SHARD_ID}.yaml
git commit -m "Auto-update: Metrics for ${SHARD_ID}" || true
```

### 2. Manuelle Updates (bei Bedarf)
```bash
# 1. Datei bearbeiten
vim config/capabilities/shard_hamburg_bauamt_001.yaml

# 2. Version erhöhen (gemäß Semantic Versioning)
# MAJOR.MINOR.PATCH

# 3. Timestamp aktualisieren
yq eval -i ".last_updated = \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"" \
  config/capabilities/shard_hamburg_bauamt_001.yaml

# 4. Committen
git add config/capabilities/shard_hamburg_bauamt_001.yaml
git commit -m "feat: Add renovation permits to Hamburg Bauamt capabilities"

# 5. Embeddings neu generieren (wenn Keywords/Domains geändert)
themis-admin capabilities generate-embeddings \
  --shard shard_hamburg_bauamt_001
```

### 3. Keyword-Discovery aus Query-Logs
```bash
# Analyse durchführen
themis-admin capabilities analyze-queries \
  --shard shard_hamburg_bauamt_001 \
  --period 7d \
  --min-frequency 50

# Gibt aus:
# Suggested keywords (appeared 50+ times in queries not matching well):
# - sanierungsgenehmigung (127 times, avg match score: 0.42)
# - denkmalschutz (89 times, avg match score: 0.38)
# - energieausweis (76 times, avg match score: 0.51)
```

## Git Workflow

### Branch Strategy
```
main
  └── capabilities/update-hamburg-bauamt-keywords
```

### Commit Message Convention
```
feat: Add new capability/keyword
fix: Correct capability definition
perf: Improve matching performance
docs: Update capability documentation
chore: Update metadata/timestamps
```

### Pull Request Template
```markdown
## Capability Update

**Shard:** shard_hamburg_bauamt_001
**Version:** 1.4.2 → 1.4.3
**Type:** Keywords addition

### Changes
- Added keywords: sanierungsgenehmigungen, renovation_permits
- Added data_type: renovation_permits

### Reason
Query log analysis showed 127 searches for "Sanierungsgenehmigung" in the last 7 days
with low match scores (avg: 0.42). Adding these keywords should improve routing.

### Testing
- [ ] Keywords validated against query logs
- [ ] Embeddings regenerated
- [ ] Test queries verified: "Sanierungsgenehmigung Hamburg" → score > 0.8
- [ ] No regression in existing queries

### Deployment
- [ ] Sync to production via CI/CD
- [ ] Monitor match scores for 24h
- [ ] Rollback plan documented
```

## Monitoring & Alerting

### Metrics to Watch
```yaml
# alerts.yaml
alerts:
  - name: LowCapabilityMatchScore
    condition: avg_match_score < 0.6
    window: 1h
    action: notify_team
    
  - name: StaleEmbeddings
    condition: days_since_embedding_update > 30
    action: auto_regenerate
    
  - name: HighUnmatchedQueryRate
    condition: unmatched_queries_ratio > 0.15
    window: 1h
    action: trigger_keyword_analysis
```

## Lessons Learned

### Best Practices
✅ **DO:**
- Update version on every change (even small)
- Keep git history clean and descriptive
- Run keyword analysis monthly
- Regenerate embeddings after keyword changes
- Test changes with sample queries before deploying

❌ **DON'T:**
- Change structure without version bump
- Add too generic keywords
- Skip embedding regeneration
- Forget to update timestamp
- Directly edit production without git workflow

### Common Pitfalls
1. **Keywords zu generisch** → Shard matched zu viele irrelevante Queries
2. **Embeddings veraltet** → Semantic matching funktioniert schlecht
3. **Keine Query-Log-Analyse** → Neue Keywords werden nicht entdeckt
4. **Version nicht erhöht** → Sync-Probleme in verteilten Systemen

## Tools & Scripts

### Capability Management CLI
```bash
# Capability-Datei validieren
themis-admin capabilities validate \
  --file config/capabilities/shard_hamburg_bauamt_001.yaml

# Keywords vorschlagen
themis-admin capabilities suggest-keywords \
  --shard shard_hamburg_bauamt_001 \
  --source query-logs \
  --days 7

# Embedding-Status prüfen
themis-admin capabilities check-embeddings \
  --all \
  --stale-threshold 30d

# Synchronisieren
themis-admin capabilities sync \
  --file config/capabilities/shard_hamburg_bauamt_001.yaml \
  --target production
```

---

**Zusammenfassung:** Capability-Konfigurationen sind "lebende Dokumente", die kontinuierlich
an neue Anforderungen, Query-Patterns und Organisationsstrukturen angepasst werden.
Der Git-basierte Workflow ermöglicht Versionierung, Review und Rollback.
