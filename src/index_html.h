#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"description\" content=\"Ultra-fast, privacy-preserving URL shortener, native SVG QR code generator, smart device routing, and link analytics platform written in pure C.\">\n" \
"  <meta name=\"theme-color\" content=\"#3b82f6\">\n" \
"  <meta property=\"og:type\" content=\"website\">\n" \
"  <meta property=\"og:title\" content=\"c.micutu.com - High-Performance C URL Shortener\">\n" \
"  <meta property=\"og:description\" content=\"Shorten URLs with expiration, self-destruct click limits, PBKDF2 password protection, native C QR codes, smart mobile routing, and privacy-first analytics.\">\n" \
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
"      <a class=\"btn btn-sm\" href=\"/docs\">API Playground</a>\n" \
"      <a class=\"btn btn-sm\" href=\"/admin\">Admin Dashboard</a>\n" \
"    </nav>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container\" aria-labelledby=\"pageTitle\">\n" \
"      <h1 id=\"pageTitle\" class=\"text-center\">High-Speed URL Shortener</h1>\n" \
"      <p class=\"subtitle text-center\">Zero bloat, PBKDF2 password locks, self-destruct links, smart device routing &amp; native C QR codes</p>\n" \
"      <div class=\"tabs\" role=\"tablist\">\n" \
"        <button type=\"button\" class=\"tab-btn active\" data-tab-target=\"shorten\">Shorten URL</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-tab-target=\"bulk\">Bulk Shorten</button>\n" \
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
"              <div class=\"field\"><label for=\"tags\">Tags (comma-separated, creates /bio/tag)</label><input id=\"tags\" type=\"text\" autocomplete=\"off\" placeholder=\"work,launch\"></div>\n" \
"              <div class=\"field\"><label for=\"password\">Link password</label><input id=\"password\" type=\"password\" autocomplete=\"new-password\" placeholder=\"Optional protection\"></div>\n" \
"            </div>\n" \
"            <div class=\"row\">\n" \
"              <div class=\"field\">\n" \
"                <label for=\"mobileUrl\">Mobile Deep-Link / Smart Target URL (optional)</label>\n" \
"                <input id=\"mobileUrl\" type=\"url\" autocomplete=\"off\" placeholder=\"https://m.example.com/app-landing\">\n" \
"              </div>\n" \
"              <div class=\"field\">\n" \
"                <label for=\"redirectCode\">HTTP Redirect Status</label>\n" \
"                <select id=\"redirectCode\">\n" \
"                  <option value=\"302\">302 Found (Temporary, Default)</option>\n" \
"                  <option value=\"301\">301 Moved Permanently (SEO)</option>\n" \
"                  <option value=\"307\">307 Temporary Redirect (Method Preserving)</option>\n" \
"                  <option value=\"308\">308 Permanent Redirect (Method Preserving)</option>\n" \
"                </select>\n" \
"              </div>\n" \
"            </div>\n" \
"            <div class=\"row\">\n" \
"              <div class=\"field\"><label for=\"altUrl\">A/B Split Variant B URL (optional)</label><input id=\"altUrl\" type=\"url\" autocomplete=\"off\" placeholder=\"https://example.com/landing-v2\"></div>\n" \
"              <div class=\"field\"><label for=\"altWeight\">Variant B Traffic % (1-99)</label><input id=\"altWeight\" type=\"number\" min=\"1\" max=\"99\" placeholder=\"50\"></div>\n" \
"            </div>\n" \
"            <div class=\"row\">\n" \
"              <div class=\"field\"><label for=\"geoCountry\">Geo-Target Country ISO (e.g. RO, DE, US)</label><input id=\"geoCountry\" type=\"text\" maxlength=\"2\" autocomplete=\"off\" placeholder=\"RO\"></div>\n" \
"              <div class=\"field\"><label for=\"geoUrl\">Geo-Target Destination URL (optional)</label><input id=\"geoUrl\" type=\"url\" autocomplete=\"off\" placeholder=\"https://ro.example.com/oferta\"></div>\n" \
"            </div>\n" \
"            <div class=\"row-3\">\n" \
"              <div class=\"field\"><label for=\"utmSource\">UTM Source</label><input id=\"utmSource\" type=\"text\" autocomplete=\"off\" placeholder=\"newsletter\"></div>\n" \
"              <div class=\"field\"><label for=\"utmMedium\">UTM Medium</label><input id=\"utmMedium\" type=\"text\" autocomplete=\"off\" placeholder=\"social\"></div>\n" \
"              <div class=\"field\"><label for=\"utmCampaign\">UTM Campaign</label><input id=\"utmCampaign\" type=\"text\" autocomplete=\"off\" placeholder=\"q4_launch\"></div>\n" \
"            </div>\n" \
"            <div class=\"row-3\">\n" \
"              <div class=\"field\"><label for=\"ttl\">Expiry (hours)</label><input id=\"ttl\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"8760\" placeholder=\"Optional\"></div>\n" \
"              <div class=\"field\"><label for=\"maxClicks\">Max clicks</label><input id=\"maxClicks\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"1000000\" placeholder=\"Unlimited\"></div>\n" \
"              <div class=\"field\"><label for=\"activatesIn\">Delay (hours)</label><input id=\"activatesIn\" type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"8760\" placeholder=\"Immediate\"></div>\n" \
"            </div>\n" \
"            <div class=\"field\">\n" \
"              <label for=\"interstitial\">Redirect Mode</label>\n" \
"              <select id=\"interstitial\">\n" \
"                <option value=\"0\">Instant HTTP Redirect (Default)</option>\n" \
"                <option value=\"1\">Safety Preview Interstitial Page (Show destination before redirect)</option>\n" \
"              </select>\n" \
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
"      <div data-tab-pane=\"bulk\" hidden>\n" \
"        <p class=\"hint\">Shorten up to 25 URLs in a single atomic batch. Tagging a batch automatically publishes a Link-in-Bio collection at <code>/bio/&lt;tag&gt;</code>.</p>\n" \
"        <form id=\"bulkForm\" novalidate>\n" \
"          <div class=\"field\">\n" \
"            <label for=\"bulkApiKey\">API Key or Scoped Creator Token</label>\n" \
"            <input id=\"bulkApiKey\" type=\"password\" placeholder=\"Admin key or csh_... creator token\" required>\n" \
"          </div>\n" \
"          <div class=\"field\">\n" \
"            <label for=\"bulkUrls\">URLs (one per line, max 25)</label>\n" \
"            <textarea id=\"bulkUrls\" rows=\"5\" placeholder=\"https://example.com/first-page&#10;https://example.com/second-page\" required></textarea>\n" \
"          </div>\n" \
"          <div class=\"row\">\n" \
"            <div class=\"field\"><label for=\"bulkTags\">Batch Tag / Bio Profile</label><input id=\"bulkTags\" type=\"text\" placeholder=\"myprofile\"></div>\n" \
"            <div class=\"field\"><label for=\"bulkTtl\">Expiry (hours, optional)</label><input id=\"bulkTtl\" type=\"number\" min=\"1\" max=\"8760\" placeholder=\"Optional\"></div>\n" \
"          </div>\n" \
"          <button type=\"submit\" class=\"primary\" id=\"bulkSubmitBtn\">Shorten Batch</button>\n" \
"        </form>\n" \
"        <div id=\"bulkResult\" class=\"panel\" hidden></div>\n" \
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
"        <h3>Create a short link (with UTM, Mobile Routing &amp; Idempotency)</h3>\n" \
"        <pre><code>curl -X POST https://c.micutu.com/shorten \\\n" \
"  -H \"Content-Type: application/json\" \\\n" \
"  -H \"X-API-Key: csh_your_scoped_token\" \\\n" \
"  -H \"Idempotency-Key: release-2026-v1\" \\\n" \
"  -d '{\"url\":\"https://example.com\",\"mobile_url\":\"https://m.example.com\",\"custom_slug\":\"demo\",\"tags\":\"launch\",\"utm_source\":\"cli\"}'</code></pre>\n" \
"        <h3>Native C CLI Client (<code>bin/shortener-cli</code>)</h3>\n" \
"        <pre><code>./bin/shortener-cli shorten https://example.com --slug demo --tag launch\n" \
"./bin/shortener-cli inspect demo\n" \
"./bin/shortener-cli stats demo</code></pre>\n" \
"        <h3>QR SVG, Social Preview Card &amp; Link-in-Bio Hub</h3>\n" \
"        <pre><code>curl -sO https://c.micutu.com/qr/demo.svg\n" \
"curl -s https://c.micutu.com/p/demo\n" \
"curl -s https://c.micutu.com/bio/launch</code></pre>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"  <footer>\n" \
"    <a href=\"/health\">Service Health</a> &middot; <a href=\"/openapi.json\">OpenAPI 3.0</a> &middot; <a href=\"/admin\">Admin</a> &middot; <a href=\"/sitemap.xml\">Sitemap</a> &middot; Built in C17 with libmicrohttpd &amp; SQLite\n" \
"  </footer>\n" \
"  <script src=\"/assets/index.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#endif
