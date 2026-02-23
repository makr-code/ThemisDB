/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            replication_topology_api_handler.cpp               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-23 03:58:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     659                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • da1a879d5  2026-02-22  feat(replication): add topology visualizer web UI (Issue ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ReplicationTopologyApiHandler
 *
 * REST API + embedded web UI for visualizing the live WAL-replication topology.
 *
 * Endpoints
 *   GET /api/v1/replication/topology   – per-replica snapshot (JSON)
 *   GET /api/v1/replication/health     – aggregated health summary (JSON)
 *   GET /ui/replication/topology       – interactive SVG topology viewer (HTML)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server/replication_topology_api_handler.h"

#include <sstream>

namespace themis {
namespace server {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ReplicationTopologyApiHandler::ReplicationTopologyApiHandler(
    std::shared_ptr<sharding::ReplicationCoordinator> coordinator,
    std::shared_ptr<sharding::WALManager>             wal_manager,
    std::string                                       primary_id,
    std::shared_ptr<AuthMiddleware>                   auth)
    : coordinator_(std::move(coordinator))
    , wal_manager_(std::move(wal_manager))
    , primary_id_(std::move(primary_id))
    , auth_(std::move(auth))
{
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/replication/topology
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ReplicationTopologyApiHandler::handleTopologyGet(
    const http::request<http::string_body>& req)
{
    if (!coordinator_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Replication not configured", req);
    }

    try {
        const auto replicas = coordinator_->getReplicaInfo();
        const uint64_t primary_lsn = wal_manager_
            ? wal_manager_->getCurrentLSN().segment : 0;

        // Build primary (this) node entry
        json primary_node = {
            {"node_id",             primary_id_.empty() ? "primary" : primary_id_},
            {"role",                "PRIMARY"},
            {"is_primary",          true},
            {"health_status",       "HEALTHY"},
            {"replication_lag_ms",  0},
            {"replication_lag_bytes", 0},
            {"endpoint",            ""},
            {"last_confirmed_lsn",  primary_lsn}
        };

        json nodes = json::array();
        nodes.push_back(primary_node);

        // Build replica entries
        for (const auto& r : replicas) {
            json node = {
                {"node_id",              r.replica_id},
                {"role",                 "REPLICA"},
                {"is_primary",           false},
                {"health_status",        r.is_healthy ? "HEALTHY" : "FAILED"},
                {"replication_lag_ms",   r.lag_ms},
                {"replication_lag_bytes", r.lag_bytes},
                {"endpoint",             r.endpoint},
                {"consecutive_failures", r.consecutive_failures},
                {"last_confirmed_lsn",   r.last_confirmed_lsn.segment}
            };
            nodes.push_back(std::move(node));
        }

        // Build directed edges: primary → each replica
        json edges = json::array();
        const std::string local_id = primary_id_.empty() ? "primary" : primary_id_;
        for (const auto& r : replicas) {
            edges.push_back({
                {"from", local_id},
                {"to",   r.replica_id},
                {"type", "WAL_STREAM"}
            });
        }

        json response_body = {
            {"primary_node_id", local_id},
            {"primary_lsn",     primary_lsn},
            {"nodes",           nodes},
            {"edges",           edges},
            {"total_nodes",     nodes.size()},
            {"replica_count",   replicas.size()}
        };

        return makeResponse(http::status::ok, response_body.dump(),
                            "application/json", req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/replication/health
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ReplicationTopologyApiHandler::handleHealthGet(
    const http::request<http::string_body>& req)
{
    if (!coordinator_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Replication not configured", req);
    }

    try {
        const auto replicas = coordinator_->getReplicaInfo();
        const auto stats    = coordinator_->getShipperStats();

        uint32_t healthy_count  = 0;
        uint32_t failed_count   = 0;
        uint64_t max_lag_ms     = 0;
        uint64_t max_lag_bytes  = 0;

        for (const auto& r : replicas) {
            if (r.is_healthy) {
                ++healthy_count;
            } else {
                ++failed_count;
            }
            if (r.lag_ms    > max_lag_ms)    max_lag_ms    = r.lag_ms;
            if (r.lag_bytes > max_lag_bytes) max_lag_bytes = r.lag_bytes;
        }

        const uint32_t total = static_cast<uint32_t>(replicas.size()) + 1; // +1 for primary
        const bool has_quorum =
            (replicas.empty()) ||                          // standalone is fine
            (healthy_count >= (replicas.size() / 2) + 1); // majority quorum
        const std::string overall =
            (failed_count > 0 && !has_quorum) ? "CRITICAL" :
            (failed_count > 0)                ? "DEGRADED"  : "HEALTHY";

        json response_body = {
            {"primary_node_id",      primary_id_.empty() ? "primary" : primary_id_},
            {"has_quorum",           has_quorum},
            {"total_nodes",          total},
            {"healthy_replicas",     healthy_count},
            {"failed_replicas",      failed_count},
            {"max_replication_lag_ms",    max_lag_ms},
            {"max_replication_lag_bytes", max_lag_bytes},
            {"total_entries_shipped",     stats.total_entries_shipped},
            {"total_bytes_shipped",       stats.total_bytes_shipped},
            {"failed_ships",              stats.failed_ships},
            {"overall_status",       overall}
        };

        return makeResponse(http::status::ok, response_body.dump(),
                            "application/json", req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /ui/replication/topology — serve the interactive HTML visualizer
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ReplicationTopologyApiHandler::handleUiGet(
    const http::request<http::string_body>& req)
{
    // Derive API base from the Host header so the page works regardless of
    // bind address or port.
    std::string host;
    auto host_it = req.find(http::field::host);
    if (host_it != req.end()) {
        host = std::string(host_it->value());
    }
    const std::string api_base = host.empty() ? "" : ("http://" + host);
    return makeResponse(http::status::ok, buildUiHtml(api_base),
                        "text/html; charset=utf-8", req);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ReplicationTopologyApiHandler::makeErrorResponse(
    http::status          status,
    const std::string&    message,
    const http::request<http::string_body>& req) const
{
    json error_body = {
        {"error",       true},
        {"message",     message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), "application/json", req);
}

http::response<http::string_body> ReplicationTopologyApiHandler::makeResponse(
    http::status          status,
    const std::string&    body,
    const std::string&    content_type,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, content_type);
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// Embedded HTML for the topology visualizer
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::string ReplicationTopologyApiHandler::buildUiHtml(const std::string& api_base) {
    // Self-contained single-page application that auto-refreshes every 5 s.
    // Fetches /api/v1/replication/topology and /api/v1/replication/health then
    // renders an SVG radial graph (primary in the centre, replicas around it).
    std::ostringstream html;
    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>ThemisDB – Replication Topology</title>
<style>
  :root {
    --bg:       #0f1117;
    --surface:  #1a1d27;
    --border:   #2e3148;
    --text:     #e2e8f0;
    --muted:    #64748b;
    --primary:  #3b82f6;
    --replica:  #10b981;
    --healthy:  #10b981;
    --degraded: #f59e0b;
    --failed:   #ef4444;
    --critical: #dc2626;
    --edge:     #4a5568;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Inter', system-ui, sans-serif;
    min-height: 100vh;
    padding: 1.5rem;
  }
  header {
    display: flex;
    align-items: center;
    gap: 1rem;
    margin-bottom: 1.5rem;
    padding-bottom: 1rem;
    border-bottom: 1px solid var(--border);
  }
  header h1 { font-size: 1.25rem; font-weight: 600; }
  #status-badge {
    margin-left: auto;
    padding: 0.25rem 0.75rem;
    border-radius: 9999px;
    font-size: 0.75rem;
    font-weight: 600;
    border: 1px solid var(--border);
    background: var(--surface);
  }
  #status-badge.healthy  { color: var(--healthy);  border-color: var(--healthy); }
  #status-badge.degraded { color: var(--degraded); border-color: var(--degraded); }
  #status-badge.critical { color: var(--critical); border-color: var(--critical); }
  #last-updated { font-size: 0.7rem; color: var(--muted); }
  .layout {
    display: grid;
    grid-template-columns: 1fr 320px;
    gap: 1.5rem;
    align-items: start;
  }
  @media (max-width: 768px) { .layout { grid-template-columns: 1fr; } }
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 0.75rem;
    padding: 1rem;
  }
  .card h2 {
    font-size: 0.8rem;
    font-weight: 600;
    color: var(--muted);
    margin-bottom: 1rem;
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  #topology-svg { width: 100%; display: block; overflow: visible; }
  .node-label { font-size: 11px; fill: var(--text); font-family: inherit; }
  .node-sub   { font-size:  9px; fill: var(--muted); font-family: inherit; }
  .edge-line  { stroke: var(--edge); stroke-width: 1.5; fill: none; }
  .edge-arrow { fill: var(--edge); }
  table { width: 100%; border-collapse: collapse; font-size: 0.8rem; }
  th { text-align: left; color: var(--muted); font-weight: 500; padding: 0.4rem 0.5rem; border-bottom: 1px solid var(--border); }
  td { padding: 0.5rem; border-bottom: 1px solid var(--border); vertical-align: middle; }
  tr:last-child td { border-bottom: none; }
  .dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; margin-right: 4px; vertical-align: middle; }
  .dot.healthy  { background: var(--healthy); }
  .dot.degraded { background: var(--degraded); }
  .dot.failed   { background: var(--failed); }
  .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 0.75rem; margin-bottom: 1rem; }
  .stat-box { background: var(--bg); border: 1px solid var(--border); border-radius: 0.5rem; padding: 0.6rem 0.75rem; }
  .stat-box .val { font-size: 1.4rem; font-weight: 700; }
  .stat-box .lbl { font-size: 0.7rem; color: var(--muted); }
  #error-banner {
    display: none;
    background: rgba(239,68,68,0.1);
    border: 1px solid var(--failed);
    color: var(--failed);
    border-radius: 0.5rem;
    padding: 0.75rem 1rem;
    margin-bottom: 1rem;
    font-size: 0.85rem;
  }
</style>
</head>
<body>
<header>
  <svg width="26" height="26" viewBox="0 0 26 26" fill="none">
    <circle cx="6"  cy="13" r="5" fill="#3b82f6"/>
    <circle cx="20" cy="6"  r="4" fill="#10b981"/>
    <circle cx="20" cy="20" r="4" fill="#10b981"/>
    <line x1="11" y1="13" x2="16" y2="8"  stroke="#4a5568" stroke-width="1.5"/>
    <line x1="11" y1="13" x2="16" y2="18" stroke="#4a5568" stroke-width="1.5"/>
  </svg>
  <h1>Replication Topology</h1>
  <span id="last-updated"></span>
  <span id="status-badge">–</span>
</header>

<div id="error-banner"></div>

<div class="layout">
  <!-- SVG graph card -->
  <div class="card">
    <h2>Topology Graph</h2>
    <svg id="topology-svg" xmlns="http://www.w3.org/2000/svg">
      <defs>
        <marker id="arrow" markerWidth="8" markerHeight="8" refX="6" refY="3" orient="auto">
          <path d="M0,0 L0,6 L8,3 z" class="edge-arrow"/>
        </marker>
      </defs>
      <g id="edges-layer"></g>
      <g id="nodes-layer"></g>
    </svg>
  </div>

  <!-- Sidebar -->
  <div style="display:flex;flex-direction:column;gap:1.5rem;">
    <div class="card">
      <h2>Cluster Overview</h2>
      <div class="stat-grid">
        <div class="stat-box"><div class="val" id="s-total">–</div><div class="lbl">Total Nodes</div></div>
        <div class="stat-box"><div class="val" id="s-healthy" style="color:var(--healthy)">–</div><div class="lbl">Healthy Replicas</div></div>
        <div class="stat-box"><div class="val" id="s-failed"  style="color:var(--failed)">–</div><div class="lbl">Failed Replicas</div></div>
        <div class="stat-box"><div class="val" id="s-lag" style="color:var(--text)">–</div><div class="lbl">Max Lag (ms)</div></div>
      </div>
    </div>

    <div class="card">
      <h2>Node List</h2>
      <table>
        <thead>
          <tr>
            <th>Node</th><th>Role</th><th>Status</th><th>Lag&nbsp;ms</th>
          </tr>
        </thead>
        <tbody id="node-table-body"></tbody>
      </table>
    </div>

    <div class="card">
      <h2>Ship Statistics</h2>
      <table>
        <tbody id="stats-table-body"></tbody>
      </table>
    </div>
  </div>
</div>

<script>
)";
    html << "const API_BASE = " << (api_base.empty() ? "''" : ("'" + api_base + "'")) << ";\n";
    html << R"(
const REFRESH_MS = 5000;

function svgNS(tag) {
  return document.createElementNS('http://www.w3.org/2000/svg', tag);
}

function nodeColor(node) {
  if (node.is_primary) return 'var(--primary)';
  return node.health_status === 'HEALTHY' ? 'var(--replica)' :
         node.health_status === 'FAILED'  ? 'var(--failed)'  : 'var(--degraded)';
}

function healthDotClass(status) {
  if (!status) return 'dot';
  return 'dot ' + status.toLowerCase();
}

function layoutNodes(nodes) {
  const svgEl = document.getElementById('topology-svg');
  const W = Math.max(svgEl.clientWidth || 0, 480);
  const count = nodes.length;
  const H = Math.max(300, count * 56);
  svgEl.setAttribute('viewBox', `0 0 ${W} ${H}`);
  svgEl.setAttribute('height', H);

  const cx = W / 2, cy = H / 2;
  const R  = Math.min(cx, cy) * 0.60;

  const primary = nodes.find(n => n.is_primary) || nodes[0];
  const others  = nodes.filter(n => n !== primary);

  const pos = new Map();
  if (primary) pos.set(primary.node_id, { x: cx, y: cy });

  others.forEach((n, i) => {
    const angle = (2 * Math.PI * i) / Math.max(others.length, 1) - Math.PI / 2;
    pos.set(n.node_id, {
      x: cx + R * Math.cos(angle),
      y: cy + R * Math.sin(angle)
    });
  });
  return pos;
}

function renderGraph(topo) {
  const eLayer = document.getElementById('edges-layer');
  const nLayer = document.getElementById('nodes-layer');
  eLayer.innerHTML = '';
  nLayer.innerHTML = '';

  if (!topo.nodes || topo.nodes.length === 0) return;

  const pos = layoutNodes(topo.nodes);

  // Edges
  (topo.edges || []).forEach(edge => {
    const from = pos.get(edge.from);
    const to   = pos.get(edge.to);
    if (!from || !to) return;

    const r = 24;
    const dx = to.x - from.x, dy = to.y - from.y;
    const dist = Math.sqrt(dx * dx + dy * dy) || 1;
    const ex = to.x - (dx / dist) * r;
    const ey = to.y - (dy / dist) * r;

    const line = svgNS('line');
    line.setAttribute('x1', from.x); line.setAttribute('y1', from.y);
    line.setAttribute('x2', ex);     line.setAttribute('y2', ey);
    line.setAttribute('class', 'edge-line');
    line.setAttribute('marker-end', 'url(#arrow)');
    eLayer.appendChild(line);
  });

  // Nodes
  topo.nodes.forEach(node => {
    const p = pos.get(node.node_id);
    if (!p) return;

    const g = svgNS('g');
    g.setAttribute('transform', `translate(${p.x},${p.y})`);

    // Glow halo for primary
    if (node.is_primary) {
      const glow = svgNS('circle');
      glow.setAttribute('r', 30);
      glow.setAttribute('fill',   'rgba(59,130,246,0.10)');
      glow.setAttribute('stroke', 'rgba(59,130,246,0.25)');
      glow.setAttribute('stroke-width', '1');
      g.appendChild(glow);
    }

    // Health ring
    const ring = svgNS('circle');
    ring.setAttribute('r', 24);
    ring.setAttribute('fill',   'var(--surface)');
    ring.setAttribute('stroke', nodeColor(node));
    ring.setAttribute('stroke-width', '2.5');
    g.appendChild(ring);

    // Role fill
    const inner = svgNS('circle');
    inner.setAttribute('r', 16);
    inner.setAttribute('fill', nodeColor(node));
    inner.setAttribute('fill-opacity', '0.18');
    g.appendChild(inner);

    // Role initial
    const letter = svgNS('text');
    letter.setAttribute('text-anchor', 'middle');
    letter.setAttribute('dominant-baseline', 'central');
    letter.setAttribute('fill', nodeColor(node));
    letter.setAttribute('font-size', '13');
    letter.setAttribute('font-weight', '700');
    letter.setAttribute('font-family', 'inherit');
    letter.textContent = node.is_primary ? 'P' : 'R';
    g.appendChild(letter);

    // Node ID label
    const label = svgNS('text');
    label.setAttribute('text-anchor', 'middle');
    label.setAttribute('y', 36);
    label.setAttribute('class', 'node-label');
    label.textContent = node.node_id.length > 16
      ? node.node_id.substring(0, 14) + '…'
      : node.node_id;
    g.appendChild(label);

    // Role sublabel
    const role = svgNS('text');
    role.setAttribute('text-anchor', 'middle');
    role.setAttribute('y', 48);
    role.setAttribute('class', 'node-sub');
    role.setAttribute('fill', nodeColor(node));
    role.textContent = node.role;
    g.appendChild(role);

    // Lag sublabel (replicas only)
    if (!node.is_primary && node.replication_lag_ms > 0) {
      const lag = svgNS('text');
      lag.setAttribute('text-anchor', 'middle');
      lag.setAttribute('y', 60);
      lag.setAttribute('class', 'node-sub');
      lag.textContent = node.replication_lag_ms + ' ms lag';
      g.appendChild(lag);
    }

    nLayer.appendChild(g);
  });
}

function renderTable(nodes) {
  const tbody = document.getElementById('node-table-body');
  tbody.innerHTML = '';
  (nodes || []).forEach(node => {
    const dotClass = node.health_status
      ? node.health_status.toLowerCase() : '';
    const tr = document.createElement('tr');
    tr.innerHTML =
      `<td>${node.node_id}${node.is_primary ? ' <em style="color:var(--muted);font-size:0.7rem">(primary)</em>' : ''}</td>` +
      `<td>${node.role || '–'}</td>` +
      `<td><span class="dot ${dotClass}"></span>${node.health_status || 'UNKNOWN'}</td>` +
      `<td>${node.is_primary ? '–' : (node.replication_lag_ms != null ? node.replication_lag_ms : '–')}</td>`;
    tbody.appendChild(tr);
  });
}

function renderStats(health) {
  document.getElementById('s-total').textContent   = health.total_nodes   ?? '–';
  document.getElementById('s-healthy').textContent  = health.healthy_replicas != null ? health.healthy_replicas : '–';
  document.getElementById('s-failed').textContent   = health.failed_replicas  != null ? health.failed_replicas  : '–';
  document.getElementById('s-lag').textContent      = health.max_replication_lag_ms != null ? health.max_replication_lag_ms : '–';

  const badge = document.getElementById('status-badge');
  const s = (health.overall_status || 'UNKNOWN').toLowerCase();
  badge.textContent = health.overall_status || 'UNKNOWN';
  badge.className   = s;

  const stbody = document.getElementById('stats-table-body');
  stbody.innerHTML = '';
  const rows = [
    ['Entries shipped',    health.total_entries_shipped ?? '–'],
    ['Bytes shipped',      health.total_bytes_shipped   != null
      ? (health.total_bytes_shipped / 1024).toFixed(1) + ' KB' : '–'],
    ['Failed ships',       health.failed_ships          ?? '–'],
    ['Max lag bytes',      health.max_replication_lag_bytes ?? '–'],
    ['Quorum',             health.has_quorum ? '✓ Yes' : '✗ No']
  ];
  rows.forEach(([k, v]) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td style="color:var(--muted)">${k}</td><td>${v}</td>`;
    stbody.appendChild(tr);
  });
}

async function refresh() {
  const errBanner = document.getElementById('error-banner');
  try {
    const [topoRes, healthRes] = await Promise.all([
      fetch(API_BASE + '/api/v1/replication/topology'),
      fetch(API_BASE + '/api/v1/replication/health')
    ]);

    if (!topoRes.ok)   throw new Error(`Topology API: HTTP ${topoRes.status}`);
    if (!healthRes.ok) throw new Error(`Health API: HTTP ${healthRes.status}`);

    const topo   = await topoRes.json();
    const health = await healthRes.json();

    errBanner.style.display = 'none';
    renderGraph(topo);
    renderTable(topo.nodes);
    renderStats(health);

    document.getElementById('last-updated').textContent =
      'Updated ' + new Date().toLocaleTimeString();
  } catch (err) {
    errBanner.style.display = 'block';
    errBanner.textContent   = 'Failed to fetch topology data: ' + err.message;
  }
}

refresh();
setInterval(refresh, REFRESH_MS);
</script>
</body>
</html>
)";
    return html.str();
}

} // namespace server
} // namespace themis
