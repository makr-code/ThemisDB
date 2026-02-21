/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisDbSeeder.cs                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     392                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.TestData;

/// <summary>
/// ThemisDB Test-Datenbank Seeder
/// Befüllt ThemisDB mit realistischen Testdaten
/// </summary>
public class ThemisDbSeeder
{
    private readonly ThemisTestDataGenerator _generator;
    private readonly string _exportPath;

    public ThemisDbSeeder(string? exportPath = null)
    {
        _generator = new ThemisTestDataGenerator();
        _exportPath = exportPath ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "ThemisDB_TestData");
    }

    /// <summary>
    /// Generiert und exportiert Testdaten
    /// </summary>
    public async Task<SeedResult> SeedDatabaseAsync(SeedOptions options)
    {
        var result = new SeedResult
        {
            StartTime = DateTime.Now,
            Options = options
        };

        try
        {
            Console.WriteLine($"Generiere {options.DocumentCount} Testdatensätze...");

            // Generiere Batch
            var batch = _generator.GenerateBatch(options.DocumentCount);

            // Optional: Spezialisierte Datensätze hinzufügen
            if (options.IncludeSpecializedData)
            {
                batch.AddRange(GenerateSpecializedBatch(options.SpecializedCount));
            }

            result.GeneratedDocuments = batch.Count;
            result.Statistics = _generator.GetStatistics(batch);

            // Export je nach Modus
            switch (options.ExportMode)
            {
                case ExportMode.Json:
                    await ExportToJsonAsync(batch, options.OutputFileName);
                    break;
                case ExportMode.Csv:
                    await ExportToCsvAsync(batch, options.OutputFileName);
                    break;
                case ExportMode.Sql:
                    await ExportToSqlAsync(batch, options.OutputFileName);
                    break;
                case ExportMode.ThemisDbApi:
                    // TODO: Direkt zu ThemisDB via API
                    await ExportViaApiAsync(batch, options.ThemisDbUrl ?? "http://localhost:8765");
                    break;
            }

            result.Success = true;
            result.EndTime = DateTime.Now;
            result.ExportPath = Path.Combine(_exportPath, options.OutputFileName);

            Console.WriteLine($"✅ Erfolgreich! {result.GeneratedDocuments} Dokumente generiert.");
            Console.WriteLine(result.Statistics.ToString());
        }
        catch (Exception ex)
        {
            result.Success = false;
            result.ErrorMessage = ex.Message;
            Console.WriteLine($"❌ Fehler: {ex.Message}");
        }

        return result;
    }

    /// <summary>
    /// Generiert spezialisierte Datensätze für verschiedene Vorgangstypen
    /// </summary>
    private List<DocumentMetadataBinding> GenerateSpecializedBatch(int count)
    {
        var processTypes = new List<string>
        {
            "Baugenehmigung",
            "Betriebsgenehmigung",
            "Gewerbeanmeldung",
            "Verkehrsregelung",
            "Umweltgenehmigung",
            "Fördermittelbescheid"
        };

        var specialized = new List<DocumentMetadataBinding>();
        int perType = count / processTypes.Count;

        foreach (var type in processTypes)
        {
            for (int i = 0; i < perType; i++)
            {
                specialized.Add(_generator.GenerateSpecializedMetadata(type));
            }
        }

        return specialized;
    }

    /// <summary>
    /// Export als JSON
    /// </summary>
    private async Task ExportToJsonAsync(List<DocumentMetadataBinding> batch, string fileName)
    {
        Directory.CreateDirectory(_exportPath);
        var filePath = Path.Combine(_exportPath, fileName);

        var json = JsonSerializer.Serialize(batch, new JsonSerializerOptions
        {
            WriteIndented = true,
            Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
        });

        await File.WriteAllTextAsync(filePath, json);
        Console.WriteLine($"📄 JSON exportiert: {filePath}");
    }

    /// <summary>
    /// Export als CSV (flach für einfache Analyse)
    /// </summary>
    private async Task ExportToCsvAsync(List<DocumentMetadataBinding> batch, string fileName)
    {
        Directory.CreateDirectory(_exportPath);
        var filePath = Path.Combine(_exportPath, fileName);

        using var writer = new StreamWriter(filePath);

        // Header (nur wichtigste Felder)
        await writer.WriteLineAsync("DocumentId;ProcessId;Aktenzeichen;Betreff;Vorgangsart;Status;Priorität;Sachbearbeiter;Behörde;Abteilung;Erstellungsdatum");

        // Daten
        foreach (var doc in batch)
        {
            var fields = doc.BoundFields.ToDictionary(f => f.FieldName, f => f.CurrentValue ?? "");

            var row = string.Join(";",
                doc.DocumentId,
                doc.ProcessId,
                fields.GetValueOrDefault("Aktenzeichen", ""),
                fields.GetValueOrDefault("Betreff", "").Replace(";", ","),
                fields.GetValueOrDefault("Vorgangsart", ""),
                fields.GetValueOrDefault("Status", ""),
                fields.GetValueOrDefault("Priorität", ""),
                fields.GetValueOrDefault("Sachbearbeiter", ""),
                fields.GetValueOrDefault("Behörde", ""),
                fields.GetValueOrDefault("Abteilung", ""),
                fields.GetValueOrDefault("Erstellungsdatum", "")
            );

            await writer.WriteLineAsync(row);
        }

        Console.WriteLine($"📊 CSV exportiert: {filePath}");
    }

    /// <summary>
    /// Export als SQL INSERT-Statements
    /// </summary>
    private async Task ExportToSqlAsync(List<DocumentMetadataBinding> batch, string fileName)
    {
        Directory.CreateDirectory(_exportPath);
        var filePath = Path.Combine(_exportPath, fileName);

        using var writer = new StreamWriter(filePath);

        await writer.WriteLineAsync("-- ThemisDB Test Data SQL Export");
        await writer.WriteLineAsync($"-- Generated: {DateTime.Now}");
        await writer.WriteLineAsync($"-- Documents: {batch.Count}");
        await writer.WriteLineAsync();

        foreach (var doc in batch)
        {
            await writer.WriteLineAsync($"INSERT INTO documents (id, process_id, status, created_at, created_by)");
            await writer.WriteLineAsync($"VALUES ('{doc.DocumentId}', '{doc.ProcessId}', '{doc.Status}', '{doc.CreatedAt:yyyy-MM-dd HH:mm:ss}', '{doc.CreatedBy}');");

            foreach (var field in doc.BoundFields.Where(f => !string.IsNullOrEmpty(f.CurrentValue)))
            {
                var escapedValue = field.CurrentValue?.Replace("'", "''") ?? "";
                await writer.WriteLineAsync($"INSERT INTO metadata_fields (document_id, field_name, themis_path, field_type, is_required, current_value)");
                await writer.WriteLineAsync($"VALUES ('{doc.DocumentId}', '{field.FieldName}', '{field.ThemisPath}', '{field.Type}', {(field.IsRequired ? 1 : 0)}, '{escapedValue}');");
            }

            await writer.WriteLineAsync();
        }

        Console.WriteLine($"🗄️ SQL exportiert: {filePath}");
    }

    /// <summary>
    /// Export direkt via ThemisDB API
    /// </summary>
    private async Task ExportViaApiAsync(List<DocumentMetadataBinding> batch, string themisDbUrl)
    {
        Console.WriteLine($"🌐 Verbinde mit ThemisDB: {themisDbUrl}");

        var client = new ThemisApiClient(themisDbUrl);
        
        // 1. Prüfe ThemisDB-Verfügbarkeit
        bool isOnline = false;
        try
        {
            isOnline = await client.CheckHealthAsync();
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException(
                $"ThemisDB ist nicht erreichbar unter {themisDbUrl}.\n" +
                $"Bitte starten Sie ThemisDB oder wählen Sie einen anderen Export-Modus (JSON/CSV).\n" +
                $"Details: {ex.Message}", ex);
        }

        if (!isOnline)
        {
            throw new InvalidOperationException(
                $"ThemisDB unter {themisDbUrl} antwortet nicht.\n" +
                $"Bitte starten Sie ThemisDB auf Port 8765 oder wählen Sie einen anderen Export-Modus.");
        }

        Console.WriteLine($"✅ ThemisDB ist online");

        // 2. Dokumente hochladen
        int success = 0;
        int failed = 0;
        var errors = new List<string>();

        foreach (var doc in batch)
        {
            try
            {
                var response = await client.PostAsync<DocumentMetadataBinding, object>("/api/documents/metadata", doc);
                if (response != null)
                {
                    success++;
                    if (success % 10 == 0)
                        Console.Write($"\r✅ {success}/{batch.Count} gesendet...");
                }
                else
                {
                    failed++;
                    errors.Add($"Dokument {doc.DocumentId}: Keine Antwort vom Server");
                }
            }
            catch (Exception ex)
            {
                failed++;
                errors.Add($"Dokument {doc.DocumentId}: {ex.Message}");
                
                // Bei zu vielen Fehlern abbrechen
                if (failed > 5)
                {
                    throw new InvalidOperationException(
                        $"Zu viele Fehler beim Hochladen ({failed} Fehler).\n" +
                        $"Erste Fehler:\n" + string.Join("\n", errors.Take(3)));
                }
            }
        }

        Console.WriteLine();
        Console.WriteLine($"✅ Erfolgreich: {success}");
        
        if (failed > 0)
        {
            Console.WriteLine($"❌ Fehlgeschlagen: {failed}");
            Console.WriteLine("Fehler-Details:");
            foreach (var error in errors.Take(5))
            {
                Console.WriteLine($"  - {error}");
            }
            
            throw new InvalidOperationException(
                $"{failed} von {batch.Count} Dokumenten konnten nicht hochgeladen werden.\n" +
                $"Erfolgreich: {success}\n" +
                $"Fehler: {string.Join(", ", errors.Take(3))}");
        }
    }

    /// <summary>
    /// Schnell-Start für Standard-Seeding
    /// </summary>
    public static async Task QuickSeedAsync(int count = 100)
    {
        var seeder = new ThemisDbSeeder();
        var options = new SeedOptions
        {
            DocumentCount = count,
            ExportMode = ExportMode.Json,
            OutputFileName = $"themis_testdata_{DateTime.Now:yyyyMMdd_HHmmss}.json",
            IncludeSpecializedData = true,
            SpecializedCount = 50
        };

        await seeder.SeedDatabaseAsync(options);
    }
}

