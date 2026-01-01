using System.CommandLine;
using System.Text.Json;
using Microsoft.Extensions.Logging;
using Themis.IngestionTool.Models;
using Themis.IngestionTool.Services;
using YamlDotNet.Serialization;

namespace Themis.IngestionTool;

class Program
{
    static async Task<int> Main(string[] args)
    {
        var rootCommand = new RootCommand("ThemisDB Ingestion Tool - Rekursive Verzeichnisdurchsuchung mit Hash-basierter Duplikaterkennung");

        var sourceOption = new Option<string?>(
            aliases: new[] { "--source", "-s" },
            description: "Quellverzeichnis zum Scannen");

        var outputOption = new Option<string>(
            aliases: new[] { "--output", "-o" },
            getDefaultValue: () => "ingestion_output.json",
            description: "Ausgabedatei für Ingestion-Ergebnisse");

        var configOption = new Option<string?>(
            aliases: new[] { "--config", "-c" },
            description: "Konfigurationsdatei (YAML oder JSON)");

        var dbOption = new Option<string>(
            aliases: new[] { "--db", "-d" },
            getDefaultValue: () => "ingestion_tracker.db",
            description: "SQLite-Datenbank für Tracking");

        var includeExtOption = new Option<string[]?>(
            aliases: new[] { "--include-ext" },
            description: "Nur diese Dateierweiterungen einbeziehen (z.B. .json .yaml)");

        var excludeExtOption = new Option<string[]?>(
            aliases: new[] { "--exclude-ext" },
            description: "Diese Dateierweiterungen ausschließen");

        var maxSizeOption = new Option<double>(
            aliases: new[] { "--max-size" },
            getDefaultValue: () => 100.0,
            description: "Maximale Dateigröße in MB");

        var noVectorOption = new Option<bool>(
            aliases: new[] { "--no-vector" },
            description: "Vektor-Metadaten deaktivieren");

        var noGraphOption = new Option<bool>(
            aliases: new[] { "--no-graph" },
            description: "Graph-Metadaten deaktivieren");

        var noRelationalOption = new Option<bool>(
            aliases: new[] { "--no-relational" },
            description: "Relationale Metadaten deaktivieren");

        var verboseOption = new Option<bool>(
            aliases: new[] { "--verbose", "-v" },
            description: "Ausführliches Logging aktivieren");

        rootCommand.AddOption(sourceOption);
        rootCommand.AddOption(outputOption);
        rootCommand.AddOption(configOption);
        rootCommand.AddOption(dbOption);
        rootCommand.AddOption(includeExtOption);
        rootCommand.AddOption(excludeExtOption);
        rootCommand.AddOption(maxSizeOption);
        rootCommand.AddOption(noVectorOption);
        rootCommand.AddOption(noGraphOption);
        rootCommand.AddOption(noRelationalOption);
        rootCommand.AddOption(verboseOption);

        rootCommand.SetHandler(async (context) =>
        {
            var source = context.ParseResult.GetValueForOption(sourceOption);
            var output = context.ParseResult.GetValueForOption(outputOption) ?? "ingestion_output.json";
            var configFile = context.ParseResult.GetValueForOption(configOption);
            var db = context.ParseResult.GetValueForOption(dbOption) ?? "ingestion_tracker.db";
            var includeExt = context.ParseResult.GetValueForOption(includeExtOption);
            var excludeExt = context.ParseResult.GetValueForOption(excludeExtOption);
            var maxSize = context.ParseResult.GetValueForOption(maxSizeOption);
            var noVector = context.ParseResult.GetValueForOption(noVectorOption);
            var noGraph = context.ParseResult.GetValueForOption(noGraphOption);
            var noRelational = context.ParseResult.GetValueForOption(noRelationalOption);
            var verbose = context.ParseResult.GetValueForOption(verboseOption);

            await RunIngestionAsync(source, output, configFile, db, includeExt, excludeExt,
                maxSize, noVector, noGraph, noRelational, verbose);
        });

        return await rootCommand.InvokeAsync(args);
    }

