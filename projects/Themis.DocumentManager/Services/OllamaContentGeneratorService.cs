/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OllamaContentGeneratorService.cs                   ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service zur Generierung authentischer Pseudo-Daten mittels lokaler Ollama LLM
/// Generiert realistische Briefe, Notizen, Tabellen, Formulare
/// </summary>
public class OllamaContentGeneratorService
{
    private readonly OllamaService _ollamaService;
    private bool _isAvailable = false;

    public OllamaContentGeneratorService(string endpoint = "http://localhost:11434")
    {
        _ollamaService = new OllamaService(endpoint);
    }

    /// <summary>
    /// Prüft Verfügbarkeit der Ollama-Instanz
    /// </summary>
    public async Task<bool> CheckAvailabilityAsync()
    {
        try
        {
            var models = await _ollamaService.GetAvailableModelsAsync();
            _isAvailable = models.Count > 0;
            return _isAvailable;
        }
        catch
        {
            _isAvailable = false;
            return false;
        }
    }

    /// <summary>
    /// Generiert einen authentischen Brief
    /// </summary>
    public async Task<string> GenerateLetterAsync(string department, string subject, CancellationToken cancellationToken = default)
    {
        if (!_isAvailable) return GenerateFallbackLetter(department, subject);

        var prompt = $"""
            Generiere einen authentischen deutschen behördlichen Brief:
            - Behörde: {department}
            - Betreff: {subject}
            - Format: Deutsch, professionell, realistische Inhalte
            - Struktur: Datum, Anrede, Ansprache, Sachverhalt, Rechtlicher Bezug, Entscheidung, Schluss
            - Länge: 300-500 Wörter
            
            Antworte NUR mit dem Brief-Text, keine Metadaten.
            """;

        try
        {
            return await _ollamaService.ChatAsync(prompt, "llama2", cancellationToken);
        }
        catch
        {
            return GenerateFallbackLetter(department, subject);
        }
    }

    /// <summary>
    /// Generiert eine authentische Notiz
    /// </summary>
    public async Task<string> GenerateNoteAsync(string topic, string context, CancellationToken cancellationToken = default)
    {
        if (!_isAvailable) return GenerateFallbackNote(topic, context);

        var prompt = $"""
            Generiere eine authentische deutsche Verwaltungsnotiz:
            - Thema: {topic}
            - Kontext: {context}
            - Format: Kurz, präzise, professionell
            - Struktur: Datum, Uhrzeit, Verfasser (Kürzel), Notiztext
            - Länge: 100-200 Wörter
            
            Antworte NUR mit der Notiz, keine zusätzlichen Informationen.
            """;

        try
        {
            return await _ollamaService.ChatAsync(prompt, "llama2", cancellationToken);
        }
        catch
        {
            return GenerateFallbackNote(topic, context);
        }
    }

    /// <summary>
    /// Generiert Tabellendaten
    /// </summary>
    public async Task<string> GenerateTableAsync(string title, int rows, string[] columns, CancellationToken cancellationToken = default)
    {
        if (!_isAvailable) return GenerateFallbackTable(title, rows, columns);

        var columnList = string.Join(", ", columns);
        var prompt = $"""
            Generiere eine realistische deutsche Verwaltungstabelle:
            - Titel: {title}
            - Spalten: {columnList}
            - Anzahl Zeilen: {rows}
            - Format: CSV-ähnlich mit Trennzeichen |
            - Daten: Authentisch, plausibel, variiert
            
            Antworte NUR mit der Tabelle im Format: Kopfzeile | Daten-Zeilen
            Kein Markdown, nur reiner Text.
            """;

        try
        {
            return await _ollamaService.ChatAsync(prompt, "llama2", cancellationToken);
        }
        catch
        {
            return GenerateFallbackTable(title, rows, columns);
        }
    }

    /// <summary>
    /// Generiert Formular-Einträge
    /// </summary>
    public async Task<Dictionary<string, string>> GenerateFormDataAsync(string formType, string[] fields, CancellationToken cancellationToken = default)
    {
        if (!_isAvailable) return GenerateFallbackFormData(formType, fields);

        var fieldList = string.Join(", ", fields);
        var prompt = $"""
            Generiere realistische Daten für ein deutsches Verwaltungsformular:
            - Formulartyp: {formType}
            - Felder: {fieldList}
            - Format: Authentisch, plausibel, vollständig
            - Sprache: Deutsch
            
            Antworte im Format: FELDNAME: Wert
            Ein Feld pro Zeile, keine Zusätze.
            """;

        try
        {
            var content = await _ollamaService.ChatAsync(prompt, "llama2", cancellationToken);
            return ParseFormData(content, fields);
        }
        catch
        {
            return GenerateFallbackFormData(formType, fields);
        }
    }

