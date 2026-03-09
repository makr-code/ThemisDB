/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Result.cs                                          ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Generic;

namespace Themis.DocumentManager.Application.Common;

/// <summary>
/// Ergebnis-Wrapper für Commands und Queries mit Erfolgs-/Fehler-Handling.
/// Verwendet im gesamten Application Layer für konsistentes Error Handling.
/// </summary>
/// <typeparam name="T">Typ des Ergebnisses</typeparam>
public class Result<T>
{
    public bool Success { get; init; }
    public T? Value { get; init; }
    public string? ErrorMessage { get; init; }
    public List<string> Errors { get; init; } = new();

    public static Result<T> Ok(T value) => new() { Success = true, Value = value };
    public static Result<T> Fail(string error) => new() { Success = false, ErrorMessage = error };
    public static Result<T> Fail(List<string> errors) => new() { Success = false, Errors = errors };
}

/// <summary>
/// Result ohne Wert für Commands die nur Erfolg/Fehler zurückgeben.
/// </summary>
public class Result : Result<bool>
{
    public static Result Ok() => new() { Success = true, Value = true };
    public new static Result Fail(string error) => new() { Success = false, ErrorMessage = error };
}
