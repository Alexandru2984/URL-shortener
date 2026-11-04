#ifndef ASSETS_H
#define ASSETS_H

#define FAVICON_SVG \
"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 64 64\" role=\"img\" aria-label=\"c.micutu.com\">" \
"<defs><linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\">" \
"<stop offset=\"0%\" stop-color=\"#3b82f6\"/><stop offset=\"100%\" stop-color=\"#06b6d4\"/>" \
"</linearGradient></defs>" \
"<rect width=\"64\" height=\"64\" rx=\"16\" fill=\"#0f172a\"/>" \
"<rect x=\"2\" y=\"2\" width=\"60\" height=\"60\" rx=\"14\" fill=\"none\" stroke=\"url(#g)\" stroke-width=\"3\"/>" \
"<path d=\"M26 24a8 8 0 0 1 11.3 0l3.7 3.7a8 8 0 0 1-11.3 11.3l-1.5-1.5\" fill=\"none\" stroke=\"#38bdf8\" stroke-width=\"4.5\" stroke-linecap=\"round\"/>" \
"<path d=\"M38 40a8 8 0 0 1-11.3 0L23 36.3A8 8 0 0 1 34.3 25l1.5 1.5\" fill=\"none\" stroke=\"#60a5fa\" stroke-width=\"4.5\" stroke-linecap=\"round\"/>" \
"</svg>"

#define MANIFEST_JSON \
"{\n" \
"  \"name\": \"c.micutu.com - C URL Shortener\",\n" \
"  \"short_name\": \"c.micutu\",\n" \
"  \"description\": \"Ultra-fast, privacy-first URL shortener and QR generator written in C.\",\n" \
"  \"start_url\": \"/\",\n" \
"  \"display\": \"standalone\",\n" \
"  \"background_color\": \"#0f172a\",\n" \
"  \"theme_color\": \"#3b82f6\",\n" \
"  \"icons\": [\n" \
"    {\n" \
"      \"src\": \"/favicon.svg\",\n" \
"      \"sizes\": \"any\",\n" \
"      \"type\": \"image/svg+xml\",\n" \
"      \"purpose\": \"any maskable\"\n" \
"    }\n" \
"  ]\n" \
"}\n"

