> **Hinweis:** Testfixture/Demo-Dokument – kein produktiver Sourcebezug erforderlich.

---
{
  "version": "1.0",
  "antrag_typ": "BAUGENEHMIGUNG",
  "rechtsgrundlage": "§ 63 BauO NRW",
  "ozg_dienst_id": "DE-NW-BAUGENEHMIGUNG",
  "xoev_standard": "XBAU",
  "aktenzeichen": "BAUAMT-KN-2026-0042",
  "antragsteller": {
    "vorname": "Hans",
    "nachname": "Mustermann",
    "geburtsdatum": "19800315",
    "ags": "05315000",
    "adresse": "Musterstraße 1, 50667 Köln",
    "eid_tx_id": "TX-BAU-2026-001",
    "eid_server": "eid-server-bund-01",
    "eid_assurance": "HIGH"
  },
  "bauvorhaben": {
    "bezeichnung": "Neubau Wohngebäude",
    "nutzungsart": "Wohngebäude",
    "standort": "Musterstraße 1, 50667 Köln",
    "flaeche_qm": 320,
    "geschosse": 3,
    "baugenehmigung_erforderlich": true,
    "grundstueck": "Flur 12, Flurstück 345, Gemarkung Altstadt-Nord"
  },
  "unterlagen": [
    { "id": "DOC-BAU-001", "typ": "LAGEPLAN",             "bezeichnung": "Lageplan M 1:1000",               "pflicht": true  },
    { "id": "DOC-BAU-002", "typ": "BAUZEICHNUNGEN",       "bezeichnung": "Grundrisse, Schnitte, Ansichten", "pflicht": true  },
    { "id": "DOC-BAU-003", "typ": "BAUBESCHREIBUNG",      "bezeichnung": "Technische Baubeschreibung",      "pflicht": true  },
    { "id": "DOC-BAU-004", "typ": "STANDSICHERHEITSNACHWEIS", "bezeichnung": "Statische Berechnungen",      "pflicht": true  },
    { "id": "DOC-BAU-005", "typ": "BRANDSCHUTZNACHWEIS",  "bezeichnung": "Brandschutzkonzept",              "pflicht": true  }
  ],
  "xoev_xml_vorlage": {
    "wurzelelement": "xbau",
    "felder": {
      "antragsteller": "Hans Mustermann",
      "bauvorhaben":   "Neubau Wohngebäude",
      "standort":      "Musterstraße 1, 50667 Köln"
    }
  }
}
---

# Bauantrag — Neubau Wohngebäude

**Aktenzeichen:** BAUAMT-KN-2026-0042  
**Rechtsgrundlage:** § 63 BauO NRW  
**Bearbeitendes Amt:** Bauamt Köln  
**Datum:** 01.06.2026

---

## 1 · Antragsteller

| Feld         | Wert                         |
|--------------|------------------------------|
| Name         | Hans Mustermann              |
| Geburtsdatum | 15.03.1980                   |
| Adresse      | Musterstraße 1, 50667 Köln   |
| AGS          | 05315000                     |
| eID-Server   | eid-server-bund-01           |
| Assurance    | HIGH (BSI TR-03130)          |

---

## 2 · Bauvorhaben

| Feld               | Wert                                               |
|--------------------|----------------------------------------------------|
| Bezeichnung        | Neubau Wohngebäude                                 |
| Nutzungsart        | Wohngebäude (§ 2 Abs. 3 BauO NRW)                  |
| Standort           | Musterstraße 1, 50667 Köln                         |
| Grundstück         | Flur 12, Flurstück 345, Gemarkung Altstadt-Nord    |
| Grundfläche        | 320 m²                                             |
| Geschosse          | 3 Vollgeschosse (EG + OG1 + OG2)                   |
| Genehmigungspflichtig | Ja (vereinfachtes Verfahren § 63 BauO NRW)      |

---

## 3 · Beizufügende Unterlagen

| Nr.  | Bezeichnung                           | Pflicht |
|------|---------------------------------------|---------|
| 1    | Lageplan M 1:1000                     | ✅ Ja   |
| 2    | Grundrisse, Schnitte, Ansichten       | ✅ Ja   |
| 3    | Technische Baubeschreibung            | ✅ Ja   |
| 4    | Statische Berechnungen (Standsicherheit) | ✅ Ja |
| 5    | Brandschutzkonzept                    | ✅ Ja   |

---

## 4 · Beteiligte Fachbehörden

- **Denkmalschutzbehörde NRW** — Stellungnahme zum Denkmalschutz (30 Tage Frist)
- **Umweltamt Köln** — Stellungnahme Gewässer- und Bodenschutz (30 Tage Frist)
- **Feuerwehr Köln / Brandschutzamt** — Brandschutzstellungnahme (21 Tage Frist)

---

## 5 · Ablauf (§ 63 BauO NRW — Vereinfachtes Verfahren)

1. **Antragstellung** — Einreichung mit vollständigen Unterlagen via OZG-Dienst
2. **Vollständigkeitsprüfung** — Bauamt prüft Unterlagen (§ 65 BauO NRW)
3. **Fachbehörden-Beteiligung** — Gleichzeitige Versendung an Denkmalschutz, Umweltamt, Feuerwehr
4. **Entscheidung** — Aggregation der Stellungnahmen, Bescheid (Frist: 3 Monate)
5. **Bescheiderteilung** — Zustellung per XDOMEA / OZG

---

## 6 · Erklärungen des Antragstellers

Der Antragsteller versichert, dass alle Angaben vollständig und wahrheitsgemäß sind.
Die Unterlagen entsprechen den Anforderungen der Bauvorlagenverordnung NRW.

---

*Dieser Antrag wurde elektronisch gestellt. Authentifizierung erfolgte über die eID (BSI TR-03130, eIDAS-Niveau HOCH).*
