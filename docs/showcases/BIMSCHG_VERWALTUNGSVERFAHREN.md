---
Author: "ThemisDB Team"
Created: "2026-11-19"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level3
SOT_Domain: e-government-showcase
Canonical_References:
  - DOCUMENTATION_GOVERNANCE.md
  - https://www.gesetze-im-internet.de/rog_2008/
  - https://www.gesetze-im-internet.de/bbaug/
  - https://www.gesetze-im-internet.de/uvpg/
  - https://www.gesetze-im-internet.de/bnatschg_2009/
  - https://gl.berlin-brandenburg.de/landesplanung/landesentwicklungsplan-hauptstadtregion-berlin-brandenburg-lep-hr/
  - https://geoportal.brandenburg.de/
  - https://lfu.brandenburg.de/lfu/de/aufgaben/natur/naturschutzfachdaten/kartenanwendung/
  - https://www.gesetze-im-internet.de/bimschg/
  - https://www.gesetze-im-internet.de/vwvfg/
  - https://bravors.brandenburg.de/gesetze/vwvfgbbg
  - https://www.gesetze-im-internet.de/bimschv_9/
  - https://buergerbeteiligung-mil.brandenburg.de/bbp/de/planungsverfahren/kommunale-bauleitplanung/ueberblick/
  - https://mleuv.brandenburg.de/mleuv/de/umwelt/immissionsschutz/genehmigungsverfahren/
  - https://lfu.brandenburg.de/lfu/de/aufgaben/immissionsschutz/anlagenueberwachung/
---

> Exact model backend not verifiable from final document text alone.

# Lebenszyklusmodell einer BImSchG-Anlage — Brandenburg

## Zweck und Grenzen

Dieses Showcase modelliert den gesamten Lebenszyklus einer fiktiven Anlage in Brandenburg: von der ersten Projektidee und Standortsuche über Bauleitplanung (einschließlich eines B-Plans, sofern erforderlich), Zulassungs- und Beteiligungsverfahren bis zu Bau, Betrieb, Überwachung, Änderungen und Stilllegung. Der BImSchG-Vorgang ist ein zentraler, aber nicht der einzige öffentlich-rechtliche Teilprozess.

Es ist das fachliche Obermodell für eine ThemisDB-Demo, keine Rechtsberatung, kein verbindlicher Behördenworkflow und keine Feststellung, dass eine konkrete Anlage genehmigungs-, planungs- oder UVP-pflichtig ist. Nicht jede Anlage benötigt einen neuen B-Plan; vorhandene Bauleitplanung, §§ 34/35 BauGB oder andere planungsrechtliche Wege können einschlägig sein. Fristen, Zuständigkeiten, Rechtsbehelfe, Gebühren und fachliche Anforderungen werden in einem realen Vorhaben anhand der am jeweiligen Ereignisdatum geltenden Rechtslage und der zuständigen Behörde geprüft. Die Beispieldaten sind vollständig fiktiv.

## Brandenburgischer Verfahrenskontext

- Für BImSchG-Genehmigungsverfahren ist in Brandenburg das **Landesamt für Umwelt (LfU)** mit den Genehmigungsverfahrensstellen Ost, Süd und West zuständig. Das „Umweltamt“ einer Gemeinde oder eines Landkreises wird im Modell daher nicht pauschal als Genehmigungsbehörde gesetzt. Die zuständige LfU-Stelle wird anhand des Anlagenstandorts und der aktuellen Zuständigkeitsinformationen bestimmt.
- Die **Bauleitplanung ist kommunale Aufgabe**. Die Gemeinde entscheidet über Aufstellung und Inhalt ihrer Bauleitpläne und führt die jeweilige Öffentlichkeits- und Behördenbeteiligung durch. Ein B-Plan ist daher ein eigener kommunaler Planungsprozess und weder ein Antrag noch eine Entscheidung der BImSchG-Genehmigungsbehörde.
- Die Planungsebenen werden ausdrücklich getrennt: **Bundesraumordnung** und gegebenenfalls sektorale Bundesfachplanung; **Landes- und Regionalplanung** (in Brandenburg insbesondere LEP HR und einschlägige Regionalpläne); darunter die **kommunale Bauleitplanung**. Der **Flächennutzungsplan (FNP) ist kein Landesplan**, sondern der vorbereitende Bauleitplan der Gemeinde; der B-Plan ist der verbindlichere kommunale Bauleitplan.
- Bundes- und Landes-/Regionalpläne sind räumliche Prüfgrundlagen für raumbedeutsame Planungen und Vorhaben. Eine Bundesfachplanung, etwa für bestimmte Höchstspannungsleitungen, wird nur bei einschlägigem Fachrecht und Projektbezug als eigener Verfahrenszweig aktiviert; sie ist kein allgemeiner Genehmigungsschritt für jede Anlage.
- Zuerst wird der planungsrechtliche Ausgangspunkt erfasst: FNP, vorhandener B-Plan, Innenbereich, Außenbereich oder ein noch zu schaffender Plan. Ob ein neuer/angepasster B-Plan erforderlich ist und ob ein Vorhaben bis zum Inkrafttreten eines Plans zulässig sein kann, bleibt eine fallbezogene Prüfung.
- Raumordnungs- und Landesplanung wird als bedingter eigener Vorgang modelliert, wenn das Vorhaben raumbedeutsam ist. Sie ist nicht pauschal mit dem kommunalen B-Plan oder der Anlagenzulassung gleichzusetzen.
- Die konkrete Anlagenart, der Eintrag in Anhang 1 der 4. BImSchV sowie die Kennzeichnungen für förmliches Verfahren (`G`), vereinfachtes Verfahren (`V`) und IED (`E`) sind fallbezogene Klassifikationen. Auch ein UVP-pflichtiges Vorhaben kann ein förmliches Verfahren auslösen. Diese Entscheidungen werden als begründete, datierte Feststellungen der Behörde gespeichert; sie werden nicht aus einer frei erfundenen Standardleistung abgeleitet.
- Die Genehmigung hat grundsätzlich eine Konzentrationswirkung, aber nicht jede Zulassung wird eingeschlossen. Insbesondere können wasserrechtliche Erlaubnisse gesondert erforderlich sein. Das Showcase führt solche Zulassungen deshalb als eigene Vorgänge mit Abhängigkeiten und Zuständigkeit.
- Fachbehörden werden nach den betroffenen Schutzgütern und Rechtsmaterien fallbezogen beteiligt. Das Modell enthält keine starre „alle Behörden“-Liste: jede Beteiligung braucht einen Zuständigkeitsgrund, eine konkrete Anfrage, Frist, Stellungnahme und dokumentierte Behandlung durch die Genehmigungsbehörde.
- Die Überwachung umfasst regelmäßige, risikoorientierte Kontrollen sowie anlassbezogene Prüfungen, zum Beispiel nach Beschwerden, Betriebsstörungen oder Unfällen. Eigen-, Fremd- und staatliche Überwachung werden als getrennte Rollen und Nachweise erfasst.

## Lebenszyklus und Entscheidungspunkte