#define APP_CSS \
":root {\n" \
"  color-scheme: dark;\n" \
"  --bg: #0b1120;\n" \
"  --bg-accent: #13233f;\n" \
"  --surface: #152033;\n" \
"  --surface-2: #1e2d47;\n" \
"  --surface-3: #263859;\n" \
"  --text: #f8fafc;\n" \
"  --muted: #94a3b8;\n" \
"  --border: #2e405e;\n" \
"  --primary: #3b82f6;\n" \
"  --primary-hover: #2563eb;\n" \
"  --accent: #06b6d4;\n" \
"  --danger: #f87171;\n" \
"  --danger-bg: rgba(239, 68, 68, 0.14);\n" \
"  --success: #4ade80;\n" \
"  --success-bg: rgba(74, 222, 128, 0.14);\n" \
"  --warning: #facc15;\n" \
"  --warning-bg: rgba(250, 204, 21, 0.14);\n" \
"  --shadow: 0 22px 55px rgba(0, 0, 0, 0.38);\n" \
"}\n" \
"[data-theme=\"light\"] {\n" \
"  color-scheme: light;\n" \
"  --bg: #f1f5f9;\n" \
"  --bg-accent: #dbeafe;\n" \
"  --surface: #ffffff;\n" \
"  --surface-2: #f8fafc;\n" \
"  --surface-3: #e2e8f0;\n" \
"  --text: #0f172a;\n" \
"  --muted: #475569;\n" \
"  --border: #cbd5e1;\n" \
"  --primary: #2563eb;\n" \
"  --primary-hover: #1d4ed8;\n" \
"  --accent: #0891b2;\n" \
"  --danger: #dc2626;\n" \
"  --danger-bg: rgba(220, 38, 38, 0.1);\n" \
"  --success: #16a34a;\n" \
"  --success-bg: rgba(22, 163, 74, 0.1);\n" \
"  --warning: #ca8a04;\n" \
"  --warning-bg: rgba(202, 138, 4, 0.12);\n" \
"  --shadow: 0 18px 40px rgba(15, 23, 42, 0.08);\n" \
"}\n" \
"* { box-sizing: border-box; }\n" \
"[hidden] { display: none !important; }\n" \
"body {\n" \
"  min-width: 320px;\n" \
"  min-height: 100vh;\n" \
"  margin: 0;\n" \
"  display: flex;\n" \
"  flex-direction: column;\n" \
"  background: radial-gradient(circle at 50% 0%, var(--bg-accent) 0%, var(--bg) 48rem);\n" \
"  color: var(--text);\n" \
"  font: 15px/1.5 system-ui, -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif;\n" \
"}\n" \
"a { color: var(--primary); text-decoration: none; }\n" \
"a:hover { text-decoration: underline; }\n" \
".navbar {\n" \
"  width: min(1140px, 100%);\n" \
"  margin: 0 auto;\n" \
"  padding: 1rem clamp(1rem, 3vw, 2rem);\n" \
"  display: flex;\n" \
"  align-items: center;\n" \
"  justify-content: space-between;\n" \
"  gap: 1rem;\n" \
"  flex-wrap: wrap;\n" \
"}\n" \
".brand {\n" \
"  display: inline-flex;\n" \
"  align-items: center;\n" \
"  gap: 0.65rem;\n" \
"  font-weight: 750;\n" \
"  font-size: 1.15rem;\n" \
"  color: var(--text);\n" \
"  text-decoration: none;\n" \
"}\n" \
".brand:hover { text-decoration: none; color: var(--primary); }\n" \
".brand img { width: 32px; height: 32px; border-radius: 8px; }\n" \
".brand-badge {\n" \
"  font-size: 0.7rem;\n" \
"  padding: 0.15rem 0.45rem;\n" \
"  border-radius: 999px;\n" \
"  background: var(--surface-2);\n" \
"  border: 1px solid var(--border);\n" \
"  color: var(--accent);\n" \
"  font-weight: 700;\n" \
"  letter-spacing: 0.04em;\n" \
"}\n" \
".nav-links {\n" \
"  display: flex;\n" \
"  align-items: center;\n" \
"  gap: 0.5rem;\n" \
"  flex-wrap: wrap;\n" \
"}\n" \
"main {\n" \
"  flex: 1;\n" \
"  width: min(1140px, 100%);\n" \
"  margin: 0 auto;\n" \
"  padding: 0.5rem clamp(1rem, 3vw, 2rem) 2.5rem;\n" \
"}\n" \
"main.narrow { width: min(640px, 100%); }\n" \
"main.centered {\n" \
"  display: flex;\n" \
"  flex-direction: column;\n" \
"  justify-content: center;\n" \
"}\n" \
".container, .panel {\n" \
"  background: var(--surface);\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 16px;\n" \
"  padding: clamp(1.25rem, 4vw, 2rem);\n" \
"  box-shadow: var(--shadow);\n" \
"  margin-bottom: 1.25rem;\n" \
"}\n" \
"h1 { margin: 0 0 0.35rem; font-size: clamp(1.45rem, 4vw, 1.95rem); letter-spacing: -0.02em; }\n" \
"h2 { margin: 0 0 0.5rem; font-size: 1.25rem; }\n" \
"h3 { margin: 0 0 0.5rem; font-size: 1.02rem; }\n" \
".text-center { text-align: center; }\n" \
".subtitle, .hint {\n" \
"  margin: 0 0 1.35rem;\n" \
"  color: var(--muted);\n" \
"  font-size: 0.92rem;\n" \
"}\n" \
".tabs {\n" \
"  display: flex;\n" \
"  gap: 0.4rem;\n" \
"  padding: 0.3rem;\n" \
"  background: var(--bg);\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 12px;\n" \
"  margin-bottom: 1.35rem;\n" \
"  overflow-x: auto;\n" \
"}\n" \
".tab-btn {\n" \
"  flex: 1;\n" \
"  min-height: 38px;\n" \
"  border: 0;\n" \
"  border-radius: 8px;\n" \
"  background: transparent;\n" \
"  color: var(--muted);\n" \
"  font-weight: 650;\n" \
"  font-size: 0.88rem;\n" \
"  padding: 0.45rem 0.75rem;\n" \
"  cursor: pointer;\n" \
"  white-space: nowrap;\n" \
"}\n" \
".tab-btn:hover { color: var(--text); }\n" \
".tab-btn.active {\n" \
"  background: var(--surface-2);\n" \
"  color: var(--text);\n" \
"  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);\n" \
"}\n" \
"label {\n" \
"  display: block;\n" \
"  margin: 0 0 0.38rem;\n" \
"  color: var(--muted);\n" \
"  font-size: 0.84rem;\n" \
"  font-weight: 650;\n" \
"}\n" \
".field { margin-bottom: 1rem; }\n" \
"input, select, textarea {\n" \
"  width: 100%;\n" \
"  min-height: 44px;\n" \
"  padding: 0.65rem 0.85rem;\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 10px;\n" \
"  background: var(--bg);\n" \
"  color: var(--text);\n" \
"  font: inherit;\n" \
"}\n" \
"input::placeholder, textarea::placeholder { color: #64748b; }\n" \
"input:focus, select:focus, textarea:focus {\n" \
"  outline: 2px solid var(--primary);\n" \
"  outline-offset: 2px;\n" \
"}\n" \
".row {\n" \
"  display: grid;\n" \
"  grid-template-columns: repeat(2, minmax(0, 1fr));\n" \
"  gap: 0.85rem;\n" \
"}\n" \
".row-3 {\n" \
"  display: grid;\n" \
"  grid-template-columns: repeat(3, minmax(0, 1fr));\n" \
"  gap: 0.85rem;\n" \
"}\n" \
".options-toggle {\n" \
"  width: auto;\n" \
"  margin: 0 0 1rem;\n" \
"  padding: 0.35rem 0.65rem;\n" \
"  border: 1px dashed var(--border);\n" \
"  border-radius: 8px;\n" \
"  color: var(--muted);\n" \
"  background: transparent;\n" \
"  font-size: 0.85rem;\n" \
"}\n" \
".options-toggle:hover { color: var(--primary); border-color: var(--primary); }\n" \
".advanced { display: none; padding: 1rem; border-radius: 12px; background: var(--surface-2); border: 1px solid var(--border); margin-bottom: 1rem; }\n" \
".advanced.open { display: block; }\n" \
".preset-bar { display: flex; flex-wrap: wrap; gap: 0.45rem; margin-bottom: 1rem; }\n" \
".preset-chip {\n" \
"  min-height: 30px;\n" \
"  padding: 0.25rem 0.65rem;\n" \
"  font-size: 0.78rem;\n" \
"  border-radius: 999px;\n" \
"  background: var(--bg);\n" \
"  border: 1px solid var(--border);\n" \
"  color: var(--muted);\n" \
"}\n" \
".preset-chip:hover { color: var(--primary); border-color: var(--primary); }\n" \
"button, .btn {\n" \
"  display: inline-flex;\n" \
"  align-items: center;\n" \
"  justify-content: center;\n" \
"  gap: 0.4rem;\n" \
"  min-height: 42px;\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 10px;\n" \
"  padding: 0.55rem 0.95rem;\n" \
"  color: var(--text);\n" \
"  background: var(--surface-2);\n" \
"  font: inherit;\n" \
"  font-weight: 650;\n" \
"  cursor: pointer;\n" \
"  text-decoration: none;\n" \
"}\n" \
"button:hover, .btn:hover { border-color: var(--primary); text-decoration: none; }\n" \
"button:focus-visible, .btn:focus-visible { outline: 2px solid var(--primary); outline-offset: 2px; }\n" \
"button:disabled { cursor: wait; opacity: 0.65; }\n" \
".btn-sm { min-height: 34px; padding: 0.35rem 0.65rem; font-size: 0.82rem; border-radius: 8px; }\n" \
".primary, button.primary {\n" \
"  width: 100%;\n" \
"  border-color: var(--primary);\n" \
"  background: var(--primary);\n" \
"  color: #ffffff;\n" \
"}\n" \
".primary:hover, button.primary:hover { background: var(--primary-hover); }\n" \
".primary-inline {\n" \
"  width: auto;\n" \
"  border-color: var(--primary);\n" \
"  background: var(--primary);\n" \
"  color: #ffffff;\n" \
"}\n" \
".danger, button.danger {\n" \
"  border-color: rgba(248, 113, 113, 0.45);\n" \
"  background: var(--danger-bg);\n" \
"  color: var(--danger);\n" \
"}\n" \
"#result {\n" \
"  display: none;\n" \
"  margin-top: 1.25rem;\n" \
"  padding: 1.2rem;\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 12px;\n" \
"  background: var(--bg);\n" \
"}\n" \
"#result.show { display: block; }\n" \
".result-grid {\n" \
"  display: grid;\n" \
"  grid-template-columns: 1fr auto;\n" \
"  gap: 1rem;\n" \
"  align-items: center;\n" \
"}\n" \
".result-label { color: var(--muted); font-size: 0.82rem; text-transform: uppercase; letter-spacing: 0.05em; }\n" \
".result-link {\n" \
"  display: block;\n" \
"  margin: 0.35rem 0 0.85rem;\n" \
"  overflow-wrap: anywhere;\n" \
"  color: var(--primary);\n" \
"  font-size: 1.2rem;\n" \
"  font-weight: 750;\n" \
"}\n" \
".result-actions { display: flex; flex-wrap: wrap; gap: 0.55rem; }\n" \
".qr-preview {\n" \
"  width: 124px;\n" \
"  height: 124px;\n" \
"  padding: 0.45rem;\n" \
"  background: #ffffff;\n" \
"  border-radius: 12px;\n" \
"  border: 1px solid var(--border);\n" \
"  display: flex;\n" \
"  align-items: center;\n" \
"  justify-content: center;\n" \
"}\n" \
".qr-preview img { width: 100%; height: 100%; display: block; }\n" \
".error { color: var(--danger); font-weight: 600; }\n" \
".success { color: var(--success); font-weight: 600; }\n" \
".badge {\n" \
"  display: inline-flex;\n" \
"  align-items: center;\n" \
"  margin: 0.1rem 0.25rem 0.1rem 0;\n" \
"  padding: 0.15rem 0.5rem;\n" \
"  border-radius: 999px;\n" \
"  background: var(--warning-bg);\n" \
"  color: var(--warning);\n" \
"  font-size: 0.74rem;\n" \
"  font-weight: 650;\n" \
"}\n" \
".badge.ok { background: var(--success-bg); color: var(--success); }\n" \
".badge.exp, .badge.danger { background: var(--danger-bg); color: var(--danger); }\n" \
".badge.tag { background: var(--surface-3); color: var(--text); }\n" \
".summary {\n" \
"  display: grid;\n" \
"  grid-template-columns: repeat(4, minmax(0, 1fr));\n" \
"  gap: 0.85rem;\n" \
"  margin: 1rem 0 1.25rem;\n" \
"}\n" \
".metric {\n" \
"  padding: 1.05rem;\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 12px;\n" \
"  background: var(--surface);\n" \
"}\n" \
".metric strong {\n" \
"  display: block;\n" \
"  color: var(--primary);\n" \
"  font-size: 1.65rem;\n" \
"  line-height: 1.2;\n" \
"}\n" \
".metric span {\n" \
"  color: var(--muted);\n" \
"  font-size: 0.78rem;\n" \
"  text-transform: uppercase;\n" \
"  letter-spacing: 0.06em;\n" \
"}\n" \
".toolbar {\n" \
"  display: grid;\n" \
"  grid-template-columns: 2fr 1fr 1fr auto;\n" \
"  gap: 0.65rem;\n" \
"  margin-bottom: 1rem;\n" \
"  align-items: center;\n" \
"}\n" \
".actions { display: flex; flex-wrap: wrap; gap: 0.6rem; align-items: center; }\n" \
".table-wrap {\n" \
"  overflow-x: auto;\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 14px;\n" \
"  background: var(--surface);\n" \
"}\n" \
"table { width: 100%; min-width: 780px; border-collapse: collapse; }\n" \
"th, td {\n" \
"  padding: 0.75rem 0.9rem;\n" \
"  text-align: left;\n" \
"  border-bottom: 1px solid var(--border);\n" \
"  vertical-align: middle;\n" \
"}\n" \
"th {\n" \
"  color: var(--muted);\n" \
"  font-size: 0.75rem;\n" \
"  text-transform: uppercase;\n" \
"  letter-spacing: 0.06em;\n" \
"  background: var(--surface-2);\n" \
"}\n" \
"tr:last-child td { border-bottom: 0; }\n" \
".url-cell {\n" \
"  max-width: 260px;\n" \
"  overflow: hidden;\n" \
"  text-overflow: ellipsis;\n" \
"  white-space: nowrap;\n" \
"  color: var(--muted);\n" \
"}\n" \
".empty { padding: 2rem; text-align: center; color: var(--muted); }\n" \
".stat-grid {\n" \
"  display: grid;\n" \
"  grid-template-columns: repeat(2, minmax(0, 1fr));\n" \
"  gap: 0.85rem;\n" \
"  margin-bottom: 1.25rem;\n" \
"}\n" \
".stat-box {\n" \
"  background: var(--bg);\n" \
"  border: 1px solid var(--border);\n" \
"  padding: 1.25rem;\n" \
"  border-radius: 12px;\n" \
"  text-align: center;\n" \
"}\n" \
".stat-value { font-size: 2.25rem; font-weight: 800; color: var(--primary); }\n" \
".stat-label { color: var(--muted); font-size: 0.8rem; text-transform: uppercase; letter-spacing: 0.06em; }\n" \
".list-row {\n" \
"  display: flex;\n" \
"  justify-content: space-between;\n" \
"  align-items: center;\n" \
"  gap: 0.75rem;\n" \
"  padding: 0.55rem 0;\n" \
"  border-bottom: 1px solid var(--border);\n" \
"  font-size: 0.9rem;\n" \
"}\n" \
".list-row:last-child { border-bottom: 0; }\n" \
"progress {\n" \
"  width: 100%;\n" \
"  height: 10px;\n" \
"  border-radius: 999px;\n" \
"  overflow: hidden;\n" \
"  accent-color: var(--primary);\n" \
"}\n" \
"pre, code {\n" \
"  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;\n" \
"  font-size: 0.84rem;\n" \
"}\n" \
"pre {\n" \
"  background: var(--bg);\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 10px;\n" \
"  padding: 0.9rem;\n" \
"  overflow-x: auto;\n" \
"  color: var(--text);\n" \
"}\n" \
".modal-backdrop {\n" \
"  position: fixed;\n" \
"  inset: 0;\n" \
"  background: rgba(2, 6, 23, 0.75);\n" \
"  display: flex;\n" \
"  align-items: center;\n" \
"  justify-content: center;\n" \
"  padding: 1rem;\n" \
"  z-index: 50;\n" \
"}\n" \
".modal {\n" \
"  width: min(540px, 100%);\n" \
"  max-height: 92vh;\n" \
"  overflow-y: auto;\n" \
"  background: var(--surface);\n" \
"  border: 1px solid var(--border);\n" \
"  border-radius: 16px;\n" \
"  padding: 1.5rem;\n" \
"  box-shadow: var(--shadow);\n" \
"}\n" \
".code-hero { font-size: 3.8rem; font-weight: 800; color: var(--primary); line-height: 1; margin-bottom: 0.5rem; }\n" \
"footer {\n" \
"  width: min(1140px, 100%);\n" \
"  margin: 0 auto;\n" \
"  padding: 1.2rem clamp(1rem, 3vw, 2rem);\n" \
"  color: var(--muted);\n" \
"  text-align: center;\n" \
"  font-size: 0.82rem;\n" \
"  border-top: 1px solid var(--border);\n" \
"}\n" \
"footer a { color: var(--muted); }\n" \
"footer a:hover { color: var(--primary); }\n" \
"@media (max-width: 760px) {\n" \
"  .summary { grid-template-columns: repeat(2, minmax(0, 1fr)); }\n" \
"  .toolbar { grid-template-columns: 1fr; }\n" \
"  .row, .row-3, .stat-grid, .result-grid { grid-template-columns: 1fr; }\n" \
"  .qr-preview { margin: 0 auto; }\n" \
"}\n"

