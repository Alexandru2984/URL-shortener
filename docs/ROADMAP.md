# Product and Hardening Roadmap

## Completed Milestones

1. **Strict CSP & Same-Origin C Assets**: Replaced all inline `<style>` and executable `<script>` blocks with static assets served directly by the C binary (`/assets/*`), removing `'unsafe-inline'` from both application and NGINX CSP headers.
2. **SSRF & Redirect Target Hardening**: Added strict host/port parsing and IPv4/IPv6 CIDR filtering (`is_safe_redirect_url`) blocking loopback, RFC1918 private ranges, link-local/cloud metadata (`169.254.0.0/16`), non-standard IP encodings, and self-redirect loops.
3. **Scoped Creator Tokens & Audit Logging**: Added SHA-256 hashed `csh_...` creator tokens (`api_tokens`) scoped exclusively to `POST /shorten`, plus an `audit_logs` table tracking administrative and authentication events.
4. **Advanced Link Controls**: Added `max_clicks` (including 1-click burn links), `activates_at` scheduled links, `is_active` pause/resume toggles, link metadata (`title`, `tags`), in-place link editing (`PUT /api/admin/links/<slug>`), and formula-sanitized CSV export (`GET /api/admin/export.csv`).
5. **Native Pure-C SVG QR Generator**: Implemented ISO/IEC 18004 QR Code generation (`GET /qr/<slug>.svg`) in pure C with Reed-Solomon ECC and zero external requests.
6. **Rich Privacy-Preserving Analytics & Link Inspector**: Added 14-day daily click time-series, in-memory device classification, stripped referrer host metrics, configurable analytics retention (`ANALYTICS_RETENTION_DAYS`), and `/inspect/<slug>` link previews.
7. **SEO & Web Standards**: Added dynamic `/sitemap.xml`, `/robots.txt`, `/manifest.json`, `/favicon.svg`, `HEAD` request support, OpenGraph/Twitter tags, and JSON-LD structured data.

## Future Enhancements

1. Optional webhook notifications with HMAC-SHA-256 signed payloads when links reach their `max_clicks` limit or expire.
2. Optional Prometheus metrics endpoint (`/metrics`) protected by bearer token for infrastructure monitoring.
3. Bulk JSON/CSV link import endpoint for batch campaign creation.
