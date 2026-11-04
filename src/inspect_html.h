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
"        <a class=\"btn primary-inline\" href=\"/%s?direct=1\" rel=\"noopener noreferrer\">Proceed to /%s &rarr;</a>\n" \
"        <a class=\"btn\" href=\"/stats/%s\">Analytics</a>\n" \
"        <a class=\"btn\" href=\"/\">Back Home</a>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"</body>\n" \
"</html>"

#define SOCIAL_PREVIEW_HTML_TEMPLATE \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta property=\"og:type\" content=\"website\">\n" \
"  <meta property=\"og:title\" content=\"%s - c.micutu.com\">\n" \
"  <meta property=\"og:description\" content=\"Verified short link pointing to %s (%d visits).\">\n" \
"  <meta property=\"og:image\" content=\"%s/qr/%s.svg\">\n" \
"  <meta property=\"og:url\" content=\"%s/p/%s\">\n" \
"  <meta name=\"twitter:card\" content=\"summary\">\n" \
"  <meta name=\"twitter:title\" content=\"%s - c.micutu.com\">\n" \
"  <meta name=\"twitter:description\" content=\"Verified short link pointing to %s.\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>%s - Link Card</title>\n" \
"</head>\n" \
"<body>\n" \
"  <main class=\"narrow centered\">\n" \
"    <section class=\"container text-center\">\n" \
"      <span class=\"badge ok\">Verified Short Link</span>\n" \
"      <h1>%s</h1>\n" \
"      <p class=\"subtitle\">Destination host: <strong>%s</strong></p>\n" \
"      <div class=\"qr-preview\"><img src=\"/qr/%s.svg\" alt=\"QR Code\"></div>\n" \
"      <div class=\"field\">\n" \
"        <a class=\"btn primary\" href=\"/%s?direct=1\" rel=\"noopener noreferrer\">Continue to Destination &rarr;</a>\n" \
"      </div>\n" \
"      <div class=\"actions\">\n" \
"        <a class=\"btn btn-sm\" href=\"/inspect/%s\">Full Security Inspect</a>\n" \
"        <a class=\"btn btn-sm\" href=\"/stats/%s\">View Analytics (%d)</a>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"</body>\n" \
"</html>"

#define BIO_HTML_HEADER \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta property=\"og:type\" content=\"profile\">\n" \
"  <meta property=\"og:title\" content=\"@%s - Curated Links on c.micutu.com\">\n" \
"  <meta property=\"og:description\" content=\"Public link collection for @%s, powered by C17.\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>@%s - Bio Links</title>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">BIO HUB</span></a>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container text-center\">\n" \
"      <h1>@%s</h1>\n" \
"      <p class=\"subtitle\">Public Link Collection</p>\n"

#define BIO_HTML_FOOTER \
"      <p class=\"hint\"><a href=\"/\">Powered by c.micutu.com C17 Engine</a></p>\n" \
"    </section>\n" \
"  </main>\n" \
"</body>\n" \
"</html>"

#endif
