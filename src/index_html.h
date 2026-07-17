#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"description\" content=\"Fast, secure URL shortener built in C.\">\n" \
"  <title>c.micutu.com - URL Shortener</title>\n" \
"  <style>\n" \
"    :root { color-scheme:dark; --bg:#0f172a; --surface:#1e293b; --text:#f8fafc; --muted:#94a3b8; --primary:#3b82f6; --primary-hover:#2563eb; --border:#334155; --error:#f87171; --success:#4ade80; }\n" \
"    * { box-sizing:border-box; } body { min-width:320px; min-height:100vh; margin:0; padding:1.25rem; display:flex; flex-direction:column; justify-content:center; align-items:center; background:radial-gradient(circle at top,#1e3a5f 0,var(--bg) 42rem); color:var(--text); font:16px/1.45 system-ui,-apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif; }\n" \
"    main { width:min(100%,500px); } .container { padding:clamp(1.25rem,5vw,2.25rem); border:1px solid var(--border); border-radius:16px; background:rgb(30 41 59 / .96); box-shadow:0 28px 70px rgb(0 0 0 / .35); } h1 { margin:0; text-align:center; font-size:clamp(1.45rem,5vw,1.85rem); } .subtitle { margin:.35rem 0 1.7rem; text-align:center; color:var(--muted); font-size:.9rem; }\n" \
"    label { display:block; margin:0 0 .4rem; color:var(--muted); font-size:.85rem; font-weight:650; } .field { margin-bottom:1rem; } input { width:100%; min-height:46px; padding:.7rem .85rem; border:1px solid var(--border); border-radius:9px; background:var(--bg); color:var(--text); font:inherit; } input::placeholder { color:#64748b; } input:focus { outline:2px solid var(--primary); outline-offset:2px; }\n" \
"    .options-toggle { width:auto; margin:0 0 1rem; padding:0; border:0; color:var(--muted); background:transparent; font-size:.86rem; } .options-toggle:hover { color:var(--primary); border-color:transparent; } .advanced { display:none; } .advanced.open { display:block; } .row { display:grid; grid-template-columns:1fr 1fr; gap:.75rem; }\n" \
"    button { min-height:44px; border:1px solid var(--border); border-radius:9px; padding:.65rem .9rem; color:var(--text); background:var(--surface); font:inherit; font-weight:650; cursor:pointer; } button:hover { border-color:var(--primary); } button:focus-visible { outline:2px solid var(--primary); outline-offset:2px; } button:disabled { cursor:wait; opacity:.65; } .primary { width:100%; margin-top:.25rem; border-color:var(--primary); background:var(--primary); color:white; } .primary:hover { background:var(--primary-hover); }\n" \
"    #result { display:none; margin-top:1.25rem; padding:1rem; border:1px solid var(--border); border-radius:10px; background:var(--bg); text-align:center; } #result.show { display:block; } .result-label { color:var(--muted); font-size:.82rem; } .result-link { display:block; margin:.45rem 0 .85rem; overflow-wrap:anywhere; color:#60a5fa; font-weight:700; text-decoration:none; } .result-link:hover { text-decoration:underline; } .result-actions { display:flex; gap:.6rem; } .result-actions button,.result-actions a { flex:1; display:inline-flex; align-items:center; justify-content:center; min-height:40px; border:1px solid var(--border); border-radius:8px; color:var(--text); background:var(--surface); text-decoration:none; font-size:.85rem; font-weight:650; } .result-actions button:hover,.result-actions a:hover { border-color:var(--primary); color:var(--primary); } .error { color:var(--error); } .success { color:var(--success); }\n" \
"    footer { margin-top:1rem; color:var(--muted); text-align:center; font-size:.78rem; } footer a { color:var(--muted); text-decoration:none; } footer a:hover { color:var(--primary); } @media (max-width:390px) { .row { grid-template-columns:1fr; gap:0; } .result-actions { flex-direction:column; } }\n" \
"  </style>\n" \
"</head>\n" \
"<body>\n" \
"  <main><section class=\"container\" aria-labelledby=\"pageTitle\">\n" \
"    <h1 id=\"pageTitle\">&#128279; URL Shortener</h1><p class=\"subtitle\">Fast and secure, written in C</p>\n" \
"    <form id=\"shortenForm\" novalidate>\n" \
"      <div class=\"field\"><label for=\"url\">Long URL</label><input id=\"url\" type=\"url\" inputmode=\"url\" autocomplete=\"url\" placeholder=\"https://example.com/very-long-url\" required autofocus></div>\n" \
"      <button class=\"options-toggle\" id=\"optionsToggle\" type=\"button\" aria-expanded=\"false\" aria-controls=\"advanced\">&#9654; More options</button>\n" \
"      <div class=\"advanced\" id=\"advanced\">\n" \
"        <div class=\"field\"><label for=\"apiKey\">API key</label><input id=\"apiKey\" type=\"password\" autocomplete=\"current-password\" placeholder=\"Required to create a link\"></div>\n" \
"        <div class=\"field\"><label for=\"slug\">Custom slug</label><input id=\"slug\" type=\"text\" autocomplete=\"off\" autocapitalize=\"none\" placeholder=\"my-link\"></div>\n" \
"        <div class=\"row\"><div class=\"field\"><label for=\"password\">Link password</label><input id=\"password\" type=\"password\" autocomplete=\"new-password\" placeholder=\"Optional\"></div><div class=\"field\"><label for=\"ttl\">Expiry (hours)</label><input id=\"ttl\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"8760\" placeholder=\"Optional\"></div></div>\n" \
"      </div>\n" \
"      <button class=\"primary\" id=\"submitButton\" type=\"submit\">Shorten link</button>\n" \
"    </form>\n" \
"    <div id=\"result\" role=\"status\" aria-live=\"polite\"></div>\n" \
"  </section><footer><a href=\"/health\">Service status</a> &middot; <a href=\"/admin\">Admin</a></footer></main>\n" \
"  <script>\n" \
"  (() => {\n" \
"    'use strict';\n" \
"    const byId = (id) => document.getElementById(id);\n" \
"    const result = byId('result'), form = byId('shortenForm'), button = byId('submitButton'), options = byId('advanced'), toggle = byId('optionsToggle');\n" \
"    function showMessage(message, className) { result.replaceChildren(); const node = document.createElement('p'); node.className = className || ''; node.textContent = message; result.append(node); result.classList.add('show'); }\n" \
"    function showLink(shortUrl) { result.replaceChildren(); const label = document.createElement('div'); label.className = 'result-label'; label.textContent = 'Your short link'; const link = document.createElement('a'); link.className = 'result-link'; link.href = shortUrl; link.target = '_blank'; link.rel = 'noopener noreferrer'; link.textContent = shortUrl; const actions = document.createElement('div'); actions.className = 'result-actions'; const copy = document.createElement('button'); copy.type = 'button'; copy.textContent = 'Copy URL'; copy.addEventListener('click', async () => { try { await navigator.clipboard.writeText(shortUrl); copy.textContent = 'Copied'; window.setTimeout(() => { copy.textContent = 'Copy URL'; }, 1500); } catch (error) { copy.textContent = 'Copy unavailable'; } }); actions.append(copy); try { const parsed = new URL(shortUrl); const segments = parsed.pathname.split('/').filter(Boolean); const slug = segments[segments.length - 1]; if (slug) { const stats = document.createElement('a'); stats.href = '/stats/' + encodeURIComponent(slug); stats.target = '_blank'; stats.rel = 'noopener noreferrer'; stats.textContent = 'View stats'; actions.append(stats); } } catch (error) {} result.append(label, link, actions); result.classList.add('show'); }\n" \
"    toggle.addEventListener('click', () => { const open = options.classList.toggle('open'); toggle.setAttribute('aria-expanded', String(open)); toggle.textContent = open ? '\u25be Hide options' : '\u25b8 More options'; });\n" \
"    form.addEventListener('submit', async (event) => { event.preventDefault(); const url = byId('url').value.trim(); const apiKey = byId('apiKey').value; const slug = byId('slug').value.trim(); const password = byId('password').value; const ttlText = byId('ttl').value; if (!url) { showMessage('Enter a URL first.', 'error'); byId('url').focus(); return; } if (!apiKey) { options.classList.add('open'); toggle.setAttribute('aria-expanded', 'true'); toggle.textContent = '\u25be Hide options'; showMessage('An API key is required to create a link.', 'error'); byId('apiKey').focus(); return; } const payload = { url }; if (slug) payload.custom_slug = slug; if (password) payload.password = password; if (ttlText) payload.ttl_hours = Number(ttlText); button.disabled = true; button.textContent = 'Shortening...'; try { const response = await fetch('/shorten', { method:'POST', headers:{ 'Content-Type':'application/json', 'X-API-Key':apiKey }, body:JSON.stringify(payload), cache:'no-store' }); let data = null; try { data = await response.json(); } catch (error) {} if (!response.ok || !data || typeof data.short_url !== 'string') { showMessage(data && typeof data.error === 'string' ? data.error : 'Could not shorten this URL.', 'error'); return; } showLink(data.short_url); byId('url').value = ''; byId('slug').value = ''; byId('password').value = ''; byId('ttl').value = ''; } catch (error) { showMessage('Network error. Please try again.', 'error'); } finally { button.disabled = false; button.textContent = 'Shorten link'; } });\n" \
"  })();\n" \
"  </script>\n" \
"</body>\n" \
"</html>"

#endif