#define INDEX_JS \
"(() => {\n" \
"  'use strict';\n" \
"  const byId = (id) => document.getElementById(id);\n" \
"  const sessionLinks = [];\n" \
"\n" \
"  // Theme toggle\n" \
"  const themeBtn = byId('themeToggle');\n" \
"  if (themeBtn) {\n" \
"    themeBtn.addEventListener('click', () => {\n" \
"      const root = document.documentElement;\n" \
"      const next = root.getAttribute('data-theme') === 'light' ? 'dark' : 'light';\n" \
"      root.setAttribute('data-theme', next);\n" \
"      themeBtn.textContent = next === 'light' ? 'Dark mode' : 'Light mode';\n" \
"    });\n" \
"  }\n" \
"\n" \
"  // Tabs\n" \
"  const tabButtons = document.querySelectorAll('[data-tab-target]');\n" \
"  tabButtons.forEach((btn) => {\n" \
"    btn.addEventListener('click', () => {\n" \
"      const target = btn.getAttribute('data-tab-target');\n" \
"      tabButtons.forEach((b) => b.classList.toggle('active', b === btn));\n" \
"      document.querySelectorAll('[data-tab-pane]').forEach((pane) => {\n" \
"        pane.hidden = pane.getAttribute('data-tab-pane') !== target;\n" \
"      });\n" \
"    });\n" \
"  });\n" \
"\n" \
"  const result = byId('result');\n" \
"  const form = byId('shortenForm');\n" \
"  const button = byId('submitButton');\n" \
"  const options = byId('advanced');\n" \
"  const toggle = byId('optionsToggle');\n" \
"\n" \
"  // Quick presets\n" \
"  document.querySelectorAll('[data-preset]').forEach((chip) => {\n" \
"    chip.addEventListener('click', () => {\n" \
"      options.classList.add('open');\n" \
"      toggle.setAttribute('aria-expanded', 'true');\n" \
"      toggle.textContent = 'Hide options';\n" \
"      const preset = chip.getAttribute('data-preset');\n" \
"      if (preset === 'burn') { byId('maxClicks').value = '1'; byId('ttl').value = '24'; }\n" \
"      else if (preset === 'temp1h') { byId('ttl').value = '1'; }\n" \
"      else if (preset === 'temp24h') { byId('ttl').value = '24'; }\n" \
"      else if (preset === 'clear') {\n" \
"        byId('maxClicks').value = '';\n" \
"        byId('ttl').value = '';\n" \
"        byId('activatesIn').value = '';\n" \
"      }\n" \
"    });\n" \
"  });\n" \
"\n" \
"  function showMessage(message, className) {\n" \
"    result.replaceChildren();\n" \
"    const node = document.createElement('p');\n" \
"    node.className = className || '';\n" \
"    node.textContent = message;\n" \
"    result.append(node);\n" \
"    result.classList.add('show');\n" \
"  }\n" \
"\n" \
"  function renderSessionHistory() {\n" \
"    const wrap = byId('sessionHistory');\n" \
"    const list = byId('sessionList');\n" \
"    if (!wrap || !list || !sessionLinks.length) return;\n" \
"    wrap.hidden = false;\n" \
"    list.replaceChildren();\n" \
"    for (const item of sessionLinks) {\n" \
"      const row = document.createElement('div');\n" \
"      row.className = 'list-row';\n" \
"      const a = document.createElement('a');\n" \
"      a.href = item.shortUrl;\n" \
"      a.target = '_blank';\n" \
"      a.rel = 'noopener noreferrer';\n" \
"      a.textContent = item.shortUrl;\n" \
"      const actions = document.createElement('div');\n" \
"      actions.className = 'actions';\n" \
"      const statsLink = document.createElement('a');\n" \
"      statsLink.className = 'btn btn-sm';\n" \
"      statsLink.href = '/stats/' + encodeURIComponent(item.slug);\n" \
"      statsLink.textContent = 'Stats';\n" \
"      const qrLink = document.createElement('a');\n" \
"      qrLink.className = 'btn btn-sm';\n" \
"      qrLink.href = '/qr/' + encodeURIComponent(item.slug) + '.svg';\n" \
"      qrLink.target = '_blank';\n" \
"      qrLink.textContent = 'QR';\n" \
"      actions.append(statsLink, qrLink);\n" \
"      row.append(a, actions);\n" \
"      list.append(row);\n" \
"    }\n" \
"  }\n" \
"\n" \
"  function showLink(data) {\n" \
"    const shortUrl = data.short_url;\n" \
"    let slug = data.slug || '';\n" \
"    if (!slug) {\n" \
"      try {\n" \
"        const parsed = new URL(shortUrl);\n" \
"        const segments = parsed.pathname.split('/').filter(Boolean);\n" \
"        slug = segments[segments.length - 1] || '';\n" \
"      } catch (e) {}\n" \
"    }\n" \
"    result.replaceChildren();\n" \
"    const grid = document.createElement('div');\n" \
"    grid.className = 'result-grid';\n" \
"    const left = document.createElement('div');\n" \
"    const label = document.createElement('div');\n" \
"    label.className = 'result-label';\n" \
"    label.textContent = 'Your short link is ready';\n" \
"    const link = document.createElement('a');\n" \
"    link.className = 'result-link';\n" \
"    link.href = shortUrl;\n" \
"    link.target = '_blank';\n" \
"    link.rel = 'noopener noreferrer';\n" \
"    link.textContent = shortUrl;\n" \
"    const actions = document.createElement('div');\n" \
"    actions.className = 'result-actions';\n" \
"    const copy = document.createElement('button');\n" \
"    copy.type = 'button';\n" \
"    copy.className = 'btn btn-sm primary-inline';\n" \
"    copy.textContent = 'Copy URL';\n" \
"    copy.addEventListener('click', async () => {\n" \
"      try {\n" \
"        await navigator.clipboard.writeText(shortUrl);\n" \
"        copy.textContent = 'Copied!';\n" \
"        window.setTimeout(() => { copy.textContent = 'Copy URL'; }, 1500);\n" \
"      } catch (error) { copy.textContent = 'Copy unavailable'; }\n" \
"    });\n" \
"    actions.append(copy);\n" \
"    if (slug) {\n" \
"      const stats = document.createElement('a');\n" \
"      stats.className = 'btn btn-sm';\n" \
"      stats.href = '/stats/' + encodeURIComponent(slug);\n" \
"      stats.textContent = 'Analytics';\n" \
"      const inspect = document.createElement('a');\n" \
"      inspect.className = 'btn btn-sm';\n" \
"      inspect.href = '/inspect/' + encodeURIComponent(slug);\n" \
"      inspect.textContent = 'Inspect';\n" \
"      const qrBtn = document.createElement('a');\n" \
"      qrBtn.className = 'btn btn-sm';\n" \
"      qrBtn.href = '/qr/' + encodeURIComponent(slug) + '.svg';\n" \
"      qrBtn.download = slug + '-qr.svg';\n" \
"      qrBtn.textContent = 'Download QR SVG';\n" \
"      const cardBtn = document.createElement('a');\n" \
"      cardBtn.className = 'btn btn-sm';\n" \
"      cardBtn.href = '/p/' + encodeURIComponent(slug);\n" \
"      cardBtn.target = '_blank';\n" \
"      cardBtn.textContent = 'Social Card';\n" \
"      actions.append(stats, inspect, qrBtn, cardBtn);\n" \
"    }\n" \
"    left.append(label, link, actions);\n" \
"    grid.append(left);\n" \
"    if (slug) {\n" \
"      const qrWrap = document.createElement('div');\n" \
"      const qrBox = document.createElement('div');\n" \
"      qrBox.className = 'qr-preview';\n" \
"      const qrImg = document.createElement('img');\n" \
"      qrImg.src = '/qr/' + encodeURIComponent(slug) + '.svg';\n" \
"      qrImg.alt = 'QR Code for ' + slug;\n" \
"      qrBox.append(qrImg);\n" \
"      const themeSelect = document.createElement('select');\n" \
"      themeSelect.className = 'btn-sm';\n" \
"      for (const [val, labelTxt] of [['', 'QR: Classic'], ['cyber', 'QR: Cyber'], ['dark', 'QR: Dark'], ['emerald', 'QR: Emerald'], ['sunset', 'QR: Sunset']]) {\n" \
"        const opt = document.createElement('option');\n" \
"        opt.value = val;\n" \
"        opt.textContent = labelTxt;\n" \
"        themeSelect.append(opt);\n" \
"      }\n" \
"      themeSelect.addEventListener('change', () => {\n" \
"        const t = themeSelect.value;\n" \
"        const qrUrl = '/qr/' + encodeURIComponent(slug) + '.svg' + (t ? '?theme=' + encodeURIComponent(t) : '');\n" \
"        qrImg.src = qrUrl;\n" \
"      });\n" \
"      qrWrap.append(qrBox, themeSelect);\n" \
"      grid.append(qrWrap);\n" \
"      sessionLinks.unshift({ slug, shortUrl });\n" \
"      if (sessionLinks.length > 8) sessionLinks.pop();\n" \
"      renderSessionHistory();\n" \
"    }\n" \
"    result.append(grid);\n" \
"    result.classList.add('show');\n" \
"  }\n" \
"\n" \
"  if (toggle) {\n" \
"    toggle.addEventListener('click', () => {\n" \
"      const open = options.classList.toggle('open');\n" \
"      toggle.setAttribute('aria-expanded', String(open));\n" \
"      toggle.textContent = open ? 'Hide options' : 'More options';\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (form) {\n" \
"    form.addEventListener('submit', async (event) => {\n" \
"      event.preventDefault();\n" \
"      const url = byId('url').value.trim();\n" \
"      const apiKey = byId('apiKey').value.trim();\n" \
"      const slug = byId('slug').value.trim();\n" \
"      const title = byId('title') ? byId('title').value.trim() : '';\n" \
"      const tags = byId('tags') ? byId('tags').value.trim() : '';\n" \
"      const mobileUrl = byId('mobileUrl') ? byId('mobileUrl').value.trim() : '';\n" \
"      const altUrl = byId('altUrl') ? byId('altUrl').value.trim() : '';\n" \
"      const altWeight = byId('altWeight') ? Number(byId('altWeight').value || 0) : 0;\n" \
"      const geoCountry = byId('geoCountry') ? byId('geoCountry').value.trim().toUpperCase() : '';\n" \
"      const geoUrl = byId('geoUrl') ? byId('geoUrl').value.trim() : '';\n" \
"      const redirectCode = byId('redirectCode') ? Number(byId('redirectCode').value || 302) : 302;\n" \
"      const interstitial = byId('interstitial') ? byId('interstitial').value === '1' : false;\n" \
"      const utmSource = byId('utmSource') ? byId('utmSource').value.trim() : '';\n" \
"      const utmMedium = byId('utmMedium') ? byId('utmMedium').value.trim() : '';\n" \
"      const utmCampaign = byId('utmCampaign') ? byId('utmCampaign').value.trim() : '';\n" \
"      const password = byId('password').value;\n" \
"      const ttlText = byId('ttl').value;\n" \
"      const maxClicksText = byId('maxClicks') ? byId('maxClicks').value : '';\n" \
"      const activatesText = byId('activatesIn') ? byId('activatesIn').value : '';\n" \
"      if (!url) {\n" \
"        showMessage('Enter a valid HTTP or HTTPS URL first.', 'error');\n" \
"        byId('url').focus();\n" \
"        return;\n" \
"      }\n" \
"      if (!apiKey) {\n" \
"        options.classList.add('open');\n" \
"        toggle.setAttribute('aria-expanded', 'true');\n" \
"        toggle.textContent = 'Hide options';\n" \
"        showMessage('An API key or Scoped Creator Token (csh_...) is required to create a link.', 'error');\n" \
"        byId('apiKey').focus();\n" \
"        return;\n" \
"      }\n" \
"      const payload = { url };\n" \
"      if (slug) payload.custom_slug = slug;\n" \
"      if (title) payload.title = title;\n" \
"      if (tags) payload.tags = tags;\n" \
"      if (mobileUrl) payload.mobile_url = mobileUrl;\n" \
"      if (altUrl && altWeight > 0) { payload.alt_url = altUrl; payload.alt_weight = altWeight; }\n" \
"      if (geoCountry && geoUrl) { payload.geo_country = geoCountry; payload.geo_url = geoUrl; }\n" \
"      if (redirectCode !== 302) payload.redirect_code = redirectCode;\n" \
"      if (interstitial) payload.interstitial = true;\n" \
"      if (utmSource) payload.utm_source = utmSource;\n" \
"      if (utmMedium) payload.utm_medium = utmMedium;\n" \
"      if (utmCampaign) payload.utm_campaign = utmCampaign;\n" \
"      if (password) payload.password = password;\n" \
"      if (ttlText) payload.ttl_hours = Number(ttlText);\n" \
"      if (maxClicksText) payload.max_clicks = Number(maxClicksText);\n" \
"      if (activatesText) payload.activates_in_hours = Number(activatesText);\n" \
"      button.disabled = true;\n" \
"      button.textContent = 'Shortening...';\n" \
"      try {\n" \
"        const response = await fetch('/shorten', {\n" \
"          method: 'POST',\n" \
"          headers: { 'Content-Type': 'application/json', 'X-API-Key': apiKey },\n" \
"          body: JSON.stringify(payload),\n" \
"          cache: 'no-store'\n" \
"        });\n" \
"        let data = null;\n" \
"        try { data = await response.json(); } catch (error) {}\n" \
"        if (!response.ok || !data || typeof data.short_url !== 'string') {\n" \
"          showMessage(data && typeof data.error === 'string' ? data.error : 'Could not shorten this URL.', 'error');\n" \
"          return;\n" \
"        }\n" \
"        showLink(data);\n" \
"        byId('url').value = '';\n" \
"        byId('slug').value = '';\n" \
"        if (byId('title')) byId('title').value = '';\n" \
"        if (byId('tags')) byId('tags').value = '';\n" \
"        if (byId('mobileUrl')) byId('mobileUrl').value = '';\n" \
"        if (byId('altUrl')) byId('altUrl').value = '';\n" \
"        if (byId('altWeight')) byId('altWeight').value = '';\n" \
"        if (byId('utmSource')) byId('utmSource').value = '';\n" \
"        if (byId('utmMedium')) byId('utmMedium').value = '';\n" \
"        if (byId('utmCampaign')) byId('utmCampaign').value = '';\n" \
"        byId('password').value = '';\n" \
"        byId('ttl').value = '';\n" \
"        if (byId('maxClicks')) byId('maxClicks').value = '';\n" \
"        if (byId('activatesIn')) byId('activatesIn').value = '';\n" \
"      } catch (error) {\n" \
"        showMessage('Network error. Please try again.', 'error');\n" \
"      } finally {\n" \
"        button.disabled = false;\n" \
"        button.textContent = 'Shorten link';\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  // Bulk Shorten form on homepage\n" \
"  const bulkForm = byId('bulkForm');\n" \
"  if (bulkForm) {\n" \
"    bulkForm.addEventListener('submit', async (ev) => {\n" \
"      ev.preventDefault();\n" \
"      const key = byId('bulkApiKey').value.trim();\n" \
"      const rawUrls = byId('bulkUrls').value.split(/\\r?\\n/).map((s) => s.trim()).filter(Boolean);\n" \
"      const tag = byId('bulkTags') ? byId('bulkTags').value.trim() : '';\n" \
"      const ttl = byId('bulkTtl') ? Number(byId('bulkTtl').value || 0) : 0;\n" \
"      const box = byId('bulkResult');\n" \
"      const btn = byId('bulkSubmitBtn');\n" \
"      if (!key || !rawUrls.length || !box) return;\n" \
"      const items = rawUrls.slice(0, 25).map((u) => {\n" \
"        const item = { url: u };\n" \
"        if (tag) item.tags = tag;\n" \
"        if (ttl > 0) item.ttl_hours = ttl;\n" \
"        return item;\n" \
"      });\n" \
"      btn.disabled = true;\n" \
"      box.hidden = false;\n" \
"      box.textContent = 'Shortening ' + items.length + ' URL(s)...';\n" \
"      try {\n" \
"        const res = await fetch('/api/bulk/shorten', {\n" \
"          method: 'POST',\n" \
"          headers: { 'Content-Type': 'application/json', 'X-API-Key': key },\n" \
"          body: JSON.stringify({ links: items })\n" \
"        });\n" \
"        const data = await res.json();\n" \
"        if (!res.ok || !data || !Array.isArray(data.results)) {\n" \
"          box.textContent = (data && data.error) ? data.error : 'Bulk shorten failed.';\n" \
"          return;\n" \
"        }\n" \
"        box.replaceChildren();\n" \
"        const summary = document.createElement('p');\n" \
"        summary.className = 'success';\n" \
"        summary.textContent = 'Created ' + (data.created || 0) + ' link(s)' + (data.failed ? ' (' + data.failed + ' failed)' : '');\n" \
"        box.append(summary);\n" \
"        if (tag && /^[A-Za-z0-9_-]{1,32}$/.test(tag)) {\n" \
"          const bioRow = document.createElement('div');\n" \
"          bioRow.className = 'list-row';\n" \
"          const bioLabel = document.createElement('span');\n" \
"          bioLabel.textContent = 'Public Link-in-Bio Hub:';\n" \
"          const bioLink = document.createElement('a');\n" \
"          bioLink.href = '/bio/' + encodeURIComponent(tag);\n" \
"          bioLink.target = '_blank';\n" \
"          bioLink.textContent = '/bio/' + tag;\n" \
"          bioRow.append(bioLabel, bioLink);\n" \
"          box.append(bioRow);\n" \
"        }\n" \
"        for (const r of data.results) {\n" \
"          const row = document.createElement('div');\n" \
"          row.className = 'list-row';\n" \
"          if (r.short_url) {\n" \
"            const a = document.createElement('a');\n" \
"            a.href = r.short_url;\n" \
"            a.target = '_blank';\n" \
"            a.textContent = r.short_url;\n" \
"            const dest = document.createElement('span');\n" \
"            dest.className = 'hint';\n" \
"            dest.textContent = r.url || '';\n" \
"            row.append(a, dest);\n" \
"          } else {\n" \
"            const errSpan = document.createElement('span');\n" \
"            errSpan.className = 'error';\n" \
"            errSpan.textContent = r.error || 'Error';\n" \
"            row.append(errSpan);\n" \
"          }\n" \
"          box.append(row);\n" \
"        }\n" \
"      } catch (e) {\n" \
"        box.textContent = 'Network error during bulk shorten.';\n" \
"      } finally {\n" \
"        btn.disabled = false;\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  // Link Inspector form on homepage\n" \
"  const inspectForm = byId('inspectForm');\n" \
"  if (inspectForm) {\n" \
"    inspectForm.addEventListener('submit', async (ev) => {\n" \
"      ev.preventDefault();\n" \
"      const raw = byId('inspectInput').value.trim();\n" \
"      const box = byId('inspectResult');\n" \
"      if (!raw || !box) return;\n" \
"      let slug = raw;\n" \
"      try {\n" \
"        if (raw.includes('/')) {\n" \
"          const u = new URL(raw, window.location.origin);\n" \
"          const parts = u.pathname.split('/').filter(Boolean);\n" \
"          slug = parts[parts.length - 1] || '';\n" \
"        }\n" \
"      } catch (e) {}\n" \
"      box.hidden = false;\n" \
"      box.textContent = 'Inspecting /' + slug + '...';\n" \
"      try {\n" \
"        const res = await fetch('/api/inspect/' + encodeURIComponent(slug), { cache: 'no-store' });\n" \
"        const data = await res.json();\n" \
"        if (!res.ok) {\n" \
"          box.textContent = data && data.error ? data.error : 'Link not found.';\n" \
"          return;\n" \
"        }\n" \
"        box.replaceChildren();\n" \
"        const addRow = (k, v) => {\n" \
"          const r = document.createElement('div');\n" \
"          r.className = 'list-row';\n" \
"          const s1 = document.createElement('span'); s1.className = 'hint'; s1.textContent = k;\n" \
"          const s2 = document.createElement('strong'); s2.textContent = v;\n" \
"          r.append(s1, s2);\n" \
"          box.append(r);\n" \
"        };\n" \
"        addRow('Slug', '/' + data.slug);\n" \
"        addRow('Status', String(data.status || 'active').toUpperCase());\n" \
"        addRow('Password Protected', data.has_password ? 'Yes (Target hidden)' : 'No');\n" \
"        if (data.target_url) addRow('Destination URL', data.target_url);\n" \
"        addRow('Total Visits', String(data.total_visits || 0));\n" \
"        if (data.max_clicks) addRow('Max Clicks Limit', String(data.max_clicks));\n" \
"        if (data.expires_at) addRow('Expires At (UTC)', data.expires_at);\n" \
"      } catch (e) {\n" \
"        box.textContent = 'Could not inspect link.';\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"})();\n"

