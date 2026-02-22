/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TestDataGeneratorCli.cs                            ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     253                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// CLI entrypoint is unused in WPF app; suppress compiler warning.
#pragma warning disable CS8892

using System;
using System.Threading.Tasks;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.TestData;

namespace Themis.DocumentManager.TestData;

/// <summary>
/// Kommandozeilen-Tool zum Generieren von ThemisDB Testdaten
/// </summary>
class TestDataGeneratorCli
{
    static async Task Main(string[] args)
    {
        Console.WriteLine("╔════════════════════════════════════════════════════╗");
        Console.WriteLine("║   ThemisDB Testdaten-Generator                     ║");
        Console.WriteLine("║   Generiert repräsentative Verwaltungsdaten        ║");
        Console.WriteLine("╚════════════════════════════════════════════════════╝");
        Console.WriteLine();

        if (args.Length > 0 && args[0] == "quick")
        {
            // Schnell-Modus
            int count = args.Length > 1 && int.TryParse(args[1], out var c) ? c : 100;
            await ThemisDbSeeder.QuickSeedAsync(count);
            return;
        }

        // Interaktiver Modus
        var options = await GetOptionsFromUserAsync();
        var seeder = new ThemisDbSeeder();
        var result = await seeder.SeedDatabaseAsync(options);

        Console.WriteLine();
        Console.WriteLine(result.ToString());

        Console.WriteLine();
        Console.WriteLine("Drücken Sie eine Taste zum Beenden...");
        Console.ReadKey();
    }

    static async Task<SeedOptions> GetOptionsFromUserAsync()
    {
        var options = new SeedOptions();

        // Anzahl Dokumente
        Console.Write("Anzahl Dokumente [100]: ");
        var countInput = Console.ReadLine();
        if (int.TryParse(countInput, out var count) && count > 0)
            options.DocumentCount = count;

        // Export-Modus
        Console.WriteLine();
        Console.WriteLine("Export-Modi:");
        Console.WriteLine("  1 - JSON (empfohlen)");
        Console.WriteLine("  2 - CSV (für Excel)");
        Console.WriteLine("  3 - SQL INSERT-Statements");
        Console.WriteLine("  4 - Direkt zu ThemisDB via API");
        Console.Write("Auswahl [1]: ");
        var modeInput = Console.ReadLine();
        options.ExportMode = modeInput switch
        {
            "2" => ExportMode.Csv,
            "3" => ExportMode.Sql,
            "4" => ExportMode.ThemisDbApi,
            _ => ExportMode.Json
        };

        // Dateiname
        Console.Write($"Dateiname [{options.OutputFileName}]: ");
        var fileInput = Console.ReadLine();
        if (!string.IsNullOrEmpty(fileInput))
        {
            options.OutputFileName = fileInput;
        }
        else
        {
            // Auto-Dateiname mit Timestamp
            var ext = options.ExportMode switch
            {
                ExportMode.Json => "json",
                ExportMode.Csv => "csv",
                ExportMode.Sql => "sql",
                _ => "json"
            };
            options.OutputFileName = $"themis_testdata_{DateTime.Now:yyyyMMdd_HHmmss}.{ext}";
        }

        // Spezialisierte Daten
        Console.Write("Spezialisierte Datensätze inkludieren? [J/n]: ");
        var specialInput = Console.ReadLine()?.ToLower();
        options.IncludeSpecializedData = specialInput != "n";

        if (options.IncludeSpecializedData)
        {
            Console.Write("Anzahl spezialisierter Datensätze [50]: ");
            var specialCount = Console.ReadLine();
            if (int.TryParse(specialCount, out var sc) && sc > 0)
                options.SpecializedCount = sc;
        }

        // API-URL wenn ThemisDbApi-Modus
        if (options.ExportMode == ExportMode.ThemisDbApi)
        {
            Console.Write("ThemisDB URL [http://localhost:8765]: ");
            var urlInput = Console.ReadLine();
            options.ThemisDbUrl = string.IsNullOrEmpty(urlInput) ? "http://localhost:8765" : urlInput;
        }

        Console.WriteLine();
        return await Task.FromResult(options);
    }

