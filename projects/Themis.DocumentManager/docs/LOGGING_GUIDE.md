# Logging Guide - Serilog in Themis.DocumentManager

**Version:** 1.0  
**Date:** 2025-12-10  
**Library:** Serilog v3.1.1

---

## 🎯 Why Serilog?

**Structured Logging:**
- Rich, structured event data
- Machine-readable JSON logs
- Easy querying and analysis

**Performance:**
- Asynchronous logging
- Minimal overhead
- Batched writes

**Flexibility:**
- Multiple sinks (Console, File, Seq, Elasticsearch, etc.)
- Easy configuration
- Extensible with enrichers

---

## 📦 Installation

### NuGet Packages

```bash
dotnet add package Serilog
dotnet add package Serilog.Sinks.Console
dotnet add package Serilog.Sinks.File
dotnet add package Serilog.Extensions.Logging
dotnet add package Serilog.Enrichers.Environment
```

### Package Reference

```xml
<ItemGroup>
  <PackageReference Include="Serilog" Version="3.1.1" />
  <PackageReference Include="Serilog.Sinks.Console" Version="5.0.1" />
  <PackageReference Include="Serilog.Sinks.File" Version="5.0.0" />
  <PackageReference Include="Serilog.Extensions.Logging" Version="8.0.0" />
  <PackageReference Include="Serilog.Enrichers.Environment" Version="2.3.0" />
</ItemGroup>
```

---

## ⚙️ Configuration

### Basic Setup (App.xaml.cs)

```csharp
using Serilog;
using Serilog.Events;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        // Configure Serilog
        Log.Logger = new LoggerConfiguration()
            .MinimumLevel.Debug()
            .MinimumLevel.Override("Microsoft", LogEventLevel.Information)
            .MinimumLevel.Override("System", LogEventLevel.Warning)
            .Enrich.FromLogContext()
            .Enrich.WithProperty("Application", "ThemisDB.DocumentManager")
            .Enrich.WithMachineName()
            .Enrich.WithEnvironmentName()
            .WriteTo.Console(
                outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] {Message:lj}{NewLine}{Exception}")
            .WriteTo.File(
                path: "logs/themisdb-.log",
                rollingInterval: RollingInterval.Day,
                retainedFileCountLimit: 30,
                outputTemplate: "{Timestamp:yyyy-MM-dd HH:mm:ss.fff zzz} [{Level:u3}] {Message:lj}{NewLine}{Exception}")
            .CreateLogger();

        Log.Information("Application starting up");

        try
        {
            base.OnStartup(e);
            ConfigureServices();
        }
        catch (Exception ex)
        {
            Log.Fatal(ex, "Application start-up failed");
            throw;
        }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        Log.Information("Application shutting down");
        Log.CloseAndFlush();
        base.OnExit(e);
    }

    private void ConfigureServices()
    {
        var services = new ServiceCollection();

        // Add Serilog to DI
        services.AddLogging(builder =>
        {
            builder.ClearProviders();
            builder.AddSerilog(dispose: true);
        });

        // ... other services
    }
}
```

### Advanced Configuration

