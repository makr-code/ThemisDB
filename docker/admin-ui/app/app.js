/**
 * ThemisDB Admin UI — Application Logic
 *
 * All API calls are routed through the nginx reverse proxy at /api/*,
 * which strips the /api prefix and forwards to the ThemisDB backend.
 * This avoids any CORS configuration on the ThemisDB server itself.
 *
 * Phase 2 additions:
 *   - Auth state machine: login overlay, Bearer token, session storage
 *   - CSRF nonce: generated at page load, sent as X-CSRF-Token on all
 *     state-changing requests (POST/PUT/PATCH/DELETE)
 *   - 401 interception: any 401 response shows the login overlay
 *   - Logout: DELETE /api/auth/sessions/{id}
 */

"use strict";

/* ============================================================================
 * Constants & helpers
 * ============================================================================ */

/** Base path for ThemisDB REST API (proxied through nginx). */
const API_BASE = "/api";

/** Session storage keys */
const SS_TOKEN      = "themisdb_token";
const SS_SESSION_ID = "themisdb_session_id";
const SS_USERNAME   = "themisdb_username";
const SS_CSRF_NONCE = "themisdb_csrf_nonce";

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

/* ============================================================================
 * Auth & CSRF (Phase 2)
 * ============================================================================ */

/**
 * Generate a cryptographically random CSRF nonce (32 hex chars).
 * Stored in sessionStorage so it survives page navigation within the tab
 * but is discarded when the tab is closed.
 */
function getOrCreateCsrfNonce() {
  let nonce = sessionStorage.getItem(SS_CSRF_NONCE);
  if (!nonce) {
    const arr = new Uint8Array(16);
    crypto.getRandomValues(arr);
    nonce = Array.from(arr, b => b.toString(16).padStart(2, "0")).join("");
    sessionStorage.setItem(SS_CSRF_NONCE, nonce);
  }
  // Also embed in the meta tag for potential server-side reads
  const meta = document.getElementById("meta-csrf-nonce");
  if (meta) meta.content = nonce;
  return nonce;
}

/** Returns the stored Bearer token (or null if not authenticated). */
function getToken() {
  return sessionStorage.getItem(SS_TOKEN);
}

/** Returns true when a valid token exists in storage. */
function isAuthenticated() {
  return Boolean(getToken());
}

/** Persist a successful login response into sessionStorage. */
function persistSession(token, sessionId, username) {
  sessionStorage.setItem(SS_TOKEN,      token);
  sessionStorage.setItem(SS_SESSION_ID, sessionId);
  sessionStorage.setItem(SS_USERNAME,   username ?? "");
  // Regenerate CSRF nonce on every new session
  sessionStorage.removeItem(SS_CSRF_NONCE);
  getOrCreateCsrfNonce();
}

/** Clear session data (logout). */
function clearSession() {
  sessionStorage.removeItem(SS_TOKEN);
  sessionStorage.removeItem(SS_SESSION_ID);
  sessionStorage.removeItem(SS_USERNAME);
  sessionStorage.removeItem(SS_CSRF_NONCE);
}

/* ---- Login overlay helpers --------------------------------------------- */

const loginOverlay   = document.getElementById("login-overlay");
const loginForm      = document.getElementById("login-form");
const loginErrorEl   = document.getElementById("login-error");
const btnLogout      = document.getElementById("btn-logout");
const loggedInUser   = document.getElementById("logged-in-user");

function showLoginOverlay() {
  loginOverlay.classList.remove("hidden");
  document.getElementById("login-username").focus();
}

function hideLoginOverlay() {
  loginOverlay.classList.add("hidden");
  loginErrorEl.classList.add("hidden");
  loginErrorEl.textContent = "";
}

function showLoginError(msg) {
  loginErrorEl.textContent = msg;
  loginErrorEl.classList.remove("hidden");
}

function updateAuthUI() {
  const user = sessionStorage.getItem(SS_USERNAME) || "";
  if (isAuthenticated()) {
    hideLoginOverlay();
    btnLogout.classList.remove("hidden");
    if (user) {
      loggedInUser.textContent = `👤 ${user}`;
      loggedInUser.classList.remove("hidden");
    }
  } else {
    btnLogout.classList.add("hidden");
    loggedInUser.classList.add("hidden");
    showLoginOverlay();
  }
}

/* ---- Login form submit -------------------------------------------------- */