    /// <summary>
    /// Zeigt Beispiel-Verwendung an
    /// </summary>
    static void ShowExamples()
    {
        Console.WriteLine("Beispiele:");
        Console.WriteLine();
        Console.WriteLine("  # Schnell-Modus: 100 Datensätze");
        Console.WriteLine("  TestDataGenerator.exe quick");
        Console.WriteLine();
        Console.WriteLine("  # Schnell-Modus: 500 Datensätze");
        Console.WriteLine("  TestDataGenerator.exe quick 500");
        Console.WriteLine();
        Console.WriteLine("  # Interaktiver Modus");
        Console.WriteLine("  TestDataGenerator.exe");
        Console.WriteLine();
    }
}

/// <summary>
/// Beispiel-Verwendung in Code
/// </summary>
public static class TestDataGeneratorExamples
{
    /// <summary>
    /// Beispiel 1: Einfache Generierung
    /// </summary>
    public static async Task Example1_SimpleGeneration()
    {
        var generator = new ThemisTestDataGenerator();
        
        // Ein Dokument
        var doc = generator.GenerateMetadata();
        Console.WriteLine($"Generiert: {doc.DocumentId} mit {doc.BoundFields.Count} Feldern");

        // Batch von 100
        var batch = generator.GenerateBatch(100);
        var stats = generator.GetStatistics(batch);
        Console.WriteLine(stats.ToString());
    }

    /// <summary>
    /// Beispiel 2: Spezialisierte Daten
    /// </summary>
    public static void Example2_SpecializedData()
    {
        var generator = new ThemisTestDataGenerator();

        var baugenehmigung = generator.GenerateSpecializedMetadata("Baugenehmigung");
        var gewerbe = generator.GenerateSpecializedMetadata("Gewerbeanmeldung");
        var umwelt = generator.GenerateSpecializedMetadata("Umweltgenehmigung");

        Console.WriteLine($"Baugenehmigung: {baugenehmigung.BoundFields.First(f => f.FieldName == "Betreff").CurrentValue}");
    }

    /// <summary>
    /// Beispiel 3: Export in verschiedene Formate
    /// </summary>
    public static async Task Example3_ExportFormats()
    {
        // JSON Export
        var seeder = new ThemisDbSeeder();
        await seeder.SeedDatabaseAsync(new SeedOptions
        {
            DocumentCount = 50,
            ExportMode = ExportMode.Json,
            OutputFileName = "testdata.json"
        });

        // CSV Export für Excel
        await seeder.SeedDatabaseAsync(new SeedOptions
        {
            DocumentCount = 50,
            ExportMode = ExportMode.Csv,
            OutputFileName = "testdata.csv"
        });

        // SQL Export für Datenbank-Import
        await seeder.SeedDatabaseAsync(new SeedOptions
        {
            DocumentCount = 50,
            ExportMode = ExportMode.Sql,
            OutputFileName = "testdata.sql"
        });
    }

    /// <summary>
    /// Beispiel 4: Direkt zu ThemisDB
    /// </summary>
    public static async Task Example4_DirectToApi()
    {
        var seeder = new ThemisDbSeeder();
        await seeder.SeedDatabaseAsync(new SeedOptions
        {
            DocumentCount = 1000,
            ExportMode = ExportMode.ThemisDbApi,
            ThemisDbUrl = "http://localhost:8765",
            IncludeSpecializedData = true,
            SpecializedCount = 200
        });
    }

    /// <summary>
    /// Beispiel 5: Quick-Seed für schnelle Tests
    /// </summary>
    public static async Task Example5_QuickSeed()
    {
        // Schnellste Methode: 100 Dokumente als JSON
        await ThemisDbSeeder.QuickSeedAsync(100);
    }
}