```csharp
Log.Logger = new LoggerConfiguration()
    // Minimum log level
    .MinimumLevel.Debug()
    
    // Override for specific namespaces
    .MinimumLevel.Override("Microsoft", LogEventLevel.Information)
    .MinimumLevel.Override("Microsoft.EntityFrameworkCore", LogEventLevel.Warning)
    .MinimumLevel.Override("System", LogEventLevel.Warning)
    
    // Enrichers
    .Enrich.FromLogContext()
    .Enrich.WithProperty("Application", "ThemisDB.DocumentManager")
    .Enrich.WithProperty("Version", "1.0.0")
    .Enrich.WithMachineName()
    .Enrich.WithEnvironmentName()
    .Enrich.WithThreadId()
    .Enrich.WithProcessId()
    
    // Console Sink (Development)
    .WriteTo.Console(
        restrictedToMinimumLevel: LogEventLevel.Debug,
        outputTemplate: "[{Timestamp:HH:mm:ss} {Level:u3}] [{SourceContext}] {Message:lj}{NewLine}{Exception}")
    
    // File Sink (Always)
    .WriteTo.File(
        path: "logs/themisdb-.log",
        rollingInterval: RollingInterval.Day,
        retainedFileCountLimit: 30,
        fileSizeLimitBytes: 100_000_000, // 100 MB per file
        rollOnFileSizeLimit: true,
        shared: false,
        outputTemplate: "{Timestamp:yyyy-MM-dd HH:mm:ss.fff zzz} [{Level:u3}] [{SourceContext}] {Message:lj}{NewLine}{Exception}")
    
    // JSON File Sink (for structured log analysis)
    .WriteTo.File(
        path: "logs/themisdb-.json",
        rollingInterval: RollingInterval.Day,
        retainedFileCountLimit: 7,
        formatter: new Serilog.Formatting.Json.JsonFormatter())
    
    // Seq Sink (if available - centralized logging)
    .WriteTo.Seq("http://localhost:5341")
    
    .CreateLogger();
```

---

## 📝 Logging Best Practices

### 1. Log Levels

Use appropriate log levels:

```csharp
// Verbose - Most detailed, for tracing
_logger.LogTrace("Entering method GetDocument with id {DocumentId}", documentId);

// Debug - Internal system events
_logger.LogDebug("Cache miss for document {DocumentId}", documentId);

// Information - General flow of application
_logger.LogInformation("Document {DocumentId} created successfully", documentId);

// Warning - Unexpected but not critical
_logger.LogWarning("Document {DocumentId} not found in cache, fetching from DB", documentId);

// Error - Errors and exceptions
_logger.LogError(ex, "Failed to create document {Title}", title);

// Fatal/Critical - Application crashes
_logger.LogCritical(ex, "Database connection failed, application cannot continue");
```

### 2. Structured Logging

**❌ String Interpolation (BAD):**
```csharp
_logger.LogInformation($"User {userId} created document {documentId}");
// String is already formatted, loses structure
```

**✅ Message Templates (GOOD):**
```csharp
_logger.LogInformation(
    "User {UserId} created document {DocumentId}", 
    userId, 
    documentId);
// Structured data preserved for querying
```

### 3. Include Context

```csharp
public class CreateDocumentCommandHandler
{
    private readonly ILogger<CreateDocumentCommandHandler> _logger;

    public async Task<string> Handle(CreateDocumentCommand request, ...)
    {
        // Include request properties
        _logger.LogInformation(
            "Creating document {Title} by {Author} with size {FileSize} bytes",
            request.Title,
            request.Author,
            request.Content?.Length ?? 0);

        try
        {
            var documentId = await _repository.CreateDocumentAsync(...);
            
            // Include result
            _logger.LogInformation(
                "Document {DocumentId} created successfully in {ElapsedMs}ms",
                documentId,
                stopwatch.ElapsedMilliseconds);
            
            return documentId;
        }
        catch (Exception ex)
        {
            // Include error context
            _logger.LogError(ex,
                "Failed to create document {Title}. Author: {Author}, Size: {FileSize}",
                request.Title,
                request.Author,
                request.Content?.Length ?? 0);
            throw;
        }
    }
}
```

### 4. Using LogContext

**Enrich logs with ambient context:**

```csharp
using Serilog.Context;

public class DocumentService
{
    public async Task ProcessDocumentAsync(string documentId)
    {
        // Add DocumentId to all logs in this scope
        using (LogContext.PushProperty("DocumentId", documentId))
        {
            _logger.LogInformation("Processing document");
            
            await ValidateDocument();  // Logs will include DocumentId
            await SaveDocument();      // Logs will include DocumentId
            
            _logger.LogInformation("Document processed successfully");
        }
    }
}
```

### 5. Performance Considerations

**Use LoggerMessage for high-frequency logs:**

