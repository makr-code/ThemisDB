/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisTestDataGenerator.cs                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     671                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Generator für repräsentative Testdaten zur ThemisDB-Befüllung
/// Generiert realistische deutsche Verwaltungsdaten
/// </summary>
public class ThemisTestDataGenerator
{
    private Random _random = new Random(42); // Fixed seed für Reproduzierbarkeit
    private OllamaContentGeneratorService? _ollamaService;

    public ThemisTestDataGenerator(OllamaContentGeneratorService? ollamaService = null)
    {
        _ollamaService = ollamaService;
    }

    #region Stammdaten

    private readonly List<string> _departments = new()
    {
        "Bauordnungsamt", "Liegenschaftsamt", "Ordnungsamt", "Sozialamt",
        "Jugendamt", "Umweltamt", "Verkehrsamt", "Standesamt",
        "Finanzverwaltung", "Personalverwaltung", "Kämmerei", "Hauptamt",
        "Schulverwaltungsamt", "Kulturamt", "Sportamt", "Gesundheitsamt"
    };

    private readonly List<string> _authorities = new()
    {
        "Stadt Mannheim", "Landratsamt Rhein-Neckar-Kreis", "Stadt Heidelberg",
        "Stadt Ludwigshafen", "Gemeindeverwaltung Schwetzingen", "Stadt Speyer",
        "Landratsamt Karlsruhe", "Stadt Karlsruhe", "Regierungspräsidium Karlsruhe",
        "Stadt Freiburg", "Landkreis Breisgau-Hochschwarzwald"
    };

    private readonly List<string> _processTypes = new()
    {
        "Baugenehmigung", "Betriebsgenehmigung", "Gewerbeanmeldung",
        "Verkehrsregelung", "Umweltgenehmigung", "Fördermittelbescheid",
        "Personalakte", "Vertragsangelegenheit", "Beschaffungsvorgang",
        "Grundstücksangelegenheit", "Sozialleistung", "Ordnungswidrigkeit",
        "BImSchG-Genehmigung", "BImSchG-Anzeige", "BImSchG-Überwachung"
    };

    private readonly List<string> _statusValues = new()
    {
        "Eingang", "In Bearbeitung", "Rückfrage", "Externe Prüfung",
        "Wartend", "Genehmigt", "Abgelehnt", "Archiviert", "Wiedervorgelegt"
    };

    private readonly List<string> _priorities = new()
    {
        "Niedrig", "Normal", "Hoch", "Dringend", "Sofort"
    };

    private readonly List<string> _firstNames = new()
    {
        "Anna", "Julia", "Laura", "Sophie", "Marie", "Lisa", "Lena", "Emma",
        "Lukas", "Leon", "Tim", "Paul", "Felix", "Max", "Jonas", "Finn",
        "Michael", "Thomas", "Stefan", "Andreas", "Markus", "Christian",
        "Sabine", "Petra", "Andrea", "Susanne", "Martina", "Claudia"
    };

    private readonly List<string> _lastNames = new()
    {
        "Müller", "Schmidt", "Schneider", "Fischer", "Weber", "Meyer",
        "Wagner", "Becker", "Schulz", "Hoffmann", "Koch", "Bauer",
        "Richter", "Klein", "Wolf", "Schröder", "Neumann", "Schwarz",
        "Zimmermann", "Braun", "Krüger", "Hofmann", "Hartmann", "Lange"
    };

    private readonly List<string> _streetNames = new()
    {
        "Hauptstraße", "Bahnhofstraße", "Schulstraße", "Kirchstraße",
        "Marktplatz", "Gartenstraße", "Lindenstraße", "Ringstraße",
        "Bergstraße", "Waldweg", "Schillerstraße", "Goethestraße",
        "Mozartstraße", "Beethovenstraße", "Kantstraße", "Friedrichstraße"
    };

    private readonly List<string> _cities = new()
    {
        "Mannheim", "Heidelberg", "Ludwigshafen", "Speyer", "Schwetzingen",
        "Hockenheim", "Weinheim", "Wiesloch", "Sinsheim", "Karlsruhe",
        "Bruchsal", "Pforzheim", "Freiburg", "Offenburg", "Rastatt"
    };

