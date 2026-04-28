#ifndef PASSWORD_HTML_H
#define PASSWORD_HTML_H

#define PASSWORD_HTML \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>Protected Link</title>\n" \
"    <style>\n" \
"        :root { --bg: #0f172a; --surface: #1e293b; --text: #f8fafc; --primary: #3b82f6; --primary-hover: #2563eb; --border: #334155; --error: #ef4444; }\n" \
"        * { box-sizing: border-box; margin: 0; padding: 0; }\n" \
"        body { font-family: sans-serif; background: var(--bg); color: var(--text); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }\n" \
"        .container { background: var(--surface); padding: 2rem; border-radius: 12px; width: 100%; max-width: 400px; border: 1px solid var(--border); text-align: center; }\n" \
"        h2 { margin-bottom: 1rem; }\n" \
"        input { width: 100%; padding: 0.75rem; margin-bottom: 1rem; background: var(--bg); border: 1px solid var(--border); border-radius: 8px; color: var(--text); outline: none; }\n" \
"        button { width: 100%; padding: 0.75rem; background: var(--primary); color: white; border: none; border-radius: 8px; cursor: pointer; }\n" \
"        button:hover { background: var(--primary-hover); }\n" \
"        #err { color: var(--error); margin-top: 1rem; }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"container\">\n" \
"        <h2>Protected Link</h2>\n" \
"        <p style=\"margin-bottom: 1rem; color: #94a3b8;\">This link requires a password.</p>\n" \
"        <input type=\"password\" id=\"pwd\" placeholder=\"Enter password\" />\n" \
"        <button id=\"btn\">Unlock</button>\n" \
"        <div id=\"err\"></div>\n" \
"    </div>\n" \
"    <script>\n" \
"        document.getElementById('btn').addEventListener('click', async () => {\n" \
"            const pwd = document.getElementById('pwd').value;\n" \
"            const slug = window.location.pathname.substring(1);\n" \
"            try {\n" \
"                const res = await fetch('/unlock/' + slug, {\n" \
"                    method: 'POST',\n" \
"                    headers: { 'Content-Type': 'application/json' },\n" \
"                    body: JSON.stringify({ password: pwd })\n" \
"                });\n" \
"                const data = await res.json();\n" \
"                if(res.ok) { window.location.href = data.url; }\n" \
"                else { document.getElementById('err').innerText = data.error; }\n" \
"            } catch(e) { document.getElementById('err').innerText = 'Network error'; }\n" \
"        });\n" \
"    </script>\n" \
"</body>\n" \
"</html>"

#endif