```csharp
public partial class CreateDocumentCommandHandler
{
    private readonly ILogger<CreateDocumentCommandHandler> _logger;

    // Define LoggerMessage delegate (compile-time optimization)
    private static readonly Action<ILogger, string, string, Exception?> _documentCreating =
        LoggerMessage.Define<string, string>(
            LogLevel.Information,
            new EventId(1, "DocumentCreating"),
            "Creating document {Title} by {Author}");

    private static readonly Action<ILogger, string, Exception?> _documentCreated =
        LoggerMessage.Define<string>(
            LogLevel.Information,
            new EventId(2, "DocumentCreated"),
            "Document {DocumentId} created successfully");

    public async Task<string> Handle(CreateDocumentCommand request, ...)
    {
        // Use pre-compiled logger message (faster)
        _documentCreating(_logger, request.Title, request.Author, null);

        var documentId = await _repository.CreateDocumentAsync(...);

        _documentCreated(_logger, documentId, null);

        return documentId;
    }
}
```

---

## 🎨 Log Output Examples

### Console Output

```
[10:35:42 INF] Application starting up
[10:35:43 INF] [CreateDocumentCommandHandler] Creating document Vertrag.pdf by max@example.com with size 52428800 bytes
[10:35:44 INF] [CreateDocumentCommandHandler] Document doc123 created successfully in 1234ms
[10:36:12 WRN] [GetDocumentQueryHandler] Document doc999 not found in cache, fetching from DB
[10:36:15 ERR] [DeleteDocumentCommandHandler] Failed to delete document doc456
System.InvalidOperationException: Document is locked by another user
   at ThemisDB.Repository.DeleteAsync(String id)
   ...
```

### File Output (Structured)

```
2025-12-10 10:35:42.123 +01:00 [INF] Application starting up
2025-12-10 10:35:43.456 +01:00 [INF] [CreateDocumentCommandHandler] Creating document Vertrag.pdf by max@example.com with size 52428800 bytes
2025-12-10 10:35:44.789 +01:00 [INF] [CreateDocumentCommandHandler] Document doc123 created successfully in 1234ms
```

### JSON Output (for Seq/ELK)

```json
{
  "@t": "2025-12-10T09:35:43.4560000Z",
  "@l": "Information",
  "@mt": "Creating document {Title} by {Author} with size {FileSize} bytes",
  "Title": "Vertrag.pdf",
  "Author": "max@example.com",
  "FileSize": 52428800,
  "SourceContext": "CreateDocumentCommandHandler",
  "Application": "ThemisDB.DocumentManager",
  "MachineName": "DEV-MACHINE",
  "EnvironmentName": "Development"
}
```

---

## 🔍 Log Analysis

### Querying Structured Logs

**With Seq:**
```sql
-- Find all errors in last hour
@Level = 'Error' and @Timestamp > Now() - 1h

-- Find slow document creations
ElapsedMs > 5000 and @MessageTemplate like '%created successfully%'

-- Find documents by specific author
Author = 'max@example.com'

-- Group by error type
select Count(*) as ErrorCount, @Exception.Type
from stream
where @Level = 'Error'
group by @Exception.Type
```

**With grep (file logs):**
```bash
# Find all errors
grep "[ERR]" logs/themisdb-*.log

# Find specific document
grep "doc123" logs/themisdb-*.log

# Count errors by type
grep "[ERR]" logs/themisdb-*.log | cut -d' ' -f6- | sort | uniq -c

# Find long-running operations
grep "ElapsedMs" logs/themisdb-*.log | awk '$NF > 5000'
```

---

## 🛠️ Troubleshooting

### Common Issues

**1. Logs not appearing:**
```csharp
// Ensure Serilog is flushed on exit
protected override void OnExit(ExitEventArgs e)
{
    Log.CloseAndFlush();
    base.OnExit(e);
}
```

**2. File permission errors:**
```csharp
// Use shared file access
.WriteTo.File(
    path: "logs/themisdb-.log",
    shared: true,  // Allow multiple processes
    ...
)
```

**3. Too many log files:**
```csharp
.WriteTo.File(
    path: "logs/themisdb-.log",
    rollingInterval: RollingInterval.Day,
    retainedFileCountLimit: 7,  // Keep only 7 days
    ...
)
```

