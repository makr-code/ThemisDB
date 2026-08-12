/**
 * @file import_wizard_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=170, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/import_wizard_builder.h"
#include <string>

namespace themis {
namespace server {

std::string buildImportWizardHtml() {
    // Inline HTML+CSS+JS import wizard.  The wizard is a self-contained
    // single-page application that drives the existing REST API endpoints:
    //   POST /api/v1/import/postgresql
    //   POST /api/v1/import/s3
    //   GET  /api/v1/import/{job_id}/status
    //   GET  /api/v1/import/jobs
    // No external CDN dependencies – all assets are embedded here so the
    // wizard works in air-gapped environments.
    std::string html;
        // Large single-pass builder with many concatenations: reserve generously
        // to reduce repeated reallocations/copies during assembly.
        html.reserve(96 * 1024);

    // ---- <head> ----
    html += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html += "<meta charset=\"UTF-8\">\n";
    html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n";
    html += "<title>ThemisDB Import Wizard</title>\n";
    html += "<style>\n";
    // Base
    html += "*{box-sizing:border-box;margin:0;padding:0}\n";
    html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;"
            "background:#0f0f1a;color:#d0d0e8;min-height:100vh;display:flex;"
            "flex-direction:column;align-items:center;padding:24px 16px}\n";
    // Card
    html += ".card{background:#16213e;border:1px solid #0f3460;border-radius:8px;"
            "padding:32px;width:100%;max-width:680px;margin-bottom:24px}\n";
    html += "h1{color:#00d4ff;font-size:1.6rem;margin-bottom:4px}\n";
    html += "h2{color:#00d4ff;font-size:1.1rem;margin-bottom:16px}\n";
    html += ".sub{color:#7a7aaa;font-size:0.85rem;margin-bottom:24px}\n";
    // Stepper
    html += ".stepper{display:flex;gap:0;margin-bottom:32px}\n";
    html += ".step{flex:1;text-align:center;padding:8px 4px;font-size:0.78rem;"
            "color:#555;border-bottom:3px solid #1e2a4a;cursor:default}\n";
    html += ".step.active{color:#00d4ff;border-bottom-color:#00d4ff;font-weight:600}\n";
    html += ".step.done{color:#00ff9f;border-bottom-color:#00ff9f}\n";
    // Form controls
    html += "label{display:block;font-size:0.82rem;color:#8888bb;margin-bottom:4px;"
            "margin-top:14px}\n";
    html += "label:first-child{margin-top:0}\n";
    html += "input,select,textarea{width:100%;padding:8px 10px;background:#0f1829;"
            "border:1px solid #0f3460;border-radius:4px;color:#d0d0e8;font-size:0.9rem}\n";
    html += "input:focus,select:focus,textarea:focus{outline:2px solid #00d4ff;"
            "border-color:transparent}\n";
    html += "textarea{resize:vertical;min-height:72px;font-family:monospace;"
            "font-size:0.82rem}\n";
    html += "select option{background:#16213e}\n";
    // Buttons
    html += ".btn{display:inline-block;padding:9px 22px;border:none;border-radius:4px;"
            "font-size:0.9rem;cursor:pointer;transition:opacity .15s}\n";
    html += ".btn-primary{background:#00d4ff;color:#0f0f1a;font-weight:600}\n";
    html += ".btn-secondary{background:#1e2a4a;color:#d0d0e8;border:1px solid #0f3460}\n";
    html += ".btn-danger{background:#ff4466;color:#fff;font-weight:600}\n";
    html += ".btn:hover{opacity:.85}\n";
    html += ".btn:disabled{opacity:.4;cursor:not-allowed}\n";
    html += ".btn-row{display:flex;gap:10px;margin-top:24px;justify-content:flex-end}\n";
    // Source selector cards
    html += ".source-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;"
            "margin-top:8px}\n";
    html += ".source-card{padding:16px;border:2px solid #1e2a4a;border-radius:6px;"
            "cursor:pointer;text-align:center;transition:border-color .15s}\n";
    html += ".source-card:hover{border-color:#0f3460}\n";
    html += ".source-card.selected{border-color:#00d4ff;background:#0f1829}\n";
    html += ".source-card .icon{font-size:1.8rem;margin-bottom:6px}\n";
    html += ".source-card .name{font-size:0.9rem;font-weight:600;color:#d0d0e8}\n";
    html += ".source-card .desc{font-size:0.75rem;color:#7a7aaa;margin-top:2px}\n";
    // Progress & status
    html += ".progress-bar-wrap{background:#0f1829;border-radius:4px;height:12px;"
            "overflow:hidden;margin:10px 0}\n";
    html += ".progress-bar{height:100%;background:#00d4ff;border-radius:4px;"
            "transition:width .4s ease}\n";
    html += ".status-badge{display:inline-block;padding:2px 10px;border-radius:10px;"
            "font-size:0.78rem;font-weight:600}\n";
    html += ".s-running{background:#1e4a7a;color:#00d4ff}\n";
    html += ".s-completed{background:#1a4a2e;color:#00ff9f}\n";
    html += ".s-failed{background:#4a1a1a;color:#ff4466}\n";
    html += ".s-pending{background:#2a2a1e;color:#ffcc00}\n";
    html += ".s-cancelled{background:#2a1e2a;color:#cc88ff}\n";
    // Job list
    html += ".job-row{display:flex;align-items:center;gap:12px;padding:10px 0;"
            "border-bottom:1px solid #1e2a4a;font-size:0.85rem}\n";
    html += ".job-id{font-family:monospace;color:#8888bb;flex:0 0 auto;width:130px;"
            "overflow:hidden;text-overflow:ellipsis;white-space:nowrap}\n";
    html += ".job-stats{flex:1;color:#8888bb}\n";
    html += ".job-stage{flex:1;color:#aaaacc;font-style:italic;font-size:0.8rem}\n";
    // Alert / error box
    html += ".alert{padding:10px 14px;border-radius:4px;margin-top:14px;"
            "font-size:0.85rem}\n";
    html += ".alert-error{background:#3a0f0f;border:1px solid #ff4466;color:#ffaaaa}\n";
    html += ".alert-info{background:#0f1f3a;border:1px solid #00d4ff;color:#aaddff}\n";
    html += ".alert-success{background:#0f2a1a;border:1px solid #00ff9f;color:#aaffcc}\n";
    // Checkbox row
    html += ".check-row{display:flex;align-items:center;gap:8px;margin-top:10px}\n";
    html += ".check-row input[type=checkbox]{width:auto}\n";
    html += ".check-row label{margin:0;font-size:0.85rem;color:#d0d0e8}\n";
    // Collapsible advanced section
    html += "details>summary{cursor:pointer;color:#8888bb;font-size:0.82rem;"
            "margin-top:18px;user-select:none}\n";
    html += "details>summary:hover{color:#00d4ff}\n";
    html += "</style>\n</head>\n<body>\n";

    // ---- Page header ----
    html += "<div class=\"card\">\n";
    html += "<h1>&#128190; ThemisDB Import Wizard</h1>\n";
    html += "<p class=\"sub\">Import data from PostgreSQL or S3 into ThemisDB in a few guided steps.</p>\n";

    // ---- Stepper ----
    html += "<div class=\"stepper\" id=\"stepper\">\n";
    html += "  <div class=\"step active\" id=\"step-tab-1\">1&nbsp;Source</div>\n";
    html += "  <div class=\"step\" id=\"step-tab-2\">2&nbsp;Connection</div>\n";
    html += "  <div class=\"step\" id=\"step-tab-3\">3&nbsp;Options</div>\n";
    html += "  <div class=\"step\" id=\"step-tab-4\">4&nbsp;Review</div>\n";
    html += "  <div class=\"step\" id=\"step-tab-5\">5&nbsp;Progress</div>\n";
    html += "</div>\n";

    // ---- Step 1: Choose source ----
    html += "<div id=\"panel-1\">\n";
    html += "<h2>Choose a data source</h2>\n";
    html += "<div class=\"source-grid\">\n";
    html += "  <div class=\"source-card selected\" data-src=\"postgresql\" onclick=\"selectSource('postgresql',this)\">\n";
    html += "    <div class=\"icon\">&#128036;</div>\n";
    html += "    <div class=\"name\">PostgreSQL</div>\n";
    html += "    <div class=\"desc\">pg_dump file or connection string</div>\n";
    html += "  </div>\n";
    html += "  <div class=\"source-card\" data-src=\"s3\" onclick=\"selectSource('s3',this)\">\n";
    html += "    <div class=\"icon\">&#9729;</div>\n";
    html += "    <div class=\"name\">S3 / Object Storage</div>\n";
    html += "    <div class=\"desc\">s3://bucket/key (CSV, TSV, JSONL)</div>\n";
    html += "  </div>\n";
    html += "</div>\n";
    html += "<div class=\"btn-row\"><button class=\"btn btn-primary\" onclick=\"goStep(2)\">Next &rarr;</button></div>\n";
    html += "</div>\n";  // panel-1

    // ---- Step 2: Connection / source path ----
    html += "<div id=\"panel-2\" style=\"display:none\">\n";
    html += "<h2>Configure source</h2>\n";
    html += "<div id=\"pg-fields\">\n";
    html += "  <label for=\"pg-path\">pg_dump file path or PostgreSQL connection string</label>\n";
    html += "  <input id=\"pg-path\" type=\"text\" placeholder=\"/path/to/dump.sql  or  postgresql://user:pass@host/db\">\n";
    html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">The path is resolved on the ThemisDB server.</p>\n";
    html += "</div>\n";
    html += "<div id=\"s3-fields\" style=\"display:none\">\n";
    html += "  <label for=\"s3-path\">S3 URL</label>\n";
    html += "  <input id=\"s3-path\" type=\"text\" placeholder=\"s3://my-bucket/export/dump.csv\">\n";
    html += "  <p style=\"font-size:0.78rem;color:#7a7aaa;margin-top:4px\">Use a trailing / to import an entire prefix.</p>\n";
    html += "</div>\n";
    html += "<div id=\"step2-error\" class=\"alert alert-error\" style=\"display:none\"></div>\n";
    html += "<div class=\"btn-row\">\n";
    html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(1)\">&larr; Back</button>\n";
    html += "  <button class=\"btn btn-primary\" onclick=\"goStep(3)\">Next &rarr;</button>\n";
    html += "</div>\n";
    html += "</div>\n";  // panel-2

    // ---- Step 3: Import options ----
    html += "<div id=\"panel-3\" style=\"display:none\">\n";
    html += "<h2>Import options</h2>\n";
    html += "<label for=\"opt-namespace\">Target namespace</label>\n";
    html += "<input id=\"opt-namespace\" type=\"text\" value=\"imported\" placeholder=\"imported\">\n";
    html += "<label for=\"opt-batch\">Batch size (rows per transaction)</label>\n";
    html += "<input id=\"opt-batch\" type=\"number\" value=\"1000\" min=\"1\" max=\"100000\">\n";
    html += "<div class=\"check-row\">\n";
    html += "  <input id=\"opt-dryrun\" type=\"checkbox\">\n";
    html += "  <label for=\"opt-dryrun\">Dry-run (validate without writing data)</label>\n";
    html += "</div>\n";
    html += "<div class=\"check-row\">\n";
    html += "  <input id=\"opt-skip-dup\" type=\"checkbox\" checked>\n";
    html += "  <label for=\"opt-skip-dup\">Skip duplicate records</label>\n";
    html += "</div>\n";
    html += "<div class=\"check-row\">\n";
    html += "  <input id=\"opt-continue-err\" type=\"checkbox\" checked>\n";
    html += "  <label for=\"opt-continue-err\">Continue on row errors</label>\n";
    html += "</div>\n";
    html += "<details><summary>&#9881; Advanced options</summary>\n";
    html += "<label for=\"opt-include\">Include tables (comma-separated, empty = all)</label>\n";
    html += "<input id=\"opt-include\" type=\"text\" placeholder=\"users, orders, products\">\n";
    html += "<label for=\"opt-exclude\">Exclude tables (comma-separated)</label>\n";
    html += "<input id=\"opt-exclude\" type=\"text\" placeholder=\"audit_log, tmp_\">\n";
    html += "<label for=\"opt-conflict\">Conflict strategy</label>\n";
    html += "<select id=\"opt-conflict\">\n";
    html += "  <option value=\"0\" selected>Overwrite (replace existing)</option>\n";
    html += "  <option value=\"1\">Skip (keep existing)</option>\n";
    html += "  <option value=\"2\">Merge (field-level, incoming wins)</option>\n";
    html += "  <option value=\"3\">Error (abort on first conflict)</option>\n";
    html += "</select>\n";
    html += "</details>\n";
    html += "<div class=\"btn-row\">\n";
    html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(2)\">&larr; Back</button>\n";
    html += "  <button class=\"btn btn-primary\" onclick=\"goStep(4)\">Review &rarr;</button>\n";
    html += "</div>\n";
    html += "</div>\n";  // panel-3

    // ---- Step 4: Review & start ----
    html += "<div id=\"panel-4\" style=\"display:none\">\n";
    html += "<h2>Review &amp; start import</h2>\n";
    html += "<div id=\"review-summary\" style=\"font-size:0.88rem;line-height:1.7;"
            "background:#0f1829;border-radius:4px;padding:14px\"></div>\n";
    html += "<div class=\"btn-row\">\n";
    html += "  <button class=\"btn btn-secondary\" onclick=\"goStep(3)\">&larr; Back</button>\n";
    html += "  <button class=\"btn btn-primary\" id=\"btn-start\" onclick=\"startImport()\">&#9654; Start Import</button>\n";
    html += "</div>\n";
    html += "</div>\n";  // panel-4

    // ---- Step 5: Progress & results ----
    html += "<div id=\"panel-5\" style=\"display:none\">\n";
    html += "<h2>Import progress</h2>\n";
    html += "<div id=\"progress-wrap\">\n";
    html += "  <div style=\"display:flex;justify-content:space-between;align-items:center;margin-bottom:4px\">\n";
    html += "    <span id=\"prog-stage\" style=\"font-size:0.85rem;color:#aaaacc;font-style:italic\">Initializing&#8230;</span>\n";
    html += "    <span id=\"prog-badge\" class=\"status-badge s-running\">running</span>\n";
    html += "  </div>\n";
    html += "  <div class=\"progress-bar-wrap\"><div class=\"progress-bar\" id=\"prog-bar\" style=\"width:0%\"></div></div>\n";
    html += "  <div style=\"font-size:0.8rem;color:#7a7aaa;margin-top:4px\">"
            "<span id=\"prog-current\">0</span> / <span id=\"prog-total\">?</span> records</div>\n";
    html += "</div>\n";
    html += "<div id=\"result-box\" style=\"display:none;margin-top:18px\"></div>\n";
    html += "<div class=\"btn-row\">\n";
    html += "  <button class=\"btn btn-secondary\" id=\"btn-new\" onclick=\"resetWizard()\">&#8635; New Import</button>\n";
    html += "  <button class=\"btn btn-danger\" id=\"btn-cancel\" onclick=\"cancelImport()\">&#9632; Cancel</button>\n";
    html += "</div>\n";
    html += "</div>\n";  // panel-5

    html += "</div>\n";  // card

    // ---- Recent jobs panel ----
    html += "<div class=\"card\" id=\"jobs-panel\">\n";
    html += "<h2>Recent import jobs</h2>\n";
    html += "<div id=\"jobs-list\"><em style=\"color:#555\">Loading&#8230;</em></div>\n";
    html += "<div style=\"margin-top:10px\">"
            "<button class=\"btn btn-secondary\" style=\"font-size:0.8rem\" onclick=\"loadJobs()\">&#8635; Refresh</button>"
            "</div>\n";
    html += "</div>\n";  // jobs-panel

    // ---- JavaScript ----
    html += "<script>\n";
    html += "var currentStep=1,currentSource='postgresql',currentJobId=null,pollTimer=null;\n";

    // selectSource
    html += "function selectSource(src,el){\n";
    html += "  currentSource=src;\n";
    html += "  document.querySelectorAll('.source-card').forEach(function(c){c.classList.remove('selected');});\n";
    html += "  el.classList.add('selected');\n";
    html += "}\n";

    // goStep
    html += "function goStep(n){\n";
    html += "  if(n===2){\n";
    html += "    document.getElementById('pg-fields').style.display=(currentSource==='postgresql')?'':'none';\n";
    html += "    document.getElementById('s3-fields').style.display=(currentSource==='s3')?'':'none';\n";
    html += "    document.getElementById('step2-error').style.display='none';\n";
    html += "  }\n";
    html += "  if(n===4) buildReview();\n";
    html += "  for(var i=1;i<=5;i++){\n";
    html += "    var p=document.getElementById('panel-'+i);\n";
    html += "    var t=document.getElementById('step-tab-'+i);\n";
    html += "    if(p) p.style.display=(i===n)?'':'none';\n";
    html += "    if(t){\n";
    html += "      t.className='step'+(i===n?' active':i<n?' done':'');\n";
    html += "    }\n";
    html += "  }\n";
    html += "  currentStep=n;\n";
    html += "}\n";

    // buildReview
    html += "function buildReview(){\n";
    html += "  var path=(currentSource==='postgresql')\n";
    html += "    ?document.getElementById('pg-path').value.trim()\n";
    html += "    :document.getElementById('s3-path').value.trim();\n";
    html += "  var ns=document.getElementById('opt-namespace').value.trim()||'imported';\n";
    html += "  var bs=document.getElementById('opt-batch').value||'1000';\n";
    html += "  var dr=document.getElementById('opt-dryrun').checked;\n";
    html += "  var sd=document.getElementById('opt-skip-dup').checked;\n";
    html += "  var ce=document.getElementById('opt-continue-err').checked;\n";
    html += "  var inc=document.getElementById('opt-include').value.trim();\n";
    html += "  var exc=document.getElementById('opt-exclude').value.trim();\n";
    html += "  var conf=['Overwrite','Skip','Merge','Error'][parseInt(document.getElementById('opt-conflict').value)||0];\n";
    html += "  var html='<table style=\"border-collapse:collapse;width:100%\">';\n";
    html += "  function row(k,v){return '<tr><td style=\"color:#7a7aaa;padding:3px 8px 3px 0;white-space:nowrap\">'+k+'</td>"
            "<td style=\"color:#d0d0e8;padding:3px 0\">'+v+'</td></tr>';}\n";
    html += "  html+=row('Source type',currentSource==='postgresql'?'PostgreSQL':'S3 / Object Storage');\n";
    html += "  html+=row('Source path','<code>'+escHtml(path)+'</code>');\n";
    html += "  html+=row('Target namespace','<code>'+escHtml(ns)+'</code>');\n";
    html += "  html+=row('Batch size',bs);\n";
    html += "  html+=row('Dry-run',dr?'<span style=\"color:#ffcc00\">Yes (no data written)</span>':'No');\n";
    html += "  html+=row('Skip duplicates',sd?'Yes':'No');\n";
    html += "  html+=row('Continue on error',ce?'Yes':'No');\n";
    html += "  if(inc) html+=row('Include tables','<code>'+escHtml(inc)+'</code>');\n";
    html += "  if(exc) html+=row('Exclude tables','<code>'+escHtml(exc)+'</code>');\n";
    html += "  html+=row('Conflict strategy',conf);\n";
    html += "  html+='</table>';\n";
    html += "  document.getElementById('review-summary').innerHTML=html;\n";
    html += "}\n";

    // escHtml
    html += "function escHtml(s){\n";
    html += "  return String(s).replace(/&/g,'&amp;').replace(/</g,'<').replace(/>/g,'>');\n";
    html += "}\n";

    // buildOptions
    html += "function buildOptions(){\n";
    html += "  var opts={\n";
    html += "    default_namespace:document.getElementById('opt-namespace').value.trim()||'imported',\n";
    html += "    batch_size:parseInt(document.getElementById('opt-batch').value)||1000,\n";
    html += "    dry_run:document.getElementById('opt-dryrun').checked,\n";
    html += "    skip_duplicates:document.getElementById('opt-skip-dup').checked,\n";
    html += "    continue_on_error:document.getElementById('opt-continue-err').checked,\n";
    html += "    conflict_strategy:parseInt(document.getElementById('opt-conflict').value)||0\n";
    html += "  };\n";
    html += "  var inc=document.getElementById('opt-include').value.trim();\n";
    html += "  var exc=document.getElementById('opt-exclude').value.trim();\n";
    html += "  if(inc) opts.include_tables=inc.split(',').map(function(s){return s.trim();}).filter(Boolean);\n";
    html += "  if(exc) opts.exclude_tables=exc.split(',').map(function(s){return s.trim();}).filter(Boolean);\n";
    html += "  return opts;\n";
    html += "}\n";

    // startImport
    html += "function startImport(){\n";
    html += "  var path=(currentSource==='postgresql')\n";
    html += "    ?document.getElementById('pg-path').value.trim()\n";
    html += "    :document.getElementById('s3-path').value.trim();\n";
    html += "  if(!path){alert('Please enter a source path.');return;}\n";
    html += "  var url=(currentSource==='postgresql')?'/api/v1/import/postgresql':'/api/v1/import/s3';\n";
    html += "  var body=JSON.stringify({source_path:path,options:buildOptions()});\n";
    html += "  document.getElementById('btn-start').disabled=true;\n";
    html += "  fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:body})\n";
    html += "    .then(function(r){return r.json();})\n";
    html += "    .then(function(data){\n";
    html += "      if(data.error){alert('Error: '+data.error);document.getElementById('btn-start').disabled=false;return;}\n";
    html += "      currentJobId=data.id;\n";
    html += "      goStep(5);\n";
    html += "      pollProgress();\n";
    html += "    })\n";
    html += "    .catch(function(e){alert('Request failed: '+e);document.getElementById('btn-start').disabled=false;});\n";
    html += "}\n";

    // pollProgress
    html += "function pollProgress(){\n";
    html += "  if(!currentJobId) return;\n";
    html += "  fetch('/api/v1/import/'+currentJobId+'/status')\n";
    html += "    .then(function(r){return r.json();})\n";
    html += "    .then(function(data){\n";
    html += "      updateProgress(data);\n";
    html += "      if(data.status==='running'||data.status==='pending'){\n";
    html += "        pollTimer=setTimeout(pollProgress,1200);\n";
    html += "      } else {\n";
    html += "        showResult(data);\n";
    html += "        loadJobs();\n";
    html += "      }\n";
    html += "    })\n";
    html += "    .catch(function(){\n";
    html += "      pollTimer=setTimeout(pollProgress,3000);\n";
    html += "    });\n";
    html += "}\n";

    // updateProgress
    html += "function updateProgress(data){\n";
    html += "  var cur=data.current_records||0,tot=data.total_records||0;\n";
    html += "  document.getElementById('prog-current').textContent=cur;\n";
    html += "  document.getElementById('prog-total').textContent=tot>0?tot:'?';\n";
    html += "  var pct=(tot>0)?Math.min(100,Math.round(cur/tot*100)):0;\n";
    html += "  document.getElementById('prog-bar').style.width=pct+'%';\n";
    html += "  document.getElementById('prog-stage').textContent=data.stage||data.status;\n";
    html += "  var badge=document.getElementById('prog-badge');\n";
    html += "  badge.textContent=data.status;\n";
    html += "  badge.className='status-badge s-'+(data.status||'running');\n";
    html += "}\n";

    // showResult
    html += "function showResult(data){\n";
    html += "  document.getElementById('btn-cancel').style.display='none';\n";
    html += "  var box=document.getElementById('result-box');\n";
    html += "  box.style.display='';\n";
    html += "  if(data.status==='completed'&&data.stats){\n";
    html += "    var s=data.stats;\n";
    html += "    var cls=(s.failed_records>0)?'alert-info':'alert-success';\n";
    html += "    box.innerHTML='<div class=\"alert '+cls+'\">';\n";
    html += "    box.innerHTML+='<strong>Import complete</strong><br>';\n";
    html += "    box.innerHTML+='Imported: <b>'+s.imported_records+'</b> &nbsp; ';\n";
    html += "    box.innerHTML+='Skipped: <b>'+s.skipped_records+'</b> &nbsp; ';\n";
    html += "    box.innerHTML+='Failed: <b>'+s.failed_records+'</b> &nbsp; ';\n";
    html += "    box.innerHTML+='Time: <b>'+(s.elapsed_seconds||0).toFixed(2)+'s</b>';\n";
    html += "    box.innerHTML+='</div>';\n";
    html += "  } else if(data.status==='failed'||data.status==='cancelled'){\n";
    html += "    box.innerHTML='<div class=\"alert alert-error\">Import '+data.status+'.</div>';\n";
    html += "  }\n";
    html += "}\n";

    // cancelImport
    html += "function cancelImport(){\n";
    html += "  if(!currentJobId) return;\n";
    html += "  if(pollTimer){clearTimeout(pollTimer);pollTimer=null;}\n";
    html += "  fetch('/api/v1/import/'+currentJobId+'/cancel',{method:'POST'})\n";
    html += "    .then(function(r){return r.json();})\n";
    html += "    .then(function(data){\n";
    html += "      updateProgress(data);\n";
    html += "      showResult(data);\n";
    html += "    }).catch(function(){});\n";
    html += "}\n";

    // resetWizard
    html += "function resetWizard(){\n";
    html += "  if(pollTimer){clearTimeout(pollTimer);pollTimer=null;}\n";
    html += "  currentJobId=null;\n";
    html += "  document.getElementById('btn-start').disabled=false;\n";
    html += "  document.getElementById('btn-cancel').style.display='';\n";
    html += "  document.getElementById('result-box').style.display='none';\n";
    html += "  document.getElementById('prog-bar').style.width='0%';\n";
    html += "  goStep(1);\n";
    html += "}\n";

    // loadJobs
    html += "function loadJobs(){\n";
    html += "  fetch('/api/v1/import/jobs')\n";
    html += "    .then(function(r){return r.json();})\n";
    html += "    .then(function(jobs){\n";
    html += "      var el=document.getElementById('jobs-list');\n";
    html += "      if(!jobs.length){el.innerHTML='<em style=\"color:#555\">No jobs yet.</em>';return;}\n";
    html += "      var html='';\n";
    html += "      jobs.slice().reverse().forEach(function(j){\n";
    html += "        var stats=j.stats||{};\n";
    html += "        html+='<div class=\"job-row\">';\n";
    html += "        html+='<span class=\"job-id\" title=\"'+escHtml(j.id)+'\">'+escHtml(j.id)+'</span>';\n";
    html += "        html+='<span class=\"status-badge s-'+(j.status||'pending')+'\">'+escHtml(j.status)+'</span>';\n";
    html += "        if(stats.imported_records!==undefined){\n";
    html += "          html+='<span class=\"job-stats\">'+stats.imported_records+' imported, '+stats.failed_records+' failed</span>';\n";
    html += "        } else {\n";
    html += "          html+='<span class=\"job-stage\">'+escHtml(j.stage||'')+'</span>';\n";
    html += "        }\n";
    html += "        html+='</div>';\n";
    html += "      });\n";
    html += "      el.innerHTML=html;\n";
    html += "    }).catch(function(e){document.getElementById('jobs-list').innerHTML='<em style=\"color:#ff4466\">Failed to load jobs.</em>';});\n";
    html += "}\n";

    // init on load
    html += "loadJobs();\n";
    html += "</script>\n";
    html += "</body>\n</html>\n";

    return html;
}

} // namespace server
} // namespace themis

