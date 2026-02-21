/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DialogService.cs                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     23                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Threading.Tasks;
using System.Windows;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für Dialog-Management
/// </summary>
public interface IDialogService
{
    Task<bool> ShowProcessLinkingDialogAsync(string entityId, string entityType);
}

public class DialogService : IDialogService
{
    public async Task<bool> ShowProcessLinkingDialogAsync(string entityId, string entityType)
    {
        // Placeholder: würde in echtem Setup einen Dialog öffnen
        // Hier nur für Demo
        return await Task.FromResult(true);
    }
}
