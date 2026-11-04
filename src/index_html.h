#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"description\" content=\"Ultra-fast, privacy-preserving URL shortener, native SVG QR code generator, and link analytics platform written in pure C.\">\n" \
"  <meta name=\"theme-color\" content=\"#3b82f6\">\n" \
"  <meta property=\"og:type\" content=\"website\">\n" \
"  <meta property=\"og:title\" content=\"c.micutu.com - High-Performance C URL Shortener\">\n" \
"  <meta property=\"og:description\" content=\"Shorten URLs with expiration, self-destruct click limits, PBKDF2 password protection, native C QR codes, and privacy-first analytics.\">\n" \
"  <meta property=\"og:url\" content=\"https://c.micutu.com/\">\n" \
"  <meta name=\"twitter:card\" content=\"summary\">\n" \
"  <meta name=\"twitter:title\" content=\"c.micutu.com - C URL Shortener\">\n" \
"  <meta name=\"twitter:description\" content=\"Fast, secure URL shortener and SVG QR generator built in pure C.\">\n" \
"  <link rel=\"canonical\" href=\"https://c.micutu.com/\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"manifest\" href=\"/manifest.json\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>c.micutu.com - Fast &amp; Secure C URL Shortener</title>\n" \
"  <script type=\"application/ld+json\">\n" \
"  {\n" \
"    \"@context\": \"https://schema.org\",\n" \
"    \"@type\": \"WebApplication\",\n" \
"    \"name\": \"c.micutu.com URL Shortener\",\n" \
"    \"url\": \"https://c.micutu.com/\",\n" \
"    \"applicationCategory\": \"UtilitiesApplication\",\n" \
"    \"operatingSystem\": \"All\",\n" \
"    \"description\": \"High-performance, privacy-preserving URL shortener and SVG QR generator written in C17.\"\n" \
"  }\n" \
"  </script>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">C17 ENGINE</span></a>\n" \
"    <nav class=\"nav-links\" aria-label=\"Primary\">\n" \
"      <button type=\"button\" class=\"btn btn-sm\" id=\"themeToggle\">Light mode</button>\n" \
"      <a class=\"btn btn-sm\" href=\"/admin\">Admin Dashboard</a>\n" \
"    </nav>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container\" aria-labelledby=\"pageTitle\">\n" \
"      <h1 id=\"pageTitle\" class=\"text-center\">High-Speed URL Shortener</h1>\n" \
"      <p class=\"subtitle text-center\">Zero bloat, PBKDF2 password locks, self-destruct links &amp; native C QR codes</p>\n" \
"      <div class=\"tabs\" role=\"tablist\">\n" \
"        <button type=\"button\" class=\"tab-btn active\" data-tab-target=\"shorten\">Shorten URL</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-tab-target=\"inspect\">Inspect Link</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-tab-target=\"api\">API &amp; C CLI</button>\n" \
"      </div>\n" \
"      <div data-tab-pane=\"shorten\">\n" \
"        <form id=\"shortenForm\" novalidate>\n" \
"          <div class=\"field\">\n" \
"            <label for=\"url\">Destination URL</label>\n" \
"            <input id=\"url\" type=\"url\" inputmode=\"url\" autocomplete=\"url\" placeholder=\"https://example.com/very-long-article-path\" required autofocus>\n" \
"          </div>\n" \
"          <div class=\"preset-bar\">\n" \
"            <button type=\"button\" class=\"preset-chip\" data-preset=\"burn\">1-Click Burn Link</button>\n" \
"            <button type=\"button\" class=\"preset-chip\" data-preset=\"temp1h\">Expire in 1h</button>\n" \
"            <button type=\"button\" class=\"preset-chip\" data-preset=\"temp24h\">Expire in 24h</button>\n" \
"            <button type=\"button\" class=\"preset-chip\" data-preset=\"clear\">Reset limits</button>\n" \
"          </div>\n" \
"          <button class=\"options-toggle\" id=\"optionsToggle\" type=\"button\" aria-expanded=\"false\" aria-controls=\"advanced\">More options</button>\n" \
"          <div class=\"advanced\" id=\"advanced\">\n" \
"            <div class=\"field\">\n" \
"              <label for=\"apiKey\">API Key or Scoped Creator Token</label>\n" \
"              <input id=\"apiKey\" type=\"password\" autocomplete=\"current-password\" placeholder=\"Admin key or csh_... creator token\">\n" \
"            </div>\n" \
"            <div class=\"row\">\n" \
"              <div class=\"field\"><label for=\"slug\">Custom slug</label><input id=\"slug\" type=\"text\" autocomplete=\"off\" autocapitalize=\"none\" placeholder=\"my-custom-slug\"></div>\n" \
"              <div class=\"field\"><label for=\"title\">Title / Note</label><input id=\"title\" type=\"text\" autocomplete=\"off\" placeholder=\"Release notes v2\"></div>\n" \
"            </div>\n" \
"            <div class=\"row\">\n" \
"              <div class=\"field\"><label for=\"tags\">Tags (comma-separated)</label><input id=\"tags\" type=\"text\" autocomplete=\"off\" placeholder=\"work,launch\"></div>\n" \
"              <div class=\"field\"><label for=\"password\">Link password</label><input id=\"password\" type=\"password\" autocomplete=\"new-password\" placeholder=\"Optional protection\"></div>\n" \
"            </div>\n" \
"            <div class=\"row-3\">\n" \
"              <div class=\"field\"><label for=\"ttl\">Expiry (hours)</label><input id=\"ttl\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"8760\" placeholder=\"Optional\"></div>\n" \
"              <div class=\"field\"><label for=\"maxClicks\">Max clicks</label><input id=\"maxClicks\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"1000000\" placeholder=\"Unlimited\"></div>\n" \
"              <div class=\"field\"><label for=\"activatesIn\">Delay (hours)</label><input id=\"activatesIn\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"8760\" placeholder=\"Immediate\"></div>\n" \
"            </div>\n" \
"          </div>\n" \
"          <button class=\"primary\" id=\"submitButton\" type=\"submit\">Shorten link</button>\n" \
"        </form>\n" \
"        <div id=\"result\" role=\"status\" aria-live=\"polite\"></div>\n" \
"        <div id=\"sessionHistory\" class=\"field\" hidden>\n" \
"          <h3>Created in this session</h3>\n" \
"          <div id=\"sessionList\"></div>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div data-tab-pane=\"inspect\" hidden>\n" \
"        <p class=\"hint\">Preview a short link's status, safety metadata, click limits, and destination before visiting.</p>\n" \
"        <form id=\"inspectForm\">\n" \
"          <div class=\"field\">\n" \
"            <label for=\"inspectInput\">Short URL or Slug</label>\n" \
"            <input id=\"inspectInput\" type=\"text\" placeholder=\"e.g. my-link or https://c.micutu.com/my-link\" required>\n" \
"          </div>\n" \
"          <button type=\"submit\" class=\"primary\">Inspect Link</button>\n" \
"        </form>\n" \
"        <div id=\"inspectResult\" class=\"panel\" hidden></div>\n" \
"      </div>\n" \
"      <div data-tab-pane=\"api\" hidden>\n" \
"        <p class=\"hint\">Use a Scoped Creator Token (<code>csh_...</code>) or Admin API Key in the <code>X-API-Key</code> header.</p>\n" \
"        <h3>Create a short link via curl</h3>\n" \
"        <pre><code>curl -X POST https://c.micutu.com/shorten \\\n" \
"  -H \"Content-Type: application/json\" \\\n" \
"  -H \"X-API-Key: csh_your_scoped_token\" \\\n" \
"  -d '{\"url\":\"https://example.com\",\"custom_slug\":\"demo\",\"max_clicks\":10,\"ttl_hours\":24}'</code></pre>\n" \
"        <h3>Download Native SVG QR Code</h3>\n" \
"        <pre><code>curl -sO https://c.micutu.com/qr/demo.svg</code></pre>\n" \
"        <h3>Inspect or Query JSON Analytics</h3>\n" \
"        <pre><code>curl -s https://c.micutu.com/api/inspect/demo\n" \
"curl -s https://c.micutu.com/api/stats/demo</code></pre>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"  <footer>\n" \
"    <a href=\"/health\">Service Health</a> &middot; <a href=\"/admin\">Admin</a> &middot; <a href=\"/sitemap.xml\">Sitemap</a> &middot; Built in C17 with libmicrohttpd &amp; SQLite\n" \
"  </footer>\n" \
"  <script src=\"/assets/index.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#endif
