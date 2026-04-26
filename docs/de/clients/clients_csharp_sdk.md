# ThemisDB C# Client SDK

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Clients  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Installation](#installation)
- [Basic Usage](#basic-usage)
- [Authentication](#authentication)
- [CRUD Operations](#crud-operations)
- [Query Execution](#query-execution)
- [Performance Optimization](#performance-optimization)
- [Async/Await](#asyncawait)
- [Beispiel-Anwendungen](#beispiel-anwendungen)

---

## Installation

### NuGet Package

```powershell
# Package Manager Console
Install-Package ThemisDB.Client

# .NET CLI
dotnet add package ThemisDB.Client
```

### PackageReference

```xml
<PackageReference Include="ThemisDB.Client" Version="1.0.0" />
```

---

## Basic Usage

### Connection

```csharp
using ThemisDB.Client;

// Simple connection
var client = new ThemisClient("http://localhost:8765");

// With options
var options = new ThemisClientOptions
{
    ConnectionTimeout = TimeSpan.FromSeconds(10),
    RequestTimeout = TimeSpan.FromSeconds(30),
    MaxRetries = 3,
    RetryDelay = TimeSpan.FromMilliseconds(500)
};

var client = new ThemisClient("http://localhost:8765", options);
```

### Dependency Injection (ASP.NET Core)

```csharp
// Startup.cs or Program.cs
public void ConfigureServices(IServiceCollection services)
{
    services.AddThemisClient(options =>
    {
        options.ServerUrl = "http://localhost:8765";
        options.Username = "admin";
        options.Password = Configuration["ThemisDB:Password"];
        options.ConnectionTimeout = TimeSpan.FromSeconds(10);
    });
}

// Usage in controller
public class UsersController : ControllerBase
{
    private readonly IThemisClient _client;
    
    public UsersController(IThemisClient client)
    {
        _client = client;
    }
    
    [HttpGet]
    public async Task<IActionResult> GetUsers()
    {
        var result = await _client.QueryAsync<User>(
            "FOR doc IN users RETURN doc"
        );
        
        return Ok(result.Entities);
    }
}
```

---

## Authentication

### Basic Authentication

```csharp
using ThemisDB.Client;

var options = new ThemisClientOptions
{
    Username = "admin",
    Password = "secret"
};

var client = new ThemisClient("http://localhost:8765", options);

// All requests now use Basic Auth
var result = await client.QueryAsync<User>("FOR doc IN users RETURN doc");
```

### Token-based Authentication

```csharp
using ThemisDB.Client;

var client = new ThemisClient("http://localhost:8765");

// 1. Login to get token
var loginResult = await client.LoginAsync("admin", "secret");
string token = loginResult.Token;
string refreshToken = loginResult.RefreshToken;

// 2. Create authenticated client
var authenticatedClient = new ThemisClient("http://localhost:8765", 
    new ThemisClientOptions
    {
        AuthToken = token
    });

// 3. Refresh token when expired
try
{
    var result = await authenticatedClient.QueryAsync<User>(
        "FOR doc IN users RETURN doc"
    );
}
catch (UnauthorizedException)
{
    // Token expired, refresh it
    var newToken = await authenticatedClient.RefreshTokenAsync(refreshToken);
    authenticatedClient.UpdateAuthToken(newToken);
    
    // Retry request
    var result = await authenticatedClient.QueryAsync<User>(
        "FOR doc IN users RETURN doc"
    );
}
```

### SSL/TLS Configuration

```csharp
using ThemisDB.Client;

var options = new ThemisClientOptions
{
    UseSsl = true,
    SslVerify = true,
    SslCertificatePath = @"C:\certs\client-cert.pfx",
    SslCertificatePassword = "cert-password"
};

var client = new ThemisClient("https://localhost:8765", options);
```

### Azure AD Authentication

```csharp
using ThemisDB.Client;
using Microsoft.Identity.Client;

// Get Azure AD token
var app = ConfidentialClientApplicationBuilder.Create(clientId)
    .WithClientSecret(clientSecret)
    .WithAuthority(authority)
    .Build();

var result = await app.AcquireTokenForClient(scopes).ExecuteAsync();
string azureToken = result.AccessToken;

// Use with ThemisDB
var options = new ThemisClientOptions
{
    AuthToken = azureToken,
    AuthType = AuthenticationType.Bearer
};

var client = new ThemisClient("https://themis.company.com", options);
```

---

## CRUD Operations

### Create (Insert)

```csharp
using ThemisDB.Client;
using System.Text.Json.Serialization;

public class User
{
    [JsonPropertyName("_key")]
    public string Key { get; set; }
    
    [JsonPropertyName("name")]
    public string Name { get; set; }
    
    [JsonPropertyName("age")]
    public int Age { get; set; }
    
    [JsonPropertyName("email")]
    public string Email { get; set; }
}

var client = new ThemisClient("http://localhost:8765");

// Single document
var user = new User
{
    Key = "user-123",
    Name = "Alice",
    Age = 30,
    Email = "alice@example.com"
};

await client.InsertAsync("users", user);

// Batch insert
var users = new List<User>
{
    new User { Key = "user-124", Name = "Bob", Age = 25 },
    new User { Key = "user-125", Name = "Charlie", Age = 35 }
};

var batchResult = await client.BatchInsertAsync("users", users);
Console.WriteLine($"Inserted {batchResult.SuccessCount} documents");
```

### Read (Get)

```csharp
// Get single document
var user = await client.GetAsync<User>("users", "user-123");
if (user != null)
{
    Console.WriteLine($"Name: {user.Name}");
}

// Batch get
var keys = new[] { "user-123", "user-124", "user-125" };
var batchResult = await client.BatchGetAsync<User>("users", keys);

foreach (var doc in batchResult.Found)
{
    Console.WriteLine($"Found: {doc.Key}");
}

foreach (var key in batchResult.Missing)
{
    Console.WriteLine($"Missing: {key}");
}
```

### Update

```csharp
// Partial update
var updates = new Dictionary<string, object>
{
    ["age"] = 31,
    ["last_login"] = DateTime.UtcNow
};

await client.UpdateAsync("users", "user-123", updates);

// Full replace
var updatedUser = new User
{
    Key = "user-123",
    Name = "Alice Smith",
    Age = 31,
    Email = "alice.smith@example.com"
};

await client.ReplaceAsync("users", "user-123", updatedUser);
```

### Delete

```csharp
// Delete single document
await client.RemoveAsync("users", "user-123");

// Batch delete
var keys = new[] { "user-124", "user-125" };
var batchResult = await client.BatchRemoveAsync("users", keys);

Console.WriteLine($"Deleted {batchResult.DeletedCount} documents");
```

---

## Query Execution

### Basic Query

```csharp
var result = await client.QueryAsync<User>(@"
    FOR doc IN users
      FILTER doc.age > 25
      SORT doc.name ASC
      LIMIT 10
      RETURN doc
");

foreach (var user in result.Entities)
{
    Console.WriteLine($"Name: {user.Name}, Age: {user.Age}");
}
```

### Parameterized Query

```csharp
var bindVars = new Dictionary<string, object>
{
    ["min_age"] = 25,
    ["limit"] = 10
};

var result = await client.QueryAsync<User>(@"
    FOR doc IN users
      FILTER doc.age > @min_age
      LIMIT @limit
      RETURN doc
", bindVars);

Console.WriteLine($"Found {result.Entities.Count} users");
```

### LINQ Integration

```csharp
using ThemisDB.Client.Linq;

// LINQ to AQL translation
var query = client.Collection<User>("users")
    .Where(u => u.Age > 25)
    .OrderBy(u => u.Name)
    .Take(10);

var users = await query.ToListAsync();

// Complex query
var query = from u in client.Collection<User>("users")
            where u.Age > 25 && u.Email.Contains("@example.com")
            orderby u.Name
            select new { u.Name, u.Email };

var results = await query.ToListAsync();
```

### Cursor-based Pagination

```csharp
var options = new QueryOptions
{
    BatchSize = 100,
    UseCursor = true
};

var cursor = await client.QueryCursorAsync<User>(@"
    FOR doc IN large_collection
      RETURN doc
", options);

while (await cursor.HasMoreAsync())
{
    var batch = await cursor.NextAsync();
    
    foreach (var doc in batch)
    {
        // Process document
        ProcessDocument(doc);
    }
}
```

---

## Performance Optimization

### Connection Pooling

```csharp
// Use HttpClientFactory (recommended for ASP.NET Core)
public void ConfigureServices(IServiceCollection services)
{
    services.AddHttpClient<IThemisClient, ThemisClient>(client =>
    {
        client.BaseAddress = new Uri("http://localhost:8765");
        client.Timeout = TimeSpan.FromSeconds(30);
    })
    .SetHandlerLifetime(TimeSpan.FromMinutes(5))
    .ConfigurePrimaryHttpMessageHandler(() => new SocketsHttpHandler
    {
        PooledConnectionLifetime = TimeSpan.FromMinutes(2),
        MaxConnectionsPerServer = 50
    });
}
```

### Batch Operations

```csharp
// ❌ Bad: Many individual requests
foreach (var key in keys)
{
    var user = await client.GetAsync<User>("users", key);
    // Process...
}

// ✅ Good: Single batch request
var batchResult = await client.BatchGetAsync<User>("users", keys);
foreach (var user in batchResult.Found)
{
    // Process...
}
```

### Parallel Queries

```csharp
var tasks = new List<Task<QueryResult<User>>>();

foreach (var shard in shards)
{
    tasks.Add(client.QueryAsync<User>(
        $"FOR doc IN users_{shard} RETURN doc"
    ));
}

var results = await Task.WhenAll(tasks);

var allUsers = results.SelectMany(r => r.Entities).ToList();
```

### Caching

```csharp
using Microsoft.Extensions.Caching.Memory;

public class CachedThemisService
{
    private readonly IThemisClient _client;
    private readonly IMemoryCache _cache;
    
    public CachedThemisService(IThemisClient client, IMemoryCache cache)
    {
        _client = client;
        _cache = cache;
    }
    
    public async Task<User> GetUserAsync(string key)
    {
        var cacheKey = $"user:{key}";
        
        if (_cache.TryGetValue<User>(cacheKey, out var cachedUser))
        {
            return cachedUser;
        }
        
        var user = await _client.GetAsync<User>("users", key);
        
        _cache.Set(cacheKey, user, TimeSpan.FromMinutes(5));
        
        return user;
    }
}
```

---

## Async/Await

### Best Practices

```csharp
// ✅ Good: Async all the way
public async Task<List<User>> GetActiveUsersAsync()
{
    var result = await client.QueryAsync<User>(@"
        FOR doc IN users
          FILTER doc.active == true
          RETURN doc
    ");
    
    return result.Entities;
}

// ❌ Bad: Blocking async code
public List<User> GetActiveUsers()
{
    var result = client.QueryAsync<User>(@"
        FOR doc IN users
          FILTER doc.active == true
          RETURN doc
    ").Result;  // Deadlock risk!
    
    return result.Entities;
}
```

### Cancellation

```csharp
public async Task<List<User>> SearchUsersAsync(
    string query, 
    CancellationToken cancellationToken = default)
{
    var result = await client.QueryAsync<User>(
        "FOR doc IN users FILTER doc.name LIKE @query RETURN doc",
        new Dictionary<string, object> { ["query"] = $"%{query}%" },
        cancellationToken
    );
    
    return result.Entities;
}

// Usage
var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5));
try
{
    var users = await SearchUsersAsync("Alice", cts.Token);
}
catch (OperationCanceledException)
{
    Console.WriteLine("Search timed out");
}
```

### Error Handling

```csharp
try
{
    var result = await client.QueryAsync<User>(
        "FOR doc IN users RETURN doc"
    );
}
catch (ThemisConnectionException ex)
{
    // Connection failed
    logger.LogError(ex, "Failed to connect to ThemisDB");
    // Retry logic...
}
catch (ThemisQueryException ex)
{
    // Query error
    logger.LogError(ex, "Query failed: {ErrorCode}", ex.ErrorCode);
}
catch (ThemisUnauthorizedException ex)
{
    // Authentication failed
    logger.LogError(ex, "Unauthorized");
    // Re-authenticate...
}
catch (ThemisException ex)
{
    // Generic ThemisDB error
    logger.LogError(ex, "ThemisDB error");
}
```

---

## Beispiel-Anwendungen

### ASP.NET Core Web API

```csharp
// Program.cs
var builder = WebApplication.CreateBuilder(args);

builder.Services.AddThemisClient(options =>
{
    options.ServerUrl = builder.Configuration["ThemisDB:ServerUrl"];
    options.Username = builder.Configuration["ThemisDB:Username"];
    options.Password = builder.Configuration["ThemisDB:Password"];
});

builder.Services.AddMemoryCache();
builder.Services.AddScoped<IUserService, UserService>();

var app = builder.Build();

app.MapControllers();
app.Run();

// Controllers/UsersController.cs
[ApiController]
[Route("api/[controller]")]
public class UsersController : ControllerBase
{
    private readonly IUserService _userService;
    
    public UsersController(IUserService userService)
    {
        _userService = userService;
    }
    
    [HttpGet]
    public async Task<ActionResult<List<User>>> GetUsers(
        [FromQuery] int? minAge = null)
    {
        var users = await _userService.GetUsersAsync(minAge);
        return Ok(users);
    }
    
    [HttpGet("{id}")]
    public async Task<ActionResult<User>> GetUser(string id)
    {
        var user = await _userService.GetUserAsync(id);
        
        if (user == null)
            return NotFound();
        
        return Ok(user);
    }
    
    [HttpPost]
    public async Task<ActionResult<User>> CreateUser([FromBody] User user)
    {
        await _userService.CreateUserAsync(user);
        return CreatedAtAction(nameof(GetUser), new { id = user.Key }, user);
    }
    
    [HttpPut("{id}")]
    public async Task<IActionResult> UpdateUser(
        string id, 
        [FromBody] User user)
    {
        await _userService.UpdateUserAsync(id, user);
        return NoContent();
    }
    
    [HttpDelete("{id}")]
    public async Task<IActionResult> DeleteUser(string id)
    {
        await _userService.DeleteUserAsync(id);
        return NoContent();
    }
}

// Services/UserService.cs
public interface IUserService
{
    Task<List<User>> GetUsersAsync(int? minAge = null);
    Task<User> GetUserAsync(string id);
    Task CreateUserAsync(User user);
    Task UpdateUserAsync(string id, User user);
    Task DeleteUserAsync(string id);
}

public class UserService : IUserService
{
    private readonly IThemisClient _client;
    private readonly IMemoryCache _cache;
    private readonly ILogger<UserService> _logger;
    
    public UserService(
        IThemisClient client, 
        IMemoryCache cache,
        ILogger<UserService> logger)
    {
        _client = client;
        _cache = cache;
        _logger = logger;
    }
    
    public async Task<List<User>> GetUsersAsync(int? minAge = null)
    {
        var query = "FOR doc IN users";
        var bindVars = new Dictionary<string, object>();
        
        if (minAge.HasValue)
        {
            query += " FILTER doc.age >= @min_age";
            bindVars["min_age"] = minAge.Value;
        }
        
        query += " RETURN doc";
        
        var result = await _client.QueryAsync<User>(query, bindVars);
        return result.Entities;
    }
    
    public async Task<User> GetUserAsync(string id)
    {
        var cacheKey = $"user:{id}";
        
        if (_cache.TryGetValue<User>(cacheKey, out var cachedUser))
        {
            return cachedUser;
        }
        
        var user = await _client.GetAsync<User>("users", id);
        
        if (user != null)
        {
            _cache.Set(cacheKey, user, TimeSpan.FromMinutes(5));
        }
        
        return user;
    }
    
    public async Task CreateUserAsync(User user)
    {
        await _client.InsertAsync("users", user);
        _logger.LogInformation("Created user {UserId}", user.Key);
    }
    
    public async Task UpdateUserAsync(string id, User user)
    {
        await _client.ReplaceAsync("users", id, user);
        
        // Invalidate cache
        var cacheKey = $"user:{id}";
        _cache.Remove(cacheKey);
        
        _logger.LogInformation("Updated user {UserId}", id);
    }
    
    public async Task DeleteUserAsync(string id)
    {
        await _client.RemoveAsync("users", id);
        
        // Invalidate cache
        var cacheKey = $"user:{id}";
        _cache.Remove(cacheKey);
        
        _logger.LogInformation("Deleted user {UserId}", id);
    }
}
```

### Console Application

```csharp
using ThemisDB.Client;

class Program
{
    static async Task Main(string[] args)
    {
        var client = new ThemisClient("http://localhost:8765");
        
        // Import data
        Console.WriteLine("Importing users...");
        await ImportUsersAsync(client);
        
        // Query data
        Console.WriteLine("\nQuerying users...");
        await QueryUsersAsync(client);
        
        // Update data
        Console.WriteLine("\nUpdating users...");
        await UpdateUsersAsync(client);
        
        Console.WriteLine("\nDone!");
    }
    
    static async Task ImportUsersAsync(IThemisClient client)
    {
        var users = Enumerable.Range(1, 100)
            .Select(i => new User
            {
                Key = $"user-{i}",
                Name = $"User {i}",
                Age = 20 + (i % 50),
                Email = $"user{i}@example.com"
            })
            .ToList();
        
        await client.BatchInsertAsync("users", users);
        Console.WriteLine($"Imported {users.Count} users");
    }
    
    static async Task QueryUsersAsync(IThemisClient client)
    {
        var result = await client.QueryAsync<User>(@"
            FOR doc IN users
              FILTER doc.age > 30
              SORT doc.name ASC
              LIMIT 10
              RETURN doc
        ");
        
        Console.WriteLine($"Found {result.Entities.Count} users:");
        foreach (var user in result.Entities)
        {
            Console.WriteLine($"  - {user.Name} ({user.Age})");
        }
    }
    
    static async Task UpdateUsersAsync(IThemisClient client)
    {
        var result = await client.QueryAsync<User>(@"
            FOR doc IN users
              UPDATE doc WITH {last_updated: DATE_NOW()} IN users
              RETURN NEW
        ");
        
        Console.WriteLine($"Updated {result.Entities.Count} users");
    }
}
```

---

## Siehe auch

- [C++ Client SDK](clients_cpp_sdk.md)
- [REST API Documentation](clients_rest_api.md)
- [Python SDK](clients_python_sdk.md)
- [Performance Best Practices](../performance/PERFORMANCE_CLIENT_OPTIMIZATION.md)
