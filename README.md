# C URL Shortener (`c.micutu.com`)

A high-performance, security-hardened URL shortener, native SVG QR code generator, smart device router, link-in-bio hub generator, and privacy-first link analytics platform written in pure C17 (`libmicrohttpd`, `SQLite`, `cJSON`, and `OpenSSL`). It serves its responsive browser UI and static assets directly from the compiled binary and includes a standalone pure-C POSIX socket CLI client (`bin/shortener-cli`).

## Capabilities

- **Cryptographic Slugs, Idempotency & Custom Aliases**:
  - Unbiased CSPRNG (`RAND_bytes`) short slugs or normalized custom slugs with reserved-route protection.
  - Safe API retry support via `Idempotency-Key` request headers (`X-Idempotency-Key` / `Idempotency-Key`).
- **Smart Device Routing, UTM Campaign Builder & Bulk Creation**:
  - **Mobile Deep-Links (`mobile_url`)**: Automatically routes mobile User-Agents to an optional mobile/app destination while desktop visitors receive the primary URL.
  - **Native UTM Builder**: Automatically URL-encodes and appends `utm_source`, `utm_medium`, and `utm_campaign` parameters before storing the link, preserving fragment identifiers (`#...`).
  - **Batch Shortening (`POST /api/bulk/shorten`)**: Shorten up to 25 links in a single request via UI or API.
- **Social OpenGraph Cards & Link-in-Bio Micro-Hubs**:
  - **Social Card Preview (`GET /p/<slug>`)**: Renders rich OpenGraph/Twitter Card metadata with embedded SVG QR code and safe destination preview for social sharing.
  - **Link-in-Bio Collections (`GET /bio/<tag>` & `GET /api/bio/<tag>`)**: Any tag automatically generates a responsive Link-in-Bio landing page listing all active, public (non-password) links carrying that tag.
- **Advanced Link Controls**:
  - Optional expiration (`ttl_hours`), scheduled activation (`activates_in_hours`), and maximum click limits (`max_clicks`, including 1-click self-destruct links).
  - Optional title/notes, comma-separated tags, and instant enable/disable toggling from the Admin Dashboard.
  - PBKDF2-HMAC-SHA-256 (600,000 iterations) password protection with automatic legacy hash migration and `OPENSSL_cleanse` memory scrubbing.
- **Scoped Creator Tokens, Security Audit Log & Prometheus Telemetry**:
  - Issue scoped `csh_...` creator tokens (`SHA-256` hashed at rest) that can only call `POST /shorten` and `POST /api/bulk/shorten` without exposing the master Admin API key.
  - Built-in security audit log tracking link creations, updates, deletions, token lifecycle events, and failed authentication/unlock attempts.
  - Authenticated Prometheus exporter (`GET /metrics`) and JSON runtime diagnostics (`GET /api/admin/system`).
- **Native Pure-C SVG QR Code Generator**:
  - Standards-compliant ISO/IEC 18004 QR Code encoder (Versions 1–6, Reed-Solomon GF(256) ECC, automatic penalty mask selection) implemented in C (`src/qr.c`) with zero third-party network calls.
- **Strict Content Security Policy (Zero `'unsafe-inline'`)**:
  - Same-origin static CSS/JS assets served directly from C memory (`/assets/*`), enabling `script-src 'self'; style-src 'self'` without `'unsafe-inline'`.
  - Hardened SSRF/redirect target validation blocking loopback, RFC1918 private networks, link-local/cloud metadata IPs, and self-redirect loops.

## HTTP & JSON API

Send the master Admin API key or a Scoped Creator Token (`csh_...`) in the `X-API-Key` header (never in a URL).