    private readonly List<string> _legalBases = new()
    {
        "§ 29 BauGB", "§ 34 BauGB", "§ 35 BauGB",
        "§ 14 GewO", "§ 15 GewO", "§ 38 GewO",
        "§ 4 BImSchG", "§ 5 BImSchG", "§ 6 BImSchG", "§ 10 BImSchG", "§ 13 BImSchG", "§ 15 BImSchG", "§ 16 BImSchG", "§ 17 BImSchG", "§ 20 BImSchG", "§ 22 BImSchG", "§ 24 BImSchG",
        "§ 8 SGB VIII", "§ 27 SGB VIII", "§ 35a SGB VIII",
        "Art. 3 GG", "Art. 12 GG", "Art. 14 GG",
        "§ 123 VwGO", "§ 80 VwGO", "§ 88 VwGO"
    };

    private readonly List<string> _topics = new()
    {
        "Baurecht", "Gewerbewesen", "Umweltschutz", "Verkehrssicherheit",
        "Sozialleistungen", "Jugendhilfe", "Grundstücksverwaltung",
        "Personalangelegenheiten", "Haushaltswesen", "Beschaffung",
        "Kulturförderung", "Sportförderung", "Gesundheitswesen"
    };

    #endregion

    /// <summary>
    /// Generiert kompletten Metadaten-Datensatz
    /// </summary>
    public DocumentMetadataBinding GenerateMetadata(int seed = -1)
    {
        if (seed >= 0) _random = new Random(seed);

        var binding = new DocumentMetadataBinding
        {
            DocumentId = $"DOC-{Guid.NewGuid().ToString().Substring(0, 8).ToUpper()}",
            ProcessId = $"PROC-{_random.Next(100000, 999999)}",
            Status = BindingStatus.Active,
            CreatedAt = RandomDate(DateTime.Now.AddYears(-2), DateTime.Now),
            CreatedBy = RandomPerson()
        };

        binding.BoundFields = GenerateAllFields();

        return binding;
    }