#define PASSWORD_JS \
"(() => {\n" \
"  'use strict';\n" \
"  const form = document.getElementById('unlockForm');\n" \
"  const input = document.getElementById('pwd');\n" \
"  const btn = document.getElementById('btn');\n" \
"  const err = document.getElementById('err');\n" \
"  if (!form) return;\n" \
"  form.addEventListener('submit', async (event) => {\n" \
"    event.preventDefault();\n" \
"    const pwd = input.value;\n" \
"    const slug = window.location.pathname.replace(/^\\/+/, '').split('/')[0];\n" \
"    if (!pwd) { err.textContent = 'Please enter the link password.'; return; }\n" \
"    btn.disabled = true;\n" \
"    btn.textContent = 'Unlocking...';\n" \
"    err.textContent = '';\n" \
"    try {\n" \
"      const res = await fetch('/unlock/' + encodeURIComponent(slug), {\n" \
"        method: 'POST',\n" \
"        headers: { 'Content-Type': 'application/json' },\n" \
"        body: JSON.stringify({ password: pwd })\n" \
"      });\n" \
"      const data = await res.json();\n" \
"      if (res.ok && data && typeof data.url === 'string') {\n" \
"        window.location.href = data.url;\n" \
"      } else {\n" \
"        err.textContent = (data && data.error) ? data.error : 'Unlock failed.';\n" \
"      }\n" \
"    } catch (e) {\n" \
"      err.textContent = 'Network error. Please try again.';\n" \
"    } finally {\n" \
"      btn.disabled = false;\n" \
"      btn.textContent = 'Unlock & Continue';\n" \
"    }\n" \
"  });\n" \
"})();\n"