/// <summary>
/// Seed-Optionen
/// </summary>
public class SeedOptions
{
    public int DocumentCount { get; set; } = 100;
    public ExportMode ExportMode { get; set; } = ExportMode.Json;
    public string OutputFileName { get; set; } = "themis_testdata.json";
    public bool IncludeSpecializedData { get; set; } = true;
    public int SpecializedCount { get; set; } = 50;
    public string? ThemisDbUrl { get; set; }
}

/// <summary>
/// Export-Modi
/// </summary>
public enum ExportMode
{
    Json,           // JSON-Dateien
    Csv,            // CSV-Dateien (flach)
    Sql,            // SQL INSERT-Statements
    ThemisDbApi     // Direkt via API
}

/// <summary>
/// Seed-Ergebnis
/// </summary>
public class SeedResult
{
    public bool Success { get; set; }
    public int GeneratedDocuments { get; set; }
    public TestDataStatistics? Statistics { get; set; }
    public string? ExportPath { get; set; }
    public string? ErrorMessage { get; set; }
    public DateTime StartTime { get; set; }
    public DateTime EndTime { get; set; }
    public SeedOptions? Options { get; set; }

    public TimeSpan Duration => EndTime - StartTime;

    public override string ToString()
    {
        return $@"
=== Seed-Ergebnis ===
Status: {(Success ? "✅ Erfolgreich" : "❌ Fehlgeschlagen")}
Dokumente: {GeneratedDocuments}
Dauer: {Duration.TotalSeconds:F2}s
Export: {ExportPath ?? "N/A"}
{(ErrorMessage != null ? $"Fehler: {ErrorMessage}" : "")}
{Statistics?.ToString() ?? ""}
";
    }
}