**4. Performance impact:**
```csharp
// Use async sinks for I/O-heavy logging
.WriteTo.Async(a => a.File("logs/themisdb-.log"))
```

---

## 📊 Monitoring

### Key Metrics to Log

**Application Health:**
```csharp
_logger.LogInformation("Application health check passed. Uptime: {Uptime}", uptime);
```

**Performance Metrics:**
```csharp
_logger.LogInformation(
    "Query performance: {QueryType} completed in {ElapsedMs}ms, returned {ResultCount} results",
    queryType,
    stopwatch.ElapsedMilliseconds,
    results.Count);
```

**User Actions:**
```csharp
_logger.LogInformation(
    "User action: {UserId} performed {Action} on {EntityType} {EntityId}",
    userId,
    action,
    entityType,
    entityId);
```

**Resource Usage:**
```csharp
_logger.LogDebug(
    "Cache statistics: Hit rate {HitRate}%, Size {CacheSize} items, Memory {MemoryMB}MB",
    hitRate,
    cacheSize,
    memoryMB);
```

---

## 🔒 Security Considerations

### 1. Sensitive Data

**❌ DON'T log sensitive information:**
```csharp
// BAD - Logs password
_logger.LogInformation("User {Email} logged in with password {Password}", email, password);
```

**✅ DO redact or omit:**
```csharp
// GOOD - No sensitive data
_logger.LogInformation("User {Email} logged in successfully", email);

// GOOD - Hash if needed
var passwordHash = ComputeHash(password);
_logger.LogDebug("Authentication attempt with hash {PasswordHash}", passwordHash);
```

### 2. PII (Personal Identifiable Information)

```csharp
// Use email domain instead of full email
var domain = email.Split('@')[1];
_logger.LogInformation("User from domain {Domain} created document", domain);

// Log user ID instead of name
_logger.LogInformation("User {UserId} accessed document {DocumentId}", userId, documentId);
```

### 3. Log File Protection

```csharp
// Ensure log files have restricted permissions
// On Windows: NTFS permissions
// On Linux: chmod 600 logs/themisdb-*.log
```

---

## 🚀 Advanced Features

### Custom Enrichers

```csharp
public class UserContextEnricher : ILogEventEnricher
{
    private readonly ICurrentUserService _currentUserService;

    public UserContextEnricher(ICurrentUserService currentUserService)
    {
        _currentUserService = currentUserService;
    }

    public void Enrich(LogEvent logEvent, ILogEventPropertyFactory propertyFactory)
    {
        var user = _currentUserService.GetCurrentUser();
        if (user != null)
        {
            logEvent.AddPropertyIfAbsent(
                propertyFactory.CreateProperty("UserId", user.Id));
            logEvent.AddPropertyIfAbsent(
                propertyFactory.CreateProperty("UserRole", user.Role));
        }
    }
}

// Register enricher
.Enrich.With<UserContextEnricher>()
```

### Custom Sinks

```csharp
public class DatabaseSink : ILogEventSink
{
    private readonly ILogRepository _repository;

    public DatabaseSink(ILogRepository repository)
    {
        _repository = repository;
    }

    public void Emit(LogEvent logEvent)
    {
        var logEntry = new LogEntry
        {
            Timestamp = logEvent.Timestamp,
            Level = logEvent.Level.ToString(),
            Message = logEvent.RenderMessage(),
            Exception = logEvent.Exception?.ToString()
        };

        _repository.InsertAsync(logEntry);
    }
}

// Register sink
.WriteTo.Sink(new DatabaseSink(repository))
```

---

## 📚 Resources

**Serilog Documentation:** https://serilog.net/  
**Seq (Log Server):** https://datalust.co/seq  
**Best Practices:** https://github.com/serilog/serilog/wiki/Writing-Log-Events  
**Common Sinks:** https://github.com/serilog/serilog/wiki/Provided-Sinks

---

**Last Updated:** 2025-12-10  
**Next Review:** Sprint 3-4