#define STATS_JS \
"(() => {\n" \
"  'use strict';\n" \
"  const byId = (id) => document.getElementById(id);\n" \
"  const parts = window.location.pathname.split('/').filter(Boolean);\n" \
"  const slug = parts[parts.length - 1] || '';\n" \
"  if (!slug) return;\n" \
"\n" \
"  async function loadDetailedStats() {\n" \
"    try {\n" \
"      const res = await fetch('/api/stats/' + encodeURIComponent(slug), { cache: 'no-store' });\n" \
"      if (!res.ok) return;\n" \
"      const data = await res.json();\n" \
"      const meta = byId('statsMeta');\n" \
"      if (meta) {\n" \
"        meta.hidden = false;\n" \
"        meta.replaceChildren();\n" \
"        const addBadge = (txt, cls) => {\n" \
"          const b = document.createElement('span');\n" \
"          b.className = 'badge ' + (cls || '');\n" \
"          b.textContent = txt;\n" \
"          meta.append(b);\n" \
"        };\n" \
"        addBadge('Status: ' + String(data.status || 'active').toUpperCase(), data.status === 'active' ? 'ok' : 'exp');\n" \
"        if (data.target_domain) addBadge('Host: ' + data.target_domain, 'tag');\n" \
"        if (data.has_password) addBadge('Password Protected', '');\n" \
"        if (data.max_clicks > 0) addBadge('Click Limit: ' + data.total_visits + ' / ' + data.max_clicks, '');\n" \
"        if (data.expires_at) addBadge('Expires: ' + data.expires_at.slice(0, 16), 'exp');\n" \
"      }\n" \
"      const dailyBox = byId('dailyList');\n" \
"      if (dailyBox && Array.isArray(data.daily) && data.daily.length) {\n" \
"        dailyBox.replaceChildren();\n" \
"        let maxV = 1;\n" \
"        for (const d of data.daily) { if (d.visits > maxV) maxV = d.visits; }\n" \
"        for (const d of data.daily) {\n" \
"          const wrap = document.createElement('div');\n" \
"          wrap.className = 'field';\n" \
"          const row = document.createElement('div');\n" \
"          row.className = 'list-row';\n" \
"          const sDay = document.createElement('span'); sDay.textContent = d.day;\n" \
"          const sVal = document.createElement('strong'); sVal.textContent = d.visits + ' visits (' + d.unique + ' unique)';\n" \
"          row.append(sDay, sVal);\n" \
"          const prog = document.createElement('progress');\n" \
"          prog.max = maxV;\n" \
"          prog.value = d.visits;\n" \
"          wrap.append(row, prog);\n" \
"          dailyBox.append(wrap);\n" \
"        }\n" \
"      }\n" \
"      const devBox = byId('deviceList');\n" \
"      if (devBox && data.devices) {\n" \
"        devBox.replaceChildren();\n" \
"        const entries = Object.entries(data.devices);\n" \
"        if (entries.length) {\n" \
"          for (const [k, v] of entries) {\n" \
"            const r = document.createElement('div'); r.className = 'list-row';\n" \
"            const s1 = document.createElement('span'); s1.textContent = k.toUpperCase();\n" \
"            const s2 = document.createElement('strong'); s2.textContent = String(v);\n" \
"            r.append(s1, s2);\n" \
"            devBox.append(r);\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"      const refBox = byId('referrerList');\n" \
"      if (refBox && Array.isArray(data.referrers) && data.referrers.length) {\n" \
"        refBox.replaceChildren();\n" \
"        for (const ref of data.referrers) {\n" \
"          const r = document.createElement('div'); r.className = 'list-row';\n" \
"          const s1 = document.createElement('span'); s1.textContent = ref.domain;\n" \
"          const s2 = document.createElement('strong'); s2.textContent = String(ref.visits);\n" \
"          r.append(s1, s2);\n" \
"          refBox.append(r);\n" \
"        }\n" \
"      }\n" \
"      const ccBox = byId('countryList');\n" \
"      if (ccBox && Array.isArray(data.countries) && data.countries.length) {\n" \
"        ccBox.replaceChildren();\n" \
"        for (const c of data.countries) {\n" \
"          const r = document.createElement('div'); r.className = 'list-row';\n" \
"          const s1 = document.createElement('span'); s1.textContent = c.country;\n" \
"          const s2 = document.createElement('strong'); s2.textContent = String(c.visits);\n" \
"          r.append(s1, s2);\n" \
"          ccBox.append(r);\n" \
"        }\n" \
"      }\n" \
"      const hourBox = byId('hourlyList');\n" \
"      if (hourBox && Array.isArray(data.hourly) && data.hourly.length === 24) {\n" \
"        let maxH = 0;\n" \
"        for (const h of data.hourly) { if (h > maxH) maxH = h; }\n" \
"        if (maxH > 0) {\n" \
"          hourBox.replaceChildren();\n" \
"          for (let h = 0; h < 24; h++) {\n" \
"            const cnt = data.hourly[h] || 0;\n" \
"            if (cnt === 0) continue;\n" \
"            const wrap = document.createElement('div');\n" \
"            wrap.className = 'field';\n" \
"            const row = document.createElement('div');\n" \
"            row.className = 'list-row';\n" \
"            const sHr = document.createElement('span');\n" \
"            sHr.textContent = String(h).padStart(2, '0') + ':00 - ' + String(h).padStart(2, '0') + ':59 UTC';\n" \
"            const sCnt = document.createElement('strong');\n" \
"            sCnt.textContent = cnt + ' visits';\n" \
"            row.append(sHr, sCnt);\n" \
"            const prog = document.createElement('progress');\n" \
"            prog.max = maxH;\n" \
"            prog.value = cnt;\n" \
"            wrap.append(row, prog);\n" \
"            hourBox.append(wrap);\n" \
"          }\n" \
"        }\n" \
"      }\n" \
"      const badgeBox = byId('badgePreviewBox');\n" \
"      if (badgeBox) {\n" \
"        badgeBox.replaceChildren();\n" \
"        const h2 = document.createElement('h2');\n" \
"        h2.textContent = 'Embeddable SVG Analytics Badge';\n" \
"        const row = document.createElement('div');\n" \
"        row.className = 'list-row';\n" \
"        const img = document.createElement('img');\n" \
"        img.src = '/badge/' + encodeURIComponent(slug) + '.svg';\n" \
"        img.alt = 'Visits badge';\n" \
"        const copyMd = document.createElement('button');\n" \
"        copyMd.type = 'button';\n" \
"        copyMd.className = 'btn btn-sm';\n" \
"        copyMd.textContent = 'Copy Markdown Badge';\n" \
"        copyMd.addEventListener('click', async () => {\n" \
"          const md = '[![Visits](' + window.location.origin + '/badge/' + slug + '.svg)](' + window.location.origin + '/' + slug + ')';\n" \
"          try { await navigator.clipboard.writeText(md); copyMd.textContent = 'Copied Markdown!'; } catch (e) {}\n" \
"        });\n" \
"        const csvLink = document.createElement('a');\n" \
"        csvLink.className = 'btn btn-sm';\n" \
"        csvLink.href = '/api/stats/' + encodeURIComponent(slug) + '/export.csv';\n" \
"        csvLink.textContent = 'Export Analytics CSV';\n" \
"        const actWrap = document.createElement('div');\n" \
"        actWrap.className = 'actions';\n" \
"        actWrap.append(copyMd, csvLink);\n" \
"        row.append(img, actWrap);\n" \
"        badgeBox.append(h2, row);\n" \
"      }\n" \
"    } catch (e) {}\n" \
"  }\n" \
"  loadDetailedStats();\n" \
"})();\n"

