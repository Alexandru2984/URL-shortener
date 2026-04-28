#ifndef STATS_HTML_H
#define STATS_HTML_H

#define STATS_HTML \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>Link Analytics</title>\n" \
"    <style>\n" \
"        :root { --bg: #0f172a; --surface: #1e293b; --text: #f8fafc; --primary: #3b82f6; --border: #334155; }\n" \
"        * { box-sizing: border-box; margin: 0; padding: 0; }\n" \
"        body { font-family: sans-serif; background: var(--bg); color: var(--text); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }\n" \
"        .container { background: var(--surface); padding: 2rem; border-radius: 12px; width: 100%%; max-width: 450px; border: 1px solid var(--border); text-align: center; }\n" \
"        h1 { margin-bottom: 1.5rem; font-size: 1.5rem; font-weight: 600; }\n" \
"        .stat-box { background: var(--bg); border: 1px solid var(--border); padding: 1.5rem; border-radius: 8px; margin-bottom: 1rem; }\n" \
"        .stat-value { font-size: 2.5rem; font-weight: bold; color: var(--primary); margin-bottom: 0.5rem; }\n" \
"        .stat-label { color: #94a3b8; font-size: 0.875rem; text-transform: uppercase; letter-spacing: 0.05em; }\n" \
"        a { color: var(--primary); text-decoration: none; margin-top: 1rem; display: inline-block; }\n" \
"        a:hover { text-decoration: underline; }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"container\">\n" \
"        <h1>Analytics for /%s</h1>\n" \
"        <div class=\"stat-box\">\n" \
"            <div class=\"stat-value\">%d</div>\n" \
"            <div class=\"stat-label\">Total Visits</div>\n" \
"        </div>\n" \
"        <div class=\"stat-box\">\n" \
"            <div class=\"stat-value\">%d</div>\n" \
"            <div class=\"stat-label\">Unique Visitors</div>\n" \
"        </div>\n" \
"        <a href=\"/\">&larr; Back to Shortener</a>\n" \
"    </div>\n" \
"</body>\n" \
"</html>"

#endif