    /// <summary>
    /// Generiert alle 50 Metadatenfelder (erweitert von StandardMetadataFields)
    /// </summary>
    public List<MetadataField> GenerateAllFields()
    {
        var fields = new List<MetadataField>();

        // Zeitliche Daten (Zeit-Kategorie)
        fields.Add(CreateField("Erstellungsdatum", "document.createdAt", FieldType.Date, true, RandomDate(DateTime.Now.AddYears(-1), DateTime.Now).ToString("dd.MM.yyyy")));
        fields.Add(CreateField("Letzte Änderung", "document.modifiedAt", FieldType.DateTime, false, RandomDateWithTime(DateTime.Now.AddMonths(-3), DateTime.Now).ToString("dd.MM.yyyy HH:mm")));
        fields.Add(CreateField("Eingangsdatum", "process.receivedDate", FieldType.Date, true, RandomDate(DateTime.Now.AddMonths(-6), DateTime.Now).ToString("dd.MM.yyyy")));
        fields.Add(CreateField("Wiedervorlage", "process.nextDeadline", FieldType.Date, false, RandomBool(0.6) ? RandomDate(DateTime.Now, DateTime.Now.AddMonths(6)).ToString("dd.MM.yyyy") : ""));
        fields.Add(CreateField("Frist", "process.deadline", FieldType.Date, false, RandomBool(0.7) ? RandomDate(DateTime.Now.AddDays(7), DateTime.Now.AddMonths(3)).ToString("dd.MM.yyyy") : ""));

        // Organisation (Organisation-Kategorie)
        fields.Add(CreateField("Behörde", "authority.name", FieldType.Text, true, RandomItem(_authorities)));
        fields.Add(CreateField("Abteilung", "process.department", FieldType.Text, true, RandomItem(_departments)));
        fields.Add(CreateField("Referat", "process.division", FieldType.Text, false, RandomBool(0.5) ? $"Referat {_random.Next(1, 9)}" : ""));
        fields.Add(CreateField("Organisationseinheit", "authority.organizationUnit", FieldType.Text, false, RandomBool(0.4) ? $"OE-{_random.Next(100, 999)}" : ""));

        // Vorgangsdetails (Vorgang-Kategorie)
        fields.Add(CreateField("Aktenzeichen", "process.fileReference", FieldType.Text, true, GenerateFileReference()));
        fields.Add(CreateField("Aktennummer", "file.fileNumber", FieldType.Text, true, GenerateFileNumber()));
        fields.Add(CreateField("Geschäftszeichen", "file.businessReference", FieldType.Text, false, RandomBool(0.7) ? $"GZ-{_random.Next(1000, 9999)}/{DateTime.Now.Year}" : ""));
        fields.Add(CreateField("Vorgangsart", "process.type", FieldType.Text, true, RandomItem(_processTypes)));
        fields.Add(CreateField("Betreff", "process.subject", FieldType.Text, true, GenerateSubject()));
        fields.Add(CreateField("Vorgangsnummer", "process.number", FieldType.Text, false, $"VG-{_random.Next(100000, 999999)}"));
        fields.Add(CreateField("Aktenplan", "file.filePlan", FieldType.Text, false, RandomBool(0.6) ? GenerateFilePlan() : ""));

        // Status & Workflow (Status-Kategorie)
        fields.Add(CreateField("Status", "process.status", FieldType.Text, true, RandomItem(_statusValues)));
        fields.Add(CreateField("Bearbeitungsstand", "process.progress", FieldType.Text, false, RandomBool(0.8) ? $"{_random.Next(10, 100)}% abgeschlossen" : ""));
        fields.Add(CreateField("Workflow-Phase", "process.workflowPhase", FieldType.Text, false, RandomBool(0.7) ? $"Phase {_random.Next(1, 5)}" : ""));
        fields.Add(CreateField("Freigabestatus", "process.approvalStatus", FieldType.Text, false, RandomBool(0.5) ? RandomItem(new List<string> { "Offen", "Freigegeben", "Abgelehnt" }) : ""));

        // Priorität & Fristen (Priorität-Kategorie)
        fields.Add(CreateField("Priorität", "process.priority", FieldType.Text, true, RandomItem(_priorities)));
        fields.Add(CreateField("Dringlichkeit", "process.urgency", FieldType.Text, false, RandomBool(0.6) ? RandomItem(new List<string> { "Normal", "Eilig", "Sehr eilig" }) : ""));
        fields.Add(CreateField("Bearbeitungsfrist", "process.processingDeadline", FieldType.Text, false, RandomBool(0.7) ? $"{_random.Next(1, 30)} Tage" : ""));

        // Beteiligte Personen (Personen-Kategorie)
        fields.Add(CreateField("Sachbearbeiter", "process.assignedTo", FieldType.Text, true, RandomPerson()));
        fields.Add(CreateField("Ersteller", "document.createdBy", FieldType.Text, true, RandomPerson()));
        fields.Add(CreateField("Letzte Bearbeitung durch", "document.modifiedBy", FieldType.Text, false, RandomPerson()));
        fields.Add(CreateField("Verantwortlicher", "process.responsible", FieldType.Text, false, RandomBool(0.8) ? RandomPerson() : ""));
        fields.Add(CreateField("Vertretung", "process.deputy", FieldType.Text, false, RandomBool(0.4) ? RandomPerson() : ""));
        fields.Add(CreateField("Antragsteller", "process.applicant", FieldType.Text, false, RandomBool(0.7) ? RandomPerson() : ""));
        fields.Add(CreateField("Zuständigkeit", "process.responsibility", FieldType.Text, false, RandomBool(0.8) ? RandomItem(_departments) : ""));
        fields.Add(CreateField("E-Mail Sachbearbeiter", "contact.email", FieldType.Text, false, RandomBool(0.7) ? GenerateEmail(fields.FirstOrDefault(f => f.FieldName == "Sachbearbeiter")?.CurrentValue ?? "") : ""));
        fields.Add(CreateField("Telefon", "contact.phone", FieldType.Text, false, RandomBool(0.7) ? GeneratePhoneNumber() : ""));
        fields.Add(CreateField("Fax", "contact.fax", FieldType.Text, false, RandomBool(0.4) ? GeneratePhoneNumber() : ""));

        // Rechtsgrundlagen (Rechtsgrundlagen-Kategorie)
        fields.Add(CreateField("Rechtsgrundlage", "process.legalBasis", FieldType.Text, false, RandomBool(0.8) ? RandomItem(_legalBases) : ""));
        fields.Add(CreateField("Weitere Rechtsgrundlagen", "process.additionalLegalBases", FieldType.Text, false, RandomBool(0.5) ? string.Join(", ", RandomItems(_legalBases, 2)) : ""));
        fields.Add(CreateField("Gesetzliche Frist", "process.statutoryDeadline", FieldType.Text, false, RandomBool(0.6) ? $"{_random.Next(1, 12)} Monate" : ""));

        // Finanzdaten (Finanzen-Kategorie)
        fields.Add(CreateField("Betrag", "process.amount", FieldType.Number, false, RandomBool(0.6) ? $"{_random.Next(100, 50000):N2} €" : ""));
        fields.Add(CreateField("Haushaltsstelle", "finance.budgetItem", FieldType.Text, false, RandomBool(0.5) ? $"HST-{_random.Next(1000, 9999)}" : ""));
        fields.Add(CreateField("Kostenstelle", "finance.costCenter", FieldType.Text, false, RandomBool(0.5) ? $"KST-{_random.Next(100, 999)}" : ""));
        fields.Add(CreateField("Gebühr", "finance.fee", FieldType.Number, false, RandomBool(0.4) ? $"{_random.Next(50, 5000):N2} €" : ""));

        // Räumliche Zuordnung (Räumlich-Kategorie)
        fields.Add(CreateField("Flurstück", "geo.parcel", FieldType.Text, false, RandomBool(0.5) ? $"Flst. {_random.Next(100, 9999)}" : ""));
        fields.Add(CreateField("Flur", "geo.flur", FieldType.Text, false, RandomBool(0.5) ? $"Flur {_random.Next(1, 50)}" : ""));
        fields.Add(CreateField("Gemarkung", "geo.district", FieldType.Text, false, RandomBool(0.5) ? RandomItem(_cities) : ""));
        fields.Add(CreateField("Straße", "geo.street", FieldType.Text, false, RandomBool(0.8) ? RandomItem(_streetNames) : ""));
        fields.Add(CreateField("Hausnummer", "geo.houseNumber", FieldType.Text, false, RandomBool(0.8) ? $"{_random.Next(1, 150)}{(RandomBool(0.2) ? RandomItem(new List<string> { "a", "b", "c" }) : "")}" : ""));
        fields.Add(CreateField("Adresszusatz", "geo.addressSupplement", FieldType.Text, false, RandomBool(0.2) ? RandomItem(new List<string> { "Hinterhaus", "OG links", "Erdgeschoss", "3. Stock" }) : ""));
        fields.Add(CreateField("PLZ", "geo.postalCode", FieldType.Text, false, RandomBool(0.8) ? $"{_random.Next(68000, 77000)}" : ""));
        fields.Add(CreateField("Ort", "geo.city", FieldType.Text, false, RandomBool(0.8) ? RandomItem(_cities) : ""));
        fields.Add(CreateField("Ortsteil", "geo.district", FieldType.Text, false, RandomBool(0.4) ? RandomItem(new List<string> { "Innenstadt", "Nord", "Süd", "Ost", "West" }) : ""));
        fields.Add(CreateField("Vollständige Anschrift", "geo.fullAddress", FieldType.Text, false, RandomBool(0.8) ? GenerateFullAddress() : ""));
        fields.Add(CreateField("GPS-Koordinaten", "geo.coordinates", FieldType.Text, false, RandomBool(0.3) ? $"{49 + _random.NextDouble():F6}, {8 + _random.NextDouble():F6}" : ""));

        // Thematische Zuordnung (Thematik-Kategorie)
        fields.Add(CreateField("Thema", "metadata.topic", FieldType.Text, false, RandomBool(0.8) ? RandomItem(_topics) : ""));
        fields.Add(CreateField("Schlagwörter", "metadata.keywords", FieldType.Text, false, RandomBool(0.6) ? string.Join(", ", RandomItems(_topics, 3)) : ""));
        fields.Add(CreateField("Kategorie", "metadata.category", FieldType.Text, false, RandomBool(0.7) ? RandomItem(new List<string> { "A", "B", "C", "D" }) : ""));

        // Technische Metadaten (Technisch-Kategorie)
        fields.Add(CreateField("Dokumenttyp", "document.type", FieldType.Text, true, RandomItem(new List<string> { "Antrag", "Bescheid", "Protokoll", "Vertrag", "Schreiben" })));
        fields.Add(CreateField("Version", "document.version", FieldType.Text, false, $"v{_random.Next(1, 5)}.{_random.Next(0, 10)}"));
        fields.Add(CreateField("Dateiformat", "document.format", FieldType.Text, false, RandomItem(new List<string> { "PDF", "DOCX", "XLSX" })));
        fields.Add(CreateField("Dateigröße", "document.size", FieldType.Text, false, $"{_random.Next(50, 5000)} KB"));

        // BImSchG-spezifische Felder (wenn Vorgangsart BImSchG)
        var vorgangsart = fields.FirstOrDefault(f => f.FieldName == "Vorgangsart")?.CurrentValue ?? "";
        if (vorgangsart.Contains("BImSchG"))
        {
            fields.Add(CreateField("Anlagentyp", "bimschg.facilityType", FieldType.Text, false, RandomItem(new List<string> { "4. BImSchV Anlage", "Genehmigungsbedürftige Anlage", "Nicht genehmigungsbedürftige Anlage" })));
            fields.Add(CreateField("Spalte 1 4. BImSchV", "bimschg.column1", FieldType.Text, false, RandomBool(0.8) ? $"{_random.Next(1, 10)}.{_random.Next(1, 20)}" : ""));
            fields.Add(CreateField("Emissionsart", "bimschg.emissionType", FieldType.Text, false, RandomBool(0.7) ? RandomItem(new List<string> { "Staub", "Lärm", "Geruch", "Abwasser", "Luftschadstoffe" }) : ""));
            fields.Add(CreateField("Grenzwert", "bimschg.limitValue", FieldType.Text, false, RandomBool(0.6) ? $"{_random.Next(10, 100)} mg/m³" : ""));
            fields.Add(CreateField("Messstelle", "bimschg.measurementPoint", FieldType.Text, false, RandomBool(0.5) ? $"MP-{_random.Next(1, 20)}" : ""));
            fields.Add(CreateField("Prüfbericht", "bimschg.inspectionReport", FieldType.Text, false, RandomBool(0.6) ? $"PR-{_random.Next(1000, 9999)}/{DateTime.Now.Year}" : ""));
            fields.Add(CreateField("Überwachungsturnus", "bimschg.monitoringInterval", FieldType.Text, false, RandomBool(0.7) ? RandomItem(new List<string> { "Jährlich", "Halbjährlich", "Quartalsweise", "Monatlich" }) : ""));
        }

        // Aktionen & Aufgaben (Aktionen-Kategorie)
        fields.Add(CreateField("Nächster Schritt", "process.nextAction", FieldType.Text, false, RandomBool(0.7) ? RandomItem(new List<string> { "Prüfung", "Rücksprache", "Bescheid erstellen", "Archivierung" }) : ""));
        fields.Add(CreateField("Aufgabe", "task.description", FieldType.Text, false, RandomBool(0.6) ? "Prüfung durchführen und Stellungnahme einholen" : ""));
        fields.Add(CreateField("Erledigungsvermerk", "process.completionNote", FieldType.Text, false, RandomBool(0.3) ? "Vorgang abgeschlossen am " + DateTime.Now.AddDays(-_random.Next(1, 30)).ToString("dd.MM.yyyy") : ""));

        return fields;
    }