| Abschnitt | Fachlicher Vorgang | Ergebnis und Kontrollpunkt |
|---|---|---|
| 0. Idee und Brainstorming | Zweck, mögliche Anlagentypen, Energie-/Stoffströme, Alternativen, Betroffene und offene Annahmen erfassen | Versioniertes Konzept; keine Genehmigungs- oder Zulässigkeitsbehauptung |
| 1. Standort- und Machbarkeitsprüfung | Standortvarianten georeferenzieren; Bundes-, Landes-/Regional- und kommunale Planstände sowie Umwelt- und Naturschutzdaten überlagern; Datenqualität und Konflikte prüfen | Variantenvergleich mit Quellen, Datenständen, Maßstab, Rechtswirkung und Prüfbedarf; frühe Behördenkontakte als Beratung, nicht als Zulassung |
| 2. Übergeordnete Planung | Bundesraumordnung, LEP HR, Regionalpläne und gegebenenfalls sektorale Bundesfachplanung gegen Vorhaben und Varianten prüfen | Betroffene Ziele/Grundsätze, Zielkonflikte und erforderliche Klärungen mit Rechtswirkung und räumlichem Bezug dokumentieren |
| 3. Kommunaler Planungsrechtspfad | FNP als vorbereitenden Bauleitplan, B-Plan als verbindlichen Bauleitplan sowie §§ 34/35 BauGB unterscheiden; nötigenfalls Planaufstellung/-änderung der Gemeinde erfassen | Planverfahren mit Umweltprüfung, Behörden-/Öffentlichkeitsbeteiligung, Abwägung, Beschluss und Bekanntmachung; ein neuer B-Plan ist keine automatische Voraussetzung |
| 4. Umwelt- und Naturschutzgrundlagen | UVPG-Klassifikation/Vorprüfung anhand Vorhabenmerkmalen und Standortkriterien durchführen; Natura-2000-, Arten-, Eingriffs- und weitere Fachprüfungen separat erfassen | GIS-Treffer sind Screening-Hinweise, keine automatische UVP- oder Zulassungsentscheidung; fachliche Erhebungen und behördliche Feststellung bleiben erforderlich |
| 5. Zuständigkeit und Anlagenklassifikation | Standort, Betreiber, Anlagenteile und Betriebsweisen erfassen; 4. BImSchV-Eintrag, Verfahrensart, UVP- und IED-Relevanz prüfen | Begründete Klassifikationsentscheidung mit Rechtsstand, Prüfer und offenen Fragen |
| 6. Vorberatung und Antrag | Voranfragen, Antragsunterlagen, Fachgutachten, Planstände und erforderliche separate Zulassungen erfassen | Eingangsbestätigung; Vollständigkeitsprüfung; nachgeforderte Unterlagen bleiben einzeln nachvollziehbar |
| 7. UVP und Beteiligungsplanung | UVP-Vorprüfung oder UVP-Pflicht dokumentieren; Umfang der Untersuchung und Verfahrensschritte festlegen | Behördliche Entscheidung samt Kriterien, Belegen und Bekanntmachungsanforderungen |
| 8. Behördenbeteiligung | Betroffene Stellen parallel beteiligen, Rückfragen und Stellungnahmen fristbezogen verwalten | Jede Stellungnahme einer Rechtsmaterie, einer Anlage/Komponente und einer behördlichen Bewertung zuordnen |
| 9. Öffentlichkeit | Bekanntmachung, Auslegung/Online-Zugang, Einwendungen, Stellungnahmen anerkannter Vereinigungen und gegebenenfalls Erörterung/Onlinekonsultation protokollieren | Einwendungen bleiben von Beschwerden und Rechtsbehelfen getrennt; Antwort und Abwägung sind belegbar |
| 10. Entscheidung und Bekanntgabe | Genehmigungsfähigkeit, Inhalts- und Nebenbestimmungen, separate Zulassungen, Begründung, Gebühren und Rechtsbehelfsbelehrung prüfen | Versionierter Bescheid mit Zustell- und Veröffentlichungsnachweisen; keine automatische Rechtskraftannahme |
| 11. Bau und Inbetriebnahme | Baubeginn, Nachweise vor Inbetriebnahme, Abnahmen und Erfüllung von Nebenbestimmungen verfolgen | Inbetriebnahmefreigaben und offene Auflagen sind fallbezogen und zuständigkeitsgebunden |
| 12. Betrieb und Überwachung | Messungen, Berichte, Eigen-/Fremdüberwachung, Vor-Ort-Kontrollen, Unfälle, Beschwerden und Abweichungen erfassen | Feststellungen, Fristen, Abhilfe und Wiederholungsprüfung bilden eine revisionsfähige Ereigniskette |
| 13. Änderungen und Eingriffe | Geplante Änderung zunächst auf Anzeige nach § 15 oder Änderungsgenehmigung nach § 16 BImSchG prüfen; nachträgliche Anordnungen und Betriebsuntersagungen separat führen | Keine Änderung wird als zulässig markiert, bevor erforderliche behördliche Entscheidungen und andere Zulassungen geprüft sind |
| 14. Betriebseinstellung und Nachsorge | Betreiber-Mitteilung, Sicherung, Reststoffe, Rückbau, Boden-/Grundwasserzustand und erforderliche Nachsorgemaßnahmen verfolgen | Behördlich dokumentierter Abschluss; offene Gefahren, Pflichten und Untersuchungen verhindern einen irreführenden Status „erledigt“ |

Die Statusprojektion darf nicht nur einen einzigen linearen Status verwenden. Ein Verfahren kann beispielsweise gleichzeitig eine anhängige Klage, laufende Überwachungsauflagen und eine separate Änderungsanzeige haben. Der Status ergibt sich daher aus Teilvorgängen und Ereignissen, nicht durch Überschreiben des bisherigen Verlaufs.

## Geobasierte Grundlagenplanung und Naturschutz

Die räumliche Prüfung beginnt vor der Detailplanung und wird für jede Standortvariante wiederholt. Ein Kartenoverlay unterstützt die Sachverhaltsermittlung, ersetzt aber weder die rechtsverbindliche Planprüfung noch eine UVP-Vorprüfung, eine FFH-Verträglichkeitsprüfung oder die fachliche Entscheidung der zuständigen Stelle.

### Planungsebenen als räumliche Prüfkulisse

1. **Bund:** Raumordnung nach ROG und einschlägige Bundesraumordnungspläne (zum Beispiel Hochwasserschutz) auf räumliche Berührung und Bindungswirkung prüfen. Sektorale Verfahren wie Bundesfachplanung nach NABEG sind nur relevant, wenn Vorhabenart und Fachrecht sie auslösen.
2. **Land/Region:** LEP HR, sachliche Teilpläne und gültige Regionalpläne einschließlich zeichnerischer Festlegungen, Zielen, Grundsätzen und gegebenenfalls Zielabweichungen erfassen. Bei raumbedeutsamen Projekten die Erforderlichkeit einer Raumverträglichkeitsprüfung klären.
3. **Gemeinde:** FNP-Darstellungen, B-Plan-Festsetzungen und kommunale Landschafts-/Beteiligungsinformationen prüfen. Der FNP ist die vorbereitende kommunale Planung und kein Landesentwicklungsplan; Planstände und Rechtsverbindlichkeit müssen getrennt gespeichert werden.
4. **Standort/Fachrecht:** Schutzgebietsgrenzen, Natura-2000-Erhaltungsziele, Biotope, bekannte Artvorkommen, Wasser-/Hochwasserrisiken, Boden, Wald, Denkmäler, Erschließung und empfindliche Nutzungen als fachliche Hinweise oder rechtsverbindliche Festsetzungen klassifizieren.

### Geodatenmodell und Qualität

Jede Geodatenebene erhält mindestens: `layer_id`, Geometrie/Geometrietyp, Koordinatenreferenzsystem, zuständige Quelle, Metadatensatz/Download- oder Dienst-URL, Lizenz und Namensnennung, Maßstab/Auflösung, Erfassungs- und Aktualisierungsdatum, fachliche und räumliche Gültigkeit, Rechtswirkung (`NORMATIV`, `HINWEIS`, `FACHBEFUND`), Datenqualität/Unsicherheit sowie Zugriffs- und Schutzklasse. Standortvarianten und Anlagenkomponenten werden mit eigener Geometrie und Version geführt; ein Treffer enthält Layer-Version, räumlichen Bezug, Distanz/Überlappung und zugrunde liegende Feature-ID.

Für den Showcase werden Geoportal Brandenburg und LfU-Geodienste als Einstiegspunkte behandelt. Nutzungsrechte, Pflicht zur Quellenangabe und Metadaten gelten je Datensatz; nicht veröffentlichte Artvorkommen oder personenbezogene Hinweise dürfen nicht in öffentliche Karten übernommen werden. Datenstände und Darstellungsmaßstäbe sind wichtig: eine Übersichtskarte ist kein parzellenscharfer Rechtsnachweis.

### Natur- und UVP-Prüfung