    private string GenerateFallbackLetter(string department, string subject)
    {
        return $"""
            Mannheim, {DateTime.Now:d. MMMM yyyy}
            
            Sehr geehrte Damen und Herren,

            mit Schreiben vom {DateTime.Now.AddDays(-5):d. MMMM yyyy} haben Sie Ihren Antrag zur {subject} eingereicht.

            Nach Prüfung Ihres Antrags durch die zuständige Stelle des {department} haben wir festgestellt, dass alle erforderlichen Unterlagen vorliegen und die antragsgegenständliche Maßnahme den geltenden Bestimmungen entspricht.

            Gemäß {(new Random().Next() % 2 == 0 ? "§ 19 VwVfG" : "§ 35a VwVfG")} teilen wir Ihnen hiermit mit, dass Ihr Antrag genehmigt wird.

            Die Genehmigung gilt ab sofort und wird regelmäßig überprüft.

            Mit freundlichen Grüßen

            {department}
            """;
    }

    private string GenerateFallbackNote(string topic, string context)
    {
        return $"""
            {DateTime.Now:dd.MM.yyyy HH:mm} Uhr - Notiz
            Verfasser: MN/ST
            
            {topic}: {context}
            
            Beobachtung: Sachverhalt geprüft. Weitere Maßnahmen erforderlich.
            Fortfolgung: Nachbearbeitung am {DateTime.Now.AddDays(3):dd.MM.yyyy} geplant.
            """;
    }

    private string GenerateFallbackTable(string title, int rows, string[] columns)
    {
        var lines = new List<string>
        {
            $"Tabelle: {title}",
            "---",
            string.Join(" | ", columns),
            string.Join(" | ", Enumerable.Range(0, columns.Length).Select(_ => "---"))
        };

        for (int i = 1; i <= rows; i++)
        {
            var values = columns.Select((col, idx) => GenerateTableCellValue(col, idx)).ToList();
            lines.Add(string.Join(" | ", values));
        }

        return string.Join("\n", lines);
    }

    private Dictionary<string, string> GenerateFallbackFormData(string formType, string[] fields)
    {
        var result = new Dictionary<string, string>();
        foreach (var field in fields)
        {
            result[field] = GenerateFormFieldValue(field);
        }
        return result;
    }

    private string GenerateTableCellValue(string column, int index)
    {
        return column switch
        {
            var c when c.Contains("Datum") => DateTime.Now.AddDays(-new Random().Next(30)).ToString("dd.MM.yyyy"),
            var c when c.Contains("Status") => new[] { "Offen", "In Bearbeitung", "Abgeschlossen" }[new Random().Next(3)],
            var c when c.Contains("Betrag") => $"{new Random().Next(100, 10000)},00 €",
            _ => $"Wert_{index}"
        };
    }

    private string GenerateFormFieldValue(string fieldName)
    {
        return fieldName switch
        {
            var f when f.Contains("Name") => "Max Mustermann",
            var f when f.Contains("Adresse") => "Hauptstraße 42, 68159 Mannheim",
            var f when f.Contains("Telefon") => "0621 123-4567",
            var f when f.Contains("E-Mail") => "max.mustermann@example.de",
            var f when f.Contains("Datum") => DateTime.Now.ToString("dd.MM.yyyy"),
            var f when f.Contains("Unterschrift") => "________________________",
            _ => "Eintrag"
        };
    }

    private Dictionary<string, string> ParseFormData(string content, string[] fields)
    {
        var result = new Dictionary<string, string>();
        var lines = content.Split(new[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);

        foreach (var line in lines)
        {
            if (line.Contains(":"))
            {
                var parts = line.Split(new[] { ':' }, 2, StringSplitOptions.TrimEntries);
                if (parts.Length == 2)
                {
                    result[parts[0]] = parts[1];
                }
            }
        }

        return result;
    }
}
