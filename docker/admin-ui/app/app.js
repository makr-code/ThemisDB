/**
 * ThemisDB Admin UI — Application Logic
 *
 * All API calls are routed through the nginx reverse proxy at /api/*,
 * which strips the /api prefix and forwards to the ThemisDB backend.
 * This avoids any CORS configuration on the ThemisDB server itself.
 */

"use strict";

/* ============================================================================
 * Constants & helpers
 * ============================================================================ */

/** Base path for ThemisDB REST API (proxied through nginx). */
const API_BASE = "/api";

/** Formats a byte count into a human-readable string (e.g. "1.2 MB"). */
function formatBytes(bytes) {
  if (bytes == null || isNaN(bytes)) return "—";
  const units = ["B", "KB", "MB", "GB", "TB"];
  let i = 0;
  let val = Number(bytes);
  while (val >= 1024 && i < units.length - 1) { val /= 1024; i++; }
  return `${val.toFixed(i === 0 ? 0 : 1)} ${units[i]}`;
}

/** Formats an uptime in seconds into "Xd Xh Xm Xs". */
function formatUptime(seconds) {
  if (seconds == null || isNaN(seconds)) return "—";
  const s = Number(seconds);
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  const parts = [];
  if (d) parts.push(`${d}d`);
  if (h) parts.push(`${h}h`);
  if (m) parts.push(`${m}m`);
  parts.push(`${s % 60}s`);
  return parts.join(" ");
}

/** Makes a fetch call and returns { ok, data, error }. */
async function apiFetch(path, options = {}) {
  try {
    const res = await fetch(`${API_BASE}${path}`, {
      headers: { "Accept": "application/json", ...options.headers },
      ...options,
    });
    const text = await res.text();
    let data;
    try { data = JSON.parse(text); } catch { data = text; }
    return { ok: res.ok, status: res.status, data };
  } catch (err) {
    return { ok: false, status: 0, data: null, error: err.message };
  }
}

/* ============================================================================
 * Navigation
 * ============================================================================ */

function activateSection(sectionId) {
  document.querySelectorAll(".section").forEach(el => el.classList.remove("active"));
  document.querySelectorAll(".nav-link").forEach(el => el.classList.remove("active"));

  const section = document.getElementById(`section-${sectionId}`);
  if (section) section.classList.add("active");

  const navLink = document.querySelector(`[data-section="${sectionId}"]`);
  if (navLink) navLink.classList.add("active");
}

function initNavigation() {
  document.querySelectorAll("[data-section]").forEach(el => {
    el.addEventListener("click", e => {
      e.preventDefault();
      const target = el.getAttribute("data-section");
      activateSection(target);
      window.location.hash = target;
    });
  });

  const hash = window.location.hash.replace("#", "");
  if (hash) activateSection(hash);
}

/* ============================================================================
 * Connection status
 * ============================================================================ */

const statusBadge = document.getElementById("connection-status");

function setStatus(state, label) {
  statusBadge.textContent = label;
  statusBadge.className = `status-badge status-${state}`;
}

async function checkHealth() {
  const { ok, data } = await apiFetch("/health");
  if (ok) {
    setStatus("ok", "Connected");
    document.getElementById("health-status").textContent = data?.status ?? "ok";
    document.getElementById("health-version").textContent = data?.version ?? "—";
  } else {
    setStatus("error", "Offline");
    document.getElementById("health-status").textContent = "unreachable";
  }
}

async function loadStats() {
  const { ok, data } = await apiFetch("/stats");
  if (ok && data) {
    document.getElementById("stat-uptime").textContent   = formatUptime(data.uptime_seconds);
    document.getElementById("stat-requests").textContent = data.total_requests?.toLocaleString() ?? "—";
    document.getElementById("stat-dbsize").textContent   = formatBytes(data.db_size_bytes);
    document.getElementById("stat-edition").textContent  = data.edition ?? "Community";
  }
}

/* ============================================================================
 * Collections
 * ============================================================================ */

function renderCollectionsTable(collections) {
  if (!Array.isArray(collections) || collections.length === 0) {
    return "<p class=\"hint\">No collections found.</p>";
  }

  const rows = collections.map(c => {
    const name     = c.name ?? c.collection ?? "—";
    const count    = c.count ?? c.document_count ?? c.size ?? "—";
    const sizeStr  = c.size_bytes != null ? formatBytes(c.size_bytes) : "—";
    const status   = c.status ?? "—";
    return `<tr>
      <td><strong>${escapeHtml(String(name))}</strong></td>
      <td>${typeof count === "number" ? count.toLocaleString() : escapeHtml(String(count))}</td>
      <td>${escapeHtml(sizeStr)}</td>
      <td>${escapeHtml(String(status))}</td>
    </tr>`;
  }).join("");

  return `<table>
    <thead><tr>
      <th>Collection</th><th>Documents</th><th>Size</th><th>Status</th>
    </tr></thead>
    <tbody>${rows}</tbody>
  </table>`;
}

async function loadCollections() {
  const container = document.getElementById("collections-content");
  container.innerHTML = "<p class=\"hint\">Loading…</p>";

  // Try /api/v1/collections first, fall back to /collections
  let result = await apiFetch("/v1/collections");
  if (!result.ok) result = await apiFetch("/collections");

  if (result.ok) {
    const list = Array.isArray(result.data) ? result.data
                 : result.data?.collections ?? result.data?.result ?? [];
    container.innerHTML = renderCollectionsTable(list);
  } else {
    container.innerHTML = `<p class="hint" style="color:var(--color-danger)">
      Failed to load collections (HTTP ${result.status}). The endpoint may not be available.
    </p>`;
  }
}