- Die UVP-Pflicht ergibt sich aus der Vorhabenart und den Tatbeständen/Schwellenwerten des UVPG. Bei einer Vorprüfung nach § 7 UVPG werden die Kriterien aus Anlage 3 einschließlich der besonderen örtlichen Gegebenheiten und möglichen erheblichen Auswirkungen dokumentiert. Ein Geodaten-Treffer kann ein Prüfanlass oder Beleg sein, löst aber nicht allein automatisch eine UVP-Pflicht aus.
- Naturschutzprüfungen werden parallel und eigenständig erfasst: Natura-2000-Vorprüfung/Verträglichkeitsprüfung nach § 34 BNatSchG, besonderer Artenschutz nach § 44 BNatSchG, Eingriffsregelung/Vermeidung und Ausgleich nach §§ 14–17 BNatSchG sowie weitere gebiets- oder landesrechtliche Anforderungen, soweit einschlägig.
- Umweltbericht und Umweltprüfung in der Bauleitplanung (insbesondere §§ 2 Abs. 4 und 1a BauGB) sind mit der späteren projektbezogenen UVP verknüpfbar, aber nicht mit ihr identisch. Vorhandene Prüfergebnisse können berücksichtigt werden; Aktualität, Untersuchungsraum und abweichender Prüfgegenstand bleiben nachvollziehbar.
- GIS-Screening priorisiert Geländebegehungen und saisonale Arten-/Biotoperhebungen, entscheidet sie aber nicht automatisch weg. Befunde werden mit Methode, Zeitraum, Bearbeiter, räumlicher Genauigkeit, Unsicherheit und fachlicher Freigabe erfasst.

## Rechtsdimension: Quellen, Geltung und Auslegung

Jede fachliche Verwaltungsaufgabe muss auf eine nachvollziehbare Rechts- und Erkenntnisgrundlage verweisen. Das Modell trennt **Rechtsnormen**, **verwaltungsinterne Steuerungsdokumente**, **gerichtliche Entscheidungen** und **Auslegungshilfen**. Diese Quellen sind nicht austauschbar und dürfen insbesondere nicht als ein flacher Katalog von gleichrangigen „Rechtsgrundlagen“ verarbeitet werden.

### Quellentypen und Wirkung

| Quellentyp | Was erfasst wird | Wirkung im Modell |
|---|---|---|
| EU-Recht und völkerrechtliche Vorgaben | Vertrag, Verordnung, Richtlinie, Beschluss, Umsetzung und betroffene Rechtsmaterie | Rang, unmittelbare Anwendbarkeit/Umsetzungsbedarf, Adressatenkreis und Umsetzungsstand konkret prüfen; „EU-Recht“ ist kein einheitlicher Wirkungstyp |
| Verfassungsrecht und Gesetze | Grundgesetz, Bundesgesetze und Landesgesetze einschließlich konkreter Paragrafen/Fassungen | Normgeber, Regelungsbereich, räumlicher und persönlicher Anwendungsbereich sowie zeitliche Geltung |
| Rechtsverordnungen und Satzungen | Delegationsgrundlage, erlassende Stelle, Ermächtigungsrahmen, Verkündung und Geltung | Eigenständige normative Regelung; Ermächtigung und Vereinbarkeit mit höherrangigem Recht bleiben prüfbar |
| Verwaltungsvorschriften, Erlasse und Rundschreiben | Herausgeber, Adressaten, Organisations-/Zuständigkeitsbereich, Bekanntmachung, Geltungs- und Aufhebungsstatus | Regelmäßig interne Steuerung der Verwaltung. Außenwirkung, Gleichbehandlung/Selbstbindung und Bedeutung im konkreten Verfahren sind getrennt zu beurteilen; das Dokument wird nicht automatisch wie ein Gesetz angewendet |
| Dienstanweisungen und Arbeitsanweisungen | Urheber, befugte Stelle, adressierte Organisationseinheit, Version, Verteilung und Schutzklasse | Interne Verfahrens- oder Arbeitssteuerung. Keine selbstständige Eingriffsgrundlage; die zugrunde liegende Rechtsnorm und Delegations-/Zuständigkeitskette müssen separat vorliegen |
| Technische Regeln, Standards und Vollzugshinweise | Herausgeber, Fassung, Anwendungsbereich, gesetzliche Verweisung/Einführung, vertragliche oder bescheidliche Bezugnahme und Aktualität | Nicht allein durch Bezeichnung oder technische Autorität allgemein verbindlich. Rechtswirkung nur aus der einschlägigen Rechtsgrundlage oder konkreten Einbeziehung ermitteln |
| Gerichtsentscheidungen | Gericht, Spruchkörper, Entscheidungsart und -datum, Aktenzeichen, Instanzenzug, Tenor, tragende Gründe/Randnummern und Rechtskraft, soweit bekannt | Bindungswirkung für den konkreten Rechtsstreit und mögliche besondere gesetzliche Bindungswirkung von Gericht/Entscheidungsart unterscheiden. Andere Entscheidungen sind wichtige Auslegungshinweise, aber nicht pauschal neue Gesetze oder automatisch auf andere Sachverhalte übertragbar |
| Behördenauffassungen und Einzelfallauslegung | Urheber und Funktion, Rechtsfrage, Tatsachengrundlage, verwendeter Rechtsstand, Begründung und Verfahrenskontext | Als parteiische, interne oder fallbezogene Rechtsauffassung kenntlich machen; keine behördliche Auffassung als gerichtliche Klärung oder allgemein verbindliche Norm ausgeben |
| Verwaltungsakte und Nebenbestimmungen | Behörde, Adressat, Tenor, Rechtsgrundlage, Bekanntgabe/Zustellung, Wirksamkeit, Vollziehbarkeit und Rechtsbehelfsstatus | Individuelle Regelung für den Einzelfall, keine generell-abstrakte Rechtsnorm. Auflagen, Bedingungen, Befristungen und Widerrufsvorbehalte separat mit Bescheidfassung und Anlagenzustand verknüpfen |
| Kommentare und sonstige Literatur | Werk/Autor:innen, Auflage, Erscheinungsjahr, Bearbeiter:in, kommentierte Norm/Randnummer und zitierte Primärquellen | Sekundärliteratur und nicht bindende Auslegungshilfe. Gegen Gesetzesfassung, amtliche Materialien und einschlägige Rechtsprechung prüfen; nur bibliografische Metadaten, zulässige Zitate und selbst erstellte Fundstellen speichern, nicht geschützte Volltexte übernehmen |

Die Hierarchie ist nicht allein durch eine Rangzahl lösbar. Bei der Zuordnung sind mindestens Normgeber/Kompetenz, Ermächtigungsgrundlage, Gebiet, Adressat, Sachverhalt, zeitliche Geltung, Spezialität, Übergangsrecht, Verkündung/Bekanntmachung und die jeweils behauptete Bindungswirkung zu prüfen. Konflikte werden als offene Rechtsfrage mit konkurrierenden Quellen und menschlicher Entscheidung dokumentiert, nicht durch „neueste Quelle gewinnt“ oder eine automatische Rangsortierung entschieden.

### Rechtsstand, Version und Fundstelle

Eine Quelle ist ein dauerhaft identifizierbares Werk; ihre Fassungen und die für einen Vorgang maßgebliche konkrete Vorschrift sind separate Objekte. Für Normtexte werden, soweit verfügbar, Verkündungsfundstelle, Änderungsakte, Inkrafttreten, Außerkrafttreten, Übergangsregel, konsolidierte Fassung, Abrufzeitpunkt und Prüfsumme erfasst. Der Abruf einer konsolidierten Webseite beweist allein weder die historische Fassung noch deren Anwendbarkeit an einem Ereignistag. Für Urteile zählen außerdem Entscheidungstag und Verfahrensstand; ein Erlass kann geändert oder aufgehoben sein. Für jede Anwendung gilt „Recht am maßgeblichen Stichtag“ und eine nachvollziehbare Belegkette.

Jede `rechtsanwendung` verweist auf konkrete Vorschriften (nicht nur den Gesetzestitel), die einschlägige Quellenfassung und relevante Absätze/Randnummern. Sie hält Sachverhalt, Rechtsfrage, Auslegungsschritte, Tatsachenfeststellungen, Gegenargumente, zuständige prüfende Stelle, Ergebnis, Unsicherheit und Freigabe fest. Ein automatisch erzeugter Treffer, eine KI-Zusammenfassung oder ein Kommentar ist kein Rechtsgutachten und keine Behördenentscheidung. Entscheidungen und abweichende Auffassungen bleiben versioniert; spätere Erkenntnisse überschreiben den damaligen Entscheidungsstand nicht.

### Showcase-Beispiele und Grenzen

