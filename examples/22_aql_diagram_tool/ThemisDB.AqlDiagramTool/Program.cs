/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Program.cs                                         ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     452                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿using ThemisDB.AqlDiagramTool.Generators;
using ThemisDB.AqlDiagramTool.Parsers;
using ThemisDB.AqlDiagramTool.Models;

namespace ThemisDB.AqlDiagramTool;

class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("╔═══════════════════════════════════════════════════════════╗");
        Console.WriteLine("║  ThemisDB AQL Diagram Tool                                ║");
        Console.WriteLine("║  ERD, ER, and DFD Generator for easier AQL queries        ║");
        Console.WriteLine("╚═══════════════════════════════════════════════════════════╝");
        Console.WriteLine();

        if (args.Length == 0)
        {
            ShowHelp();
            return;
        }

        try
        {
            var command = args[0].ToLower();
            switch (command)
            {
                case "erd":
                    GenerateErd(args);
                    break;
                case "dfd":
                    GenerateDfd(args);
                    break;
                case "aql":
                    GenerateAqlQueries(args);
                    break;
                case "example":
                    GenerateExample(args);
                    break;
                case "help":
                case "-h":
                case "--help":
                    ShowHelp();
                    break;
                default:
                    Console.WriteLine($"Unknown command: {command}");
                    Console.WriteLine("Use 'help' to see available commands.");
                    break;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
            Environment.Exit(1);
        }
    }

    static void GenerateErd(string[] args)
    {
        if (args.Length < 2)
        {
            Console.WriteLine("Usage: erd <schema-file.json> [output-file.md]");
            return;
        }

        var schemaFile = args[1];
        var outputFile = args.Length > 2 ? args[2] : "erd_output.md";

        Console.WriteLine($"Parsing schema from: {schemaFile}");
        var parser = new JsonSchemaParser();
        var schema = parser.ParseFile(schemaFile);

        Console.WriteLine($"Generating ERD for schema: {schema.Name}");
        var generator = new MermaidErdGenerator();
        var mermaidCode = generator.Generate(schema);

        var output = $"# Entity-Relationship Diagram: {schema.Name}\n\n";
        if (!string.IsNullOrEmpty(schema.Description))
        {
            output += $"{schema.Description}\n\n";
        }
        output += "```mermaid\n";
        output += mermaidCode;
        output += "```\n";

        File.WriteAllText(outputFile, output);
        Console.WriteLine($"✓ ERD generated successfully: {outputFile}");
    }

    static void GenerateDfd(string[] args)
    {
        if (args.Length < 2)
        {
            Console.WriteLine("Usage: dfd <schema-file.json> [output-file.md]");
            return;
        }

        var schemaFile = args[1];
        var outputFile = args.Length > 2 ? args[2] : "dfd_output.md";

        Console.WriteLine($"Parsing schema from: {schemaFile}");
        var parser = new JsonSchemaParser();
        var schema = parser.ParseFile(schemaFile);

        Console.WriteLine($"Generating DFD for schema: {schema.Name}");
        
        // Create DFD from schema collections
        var collections = schema.Entities.Select(e => e.Name).ToList();
        var sampleQueries = new List<string>
        {
            $"FOR doc IN {collections.FirstOrDefault() ?? "collection"} RETURN doc"
        };

        var generator = new MermaidDfdGenerator();
        var mermaidCode = generator.GenerateFromAql(collections, sampleQueries);

        var output = $"# Data Flow Diagram: {schema.Name}\n\n";
        if (!string.IsNullOrEmpty(schema.Description))
        {
            output += $"{schema.Description}\n\n";
        }
        output += "```mermaid\n";
        output += mermaidCode;
        output += "```\n";

        File.WriteAllText(outputFile, output);
        Console.WriteLine($"✓ DFD generated successfully: {outputFile}");
    }

    static void GenerateAqlQueries(string[] args)
    {
        if (args.Length < 2)
        {
            Console.WriteLine("Usage: aql <schema-file.json> [output-file.md]");
            return;
        }

        var schemaFile = args[1];
        var outputFile = args.Length > 2 ? args[2] : "aql_queries.md";

        Console.WriteLine($"Parsing schema from: {schemaFile}");
        var parser = new JsonSchemaParser();
        var schema = parser.ParseFile(schemaFile);

        Console.WriteLine($"Generating AQL queries for schema: {schema.Name}");
        var helper = new AqlQueryHelper();

        var output = $"# AQL Query Examples: {schema.Name}\n\n";
        if (!string.IsNullOrEmpty(schema.Description))
        {
            output += $"{schema.Description}\n\n";
        }

        foreach (var entity in schema.Entities)
        {
            output += $"## {entity.Name}\n\n";
            if (!string.IsNullOrEmpty(entity.Description))
            {
                output += $"{entity.Description}\n\n";
            }

            // CRUD queries
            output += "### CRUD Operations\n\n";
            var crudQueries = helper.GenerateCrudQueries(entity);
            foreach (var query in crudQueries)
            {
                output += $"```aql\n{query}\n```\n\n";
            }

            // Aggregation queries
            output += "### Aggregations\n\n";
            var aggQueries = helper.GenerateAggregationQueries(entity);
            foreach (var query in aggQueries)
            {
                output += $"```aql\n{query}\n```\n\n";
            }
        }

        // Join queries
        if (schema.Relationships.Any())
        {
            output += "## Join Queries\n\n";
            var joinQueries = helper.GenerateJoinQueries(schema);
            foreach (var query in joinQueries)
            {
                output += $"```aql\n{query}\n```\n\n";
            }
        }

        // Graph traversal queries
        var graphQueries = helper.GenerateGraphTraversalQueries(schema);
        if (graphQueries.Any())
        {
            output += "## Graph Traversal\n\n";
            foreach (var query in graphQueries)
            {
                output += $"```aql\n{query}\n```\n\n";
            }
        }

        File.WriteAllText(outputFile, output);
        Console.WriteLine($"✓ AQL queries generated successfully: {outputFile}");
    }

    static void GenerateExample(string[] args)
    {
        var exampleType = args.Length > 1 ? args[1].ToLower() : "todo";
        
        Console.WriteLine($"Generating example schema: {exampleType}");
        
        DatabaseSchema schema = exampleType switch
        {
            "ecommerce" => CreateEcommerceExample(),
            "blog" => CreateBlogExample(),
            _ => CreateTodoExample()
        };

        var outputFile = $"example_{exampleType}_schema.json";
        var json = System.Text.Json.JsonSerializer.Serialize(new
        {
            name = schema.Name,
            description = schema.Description,
            entities = schema.Entities.Select(e => new
            {
                name = e.Name,
                description = e.Description,
                type = e.Type.ToString().ToLower(),
                attributes = e.Attributes.Select(a => new
                {
                    name = a.Name,
                    type = a.DataType,
                    isKey = a.IsKey,
                    required = a.IsRequired,
                    unique = a.IsUnique,
                    description = a.Description
                })
            }),
            relationships = schema.Relationships.Select(r => new
            {
                name = r.Name,
                from = r.FromEntity.Name,
                to = r.ToEntity.Name,
                type = r.Type.ToString().ToLower().Replace("to", "-to-"),
                description = r.Description,
                foreignKey = r.ForeignKeyAttribute
            })
        }, new System.Text.Json.JsonSerializerOptions { WriteIndented = true });

        File.WriteAllText(outputFile, json);
        Console.WriteLine($"✓ Example schema generated: {outputFile}");
        Console.WriteLine($"\nYou can now generate diagrams with:");
        Console.WriteLine($"  dotnet run erd {outputFile}");
        Console.WriteLine($"  dotnet run dfd {outputFile}");
        Console.WriteLine($"  dotnet run aql {outputFile}");
    }

    static DatabaseSchema CreateTodoExample()
    {
        var schema = new DatabaseSchema("TodoApp", "Simple todo application schema");
        
        var tasks = new Entity("tasks", "Task items");
        var keyAttr = new Models.Attribute
        {
            Name = "_key",
            DataType = "string",
            IsKey = true,
            IsRequired = true,
            IsUnique = true
        };
        tasks.AddAttribute(keyAttr);
        tasks.AddAttribute("title", "string", isRequired: true);
        tasks.AddAttribute("description", "string");
        tasks.AddAttribute("status", "string", isRequired: true);
        tasks.AddAttribute("priority", "string", isRequired: true);
        tasks.AddAttribute("created_at", "datetime", isRequired: true);
        tasks.AddAttribute("due_date", "datetime");
        
        schema.AddEntity(tasks);
        return schema;
    }

    static DatabaseSchema CreateBlogExample()
    {
        var schema = new DatabaseSchema("BlogSystem", "Blog with posts and comments");
        
        var users = new Entity("users", "Blog users");
        users.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        users.AddAttribute("username", "string", isRequired: true);
        users.AddAttribute("email", "string", isRequired: true);
        users.AddAttribute("created_at", "datetime", isRequired: true);
        
        var posts = new Entity("posts", "Blog posts");
        posts.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        posts.AddAttribute("title", "string", isRequired: true);
        posts.AddAttribute("content", "string", isRequired: true);
        posts.AddAttribute("author_id", "string", isRequired: true);
        posts.AddAttribute("created_at", "datetime", isRequired: true);
        
        var comments = new Entity("comments", "Post comments");
        comments.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        comments.AddAttribute("post_id", "string", isRequired: true);
        comments.AddAttribute("author_id", "string", isRequired: true);
        comments.AddAttribute("content", "string", isRequired: true);
        comments.AddAttribute("created_at", "datetime", isRequired: true);
        
        schema.AddEntity(users);
        schema.AddEntity(posts);
        schema.AddEntity(comments);
        
        schema.AddRelationship(new Relationship("writes", users, posts, RelationshipType.OneToMany)
        {
            ForeignKeyAttribute = "author_id"
        });
        schema.AddRelationship(new Relationship("comments_on", comments, posts, RelationshipType.ManyToOne)
        {
            ForeignKeyAttribute = "post_id"
        });
        schema.AddRelationship(new Relationship("authored_by", comments, users, RelationshipType.ManyToOne)
        {
            ForeignKeyAttribute = "author_id"
        });
        
        return schema;
    }

    static DatabaseSchema CreateEcommerceExample()
    {
        var schema = new DatabaseSchema("EcommerceStore", "E-commerce product catalog");
        
        var products = new Entity("products", "Product catalog");
        products.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        products.AddAttribute("name", "string", isRequired: true);
        products.AddAttribute("description", "string");
        products.AddAttribute("price", "decimal", isRequired: true);
        products.AddAttribute("category_id", "string", isRequired: true);
        products.AddAttribute("stock", "int", isRequired: true);
        
        var categories = new Entity("categories", "Product categories");
        categories.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        categories.AddAttribute("name", "string", isRequired: true);
        categories.AddAttribute("parent_id", "string");
        
        var orders = new Entity("orders", "Customer orders");
        orders.AddAttribute(new Models.Attribute 
        { 
            Name = "_key", 
            DataType = "string", 
            IsKey = true, 
            IsRequired = true, 
            IsUnique = true 
        });
        orders.AddAttribute("customer_id", "string", isRequired: true);
        orders.AddAttribute("total", "decimal", isRequired: true);
        orders.AddAttribute("status", "string", isRequired: true);
        orders.AddAttribute("created_at", "datetime", isRequired: true);
        
        schema.AddEntity(products);
        schema.AddEntity(categories);
        schema.AddEntity(orders);
        
        schema.AddRelationship(new Relationship("belongs_to", products, categories, RelationshipType.ManyToOne)
        {
            ForeignKeyAttribute = "category_id"
        });
        
        return schema;
    }

    static void ShowHelp()
    {
        Console.WriteLine("Usage: ThemisDB.AqlDiagramTool <command> [options]");
        Console.WriteLine();
        Console.WriteLine("Commands:");
        Console.WriteLine("  erd <schema-file.json> [output.md]    Generate Entity-Relationship Diagram");
        Console.WriteLine("  dfd <schema-file.json> [output.md]    Generate Data Flow Diagram");
        Console.WriteLine("  aql <schema-file.json> [output.md]    Generate AQL query examples");
        Console.WriteLine("  example [todo|blog|ecommerce]         Generate example schema");
        Console.WriteLine("  help                                   Show this help message");
        Console.WriteLine();
        Console.WriteLine("Examples:");
        Console.WriteLine("  # Generate example schema");
        Console.WriteLine("  dotnet run example todo");
        Console.WriteLine();
        Console.WriteLine("  # Generate ERD from schema");
        Console.WriteLine("  dotnet run erd example_todo_schema.json todo_erd.md");
        Console.WriteLine();
        Console.WriteLine("  # Generate AQL queries");
        Console.WriteLine("  dotnet run aql example_todo_schema.json todo_queries.md");
        Console.WriteLine();
        Console.WriteLine("  # Generate DFD");
        Console.WriteLine("  dotnet run dfd example_blog_schema.json blog_dfd.md");
        Console.WriteLine();
    }
}