    /// <summary>
    /// Generiert Batch von Metadaten (für Massentests)
    /// </summary>
    public List<DocumentMetadataBinding> GenerateBatch(int count)
    {
        var batch = new List<DocumentMetadataBinding>();
        for (int i = 0; i < count; i++)
        {
            batch.Add(GenerateMetadata(i));
        }
        return batch;
    }

    /// <summary>
    /// Generiert spezialisierte Datensätze für bestimmte Vorgangstypen
    /// </summary>
    public DocumentMetadataBinding GenerateSpecializedMetadata(string processType)
    {
        var metadata = GenerateMetadata();
        
        // Passe Felder an Vorgangstyp an
        var vorgangsartField = metadata.BoundFields.First(f => f.FieldName == "Vorgangsart");
        vorgangsartField.CurrentValue = processType;

        var betreffField = metadata.BoundFields.First(f => f.FieldName == "Betreff");
        betreffField.CurrentValue = GenerateSpecializedSubject(processType);

        return metadata;
    }

    #region Helper Methods

    private MetadataField CreateField(string name, string path, FieldType type, bool required, string value)
    {
        return new MetadataField
        {
            FieldName = name,
            ThemisPath = path,
            Type = type,
            IsRequired = required,
            CurrentValue = value,
            LastUpdated = RandomBool(0.8) ? RandomDate(DateTime.Now.AddMonths(-6), DateTime.Now) : null
        };
    }