Der Seed enthält als echte, öffentlich nachprüfbare Beispiele die 4. BImSchV als Bundesrechtsverordnung, den brandenburgischen Runderlass zur Zusammenarbeit der BImSchG-Behörden vom 11. Juli 2023 und das BVerwG-Urteil vom 25. März 2026 (7 C 3.25). Der Runderlass wird als Verwaltungsvorgabe mit behördeninternem Koordinierungszweck erfasst, nicht als Gesetz. Das Urteil betrifft die Konzentrationswirkung einer fingierten Änderungsgenehmigung für Windenergieanlagen nach einer speziellen Fassung des § 16b BImSchG; es ist ein Vergleichsfall und wird **nicht** als unmittelbar entscheidend für die fiktive Biomasseanlage modelliert. Eine konkrete Kommentierung ist noch nicht ausgewählt und wird im JSON nur durch ein leeres Metadatenschema dargestellt.

## Fachliches Datenmodell

### Kernobjekte

| Objekt | Inhalt und Invarianten |
|---|---|
| `projektidee` / `projektvariante` | Ziele, Varianten, Annahmen, Prüffragen, Beteiligte und Entscheidungsverlauf; Brainstorming bleibt von behördlichen Feststellungen getrennt |
| `standortpruefung` | Geometrie, Eigentum/Verfügbarkeit, Erschließung, Schutzgebiete, Konflikte, Planstatus und dokumentierte Alternativen |
| `raumordnungsvorgang` | Bundes-, Landes- und Regionalplanung mit Planebene, Instrument, Ziel/Grundsatz, Geltungsbereich, Bindungswirkung, Beteiligung und gegebenenfalls Raumverträglichkeitsprüfung |
| `planungsvorgang` | Kommunaler FNP-/B-Plan-Prozess mit Planungsträger, Planständen, Beteiligungen, Umweltbericht, Abwägung, Beschlüssen und Bekanntmachung |
| `geodatenlayer` / `geodatenbefund` | Quelle, Metadaten, Geometrie, Koordinatensystem, Datenstand, Maßstab, Lizenz, Rechtswirkung, Qualität, räumlicher Treffer und menschliche Validierung |
| `uvp_screening` / `naturschutzpruefung` | Vorhabenklasse, UVPG-Kriterien, betroffene Schutzgüter, Natura-2000-/Artenschutz-/Eingriffsprüfungen, Untersuchungsbedarf, Befunde und Behördenfeststellung |
| `behoerdenaufgabe` | Fachaufgabe mit Auslöser, Rechtsgrundlage und Rechtsstand, zuständiger Stelle, Eingangsvoraussetzungen, Fristregel, erforderlichen Belegen, Entscheidung und Rechtsbehelf |
| `verfahren` | Aktenzeichen, Verfahrensart, Genehmigungsbehörde, Verfahrensstand, Rechtsstand und Verknüpfungen zu Änderungs-, Überwachungs-, Gebühren- und Rechtsbehelfsakten |
| `anlage` / `anlagenkomponente` | Betreiber, Standort/Geometrie, technische Parameter, Betriebsweisen, Anlagenverzeichnis-Klassifikation und Historie genehmigter Konfigurationen |
| `beteiligter` | Natürliche/juristische Person oder Behörde mit Rolle je Teilvorgang; Betroffenenstatus und Zugriff werden nicht aus einer bloßen Beschwerde oder Einwendung abgeleitet |
| `behoerde` / `zustaendigkeit` | Behörde, sachliche/räumliche Zuständigkeit, gültig-von/bis, Zuordnungsgrund und delegierte Aufgaben |
| `dokument` / `dokumentversion` | Dokumenttyp, Urheber, Eingangs-/Erstellzeit, Version, Prüfsumme, Rechtsstand, Schutz-/Veröffentlichungsklasse und Ablösungsvorgänger |
| `beteiligung` / `stellungnahme` | Angefragte Stelle, Rechtsmaterie, Fristquelle, Antwort, Nachforderung, Ergebnis und Übernahme/Abweichung in der Entscheidung |
| `oeffentlichkeitsvorgang` | Bekanntmachung, Auslegungsort/-zeitraum, zugängliche Unterlagen, Einwendung und deren Verfahrensbehandlung |
| `bescheid` / `nebenbestimmung` | Tenor, Begründung, Rechtsbehelfsbelehrung, Zustellung/öffentliche Bekanntmachung sowie maschinenlesbare Pflicht, Messgröße, Grenzwert, Gültigkeit und Nachweis |
| `rechtsbehelf` | Typ (z. B. Widerspruch, Klage, Eilantrag), angegriffener Verwaltungsakt, Einlegung, Fristgrundlage, Rechtsbehelfsbelehrung, Zulässigkeitsprüfung und Verfahrensstand |
| `beschwerde` / `hinweis` | Eingangskanal, behaupteter Sachverhalt, möglicher Immissionspfad, Schutzbedarf, zuständige Prüfung und Verknüpfung zu Überwachungsereignissen |
| `gebuehrenvorgang` | Gebührenschuldner, Tarifstelle und Fassung, Berechnungsparameter, Auslagen, Festsetzung, Bescheid, Zahlung und gegebenenfalls Rechtsbehelf |
| `aenderungsvorgang` | Geplante Änderung, Anzeige/Genehmigungsroute, behördliche Einordnung, Startvoraussetzungen und Abhängigkeiten von anderen Zulassungen |
| `ueberwachungsvorgang` | Anlass (turnusmäßig, Beschwerde, Ereignis), Prüfumfang, Beteiligte, Vor-Ort-Feststellungen, Messwerte, Mängel, Maßnahmen und Folgeprüfung |
| `anordnung` | Rechtsgrundlage, Sachverhalt, Adressat, Tenor, Verhältnismäßigkeits-/Ermessensprüfung soweit einschlägig, Frist, Vollziehbarkeit und Erfüllungsnachweis |
| `betriebseinstellung` | Zeitpunkt und Umfang, Sicherungs-/Rückbauplan, Reststoffe, Boden-/Grundwasserbewertung, Nachsorge, behördliche Prüfungen und Abschlussstatus |
| `ereignis` | Append-only Fachereignis mit Akteur, Zeitpunkt, Objekt, Quelle, Korrelation und Rechtsstand; Korrekturen erfolgen als Folgeereignis statt stiller Änderung |
| `rechtsquelle` / `rechtsquellenfassung` | Quellentyp, Normgeber/Herausgeber, Jurisdiktion, Rang bzw. Wirkung, Fundstelle, Geltungsgebiet/-zeit, Version, offizieller Text und Ablösung/Änderung |
| `rechtsvorschrift` | Konkreter Artikel/Paragraf/Absatz/Nummer, Fassungsbezug, Tatbestand/Rechtsfolge als zitierte oder redaktionell erschlossene Struktur, nicht als ungeprüfter Volltextimport |
| `rechtsanwendung` / `rechtsauslegung` | Einzelfallfrage, Sachverhalt, verwendete Quellenfassungen, Argumentation, Belege, Gegenauffassung, Ergebnis, Autorität, Unsicherheit und menschliche Freigabe |
| `gerichtliche_entscheidung` | Gericht, Instanz, Entscheidungsart/-datum, Aktenzeichen, Tenor, Fundstelle, Randnummern, Verfahrensstand und sachlicher Anwendungsbereich |
| `kommentar_referenz` | Bibliografische Angaben, Auflage, Bearbeiter:in, Norm, Randnummer und Werkzugriff; Sekundärquelle ohne normative Bindungswirkung |
| `erlass_oder_anweisung` | Herausgeber, interne Adressaten, Zuständigkeitsbereich, Version, Verteilung/Schutzklasse, Geltungsstatus und zugrunde liegende Rechtsgrundlage |
| `technische_regel` | Herausgeber, Ausgabe, Anwendungsbereich und konkrete gesetzliche, vertragliche oder bescheidliche Einbeziehung; keine automatische Rechtsnormqualität |

### Beziehungen und ThemisDB-Abbildung

