#ifndef ADMIN_HTML_H
#define ADMIN_HTML_H

#define ADMIN_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <meta name=\"robots\" content=\"noindex,nofollow\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>Admin Dashboard - c.micutu.com</title>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">ADMIN</span></a>\n" \
"    <span class=\"hint\">Key remains strictly in this tab's memory.</span>\n" \
"  </header>\n" \
"  <main>\n" \
"    <section class=\"panel narrow\" id=\"authPanel\" aria-labelledby=\"authTitle\">\n" \
"      <h1 id=\"authTitle\">Unlock Admin Dashboard</h1>\n" \
"      <p class=\"hint\">Enter the master Admin API key to manage links, issue scoped creator tokens, inspect security audit logs, and view live C runtime telemetry.</p>\n" \
"      <form id=\"authForm\">\n" \
"        <div class=\"field\">\n" \
"          <label for=\"apiKey\">Master API Key</label>\n" \
"          <input id=\"apiKey\" type=\"password\" autocomplete=\"current-password\" required>\n" \
"        </div>\n" \
"        <button class=\"primary\" type=\"submit\">Open Dashboard</button>\n" \
"      </form>\n" \
"      <p class=\"hint error\" id=\"authStatus\" aria-live=\"polite\"></p>\n" \
"    </section>\n" \
"    <section id=\"dashboard\" hidden aria-live=\"polite\">\n" \
"      <div class=\"summary\">\n" \
"        <div class=\"metric\"><strong id=\"totalLinks\">0</strong><span>Total Links</span></div>\n" \
"        <div class=\"metric\"><strong id=\"activeLinks\">0</strong><span>Active Links</span></div>\n" \
"        <div class=\"metric\"><strong id=\"totalVisits\">0</strong><span>Total Visits</span></div>\n" \
"        <div class=\"metric\"><strong id=\"totalUnique\">0</strong><span>Unique Visitors</span></div>\n" \
"      </div>\n" \
"      <div class=\"tabs\" role=\"tablist\">\n" \
"        <button type=\"button\" class=\"tab-btn active\" data-admin-tab=\"links\">Links &amp; Controls</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-admin-tab=\"tokens\">Scoped Creator Tokens</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-admin-tab=\"blacklist\">Domain Blacklist</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-admin-tab=\"audit\">Security Audit Log</button>\n" \
"        <button type=\"button\" class=\"tab-btn\" data-admin-tab=\"system\">System &amp; Metrics</button>\n" \
"      </div>\n" \
"      <p class=\"hint\" id=\"dashboardStatus\" aria-live=\"polite\"></p>\n" \
"      <div data-admin-pane=\"links\">\n" \
"        <div class=\"toolbar\">\n" \
"          <input id=\"filterSearch\" type=\"search\" placeholder=\"Search slug, URL, or title...\">\n" \
"          <input id=\"filterTag\" type=\"text\" placeholder=\"Filter by tag...\">\n" \
"          <select id=\"filterStatus\">\n" \
"            <option value=\"\">All statuses</option>\n" \
"            <option value=\"active\">Active only</option>\n" \
"            <option value=\"expired\">Expired only</option>\n" \
"            <option value=\"disabled\">Disabled only</option>\n" \
"          </select>\n" \
"          <div class=\"actions\">\n" \
"            <button id=\"filterBtn\" type=\"button\" class=\"primary-inline\">Filter</button>\n" \
"            <button id=\"refresh\" type=\"button\">Refresh</button>\n" \
"            <button id=\"exportCsv\" type=\"button\">Export CSV</button>\n" \
"            <button id=\"exportJson\" type=\"button\">Backup JSON</button>\n" \
"            <button id=\"importJsonBtn\" type=\"button\">Restore JSON</button>\n" \
"            <input id=\"importJsonFile\" type=\"file\" accept=\"application/json,.json\" hidden>\n" \
"            <button id=\"cleanupBtn\" type=\"button\">Purge Expired</button>\n" \
"            <button id=\"maintenanceBtn\" type=\"button\">Optimize DB &amp; WAL</button>\n" \
"            <button id=\"lock\" type=\"button\" class=\"danger\">Lock</button>\n" \
"          </div>\n" \
"        </div>\n" \
"        <div class=\"table-wrap\">\n" \
"          <table>\n" \
"            <thead><tr><th>Slug / Title</th><th>Target URL</th><th>Created</th><th>Status &amp; Flags</th><th>Visits / Unique</th><th>Actions</th></tr></thead>\n" \
"            <tbody id=\"linkRows\"></tbody>\n" \
"          </table>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div data-admin-pane=\"tokens\" hidden>\n" \
"        <div class=\"panel\">\n" \
"          <h2>Create Scoped Creator Token</h2>\n" \
"          <p class=\"hint\">Creator tokens (<code>csh_...</code>) can only call <code>POST /shorten</code> and <code>POST /api/bulk/shorten</code>, and cannot read, edit, or delete existing links.</p>\n" \
"          <form id=\"tokenForm\" class=\"row-3\">\n" \
"            <div class=\"field\"><label for=\"tokenName\">Token Label</label><input id=\"tokenName\" type=\"text\" placeholder=\"e.g. Marketing Bot, Mobile App\" required></div>\n" \
"            <div class=\"field\"><label for=\"tokenTtl\">Expiry (days, optional)</label><input id=\"tokenTtl\" type=\"number\" min=\"1\" max=\"3650\" placeholder=\"Never expires\"></div>\n" \
"            <div class=\"field\"><label>&nbsp;</label><button type=\"submit\" class=\"primary\">Generate Token</button></div>\n" \
"          </form>\n" \
"          <pre id=\"newTokenDisplay\" class=\"success\" hidden></pre>\n" \
"        </div>\n" \
"        <div class=\"table-wrap\">\n" \
"          <table>\n" \
"            <thead><tr><th>Name</th><th>Prefix</th><th>Created</th><th>Expires</th><th>Uses (Last Used)</th><th>Action</th></tr></thead>\n" \
"            <tbody id=\"tokenRows\"></tbody>\n" \
"          </table>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div data-admin-pane=\"blacklist\" hidden>\n" \
"        <div class=\"panel\">\n" \
"          <h2>Block Malicious or Phishing Domain</h2>\n" \
"          <p class=\"hint\">Blocks link shortening to target host (supports wildcards like <code>*.evil.com</code> or exact <code>phish.site</code>). Existing links to this domain are immediately blocked on redirect.</p>\n" \
"          <form id=\"blacklistForm\" class=\"grid-form\">\n" \
"            <div class=\"field\"><label for=\"blacklistPattern\">Domain Pattern</label><input id=\"blacklistPattern\" required placeholder=\"*.phishing.com or spam.site\"></div>\n" \
"            <div class=\"field\"><label for=\"blacklistReason\">Reason (optional)</label><input id=\"blacklistReason\" placeholder=\"Phishing / scam / malware\"></div>\n" \
"            <div class=\"field\"><label>&nbsp;</label><button type=\"submit\" class=\"primary\">Block Domain</button></div>\n" \
"          </form>\n" \
"        </div>\n" \
"        <div class=\"table-wrap\">\n" \
"          <table>\n" \
"            <thead><tr><th>Domain Pattern</th><th>Reason</th><th>Blocked At (UTC)</th><th>Action</th></tr></thead>\n" \
"            <tbody id=\"blacklistRows\"></tbody>\n" \
"          </table>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div data-admin-pane=\"audit\" hidden>\n" \
"        <div class=\"table-wrap\">\n" \
"          <table>\n" \
"            <thead><tr><th>Timestamp (UTC)</th><th>Event</th><th>Actor</th><th>Target</th><th>Details</th></tr></thead>\n" \
"            <tbody id=\"auditRows\"></tbody>\n" \
"          </table>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div data-admin-pane=\"system\" hidden>\n" \
"        <div class=\"panel\">\n" \
"          <h2>C17 Engine &amp; SQLite Diagnostics</h2>\n" \
"          <div id=\"systemGrid\"></div>\n" \
"        </div>\n" \
"        <div class=\"panel\">\n" \
"          <h2>Prometheus Exporter (<code>GET /metrics</code>)</h2>\n" \
"          <pre id=\"prometheusBox\">Loading metrics...</pre>\n" \
"        </div>\n" \
"      </div>\n" \
"    </section>\n" \
"    <div id=\"editModal\" class=\"modal-backdrop\" hidden>\n" \
"      <div class=\"modal\" role=\"dialog\" aria-modal=\"true\" aria-labelledby=\"editModalTitle\">\n" \
"        <h2 id=\"editModalTitle\">Edit Link</h2>\n" \
"        <form id=\"editForm\">\n" \
"          <input id=\"editSlug\" type=\"hidden\">\n" \
"          <div class=\"field\"><label for=\"editUrl\">Destination URL</label><input id=\"editUrl\" type=\"url\" required></div>\n" \
"          <div class=\"field\"><label for=\"editMobileUrl\">Mobile Destination URL (optional)</label><input id=\"editMobileUrl\" type=\"url\" placeholder=\"https://m.example.com\"></div>\n" \
"          <div class=\"row\">\n" \
"            <div class=\"field\"><label for=\"editTitle\">Title / Note</label><input id=\"editTitle\" type=\"text\"></div>\n" \
"            <div class=\"field\"><label for=\"editTags\">Tags</label><input id=\"editTags\" type=\"text\" placeholder=\"work,promo\"></div>\n" \
"          </div>\n" \
"          <div class=\"row-3\">\n" \
"            <div class=\"field\"><label for=\"editMaxClicks\">Max Clicks (0=unl)</label><input id=\"editMaxClicks\" type=\"number\" min=\"0\" max=\"1000000\"></div>\n" \
"            <div class=\"field\"><label for=\"editTtl\">New TTL Hours (-1=keep, 0=none)</label><input id=\"editTtl\" type=\"number\" min=\"-1\" max=\"8760\" value=\"-1\"></div>\n" \
"            <div class=\"field\"><label for=\"editActive\">Status</label><select id=\"editActive\"><option value=\"1\">Active</option><option value=\"0\">Disabled</option></select></div>\n" \
"          </div>\n" \
"          <div class=\"field\"><label for=\"editPassword\">New Password (leave blank to keep, or type __clear__ to remove)</label><input id=\"editPassword\" type=\"text\" autocomplete=\"off\"></div>\n" \
"          <div class=\"actions\">\n" \
"            <button type=\"submit\" class=\"primary-inline\">Save Changes</button>\n" \
"            <button type=\"button\" id=\"editCancel\">Cancel</button>\n" \
"          </div>\n" \
"        </form>\n" \
"      </div>\n" \
"    </div>\n" \
"  </main>\n" \
"  <script src=\"/assets/admin.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#endif
