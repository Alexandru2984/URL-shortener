#ifndef INSPECT_HTML_H
#define INSPECT_HTML_H

#define INSPECT_HTML_TEMPLATE \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>Inspect /%s - c.micutu.com</title>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">LINK INSPECTOR</span></a>\n" \
"    <nav class=\"nav-links\">\n" \
"      <a class=\"btn btn-sm\" href=\"/stats/%s\">View Analytics</a>\n" \
"    </nav>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container\">\n" \
"      <div class=\"result-grid\">\n" \
"        <div>\n" \
"          <h1>Link Preview: /%s</h1>\n" \
"          <p class=\"subtitle\">Verify destination and security properties before visiting.</p>\n" \
"        </div>\n" \
"        <div class=\"qr-preview\">\n" \
"          <img src=\"/qr/%s.svg\" alt=\"QR Code\">\n" \
"        </div>\n" \
"      </div>\n" \
"      <div class=\"panel\">\n" \
"        <div class=\"list-row\"><span class=\"hint\">Status</span><strong>%s</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Title / Note</span><strong>%s</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Destination</span><strong>%s</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Password Protected</span><strong>%s</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Total Visits</span><strong>%d</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Created (UTC)</span><strong>%s</strong></div>\n" \
"        <div class=\"list-row\"><span class=\"hint\">Expires (UTC)</span><strong>%s</strong></div>\n" \
"      </div>\n" \
"      <div class=\"actions\">\n" \
"        <a class=\"btn primary-inline\" href=\"/%s\" rel=\"noopener noreferrer\">Proceed to /%s &rarr;</a>\n" \
"        <a class=\"btn\" href=\"/stats/%s\">Analytics</a>\n" \
"        <a class=\"btn\" href=\"/\">Back Home</a>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"</body>\n" \
"</html>"

#endif