document.getElementById("btn-refresh-collections")
  .addEventListener("click", loadCollections);

/* ============================================================================
 * AQL Query
 * ============================================================================ */

function showResult(el, content, isError) {
  el.classList.remove("hidden", "error", "success");
  if (isError) el.classList.add("error");
  else el.classList.add("success");
  el.textContent = content;
}

async function runQuery() {
  const queryInput = document.getElementById("aql-input").value.trim();
  const resultEl   = document.getElementById("query-result");
  if (!queryInput) return;

  resultEl.classList.remove("hidden");
  resultEl.classList.remove("error", "success");
  resultEl.textContent = "Executing…";

  // Try POST /api/v1/query, fallback to /query
  let res = await apiFetch("/v1/query", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ query: queryInput }),
  });
  if (!res.ok && res.status === 404) {
    res = await apiFetch("/query", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ query: queryInput }),
    });
  }

  if (res.ok) {
    showResult(resultEl, JSON.stringify(res.data, null, 2), false);
  } else {
    showResult(resultEl,
      `Error ${res.status}:\n${typeof res.data === "string" ? res.data : JSON.stringify(res.data, null, 2)}`,
      true);
  }
}

document.getElementById("btn-run-query").addEventListener("click", runQuery);
document.getElementById("btn-clear-query").addEventListener("click", () => {
  document.getElementById("aql-input").value = "";
  document.getElementById("query-result").classList.add("hidden");
});
document.getElementById("aql-input").addEventListener("keydown", e => {
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter") runQuery();
});

/* ============================================================================
 * Backup / Restore
 * ============================================================================ */

async function runBackup() {
  const path     = document.getElementById("backup-path").value.trim();
  const resultEl = document.getElementById("backup-result");
  resultEl.classList.remove("hidden", "error", "success");
  resultEl.textContent = "Creating backup…";

  const body = path ? { path } : {};
  const res  = await apiFetch("/admin/backup", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });

  showResult(resultEl,
    res.ok
      ? `✓ Backup created:\n${JSON.stringify(res.data, null, 2)}`
      : `✗ Backup failed (${res.status}):\n${JSON.stringify(res.data, null, 2)}`,
    !res.ok);
}

async function runRestore() {
  const path     = document.getElementById("restore-path").value.trim();
  const resultEl = document.getElementById("restore-result");
  if (!path) {
    showResult(resultEl, "Please enter a backup path.", true);
    return;
  }

  if (!confirm(`Restore from "${path}"?\nThis will overwrite current data.`)) return;

  resultEl.classList.remove("hidden", "error", "success");
  resultEl.textContent = "Restoring…";

  const res = await apiFetch("/admin/restore", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ path }),
  });

  showResult(resultEl,
    res.ok
      ? `✓ Restore completed:\n${JSON.stringify(res.data, null, 2)}`
      : `✗ Restore failed (${res.status}):\n${JSON.stringify(res.data, null, 2)}`,
    !res.ok);
}

document.getElementById("btn-backup").addEventListener("click", runBackup);
document.getElementById("btn-restore").addEventListener("click", runRestore);

/* ============================================================================
 * Monitoring — raw Prometheus metrics
 * ============================================================================ */

function renderMetrics(text) {
  if (typeof text !== "string") return String(text);
  return text
    .split("\n")
    .map(line => {
      if (line.startsWith("#")) return `<span class="metric-comment">${escapeHtml(line)}</span>`;
      const match = line.match(/^([^{]+(?:\{[^}]*\})?\s+)([\d.e+\-]+)(?: [\d.]+)?$/);
      if (match) {
        return `${escapeHtml(match[1])}<span class="metric-value">${escapeHtml(match[2])}</span>`;
      }
      return escapeHtml(line);
    })
    .join("\n");
}

async function loadMetrics() {
  const container = document.getElementById("metrics-content");
  container.innerHTML = "Loading metrics…";

  // Metrics endpoint returns text/plain
  try {
    const res = await fetch(`${API_BASE}/metrics`, { headers: { Accept: "text/plain" } });
    if (res.ok) {
      const text = await res.text();
      container.innerHTML = renderMetrics(text);
    } else {
      container.innerHTML = `<span style="color:var(--color-danger)">HTTP ${res.status} — metrics endpoint unavailable.</span>`;
    }
  } catch (err) {
    container.innerHTML = `<span style="color:var(--color-danger)">Error: ${escapeHtml(err.message)}</span>`;
  }
}

document.getElementById("btn-refresh-metrics").addEventListener("click", loadMetrics);

/* ============================================================================
 * Utility
 * ============================================================================ */

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

/* ============================================================================
 * Initialisation
 * ============================================================================ */

async function init() {
  initNavigation();

  // Update metrics link to point via proxy
  document.getElementById("link-metrics").href = `${API_BASE}/metrics`;

  // Initial data load
  await checkHealth();
  await loadStats();

  // Refresh health status every 30 seconds
  setInterval(async () => {
    await checkHealth();
    await loadStats();
  }, 30_000);
}

init();