    private string GenerateFileReference()
    {
        var dept = _random.Next(1, 99);
        var year = DateTime.Now.Year - _random.Next(0, 3);
        var number = _random.Next(1000, 9999);
        return $"{dept:D2}-{year}-{number}";
    }

    private string GenerateFileNumber()
    {
        var prefix = RandomItem(new List<string> { "A", "B", "F", "G", "K", "V" });
        var year = DateTime.Now.Year % 100;
        var number = _random.Next(10000, 99999);
        return $"{prefix}{year:D2}-{number}";
    }

    private string GeneratePhoneNumber()
    {
        var areaCode = _random.Next(100, 999);
        var exchange = _random.Next(100, 999);
        var subscriber = _random.Next(1000, 9999);
        return $"0{areaCode} {exchange}-{subscriber}";
    }

    private string GenerateEmail(string personName)
    {
        if (string.IsNullOrEmpty(personName)) return "";
        
        var parts = personName.Split(' ');
        if (parts.Length < 2) return "";
        
        var firstName = parts[0].ToLower();
        var lastName = parts[1].ToLower();
        var domain = RandomItem(new List<string> { "mannheim.de", "rhein-neckar-kreis.de", "heidelberg.de", "karlsruhe.de" });
        
        return $"{firstName}.{lastName}@{domain}";
    }

