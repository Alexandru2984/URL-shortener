# C URL Shortener

A compact URL shortener written in C, using libmicrohttpd and SQLite. It serves
its responsive browser UI directly from the binary and is intended to run behind
an NGINX TLS reverse proxy.

## Capabilities

- Cryptographically generated short slugs and optional custom slugs
- Optional expiration and password-protected links
- PBKDF2-HMAC-SHA-256 password storage with migration of legacy plaintext rows
- Privacy-preserving visit/unique-visitor counters (HMAC identifiers, no stored user agents), protected routes and an in-memory admin UI
- SQLite WAL, transactional deletion/expiry cleanup and prepared SQL statements
- IP-based application rate limits plus NGINX edge rate-limit templates
- Health endpoint, graceful shutdown and hardened compiler/linker settings

## API

`API_KEY` or `API_KEY_FILE` is required for write/admin operations. Send it only
in the `X-API-Key` request header; never place it in a URL.

| Endpoint | Purpose |
| --- | --- |
| `POST /shorten` | Create a link from JSON: `url`, optional `custom_slug`, `password`, `ttl_hours` |
| `GET /<slug>` | Redirect to a target, or display the password form |
| `POST /unlock/<slug>` | Unlock a password-protected link with JSON `{ "password": "..." }` |
| `GET /stats/<slug>` | Show aggregate link statistics |
| `DELETE /<slug>` | Delete a link and its visit records (API key required) |
| `GET /admin` | Open the dashboard; the key remains only in page memory |
| `GET /api/admin/links` | List up to 1000 links (API key required) |
| `GET /health` | Health check |

Only `http://` and `https://` redirect targets are accepted. URL credentials,
control characters and unsafe custom-slug characters are rejected.

## Build and verification

Install development packages for libmicrohttpd, SQLite, cJSON and OpenSSL, then:

```sh
make
make test
make check
make sanitize
```

`make check` runs the C tests and cppcheck. `make sanitize` builds an AddressSanitizer
and UndefinedBehaviorSanitizer binary at `bin/shortener-asan`.

## Production configuration

The service defaults to `127.0.0.1:8080`; configure the actual listener through
`PORT` and `BIND_ADDRESS`. Use `BASE_URL` without a trailing slash. For production,
prefer `API_KEY_FILE=/etc/shortener/api_key` over an environment variable. Set
`ANALYTICS_HMAC_KEY_FILE` to a separate stable secret so unique-visitor metrics
do not retain raw visitor addresses.

Deployment templates and the ordered rollout checklist live in
[docs/OPERATIONS.md](docs/OPERATIONS.md). NGINX rate-limit/proxy snippets are in
`deploy/nginx/`, while the systemd unit and public configuration example are in
`deploy/systemd/`.

The proposed security and feature backlog is maintained in
[docs/ROADMAP.md](docs/ROADMAP.md).