- **Dokumentmodell:** Fachobjekte und ihre versionierten Projektionen werden als Dokumente mit stabilen IDs gespeichert. Verfahrensakte und Dokumentenbestand bleiben nach Zuständigkeit, Schutzbedarf und Veröffentlichungsklasse getrennt zugreifbar.
- **Graphmodell:** Beziehungen wie `BETRIFFT`, `ZUSTAENDIG_FUER`, `BETEILIGT_AN`, `STELLUNGNAHME_ZU`, `BELEGT`, `PLANUNGSVORAUSSETZUNG`, `ERFUELLT_VORAUSSETZUNG`, `AUFLAGE_GILT_FUER`, `WIDERSPRUCH_GEGEN`, `KLAGE_GEGEN`, `BESCHWERDE_LOEST_AUS`, `ANORDNUNG_FOLGT_AUF`, `AENDERT` und `NACHSORGE_FUER` machen die fachliche Evidenzkette abfragbar.
- **Geodaten:** Projektgeometrien, Raumplan- und Schutzgebietslayer sowie räumliche Befunde werden versioniert und über räumliche Abfragen für Variantenvergleich und Screening verbunden. Das Ergebnis bewahrt die Quelldatenstände und ist als Hinweis von einer verbindlichen behördlichen Feststellung unterscheidbar.
- **Zeitachse:** Verfahrensereignisse, Fristberechnung, Messreihen und Anlagenzustände erhalten fachliche Gültigkeitszeit und Erfassungszeit. Für jede Frist werden Startauslöser, Rechtsgrundlage, Berechnungsregel, Feiertagskalender, Hemmung/Verlängerung und manuelle Prüfung gespeichert; Fristen werden nicht als ungeprüfte feste Tageszahl in Prozessvorlagen kodiert.
- **Rechtsstand:** Bescheid, Rechtsbehelf und Gebührenberechnung referenzieren die zum maßgeblichen Zeitpunkt verwendete Rechts-/Tariffassung. Spätere Änderungen überschreiben diese Evidenz nicht.
- **Rechtsquellen-Graph:** `AENDERT_ODER_ERSETZT`, `GESTUETZT_AUF`, `KONKRETISIERT`, `UMSETZT`, `GILT_IN_GEBIET`, `GILT_IM_ZEITRAUM`, `WIRD_AUSGELEGT_DURCH`, `ZITIERT`, `WEICHT_AB_VON` und `WIRD_ANGEWENDET_IN` verbinden Normfassung, Vorschrift, Erlass, Urteil, Literaturstelle, Rechtsfrage und Verwaltungsentscheidung. Kanten enthalten Quelle, Gültigkeitszeit und Begründung.
- **Rechtswirkung und Autorität:** Eine maschinenlesbare Einstufung ist ein Recherchefilter, kein Rechtsentscheid. Rang-/Bindungswerte müssen neben Jurisdiktion, Normgeber, Ermächtigung, Adressat, Verfahren, Sachverhalt und Stichtag angezeigt werden; unaufgelöste Konflikte gehen an qualifizierte menschliche Prüfung.
- **Nachweis und Urheberrecht:** Primärtexte werden über amtliche Fundstellen und zugelassene Abrufe referenziert. Für Kommentare genügt grundsätzlich bibliografische Metadaten-/Fundstellenerschließung; vollständige geschützte Werke werden nicht kopiert. Nicht öffentliche Weisungen und personenbezogene Prozessakten erhalten eine Zugriffsklasse und dürfen nicht in öffentliche Suche/Antworten gelangen.
- **Datenschutz und Veröffentlichung:** Namen von Einwendenden, Beschwerdeführenden und Sachverständigen sind personenbezogene Daten. Öffentliche Dokumentkopien werden als redigierte Versionen geführt; die Verfahrensakte und die veröffentlichte Fassung erhalten getrennte Zugriffsregeln und Prüfsummen.
- **Assistenzfunktionen:** Volltext-/Vektorsuche und KI-Zusammenfassungen können Akteninhalte auffindbar machen. Sie dürfen Zuständigkeit, UVP-Pflicht, Zulässigkeit eines Rechtsbehelfs, Gebühren oder eine Ermessensentscheidung weder selbst verbindlich festlegen noch Originalbelege ersetzen. Jede generierte Aussage muss auf konkrete Dokumentversionen verweisen und menschlich geprüft werden.

## Behördenaufgaben als erweiterbarer Katalog

Das Obermodell legt fest, **wie** eine Verwaltungsaufgabe erfasst wird; die konkrete Aufgabenbefüllung erfolgt anschließend je Rechtsmaterie und Anlagenfall. Eine Aufgabe wird nicht allein durch einen Gesetzesnamen erzeugt, sondern durch einen dokumentierten Auslöser und eine fallbezogene Zuständigkeitsprüfung.

| Aufgabenbereich | Typischer Träger im Modell | Beispiel für eigene Vorgänge |
|---|---|---|
| Bundesraumordnung | zuständige Bundesstellen sowie öffentliche Planungs-/Zulassungsstellen im jeweiligen Bindungsrahmen | Bundesraumordnungspläne, räumliche Betroffenheit, Hochwasserschutzfestlegungen und Ziel-/Grundsatzprüfung |
| Bundesfachplanung | Bundesnetzagentur oder weitere Bundesstelle, nur bei besonderem sektoralen Rechtsgrund | Trassenkorridor-/Fachplanverfahren, Beteiligung, Ergebnis und Abhängigkeit zur späteren Zulassung |
| Landes-/Regionalplanung | Gemeinsame Landesplanungsabteilung Berlin-Brandenburg und zuständige Regionalplanung | LEP HR/Regionalplan, Ziele und Grundsätze, Zielabweichung sowie gegebenenfalls Raumverträglichkeitsprüfung |
| Kommunale Bauleitplanung | Gemeinde als Planungsträger | FNP als vorbereitender Bauleitplan, B-Plan-Aufstellung/Änderung, Umweltbericht, TÖB- und Öffentlichkeitsbeteiligung, Abwägung und Inkrafttreten |
| Geobasierte Grundlagenplanung | Vorhabenträger/Fachgutachter liefern Befunde; zuständige Behörden prüfen im Aufgabenbereich | Standortvarianten, Metadaten- und Layerregister, Schutzgüter-Overlay, Datenlücken und Vor-Ort-Erhebungen |
| UVP und Naturschutz | zuständige UVP-/Genehmigungs- und Naturschutzbehörden nach Zuständigkeit | UVPG-Screening, Umweltbericht/UVP-Bericht, Natura-2000-, Artenschutz- und Eingriffsregelungsprüfungen |
| Immissionsschutz | LfU Brandenburg als zuständige Genehmigungs-/Überwachungsbehörde | Genehmigungsbedürftigkeit, Verfahrensart, UVP-Anknüpfung, Genehmigung, §-15-Anzeige, §-16-Änderung, §-17-Anordnung und §-52-Überwachung |
| Bauen und Erschließung | zuständige Bauaufsicht, Gemeinde und/oder Träger der Wege/Netze nach Zuständigkeit | planungsrechtliche Zulässigkeit, bauliche Anforderungen, Zufahrt, Leitungen und gegebenenfalls eigenständige Erlaubnisse |
| Wasser und Boden | zuständige Wasser-/Bodenschutzstellen | Wasserrechtliche Erlaubnis, Einleitung/Entnahme, Umgang mit wassergefährdenden Stoffen, Bodenschutz und Nachsorge |
| Natur, Arten und Wald | zuständige Naturschutz-/Forststellen | Eingriffsregelung, Artenschutz, Schutzgebiete, Waldumwandlung und Ausgleich |
| Weitere materielle Anforderungen | nach Vorhaben zuständige Fachstellen | Abfall, Denkmalschutz, Straßenverkehr, Brandschutz, Arbeitsschutz, Störfallvorsorge und sonstige erforderliche Zulassungen |
| Öffentlichkeit und Rechtsschutz | betroffene Personen, Vereinigungen, Ausgangs-/Widerspruchsbehörde und Gerichte | Stellungnahme/Einwendung, Beschwerde, Widerspruch, Klage und Eilverfahren als jeweils unterschiedliche Vorgänge |

Die Tabelle ist ein **Aufgabenfinder, keine abschließende Zuständigkeits- oder Beteiligungsliste**. Für jede konkrete Aufgabe werden Rechtsgrundlage, Zuständigkeitsgebiet, Aufgabenbereich, Gültigkeitszeitraum und Zuständigkeitsquelle gespeichert. Eine im BImSchG-Verfahren eingeholte Stellungnahme ist nicht automatisch eine separate Genehmigung oder Erlaubnis dieser Stelle. Der FNP bleibt kommunal; Bundesfachplanung und Raumverträglichkeitsprüfung werden nicht pauschal jedem Vorhaben zugeordnet.