    private string GenerateFullAddress()
    {
        var street = RandomItem(_streetNames);
        var number = _random.Next(1, 150);
        var supplement = RandomBool(0.2) ? RandomItem(new List<string> { "a", "b", "c" }) : "";
        var plz = _random.Next(68000, 77000);
        var city = RandomItem(_cities);
        
        return $"{street} {number}{supplement}, {plz} {city}";
    }

    private string GenerateSubject()
    {
        var subjects = new List<string>
        {
            "Antrag auf Baugenehmigung für Einfamilienhaus",
            "Gewerbeanmeldung Gaststättenbetrieb",
            "Verlängerung der Betriebsgenehmigung",
            "Antrag auf Baumfällgenehmigung",
            "Sondernutzungserlaubnis für Baustelleneinrichtung",
            "Antrag auf Befreiung von Festsetzungen des Bebauungsplans",
            "Stellungnahme zur Verkehrsregelung",
            "Fördermittelantrag für energetische Sanierung",
            "Antrag auf Grundstücksübertragung",
            "Beschaffungsvorgang IT-Ausstattung"
        };
        return RandomItem(subjects);
    }

    private string GenerateSpecializedSubject(string processType)
    {
        return processType switch
        {
            "Baugenehmigung" => $"Baugenehmigung für {RandomItem(new List<string> { "Einfamilienhaus", "Anbau", "Garage", "Gewerbebau" })}",
            "Gewerbeanmeldung" => $"Gewerbeanmeldung {RandomItem(new List<string> { "Einzelhandel", "Gastronomie", "Handwerk", "Dienstleistung" })}",
            "Umweltgenehmigung" => $"Umweltgenehmigung für {RandomItem(new List<string> { "Lärmschutz", "Abwasser", "Emissionen" })}",
            "BImSchG-Genehmigung" => $"BImSchG-Genehmigung für {RandomItem(new List<string> { "Produktionsanlage", "Lackieranlage", "Kraftwerk", "Chemische Anlage" })}",
            "BImSchG-Anzeige" => $"BImSchG-Anzeige {RandomItem(new List<string> { "Änderung Betriebsweise", "Betreiberwechsel", "Anlagenänderung" })}",
            "BImSchG-Überwachung" => $"BImSchG-Überwachung {RandomItem(new List<string> { "Emissionsmessung", "Vor-Ort-Kontrolle", "Prüfbericht" })}",
            _ => GenerateSubject()
        };
    }

    private string GenerateFilePlan()
    {
        var main = _random.Next(1, 9);
        var sub = _random.Next(1, 99);
        var subsub = _random.Next(1, 999);
        return $"{main}.{sub:D2}.{subsub:D3}";
    }

    private string GenerateAddress()
    {
        var street = RandomItem(_streetNames);
        var number = _random.Next(1, 150);
        return $"{street} {number}";
    }

    private string RandomPerson()
    {
        var first = RandomItem(_firstNames);
        var last = RandomItem(_lastNames);
        return $"{first} {last}";
    }

    private string RandomItem(List<string> list) => list[_random.Next(list.Count)];

    private List<string> RandomItems(List<string> list, int count)
    {
        return list.OrderBy(_ => _random.Next()).Take(count).ToList();
    }

    private bool RandomBool(double probability) => _random.NextDouble() < probability;

    private DateTime RandomDate(DateTime start, DateTime end)
    {
        int range = (end - start).Days;
        return start.AddDays(_random.Next(range));
    }

