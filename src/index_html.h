#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>C URL Shortener</title>\n" \
"    <style>\n" \
"        :root {\n" \
"            --bg: #0f172a;\n" \
"            --surface: #1e293b;\n" \
"            --text: #f8fafc;\n" \
"            --primary: #3b82f6;\n" \
"            --primary-hover: #2563eb;\n" \
"            --border: #334155;\n" \
"            --error: #ef4444;\n" \
"        }\n" \
"        * { box-sizing: border-box; margin: 0; padding: 0; }\n" \
"        body {\n" \
"            font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif;\n" \
"            background-color: var(--bg);\n" \
"            color: var(--text);\n" \
"            display: flex;\n" \
"            justify-content: center;\n" \
"            align-items: center;\n" \
"            min-height: 100vh;\n" \
"            padding: 20px;\n" \
"        }\n" \
"        .container {\n" \
"            background: var(--surface);\n" \
"            padding: 2rem;\n" \
"            border-radius: 12px;\n" \
"            box-shadow: 0 10px 15px -3px rgb(0 0 0 / 0.5);\n" \
"            width: 100%;\n" \
"            max-width: 450px;\n" \
"            border: 1px solid var(--border);\n" \
"        }\n" \
"        h1 { text-align: center; margin-bottom: 1.5rem; font-size: 1.5rem; font-weight: 600; }\n" \
"        .form-group { margin-bottom: 1rem; }\n" \
"        .row { display: flex; gap: 1rem; }\n" \
"        .col { flex: 1; }\n" \
"        label { display: block; margin-bottom: 0.5rem; font-size: 0.875rem; color: #cbd5e1; }\n" \
"        input {\n" \
"            width: 100%;\n" \
"            padding: 0.75rem 1rem;\n" \
"            background: var(--bg);\n" \
"            border: 1px solid var(--border);\n" \
"            border-radius: 8px;\n" \
"            color: var(--text);\n" \
"            font-size: 1rem;\n" \
"            outline: none;\n" \
"            transition: border-color 0.2s;\n" \
"        }\n" \
"        input:focus { border-color: var(--primary); }\n" \
"        button {\n" \
"            width: 100%;\n" \
"            padding: 0.75rem;\n" \
"            background: var(--primary);\n" \
"            color: white;\n" \
"            border: none;\n" \
"            border-radius: 8px;\n" \
"            font-size: 1rem;\n" \
"            font-weight: 500;\n" \
"            cursor: pointer;\n" \
"            transition: background-color 0.2s;\n" \
"            margin-top: 10px;\n" \
"        }\n" \
"        button:hover { background: var(--primary-hover); }\n" \
"        #result {\n" \
"            margin-top: 1.5rem;\n" \
"            padding: 1rem;\n" \
"            border-radius: 8px;\n" \
"            background: var(--bg);\n" \
"            border: 1px solid var(--border);\n" \
"            display: none;\n" \
"            word-break: break-all;\n" \
"            text-align: center;\n" \
"        }\n" \
"        #result a { color: var(--primary); text-decoration: none; font-weight: 500; display: block; margin-bottom: 10px; }\n" \
"        #result a:hover { text-decoration: underline; }\n" \
"        .copy-btn {\n" \
"            background: var(--surface);\n" \
"            border: 1px solid var(--border);\n" \
"            font-size: 0.875rem;\n" \
"            padding: 0.5rem;\n" \
"        }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"container\">\n" \
"        <h1>URL Shortener</h1>\n" \
"        <div class=\"form-group\">\n" \
"            <label for=\"url\">Long URL</label>\n" \
"            <input type=\"url\" id=\"url\" placeholder=\"https://example.com/very-long-link\" required>\n" \
"        </div>\n" \
"        <div class=\"form-group\">\n" \
"            <label for=\"slug\">Custom Slug (Optional)</label>\n" \
"            <input type=\"text\" id=\"slug\" placeholder=\"my-link\">\n" \
"        </div>\n" \
"        <div class=\"row\">\n" \
"            <div class=\"form-group col\">\n" \
"                <label for=\"pwd\">Password (Opt)</label>\n" \
"                <input type=\"password\" id=\"pwd\" placeholder=\"secret\">\n" \
"            </div>\n" \
"            <div class=\"form-group col\">\n" \
"                <label for=\"ttl\">Expire in (Hours, Opt)</label>\n" \
"                <input type=\"number\" id=\"ttl\" placeholder=\"24\" min=\"1\">\n" \
"            </div>\n" \
"        </div>\n" \
"        <button id=\"submitBtn\">Shorten Link</button>\n" \
"        <div id=\"result\"></div>\n" \
"    </div>\n" \
"    <script>\n" \
"        document.getElementById('submitBtn').addEventListener('click', async () => {\n" \
"            const url = document.getElementById('url').value;\n" \
"            const slug = document.getElementById('slug').value;\n" \
"            const pwd = document.getElementById('pwd').value;\n" \
"            const ttl = parseInt(document.getElementById('ttl').value) || 0;\n" \
"            const resDiv = document.getElementById('result');\n" \
"            const btn = document.getElementById('submitBtn');\n" \
"            if(!url) {\n" \
"                resDiv.style.display = 'block';\n" \
"                resDiv.innerHTML = '<span style=\"color: var(--error)\">Please enter a URL</span>';\n" \
"                return;\n" \
"            }\n" \
"            btn.innerText = 'Shortening...';\n" \
"            btn.disabled = true;\n" \
"            try {\n" \
"                const response = await fetch('/shorten', {\n" \
"                    method: 'POST',\n" \
"                    headers: { 'Content-Type': 'application/json' },\n" \
"                    body: JSON.stringify({ url: url, custom_slug: slug, password: pwd, ttl_hours: ttl })\n" \
"                });\n" \
"                const data = await response.json();\n" \
"                resDiv.style.display = 'block';\n" \
"                if(response.ok) {\n" \
"                    resDiv.innerHTML = `\n" \
"                        <div style=\"margin-bottom: 8px; color: #94a3b8; font-size: 0.875rem;\">Your short link:</div>\n" \
"                        <a href=\"${data.short_url}\" target=\"_blank\">${data.short_url}</a>\n" \
"                        <img src=\"https://api.qrserver.com/v1/create-qr-code/?size=150x150&data=${encodeURIComponent(data.short_url)}\" style=\"margin-bottom: 10px; border-radius: 8px;\" />\n" \
"                        <button class=\"copy-btn\" onclick=\"navigator.clipboard.writeText('${data.short_url}'); this.innerText='Copied!'; setTimeout(()=>this.innerText='Copy to clipboard', 2000);\">Copy to clipboard</button>\n" \
"                    `;\n" \
"                } else {\n" \
"                    resDiv.innerHTML = `<span style=\"color: var(--error)\">Error: ${data.error}</span>`;\n" \
"                }\n" \
"            } catch(e) {\n" \
"                resDiv.style.display = 'block';\n" \
"                resDiv.innerHTML = `<span style=\"color: var(--error)\">Network error occurred.</span>`;\n" \
"            } finally {\n" \
"                btn.innerText = 'Shorten Link';\n" \
"                btn.disabled = false;\n" \
"            }\n" \
"        });\n" \
"    </script>\n" \
"</body>\n" \
"</html>"

#endif
