#ifndef ADMIN_HTML_H
#define ADMIN_HTML_H

#define ADMIN_HTML \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>Admin Dashboard</title>\n" \
"    <style>\n" \
"        :root { --bg: #0f172a; --surface: #1e293b; --text: #f8fafc; --primary: #3b82f6; --primary-hover: #2563eb; --border: #334155; --dim: #94a3b8; --error: #ef4444; --success: #22c55e; }\n" \
"        * { box-sizing: border-box; margin: 0; padding: 0; }\n" \
"        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: var(--bg); color: var(--text); padding: 2rem; }\n" \
"        .header { max-width: 1000px; margin: 0 auto 2rem; display: flex; justify-content: space-between; align-items: center; }\n" \
"        h1 { font-size: 1.5rem; font-weight: 600; }\n" \
"        .stats-bar { display: flex; gap: 1.5rem; }\n" \
"        .stat { background: var(--surface); border: 1px solid var(--border); border-radius: 8px; padding: 0.75rem 1.25rem; text-align: center; }\n" \
"        .stat-val { font-size: 1.5rem; font-weight: 700; color: var(--primary); }\n" \
"        .stat-lbl { font-size: 0.75rem; color: var(--dim); text-transform: uppercase; letter-spacing: 0.05em; }\n" \
"        .table-wrap { max-width: 1000px; margin: 0 auto; background: var(--surface); border: 1px solid var(--border); border-radius: 12px; overflow: hidden; }\n" \
"        table { width: 100%%; border-collapse: collapse; }\n" \
"        th { background: var(--bg); padding: 0.75rem 1rem; text-align: left; font-size: 0.8rem; color: var(--dim); text-transform: uppercase; letter-spacing: 0.05em; }\n" \
"        td { padding: 0.75rem 1rem; border-top: 1px solid var(--border); font-size: 0.875rem; vertical-align: middle; }\n" \
"        .slug { font-weight: 600; color: var(--primary); }\n" \
"        .url { max-width: 250px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; color: var(--dim); }\n" \
"        .badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 0.7rem; font-weight: 600; }\n" \
"        .badge-pwd { background: rgba(234,179,8,0.15); color: #eab308; }\n" \
"        .badge-exp { background: rgba(239,68,68,0.15); color: var(--error); }\n" \
"        .del-btn { background: var(--error); color: white; border: none; padding: 4px 12px; border-radius: 6px; cursor: pointer; font-size: 0.8rem; }\n" \
"        .del-btn:hover { opacity: 0.85; }\n" \
"        .del-btn:disabled { opacity: 0.4; cursor: default; }\n" \
"        .empty { text-align: center; padding: 3rem; color: var(--dim); }\n" \
"        a { color: var(--primary); text-decoration: none; }\n" \
"        a:hover { text-decoration: underline; }\n" \
"        @media (max-width: 700px) { .header { flex-direction: column; gap: 1rem; } .stats-bar { flex-wrap: wrap; } td, th { padding: 0.5rem; font-size: 0.75rem; } }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"header\">\n" \
"        <h1><a href=\"/\">&#8592;</a> &nbsp;Admin Dashboard</h1>\n" \
"        <div class=\"stats-bar\">\n" \
"            <div class=\"stat\"><div class=\"stat-val\" id=\"totalLinks\">-</div><div class=\"stat-lbl\">Links</div></div>\n" \
"            <div class=\"stat\"><div class=\"stat-val\" id=\"totalVisits\">-</div><div class=\"stat-lbl\">Total Visits</div></div>\n" \
"            <div class=\"stat\"><div class=\"stat-val\" id=\"totalUnique\">-</div><div class=\"stat-lbl\">Unique Visitors</div></div>\n" \
"        </div>\n" \
"    </div>\n" \
"    <div class=\"table-wrap\">\n" \
"        <table>\n" \
"            <thead><tr><th>Slug</th><th>Target URL</th><th>Created</th><th>Flags</th><th>Visits</th><th></th></tr></thead>\n" \
"            <tbody id=\"tbody\"><tr><td colspan=\"6\" class=\"empty\">Loading...</td></tr></tbody>\n" \
"        </table>\n" \
"    </div>\n" \
"    <script>\n" \
"        const KEY = new URLSearchParams(window.location.search).get('key') || '';\n" \
"        const headers = KEY ? {'X-API-Key': KEY} : {};\n" \
"        async function load() {\n" \
"            try {\n" \
"                const res = await fetch('/api/admin/links', {headers});\n" \
"                if (!res.ok) { document.getElementById('tbody').innerHTML = '<tr><td colspan=\"6\" class=\"empty\">Unauthorized</td></tr>'; return; }\n" \
"                const links = await res.json();\n" \
"                let totalV = 0, totalU = 0;\n" \
"                links.forEach(l => { totalV += l.total_visits; totalU += l.unique_visitors; });\n" \
"                document.getElementById('totalLinks').textContent = links.length;\n" \
"                document.getElementById('totalVisits').textContent = totalV;\n" \
"                document.getElementById('totalUnique').textContent = totalU;\n" \
"                if (links.length === 0) { document.getElementById('tbody').innerHTML = '<tr><td colspan=\"6\" class=\"empty\">No links yet</td></tr>'; return; }\n" \
"                document.getElementById('tbody').innerHTML = links.map(l => {\n" \
"                    let flags = '';\n" \
"                    if (l.has_password) flags += '<span class=\"badge badge-pwd\">PWD</span> ';\n" \
"                    if (l.expires_at) flags += '<span class=\"badge badge-exp\">EXP ' + l.expires_at.slice(0,10) + '</span>';\n" \
"                    return `<tr><td class=\"slug\"><a href=\"/${l.slug}\" target=\"_blank\">${l.slug}</a></td><td class=\"url\" title=\"${l.url}\">${l.url}</td><td>${l.created_at ? l.created_at.slice(0,16) : '-'}</td><td>${flags || '-'}</td><td>${l.total_visits} / ${l.unique_visitors}</td><td><button class=\"del-btn\" onclick=\"del('${l.slug}',this)\">Delete</button></td></tr>`;\n" \
"                }).join('');\n" \
"            } catch(e) { document.getElementById('tbody').innerHTML = '<tr><td colspan=\"6\" class=\"empty\">Error loading data</td></tr>'; }\n" \
"        }\n" \
"        async function del(slug, btn) {\n" \
"            if (!confirm('Delete /' + slug + '?')) return;\n" \
"            btn.disabled = true;\n" \
"            try {\n" \
"                const res = await fetch('/' + slug, {method: 'DELETE', headers});\n" \
"                if (res.ok) { btn.closest('tr').remove(); load(); }\n" \
"                else { const d = await res.json(); alert(d.error || 'Failed'); btn.disabled = false; }\n" \
"            } catch(e) { alert('Network error'); btn.disabled = false; }\n" \
"        }\n" \
"        load();\n" \
"    </script>\n" \
"</body>\n" \
"</html>"

#endif