#define ADMIN_JS \
"(() => {\n" \
"  'use strict';\n" \
"  let apiKey = '';\n" \
"  let cachedLinks = [];\n" \
"  const byId = (id) => document.getElementById(id);\n" \
"  const authPanel = byId('authPanel'), dashboard = byId('dashboard');\n" \
"  const authStatus = byId('authStatus'), dashboardStatus = byId('dashboardStatus');\n" \
"  const rows = byId('linkRows'), tokenRows = byId('tokenRows'), blacklistRows = byId('blacklistRows'), auditRows = byId('auditRows');\n" \
"  const text = (v) => typeof v === 'string' ? v : '';\n" \
"  const count = (v) => Number.isSafeInteger(v) && v >= 0 ? v : 0;\n" \
"\n" \
"  function setStatus(el, msg) { if (el) el.textContent = msg || ''; }\n" \
"  function headers(json) {\n" \
"    const h = apiKey ? { 'X-API-Key': apiKey } : {};\n" \
"    if (json) h['Content-Type'] = 'application/json';\n" \
"    return h;\n" \
"  }\n" \
"\n" \
"  // Admin tabs\n" \
"  const tabButtons = document.querySelectorAll('[data-admin-tab]');\n" \
"  tabButtons.forEach((btn) => {\n" \
"    btn.addEventListener('click', () => {\n" \
"      const target = btn.getAttribute('data-admin-tab');\n" \
"      tabButtons.forEach((b) => b.classList.toggle('active', b === btn));\n" \
"      document.querySelectorAll('[data-admin-pane]').forEach((pane) => {\n" \
"        pane.hidden = pane.getAttribute('data-admin-pane') !== target;\n" \
"      });\n" \
"      if (target === 'tokens') loadTokens();\n" \
"      if (target === 'blacklist') loadBlacklist();\n" \
"      if (target === 'audit') loadAudit();\n" \
"      if (target === 'system') loadSystem();\n" \
"    });\n" \
"  });\n" \
"\n" \
"  function emptyRow(tbody, cols, msg) {\n" \
"    if (!tbody) return;\n" \
"    const tr = document.createElement('tr'), td = document.createElement('td');\n" \
"    td.colSpan = cols;\n" \
"    td.className = 'empty';\n" \
"    td.textContent = msg;\n" \
"    tr.append(td);\n" \
"    tbody.replaceChildren(tr);\n" \
"  }\n" \
"\n" \
"  function cell(val, cls) {\n" \
"    const td = document.createElement('td');\n" \
"    if (cls) td.className = cls;\n" \
"    td.textContent = val;\n" \
"    return td;\n" \
"  }\n" \
"\n" \
"  function flagsCell(link) {\n" \
"    const td = document.createElement('td');\n" \
"    const addBadge = (txt, cls) => {\n" \
"      const b = document.createElement('span');\n" \
"      b.className = 'badge ' + (cls || '');\n" \
"      b.textContent = txt;\n" \
"      td.append(b);\n" \
"    };\n" \
"    if (link.is_active === false) addBadge('Disabled', 'danger');\n" \
"    else if (link.is_expired) addBadge('Expired', 'exp');\n" \
"    else addBadge('Active', 'ok');\n" \
"    if (link.has_password) addBadge('Password', '');\n" \
"    if (text(link.mobile_url)) addBadge('Mobile Route', 'tag');\n" \
"    if (text(link.alt_url) && count(link.alt_weight) > 0) addBadge('A/B ' + count(link.alt_weight) + '%', 'tag');\n" \
"    if (count(link.max_clicks) > 0) addBadge('Max ' + count(link.max_clicks), '');\n" \
"    if (text(link.expires_at)) addBadge('Exp ' + text(link.expires_at).slice(0, 10), 'exp');\n" \
"    if (text(link.tags)) {\n" \
"      for (const t of text(link.tags).split(',').filter(Boolean)) addBadge('#' + t, 'tag');\n" \
"    }\n" \
"    return td;\n" \
"  }\n" \
"\n" \
"  function slugCell(link) {\n" \
"    const slug = text(link.slug);\n" \
"    const td = document.createElement('td');\n" \
"    const a = document.createElement('a');\n" \
"    a.href = '/' + encodeURIComponent(slug);\n" \
"    a.target = '_blank';\n" \
"    a.rel = 'noopener noreferrer';\n" \
"    a.textContent = '/' + slug;\n" \
"    td.append(a);\n" \
"    if (text(link.title)) {\n" \
"      const sub = document.createElement('div');\n" \
"      sub.className = 'hint';\n" \
"      sub.textContent = text(link.title);\n" \
"      td.append(sub);\n" \
"    }\n" \
"    return td;\n" \
"  }\n" \
"\n" \
"  async function removeLink(slug, btn) {\n" \
"    if (!window.confirm('Delete /' + slug + ' and all its analytics?')) return;\n" \
"    btn.disabled = true;\n" \
"    try {\n" \
"      const res = await fetch('/' + encodeURIComponent(slug), { method: 'DELETE', headers: headers(false) });\n" \
"      if (!res.ok) throw new Error('Delete failed');\n" \
"      await loadLinks();\n" \
"    } catch (e) {\n" \
"      setStatus(dashboardStatus, 'Could not delete /' + slug);\n" \
"      btn.disabled = false;\n" \
"    }\n" \
"  }\n" \
"\n" \
"  function openEditModal(link) {\n" \
"    const modal = byId('editModal');\n" \
"    if (!modal) return;\n" \
"    byId('editSlug').value = text(link.slug);\n" \
"    byId('editModalTitle').textContent = 'Edit /' + text(link.slug);\n" \
"    byId('editUrl').value = text(link.url);\n" \
"    if (byId('editMobileUrl')) byId('editMobileUrl').value = text(link.mobile_url);\n" \
"    byId('editTitle').value = text(link.title);\n" \
"    byId('editTags').value = text(link.tags);\n" \
"    byId('editMaxClicks').value = String(count(link.max_clicks));\n" \
"    byId('editTtl').value = '-1';\n" \
"    byId('editActive').value = link.is_active === false ? '0' : '1';\n" \
"    byId('editPassword').value = '';\n" \
"    modal.hidden = false;\n" \
"  }\n" \
"\n" \
"  function renderLinks(links) {\n" \
"    cachedLinks = links;\n" \
"    let totalVisits = 0, totalUnique = 0, activeCount = 0;\n" \
"    byId('totalLinks').textContent = String(links.length);\n" \
"    const frag = document.createDocumentFragment();\n" \
"    for (const link of links) {\n" \
"      const slug = text(link.slug);\n" \
"      if (!slug) continue;\n" \
"      const visits = count(link.total_visits), unique = count(link.unique_visitors);\n" \
"      totalVisits += visits;\n" \
"      totalUnique += unique;\n" \
"      if (link.is_active !== false && !link.is_expired) activeCount++;\n" \
"      const tr = document.createElement('tr');\n" \
"      tr.append(\n" \
"        slugCell(link),\n" \
"        cell(text(link.url), 'url-cell'),\n" \
"        cell(text(link.created_at).slice(0, 16) || '-'),\n" \
"        flagsCell(link),\n" \
"        cell(visits + ' / ' + unique)\n" \
"      );\n" \
"      const actionTd = document.createElement('td');\n" \
"      const actionWrap = document.createElement('div');\n" \
"      actionWrap.className = 'actions';\n" \
"      const editBtn = document.createElement('button');\n" \
"      editBtn.type = 'button';\n" \
"      editBtn.className = 'btn-sm';\n" \
"      editBtn.textContent = 'Edit';\n" \
"      editBtn.addEventListener('click', () => openEditModal(link));\n" \
"      const statsA = document.createElement('a');\n" \
"      statsA.className = 'btn btn-sm';\n" \
"      statsA.href = '/stats/' + encodeURIComponent(slug);\n" \
"      statsA.target = '_blank';\n" \
"      statsA.textContent = 'Stats';\n" \
"      const qrA = document.createElement('a');\n" \
"      qrA.className = 'btn btn-sm';\n" \
"      qrA.href = '/qr/' + encodeURIComponent(slug) + '.svg';\n" \
"      qrA.target = '_blank';\n" \
"      qrA.textContent = 'QR';\n" \
"      const delBtn = document.createElement('button');\n" \
"      delBtn.type = 'button';\n" \
"      delBtn.className = 'btn-sm danger';\n" \
"      delBtn.textContent = 'Delete';\n" \
"      delBtn.addEventListener('click', () => removeLink(slug, delBtn));\n" \
"      actionWrap.append(editBtn, statsA, qrA, delBtn);\n" \
"      actionTd.append(actionWrap);\n" \
"      tr.append(actionTd);\n" \
"      frag.append(tr);\n" \
"    }\n" \
"    if (byId('activeLinks')) byId('activeLinks').textContent = String(activeCount);\n" \
"    byId('totalVisits').textContent = String(totalVisits);\n" \
"    byId('totalUnique').textContent = String(totalUnique);\n" \
"    if (frag.childNodes.length) rows.replaceChildren(frag);\n" \
"    else emptyRow(rows, 6, 'No matching links found.');\n" \
"  }\n" \
"\n" \
"  async function loadLinks() {\n" \
"    if (!apiKey) return;\n" \
"    setStatus(dashboardStatus, 'Loading links...');\n" \
"    const q = byId('filterSearch') ? byId('filterSearch').value.trim() : '';\n" \
"    const tag = byId('filterTag') ? byId('filterTag').value.trim() : '';\n" \
"    const status = byId('filterStatus') ? byId('filterStatus').value : '';\n" \
"    const params = new URLSearchParams();\n" \
"    if (q) params.set('q', q);\n" \
"    if (tag) params.set('tag', tag);\n" \
"    if (status) params.set('status', status);\n" \
"    const qs = params.toString();\n" \
"    try {\n" \
"      const res = await fetch('/api/admin/links' + (qs ? '?' + qs : ''), { headers: headers(false), cache: 'no-store' });\n" \
"      if (res.status === 401) {\n" \
"        apiKey = '';\n" \
"        byId('apiKey').value = '';\n" \
"        dashboard.hidden = true;\n" \
"        authPanel.hidden = false;\n" \
"        setStatus(authStatus, 'Invalid Admin API key.');\n" \
"        return;\n" \
"      }\n" \
"      if (!res.ok) throw new Error('Load failed');\n" \
"      const links = await res.json();\n" \
"      if (!Array.isArray(links)) throw new Error('Invalid response');\n" \
"      renderLinks(links);\n" \
"      setStatus(dashboardStatus, '');\n" \
"    } catch (e) {\n" \
"      emptyRow(rows, 6, 'Could not load links.');\n" \
"      setStatus(dashboardStatus, 'Error loading links.');\n" \
"    }\n" \
"  }\n" \
"\n" \
"  async function loadTokens() {\n" \
"    if (!apiKey || !tokenRows) return;\n" \
"    emptyRow(tokenRows, 6, 'Loading tokens...');\n" \
"    try {\n" \
"      const res = await fetch('/api/admin/tokens', { headers: headers(false), cache: 'no-store' });\n" \
"      if (!res.ok) throw new Error('Failed');\n" \
"      const list = await res.json();\n" \
"      const frag = document.createDocumentFragment();\n" \
"      for (const tok of list) {\n" \
"        const tr = document.createElement('tr');\n" \
"        tr.append(\n" \
"          cell(text(tok.name)),\n" \
"          cell(text(tok.token_prefix) + '...'),\n" \
"          cell(text(tok.created_at).slice(0, 16)),\n" \
"          cell(text(tok.expires_at).slice(0, 10) || 'Never'),\n" \
"          cell(String(count(tok.usage_count)) + (tok.last_used_at ? ' (' + text(tok.last_used_at).slice(0, 16) + ')' : ''))\n" \
"        );\n" \
"        const td = document.createElement('td');\n" \
"        const btn = document.createElement('button');\n" \
"        btn.type = 'button';\n" \
"        btn.className = 'btn-sm danger';\n" \
"        btn.textContent = 'Revoke';\n" \
"        btn.addEventListener('click', async () => {\n" \
"          if (!window.confirm('Revoke token ' + text(tok.name) + '?')) return;\n" \
"          await fetch('/api/admin/tokens/' + encodeURIComponent(String(tok.id)), { method: 'DELETE', headers: headers(false) });\n" \
"          loadTokens();\n" \
"        });\n" \
"        td.append(btn);\n" \
"        tr.append(td);\n" \
"        frag.append(tr);\n" \
"      }\n" \
"      if (frag.childNodes.length) tokenRows.replaceChildren(frag);\n" \
"      else emptyRow(tokenRows, 6, 'No scoped creator tokens created yet.');\n" \
"    } catch (e) {\n" \
"      emptyRow(tokenRows, 6, 'Could not load tokens.');\n" \
"    }\n" \
"  }\n" \
"\n" \
"  async function loadAudit() {\n" \
"    if (!apiKey || !auditRows) return;\n" \
"    emptyRow(auditRows, 5, 'Loading audit events...');\n" \
"    try {\n" \
"      const res = await fetch('/api/admin/audit', { headers: headers(false), cache: 'no-store' });\n" \
"      if (!res.ok) throw new Error('Failed');\n" \
"      const list = await res.json();\n" \
"      const frag = document.createDocumentFragment();\n" \
"      for (const item of list) {\n" \
"        const tr = document.createElement('tr');\n" \
"        tr.append(\n" \
"          cell(text(item.created_at)),\n" \
"          cell(text(item.event_type)),\n" \
"          cell(text(item.actor)),\n" \
"          cell(text(item.target) || '-'),\n" \
"          cell(text(item.details) || '-')\n" \
"        );\n" \
"        frag.append(tr);\n" \
"      }\n" \
"      if (frag.childNodes.length) auditRows.replaceChildren(frag);\n" \
"      else emptyRow(auditRows, 5, 'No audit events recorded yet.');\n" \
"    } catch (e) {\n" \
"      emptyRow(auditRows, 5, 'Could not load audit log.');\n" \
"    }\n" \
"  }\n" \
"\n" \
"  async function loadBlacklist() {\n" \
"    if (!apiKey || !blacklistRows) return;\n" \
"    emptyRow(blacklistRows, 4, 'Loading domain blacklist...');\n" \
"    try {\n" \
"      const res = await fetch('/api/admin/blacklist', { headers: headers(false), cache: 'no-store' });\n" \
"      if (!res.ok) throw new Error('Failed');\n" \
"      const data = await res.json();\n" \
"      const list = Array.isArray(data.blacklist) ? data.blacklist : [];\n" \
"      const frag = document.createDocumentFragment();\n" \
"      for (const item of list) {\n" \
"        const tr = document.createElement('tr');\n" \
"        tr.append(\n" \
"          cell(text(item.pattern)),\n" \
"          cell(text(item.reason) || '-'),\n" \
"          cell(text(item.created_at))\n" \
"        );\n" \
"        const td = document.createElement('td');\n" \
"        const btn = document.createElement('button');\n" \
"        btn.type = 'button';\n" \
"        btn.className = 'danger small';\n" \
"        btn.textContent = 'Unblock';\n" \
"        btn.addEventListener('click', async () => {\n" \
"          if (!confirm('Unblock pattern ' + item.pattern + '?')) return;\n" \
"          try {\n" \
"            const delRes = await fetch('/api/admin/blacklist/' + encodeURIComponent(item.pattern), {\n" \
"              method: 'DELETE',\n" \
"              headers: headers(false)\n" \
"            });\n" \
"            if (delRes.ok) loadBlacklist();\n" \
"          } catch (_) {}\n" \
"        });\n" \
"        td.append(btn);\n" \
"        tr.append(td);\n" \
"        frag.append(tr);\n" \
"      }\n" \
"      if (frag.childNodes.length) blacklistRows.replaceChildren(frag);\n" \
"      else emptyRow(blacklistRows, 4, 'No domains currently blacklisted.');\n" \
"    } catch (e) {\n" \
"      emptyRow(blacklistRows, 4, 'Could not load domain blacklist.');\n" \
"    }\n" \
"  }\n" \
"\n" \
"  async function loadSystem() {\n" \
"    if (!apiKey) return;\n" \
"    const grid = byId('systemGrid'), prom = byId('prometheusBox');\n" \
"    try {\n" \
"      const [sysRes, promRes] = await Promise.all([\n" \
"        fetch('/api/admin/system', { headers: headers(false), cache: 'no-store' }),\n" \
"        fetch('/metrics', { headers: headers(false), cache: 'no-store' })\n" \
"      ]);\n" \
"      if (sysRes.ok && grid) {\n" \
"        const sys = await sysRes.json();\n" \
"        grid.replaceChildren();\n" \
"        const addRow = (k, v) => {\n" \
"          const r = document.createElement('div');\n" \
"          r.className = 'list-row';\n" \
"          const s1 = document.createElement('span'); s1.className = 'hint'; s1.textContent = k;\n" \
"          const s2 = document.createElement('strong'); s2.textContent = String(v);\n" \
"          r.append(s1, s2);\n" \
"          grid.append(r);\n" \
"        };\n" \
"        addRow('C Standard', text(sys.c_standard));\n" \
"        addRow('Hardening Profile', text(sys.hardening));\n" \
"        addRow('SQLite Version', text(sys.sqlite_version));\n" \
"        addRow('Uptime (seconds)', count(sys.uptime_seconds));\n" \
"        addRow('Database Size (bytes)', count(sys.db_size_bytes));\n" \
"        addRow('Total Links / Active / Expired', count(sys.total_links) + ' / ' + count(sys.active_links) + ' / ' + count(sys.expired_links));\n" \
"        addRow('Total Visits / Unique Visitors', count(sys.total_visits) + ' / ' + count(sys.unique_visitors));\n" \
"        addRow('Scoped Tokens / Audit Events', count(sys.active_tokens) + ' / ' + count(sys.audit_events));\n" \
"      }\n" \
"      if (promRes.ok && prom) {\n" \
"        prom.textContent = await promRes.text();\n" \
"      }\n" \
"    } catch (e) {\n" \
"      if (prom) prom.textContent = 'Could not load system metrics.';\n" \
"    }\n" \
"  }\n" \
"\n" \
"  byId('authForm').addEventListener('submit', (ev) => {\n" \
"    ev.preventDefault();\n" \
"    apiKey = byId('apiKey').value.trim();\n" \
"    if (!apiKey) return;\n" \
"    authPanel.hidden = true;\n" \
"    dashboard.hidden = false;\n" \
"    setStatus(authStatus, '');\n" \
"    loadLinks();\n" \
"  });\n" \
"\n" \
"  byId('refresh').addEventListener('click', loadLinks);\n" \
"  if (byId('filterBtn')) byId('filterBtn').addEventListener('click', loadLinks);\n" \
"  byId('lock').addEventListener('click', () => {\n" \
"    apiKey = '';\n" \
"    cachedLinks = [];\n" \
"    byId('apiKey').value = '';\n" \
"    dashboard.hidden = true;\n" \
"    authPanel.hidden = false;\n" \
"    emptyRow(rows, 6, '');\n" \
"    setStatus(authStatus, 'Dashboard locked.');\n" \
"    byId('apiKey').focus();\n" \
"  });\n" \
"\n" \
"  if (byId('exportCsv')) {\n" \
"    byId('exportCsv').addEventListener('click', async () => {\n" \
"      if (!apiKey) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/export.csv', { headers: headers(false) });\n" \
"        if (!res.ok) throw new Error('Export failed');\n" \
"        const blob = await res.blob();\n" \
"        const url = URL.createObjectURL(blob);\n" \
"        const a = document.createElement('a');\n" \
"        a.href = url;\n" \
"        a.download = 'shortener-links.csv';\n" \
"        document.body.append(a);\n" \
"        a.click();\n" \
"        a.remove();\n" \
"        URL.revokeObjectURL(url);\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'CSV export failed.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('exportJson')) {\n" \
"    byId('exportJson').addEventListener('click', async () => {\n" \
"      if (!apiKey) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/backup.json', { headers: headers(false) });\n" \
"        if (!res.ok) throw new Error('Backup failed');\n" \
"        const blob = await res.blob();\n" \
"        const url = URL.createObjectURL(blob);\n" \
"        const a = document.createElement('a');\n" \
"        a.href = url;\n" \
"        a.download = 'shortener-backup.json';\n" \
"        document.body.append(a);\n" \
"        a.click();\n" \
"        a.remove();\n" \
"        URL.revokeObjectURL(url);\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'JSON backup export failed.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('importJsonBtn') && byId('importJsonFile')) {\n" \
"    byId('importJsonBtn').addEventListener('click', () => byId('importJsonFile').click());\n" \
"    byId('importJsonFile').addEventListener('change', async () => {\n" \
"      const file = byId('importJsonFile').files && byId('importJsonFile').files[0];\n" \
"      if (!file || !apiKey) return;\n" \
"      try {\n" \
"        const rawText = await file.text();\n" \
"        const res = await fetch('/api/admin/restore', {\n" \
"          method: 'POST',\n" \
"          headers: headers(true),\n" \
"          body: rawText\n" \
"        });\n" \
"        const data = await res.json();\n" \
"        if (res.ok && data) {\n" \
"          setStatus(dashboardStatus, 'Restored ' + count(data.restored) + ' link(s), skipped ' + count(data.skipped) + '.');\n" \
"          loadLinks();\n" \
"        } else {\n" \
"          setStatus(dashboardStatus, (data && data.error) ? data.error : 'JSON restore failed.');\n" \
"        }\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'Could not restore JSON snapshot.');\n" \
"      } finally {\n" \
"        byId('importJsonFile').value = '';\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('cleanupBtn')) {\n" \
"    byId('cleanupBtn').addEventListener('click', async () => {\n" \
"      if (!apiKey) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/cleanup', { method: 'POST', headers: headers(true), body: '{}' });\n" \
"        const data = await res.json();\n" \
"        setStatus(dashboardStatus, 'Cleaned ' + count(data && data.expired_removed) + ' expired link(s).');\n" \
"        loadLinks();\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'Cleanup request failed.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('blacklistForm')) {\n" \
"    byId('blacklistForm').addEventListener('submit', async (e) => {\n" \
"      e.preventDefault();\n" \
"      const pEl = byId('blacklistPattern'), rEl = byId('blacklistReason');\n" \
"      const pattern = (pEl ? pEl.value : '').trim();\n" \
"      const reason = (rEl ? rEl.value : '').trim();\n" \
"      if (!pattern || !apiKey) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/blacklist', {\n" \
"          method: 'POST',\n" \
"          headers: headers(true),\n" \
"          body: JSON.stringify({ pattern: pattern, reason: reason })\n" \
"        });\n" \
"        if (res.ok) {\n" \
"          if (pEl) pEl.value = '';\n" \
"          if (rEl) rEl.value = '';\n" \
"          setStatus(dashboardStatus, 'Domain pattern blocked: ' + pattern);\n" \
"          loadBlacklist();\n" \
"        } else {\n" \
"          setStatus(dashboardStatus, 'Could not block pattern. Check syntax or duplicate.');\n" \
"        }\n" \
"      } catch (err) {\n" \
"        setStatus(dashboardStatus, 'Network error blocking pattern.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('maintenanceBtn')) {\n" \
"    byId('maintenanceBtn').addEventListener('click', async () => {\n" \
"      if (!apiKey) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/maintenance', { method: 'POST', headers: headers(true), body: '{}' });\n" \
"        const data = await res.json();\n" \
"        if (res.ok && data) {\n" \
"          setStatus(dashboardStatus, 'SQLite optimized: purged ' + count(data.expired_purged) + ' expired links, ' + count(data.ratelimits_purged) + ' rate-limit windows, WAL checkpointed (' + count(data.db_size_bytes) + ' bytes).');\n" \
"          loadLinks();\n" \
"        } else {\n" \
"          setStatus(dashboardStatus, 'Maintenance failed.');\n" \
"        }\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'Maintenance request failed.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('editCancel')) {\n" \
"    byId('editCancel').addEventListener('click', () => { byId('editModal').hidden = true; });\n" \
"  }\n" \
"  if (byId('editForm')) {\n" \
"    byId('editForm').addEventListener('submit', async (ev) => {\n" \
"      ev.preventDefault();\n" \
"      const slug = byId('editSlug').value;\n" \
"      const payload = {\n" \
"        url: byId('editUrl').value.trim(),\n" \
"        mobile_url: byId('editMobileUrl') ? byId('editMobileUrl').value.trim() : '',\n" \
"        title: byId('editTitle').value.trim(),\n" \
"        tags: byId('editTags').value.trim(),\n" \
"        max_clicks: Number(byId('editMaxClicks').value || 0),\n" \
"        ttl_hours: Number(byId('editTtl').value),\n" \
"        is_active: byId('editActive').value === '1'\n" \
"      };\n" \
"      const newPwd = byId('editPassword').value;\n" \
"      if (newPwd === '__clear__') payload.password = '';\n" \
"      else if (newPwd) payload.password = newPwd;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/links/' + encodeURIComponent(slug), {\n" \
"          method: 'PUT',\n" \
"          headers: headers(true),\n" \
"          body: JSON.stringify(payload)\n" \
"        });\n" \
"        const data = await res.json();\n" \
"        if (!res.ok) {\n" \
"          setStatus(dashboardStatus, data && data.error ? data.error : 'Update failed.');\n" \
"          return;\n" \
"        }\n" \
"        byId('editModal').hidden = true;\n" \
"        setStatus(dashboardStatus, 'Updated /' + slug);\n" \
"        loadLinks();\n" \
"      } catch (e) {\n" \
"        setStatus(dashboardStatus, 'Could not update link.');\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"\n" \
"  if (byId('tokenForm')) {\n" \
"    byId('tokenForm').addEventListener('submit', async (ev) => {\n" \
"      ev.preventDefault();\n" \
"      const name = byId('tokenName').value.trim();\n" \
"      const ttl = Number(byId('tokenTtl').value || 0);\n" \
"      const out = byId('newTokenDisplay');\n" \
"      if (!name) return;\n" \
"      try {\n" \
"        const res = await fetch('/api/admin/tokens', {\n" \
"          method: 'POST',\n" \
"          headers: headers(true),\n" \
"          body: JSON.stringify({ name, ttl_days: ttl })\n" \
"        });\n" \
"        const data = await res.json();\n" \
"        if (res.ok && data && data.token) {\n" \
"          out.hidden = false;\n" \
"          out.textContent = 'Copy your new Scoped Creator Token now (shown only once): ' + data.token;\n" \
"          byId('tokenName').value = '';\n" \
"          byId('tokenTtl').value = '';\n" \
"          loadTokens();\n" \
"        } else {\n" \
"          out.hidden = false;\n" \
"          out.textContent = (data && data.error) ? data.error : 'Could not create token.';\n" \
"        }\n" \
"      } catch (e) {}\n" \
"    });\n" \
"  }\n" \
"\n" \
"  emptyRow(rows, 6, 'Unlock the dashboard to load links.');\n" \
"})();\n"

