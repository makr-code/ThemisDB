/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DeleteInboxItemCommandHandler.cs                   ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;

namespace Themis.DocumentManager.Application.Inbox.Commands.DeleteInboxItem;

public class DeleteInboxItemCommandHandler : IRequestHandler<DeleteInboxItemCommand, Result<bool>>
{
    private static readonly Dictionary<string, object> _items = GetSharedStorage();

    private static Dictionary<string, object> GetSharedStorage()
    {
        var createHandlerType = Type.GetType("Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2.CreateInboxItemV2CommandHandler");
        if (createHandlerType == null) return new();
        
        var field = createHandlerType.GetField("_items", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return field?.GetValue(null) as Dictionary<string, object> ?? new();
    }

    public async Task<Result<bool>> Handle(DeleteInboxItemCommand request, CancellationToken cancellationToken)
    {
        if (!_items.ContainsKey(request.Id))
        {
            return await Task.FromResult(Result<bool>.Fail("Inbox-Element nicht gefunden"));
        }

        _items.Remove(request.Id);
        return await Task.FromResult(Result<bool>.Ok(true));
    }
}