loginForm.addEventListener("submit", async e => {
  e.preventDefault();
  showLoginError("");
  const username = document.getElementById("login-username").value.trim();
  const password = document.getElementById("login-password").value;
  const btn = document.getElementById("btn-login");

  btn.disabled = true;
  btn.textContent = "Signing in…";

  // POST credentials to ThemisDB session endpoint (no CSRF on login itself)
  try {
    const res = await fetch(`${API_BASE}/auth/sessions`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "Accept": "application/json",
      },
      body: JSON.stringify({ username, password }),
    });

    const text = await res.text();
    let data;
    try { data = JSON.parse(text); } catch { data = {}; }

    if (res.ok && (data.token || data.session_id)) {
      const token     = data.token ?? data.session_id;
      const sessionId = data.session_id ?? token;
      persistSession(token, sessionId, data.user_id ?? username);
      document.getElementById("login-password").value = "";
      updateAuthUI();
      // Re-run the initial load now that we're authenticated
      await checkHealth();
      await loadStats();
    } else {
      const errMsg = data?.message ?? data?.error ?? `HTTP ${res.status}`;
      if (res.status === 429) {
        showLoginError("Too many login attempts. Please wait and retry.");
      } else if (res.status === 401 || res.status === 403) {
        showLoginError("Invalid username or password.");
      } else {
        showLoginError(`Login failed: ${errMsg}`);
      }
    }
  } catch (err) {
    showLoginError(`Network error: ${err.message}`);
  } finally {
    btn.disabled = false;
    btn.textContent = "Sign in";
  }
});

/* ---- Logout ---------------------------------------------------------------- */

async function logout() {
  const sessionId = sessionStorage.getItem(SS_SESSION_ID);
  if (sessionId) {
    // Best-effort — ignore errors (session may already be expired)
    await apiFetch(`/auth/sessions/${encodeURIComponent(sessionId)}`, {
      method: "DELETE",
    }).catch(() => {});
  }
  clearSession();
  updateAuthUI();
  // Reset dashboard to blank state
  document.getElementById("health-status").textContent  = "—";
  document.getElementById("health-version").textContent = "—";
  document.getElementById("stat-uptime").textContent    = "—";
  document.getElementById("stat-requests").textContent  = "—";
  document.getElementById("stat-dbsize").textContent    = "—";
  document.getElementById("stat-edition").textContent   = "—";
  setStatus("checking", "Connecting…");
}

btnLogout.addEventListener("click", logout);

/* ============================================================================
 * apiFetch — authenticated, CSRF-aware
 * ============================================================================ */

/**
 * Makes a fetch call with automatic:
 *   - Authorization: Bearer <token>  (when authenticated)
 *   - X-CSRF-Token: <nonce>          (on POST/PUT/PATCH/DELETE)
 * Returns { ok, status, data, error }.
 * On HTTP 401, triggers the login overlay.
 */
async function apiFetch(path, options = {}) {
  const token  = getToken();
  const method = (options.method ?? "GET").toUpperCase();
  const isStateChange = ["POST", "PUT", "PATCH", "DELETE"].includes(method);

  const headers = {
    "Accept": "application/json",
    ...options.headers,
  };

  if (token) {
    headers["Authorization"] = `Bearer ${token}`;
  }

  if (isStateChange) {
    headers["X-CSRF-Token"] = getOrCreateCsrfNonce();
  }

  try {
    const res = await fetch(`${API_BASE}${path}`, {
      ...options,
      headers,
    });

    // Session expired or token rejected — show login
    if (res.status === 401) {
      clearSession();
      updateAuthUI();
      return { ok: false, status: 401, data: null, error: "Unauthenticated" };
    }

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
  const { ok, status, data } = await apiFetch("/health");
  if (ok) {
    setStatus("ok", "Connected");
    document.getElementById("health-status").textContent = data?.status ?? "ok";
    document.getElementById("health-version").textContent = data?.version ?? "—";
  } else if (status === 401) {
    setStatus("error", "Auth required");
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
  const token = getToken();

  try {
    const headers = { Accept: "text/plain" };
    if (token) headers["Authorization"] = `Bearer ${token}`;
    const res = await fetch(`${API_BASE}/metrics`, { headers });
    if (res.ok) {
      const text = await res.text();
      container.innerHTML = renderMetrics(text);
    } else if (res.status === 401) {
      clearSession();
      updateAuthUI();
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
  // Generate (or restore) CSRF nonce immediately
  getOrCreateCsrfNonce();

  initNavigation();

  // Update metrics link to point via proxy
  document.getElementById("link-metrics").href = `${API_BASE}/metrics`;

  // Show login overlay if not authenticated; otherwise run initial data load
  if (!isAuthenticated()) {
    // Probe the health endpoint to detect whether auth is required
    // (unauthenticated health check may succeed on open instances)
    const probe = await fetch(`${API_BASE}/health`, {
      headers: { Accept: "application/json" },
    }).catch(() => null);

    if (probe && probe.status === 401) {
      // Auth required — show login overlay
      showLoginOverlay();
      setStatus("error", "Auth required");
    } else if (probe && probe.ok) {
      // Open instance (THEMIS_AUTH_ENABLED=false) — proceed unauthenticated
      hideLoginOverlay();
      await checkHealth();
      await loadStats();
    } else {
      // Backend unreachable
      setStatus("error", "Offline");
      // Still show login so the user can provide credentials in case the
      // backend becomes available after a restart
      showLoginOverlay();
    }
  } else {
    hideLoginOverlay();
    updateAuthUI();
    await checkHealth();
    await loadStats();
  }

  // Refresh health + stats every 30 seconds (silently skips when logged out)
  setInterval(async () => {
    if (isAuthenticated() || !loginOverlay.classList.contains("hidden") === false) {
      await checkHealth();
      await loadStats();
    }
  }, 30_000);
}

init();
