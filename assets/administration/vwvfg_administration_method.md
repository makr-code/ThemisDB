# vwvfg_administration_method.yaml — Detailerklärung

## Zweck
Dieses Prozessmodell definiert einen rechtlich gebundenen Verwaltungs-Workflow nach VwVfG-Prinzipien innerhalb des ThemisDB-Methodenrahmens.

## Rollenverständnis
- **Nicht**: automatische Ersetzung menschlicher Rechtsentscheidung.
- **Doch**: strukturierte, auditierbare Entscheidungsunterstützung mit Fail-Closed-Gates.

## Jurisdiktionsprofil
- `bund_default` als Basisprofil.
- Länder-Overlays sind erlaubt.
- Für bindende Entscheidungen muss ein passendes Overlay geprüft/aktiviert sein.

## Prozessphasen
1. **Zuständigkeit und Anwendbarkeit**  
   Verifiziert Behördenzuständigkeit und passenden Verfahrensrahmen.
2. **Verfahrensdesign und Anhörung**  
   Plant Beteiligtenrechte und Anhörung mit prüfbarer Protokollierung.
3. **Sachverhalt und Beweislage**  
   Bewertet Tatsachenbasis inkl. Widerspruchslage.
4. **Verhältnismäßigkeit und Ermessen**  
   Prüft Geeignetheit/Erforderlichkeit/Angemessenheit und Ermessensgrenzen.
5. **Form, Fristen und Begründung**  
   Validiert Formalien, Fristen und tragfähige Begründung.
6. **Governance und Nachvollziehbarkeit**  
   Erzwingt Gate-Vollständigkeit und vollständige Traceability.
7. **Entscheidung, Erlass und Monitoring**  
   Dokumentiert Entscheidung inkl. Rechtsgrundlage und Post-Decision-Monitoring.

## Harte Blocker (Beispiele)
- ungeklärte Zuständigkeit,
- fehlende Anhörung trotz Pflicht,
- Fristverletzung,
- fehlende oder unzureichende Begründung,
- ungeklärte formelle Rechtmäßigkeit.

## Observability- und Evidenzfelder
Pflichtfelder für auditable Ausführung:
- `knowledge_source_class`
- `wiki_context_refs`
- `jurisdiction_profile`
- `legal_basis_refs`
- `hearing_event_refs`
- `reasoning_packet_ref`

## Ergebnisgrenzen
- Zulässige Entscheidungen: `no_go`, `research_only`, `promotion_candidate`
- Ohne geschlossene Gates bleibt Ergebnis mindestens `research_only`.
