#ifndef ADMIN_HTML_H
#define ADMIN_HTML_H

#define ADMIN_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <title>Shortener admin</title>\n" \
"  <style>\n" \
"    :root { color-scheme: dark; --bg:#0f172a; --surface:#172033; --surface-2:#202c42; --text:#f8fafc; --muted:#94a3b8; --border:#34435f; --primary:#60a5fa; --danger:#f87171; --ok:#4ade80; }\n" \
"    * { box-sizing:border-box; } body { margin:0; min-width:320px; font:15px/1.45 system-ui,-apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif; background:var(--bg); color:var(--text); }\n" \
"    main { width:min(1120px,100%); margin:0 auto; padding:clamp(1rem,4vw,2.5rem); } .top { display:flex; justify-content:space-between; align-items:center; gap:1rem; margin-bottom:1.5rem; } h1 { margin:0; font-size:clamp(1.25rem,3vw,1.75rem); } a { color:var(--primary); }\n" \
"    .panel { background:var(--surface); border:1px solid var(--border); border-radius:14px; padding:clamp(1rem,3vw,1.5rem); box-shadow:0 18px 45px rgb(0 0 0 / .2); } .auth { max-width:620px; margin:8vh auto; } .auth h2 { margin-top:0; } .hint,.status { color:var(--muted); font-size:.9rem; } .status[aria-live] { min-height:1.4em; }\n" \
"    label { display:block; margin:1rem 0 .4rem; color:var(--muted); font-size:.85rem; font-weight:600; } input { width:100%; min-height:44px; padding:.7rem .8rem; border:1px solid var(--border); border-radius:8px; background:var(--bg); color:var(--text); font:inherit; } input:focus { outline:2px solid var(--primary); outline-offset:2px; }\n" \
"    .actions { display:flex; flex-wrap:wrap; gap:.65rem; margin-top:1rem; } button { min-height:40px; border:1px solid var(--border); border-radius:8px; padding:.55rem .85rem; color:var(--text); background:var(--surface-2); font:inherit; font-weight:650; cursor:pointer; } button:hover { border-color:var(--primary); } button:focus-visible { outline:2px solid var(--primary); outline-offset:2px; } button.primary { border-color:var(--primary); background:#2563eb; color:white; } button.danger { border-color:#b91c1c; background:transparent; color:var(--danger); } button:disabled { cursor:wait; opacity:.65; }\n" \
"    .summary { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:.75rem; margin:1rem 0; } .metric { padding:1rem; border:1px solid var(--border); border-radius:10px; background:var(--surface); } .metric strong { display:block; color:var(--primary); font-size:1.45rem; } .metric span { color:var(--muted); font-size:.78rem; text-transform:uppercase; letter-spacing:.06em; }\n" \
"    .table-wrap { overflow-x:auto; border:1px solid var(--border); border-radius:12px; background:var(--surface); } table { width:100%; min-width:760px; border-collapse:collapse; } th,td { padding:.75rem .85rem; text-align:left; border-bottom:1px solid var(--border); vertical-align:middle; } th { color:var(--muted); font-size:.75rem; text-transform:uppercase; letter-spacing:.06em; } tr:last-child td { border-bottom:0; } .url { max-width:290px; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; color:var(--muted); } .badge { display:inline-block; margin:.1rem .2rem .1rem 0; padding:.1rem .4rem; border-radius:999px; background:#3f2e17; color:#facc15; font-size:.72rem; } .badge.exp { background:#472530; color:#fda4af; } .empty { padding:2rem; text-align:center; color:var(--muted); }\n" \
"    [hidden] { display:none !important; } @media (max-width:620px) { .top { align-items:flex-start; flex-direction:column; } .summary { grid-template-columns:1fr; } .actions button { flex:1 1 auto; } }\n" \
"  </style>\n" \
"</head>\n" \
"<body>\n" \
"  <main>\n" \
"    <header class=\"top\"><h1><a href=\"/\" aria-label=\"Back to shortener\">&larr;</a> Admin dashboard</h1><span class=\"hint\">Key stays only in this tab's memory.</span></header>\n" \
"    <section class=\"panel auth\" id=\"authPanel\" aria-labelledby=\"authTitle\">\n" \
"      <h2 id=\"authTitle\">Unlock dashboard</h2><p class=\"hint\">Enter the API key to load and manage links. It is never placed in the URL or browser storage.</p>\n" \
"      <form id=\"authForm\"><label for=\"apiKey\">API key</label><input id=\"apiKey\" type=\"password\" autocomplete=\"current-password\" required><div class=\"actions\"><button class=\"primary\" type=\"submit\">Open dashboard</button></div></form>\n" \
"      <p class=\"status\" id=\"authStatus\" aria-live=\"polite\"></p>\n" \
"    </section>\n" \
"    <section id=\"dashboard\" hidden aria-live=\"polite\">\n" \
"      <div class=\"actions\"><button id=\"refresh\" type=\"button\">Refresh</button><button id=\"lock\" type=\"button\">Lock dashboard</button></div>\n" \
"      <div class=\"summary\"><div class=\"metric\"><strong id=\"totalLinks\">0</strong><span>Links</span></div><div class=\"metric\"><strong id=\"totalVisits\">0</strong><span>Total visits</span></div><div class=\"metric\"><strong id=\"totalUnique\">0</strong><span>Unique visitors</span></div></div>\n" \
"      <p class=\"status\" id=\"dashboardStatus\" aria-live=\"polite\"></p>\n" \
"      <div class=\"table-wrap\"><table><thead><tr><th>Slug</th><th>Target URL</th><th>Created</th><th>Flags</th><th>Visits</th><th>Action</th></tr></thead><tbody id=\"linkRows\"></tbody></table></div>\n" \
"    </section>\n" \
"  </main>\n" \
"  <script>\n" \
"  (() => {\n" \
"    'use strict';\n" \
"    let apiKey = '';\n" \
"    const byId = (id) => document.getElementById(id);\n" \
"    const authPanel = byId('authPanel'), dashboard = byId('dashboard'), authStatus = byId('authStatus'), dashboardStatus = byId('dashboardStatus'), rows = byId('linkRows');\n" \
"    const text = (value) => typeof value === 'string' ? value : '';\n" \
"    const count = (value) => Number.isSafeInteger(value) && value >= 0 ? value : 0;\n" \
"    function setStatus(element, message) { element.textContent = message || ''; }\n" \
"    function headers() { return apiKey ? { 'X-API-Key': apiKey } : {}; }\n" \
"    function emptyRow(message) { const tr = document.createElement('tr'), td = document.createElement('td'); td.colSpan = 6; td.className = 'empty'; td.textContent = message; tr.append(td); rows.replaceChildren(tr); }\n" \
"    function cell(value, className) { const td = document.createElement('td'); if (className) td.className = className; td.textContent = value; return td; }\n" \
"    function flagsCell(link) { const td = document.createElement('td'); if (link.has_password) { const badge = document.createElement('span'); badge.className = 'badge'; badge.textContent = 'Password'; td.append(badge); } if (text(link.expires_at)) { const badge = document.createElement('span'); badge.className = 'badge exp'; badge.textContent = 'Expires ' + text(link.expires_at).slice(0, 10); td.append(badge); } if (!td.childNodes.length) td.textContent = '-'; return td; }\n" \
"    function slugCell(slug) { const td = document.createElement('td'), link = document.createElement('a'); link.href = '/' + encodeURIComponent(slug); link.target = '_blank'; link.rel = 'noopener noreferrer'; link.textContent = slug; td.append(link); return td; }\n" \
"    async function removeLink(slug, button) { if (!window.confirm('Delete /' + slug + '?')) return; button.disabled = true; try { const response = await fetch('/' + encodeURIComponent(slug), { method: 'DELETE', headers: headers() }); if (!response.ok) throw new Error('Delete failed'); await load(); } catch (error) { setStatus(dashboardStatus, 'Could not delete the link.'); button.disabled = false; } }\n" \
"    function render(links) { let totalVisits = 0, totalUnique = 0; byId('totalLinks').textContent = String(links.length); const fragment = document.createDocumentFragment(); for (const link of links) { const slug = text(link.slug); if (!slug) continue; const visits = count(link.total_visits), unique = count(link.unique_visitors); totalVisits += visits; totalUnique += unique; const tr = document.createElement('tr'); tr.append(slugCell(slug), cell(text(link.url), 'url'), cell(text(link.created_at).slice(0, 16) || '-'), flagsCell(link), cell(String(visits) + ' / ' + String(unique))); const action = document.createElement('td'), button = document.createElement('button'); button.type = 'button'; button.className = 'danger'; button.textContent = 'Delete'; button.addEventListener('click', () => removeLink(slug, button)); action.append(button); tr.append(action); fragment.append(tr); } byId('totalVisits').textContent = String(totalVisits); byId('totalUnique').textContent = String(totalUnique); if (fragment.childNodes.length) rows.replaceChildren(fragment); else emptyRow('No links yet'); }\n" \
"    async function load() { if (!apiKey) return; setStatus(dashboardStatus, 'Loading links...'); emptyRow('Loading...'); try { const response = await fetch('/api/admin/links', { headers: headers(), cache: 'no-store' }); if (response.status === 401) { apiKey = ''; byId('apiKey').value = ''; dashboard.hidden = true; authPanel.hidden = false; setStatus(authStatus, 'The API key was not accepted.'); return; } if (!response.ok) throw new Error('Load failed'); const links = await response.json(); if (!Array.isArray(links)) throw new Error('Invalid response'); render(links); setStatus(dashboardStatus, ''); } catch (error) { emptyRow('Could not load links.'); setStatus(dashboardStatus, 'Check the connection and try again.'); } }\n" \
"    byId('authForm').addEventListener('submit', (event) => { event.preventDefault(); apiKey = byId('apiKey').value; if (!apiKey) return; authPanel.hidden = true; dashboard.hidden = false; setStatus(authStatus, ''); load(); });\n" \
"    byId('refresh').addEventListener('click', load);\n" \
"    byId('lock').addEventListener('click', () => { apiKey = ''; byId('apiKey').value = ''; dashboard.hidden = true; authPanel.hidden = false; emptyRow(''); setStatus(authStatus, 'Dashboard locked.'); byId('apiKey').focus(); });\n" \
"    emptyRow('Unlock the dashboard to load links.');\n" \
"  })();\n" \
"  </script>\n" \
"</body>\n" \
"</html>"

#endif
