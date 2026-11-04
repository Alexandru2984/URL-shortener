# C URL Shortener (`c.micutu.com`)

A high-performance, security-hardened URL shortener, native SVG QR code generator, and privacy-first link analytics platform written in pure C17 (`libmicrohttpd`, `SQLite`, `cJSON`, and `OpenSSL`). It serves its responsive browser UI and static assets directly from the compiled binary and is designed to run behind an NGINX TLS reverse proxy and Cloudflare edge.

## Capabilities

- **Cryptographic Slugs & Custom Aliases**: Unbiased CSPRNG (`RAND_bytes`) short slugs or normalized custom slugs with reserved-route protection.
- **Advanced Link Controls**:
  - Optional expiration (`ttl_hours`), scheduled activation (`activates_in_hours`), and maximum click limits (`max_clicks`, including 1-click self-destruct links).
  - Optional title/notes, comma-separated tags, and instant enable/disable toggling from the Admin Dashboard.
  - PBKDF2-HMAC-SHA-256 (600,000 iterations) password protection with automatic legacy hash migration and `OPENSSL_cleanse` memory scrubbing.
- **Scoped Creator Tokens & Security Audit Log**:
  - Issue scoped `csh_...` creator tokens (`SHA-256` hashed at rest) that can only call `POST /shorten` without exposing the master Admin API key.
  - Built-in security audit log tracking link creations, updates, deletions, token lifecycle events, and failed authentication/unlock attempts.
- **Native Pure-C SVG QR Code Generator**:
  - Standards-compliant ISO/IEC 18004 QR Code encoder (Versions 1–6, Reed-Solomon GF(256) ECC, automatic penalty mask selection) implemented in C (`src/qr.c`) with zero third-party network calls.
- **Link Inspector & Rich Privacy-Preserving Analytics**:
  - Safe link preview at `/inspect/<slug>` (or `/<slug>+`) showing status, metadata, and destination (automatically redacted for password-protected links).
  - Aggregate visitor analytics (`HMAC-SHA-256` visitor identifiers, in-memory User-Agent device classification, and stripped referrer hostnames—never storing raw IPs or User-Agents) with 14-day daily activity, device breakdown, top referrers, and configurable retention (`ANALYTICS_RETENTION_DAYS`).
- **Strict Content Security Policy (Zero `'unsafe-inline'`)**:
  - Same-origin static CSS/JS assets served directly from C memory (`/assets/*`), enabling `script-src 'self'; style-src 'self'` without `'unsafe-inline'`.
  - Hardened SSRF/redirect target validation blocking loopback, RFC1918 private networks, link-local/cloud metadata IPs, and self-redirect loops.

## HTTP & JSON API

Send the master Admin API key or a Scoped Creator Token (`csh_...`) in the `X-API-Key` header (never in a URL).

| Endpoint | Auth | Purpose |
| --- | --- | --- |
| `POST /shorten` | Admin Key or Creator Token | Create a short link (`url`, optional `custom_slug`, `password`, `ttl_hours`, `max_clicks`, `activates_in_hours`, `title`, `tags`) |
| `GET /<slug>` | Public | Redirect to target (`302`), or render password prompt / expiration status |
| `GET /<slug>+` | Public | Shortcut to open the Link Inspector page for `<slug>` |
| `POST /unlock/<slug>` | Public | Unlock a password-protected link with JSON `{ "password": "..." }` |
| `GET /qr/<slug>.svg` | Public | Generate a crisp SVG QR code for the short link in pure C |
| `GET /inspect/<slug>` | Public | Interactive Link Inspector page |
| `GET /api/inspect/<slug>` | Public | JSON link safety & metadata inspection |
| `GET /stats/<slug>` | Public | Interactive link analytics dashboard |
| `GET /api/stats/<slug>` | Public | JSON analytics (14-day series, devices, referrers, remaining clicks) |
| `GET /admin` | Public HTML (In-memory key) | Responsive Admin Dashboard (Links, Creator Tokens, Security Audit Log) |
| `GET /api/admin/links` | Admin Key | List/filter links (`?q=`, `?tag=`, `?status=active\|expired\|disabled`) |
| `PUT /api/admin/links/<slug>` | Admin Key | Edit link URL, title, tags, TTL, max clicks, active status, or password |
| `DELETE /<slug>` | Admin Key | Delete a link and its visit records |
| `GET /api/admin/export.csv` | Admin Key | Export links & metrics as formula-sanitized RFC 4180 CSV |
| `GET /api/admin/tokens` | Admin Key | List scoped creator tokens |
| `POST /api/admin/tokens` | Admin Key | Create a scoped creator token (`name`, optional `ttl_days`) |
| `DELETE /api/admin/tokens/<id>` | Admin Key | Revoke a scoped creator token |
| `GET /api/admin/audit` | Admin Key | Retrieve recent security audit events |
| `POST /api/admin/cleanup` | Admin Key | Immediately purge expired links |
| `GET /health` | Public | JSON health check (`{"status":"ok"}`) |
| `GET /robots.txt`, `/sitemap.xml`, `/manifest.json` | Public | SEO & Web App Manifest endpoints |

## Build and Verification

Install development packages for `libmicrohttpd`, `SQLite3`, `cJSON`, `OpenSSL`, and `cppcheck`, then run:

```sh
make
make test
make check
make sanitize
```

`make check` runs all unit, database, QR, and end-to-end HTTP integration tests plus exhaustive `cppcheck` static analysis. `make sanitize` builds an AddressSanitizer + UndefinedBehaviorSanitizer binary at `bin/shortener-asan` and runs the test suite under sanitizers.

## Production Configuration

The service defaults to `127.0.0.1:8080`; configure the listener through `PORT` and `BIND_ADDRESS`. Use `BASE_URL` without a trailing slash. In production, configure `API_KEY_FILE=/etc/shortener/api_key`, `ANALYTICS_HMAC_KEY_FILE=/etc/shortener/analytics_hmac_key`, and optional `ANALYTICS_RETENTION_DAYS=90`.

Deployment templates and the rollout checklist are documented in [docs/OPERATIONS.md](docs/OPERATIONS.md).