    private DateTime RandomDateWithTime(DateTime start, DateTime end)
    {
        var date = RandomDate(start, end);
        return date.AddHours(_random.Next(8, 18)).AddMinutes(_random.Next(0, 60));
    }

    #endregion

    /// <summary>
    /// Erstellt Statistik über generierten Datensatz
    /// </summary>
    public TestDataStatistics GetStatistics(List<DocumentMetadataBinding> batch)
    {
        return new TestDataStatistics
        {
            TotalDocuments = batch.Count,
            TotalFields = batch.Sum(b => b.BoundFields.Count),
            FilledFields = batch.Sum(b => b.BoundFields.Count(f => !string.IsNullOrEmpty(f.CurrentValue))),
            AverageFillRate = batch.Average(b => b.BoundFields.Count(f => !string.IsNullOrEmpty(f.CurrentValue)) / (double)b.BoundFields.Count) * 100,
            UniqueProcessTypes = batch.SelectMany(b => b.BoundFields.Where(f => f.FieldName == "Vorgangsart").Select(f => f.CurrentValue)).Distinct().Count(),
            UniqueDepartments = batch.SelectMany(b => b.BoundFields.Where(f => f.FieldName == "Abteilung").Select(f => f.CurrentValue)).Distinct().Count()
        };
    }

    /// <summary>
    /// Generiert Datensätze mit authentischen Inhalten (Briefe, Notizen, Tabellen, Formulare)
    /// mit Fortschrittsanzeige
    /// </summary>
    public async Task<List<DocumentMetadataBinding>> GenerateWithAuthenticContentAsync(
        int count,
        bool useLlm = true,
        CancellationToken cancellationToken = default,
        Action<int, string>? progressCallback = null)
    {
        var batch = new List<DocumentMetadataBinding>();
        var ollamaAvailable = useLlm && _ollamaService != null && await _ollamaService.CheckAvailabilityAsync();

        for (int i = 0; i < count; i++)
        {
            progressCallback?.Invoke(i + 1, $"Generiere Dokument {i + 1}/{count}...");
            await Task.Delay(50, cancellationToken); // Verhindert UI-Blockierung

            var metadata = GenerateMetadata(i);
            
            // Zufälliger Inhaltstyp
            var contentType = _random.Next(4);
            var content = contentType switch
            {
                0 => await GenerateLetterContentAsync(metadata, ollamaAvailable),
                1 => await GenerateNoteContentAsync(metadata, ollamaAvailable),
                2 => await GenerateTableContentAsync(metadata, ollamaAvailable),
                3 => await GenerateFormContentAsync(metadata, ollamaAvailable),
                _ => ""
            };

            // Inhalt als Notiz-Feld speichern
            var contentField = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Notizen");
            if (contentField != null)
            {
                contentField.CurrentValue = content;
            }

            batch.Add(metadata);

            if (i % 10 == 0 && i > 0)
            {
                progressCallback?.Invoke(i, $"Verarbeitet: {i}/{count} Dokumente...");
            }
        }

        progressCallback?.Invoke(count, $"Abgeschlossen: {count} Dokumente generiert");
        return batch;
    }

    private async Task<string> GenerateLetterContentAsync(DocumentMetadataBinding metadata, bool useLlm)
    {
        if (!useLlm || _ollamaService == null)
            return GenerateFallbackLetter(metadata);

        var department = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Abteilung")?.CurrentValue ?? "Behörde";
        var subject = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Antrag";

        try
        {
            return await _ollamaService.GenerateLetterAsync(department, subject);
        }
        catch
        {
            return GenerateFallbackLetter(metadata);
        }
    }

    private async Task<string> GenerateNoteContentAsync(DocumentMetadataBinding metadata, bool useLlm)
    {
        if (!useLlm || _ollamaService == null)
            return GenerateFallbackNote(metadata);

        var topic = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Vorgangsart")?.CurrentValue ?? "Notiz";
        var context = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Sachverhalt";

        try
        {
            return await _ollamaService.GenerateNoteAsync(topic, context);
        }
        catch
        {
            return GenerateFallbackNote(metadata);
        }
    }

    private async Task<string> GenerateTableContentAsync(DocumentMetadataBinding metadata, bool useLlm)
    {
        if (!useLlm || _ollamaService == null)
            return GenerateFallbackTable(metadata);

        var title = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Übersicht";
        var columns = new[] { "Datum", "Status", "Beschreibung", "Betrag" };

        try
        {
            return await _ollamaService.GenerateTableAsync(title, 5, columns);
        }
        catch
        {
            return GenerateFallbackTable(metadata);
        }
    }

