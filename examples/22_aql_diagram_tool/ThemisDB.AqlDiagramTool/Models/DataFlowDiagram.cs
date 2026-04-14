/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DataFlowDiagram.cs                                 ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:36:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     83                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace ThemisDB.AqlDiagramTool.Models;

/// <summary>
/// Represents a Data Flow Diagram
/// </summary>
public class DataFlowDiagram
{
    public string Name { get; set; } = string.Empty;
    public List<Process> Processes { get; set; } = new();
    public List<DataStore> DataStores { get; set; } = new();
    public List<DataFlow> DataFlows { get; set; } = new();
    public List<ExternalEntity> ExternalEntities { get; set; } = new();

    public void AddProcess(Process process) => Processes.Add(process);
    public void AddDataStore(DataStore dataStore) => DataStores.Add(dataStore);
    public void AddDataFlow(DataFlow dataFlow) => DataFlows.Add(dataFlow);
    public void AddExternalEntity(ExternalEntity entity) => ExternalEntities.Add(entity);
}

/// <summary>
/// Represents a process in DFD (AQL query or operation)
/// </summary>
public class Process
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string? AqlQuery { get; set; }
}

/// <summary>
/// Represents a data store in DFD (Collection)
/// </summary>
public class DataStore
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Represents a data flow between components
/// </summary>
public class DataFlow
{
    public string Name { get; set; } = string.Empty;
    public string From { get; set; } = string.Empty;
    public string To { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
}

/// <summary>
/// Represents an external entity in DFD
/// </summary>
public class ExternalEntity
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
}