    static async Task RunIngestionAsync(
        string? source,
        string output,
        string? configFile,
        string db,
        string[]? includeExt,
        string[]? excludeExt,
        double maxSize,
        bool noVector,
        bool noGraph,
        bool noRelational,
        bool verbose)
    {
        // Logger konfigurieren
        using var loggerFactory = LoggerFactory.Create(builder =>
        {
            builder
                .AddConsole()
                .SetMinimumLevel(verbose ? LogLevel.Debug : LogLevel.Information);
        });

        var logger = loggerFactory.CreateLogger<Program>();

        try
        {
            // Konfiguration laden
            IngestionConfig config;
            if (!string.IsNullOrEmpty(configFile))
            {
                logger.LogInformation("Loading configuration from {ConfigFile}", configFile);
                config = LoadConfigFile(configFile);
            }
            else
            {
                if (string.IsNullOrEmpty(source))
                {
                    logger.LogError("Either --source or --config must be specified");
                    return;
                }

                config = new IngestionConfig
                {
                    SourceDir = source,
                    OutputFile = output,
                    DbPath = db,
                    IncludeExtensions = includeExt?.ToList() ?? new List<string>(),
                    ExcludeExtensions = excludeExt?.ToList() ?? new List<string>
                    {
                        ".exe", ".dll", ".so", ".dylib", ".bin", ".pdb", ".obj", ".o"
                    },
                    MaxFileSizeMb = maxSize,
                    GenerateVectorMetadata = !noVector,
                    GenerateGraphMetadata = !noGraph,
                    GenerateRelationalMetadata = !noRelational
                };
            }

            // Quellverzeichnis validieren
            if (!Directory.Exists(config.SourceDir))
            {
                logger.LogError("Source directory does not exist: {SourceDir}", config.SourceDir);
                return;
            }

            // Services erstellen
            var trackerLogger = loggerFactory.CreateLogger<IngestionTracker>();
            var processorLogger = loggerFactory.CreateLogger<FileProcessor>();
            var engineLogger = loggerFactory.CreateLogger<IngestionEngine>();

            using var tracker = new IngestionTracker(config.DbPath, trackerLogger);
            var processor = new FileProcessor(config, processorLogger);
            var engine = new IngestionEngine(config, tracker, processor, engineLogger);

            // Ingestion ausführen mit Fortschrittsanzeige
            var progress = new Progress<(int current, int total, int skipped)>(report =>
            {
                if (report.total > 0 && report.current % 10 == 0)
                {
                    Console.WriteLine($"Progress: {report.current}/{report.total} files " +
                                    $"(skipped: {report.skipped})");
                }
            });

            var result = await engine.IngestAsync(progress);

            logger.LogInformation("Ingestion completed successfully!");
            Console.WriteLine($"\n=== Ingestion Summary ===");
            Console.WriteLine($"Files scanned:   {result.Statistics.TotalFilesScanned}");
            Console.WriteLine($"Files processed: {result.Statistics.FilesProcessed}");
            Console.WriteLine($"Files skipped:   {result.Statistics.FilesSkipped}");
            Console.WriteLine($"Files failed:    {result.Statistics.FilesFailed}");
            Console.WriteLine($"Total size:      {result.Statistics.TotalSizeBytes / 1024.0 / 1024.0:F2} MB");
            Console.WriteLine($"Elapsed time:    {result.Statistics.ElapsedSeconds:F2} seconds");
            Console.WriteLine($"Output file:     {config.OutputFile}");
        }
        catch (Exception ex)
        {
            logger.LogError(ex, "Ingestion failed");
        }
    }

    static IngestionConfig LoadConfigFile(string configPath)
    {
        var extension = Path.GetExtension(configPath).ToLowerInvariant();
        var text = File.ReadAllText(configPath);

        if (extension == ".json")
        {
            return JsonSerializer.Deserialize<IngestionConfig>(text,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
                ?? throw new InvalidOperationException("Failed to deserialize config");
        }
        else if (extension is ".yaml" or ".yml")
        {
            var deserializer = new DeserializerBuilder().Build();
            var yamlObject = deserializer.Deserialize<Dictionary<string, object>>(text);
            var json = JsonSerializer.Serialize(yamlObject);
            return JsonSerializer.Deserialize<IngestionConfig>(json,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
                ?? throw new InvalidOperationException("Failed to deserialize config");
        }
        else
        {
            throw new NotSupportedException($"Unsupported config file format: {extension}");
        }
    }
}