## Verfahrensrechtliche Trennlinien

1. **Einwendung** ist ein Beitrag im Beteiligungsverfahren. Sie ist kein Widerspruch und keine Klage.
2. **Beschwerde/Hinweis** ist ein Anlass für die Sachverhalts- und gegebenenfalls Überwachungsprüfung. Daraus folgt nicht automatisch Beteiligtenstellung oder ein bestimmter Rechtsbehelf.
3. **Widerspruch** wird als Eingangsart unterstützt, seine Statthaftigkeit und Frist werden aber anhand des angegriffenen Verwaltungsakts und der einschlägigen Rechtsbehelfsbelehrung geprüft. Bei einem im förmlichen Verwaltungsverfahren erlassenen Verwaltungsakt ist vor der verwaltungsgerichtlichen Klage nach § 70 VwVfG keine Nachprüfung in einem Vorverfahren erforderlich.
4. **Klage/Eilantrag** sind eigenständige gerichtliche Vorgänge mit Gericht, Aktenzeichen, Zustellungsdaten, Verfahrensstand und gesonderter Prüfung etwaiger aufschiebender Wirkung. Eine Klage wird nicht automatisch als Widerspruch oder Beschwerde behandelt.
5. **Änderung**: Die Behörde prüft den konkreten Änderungsumfang und die einschlägigen Anforderungen, bevor das Showcase die Route „Anzeige“ oder „Änderungsgenehmigung“ darstellt. Für Anzeigen nach § 15 BImSchG werden behördliche Reaktion, Frist und notwendige andere Zulassungen als Startvoraussetzungen festgehalten.
6. **Gebühr**: Das Verfahren speichert Tarifstelle, Tarifversion, Berechnungsgrundlage und Auslagen. Die Beispielsummen sind keine Berechnung nach der aktuell geltenden Gebührenordnung.

## Verwaltungsaktprozess: formelle und materielle Prüfung

Der Prozess ist ein generisches Prüfskelett für den Erlass und die Bekanntgabe eines Verwaltungsakts, angewendet auf den fiktiven BImSchG-Genehmigungsbescheid. Er ersetzt keine fachrechtliche Verfahrensregel: Zuerst werden Behördenart und Anwendungsbereich nach § 1 VwVfGBbg, VwVfG, Ausnahmen und entgegenstehendes Landesrecht festgestellt. BImSchG, 9. BImSchV und weiteres einschlägiges Fachrecht können besondere oder vorrangige Schritte vorgeben. Fristen und Beteiligungspflichten werden deshalb nicht aus einem allgemeinen VwVfG-Schema übernommen.

### Ablauf und Prüfschichten

1. **Handlungsform und Zuständigkeit:** Prüfen, ob eine Regelung mit Außenwirkung im Einzelfall und damit ein Verwaltungsakt nach § 35 VwVfG vorliegt. Sachliche, instanzielle und örtliche Zuständigkeit, Delegation und entscheidungsbefugte Person anhand des konkreten Rechtsstands belegen.
2. **Verfahrensbeteiligte und Akte:** Beteiligtenstellung, Vertretung/Beistand, Mitwirkungsverbote, Befangenheit, Akteneinsicht, Geheimnisschutz und Schutzklasse der Akte erfassen.
3. **Sachverhaltsermittlung:** Die Behörde ermittelt von Amts wegen (§ 24 VwVfG), erhebt erforderliche Beweise (§ 26) und würdigt auch für die Beteiligten günstige Umstände. Tatsachen, Quelle, Erhebungszeit, Unsicherheit und Gegenbelege werden getrennt von rechtlichen Bewertungen gespeichert.
4. **Anhörung und besondere Beteiligung:** Vor einem eingreifenden Verwaltungsakt ist die Anhörung nach § 28 VwVfG zu prüfen. Ausnahmen benötigen die konkrete Rechtsgrundlage und dokumentierte Einzelfallbegründung. Anhörung, Öffentlichkeitsbeteiligung sowie gesetzlich erforderliche Mitwirkung/Fachstellungnahmen sind unterschiedliche Vorgänge; das BImSchG-/9.-BImSchV-Verfahren bleibt maßgeblich.
5. **Formelle Rechtmäßigkeit:** Zustaendigkeit, Verfahrensgang, zwingende Beteiligung, Ermittlung, Anhörung und vorgeschriebene Form prüfen. Ein festgestellter Fehler verzweigt in eine eigenständige Prüfung von Nichtigkeit (§ 44), Heilung (§ 45), Unbeachtlichkeit (§ 46) und Fachrechtsfolgen. Keine dieser Folgen wird automatisch aus einem Checklistentreffer abgeleitet.
6. **Materielle Rechtmäßigkeit:** Eine konkrete Ermächtigungs-/Anspruchsgrundlage und ihre zeitlich maßgebliche Fassung bestimmen. Tatbestandsmerkmale jeweils mit festgestellten Tatsachen und Belegen abgleichen; gebundene Rechtsfolge, Anspruch, Versagungsgrund und gesetzlich eröffnetes Ermessen unterscheiden.
7. **Rechtsfolge und Bescheid:** Bei Ermessen Zweckbindung und gesetzliche Grenzen (§ 40 VwVfG) sowie einschlägige Gleichbehandlungs- und Verhältnismäßigkeitsanforderungen nachvollziehbar würdigen. Nebenbestimmungen benötigen eine eigene Grundlage und Prüfung. Der Tenor muss hinreichend bestimmt sein (§ 37); tatsächliche und rechtliche Gründe sowie Ermessensgesichtspunkte gehören in die Begründungsprüfung (§ 39).
8. **Freigabe und Bekanntgabe:** Entscheidungsbefugnis, Bescheidfassung und offene Prüfungen menschlich freigeben. Adressat, Bekanntgabeweg, Zugangsnachweis und Inhalt erfassen (§ 41); Wirksamkeit nach § 43 separat bestimmen. Bekanntgabe bedeutet nicht automatisch Bestandskraft oder Vollziehbarkeit.
9. **Folgeprozess:** Rechtsbehelf, Fehlerhinweis oder neue Tatsachen öffnen einen eigenen Vorgang. Rücknahme eines rechtswidrigen Verwaltungsakts (§ 48), Widerruf eines rechtmäßigen Verwaltungsakts (§ 49), gerichtlicher Rechtsschutz und Vollziehung werden nicht rückwirkend in die ursprüngliche Bescheiderstellung hineingeschrieben.

### Prozessartefakte und Laufzeitgrenzen

- [`bimschg-verwaltungsakt-bpmn.json`](bimschg-verwaltungsakt-bpmn.json) ist ein ThemisDB-JSON-Graph mit BPMN-Elementnamen und den getrennten Prüfpfaden. BPMN 2.0 definiert hier nicht das verwendete JSON-Serialisierungsformat; die Datei ist daher kein als OMG-Standard ausgegebenes BPMN-JSON.
- Der Serializer `BpmnSerializer::exportFromJson` erwartet `process_id`, `name`, `nodes` und `edges` am Graphen und erzeugt **BPMN-XML**, nicht BPMN-JSON. Die aktuelle Implementierung schreibt Kanten als `sequenceFlow`; ein `condition` wird als Kantenname ausgegeben, nicht als ausführbare BPMN-`conditionExpression`. Lane-, Rechtsgrundlagen- und Prüfnachweise bleiben ergänzende JSON-Metadaten. Eine verlustfreie oder ausführbare Prozesskonvertierung wird nicht behauptet.
- [`verwaltungsakt-pruefung.yaml`](../../config/agent_skills/process-bpmn-mining/references/verwaltungsakt-pruefung.yaml) stellt zwei wiederverwendbare LLM-Prüfprofile bereit: formelle und materielle Rechtmäßigkeit. Die Profile sind eine optionale Ressource des bestehenden `process-bpmn-mining`-Skillpakets, keine eigenständigen registrierten Skills; ein Host muss sie explizit laden. Sie erteilen keine Toolrechte, verkoppeln die Schritte nicht automatisch und dürfen keine verbindliche Rechtsentscheidung erzeugen.
- Für jedes Prüffeld werden Status (`NOT_CHECKED`, `SUPPORTED`, `DEVIATION_FOUND`, `OPEN`, `NOT_APPLICABLE`), Stichtag, Quellenfassung/Fundstelle, Tatsachenbeleg, Begründung und menschlich verantwortliche Prüfung festgehalten. Ein LLM kann strukturieren und auf Lücken hinweisen, aber nicht Zuständigkeit, Rechtmäßigkeit, Heilbarkeit oder Rechtsfolge verbindlich feststellen.

