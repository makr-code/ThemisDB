<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ThemisDB Telemetry Admin</title>
  <style>
    :root {
      --bg: #f4efe6;
      --panel: #fffdf9;
      --ink: #23211f;
      --muted: #6b645d;
      --accent: #a64b2a;
      --accent-2: #d08c60;
      --ok: #2e7d32;
      --warn: #a56a00;
      --danger: #b71c1c;
      --line: #ded6cb;
      --shadow: 0 10px 30px rgba(35, 33, 31, 0.08);
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "IBM Plex Sans", "Segoe UI", sans-serif;
      background: radial-gradient(1200px 500px at 10% -10%, #f7dccb 0%, transparent 60%),
                  radial-gradient(900px 500px at 100% -15%, #f2d9c5 0%, transparent 65%),
                  var(--bg);
      color: var(--ink);
    }

    .wrap {
      max-width: 1200px;
      margin: 0 auto;
      padding: 20px;
    }

    .head {
      display: flex;
      justify-content: space-between;
      gap: 16px;
      align-items: center;
      margin-bottom: 16px;
    }

    .title {
      margin: 0;
      font-family: "IBM Plex Serif", Georgia, serif;
      font-size: clamp(1.3rem, 2vw, 2rem);
      letter-spacing: 0.02em;
    }

    .panel {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 14px;
      box-shadow: var(--shadow);
      padding: 14px;
      margin-bottom: 14px;
    }

    .row {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      align-items: center;
    }

    input, select, button, textarea {
      border: 1px solid #cdbfae;
      border-radius: 10px;
      padding: 9px 10px;
      font: inherit;
      background: #fff;
      color: var(--ink);
    }

    input, select, textarea { min-width: 160px; }
    textarea { min-height: 120px; width: 100%; }

    button {
      cursor: pointer;
      background: linear-gradient(180deg, #c6663f, #9c4626);
      color: #fff;
      border: 0;
      font-weight: 600;
    }

    button.secondary {
      background: #efe4d7;
      color: #443a31;
      border: 1px solid #d9ccb9;
    }

    button.danger {
      background: linear-gradient(180deg, #cc4d4d, #9f2626);
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 10px;
    }

    .kpi {
      border: 1px solid var(--line);
      border-radius: 10px;
      padding: 10px;
      background: #fff;
    }

    .kpi .label {
      color: var(--muted);
      font-size: 0.85rem;
    }

    .kpi .val {
      font-size: 1.3rem;
      font-weight: 700;
      margin-top: 4px;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 0.92rem;
    }

    th, td {
      border-bottom: 1px solid #ece3d8;
      padding: 8px;
      text-align: left;
      vertical-align: top;
    }

    th { color: #5a4f46; }

    .badge {
      display: inline-block;
      border-radius: 999px;
      padding: 2px 8px;
      font-size: 0.75rem;
      background: #eee;
    }

    .mono { font-family: ui-monospace, Consolas, monospace; }

    .status { font-size: 0.9rem; }
    .status.ok { color: var(--ok); }
    .status.warn { color: var(--warn); }
    .status.err { color: var(--danger); }

    @media (max-width: 980px) {
      .grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    }

    @media (max-width: 640px) {
      .grid { grid-template-columns: 1fr; }
      .head { flex-direction: column; align-items: flex-start; }
      input, select { min-width: 120px; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="head">
      <h1 class="title">Telemetry Admin Dashboard</h1>
      <div class="row">
        <input id="token" type="password" placeholder="Admin Token">
        <button class="secondary" id="saveToken">Token speichern</button>
      </div>
    </div>

    <div class="panel">
      <div class="row">
        <button id="setupBtn">Setup ausfuehren</button>
        <button class="secondary" id="statusBtn">Status laden</button>
        <button class="secondary" id="statsBtn">Statistiken laden</button>
        <span id="status" class="status">Bereit.</span>
      </div>
    </div>

    <div class="panel grid">
      <div class="kpi"><div class="label">Datensaetze gesamt</div><div class="val" id="kpiTotal">-</div></div>
      <div class="kpi"><div class="label">Neuester Empfang (epoch)</div><div class="val mono" id="kpiLatest">-</div></div>
      <div class="kpi"><div class="label">Avg Query Latency (us)</div><div class="val" id="kpiAvgLat">-</div></div>
      <div class="kpi"><div class="label">Avg Cache Hit (%)</div><div class="val" id="kpiCache">-</div></div>
    </div>

    <div class="panel">
      <div class="row">
        <select id="fVersion"><option value="">Version (alle)</option></select>
        <select id="fOs"><option value="">OS (alle)</option></select>
        <select id="fArch"><option value="">Arch (alle)</option></select>
        <select id="fChannel">
          <option value="">Channel (alle)</option>
          <option value="official">official</option>
          <option value="community">community</option>
        </select>
        <button id="loadBtn">Liste laden</button>
        <button class="secondary" id="exportCsvBtn">Export CSV</button>
        <button class="secondary" id="exportJsonBtn">Export JSON</button>
        <button class="secondary" id="newBtn">Neu anlegen</button>
      </div>
    </div>

    <div class="panel" style="overflow:auto;">
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>Instance</th>
            <th>Version</th>
            <th>OS/Arch</th>
            <th>CPU Cores</th>
            <th>RAM MB</th>
            <th>Build</th>
            <th>Aktion</th>
          </tr>
        </thead>
        <tbody id="rows"></tbody>
      </table>
      <div class="row" style="margin-top:10px;">
        <button class="secondary" id="prevBtn">Zurueck</button>
        <button class="secondary" id="nextBtn">Weiter</button>
        <span id="pageInfo" class="status">-</span>
      </div>
    </div>

    <div class="panel">
      <div class="row">
        <strong>Detail / Edit (JSON)</strong>
      </div>
      <textarea id="editor" placeholder='{"instance_id":"...","themis_version":"2.0.0"}'></textarea>
      <div class="row" style="margin-top:10px;">
        <button id="createBtn">Create</button>
        <button class="secondary" id="updateBtn">Update (per ?id)</button>
        <button class="danger" id="deleteBtn">Delete (per ?id)</button>
        <input id="recordId" type="number" min="1" placeholder="ID">
      </div>
    </div>
  </div>

  <script>
    const apiBase = 'admin_api.php';
    const state = { limit: 20, offset: 0, total: 0 };

    const el = (id) => document.getElementById(id);
    const statusEl = el('status');

    function setStatus(msg, cls = '') {
      statusEl.textContent = msg;
      statusEl.className = 'status ' + cls;
    }

    function token() {
      return el('token').value.trim();
    }

    function saveToken() {
      localStorage.setItem('themis_admin_token', token());
      setStatus('Token gespeichert', 'ok');
    }

    function loadToken() {
      const t = localStorage.getItem('themis_admin_token') || '';
      el('token').value = t;
    }

    function withToken(params = {}) {
      return new URLSearchParams({ ...params, token: token() });
    }

    function filterParams() {
      return {
        themis_version: el('fVersion').value,
        os_family: el('fOs').value,
        cpu_arch: el('fArch').value,
        build_channel: el('fChannel').value,
      };
    }

    async function api(action, method = 'GET', body = null, extra = {}) {
      const qs = withToken({ action, ...extra });
      const options = { method, headers: {} };
      if (body !== null) {
        options.headers['Content-Type'] = 'application/json';
        options.body = JSON.stringify(body);
      }
      const res = await fetch(apiBase + '?' + qs.toString(), options);
      const data = await res.json().catch(() => ({}));
      if (!res.ok || !data.ok) {
        throw new Error(data.error || ('HTTP ' + res.status));
      }
      return data;
    }

    function optionize(select, values) {
      const prev = select.value;
      while (select.options.length > 1) select.remove(1);
      [...new Set(values.filter(Boolean))].sort().forEach(v => {
        const o = document.createElement('option');
        o.value = v;
        o.textContent = v;
        select.appendChild(o);
      });
      select.value = prev;
    }

    function rowHtml(item) {
      const buildBadge = `<span class="badge">${item.build_channel || '-'}</span>`;
      return `<tr>
        <td class="mono">${item.id}</td>
        <td class="mono">${item.instance_id || ''}</td>
        <td>${item.themis_version || ''}</td>
        <td>${item.os_family || ''} / ${item.cpu_arch || ''}</td>
        <td>${item.cpu_cores ?? ''}</td>
        <td>${item.total_ram_mb ?? ''}</td>
        <td>${buildBadge} ${item.build_verified ? 'signed' : 'unsigned'}</td>
        <td>
          <button class="secondary" onclick='loadOne(${item.id})'>Open</button>
        </td>
      </tr>`;
    }

    async function refreshStatus() {
      try {
        const data = await api('status');
        el('kpiTotal').textContent = data.total_records;
        el('kpiLatest').textContent = data.latest_received_at ?? '-';
        setStatus('Status geladen', 'ok');
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function refreshStats() {
      try {
        const data = await api('stats');
        el('kpiTotal').textContent = data.total_records;
        el('kpiAvgLat').textContent = data.performance_overview.avg_query_latency_us ? data.performance_overview.avg_query_latency_us.toFixed(1) : '-';
        el('kpiCache').textContent = data.performance_overview.avg_cache_hit_rate_pct ? data.performance_overview.avg_cache_hit_rate_pct.toFixed(1) : '-';
        setStatus('Statistiken geladen', 'ok');
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function refreshList() {
      try {
        const data = await api('list', 'GET', null, {
          limit: state.limit,
          offset: state.offset,
          ...filterParams(),
        });

        state.total = data.total;
        const rows = el('rows');
        rows.innerHTML = data.items.map(rowHtml).join('');
        if (!data.items.length) {
          rows.innerHTML = '<tr><td colspan="8">Keine Daten</td></tr>';
        }

        optionize(el('fVersion'), data.items.map(x => x.themis_version));
        optionize(el('fOs'), data.items.map(x => x.os_family));
        optionize(el('fArch'), data.items.map(x => x.cpu_arch));

        const start = state.offset + 1;
        const end = Math.min(state.offset + state.limit, state.total);
        el('pageInfo').textContent = `${state.total} gesamt | ${state.total ? start : 0}-${end}`;
        setStatus('Liste geladen', 'ok');
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function loadOne(id) {
      try {
        const data = await api('get', 'GET', null, { id });
        el('recordId').value = id;
        el('editor').value = JSON.stringify(data.item, null, 2);
        setStatus('Datensatz geladen: ' + id, 'ok');
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function runSetup() {
      try {
        const data = await api('setup', 'POST', {});
        setStatus(data.message, 'ok');
        await refreshStatus();
        await refreshStats();
        await refreshList();
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    function jsonFromEditor() {
      try {
        return JSON.parse(el('editor').value || '{}');
      } catch (e) {
        throw new Error('JSON im Editor ist ungueltig');
      }
    }

    async function createRecord() {
      try {
        const data = await api('create', 'POST', jsonFromEditor());
        setStatus('Datensatz erstellt: ' + data.id, 'ok');
        await refreshList();
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function updateRecord() {
      try {
        const id = Number(el('recordId').value);
        if (!id) throw new Error('ID fuer Update fehlt');
        await api('update', 'PATCH', jsonFromEditor(), { id });
        setStatus('Datensatz aktualisiert: ' + id, 'ok');
        await refreshList();
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    async function deleteRecord() {
      try {
        const id = Number(el('recordId').value);
        if (!id) throw new Error('ID fuer Delete fehlt');
        await api('delete', 'DELETE', null, { id });
        setStatus('Datensatz geloescht: ' + id, 'ok');
        await refreshList();
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    function exportData(format) {
      try {
        const action = format === 'json' ? 'export_json' : 'export_csv';
        const qs = withToken({ action, limit: 10000, ...filterParams() }).toString();
        window.open(apiBase + '?' + qs, '_blank');
        setStatus('Export gestartet: ' + format.toUpperCase(), 'ok');
      } catch (e) {
        setStatus(e.message, 'err');
      }
    }

    function resetEditorForCreate() {
      el('recordId').value = '';
      const now = Math.floor(Date.now() / 1000);
      el('editor').value = JSON.stringify({
        instance_id: '00000000-0000-4000-8000-000000000001',
        themis_version: '2.0.0',
        timestamp_utc: now,
        os_family: 'Linux',
        cpu_arch: 'x86_64',
        cpu_cores: 8,
        total_ram_mb: 16384,
        build_channel: 'community',
        build_verified: 0
      }, null, 2);
    }

    el('saveToken').addEventListener('click', saveToken);
    el('setupBtn').addEventListener('click', runSetup);
    el('statusBtn').addEventListener('click', refreshStatus);
    el('statsBtn').addEventListener('click', refreshStats);
    el('loadBtn').addEventListener('click', () => { state.offset = 0; refreshList(); });
    el('exportCsvBtn').addEventListener('click', () => exportData('csv'));
    el('exportJsonBtn').addEventListener('click', () => exportData('json'));
    el('newBtn').addEventListener('click', resetEditorForCreate);
    el('createBtn').addEventListener('click', createRecord);
    el('updateBtn').addEventListener('click', updateRecord);
    el('deleteBtn').addEventListener('click', deleteRecord);

    el('prevBtn').addEventListener('click', () => {
      state.offset = Math.max(0, state.offset - state.limit);
      refreshList();
    });

    el('nextBtn').addEventListener('click', () => {
      if (state.offset + state.limit < state.total) {
        state.offset += state.limit;
        refreshList();
      }
    });

    loadToken();
    resetEditorForCreate();
  </script>
</body>
</html>
