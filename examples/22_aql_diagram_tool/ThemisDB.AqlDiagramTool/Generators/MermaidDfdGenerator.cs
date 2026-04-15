/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MermaidDfdGenerator.cs                             ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     122                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text;
using ThemisDB.AqlDiagramTool.Models;

namespace ThemisDB.AqlDiagramTool.Generators;

/// <summary>
/// Generates Data Flow Diagrams in Mermaid format
/// </summary>
public class MermaidDfdGenerator
{
    /// <summary>
    /// Generate a DFD from a DataFlowDiagram model
    /// </summary>
    public string Generate(DataFlowDiagram dfd)
    {
        var sb = new StringBuilder();
        
        sb.AppendLine("flowchart TD");
        
        // Generate external entities (square boxes)
        foreach (var entity in dfd.ExternalEntities)
        {
            sb.AppendLine($"    {entity.Id}[\"{entity.Name}\"]");
        }
        
        // Generate processes (rounded boxes)
        foreach (var process in dfd.Processes)
        {
            sb.AppendLine($"    {process.Id}(\"{process.Name}\")");
        }
        
        // Generate data stores (cylinder/database shape)
        foreach (var store in dfd.DataStores)
        {
            sb.AppendLine($"    {store.Id}[(\"{store.Name}\")]");
        }
        
        // Generate data flows (arrows)
        foreach (var flow in dfd.DataFlows)
        {
            var label = string.IsNullOrEmpty(flow.Name) ? "" : $"|{flow.Name}|";
            sb.AppendLine($"    {flow.From} -->{label} {flow.To}");
        }
        
        return sb.ToString();
    }

    /// <summary>
    /// Generate a DFD from AQL queries and collections
    /// </summary>
    public string GenerateFromAql(List<string> collections, List<string> aqlQueries)
    {
        var dfd = new DataFlowDiagram { Name = "AQL Data Flow" };
        
        // Add collections as data stores
        foreach (var collection in collections)
        {
            dfd.AddDataStore(new DataStore
            {
                Id = $"DS_{SanitizeName(collection)}",
                Name = collection
            });
        }
        
        // Add AQL queries as processes
        for (int i = 0; i < aqlQueries.Count; i++)
        {
            var query = aqlQueries[i];
            dfd.AddProcess(new Process
            {
                Id = $"P{i + 1}",
                Name = $"Query {i + 1}",
                Description = query,
                AqlQuery = query
            });
            
            // Try to extract collection references from query
            foreach (var collection in collections)
            {
                if (query.Contains(collection, StringComparison.OrdinalIgnoreCase))
                {
                    dfd.AddDataFlow(new DataFlow
                    {
                        From = $"DS_{SanitizeName(collection)}",
                        To = $"P{i + 1}",
                        Name = "read"
                    });
                }
            }
        }
        
        return Generate(dfd);
    }

    private string SanitizeName(string name)
    {
        return name.Replace(" ", "_").Replace("-", "_");
    }
}