## Fiktiver Demo-Fall

Die Datei [`bimschg-brandenburg-demo.json`](bimschg-brandenburg-demo.json) enthält einen maschinenlesbaren Seed-Datensatz für die Lebenszeit einer fiktiven Biomasseanlage im fiktiven brandenburgischen „Landkreis Havelgrund“. Er beginnt mit Brainstorming, Standortvarianten und einem ausdrücklich hypothetischen B-Plan-Verfahren. B-Plan und BImSchG-Verfahren sind getrennte Vorgänge mit einer bedingten Abhängigkeit; der Seed behauptet nicht, dass jede reale Anlage einen neuen B-Plan benötigt. Die konkrete Genehmigungsstelle ist standortabhängig markiert. Der Fall nimmt für die Demo ein förmliches Verfahren mit UVP-Beteiligung an; die genaue Anlagenziffer der 4. BImSchV bleibt offen und muss für eine reale Anlage fachlich bestimmt werden.

Der Seed enthält:

- fiktive Betreiber-, Behörden-, Nachbarschafts- und Verbandsrollen;
- Projektidee, Standortvergleich und kommunaler Planungszweig mit eigener öffentlicher und Behördenbeteiligung;
- getrennte Ebenen für Bundesraumordnung, Landes-/Regionalplanung und kommunalen FNP/B-Plan sowie ein Geodatenlayer- und Schutzgüterprüfmodell;
- verknüpfte, aber getrennte Bauleitplanung, BImSchG-Genehmigung und mögliche Einzelzulassungen;
- Antrag, Nachforderungen, Fachgutachten, Behördenstellungnahmen, Bekanntmachung, Einwendung, Erörterungsprotokoll und Bescheid;
- getrennte Haupt- und Alternativpfade für Klage und Widerspruch, ohne Zulässigkeit oder Erfolg vorwegzunehmen;
- Gebührenfestsetzung mit ausdrücklich illustrativem Betrag;
- Inbetriebnahme, risikoorientierte und anlassbezogene Überwachung, Beschwerde, §-17-Anordnung und Nachprüfung;
- §-15-Anzeige mit behördlicher Einordnung, nachfolgende Änderungsgenehmigung sowie Betriebseinstellung und Nachsorge;
- datierte Ereignisse, Dokument-IDs, Rechtsgrundlagen und den Status jeder noch offenen Entscheidung;
- typisierte Rechtsquellen mit Bundesverordnungs-, Erlass- und Rechtsprechungsbeispiel, zeitbezogenem Geltungsvorbehalt, Auslegungsverknüpfungen und einem leeren Kommentarmetadatenschema;
- Verwaltungsaktprüfprozess als BPMN-orientierter JSON-Graph und zwei explizit ladbare LLM-Prüfprofile für formelle und materielle Rechtmäßigkeit.

Alle Personen, Firmen, Orte, Aktenzeichen, Messwerte, Fristen und Beträge in der JSON-Datei sind Testdaten. Die Daten behaupten nicht, dass die beispielhafte Anlage unter eine konkrete Nummer der 4. BImSchV fällt oder dass ein dargestellter Rechtsbehelf im Einzelfall zulässig ist.

## Demo-Ablauf und Abnahmepunkte

1. **Aktenübersicht:** Anlagenstandort, Zuständigkeit, offene Vollständigkeitsanforderungen, Beteiligungsstatus und nächste manuell bestätigte Frist anzeigen.
2. **Beteiligungsgraph:** Für jede Fachbehörde den Zuständigkeitsgrund, die Anfrage, Stellungnahme und ihre Behandlung im Bescheid verfolgen; Einwendungen mit Antwort und Protokoll verknüpfen.
3. **Bescheidprüfung:** Jede Nebenbestimmung zu Schutzgut, Anlagenkomponente, Messgröße, Nachweis, Frist und Verantwortlichem aufklappen; getrennte Zulassungen sichtbar halten.
4. **Rechtsbehelfe:** Klage und Widerspruch nach Typ, Zielbescheid, Rechtsbehelfsbelehrung und manuell geprüfter Frist filtern; keine automatische Aussage zur aufschiebenden Wirkung.
5. **Betriebsmonitoring:** Messreihen, Prüfberichte, Beschwerden, Inspektionsbefunde, Anordnung und Nachkontrolle in einer Zeitlinie mit Beleglinks verfolgen.
6. **Änderungs- und Stilllegungsroute:** Prüfen, ob eine Änderung noch offen ist, ob der Betrieb bis zur notwendigen Entscheidung blockiert bleibt und ob zum Stilllegungsabschluss noch Nachsorgepflichten offen sind.
7. **Revisionsprüfung:** Für jeden Statuswechsel Akteur, Ereigniszeit, Erfassungszeit, Quelldokumentversion und Rechtsstand nachweisen.

Abnahmekriterien für einen späteren lauffähigen Showcase:

- Der Seed ist syntaktisch valides JSON und alle referenzierten IDs sind auflösbar.
- Kein Widerspruch, keine Klage, Beschwerde, Einwendung oder separate Zulassung wird in einen anderen Vorgangstyp umgedeutet.
- Ein nicht bestätigter Rechtsstand oder eine nicht geprüfte Zuständigkeit bleibt sichtbar „offen“ statt als rechtliche Tatsache dargestellt zu werden.
- Ein B-Plan wird nur als bedingte Planungsroute modelliert und nicht als zwingender Vorläufer jeder BImSchG-Anlage ausgegeben.
- Gemeinde als Planungsträger und LfU als BImSchG-Genehmigungsbehörde bleiben als unterschiedliche Zuständigkeiten erkennbar.
- FNP wird als kommunaler vorbereitender Bauleitplan geführt; Bundes-, Landes-/Regional- und kommunale Planebenen werden nicht vermischt.
- Jeder Geodatenbefund nennt Quelle, Stand, Maßstab, Rechtswirkung und Unsicherheit; ein Overlay allein entscheidet nicht über UVP-Pflicht oder Naturschutzrecht.
- UVPG-Vorprüfung und eigenständige Naturschutzprüfungen sind verknüpft, aber nicht zu einer einzigen automatischen GIS-Entscheidung zusammengezogen.
- Gesetze, Verordnungen, Erlasse/Anweisungen, Urteile, Behördenauffassungen und Kommentierung sind mit eigener Quellenart und Wirkung modelliert; kein Suchrang erzeugt automatisch eine rechtliche Entscheidung.
- Jede Rechtsanwendung verweist auf konkrete Vorschrift und maßgebliche Fassung; spätere Änderungen überschreiben den dokumentierten Rechtsstand nicht.
- Rechtsprechung und Kommentarliteratur werden nur im geprüften sachlichen Umfang herangezogen; urheberrechtlich geschützte Kommentierung wird nicht als Volltext in den Seed kopiert.
- Formelle und materielle Rechtmäßigkeitsprüfung sind getrennte Prozessschichten; Anwendungsbereich, Spezialrecht, Fehlerfolgen und Bekanntgabe bleiben fallbezogen prüfbar.
- LLM-Prüfprofile liefern strukturierte Evidenz- und Lückenlisten, aber keine automatische rechtliche Freigabe.
- JSON-Graph und BPMN-XML-Exportgrenzen sind ausgewiesen; nicht unterstützte Bedingungen oder Lane-Metadaten werden nicht als verlustfrei serialisiert behauptet.
- Öffentlich zugängliche Dokumente sind von nicht redigierten Verfahrensakten unterscheidbar.
- Die Historie eines Bescheids, einer Nebenbestimmung, einer Anlagenänderung und eines Überwachungsbefunds ist ohne stilles Überschreiben rekonstruierbar.

## Maßgebliche Quellen und Prüfhinweise