| Endpoint | Auth | Purpose |
| --- | --- | --- |
| `POST /shorten` | Admin Key or Creator Token | Create a short link (`url`, optional `mobile_url`, `custom_slug`, `password`, `ttl_hours`, `max_clicks`, `activates_in_hours`, `title`, `tags`, `utm_source`, `utm_medium`, `utm_campaign`, `Idempotency-Key` header) |
| `POST /api/bulk/shorten` | Admin Key or Creator Token | Bulk shorten up to 25 links in one call (`{"links":[...]}`) |
| `GET /<slug>` | Public | Redirect to target (`302`, device-aware if `mobile_url` is set), or render password prompt / expiration status |
| `GET /<slug>+` | Public | Shortcut to open the Link Inspector page for `<slug>` |
| `GET /p/<slug>` | Public | Social Preview Card page with OpenGraph/Twitter tags and embedded QR code |
| `GET /bio/<tag>` | Public | Responsive Link-in-Bio micro-hub listing active public links for `<tag>` |
| `GET /api/bio/<tag>` | Public | JSON Link-in-Bio collection for `<tag>` |
| `POST /unlock/<slug>` | Public | Unlock a password-protected link with JSON `{ "password": "..." }` |
| `GET /qr/<slug>.svg` | Public | Generate a crisp SVG QR code for the short link in pure C |
| `GET /inspect/<slug>` | Public | Interactive Link Inspector page |
| `GET /api/inspect/<slug>` | Public | JSON link safety & metadata inspection |
| `GET /stats/<slug>` | Public | Interactive link analytics dashboard |
| `GET /api/stats/<slug>` | Public | JSON analytics (14-day series, devices, referrers, remaining clicks) |
| `GET /admin` | Public HTML (In-memory key) | Responsive Admin Dashboard (Links, Creator Tokens, Audit Log, System & Metrics) |
| `GET /api/admin/links` | Admin Key | List/filter links (`?q=`, `?tag=`, `?status=active\|expired\|disabled`) |
| `PUT /api/admin/links/<slug>` | Admin Key | Edit link URL, mobile URL, title, tags, TTL, max clicks, active status, or password |
| `DELETE /<slug>` | Admin Key | Delete a link and its visit records |
| `GET /api/admin/export.csv` | Admin Key | Export links & metrics as formula-sanitized RFC 4180 CSV |
| `GET /api/admin/tokens` | Admin Key | List scoped creator tokens |
| `POST /api/admin/tokens` | Admin Key | Create a scoped creator token (`name`, optional `ttl_days`) |
| `DELETE /api/admin/tokens/<id>` | Admin Key | Revoke a scoped creator token |
| `GET /api/admin/audit` | Admin Key | Retrieve recent security audit events |
| `GET /api/admin/system` | Admin Key | C17 runtime, SQLite database size, uptime, and link/visit counters JSON |
| `GET /metrics` | Admin Key | Prometheus text-format metrics exporter |
| `POST /api/admin/cleanup` | Admin Key | Immediately purge expired links |
| `GET /health` | Public | JSON health check (`{"status":"ok"}`) |
| `GET /robots.txt`, `/sitemap.xml`, `/manifest.json` | Public | SEO & Web App Manifest endpoints |

## Pure-C CLI Client (`bin/shortener-cli`)

`make` also compiles `bin/shortener-cli` (`tools/shortener_cli.c`), a zero-bloat C17 terminal client using POSIX sockets:

```sh
export SHORTENER_HOST=127.0.0.1
export SHORTENER_PORT=8086
export SHORTENER_API_KEY="csh_your_scoped_token"

./bin/shortener-cli health
./bin/shortener-cli shorten https://example.com --slug demo --title "Demo" --tag launch --mobile https://m.example.com
./bin/shortener-cli inspect demo
./bin/shortener-cli stats demo
./bin/shortener-cli qr demo demo-qr.svg
```

## Build and Verification

Install development packages for `libmicrohttpd`, `SQLite3`, `cJSON`, `OpenSSL`, and `cppcheck`, then run:

```sh
make
make test
make check
make sanitize
```

`make check` runs all unit, database, QR, and end-to-end HTTP integration tests plus exhaustive `cppcheck` static analysis on both `src/` and `tools/`. `make sanitize` builds an AddressSanitizer + UndefinedBehaviorSanitizer binary at `bin/shortener-asan`.

## Production Configuration

The service defaults to `127.0.0.1:8080`; configure the listener through `PORT` and `BIND_ADDRESS`. Use `BASE_URL` without a trailing slash. In production, configure `API_KEY_FILE=/etc/shortener/api_key`, `ANALYTICS_HMAC_KEY_FILE=/etc/shortener/analytics_hmac_key`, and optional `ANALYTICS_RETENTION_DAYS=90`.

Deployment templates and the rollout checklist are documented in [docs/OPERATIONS.md](docs/OPERATIONS.md).