#define DOCS_HTML \
"<!doctype html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
"  <meta charset=\"utf-8\">\n" \
"  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
"  <link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">\n" \
"  <link rel=\"stylesheet\" href=\"/assets/styles.css\">\n" \
"  <title>API Explorer &amp; Playground - c.micutu.com</title>\n" \
"</head>\n" \
"<body>\n" \
"  <header class=\"navbar\">\n" \
"    <a class=\"brand\" href=\"/\"><img src=\"/favicon.svg\" alt=\"Logo\"><span>c.micutu.com</span><span class=\"brand-badge\">API DOCS</span></a>\n" \
"    <nav class=\"nav-links\">\n" \
"      <a class=\"btn btn-sm\" href=\"/openapi.json\" target=\"_blank\">OpenAPI 3.0 JSON</a>\n" \
"      <a class=\"btn btn-sm\" href=\"/\">&larr; Shortener</a>\n" \
"    </nav>\n" \
"  </header>\n" \
"  <main class=\"narrow\">\n" \
"    <section class=\"container\">\n" \
"      <h1>Interactive C17 API Playground</h1>\n" \
"      <p class=\"subtitle\">Explore endpoints defined in <code>/openapi.json</code> and execute live requests directly against the C17 engine.</p>\n" \
"      <div class=\"panel\">\n" \
"        <h2>Live Request Tester</h2>\n" \
"        <form id=\"playgroundForm\">\n" \
"          <div class=\"row\">\n" \
"            <div class=\"field\">\n" \
"              <label for=\"pgEndpoint\">Endpoint Preset</label>\n" \
"              <select id=\"pgEndpoint\">\n" \
"                <option value=\"GET|/health\">GET /health (Service Health)</option>\n" \
"                <option value=\"GET|/openapi.json\">GET /openapi.json (OpenAPI Spec)</option>\n" \
"                <option value=\"GET|/api/inspect/demo\">GET /api/inspect/{slug} (Link Safety &amp; Routing)</option>\n" \
"                <option value=\"GET|/api/stats/demo\">GET /api/stats/{slug} (Analytics &amp; Heatmap)</option>\n" \
"                <option value=\"GET|/api/bio/launch\">GET /api/bio/{tag} (Link-in-Bio Hub JSON)</option>\n" \
"                <option value=\"POST|/shorten\">POST /shorten (Create Short Link)</option>\n" \
"              </select>\n" \
"            </div>\n" \
"            <div class=\"field\">\n" \
"              <label for=\"pgPath\">Request Path</label>\n" \
"              <input id=\"pgPath\" type=\"text\" value=\"/health\" required>\n" \
"            </div>\n" \
"          </div>\n" \
"          <div class=\"field\" id=\"pgKeyField\" hidden>\n" \
"            <label for=\"pgKey\">X-API-Key / Scoped Creator Token (<code>csh_...</code>)</label>\n" \
"            <input id=\"pgKey\" type=\"password\" placeholder=\"Enter Admin API Key or csh_... token\">\n" \
"          </div>\n" \
"          <div class=\"field\" id=\"pgBodyField\" hidden>\n" \
"            <label for=\"pgBody\">JSON Request Body</label>\n" \
"            <textarea id=\"pgBody\" rows=\"4\">{\"url\":\"https://example.com/docs\",\"geo_country\":\"RO\",\"geo_url\":\"https://example.com/ro\"}</textarea>\n" \
"          </div>\n" \
"          <button type=\"submit\" class=\"primary\" id=\"pgSendBtn\">Send Live Request</button>\n" \
"        </form>\n" \
"        <div class=\"field\" id=\"pgResultWrap\" hidden>\n" \
"          <div class=\"list-row\"><span class=\"hint\">HTTP Status</span><strong id=\"pgStatus\">200 OK</strong></div>\n" \
"          <pre id=\"pgOutput\"></pre>\n" \
"        </div>\n" \
"      </div>\n" \
"      <div class=\"panel\">\n" \
"        <h2>Registered OpenAPI 3.0 Paths</h2>\n" \
"        <div id=\"openapiPaths\"><p class=\"hint\">Loading /openapi.json...</p></div>\n" \
"      </div>\n" \
"    </section>\n" \
"  </main>\n" \
"  <script src=\"/assets/docs.js\" defer></script>\n" \
"</body>\n" \
"</html>"