    private async Task<string> GenerateFormContentAsync(DocumentMetadataBinding metadata, bool useLlm)
    {
        if (!useLlm || _ollamaService == null)
            return GenerateFallbackForm(metadata);

        var formType = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Vorgangsart")?.CurrentValue ?? "Antrag";
        var fields = new[] { "Name", "Adresse", "Telefon", "E-Mail", "Antrag", "Datum" };

        try
        {
            var formData = await _ollamaService.GenerateFormDataAsync(formType, fields);
            return string.Join("\n", formData.Select(kvp => $"{kvp.Key}: {kvp.Value}"));
        }
        catch
        {
            return GenerateFallbackForm(metadata);
        }
    }

    private string GenerateFallbackLetter(DocumentMetadataBinding metadata)
    {
        var department = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Abteilung")?.CurrentValue ?? "Behörde";
        var subject = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Antrag";

        return $"""
            {DateTime.Now:d. MMMM yyyy}

            Sehr geehrte Damen und Herren,

            mit Schreiben vom {DateTime.Now.AddDays(-5):d. MMMM yyyy} haben Sie Ihren Antrag zur {subject} eingereicht.

            Nach gründlicher Prüfung Ihres Antrags durch die zuständigen Stellen des {department} haben wir festgestellt, dass alle erforderlichen Unterlagen vollständig vorliegen und die antragsgegenständliche Maßnahme den geltenden rechtlichen Bestimmungen entspricht.

            Gemäß § {_random.Next(1, 40)} ordnen wir hiermit folgendes an:

            Die erteilte Genehmigung gilt ab sofort und wird regelmäßig überprüft. Sollten Sie Fragen haben, kontaktieren Sie uns bitte.

            Mit freundlichen Grüßen

            {department}
            """;
    }

    private string GenerateFallbackNote(DocumentMetadataBinding metadata)
    {
        var topic = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Vorgangsart")?.CurrentValue ?? "Notiz";

        return $"""
            {DateTime.Now:dd.MM.yyyy HH:mm} Uhr - Verwaltungsnotiz
            Verfasser: {RandomPerson()}
            Thema: {topic}

            Sachverhalt: Angelegenheit wurde geprüft und dokumentiert.
            Aktuelle Situation: In Bearbeitung
            Nächste Schritte: Weiterführung am {DateTime.Now.AddDays(3):dd.MM.yyyy}
            """;
    }

    private string GenerateFallbackTable(DocumentMetadataBinding metadata)
    {
        var sb = new System.Text.StringBuilder();
        var title = metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Übersicht";

        sb.AppendLine($"Tabelle: {title}");
        sb.AppendLine("---");
        sb.AppendLine("Datum | Status | Beschreibung | Betrag");
        sb.AppendLine("----- | ------ | ------------ | ------");

        for (int i = 0; i < 5; i++)
        {
            sb.AppendLine($"{DateTime.Now.AddDays(-i):dd.MM.yyyy} | In Bearbeitung | Eintrag {i + 1} | {_random.Next(100, 5000):N2} €");
        }

        return sb.ToString();
    }

    private string GenerateFallbackForm(DocumentMetadataBinding metadata)
    {
        return $"""
            FORMULARAUSKUNFT
            ================
            
            Name: {RandomPerson()}
            Adresse: {GenerateFullAddress()}
            Telefon: {GeneratePhoneNumber()}
            E-Mail: {GenerateEmail(RandomPerson())}
            Antrag: {metadata.BoundFields.FirstOrDefault(f => f.FieldName == "Betreff")?.CurrentValue ?? "Antrag"}
            Datum: {DateTime.Now:dd.MM.yyyy}
            """;
    }
}

public class TestDataStatistics
{
    public int TotalDocuments { get; set; }
    public int TotalFields { get; set; }
    public int FilledFields { get; set; }
    public double AverageFillRate { get; set; }
    public int UniqueProcessTypes { get; set; }
    public int UniqueDepartments { get; set; }

    public override string ToString()
    {
        return $@"
Testdaten-Statistik:
  Dokumente: {TotalDocuments}
  Felder gesamt: {TotalFields}
  Ausgefüllte Felder: {FilledFields}
  Durchschnittliche Befüllung: {AverageFillRate:F1}%
  Vorgangsarten: {UniqueProcessTypes}
  Abteilungen: {UniqueDepartments}
";
    }
}
