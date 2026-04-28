# C URL Shortener

A high-performance, lightweight, production-ready URL shortener built in C.

## Features
- **Fast HTTP Server**: Powered by `libmicrohttpd`.
- **Persistent Storage**: Backed by `SQLite`.
- **Custom Slugs**: Supports both auto-generated (base62) and custom short links.
- **Analytics**: Tracks total visits and unique IP visitors per link.
- **Rate Limiting**: Protects endpoints from IP-based spam/abuse.
- **Secure**: Checks payload size, validates inputs, and uses parameterized DB queries.
- **Dynamic Port Selection**: Automatically binds to the next available port starting from 8080 if occupied.
- **HTML Frontend**: Includes a minimal, responsive web UI for creating links right from the browser.

## Deployment Environment
- **Domain**: c.micutu.com
- **OS**: Ubuntu Linux (headless VPS)
- **Reverse Proxy**: NGINX with Let's Encrypt SSL/TLS.
- **Daemon**: Managed by `systemd`.

## API Endpoints
- **`POST /shorten`**
  - **Body**: `{"url": "https://example.com", "custom_slug": "optional"}`
  - **Response**: `{"short_url": "https://c.micutu.com/..."}`

- **`GET /<slug>`**
  - Redirects to the previously shortened target URL. Returns a `302 Found`.

- **`GET /stats/<slug>`**
  - **Response**: `{"total_visits": 5, "unique_visitors": 2}`

- **`GET /`**
  - Interactive HTML interface for generating short links.

## Building and Running
1. `make` to compile the `bin/shortener` executable.
2. `make clean` to clean up object and binary files.
3. Start the service with `sudo systemctl start shortener`.
4. Ensure NGINX configuration inside `/etc/nginx/sites-enabled/shortener` is valid and points to the internally assigned port.

## Author
Senior Systems Engineer / DevOps
