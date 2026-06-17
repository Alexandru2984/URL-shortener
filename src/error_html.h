#ifndef ERROR_HTML_H
#define ERROR_HTML_H

#define ERROR_HTML_TEMPLATE \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"    <meta charset=\"UTF-8\">\n" \
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
"    <title>%s</title>\n" \
"    <style>\n" \
"        :root { --bg: #0f172a; --surface: #1e293b; --text: #f8fafc; --primary: #3b82f6; --border: #334155; --dim: #94a3b8; }\n" \
"        * { box-sizing: border-box; margin: 0; padding: 0; }\n" \
"        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: var(--bg); color: var(--text); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }\n" \
"        .container { background: var(--surface); padding: 2.5rem; border-radius: 12px; width: 100%%; max-width: 450px; border: 1px solid var(--border); text-align: center; }\n" \
"        .code { font-size: 4rem; font-weight: 700; color: var(--primary); margin-bottom: 0.5rem; }\n" \
"        .title { font-size: 1.25rem; font-weight: 600; margin-bottom: 1rem; }\n" \
"        .desc { color: var(--dim); font-size: 0.9rem; margin-bottom: 1.5rem; line-height: 1.5; }\n" \
"        a { color: var(--primary); text-decoration: none; }\n" \
"        a:hover { text-decoration: underline; }\n" \
"    </style>\n" \
"</head>\n" \
"<body>\n" \
"    <div class=\"container\">\n" \
"        <div class=\"code\">%d</div>\n" \
"        <div class=\"title\">%s</div>\n" \
"        <div class=\"desc\">%s</div>\n" \
"        <a href=\"/\">&larr; Back to Shortener</a>\n" \
"    </div>\n" \
"</body>\n" \
"</html>"

#endif