Rechtsquellen wurden am **19.11.2026** als Einstieg geprüft. Vor einer Umsetzung oder Nutzung mit realen Fällen sind Fassungsstand und Anwendbarkeit erneut zu prüfen.

- [BImSchG](https://www.gesetze-im-internet.de/bimschg/) — insbesondere §§ 4–6 (Genehmigung), § 10 (förmliches Verfahren), §§ 15–17 (Änderungsanzeige, Änderungsgenehmigung, nachträgliche Anordnungen), §§ 20–21 (Betriebsuntersagung/Widerruf) und § 52 (Überwachung).
- [Verwaltungsverfahrensgesetz (VwVfG)](https://www.gesetze-im-internet.de/vwvfg/) — insbesondere §§ 24, 26, 28–29 (Ermittlung, Beweis, Anhörung, Akteneinsicht), § 35 (Verwaltungsakt), §§ 36–40 (Nebenbestimmungen, Bestimmtheit, Begründung, Ermessen), §§ 41, 43–46 (Bekanntgabe, Wirksamkeit und Fehlerfolgen) sowie §§ 48–49 (Rücknahme/Widerruf); Anwendbarkeit und maßgebliche Fassung fallbezogen prüfen.
- [Verwaltungsverfahrensgesetz für das Land Brandenburg (VwVfGBbg)](https://bravors.brandenburg.de/gesetze/vwvfgbbg) — insbesondere §§ 1–2 (Anwendungsbereich/Ausnahmen), § 3 (landesrechtliche Ergänzung) und § 5 (Daten- und Geheimnisschutz); zusammen mit VwVfG und einschlägigem Fachrecht lesen.
- [BauGB](https://www.gesetze-im-internet.de/bbaug/) — insbesondere §§ 1–4 (Bauleitplanung und Beteiligung), §§ 8–10 (Bebauungsplan) sowie §§ 29–35 (planungsrechtliche Zulässigkeit); welcher Pfad greift, ist standort- und vorhabenbezogen.
- [Raumordnungsgesetz (ROG)](https://www.gesetze-im-internet.de/rog_2008/) — Bundes-, Landes- und Regionalplanung, Bindungswirkung und Raumverträglichkeitsprüfung.
- [Landesentwicklungsplan Hauptstadtregion Berlin-Brandenburg (LEP HR)](https://gl.berlin-brandenburg.de/landesplanung/landesentwicklungsplan-hauptstadtregion-berlin-brandenburg-lep-hr/) — Ziele und Grundsätze der Raumordnung für Berlin-Brandenburg.
- [UVPG § 7](https://www.gesetze-im-internet.de/uvpg/__7.html) und [Anlage 3](https://www.gesetze-im-internet.de/uvpg/anlage_3.html) — Vorprüfung und Kriterien einschließlich besonderer örtlicher Gegebenheiten.
- [BNatSchG §§ 15](https://www.gesetze-im-internet.de/bnatschg_2009/__15.html), [34](https://www.gesetze-im-internet.de/bnatschg_2009/__34.html) und [44](https://www.gesetze-im-internet.de/bnatschg_2009/__44.html) — Eingriffsregelung, Natura-2000-Verträglichkeit und Artenschutz, soweit einschlägig.
- [4. BImSchV](https://www.gesetze-im-internet.de/bimschv_4_2013/) — Rechtsverordnung für genehmigungsbedürftige Anlagen und Verfahrenszuordnung.
- [Brandenburgischer Runderlass zur Zusammenarbeit der im Rahmen des BImSchG zuständigen Behörden vom 11.07.2023](https://bravors.brandenburg.de/verwaltungsvorschriften/zusammenarbeit_bimschg_2023) — amtliche Verwaltungsvorschrift; Geltungsstatus und Adressaten sind zum Vorgangszeitpunkt zu prüfen.
- [BVerwG, Urteil vom 25.03.2026 – 7 C 3.25](https://www.bverwg.de/de/250326U7C3.25.0) — Rechtsprechungsbeispiel zur Konzentrationswirkung einer fingierten Änderungsgenehmigung für Windenergieanlagen; keine automatische Übertragung auf den Demo-Fall.
- [Verwaltungsvorschriften im Internet](https://www.verwaltungsvorschriften-im-internet.de/) und [Rechtsprechung im Internet](https://www.rechtsprechung-im-internet.de/) — Rechercheeinstiege; maßgeblich bleiben Originalfundstelle, Volltext, Geltungsstatus und Einzelfallprüfung.
- [Geoportal Brandenburg](https://geoportal.brandenburg.de/) und [LfU-Kartenanwendung](https://lfu.brandenburg.de/lfu/de/aufgaben/natur/naturschutzfachdaten/kartenanwendung/) — Einstieg in Geodienste/Geodaten; Nutzungsbedingungen und Zitierpflicht sind je Datensatzmetadatum zu prüfen.
- [Brandenburgisches Planungsportal: kommunale Bauleitplanung](https://buergerbeteiligung-mil.brandenburg.de/bbp/de/planungsverfahren/kommunale-bauleitplanung/ueberblick/) — kommunale Planungshoheit und Zugang zu Bauleitplan-Beteiligungen.
- [Gemeinsame Raumordnungsverfahrensverordnung Berlin-Brandenburg](https://bravors.brandenburg.de/de/verordnungen-212509) — bedingter Raumordnungsprozess bei raumbedeutsamen Planungen.
- [9. BImSchV](https://www.gesetze-im-internet.de/bimschv_9/) — Verfahrensausgestaltung; die konkrete Beteiligungs- und Fristenlogik ist anhand der geltenden Fassung zu implementieren.
- [4. BImSchV](https://www.gesetze-im-internet.de/bimschv_4_2013/) und [UVPG](https://www.gesetze-im-internet.de/uvpg/) — Anlagenzuordnung und UVP-Prüfung.
- [VwVfG § 70](https://www.gesetze-im-internet.de/vwvfg/__70.html), [VwGO §§ 68](https://www.gesetze-im-internet.de/vwgo/__68.html), [70](https://www.gesetze-im-internet.de/vwgo/__70.html) und [74](https://www.gesetze-im-internet.de/vwgo/__74.html) — Vorverfahren, Widerspruch und Klagefrist; das Modell übernimmt keine pauschale Rechtsbehelfsroute.
- [Brandenburg: Immissionsschutzrechtliche Genehmigungsverfahren](https://mleuv.brandenburg.de/mleuv/de/umwelt/immissionsschutz/genehmigungsverfahren/) — LfU-Zuständigkeit, Verfahrensarten, Konzentrationswirkung und separate wasserrechtliche Erlaubnisse.
- [Brandenburg: Anlagenüberwachung](https://lfu.brandenburg.de/lfu/de/aufgaben/immissionsschutz/anlagenueberwachung/) — regelmäßige risikoorientierte und anlassbezogene Überwachung.
- [Brandenburg: Änderungsanzeige nach § 15 Abs. 1 BImSchG](https://lfu.brandenburg.de/lfu/de/aufgaben/immissionsschutz/anlagenueberwachung/aenderungsanzeige-15-1-bimschg/) — Anzeigeweg und Voraussetzungen für den Beginn der Änderung.
- [Gebührenordnung Umwelt Brandenburg](https://bravors.brandenburg.de/de/verordnungen-212886?history) — Gebühren und Tarifstellen; für eine produktive Berechnung muss die zum Vorgangsdatum geltende Fassung herangezogen werden.

**Offene fachliche Validierung vor Produktivsetzung:** Planungsrechtlicher Pfad, räumliche Datenstände/-lizenzen, Gemeinde-/Kreiszuständigkeiten, konkrete LfU-Zuständigkeit, 4.-BImSchV-Ziffer, UVP-Entscheidung, Naturschutzprüfungen, fachbehördliche Beteiligungsliste, Rechtsbehelfsbelehrungen, Gebührenformeln sowie Anzeige-/Stilllegungsanforderungen müssen für den ausgewählten Anlagenfall bestätigt werden. In den konsultierten ThemisDB-Wiki-Artefakten gibt es keinen kuratierten Fachkontext für deutsches Bauplanungs-, Raumordnungs-, Naturschutz- und BImSchG-Verwaltungsverfahrensrecht; diese Lücke wird durch die hier verlinkten Primärquellen kenntlich gemacht, nicht durch die Wiki-Synthese ersetzt.