#define DOCS_JS \
"(() => {\n" \
"  'use strict';\n" \
"  const byId = (id) => document.getElementById(id);\n" \
"  const sel = byId('pgEndpoint'), pathIn = byId('pgPath');\n" \
"  const keyWrap = byId('pgKeyField'), bodyWrap = byId('pgBodyField');\n" \
"  if (sel) {\n" \
"    sel.addEventListener('change', () => {\n" \
"      const [method, path] = sel.value.split('|');\n" \
"      pathIn.value = path;\n" \
"      const isPost = method === 'POST';\n" \
"      keyWrap.hidden = !isPost;\n" \
"      bodyWrap.hidden = !isPost;\n" \
"    });\n" \
"  }\n" \
"  const form = byId('playgroundForm');\n" \
"  if (form) {\n" \
"    form.addEventListener('submit', async (ev) => {\n" \
"      ev.preventDefault();\n" \
"      const [method] = sel.value.split('|');\n" \
"      const path = pathIn.value.trim();\n" \
"      const key = byId('pgKey') ? byId('pgKey').value.trim() : '';\n" \
"      const outWrap = byId('pgResultWrap'), statusEl = byId('pgStatus'), outPre = byId('pgOutput');\n" \
"      outWrap.hidden = false;\n" \
"      statusEl.textContent = 'Sending...';\n" \
"      const opts = { method, headers: {}, cache: 'no-store' };\n" \
"      if (key) opts.headers['X-API-Key'] = key;\n" \
"      if (method === 'POST') {\n" \
"        opts.headers['Content-Type'] = 'application/json';\n" \
"        opts.body = byId('pgBody').value;\n" \
"      }\n" \
"      try {\n" \
"        const res = await fetch(path, opts);\n" \
"        statusEl.textContent = res.status + ' ' + res.statusText;\n" \
"        const raw = await res.text();\n" \
"        try {\n" \
"          outPre.textContent = JSON.stringify(JSON.parse(raw), null, 2);\n" \
"        } catch (e) {\n" \
"          outPre.textContent = raw;\n" \
"        }\n" \
"      } catch (e) {\n" \
"        statusEl.textContent = 'Network Error';\n" \
"        outPre.textContent = String(e);\n" \
"      }\n" \
"    });\n" \
"  }\n" \
"  async function loadSpec() {\n" \
"    const box = byId('openapiPaths');\n" \
"    if (!box) return;\n" \
"    try {\n" \
"      const res = await fetch('/openapi.json', { cache: 'no-store' });\n" \
"      const spec = await res.json();\n" \
"      if (!spec || !spec.paths) return;\n" \
"      box.replaceChildren();\n" \
"      for (const [p, methods] of Object.entries(spec.paths)) {\n" \
"        for (const [m, info] of Object.entries(methods)) {\n" \
"          const r = document.createElement('div');\n" \
"          r.className = 'list-row';\n" \
"          const left = document.createElement('span');\n" \
"          const badge = document.createElement('span');\n" \
"          badge.className = 'badge ' + (m === 'post' ? '' : 'ok');\n" \
"          badge.textContent = m.toUpperCase();\n" \
"          const code = document.createElement('code');\n" \
"          code.textContent = ' ' + p;\n" \
"          left.append(badge, code);\n" \
"          const right = document.createElement('span');\n" \
"          right.className = 'hint';\n" \
"          right.textContent = info.summary || '';\n" \
"          r.append(left, right);\n" \
"          box.append(r);\n" \
"        }\n" \
"      }\n" \
"    } catch (e) {}\n" \
"  }\n" \
"  loadSpec();\n" \
"})();\n"

#endif
