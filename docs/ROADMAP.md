# Product and Hardening Roadmap

## Completed Milestones

1. **Strict CSP & Same-Origin C Assets**: Replaced all inline `<style>` and executable `<script>` blocks with static assets served directly by the C binary (`/assets/*`), removing `'unsafe-inline'` from both application and NGINX CSP headers.
2. **SSRF & Redirect Target Hardening**: Added strict host/port parsing and IPv4/IPv6 CIDR filtering (`is_safe_redirect_url`) blocking loopback, RFC1918 private ranges, link-local/cloud metadata (`169.254.0.0/16`), non-standard IP encodings, and self-redirect loops.
3. **Scoped Creator Tokens & Audit Logging**: Added SHA-256 hashed `csh_...` creator tokens (`api_tokens`) scoped exclusively to link creation (`POST /shorten` and `POST /api/bulk/shorten`), plus an `audit_logs` table tracking administrative and authentication events.
4. **Advanced Link Controls, Smart Routing, A/B Split Testing & UTM Builder**: Added `max_clicks` (including 1-click burn links), `activates_at` scheduled links, `is_active` pause/resume toggles, device-aware `mobile_url` routing, CSPRNG-weighted A/B split testing (`alt_url`, `alt_weight`), automatic UTM parameter builder, `Idempotency-Key` support, in-place link editing (`PUT /api/admin/links/<slug>`), batch admin actions (`POST /api/admin/bulk-action`), JSON snapshot backups (`GET /api/admin/backup.json`), and formula-sanitized CSV export (`GET /api/admin/export.csv`).
5. **Native Pure-C SVG QR Generator with Themes**: Implemented ISO/IEC 18004 QR Code generation (`GET /qr/<slug>.svg`) in pure C with Reed-Solomon ECC, customizable color palettes (`?theme=cyber|dark|emerald|sunset`), and zero external requests.
6. **Social Preview Cards, Link-in-Bio Hubs & Bulk Shortening**: Added `/p/<slug>` OpenGraph social cards, `/bio/<tag>` public link-in-bio micro-pages, and `POST /api/bulk/shorten` batch creation.
7. **Rich Privacy-Preserving Analytics, 24h Heatmap, Link Inspector & Prometheus Telemetry**: Added 14-day daily click time-series, 24-hour UTC traffic distribution, in-memory device classification, stripped referrer host metrics, `/inspect/<slug>` link previews, `/metrics` Prometheus exporter, and `/api/admin/system` diagnostics.
8. **Native C17 CLI Client**: Added `bin/shortener-cli` (`tools/shortener_cli.c`) communicating over POSIX sockets for terminal workflows.
9. **SEO & Web Standards**: Added dynamic `/sitemap.xml`, `/robots.txt`, `/manifest.json`, `/favicon.svg`, `HEAD` request support, OpenGraph/Twitter tags, and JSON-LD structured data.
