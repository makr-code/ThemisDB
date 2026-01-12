# Themis.AqlQueryBuilder - Visual AQL Query Builder

## Overview

**Themis.AqlQueryBuilder** is a desktop application that provides a visual, form-based interface for constructing AQL (Advanced Query Language) queries. It eliminates the need to manually write AQL syntax while providing real-time query preview and execution capabilities against a running ThemisDB instance.

## Use Cases

- **Query Development:** Build complex queries without memorizing AQL syntax
- **Schema Exploration:** Browse collections and field types interactively
- **Query Testing:** Execute queries and view results immediately
- **Learning AQL:** Understand AQL syntax through visual examples
- **Multi-Table Queries:** Construct implicit JOINs across collections
- **Prototyping:** Quickly test query logic before embedding in applications

## Requirements

- **.NET 8.0 SDK** or later
- **Windows, Linux, or macOS**
- **Running ThemisDB server** (default: http://localhost:8080)
- **Visual Studio 2022** or **VS Code with C# Dev Kit** (for development)

## Installation

### From Source

```bash
cd tools/Themis.AqlQueryBuilder

# Restore dependencies
dotnet restore

# Build application
dotnet build --configuration Release

# Run application
dotnet run
```

### Published Binary

```bash
# Publish self-contained executable
dotnet publish -c Release -r win-x64 --self-contained

# Run published app
./bin/Release/net8.0/win-x64/publish/Themis.AqlQueryBuilder.exe
```

## Features

### Visual Query Construction

- **FOR Clauses:** Iterate over collections (supports multiple FOR for implicit JOINs)
- **LET Clauses:** Define variables and expressions
- **FILTER Clauses:** Add conditions with visual operator selection
- **SORT Clauses:** Order results by field (ASC/DESC)
- **LIMIT Clause:** Pagination with offset and count
- **RETURN Clause:** Project specific fields or return whole documents

### Schema Explorer

- Browse all collections in the database
- View field names and types for each collection
- Type indicators (string, number, boolean, object, array)
- Expandable nested object structures

### Real-Time Query Preview

- See generated AQL as you build the query
- Syntax-highlighted query display
- Update preview on demand

### Query Execution

- Execute queries against connected server
- View results in formatted JSON
- Display execution time and result count
- Handle errors with detailed messages

### Connection Methods

Supports multiple connection types:

1. **HTTP/HTTPS REST API** (Default - Implemented)
2. **TCP Socket Connection** (Planned)
3. **UDP Connection** (Planned)
4. **Direct C# API** (Planned)
5. **Direct C++ API via P/Invoke** (Planned)

## Basic Usage

### 1. Connect to Server

1. Launch the application
2. Enter server URL in the toolbar (default: `http://localhost:8080`)
3. Click "Connect" to establish connection
4. Schema Explorer will populate with available collections

### 2. Build a Query

**Example: Find adult users sorted by name**

1. **Add FOR Clause:**
   - Variable: `u`
   - Collection: `users`

2. **Add FILTER Clause:**
   - Left: `u.age`
   - Operator: `>`
   - Right: `18`

3. **Add SORT Clause:**
   - Field: `u.name`
   - Direction: `ASC`

4. **Add LIMIT:**
   - Offset: `0`
   - Count: `10`

5. **Configure RETURN:**
   - Return type: `Whole document`

6. **Update Preview:**
   - Click "🔄 Update Query Preview"

**Generated AQL:**
```aql
FOR u IN users
  FILTER u.age > 18
  SORT u.name ASC
  LIMIT 10
  RETURN u
```

### 3. Execute Query

1. Click "Execute" button
2. View results in the results panel
3. Check execution time and count

## Example Queries

### Simple Query

**Find recent orders:**

```aql
FOR o IN orders
  FILTER o.timestamp > "2026-01-01"
  SORT o.timestamp DESC
  LIMIT 20
  RETURN o
```

**Visual Steps:**
1. FOR: `o` IN `orders`
2. FILTER: `o.timestamp` > `"2026-01-01"`
3. SORT: `o.timestamp` DESC
4. LIMIT: 20
5. RETURN: `o`

### Multi-Collection Query (Implicit JOIN)

**Find orders with user information:**

```aql
FOR u IN users
FOR o IN orders
  FILTER o.user_id == u.id
  FILTER o.status == "completed"
  RETURN {
    user_name: u.name,
    order_id: o.id,
    amount: o.amount
  }
```

**Visual Steps:**
1. FOR: `u` IN `users`
2. FOR: `o` IN `orders`
3. FILTER: `o.user_id` == `u.id`
4. FILTER: `o.status` == `"completed"`
5. RETURN: Custom object with selected fields

### Aggregation with LET

**Calculate user statistics:**

```aql
FOR u IN users
  LET order_count = LENGTH(
    FOR o IN orders
      FILTER o.user_id == u.id
      RETURN o
  )
  FILTER order_count > 5
  RETURN {
    user: u.name,
    orders: order_count
  }
```

## Configuration

### Server Connection

Edit `appsettings.json`:

```json
{
  "ThemisServer": {
    "BaseUrl": "http://localhost:8080",
    "ApiKey": "",
    "Timeout": 30
  },
  "QueryBuilder": {
    "MaxResults": 1000,
    "EnableAutoComplete": true,
    "SaveQueryHistory": true
  }
}
```

### Connection Profiles

Create multiple connection profiles:

```json
{
  "ConnectionProfiles": {
    "Local": {
      "BaseUrl": "http://localhost:8080"
    },
    "Development": {
      "BaseUrl": "https://dev.example.com:8080",
      "ApiKey": "dev-api-key"
    },
    "Production": {
      "BaseUrl": "https://prod.example.com:8080",
      "ApiKey": "prod-api-key"
    }
  }
}
```

## Advanced Usage

### Custom Return Objects

Build custom return structures:

1. Set Return Type to "Custom object"
2. Enter return expression:
```
{
  user_id: u.id,
  full_name: CONCAT(u.first_name, " ", u.last_name),
  age_group: u.age < 30 ? "young" : "mature",
  order_count: LENGTH(u.orders)
}
```

### Complex Filters

Chain multiple conditions:

```aql
FOR u IN users
  FILTER u.age >= 18
  FILTER u.age <= 65
  FILTER u.country IN ["US", "CA", "UK"]
  FILTER u.active == true
  RETURN u
```

### Nested Queries

Use LET for subqueries:

```aql
FOR u IN users
  LET recent_orders = (
    FOR o IN orders
      FILTER o.user_id == u.id
      FILTER o.timestamp > DATE_NOW() - 86400
      RETURN o
  )
  FILTER LENGTH(recent_orders) > 0
  RETURN {
    user: u.name,
    orders: recent_orders
  }
```

## Architecture

### MVVM Pattern

The application follows the Model-View-ViewModel pattern:

**Models** (`Models/`):
- `AqlQueryModel` - Complete query representation
- `ForClause`, `FilterClause`, `SortClause` - Individual clause models
- Data structures for AQL components

**ViewModels** (`ViewModels/`):
- `MainViewModel` - Main application logic
- `ObservableCollection<T>` for dynamic UI updates
- `RelayCommand` for user interactions
- `INotifyPropertyChanged` for data binding

**Views** (`Views/`):
- `MainWindow.xaml` - Main application window
- XAML data binding to ViewModel properties
- WPF controls (DataGrid, ComboBox, TextBox)

### Key Dependencies

```xml
<PackageReference Include="CommunityToolkit.Mvvm" Version="8.2.2" />
<PackageReference Include="Themis.AdminTools.Shared" Version="1.0.0" />
<PackageReference Include="System.Text.Json" Version="8.0.0" />
```

### OOP Principles

- **Separation of Concerns:** Logic separated from UI
- **Single Responsibility:** Each class has one purpose
- **Encapsulation:** Private fields with public properties
- **Composition:** Query composed of clause objects
- **SOLID Principles:** Maintained throughout codebase

## API Requirements

ThemisDB server must provide:

### POST /api/query/aql

Execute AQL query:

**Request:**
```json
{
  "query": "FOR u IN users FILTER u.age > 18 RETURN u",
  "explain": false
}
```

**Response:**
```json
{
  "results": [
    {"id": 1, "name": "Alice", "age": 25},
    {"id": 2, "name": "Bob", "age": 30}
  ],
  "count": 2,
  "executionTime": "15ms"
}
```

### GET /api/schema/collections

Get collection list:

**Response:**
```json
{
  "collections": [
    {
      "name": "users",
      "fields": [
        {"name": "id", "type": "number"},
        {"name": "name", "type": "string"},
        {"name": "age", "type": "number"}
      ]
    }
  ]
}
```

## Troubleshooting

### Cannot Connect to Server

**Symptoms:** Connection error when clicking Connect

**Solutions:**
- Verify ThemisDB server is running: `curl http://localhost:8080/api/health`
- Check firewall settings
- Verify server URL is correct
- Review server logs for errors

### Schema Explorer Empty

**Symptoms:** No collections shown after connecting

**Solutions:**
- Verify collections exist in database
- Check API permissions
- Review `/api/schema/collections` endpoint response
- Ensure API key (if required) is configured

### Query Execution Fails

**Symptoms:** Error when executing query

**Solutions:**
- Check generated AQL syntax in preview
- Verify collection names are correct
- Ensure field names exist in schema
- Review error message for details

### Application Crashes on Startup

**Symptoms:** Application won't start

**Solutions:**
```bash
# Check .NET runtime
dotnet --version

# Rebuild application
dotnet clean
dotnet build

# Check for config errors
cat appsettings.json | jq .
```

## Future Enhancements

- [ ] Save/load queries to/from files
- [ ] Query history with search
- [ ] Syntax highlighting in query preview
- [ ] Auto-completion for fields and collections
- [ ] COLLECT clause builder for aggregations
- [ ] Graph traversal query builder
- [ ] Query performance analysis and optimization hints
- [ ] Export results to CSV/JSON/Excel
- [ ] Dark theme support
- [ ] Plugin system for custom query builders

## See Also

- [AQL Reference](../../aql/AQL_REFERENCE.md) - Complete AQL syntax guide
- [Themis.AdminTools.Shared](admin-tools-shared.md) - Shared library documentation
- [Admin Tools User Guide](../../admin_tools_user_guide.md)
- [ThemisDB API Documentation](../../api/README.md)

## License

Part of ThemisDB, licensed under the project's main license.
