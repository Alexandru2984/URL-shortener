#ifndef STATS_HTML_H
#define STATS_HTML_H

#define STATS_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>Analytics for /%s - c.micutu.com</title>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">ANALYTICS</span></a>\n" \
"    <nav class=\"nav-links\">\n" \
"      <a class=\"btn btn-sm\" href=\"/inspect/%s\">Inspect Link</a>\n" \
"      <a class=\"btn btn-sm\" href=\"/qr/%s.svg\" target=\"_blank\">QR SVG</a>\n" \
"    </nav>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container\">\n" \
"      <div class=\"result-grid\">\n" \
"        <div>\n" \
"          <h1>Analytics for /%s</h1>\n" \
"          <p class=\"subtitle\">Privacy-preserving aggregate metrics (HMAC visitor identifiers)</p>\n" \
"          <div id=\"statsMeta\" hidden></div>\n" \
"        </div>\n" \
"        <div class=\"qr-preview\">\n" \
"          <img src=\"/qr/%s.svg\" alt=\"QR Code\">\n" \
"        </div>\n" \
"      </div>\n" \
"      <div class=\"stat-grid\">\n" \
"        <div class=\"stat-box\">\n" \
"          <div class=\"stat-value\">%d</div>\n" \
"          <div class=\"stat-label\">Total Visits</div>\n" \
"        </div>\n" \
"        <div class=\"stat-box\">\n" \
"          <div class=\"stat-value\">%d</div>\n" \
"          <div class=\"stat-label\">Unique Visitors</div>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div class=\"row\">\n" \
"        <div class=\"panel\">\n" \
"          <h2>Device Categories</h2>\n" \
"          <div id=\"deviceList\"><p class=\"hint\">No device data yet.</p></div>\n" \
"        </div>\n" \
"        <div class=\"panel\">\n" \
"          <h2>Top Referrer Hosts</h2>\n" \
"          <div id=\"referrerList\"><p class=\"hint\">No referrer data yet.</p></div>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div class=\"panel\">\n" \
"        <h2>Daily Activity (Last 14 Days)</h2>\n" \
"        <div id=\"dailyList\"><p class=\"hint\">No recent daily visits recorded.</p></div>\n" \
"      </div>\n" \
"      <div class=\"panel\">\n" \
"        <h2>24-Hour Traffic Distribution (UTC)</h2>\n" \
"        <div id=\"hourlyList\"><p class=\"hint\">No hourly activity recorded yet.</p></div>\n" \
"      </div>\n" \
"      <div class=\"actions\">\n" \
"        <a class=\"btn\" href=\"/\">&larr; Back to Shortener</a>\n" \
"        <a class=\"btn\" href=\"/api/stats/%s\" target=\"_blank\">View Raw JSON</a>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"  <script src=\"/assets/stats.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#endif
